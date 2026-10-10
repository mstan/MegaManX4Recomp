import sys
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from test_mmx4_coop_netplay import Exercise, Peer, PLAY, MENU, DIAGNOSTIC_MAGIC, ValidationError


class NativeObservationTest(unittest.TestCase):
    def test_completed_player_and_stage_snapshot_stays_coherent_during_load(self):
        peer = Peer(0, Path('.'), 1, 2, [], {})
        peer.diagnostic = 0x81000000
        diagnostic = bytearray(0x500)
        struct.pack_into('<5I', diagnostic, 0, DIAGNOSTIC_MAGIC, 30, 1, 0, 8)
        struct.pack_into('<I', diagnostic, 0x1c, 1)
        diagnostic[0x400] = 6
        diagnostic[0x40c] = 1
        diagnostic[0x443] = 1
        # A separate PLAY read can retain the selector's identity while the
        # stage initializes. Native diagnostics pair identity with bodies;
        # menu mode must stay live when that completed world is paused.
        previous_play = bytearray(0x64)
        previous_play[0] = 6
        previous_play[1] = 2
        previous_play[12] = 8
        peer.read = Mock(side_effect=lambda address, size: {
            PLAY: bytes(previous_play), MENU: bytes(0x34),
            peer.diagnostic: bytes(diagnostic)}[address])
        observed = peer.observe()
        self.assertEqual((observed['mode'], observed['stage'], observed['campaign']), (6, 1, 1))
        self.assertEqual(observed['minor'], 2)
        self.assertEqual(observed['frames'], 30)
        previous_play[0] = 3
        observed = peer.observe()
        self.assertEqual((observed['mode'], observed['stage'], observed['campaign']), (3, 8, 0))


class DisconnectEvidenceTest(unittest.TestCase):
    def test_waiting_survivor_needs_no_emulation_thread_debug_response(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / 'runtime.log'
            log.write_text('')
            host = SimpleNamespace(log=log, running=Mock(return_value=True),
                                   request=Mock(side_effect=AssertionError('debug thread blocked')),
                                   process=SimpleNamespace(poll=lambda: None))
            guest = SimpleNamespace(process=SimpleNamespace(pid=42), stop=Mock())
            exercise = Exercise(SimpleNamespace(disconnect_timeout=5), [host, guest], {})
            exercise.both = Mock()
            exercise.wait_ticks = Mock()
            try:
                with patch('test_mmx4_coop_netplay.time.sleep', side_effect=lambda unused:
                           log.write_text('The match ended: the other player left or stopped responding.')):
                    exercise.disconnect()
                self.assertEqual(exercise.record['disconnect']['stopped_pid'], 42)
                self.assertIn('evidence', exercise.record['disconnect'])
                host.request.assert_not_called()
                guest.stop.assert_called_once()
            finally:
                exercise.pool.shutdown(wait=True)

    def test_no_disconnect_reason_still_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / 'runtime.log'
            log.write_text('')
            host = SimpleNamespace(log=log, running=Mock(return_value=False))
            guest = SimpleNamespace(process=SimpleNamespace(pid=42), stop=Mock())
            exercise = Exercise(SimpleNamespace(disconnect_timeout=5), [host, guest], {})
            exercise.both = Mock()
            exercise.wait_ticks = Mock()
            try:
                with self.assertRaisesRegex(ValidationError, 'without reporting a peer disconnect'):
                    exercise.disconnect()
            finally:
                exercise.pool.shutdown(wait=True)


if __name__ == '__main__':
    unittest.main()
