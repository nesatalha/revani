package com.nesatalha.revani.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.os.ParcelUuid
import android.util.Log
import com.nesatalha.revani.RevaniState
import com.nesatalha.revani.RevaniState.BleState
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.selects.select
import kotlinx.coroutines.withTimeoutOrNull

/**
 * Gidondaki ekranla BLE bağlantısı (Android central).
 *
 * Bağlantı sırası: connectGatt → MTU 517 → 2M PHY → yüksek öncelik → servis keşfi →
 * TX bildirimi açılır → hazır. Kopunca üstel beklemeyle (1, 2, 4 … 30 sn) yeniden bağlanır.
 * Gönderilecek mesajlar tipe göre birleştirilir: aynı tipten yalnız en sonuncusu gider.
 */
@SuppressLint("MissingPermission") // izinler Flutter izin akışında alınıyor
class DashLink(
    private val context: Context,
    private val scope: CoroutineScope,
    private val onReady: () -> Unit,
    private val onIncoming: (ByteArray) -> Unit,
) {
    private val adapter: BluetoothAdapter? =
        context.getSystemService(BluetoothManager::class.java)?.adapter

    private var connectionJob: Job? = null
    private var gatt: BluetoothGatt? = null

    // ---------------------------------------------------------------- tarama

    private var scanCallback: ScanCallback? = null

    fun startScan() {
        val scanner = adapter?.bluetoothLeScanner ?: return fail("Bluetooth kapalı")
        stopScan()
        RevaniState.scanResults.value = emptyList()
        val cb = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                val d = RevaniState.ScannedDevice(
                    name = result.scanRecord?.deviceName ?: result.device.name ?: "Revani",
                    address = result.device.address,
                    rssi = result.rssi,
                )
                RevaniState.scanResults.value =
                    (RevaniState.scanResults.value.filter { it.address != d.address } + d)
                        .sortedByDescending { it.rssi }
            }

            override fun onScanFailed(errorCode: Int) = fail("Tarama hatası $errorCode")
        }
        scanCallback = cb
        val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(BleUuids.SERVICE)).build()
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        scanner.startScan(listOf(filter), settings, cb)
        if (RevaniState.status.value.bleState == BleState.IDLE) setState(BleState.SCANNING)
        scope.launch {
            delay(SCAN_TIMEOUT_MS)
            if (scanCallback === cb) stopScan()
        }
    }

    fun stopScan() {
        scanCallback?.let { adapter?.bluetoothLeScanner?.stopScan(it) }
        scanCallback = null
        if (RevaniState.status.value.bleState == BleState.SCANNING) setState(BleState.IDLE)
    }

    // ---------------------------------------------------------------- bağlantı

    fun connect(address: String) {
        val device = try {
            adapter?.getRemoteDevice(address)
        } catch (e: IllegalArgumentException) {
            null
        } ?: return fail("Geçersiz cihaz adresi: $address")
        stopScan()
        disconnect()
        connectionJob = scope.launch { connectionLoop(device) }
    }

    fun disconnect() {
        connectionJob?.cancel()
        connectionJob = null
        closeGatt()
        RevaniState.update { it.copy(bleState = BleState.IDLE, displayBattery = null) }
    }

    private suspend fun connectionLoop(device: BluetoothDevice) {
        var attempt = 0
        RevaniState.update { it.copy(deviceName = device.name ?: "Revani", deviceAddress = device.address) }
        while (scope.isActive) {
            setState(if (attempt == 0) BleState.CONNECTING else BleState.RECONNECTING)
            val wasReady = runConnection(device)
            closeGatt()
            RevaniState.update { it.copy(displayBattery = null) }
            if (wasReady) attempt = 0
            val backoff = minOf(MAX_BACKOFF_MS, INITIAL_BACKOFF_MS shl minOf(attempt, 5))
            attempt++
            setState(BleState.RECONNECTING)
            delay(backoff)
        }
    }

    private sealed interface Event {
        data class Connection(val status: Int, val state: Int) : Event
        data class Mtu(val mtu: Int, val status: Int) : Event
        data class Services(val status: Int) : Event
        data class Descriptor(val status: Int) : Event
    }

    private val events = Channel<Event>(Channel.UNLIMITED)
    private val writeAcks = Channel<Int>(Channel.CONFLATED)
    private var disconnected = CompletableDeferred<Unit>()

    private val callback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_DISCONNECTED) disconnected.complete(Unit)
            events.trySend(Event.Connection(status, newState))
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            events.trySend(Event.Mtu(mtu, status))
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            events.trySend(Event.Services(status))
        }

        override fun onDescriptorWrite(g: BluetoothGatt, d: BluetoothGattDescriptor, status: Int) {
            events.trySend(Event.Descriptor(status))
        }

        override fun onCharacteristicWrite(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            writeAcks.trySend(status)
        }

        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            c: BluetoothGattCharacteristic,
            value: ByteArray,
        ) {
            if (c.uuid == BleUuids.TX) onIncoming(value)
        }

        @Deprecated("API 33 öncesi")
        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic) {
            @Suppress("DEPRECATION")
            if (Build.VERSION.SDK_INT < 33 && c.uuid == BleUuids.TX) onIncoming(c.value ?: return)
        }
    }

    private suspend inline fun <reified T : Event> await(timeoutMs: Long = STEP_TIMEOUT_MS): T? =
        withTimeoutOrNull(timeoutMs) {
            var result: T? = null
            while (result == null) {
                val e = events.receive()
                if (e is Event.Connection && e.state == BluetoothProfile.STATE_DISCONNECTED) break
                if (e is T) result = e
            }
            result
        }

    /** Tek bağlantı denemesi; bağlantı kopana kadar döner. Hazır hale geldiyse true. */
    private suspend fun runConnection(device: BluetoothDevice): Boolean {
        while (events.tryReceive().isSuccess) Unit
        disconnected = CompletableDeferred()
        val g = device.connectGatt(context, false, callback, BluetoothDevice.TRANSPORT_LE)
            ?: return false
        gatt = g

        val conn = await<Event.Connection>(CONNECT_TIMEOUT_MS)
        if (conn == null || conn.state != BluetoothProfile.STATE_CONNECTED) return false

        g.requestMtu(MTU)
        val mtu = await<Event.Mtu>()?.mtu ?: DEFAULT_MTU
        maxPayload = mtu - 3
        if (adapter?.isLe2MPhySupported == true) {
            g.setPreferredPhy(BluetoothDevice.PHY_LE_2M_MASK, BluetoothDevice.PHY_LE_2M_MASK, BluetoothDevice.PHY_OPTION_NO_PREFERRED)
        }
        g.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH)

        if (!g.discoverServices()) return false
        if (await<Event.Services>()?.status != BluetoothGatt.GATT_SUCCESS) return false
        val service = g.getService(BleUuids.SERVICE) ?: return false.also { fail("Revani servisi bulunamadı") }
        val rx = service.getCharacteristic(BleUuids.RX) ?: return false
        val tx = service.getCharacteristic(BleUuids.TX) ?: return false

        g.setCharacteristicNotification(tx, true)
        val cccd = tx.getDescriptor(BleUuids.CCCD) ?: return false
        writeDescriptor(g, cccd, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
        if (await<Event.Descriptor>()?.status != BluetoothGatt.GATT_SUCCESS) return false

        setState(BleState.READY)
        onReady()
        writerLoop(g, rx)
        return true
    }

    // ---------------------------------------------------------------- gönderme

    private val pending = LinkedHashMap<Int, ByteArray>()
    private val wake = Channel<Unit>(Channel.CONFLATED)
    private var maxPayload = DEFAULT_MTU - 3

    /** Mesajı kuyruğa koyar; aynı tipte bekleyen eski mesajın yerini alır. */
    fun send(message: ByteArray) {
        if (message.isEmpty()) return
        synchronized(pending) {
            val type = message[0].toInt() and 0xFF
            pending.remove(type) // sıraya yeniden en sona girsin
            pending[type] = message
        }
        wake.trySend(Unit)
    }

    private fun nextPending(): ByteArray? = synchronized(pending) {
        val it = pending.entries.iterator()
        if (!it.hasNext()) null else it.next().value.also { _ -> it.remove() }
    }

    private suspend fun writerLoop(g: BluetoothGatt, rx: BluetoothGattCharacteristic) {
        while (!disconnected.isCompleted) {
            val msg = nextPending()
            if (msg == null) {
                select {
                    wake.onReceive { }
                    disconnected.onAwait { }
                }
                continue
            }
            if (msg.size > maxPayload) {
                fail("Mesaj MTU'ya sığmıyor (${msg.size} > $maxPayload), tip ${msg[0]}")
                continue
            }
            while (writeAcks.tryReceive().isSuccess) Unit
            if (!writeCharacteristic(g, rx, msg)) {
                delay(20) // yığın meşgul, kısa bekle
                send(msg)
                continue
            }
            withTimeoutOrNull(WRITE_TIMEOUT_MS) { writeAcks.receive() }
        }
    }

    @Suppress("DEPRECATION")
    private fun writeCharacteristic(g: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray): Boolean {
        val type = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        return if (Build.VERSION.SDK_INT >= 33) {
            g.writeCharacteristic(c, value, type) == BluetoothGatt.GATT_SUCCESS
        } else {
            c.writeType = type
            c.value = value
            g.writeCharacteristic(c)
        }
    }

    @Suppress("DEPRECATION")
    private fun writeDescriptor(g: BluetoothGatt, d: BluetoothGattDescriptor, value: ByteArray) {
        if (Build.VERSION.SDK_INT >= 33) {
            g.writeDescriptor(d, value)
        } else {
            d.value = value
            g.writeDescriptor(d)
        }
    }

    private fun closeGatt() {
        gatt?.let {
            it.disconnect()
            it.close()
        }
        gatt = null
        disconnected.complete(Unit)
    }

    private fun setState(s: BleState) = RevaniState.update { it.copy(bleState = s) }

    private fun fail(msg: String) {
        Log.w(TAG, msg)
        RevaniState.error(msg)
    }

    companion object {
        private const val TAG = "DashLink"
        private const val MTU = 517
        private const val DEFAULT_MTU = 23
        private const val SCAN_TIMEOUT_MS = 20_000L
        private const val CONNECT_TIMEOUT_MS = 15_000L
        private const val STEP_TIMEOUT_MS = 5_000L
        private const val WRITE_TIMEOUT_MS = 1_000L
        private const val INITIAL_BACKOFF_MS = 1_000L
        private const val MAX_BACKOFF_MS = 30_000L
    }
}
