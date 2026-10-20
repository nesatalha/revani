package com.nesatalha.revani.map

import android.content.Context
import android.graphics.Bitmap
import android.util.Log
import com.nesatalha.revani.BuildConfig
import org.maplibre.android.MapLibre
import org.maplibre.android.camera.CameraPosition
import org.maplibre.android.geometry.LatLng
import org.maplibre.android.maps.Style
import org.maplibre.android.snapshotter.MapSnapshotter
import java.io.ByteArrayOutputStream

/**
 * Ekranın ortası için 240×240 harita karesi üretir (2.1.4): yön yukarıda (heading-up),
 * konum merkezin biraz altında (önümüzdeki yol daha çok görünsün). Ana iş parçacığından çağrılmalı.
 *
 * Stil: MapTiler anahtarı varsa koyu sade stil, yoksa MapLibre demo stili
 * (sadece ülke sınırları; akışı denemek için).
 */
class MapRenderer(context: Context) {

    private val snapshotter: MapSnapshotter
    private var busy = false

    val usingDemoStyle = BuildConfig.MAPTILER_API_KEY.isBlank()

    init {
        MapLibre.getInstance(context.applicationContext)
        val options = MapSnapshotter.Options(MapMath.SIZE_PX, MapMath.SIZE_PX)
            .withStyleBuilder(Style.Builder().fromUri(styleUrl()))
            .withPixelRatio(1f)
            .withLogo(false)
        snapshotter = MapSnapshotter(context.applicationContext, options)
    }

    /** Kare üretir; önceki kare bitmediyse false döner (kare atlanır). */
    fun render(lat: Double, lon: Double, bearingDeg: Double, speedKmh: Int, onJpeg: (ByteArray) -> Unit): Boolean {
        if (busy) return false
        busy = true
        val zoom = MapMath.zoomFor(speedKmh)
        val (camLat, camLon) = MapMath.cameraTarget(lat, lon, bearingDeg, zoom)
        snapshotter.setCameraPosition(
            CameraPosition.Builder()
                .target(LatLng(camLat, camLon))
                .zoom(zoom)
                .bearing(bearingDeg)
                .tilt(0.0)
                .build(),
        )
        snapshotter.start(
            { snapshot ->
                busy = false
                onJpeg(encode(snapshot.bitmap))
            },
            { error ->
                busy = false
                Log.w(TAG, "Harita karesi üretilemedi: $error")
            },
        )
        return true
    }

    fun close() {
        snapshotter.cancel()
    }

    private fun encode(bitmap: Bitmap): ByteArray =
        ByteArrayOutputStream(16 * 1024).also {
            bitmap.compress(Bitmap.CompressFormat.JPEG, MapMath.JPEG_QUALITY, it)
        }.toByteArray()

    companion object {
        private const val TAG = "MapRenderer"
        private const val DEMO_STYLE = "https://demotiles.maplibre.org/style.json"

        // 2.1.3: koyu ve sade, yol adları az; küçük ekranda okunur
        private const val MAPTILER_STYLE = "https://api.maptiler.com/maps/streets-v2-dark/style.json?key="

        private fun styleUrl(): String {
            val key = BuildConfig.MAPTILER_API_KEY
            return if (key.isBlank()) DEMO_STYLE else MAPTILER_STYLE + key
        }
    }
}
