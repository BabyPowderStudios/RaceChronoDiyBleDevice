#ifndef WEB_UI_H
#define WEB_UI_H

#include <ArduinoJson.h>
#include <WebServer.h>

#include "CanTracker.h"
#include "DbcParser.h"
#include "SettingsStore.h"

class WebUiServer {
public:
  WebUiServer(SettingsStore &settingsStore, CanTracker &canTracker, bool *restartCanRequested);

  void begin();
  void handleClient();

private:
  void configureRoutes();
  void handleRoot();
  void handleState();
  void handleStats();
  void handleCanDetails();
  void handleCanConfig();
  void handleRuleUpdate();
  void handleDbcUpload();
  void handleDbcUploadData();
  void sendJsonResponse(
      int statusCode,
      const ArduinoJson::JsonDocument &document);
  void sendError(int statusCode, const String &message);
  void sendMessage(const String &message);
  void buildAccessPointSsid();
  String sanitizeFileName(const String &fileName) const;

  SettingsStore &settingsStore_;
  CanTracker &canTracker_;
  bool *restartCanRequested_;
  WebServer server_;
  DbcParser dbcParser_;
  File uploadFile_;
  String uploadError_;
  String uploadedFileName_;
  char accessPointSsid_[32] = {0};
};

#endif