package com.nesatalha.revani.nav

import android.content.Context
import android.graphics.BitmapFactory
import android.util.Log

/**
 * `assets/maneuver_templates/` altındaki PNG şablonlarını yükler.
 * Dosya adı: `<manevra_kodu>_<çıkış_no>_<serbest>.png`, örn. `2_0_sag.png`, `8_2_kavsak.png`.
 * Şablonlar gerçek cihazdan toplanacak (adım 1.4.8); klasör boşken liste boş döner.
 */
object ManeuverTemplates {
    private const val DIR = "maneuver_templates"

    fun load(context: Context): List<ManeuverMatcher.Template> {
        val files = try {
            context.assets.list(DIR).orEmpty()
        } catch (e: Exception) {
            emptyArray()
        }
        return files.filter { it.endsWith(".png") }.mapNotNull { file ->
            val parts = file.removeSuffix(".png").split('_')
            val code = parts.getOrNull(0)?.toIntOrNull()
            val maneuver = Maneuver.entries.firstOrNull { it.code == code } ?: return@mapNotNull null
            val exit = parts.getOrNull(1)?.toIntOrNull() ?: 0
            val bmp = context.assets.open("$DIR/$file").use { BitmapFactory.decodeStream(it) }
                ?: return@mapNotNull null
            val px = IntArray(bmp.width * bmp.height)
            bmp.getPixels(px, 0, bmp.width, 0, 0, bmp.width, bmp.height)
            ManeuverMatcher.Template(maneuver, exit, ManeuverMatcher.signature(px, bmp.width, bmp.height))
        }.also { Log.i("ManeuverTemplates", "${it.size} şablon yüklendi") }
    }
}
