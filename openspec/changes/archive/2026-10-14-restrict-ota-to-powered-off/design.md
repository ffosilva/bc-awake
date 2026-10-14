# Design

## Context

The ESP32 manages power control for the BC250 host machine and provides a persistent setup portal web interface and REST API over Wi-Fi Station mode.

The host power state is tracked in `src/main.cpp` using the `PowerState` enum (`STATE_OFF`, `STATE_BOOTING`, `STATE_ON`) and queried in `src/portal.cpp` via `getBoardStateName()` and the helper `isBoardPowered()`:
```cpp
static bool isBoardPowered() {
  const char *st = getBoardStateName();
  return (strcmp(st, "ON") == 0 || strcmp(st, "BOOTING") == 0);
}
```
Currently, `handleDevices()`, `handleListenerStart()`, and `handleNetwork()` check `isBoardPowered()` to guard operations that could disrupt or conflict with an active host session. However, the OTA endpoint (`/api/update`) and the frontend OTA card in `app/index.html` do not inspect the board state, allowing firmware uploads and subsequent microcontroller reboots while the host machine is running.

## Goals / Non-Goals

**Goals:**
- Enforce in `src/portal.cpp` that `/api/update` rejects requests with HTTP 409 Conflict if `isBoardPowered()` is true, preventing flash writes.
- Abort chunked upload processing immediately on chunk `index == 0` if `isBoardPowered()`, ensuring `Update.begin()` is never called.
- In `app/index.html`, extend `applyBoardStateLocks()` to disable the OTA file input (`#ota-file`) and upload button (`#ota-btn`) and display an explanatory warning notice when the board is in `STATE_ON` or `STATE_BOOTING`.
- Automatically re-enable file selection when the board returns to `STATE_OFF`, keeping the upload button disabled until a `.bin` file is chosen.
- Keep `include/portal_html.h` in sync via `scripts/build_html.py`.

**Non-Goals:**
- Modifying the dual-bank OTA partition table layout or flash geometry.
- Implementing automatic host shutdown prior to firmware update (user must shut down the host machine manually).
- Restricting other non-disruptive System tab actions (such as password changes).

## Decisions

### Decision 1: Reject OTA in both upload callback and response handler
In `src/portal.cpp`, `/api/update` uses two callbacks: `handleUpdateResponse` and `handleUpdateUpload`:
- In `handleUpdateUpload`:
  - If `isBoardPowered()`, abort immediately without invoking `Update.begin()`, writing bytes, or disabling Bluetooth.
  - If the machine powers on mid-transfer, abort the active update via `Update.abort()`.
- In `handleUpdateResponse`:
  - Check `isBoardPowered()` before validating update completion.
  - If powered on, return HTTP 409 Conflict with JSON `{"error":"Firmware update disabled while host machine is powered on"}`.

*Alternatives considered:*
- Only checking in `handleUpdateResponse`: Rejected because firmware chunks would already be written to the inactive flash bank, wasting flash write cycles and risking incomplete partition writes.

### Decision 2: Use HTTP 409 Conflict
HTTP 409 Conflict indicates the request cannot be fulfilled due to the current operational state of the device. This matches the existing convention in `handleDevices()` and `handleListenerStart()`.

*Alternatives considered:*
- HTTP 403 Forbidden: Indicates lack of permissions or authorization, rather than a transient operational state conflict.

### Decision 3: Centralize UI state in `applyBoardStateLocks()`
`app/index.html` already executes `applyBoardStateLocks()` upon receiving status updates from `/api/status`. Extending `applyBoardStateLocks()` to inspect `isBoardOn`:
- Disables `#ota-file`.
- Disables `#ota-btn`.
- Adds an inline notice (`#ota-readonly-notice`) informing the user that firmware updates are only available when the host is powered off.
- Clears the notice and re-enables `#ota-file` when the host powers off.

*Alternatives considered:*
- Separate periodic timer for the OTA card: Rejected to avoid duplicate status polling and keep UI locking logic cohesive in one function.

## Risks / Trade-offs

- **[Risk] Host machine powers on while an OTA upload is already in progress**
  → *Mitigation*: Each chunk received in `handleUpdateUpload` verifies `!isBoardPowered()`. If the host turns on mid-stream, `Update.abort()` is called and the upload is rejected.

- **[Risk] File input re-enabling re-activates upload button prematurely**
  → *Mitigation*: When `applyBoardStateLocks()` re-enables the controls after the board turns off, `#ota-btn` remains disabled unless a valid file has already been chosen in `#ota-file`.
