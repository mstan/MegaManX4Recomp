"""Exercise two private X4 delay-sync peers, using host input and original assets.

Example (run only after rebuilding the co-op title):
  python tools/test_mmx4_coop_netplay.py --exe build-coop/MegaManX4Recomp.exe \
      --bios psxrecomp-v4/bios/SCPH1001.BIN --disc "mmx4/Mega Man X4.cue"

Each case copies the executable and its declared settings manifest into its own
run directory, disables offline co-op, uses private saves, and binds newly reserved
loopback UDP/TCP ports. The trusted profile must enable co-op itself. Optional
card seeds are cloned and loaded through native Continue. No RAM writes,
save-state loads, live lobby rooms or simulation-layer pad overrides are used.
Only PIDs created here are stopped.
Native state reads are asynchronous observations; matching sim-tick CRCs and
the existing matched-state watermark provide the network agreement evidence.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
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
MENU = 0x801754A0
BODY_BYTES = 0xE4
DIAGNOSTIC_MAGIC = 0x5834434F
START, CROSS, SQUARE, LEFT, RIGHT = 0x0008, 0x4000, 0x8000, 0x0080, 0x0020
UP, DOWN, FRONT = 0x0010, 0x0040, 0x80173C70
CIRCLE = 0x2000
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
    request_timings: dict = field(default_factory=dict)

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
        started = time.monotonic()
        try:
            return self._request(cmd, allow_error=allow_error, **fields)
        finally:
            elapsed_ms = (time.monotonic() - started) * 1000
            row = self.request_timings.setdefault(cmd, dict(count=0, max_ms=0, total_ms=0))
            row['count'] += 1
            row['max_ms'] = max(row['max_ms'], round(elapsed_ms, 3))
            row['total_ms'] += round(elapsed_ms, 3)
            if elapsed_ms >= 750:
                print(f'P{self.seat + 1} debug {cmd}: {elapsed_ms:.0f} ms', flush=True)

    def _request(self, cmd: str, *, allow_error: bool = False, **fields) -> dict:
        require(self.running(), f"P{self.seat + 1} process exited before {cmd}")
        with socket.create_connection(("127.0.0.1", self.tcp), timeout=2) as connection:
            connection.settimeout(3)
            connection.sendall((json.dumps(dict(id=1, cmd=cmd, **fields)) + "\n").encode())
            with connection.makefile("rb") as stream:
                line = stream.readline(2 * 1024 * 1024)
            require(bool(line), f"P{self.seat + 1} closed its debug connection during {cmd}")
            result = json.loads(line)
        require(result.get("id") == 1,
                f"P{self.seat + 1}: unexpected debug response for {cmd}: {result}")
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
        menu = self.read(MENU, 0x34)
        diagnostic = self.read(self.diagnostic, 0x500)
        completed_context = struct.unpack_from("<I", diagnostic, 0x1C)[0] == 1
        # Pair world identity/lives with the completed player bodies. Live
        # mode/menu gates must remain live: diagnostics stop advancing while
        # the native menu freezes gameplay and during non-gameplay loading.
        world = (diagnostic[0x400:0x464] if completed_context and
                 play[0] == 6 and diagnostic[0x400] == 6 else play)
        first = (diagnostic[0x300:0x300 + BODY_BYTES] if completed_context
                 else self.read(PLAYER, BODY_BYTES))
        magic, frames, enrolled, failed, banks = struct.unpack_from("<5I", diagnostic)
        require(magic == DIAGNOSTIC_MAGIC, f"P{self.seat + 1}: invalid co-op diagnostic header")
        require(failed == 0, f"P{self.seat + 1}: co-op plugin reported failure")
        return dict(mode=play[0], minor=play[1], stage=world[0xC], section=world[0xD],
                    campaign=world[0x43], lives=world[0x44],
                    pause_suppressed=play[0x1C], script_gate=play[0x10],
                    frames=frames, enrolled=enrolled, failed=failed, banks=banks,
                    completed_context=completed_context,
                    menu_main=menu[4], menu_sub=menu[5], menu_item=menu[0x14],
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
                action=body[5], grounded=bool(body[0x89] & 8),
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
    (directory / "mods").mkdir()
    settings = args.exe.parent / "mods/bundled/mmx4.coop"
    require(settings.is_dir(), "Built executable is missing its declared co-op settings package")
    shutil.copytree(settings, directory / "mods/bundled/mmx4.coop")
    (directory / "mods/state.toml").write_text(
        'format_version = 2\n\n[[feature]]\npackage_id = "mmx4.coop"\n'
        f'id = "coop"\nenabled = false\n[feature.values]\ncameras = "{args.cameras}"\n'
        f'hud_layout = "{getattr(args, "hud_layout", "stacked")}"\n', encoding="utf-8")
    saves = directory / "saves"
    saves.mkdir()
    card_seed = getattr(args, 'card_seed', None)
    if card_seed is not None:
        for name in ('card1.mcd', 'card2.mcd'):
            source_card = card_seed / name
            require(source_card.is_file(), f'Missing private save seed: {source_card}')
            shutil.copy2(source_card, saves / name)
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
        # Disagree offline preferences deliberately. The trusted session must
        # force 4:3; explicit CLI OpenGL chooses the supported presentation
        # backend while unequal quality settings leave guest VRAM unchanged.
        ("video", "aspect_ratio", "16:9" if seat == 0 else "21:9"),
        ("video", "renderer", "software" if seat == 0 else "vulkan"),
        ("video", "internal_resolution", "1080p" if seat == 0 else "720p"),
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
                       PSX_RENDER_PASS_VERIFY="1",
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
        if getattr(self.args, 'continue_jungle', False):
            self.boot_continue_jungle(campaign, deadline)
            return
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
            if all(state["mode"] == 6 and state["minor"] == 0 and state["enrolled"] == 1
                   for state in states):
                self.both("input", 0)
                self.wait_ticks(max(8, self.args.delay + 4),
                                timeout=max(1, deadline - time.monotonic()))
                settled = self.both("observe")
                if any(state["minor"] == 2 for state in settled):
                    continue
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
            if any(state["mode"] == 6 for state in states):
                # Enrollment can lag the first PLAY=6 observation. A held
                # navigation Start across that boundary opens the native menu.
                self.both("input", 0)
                if all(state["mode"] == 6 for state in states) and any(
                        state["minor"] == 2 for state in states):
                    self.peers[0].input(START)
                    self.wait_ticks(6, timeout=max(1, deadline - time.monotonic()))
                    self.peers[0].input(0)
                self.wait_ticks(12, timeout=max(1, deadline - time.monotonic()))
                continue
            # Character-select routine 800297D8 uses horizontal edges to choose
            # PLAY+43, then Start/Cross to confirm. Keep that choice in every
            # native navigation pulse; never write campaign or stage RAM.
            self.peers[0].input(direction | (START if pulse % 2 == 0 else CROSS))
            self.wait_ticks(12, timeout=max(1, deadline - time.monotonic()))
            self.peers[0].input(0)
            self.wait_ticks(10, timeout=max(1, deadline - time.monotonic()))
            pulse += 1
        raise ValidationError("Input-only navigation failed to enroll X and Zero in the intro")

    def boot_continue_jungle(self, campaign: int, deadline: float) -> None:
        """Use the native front-end thread, card reader and stage selector.

        PLAY is still zero in the main menu. Watching PLAY alone skips Continue
        and accepts Game Start instead; FRONT is the original 8001DAF8 thread.
        The read-only card clones contain Zero in data 1 and normal X in data 3.
        """
        navigation = self.record.setdefault('continue_navigation', [])

        def pulse(buttons: int) -> None:
            self.peers[0].input(buttons)
            self.wait_ticks(12, timeout=max(1, deadline - time.monotonic()))
            self.peers[0].input(0)
            self.wait_ticks(12, timeout=max(1, deadline - time.monotonic()))

        def wait_native(label, predicate, intro=False):
            previous = None
            while time.monotonic() < deadline:
                self.both('status')
                fronts = self.both('read', FRONT, 0x20)
                plays = self.both('read', PLAY, 0x64)
                key = [(f[0], f[1], p[0], p[1]) for f, p in zip(fronts, plays)]
                if key != previous:
                    navigation.append(dict(label=label, modes=key))
                    print(f'{label}: native front/PLAY {key}', flush=True)
                    previous = key
                if all(predicate(f, p) for f, p in zip(fronts, plays)) and all(
                        not fade[0] for fade in self.both('read', 0x80141BDC, 1)):
                    return plays
                if intro and all(f[0] != 6 for f in fronts):
                    pulse(START)
                elif label == 'loaded stage selection' and all(p[0] == 3 and p[1] == 9 for p in plays):
                    # Original story dialogue after loading the all-Mavericks
                    # save needs the campaign controller's confirm edges.
                    pulse(CROSS)
                else:
                    self.wait_ticks(8, timeout=max(1, deadline - time.monotonic()))
            raise ValidationError(f'{label}: native input navigation timed out')

        wait_native('main menu', lambda f, p: f[0] == 6 and f[1] == 1, intro=True)
        self.wait_ticks(24)
        pulse(DOWN)
        require(all(f[2] == 1 for f in self.both('read', FRONT, 0x20)),
                'Continue was not selected in the native main menu')
        pulse(CROSS)
        wait_native('Continue card choice', lambda f, p: f[0] == 7 and f[1] == 3)
        pulse(CROSS)
        wait_native('memory card slot choice', lambda f, p: p[0] == 0 and p[1] == 1)
        pulse(CROSS)
        wait_native('saved data list', lambda f, p: p[0] == 0 and p[1] == 3)
        # The card reader enters this mode before its asynchronous directory
        # read finishes. Let the original reader populate the data list.
        self.wait_ticks(90)
        continue_data = getattr(self.args, 'continue_data', None)
        for unused in range(continue_data - 1 if continue_data else (0 if campaign else 2)):
            pulse(DOWN)
        pulse(CROSS)
        wait_native('saved data confirmation', lambda f, p: p[0] == 0 and p[1] == 4)
        pulse(CROSS)
        wait_native('loaded stage selection', lambda f, p: p[0] == 3 and p[1] == 4)
        expected_rewards = getattr(self.args, 'expected_rewards', 255)
        require(all(p[0x43] == campaign and p[0x59] == expected_rewards for p in self.both('read', PLAY, 0x64)),
                'Continue loaded a different campaign or Maverick rewards')
        for peer in self.peers:
            peer.request('screenshot_hires', path=str(peer.directory / 'continue-stage-select.png'))
        # 8002E994: Up moves the cleared-save centre cursor (8) to top-left (0).
        pulse(UP)
        require(all(p[3] == 0 for p in self.both('read', PLAY, 0x64)),
                'Native stage cursor did not select Jungle')
        pulse(CROSS)
        while time.monotonic() < deadline:
            states = self.both('observe')
            if all(s['mode'] == 6 and s['minor'] == 0 and s['enrolled'] and
                   not s['pause_suppressed'] and not s['script_gate'] and
                   all(s[b]['state'] == 1 and s[b]['hp'] > 0 for b in ('p1', 'p2'))
                   for s in states):
                require(all(s['stage'] == 1 and s['campaign'] == campaign for s in states),
                        'Native Continue did not reach the requested Jungle campaign')
                self.record['observations'].append(dict(name='Continue Jungle enrolled', peers=states))
                self.checkpoint('Continue Jungle CRC', 0, min_digests=3)
                return
            # The original boss introduction/mission transition accepts confirm.
            if all(s['mode'] != 6 for s in states):
                pulse(CROSS)
            else:
                self.wait_ticks(8, timeout=max(1, deadline - time.monotonic()))
        raise ValidationError('Native Jungle did not become playable after Continue')

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
            if expected == 'left' and all(state[f'p{seat + 1}']['x'] <= 16 for state in before):
                # Continue may spawn P2 at Jungle's native left boundary.
                # Exercise the permitted direction instead of expecting a
                # correctly clamped actor to walk outside the level.
                held, expected = RIGHT | SQUARE, 'right'
            if getattr(self.args, 'continue_jungle', False):
                # A fully upgraded Zero can enter a stationary native
                # technique on Sword. Test loaded-stage movement separately.
                held &= ~SQUARE
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
            kind = 'movement' if getattr(self.args, 'continue_jungle', False) else 'movement/attack'
            self.checkpoint(f"P{seat + 1} {kind} CRC", start_tick)

    def profile_policy(self) -> None:
        policies = []
        for peer in self.peers:
            source = tomllib.loads((peer.directory / "game.toml").read_text(encoding="utf-8"))
            log = peer.log.read_text(errors="replace")
            selected = re.findall(r"mod selected fixed display aspect (\d+):(\d+)", log)
            require(selected and selected[-1] == ("4", "3"),
                    f"P{peer.seat + 1}: no trusted fixed 4:3 activation evidence")
            require("dual-raster" in log,
                    f"P{peer.seat + 1}: no CPU-authority OpenGL presentation evidence")
            scales = re.findall(r"GL GPU pipeline ready \(dual-raster, internal scale (\d+)x", log)
            require(scales, f"P{peer.seat + 1}: missing actual GL presentation scale")
            latency = int(peer.environment["RNET_SIM_LATENCY_MS"])
            jitter = int(peer.environment["RNET_SIM_JITTER_MS"])
            if latency:
                banner = f"+{latency}ms recv delay, +/-{jitter}ms jitter"
                held = [int(value) for value in re.findall(r"held=(\d+)", log)]
                require("LINK SIMULATOR ENGAGED" in log and banner in log and held and max(held) > 0,
                        f"P{peer.seat + 1}: requested link simulation has no held-packet evidence")
            policies.append(dict(seat=peer.seat, offline_video_preferences=source["video"],
                                 active_aspect="4:3", renderer="OpenGL dual-raster",
                                 presentation_scale=int(scales[-1]),
                                 link_simulation=dict(latency_ms=latency, jitter_ms=jitter),
                                 evidence="trusted activation and backend startup logs"))
        self.record["profile_policy"] = policies

    def split_presentation_probe(self) -> None:
        self.wait_ticks(12)
        probes = []
        for peer in self.peers:
            counters = {row["name"]: int(row["count"]) for row in peer.request("mod_counters")["counters"]}
            stats = peer.request("render_pass_stats")
            probe = dict(seat=peer.seat, counters=counters, render_pass_stats=stats)
            probes.append(probe)
            self.record["split_presentation_probe"] = probes
            require(counters.get("mmx4.coop.local-view-committed", 0) > 0,
                    f"P{peer.seat + 1}: no committed own view: {stats}")
            require(counters.get("mmx4.coop.local-view-state-leak", 0) == 0,
                    f"P{peer.seat + 1}: own view changed host gameplay state")
            require(stats.get("verify_mismatch", 0) == 0,
                    f"P{peer.seat + 1}: sandbox restoration mismatch: {stats}")
            require(stats.get("verify_checks", 0) > 0 and stats.get("local_views", 0) > 0,
                    f"P{peer.seat + 1}: no verified sandbox draw: {stats}")
            require(all(stats.get(key, 0) == 0 for key in ("watchdog", "vram_leaks", "device_reads")),
                    f"P{peer.seat + 1}: local draw sandbox fault: {stats}")
            path = peer.directory / "split-intro-presented.png"
            probe["screenshot"] = peer.request("screenshot_hires", path=str(path))
        start_tick = max(int(status["tick"]) for status in self.both("status"))
        self.checkpoint("Split own-view CRC", start_tick)

    def split_world_observation(self, label: str) -> dict:
        views = []
        for peer in self.peers:
            counters = {row['name']: int(row['count']) for row in peer.request('mod_counters')['counters']}
            address = counters.get('mmx4.coop.split-state', 0)
            require(address != 0, 'Missing native independent camera state')
            layers = peer.read(address + 16, 2 * 0xFC)
            cameras = [dict(x=struct.unpack_from('<h', layers, seat * 0xFC + 10)[0],
                            y=struct.unpack_from('<h', layers, seat * 0xFC + 14)[0]) for seat in (0, 1)]
            pool = peer.read(0x8013BED0, 48 * 0x9C)
            actors = [dict(slot=index, type=pool[index * 0x9C + 1],
                           state=pool[index * 0x9C + 4], hp=pool[index * 0x9C + 0x5C],
                           x=struct.unpack_from('<h', pool, index * 0x9C + 10)[0],
                           y=struct.unpack_from('<h', pool, index * 0x9C + 14)[0])
                      for index in range(48) if pool[index * 0x9C]]
            image = peer.directory / f'{label}-presented.png'
            capture = peer.request('screenshot_hires', path=str(image))
            stats = peer.request('render_pass_stats')
            require(all(stats.get(key, 0) == 0 for key in
                        ('verify_mismatch', 'watchdog', 'vram_leaks', 'device_reads')) and
                    counters.get('mmx4.coop.local-view-state-leak', 0) == 0,
                    f'P{peer.seat + 1}: own-view sandbox fault during {label}: {stats}')
            views.append(dict(seat=peer.seat, cameras=cameras, enemies=actors,
                              counters=counters, capture=capture, render_pass_stats=stats,
                              players=peer.observe()))
        observation = dict(name=label, peers=views)
        self.record.setdefault('split_world', []).append(observation)
        return observation

    def split_separation_probe(self) -> None:
        self.both('input', 0)
        before = self.split_world_observation('split-before-separation')
        first_x = before['peers'][0]['players']['p1']['x']
        start_tick = max(int(status['tick']) for status in self.both('status'))
        reached = False
        try:
            for pulse in range(30):
                self.peers[1].input(RIGHT | CROSS | CIRCLE)
                self.wait_ticks(12)
                self.peers[1].input(RIGHT)
                self.wait_ticks(10)
                states = self.both('observe')
                self.record.setdefault('separation_controls', []).append(dict(pulse=pulse, peers=states))
                require(all(state['p1']['hp'] and state['p2']['hp'] and
                            state['p1']['state'] < 2 and state['p2']['state'] < 2 for state in states),
                        'A player fell during input-only separation; retain the failure evidence')
                if all(state['p2']['x'] - state['p1']['x'] > 640 for state in states):
                    reached = True
                    break
            require(reached, 'P2 did not reach an independent distant view with native input')
        finally:
            self.both('input', 0)
            self.split_world_observation('split-separated')
        separated = self.record['split_world'][-1]
        require(all(abs(peer['cameras'][1]['x'] - peer['cameras'][0]['x']) > 416
                    for peer in separated['peers']), 'Native activation rectangles still overlap horizontally')
        require(all(abs(peer['players']['p1']['x'] - first_x) < 48 for peer in separated['peers']),
                'Distant P2 pulled the stationary P1')
        remote_actors = []
        for peer in separated['peers']:
            first, second = peer['cameras']
            actors = [actor for actor in peer['enemies'] if
                      second['x'] - 64 < actor['x'] < second['x'] + 384 and
                      not first['x'] - 64 < actor['x'] < first['x'] + 384]
            require(actors, 'No native enemies activated around the distant player')
            gap = [actor for actor in peer['enemies'] if
                   first['x'] + 384 < actor['x'] < second['x'] - 64]
            require(not gap, f'Native intro enemy occupies the empty horizontal view gap: {gap}')
            remote_actors.append(actors)
        self.record['split_interest_evidence'] = dict(remote_only_enemies=remote_actors,
                                                     empty_gap_snapshot=True)
        self.checkpoint('Split separated-world CRC', start_tick)
        start_tick = max(int(status['tick']) for status in self.both('status'))
        returned = False
        try:
            for pulse in range(30):
                self.peers[1].input(LEFT | CROSS | CIRCLE)
                self.wait_ticks(12)
                self.peers[1].input(LEFT)
                self.wait_ticks(10)
                states = self.both('observe')
                require(all(state['p1']['hp'] and state['p2']['hp'] for state in states),
                        'A player died during input-only overlap/rejoin')
                if all(abs(state['p2']['x'] - state['p1']['x']) < 80 for state in states):
                    returned = True
                    break
            require(returned, 'Native controller traversal did not rejoin the views')
        finally:
            self.both('input', 0)
            self.split_world_observation('split-rejoined')
        rejoined = self.record['split_world'][-1]
        for seat, peer in enumerate(rejoined['peers']):
            live = {(actor['slot'], actor['type']): actor for actor in peer['enemies']}
            for previous in remote_actors[seat]:
                actor = live.get((previous['slot'], previous['type']))
                if actor is not None:
                    require(any(camera['x'] - 64 < actor['x'] < camera['x'] + 384
                                for camera in peer['cameras']),
                            'Previously remote native enemy persists outside both returned views')
        self.record['split_interest_evidence']['remote_retained_or_despawned_after_rejoin'] = True
        self.checkpoint('Split overlap-rejoin CRC', start_tick)

    def split_team_retry_probe(self) -> None:
        self.both('input', 0)
        initial = self.both('observe')
        first_victim = int(getattr(self.args, 'split_first_death', 'p2') == 'p2')
        survivor = first_victim ^ 1
        fallback_counter = f'mmx4.coop.local-view-seat-{survivor}'
        previous_fallback = self.peers[first_victim].request('mod_counters')['counters']
        fallback_count = next((row['count'] for row in previous_fallback if
                               row['name'] == fallback_counter), 0)
        lives = initial[0]['lives']
        require(lives > 0 and all(state['lives'] == lives for state in initial),
                'Native shared-life baseline is not eligible for a retry')
        start_tick = max(int(status['tick']) for status in self.both('status'))
        history = []
        self.record['split_death_retry'] = dict(initial=initial, first_victim=first_victim, history=history)
        for victim in (first_victim, survivor):
            finished = False
            previous_x, stuck = None, 0
            try:
                for pulse in range(150):
                    # Walk into native hazards; climb a blocking ledge when
                    # walking stops. Attacking continuously can clear the
                    # very enemies needed to exercise original fatal contact.
                    held = RIGHT | (CROSS if stuck >= 3 else 0)
                    self.peers[victim].input(held)
                    self.wait_ticks(12)
                    self.peers[victim].input(RIGHT)
                    self.wait_ticks(8)
                    states = self.both('observe')
                    x = states[0][f'p{victim + 1}']['x']
                    stuck = stuck + 1 if previous_x is not None and abs(x - previous_x) < 1 else 0
                    previous_x = x
                    history.append(dict(victim=victim, pulse=pulse, held=held, peers=states))
                    if pulse % 10 == 0:
                        print(f'P{victim + 1} native hazard traversal: x={x}, '
                              f"HP={states[0][f'p{victim + 1}']['hp']}", flush=True)
                    if victim == first_victim:
                        require(all(state[f'p{survivor + 1}']['hp'] > 0 for state in states),
                                'Stationary survivor died before the intended first death')
                    if all(state[f'p{victim + 1}']['hp'] == 0 and
                           state[f'p{victim + 1}']['state'] >= 2 for state in states):
                        finished = True
                        break
                    if victim == survivor and all(state['lives'] == ((lives - 1) & 255) for state in states):
                        finished = True
                        break
                require(finished, f'P{victim + 1} did not enter native death/retry with controller traversal')
            finally:
                self.peers[victim].input(0)
            if victim == first_victim:
                fallen_body, living_body = f'p{victim + 1}', f'p{survivor + 1}'
                self.wait_observation(f'Split P{victim + 1} native death completes', lambda state:
                                      state[fallen_body]['hp'] == 0 and state[fallen_body]['state'] == 3 and
                                      state[living_body]['hp'] > 0 and state['lives'] == lives)
                self.wait_ticks(8)
                fallen = self.split_world_observation(f'split-p{victim + 1}-fallen')
                require(all(peer['cameras'][0] == peer['cameras'][1] for peer in fallen['peers']),
                        'Fallen local peer did not adopt the survivor camera')
                require(fallen['peers'][victim]['counters'].get(fallback_counter, 0) > fallback_count,
                        'Fallen local peer did not actually present the surviving seat')
                self.checkpoint('Split survivor-world CRC', start_tick)
                start_tick = max(int(status['tick']) for status in self.both('status'))
        revived = self.wait_observation('Split shared native checkpoint retry', lambda state:
                                       state['mode'] == 6 and state['minor'] == 0 and state['enrolled'] and
                                       state['lives'] == ((lives - 1) & 255) and
                                       state['p1']['hp'] > 0 and state['p1']['state'] == 1 and
                                       state['p2']['hp'] > 0 and state['p2']['state'] == 1)
        require(all(state['campaign'] == self.campaign for state in revived),
                'Team retry changed the shared campaign')
        self.record['split_death_retry']['revived'] = revived
        self.wait_ticks(90)
        self.split_world_observation('split-team-retried')
        self.checkpoint('Split native team-retry CRC', start_tick)

    def menus(self) -> None:
        for owner in (0, 1):
            self.both("input", 0)
            start_tick = max(int(status["tick"]) for status in self.both("status"))
            self.peers[owner].input(START)
            self.wait_ticks(6)
            self.peers[owner].input(0)
            before = self.wait_observation(f"P{owner + 1} menu opens", lambda state:
                                          state["minor"] == 2 and state["menu_main"] == 1)
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
            # Hold until the original interactive menu accepts the close.
            # A sim-tick pulse can expire while nested guest code waits for
            # a device, especially with a jittered transport.
            self.wait_observation(f"P{owner + 1} menu accepts close", lambda state:
                                  state["minor"] != 2 or state["menu_main"] == 2)
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
                  status="running", success=False, ports=ports,
                  offline_catalog=f"declared settings manifest only; offline co-op disabled; {args.cameras}",
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
        exercise.profile_policy()
        exercise.controls()
        record['hud_captures'] = [peer.request('screenshot_hires',
            path=str(peer.directory / 'coop-hud-presented.png')) for peer in peers]
        if args.cameras == "split":
            exercise.split_presentation_probe()
            if args.split_separation:
                exercise.split_separation_probe()
            if args.split_death_retry:
                exercise.split_team_retry_probe()
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
                    observed["render_pass_stats"] = peer.request("render_pass_stats", allow_error=True)
                    observed["mod_counters"] = peer.request("mod_counters", allow_error=True)
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
        record['debug_request_timings'] = [peer.request_timings for peer in peers]
        (case / "report.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return record


def reject_unsupported_renderers(args, directory: Path) -> list[dict]:
    """Require a clear pre-boot rejection, with transport ports held closed."""
    results = []
    for renderer in ("software", "vulkan"):
        case = directory / f"reject-{renderer}"
        case.mkdir()
        ports, reservations = reserve_ports()
        peer = None
        try:
            peer = prepare_peer(args, case, 0, ports, False, args.session_id)
            peer.argv[peer.argv.index("--renderer") + 1] = renderer
            peer.argv[peer.argv.index("--headless-opengl") if "--headless-opengl"
                      in peer.argv else peer.argv.index("--hidden-window")] = "--hidden-window"
            peer.launch()
            try:
                code = peer.process.wait(timeout=15)
            except subprocess.TimeoutExpired as error:
                raise ValidationError(f"Unsupported {renderer} renderer did not fail before boot") from error
            text = peer.log.read_text(errors="replace")
            require(code != 0 and "netplay profile requires OpenGL retained textures" in text,
                    f"Unsupported {renderer} renderer was not clearly rejected: {text[-2000:]}")
            require("delay-sync active" not in text and "game entry" not in text,
                    f"Unsupported {renderer} renderer continued to guest/transport startup")
            results.append(dict(renderer=renderer, pid=peer.process.pid, returncode=code,
                                argv=peer.argv, evidence=text[-2000:]))
        finally:
            if peer is not None:
                peer.stop()
            for reservation in reservations:
                reservation.close()
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--bios", type=Path, required=True)
    parser.add_argument("--disc", type=Path, required=True)
    parser.add_argument("--config", type=Path)
    parser.add_argument("--card-seed", type=Path)
    parser.add_argument("--continue-jungle", action="store_true")
    parser.add_argument("--continue-data", type=int, choices=range(1, 4))
    parser.add_argument("--expected-rewards", type=lambda value: int(value, 0), default=255)
    parser.add_argument("--report-dir", type=Path)
    parser.add_argument("--campaign", choices=("both", "x", "zero"), default="both")
    parser.add_argument("--scenario", choices=("both", "delay", "jitter"), default="both")
    parser.add_argument("--frontend", choices=("headless-opengl", "hidden-window"), default="headless-opengl")
    parser.add_argument("--cameras", choices=("unified", "split"), default="unified")
    parser.add_argument("--split-separation", action="store_true")
    parser.add_argument("--split-death-retry", action="store_true")
    parser.add_argument("--split-first-death", choices=('p1', 'p2'), default='p2')
    parser.add_argument("--delay", type=int, default=2)
    parser.add_argument("--latency", type=int, default=25)
    parser.add_argument("--jitter", type=int, default=10)
    parser.add_argument("--seed", type=int, default=1296914484)
    parser.add_argument("--session-id", type=int, default=1296914484)
    parser.add_argument("--boot-timeout", type=float, default=180)
    parser.add_argument("--phase-timeout", type=float, default=45)
    parser.add_argument("--disconnect-timeout", type=float, default=15)
    args = parser.parse_args()
    require(not args.continue_jungle or args.card_seed is not None,
            '--continue-jungle requires private --card-seed clones')
    require(not args.split_separation or args.cameras == 'split', '--split-separation requires Split')
    require(not args.split_death_retry or args.cameras == 'split', '--split-death-retry requires Split')
    args.game_root = args.game_root.resolve()
    if args.card_seed is not None:
        args.card_seed = args.card_seed.resolve()
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
                  validation_scope="Native input, menu ownership/freeze, state CRC agreement and disconnect; "
                                   "captured screenshots require separate visual review",
                  executable=str(args.exe), executable_sha256=sha256_file(args.exe),
                  bios=str(args.bios), bios_sha256=sha256_file(args.bios), disc=str(args.disc),
                  config=str(args.config), config_sha256=sha256_file(args.config), cases=[])
    campaigns = (0, 1) if args.campaign == "both" else (int(args.campaign == "zero"),)
    scenarios = (False, True) if args.scenario == "both" else (args.scenario == "jitter",)
    report_path = directory / "report.json"
    try:
        report["renderer_rejections"] = reject_unsupported_renderers(args, directory)
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
