"""Exercise two private X4 delay-sync peers, using host input and original assets.

Example (run only after rebuilding the co-op title):
  python tools/test_mmx4_coop_netplay.py --exe build-coop/MegaManX4Recomp.exe \
      --bios psxrecomp-v4/bios/SCPH1001.BIN --disc "mmx4/Mega Man X4.cue"

Each case copies the executable into its own run directory, uses an empty mod
catalog and private saves, and binds newly reserved loopback UDP/TCP ports. The
trusted profile must enable co-op itself. No RAM writes, save loads, lobby rooms,
or simulation-layer pad overrides are used. Only PIDs created here are stopped.
Native state reads are asynchronous observations; matching sim-tick CRCs and
the existing matched-state watermark provide the network agreement evidence.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import socket
import struct
import subprocess
import sys
import time
import tomllib
import traceback
import uuid

PLAY = 0x801721C0
PLAYER = 0x801418C8
BODY_BYTES = 0xE4
DIAGNOSTIC_MAGIC = 0x5834434F
START, CROSS, SQUARE, LEFT, RIGHT = 0x0008, 0x4000, 0x8000, 0x0080, 0x0020
CRC_LINE = re.compile(
    r"rb live dig local sim=(\d+) core=([0-9a-fA-F]{8}).*? mod=([0-9a-fA-F]{8})"
)


class ValidationError(RuntimeError):
    pass


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValidationError(reason)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def change_config(source: str, section: str, key: str, value: object) -> str:
    """Change one scalar without rewriting unrelated title configuration."""
    encoded = "true" if value is True else "false" if value is False else json.dumps(value)
    block = re.search(r"(?m)^\[" + re.escape(section) + r"\][ \t]*$", source)
    if not block:
        return source.rstrip() + f"\n\n[{section}]\n{key} = {encoded}\n"
    end = re.search(r"(?m)^\[", source[block.end():])
    stop = block.end() + end.start() if end else len(source)
    body = source[block.end():stop]
    pattern = re.compile(r"(?m)^[ \t]*" + re.escape(key) + r"[ \t]*=.*$")
    if pattern.search(body):
        body = pattern.sub(lambda _match: f"{key} = {encoded}", body, count=1)
    else:
        body = body.rstrip() + f"\n{key} = {encoded}\n\n"
    return source[:block.end()] + body + source[stop:]


def parse_crc_log(path: Path) -> dict[int, tuple[str, str]]:
    result: dict[int, tuple[str, str]] = {}
    for match in CRC_LINE.finditer(path.read_text(errors="replace")):
        tick = int(match[1])
        value = (match[2].lower(), match[3].lower())
        require(tick not in result or result[tick] == value,
                f"{path.name}: conflicting state digests for sim tick {tick}")
        result[tick] = value
    return result


@dataclass
class Peer:
    seat: int
    directory: Path
    tcp: int
    udp: int
    argv: list[str]
    environment: dict[str, str]
    process: subprocess.Popen | None = None
    diagnostic: int | None = None
    output: object | None = None

    @property
    def log(self) -> Path:
        return self.directory / "runtime.log"

    def launch(self) -> None:
        require(self.process is None, "Attempted to relaunch an owned peer")
        self.output = self.log.open("wb")
        flags = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0
        self.process = subprocess.Popen(
            self.argv, cwd=self.directory, env=self.environment,
            stdin=subprocess.DEVNULL, stdout=self.output, stderr=subprocess.STDOUT,
            creationflags=flags,
        )

    def running(self) -> bool:
        return self.process is not None and self.process.poll() is None

    def request(self, cmd: str, *, allow_error: bool = False, **fields) -> dict:
        require(self.running(), f"P{self.seat + 1} process exited before {cmd}")
        with socket.create_connection(("127.0.0.1", self.tcp), timeout=2) as connection:
            connection.settimeout(3)
            connection.sendall((json.dumps(dict(id=1, cmd=cmd, **fields)) + "\n").encode())
            with connection.makefile("rb") as stream:
                line = stream.readline(2 * 1024 * 1024)
            require(bool(line), f"P{self.seat + 1} closed its debug connection during {cmd}")
            result = json.loads(line)
        require(result.get("id") == 1, f"Unexpected debug response ID for {cmd}")
        require(allow_error or result.get("ok") is True, f"P{self.seat + 1}: {cmd}: {result}")
        return result

    def read(self, address: int, size: int) -> bytes:
        data = bytes.fromhex(self.request("read_ram", addr=hex(address), len=size)["hex"])
        require(len(data) == size, f"P{self.seat + 1}: truncated state read")
        return data

    def input(self, held: int = 0) -> None:
        # Both peers sample their own local host P1 device. The session routes
        # it to seat 0/1; writing simulation port 2 would bypass that contract.
        self.request("set_input", layer="host", buttons=f"0x{(~held) & 0xffff:04x}",
                     lx=128, ly=128, rx=128, ry=128)

    def status(self) -> dict:
        status = self.request("netplay_status")
        require(status.get("active") == 1, f"P{self.seat + 1}: netplay is inactive")
        require(status.get("rollback") == 0, f"P{self.seat + 1}: profile did not force delay-sync")
        require(status.get("players") == 2, f"P{self.seat + 1}: session is not two-player")
        require(status.get("desync") == 0, f"P{self.seat + 1}: input contract desynchronized: {status}")
        return status

    def observe(self) -> dict:
        if self.diagnostic is None:
            counters = self.request("mod_counters")["counters"]
            candidates = [int(row["count"]) for row in counters
                          if row["name"] == "mmx4.coop.diagnostic-address"]
            require(len(candidates) == 1 and candidates[0] != 0,
                    f"P{self.seat + 1}: trusted co-op profile did not activate")
            self.diagnostic = candidates[0]
        play = self.read(PLAY, 0x64)
        diagnostic = self.read(self.diagnostic, 0x300)
        first = self.read(PLAYER, BODY_BYTES)
        magic, frames, enrolled, failed, banks = struct.unpack_from("<5I", diagnostic)
        require(magic == DIAGNOSTIC_MAGIC, f"P{self.seat + 1}: invalid co-op diagnostic header")
        require(failed == 0, f"P{self.seat + 1}: co-op plugin reported failure")
        return dict(mode=play[0], minor=play[1], stage=play[0xC], section=play[0xD],
                    campaign=play[0x43], pause_suppressed=play[0x1C], script_gate=play[0x10],
                    frames=frames, enrolled=enrolled, failed=failed, banks=banks,
                    p1=body_state(first), p2=body_state(diagnostic[0x100:0x100 + BODY_BYTES]))

    def stop(self) -> None:
        if self.running():
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)
        if self.output is not None:
            self.output.close()
            self.output = None


def body_state(body: bytes) -> dict:
    return dict(active=body[0], character=body[2], visible=body[3], state=body[4],
                hp=body[0x5C] & 0x7F, x=struct.unpack_from("<i", body, 8)[0] / 65536,
                y=struct.unpack_from("<i", body, 12)[0] / 65536)


def reserve_ports() -> tuple[list[int], list[socket.socket]]:
    sockets, ports = [], []
    try:
        for kind in (socket.SOCK_DGRAM, socket.SOCK_DGRAM, socket.SOCK_STREAM, socket.SOCK_STREAM):
            while True:
                candidate = socket.socket(socket.AF_INET, kind)
                candidate.bind(("127.0.0.1", 0))
                port = candidate.getsockname()[1]
                if port not in ports:
                    break
                candidate.close()
            sockets.append(candidate)
            ports.append(port)
        return ports, sockets
    except BaseException:
        for connection in sockets:
            connection.close()
        raise


def prepare_peer(args, case: Path, seat: int, ports: list[int], jitter: bool,
                 session: int) -> Peer:
    directory = case / f"peer-{seat}"
    directory.mkdir()
    executable = directory / args.exe.name
    shutil.copy2(args.exe, executable)
    for dll in args.exe.parent.glob("*.dll"):
        if dll.is_file():
            shutil.copy2(dll, directory / dll.name)
    # No installed/bundled offline packages, no personal state selections.
    (directory / "mods").mkdir()
    (directory / "mods/state.toml").write_text("format_version = 2\n", encoding="utf-8")
    saves = directory / "saves"
    saves.mkdir()
    source = args.config.read_text(encoding="utf-8")
    original = tomllib.loads(source)
    game_exe = Path(original["game"]["exe"])
    if not game_exe.is_absolute():
        game_exe = args.game_root / game_exe
    for section, key, value in (
        ("game", "exe", str(game_exe.resolve())),
        ("game", "disc", str(args.disc)),
        ("runtime", "memcard_dir", str(saves)),
        ("runtime", "overlay_cache", False),
        ("runtime", "overlay_autocompile_cmd", ""),
        ("runtime", "turbo_loads", False),
        ("runtime", "bios_hle", False),
        ("video", "aspect_ratio", "4:3"),
        ("video", "renderer", "opengl"),
        ("video", "auto_skip_fmv", False),
        ("video", "frame_interpolation", False),
        ("netplay", "content_negotiation", False),
    ):
        source = change_config(source, section, key, value)
    config = directory / "game.toml"
    tomllib.loads(source)  # Refuse malformed config before starting any process.
    config.write_text(source, encoding="utf-8")
    environment = {key: value for key, value in os.environ.items()
                   if not key.upper().startswith(("PSX_", "RNET_"))}
    environment.update(PSX_NET_TRANSPORT="lan", PSX_NET_MODE="delay",
                       RNET_SIM_LATENCY_MS=str(args.latency if jitter else 0),
                       RNET_SIM_JITTER_MS=str(args.jitter if jitter else 0),
                       RNET_SIM_SEED=str(args.seed + seat), RNET_SIM_LOSS_PCT="0")
    argv = [str(executable), "--bios", str(args.bios), "--game", str(config),
            "--disc", str(args.disc), "--memcard-dir", str(saves), "--renderer", "opengl",
            "--no-launcher", "--headless-opengl" if args.frontend == "headless-opengl"
            else "--hidden-window", "--netplay", "--net-slot", str(seat),
            "--net-input-player", "0", "--net-bind", f"127.0.0.1:{ports[seat]}",
            "--net-peer", f"127.0.0.1:{ports[1 - seat]}", "--net-delay", str(args.delay),
            "--net-session-id", str(session), "--debug-port", str(ports[2 + seat])]
    return Peer(seat, directory, ports[2 + seat], ports[seat], argv, environment)


class Exercise:
    def __init__(self, args, peers: list[Peer], record: dict):
        self.args, self.peers, self.record = args, peers, record
        self.pool = ThreadPoolExecutor(max_workers=2)

    def both(self, method: str, *values) -> list:
        futures = [self.pool.submit(getattr(peer, method), *values) for peer in self.peers]
        return [future.result() for future in futures]

    def checkpoint(self, name: str, after_tick: int, min_digests: int = 2) -> dict:
        deadline = time.monotonic() + self.args.phase_timeout
        while time.monotonic() < deadline:
            statuses = self.both("status")
            logs = [parse_crc_log(peer.log) for peer in self.peers]
            common = sorted(set(logs[0]) & set(logs[1]))
            for tick in common:
                require(logs[0][tick] == logs[1][tick],
                        f"{name}: core/mod CRC mismatch at sim tick {tick}: "
                        f"{logs[0][tick]} vs {logs[1][tick]}")
            recent = [tick for tick in common if tick > after_tick]
            watermark = min(int(status["resolved_through"]) for status in statuses)
            agreed = [tick for tick in recent if tick <= watermark]
            if len(agreed) >= min_digests:
                samples = [dict(tick=tick, core=logs[0][tick][0], mod=logs[0][tick][1])
                           for tick in agreed]
                result = dict(name=name, after_tick=after_tick, statuses=statuses,
                              matched_crc_samples=samples)
                self.record["checkpoints"].append(result)
                print(f"{name}: {len(samples)} identical core/mod CRCs, matched through {watermark}", flush=True)
                return result
            time.sleep(.2)
        raise ValidationError(f"{name}: insufficient matched CRC evidence after sim tick {after_tick}")

    def wait_ticks(self, ticks: int, timeout: float | None = None) -> list[dict]:
        start = min(int(status["tick"]) for status in self.both("status"))
        deadline = time.monotonic() + (timeout or self.args.phase_timeout)
        while time.monotonic() < deadline:
            statuses = self.both("status")
            if min(int(status["tick"]) for status in statuses) >= start + ticks:
                return statuses
            time.sleep(.1)
        raise ValidationError(f"Simulation did not advance {ticks} ticks from {start}")

    def wait_observation(self, label: str, predicate, timeout: float | None = None) -> list[dict]:
        deadline = time.monotonic() + (timeout or self.args.phase_timeout)
        stable, observations = 0, []
        while time.monotonic() < deadline:
            self.both("status")
            observations = self.both("observe")
            stable = stable + 1 if all(predicate(state) for state in observations) else 0
            if stable >= 3:
                self.record["observations"].append(dict(name=label, peers=observations))
                return observations
            time.sleep(.15)
        raise ValidationError(f"{label}: native state did not meet acceptance criteria: {observations}")

    def boot(self, campaign: int) -> None:
        self.campaign = campaign
        deadline = time.monotonic() + self.args.boot_timeout
        for peer in self.peers:
            while time.monotonic() < deadline:
                require(peer.running(), f"P{peer.seat + 1} exited during boot; inspect {peer.log}")
                try:
                    peer.request("netplay_status")
                    peer.input(0)
                    break
                except (OSError, json.JSONDecodeError):
                    time.sleep(.2)
            else:
                raise ValidationError(f"P{peer.seat + 1}: debug server did not become ready")
        direction = RIGHT if campaign else LEFT
        pulse = 0
        previous = None
        while time.monotonic() < deadline:
            self.both("status")
            states = self.both("observe")
            modes = tuple((state["mode"], state["minor"]) for state in states)
            if modes != previous:
                print(f"native boot modes: {modes}", flush=True)
                self.record["boot_modes"].append(dict(elapsed=self.args.boot_timeout -
                    max(0, deadline - time.monotonic()), modes=modes))
                previous = modes
            if all(state["mode"] == 6 and state["enrolled"] == 1 for state in states):
                self.both("input", 0)
                self.wait_observation("intro-enrolled", lambda state:
                    state["mode"] == 6 and state["minor"] == 0 and state["enrolled"] == 1
                    and state["p1"]["character"] == campaign
                    and state["p2"]["character"] == 1 - campaign
                    and all(state[seat]["active"] and state[seat]["hp"] > 0
                            and state[seat]["state"] == 1 for seat in ("p1", "p2"))
                    and not state["pause_suppressed"] and not state["script_gate"],
                    timeout=max(1, deadline - time.monotonic()))
                self.checkpoint("intro CRC", 0, min_digests=3)
                return
            # Character-select routine 800297D8 uses horizontal edges to choose
            # PLAY+43, then Start/Cross to confirm. Keep that choice in every
            # native navigation pulse; never write campaign or stage RAM.
            self.peers[0].input(direction | (START if pulse % 2 == 0 else CROSS))
            self.wait_ticks(12, timeout=max(1, deadline - time.monotonic()))
            self.peers[0].input(0)
            self.wait_ticks(10, timeout=max(1, deadline - time.monotonic()))
            pulse += 1
        raise ValidationError("Input-only navigation failed to enroll X and Zero in the intro")

    def controls(self) -> None:
        for seat, held, expected in ((0, RIGHT | SQUARE, "right"), (1, LEFT | SQUARE, "left")):
            self.both("input", 0)
            self.wait_ticks(12)
            def ordinary(state):
                return (state["mode"] == 6 and state["minor"] == 0 and state["enrolled"] == 1
                        and state["p1"]["character"] == self.campaign
                        and state["p2"]["character"] == 1 - self.campaign
                        and all(state[body]["active"] and state[body]["hp"] > 0
                                for body in ("p1", "p2")))
            before = self.wait_observation(f"P{seat + 1} movement baseline", ordinary)
            start_tick = max(int(status["tick"]) for status in self.both("status"))
            self.peers[seat].input(held)
            self.wait_ticks(36)
            self.peers[seat].input(0)
            self.wait_ticks(8)
            after = self.wait_observation(f"P{seat + 1} movement result", ordinary)
            body = f"p{seat + 1}"
            deltas = [new[body]["x"] - old[body]["x"] for old, new in zip(before, after)]
            require(all(delta > 1 if expected == "right" else delta < -1 for delta in deltas),
                    f"P{seat + 1} independent {expected} movement not observed: {deltas}")
            require(all(state[body]["active"] and state[body]["hp"] > 0 for state in after),
                    f"P{seat + 1} died before its controls could be qualified")
            self.record["controls"].append(dict(seat=seat, held=hex(held), before=before,
                                                 after=after, delta_x=deltas))
            self.checkpoint(f"P{seat + 1} movement/attack CRC", start_tick)

    def menus(self) -> None:
        for owner in (0, 1):
            self.both("input", 0)
            start_tick = max(int(status["tick"]) for status in self.both("status"))
            self.peers[owner].input(START)
            self.wait_ticks(6)
            self.peers[owner].input(0)
            before = self.wait_observation(f"P{owner + 1} menu opens", lambda state: state["minor"] == 2)
            self.wait_ticks(36)
            after = self.both("observe")
            require(all(new["frames"] == old["frames"] for old, new in zip(before, after)),
                    f"P{owner + 1} menu did not freeze the second gameplay pass")
            counter_name = f"mmx4.coop.p{owner + 1}-menu"
            counters = [peer.request("mod_counters")["counters"] for peer in self.peers]
            require(all(any(row["name"] == counter_name and int(row["count"]) > 0 for row in rows)
                        for rows in counters), f"P{owner + 1} menu ownership was not recorded")
            for peer in self.peers:
                shot = peer.directory / f"menu-p{owner + 1}.png"
                peer.request("screenshot_hires", path=str(shot))
                require(shot.is_file() and shot.stat().st_size > 64,
                        f"P{peer.seat + 1}: menu screenshot was not produced")
            self.peers[owner].input(START)
            self.wait_ticks(6)
            self.peers[owner].input(0)
            self.wait_observation(f"P{owner + 1} menu closes", lambda state:
                                  state["mode"] == 6 and state["minor"] == 0)
            self.checkpoint(f"P{owner + 1} menu CRC", start_tick)

    def disconnect(self) -> None:
        self.both("input", 0)
        self.both("request", "clear_input")
        self.wait_ticks(8)
        stopped_pid = self.peers[1].process.pid
        self.peers[1].stop()
        deadline = time.monotonic() + self.args.disconnect_timeout
        observation = None
        while time.monotonic() < deadline:
            host = self.peers[0]
            text = host.log.read_text(errors="replace")
            noticed = ("other player left or stopped responding" in text or
                       "netplay_peer_disconnect" in text)
            if noticed:
                observation = dict(stopped_pid=stopped_pid,
                                   survivor_returncode=host.process.poll(),
                                   evidence="native peer disconnect exit reason")
                break
            require(host.running(), "Survivor exited without reporting a peer disconnect")
            try:
                observation = host.request("netplay_status")
            except OSError:
                pass
            time.sleep(.2)
        require(not observation or observation.get("evidence") == "native peer disconnect exit reason",
                f"Survivor did not notice private peer disconnect: {observation}")
        require(observation is not None and "evidence" in observation,
                "No native disconnect evidence appeared before the deadline")
        self.record["disconnect"] = observation

    def close(self) -> None:
        for peer in self.peers:
            if peer.running():
                try:
                    peer.request("clear_input")
                except Exception:
                    pass
        try:
            self.pool.shutdown(wait=True)
        finally:
            for peer in self.peers:
                peer.stop()


def run_case(args, directory: Path, campaign: int, jitter: bool, index: int) -> dict:
    case = directory / f"{'jitter' if jitter else 'delay'}-{'zero' if campaign else 'x'}"
    case.mkdir()
    ports, reservations = reserve_ports()
    record = dict(campaign="zero" if campaign else "x", scenario="jitter" if jitter else "delay",
                  status="running", success=False, ports=ports, offline_catalog="empty isolated catalog",
                  session_id=args.session_id + index, boot_modes=[], checkpoints=[], observations=[], controls=[])
    peers, exercise = [], None
    try:
        peers = [prepare_peer(args, case, seat, ports, jitter, args.session_id + index) for seat in (0, 1)]
        record["launch"] = [dict(seat=peer.seat, argv=peer.argv,
                                 environment={key: value for key, value in peer.environment.items()
                                              if key.startswith(("PSX_", "RNET_"))}) for peer in peers]
        for reservation in reservations:
            reservation.close()
        reservations = []
        for peer in peers:
            peer.launch()
        record["pids"] = [peer.process.pid for peer in peers]
        exercise = Exercise(args, peers, record)
        exercise.boot(campaign)
        exercise.controls()
        exercise.menus()
        exercise.disconnect()
        record.update(status="passed", success=True)
    except BaseException as error:
        record.update(status="failed", success=False, error=f"{type(error).__name__}: {error}",
                      traceback=traceback.format_exc())
        record["failure_observations"] = []
        for peer in peers:
            observed = dict(seat=peer.seat, running=peer.running(), log=str(peer.log))
            if peer.running():
                try:
                    observed["netplay_status"] = peer.request("netplay_status", allow_error=True)
                    observed["co_op"] = peer.observe()
                except Exception as diagnostic_error:
                    observed["diagnostic_error"] = str(diagnostic_error)
            if peer.log.exists():
                observed["log_tail"] = peer.log.read_text(errors="replace").splitlines()[-32:]
            record["failure_observations"].append(observed)
    finally:
        for reservation in reservations:
            reservation.close()
        if exercise is not None:
            exercise.close()
        else:
            for peer in peers:
                peer.stop()
        record["returncodes"] = [peer.process.poll() if peer.process else None for peer in peers]
        (case / "report.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--bios", type=Path, required=True)
    parser.add_argument("--disc", type=Path, required=True)
    parser.add_argument("--config", type=Path)
    parser.add_argument("--report-dir", type=Path)
    parser.add_argument("--campaign", choices=("both", "x", "zero"), default="both")
    parser.add_argument("--scenario", choices=("both", "delay", "jitter"), default="both")
    parser.add_argument("--frontend", choices=("headless-opengl", "hidden-window"), default="headless-opengl")
    parser.add_argument("--delay", type=int, default=2)
    parser.add_argument("--latency", type=int, default=25)
    parser.add_argument("--jitter", type=int, default=10)
    parser.add_argument("--seed", type=int, default=1296914484)
    parser.add_argument("--session-id", type=int, default=1296914484)
    parser.add_argument("--boot-timeout", type=float, default=180)
    parser.add_argument("--phase-timeout", type=float, default=45)
    parser.add_argument("--disconnect-timeout", type=float, default=15)
    args = parser.parse_args()
    args.game_root = args.game_root.resolve()
    for name in ("exe", "bios", "disc"):
        value = getattr(args, name).resolve()
        parser.error(f"{name} file does not exist: {value}") if not value.is_file() else None
        setattr(args, name, value)
    args.config = (args.config or args.game_root / "game.toml").resolve()
    require(args.config.is_file(), f"Title config is missing: {args.config}")
    require(0 <= args.delay <= 20 and 0 <= args.jitter <= args.latency <= 2000,
            "Invalid delay/latency/jitter parameters")
    require(all(value > 0 for value in (args.boot_timeout, args.phase_timeout, args.disconnect_timeout)),
            "Timeouts must be positive")
    tag = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + uuid.uuid4().hex[:8]
    directory = (args.report_dir or args.game_root / ".cache/mmx4-coop-netplay" / tag).resolve()
    directory.mkdir(parents=True, exist_ok=False)
    report = dict(schema=1, status="running", success=False, started_utc=datetime.now(timezone.utc).isoformat(),
                  executable=str(args.exe), executable_sha256=sha256_file(args.exe),
                  bios=str(args.bios), bios_sha256=sha256_file(args.bios), disc=str(args.disc),
                  config=str(args.config), config_sha256=sha256_file(args.config), cases=[])
    campaigns = (0, 1) if args.campaign == "both" else (int(args.campaign == "zero"),)
    scenarios = (False, True) if args.scenario == "both" else (args.scenario == "jitter",)
    report_path = directory / "report.json"
    try:
        for jitter in scenarios:
            for campaign in campaigns:
                print(f"Private pair: {'jitter' if jitter else 'delay'}, {'Zero' if campaign else 'X'} campaign", flush=True)
                result = run_case(args, directory, campaign, jitter, len(report["cases"]))
                report["cases"].append(result)
                if not result["success"]:
                    print(result["error"], file=sys.stderr, flush=True)
                    report.update(status="failed", success=False)
                    return 1
        report.update(status="passed", success=True)
        return 0
    except BaseException as error:
        report.update(status="failed", success=False, error=f"{type(error).__name__}: {error}",
                      traceback=traceback.format_exc())
        return 1
    finally:
        report["finished_utc"] = datetime.now(timezone.utc).isoformat()
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Report: {report_path}", flush=True)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValidationError, OSError) as error:
        print(f"Validation failed before launch: {error}", file=sys.stderr)
        sys.exit(1)
