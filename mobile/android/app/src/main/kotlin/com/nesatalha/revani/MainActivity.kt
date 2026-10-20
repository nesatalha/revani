package com.nesatalha.revani

import android.Manifest
import android.annotation.SuppressLint
import android.content.ComponentName
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.PowerManager
import android.provider.Settings as AndroidSettings
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.nesatalha.revani.nav.NavInfo
import com.nesatalha.revani.nav.NavNotificationReader
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.launch

/**
 * Flutter kanallarını kaydeder (docs/PLAN.md bölüm 3.2). İş mantığı yok:
 * komutları [RevaniService]'e iletir, [RevaniState]'i Flutter'a akıtır.
 */
class MainActivity : FlutterActivity() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var pendingPermissionResult: MethodChannel.Result? = null

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        val messenger = flutterEngine.dartExecutor.binaryMessenger

        MethodChannel(messenger, "revani/control").setMethodCallHandler(::onControl)
        EventChannel(messenger, "revani/status").setStreamHandler(
            FlowStreamHandler(RevaniState.status, minIntervalMs = STATUS_INTERVAL_MS) { it.toMap() },
        )
        EventChannel(messenger, "revani/scan").setStreamHandler(
            FlowStreamHandler(RevaniState.scanResults) { list ->
                list.map { mapOf("name" to it.name, "address" to it.address, "rssi" to it.rssi) }
            },
        )
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }

    private fun onControl(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "startService" -> RevaniService.command(this, RevaniService.ACTION_START).also { result.success(null) }
            "stopService" -> RevaniService.command(this, RevaniService.ACTION_STOP).also { result.success(null) }
            "scanDevices" -> RevaniService.command(this, RevaniService.ACTION_SCAN).also { result.success(null) }
            "stopScan" -> RevaniService.command(this, RevaniService.ACTION_STOP_SCAN).also { result.success(null) }
            "connect" -> {
                val id = call.argument<String>("deviceId")
                    ?: return result.error("ARG", "deviceId gerekli", null)
                RevaniService.command(this, RevaniService.ACTION_CONNECT) {
                    putExtra(RevaniService.EXTRA_ADDRESS, id)
                }
                result.success(null)
            }
            "disconnect" -> RevaniService.command(this, RevaniService.ACTION_DISCONNECT).also { result.success(null) }
            "getSettings" -> result.success(Settings(this).toMap())
            "setSettings" -> {
                (call.arguments as? Map<*, *>)?.let { Settings(this).apply(it) }
                result.success(Settings(this).toMap())
            }
            "getPermissions" -> result.success(permissionState())
            "requestRuntimePermissions" -> requestRuntimePermissions(result)
            "openNotificationAccessSettings" -> {
                startActivity(Intent(AndroidSettings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
                result.success(null)
            }
            "requestIgnoreBatteryOptimizations" -> {
                requestIgnoreBatteryOptimizations()
                result.success(null)
            }
            "setFakeNav" -> {
                val enabled = call.argument<Boolean>("enabled") ?: false
                RevaniService.command(this, RevaniService.ACTION_FAKE_NAV) {
                    putExtra(RevaniService.EXTRA_ENABLED, enabled)
                }
                result.success(null)
            }
            else -> result.notImplemented()
        }
    }

    // ---------------------------------------------------------------- izinler

    private fun runtimePermissions(): List<String> = buildList {
        if (Build.VERSION.SDK_INT >= 31) {
            add(Manifest.permission.BLUETOOTH_SCAN)
            add(Manifest.permission.BLUETOOTH_CONNECT)
        }
        add(Manifest.permission.ACCESS_FINE_LOCATION)
        if (Build.VERSION.SDK_INT >= 33) add(Manifest.permission.POST_NOTIFICATIONS)
    }

    private fun granted(p: String) =
        ContextCompat.checkSelfPermission(this, p) == PackageManager.PERMISSION_GRANTED

    private fun permissionState(): Map<String, Boolean> {
        val bluetooth = Build.VERSION.SDK_INT < 31 ||
            (granted(Manifest.permission.BLUETOOTH_SCAN) && granted(Manifest.permission.BLUETOOTH_CONNECT))
        val notifications = Build.VERSION.SDK_INT < 33 || granted(Manifest.permission.POST_NOTIFICATIONS)
        val listeners = AndroidSettings.Secure.getString(contentResolver, "enabled_notification_listeners").orEmpty()
        val me = ComponentName(this, NavNotificationReader::class.java)
        val power = getSystemService(PowerManager::class.java)
        return mapOf(
            "bluetooth" to bluetooth,
            "location" to granted(Manifest.permission.ACCESS_FINE_LOCATION),
            "notifications" to notifications,
            "notificationAccess" to (listeners.contains(me.flattenToString()) || listeners.contains(me.flattenToShortString())),
            "batteryOptimizationIgnored" to power.isIgnoringBatteryOptimizations(packageName),
            "gpsBlockedByPowerSave" to gpsBlockedByPowerSave(power),
        )
    }

    /** Güç tasarrufu modu ekran kapalıyken GPS'i kapatıyorsa (S22'de görüldü) sürüşte hız gelmez. */
    private fun gpsBlockedByPowerSave(power: PowerManager): Boolean =
        power.isPowerSaveMode && power.locationPowerSaveMode in setOf(
            PowerManager.LOCATION_MODE_GPS_DISABLED_WHEN_SCREEN_OFF,
            PowerManager.LOCATION_MODE_ALL_DISABLED_WHEN_SCREEN_OFF,
            PowerManager.LOCATION_MODE_THROTTLE_REQUESTS_WHEN_SCREEN_OFF,
        )

    private fun requestRuntimePermissions(result: MethodChannel.Result) {
        val missing = runtimePermissions().filterNot(::granted)
        if (missing.isEmpty()) return result.success(permissionState())
        pendingPermissionResult?.success(permissionState())
        pendingPermissionResult = result
        ActivityCompat.requestPermissions(this, missing.toTypedArray(), PERMISSION_REQUEST)
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PERMISSION_REQUEST) {
            pendingPermissionResult?.success(permissionState())
            pendingPermissionResult = null
        }
    }

    @SuppressLint("BatteryLife") // sürekli BLE + GPS için muafiyet gerekli
    private fun requestIgnoreBatteryOptimizations() {
        startActivity(
            Intent(AndroidSettings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS)
                .setData(Uri.parse("package:$packageName")),
        )
    }

    /** Bir Flow'u EventChannel'a bağlar; [minIntervalMs] ile hızı sınırlar. */
    private inner class FlowStreamHandler<T>(
        private val flow: Flow<T>,
        private val minIntervalMs: Long = 0,
        private val encode: (T) -> Any?,
    ) : EventChannel.StreamHandler {
        private var job: Job? = null

        override fun onListen(arguments: Any?, events: EventChannel.EventSink) {
            job?.cancel()
            job = scope.launch {
                flow.collect {
                    events.success(encode(it))
                    if (minIntervalMs > 0) delay(minIntervalMs)
                }
            }
        }

        override fun onCancel(arguments: Any?) {
            job?.cancel()
            job = null
        }
    }

    companion object {
        private const val PERMISSION_REQUEST = 42
        private const val STATUS_INTERVAL_MS = 500L // ~2 Hz
    }
}

private fun NavInfo.toMap(): Map<String, Any> = mapOf(
    "maneuver" to maneuver.code,
    "roundaboutExit" to roundaboutExit,
    "distanceM" to distanceM,
    "etaMin" to etaMin,
    "street" to street,
)

private fun RevaniState.Status.toMap(): Map<String, Any?> = mapOf(
    "serviceRunning" to serviceRunning,
    "bleState" to bleState.name.lowercase(),
    "deviceName" to deviceName,
    "deviceAddress" to deviceAddress,
    "displayBattery" to displayBattery,
    "lastNav" to lastNav?.toMap(),
    "navActive" to navActive,
    "gpsSpeed" to gpsSpeed,
    "gpsHeading" to gpsHeading,
    "mapFps" to mapFps,
    "errors" to errors,
)
