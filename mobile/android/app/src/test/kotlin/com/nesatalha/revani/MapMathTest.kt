package com.nesatalha.revani

import com.nesatalha.revani.ble.Protocol
import com.nesatalha.revani.link.MessageQueue
import com.nesatalha.revani.map.MapMath
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class MapMathTest {
    @Test
    fun zoomBySpeed() {
        assertEquals(17.0, MapMath.zoomFor(0), 0.0)
        assertEquals(17.0, MapMath.zoomFor(30), 0.0)
        assertEquals(16.0, MapMath.zoomFor(55), 0.001)
        assertEquals(15.0, MapMath.zoomFor(120), 0.0)
    }

    @Test
    fun frameRateNearTurn() {
        assertEquals(1000L, MapMath.frameIntervalMs(null))
        assertEquals(1000L, MapMath.frameIntervalMs(350))
        assertEquals(400L, MapMath.frameIntervalMs(80))
    }

    @Test
    fun chunking() {
        val jpeg = ByteArray(1000) { it.toByte() }
        val parts = MapMath.chunks(258, jpeg)
        assertEquals(3, parts.size)
        // 03, kare 258, parça 1/3, ardından 480 bayt
        assertArrayEquals(byteArrayOf(0x03, 0x02, 0x01, 0x01, 0x00, 0x03, 0x00), parts[1].copyOf(7))
        assertEquals(7 + 480, parts[0].size)
        assertEquals(7 + 40, parts[2].size)
        assertArrayEquals(jpeg, parts.flatMap { it.drop(7) }.toByteArray())
    }

    @Test
    fun queueSendsNavBeforeMapAndDropsOldFrame() {
        val q = MessageQueue()
        MapMath.chunks(1, ByteArray(1000)).forEach(q::put)
        q.put(Protocol.gps(40, 0))
        MapMath.chunks(2, ByteArray(500)).forEach(q::put) // yeni kare eskiyi siler
        assertEquals(Protocol.GPS, q.poll()!![0].toInt())
        val a = q.poll()!!
        val b = q.poll()!!
        assertEquals(2, a[1].toInt()) // kare 2, parça 0
        assertEquals(1, b[3].toInt()) // kare 2, parça 1
        assertNull(q.poll())
    }
}
