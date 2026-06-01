#ifndef CAN_TRACKER_H
#define CAN_TRACKER_H

#include "AppTypes.h"

class CanTracker {
public:
  void observe(uint32_t canId, const uint8_t *data, uint8_t length, uint32_t nowMs);
  const ObservedCanId *find(uint32_t canId) const;
  size_t getCount() const;
  const ObservedCanId &getByIndex(size_t index) const;
  uint32_t getCurrentFrequencyMilliHz(uint32_t canId, uint32_t nowMs) const;
  uint32_t getCurrentFrequencyMilliHz(
      const ObservedCanId &entry,
      uint32_t nowMs) const;

private:
  int findIndex(uint32_t canId) const;
  int findInsertPosition(uint32_t canId) const;

  ObservedCanId entries_[kMaxObservedCanIds];
  size_t count_ = 0;
};

#endif