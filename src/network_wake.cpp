#include "network_wake.h"

#include "board.h"
#include "config.h"
#include "portal.h"
#include "version.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_mac.h>
#include <esp_wifi.h>

static WiFiUDP udpPort9;
static WiFiUDP udpPort7;
static WiFiUDP udpDiscovery;
static const uint16_t DISCOVERY_PORT = 25025;
static bool udpStarted = false;

static volatile bool g_wolSeenEver = false;
static volatile unsigned long g_wolLastSeen = 0;

static unsigned long g_lastReconnectCheck = 0;
static const unsigned long RECONNECT_INTERVAL_MS = 10000;
static bool g_wasConnected = false;
static unsigned long g_connectStart = 0;
static bool g_fallbackApActive = false;
static const unsigned long WIFI_FALLBACK_TIMEOUT_MS = 20000;

static uint8_t g_defaultMac[6] = {0};
static bool g_defaultMacCached = false;

static const unsigned long MAC_SWAP_DEFER_MS = 5000;
static bool g_pendingMacSwap = false;
static bool g_pendingMacSwapIsOff = false;
static unsigned long g_pendingMacSwapAt = 0;

static void cacheDefaultMac() {
  if (!g_defaultMacCached) {
    esp_read_mac(g_defaultMac, ESP_MAC_WIFI_STA);
    g_defaultMacCached = true;
    Serial.printf(
        "[WOL ] Factory ESP32 STA MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        g_defaultMac[0], g_defaultMac[1], g_defaultMac[2], g_defaultMac[3],
        g_defaultMac[4], g_defaultMac[5]);
  }
}

static bool parseMac(const String &str, uint8_t mac[6]) {
  int values[6];
  if (sscanf(str.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1],
             &values[2], &values[3], &values[4], &values[5]) != 6) {
    if (sscanf(str.c_str(), "%x-%x-%x-%x-%x-%x", &values[0], &values[1],
               &values[2], &values[3], &values[4], &values[5]) != 6) {
      return false;
    }
  }
  for (int i = 0; i < 6; i++) {
    mac[i] = (uint8_t)values[i];
  }
  return true;
}

static bool checkMagicPacket(WiFiUDP &udp, const uint8_t targetMac[6]) {
  int packetSize = udp.parsePacket();
  if (packetSize < 102)
    return false;

  uint8_t buf[128];
  int readLen = udp.read(buf, sizeof(buf));
  if (readLen < 102)
    return false;

  // Header must be 6 bytes of 0xFF
  for (int i = 0; i < 6; i++) {
    if (buf[i] != 0xFF)
      return false;
  }

  // Followed by 16 repetitions of the target MAC
  for (int rep = 0; rep < 16; rep++) {
    if (memcmp(buf + 6 + (rep * 6), targetMac, 6) != 0) {
      return false;
    }
  }

  return true;
}

static void checkDiscoveryPacket() {
  int packetSize = udpDiscovery.parsePacket();
  if (packetSize <= 0)
    return;

  char buf[128];
  int readLen = udpDiscovery.read(buf, sizeof(buf) - 1);
  if (readLen <= 0)
    return;
  buf[readLen] = '\0';

  int start = 0;
  while (start < readLen && isspace((unsigned char)buf[start]))
    start++;
  int end = readLen - 1;
  while (end >= start && isspace((unsigned char)buf[end])) {
    buf[end] = '\0';
    end--;
  }

  if (strcmp(buf + start, "BC250_DISCOVER") == 0) {
    uint8_t curMac[6] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, curMac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", curMac[0],
             curMac[1], curMac[2], curMac[3], curMac[4], curMac[5]);

    const char *host =
        config.hostname.isEmpty() ? "bc-awake" : config.hostname.c_str();

    JsonDocument doc;
    doc["proto"] = "v1";
    doc["device"] = "bc-awake";
    doc["hostname"] = host;
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = macStr;
    doc["boardState"] = getBoardStateName();
    doc["version"] = FW_VERSION;
    doc["port"] = 80;

    String reply;
    serializeJson(doc, reply);

    udpDiscovery.beginPacket(udpDiscovery.remoteIP(),
                             udpDiscovery.remotePort());
    udpDiscovery.write((const uint8_t *)reply.c_str(), reply.length());
    udpDiscovery.endPacket();

    Serial.printf("[DISC] Discovery probe answered to %s:%u\n",
                  udpDiscovery.remoteIP().toString().c_str(),
                  udpDiscovery.remotePort());
  }
}

static void startUdpListeners() {
  if (udpStarted)
    return;
  udpPort9.begin(9);
  udpPort7.begin(7);
  udpDiscovery.begin(DISCOVERY_PORT);
  udpStarted = true;
  Serial.printf("[WOL ] UDP listeners started on ports 9, 7 and %u\n",
                DISCOVERY_PORT);
}

static void stopUdpListeners() {
  if (!udpStarted)
    return;
  udpPort9.stop();
  udpPort7.stop();
  udpDiscovery.stop();
  udpStarted = false;
  Serial.println("[WOL ] UDP listeners stopped");
}

bool networkWakeBegin(bool isOff) {
  cacheDefaultMac();

  if (!config.wolEnabled) {
    Serial.println("[WOL ] Network wake disabled; Wi-Fi station off");
    WiFi.mode(WIFI_OFF);
    return false;
  }

  if (config.wifiSsid.isEmpty()) {
    Serial.println("[WOL ] Network wake enabled but no Wi-Fi SSID configured");
    WiFi.mode(WIFI_OFF);
    return false;
  }

  WiFi.mode(WIFI_STA);
  esp_wifi_set_max_tx_power((int8_t)AP_TX_POWER);
  WiFi.setAutoReconnect(true);
  const char *host =
      config.hostname.isEmpty() ? "bc-awake" : config.hostname.c_str();
  WiFi.setHostname(host);

  uint8_t targetMac[6];
  memcpy(targetMac, g_defaultMac, 6);
  if (isOff && !config.bc250Mac.isEmpty() &&
      parseMac(config.bc250Mac, targetMac)) {
    Serial.printf("[WOL ] Adopting Wi-Fi dongle MAC for STATE_OFF: %s\n",
                  config.bc250Mac.c_str());
  } else {
    Serial.printf(
        "[WOL ] Using factory ESP32 MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        g_defaultMac[0], g_defaultMac[1], g_defaultMac[2], g_defaultMac[3],
        g_defaultMac[4], g_defaultMac[5]);
  }

  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, targetMac);
  if (err != ESP_OK) {
    Serial.printf("[WOL ] esp_wifi_set_mac failed: %d\n", err);
  }

  Serial.printf("[WOL ] Initializing Wi-Fi station: SSID='%s'%s\n",
                config.wifiSsid.c_str(),
                config.wifiPassword.isEmpty() ? " (open network)" : "");

  if (config.wifiPassword.isEmpty()) {
    WiFi.begin(config.wifiSsid.c_str());
  } else {
    WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  }
  WiFi.setTxPower(AP_TX_POWER);

  g_wasConnected = false;
  g_connectStart = millis();
  g_fallbackApActive = false;
  g_pendingMacSwap = false;
  return true;
}

static void applyStateMacChange(bool isOff) {
  cacheDefaultMac();

  uint8_t targetMac[6];
  memcpy(targetMac, g_defaultMac, 6);
  if (isOff && !config.bc250Mac.isEmpty() &&
      parseMac(config.bc250Mac, targetMac)) {
    Serial.printf("[WOL ] Power transition -> adopting Wi-Fi dongle MAC: %s\n",
                  config.bc250Mac.c_str());
  } else {
    Serial.printf("[WOL ] Power transition -> restoring factory ESP32 MAC: "
                  "%02x:%02x:%02x:%02x:%02x:%02x\n",
                  g_defaultMac[0], g_defaultMac[1], g_defaultMac[2],
                  g_defaultMac[3], g_defaultMac[4], g_defaultMac[5]);
  }

  uint8_t currentMac[6];
  if (esp_wifi_get_mac(WIFI_IF_STA, currentMac) == ESP_OK) {
    if (memcmp(currentMac, targetMac, 6) == 0) {
      Serial.println(
          "[WOL ] Wi-Fi Station MAC already matches target; no switch needed");
      return;
    }
  }

  Serial.println("[WOL ] Disconnecting Wi-Fi to swap MAC address...");
  stopUdpListeners();
  MDNS.end();
  WiFi.disconnect(true);
  delay(50);

  networkWakeBegin(isOff);
  g_lastReconnectCheck = millis();
}

void networkWakeOnStateChanged(PowerState state) {
  if (!config.wolEnabled || config.wifiSsid.isEmpty()) {
    return;
  }
  if (state == STATE_BOOTING) {
    // Keep Wi-Fi completely quiet during ATX PSU turn-on and VRM inrush
    return;
  }

  cacheDefaultMac();

  bool isOff = (state == STATE_OFF);
  uint8_t targetMac[6];
  memcpy(targetMac, g_defaultMac, 6);
  if (isOff && !config.bc250Mac.isEmpty() &&
      parseMac(config.bc250Mac, targetMac)) {
    // targetMac is dongle MAC
  }

  uint8_t currentMac[6];
  if (esp_wifi_get_mac(WIFI_IF_STA, currentMac) == ESP_OK &&
      memcmp(currentMac, targetMac, 6) == 0) {
    g_pendingMacSwap = false;
    Serial.println(
        "[WOL ] Wi-Fi Station MAC already matches target; no switch needed");
    return;
  }

  if (state == STATE_ON) {
    if (!g_pendingMacSwap || g_pendingMacSwapIsOff) {
      g_pendingMacSwap = true;
      g_pendingMacSwapIsOff = false;
      g_pendingMacSwapAt = millis();
      Serial.printf("[WOL ] Deferring Wi-Fi MAC restore by %lu ms to avoid PSU "
                    "inrush overlap\n",
                    MAC_SWAP_DEFER_MS);
    }
    return;
  }

  g_pendingMacSwap = false;
  applyStateMacChange(true);
}

void networkWakeLoop() {
  if (!config.wolEnabled || config.wifiSsid.isEmpty()) {
    return;
  }

  unsigned long now = millis();
  if (g_pendingMacSwap && (now - g_pendingMacSwapAt >= MAC_SWAP_DEFER_MS)) {
    g_pendingMacSwap = false;
    applyStateMacChange(g_pendingMacSwapIsOff);
    now = millis();
  }

  bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !g_wasConnected) {
    g_wasConnected = true;
    Serial.printf("[WOL ] Wi-Fi connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());
    if (g_fallbackApActive && !config.forceSetup && isConfigured()) {
      Serial.println("[WOL ] Wi-Fi connected to station; disabling fallback SoftAP");
      WiFi.softAPdisconnect(true);
      g_fallbackApActive = false;
    }
    const char *host =
        config.hostname.isEmpty() ? "bc-awake" : config.hostname.c_str();
    if (MDNS.begin(host)) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[MDNS] Responding at http://%s.local\n", host);
    } else {
      Serial.println("[MDNS] Failed to start responder");
    }
    startUdpListeners();
    portalServerBegin();
  } else if (!connected && g_wasConnected) {
    g_wasConnected = false;
    g_connectStart = now;
    Serial.println("[WOL ] Wi-Fi disconnected");
    MDNS.end();
    stopUdpListeners();
  }

  if (!connected) {
    if (!g_fallbackApActive && (now - g_connectStart >= WIFI_FALLBACK_TIMEOUT_MS)) {
      g_fallbackApActive = true;
      Serial.println("[WOL ] Wi-Fi connection timed out (20s); activating fallback SoftAP");
      portalBegin();
    }
    if (now - g_lastReconnectCheck >= RECONNECT_INTERVAL_MS) {
      g_lastReconnectCheck = now;
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WOL ] Attempting non-blocking Wi-Fi reconnect...");
        WiFi.reconnect();
      }
    }
    return;
  }

  if (!udpStarted) {
    startUdpListeners();
  }

  checkDiscoveryPacket();

  uint8_t targetMac[6];
  if (!parseMac(config.bc250Mac, targetMac)) {
    return;
  }

  if (checkMagicPacket(udpPort9, targetMac) ||
      checkMagicPacket(udpPort7, targetMac)) {
    Serial.println("[WOL ] Valid Magic Packet received for BC250!");
    g_wolLastSeen = now;
    g_wolSeenEver = true;
  }
}

bool networkWakeSeen(unsigned long *lastSeenMs) {
  if (lastSeenMs)
    *lastSeenMs = g_wolLastSeen;
  if (g_wolSeenEver) {
    g_wolSeenEver = false;
    return true;
  }
  return false;
}

bool networkWakeIsConnected() { return (WiFi.status() == WL_CONNECTED); }

String networkWakeIp() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "";
}
