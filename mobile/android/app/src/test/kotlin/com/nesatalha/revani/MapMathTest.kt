package com.nesatalha.revani

import com.nesatalha.revani.ble.Protocol
import com.nesatalha.revani.link.MessageQueue
import com.nesatalha.revani.map.MapMath
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class MapMathTest {
    @Test
    fun zoomBySpeed() {
        assertEquals(16.0, MapMath.zoomFor(0), 0.0)
        assertEquals(16.0, MapMath.zoomFor(30), 0.0)
        assertEquals(15.25, MapMath.zoomFor(55), 0.001)
        assertEquals(14.5, MapMath.zoomFor(120), 0.0)
    }

    @Test
    fun cameraIsAheadOfPosition() {
        val lat = 41.0
        val lon = 29.0
        val zoom = 16.0
        val ahead = MapMath.MARKER_OFFSET_FRAC * MapMath.SIZE_PX * MapMath.metersPerPixel(zoom, lat)
        assertEquals(32.4, ahead, 0.5) // 36 px × 0,9 m/px (zoom 16, enlem 41)
        val north = MapMath.cameraTarget(lat, lon, 0.0, zoom)
        assertEquals(lat + ahead / 111_320.0, north.first, 1e-7)
        assertEquals(lon, north.second, 1e-9)
        val east = MapMath.cameraTarget(lat, lon, 90.0, zoom)
        assertEquals(lat, east.first, 1e-9)
        assertTrue(east.second > lon)
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
