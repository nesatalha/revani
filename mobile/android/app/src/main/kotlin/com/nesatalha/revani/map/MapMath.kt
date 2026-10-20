package com.nesatalha.revani.map

import com.nesatalha.revani.ble.Protocol

/** Harita karesi için saf hesaplar (birim testli). */
object MapMath {
    const val SIZE_PX = 240
    const val JPEG_QUALITY = 60

    /** Hıza göre zoom (2.1.5): 30 km/h ve altı 17, 80 km/h ve üstü 15, arası doğrusal. */
    fun zoomFor(speedKmh: Int): Double {
        val t = ((speedKmh - 30) / 50.0).coerceIn(0.0, 1.0)
        return 17.0 - 2.0 * t
    }

    /** Kare aralığı (2.1.8): normalde 1/sn, dönüşe 100 m kala ~2.5/sn. */
    fun frameIntervalMs(distanceToTurnM: Int?): Long =
        if (distanceToTurnM != null && distanceToTurnM <= 100) 400L else 1_000L

    /** JPEG'i MAP_CHUNK mesajlarına böler (2.1.7). */
    fun chunks(frame: Int, jpeg: ByteArray): List<ByteArray> {
        val n = (jpeg.size + Protocol.MAP_CHUNK_DATA - 1) / Protocol.MAP_CHUNK_DATA
        return (0 until n).map { i ->
            val from = i * Protocol.MAP_CHUNK_DATA
            Protocol.mapChunk(frame, i, n, jpeg.copyOfRange(from, minOf(jpeg.size, from + Protocol.MAP_CHUNK_DATA)))
        }
    }
}
