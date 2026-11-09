#include "setup_portal.h"
#include "wifi_policy.h"
#include "mesh/companion_settings.h"
#include "mesh/radio_presets.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_system.h>
#include <esp_mac.h>
namespace device {
static WebServer server(80);
static DNSServer dns;
static SetupPortalStatus status;
static SetupPortalSettings settings;
static SetupPortalSaveCallback saveCallback = nullptr;
static SetupPortalReconnectCallback reconnectCallback = nullptr;
static uint32_t restartAt = 0, reconnectAt = 0;

static String escapeHtml(const String& value) {
  String out;
  out.reserve(value.length() + 16);
  for (unsigned i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '&')
      out += "&amp;";
    else if (c == '<')
      out += "&lt;";
    else if (c == '>')
      out += "&gt;";
    else if (c == '\"')
      out += "&quot;";
    else if (c == '\'')
      out += "&#39;";
    else
      out += c;
  }
  return out;
}
static void headers() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
}
static String pageHead(const char* title) {
  String html = F("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta "
                  "name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>");
  html += title;
  html += F("</"
            "title><style>*{box-sizing:border-box}body{margin:0;background:#f2f4f5;color:#172126;"
            "font:16px system-ui,sans-serif}main{max-width:620px;margin:24px auto;padding:0 "
            "16px}section{background:white;border:1px solid "
            "#d8dee1;border-radius:14px;padding:20px;box-shadow:0 3px 16px "
            "#0001}h1{font-size:25px;margin:0 0 "
            "8px}p{line-height:1.45}label{display:block;font-weight:650;margin:16px 0 "
            "6px}input,select{width:100%;font:inherit;padding:12px;border:1px solid "
            "#9aa8ae;border-radius:8px;background:white}button{width:100%;font:inherit;font-weight:"
            "700;padding:13px;margin-top:22px;border:0;border-radius:8px;background:#146c60;color:"
            "white}.note{font-size:14px;color:#536168}.error{padding:10px;border-radius:8px;"
            "background:#fde8e7;color:#8d2019}.ok{font-size:52px;color:#146c60;margin:4px "
            "0}</style></head><body><main><section><h1>");
  html += title;
  return html;
}
static void sendForm(int code, const String& error = "") {
  String html = pageHead("MatterMesh setup");
  html += F("</h1><p>Configure Wi-Fi and the MeshCore companion. All fields are saved on this "
            "device.</p>");
  if (error.length()) {
    html += F("<p class=\"error\">");
    html += escapeHtml(error);
    html += F("</p>");
  }
  html += F("<form method=\"post\" action=\"/save\"><label for=\"ssid\">Wi-Fi name "
            "(SSID)</label><input id=\"ssid\" name=\"ssid\" maxlength=\"32\" required value=\"");
  html += escapeHtml(settings.ssid);
  html += F("\"><label for=\"password\">Wi-Fi password</label><input id=\"password\" "
            "type=\"password\" name=\"password\" maxlength=\"63\" autocomplete=\"new-password\"><p "
            "class=\"note\">Leave empty only for an open network.</p><label for=\"band\">Frequency "
            "band</label><select id=\"band\" name=\"band\"><option value=\"868\"");
  if (settings.band == 868)
    html += F(" selected");
  html += F(">868 MHz</option><option value=\"900\"");
  if (settings.band == 900)
    html += F(" selected");
  html += F(">900 MHz</option></select><label for=\"preset\">Country / network "
            "preset</label><select id=\"preset\" name=\"preset\">");
  for (int i = 0; i < matter::RADIO_PRESET_COUNT; i++) {
    const auto& p = matter::RADIO_PRESETS[i];
    html += F("<option value=\"");
    html += i;
    html += F("\" data-band=\"");
    html += matter::radioBand(p.frequency);
    html += F("\"");
    if (i == settings.preset)
      html += F(" selected");
    html += F(">");
    html += escapeHtml(p.name);
    html += F("</option>");
  }
  html += F("</select><label for=\"timeZone\">Time zone</label><select id=\"timeZone\" "
            "name=\"timeZone\">");
  for (int i = 0; i < TIME_ZONE_COUNT; i++) {
    html += F("<option value=\"");
    html += i;
    html += F("\"");
    if (i == settings.timeZone)
      html += F(" selected");
    html += F(">");
    html += escapeHtml(TIME_ZONES[i].label);
    html += F("</option>");
  }
  html += F("</select><p class=\"note\">Used for the device clock after Wi-Fi, GPS or manual time "
            "synchronization.</p><label for=\"name\">Companion Name</label><input id=\"name\" "
            "name=\"name\" maxlength=\"31\" required value=\"");
  html += escapeHtml(settings.name);
  html += F("\"><label for=\"pathBytes\">Path hash size</label><select id=\"pathBytes\" "
            "name=\"pathBytes\">");
  for (int bytes = 1; bytes <= 3; bytes++) {
    html += F("<option value=\"");
    html += bytes;
    html += F("\"");
    if (bytes == settings.pathBytes)
      html += F(" selected");
    html += F(">");
    html += bytes;
    html += bytes == 1 ? F(" byte</option>") : F(" bytes</option>");
  }
  html += F("</select><button type=\"submit\">Save configuration</button></form><p "
            "class=\"note\">After saving, reconnect Wi-Fi immediately or restart into "
            "MatterMesh.</p></section></main><script>const "
            "band=document.getElementById('band'),preset=document.getElementById('preset');"
            "function syncPresets(){let visible=false;for(const o of preset.options){const "
            "show=o.dataset.band===band.value;o.hidden=!show;o.disabled=!show;if(show&&o.selected)"
            "visible=true}if(!visible)for(const o of "
            "preset.options)if(!o.disabled){preset.value=o.value;break}}band.addEventListener('"
            "change',syncPresets);syncPresets();</script></body></html>");
  headers();
  server.send(code, "text/html", html);
}
static void sendReady() {
  String html = pageHead("Ready");
  html +=
      F("</h1><p class=\"ok\">&#10003;</p><p><strong>Configuration saved.</strong></p><p>Reconnect "
        "now to close this hotspot and join the configured Wi-Fi, or restart the device.</p><form "
        "method=\"post\" action=\"/reconnect\"><button type=\"submit\">Reconnect Wi-Fi "
        "now</button></form><form method=\"post\" action=\"/restart\"><button "
        "type=\"submit\">Restart device</button></form></section></main></body></html>");
  headers();
  server.send(200, "text/html", html);
}
static bool valid(const SetupPortalSettings& candidate, String& reason) {
  if (!validWifiCredentials(std::string(candidate.ssid.c_str(), candidate.ssid.length()),
                            std::string(candidate.password.c_str(), candidate.password.length()))) {
    reason = "Enter an SSID of 1-32 bytes and a password of 8-63 bytes (or leave it empty).";
    return false;
  }
  if (!matter::validCompanionName(candidate.name.c_str())) {
    reason = "Companion Name must contain 1-31 visible bytes.";
    return false;
  }
  if (candidate.preset < 0 || candidate.preset >= matter::RADIO_PRESET_COUNT ||
      (candidate.band != 868 && candidate.band != 900) ||
      matter::radioBand(matter::RADIO_PRESETS[candidate.preset].frequency) != candidate.band) {
    reason = "Select a country / preset that belongs to the chosen band.";
    return false;
  }
  if (candidate.pathBytes < 1 || candidate.pathBytes > 3) {
    reason = "Path hash size must be 1, 2 or 3 bytes.";
    return false;
  }
  if (candidate.timeZone < 0 || candidate.timeZone >= TIME_ZONE_COUNT) {
    reason = "Select a valid time zone.";
    return false;
  }
  return true;
}
static void handleSave() {
  if (status.ready) {
    sendReady();
    return;
  }
  SetupPortalSettings candidate;
  candidate.ssid = server.arg("ssid");
  candidate.password = server.arg("password");
  candidate.name = server.arg("name");
  candidate.name.trim();
  candidate.band = server.arg("band").toInt();
  candidate.preset = server.arg("preset").toInt();
  candidate.pathBytes = server.arg("pathBytes").toInt();
  candidate.timeZone = server.arg("timeZone").toInt();
  String reason;
  if (!valid(candidate, reason) || !saveCallback || !saveCallback(candidate, reason)) {
    settings = candidate;
    settings.password = "";
    if (!reason.length())
      reason = "Configuration could not be saved. Check storage and try again.";
    sendForm(400, reason);
    return;
  }
  settings = candidate;
  settings.password = "";
  status.ready = true;
  status.message = "Configuration saved / reconnect or restart";
  status.revision++;
  sendReady();
}
static void redirectHome() {
  headers();
  server.sendHeader("Location", "http://192.168.4.1/");
  server.send(302, "text/plain", "Open the MatterMesh setup page.");
}
bool setupPortalStart(const SetupPortalSettings& current, SetupPortalSaveCallback save,
                      SetupPortalReconnectCallback reconnect) {
  if (status.active)
    return true;
  settings = current;
  settings.password = "";
  saveCallback = save;
  reconnectCallback = reconnect;
  restartAt = reconnectAt = 0;
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  String apPassword = "12345678";
  String apSsid = String("MatterMesh-Setup-") + suffix;
  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 4, 1), mask(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, mask);
  if (!WiFi.softAP(apSsid.c_str(), apPassword.c_str(), 1, false, 2)) {
    WiFi.softAPdisconnect(false);
    saveCallback = nullptr;
    reconnectCallback = nullptr;
    return false;
  }
  dns.start(53, "*", ip);
  server.on("/", HTTP_GET, [] {
    if (status.ready)
      sendReady();
    else
      sendForm(200);
  });
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reconnect", HTTP_POST, [] {
    if (!status.ready) {
      redirectHome();
      return;
    }
    String html = pageHead("Reconnecting");
    html += F("</h1><p>The setup hotspot is closing and the device is joining your Wi-Fi. "
              "Reconnect your phone to that network.</p></section></main></body></html>");
    headers();
    server.send(200, "text/html", html);
    reconnectAt = millis() + 750;
  });
  server.on("/restart", HTTP_POST, [] {
    if (!status.ready) {
      redirectHome();
      return;
    }
    String html = pageHead("Restarting");
    html += F("</h1><p>The device is restarting. You can close this "
              "page.</p></section></main></body></html>");
    headers();
    server.send(200, "text/html", html);
    restartAt = millis() + 750;
  });
  server.onNotFound(redirectHome);
  server.begin();
  uint32_t revision = status.revision + 1;
  status = SetupPortalStatus();
  status.active = true;
  status.revision = revision;
  status.ssid = apSsid;
  status.password = apPassword;
  status.address = "http://" + WiFi.softAPIP().toString();
  status.message = "Open the setup page on your phone";
  return true;
}
void setupPortalLoop() {
  if (!status.active)
    return;
  dns.processNextRequest();
  server.handleClient();
  if (restartAt && int32_t(millis() - restartAt) >= 0)
    ESP.restart();
  if (reconnectAt && int32_t(millis() - reconnectAt) >= 0) {
    reconnectAt = 0;
    auto callback = reconnectCallback;
    reconnectCallback = nullptr;
    if (callback)
      callback();
  }
}
void setupPortalStop() {
  // Stop only the AP interface. The caller selects STA or WIFI_OFF without an AP->OFF->STA
  // deinit/reinit race.
  if (!status.active)
    return;
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(false);
  saveCallback = nullptr;
  reconnectCallback = nullptr;
  settings.password = "";
  restartAt = reconnectAt = 0;
  uint32_t revision = status.revision + 1;
  status = SetupPortalStatus();
  status.revision = revision;
}
void setupPortalRestart() {
  if (status.ready)
    restartAt = millis() + 400;
}
void setupPortalReconnect() {
  if (status.ready)
    reconnectAt = millis() + 400;
}
SetupPortalStatus setupPortalStatus() {
  return status;
}
} // namespace device
