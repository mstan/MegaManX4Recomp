"""Drive two isolated, hidden launchers through the rendered netplay lobby.

The JSON plan contains {seat, command} rows using launcher TCP commands.
Each peer owns its settings/cards and only processes created here are stopped.
No desktop input APIs or visible windows are used. Screenshots are retained.
"""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
import json
import os
from pathlib import Path
import shutil
import socket
import time

from test_mmx4_coop_netplay import prepare_peer, reserve_ports, change_config, Exercise


def require_hidden(pid: int) -> None:
    if os.name != "nt":
        return
    user = ctypes.WinDLL("user32")
    callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    visible = []

    @callback
    def visit(handle, _):
        owner = wintypes.DWORD()
        user.GetWindowThreadProcessId(handle, ctypes.byref(owner))
        if owner.value == pid and user.IsWindowVisible(handle):
            visible.append(int(handle))
        return True

    user.EnumWindows(visit, 0)
    assert not visible, ("QA created a visible window", pid, visible)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--config", type=Path, default=Path("game.toml"))
    parser.add_argument("--disc", type=Path, default=Path("mmx4/Mega Man X4.cue"))
    parser.add_argument("--cameras", choices=("unified", "split"), default="unified")
    parser.add_argument("--boot-game", action="store_true",
                        help="Enter native co-op gameplay and compare peer state digests")
    args = parser.parse_args()
    args.exe, args.config, args.disc = (p.resolve() for p in
                                       (args.exe, args.config, args.disc))
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    args.bios = args.exe.parent / "bios/openbios.bin"
    args.game_root = args.config.parent
    args.latency = args.jitter = 0
    args.seed, args.delay, args.frontend = 77, 2, "hidden"
    ports, locks = reserve_ports()
    peers, connections, records = [], [], []
    report = dict(passed=False, desktop_input=False, debug_ports=ports[2:])
    exercise = None

    def command(seat, text):
        peer = peers[seat]
        require_hidden(peer.process.pid)
        connection, reader = connections[seat]
        connection.sendall((text + "\n").encode("utf-8"))
        line = reader.readline(2 * 1024 * 1024)
        assert line, ("Launcher closed", seat, text)
        reply = json.loads(line)
        records.append(dict(seat=seat, command=text, reply=reply))
        assert reply.get("ok"), records[-1]
        return reply

    try:
        for seat in range(2):
            peer = prepare_peer(args, out, seat, ports, False, 77)
            peers.append(peer)
            for name in ("assets", "bios"):
                shutil.copytree(args.exe.parent / name, peer.directory / name)
            # Include all launcher-visible packages and execution identity.
            shutil.copytree(args.exe.parent / "mods", peer.directory / "mods",
                            dirs_exist_ok=True)
            manifest = args.exe.with_suffix(".execution.json")
            shutil.copy2(manifest, peer.directory / manifest.name)
            config = peer.directory / "game.toml"
            source = config.read_text(encoding="utf-8")
            for section, key, value in (("video", "renderer", "opengl"),
                                        ("video", "aspect_ratio", "4:3"),
                                        ("netplay", "content_negotiation", True),
                                        ("runtime", "debug_port", ports[2 + seat])):
                source = change_config(source, section, key, value)
            config.write_text(source, encoding="utf-8", newline="\n")
            (peer.directory / "settings.toml").write_text(
                '[netplay]\nplayer_name = "X4 QA seat ' + str(seat + 1) + '"\n',
                encoding="utf-8", newline="\n")
            peer.environment = {k: v for k, v in peer.environment.items()
                                if not k.startswith("LNG_")}
            peer.environment.update(LNG_TCP_PORT="0", LNG_TEST_HIDDEN="1",
                PSX_NET_LOBBY_TRACE="1",
                LNG_TCP_PORT_FILE=str(peer.directory / "launcher-port.txt"))
            peer.argv = [str(peer.directory / args.exe.name), "--launcher",
                         "--hidden-window", "--game", str(config),
                         "--bios", str(args.bios), "--disc", str(args.disc),
                         "--memcard-dir", str(peer.directory / "saves"),
                         "--debug-port", str(peer.tcp)]
        for lock in locks:
            lock.close()
        for peer in peers:
            peer.launch()
            deadline = time.monotonic() + 30
            portfile = peer.directory / "launcher-port.txt"
            while not portfile.exists():
                assert peer.running(), ("Launcher exited", peer.seat)
                assert time.monotonic() < deadline, "Launcher listener timeout"
                time.sleep(.05)
            connection = socket.create_connection(("127.0.0.1", int(portfile.read_text())), 5)
            connection.settimeout(20)
            connections.append((connection, connection.makefile("rb")))
            state = command(peer.seat, "state")
            assert state["hidden"] and state["window_hidden"] and state["players"] == 2
            command(peer.seat, "size:1120x860")
        plan = json.loads(args.plan.read_text(encoding="utf-8"))
        for index, row in enumerate(plan):
            if "sleep" in row:
                time.sleep(min(float(row["sleep"]), 10))
            elif "runtime" in row:
                deadline = time.monotonic() + 30
                while True:
                    try:
                        value = peers[row["seat"]].request(row["runtime"], **row.get("fields", {}))
                        require_hidden(peers[row["seat"]].process.pid)
                        expected = row.get("expect", {})
                        assert all(value.get(k) == v for k, v in expected.items()), (expected, value)
                        if value.get("tick", 0) >= row.get("tick_at_least", 0):
                            break
                    except OSError:
                        if time.monotonic() >= deadline:
                            raise
                    assert time.monotonic() < deadline, ("Runtime acceptance timeout", row)
                    time.sleep(.1)
                records.append(dict(seat=row["seat"], runtime=row["runtime"], reply=value))
            else:
                seat = row["seat"]
                text = row["command"].replace("{out}", str(out)).replace("{udp}", str(ports[0]))
                command(seat, text)
                if row.get("shot"):
                    command(seat, "wait:3")
                    command(seat, "shot:" + str(out / f"{index:02d}-seat{seat}.png"))
        if args.boot_game:
            args.boot_timeout, args.phase_timeout = 60, 30
            args.delay = peers[0].status().get("input_delay", 4)
            report.update(boot_modes=[], observations=[], checkpoints=[])
            exercise = Exercise(args, peers, report)
            exercise.boot(0)
            exercise.checkpoint("lobby-native-coop", 0)
            for peer in peers:
                require_hidden(peer.process.pid)
                peer.request("screenshot_hires", path=str(out / f"game-seat{peer.seat}.png"))
        report["passed"] = True
    except BaseException as error:
        report["error"] = repr(error)
        raise
    finally:
        if exercise:
            exercise.pool.shutdown(wait=True)
        for connection, reader in connections:
            reader.close()
            connection.close()
        for peer in peers:
            peer.stop()
        for lock in locks:
            lock.close()
        report["records"] = records
        (out / "report.json").write_text(json.dumps(report, indent=2) + "\n",
                                         encoding="utf-8", newline="\n")
        print(json.dumps(dict(passed=report["passed"], error=report.get("error"),
            desktop_input=False, debug_ports=ports[2:],
            checkpoints=[dict(name=c["name"], samples=len(c["matched_crc_samples"]))
                         for c in report.get("checkpoints", [])])))


if __name__ == "__main__":
    main()
