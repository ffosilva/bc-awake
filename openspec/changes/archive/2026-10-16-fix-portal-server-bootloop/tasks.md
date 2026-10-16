# Tasks

## 1. Portal Server Lifecycle

- [x] 1.1 Add defensive guard inside `portalServerBegin()` in `src/portal.cpp` to return early if `WiFi.getMode() == WIFI_OFF`
- [x] 1.2 Remove premature `portalServerBegin()` invocation from `normalBegin()` in `src/main.cpp`
- [x] 1.3 Invoke `portalServerBegin()` inside `networkWakeLoop()` in `src/network_wake.cpp` upon successful Wi-Fi Station connection

## 2. Verification and Build Validation

- [x] 2.1 Build firmware for all targets (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`) using PlatformIO
- [x] 2.2 Validate change artifacts with `openspec validate fix-portal-server-bootloop --type change`
