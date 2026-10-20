package com.nesatalha.revani

import com.nesatalha.revani.nav.NavInfo
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** Servis bileşenleri ve Flutter arasında paylaşılan durum. */
object RevaniState {
    enum class BleState { IDLE, SCANNING, CONNECTING, READY, RECONNECTING }

    data class ScannedDevice(val name: String, val address: String, val rssi: Int)

    data class Status(
        val serviceRunning: Boolean = false,
        val bleState: BleState = BleState.IDLE,
        val deviceName: String? = null,
        val deviceAddress: String? = null,
        val displayBattery: Int? = null,
        val lastNav: NavInfo? = null,
        val navActive: Boolean = false,
        val gpsSpeed: Int? = null,
        val gpsHeading: Int? = null,
        val mapFps: Double = 0.0,
        val errors: List<String> = emptyList(),
    )

    private val _status = MutableStateFlow(Status())
    val status: StateFlow<Status> = _status

    /** Bildirimden gelen güncel navigasyon; navigasyon yoksa null. */
    val nav = MutableStateFlow<NavInfo?>(null)

    val scanResults = MutableStateFlow<List<ScannedDevice>>(emptyList())

    fun update(block: (Status) -> Status) = _status.update(block)

    private val timeFormat = SimpleDateFormat("HH:mm:ss", Locale.ROOT)

    fun error(message: String) {
        val line = "${timeFormat.format(Date())} $message"
        _status.update { it.copy(errors = (it.errors + line).takeLast(MAX_ERRORS)) }
    }

    private const val MAX_ERRORS = 20
}
