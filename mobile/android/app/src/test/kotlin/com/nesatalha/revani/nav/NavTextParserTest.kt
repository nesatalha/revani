package com.nesatalha.revani.nav

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/**
 * Örnekler Google Maps bildirim biçimine göre tahmini; gerçek cihazdan toplanan
 * örneklerle (adım 1.6.1) genişletilmeli.
 */
class NavTextParserTest {

    @Test
    fun distances() {
        assertEquals(200, NavTextParser.parseDistanceM("200 m"))
        assertEquals(1200, NavTextParser.parseDistanceM("1,2 km"))
        assertEquals(1500, NavTextParser.parseDistanceM("1.5 km"))
        assertEquals(483, NavTextParser.parseDistanceM("0.3 mi"))
        assertEquals(152, NavTextParser.parseDistanceM("500 ft"))
        assertEquals(50, NavTextParser.parseDistanceM("50 m sonra sağa dönün"))
        assertNull(NavTextParser.parseDistanceM("Bağdat Caddesi"))
        assertNull(NavTextParser.parseDistanceM("12 Mart Sokak")) // "M" harfiyle başlayan kelime birim değil
    }

    @Test
    fun eta() {
        assertEquals(15, NavTextParser.parseEtaMin("15 dk · 4,2 km · Varış 18:45"))
        assertEquals(65, NavTextParser.parseEtaMin("1 sa 5 dk"))
        assertEquals(65, NavTextParser.parseEtaMin("1 hr 5 min"))
        assertNull(NavTextParser.parseEtaMin("Varış 18:45"))
    }

    @Test
    fun turkishNotification() {
        val p = NavTextParser.parse("200 m", "Bağdat Caddesi yönünde sağa dönün", "15 dk · 4,2 km")!!
        assertEquals(200, p.distanceM)
        assertEquals("Bağdat Cd.", p.street)
        assertEquals(Maneuver.RIGHT, p.maneuver)
        assertEquals(15, p.etaMin)
    }

    @Test
    fun distanceInTextField() {
        val p = NavTextParser.parse("Moda Caddesi'ne doğru hafif sola", "1,2 km")!!
        assertEquals(1200, p.distanceM)
        assertEquals("Moda Cd.", p.street)
        assertEquals(Maneuver.SLIGHT_LEFT, p.maneuver)
    }

    @Test
    fun streetOnlyText() {
        val p = NavTextParser.parse("350 m", "Kadıköy İskele Sokağı")!!
        assertEquals("Kadıköy İskele Sk.", p.street)
        assertNull(p.maneuver)
    }

    @Test
    fun instructionWithoutStreet() {
        val p = NavTextParser.parse("80 m", "Sağa dönün")!!
        assertEquals("", p.street)
        assertEquals(Maneuver.RIGHT, p.maneuver)
    }

    @Test
    fun englishNotification() {
        val p = NavTextParser.parse("0.2 mi", "Turn right onto Main Street")!!
        assertEquals(322, p.distanceM)
        assertEquals("Main St.", p.street)
        assertEquals(Maneuver.RIGHT, p.maneuver)
    }

    @Test
    fun roundabout() {
        val tr = NavTextParser.maneuverFromText("Dönel kavşakta 2. çıkışa girin")
        assertEquals(Maneuver.ROUNDABOUT to 2, tr)
        val en = NavTextParser.maneuverFromText("At the roundabout, take the 3rd exit onto A1")
        assertEquals(Maneuver.ROUNDABOUT to 3, en)
    }

    @Test
    fun maneuverKeywords() {
        fun m(s: String) = NavTextParser.maneuverFromText(s).first
        assertEquals(Maneuver.SHARP_RIGHT, m("Keskin sağa dönün"))
        assertEquals(Maneuver.SHARP_LEFT, m("Keskin sola dönün"))
        assertEquals(Maneuver.SLIGHT_RIGHT, m("Hafif sağa dönün"))
        assertEquals(Maneuver.LEFT, m("Sola dönün"))
        assertEquals(Maneuver.U_TURN, m("U dönüşü yapın"))
        assertEquals(Maneuver.ARRIVE, m("Hedefiniz sağda"))
        assertEquals(Maneuver.STRAIGHT, m("Düz devam edin"))
    }

    @Test
    fun noDistanceReturnsNull() {
        assertNull(NavTextParser.parse("Rota yeniden hesaplanıyor", null))
    }
}
