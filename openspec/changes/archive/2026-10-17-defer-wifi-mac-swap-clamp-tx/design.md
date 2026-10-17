# Design

## Context

See `proposal.md` for motivation. In `src/main.cpp`, `setState(PowerState next)` calls `networkWakeOnStateChanged(next == STATE_OFF)`. When `powerOn()` begins, the state transitions from `STATE_OFF` to `STATE_BOOTING`. Currently, `networkWakeOnStateChanged(false)` schedules `applyStateMacChange(false)` to run after `MAC_SWAP_DEFER_MS = 500` ms via `networkWakeLoop()`.

At 500 ms, the ATX PSU is actively delivering high inrush current into the BC250 motherboard VRMs, causing voltage ripple and sag on the +5VSB standby line powering the ESP32-CAM's AMS1117-3.3 regulator. When `applyStateMacChange()` calls `WiFi.disconnect(true)` and `networkWakeBegin(false)`, `WiFi.begin()` fires an unconstrained 19.5 dBm RF burst because `WiFi.setTxPower()` is only called after `WiFi.begin()`. This causes an AMS1117-3.3 brownout reset, releasing `PS_ON#` to high-Z and turning off the BC250 after just ~500 ms.

## Goals / Non-Goals

**Goals:**
- Eliminate all Wi-Fi radio disruptions, driver re-initializations, and MAC swaps while the machine is in `STATE_BOOTING`.
- Defer restoring the ESP32 factory MAC address until the host board has fully stabilized in `STATE_ON` across an extended 5000 ms settling window.
- Clamp ESP32 Wi-Fi PHY TX power at the hardware level (`esp_wifi_set_max_tx_power`) before `WiFi.begin()` starts radio activity, ensuring initial probe packets never emit at default 19.5 dBm.
- Discard pending MAC transitions if a boot fails or is cancelled before entering `STATE_ON`.

**Non-Goals:**
- Changing Wake-on-LAN magic packet parsing, port listeners, or UDP discovery protocols.
- Altering the factory MAC or dongle MAC storage structures in NVS.

## Decisions

### 1. State-Aware Network Wake State Management
- **Approach**: Update `networkWakeOnStateChanged()` to accept the system power state (`PowerState` or an explicit enum/state indicator) instead of a simple `bool isOff`.
  - In `STATE_BOOTING`: Do nothing. Maintain the existing station connection without disconnecting or scheduling a MAC swap.
  - In `STATE_ON`: If the current station MAC is not already the factory MAC, schedule a deferred MAC swap with `MAC_SWAP_DEFER_MS = 5000` ms.
  - In `STATE_OFF`: Cancel any pending swap and immediately transition the station MAC to `config.bc250Mac` (if configured) so WOL interception is active while unpowered.
- **Rationale**: The host OS takes 15–30 seconds to complete BIOS POST and load network device drivers. Swapping during `STATE_BOOTING` serves no purpose and occurs during peak PSU turn-on inrush. Deferring to `STATE_ON + 5000 ms` ensures the ATX PSU rails and BC250 motherboard are fully settled.
- **Alternatives Considered**:
  - *Keeping `isOff` boolean and deferring by 15 seconds in `STATE_BOOTING`*: If the boot times out after 10 seconds, the pending swap would still fire or need cancellation logic; gating on `STATE_ON` is semantically correct.

### 2. Clamp PHY TX Power Before `WiFi.begin()`
- **Approach**: In `networkWakeBegin()`, call `esp_wifi_set_max_tx_power((int8_t)AP_TX_POWER)` immediately after `WiFi.mode(WIFI_STA)` (which initializes the ESP-IDF Wi-Fi driver via `esp_wifi_init()`) before calling `WiFi.begin()`. Maintain `WiFi.setTxPower(AP_TX_POWER)` after `WiFi.begin()` for Arduino HAL tracking.
- **Rationale**: `WiFiGenericClass::setTxPower()` rejects calls if `WiFi.STA.started()` is false, forcing it to be called after `WiFi.begin()`. However, `esp_wifi_set_max_tx_power()` can be called as soon as `esp_wifi_init()` completes. Setting it prior to `WiFi.begin()` guarantees that initial RF calibration, channel scanning, and probe requests never exceed `11 dBm` on `BOARD_ESP32CAM`.
- **Alternatives Considered**:
  - *Only calling `WiFi.setTxPower()`*: Leaves a window where the radio transmits at 19.5 dBm during connection startup.

### 3. Extend `MAC_SWAP_DEFER_MS` to 5000 ms in `STATE_ON`
- **Approach**: Increase `MAC_SWAP_DEFER_MS` from `500` to `5000` ms.
- **Rationale**: Motherboard `TPMS1` qualifies HIGH after 250 ms, transitioning to `STATE_ON`. Giving an additional 5000 ms after entering `STATE_ON` ensures all switching transients, capacitor charging, and fans have stabilized completely before the brief Wi-Fi disconnect/reconnect cycle occurs.
- **Alternatives Considered**:
  - *Swapping immediately upon entering `STATE_ON`*: `TPMS1` rises only ~250–500 ms after `PS_ON#`; swapping immediately at `t = 250 ms` would still collide with PSU inrush.

## Risks / Trade-offs

- **[Risk] Brief Wi-Fi disconnection at t = 5–7s after power-on**
  - *Mitigation*: The disconnect lasts only ~1.5–2 seconds. Because the host OS is still executing BIOS POST and memory training, the host network interface is offline and unaffected.
- **[Risk] User attempts to access web portal during the 5s window**
  - *Mitigation*: Web portal access remains available under the dongle MAC IP until the swap occurs, then updates to the factory MAC IP.
