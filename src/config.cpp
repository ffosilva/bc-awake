#include "config.h"
#include <Preferences.h>
#include <ArduinoJson.h>

Config config{{}, "", "", "", false, true, false, "", "", "", "bc250-switch", false};

static Preferences prefs;
static const char *NS = "bc250";

static void saveControllersNvs() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto &c : config.controllers) {
    JsonObject obj = arr.add<JsonObject>();
    obj["mac"] = c.mac;
    obj["name"] = c.name;
  }
  String out;
  serializeJson(doc, out);

  prefs.begin(NS, false);
  prefs.putString("controllers", out);
  if (!config.controllers.empty()) {
    prefs.putString("wakeAddr", config.controllers[0].mac);
  } else {
    prefs.putString("wakeAddr", "");
  }
  prefs.end();
}

void loadConfig() {
  prefs.begin(NS, true);  // read-only
  config.hostAddr     = prefs.getString("hostAddr", "");
  config.passHash     = prefs.getString("passHash", "");
  config.forceSetup   = prefs.getBool("forceSetup", false);
  config.flashOnWake  = prefs.getBool("flashOnWake", true);
  config.wolEnabled   = prefs.getBool("wolEnabled", false);
  config.gracefulShutdown = prefs.getBool("gracefulShutdown", false);
  config.wifiSsid     = prefs.getString("wifiSsid", "");
  config.wifiPassword = prefs.getString("wifiPassword", "");
  config.bc250Mac     = prefs.getString("bc250Mac", "");
  config.hostname     = prefs.getString("hostname", "bc250-switch");
  if (config.hostname.isEmpty()) {
    config.hostname = "bc250-switch";
  }
  String controllersJson = prefs.getString("controllers", "");
  String legacyWakeAddr  = prefs.getString("wakeAddr", "");
  prefs.end();

  config.controllers.clear();
  if (!controllersJson.isEmpty()) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, controllersJson);
    if (!err && doc.is<JsonArray>()) {
      for (JsonObject obj : doc.as<JsonArray>()) {
        BoundController c;
        c.mac = obj["mac"] | "";
        c.name = obj["name"] | "";
        c.mac.trim();
        c.mac.toLowerCase();
        if (!c.mac.isEmpty()) {
          config.controllers.push_back(c);
          if (config.controllers.size() >= MAX_CONTROLLERS) break;
        }
      }
    }
  }

  // Automatic migration: if controllers array is empty and legacy wakeAddr exists
  if (config.controllers.empty() && !legacyWakeAddr.isEmpty()) {
    legacyWakeAddr.trim();
    legacyWakeAddr.toLowerCase();
    BoundController c;
    c.mac = legacyWakeAddr;
    c.name = "Controller 1";
    config.controllers.push_back(c);
    saveControllersNvs();
  }

  if (!config.controllers.empty()) {
    config.wakeAddr = config.controllers[0].mac;
  } else {
    config.wakeAddr = legacyWakeAddr;
  }
}

static void putString(const char *key, const String &val) {
  prefs.begin(NS, false);
  prefs.putString(key, val);
  prefs.end();
}

void setControllers(const std::vector<BoundController> &controllers) {
  config.controllers = controllers;
  if (config.controllers.size() > MAX_CONTROLLERS) {
    config.controllers.resize(MAX_CONTROLLERS);
  }
  config.wakeAddr = config.controllers.empty() ? "" : config.controllers[0].mac;
  saveControllersNvs();
}

bool addController(const String &mac, const String &name) {
  String normMac = mac;
  normMac.trim();
  normMac.toLowerCase();
  if (normMac.isEmpty()) return false;

  String friendly = name;
  friendly.trim();
  if (friendly.isEmpty()) friendly = "Wireless Controller";

  for (auto &c : config.controllers) {
    if (c.mac.equalsIgnoreCase(normMac)) {
      c.name = friendly;
      saveControllersNvs();
      return true;
    }
  }

  if (config.controllers.size() >= MAX_CONTROLLERS) {
    return false;
  }

  config.controllers.push_back({normMac, friendly});
  config.wakeAddr = config.controllers[0].mac;
  saveControllersNvs();
  return true;
}

bool removeController(const String &mac) {
  for (auto it = config.controllers.begin(); it != config.controllers.end(); ++it) {
    if (it->mac.equalsIgnoreCase(mac)) {
      config.controllers.erase(it);
      config.wakeAddr = config.controllers.empty() ? "" : config.controllers[0].mac;
      saveControllersNvs();
      return true;
    }
  }
  return false;
}

bool renameController(const String &mac, const String &name) {
  String friendly = name;
  friendly.trim();
  if (friendly.isEmpty()) friendly = "Wireless Controller";

  for (auto &c : config.controllers) {
    if (c.mac.equalsIgnoreCase(mac)) {
      c.name = friendly;
      saveControllersNvs();
      return true;
    }
  }
  return false;
}

void setWakeAddr(const String &addr) {
  String norm = addr;
  norm.trim();
  norm.toLowerCase();
  config.wakeAddr = norm;
  if (!norm.isEmpty()) {
    config.controllers.clear();
    config.controllers.push_back({norm, "Controller 1"});
    saveControllersNvs();
  } else {
    config.controllers.clear();
    saveControllersNvs();
  }
}

void setHostAddr(const String &addr) {
  config.hostAddr = addr;
  putString("hostAddr", addr);
}

void setPassHash(const String &hash) {
  config.passHash = hash;
  putString("passHash", hash);
}

void setForceSetup(bool force) {
  config.forceSetup = force;
  prefs.begin(NS, false);
  prefs.putBool("forceSetup", force);
  prefs.end();
}

void setFlashOnWake(bool enable) {
  config.flashOnWake = enable;
  prefs.begin(NS, false);
  prefs.putBool("flashOnWake", enable);
  prefs.end();
}

void setWolEnabled(bool enable) {
  config.wolEnabled = enable;
  prefs.begin(NS, false);
  prefs.putBool("wolEnabled", enable);
  prefs.end();
}

void setGracefulShutdown(bool enable) {
  config.gracefulShutdown = enable;
  prefs.begin(NS, false);
  prefs.putBool("gracefulShutdown", enable);
  prefs.end();
}

void setWifiConfig(const String &ssid, const String &password, const String &bc250Mac, const String &hostname) {
  config.wifiSsid = ssid;
  config.wifiPassword = password;
  String normMac = bc250Mac;
  normMac.trim();
  normMac.toLowerCase();
  config.bc250Mac = normMac;

  prefs.begin(NS, false);
  prefs.putString("wifiSsid", ssid);
  prefs.putString("wifiPassword", password);
  prefs.putString("bc250Mac", normMac);
  if (!hostname.isEmpty()) {
    String normHost = hostname;
    normHost.trim();
    normHost.toLowerCase();
    config.hostname = normHost;
    prefs.putString("hostname", normHost);
  }
  prefs.end();
}

bool isValidHostname(const String &name) {
  if (name.length() < 1 || name.length() > 32) return false;
  if (name[0] == '-' || name[name.length() - 1] == '-') return false;
  for (unsigned int i = 0; i < name.length(); i++) {
    char c = name[i];
    bool isLowerAlpha = (c >= 'a' && c <= 'z');
    bool isDigit = (c >= '0' && c <= '9');
    bool isHyphen = (c == '-');
    if (!isLowerAlpha && !isDigit && !isHyphen) {
      return false;
    }
  }
  return true;
}

void setHostname(const String &hostname) {
  String norm = hostname;
  norm.trim();
  norm.toLowerCase();
  if (norm.isEmpty()) {
    norm = "bc250-switch";
  }
  config.hostname = norm;
  prefs.begin(NS, false);
  prefs.putString("hostname", norm);
  prefs.end();
}

bool isConfigured() {
#if defined(WAKE_BLE)
  return config.passHash.length() > 0 && config.wakeAddr.length() > 0;
#else
  return config.passHash.length() > 0 && !config.controllers.empty() &&
         config.hostAddr.length() > 0;
#endif
}

void resetConfig() {
  prefs.begin(NS, false);
  prefs.clear();
  prefs.putBool("forceSetup", true);
  prefs.end();

  config = {{}, "", "", "", true, true, false, "", "", "", "bc250-switch", false};
}

