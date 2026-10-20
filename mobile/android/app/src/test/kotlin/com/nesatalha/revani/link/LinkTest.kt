package com.nesatalha.revani.link

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertNull
import org.junit.Test

class LinkTest {
    @Test
    fun frameHasLittleEndianLength() {
        assertArrayEquals(byteArrayOf(0x01, 0x00, 0x08), TcpTransport.frame(byteArrayOf(0x08)))
        val big = ByteArray(300) { 0x02 }
        val f = TcpTransport.frame(big)
        assertArrayEquals(byteArrayOf(0x2c, 0x01), f.copyOf(2))
        assertArrayEquals(big, f.copyOfRange(2, f.size))
    }

    @Test
    fun queueKeepsLatestPerType() {
        val q = MessageQueue()
        q.put(byteArrayOf(0x09, 10))
        q.put(byteArrayOf(0x02, 1))
        q.put(byteArrayOf(0x09, 20)) // eski GPS'in yerini alır, sona geçer
        assertArrayEquals(byteArrayOf(0x02, 1), q.poll())
        assertArrayEquals(byteArrayOf(0x09, 20), q.poll())
        assertNull(q.poll())
    }
}
