#ifndef CAN_BUS_SUPPORT_H
#define CAN_BUS_SUPPORT_H

#include <CAN_config.h>

struct SupportedCanBaudRate {
  long baudRate;
  CAN_speed_t speed;
};

size_t getSupportedCanBaudRates(const SupportedCanBaudRate **baudRatesOut);
bool isSupportedCanBaudRate(long baudRate);
bool tryGetCanSpeed(long baudRate, CAN_speed_t *canSpeed);

#endif