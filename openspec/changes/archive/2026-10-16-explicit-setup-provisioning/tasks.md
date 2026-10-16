# Tasks

## 1. Persistent Storage and Provisioning State

- [x] 1.1 Add `provisioned` member to `Config` struct and declare `setProvisioned(bool)` in `include/config.h`
- [x] 1.2 Implement `setProvisioned(bool)`, NVS loading/saving under key `"provisioned"`, factory reset behavior, and updated `isConfigured()` logic in `src/config.cpp`
- [x] 1.3 Verify firmware compiles with `pio run` across active targets

## 2. Setup Lifecycle and Transition Handling

- [x] 2.1 Update `/api/finish` in `src/portal.cpp` to call `setProvisioned(true)`
- [x] 2.2 Update setup exit button-hold handlers in `src/main.cpp` and `src/portal.cpp` to call `setProvisioned(true)`
- [x] 2.3 Verify mock setup wizard tests pass with `python3 tools/bc250-setup/test_setup.py`
- [x] 2.4 Verify full firmware build passes with `pio run`

## Workflow follow-up

- Sync spec deltas to main specs.
- Archive the change once implementation is confirmed.
