#!/usr/bin/env python3
"""Simple HTTP POST receiver that appends request details and body to a log file."""

from __future__ import annotations

import argparse
import datetime as dt
import shutil
import ssl
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


def build_handler(log_path: Path):
    class PostLoggerHandler(BaseHTTPRequestHandler):
        def log_message(self, fmt: str, *args):
            # Keep server console quiet; requests are written to log_path.
            return

        def do_POST(self):
            content_len = int(self.headers.get("Content-Length", "0"))
            payload = self.rfile.read(content_len)
            timestamp = dt.datetime.now(dt.timezone.utc).isoformat()

            with log_path.open("ab") as fp:
                fp.write(f"[{timestamp}] {self.client_address[0]} {self.path}\n".encode("utf-8"))
                for key, value in self.headers.items():
                    fp.write(f"{key}: {value}\n".encode("utf-8"))
                fp.write(b"\n")
                fp.write(payload)
                fp.write(b"\n\n---\n\n")

            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(b"ok\n")

    return PostLoggerHandler


def ensure_self_signed_cert(cert_path: Path, key_path: Path) -> None:
    if cert_path.exists() and key_path.exists():
        return

    openssl = shutil.which("openssl")
    if not openssl:
        raise RuntimeError(
            "--https requires openssl to generate a self-signed certificate when cert/key are missing"
        )

    cert_path.parent.mkdir(parents=True, exist_ok=True)
    key_path.parent.mkdir(parents=True, exist_ok=True)

    cmd = [
        openssl,
        "req",
        "-x509",
        "-newkey",
        "rsa:2048",
        "-sha256",
        "-days",
        "3650",
        "-nodes",
        "-subj",
        "/CN=localhost",
        "-addext",
        "subjectAltName=DNS:localhost,IP:127.0.0.1",
        "-keyout",
        str(key_path),
        "-out",
        str(cert_path),
    ]

    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main() -> int:
    parser = argparse.ArgumentParser(description="Receive HTTP POST requests and log them to a file")
    parser.add_argument("--host", default="0.0.0.0", help="Bind host (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=5000, help="Bind port (default: 5000)")
    parser.add_argument("--log", default="post_requests.log", help="Log file path")
    parser.add_argument("--https", action="store_true", help="Enable HTTPS with TLS")
    parser.add_argument("--cert", default="tools/certs/localhost.crt", help="TLS cert path")
    parser.add_argument("--key", default="tools/certs/localhost.key", help="TLS private key path")
    args = parser.parse_args()

    log_path = Path(args.log)
    handler = build_handler(log_path)
    server = HTTPServer((args.host, args.port), handler)

    scheme = "http"
    if args.https:
        cert_path = Path(args.cert)
        key_path = Path(args.key)
        ensure_self_signed_cert(cert_path, key_path)

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_cert_chain(certfile=str(cert_path), keyfile=str(key_path))
        server.socket = ctx.wrap_socket(server.socket, server_side=True)
        scheme = "https"

    print(f"Listening on {scheme}://{args.host}:{args.port}/")
    print(f"Logging POST requests to: {log_path}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
