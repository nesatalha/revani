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
) {
    private val manager = context.getSystemService(LocationManager::class.java)
    private var lastHeading = 0

    private val listener = LocationListener { loc: Location ->
        val speed = if (loc.hasSpeed()) (loc.speed * 3.6f).roundToInt() else 0
        // Durunca GPS yönü anlamsızlaşıyor, son yön korunur.
        if (loc.hasBearing() && speed >= MIN_SPEED_FOR_HEADING) lastHeading = loc.bearing.roundToInt() % 360
        onFix(speed, lastHeading)
    }

    @SuppressLint("MissingPermission") // izin servis başlamadan kontrol ediliyor
    fun start(): Boolean {
        val m = manager ?: return false
        if (!m.isProviderEnabled(LocationManager.GPS_PROVIDER)) return false
        m.requestLocationUpdates(LocationManager.GPS_PROVIDER, INTERVAL_MS, 0f, listener, Looper.getMainLooper())
        return true
    }

    fun stop() {
        manager?.removeUpdates(listener)
    }

    companion object {
        private const val INTERVAL_MS = 1_000L
        private const val MIN_SPEED_FOR_HEADING = 5
    }
}
