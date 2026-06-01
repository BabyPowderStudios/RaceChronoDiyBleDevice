#include "SettingsStore.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include <string.h>

namespace {

const char kSettingsFilePath[] = "/settings.json";
const char kTemporarySettingsFilePath[] = "/settings.tmp";
constexpr size_t kSettingsDocumentCapacity = 64 * 1024;

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

}  // namespace

bool SettingsStore::begin() {
  if (!LittleFS.begin(true)) {
    return false;
  }

  if (!load()) {
    return save();
  }

  return true;
}

long SettingsStore::getCanBaudRate() const {
  return canBaudRate_;
}

bool SettingsStore::setCanBaudRate(long baudRate) {
  if (canBaudRate_ == baudRate) {
    return true;
  }

  canBaudRate_ = baudRate;
  return save();
}

const CanRule *SettingsStore::findRule(uint32_t canId) const {
  int index = findRuleIndex(canId);
  if (index < 0) {
    return nullptr;
  }

  return &rules_[index];
}

size_t SettingsStore::getRuleCount() const {
  return ruleCount_;
}

const CanRule &SettingsStore::getRuleAt(size_t index) const {
  return rules_[index];
}

bool SettingsStore::upsertRule(uint32_t canId, bool ignored, uint16_t rateLimitHz) {
  int index = findRuleIndex(canId);
  bool keepRule = ignored || rateLimitHz > 0;

  if (!keepRule) {
    if (index < 0) {
      return true;
    }

    memmove(
        &rules_[index],
        &rules_[index + 1],
        sizeof(rules_[0]) * (ruleCount_ - index - 1));
    ruleCount_--;
    return save();
  }

  if (index < 0) {
    if (ruleCount_ == kMaxCanRules) {
      return false;
    }

    index = findRuleInsertPosition(canId);
    memmove(
        &rules_[index + 1],
        &rules_[index],
        sizeof(rules_[0]) * (ruleCount_ - index));
    ruleCount_++;
    rules_[index] = CanRule();
    rules_[index].canId = canId;
  }

  rules_[index].ignored = ignored;
  rules_[index].rateLimitHz = rateLimitHz;
  return save();
}

const DbcMessageName *SettingsStore::findDbcMessage(uint32_t canId) const {
  int index = findDbcMessageIndex(canId);
  if (index < 0) {
    return nullptr;
  }

  return &dbcMessages_[index];
}

size_t SettingsStore::getDbcMessageCount() const {
  return dbcMessageCount_;
}

const DbcMessageName &SettingsStore::getDbcMessageAt(size_t index) const {
  return dbcMessages_[index];
}

const char *SettingsStore::getDbcFileName() const {
  return dbcFileName_;
}

bool SettingsStore::replaceDbcMessages(
    const char *fileName,
    const DbcMessageName *messages,
    size_t count) {
  dbcMessageCount_ = count > kMaxDbcMessages ? kMaxDbcMessages : count;
  for (size_t index = 0; index < dbcMessageCount_; ++index) {
    dbcMessages_[index] = messages[index];
  }
  copyString(dbcFileName_, sizeof(dbcFileName_), fileName);
  return save();
}

bool SettingsStore::load() {
  File file = LittleFS.open(kSettingsFilePath, "r");
  if (!file) {
    return false;
  }

  DynamicJsonDocument document(kSettingsDocumentCapacity);
  DeserializationError error = deserializeJson(document, file);
  file.close();
  if (error) {
    return false;
  }

  canBaudRate_ = document["canBaudRate"] | kDefaultCanBaudRate;
  copyString(
      dbcFileName_,
      sizeof(dbcFileName_),
      document["dbcFileName"] | "");

  ruleCount_ = 0;
  JsonArray rules = document["rules"].as<JsonArray>();
  for (JsonObject rule : rules) {
    if (ruleCount_ == kMaxCanRules) {
      break;
    }

    rules_[ruleCount_].canId = rule["id"] | 0U;
    rules_[ruleCount_].ignored = rule["ignored"] | false;
    rules_[ruleCount_].rateLimitHz = rule["rateLimitHz"] | 0;
    ruleCount_++;
  }

  dbcMessageCount_ = 0;
  JsonArray dbcMessages = document["dbcMessages"].as<JsonArray>();
  for (JsonObject dbcMessage : dbcMessages) {
    if (dbcMessageCount_ == kMaxDbcMessages) {
      break;
    }

    dbcMessages_[dbcMessageCount_].canId = dbcMessage["id"] | 0U;
    copyString(
        dbcMessages_[dbcMessageCount_].name,
        sizeof(dbcMessages_[dbcMessageCount_].name),
        dbcMessage["name"] | "");
    dbcMessageCount_++;
  }

  return true;
}

bool SettingsStore::save() const {
  DynamicJsonDocument document(kSettingsDocumentCapacity);
  document["canBaudRate"] = canBaudRate_;
  if (dbcFileName_[0] != '\0') {
    document["dbcFileName"] = dbcFileName_;
  }

  JsonArray rules = document.createNestedArray("rules");
  for (size_t index = 0; index < ruleCount_; ++index) {
    JsonObject rule = rules.createNestedObject();
    rule["id"] = rules_[index].canId;
    rule["ignored"] = rules_[index].ignored;
    rule["rateLimitHz"] = rules_[index].rateLimitHz;
  }

  JsonArray dbcMessages = document.createNestedArray("dbcMessages");
  for (size_t index = 0; index < dbcMessageCount_; ++index) {
    JsonObject dbcMessage = dbcMessages.createNestedObject();
    dbcMessage["id"] = dbcMessages_[index].canId;
    dbcMessage["name"] = dbcMessages_[index].name;
  }

  File file = LittleFS.open(kTemporarySettingsFilePath, "w");
  if (!file) {
    return false;
  }

  bool success = serializeJson(document, file) > 0;
  file.close();
  if (!success) {
    LittleFS.remove(kTemporarySettingsFilePath);
    return false;
  }

  LittleFS.remove(kSettingsFilePath);
  return LittleFS.rename(kTemporarySettingsFilePath, kSettingsFilePath);
}

int SettingsStore::findRuleIndex(uint32_t canId) const {
  for (size_t index = 0; index < ruleCount_; ++index) {
    if (rules_[index].canId == canId) {
      return static_cast<int>(index);
    }
    if (rules_[index].canId > canId) {
      break;
    }
  }

  return -1;
}

int SettingsStore::findRuleInsertPosition(uint32_t canId) const {
  size_t index = 0;
  while (index < ruleCount_ && rules_[index].canId < canId) {
    ++index;
  }

  return static_cast<int>(index);
}

int SettingsStore::findDbcMessageIndex(uint32_t canId) const {
  for (size_t index = 0; index < dbcMessageCount_; ++index) {
    if (dbcMessages_[index].canId == canId) {
      return static_cast<int>(index);
    }
    if (dbcMessages_[index].canId > canId) {
      break;
    }
  }

  return -1;
}