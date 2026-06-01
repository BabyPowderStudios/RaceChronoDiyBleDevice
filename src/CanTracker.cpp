#include "CanTracker.h"

#include <string.h>

namespace {

constexpr uint32_t kFrequencyStaleTimeoutMs = 2000;

}  // namespace

void CanTracker::observe(
    uint32_t canId,
    const uint8_t *data,
    uint8_t length,
    uint32_t nowMs) {
  int index = findIndex(canId);
  if (index < 0) {
    if (count_ == kMaxObservedCanIds) {
      return;
    }

    index = findInsertPosition(canId);
    memmove(
        &entries_[index + 1],
        &entries_[index],
        sizeof(entries_[0]) * (count_ - index));
    entries_[index] = ObservedCanId();
    entries_[index].canId = canId;
    count_++;
  }

  ObservedCanId &entry = entries_[index];
  entry.lastLength = length > 8 ? 8 : length;
  memset(entry.lastData, 0, sizeof(entry.lastData));
  memcpy(entry.lastData, data, entry.lastLength);
  entry.lastSeenMs = nowMs;
  entry.totalCount++;

  if (entry.lastRateSampleMs == 0) {
    entry.lastRateSampleMs = nowMs;
    entry.rateSampleCount = 0;
    return;
  }

  entry.rateSampleCount++;
  uint32_t elapsedMs = nowMs - entry.lastRateSampleMs;
  if (elapsedMs >= 1000) {
    entry.frequencyMilliHz =
        static_cast<uint32_t>(
            (static_cast<uint64_t>(entry.rateSampleCount) * 1000000ULL)
            / elapsedMs);
    entry.lastRateSampleMs = nowMs;
    entry.rateSampleCount = 0;
  }
}

const ObservedCanId *CanTracker::find(uint32_t canId) const {
  int index = findIndex(canId);
  if (index < 0) {
    return nullptr;
  }

  return &entries_[index];
}

size_t CanTracker::getCount() const {
  return count_;
}

const ObservedCanId &CanTracker::getByIndex(size_t index) const {
  return entries_[index];
}

uint32_t CanTracker::getCurrentFrequencyMilliHz(uint32_t canId, uint32_t nowMs) const {
  const ObservedCanId *entry = find(canId);
  if (entry == nullptr) {
    return 0;
  }

  return getCurrentFrequencyMilliHz(*entry, nowMs);
}

uint32_t CanTracker::getCurrentFrequencyMilliHz(
    const ObservedCanId &entry,
    uint32_t nowMs) const {
  if (entry.lastSeenMs == 0 || nowMs - entry.lastSeenMs > kFrequencyStaleTimeoutMs) {
    return 0;
  }

  return entry.frequencyMilliHz;
}

int CanTracker::findIndex(uint32_t canId) const {
  for (size_t index = 0; index < count_; ++index) {
    if (entries_[index].canId == canId) {
      return static_cast<int>(index);
    }
    if (entries_[index].canId > canId) {
      break;
    }
  }

  return -1;
}

int CanTracker::findInsertPosition(uint32_t canId) const {
  size_t index = 0;
  while (index < count_ && entries_[index].canId < canId) {
    ++index;
  }
  return static_cast<int>(index);
}