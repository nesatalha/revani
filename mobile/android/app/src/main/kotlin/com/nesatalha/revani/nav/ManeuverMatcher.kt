package com.nesatalha.revani.nav

import kotlin.math.abs

/**
 * Google Maps bildiriminin büyük simgesinden manevra tipini bulur: simge küçültülür,
 * gri tonlu bir imzaya çevrilir ve kayıtlı şablonlarla karşılaştırılır.
 *
 * Şablonlar henüz yok: gerçek cihazda [NavNotificationReader] eşleşmeyen simgeleri
 * `files/maneuver_icons/` altına kaydediyor, oradan toplanıp etiketlenecek
 * (adım 1.4.8). Şablon yokken metinden çıkarılan manevra kullanılır.
 */
class ManeuverMatcher(private val templates: List<Template> = emptyList()) {

    data class Template(val maneuver: Maneuver, val roundaboutExit: Int, val signature: IntArray)

    data class Match(val maneuver: Maneuver, val roundaboutExit: Int, val distance: Int)

    /** En yakın şablon, [maxDistance]'tan uzaksa null. */
    fun match(signature: IntArray, maxDistance: Int = DEFAULT_MAX_DISTANCE): Match? =
        templates
            .map { Match(it.maneuver, it.roundaboutExit, distance(signature, it.signature)) }
            .minByOrNull { it.distance }
            ?.takeIf { it.distance <= maxDistance }

    companion object {
        const val SIZE = 24
        /** Piksel başına ortalama fark (0–255) sınırı. */
        const val DEFAULT_MAX_DISTANCE = 28

        /**
         * ARGB pikselleri [SIZE]×[SIZE] parlaklık imzasına çevirir (alan ortalaması).
         * Simgeler beyaz ok + saydam zemin olduğu için alfa ile çarpılır.
         */
        fun signature(argb: IntArray, width: Int, height: Int): IntArray {
            val out = IntArray(SIZE * SIZE)
            for (sy in 0 until SIZE) for (sx in 0 until SIZE) {
                val x0 = sx * width / SIZE
                val x1 = maxOf(x0 + 1, (sx + 1) * width / SIZE)
                val y0 = sy * height / SIZE
                val y1 = maxOf(y0 + 1, (sy + 1) * height / SIZE)
                var sum = 0L
                var n = 0
                for (y in y0 until y1) for (x in x0 until x1) {
                    val p = argb[y * width + x]
                    val a = (p ushr 24) and 0xFF
                    val lum = (((p shr 16) and 0xFF) * 3 + ((p shr 8) and 0xFF) * 6 + (p and 0xFF)) / 10
                    sum += lum * a / 255
                    n++
                }
                out[sy * SIZE + sx] = (sum / n).toInt()
            }
            return out
        }

        fun distance(a: IntArray, b: IntArray): Int {
            require(a.size == b.size)
            var sum = 0L
            for (i in a.indices) sum += abs(a[i] - b[i])
            return (sum / a.size).toInt()
        }
    }
}
