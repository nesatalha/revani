#include "ble_link.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/queue.h>

#include "ble_uuids.h"

static QueueHandle_t queue;
static NimBLEServer *server;
static NimBLECharacteristic *tx;

static void push(BleEventKind kind, const uint8_t *data = nullptr, size_t len = 0) {
  static BleEvent ev;  // yalnız NimBLE görevinden çağrılıyor
  ev.kind = kind;
  ev.len = (uint16_t)(len > sizeof(ev.data) ? sizeof(ev.data) : len);
  if (ev.len) memcpy(ev.data, data, ev.len);
  if (xQueueSend(queue, &ev, 0) != pdTRUE) Serial.println("BLE kuyrugu dolu, mesaj atildi");
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *s, NimBLEConnInfo &info) override {
    // Navigasyon için düşük gecikme: 15–30 ms bağlantı aralığı
    s->updateConnParams(info.getConnHandle(), 12, 24, 0, 400);
    push(BleEventKind::Connected);
  }

  void onDisconnect(NimBLEServer *s, NimBLEConnInfo &info, int reason) override {
    Serial.printf("BLE koptu, sebep %d\n", reason);
    push(BleEventKind::Disconnected);
    // advertiseOnDisconnect açık: yayın kendiliğinden yeniden başlar (1.3.5)
  }

  void onMTUChange(uint16_t mtu, NimBLEConnInfo &info) override { Serial.printf("MTU %u\n", mtu); }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &info) override {
    NimBLEAttValue v = c->getValue();
    push(BleEventKind::Data, v.data(), v.length());
  }
};

class TxCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic *c, NimBLEConnInfo &info, uint16_t sub_value) override {
    if (sub_value & 0x0001) push(BleEventKind::Subscribed);
  }
};

void ble_link_begin(const char *name) {
  // Bir harita karesi (~20–30 parça) ana döngü işleyene kadar sığsın.
  queue = xQueueCreate(48, sizeof(BleEvent));

  NimBLEDevice::init(name);
  NimBLEDevice::setMTU(517);
  NimBLEDevice::setPower(9);  // dBm; cep içinden erişim için en yüksek

  server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  server->advertiseOnDisconnect(true);

  NimBLEService *service = server->createService(REVANI_BLE_SERVICE_UUID);
  NimBLECharacteristic *rx = service->createCharacteristic(
      REVANI_BLE_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx->setCallbacks(new RxCallbacks());
  tx = service->createCharacteristic(REVANI_BLE_TX_UUID, NIMBLE_PROPERTY::NOTIFY);
  tx->setCallbacks(new TxCallbacks());

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(REVANI_BLE_SERVICE_UUID);
  adv->setName(name);
  adv->start();
}

bool ble_link_poll(BleEvent *ev) { return xQueueReceive(queue, ev, 0) == pdTRUE; }

bool ble_link_connected() { return server && server->getConnectedCount() > 0; }

void ble_link_notify(const uint8_t *data, size_t len) {
  if (!ble_link_connected()) return;
  tx->setValue(data, len);
  tx->notify();
}
