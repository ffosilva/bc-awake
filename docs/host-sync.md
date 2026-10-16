# BC250 Host Device Sync Tool (`bc250-sync`)

The `bc250-sync` utility is a lightweight, zero-dependency Python 3 tool and systemd service that automatically synchronizes your host PC's Bluetooth adapter MAC address and paired gamepad controllers to the BC250 ESP32 power switch via its REST API.

Because the ESP32 disables over-the-air Bluetooth sensing while the BC250 host is powered on (to avoid RF and HCI collisions with the host's active Bluetooth adapter), `bc250-sync` bridges the host's paired devices directly to the switch while the system is running.

> **Initial Setup vs. Ongoing Sync**: For initial device onboarding (setting password, Wi-Fi credentials, and initial controller bindings over the setup SoftAP), use the standalone [BC Awake Setup Wizard](../tools/bc250-setup/bc250-setup.py). `bc250-sync` is designed for ongoing background synchronization once the switch is on your home network.

---

## Features

- **Zero External Dependencies**: Implemented using pure Python 3 standard library (`socket`, `urllib`, `json`, `subprocess`).
- **Zero-Config UDP Auto-Discovery**: Automatically broadcasts on UDP port 25025 (`url = auto`) to locate the switch IP and custom hostname dynamically, falling back to mDNS.
- **Immutable Distro Ready**: Fully compatible with **Bazzite** and **SteamOS** without requiring `rpm-ostree` layering or `pip` package installation.
- **Systemd Integration**: Runs automatically on boot and every 5 minutes via a systemd timer.

---

## Installation

### 1. Copy the Script and Configuration

```bash
# Copy the sync script
sudo cp tools/bc250-sync/bc250-sync.py /usr/local/bin/
sudo chmod +x /usr/local/bin/bc250-sync.py

# Copy the configuration file
sudo cp tools/bc250-sync/bc250-sync.conf /etc/bc250-sync.conf
sudo chmod 600 /etc/bc250-sync.conf
```

> **Note for Bazzite / SteamOS User-Level Install (Alternative):**
> If you prefer not to write to `/usr/local/bin`, you can place the script in `~/.local/bin/bc250-sync.py` and configuration in `~/.config/bc250-sync/config.ini`, then run the service under `systemctl --user`.

### 2. Configure Your Password

Edit `/etc/bc250-sync.conf`:

```ini
[bc250]
# Use 'auto' for zero-config UDP broadcast discovery, or a specific URL
url = auto
password = your_portal_password
timeout = 10
```

### 3. Install and Enable Systemd Units

```bash
# Copy systemd unit and timer
sudo cp tools/bc250-sync/bc250-sync.service /etc/systemd/system/
sudo cp tools/bc250-sync/bc250-sync.timer /etc/systemd/system/

# Reload systemd and enable timer
sudo systemctl daemon-reload
sudo systemctl enable --now bc250-sync.timer
```

---

## Manual Execution & Verification

You can test synchronization manually anytime:

```bash
# Dry-run inspection (shows what would change without modifying ESP32 state)
python3 /usr/local/bin/bc250-sync.py --config /etc/bc250-sync.conf --dry-run

# Run synchronization pass
python3 /usr/local/bin/bc250-sync.py --config /etc/bc250-sync.conf
```

Check the systemd logs:

```bash
sudo journalctl -u bc250-sync.service -n 50 --no-pager
```

---

## Distribution Specific Notes

- **Bazzite / SteamOS**: `/usr/local/bin` and `/etc` are writable state directories on ostree and Btrfs layouts, so the standard install works cleanly out of the box.
- **Ubuntu / Debian**: Ensure `bluez` and `python3` are installed (`sudo apt install bluez python3`).
- **Arch Linux**: Standard BlueZ package (`bluez`, `bluez-utils`) provides `bluetoothctl`.
