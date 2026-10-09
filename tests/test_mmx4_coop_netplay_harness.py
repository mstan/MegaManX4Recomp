import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from test_mmx4_coop_netplay import Exercise, ValidationError


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
