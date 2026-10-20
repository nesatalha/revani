package com.nesatalha.revani

import android.Manifest
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat
import androidx.core.content.ContextCompat
import com.nesatalha.revani.RevaniState.BleState
import com.nesatalha.revani.ble.BleTransport
import com.nesatalha.revani.ble.Protocol
import com.nesatalha.revani.link.TcpTransport
import com.nesatalha.revani.location.LocationSource
import com.nesatalha.revani.map.MapMath
import com.nesatalha.revani.map.MapRenderer
import com.nesatalha.revani.nav.Maneuver
import com.nesatalha.revani.nav.NavInfo
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.util.TimeZone

/**
 * Arka planda sürekli çalışan foreground servis: BLE bağlantısı, GPS ve
 * navigasyon bildirimini ekrana aktarır. Flutter arayüzü kapalıyken de çalışır.
 */
class RevaniService : Service() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private lateinit var settings: Settings
    private lateinit var link: BleTransport
    private var bridge: TcpTransport? = null
    private lateinit var location: LocationSource
    private var lastFixAt = 0L
    private var fakeNavJob: Job? = null
    private var mapRenderer: MapRenderer? = null
    private var position: Pair<Double, Double>? = null

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        settings = Settings(this)
        startInForeground()
        link = BleTransport(this, scope, onReady = ::onLinkReady, onIncoming = ::onIncoming)
        applyBridgeSetting()
        location = LocationSource(this, ::onFix, onPosition = { lat, lon -> position = lat to lon })
        RevaniState.update { it.copy(serviceRunning = true) }

        if (hasPermission(Manifest.permission.ACCESS_FINE_LOCATION)) {
            if (!location.start()) RevaniState.error("GPS kapalı")
        } else {
            RevaniState.error("Konum izni yok, hız gönderilmeyecek")
        }

        // Navigasyon değişince ekrana gönder; bitince NAV_END.
        scope.launch {
            var wasActive = false
            RevaniState.nav.collect { nav ->
                if (nav != null) {
                    send(Protocol.nav(nav))
                    wasActive = true
                } else if (wasActive) {
                    send(Protocol.navEnd())
                    wasActive = false
                }
                RevaniState.update { it.copy(lastNav = nav ?: it.lastNav, navActive = nav != null) }
            }
        }
        // GPS kesilirse hız bilgisini boşalt (ekran kendi zaman aşımıyla grileşir).
        scope.launch {
            while (isActive) {
                delay(1_000)
                if (lastFixAt != 0L && System.currentTimeMillis() - lastFixAt > GPS_STALE_MS) {
                    RevaniState.update { it.copy(gpsSpeed = null) }
                }
            }
        }
        scope.launch { mapLoop() }
        scope.launch {
            RevaniState.status.map { it.bleState }.distinctUntilChanged().collect { updateNotification() }
        }

        val last = settings.lastDeviceAddress
        if (settings.autoConnect && last != null) link.connect(last)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_STOP -> {
                stopSelf()
                return START_NOT_STICKY
            }
            ACTION_SCAN -> link.startScan()
            ACTION_STOP_SCAN -> link.stopScan()
            ACTION_CONNECT -> intent.getStringExtra(EXTRA_ADDRESS)?.let {
                settings.lastDeviceAddress = it
                link.connect(it)
            }
            ACTION_DISCONNECT -> link.disconnect()
            ACTION_SETTINGS_CHANGED -> applyBridgeSetting()
            ACTION_FAKE_NAV -> setFakeNav(intent.getBooleanExtra(EXTRA_ENABLED, false))
        }
        return START_STICKY
    }

    /** Mesajı karta (BLE) ve açıksa simülatöre (USB köprüsü) gönderir. */
    private fun send(message: ByteArray) {
        link.send(message)
        bridge?.send(message)
    }

    private fun applyBridgeSetting() {
        if (settings.bridgeEnabled == (bridge != null)) return
        if (settings.bridgeEnabled) {
            bridge = TcpTransport(
                scope,
                onState = { st -> RevaniState.update { it.copy(bridgeState = st.name.lowercase()) } },
                onReady = { scope.launch { onLinkReady() } },
                onIncoming = { data -> scope.launch { onIncoming(data) } },
            )
        } else {
            bridge?.close()
            bridge = null
            RevaniState.update { it.copy(bridgeState = "off") }
        }
    }

    override fun onDestroy() {
        mapRenderer?.close()
        mapRenderer = null
        bridge?.close()
        bridge = null
        location.stop()
        link.disconnect()
        scope.cancel()
        RevaniState.update {
            it.copy(serviceRunning = false, bleState = BleState.IDLE, bridgeState = "off", gpsSpeed = null)
        }
        super.onDestroy()
    }

    private fun onLinkReady() {
        // Bağlanınca saat (adım 1.4.11) ve varsa güncel navigasyon
        val now = System.currentTimeMillis()
        send(Protocol.time(now / 1000, TimeZone.getDefault().getOffset(now) / 60_000))
        RevaniState.nav.value?.let { send(Protocol.nav(it)) }
    }

    private fun onIncoming(data: ByteArray) {
        when (val msg = Protocol.parseIncoming(data)) {
            is Protocol.Incoming.Status -> RevaniState.update { it.copy(displayBattery = msg.batteryPercent) }
            is Protocol.Incoming.Button -> Unit // Faz 3
            null -> RevaniState.error("Bilinmeyen mesaj: ${data.joinToString(" ") { "%02x".format(it) }}")
        }
    }

    private fun onFix(speedKmh: Int, headingDeg: Int) {
        lastFixAt = System.currentTimeMillis()
        RevaniState.update { it.copy(gpsSpeed = speedKmh, gpsHeading = headingDeg) }
        send(Protocol.gps(speedKmh, headingDeg))
    }

    /**
     * Faz 2: ekran bağlıyken ve navigasyon varken harita karesi üretip parçalar halinde
     * gönderir. Kare hızı dönüşe yaklaşınca artar (2.1.8).
     */
    private suspend fun mapLoop() {
        var frame = 0
        var fps = 0.0
        var lastSent = 0L
        while (true) {
            val nav = RevaniState.nav.value
            delay(MapMath.frameIntervalMs(nav?.distanceM))
            val st = RevaniState.status.value
            val linked = st.bleState == BleState.READY || st.bridgeState == "connected"
            val pos = position
            if (!settings.mapEnabled || nav == null || !linked || pos == null) continue

            val renderer = mapRenderer ?: try {
                MapRenderer(this).also {
                    mapRenderer = it
                    RevaniState.update { s -> s.copy(mapDemoStyle = it.usingDemoStyle) }
                }
            } catch (e: Exception) {
                RevaniState.error("Harita başlatılamadı: ${e.message}")
                continue
            }
            renderer.render(pos.first, pos.second, (st.gpsHeading ?: 0).toDouble(), st.gpsSpeed ?: 0) { jpeg ->
                frame = (frame + 1) and 0xFFFF
                MapMath.chunks(frame, jpeg).forEach(::send)
                val now = System.currentTimeMillis()
                if (lastSent != 0L) fps = 0.7 * fps + 0.3 * (1000.0 / (now - lastSent))
                lastSent = now
                RevaniState.update { it.copy(mapFps = fps, mapBytes = jpeg.size) }
            }
        }
    }

    /** Masada telefon olmadan test: Google Maps yerine sahte navigasyon üretir. */
    private fun setFakeNav(enabled: Boolean) {
        fakeNavJob?.cancel()
        fakeNavJob = null
        if (!enabled) {
            RevaniState.nav.value = null
            return
        }
        fakeNavJob = scope.launch {
            val steps = listOf(
                Maneuver.RIGHT to "Bağdat Cd.",
                Maneuver.SLIGHT_LEFT to "Ethem Efendi Cd.",
                Maneuver.ROUNDABOUT to "Kozyatağı Kavşağı",
                Maneuver.LEFT to "Moda Cd.",
                Maneuver.ARRIVE to "Moda Sahili",
            )
            for ((maneuver, street) in steps) {
                var d = 600
                while (d >= 0) {
                    RevaniState.nav.value = NavInfo(
                        maneuver = maneuver,
                        roundaboutExit = if (maneuver == Maneuver.ROUNDABOUT) 2 else 0,
                        distanceM = d,
                        etaMin = 9,
                        street = street,
                    )
                    delay(1_000)
                    d -= 25
                }
            }
            RevaniState.nav.value = null
        }
    }

    // ---------------------------------------------------------------- bildirim

    private fun startInForeground() {
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel(CHANNEL_ID, "Revani bağlantısı", NotificationManager.IMPORTANCE_LOW),
        )
        var types = 0
        if (Build.VERSION.SDK_INT >= 29) {
            if (Build.VERSION.SDK_INT < 31 || hasPermission(Manifest.permission.BLUETOOTH_CONNECT)) {
                types = types or ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE
            }
            if (hasPermission(Manifest.permission.ACCESS_FINE_LOCATION)) {
                types = types or ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION
            }
        }
        ServiceCompat.startForeground(this, NOTIFICATION_ID, buildNotification(), types)
    }

    private fun updateNotification() {
        getSystemService(NotificationManager::class.java).notify(NOTIFICATION_ID, buildNotification())
    }

    private fun buildNotification(): Notification {
        val open = PendingIntent.getActivity(
            this, 0,
            Intent(this, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP),
            PendingIntent.FLAG_IMMUTABLE,
        )
        val state = when (RevaniState.status.value.bleState) {
            BleState.READY -> "Ekrana bağlı"
            BleState.CONNECTING -> "Bağlanıyor…"
            BleState.RECONNECTING -> "Yeniden bağlanıyor…"
            BleState.SCANNING -> "Ekran aranıyor…"
            BleState.IDLE -> "Ekran bağlı değil"
        }
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.mipmap.ic_launcher)
            .setContentTitle("Revani")
            .setContentText(state)
            .setOngoing(true)
            .setContentIntent(open)
            .setForegroundServiceBehavior(NotificationCompat.FOREGROUND_SERVICE_IMMEDIATE)
            .build()
    }

    private fun hasPermission(p: String) =
        ContextCompat.checkSelfPermission(this, p) == PackageManager.PERMISSION_GRANTED

    companion object {
        private const val CHANNEL_ID = "revani_service"
        private const val NOTIFICATION_ID = 1
        private const val GPS_STALE_MS = 3_000L

        const val ACTION_START = "revani.START"
        const val ACTION_STOP = "revani.STOP"
        const val ACTION_SCAN = "revani.SCAN"
        const val ACTION_STOP_SCAN = "revani.STOP_SCAN"
        const val ACTION_CONNECT = "revani.CONNECT"
        const val ACTION_DISCONNECT = "revani.DISCONNECT"
        const val ACTION_FAKE_NAV = "revani.FAKE_NAV"
        const val ACTION_SETTINGS_CHANGED = "revani.SETTINGS_CHANGED"
        const val EXTRA_ADDRESS = "address"
        const val EXTRA_ENABLED = "enabled"

        fun command(context: Context, action: String, extras: Intent.() -> Unit = {}) {
            val intent = Intent(context, RevaniService::class.java).setAction(action).apply(extras)
            if (action == ACTION_STOP) {
                if (RevaniState.status.value.serviceRunning) context.startService(intent)
            } else {
                ContextCompat.startForegroundService(context, intent)
            }
        }
    }
}
