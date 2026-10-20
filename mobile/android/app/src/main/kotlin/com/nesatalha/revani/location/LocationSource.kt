package com.nesatalha.revani.location

import android.annotation.SuppressLint
import android.content.Context
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.os.Looper
import kotlin.math.roundToInt

/**
 * GPS'ten hız (km/h) ve yön (derece), ~1 Hz. Play Services gerektirmesin diye
 * doğrudan LocationManager kullanılıyor.
 */
class LocationSource(
    context: Context,
    private val onFix: (speedKmh: Int, headingDeg: Int) -> Unit,
    /** Harita merkezi için konum; GPS yokken ağ konumundan da gelir. */
    private val onPosition: (lat: Double, lon: Double) -> Unit = { _, _ -> },
) {
    private val manager = context.getSystemService(LocationManager::class.java)
    private var lastHeading = 0

    private var prev: Location? = null

    private val listener = LocationListener { loc: Location ->
        val speed = (speedMps(loc) * 3.6f).roundToInt()
        prev = loc
        // Durunca GPS yönü anlamsızlaşıyor, son yön korunur.
        if (loc.hasBearing() && speed >= MIN_SPEED_FOR_HEADING) lastHeading = loc.bearing.roundToInt() % 360
        onFix(speed, lastHeading)
        lastGpsAt = System.currentTimeMillis()
        onPosition(loc.latitude, loc.longitude)
    }

    private var lastGpsAt = 0L

    // İç mekânda GPS yokken haritanın yine de gelmesi için kaba konum (hız vermez).
    private val networkListener = LocationListener { loc: Location ->
        if (System.currentTimeMillis() - lastGpsAt > NETWORK_FALLBACK_AFTER_MS) onPosition(loc.latitude, loc.longitude)
    }

    /**
     * GPS hızı; bazı kaynaklar (emülatörün rota oynatması, bazı cihazlar) hız vermez,
     * o zaman son iki konumdan hesaplanır.
     */
    private fun speedMps(loc: Location): Float {
        if (loc.hasSpeed()) return loc.speed
        val p = prev ?: return 0f
        val dt = (loc.elapsedRealtimeNanos - p.elapsedRealtimeNanos) / 1e9f
        if (dt <= 0.2f || dt > 5f) return 0f
        val mps = p.distanceTo(loc) / dt
        // Konum sıçraması (rota oynatmanın başa sarması, kötü fix) hız sanılmasın.
        return if (mps * 3.6f > MAX_PLAUSIBLE_KMH) 0f else mps
    }

    @SuppressLint("MissingPermission") // izin servis başlamadan kontrol ediliyor
    fun start(): Boolean {
        val m = manager ?: return false
        if (!m.isProviderEnabled(LocationManager.GPS_PROVIDER)) return false
        m.requestLocationUpdates(LocationManager.GPS_PROVIDER, INTERVAL_MS, 0f, listener, Looper.getMainLooper())
        if (m.isProviderEnabled(LocationManager.NETWORK_PROVIDER)) {
            m.requestLocationUpdates(
                LocationManager.NETWORK_PROVIDER, NETWORK_INTERVAL_MS, 0f, networkListener, Looper.getMainLooper(),
            )
            m.getLastKnownLocation(LocationManager.NETWORK_PROVIDER)?.let { onPosition(it.latitude, it.longitude) }
        }
        return true
    }

    fun stop() {
        manager?.removeUpdates(listener)
        manager?.removeUpdates(networkListener)
    }

    companion object {
        private const val INTERVAL_MS = 1_000L
        private const val MIN_SPEED_FOR_HEADING = 5
        private const val MAX_PLAUSIBLE_KMH = 160f
        private const val NETWORK_INTERVAL_MS = 5_000L
        private const val NETWORK_FALLBACK_AFTER_MS = 5_000L
    }
}
