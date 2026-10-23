#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include "wake.h"
#include "network_wake.h"
#include "board.h"
#include "config.h"
#include "portal.h"
#include "version.h"

// The ESP32 is permanently powered from the ATX connector (5VSB), so it runs
// independently of the PSU's main rails. It controls the SFX PSU through PS_ON#
// and watches the BC250's TPMS1 line to know whether the board itself is up.
//
// Control model
// -------------
//   * Asserting PS_ON# turns the PSU on; the BC250 is expected to power up from
//     applied power (BIOS "restore on AC power"), so PSU on == board boots.
//   * Without the optional PWR_BTN# wire, "turning the board off" means cutting
//     PSU power via PS_ON#. This is a hard power-off, not a graceful OS shutdown.
//   * With the optional graceful-shutdown feature enabled (portal setting), the
//     ESP also mirrors the physical button onto the motherboard PWR_BTN# line
//     while the board is ON, so the board itself decides what a short press
//     (ACPI shutdown) or long press (hardware override) means. The 5 s
//     hold-to-cut-PSU below stays as a backstop.
//   * If the board shuts itself down (e.g. OS shutdown), TPMS1 drops to 0 while
//     the PSU is still energized. We detect that and release PS_ON# so the PSU
//     follows the board down.
//   * While OFF, the ESP watches for the bound controller (Classic BT page or BLE
//     advertisement). Detecting it counts as "controller present", which wakes the
//     machine just like a button tap.

static PowerState state = STATE_OFF;
static bool       g_setupMode = false;

// --- Button debounce / press tracking ---
static bool          buttonStable      = false;  // debounced "pressed" level
static bool          buttonLastRaw     = false;
static unsigned long buttonLastChange  = 0;
static unsigned long pressStart        = 0;
static bool          pressStartedOff   = false;  // press began while OFF
static bool          pressStartedOn    = false;  // press began while STATE_ON
static bool          longPressFired    = false;
static bool          setupFired        = false;

// --- Optional PWR_BTN# output (graceful shutdown) ---
static bool          pwrBtnDriven      = false;  // pin currently driven LOW

// --- Board-sense debounce ---
static bool          boardSenseStable  = false;  // debounced TPMS1 HIGH
static bool          boardSenseLastRaw = false;
static unsigned long boardSenseChange  = 0;

// --- Wake subsystem ---
static bool          wakeActive        = false;
static bool          radiosActive      = false;
static unsigned long wakeInhibitUntil  = 0;      // wakes ignored until this

// --- Misc timers ---
static unsigned long bootStart    = 0;
static unsigned long lastHeartbeat = 0;

static const char *stateName(PowerState s) {
  switch (s) {
    case STATE_OFF:     return "OFF";
    case STATE_BOOTING: return "BOOTING";
    case STATE_ON:      return "ON";
  }
  return "?";
}

const char *getBoardStateName() {
  return stateName(state);
}

static void startWake();

// Raw (pre-time-debounce) board-up level, filtered against transient spikes.
static bool senseLevel = false;

static void setState(PowerState next) {
  if (next == state) return;
  Serial.printf("[STATE] %s -> %s\n", stateName(state), stateName(next));
  state = next;
  if (next == STATE_OFF) {
#if defined(BOARD_ESP32CAM_CARRIER)
    // Detach ADC2 before starting Wi-Fi/BT radios, which share ADC2 hardware.
    pinMode(BOARD_SENSE, INPUT);
    senseLevel = false;
#endif
    radiosActive = true;
    if (!g_setupMode) {
      startWake();
      networkWakeBegin(true);
    } else {
      wakeSetDiscovery(true);
    }
  } else {
    wakeStop();
    wakeActive = false;
    networkWakeStop();
    radiosActive = false;
  }
}

#if defined(BOARD_ESP32CAM) && !defined(BOARD_ESP32CAM_CARRIER)
// Bare ESP32-CAM: GPIO13 is on ADC2, which cannot be sampled while Bluetooth is
// running. It is also shared with SD DAT3 which floats HIGH unless pulled down.
// In direct-wire setups we configure GPIO13 with INPUT_PULLDOWN and require
// unanimous agreement across SENSE_OVERSAMPLE consecutive digital reads.
static bool readBoardSense(uint32_t *outMv = nullptr) {
  int highCount = 0;
  for (int i = 0; i < SENSE_OVERSAMPLE; i++) {
    if (i > 0) delayMicroseconds(200);
    if (digitalRead(BOARD_SENSE) == HIGH) {
      highCount++;
    }
  }
  if (highCount == SENSE_OVERSAMPLE) {
    senseLevel = true;
  } else if (highCount == 0) {
    senseLevel = false;
  }
  if (outMv) *outMv = senseLevel ? 3300 : 0;
  return senseLevel;
}
#else

// One ADC sample in mV. The classic ESP32's analogReadMilliVolts() needs an eFuse
// calibration (vref) that many modules never had burned; without it every read
// fails and returns 0. Use the raw 12-bit count instead: the ADC1 + 11 dB range is
// roughly 0..3.3 V, which is accurate enough for the wide hysteresis thresholds.
// On ESP32-C3, analogReadMilliVolts() is factory-calibrated in eFuse.
static uint32_t senseReadMv() {
#if defined(BOARD_ESP32C3)
  return analogReadMilliVolts(BOARD_SENSE);
#else
  return (uint32_t)analogRead(BOARD_SENSE) * 3300UL / 4095UL;
#endif
}

static bool readBoardSense(uint32_t *outMv = nullptr) {
#if defined(BOARD_ESP32CAM_CARRIER)
  // GPIO13 is on ADC2, which cannot be sampled while Wi-Fi or Bluetooth radios
  // are active. Radios are only active in STATE_OFF / setup mode; during
  // initial boot qualification, STATE_BOOTING, and STATE_ON, radios are off
  // and ADC2 can be sampled cleanly.
  if (radiosActive || g_setupMode) {
    senseLevel = false;
    if (outMv) *outMv = 0;
    return false;
  }
#endif
  // Average several reads to reject single-sample ADC/line spikes. Without this,
  // one stray spike past SENSE_HIGH_MV chatters the hysteresis and restarts the
  // board-off debounce, stalling shutdown detection (see SENSE_OVERSAMPLE).
  uint32_t acc = 0;
  for (int i = 0; i < SENSE_OVERSAMPLE; i++) {
    acc += senseReadMv();
  }
  uint32_t mv = acc / SENSE_OVERSAMPLE;
  if (outMv) *outMv = mv;
  if (senseLevel) {
    if (mv < SENSE_LOW_MV) senseLevel = false;
  } else {
    if (mv > SENSE_HIGH_MV) senseLevel = true;
  }
  return senseLevel;
}
#endif

// PWR_BTN_PIN is a high-Z INPUT unless we are emulating a press. The motherboard
// owns the HIGH level (~3.4V pull-up), so we only ever pull the line LOW and
// then let go; it is never driven HIGH.
static void pwrBtnDrive(const char *why) {
  if (pwrBtnDriven) return;
  digitalWrite(PWR_BTN_PIN, LOW);
  pinMode(PWR_BTN_PIN, OUTPUT);
  pwrBtnDriven = true;
  Serial.printf("[PWR ] PWR_BTN# LOW (%s)\n", why);
}

static void pwrBtnRelease(const char *why) {
  if (!pwrBtnDriven) return;
  pinMode(PWR_BTN_PIN, INPUT);
  pwrBtnDriven = false;
  Serial.printf("[PWR ] PWR_BTN# released (%s)\n", why);
}

static void updatePwrBtn(unsigned long now) {
  if (!config.gracefulShutdown || state != STATE_ON) {
    pwrBtnRelease("feature off or board not ON");
    return;
  }

  if (buttonStable && pressStartedOn) {
    pwrBtnDrive("mirroring button press");
  } else {
    pwrBtnRelease("button released");
  }
}

static void psuOn() {
  digitalWrite(PS_ON_PIN, PS_ON_ASSERT);
  Serial.println("[PSU ] PS_ON# asserted (LOW) -> PSU ON");
}

static void psuOff() {
  digitalWrite(PS_ON_PIN, PS_ON_RELEASE);
  Serial.println("[PSU ] PS_ON# released (high-Z) -> PSU OFF");
}

// Shared power-on path, used by both the button and the wake driver. Takes the
// loop's `now` rather than calling millis() itself: the BOOTING timeout compares
// against the `now` cached at the top of normalLoop(), and a fresh millis() here
// can land a millisecond past it. Since the comparison is unsigned, bootStart >
// now makes (now - bootStart) underflow to a huge value and trip the timeout on
// the very next loop. Sharing one clock per iteration keeps the math monotonic.
static void powerOn(const char *reason, unsigned long now) {
  if (g_setupMode) {
    Serial.printf("[ACT ] %s -> power-on blocked (setup mode active)\n", reason);
    return;
  }
  Serial.printf("[ACT ] %s -> powering on\n", reason);
  psuOn();
  bootStart = now;
  setState(STATE_BOOTING);
}

// Shared power-off path. Starts the wake cooldown so the controller's
// post-shutdown reconnect burst can't immediately wake us again. Takes `now` for
// the same single-clock-per-loop reason as powerOn().
static void powerOff(const char *reason, unsigned long now) {
  Serial.printf("[ACT ] %s -> powering off\n", reason);
  psuOff();
  pwrBtnRelease("powering off");
  wakeInhibitUntil = now + WAKE_WAKE_COOLDOWN_MS;
  setState(STATE_OFF);
}

void armWakeCooldown(unsigned long durationMs) {
  wakeInhibitUntil = millis() + durationMs;
  Serial.printf("[ACT ] wake cooldown armed for %lu ms\n", durationMs);
}

static void startWake() {
  wakeActive = wakeBegin();
  if (wakeActive) {
    wakeSetConnectable(state == STATE_OFF);
  }
}

// Persist a setup request and reboot into the WiFi portal.
static void enterSetupMode(const char *reason) {
  Serial.printf("[ACT ] %s -> entering setup mode\n", reason);
  setForceSetup(true);
  delay(50);
  ESP.restart();
}

// Debounce a raw level into a stable level. Returns true if the stable value
// changed this call, writing the new value into *stable.
static bool debounce(bool raw, bool *stable, bool *lastRaw,
                     unsigned long *lastChange, unsigned long now,
                     unsigned long window) {
  if (raw != *lastRaw) {
    *lastRaw = raw;
    *lastChange = now;
  }
  if (raw != *stable && (now - *lastChange) >= window) {
    *stable = raw;
    return true;
  }
  return false;
}

// Non-blocking status LED. Setup mode (1 Hz blink) overrides the power-state
// pattern: ON = solid, BOOTING = fast blink (~4 Hz), OFF = dark. Compiles to a
// no-op on targets without a status LED (STATUS_LED_PIN < 0).
static void statusLedUpdate(unsigned long now) {
  if (STATUS_LED_PIN < 0) return;

  bool lit;
  if (g_setupMode) {
    lit = ((now / 500) % 2) == 0;
  } else if (state == STATE_BOOTING) {
    lit = ((now / 125) % 2) == 0;
  } else {
    lit = (state == STATE_ON);
  }

  static int lastLevel = -1;
  int level = lit ? STATUS_LED_ON : !STATUS_LED_ON;
  if (level != lastLevel) {
    digitalWrite(STATUS_LED_PIN, level);
    lastLevel = level;
  }
}

static void initHardware() {
  // Pre-load the hardware output latch via ESP-IDF before enabling the pin driver
  // so PS_ON# never glitches on startup.
  gpio_set_level((gpio_num_t)PS_ON_PIN, PS_ON_RELEASE);
#if defined(BOARD_ESP32CAM_CARRIER)
  pinMode(PS_ON_PIN, OUTPUT);
#else
  pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN);
#endif
  digitalWrite(PS_ON_PIN, PS_ON_RELEASE);

  // Optional PWR_BTN#: always start as a high-Z input, never driven unless the
  // graceful-shutdown feature is enabled and the board is ON.
  pinMode(PWR_BTN_PIN, INPUT);

  // Switch: optional local ground pin, sensed input with pull-up.
  if (BUTTON_GND >= 0) {
    pinMode(BUTTON_GND, OUTPUT);
    digitalWrite(BUTTON_GND, LOW);
  }
  pinMode(BUTTON_SENSE, INPUT_PULLUP);

  // Status LED: start off until statusLedUpdate() applies a pattern.
  if (STATUS_LED_PIN >= 0) {
    digitalWrite(STATUS_LED_PIN, !STATUS_LED_ON);
    pinMode(STATUS_LED_PIN, OUTPUT);
  }

#if defined(BOARD_ESP32CAM)
  pinMode(FLASH_LED, OUTPUT);
  digitalWrite(FLASH_LED, LOW);
  #if defined(BOARD_ESP32CAM_CARRIER)
    analogSetAttenuation(ADC_11db);
  #else
    pinMode(BOARD_SENSE, INPUT_PULLDOWN);
  #endif
#elif defined(BOARD_ESP32C3)
  analogSetPinAttenuation(BOARD_SENSE, ADC_11db);
#else
  // TPMS1 sense: read as ADC over the full 0-3.3V range.
  analogSetAttenuation(ADC_11db);  // global; the per-pin call errors before the first read
#endif

  // Seed debounced states from the current levels.
  buttonStable = buttonLastRaw = (digitalRead(BUTTON_SENSE) == LOW);
}

static void normalBegin() {
  Serial.printf("=== BC250 PSU controller v%s (%s) ===\n", FW_VERSION, FW_TARGET);

  initHardware();

  esp_reset_reason_t rstReason = esp_reset_reason();
  if (rstReason == ESP_RST_BROWNOUT) {
    Serial.println("[INIT] WARNING: brownout reset detected; waiting 500 ms for rails to settle, skipping STATE_ON adoption");
    delay(500);
    senseLevel = false;
    boardSenseStable = boardSenseLastRaw = false;
  } else {
    // Allow input pull-down / ADC input to settle before sampling.
    delay(10);
    bool sustainedHigh = readBoardSense();
    if (sustainedHigh) {
      unsigned long tStart = millis();
      while (millis() - tStart < BOARD_ON_DEBOUNCE_MS) {
        delay(10);
        if (!readBoardSense()) {
          sustainedHigh = false;
          break;
        }
      }
    }
    boardSenseStable = boardSenseLastRaw = sustainedHigh;

    // If the board is genuinely up across the full qualification window when
    // the ESP (re)boots, adopt the ON state rather than assuming OFF.
    if (boardSenseStable) {
      psuOn();  // make sure PS_ON# matches reality
      setState(STATE_ON);
      delay(200);  // stagger PSU assertion
    }
  }
  boardSenseChange = millis();

  Serial.printf("[INIT] state=%s board=%s bound=%s mode=%s\n",
                stateName(state), boardSenseStable ? "UP" : "DOWN",
                config.wakeAddr.isEmpty() ? "(none)" : config.wakeAddr.c_str(),
                wakeModeName());

  // Only start wireless radios if booting in STATE_OFF
  if (state == STATE_OFF) {
#if defined(BOARD_ESP32CAM_CARRIER)
    pinMode(BOARD_SENSE, INPUT);
    senseLevel = false;
#endif
    radiosActive = true;
    startWake();
    networkWakeBegin(true);
  }
}

void setup() {
  Serial.begin(115200);
  // Give USB-CDC a moment to enumerate so early logs aren't lost.
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 2000) {
    delay(10);
  }
  Serial.println();

  loadConfig();

  // If the device is not yet provisioned, or if setup was explicitly requested
  // (e.g. 8s button hold while OFF or serial 's' command), start the setup portal.
  if (!isConfigured() || config.forceSetup) {
    g_setupMode = true;
    radiosActive = true;
    initHardware();
    state = STATE_OFF;
    portalBegin();
  } else {
    g_setupMode = false;
    normalBegin();
  }
}

static void normalLoop() {
  unsigned long now = millis();

  statusLedUpdate(now);

  // --- Serial commands for debugging ---
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      if (state == STATE_OFF) {
        enterSetupMode("serial 's' command");
      } else {
        Serial.println("[ACT ] serial 's' ignored (machine is not in STATE_OFF)");
      }
    }
  }

  // --- Sample & debounce inputs ---
  uint32_t senseMv;
  bool buttonRaw = (digitalRead(BUTTON_SENSE) == LOW);   // pressed == LOW
  bool boardRaw  = readBoardSense(&senseMv);             // board up (hysteresis)

  if (debounce(buttonRaw, &buttonStable, &buttonLastRaw,
               &buttonLastChange, now, DEBOUNCE_MS)) {
    if (buttonStable) {
      // Press began.
      pressStart = now;
      pressStartedOff = (state == STATE_OFF);
      pressStartedOn  = (state == STATE_ON);
      longPressFired = false;
      setupFired = false;
      Serial.println("[BTN ] pressed");
    } else {
      // Released. A short tap that began while OFF powers on (power-on is on
      // release so that a long hold from OFF can mean "enter setup" instead).
      unsigned long held = now - pressStart;
      Serial.printf("[BTN ] released after %lu ms\n", held);
      if (pressStartedOff && !setupFired && state == STATE_OFF &&
          held < SETUP_HOLD_MS) {
        if (!g_setupMode) {
          portalPairingStop();
          powerOn("short press while OFF", now);
        } else {
          Serial.println("[BTN ] tap ignored while in setup mode");
        }
      }
    }
  }

  // --- Button holds ---
  if (buttonStable && pressStartedOff && !setupFired &&
      (now - pressStart) >= SETUP_HOLD_MS) {
    // Long hold from OFF -> toggle or enter setup mode (does not power the machine on).
    setupFired = true;
    if (g_setupMode) {
      Serial.println("[ACT ] button held (>8s) while OFF in setup mode -> leaving setup mode");
      setForceSetup(false);
      setProvisioned(true);
      delay(50);
      ESP.restart();
    } else {
      enterSetupMode("long hold (>8s) while OFF");
    }
  }
  if (buttonStable && !pressStartedOff && !longPressFired && state == STATE_ON &&
      (now - pressStart) >= LONG_PRESS_MS) {
    // Long hold that began while ON -> force off.
    longPressFired = true;
    powerOff("long press (>5s) while ON", now);
  }

  // --- Controller wake ("machine follows controller") ---
  wakeLoop();
  unsigned long wakeLast = 0;
  bool wakePresent = wakeSeen(&wakeLast) && (now - wakeLast) < WAKE_PRESENCE_TIMEOUT_MS;
  bool wakeInhibited = ((int32_t)(wakeInhibitUntil - now) > 0) || portalIsPairingActive() || g_setupMode;
  if (state == STATE_OFF && wakePresent && !wakeInhibited) {
    powerOn("controller present", now);
  }

  // --- Network Wake-on-LAN ---
  if (!g_setupMode) {
    networkWakeLoop();
    unsigned long wolLast = 0;
    bool wolPresent = networkWakeSeen(&wolLast);
    if (state == STATE_OFF && wolPresent && !wakeInhibited) {
      powerOn("WOL magic packet", now);
    }
  }

  bool boardChanged = debounce(boardRaw, &boardSenseStable, &boardSenseLastRaw,
                               &boardSenseChange, now,
                               state == STATE_BOOTING ? BOARD_ON_DEBOUNCE_MS
                                                      : BOARD_OFF_DEBOUNCE_MS);

  // --- State-driven board-sense handling ---
  switch (state) {
    case STATE_BOOTING:
      if (boardSenseStable) {
        Serial.println("[ACT ] TPMS1 HIGH -> board is up");
        setState(STATE_ON);
      } else if ((now - bootStart) >= BOOT_TIMEOUT_MS) {
        powerOff("boot timed out, board never signalled UP", now);
      }
      break;

    case STATE_ON:
      // Board dropped TPMS1 on its own (OS shutdown / crash) -> follow it down.
      if (boardChanged && !boardSenseStable) {
        powerOff("TPMS1 LOW while ON, board shut down", now);
      }
      break;

    case STATE_OFF:
    default:
      break;
  }

  // --- Optional PWR_BTN# output (after state handling so it sees this loop's state) ---
  updatePwrBtn(now);

  // --- Heartbeat ---
  if (now - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = now;
    Serial.printf("[HB  ] state=%s board=%s btn=%s pwrbtn=%s wake=%s wol=%s | sense: %umV (%s)\n",
                  stateName(state),
                  boardSenseStable ? "UP" : "DOWN",
                  buttonStable ? "down" : "up",
                  !config.gracefulShutdown ? "off" : (pwrBtnDriven ? "low" : "idle"),
                  wakePresent ? "present" : "absent",
                  config.wolEnabled ? (networkWakeIsConnected() ? networkWakeIp().c_str() : "connecting") : "off",
                  senseMv, boardRaw ? "high" : "low");
  }

  portalCheckRestart();
}

void loop() {
  normalLoop();
}
