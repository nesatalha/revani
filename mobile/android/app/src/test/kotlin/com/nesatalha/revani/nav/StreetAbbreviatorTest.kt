package com.nesatalha.revani.nav

import org.junit.Assert.assertEquals
import org.junit.Test

class StreetAbbreviatorTest {
    private fun a(s: String) = StreetAbbreviator.abbreviate(s)

    @Test
    fun abbreviations() {
        assertEquals("Bağdat Cd.", a("Bağdat Caddesi"))
        assertEquals("Şair Nefi Sk.", a("Şair Nefi Sokağı"))
        assertEquals("Şair Nefi Sk.", a("Şair Nefi Sokak"))
        assertEquals("Atatürk Bulv.", a("Atatürk Bulvarı"))
        assertEquals("Caferağa Mah.", a("Caferağa Mahallesi"))
        assertEquals("Main St.", a("Main Street"))
    }

    @Test
    fun onlyWholeWords() {
        assertEquals("Caddebostan Sahil Yolu", a("Caddebostan Sahil Yolu"))
        assertEquals("Moda Cd.", a("  Moda   Caddesi "))
    }
}
