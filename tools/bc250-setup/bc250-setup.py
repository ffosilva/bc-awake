#!/usr/bin/env python3
"""
BC Awake Setup Wizard
Discovers local BlueZ Bluetooth adapter, paired gamepads, and network interfaces,
and provisions the BC250 ESP32 switch via its REST API over Wi-Fi SoftAP.
"""

import argparse
import getpass
import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.request

DEFAULT_PORTAL_URL = "http://192.168.4.1"
MAC_REGEX = re.compile(r"^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$")


def run_command(cmd):
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return res.stdout.strip()
    except (subprocess.SubprocessError, FileNotFoundError):
        return ""


def get_local_adapter_mac():
    out = run_command(["bluetoothctl", "list"])
    for line in out.splitlines():
        match = re.search(r"Controller\s+([0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5})", line)
        if match:
            mac = match.group(1).lower()
            if "[default]" in line or not out:
                return mac
            return mac
    return ""


def get_local_paired_devices():
    devices = []
    out = run_command(["bluetoothctl", "devices"])
    for line in out.splitlines():
        match = re.search(r"Device\s+([0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5})\s+(.*)", line)
        if match:
            mac = match.group(1).lower()
            name = match.group(2).strip() or "Wireless Controller"
            devices.append({"mac": mac, "name": name})
    return devices


def get_local_network_interfaces():
    interfaces = []
    net_path = "/sys/class/net"
    if os.path.exists(net_path):
        for iface in sorted(os.listdir(net_path)):
            if iface == "lo":
                continue
            addr_file = os.path.join(net_path, iface, "address")
            operstate_file = os.path.join(net_path, iface, "operstate")
            if os.path.exists(addr_file):
                try:
                    with open(addr_file, "r", encoding="utf-8") as f:
                        mac = f.read().strip().lower()
                    state = "unknown"
                    if os.path.exists(operstate_file):
                        with open(operstate_file, "r", encoding="utf-8") as f:
                            state = f.read().strip()
                    if mac and mac != "00:00:00:00:00:00" and MAC_REGEX.match(mac):
                        interfaces.append({"name": iface, "mac": mac, "state": state})
                except OSError:
                    continue

    if not interfaces:
        out = run_command(["ip", "link"])
        current_iface = ""
        for line in out.splitlines():
            m_iface = re.match(r"^\d+:\s+([^:]+):", line)
            if m_iface:
                current_iface = m_iface.group(1).split("@")[0]
            m_mac = re.search(r"link/ether\s+([0-9a-fA-F:]{17})", line)
            if m_mac and current_iface and current_iface != "lo":
                interfaces.append({"name": current_iface, "mac": m_mac.group(1).lower(), "state": "up"})

    return interfaces


def api_request(url, path, method="GET", body=None, token=None, timeout=10):
    endpoint = f"{url.rstrip('/')}{path}"
    data = json.dumps(body).encode("utf-8") if body is not None else None
    headers = {"Content-Type": "application/json"}
    if token:
        headers["X-Auth-Token"] = token

    req = urllib.request.Request(endpoint, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            content = resp.read().decode("utf-8")
            if content:
                return json.loads(content)
            return {}
    except urllib.error.HTTPError as e:
        err_msg = f"HTTP {e.code}"
        try:
            err_json = json.loads(e.read().decode("utf-8"))
            if "error" in err_json:
                err_msg += f": {err_json['error']}"
        except Exception:
            pass
        raise RuntimeError(err_msg) from e
    except urllib.error.URLError as e:
        raise ConnectionError(f"Cannot reach ESP32 at {url}: {e.reason}") from e


def authenticate(url, password, password_set):
    if not password_set:
        print("[SETUP] First-time setup: Creating new portal password...")
        resp = api_request(url, "/api/password", method="POST", body={"password": password})
        token = resp.get("token")
        if not token:
            raise RuntimeError("Password set succeeded but no session token received.")
        return token
    else:
        print("[SETUP] Authenticating with existing portal password...")
        resp = api_request(url, "/api/login", method="POST", body={"password": password})
        token = resp.get("token")
        if not token:
            raise RuntimeError("Login failed: invalid password.")
        return token


def main():
    parser = argparse.ArgumentParser(description="BC Awake Onboarding & Setup Wizard")
    parser.add_argument("--url", default=DEFAULT_PORTAL_URL, help=f"ESP32 Portal URL (default: {DEFAULT_PORTAL_URL})")
    parser.add_argument("--password", default=None, help="Portal password (prompted if omitted)")
    parser.add_argument("--wifi-ssid", default=None, help="Home Wi-Fi SSID")
    parser.add_argument("--wifi-pass", default="", help="Home Wi-Fi password")
    parser.add_argument("--bc250-mac", default=None, help="BC250 Ethernet MAC address for WOL")
    parser.add_argument("--hostname", default="bc-awake", help="Device mDNS hostname (default: bc-awake)")
    parser.add_argument("--dry-run", action="store_true", help="Inspect local devices and show actions without applying")
    parser.add_argument("--non-interactive", action="store_true", help="Fail if any required input is missing")
    args = parser.parse_args()

    print("==================================================")
    print("           BC Awake Setup Wizard                  ")
    print("==================================================")

    # 1. Check local devices
    local_adapter = get_local_adapter_mac()
    paired_controllers = get_local_paired_devices()
    network_interfaces = get_local_network_interfaces()

    print(f"\n[HOST ] Local Bluetooth Adapter: {local_adapter or '(none detected via BlueZ)'}")
    print(f"[HOST ] Paired Gamepads: {len(paired_controllers)} found")
    for c in paired_controllers:
        print(f"        - {c['name']} ({c['mac']})")

    print(f"[HOST ] Network Interfaces: {len(network_interfaces)} found")
    for iface in network_interfaces:
        print(f"        - {iface['name']}: {iface['mac']} ({iface['state']})")

    if args.dry_run:
        print("\n[DRY  ] Dry-run requested. Exiting without contacting ESP32.")
        return 0

    # 2. Check connection to ESP32
    print(f"\n[SETUP] Connecting to ESP32 at {args.url}...")
    try:
        status = api_request(args.url, "/api/status")
    except ConnectionError as e:
        print(f"\n[ERROR] {e}", file=sys.stderr)
        print("Please ensure your BC250 is connected to the 'BC250_XXXXXX' Wi-Fi network.", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"\n[ERROR] Failed to query ESP32 status: {e}", file=sys.stderr)
        return 1

    print(f"[SETUP] Connected! Device target: {status.get('target', 'unknown')} (fw v{status.get('version', 'unknown')})")
    print(f"[SETUP] Setup Mode: {status.get('setupMode')}, Board State: {status.get('boardState')}")

    # 3. Authentication
    password_set = status.get("passwordSet", False)
    password = args.password
    if not password:
        if args.non_interactive:
            print("[ERROR] Password must be provided via --password in non-interactive mode.", file=sys.stderr)
            return 1
        prompt = "Create new portal password (min 6 chars): " if not password_set else "Enter portal password: "
        while True:
            password = getpass.getpass(prompt)
            if not password_set:
                if len(password) < 6:
                    print("Password must be at least 6 characters long.")
                    continue
                confirm = getpass.getpass("Confirm password: ")
                if password != confirm:
                    print("Passwords do not match.")
                    continue
            break

    try:
        token = authenticate(args.url, password, password_set)
        print("[SETUP] Authentication successful!")
    except Exception as e:
        print(f"[ERROR] Authentication failed: {e}", file=sys.stderr)
        return 1

    # 4. Configure Bluetooth Host Adapter
    if status.get("needsHost", False):
        if local_adapter:
            print(f"\n[SETUP] Setting host Bluetooth adapter to {local_adapter}...")
            api_request(args.url, "/api/host", method="POST", body={"addr": local_adapter}, token=token)
            print("[SETUP] Host Bluetooth adapter saved!")
        else:
            print("\n[WARN ] No BlueZ adapter detected. Skipping host adapter configuration.", file=sys.stderr)

    # 5. Bind Controllers
    if paired_controllers:
        print(f"\n[SETUP] Binding {len(paired_controllers)} paired controller(s)...")
        for c in paired_controllers[:8]:
            api_request(args.url, "/api/controllers/add", method="POST", body={"mac": c["mac"], "name": c["name"]}, token=token)
            print(f"        Bound: {c['name']} ({c['mac']})")

    # 6. Configure Network & WOL
    bc250_mac = args.bc250_mac
    if not bc250_mac:
        # Default to first non-loopback ethernet/interface or active interface
        for iface in network_interfaces:
            if iface["state"] == "up" and not iface["name"].startswith("wl"):
                bc250_mac = iface["mac"]
                break
        if not bc250_mac and network_interfaces:
            bc250_mac = network_interfaces[0]["mac"]

    wifi_ssid = args.wifi_ssid
    wifi_pass = args.wifi_pass

    if not wifi_ssid and not args.non_interactive:
        print("\n--- Home Wi-Fi Configuration ---")
        wifi_ssid = input("Enter home Wi-Fi SSID (leave empty to skip Wi-Fi setup): ").strip()
        if wifi_ssid:
            wifi_pass = getpass.getpass("Enter home Wi-Fi password (empty for open network): ")

    if wifi_ssid or bc250_mac:
        print(f"\n[SETUP] Configuring Network: SSID='{wifi_ssid or ''}', WOL MAC='{bc250_mac or ''}'...")
        net_payload = {
            "wolEnabled": bool(wifi_ssid),
            "wifiSsid": wifi_ssid or "",
            "wifiPassword": wifi_pass or "",
            "bc250Mac": bc250_mac or "",
            "hostname": args.hostname or "bc-awake"
        }
        api_request(args.url, "/api/network", method="POST", body=net_payload, token=token)
        print("[SETUP] Network configuration saved!")

    # 7. Finish setup
    if not args.non_interactive:
        confirm_finish = input("\nComplete setup and restart into normal mode? [Y/n]: ").strip().lower()
        if confirm_finish in ("", "y", "yes"):
            print("[SETUP] Finalizing configuration...")
            api_request(args.url, "/api/finish", method="POST", body={}, token=token)
            print("[SUCCESS] Setup complete! The ESP32 is restarting into normal mode.")
    else:
        api_request(args.url, "/api/finish", method="POST", body={}, token=token)
        print("[SUCCESS] Setup complete! The ESP32 is restarting into normal mode.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
