#!/usr/bin/env python3
"""Extract and run-length encode PSX pad history from a live debug server."""

from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path


def load_debug_client(repo_root: Path):
    module_path = repo_root / "psxrecomp-v4" / "tools" / "debug_client.py"
    spec = importlib.util.spec_from_file_location("psxrecomp_debug_client", module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {module_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4525)
    parser.add_argument("--start", type=int, required=True)
    parser.add_argument("--end", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.start < 0 or args.end < args.start:
        parser.error("expected 0 <= start <= end")

    repo_root = Path(__file__).resolve().parents[1]
    debug_client = load_debug_client(repo_root)
    frames: list[dict] = []

    for start in range(args.start, args.end + 1, 200):
        end = min(start + 199, args.end)
        # The runtime intentionally serves one debug request per TCP client.
        # Reconnect for each chunk just like debug_client.py's CLI does.
        with debug_client.connect(args.host, args.port) as sock:
            response = debug_client.send_cmd(
                sock, {"cmd": "frame_range", "start": start, "end": end}
            )
        if not response.get("ok"):
            raise RuntimeError(response.get("error", "frame_range failed"))
        frames.extend(response["frames"])

    if len(frames) != args.end - args.start + 1:
        raise RuntimeError("debug server returned a non-contiguous frame range")

    unavailable: list[int] = []
    previous_pad: str | None = None
    for frame in frames:
        if frame.get("available", True):
            previous_pad = frame["pad"].upper()
            continue
        unavailable.append(frame["frame"])
        if previous_pad is None:
            raise RuntimeError(
                f"frame {frame['frame']} is unavailable before any pad sample"
            )
        # Turbo-accelerated VBlanks may advance the frame counter without a
        # retained frame record. Controller state is level-triggered, so carry
        # the last observed pad word through those omitted frames.
        frame["pad"] = previous_pad

    segments: list[dict] = []
    for frame in frames:
        buttons = frame["pad"].upper()
        if segments and segments[-1]["buttons"] == buttons:
            segments[-1]["frames"] += 1
        else:
            segments.append({"frames": 1, "buttons": buttons})

    payload = {
        "format_version": 1,
        "game_id": "SLUS-00561",
        "capture_start_frame": args.start,
        "capture_end_frame": args.end,
        "frame_count": len(frames),
        "carried_forward_frames": len(unavailable),
        "segments": segments,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(
        f"Captured {len(frames)} frames as {len(segments)} segments "
        f"to {args.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
