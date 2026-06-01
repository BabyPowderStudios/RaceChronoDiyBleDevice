#ifndef DBC_PARSER_H
#define DBC_PARSER_H

#include "AppTypes.h"

class DbcParser {
public:
  bool parseFile(
      const char *path,
      DbcMessageName *messages,
      size_t maxMessages,
      size_t *countOut,
      String *errorOut) const;

  bool decodeMessage(
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
      String *errorOut) const;
};

#endif