#include "WebUi.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>

#include <new>

#include "CanBusSupport.h"
#include "WebUiHtml.h"

namespace {

const char kTemporaryDbcPath[] = "/dbc_upload.tmp";
const char kPersistentDbcPath[] = "/uploaded.dbc";
constexpr size_t kStateDocumentCapacity = 4 * 1024;

void appendJsonEscapedString(String &output, const char *value) {
  output += '"';
  if (value != nullptr) {
    for (const char *cursor = value; *cursor != '\0'; ++cursor) {
      const char ch = *cursor;
      switch (ch) {
        case '"':
          output += "\\\"";
          break;
        case '\\':
          output += "\\\\";
          break;
        case '\b':
          output += "\\b";
          break;
        case '\f':
          output += "\\f";
          break;
        case '\n':
          output += "\\n";
          break;
        case '\r':
          output += "\\r";
          break;
        case '\t':
          output += "\\t";
          break;
        default:
          if (static_cast<unsigned char>(ch) < 0x20) {
            char escaped[7];
            snprintf(escaped, sizeof(escaped), "\\u%04X", static_cast<unsigned char>(ch));
            output += escaped;
          } else {
            output += ch;
          }
          break;
      }
    }
  }
  output += '"';
}

}  // namespace

WebUiServer::WebUiServer(
    SettingsStore &settingsStore,
    CanTracker &canTracker,
    bool *restartCanRequested)
    : settingsStore_(settingsStore),
      canTracker_(canTracker),
      restartCanRequested_(restartCanRequested),
      server_(80) {
}

void WebUiServer::begin() {
  buildAccessPointSsid();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(accessPointSsid_);

  configureRoutes();
  server_.begin();

  Serial.print("Web UI available at http://");
  Serial.println(WiFi.softAPIP());
}

void WebUiServer::handleClient() {
  server_.handleClient();
}

void WebUiServer::configureRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/state", HTTP_GET, [this]() { handleState(); });
  server_.on("/api/stats", HTTP_GET, [this]() { handleStats(); });
  server_.on("/api/can-details", HTTP_GET, [this]() { handleCanDetails(); });
  server_.on("/api/can", HTTP_POST, [this]() { handleCanConfig(); });
  server_.on("/api/rule", HTTP_POST, [this]() { handleRuleUpdate(); });
  server_.on(
      "/api/dbc",
      HTTP_POST,
      [this]() { handleDbcUpload(); },
      [this]() { handleDbcUploadData(); });
}

void WebUiServer::handleRoot() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send_P(200, "text/html; charset=utf-8", kWebUiHtml);
}

void WebUiServer::handleState() {
  DynamicJsonDocument document(kStateDocumentCapacity);

  JsonObject accessPoint = document.createNestedObject("accessPoint");
  accessPoint["ssid"] = accessPointSsid_;
  accessPoint["ip"] = WiFi.softAPIP().toString();

  JsonObject can = document.createNestedObject("can");
  can["baudRate"] = settingsStore_.getCanBaudRate();
  JsonArray supportedBaudRates = can.createNestedArray("supportedBaudRates");
  const SupportedCanBaudRate *baudRates = nullptr;
  size_t baudRateCount = getSupportedCanBaudRates(&baudRates);
  for (size_t index = 0; index < baudRateCount; ++index) {
    supportedBaudRates.add(baudRates[index].baudRate);
  }

  JsonObject dbc = document.createNestedObject("dbc");
  dbc["fileName"] = settingsStore_.getDbcFileName();
  dbc["messageCount"] = settingsStore_.getDbcMessageCount();

  document["observedCanIdCount"] = canTracker_.getCount();

  sendJsonResponse(200, document);
}

void WebUiServer::handleStats() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "application/json", "");

  const uint32_t nowMs = millis();
  const size_t count = canTracker_.getCount();
  String chunk;

  server_.sendContent("{\"stats\":[");
  for (size_t index = 0; index < count; ++index) {
    const ObservedCanId &entry = canTracker_.getByIndex(index);
    chunk = "";
    chunk.reserve(192);
    if (index > 0) {
      chunk += ',';
    }
    chunk += "{\"id\":";
    chunk += String(entry.canId);
    chunk += ",\"frequencyMilliHz\":";
    chunk += String(canTracker_.getCurrentFrequencyMilliHz(entry, nowMs));
    chunk += ",\"lastSeenMsAgo\":";
    chunk += String(nowMs - entry.lastSeenMs);
    chunk += ",\"length\":";
    chunk += String(entry.lastLength);
    chunk += ",\"data\":[";
    for (uint8_t byteIndex = 0; byteIndex < 8; ++byteIndex) {
      if (byteIndex > 0) {
        chunk += ',';
      }
      chunk += String(entry.lastData[byteIndex]);
    }
    chunk += ']';

    const CanRule *rule = settingsStore_.findRule(entry.canId);
    chunk += ",\"ignored\":";
    chunk += rule != nullptr && rule->ignored ? "true" : "false";
    chunk += ",\"rateLimitHz\":";
    chunk += String(rule != nullptr ? rule->rateLimitHz : 0);
    chunk += ",\"name\":";

    const DbcMessageName *dbcMessage = settingsStore_.findDbcMessage(entry.canId);
    appendJsonEscapedString(chunk, dbcMessage != nullptr ? dbcMessage->name : "");
    chunk += '}';
    server_.sendContent(chunk);
  }
  server_.sendContent("]}");
}

void WebUiServer::handleCanDetails() {
  if (!server_.hasArg("canId")) {
    sendError(400, "CAN ID fehlt.");
    return;
  }

  String canIdText = server_.arg("canId");
  uint32_t canId = static_cast<uint32_t>(strtoul(canIdText.c_str(), nullptr, 0));
  const ObservedCanId *entry = canTracker_.find(canId);
  if (entry == nullptr) {
    sendError(404, "CAN ID wurde noch nicht empfangen.");
    return;
  }

  constexpr size_t kCanDetailsDocumentCapacity = 24 * 1024;
  DynamicJsonDocument document(kCanDetailsDocumentCapacity);
  document["canId"] = canId;
  document["length"] = entry->lastLength;

  JsonArray data = document.createNestedArray("data");
  for (uint8_t byteIndex = 0; byteIndex < 8; ++byteIndex) {
    data.add(entry->lastData[byteIndex]);
  }

  const DbcMessageName *knownMessage = settingsStore_.findDbcMessage(canId);
  document["name"] = knownMessage != nullptr ? knownMessage->name : "";
  document["hasDbcFile"] = LittleFS.exists(kPersistentDbcPath);
  document["dbcMatched"] = false;

  JsonArray signals = document.createNestedArray("signals");
  if (!LittleFS.exists(kPersistentDbcPath)) {
    document["note"] = "Keine DBC Datei hochgeladen.";
    sendJsonResponse(200, document);
    return;
  }

  DbcDecodedSignal *decodedSignals =
      new (std::nothrow) DbcDecodedSignal[kMaxDbcSignalsPerMessage];
  if (decodedSignals == nullptr) {
    sendError(500, "Nicht genug Speicher fuer die DBC Signaldekodierung.");
    return;
  }

  size_t decodedSignalCount = 0;
  bool messageFound = false;
  char messageName[kMaxDbcNameLength] = {0};
  String error;
  if (!dbcParser_.decodeMessage(
          kPersistentDbcPath,
          canId,
          entry->lastData,
          entry->lastLength,
          messageName,
          sizeof(messageName),
          decodedSignals,
          kMaxDbcSignalsPerMessage,
          &decodedSignalCount,
          &messageFound,
          &error)) {
            delete[] decodedSignals;
    document["note"] = error;
    sendJsonResponse(200, document);
    return;
  }

  document["dbcMatched"] = messageFound;
  if (messageName[0] != '\0') {
    document["name"] = messageName;
  }

  if (!messageFound) {
    delete[] decodedSignals;
    document["note"] = "Keine passende Nachricht in der DBC Datei gefunden.";
    sendJsonResponse(200, document);
    return;
  }

  if (decodedSignalCount == 0) {
    delete[] decodedSignals;
    document["note"] = "Die DBC Nachricht enthaelt keine unterstuetzten Signale.";
    sendJsonResponse(200, document);
    return;
  }

  for (size_t index = 0; index < decodedSignalCount; ++index) {
    JsonObject signal = signals.createNestedObject();
    signal["name"] = decodedSignals[index].name;
    signal["unit"] = decodedSignals[index].unit;
    signal["startBit"] = decodedSignals[index].startBit;
    signal["bitLength"] = decodedSignals[index].bitLength;
    signal["isLittleEndian"] = decodedSignals[index].isLittleEndian;
    signal["isSigned"] = decodedSignals[index].isSigned;
    signal["rawValue"] = decodedSignals[index].rawValue;
    signal["physicalValue"] = decodedSignals[index].physicalValue;
  }

  delete[] decodedSignals;
  sendJsonResponse(200, document);
}

void WebUiServer::handleCanConfig() {
  DynamicJsonDocument request(512);
  DeserializationError error = deserializeJson(request, server_.arg("plain"));
  if (error) {
    sendError(400, "Ungueltiger JSON Body fuer CAN Konfiguration.");
    return;
  }

  long baudRate = request["baudRate"] | 0L;
  if (!isSupportedCanBaudRate(baudRate)) {
    sendError(400, "Nicht unterstuetzte CAN Baudrate.");
    return;
  }

  if (!settingsStore_.setCanBaudRate(baudRate)) {
    sendError(500, "CAN Baudrate konnte nicht gespeichert werden.");
    return;
  }

  if (restartCanRequested_ != nullptr) {
    *restartCanRequested_ = true;
  }

  sendMessage("CAN Baudrate gespeichert. CAN Controller wird neu gestartet.");
}

void WebUiServer::handleRuleUpdate() {
  DynamicJsonDocument request(512);
  DeserializationError error = deserializeJson(request, server_.arg("plain"));
  if (error) {
    sendError(400, "Ungueltiger JSON Body fuer CAN Regel.");
    return;
  }

  JsonVariant canIdVariant = request["canId"];
  if (canIdVariant.isNull()) {
    sendError(400, "CAN ID fehlt.");
    return;
  }

  uint32_t canId = canIdVariant.as<uint32_t>();
  bool ignored = request["ignored"] | false;
  long rateLimitHz = request["rateLimitHz"] | 0L;
  if (rateLimitHz < 0 || rateLimitHz > 2000) {
    sendError(400, "Limit Hz muss zwischen 0 und 2000 liegen.");
    return;
  }

  if (!settingsStore_.upsertRule(canId, ignored, static_cast<uint16_t>(rateLimitHz))) {
    sendError(500, "CAN Regel konnte nicht gespeichert werden.");
    return;
  }

  sendMessage("CAN Regel gespeichert.");
}

void WebUiServer::handleDbcUploadData() {
  HTTPUpload &upload = server_.upload();

  switch (upload.status) {
    case UPLOAD_FILE_START:
      uploadError_ = "";
      uploadedFileName_ = sanitizeFileName(upload.filename);
      Serial.print("DBC upload started: ");
      Serial.println(uploadedFileName_);
      LittleFS.remove(kTemporaryDbcPath);
      uploadFile_ = LittleFS.open(kTemporaryDbcPath, "w");
      if (!uploadFile_) {
        uploadError_ = "Temporare DBC Datei konnte nicht angelegt werden.";
        Serial.println(uploadError_);
      }
      break;

    case UPLOAD_FILE_WRITE:
      if (!uploadFile_) {
        break;
      }
      if (uploadFile_.write(upload.buf, upload.currentSize) != upload.currentSize) {
        uploadError_ = "DBC Upload konnte nicht in den Flash geschrieben werden.";
        Serial.println(uploadError_);
      }
      break;

    case UPLOAD_FILE_END:
      if (uploadFile_) {
        uploadFile_.close();
      }
      Serial.println("DBC upload transfer finished.");
      break;

    case UPLOAD_FILE_ABORTED:
      if (uploadFile_) {
        uploadFile_.close();
      }
      LittleFS.remove(kTemporaryDbcPath);
      uploadError_ = "DBC Upload wurde abgebrochen.";
      Serial.println(uploadError_);
      break;
  }
}

void WebUiServer::handleDbcUpload() {
  if (!uploadError_.isEmpty()) {
    LittleFS.remove(kTemporaryDbcPath);
    Serial.print("DBC upload failed: ");
    Serial.println(uploadError_);
    sendError(500, uploadError_);
    return;
  }

  DbcMessageName *messages = new (std::nothrow) DbcMessageName[kMaxDbcMessages];
  if (messages == nullptr) {
    LittleFS.remove(kTemporaryDbcPath);
    Serial.println("DBC upload failed: not enough memory for temporary message buffer.");
    sendError(500, "Nicht genug Arbeitsspeicher fuer den DBC Upload.");
    return;
  }

  size_t messageCount = 0;
  String parseError;
  if (!dbcParser_.parseFile(
          kTemporaryDbcPath,
          messages,
          kMaxDbcMessages,
          &messageCount,
          &parseError)) {
    delete[] messages;
    LittleFS.remove(kTemporaryDbcPath);
    Serial.print("DBC upload parse failed: ");
    Serial.println(parseError);
    sendError(400, parseError);
    return;
  }

  LittleFS.remove(kPersistentDbcPath);
  if (!LittleFS.rename(kTemporaryDbcPath, kPersistentDbcPath)) {
    delete[] messages;
    Serial.println("DBC upload failed: could not move temporary file into persistent storage.");
    sendError(500, "DBC Datei konnte nicht im Flash abgelegt werden.");
    return;
  }

  if (!settingsStore_.replaceDbcMessages(
          uploadedFileName_.c_str(),
          messages,
          messageCount)) {
    delete[] messages;
    Serial.println("DBC upload failed: could not persist parsed DBC message mapping.");
    sendError(500, "DBC Mapping konnte nicht gespeichert werden.");
    return;
  }

  Serial.print("DBC upload stored successfully: ");
  Serial.print(uploadedFileName_);
  Serial.print(", messages parsed: ");
  Serial.println(messageCount);
  delete[] messages;
  sendMessage("DBC Datei gespeichert und analysiert.");
}

void WebUiServer::sendJsonResponse(
    int statusCode,
    const ArduinoJson::JsonDocument &document) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.setContentLength(measureJson(document));
  server_.send(statusCode, "application/json", "");
  serializeJson(document, server_.client());
}

void WebUiServer::sendError(int statusCode, const String &message) {
  DynamicJsonDocument document(256);
  document["error"] = message;
  sendJsonResponse(statusCode, document);
}

void WebUiServer::sendMessage(const String &message) {
  DynamicJsonDocument document(256);
  document["message"] = message;
  sendJsonResponse(200, document);
}

void WebUiServer::buildAccessPointSsid() {
  uint64_t chipId = ESP.getEfuseMac();
  snprintf(
      accessPointSsid_,
      sizeof(accessPointSsid_),
      "RaceChrono-%04X",
      static_cast<unsigned int>(chipId & 0xFFFF));
}

String WebUiServer::sanitizeFileName(const String &fileName) const {
  int slashIndex = fileName.lastIndexOf('/');
  int backslashIndex = fileName.lastIndexOf('\\');
  int separatorIndex = slashIndex > backslashIndex ? slashIndex : backslashIndex;
  if (separatorIndex >= 0) {
    return fileName.substring(separatorIndex + 1);
  }
  return fileName;
}