#include "portal.h"

#include "mbedtls/sha256.h"
#include "portal_html.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_random.h>

#include "board.h"
#include "config.h"
#include "version.h"
#include "wake.h"
#include <Update.h>
#if !defined(WAKE_BLE)
#include "bt_classic.h"
#endif

static const size_t MIN_PASSWORD_LEN = 6;

static AsyncWebServer server(80);

// Single-session bearer token, handed out on password set / login and required
// by the protected endpoints. Regenerated each set/login; "" means no session.
static String g_token;

// Deferred restart (so the HTTP response can flush before we reboot).
static unsigned long g_restartAt = 0;
static bool g_portalSetupMode = false;

// --- Helpers ---

static String sha256hex(const String &s) {
  uint8_t out[32];
  mbedtls_sha256((const unsigned char *)s.c_str(), s.length(), out, 0);
  char hex[65];
  for (int i = 0; i < 32; i++)
    sprintf(hex + i * 2, "%02x", out[i]);
  hex[64] = 0;
  return String(hex);
}

static String makeToken() {
  char buf[33];
  for (int i = 0; i < 4; i++)
    sprintf(buf + i * 8, "%08x", (unsigned)esp_random());
  buf[32] = 0;
  return String(buf);
}

static bool authed(AsyncWebServerRequest *req) {
  if (g_token.isEmpty())
    return false;
  if (!req->hasHeader("X-Auth-Token"))
    return false;
  return req->header("X-Auth-Token") == g_token;
}

static bool isBoardPowered() {
  const char *st = getBoardStateName();
  return (strcmp(st, "ON") == 0 || strcmp(st, "BOOTING") == 0);
}

static void sendJsonError(AsyncWebServerRequest *req, int code,
                          const char *msg) {
  JsonDocument doc;
  doc["error"] = msg;
  String out;
  serializeJson(doc, out);
  req->send(code, "application/json", out);
}

static void serveIndex(AsyncWebServerRequest *req) {
  AsyncWebServerResponse *response =
      req->beginResponse(200, "text/html", INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
  response->addHeader("Content-Encoding", "gzip");
  req->send(response);
}

// --- Endpoint handlers ---

static void handleStatus(AsyncWebServerRequest *req) {
  JsonDocument doc;
  doc["version"] = FW_VERSION;
  doc["target"] = FW_TARGET;
  doc["passwordSet"] = config.passHash.length() > 0;
  doc["wakeAddr"] = config.wakeAddr;
  doc["hostAddr"] = config.hostAddr;
  doc["configured"] = isConfigured();
  doc["setupMode"] = g_portalSetupMode;
  doc["mode"] = wakeModeName();
  doc["needsHost"] = wakeNeedsHostAddr();
  doc["wolEnabled"] = config.wolEnabled;
  doc["wifiSsid"] = config.wifiSsid;
  doc["hasWifiPassword"] = !config.wifiPassword.isEmpty();
  doc["bc250Mac"] = config.bc250Mac;
  doc["hostname"] = config.hostname.isEmpty() ? "bc-awake" : config.hostname;
  doc["boardState"] = getBoardStateName();
  doc["gracefulShutdown"] = config.gracefulShutdown;
#if defined(BOARD_ESP32CAM)
  doc["isCam"] = true;
  doc["flashOnWake"] = config.flashOnWake;
#else
  doc["isCam"] = false;
#endif

  JsonArray arr = doc["controllers"].to<JsonArray>();
  for (const auto &c : config.controllers) {
    JsonObject o = arr.add<JsonObject>();
    o["mac"] = c.mac;
    o["name"] = c.name;
  }

  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

static void handlePassword(AsyncWebServerRequest *req, JsonVariant &json) {
  JsonObject o = json.as<JsonObject>();
  String pw = o["password"] | "";
  if (pw.length() < MIN_PASSWORD_LEN) {
    sendJsonError(req, 400, "password too short");
    return;
  }
  // Setting the first password is open; changing an existing one needs a
  // session.
  if (config.passHash.length() > 0 && !authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  setPassHash(sha256hex(pw));
  g_token = makeToken();
  Serial.println("[PORTAL] password set");

  JsonDocument doc;
  doc["token"] = g_token;
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

static void handleLogin(AsyncWebServerRequest *req, JsonVariant &json) {
  if (config.passHash.isEmpty()) {
    sendJsonError(req, 400, "no password set");
    return;
  }
  JsonObject o = json.as<JsonObject>();
  String pw = o["password"] | "";
  if (sha256hex(pw) != config.passHash) {
    sendJsonError(req, 401, "wrong password");
    return;
  }
  g_token = makeToken();
  JsonDocument doc;
  doc["token"] = g_token;
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

static void handleDevices(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (isBoardPowered()) {
    sendJsonError(
        req, 409,
        "Bluetooth sensing disabled while host machine is powered on");
    return;
  }

  static const int MAX_OUT = 48;
  WakeDev snap[MAX_OUT];
  int n = wakeSnapshotDevices(snap, MAX_OUT);

  JsonDocument doc;
  JsonArray arr = doc["devices"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["addr"] = snap[i].addr;
    o["name"] = snap[i].name;
    o["rssi"] = snap[i].rssi;
    o["cod"] = snap[i].cod;
    o["paged"] = snap[i].paged;
  }
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

// Validate + normalise a posted MAC. Returns false (and replies 400) if bad.
static bool readMac(AsyncWebServerRequest *req, JsonVariant &json,
                    String &addr) {
  JsonObject o = json.as<JsonObject>();
  addr = o["addr"] | "";
  addr.trim();
  addr.toLowerCase();
  if (!wakeValidMac(addr)) {
    sendJsonError(req, 400, "invalid address");
    return false;
  }
  return true;
}

static void handleHost(AsyncWebServerRequest *req, JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  String addr;
  if (!readMac(req, json, addr))
    return;
  if (wakeNeedsHostAddr()) {
    wakeSetHostAddr(addr);
    Serial.printf("[PORTAL] host adapter %s\n", addr.c_str());
  }
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleSelect(AsyncWebServerRequest *req, JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  String addr;
  if (!readMac(req, json, addr))
    return;
  setWakeAddr(addr);
  wakeUpdateControllers();
  Serial.printf("[PORTAL] bound controller %s\n", addr.c_str());
  req->send(200, "application/json", "{\"ok\":true}");
}

static const unsigned long PAIRING_TIMEOUT_MS = 60000;
static unsigned long g_pairingTimeoutAt = 0;
static bool g_pairingActive = false;
static bool g_pairingTimedOut = false;

bool portalIsPairingActive() {
  if (!g_pairingActive)
    return false;
  if ((long)(millis() - g_pairingTimeoutAt) >= 0) {
    g_pairingActive = false;
    g_pairingTimedOut = true;
#if !defined(WAKE_BLE)
    btListenerStop();
#endif
    Serial.println("[PORTAL] pairing listener timed out (60s)");
    return false;
  }
  return true;
}

void portalPairingStop() {
  g_pairingActive = false;
  g_pairingTimeoutAt = 0;
#if !defined(WAKE_BLE)
  btListenerStop();
#endif
  Serial.println("[PORTAL] pairing listener stopped");
}

static void handleListenerStart(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (isBoardPowered()) {
    sendJsonError(
        req, 409,
        "Bluetooth listener disabled while host machine is powered on");
    return;
  }
  g_pairingActive = true;
  g_pairingTimedOut = false;
  g_pairingTimeoutAt = millis() + PAIRING_TIMEOUT_MS;
#if !defined(WAKE_BLE)
  btListenerStart();
#endif
#if defined(BOARD_ESP32CAM)
  if (config.flashOnWake) {
    triggerFlashBurst(3, 60, 60);
  }
#endif
  Serial.println("[PORTAL] pairing listener started (60s timeout)");
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleListenerStop(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  portalPairingStop();
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleListenerStatus(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  JsonDocument doc;
  bool active = portalIsPairingActive();
  doc["active"] = active;
  doc["timedOut"] = g_pairingTimedOut;
#if !defined(WAKE_BLE)
  BtListenerResult res = btListenerGetStatus();
  doc["detected"] = res.detected;
  if (res.detected) {
    doc["mac"] = res.addr;
    String name = res.name;
    if (name.isEmpty()) {
      name = btResolveHeuristicName(res.addr, res.cod);
    }
    doc["name"] = name;
    bool already = false;
    for (const auto &c : config.controllers) {
      if (c.mac.equalsIgnoreCase(res.addr)) {
        already = true;
        break;
      }
    }
    doc["alreadyBound"] = already;
  }
#else
  doc["detected"] = false;
#endif
  String out;
  serializeJson(doc, out);
  req->send(200, "application/json", out);
}

static void handleControllersAdd(AsyncWebServerRequest *req,
                                 JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  JsonObject o = json.as<JsonObject>();
  String mac = o["mac"] | o["addr"] | "";
  String name = o["name"] | "";
  mac.trim();
  mac.toLowerCase();
  if (!wakeValidMac(mac)) {
    sendJsonError(req, 400, "invalid address");
    return;
  }
  if (config.controllers.size() >= MAX_CONTROLLERS) {
    sendJsonError(req, 400, "controller limit reached (max 8)");
    return;
  }
  if (!addController(mac, name)) {
    sendJsonError(req, 400, "could not add controller");
    return;
  }
  wakeUpdateControllers();
  portalPairingStop();
  armWakeCooldown(WAKE_WAKE_COOLDOWN_MS);
  Serial.printf("[PORTAL] added controller %s (%s)\n", mac.c_str(),
                name.c_str());
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleControllersRemove(AsyncWebServerRequest *req,
                                    JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  JsonObject o = json.as<JsonObject>();
  String mac = o["mac"] | o["addr"] | "";
  mac.trim();
  mac.toLowerCase();
  if (!removeController(mac)) {
    sendJsonError(req, 404, "controller not found");
    return;
  }
  wakeUpdateControllers();
  Serial.printf("[PORTAL] removed controller %s\n", mac.c_str());
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleControllersRename(AsyncWebServerRequest *req,
                                    JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  JsonObject o = json.as<JsonObject>();
  String mac = o["mac"] | o["addr"] | "";
  String name = o["name"] | "";
  mac.trim();
  mac.toLowerCase();
  name.trim();
  if (name.isEmpty()) {
    sendJsonError(req, 400, "name cannot be empty");
    return;
  }
  if (!renameController(mac, name)) {
    sendJsonError(req, 404, "controller not found");
    return;
  }
  Serial.printf("[PORTAL] renamed controller %s to %s\n", mac.c_str(),
                name.c_str());
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleSettings(AsyncWebServerRequest *req, JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
#if defined(BOARD_ESP32CAM)
  JsonObject o = json.as<JsonObject>();
  if (o["flashOnWake"].is<bool>()) {
    setFlashOnWake(o["flashOnWake"].as<bool>());
    Serial.printf("[PORTAL] flashOnWake=%d\n", config.flashOnWake ? 1 : 0);
  }
#endif
  req->send(200, "application/json", "{\"ok\":true}");
}

// Persist the optional graceful-shutdown (PWR_BTN# mirroring) setting.
static void handlePower(AsyncWebServerRequest *req, JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  JsonObject o = json.as<JsonObject>();
  if (!o["gracefulShutdown"].is<bool>()) {
    sendJsonError(req, 400, "gracefulShutdown (boolean) required");
    return;
  }
  setGracefulShutdown(o["gracefulShutdown"].as<bool>());
  Serial.printf("[PORTAL] gracefulShutdown=%d\n",
                config.gracefulShutdown ? 1 : 0);
  req->send(200, "application/json", "{\"ok\":true}");
}

// Send a 300 ms PWR_BTN# pulse so the board shuts down as if its button were
// tapped.
static void handleShutdown(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (!requestGracefulShutdown()) {
    sendJsonError(req, 409,
                  config.gracefulShutdown ? "board is not ON"
                                          : "graceful shutdown is disabled");
    return;
  }
  Serial.println("[PORTAL] graceful shutdown requested");
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handlePowerOn(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (!requestPowerOn()) {
    sendJsonError(req, 409, "board is not OFF");
    return;
  }
  Serial.println("[PORTAL] power on requested");
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handlePowerOff(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (!requestPowerOff()) {
    sendJsonError(req, 409, "board is already OFF");
    return;
  }
  Serial.println("[PORTAL] force power off requested");
  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleNetwork(AsyncWebServerRequest *req, JsonVariant &json) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (isBoardPowered()) {
    sendJsonError(
        req, 403,
        "Network settings are read-only while host machine is powered on");
    return;
  }
  JsonObject o = json.as<JsonObject>();

  bool wolEnabled = config.wolEnabled;
  if (o["wolEnabled"].is<bool>()) {
    wolEnabled = o["wolEnabled"].as<bool>();
    setWolEnabled(wolEnabled);
  }

  String ssid = config.wifiSsid;
  if (o["wifiSsid"].is<const char *>()) {
    ssid = o["wifiSsid"].as<String>();
    ssid.trim();
  }

  String pass = config.wifiPassword;
  if (o["wifiPassword"].is<const char *>()) {
    pass = o["wifiPassword"].as<String>();
  }

  String mac = config.bc250Mac;
  if (o["bc250Mac"].is<const char *>()) {
    mac = o["bc250Mac"].as<String>();
    mac.trim();
    mac.toLowerCase();
    if (!mac.isEmpty() && !wakeValidMac(mac)) {
      sendJsonError(req, 400, "invalid BC250 Ethernet MAC address");
      return;
    }
  }

  String host = config.hostname;
  if (o["hostname"].is<const char *>()) {
    host = o["hostname"].as<String>();
    host.trim();
    host.toLowerCase();
    if (host.isEmpty()) {
      host = "bc-awake";
    }
    if (!isValidHostname(host)) {
      sendJsonError(req, 400,
                    "invalid hostname: must be 1-32 characters of lowercase "
                    "alphanumeric and hyphens, no leading/trailing hyphen");
      return;
    }
  }

  setWifiConfig(ssid, pass, mac, host);
  Serial.printf("[PORTAL] network config updated: wolEnabled=%d ssid='%s' "
                "passLen=%d mac='%s' hostname='%s'\n",
                wolEnabled, ssid.c_str(), (int)pass.length(), mac.c_str(),
                host.c_str());

  req->send(200, "application/json", "{\"ok\":true}");
}

static void handleFinish(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (config.passHash.isEmpty()) {
    sendJsonError(req, 400, "password not set");
    return;
  }
  setForceSetup(false);
  req->send(200, "application/json", "{\"ok\":true}");
  Serial.println("[PORTAL] setup complete, restarting into normal mode");
  g_restartAt = millis() + 800;
}

static void handleReset(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  g_token = "";
  resetConfig();
  req->send(200, "application/json", "{\"ok\":true}");
  Serial.println(
      "[PORTAL] configuration reset to defaults, restarting into setup mode");
  g_restartAt = millis() + 800;
}

static void handleUpdateResponse(AsyncWebServerRequest *req) {
  if (!authed(req)) {
    sendJsonError(req, 401, "unauthorized");
    return;
  }
  if (isBoardPowered()) {
    if (Update.isRunning()) {
      Update.abort();
    }
    sendJsonError(req, 409,
                  "Firmware update disabled while host machine is powered on");
    return;
  }
  if (Update.hasError() || !Update.isFinished()) {
    Update.abort();
    sendJsonError(req, 500, "firmware update failed");
  } else {
    req->send(200, "application/json", "{\"ok\":true}");
    Serial.println("[OTA ] Firmware update complete! Restarting in 800ms...");
    g_restartAt = millis() + 800;
  }
}

static void handleUpdateUpload(AsyncWebServerRequest *req, String filename,
                               size_t index, uint8_t *data, size_t len,
                               bool final) {
  if (!authed(req))
    return;

  if (isBoardPowered()) {
    if (Update.isRunning()) {
      Update.abort();
    }
    return;
  }

  if (index == 0) {
    // Suspend Bluetooth discovery to free heap and avoid SPI cache contention
    wakeSetDiscovery(false);
    Serial.printf("[OTA ] Starting firmware update: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
    }
  }

  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
  }

  if (final) {
    if (Update.end(true)) {
      Serial.printf("[OTA ] Firmware update written successfully: %u bytes\n",
                    (unsigned)(index + len));
    } else {
      Update.printError(Serial);
    }
  }
}

// --- Lifecycle ---

static bool g_routesRegistered = false;
static bool g_serverStarted = false;

static void registerRoutes() {
  if (g_routesRegistered)
    return;
  g_routesRegistered = true;

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/devices", HTTP_GET, handleDevices);
  server.on("/api/bt/devices", HTTP_GET, handleDevices);
  server.on("/api/ble/devices", HTTP_GET, handleDevices);
  server.on("/api/finish", HTTP_POST, handleFinish);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/shutdown", HTTP_POST, handleShutdown);
  server.on("/api/power/on", HTTP_POST, handlePowerOn);
  server.on("/api/power/off", HTTP_POST, handlePowerOff);
  server.on("/api/update", HTTP_POST, handleUpdateResponse, handleUpdateUpload);
  server.on("/api/listener/start", HTTP_POST, handleListenerStart);
  server.on("/api/listener/stop", HTTP_POST, handleListenerStop);
  server.on("/api/listener/status", HTTP_GET, handleListenerStatus);
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/password", handlePassword));
  server.addHandler(new AsyncCallbackJsonWebHandler("/api/login", handleLogin));
  server.addHandler(new AsyncCallbackJsonWebHandler("/api/host", handleHost));
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/bt/host", handleHost));
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/select", handleSelect));
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/bt/select", handleSelect));
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/ble/select", handleSelect));
  server.addHandler(new AsyncCallbackJsonWebHandler("/api/controllers/add",
                                                    handleControllersAdd));
  server.addHandler(new AsyncCallbackJsonWebHandler("/api/controllers/remove",
                                                    handleControllersRemove));
  server.addHandler(new AsyncCallbackJsonWebHandler("/api/controllers/rename",
                                                    handleControllersRename));
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/settings", handleSettings));
  server.addHandler(
      new AsyncCallbackJsonWebHandler("/api/network", handleNetwork));
  server.addHandler(new AsyncCallbackJsonWebHandler("/api/power", handlePower));

  server.on("/", HTTP_GET, serveIndex);
  server.on("/index.html", HTTP_GET, serveIndex);
  server.onNotFound([](AsyncWebServerRequest *req) {
    if (req->url().startsWith("/api/")) {
      sendJsonError(req, 404, "not found");
    } else {
      req->send(404, "text/plain", "Not Found");
    }
  });
}

void portalServerBegin() {
  if (g_serverStarted)
    return;
  if (WiFi.getMode() == WIFI_OFF) {
    return;
  }
  registerRoutes();
  server.begin();
  g_serverStarted = true;
  Serial.println("[PORTAL] web server up on port 80");
}

void portalCheckRestart() {
  if (g_restartAt && (int32_t)(millis() - g_restartAt) >= 0) {
    ESP.restart();
  }
}

void portalBegin() {
  g_portalSetupMode = true;
  Serial.println("=== BC250 PSU controller : SETUP MODE ===");

  // Keep the PSU off and the button readable while configuring.
  pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(PS_ON_PIN, PS_ON_RELEASE);
  if (BUTTON_GND >= 0) {
    pinMode(BUTTON_GND, OUTPUT);
    digitalWrite(BUTTON_GND, LOW);
  }
  pinMode(BUTTON_SENSE, INPUT_PULLUP);

  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char apSsid[32];
  snprintf(apSsid, sizeof(apSsid), "%s%02X%02X%02X", AP_SSID_PREFIX, mac[3],
           mac[4], mac[5]);

  bool ok = WiFi.softAP(apSsid); // open network
  // Must be set AFTER softAP().
  WiFi.setTxPower(AP_TX_POWER);
  Serial.printf("[PORTAL] softAP ret=%d ssid='%s' ip=%s txpwr=%d\n", ok, apSsid,
                WiFi.softAPIP().toString().c_str(), WiFi.getTxPower());

  // Start device discovery for the picker
  wakeSetDiscovery(true);

  portalServerBegin();
  Serial.println("[PORTAL] connect to AP and open http://192.168.4.1");
}

void portalLoop() {
  wakeLoop();

  // Button held in setup mode = escape back to normal mode (only useful once
  // configured; an unconfigured device just re-enters setup).
  static unsigned long pressStart = 0;
  static bool wasDown = false;
  bool down = (digitalRead(BUTTON_SENSE) == LOW);
  if (down && !wasDown) {
    pressStart = millis();
  } else if (down && (millis() - pressStart) >= SETUP_HOLD_MS) {
    Serial.println("[PORTAL] button held, leaving setup mode");
    setForceSetup(false);
    delay(50);
    ESP.restart();
  }
  wasDown = down;

  portalCheckRestart();
}
