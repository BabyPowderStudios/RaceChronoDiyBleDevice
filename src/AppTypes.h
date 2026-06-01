#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <Arduino.h>

constexpr size_t kMaxCanRules = 256;
constexpr size_t kMaxObservedCanIds = 256;
constexpr size_t kMaxDbcMessages = 256;
constexpr size_t kMaxDbcFileNameLength = 64;
constexpr size_t kMaxDbcNameLength = 48;
constexpr size_t kMaxDbcSignalUnitLength = 24;
constexpr size_t kMaxDbcSignalsPerMessage = 64;
constexpr long kDefaultCanBaudRate = 500000L;

struct CanRule {
  uint32_t canId = 0;
  bool ignored = false;
  uint16_t rateLimitHz = 0;
};

struct ObservedCanId {
  uint32_t canId = 0;
  uint8_t lastData[8] = {0};
  uint8_t lastLength = 0;
  uint32_t lastSeenMs = 0;
  uint32_t lastRateSampleMs = 0;
  uint32_t rateSampleCount = 0;
  uint32_t frequencyMilliHz = 0;
  uint32_t totalCount = 0;
};

struct DbcMessageName {
  uint32_t canId = 0;
  char name[kMaxDbcNameLength] = {0};
};

struct DbcDecodedSignal {
  char name[kMaxDbcNameLength] = {0};
  char unit[kMaxDbcSignalUnitLength] = {0};
  uint16_t startBit = 0;
  uint16_t bitLength = 0;
  bool isLittleEndian = true;
  bool isSigned = false;
  double rawValue = 0.0;
  double physicalValue = 0.0;
};

#endif