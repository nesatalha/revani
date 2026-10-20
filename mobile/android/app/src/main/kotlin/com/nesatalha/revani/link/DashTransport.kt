package com.nesatalha.revani.link

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
 * (eski GPS/NAV değerinin gecikmeli gitmesinin anlamı yok).
 */
class MessageQueue {
    private val pending = LinkedHashMap<Int, ByteArray>()

    fun put(message: ByteArray) {
        if (message.isEmpty()) return
        synchronized(pending) {
            val type = message[0].toInt() and 0xFF
            pending.remove(type) // sıraya yeniden en sona girsin
            pending[type] = message
        }
    }

    fun poll(): ByteArray? = synchronized(pending) {
        val it = pending.entries.iterator()
        if (!it.hasNext()) null else it.next().value.also { _ -> it.remove() }
    }

    fun clear() = synchronized(pending) { pending.clear() }
}
