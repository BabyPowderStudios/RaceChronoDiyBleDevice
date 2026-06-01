#include "CanBusSupport.h"

namespace {

const SupportedCanBaudRate kSupportedCanBaudRates[] = {
  {100L * 1000L, CAN_SPEED_100KBPS},
  {125L * 1000L, CAN_SPEED_125KBPS},
  {200L * 1000L, CAN_SPEED_200KBPS},
  {250L * 1000L, CAN_SPEED_250KBPS},
  {500L * 1000L, CAN_SPEED_500KBPS},
  {1000L * 1000L, CAN_SPEED_1000KBPS},
};

}  // namespace

size_t getSupportedCanBaudRates(const SupportedCanBaudRate **baudRatesOut) {
  if (baudRatesOut != nullptr) {
    *baudRatesOut = kSupportedCanBaudRates;
  }
  return sizeof(kSupportedCanBaudRates) / sizeof(kSupportedCanBaudRates[0]);
}

bool isSupportedCanBaudRate(long baudRate) {
  CAN_speed_t canSpeed;
  return tryGetCanSpeed(baudRate, &canSpeed);
}

bool tryGetCanSpeed(long baudRate, CAN_speed_t *canSpeed) {
  for (const SupportedCanBaudRate &candidate : kSupportedCanBaudRates) {
    if (candidate.baudRate != baudRate) {
      continue;
    }

    *canSpeed = candidate.speed;
    return true;
  }

  return false;
}
