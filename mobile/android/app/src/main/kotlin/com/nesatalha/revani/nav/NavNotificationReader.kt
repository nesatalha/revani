package com.nesatalha.revani.nav

import android.app.Notification
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.Icon
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import com.nesatalha.revani.RevaniState
import java.io.File

/**
 * Google Maps navigasyon bildirimini dinler ve [RevaniState.nav]'ı günceller.
 * Bildirim kaybolunca navigasyon biter (NAV_END, adım 1.4.9).
 */
class NavNotificationReader : NotificationListenerService() {

    private var navKey: String? = null
    private val matcher by lazy { ManeuverMatcher(ManeuverTemplates.load(this)) }

    override fun onListenerConnected() {
        activeNotifications?.forEach { onNotificationPosted(it) }
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        if (!isNavigation(sbn)) return
        val n = sbn.notification
        val extras = n.extras
        val title = extras.getCharSequence(Notification.EXTRA_TITLE)?.toString()
        val text = extras.getCharSequence(Notification.EXTRA_TEXT)?.toString()
        val sub = extras.getCharSequence(Notification.EXTRA_SUB_TEXT)?.toString()

        val parsed = NavTextParser.parse(title, text, sub)
        if (parsed == null) {
            RevaniState.error("Bildirim ayrıştırılamadı: '$title' / '$text'")
            return
        }
        navKey = sbn.key

        val signature = n.getLargeIcon()?.let { iconSignature(it) }
        val match = signature?.let { matcher.match(it) }
        if (signature != null && match == null) saveUnknownIcon(signature, n.getLargeIcon(), title, text)

        RevaniState.nav.value = NavInfo(
            maneuver = match?.maneuver ?: parsed.maneuver ?: Maneuver.STRAIGHT,
            roundaboutExit = match?.roundaboutExit?.takeIf { it > 0 } ?: parsed.roundaboutExit,
            distanceM = parsed.distanceM,
            etaMin = parsed.etaMin,
            street = parsed.street,
        )
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        if (sbn.key == navKey) {
            navKey = null
            RevaniState.nav.value = null
        }
    }

    override fun onListenerDisconnected() {
        navKey = null
        RevaniState.nav.value = null
    }

    private fun isNavigation(sbn: StatusBarNotification): Boolean {
        if (sbn.packageName != MAPS_PACKAGE) return false
        val n = sbn.notification
        return sbn.isOngoing && (n.category == Notification.CATEGORY_NAVIGATION ||
            n.channelId?.contains("navigation", ignoreCase = true) == true)
    }

    private fun iconBitmap(icon: Icon): Bitmap? {
        val d = icon.loadDrawable(this) ?: return null
        val w = d.intrinsicWidth.coerceIn(1, 256)
        val h = d.intrinsicHeight.coerceIn(1, 256)
        val bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        d.setBounds(0, 0, w, h)
        d.draw(Canvas(bmp))
        return bmp
    }

    private fun iconSignature(icon: Icon): IntArray? {
        val bmp = iconBitmap(icon) ?: return null
        val px = IntArray(bmp.width * bmp.height)
        bmp.getPixels(px, 0, bmp.width, 0, 0, bmp.width, bmp.height)
        return ManeuverMatcher.signature(px, bmp.width, bmp.height)
    }

    /** Şablon toplamak için tanınmayan simgeleri ve metnini saklar (adım 1.4.8). */
    private fun saveUnknownIcon(signature: IntArray, icon: Icon?, title: String?, text: String?) {
        try {
            val dir = File(filesDir, UNKNOWN_ICON_DIR).apply { mkdirs() }
            val name = "%08x".format(signature.contentHashCode())
            val png = File(dir, "$name.png")
            if (png.exists() || (dir.list()?.size ?: 0) >= MAX_SAVED_FILES) return
            val bmp = icon?.let { iconBitmap(it) } ?: return
            png.outputStream().use { bmp.compress(Bitmap.CompressFormat.PNG, 100, it) }
            File(dir, "$name.txt").writeText("$title\n$text\n")
        } catch (e: Exception) {
            Log.w(TAG, "Simge kaydedilemedi", e)
        }
    }

    companion object {
        private const val TAG = "NavReader"
        const val MAPS_PACKAGE = "com.google.android.apps.maps"
        const val UNKNOWN_ICON_DIR = "maneuver_icons"
        private const val MAX_SAVED_FILES = 400
    }
}
