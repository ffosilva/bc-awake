#!/usr/bin/env python3
"""
BC250 ESP32 Device Sync Tool
Synchronizes local Linux BlueZ Bluetooth adapter MAC and paired gamepads
to the BC250 ESP32 switch via its REST API.
"""

import argparse
import configparser
import json
import os
import re
import socket
import subprocess
import sys
import urllib.error
import urllib.request

MAC_REGEX = re.compile(r"^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$")
DISCOVERY_PORT = 25025


def discover_esp32(timeout=2.0, port=DISCOVERY_PORT):
    """
    Broadcasts 'BC250_DISCOVER' on UDP port 25025 and waits for a unicast JSON response.
    Returns the resolved HTTP URL or None if not found.
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.settimeout(timeout)
    try:
        s.sendto(b"BC250_DISCOVER", ("255.255.255.255", port))
        while True:
            try:
                data, addr = s.recvfrom(1024)
                info = json.loads(data.decode("utf-8"))
                if info.get("device") == "bc250-switch":
                    ip = info.get("ip") or addr[0]
                    web_port = info.get("port", 80)
                    hostname = info.get("hostname", "bc250-switch")
                    print(f"[SYNC] Discovered {info.get('device')} ({hostname}) at {ip}:{web_port} (state: {info.get('boardState')})")
                    return f"http://{ip}:{web_port}"
            except (json.JSONDecodeError, UnicodeDecodeError):
                continue
    except socket.timeout:
        return None
    except Exception as e:
        print(f"[SYNC] Discovery warning: {e}", file=sys.stderr)
        return None
    finally:
        s.close()


def run_command(cmd):
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return res.stdout.strip()
    except (subprocess.SubprocessError, FileNotFoundError) as e:
        return ""


def get_local_adapter_mac():
    out = run_command(["bluetoothctl", "list"])
    for line in out.splitlines():
        match = re.search(r"Controller\s+([0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5})", line)
        if match:
            mac = match.group(1).lower()
            if "[default]" in line or not out:
                return mac
            # Return first controller if no explicit default marker
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


def api_request(url, path, method="GET", body=None, token=None, timeout=10):
    endpoint = f"{url.rstrip('/')}{path}"
    data = json.dumps(body).encode("utf-8") if body else None
    headers = {"Content-Type": "application/json"}
    if token:
        headers["X-Auth-Token"] = token

    req = urllib.request.Request(endpoint, data=data, headers=headers, method=method)
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        content = resp.read().decode("utf-8")
        if content:
            return json.loads(content)
        return {}


def main():
    parser = argparse.ArgumentParser(description="Synchronize BlueZ Bluetooth devices to BC250 ESP32 switch")
    parser.add_argument("--url", default=None, help="ESP32 URL (default: http://bc250-switch.local)")
    parser.add_argument("--password", default=None, help="Portal password")
    parser.add_argument("--config", default=None, help="Path to config file")
    parser.add_argument("--dry-run", action="store_true", help="Inspect and display changes without applying")
    args = parser.parse_args()

    # Load configuration file if specified or found in standard locations
    config_paths = [
        args.config,
        "/etc/bc250-sync.conf",
        os.path.expanduser("~/.config/bc250-sync/config.ini"),
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "bc250-sync.conf"),
    ]

    cfg = configparser.ConfigParser()
    for cp in config_paths:
        if cp and os.path.exists(cp):
            cfg.read(cp)
            break

    url = args.url or cfg.get("bc250", "url", fallback="auto")
    password = args.password or cfg.get("bc250", "password", fallback="")
    timeout = cfg.getint("bc250", "timeout", fallback=10)

    if url.strip().lower() == "auto":
        print(f"[SYNC] URL set to 'auto'; broadcasting discovery probe on UDP port {DISCOVERY_PORT}...")
        discovered = discover_esp32(timeout=2.0)
        if discovered:
            url = discovered
        else:
            print("[SYNC] Auto-discovery timed out; falling back to http://bc250-switch.local")
            url = "http://bc250-switch.local"

    print(f"[SYNC] Connecting to ESP32 at {url}...")

    # Discover local BlueZ state
    adapter_mac = get_local_adapter_mac()
    if not adapter_mac:
        print("[SYNC] Error: No local Bluetooth adapter found via bluetoothctl.", file=sys.stderr)
        sys.exit(1)

    paired_devices = get_local_paired_devices()
    print(f"[SYNC] Local adapter MAC: {adapter_mac}")
    print(f"[SYNC] Local paired devices ({len(paired_devices)}):")
    for d in paired_devices:
        print(f"  - {d['mac']}: {d['name']}")

    if len(paired_devices) > 8:
        print(f"[SYNC] Notice: Local paired devices ({len(paired_devices)}) exceed ESP32 limit (8). Syncing first 8.")
        paired_devices = paired_devices[:8]

    # Authenticate with ESP32
    try:
        login_res = api_request(url, "/api/login", method="POST", body={"password": password}, timeout=timeout)
        token = login_res.get("token")
        if not token:
            print("[SYNC] Error: Authentication failed (no token received).", file=sys.stderr)
            sys.exit(1)
    except urllib.error.HTTPError as e:
        print(f"[SYNC] Authentication error: HTTP {e.code} - {e.reason}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"[SYNC] Connection error: {e}", file=sys.stderr)
        sys.exit(1)

    # Fetch status
    try:
        status = api_request(url, "/api/status", method="GET", token=token, timeout=timeout)
    except Exception as e:
        print(f"[SYNC] Failed to fetch device status: {e}", file=sys.stderr)
        sys.exit(1)

    remote_host = (status.get("hostAddr") or "").lower()
    remote_controllers = status.get("controllers") or []
    board_state = status.get("boardState", "UNKNOWN")
    print(f"[SYNC] ESP32 board state: {board_state}, host adapter: {remote_host}")

    # Synchronize adapter MAC
    if adapter_mac != remote_host:
        print(f"[SYNC] Host adapter MAC mismatch (local: {adapter_mac}, remote: {remote_host})")
        if not args.dry_run:
            api_request(url, "/api/host", method="POST", body={"addr": adapter_mac}, token=token, timeout=timeout)
            print(f"[SYNC] Updated host adapter MAC to {adapter_mac}")
        else:
            print(f"[SYNC] [DRY-RUN] Would update host adapter MAC to {adapter_mac}")

    # Reconcile bound controllers
    remote_map = {c["mac"].lower(): c.get("name", "") for c in remote_controllers}
    local_map = {d["mac"].lower(): d["name"] for d in paired_devices}

    # 1. Remove controllers that are no longer paired locally
    for r_mac in remote_map:
        if r_mac not in local_map:
            print(f"[SYNC] Controller no longer paired: {r_mac} ({remote_map[r_mac]})")
            if not args.dry_run:
                api_request(url, "/api/controllers/remove", method="POST", body={"mac": r_mac}, token=token, timeout=timeout)
                print(f"[SYNC] Removed {r_mac}")
            else:
                print(f"[SYNC] [DRY-RUN] Would remove {r_mac}")

    # 2. Add newly paired controllers
    for l_mac, l_name in local_map.items():
        if l_mac not in remote_map:
            print(f"[SYNC] Adding newly paired controller: {l_mac} ({l_name})")
            if not args.dry_run:
                api_request(url, "/api/controllers/add", method="POST", body={"mac": l_mac, "name": l_name}, token=token, timeout=timeout)
                print(f"[SYNC] Added {l_mac}")
            else:
                print(f"[SYNC] [DRY-RUN] Would add {l_mac}")
        elif remote_map[l_mac] != l_name:
            # Rename if friendly name changed
            print(f"[SYNC] Renaming controller {l_mac}: '{remote_map[l_mac]}' -> '{l_name}'")
            if not args.dry_run:
                api_request(url, "/api/controllers/rename", method="POST", body={"mac": l_mac, "name": l_name}, token=token, timeout=timeout)
                print(f"[SYNC] Renamed {l_mac}")
            else:
                print(f"[SYNC] [DRY-RUN] Would rename {l_mac}")

    print("[SYNC] Synchronization completed successfully.")


if __name__ == "__main__":
    main()
