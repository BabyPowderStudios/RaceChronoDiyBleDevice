#if defined(ARDUINO_ARCH_ESP32)

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "RaceChrono.h"

namespace {

class RaceChronoBleAgentESP32
    : public RaceChronoBleAgent, NimBLECharacteristicCallbacks {
public:
  RaceChronoBleAgentESP32();

  void setUp(const char *bluetoothName, RaceChronoBleCanHandler *handler);

  void startAdvertising();

  // Returns true on success, false on failure.
  bool waitForConnection(uint32_t timeoutMs);

  bool isConnected() const;

  void sendCanData(uint32_t pid, const uint8_t *data, uint8_t len);

private:
  void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &connInfo) override;
  void onStatus(NimBLECharacteristic *characteristic, NimBLEConnInfo &connInfo, int code) override;
  void onSubscribe(
      NimBLECharacteristic *characteristic,
      NimBLEConnInfo &connInfo,
      uint16_t subValue) override;

  RaceChronoBleCanHandler *_handler;
  bool _canDataNotificationsEnabled;

  BLEServer *_bleServer;
  BLEService *_bleService;

  NimBLECharacteristic *_pidRequestsCharacteristic;
  NimBLECharacteristic *_canBusDataCharacteristic;
};

RaceChronoBleAgentESP32 RaceChronoESP32Instance;

RaceChronoBleAgentESP32::RaceChronoBleAgentESP32() :
    _handler(nullptr),
  _canDataNotificationsEnabled(false),
    _bleServer(nullptr),
    _bleService(nullptr),
    _pidRequestsCharacteristic(nullptr),
    _canBusDataCharacteristic(nullptr)
{
}

void RaceChronoBleAgentESP32::setUp(
    const char *bluetoothName, RaceChronoBleCanHandler *handler) {
  _handler = handler;

  BLEDevice::init(bluetoothName);
  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->setName(bluetoothName);

  BLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  _bleServer = BLEDevice::createServer();
  _bleService = _bleServer->createService(RACECHRONO_SERVICE_UUID);

  _pidRequestsCharacteristic =
      _bleService->createCharacteristic(
          PID_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
  _pidRequestsCharacteristic->setCallbacks(this);

  _canBusDataCharacteristic =
      _bleService->createCharacteristic(
          CAN_BUS_CHARACTERISTIC_UUID,
          NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _canBusDataCharacteristic->setCallbacks(this);
}

void RaceChronoBleAgentESP32::startAdvertising() {
  _bleServer->start();

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->setMinInterval(32);
  // TODO: Why is this different from the value used for nRF52?
  advertising->setMaxInterval(160);
  advertising->addServiceUUID(_bleService->getUUID());

  advertising->start();
}

bool RaceChronoBleAgentESP32::isConnected() const {
  return _bleServer->getConnectedCount() > 0;
}

void RaceChronoBleAgentESP32::sendCanData(
    uint32_t pid, const uint8_t *data, uint8_t len) {
  if (_canBusDataCharacteristic == nullptr) {
    return;
  }

  if (len > 8) {
    len = 8;
  }

  unsigned char buffer[20];
  buffer[0] = pid & 0xFF;
  buffer[1] = (pid >> 8) & 0xFF;
  buffer[2] = (pid >> 16) & 0xFF;
  buffer[3] = (pid >> 24) & 0xFF;
  memcpy(buffer + 4, data, len);
  if (!_canBusDataCharacteristic->notify(buffer, 4 + len)) {
    Serial.print("RaceChrono BLE notify failed for PID 0x");
    Serial.print(pid, HEX);
    if (!_canDataNotificationsEnabled) {
      Serial.print(" (client has not enabled notifications)");
    }
    Serial.println();
  }
}

void RaceChronoBleAgentESP32::onWrite(
    NimBLECharacteristic *characteristic,
    NimBLEConnInfo &connInfo) {
  (void)connInfo;
  if (characteristic != _pidRequestsCharacteristic || _handler == nullptr) {
    return;
  }

  NimBLEAttValue value = characteristic->getValue();
  _handler->handlePidRequest(value.data(), value.length());
}

void RaceChronoBleAgentESP32::onStatus(
    NimBLECharacteristic *characteristic,
    NimBLEConnInfo &connInfo,
    int code) {
  (void)connInfo;
  if (characteristic != _canBusDataCharacteristic || code == 0) {
    return;
  }

  Serial.print("RaceChrono BLE status code: ");
  Serial.println(code);
}

void RaceChronoBleAgentESP32::onSubscribe(
    NimBLECharacteristic *characteristic,
    NimBLEConnInfo &connInfo,
    uint16_t subValue) {
  (void)connInfo;
  if (characteristic != _canBusDataCharacteristic) {
    return;
  }

  _canDataNotificationsEnabled = (subValue & 0x0001U) != 0;
  Serial.print("RaceChrono data notifications ");
  Serial.println(_canDataNotificationsEnabled ? "enabled." : "disabled.");
}

}  // namespace

RaceChronoBleAgent &RaceChronoBle = RaceChronoESP32Instance;

#endif  // ARDUINO_ARCH_ESP32
