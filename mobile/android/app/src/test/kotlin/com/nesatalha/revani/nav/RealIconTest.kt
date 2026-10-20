package com.nesatalha.revani.nav

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

/** Gerçek cihazdan toplanan Google Maps simgeleriyle eşleştirme (assets/maneuver_templates). */
class RealIconTest {
    private val dir = File("src/main/assets/maneuver_templates")

    private fun signature(file: File): IntArray {
        val img = TestPng.read(file)
        return ManeuverMatcher.signature(img.argb, img.width, img.height)
    }

    private val templates = dir.listFiles { f -> f.name.endsWith(".png") }!!.sortedBy { it.name }.map { f ->
        val parts = f.name.split('_')
        f to ManeuverMatcher.Template(
            Maneuver.entries.first { it.code == parts[0].toInt() },
            parts[1].toInt(),
            signature(f),
        )
    }

    @Test
    fun everyTemplateMatchesItself() {
        assertTrue("şablon yok", templates.size >= 4)
        val matcher = ManeuverMatcher(templates.map { it.second })
        for ((file, t) in templates) {
            assertEquals(file.name, t.maneuver, matcher.match(t.signature)?.maneuver)
        }
    }

    @Test
    fun templatesAreFarApart() {
        // Farklı manevralar eşikten belirgin uzak olmalı, yoksa yanlış ok gösterilir.
        for ((i, a) in templates.withIndex()) for (b in templates.drop(i + 1)) {
            val d = ManeuverMatcher.distance(a.second.signature, b.second.signature)
            assertTrue("${a.first.name} ~ ${b.first.name}: $d", d > ManeuverMatcher.DEFAULT_MAX_DISTANCE + 10)
        }
    }
}
