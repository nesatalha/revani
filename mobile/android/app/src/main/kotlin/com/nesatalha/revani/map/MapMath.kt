package com.nesatalha.revani.map

import com.nesatalha.revani.ble.Protocol
import kotlin.math.cos
import kotlin.math.pow
import kotlin.math.sin

/** Harita karesi için saf hesaplar (birim testli). */
object MapMath {
    const val SIZE_PX = 240
    const val JPEG_QUALITY = 60

    /**
     * Konum karenin ortasında değil bu oranda (kare yüksekliğinin) altında: önümüzdeki yol
     * daha çok görünür. Firmware'de UI_MAP_MARKER_OFFSET_PCT ile aynı olmalı.
     */
    const val MARKER_OFFSET_FRAC = 0.08

    /** Hıza göre zoom (2.1.5): 30 km/h ve altı 16, 80 km/h ve üstü 14.5, arası doğrusal. */
    fun zoomFor(speedKmh: Int): Double {
        val t = ((speedKmh - 30) / 50.0).coerceIn(0.0, 1.0)
        return 16.0 - 1.5 * t
    }

    /** MapLibre (512 px karo) piksel başına metre. */
    fun metersPerPixel(zoom: Double, latDeg: Double): Double =
        78271.517 * cos(Math.toRadians(latDeg)) / 2.0.pow(zoom)

    /**
     * Kamera merkezi: konumun gidiş yönünde ileri kaydırılmış hali. Kare yön yukarıda
     * döndüğü için konum karede merkezin [MARKER_OFFSET_FRAC] kadar altına düşer.
     */
    fun cameraTarget(lat: Double, lon: Double, bearingDeg: Double, zoom: Double): Pair<Double, Double> {
        val meters = MARKER_OFFSET_FRAC * SIZE_PX * metersPerPixel(zoom, lat)
        return offset(lat, lon, bearingDeg, meters)
    }

    /** Küçük mesafeler için düz dünya yaklaşımıyla [meters] ileri nokta. */
    fun offset(lat: Double, lon: Double, bearingDeg: Double, meters: Double): Pair<Double, Double> {
        val b = Math.toRadians(bearingDeg)
        val dLat = meters * cos(b) / 111_320.0
        val dLon = meters * sin(b) / (111_320.0 * cos(Math.toRadians(lat)))
        return (lat + dLat) to (lon + dLon)
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
