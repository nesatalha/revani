package com.nesatalha.revani.ble

import com.nesatalha.revani.nav.Maneuver
import com.nesatalha.revani.nav.NavInfo
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class ProtocolTest {
    private fun hex(s: String) = s.split(' ').filter { it.isNotEmpty() }.map { it.toInt(16).toByte() }.toByteArray()

    // Aynı test vektörü firmware/simulator/protocol_test.c içinde de çözülüyor.
    @Test
    fun navMatchesSharedVector() {
        val msg = Protocol.nav(NavInfo(Maneuver.RIGHT, 0, distanceM = 180, etaMin = 12, street = "Bağdat Cd."))
        assertArrayEquals(hex("02 02 00 b4 00 0c 00 0b 42 61 c4 9f 64 61 74 20 43 64 2e"), msg)
    }

    @Test
    fun navRoundaboutAndClamping() {
        val msg = Protocol.nav(NavInfo(Maneuver.ROUNDABOUT, 3, distanceM = 70_000, etaMin = -5, street = ""))
        assertArrayEquals(hex("02 08 03 ff ff 00 00 00"), msg)
    }

    @Test
    fun navEnd() = assertArrayEquals(hex("08"), Protocol.navEnd())

    @Test
    fun gps() {
        assertArrayEquals(hex("09 2f 0e 01"), Protocol.gps(47, 270))
        assertArrayEquals(hex("09 ff 5a 00"), Protocol.gps(300, -270)) // hız kırpılır, yön 0–359
    }

    @Test
    fun time() {
        // 2026-10-20 12:00:00 UTC, UTC+3
        assertArrayEquals(hex("07 c0 57 d7 6a b4 00"), Protocol.time(1_792_497_600L, 180))
        assertArrayEquals(hex("07 00 00 00 00 c4 ff"), Protocol.time(0, -60))
    }

    @Test
    fun streetIsTruncatedOnCharBoundary() {
        val street = "ğ".repeat(60) // 120 bayt
        val msg = Protocol.nav(NavInfo(Maneuver.STRAIGHT, distanceM = 10, street = street))
        val len = msg[7].toInt() and 0xFF
        assertEquals(Protocol.MAX_STREET_BYTES, len)
        assertEquals(8 + len, msg.size)
        assertEquals("ğ".repeat(48), String(msg, 8, len, Charsets.UTF_8))

        val odd = Protocol.truncateUtf8("a" + "ğ".repeat(60), Protocol.MAX_STREET_BYTES)
        assertEquals(95, odd.size) // 96. bayt bir 'ğ'nin ortası, öncesinde kesilir
    }

    @Test
    fun parseIncoming() {
        assertEquals(Protocol.Incoming.Status(85, 1), Protocol.parseIncoming(hex("82 55 01")))
        assertEquals(Protocol.Incoming.Status(null, 0), Protocol.parseIncoming(hex("82 ff 00")))
        assertEquals(Protocol.Incoming.Button(2, true), Protocol.parseIncoming(hex("81 02 01")))
        assertNull(Protocol.parseIncoming(hex("82 55")))
        assertNull(Protocol.parseIncoming(hex("42 00 00")))
        assertNull(Protocol.parseIncoming(ByteArray(0)))
    }
}
