package com.nesatalha.revani.nav

import java.util.Locale

/**
 * Google Maps navigasyon bildiriminin metinlerini ayrıştırır.
 *
 * Bildirimde genelde başlık mesafe ("200 m", "1,2 km"), metin talimat veya cadde
 * ("Bağdat Cd. yönünde sağa dönün"), alt metin varış bilgisi ("15 dk · 4,2 km")
 * oluyor; ama sürümler arasında yer değiştirebildiği için her alan iki rolde de
 * aranıyor. Gerçek cihazdan toplanan örneklerle (adım 1.6.1) test genişletilmeli.
 */
object NavTextParser {
    data class Parsed(
        val distanceM: Int,
        val street: String,
        /** Metinden çıkarılan manevra; simge eşleştirmesi yoksa yedek olarak kullanılır. */
        val maneuver: Maneuver?,
        val roundaboutExit: Int,
        val etaMin: Int,
    )

    private val tr = Locale.forLanguageTag("tr")

    private val distanceRegex =
        Regex("(\\d+(?:[.,]\\d+)?)\\s*(km|m|mi|ft|yd)(?![\\p{L}])", RegexOption.IGNORE_CASE)

    /** Bildirim alanlarından navigasyon bilgisini çıkarır; mesafe bulunamazsa null. */
    fun parse(title: String?, text: String?, subText: String? = null): Parsed? {
        val fields = listOfNotNull(title, text).map { it.trim() }.filter { it.isNotEmpty() }
        // Önce tamamı mesafe olan alanı seç, yoksa içinde mesafe geçen ilk alanı.
        val distanceField = fields.firstOrNull { isOnlyDistance(it) }
            ?: fields.firstOrNull { parseDistanceM(it) != null }
            ?: return null
        val distanceM = parseDistanceM(distanceField) ?: return null

        val instruction = fields.firstOrNull { it !== distanceField }
            ?: distanceRegex.replace(distanceField, "").trim()
        val (maneuver, exit) = maneuverFromText(instruction)
        return Parsed(
            distanceM = distanceM,
            street = StreetAbbreviator.abbreviate(extractStreet(instruction)),
            maneuver = maneuver,
            roundaboutExit = exit,
            etaMin = subText?.let { parseEtaMin(it) } ?: 0,
        )
    }

    fun isOnlyDistance(s: String): Boolean =
        distanceRegex.matchEntire(s.trim()) != null

    /** "200 m" → 200, "1,2 km" → 1200, "0.3 mi" → 483, "500 ft" → 152 */
    fun parseDistanceM(s: String): Int? {
        val m = distanceRegex.find(s) ?: return null
        val value = m.groupValues[1].replace(',', '.').toDoubleOrNull() ?: return null
        val meters = when (m.groupValues[2].lowercase()) {
            "km" -> value * 1000
            "m" -> value
            "mi" -> value * 1609.344
            "ft" -> value * 0.3048
            "yd" -> value * 0.9144
            else -> return null
        }
        return Math.round(meters).toInt()
    }

    /** "15 dk" → 15, "1 sa 5 dk" → 65, "1 hr 5 min" → 65 */
    fun parseEtaMin(s: String): Int? {
        val lower = s.lowercase(tr)
        val hours = Regex("(\\d+)\\s*(sa|saat|h|hr|hrs|hour|hours)(?![\\p{L}])").find(lower)
            ?.groupValues?.get(1)?.toInt()
        val mins = Regex("(\\d+)\\s*(dk|dakika|min|mins)(?![\\p{L}])").find(lower)
            ?.groupValues?.get(1)?.toInt()
        if (hours == null && mins == null) return null
        return (hours ?: 0) * 60 + (mins ?: 0)
    }

    private val streetPatterns = listOf(
        // Türkçe
        Regex("^(.+?)\\s+(?:yönünde|istikametinde|üzerinde|üzerinden|tarafına)\\b.*$", RegexOption.IGNORE_CASE),
        Regex("^(.+?)['’]?(?:[ae]|[yn][ae])\\s+(?:doğru|girin|çıkın|dönün|sapın)\\b.*$", RegexOption.IGNORE_CASE),
        Regex("^.*?\\b(?:sağa|sola)\\s+dönerek\\s+(.+?)['’]?(?:[ae]|[yn][ae])?\\s+(?:girin|gidin)$", RegexOption.IGNORE_CASE),
        // İngilizce
        Regex("^.*?\\b(?:onto|toward|towards|on)\\s+(.+)$", RegexOption.IGNORE_CASE),
    )

    /** Talimattan cadde adını ayıklar; kalıp tutmazsa metni olduğu gibi döner. */
    fun extractStreet(instruction: String): String {
        val text = instruction.trim()
        for (p in streetPatterns) {
            val m = p.matchEntire(text) ?: continue
            val street = m.groupValues[1].trim()
            if (street.isNotEmpty() && !looksLikeInstruction(street)) return street
        }
        return if (looksLikeInstruction(text)) "" else text
    }

    private val instructionWords = Regex(
        "(?<![\\p{L}])(dönün|dön|devam|edin|sapın|girin|çıkın|sağ|sol|hafif|keskin|" +
            "turn|continue|head|keep|merge|take|exit|right|left)(?![\\p{L}])",
        RegexOption.IGNORE_CASE,
    )

    private fun looksLikeInstruction(s: String) = instructionWords.containsMatchIn(s)

    /** Talimat metninden manevra (simge eşleşmezse yedek). */
    fun maneuverFromText(instruction: String): Pair<Maneuver?, Int> {
        val s = instruction.lowercase(tr)
        fun has(vararg words: String) = words.any { s.contains(it) }

        if (has("kavşak", "roundabout", "rotary")) {
            val exit = Regex("(\\d+)\\s*\\.?\\s*(?:çıkış|st|nd|rd|th)").find(s)
                ?.groupValues?.get(1)?.toIntOrNull() ?: 0
            if (has("dönel", "döner", "göbek", "roundabout", "rotary") || exit > 0) {
                return Maneuver.ROUNDABOUT to exit
            }
        }
        val m = when {
            has("u dönüşü", "u-dönüşü", "u-turn", "u turn", "geri dön") -> Maneuver.U_TURN
            has("keskin sağ", "sharp right") -> Maneuver.SHARP_RIGHT
            has("keskin sol", "sharp left") -> Maneuver.SHARP_LEFT
            has("hafif sağ", "sağdan devam", "slight right", "keep right", "bear right") -> Maneuver.SLIGHT_RIGHT
            has("hafif sol", "soldan devam", "slight left", "keep left", "bear left") -> Maneuver.SLIGHT_LEFT
            has("sağa", "turn right", "right onto") -> Maneuver.RIGHT
            has("sola", "turn left", "left onto") -> Maneuver.LEFT
            has("varış", "hedef", "ulaşacaksınız", "arrive", "destination") -> Maneuver.ARRIVE
            has("düz", "devam", "continue", "head ", "straight") -> Maneuver.STRAIGHT
            else -> null
        }
        return m to 0
    }
}
