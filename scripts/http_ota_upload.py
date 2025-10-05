from __future__ import annotations

import base64
import socket
import ssl
from http.client import HTTPConnection, HTTPSConnection, RemoteDisconnected
from pathlib import Path
from typing import Iterable
from urllib.parse import urlsplit

from SCons.Script import DefaultEnvironment

OTA_USERNAME = "ota"
OTA_PASSWORD = "marbletrack"
UPLOAD_ENDPOINT = "/ota"


def _build_multipart(payload_path: Path) -> tuple[bytes, str]:
    boundary = "----platformio-ota-boundary"
    header = (
        f"--{boundary}\r\n"
        "Content-Disposition: form-data; name=\"firmware\"; filename=\""
        f"{payload_path.name}\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n"
    ).encode("utf-8")
    footer = f"\r\n--{boundary}--\r\n".encode("utf-8")

    with payload_path.open("rb") as fh:
        binary = fh.read()

    body = header + binary + footer
    return body, boundary


def _http_upload(source: Iterable[str], target: Iterable[str], env) -> None:  # type: ignore[override]
    firmware_path = Path(env.subst("$BUILD_DIR")) / f"{env.subst('$PROGNAME')}.bin"
    if not firmware_path.exists():
        raise FileNotFoundError(f"Firmware image not found: {firmware_path}")

    host = env.subst("$UPLOAD_PORT").strip()
    if not host:
        raise RuntimeError("UPLOAD_PORT is not defined. Pass --upload-port or set it in platformio.ini")

    if not host.startswith("http://") and not host.startswith("https://"):
        url = f"http://{host}{UPLOAD_ENDPOINT}"
    else:
        url = f"{host.rstrip('/')}{UPLOAD_ENDPOINT}"

    body, boundary = _build_multipart(firmware_path)
    parsed = urlsplit(url)
    path = parsed.path or "/"
    if parsed.query:
        path = f"{path}?{parsed.query}"

    port = parsed.port
    is_https = parsed.scheme == "https"
    if port is None:
        port = 443 if is_https else 80

    credentials = base64.b64encode(f"{OTA_USERNAME}:{OTA_PASSWORD}".encode("utf-8")).decode("utf-8")

    connection_kwargs = {"timeout": 180}
    if is_https:
        # Use default context; caller can override via upload_port scheme if needed
        connection = HTTPSConnection(parsed.hostname, port, context=ssl.create_default_context(), **connection_kwargs)
    else:
        connection = HTTPConnection(parsed.hostname, port, **connection_kwargs)

    print(f"Uploading {firmware_path.name} ({firmware_path.stat().st_size} bytes) to {url}")

    try:
        connection.connect()
        connection.putrequest("POST", path)
        connection.putheader("Host", parsed.netloc)
        connection.putheader("Content-Type", f"multipart/form-data; boundary={boundary}")
        connection.putheader("Content-Length", str(len(body)))
        connection.putheader("Authorization", f"Basic {credentials}")
        connection.putheader("Connection", "close")
        connection.endheaders()
        connection.send(body)
        if connection.sock is not None:
            connection.sock.settimeout(10)
        print("Upload sent, waiting for device response...")

        try:
            response = connection.getresponse()
        except RemoteDisconnected:
            print("Firmware upload completed; device closed connection before sending a response (likely rebooting).")
            return
        except ConnectionResetError:
            print("Firmware upload completed; connection reset by device (likely rebooting).")
            return
        except socket.timeout:
            print("Firmware upload completed; device is taking too long to respond (treating as success).")
            return

        response_text = response.read().decode("utf-8", errors="ignore").strip()
        print(f"HTTP OTA response: {response.status} {response.reason}")
        if response_text:
            print(response_text)
        if response.status >= 400:
            raise SystemExit(response.status)
    except Exception as err:
        print(f"Firmware upload failed: {err}")
        raise SystemExit(1)
    finally:
        connection.close()


env = DefaultEnvironment()
env.Replace(UPLOADCMD=_http_upload)
