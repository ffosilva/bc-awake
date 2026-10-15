# Design: Multi-Controller Wake Parity and Dynamic Runtime Updates

## Context
See `proposal.md` for background and problem motivation.

In the current firmware:
- `include/config.h` defines `config.controllers` as a `std::vector<BoundController>` supporting up to 8 bound gamepads.
- `src/wake_classic.cpp` converts `config.controllers` into a vector of MAC strings and passes them to `btSetWakeAddrs()` inside `wakeBegin()`, but provides no API to update them while running.
- `src/wake_ble.cpp` only compares scanned BLE advertisements against `config.wakeAddr` in `WakeScanCallbacks::onResult`.
- `src/portal.cpp` modifies `config.controllers` via `addController()` and `removeController()`, but never triggers a wake engine update. Because the ESP32 is powered via 5V standby (`5VSB`), it does not restart when the PC powers off, leaving active wake tables stale.

## Goals / Non-Goals

**Goals:**
- Provide a unified wake abstraction (`wakeUpdateControllers()`) in `include/wake.h` implemented across all hardware targets.
- Enable immediate, runtime updates to active wake filters when controllers are added or removed in `src/portal.cpp`.
- Enable BLE scanning in `src/wake_ble.cpp` to match any bound controller from `config.controllers` (up to 8).
- Ensure thread-safe matching in BLE and Classic BT callback routines with sub-microsecond execution time.

**Non-Goals:**
- Supporting more than 8 bound controllers.
- Implementing active pairing or SDP/HID handshake spoofing for Joy-Cons beyond standard Classic BT page rejection.

## Decisions

### Decision 1: Unified `wakeUpdateControllers()` Abstraction in `wake.h`
- **Decision**: Declare `void wakeUpdateControllers()` in `include/wake.h`.
- **Implementation**:
  - In `src/wake_classic.cpp`: Extracts MAC strings from `config.controllers` and calls `btSetWakeAddrs(addrs)`.
  - In `src/wake_ble.cpp`: Copies normalized MAC addresses into a fixed-size buffer under spinlock protection for fast lookup by scan callbacks.
- **Rationale**: Keeps `src/portal.cpp` decoupled from hardware-specific Bluetooth headers (`bt_classic.h` vs `NimBLEDevice.h`).

### Decision 2: Fixed-Size In-Memory Buffer for BLE Wake Matching
- **Decision**: In `src/wake_ble.cpp`, maintain an in-memory array `static char bleWakeAddrs[8][18]` and `static int bleWakeAddrCount` protected by a spinlock (`portMUX_TYPE bleWakeMux`).
- **Matching**: Inside `WakeScanCallbacks::onResult`, acquire the lock (or use atomic read) and iterate through the 0..count-1 buffer using `strcasecmp`.
- **Rationale**: Scanning callbacks run on NimBLE host tasks. Accessing `std::vector<BoundController>` directly from an external callback risks race conditions or reallocation while iterating. A fixed-size array of 8 strings takes minimal RAM (< 150 bytes) and requires no heap allocation.
- **Alternatives Considered**: Direct iteration over `config.controllers` without locks (unsafe when portal adds controllers concurrently).

### Decision 3: Invoking `wakeUpdateControllers()` on Portal Mutations
- **Decision**: In `src/portal.cpp`, invoke `wakeUpdateControllers()` immediately following successful calls to `addController()` in `handleControllersAdd()` and `removeController()` in `handleControllersRemove()`.
- **Rationale**: Guarantees that the active in-memory wake lookup table always matches NVS state without requiring an ESP32 reboot.

## Risks / Trade-offs

- **[Risk] BLE scanner iteration latency with 8 addresses**
  &rarr; *Mitigation*: Iterating up to 8 string comparisons takes under a microsecond on a 160MHz ESP32-C3, well within the packet processing budget.
- **[Risk] Race condition during runtime update**
  &rarr; *Mitigation*: Protect address updates and read passes with existing spinlock (`stMux` in Classic BT, dedicated `bleWakeMux` in BLE).
