package com.nesatalha.revani.link

import com.nesatalha.revani.ble.Protocol

/**
 * Ekrana mesaj taşıyan bağlantı. Kartla BLE ([com.nesatalha.revani.ble.BleTransport]),
 * geliştirmede USB üzerinden simülatörle TCP ([TcpTransport]). Mesaj baytları her ikisinde
 * aynı (docs/PLAN.md bölüm 4).
 */
interface DashTransport {
    /** Mesajı kuyruğa koyar; aynı tipte bekleyen eski mesajın yerini alır. */
    fun send(message: ByteArray)

    /** Bağlantıyı kapatır, yeniden bağlanmayı durdurur. */
    fun close()
}

/**
 * Tipe göre birleştiren gönderme kuyruğu: aynı tipten yalnız en sonuncusu gider
 * (eski GPS/NAV değerinin gecikmeli gitmesinin anlamı yok). Harita parçaları parça
 * numarasına göre ayrı tutulur ve diğer mesajlardan sonra gönderilir, böylece büyük
 * bir kare ok ve hız güncellemesini geciktirmez.
 */
class MessageQueue {
    private val pending = LinkedHashMap<Int, ByteArray>()

    fun put(message: ByteArray) {
        if (message.isEmpty()) return
        synchronized(pending) {
            val key = keyOf(message)
            // Yeni karenin ilk parçası: eski karenin bekleyen parçaları artık geçersiz.
            if (key == MAP_KEY_BASE) pending.keys.removeAll { it >= MAP_KEY_BASE }
            pending.remove(key) // sıraya yeniden en sona girsin
            pending[key] = message
        }
    }

    fun poll(): ByteArray? = synchronized(pending) {
        val key = pending.keys.firstOrNull { it < MAP_KEY_BASE } ?: pending.keys.firstOrNull()
        key?.let { pending.remove(it) }
    }

    fun clear() = synchronized(pending) { pending.clear() }

    private fun keyOf(m: ByteArray): Int {
        val type = m[0].toInt() and 0xFF
        if (type != Protocol.MAP_CHUNK || m.size < 5) return type
        val chunk = (m[3].toInt() and 0xFF) or ((m[4].toInt() and 0xFF) shl 8)
        return MAP_KEY_BASE + chunk
    }

    private companion object {
        const val MAP_KEY_BASE = 0x10000
    }
}
