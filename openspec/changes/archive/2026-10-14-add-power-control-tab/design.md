# Design

## Context

See [proposal.md](proposal.md) for motivation.

The device runs `ESPAsyncWebServer` to serve `app/index.html` (embedded as a gzipped byte array via `portal_html.h`) and handle REST API endpoints. Meanwhile, `normalLoop()` in `src/main.cpp` executes synchronously on the primary core, servicing button debounce, motherboard sense (`TPMS1`), ATX `PS_ON#`, and motherboard `PWR_BTN#` pulsing.

Previously, graceful shutdown was housed in the System tab alongside administrative operations (OTA update, password change, factory reset), and the web interface lacked endpoints or UI elements to power on or force cut the ATX power supply.

## Goals / Non-Goals

**Goals:**
- Insert a dedicated **Power** tab between the Network and System tabs: `[Bluetooth] [Network] [Power] [System]`.
- Provide direct ATX power rail control: **Power On** (from `STATE_OFF`) and **Force Cut Power** (from `STATE_ON` or `STATE_BOOTING`).
- Guard **Force Cut Power** with an in-page modal confirmation dialog to prevent accidental data loss.
- Move motherboard power button configuration (`gracefulShutdown`) and the 300 ms pulse action ("Shut Down OS") entirely into the Power tab.
- Dispatch power-on and power-off requests safely through volatile loop flags to preserve monotonic timing and thread safety.

**Non-Goals:**
- Modifying GPIO pin assignments or hardware wiring (reuses `PS_ON_PIN` and `PWR_BTN_PIN`).
- Exposing unauthenticated power actions (all endpoints require valid `X-Auth-Token`).
- Introducing configurable power-button pulse lengths (retains standard 300 ms pulse).

## Decisions

### Decision 1: Asynchronous Request Queuing via Volatile Flags
**Choice**: When `/api/power/on` or `/api/power/off` is requested, the HTTP handler validates the current power state, sets `powerOnRequested = true` or `powerOffRequested = true`, and returns HTTP 200. The flags are processed at the start of `normalLoop()` in `src/main.cpp`.

**Rationale**: `ESPAsyncWebServer` executes on a background WiFi/LwIP task. Invoking `powerOn()` or `powerOff()` directly from the HTTP callback would risk race conditions with the `BUTTON_SENSE` debouncer, `bootStart` timestamp math, and concurrent GPIO writes.

**Alternatives Considered**:
- Direct synchronous execution in HTTP handler: Rejected due to clock monotonicity and race hazards with the state machine.

### Decision 2: Distinct REST Endpoints for Power Actions
**Choice**: Expose `POST /api/power/on` and `POST /api/power/off`, alongside the existing `POST /api/shutdown` (for button pulse) and `POST /api/power` (for saving `gracefulShutdown` configuration).

**Rationale**: Follows the existing API conventions (`/api/shutdown`, `/api/reset`, `/api/finish`). Keeps power commands atomic, intuitive for automation (e.g., Home Assistant or curl), and cleanly separated from configuration persistence.

**Alternatives Considered**:
- Multiplexing actions into `POST /api/power` with `{ "action": "on" }`: Rejected because `POST /api/power` is already established for persisting the `gracefulShutdown` boolean config.

### Decision 3: In-Page DOM Modal for Force Cut Confirmation
**Choice**: Implement the Force Cut confirmation using a dedicated modal overlay within `app/index.html`, matching the existing controller listener and factory reset confirmation patterns.

**Rationale**: Project specifications (`Captive Portal User Interface Resilience`) forbid browser-native dialogs (`confirm`, `alert`). An in-page modal ensures consistent styling on mobile devices and prevents captive portal browsers from suppressing prompts.

**Alternatives Considered**:
- Browser `confirm()`: Rejected per project UI resilience requirements.
- Inline reveal box: A full modal overlay provides strong visual interruption suited for high-risk actions like abrupt power cuts.

### Decision 4: Tab Placement and Migration
**Choice**: Place `Power` as the 3rd tab (`[Bluetooth] [Network] [Power] [System]`). Completely remove the Graceful Shutdown card from the System tab and place it in the Power tab.

**Rationale**: Matches the user's preference to place operational controls near configuration, and groups all power telemetry and actions into a single coherent screen.

## Risks / Trade-offs

- **[Risk]** Accidental power cut causing OS corruption.
  - **Mitigation**: Red danger button styling, active only when machine is on/booting, backed by an explicit confirmation modal with warning text.
- **[Risk]** Rapid clicking of Power On / Power Off.
  - **Mitigation**: Endpoints return HTTP 409 Conflict if the requested state transition is invalid for the current state. The UI disables buttons during in-flight requests and dynamically reflects state changes from `/api/status` polling.
- **[Risk]** Flash memory footprint increase from additional HTML/JS.
  - **Mitigation**: CSS classes and modal structure are reused from existing patterns. `scripts/build_html.py` compresses the asset via gzip, keeping binary impact under 1 KB.
