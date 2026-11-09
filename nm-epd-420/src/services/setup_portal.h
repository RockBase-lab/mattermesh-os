#pragma once
#include <Arduino.h>
#include "time_zones.h"
namespace device {
struct SetupPortalSettings {
  String ssid, password, name;
  int band = 868, preset = 0, pathBytes = 2, timeZone = DEFAULT_TIME_ZONE;
};
struct SetupPortalStatus {
  bool active = false, ready = false;
  uint32_t revision = 0;
  String ssid, password, address, message;
};
using SetupPortalSaveCallback = bool (*)(const SetupPortalSettings& settings, String& reason);
using SetupPortalReconnectCallback = void (*)();
bool setupPortalStart(const SetupPortalSettings& current, SetupPortalSaveCallback save,
                      SetupPortalReconnectCallback reconnect = nullptr);
void setupPortalLoop();
void setupPortalStop();
void setupPortalRestart();
void setupPortalReconnect();
SetupPortalStatus setupPortalStatus();
} // namespace device
