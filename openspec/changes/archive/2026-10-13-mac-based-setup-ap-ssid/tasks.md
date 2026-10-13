# Tasks

## 1. SoftAP Naming Implementation

- [x] 1.1 Update `include/board.h` to replace static `AP_SSID` with `AP_SSID_PREFIX = "BC250_"`, verifying the constant is defined and clean.
- [x] 1.2 Update `portalBegin()` in `src/portal.cpp` to read the factory eFuse MAC using `esp_read_mac(..., ESP_MAC_WIFI_STA)` and format the SSID as `BC250_%02X%02X%02X` from the last 3 MAC bytes, verifying that `WiFi.softAP()` receives the generated name and logs it to Serial.

## 2. Verification and Build Validation

- [x] 2.1 Build all PlatformIO environments (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`) using `pio run`, verifying zero-warning compilation across all targets.
- [x] 2.2 Verify SSID formatting logic to ensure uppercase 6-character hexadecimal representation of trailing MAC bytes and 12-character total string length.
