package com.nesatalha.revani.ble

import com.nesatalha.revani.nav.NavInfo
import java.io.ByteArrayOutputStream

/**
 * BLE mesaj kodlama. Biçim: `[tip: 1 bayt][veri]`, çok baytlı sayılar little-endian.
 * Tanım docs/PLAN.md bölüm 4'te; firmware tarafı firmware/protocol/protocol.c.
 */
object Protocol {
    const val TELEMETRY = 0x01
    const val NAV = 0x02
    const val MAP_CHUNK = 0x03
    const val NOTIFICATION = 0x04
    const val MEDIA = 0x05
    const val CALL = 0x06
    const val TIME = 0x07
    const val NAV_END = 0x08
    const val GPS = 0x09
    const val BUTTON = 0x81
    const val STATUS = 0x82

    /** Cadde adı için bayt sınırı; ekranda zaten tek satır gösteriliyor. */
    const val MAX_STREET_BYTES = 96

    /** STATUS'ta pil ölçülemiyorsa gelen değer. */
    const val BATTERY_UNKNOWN = 0xFF

    fun nav(info: NavInfo): ByteArray {
        val street = truncateUtf8(info.street, MAX_STREET_BYTES)
        return message(NAV) {
            u8(info.maneuver.code)
            u8(info.roundaboutExit)
            u16(info.distanceM)
            u16(info.etaMin)
            u8(street.size)
            write(street)
        }
    }

    fun navEnd(): ByteArray = byteArrayOf(NAV_END.toByte())

    fun gps(speedKmh: Int, headingDeg: Int): ByteArray = message(GPS) {
        u8(speedKmh)
        u16(((headingDeg % 360) + 360) % 360)
    }

    fun time(unixSeconds: Long, utcOffsetMin: Int): ByteArray = message(TIME) {
        u32(unixSeconds)
        i16(utcOffsetMin)
    }

    sealed interface Incoming {
        data class Button(val button: Int, val longPress: Boolean) : Incoming
        data class Status(val batteryPercent: Int?, val screen: Int) : Incoming
    }

    /** ESP32'den gelen mesajı çözer; tanınmayan veya kısa mesajda null. */
    fun parseIncoming(data: ByteArray): Incoming? {
        if (data.isEmpty()) return null
        fun u8(i: Int) = data[i].toInt() and 0xFF
        return when (u8(0)) {
            BUTTON -> if (data.size >= 3) Incoming.Button(u8(1), u8(2) == 1) else null
            STATUS -> if (data.size >= 3) {
                val pct = u8(1)
                Incoming.Status(if (pct == BATTERY_UNKNOWN) null else pct.coerceAtMost(100), u8(2))
            } else null
            else -> null
        }
    }

    /** UTF-8'e çevirip [maxBytes]'a sığdırır, çok baytlı karakteri ortadan bölmez. */
    fun truncateUtf8(text: String, maxBytes: Int): ByteArray {
        val bytes = text.toByteArray(Charsets.UTF_8)
        if (bytes.size <= maxBytes) return bytes
        var end = maxBytes
        // Devam baytlarından (10xxxxxx) geri çekil, karakterin başında kes.
        while (end > 0 && (bytes[end].toInt() and 0xC0) == 0x80) end--
        return bytes.copyOf(end)
    }

    private class Writer : ByteArrayOutputStream() {
        fun u8(v: Int) = write(v.coerceIn(0, 0xFF))
        fun u16(v: Int) {
            val c = v.coerceIn(0, 0xFFFF)
            write(c and 0xFF)
            write(c shr 8)
        }
        fun i16(v: Int) {
            val c = v.coerceIn(Short.MIN_VALUE.toInt(), Short.MAX_VALUE.toInt())
            write(c and 0xFF)
            write((c shr 8) and 0xFF)
        }
        fun u32(v: Long) {
            val c = v.coerceIn(0, 0xFFFF_FFFFL)
            for (i in 0 until 4) write(((c shr (8 * i)) and 0xFF).toInt())
        }
    }

    private inline fun message(type: Int, body: Writer.() -> Unit): ByteArray =
        Writer().apply { write(type) }.apply(body).toByteArray()
}
