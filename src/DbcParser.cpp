#include "DbcParser.h"

#include <LittleFS.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

constexpr size_t kLineBufferSize = 256;

void setError(String *errorOut, const char *message) {
  if (errorOut != nullptr) {
    *errorOut = message;
  }
}

void trimCarriageReturn(char *line) {
  size_t length = strlen(line);
  while (length > 0 && (line[length - 1] == '\r' || line[length - 1] == '\n')) {
    line[length - 1] = '\0';
    --length;
  }
}

void copyString(char *destination, size_t destinationSize, const char *source) {
  if (destinationSize == 0) {
    return;
  }

  if (source == nullptr) {
    destination[0] = '\0';
    return;
  }

  snprintf(destination, destinationSize, "%s", source);
}

uint32_t normalizeCanId(uint32_t canId) {
  if ((canId & 0x80000000UL) != 0) {
    return canId & 0x1FFFFFFFUL;
  }
  return canId;
}

bool parseMessageHeader(
    char *line,
    uint32_t *canIdOut,
    char *nameOut,
    size_t nameOutSize) {
  if (strncmp(line, "BO_ ", 4) != 0) {
    return false;
  }

  char *cursor = line + 4;
  while (*cursor == ' ') {
    ++cursor;
  }

  char *idEnd = nullptr;
  unsigned long rawId = strtoul(cursor, &idEnd, 10);
  if (idEnd == cursor) {
    return false;
  }

  while (*idEnd == ' ') {
    ++idEnd;
  }

  char *nameEnd = strchr(idEnd, ':');
  if (nameEnd == nullptr || *idEnd == '\0') {
    return false;
  }

  *nameEnd = '\0';
  if (canIdOut != nullptr) {
    *canIdOut = normalizeCanId(static_cast<uint32_t>(rawId));
  }
  if (nameOut != nullptr) {
    copyString(nameOut, nameOutSize, idEnd);
  }
  return true;
}

bool parseSignalDefinition(char *line, DbcDecodedSignal *signalOut) {
  char *cursor = line;
  if (strncmp(cursor, "SG_ ", 4) == 0) {
    cursor += 4;
  } else if (strncmp(cursor, " SG_ ", 5) == 0) {
    cursor += 5;
  } else {
    return false;
  }

  while (*cursor == ' ') {
    ++cursor;
  }

  char *nameStart = cursor;
  while (*cursor != '\0' && *cursor != ' ' && *cursor != ':') {
    ++cursor;
  }

  char savedCharacter = *cursor;
  *cursor = '\0';

  DbcDecodedSignal signal;
  copyString(signal.name, sizeof(signal.name), nameStart);

  *cursor = savedCharacter;
  while (*cursor != '\0' && *cursor != ':') {
    ++cursor;
  }
  if (*cursor != ':') {
    return false;
  }

  ++cursor;
  while (*cursor == ' ') {
    ++cursor;
  }

  char *parseEnd = nullptr;
  unsigned long startBit = strtoul(cursor, &parseEnd, 10);
  if (parseEnd == cursor || *parseEnd != '|') {
    return false;
  }
  cursor = parseEnd + 1;

  unsigned long bitLength = strtoul(cursor, &parseEnd, 10);
  if (parseEnd == cursor || *parseEnd != '@') {
    return false;
  }
  cursor = parseEnd + 1;

  if (*cursor != '0' && *cursor != '1') {
    return false;
  }
  signal.isLittleEndian = *cursor == '1';
  ++cursor;

  if (*cursor != '+' && *cursor != '-') {
    return false;
  }
  signal.isSigned = *cursor == '-';
  ++cursor;

  while (*cursor == ' ') {
    ++cursor;
  }
  if (*cursor != '(') {
    return false;
  }
  ++cursor;

  signal.startBit = static_cast<uint16_t>(startBit);
  signal.bitLength = static_cast<uint16_t>(bitLength);
  signal.rawValue = 0.0;
  signal.physicalValue = strtod(cursor, &parseEnd);
  if (parseEnd == cursor || *parseEnd != ',') {
    return false;
  }
  double factor = signal.physicalValue;
  cursor = parseEnd + 1;
  double offset = strtod(cursor, &parseEnd);
  if (parseEnd == cursor || *parseEnd != ')') {
    return false;
  }

  signal.physicalValue = factor;
  signal.rawValue = offset;

  char *unitStart = strchr(parseEnd, '"');
  if (unitStart != nullptr) {
    ++unitStart;
    char *unitEnd = strchr(unitStart, '"');
    if (unitEnd != nullptr) {
      *unitEnd = '\0';
      copyString(signal.unit, sizeof(signal.unit), unitStart);
    }
  }

  if (signalOut != nullptr) {
    *signalOut = signal;
  }
  return true;
}

uint64_t extractLittleEndianValue(
    const uint8_t *data,
    uint8_t length,
    uint16_t startBit,
    uint16_t bitLength) {
  uint64_t value = 0;

  for (uint16_t bitIndex = 0; bitIndex < bitLength && bitIndex < 64; ++bitIndex) {
    uint16_t absoluteBit = static_cast<uint16_t>(startBit + bitIndex);
    uint8_t byteIndex = absoluteBit / 8;
    uint8_t bitInByte = absoluteBit % 8;
    if (byteIndex >= length) {
      continue;
    }

    if ((data[byteIndex] & (static_cast<uint8_t>(1U << bitInByte))) != 0) {
      value |= (1ULL << bitIndex);
    }
  }

  return value;
}

uint64_t extractBigEndianValue(
    const uint8_t *data,
    uint8_t length,
    uint16_t startBit,
    uint16_t bitLength) {
  uint64_t value = 0;
  int currentBit = static_cast<int>(startBit);

  for (uint16_t bitIndex = 0; bitIndex < bitLength && bitIndex < 64; ++bitIndex) {
    uint8_t byteIndex = static_cast<uint8_t>(currentBit / 8);
    uint8_t bitInByte = static_cast<uint8_t>(currentBit % 8);
    value <<= 1;
    if (byteIndex < length && (data[byteIndex] & (static_cast<uint8_t>(1U << bitInByte))) != 0) {
      value |= 1ULL;
    }

    if ((currentBit % 8) == 0) {
      currentBit += 15;
    } else {
      currentBit -= 1;
    }
  }

  return value;
}

double decodeSignedRawValue(uint64_t rawValue, uint16_t bitLength) {
  if (bitLength == 0) {
    return 0.0;
  }
  if (bitLength >= 64) {
    return static_cast<double>(static_cast<int64_t>(rawValue));
  }

  uint64_t signBit = 1ULL << (bitLength - 1);
  if ((rawValue & signBit) == 0) {
    return static_cast<double>(static_cast<int64_t>(rawValue));
  }

  int64_t extended = static_cast<int64_t>(rawValue | (~0ULL << bitLength));
  return static_cast<double>(extended);
}

bool addOrUpdateMessage(
    DbcMessageName *messages,
    size_t maxMessages,
    size_t *countInOut,
    uint32_t canId,
    const char *name,
    String *errorOut) {
  size_t insertIndex = 0;
  while (insertIndex < *countInOut && messages[insertIndex].canId < canId) {
    ++insertIndex;
  }

  if (insertIndex < *countInOut && messages[insertIndex].canId == canId) {
    snprintf(messages[insertIndex].name, sizeof(messages[insertIndex].name), "%s", name);
    return true;
  }

  if (*countInOut == maxMessages) {
    setError(errorOut, "DBC enthaelt mehr Nachrichten als das Geraet speichern kann.");
    return false;
  }

  memmove(
      &messages[insertIndex + 1],
      &messages[insertIndex],
      sizeof(messages[0]) * (*countInOut - insertIndex));
  messages[insertIndex] = DbcMessageName();
  messages[insertIndex].canId = canId;
  snprintf(messages[insertIndex].name, sizeof(messages[insertIndex].name), "%s", name);
  (*countInOut)++;
  return true;
}

bool parseMessageDefinition(
    char *line,
    DbcMessageName *messages,
    size_t maxMessages,
    size_t *countInOut,
    String *errorOut) {
  uint32_t canId = 0;
  char messageName[kMaxDbcNameLength];
  if (!parseMessageHeader(line, &canId, messageName, sizeof(messageName))) {
    return true;
  }

  return addOrUpdateMessage(messages, maxMessages, countInOut, canId, messageName, errorOut);
}

}  // namespace

bool DbcParser::parseFile(
    const char *path,
    DbcMessageName *messages,
    size_t maxMessages,
    size_t *countOut,
    String *errorOut) const {
  if (countOut == nullptr) {
    setError(errorOut, "Kein Ausgabepuffer fuer DBC Parsergebnis angegeben.");
    return false;
  }

  *countOut = 0;

  File file = LittleFS.open(path, "r");
  if (!file) {
    setError(errorOut, "DBC Datei konnte nicht geoeffnet werden.");
    return false;
  }

  char line[kLineBufferSize];
  while (file.available()) {
    size_t bytesRead = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[bytesRead] = '\0';
    trimCarriageReturn(line);
    if (!parseMessageDefinition(line, messages, maxMessages, countOut, errorOut)) {
      file.close();
      return false;
    }
  }

  file.close();
  return true;
}

bool DbcParser::decodeMessage(
    const char *path,
    uint32_t canId,
    const uint8_t *data,
    uint8_t length,
    char *messageNameOut,
    size_t messageNameOutSize,
    DbcDecodedSignal *signals,
    size_t maxSignals,
    size_t *countOut,
    bool *messageFoundOut,
    String *errorOut) const {
  if (countOut == nullptr || messageFoundOut == nullptr) {
    setError(errorOut, "Kein Ausgabepuffer fuer DBC Signaldekodierung angegeben.");
    return false;
  }

  *countOut = 0;
  *messageFoundOut = false;
  copyString(messageNameOut, messageNameOutSize, "");

  File file = LittleFS.open(path, "r");
  if (!file) {
    setError(errorOut, "DBC Datei konnte nicht geoeffnet werden.");
    return false;
  }

  bool currentMessageMatches = false;
  bool foundTargetMessage = false;
  char line[kLineBufferSize];
  while (file.available()) {
    size_t bytesRead = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[bytesRead] = '\0';
    trimCarriageReturn(line);

    uint32_t parsedCanId = 0;
    char parsedMessageName[kMaxDbcNameLength];
    if (parseMessageHeader(line, &parsedCanId, parsedMessageName, sizeof(parsedMessageName))) {
      if (foundTargetMessage && parsedCanId != canId) {
        break;
      }

      currentMessageMatches = parsedCanId == canId;
      if (currentMessageMatches) {
        foundTargetMessage = true;
        *messageFoundOut = true;
        copyString(messageNameOut, messageNameOutSize, parsedMessageName);
      }
      continue;
    }

    if (!currentMessageMatches) {
      continue;
    }

    DbcDecodedSignal signal;
    if (!parseSignalDefinition(line, &signal)) {
      continue;
    }

    if (*countOut == maxSignals) {
      setError(errorOut, "Zu viele DBC Signale fuer diese Nachricht.");
      file.close();
      return false;
    }

    uint64_t rawBits = signal.isLittleEndian
        ? extractLittleEndianValue(data, length, signal.startBit, signal.bitLength)
        : extractBigEndianValue(data, length, signal.startBit, signal.bitLength);
    double rawValue = signal.isSigned
        ? decodeSignedRawValue(rawBits, signal.bitLength)
        : static_cast<double>(rawBits);

    double factor = signal.physicalValue;
    double offset = signal.rawValue;
    signal.rawValue = rawValue;
    signal.physicalValue = rawValue * factor + offset;
    signals[*countOut] = signal;
    (*countOut)++;
  }

  file.close();
  return true;
}