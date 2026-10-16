#!/usr/bin/env python3
"""
Unit/integration test for bc250-setup.py using a mock HTTP server.
"""

import http.server
import json
import subprocess
import sys
import threading
import time

PORT = 25088


class MockESP32Handler(http.server.BaseHTTPRequestHandler):
    state = {
        "passwordSet": False,
        "token": "test-token-12345",
        "host": "",
        "controllers": [],
        "network": {},
        "finished": False
    }

    def do_GET(self):
        if self.path == "/api/status":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            doc = {
                "version": "1.2.0",
                "target": "esp32dev",
                "passwordSet": self.state["passwordSet"],
                "setupMode": True,
                "needsHost": True,
                "boardState": "ON"
            }
            self.wfile.write(json.dumps(doc).encode())
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = json.loads(self.rfile.read(length).decode()) if length > 0 else {}

        if self.path == "/api/password":
            self.state["passwordSet"] = True
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"token": self.state["token"]}).encode())
        elif self.path == "/api/login":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"token": self.state["token"]}).encode())
        elif self.path == "/api/host":
            self.state["host"] = body.get("addr")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"ok":true}')
        elif self.path == "/api/controllers/add":
            self.state["controllers"].append(body)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"ok":true}')
        elif self.path == "/api/network":
            self.state["network"] = body
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"ok":true}')
        elif self.path == "/api/finish":
            self.state["finished"] = True
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"ok":true}')
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, format, *args):
        pass  # Quiet logging


def main():
    server = http.server.HTTPServer(("127.0.0.1", PORT), MockESP32Handler)
    t = threading.Thread(target=server.serve_forever)
    t.daemon = True
    t.start()

    time.sleep(0.1)

    cmd = [
        sys.executable,
        "tools/bc250-setup/bc250-setup.py",
        "--url", f"http://127.0.0.1:{PORT}",
        "--password", "secretpass123",
        "--wifi-ssid", "TestSSID",
        "--wifi-pass", "wifiPass123",
        "--bc250-mac", "00:11:22:33:44:55",
        "--non-interactive"
    ]

    res = subprocess.run(cmd, capture_output=True, text=True)
    print("STDOUT:", res.stdout)
    if res.stderr:
        print("STDERR:", res.stderr)

    assert res.returncode == 0, f"Script failed with code {res.returncode}"
    assert MockESP32Handler.state["passwordSet"] is True, "Password was not set"
    assert MockESP32Handler.state["network"]["wifiSsid"] == "TestSSID", "SSID mismatch"
    assert MockESP32Handler.state["network"]["bc250Mac"] == "00:11:22:33:44:55", "MAC mismatch"
    assert MockESP32Handler.state["finished"] is True, "Finish was not called"

    server.shutdown()
    print("ALL MOCK TESTS PASSED!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
