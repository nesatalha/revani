package com.nesatalha.revani.nav

import java.io.ByteArrayOutputStream
import java.io.DataInputStream
import java.io.File
import java.util.zip.InflaterInputStream

/**
 * Testler için en küçük PNG çözücü (8 bit RGBA/RGB, taramasız). JVM birim testlerinde
 * android.jar'da javax.imageio ve BitmapFactory yok.
 */
object TestPng {
    data class Image(val width: Int, val height: Int, val argb: IntArray)

    fun read(file: File): Image {
        val input = DataInputStream(file.inputStream().buffered())
        input.skipBytes(8) // imza
        var width = 0
        var height = 0
        var channels = 4
        val idat = ByteArrayOutputStream()
        while (true) {
            val len = input.readInt()
            val type = String(ByteArray(4).also { input.readFully(it) }, Charsets.US_ASCII)
            val data = ByteArray(len).also { input.readFully(it) }
            input.readInt() // crc
            when (type) {
                "IHDR" -> {
                    width = readInt(data, 0)
                    height = readInt(data, 4)
                    require(data[8].toInt() == 8 && data[12].toInt() == 0) { "sadece 8 bit, taramasız" }
                    channels = when (data[9].toInt()) {
                        6 -> 4
                        2 -> 3
                        else -> error("desteklenmeyen renk tipi ${data[9]}")
                    }
                }
                "IDAT" -> idat.write(data)
                "IEND" -> break
            }
        }
        val raw = InflaterInputStream(idat.toByteArray().inputStream()).readBytes()
        val stride = width * channels
        val prev = IntArray(stride)
        val cur = IntArray(stride)
        val argb = IntArray(width * height)
        var pos = 0
        for (y in 0 until height) {
            val filter = raw[pos++].toInt()
            for (i in 0 until stride) {
                val x = raw[pos++].toInt() and 0xFF
                val a = if (i >= channels) cur[i - channels] else 0
                val b = prev[i]
                val c = if (i >= channels) prev[i - channels] else 0
                cur[i] = (x + when (filter) {
                    0 -> 0
                    1 -> a
                    2 -> b
                    3 -> (a + b) / 2
                    4 -> paeth(a, b, c)
                    else -> error("filtre $filter")
                }) and 0xFF
            }
            for (px in 0 until width) {
                val o = px * channels
                val alpha = if (channels == 4) cur[o + 3] else 0xFF
                argb[y * width + px] = (alpha shl 24) or (cur[o] shl 16) or (cur[o + 1] shl 8) or cur[o + 2]
            }
            cur.copyInto(prev)
        }
        return Image(width, height, argb)
    }

    private fun readInt(b: ByteArray, o: Int) =
        ((b[o].toInt() and 0xFF) shl 24) or ((b[o + 1].toInt() and 0xFF) shl 16) or
            ((b[o + 2].toInt() and 0xFF) shl 8) or (b[o + 3].toInt() and 0xFF)

    private fun paeth(a: Int, b: Int, c: Int): Int {
        val p = a + b - c
        val pa = Math.abs(p - a)
        val pb = Math.abs(p - b)
        val pc = Math.abs(p - c)
        return if (pa <= pb && pa <= pc) a else if (pb <= pc) b else c
    }
}
