package com.nesatalha.revani.nav

/**
 * Cadde adını ekrana sığsın diye kısaltır (ekranda tek satır, ~290 px).
 * "Bağdat Caddesi" → "Bağdat Cd."
 */
object StreetAbbreviator {
    private val rules = listOf(
        "Caddesi" to "Cd.",
        "Cadde" to "Cd.",
        "Sokağı" to "Sk.",
        "Sokak" to "Sk.",
        "Bulvarı" to "Bulv.",
        "Bulvar" to "Bulv.",
        "Mahallesi" to "Mah.",
        "Street" to "St.",
        "Avenue" to "Ave.",
        "Boulevard" to "Blvd.",
    ).map { (word, short) -> Regex("(?<!\\p{L})${word}(?!\\p{L})", RegexOption.IGNORE_CASE) to short }

    fun abbreviate(street: String): String =
        rules.fold(street.trim()) { s, (regex, short) -> regex.replace(s, short) }
            .replace(Regex("\\s+"), " ")
}
