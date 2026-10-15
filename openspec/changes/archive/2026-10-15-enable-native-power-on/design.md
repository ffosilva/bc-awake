# Design

## Context

See `proposal.md` for motivation. Currently, `src/main.cpp` monitors `BUTTON_SENSE` for user input (short tap to power on, 8 s hold for setup portal, 5 s hold while ON for force-off). `PWR_BTN_PIN` is currently kept as a high-impedance `INPUT` and only switched to `OUTPUT` + `LOW` while in `STATE_ON` to mirror `BUTTON_SENSE` or execute an authenticated 300 ms portal shutdown pulse.

Hardware measurements confirmed that when the BC250 motherboard is unpowered (in `STATE_OFF`), the `PWR_BTN#` line presents an open circuit (> 2 MΩ) to ground. It does not clamp the pin to GND or draw leakage current, making it safe for the ESP32 to pull the line up to 3.3 V using `INPUT_PULLUP`.

## Goals / Non-Goals

**Goals:**
- Enable the native BC250 power button (wired to `PWR_BTN_PIN`) to turn on the machine from `STATE_OFF`.
- Support the 8 s setup portal hold from `STATE_OFF` and the 5 s force power-off hold from `STATE_ON` via the native button.
- Unify button input processing between `BUTTON_SENSE` and `PWR_BTN_PIN` to avoid duplicated state machines.
- Maintain 100% backward compatibility when the feature is disabled.
- Reuse the existing `gracefulShutdown` configuration setting without altering NVS schemas or breaking existing installations.

**Non-Goals:**
- Adding a separate NVS configuration key or portal checkbox (unified toggle chosen per user preference).
- Altering hardware pin assignments or adding external pull-up resistors or level-shifting circuitry.
- Modifying Bluetooth wake or Wake-on-LAN subsystems.

## Decisions

### Decision 1: Unified input debounce and press tracking
In `normalLoop()`, evaluate raw button input by combining `BUTTON_SENSE` and `PWR_BTN_PIN`:
```cpp
bool extBtnPressed = (digitalRead(BUTTON_SENSE) == LOW);
bool pwrBtnPressed = config.gracefulShutdown && (digitalRead(PWR_BTN_PIN) == LOW);
bool buttonRaw = extBtnPressed || pwrBtnPressed;
```
- **Rationale**: Combining both inputs into `buttonRaw` allows the native button to transparently inherit the existing debounce window (30 ms), short-tap power-on on release, 8 s hold for setup mode, and 5 s force-off hold without duplicating timing logic or creating race conditions.
- **Alternative considered**: Separate debounce variables and timers for `PWR_BTN_PIN`. Rejected because it duplicates ~50 lines of monotonic clock comparisons and button tracking with no functional benefit.

### Decision 2: Pin mode lifecycle management
Manage `PWR_BTN_PIN` mode based on power state and configuration:
- In `STATE_OFF`: if `config.gracefulShutdown` is enabled, set `pinMode(PWR_BTN_PIN, INPUT_PULLUP)`; otherwise `pinMode(PWR_BTN_PIN, INPUT)`.
- In `STATE_BOOTING`: set `pinMode(PWR_BTN_PIN, INPUT)`.
- In `STATE_ON`: set `pinMode(PWR_BTN_PIN, INPUT)`, switching to `OUTPUT` + `LOW` only when actively driving a portal shutdown pulse or mirroring `BUTTON_SENSE`.
- **Rationale**: In `STATE_OFF`, the internal pull-up raises the line to 3.3 V so the native button shorting to GND can be sensed. In `STATE_BOOTING` and `STATE_ON`, the line must return to high-impedance `INPUT` because the BC250 motherboard energizes its own 3.3 V rail and provides its own ~3.4 V pull-up.
- **Alternative considered**: Keeping `INPUT_PULLUP` active in all states. Rejected because having the ESP32 pull-up active simultaneously with the motherboard's active pull-up while ON is unnecessary and could shift DC logic levels or back-power signals during shutdown transitions.

### Decision 3: Reusing existing `gracefulShutdown` configuration
The existing `gracefulShutdown` boolean (persisted under NVS key `graceful`) is repurposed as the unified motherboard power button toggle:
- When disabled (default): `PWR_BTN_PIN` is strictly `INPUT` in all states, exactly matching legacy behavior.
- When enabled: `PWR_BTN_PIN` acts as `INPUT_PULLUP` in `STATE_OFF` (power-on and setup hold) and `INPUT` with graceful shutdown mirroring in `STATE_ON`.
- **Rationale**: Avoids NVS migration and UI clutter. A user connecting `PWR_BTN#` wants full motherboard button integration (both turn-on and turn-off).

### Decision 4: Safe button mirroring in `STATE_ON`
When `state == STATE_ON` and the native button is pressed, the native button physically shorts `PWR_BTN#` to GND on the motherboard directly. The ESP32 does not need to drive the pin LOW for presses on `PWR_BTN_PIN`—it only drives `PWR_BTN_PIN` LOW when `BUTTON_SENSE` (the external switch) is pressed or when an API shutdown pulse is requested.
- **Rationale**: Driving an already-grounded line from the ESP32 is redundant and keeping the pin in `INPUT` mode while monitoring it avoids unnecessary GPIO mode switching.

## Risks / Trade-offs

- **[Capacitive discharge / transient during shutdown transition to `STATE_OFF`]** → When the board drops power, the line drops to 0 V briefly until the ESP32 arms `INPUT_PULLUP`. Mitigation: `BOARD_OFF_DEBOUNCE_MS` (1500 ms) and `WAKE_WAKE_COOLDOWN_MS` ensure the transition has settled, and the 30 ms debounce filter rejects transient dips.
- **[ESP32-CAM GPIO2 strapping pin]** → GPIO2 is a strapping pin on ESP32-CAM. Internal pull-up is only armed via software in normal mode after boot. Flashing via UART with GPIO0 low occurs before software boot, so download mode is unaffected.
- **[Simultaneous press of both buttons]** → Because inputs are OR'd into `buttonRaw`, pressing both simultaneously simply acts as a single continuous press.
