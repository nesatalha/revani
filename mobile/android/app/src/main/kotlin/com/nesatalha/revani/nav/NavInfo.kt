package com.nesatalha.revani.nav

/** Manevra kodları, BLE protokolüyle aynı (docs/PLAN.md bölüm 4). */
enum class Maneuver(val code: Int) {
    STRAIGHT(0),
    SLIGHT_RIGHT(1),
    RIGHT(2),
    SHARP_RIGHT(3),
    U_TURN(4),
    SLIGHT_LEFT(5),
    LEFT(6),
    SHARP_LEFT(7),
    ROUNDABOUT(8),
    ARRIVE(9),
}

data class NavInfo(
    val maneuver: Maneuver,
    /** Dönel kavşakta çıkış numarası, yoksa 0. */
    val roundaboutExit: Int = 0,
    val distanceM: Int,
    /** Varışa kalan dakika, bilinmiyorsa 0. */
    val etaMin: Int = 0,
    val street: String = "",
)
