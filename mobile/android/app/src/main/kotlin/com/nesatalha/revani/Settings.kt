package com.nesatalha.revani

import android.content.Context

/** Kalıcı ayarlar. Flutter `setSettings` ile değiştirir. */
class Settings(context: Context) {
    private val prefs = context.getSharedPreferences("revani", Context.MODE_PRIVATE)

    var lastDeviceAddress: String?
        get() = prefs.getString(KEY_LAST_DEVICE, null)
        set(v) = prefs.edit().putString(KEY_LAST_DEVICE, v).apply()

    /** Servis başlayınca son cihaza kendiliğinden bağlan. */
    var autoConnect: Boolean
        get() = prefs.getBoolean(KEY_AUTO_CONNECT, true)
        set(v) = prefs.edit().putBoolean(KEY_AUTO_CONNECT, v).apply()

    /** USB köprüsü: mesajları adb reverse üzerinden simülatöre de gönder (1.7). */
    var bridgeEnabled: Boolean
        get() = prefs.getBoolean(KEY_BRIDGE, false)
        set(v) = prefs.edit().putBoolean(KEY_BRIDGE, v).apply()

    fun toMap(): Map<String, Any?> = mapOf(
        KEY_AUTO_CONNECT to autoConnect,
        KEY_LAST_DEVICE to lastDeviceAddress,
        KEY_BRIDGE to bridgeEnabled,
    )

    fun apply(map: Map<*, *>) {
        (map[KEY_AUTO_CONNECT] as? Boolean)?.let { autoConnect = it }
        (map[KEY_BRIDGE] as? Boolean)?.let { bridgeEnabled = it }
    }

    companion object {
        const val KEY_AUTO_CONNECT = "autoConnect"
        const val KEY_LAST_DEVICE = "lastDeviceAddress"
        const val KEY_BRIDGE = "bridgeEnabled"
    }
}
