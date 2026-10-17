# Design

## Context

See `proposal.md` for motivation. When the BC250 is running in `STATE_ON` on `BOARD_ESP32CAM`, any ESP32 reset (such as a 3.3V brownout on the onboard AMS1117-3.3 regulator during a `19.5 dBm` Wi-Fi TX burst + Classic Bluetooth coexistence spike, or a crash when the 20s Wi-Fi fallback calls `portalBegin()`) releases `GPIO 14` (`PS_ON_PIN`) to high-Z during the ~250 ms reboot, cutting ATX PSU power. In `normalBegin()`, four behaviors currently combine to lock the hardware into an infinite ~300 ms ON/OFF oscillation:

1. `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` is called before `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)`, glitching `PS_ON#` LOW via the default `0` output latch before releasing it HIGH.
2. `readBoardSense()` on `BOARD_ESP32CAM` runs 16 back-to-back `digitalRead(13)` calls in ~3.2 us with zero time debounce. Because `TPMS1` bulk capacitors take >300 ms to discharge below threshold (and `GPIO 13` / `HS2_DATA3` on the ESP32-CAM PCB has a physical 47 kOhm pull-up to 3.3V fighting the ESP32's ~45 kOhm internal `INPUT_PULLDOWN`, biasing the pin near ~1.61 V), `normalBegin()` reads `boardSenseStable == true` even though the PSU just cut off.
3. `normalBegin()` immediately calls `psuOn()` (`PS_ON#` LOW) and, 0 ms later, executes `startWake()` (Classic BT RF calibration) and `networkWakeBegin()` (`19.5 dBm` Wi-Fi STA startup) at the exact instant the ATX PSU hot-restarts into the BC250.
4. If Wi-Fi is disconnected for 20s while `STATE_ON`, `networkWakeLoop()` invokes `portalBegin()`, which calls `wakeSetDiscovery(true)` -> `btBegin(config.hostAddr)` on an already-enabled BT controller (`btUp == true`), calling `esp_iface_mac_addr_set()` in an invalid controller state, resetting the HCI command queue, and enabling BT Inquiry + Page Scan while the host machine is ON.

## Goals / Non-Goals

**Goals:**
- Prevent 3.3V brownouts and BT stack corruption while the BC250 is running in `STATE_ON`.
- Break the self-sustaining `normalBegin()` reset -> `psuOn()` -> dual-radio spike -> reset feedback loop so an ESP32 reset can never oscillate the ATX PSU.
- Ensure `BOARD_SENSE` startup adoption in `normalBegin()` only triggers when `TPMS1` is genuinely sustained HIGH by a running board, never from decaying bulk capacitors after a brownout reset.
- Ensure `PS_ON#` assertion and Wi-Fi/Bluetooth RF transitions never overlap on the same millisecond.

**Non-Goals:**
- Changing hardware wiring or requiring external pull-down resistors on `GPIO 13`.
- Altering the REST API schema or web portal UI.

## Decisions

### 1. Pre-load `PS_ON_PIN` Latch, Detect Brownout Resets, and Qualify Startup `BOARD_SENSE` Over 250 ms
- **Decision**:
  - In `normalBegin()`, always call `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` *before* `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` so the output latch is `HIGH` before the open-drain driver enables.
  - Check `esp_reset_reason()` at the start of `normalBegin()`. If the reset reason is `ESP_RST_BROWNOUT`, log a warning, wait `500 ms` with `PS_ON#` released for `+5VSB` and `TPMS1` bulk capacitors to settle completely, and skip `STATE_ON` boot adoption so the system cannot re-enter a brownout oscillation loop.
  - On non-brownout boots, after configuring `BOARD_SENSE` (`INPUT_PULLDOWN` on `BOARD_ESP32CAM`), allow a `10 ms` pin-settling delay and require `readBoardSense()` to remain continuously `HIGH` across the full `BOARD_ON_DEBOUNCE_MS` (`250 ms`) window (sampled every `10 ms`) before adopting `STATE_ON` and calling `psuOn()`.
  - In `readBoardSense()` on `BOARD_ESP32CAM`, insert `delayMicroseconds(200)` between the 16 `digitalRead(BOARD_SENSE)` samples (~3.2 ms total window) to reject high-frequency switching noise and mains-coupled transients.
- **Rationale**: A genuine ESP32-only reset while the board is powered externally holds `TPMS1` solidly at 3.3V across 250 ms, whereas residual capacitor charge after `PS_ON#` releases decays below threshold within 250–500 ms. Explicitly guarding `ESP_RST_BROWNOUT` guarantees a brownout can never oscillate `PS_ON#`.
- **Alternatives considered**: Keeping single-shot `readBoardSense()` in `normalBegin()` (rejected because 3.2 us is shorter than capacitor bleed-down and switching spikes).

### 2. Stagger ATX PSU Turn-On and Radio Transitions
- **Decision**:
  - When `psuOn()` is called in `normalBegin()` (boot adoption), delay `200 ms` before initializing Bluetooth and Wi-Fi so ATX PSU inrush current and `+5VSB` settling complete before 2.4 GHz RF calibration begins.
  - When `powerOn()` transitions from `STATE_OFF` to `STATE_BOOTING`, defer `networkWakeOnStateChanged(false)` (the Wi-Fi disconnect + MAC swap + reconnect burst) by `500 ms` via a non-blocking timestamp in `networkWakeLoop()` instead of executing it synchronously on the exact millisecond `psuOn()` asserts `PS_ON#`.
  - When entering `STATE_BOOTING` or `STATE_ON`, ensure Bluetooth discovery (`wakeSetDiscovery(false)`) and page scan (`wakeSetConnectable(false)`) are both disabled.
- **Rationale**: Separating the ATX PSU startup transient from Wi-Fi/BT RF bursts eliminates the coincident current spike on `+5VSB` and the 3.3V regulator.
- **Alternatives considered**: Blocking `delay(500)` inside `powerOn()` (rejected to keep button debouncing and state tracking responsive).

### 3. Make `btBegin()` Idempotent and Block BT Discovery While Powered On
- **Decision**:
  - In `src/bt_classic.cpp`, if `btBegin(hostMac)` is called when `btUp` is already `true` and `hostMac` matches `currentHostMac`, return `true` immediately without calling `esp_iface_mac_addr_set()`, `esp_vhci_host_register_callback()`, or `queueInit()`. If `hostMac` changed while `btUp` is `true`, delegate to `btRestart(hostMac)`.
  - In `src/portal.cpp` (`portalBegin()`) and `src/wake_classic.cpp` / `src/wake_ble.cpp` (`wakeSetDiscovery(true)`), only start Bluetooth discovery and page scan if the board is in `STATE_OFF` (`!isBoardPowered()`). When the board transitions to `STATE_OFF` while `isPortalSetupMode()` is active, enable discovery then.
- **Rationale**: Prevents `portalBegin()` (both at startup when board is ON and when triggered by the 20s Wi-Fi fallback in `networkWakeLoop()`) from corrupting the running VHCI state or firing Classic BT Inquiry scans while the BC250 is running.
- **Alternatives considered**: Skipping `portalBegin()` entirely on 20s Wi-Fi timeout when `STATE_ON` (rejected because the fallback SoftAP is still useful for web portal access as long as Bluetooth discovery is not started while the host is ON).

### 4. Cap Wi-Fi TX Power on `BOARD_ESP32CAM` and Apply TX Power in Station Mode
- **Decision**:
  - In `include/board.h`, set `AP_TX_POWER` on `BOARD_ESP32CAM` to `WIFI_POWER_11dBm` (down from `WIFI_POWER_19_5dBm`).
  - Call `WiFi.setTxPower(AP_TX_POWER)` after `WiFi.begin(...)` in `networkWakeBegin()` and `networkWakeOnStateChanged()` (`src/network_wake.cpp`) so both Station and SoftAP modes obey the target's RF power envelope.
- **Rationale**: Dropping ESP32-CAM Wi-Fi TX power from `19.5 dBm` to `11 dBm` cuts peak Wi-Fi TX current draw by ~150–200 mA with negligible indoor range impact, keeping the AMS1117-3.3 LDO well above brownout threshold.
- **Alternatives considered**: Leaving Station TX power at default `19.5 dBm` (rejected because Station mode is active continuously during `STATE_ON`).

## Risks / Trade-offs

- **[250 ms boot sense check adds 250 ms to cold boot startup time]** -> Only runs once during `normalBegin()` before entering `loop()`; imperceptible to the user.
- **[`WIFI_POWER_11dBm` on `BOARD_ESP32CAM` slightly reduces maximum Wi-Fi range]** -> `11 dBm` is standard for compact ESP32 modules with linear regulators and remains more than sufficient for local LAN/SoftAP usage while preventing 3.3V brownouts.
