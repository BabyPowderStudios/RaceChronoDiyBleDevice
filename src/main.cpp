#include <Arduino.h>
#include <RaceChrono.h>

#include <ESP32CAN.h>
#include <CAN_config.h>
#include <freertos/queue.h>

#include <string.h>

extern "C" int ets_printf(const char *fmt, ...);

#include "CanBusSupport.h"
#include "CanTracker.h"
#include "SettingsStore.h"
#include "WebUi.h"

CAN_device_t CAN_cfg;

#if !defined(CAN_TX_PIN)
#define CAN_TX_PIN 25
#endif

#if !defined(CAN_RX_PIN)
#define CAN_RX_PIN 26
#endif

#if !defined(CAN_RX_RAW_LOG_ENABLED)
#define CAN_RX_RAW_LOG_ENABLED 1
#endif

namespace {

constexpr char kDeviceName[] = "BLE CAN device demo";
constexpr gpio_num_t kCanTxPin = static_cast<gpio_num_t>(CAN_TX_PIN);
constexpr gpio_num_t kCanRxPin = static_cast<gpio_num_t>(CAN_RX_PIN);
constexpr uint32_t kCanRestartDelayMs = 1000;
constexpr uint32_t kCanRxQueueLength = 10;
constexpr uint32_t kCanTimeoutReportIntervalMs = 2000;
constexpr size_t kNumBuffers = 64;

enum class StartupPhase {
  kSettings,
  kBle,
  kWebUi,
  kReady,
};

struct PidExtra {
  uint16_t sendDivider = 1;
  uint16_t skippedUpdates = 0;
};

struct BufferedMessage {
  uint32_t pid = 0;
  uint8_t data[8] = {0};
  uint8_t length = 0;
};

SettingsStore *settingsStore = nullptr;
CanTracker *canTracker = nullptr;
WebUiServer *webUi = nullptr;
RaceChronoPidMap<PidExtra, kMaxObservedCanIds> pidMap;
StartupPhase startupPhase = StartupPhase::kSettings;
bool isCanDriverInstalled = false;
bool isCanBusReaderActive = false;
bool isRaceChronoConnected = false;
uint32_t lastCanMessageReceivedMs = 0;
uint32_t lastTimeNumCanBusTimeoutsSentMs = 0;
uint16_t numCanBusTimeouts = 0;
bool restartCanRequested = false;
uint32_t nextCanStartAttemptMs = 0;
BufferedMessage buffers[kNumBuffers];
uint32_t bufferToWriteTo = 0;
uint32_t bufferToReadFrom = 0;

void bufferNewPacket(uint32_t pid, const uint8_t *data, uint8_t dataLength);
void handleOneBufferedPacket();
void flushBufferedPackets();
void sendNumCanBusTimeouts();
void resetSkippedUpdatesCounters();
void pollRaceChronoConnection();
uint16_t computeEffectiveDivider(uint32_t pid, uint16_t updateIntervalMs, uint32_t nowMs);
void restoreDefaultPidForwarding();

void dumpMapToSerial() {
  uint16_t updateIntervalForAllEntries;
  bool areAllPidsAllowed = pidMap.areAllPidsAllowed(&updateIntervalForAllEntries);
  if (areAllPidsAllowed) {
    Serial.print("  All PIDs are allowed, requested interval: ");
    Serial.print(updateIntervalForAllEntries);
    Serial.println(" ms.");
  }

  if (pidMap.isEmpty()) {
    if (areAllPidsAllowed) {
      Serial.println("  Map is empty.");
    } else {
      Serial.println("  No PIDs are allowed.");
    }
    Serial.println("");
    return;
  }

  struct {
    void operator()(void *entry) {
      uint32_t pid = pidMap.getPid(entry);
      uint16_t updateIntervalMs = pidMap.getUpdateIntervalMs(entry);
      const PidExtra *extra = pidMap.getExtra(entry);

      Serial.print("  ");
      Serial.print(pid);
      Serial.print(" (0x");
      Serial.print(pid, HEX);
      Serial.print("), requested interval ");
      Serial.print(updateIntervalMs);
      Serial.print(" ms, effective divider ");
      Serial.println(extra->sendDivider);
    }
  } dumpEntry;
  pidMap.forEach(dumpEntry);

  Serial.println("");
}

void logReceivedCanFrame(
    uint32_t pid,
    const uint8_t *data,
    uint8_t dataLength,
    bool isExtended) {
#if CAN_RX_RAW_LOG_ENABLED
  Serial.print("CAN RX ");
  Serial.print(isExtended ? "EXT " : "STD ");
  Serial.print("0x");
  Serial.print(pid, HEX);
  Serial.print(" [");
  Serial.print(dataLength);
  Serial.print("] ");

  for (uint8_t index = 0; index < dataLength; ++index) {
    if (index > 0) {
      Serial.print(' ');
    }
    if (data[index] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[index], HEX);
  }
  Serial.println();
#else
  (void)pid;
  (void)data;
  (void)dataLength;
  (void)isExtended;
#endif
}

class UpdateMapOnRaceChronoCommands : public RaceChronoBleCanHandler {
public:
  void allowAllPids(uint16_t updateIntervalMs) {
    Serial.print("Command: ALLOW ALL PIDS, update interval: ");
    Serial.print(updateIntervalMs);
    Serial.println(" ms.");

    pidMap.allowAllPids(updateIntervalMs);
    dumpMapToSerial();
  }

  void denyAllPids() {
    Serial.println("Command: DENY ALL PIDS. Restoring default allow-all forwarding.");

    restoreDefaultPidForwarding();
    dumpMapToSerial();
  }

  void allowPid(uint32_t pid, uint16_t updateIntervalMs) {
    Serial.print("Command: ALLOW PID ");
    Serial.print(pid);
    Serial.print(" (0x");
    Serial.print(pid, HEX);
    Serial.print("), requested update interval: ");
    Serial.print(updateIntervalMs);
    Serial.println(" ms.");

    if (!pidMap.allowOnePid(pid, updateIntervalMs)) {
      Serial.println("WARNING: unable to handle this request!");
      return;
    }

    void *entry = pidMap.getEntryId(pid);
    if (entry != nullptr) {
      PidExtra *extra = pidMap.getExtra(entry);
      extra->sendDivider = 1;
      extra->skippedUpdates = 0;
    }

    dumpMapToSerial();
  }

  void handleDisconnect() {
    Serial.println("Restoring default allow-all forwarding.");

    restoreDefaultPidForwarding();
    dumpMapToSerial();
  }
} raceChronoHandler;

void restoreDefaultPidForwarding() {
  pidMap.reset();
  pidMap.allowAllPids(0);
}

bool startCanBusReader() {
  Serial.println("Connecting to the CAN bus...");

  if (settingsStore == nullptr) {
    Serial.println("ERROR: CAN reader dependencies are not initialized yet.");
    return false;
  }

  long baudRate = settingsStore->getCanBaudRate();
  CAN_speed_t canSpeed;
  if (!tryGetCanSpeed(baudRate, &canSpeed)) {
    Serial.print("ERROR: Unsupported CAN baud rate: ");
    Serial.println(baudRate);
    return false;
  }

  if (isCanDriverInstalled) {
    if (ESP32Can.CANStop() != 0) {
      Serial.println("ERROR: Unable to stop the CAN controller.");
      return false;
    }

    isCanDriverInstalled = false;
  }

  CAN_cfg.speed = canSpeed;
  CAN_cfg.tx_pin_id = kCanTxPin;
  CAN_cfg.rx_pin_id = kCanRxPin;
  if (CAN_cfg.rx_queue == nullptr) {
    CAN_cfg.rx_queue = xQueueCreate(kCanRxQueueLength, sizeof(CAN_frame_t));
  }
  if (CAN_cfg.rx_queue == nullptr) {
    Serial.println("ERROR: Unable to allocate the CAN RX queue.");
    return false;
  }
  xQueueReset(CAN_cfg.rx_queue);

  if (ESP32Can.CANInit() != 0) {
    Serial.println("ERROR: Unable to initialize the CAN controller.");
    return false;
  }

  Serial.print("CAN configured on TX GPIO ");
  Serial.print(static_cast<int>(kCanTxPin));
  Serial.print(", RX GPIO ");
  Serial.print(static_cast<int>(kCanRxPin));
  Serial.print(", baud ");
  Serial.println(baudRate);

  Serial.println("Success!");
  isCanDriverInstalled = true;
  isCanBusReaderActive = true;
  return true;
}

void stopCanBusReader() {
  if (isCanDriverInstalled) {
    if (ESP32Can.CANStop() != 0) {
      Serial.println("WARNING: Failed to stop the CAN controller.");
    }

    isCanDriverInstalled = false;
  }
  isCanBusReaderActive = false;
}

void pollRaceChronoConnection() {
  bool nowConnected = RaceChronoBle.isConnected();
  if (nowConnected == isRaceChronoConnected) {
    return;
  }

  isRaceChronoConnected = nowConnected;
  if (isRaceChronoConnected) {
    Serial.println("RaceChrono connected.");
    sendNumCanBusTimeouts();
    return;
  }

  Serial.println("RaceChrono disconnected!");
  raceChronoHandler.handleDisconnect();
  RaceChronoBle.startAdvertising();
}

void attemptCanBusStart(uint32_t nowMs) {
  if (restartCanRequested) {
    restartCanRequested = false;
    stopCanBusReader();
    nextCanStartAttemptMs = 0;
  }

  if (isCanBusReaderActive || nowMs < nextCanStartAttemptMs) {
    return;
  }

  if (!startCanBusReader()) {
    nextCanStartAttemptMs = nowMs + kCanRestartDelayMs;
    return;
  }

  flushBufferedPackets();
  resetSkippedUpdatesCounters();
  lastCanMessageReceivedMs = 0;
}

void pollCanFrames(uint32_t nowMs) {
  if (!isCanBusReaderActive || canTracker == nullptr) {
    return;
  }

  CAN_frame_t frame;
  while (xQueueReceive(CAN_cfg.rx_queue, &frame, 0) == pdTRUE) {
    if (frame.FIR.B.RTR == CAN_RTR) {
      continue;
    }

    uint32_t pid = frame.MsgID;
    bool isExtended = frame.FIR.B.FF == CAN_frame_ext;
    uint8_t frameLength = frame.FIR.B.DLC;
    if (frameLength == 0) {
      continue;
    }
    if (frameLength > 8) {
      frameLength = 8;
    }

    uint32_t frameTimeMs = millis();
    logReceivedCanFrame(pid, frame.data.u8, frameLength, isExtended);
    canTracker->observe(pid, frame.data.u8, frameLength, frameTimeMs);
    bufferNewPacket(pid, frame.data.u8, frameLength);
    lastCanMessageReceivedMs = frameTimeMs;
  }
}

uint16_t computeEffectiveDivider(uint32_t pid, uint16_t updateIntervalMs, uint32_t nowMs) {
  if (settingsStore == nullptr || canTracker == nullptr) {
    return 1;
  }

  const CanRule *rule = settingsStore->findRule(pid);
  if (rule != nullptr && rule->ignored) {
    return 0;
  }

  uint32_t observedFrequencyMilliHz = canTracker->getCurrentFrequencyMilliHz(pid, nowMs);
  if (observedFrequencyMilliHz == 0) {
    return 1;
  }

  uint32_t maxOutputFrequencyMilliHz = 0;
  if (updateIntervalMs > 0) {
    maxOutputFrequencyMilliHz = 1000000UL / updateIntervalMs;
  }

  if (rule != nullptr && rule->rateLimitHz > 0) {
    uint32_t uiLimitMilliHz = static_cast<uint32_t>(rule->rateLimitHz) * 1000UL;
    if (maxOutputFrequencyMilliHz == 0 || uiLimitMilliHz < maxOutputFrequencyMilliHz) {
      maxOutputFrequencyMilliHz = uiLimitMilliHz;
    }
  }

  if (maxOutputFrequencyMilliHz == 0) {
    return 1;
  }

  uint32_t divider =
      (observedFrequencyMilliHz + maxOutputFrequencyMilliHz - 1)
      / maxOutputFrequencyMilliHz;
  if (divider == 0) {
    return 1;
  }

  return divider > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(divider);
}

void runStartupStep() {
  switch (startupPhase) {
    case StartupPhase::kSettings:
      Serial.println("Startup: initializing settings storage...");
      Serial.flush();
      if (settingsStore == nullptr) {
        settingsStore = new SettingsStore();
      }
      if (canTracker == nullptr) {
        canTracker = new CanTracker();
      }

      if (settingsStore == nullptr || canTracker == nullptr) {
        Serial.println("ERROR: failed to allocate runtime services.");
        Serial.flush();
        return;
      }

      if (!settingsStore->begin()) {
        Serial.println("WARNING: settings storage initialization failed, continuing with defaults.");
      } else {
        Serial.println("Startup: settings storage ready.");
      }
      restoreDefaultPidForwarding();
      Serial.flush();
      startupPhase = StartupPhase::kBle;
      return;

    case StartupPhase::kBle:
      Serial.println("Startup: initializing BLE...");
      Serial.flush();
      RaceChronoBle.setUp(kDeviceName, &raceChronoHandler);
      RaceChronoBle.startAdvertising();
      Serial.println("Startup: BLE ready.");
      Serial.flush();
      startupPhase = StartupPhase::kWebUi;
      return;

    case StartupPhase::kWebUi:
      Serial.println("Startup: initializing access point and web UI...");
      Serial.flush();
      if (webUi == nullptr) {
        webUi = new WebUiServer(*settingsStore, *canTracker, &restartCanRequested);
      }
      if (webUi == nullptr) {
        Serial.println("ERROR: failed to allocate Web UI service.");
        Serial.flush();
        return;
      }
      webUi->begin();
      Serial.println("Startup: web UI ready.");
      Serial.flush();
      startupPhase = StartupPhase::kReady;
      return;

    case StartupPhase::kReady:
      return;
  }
}

}  // namespace

void setup() {
  ets_printf("App setup reached.\r\n");

  uint32_t startTimeMs = millis();
  Serial.begin(115200);
  while (!Serial && millis() - startTimeMs < 5000) {
  }

  delay(50);
  Serial.println("Booting firmware...");
  Serial.flush();
}

void loop() {
  if (startupPhase != StartupPhase::kReady) {
    runStartupStep();
    delay(1);
    return;
  }

  if (webUi != nullptr) {
    webUi->handleClient();
  }
  pollRaceChronoConnection();

  uint32_t nowMs = millis();
  attemptCanBusStart(nowMs);
  pollCanFrames(nowMs);
  handleOneBufferedPacket();

  if (nowMs - lastTimeNumCanBusTimeoutsSentMs > kCanTimeoutReportIntervalMs) {
    sendNumCanBusTimeouts();
  }
}

namespace {

void bufferNewPacket(uint32_t pid, const uint8_t *data, uint8_t dataLength) {
  if (bufferToWriteTo - bufferToReadFrom == kNumBuffers) {
    Serial.println("WARNING: Receive buffer overflow, dropping one message.");

    // In case of a buffer overflow, drop the oldest message in the buffer, as
    // it's likely less useful than the newest one.
    bufferToReadFrom++;
  }

  BufferedMessage *message = &buffers[bufferToWriteTo % kNumBuffers];
  message->pid = pid;
  memset(message->data, 0, sizeof(message->data));
  memcpy(message->data, data, dataLength);
  message->length = dataLength;
  bufferToWriteTo++;
}

void handleOneBufferedPacket() {
  if (bufferToReadFrom == bufferToWriteTo) {
    return;
  }

  BufferedMessage *message = &buffers[bufferToReadFrom % kNumBuffers];
  void *entry = pidMap.getEntryId(message->pid);
  if (entry != nullptr && isRaceChronoConnected) {
    PidExtra *extra = pidMap.getExtra(entry);
    extra->sendDivider = computeEffectiveDivider(
        message->pid,
        pidMap.getUpdateIntervalMs(entry),
        millis());

    if (extra->sendDivider > 0) {
      if (extra->skippedUpdates == 0) {
        RaceChronoBle.sendCanData(message->pid, message->data, message->length);
      }

      extra->skippedUpdates++;
      if (extra->skippedUpdates >= extra->sendDivider) {
        extra->skippedUpdates = 0;
      }
    }
  }

  bufferToReadFrom++;
}

void flushBufferedPackets() {
  bufferToWriteTo = 0;
  bufferToReadFrom = 0;
}

void sendNumCanBusTimeouts() {
  lastTimeNumCanBusTimeoutsSentMs = millis();
  if (!isRaceChronoConnected) {
    return;
  }

  uint8_t data[2];
  data[0] = numCanBusTimeouts & 0xff;
  data[1] = numCanBusTimeouts >> 8;
  RaceChronoBle.sendCanData(0x777, data, 2);
}

void resetSkippedUpdatesCounters() {
  struct {
    void operator()(void *entry) {
      PidExtra *extra = pidMap.getExtra(entry);
      extra->sendDivider = 1;
      extra->skippedUpdates = 0;
    }
  } resetSkippedUpdatesCounter;
  pidMap.forEach(resetSkippedUpdatesCounter);
}

}  // namespace