#include "network_wake.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include "config.h"

static WiFiUDP udpPort9;
static WiFiUDP udpPort7;
static bool udpStarted = false;

static volatile bool g_wolSeenEver = false;
static volatile unsigned long g_wolLastSeen = 0;

static unsigned long g_lastReconnectCheck = 0;
static const unsigned long RECONNECT_INTERVAL_MS = 10000;
static bool g_wasConnected = false;

static uint8_t g_defaultMac[6] = {0};
static bool g_defaultMacCached = false;

static void cacheDefaultMac() {
  if (!g_defaultMacCached) {
    esp_read_mac(g_defaultMac, ESP_MAC_WIFI_STA);
    g_defaultMacCached = true;
    Serial.printf("[WOL ] Factory ESP32 STA MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  g_defaultMac[0], g_defaultMac[1], g_defaultMac[2],
                  g_defaultMac[3], g_defaultMac[4], g_defaultMac[5]);
  }
}

static bool parseMac(const String &str, uint8_t mac[6]) {
  int values[6];
  if (sscanf(str.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) != 6) {
    if (sscanf(str.c_str(), "%x-%x-%x-%x-%x-%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6) {
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
  if (packetSize < 102) return false;

  uint8_t buf[128];
  int readLen = udp.read(buf, sizeof(buf));
  if (readLen < 102) return false;

  // Header must be 6 bytes of 0xFF
  for (int i = 0; i < 6; i++) {
    if (buf[i] != 0xFF) return false;
  }

  // Followed by 16 repetitions of the target MAC
  for (int rep = 0; rep < 16; rep++) {
    if (memcmp(buf + 6 + (rep * 6), targetMac, 6) != 0) {
      return false;
    }
  }

  return true;
}

static void startUdpListeners() {
  if (udpStarted) return;
  udpPort9.begin(9);
  udpPort7.begin(7);
  udpStarted = true;
  Serial.println("[WOL ] UDP listeners started on ports 9 and 7");
}

static void stopUdpListeners() {
  if (!udpStarted) return;
  udpPort9.stop();
  udpPort7.stop();
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
  WiFi.setAutoReconnect(true);

  uint8_t targetMac[6];
  memcpy(targetMac, g_defaultMac, 6);
  if (isOff && !config.bc250Mac.isEmpty() && parseMac(config.bc250Mac, targetMac)) {
    Serial.printf("[WOL ] Adopting Wi-Fi dongle MAC for STATE_OFF: %s\n", config.bc250Mac.c_str());
  } else {
    Serial.printf("[WOL ] Using factory ESP32 MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  g_defaultMac[0], g_defaultMac[1], g_defaultMac[2],
                  g_defaultMac[3], g_defaultMac[4], g_defaultMac[5]);
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

  g_wasConnected = false;
  return true;
}

void networkWakeOnStateChanged(bool isOff) {
  if (!config.wolEnabled || config.wifiSsid.isEmpty()) {
    return;
  }
  cacheDefaultMac();

  uint8_t targetMac[6];
  memcpy(targetMac, g_defaultMac, 6);
  if (isOff && !config.bc250Mac.isEmpty() && parseMac(config.bc250Mac, targetMac)) {
    Serial.printf("[WOL ] Power transition -> adopting Wi-Fi dongle MAC: %s\n", config.bc250Mac.c_str());
  } else {
    Serial.printf("[WOL ] Power transition -> restoring factory ESP32 MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  g_defaultMac[0], g_defaultMac[1], g_defaultMac[2],
                  g_defaultMac[3], g_defaultMac[4], g_defaultMac[5]);
  }

  uint8_t currentMac[6];
  if (esp_wifi_get_mac(WIFI_IF_STA, currentMac) == ESP_OK) {
    if (memcmp(currentMac, targetMac, 6) == 0) {
      Serial.println("[WOL ] Wi-Fi Station MAC already matches target; no switch needed");
      return;
    }
  }

  Serial.println("[WOL ] Disconnecting Wi-Fi to swap MAC address...");
  stopUdpListeners();
  MDNS.end();
  WiFi.disconnect(true);
  delay(50);

  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, targetMac);
  if (err != ESP_OK) {
    Serial.printf("[WOL ] Failed to set MAC: %d\n", err);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (config.wifiPassword.isEmpty()) {
    WiFi.begin(config.wifiSsid.c_str());
  } else {
    WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  }
  g_wasConnected = false;
  g_lastReconnectCheck = millis();
}

void networkWakeLoop() {
  if (!config.wolEnabled || config.wifiSsid.isEmpty()) {
    return;
  }

  unsigned long now = millis();
  bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !g_wasConnected) {
    g_wasConnected = true;
    Serial.printf("[WOL ] Wi-Fi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    if (MDNS.begin("bc250-switch")) {
      MDNS.addService("http", "tcp", 80);
      Serial.println("[MDNS] Responding at http://bc250-switch.local");
    } else {
      Serial.println("[MDNS] Failed to start responder");
    }
    startUdpListeners();
  } else if (!connected && g_wasConnected) {
    g_wasConnected = false;
    Serial.println("[WOL ] Wi-Fi disconnected");
    MDNS.end();
    stopUdpListeners();
  }

  if (!connected) {
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

  uint8_t targetMac[6];
  if (!parseMac(config.bc250Mac, targetMac)) {
    return;
  }

  if (checkMagicPacket(udpPort9, targetMac) || checkMagicPacket(udpPort7, targetMac)) {
    Serial.println("[WOL ] Valid Magic Packet received for BC250!");
    g_wolLastSeen = now;
    g_wolSeenEver = true;
  }
}

bool networkWakeSeen(unsigned long *lastSeenMs) {
  if (lastSeenMs) *lastSeenMs = g_wolLastSeen;
  if (g_wolSeenEver) {
    g_wolSeenEver = false;
    return true;
  }
  return false;
}

bool networkWakeIsConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

String networkWakeIp() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "";
}
