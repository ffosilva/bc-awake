# Design

## Context

See proposal.md for motivation. Today `normalLoop()` in `src/main.cpp` handles the physical button as: tap while OFF powers on, 5 s hold while ON force-cuts `PS_ON#`, 8 s hold while OFF enters setup. A tap while ON does nothing. When the board shuts itself down, TPMS1 stays LOW for 1500 ms in `STATE_ON` and `powerOff()` releases `PS_ON#` and arms the wake cooldown. That path completes a graceful shutdown, so no new "off" logic is required.

The BC250 `PWR_BTN#` line idles at ~3.4 V (measured), pulled up by the motherboard. The ESP32 shares ground with it through the ATX connector.

## Goals / Non-Goals

**Goals:**
- Make the ESP32 act as a transparent extension of the real power button, using a single GPIO and no extra components.
- Keep the feature fully inert unless explicitly enabled.
- Reuse existing shutdown detection; add no new power states.

**Non-Goals:**
- Hard-cut fallback timeout (the user handles a hung OS manually).
- Powering on via `PWR_BTN#`; `PS_ON#` plus BIOS restore-on-AC remains the power-on path.
- Level shifting or MOSFET buffering.

## Decisions

**Mirror the button, don't interpret it.** While `STATE_ON` and the feature is enabled, the debounced button level is copied to the pin: pressed means `OUTPUT` + `LOW`, released means `INPUT`. The motherboard decides what a short or long press means (ACPI event vs. hardware override), exactly as with its native button. Alternative: detect taps in firmware and send a fixed 300 ms pulse, with a pending-shutdown flag. Rejected: more state, behaves differently from a real button, and cannot reach the motherboard's long-press override.

**Only presses that began ON are mirrored.** Reuse `pressStartedOff`: a press that started while OFF (power-on tap, 8 s setup hold) never drives the pin, even if the state becomes ON mid-press. Presses starting in BOOTING are not mirrored either, for simplicity.

**Release on leaving ON.** `powerOff()` and any transition out of `STATE_ON` release the pin to `INPUT` immediately, so the line is never held LOW after the 5 s backstop fires.

**5 s force-off stays as backstop.** The existing hold-to-cut-PSU is unchanged. If the board's ~4 s override works, the board is already down; if it doesn't (or the board has no override), `PS_ON#` is still released at 5 s. It avoids depending on unverified hardware behavior.

**Pin assignment.** ESP32-CAM GPIO2 (header-exposed, SD DAT0 unused here, ignored at boot when GPIO0 is high), DevKitC GPIO27, C3 GPIO7. The latter two have no strapping or peripheral conflicts and sit next to existing signals on the same headers. Alternatives: GPIO12 on CAM (rejected, MTDI strapping bricks boot at 3.3 V), C3 GPIO10 (valid spare).

**Never drive HIGH; mode switching, not open-drain.** The pin is `INPUT` while idle and switches to `OUTPUT` + `LOW` only while mirroring or pulsing. The motherboard owns the high level, so the ESP32 can never fight the pull-up. `OUTPUT_OPEN_DRAIN` (used for `PS_ON#`) is not used here: keeping the pin a true input whenever idle or disabled matters most on ESP32-CAM GPIO2, a strapping pin.

**Non-blocking API pulse.** The 300 ms portal-triggered pulse is timed from the loop's cached `now` rather than `delay()`, so debouncing, wake loops, and the web server keep running. A pulse-until timestamp returns the pin to `INPUT`. A request during an active pulse is neither extended nor queued. Alternative: `delay(300)`, rejected since it stalls the loop.

**Config and API.** One persisted boolean `gracefulShutdown` (default false) next to `flashOnWake`/`wolEnabled`. Due to the ESP-IDF 15-character NVS key limit (`NVS_KEY_NAME_MAX_SIZE`), the internal NVS storage key is named `graceful` (8 characters), keeping `gracefulShutdown` in the public JSON API and in-memory `Config` struct. `POST /api/power` saves it. `POST /api/shutdown` triggers the pulse, returning 409 when the feature is off or the machine is not ON. `GET /api/status` exposes the flag and power state for the UI. The UI lives in the System tab; the tab count stays at three.

## Risks / Trade-offs

- [The BC250 board may have no hardware long-press override] → The 5 s `PS_ON#` backstop still cuts power. Test on hardware.
- [GPIO2 internal pull-down during reset on ESP32-CAM, strapping pin] → Weak relative to the motherboard pull-up and normal boot ignores GPIO2 when GPIO0 is high. Verify on hardware that it does not register as a press.
- [GPIO2 held HIGH by the motherboard blocks UART download mode on ESP32-CAM] → Document: unplug the `PWR_BTN#` wire to flash over UART. OTA is unaffected.
- [~3.4 V is close to the ~3.6 V ESP32 GPIO limit] → Document the measurement and advise re-checking on a different motherboard; a MOSFET buffer remains a later option.
- [Button debounce adds ~30 ms to the mirrored press edges] → Negligible against motherboard power-button timing.
- [Mirroring changes a previously inert gesture (press while ON)] → Only active when the user enables the feature.
