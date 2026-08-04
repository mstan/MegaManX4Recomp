#!/usr/bin/env python3
"""Launch MMX4 and queue an exact guest-VBlank digital-pad route."""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import socket
import subprocess
import time
from pathlib import Path


WINDOWS_ABSOLUTE_PATH = re.compile(r"^[A-Za-z]:[\\/]")


def executable_argument_path(value: str) -> str:
    """Return a path suitable for passing from MSYS Python to a Windows exe."""
    if WINDOWS_ABSOLUTE_PATH.match(value):
        return value
    return str(Path(value).resolve())


def load_debug_client(repo_root: Path):
    module_path = repo_root / "psxrecomp-v4" / "tools" / "debug_client.py"
    spec = importlib.util.spec_from_file_location("psxrecomp_debug_client", module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {module_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def request(debug_client, host: str, port: int, payload: dict) -> dict:
    response = debug_client.query(host, port, payload)
    if not response.get("ok"):
        raise RuntimeError(response.get("error", f"{payload['cmd']} failed"))
    return response


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("route", type=Path)
    parser.add_argument("--exe")
    parser.add_argument("--disc")
    parser.add_argument("--connect-only", action="store_true")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4525)
    parser.add_argument("--startup-timeout", type=float, default=30.0)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    route = json.loads(args.route.read_text(encoding="utf-8"))
    if route.get("format_version") != 1:
        raise RuntimeError("unsupported route format")
    segments = route.get("segments", [])
    if not segments:
        raise RuntimeError("route has no input segments")

    if not args.connect_only:
        if args.exe is None or args.disc is None:
            parser.error("--exe and --disc are required unless --connect-only is used")
        exe_path = Path(args.exe).resolve()
        subprocess.Popen(
            [
                str(exe_path),
                "--no-launcher",
                "--debug-port",
                str(args.port),
                "--disc",
                executable_argument_path(args.disc),
            ],
            cwd=exe_path.parent,
        )

    debug_client = load_debug_client(repo_root)
    deadline = time.monotonic() + args.startup_timeout
    while True:
        try:
            request(debug_client, args.host, args.port, {"cmd": "ping"})
            break
        except (ConnectionError, OSError, socket.timeout):
            if time.monotonic() >= deadline:
                raise RuntimeError("debug server did not become ready")
            time.sleep(0.05)

    request(debug_client, args.host, args.port, {"cmd": "input_route_clear"})
    for segment in segments:
        request(
            debug_client,
            args.host,
            args.port,
            {
                "cmd": "input_route_append",
                "frames": int(segment["frames"]),
                "buttons": int(segment["buttons"], 0),
            },
        )
    started = request(
        debug_client, args.host, args.port, {"cmd": "input_route_start"}
    )
    print(
        f"Queued {route['frame_count']} frames in {len(segments)} segments; "
        f"replay started at guest frame {started['start_frame']}."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
