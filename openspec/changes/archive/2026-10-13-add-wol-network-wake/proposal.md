# Proposal

## Why

The BC250 board receives power from an SFX PSU via PCIe and lacks standby power or a physical power button connection. When the PSU is off (`STATE_OFF`), the BC250's onboard Ethernet adapter is completely unpowered and cannot receive standard Wake-on-LAN packets. Adding Wi-Fi station connectivity and a Wake-on-LAN (WOL) magic packet listener to the ESP32 (which runs permanently on PSU 5V standby) allows users to wake the BC250 wirelessly over their local network using standard tools such as Moonlight, Steam Link, or mobile WOL apps.

## What Changes

- Add a Wake-on-LAN (WOL) UDP listener that runs while the system is in `STATE_OFF` and triggers power-on when receiving a standard magic packet directed at the BC250's Ethernet MAC address.
- Add an explicit enable/disable toggle for the WOL feature (`wolEnabled`). When disabled, the Wi-Fi radio remains completely off in normal mode for zero RF/power overhead and standalone offline operation.
- Add non-blocking Wi-Fi station (STA) management with automatic reconnection in normal mode when WOL is enabled.
- Add support for both password-protected and passwordless (open) Wi-Fi networks.
- Expand persistent configuration storage to persist WOL status, Wi-Fi SSID, Wi-Fi password (optional), and the target BC250 Ethernet MAC address.
- Update the Setup Portal (both captive web UI and REST API) to allow configuring Wi-Fi network parameters, toggling WOL, and entering the BC250 Ethernet MAC address.

## Capabilities

### New Capabilities
- `network-wake`: Wi-Fi station management in normal mode and Wake-on-LAN magic packet detection to power on the system from `STATE_OFF`.

### Modified Capabilities
- `configuration-storage`: Adds persistent storage in NVS for `wolEnabled`, `wifiSsid`, `wifiPassword`, and `bc250Mac`.
- `setup-portal`: Adds portal UI and API support for toggling WOL, provisioning Wi-Fi credentials (including open networks), and binding the target BC250 Ethernet MAC address.

## Impact

- Firmware binary size and RAM footprint: Uses lightweight `WiFiUDP` in normal mode without running an HTTP server, keeping memory usage minimal.
- Hardware targets: Supported on `esp32dev` (DevKitC), `esp32cam`, and `esp32-c3-devkitm-1` with 2.4 GHz radio coexistence between Bluetooth wake and Wi-Fi station mode.
- Backward compatibility: If WOL is unconfigured or disabled, the switch behaves identically to current offline firmware.
