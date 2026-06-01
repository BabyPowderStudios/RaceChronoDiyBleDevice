#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include "AppTypes.h"

class SettingsStore {
public:
  bool begin();

  long getCanBaudRate() const;
  bool setCanBaudRate(long baudRate);

  const CanRule *findRule(uint32_t canId) const;
  size_t getRuleCount() const;
  const CanRule &getRuleAt(size_t index) const;
  bool upsertRule(uint32_t canId, bool ignored, uint16_t rateLimitHz);

  const DbcMessageName *findDbcMessage(uint32_t canId) const;
  size_t getDbcMessageCount() const;
  const DbcMessageName &getDbcMessageAt(size_t index) const;
  const char *getDbcFileName() const;
  bool replaceDbcMessages(
      const char *fileName,
      const DbcMessageName *messages,
      size_t count);

private:
  bool load();
  bool save() const;
  int findRuleIndex(uint32_t canId) const;
  int findRuleInsertPosition(uint32_t canId) const;
  int findDbcMessageIndex(uint32_t canId) const;

  long canBaudRate_ = kDefaultCanBaudRate;
  CanRule rules_[kMaxCanRules];
  size_t ruleCount_ = 0;
  DbcMessageName dbcMessages_[kMaxDbcMessages];
  size_t dbcMessageCount_ = 0;
  char dbcFileName_[kMaxDbcFileNameLength] = {0};
};

#endif