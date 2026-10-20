package com.nesatalha.revani.link

import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withTimeoutOrNull
import java.io.DataInputStream
import java.io.IOException
import java.io.OutputStream
import java.net.InetSocketAddress
import java.net.Socket

/**
 * USB köprüsü (adım 1.7): mesajları `adb reverse tcp:7878 tcp:7878` üzerinden Mac'teki
 * LVGL simülatörüne gönderir. Çerçeve: `[u16 uzunluk, little-endian][mesaj]`.
 * Simülatör kapalıysa üstel beklemeyle yeniden dener.
 *
 * `adb reverse` telefonda portu her zaman dinlediği için TCP bağlantısı simülatör
 * kapalıyken de kurulur ve hemen kapanır. Bu yüzden bağlantı ancak simülatörden ilk
 * mesaj (STATUS) gelince hazır sayılır.
 */
class TcpTransport(
    scope: CoroutineScope,
    private val onState: (State) -> Unit,
    private val onReady: () -> Unit,
    private val onIncoming: (ByteArray) -> Unit,
    private val host: String = "127.0.0.1",
    private val port: Int = DEFAULT_PORT,
) : DashTransport {

    enum class State { CONNECTING, CONNECTED }

    private val queue = MessageQueue()
    @Volatile private var greeted = false
    private val wake = Channel<Unit>(Channel.CONFLATED)
    private val job: Job = scope.launch(Dispatchers.IO) { loop() }

    override fun send(message: ByteArray) {
        queue.put(message)
        wake.trySend(Unit)
    }

    override fun close() {
        job.cancel()
    }

    private suspend fun CoroutineScope.loop() {
        var attempt = 0
        while (isActive) {
            onState(State.CONNECTING)
            try {
                Socket().use { socket ->
                    socket.tcpNoDelay = true
                    socket.connect(InetSocketAddress(host, port), CONNECT_TIMEOUT_MS)
                    queue.clear()
                    val reader = launch { readLoop(DataInputStream(socket.getInputStream())) }
                    try {
                        writeLoop(socket.getOutputStream()) { reader.isActive }
                    } finally {
                        reader.cancel()
                    }
                    if (greeted) attempt = 0
                    greeted = false
                }
            } catch (e: IOException) {
                Log.d(TAG, "Köprü bağlantısı yok: ${e.message}")
            }
            delay(minOf(MAX_BACKOFF_MS, INITIAL_BACKOFF_MS shl minOf(attempt++, 4)))
        }
    }

    private suspend fun writeLoop(out: OutputStream, alive: () -> Boolean) {
        while (alive()) {
            val msg = if (greeted) queue.poll() else null
            if (msg == null) {
                // Okuyucu bağlantının koptuğunu fark etsin diye ara ara uyan.
                withTimeoutOrNull(500) { wake.receive() }
                continue
            }
            out.write(frame(msg))
            out.flush()
        }
    }

    private fun readLoop(input: DataInputStream) {
        try {
            while (true) {
                val len = input.readUnsignedByte() or (input.readUnsignedByte() shl 8)
                val msg = ByteArray(len)
                input.readFully(msg)
                if (!greeted) {
                    greeted = true
                    onState(State.CONNECTED)
                    onReady()
                }
                onIncoming(msg)
            }
        } catch (e: IOException) {
            // bağlantı koptu; yazma döngüsü bitecek
        }
    }

    companion object {
        private const val TAG = "TcpTransport"
        const val DEFAULT_PORT = 7878
        private const val CONNECT_TIMEOUT_MS = 2_000
        private const val INITIAL_BACKOFF_MS = 1_000L
        private const val MAX_BACKOFF_MS = 10_000L

        /** `[u16 uzunluk LE][mesaj]` */
        fun frame(message: ByteArray): ByteArray {
            require(message.size <= 0xFFFF)
            return byteArrayOf((message.size and 0xFF).toByte(), (message.size shr 8).toByte()) + message
        }
    }
}
