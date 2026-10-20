package com.nesatalha.revani.ble

import java.util.UUID

/** firmware/src/ble_uuids.h ve docs/PLAN.md bölüm 4 ile aynı. */
object BleUuids {
    val SERVICE: UUID = UUID.fromString("e1510001-07a6-4fc4-b4b4-6d26881dab6c")

    /** Android → ESP32, Write Without Response */
    val RX: UUID = UUID.fromString("e1510002-07a6-4fc4-b4b4-6d26881dab6c")

    /** ESP32 → Android, Notify */
    val TX: UUID = UUID.fromString("e1510003-07a6-4fc4-b4b4-6d26881dab6c")

    val CCCD: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
}
