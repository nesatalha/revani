package com.nesatalha.revani.nav

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class ManeuverMatcherTest {
    private val w = 48

    /** Saydam zemin üstünde beyaz dikdörtgen. */
    private fun shape(x0: Int, y0: Int, x1: Int, y1: Int): IntArray =
        IntArray(w * w) { i ->
            val x = i % w
            val y = i / w
            if (x in x0 until x1 && y in y0 until y1) 0xFFFFFFFF.toInt() else 0
        }

    private val vertical = shape(20, 4, 28, 44)
    private val horizontal = shape(4, 20, 44, 28)

    @Test
    fun matchesClosestTemplate() {
        val matcher = ManeuverMatcher(
            listOf(
                ManeuverMatcher.Template(Maneuver.STRAIGHT, 0, ManeuverMatcher.signature(vertical, w, w)),
                ManeuverMatcher.Template(Maneuver.RIGHT, 0, ManeuverMatcher.signature(horizontal, w, w)),
            ),
        )
        val slightlyOff = shape(21, 5, 29, 44)
        assertEquals(Maneuver.STRAIGHT, matcher.match(ManeuverMatcher.signature(slightlyOff, w, w))?.maneuver)
        assertEquals(Maneuver.RIGHT, matcher.match(ManeuverMatcher.signature(horizontal, w, w))?.maneuver)
    }

    @Test
    fun noTemplatesNoMatch() {
        assertNull(ManeuverMatcher().match(ManeuverMatcher.signature(vertical, w, w)))
    }

    @Test
    fun farShapeIsRejected() {
        val matcher = ManeuverMatcher(
            listOf(ManeuverMatcher.Template(Maneuver.STRAIGHT, 0, ManeuverMatcher.signature(vertical, w, w))),
        )
        val full = shape(0, 0, w, w)
        assertNull(matcher.match(ManeuverMatcher.signature(full, w, w)))
    }
}
