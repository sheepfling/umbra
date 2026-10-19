"""Internal launcher tests, not normative RTI evidence. No downloads required."""

from __future__ import annotations

import io
import json
import os
import socket
import subprocess
import tempfile
import unittest
import zipfile
from contextlib import ExitStack, redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import Mock, patch

from tools import observer_playtest as playtest


class ObserverPlaytestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="umbra playtest ")
        self.addCleanup(self.temporary.cleanup)
        self.layout = playtest.Layout(Path(self.temporary.name))
        self.output = io.StringIO()
        contexts = ExitStack()
        self.addCleanup(contexts.close)
        contexts.enter_context(redirect_stdout(self.output))
        contexts.enter_context(redirect_stderr(self.output))

    def write(self, relative: str, value: str = "placeholder") -> Path:
        path = self.layout.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(value, encoding="utf-8")
        return path

    def bundled_files(self) -> None:
        for name in ("Start-Observer.cmd", "Start-Observer-Limited.cmd", "tools/observer_playtest.py",
                     "packages/umbra-rti-observer/pyproject.toml",
                     "packages/umbra-rti-observer/README.md",
                     "packages/umbra-rti-observer/STORAGE.md",
                     "packages/umbra-rti-observer/RECORDING.md",
                     "packages/umbra-rti-observer/ALERTS.md",
                     "packages/umbra-rti-observer/benchmarks/synthetic.json",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/__init__.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/mom.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/fom.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/capture.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/recording_models.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/recording_routes.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/alerting.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/alert_models.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/alert_routes.py",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/py.typed",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/static/dashboard.html",
                     "packages/umbra-rti-observer/src/umbra_rti_observer/static/observer-wire-v1.schema.json",
                     "packages/umbra-rti-observer-ui/dist/index.html",
                     "packages/umbra-rti-observer-ui/dist/assets/main.js"):
            self.write(name)

    def test_bundle_is_small_scoped_and_does_not_include_local_environments(self) -> None:
        self.bundled_files()
        for name in ("packages/umbra-rti-observer/src/umbra_rti_observer/__pycache__/core.pyc",
                     "packages/umbra-rti-observer/.venv/private.py",
                     "packages/umbra-rti-observer-ui/node_modules/private.js",
                     "src/private.cpp", ".cache/observer-playtest/venv/private.py"):
            self.write(name)
        output = self.layout.root / "output with spaces" / "demo.zip"
        playtest.make_bundle(self.layout, output)
        with zipfile.ZipFile(output) as bundle:
            names = bundle.namelist()
            self.assertTrue(all(name.startswith("umbra-observer-playtest/") for name in names))
            self.assertTrue(any(name.endswith("dist/assets/main.js") for name in names))
            self.assertTrue(any(name.endswith("py.typed") for name in names))
            self.assertTrue(any(name.endswith("observer-wire-v1.schema.json") for name in names))
            for guide_name in ("STORAGE.md", "RECORDING.md", "ALERTS.md", "benchmarks/synthetic.json"):
                self.assertIn(f"umbra-observer-playtest/packages/umbra-rti-observer/{guide_name}", names)
            for module in ("mom.py", "fom.py", "capture.py", "recording_models.py", "recording_routes.py", "alerting.py", "alert_models.py", "alert_routes.py"):
                self.assertTrue(any(name.endswith(f"umbra_rti_observer/{module}") for name in names))
            self.assertFalse(any("private" in name or "__pycache__" in name for name in names))
            guide = bundle.read("umbra-observer-playtest/README.txt").decode()
            for text in ("Extract this ZIP", "MOM reports", "FOM browser", "Recordings", "Alerts"):
                self.assertIn(text, guide)

    def test_prebuilt_bundle_needs_no_node_or_ui_install(self) -> None:
        self.bundled_files()
        with patch.object(playtest.shutil, "which", side_effect=AssertionError("must not look for Node")):
            playtest.prepare_ui(self.layout)

    def test_incomplete_bundle_has_actionable_error(self) -> None:
        with self.assertRaisesRegex(playtest.PlaytestError, "Extract the complete"):
            playtest.prepare_ui(self.layout)

    def test_ui_cache_detects_source_changes_and_missing_assets(self) -> None:
        self.bundled_files()
        source = self.write("packages/umbra-rti-observer-ui/src/App.tsx", "first")
        before = playtest.ui_fingerprint(self.layout)
        source.write_text("second")
        self.assertNotEqual(before, playtest.ui_fingerprint(self.layout))
        before = playtest.ui_fingerprint(self.layout)
        (self.layout.ui / "dist/assets/main.js").unlink()
        self.assertNotEqual(before, playtest.ui_fingerprint(self.layout))

    def test_source_checkout_explains_missing_node_and_points_to_zip(self) -> None:
        self.write("packages/umbra-rti-observer-ui/package.json", "{}")
        with (
            patch.object(playtest.shutil, "which", return_value=None),
            self.assertRaisesRegex(playtest.PlaytestError, "prebuilt playtest ZIP"),
        ):
            playtest.prepare_ui(self.layout)

    def test_failed_setup_never_marks_python_ready(self) -> None:
        self.write("packages/umbra-rti-observer/pyproject.toml")
        with (
            patch.object(playtest, "run_step", side_effect=playtest.PlaytestError("no network")),
            self.assertRaisesRegex(playtest.PlaytestError, "no network"),
        ):
            playtest.prepare_python(self.layout)
        self.assertFalse((self.layout.cache / "python-ready.txt").exists())

    def test_python_install_is_isolated_and_paths_with_spaces_are_one_argument(self) -> None:
        self.write("packages/umbra-rti-observer/pyproject.toml")
        with patch.object(playtest, "run_step") as run:
            python = playtest.prepare_python(self.layout)
        self.assertEqual(python, self.layout.python)
        arguments = run.call_args.args[0]
        self.assertIn(f"{self.layout.observer}[web]", arguments)
        self.assertIn("-e", arguments)
        self.assertTrue((self.layout.cache / "python-ready.txt").is_file())

    def test_python_environment_does_not_inherit_source_overrides(self) -> None:
        with patch.dict(os.environ, {"PYTHONPATH": "unrelated deps", "PYTHONHOME": "other Python"}):
            environment = playtest.clean_environment()
        self.assertNotIn("PYTHONPATH", environment)
        self.assertNotIn("PYTHONHOME", environment)

    def test_interrupted_python_forwarder_allows_child_to_clean_up(self) -> None:
        child = Mock()
        child.wait.side_effect = [KeyboardInterrupt(), 0]
        with patch.object(playtest.subprocess, "Popen", return_value=child):
            self.assertEqual(playtest.forward_to_python(Path("new Python"), ["--check"]), 0)
        child.kill.assert_not_called()
        child.terminate.assert_not_called()
        self.assertEqual(child.wait.call_args.kwargs, {"timeout": 15})

    def test_foreground_wait_is_bounded_so_windows_can_process_ctrl_c(self) -> None:
        child = Mock()
        child.wait.side_effect = [subprocess.TimeoutExpired("server", 0.5), 0]
        self.assertEqual(playtest.wait_for_exit(child), 0)
        self.assertEqual(child.wait.call_count, 2)
        self.assertTrue(all(call.kwargs == {"timeout": 0.5} for call in child.wait.call_args_list))

    def test_partially_created_environment_retries_venv_setup(self) -> None:
        self.write("packages/umbra-rti-observer/pyproject.toml")
        self.layout.python.parent.mkdir(parents=True)
        self.layout.python.touch()
        with (
            patch.object(playtest.subprocess, "run", return_value=Mock(returncode=1)),
            patch.object(playtest, "run_step") as run,
        ):
            playtest.prepare_python(self.layout)
        self.assertIn("venv", run.call_args_list[0].args[0])

    def test_busy_default_port_falls_back_without_touching_existing_listener(self) -> None:
        with socket.socket() as existing:
            existing.bind((playtest.HOST, 0))
            existing.listen()
            occupied = existing.getsockname()[1]
            with patch.object(playtest, "DEFAULT_PORT", occupied):
                port = playtest.choose_port(None)
            self.assertNotEqual(port, occupied)
            self.assertEqual(existing.getsockname()[1], occupied)
            with self.assertRaisesRegex(playtest.PlaytestError, "No existing server was stopped"):
                playtest.choose_port(occupied)

    def test_zero_port_selects_a_free_port(self) -> None:
        self.assertGreater(playtest.choose_port(0), 0)

    def test_health_requires_expected_service_contract_and_integer_event_count(self) -> None:
        valid = {"service": "umbra-rti-observer", "contract_version": playtest.CONTRACT_VERSION,
                 "event_count": 42}
        for changes, expected in (({}, 42), ({"service": "other"}, None),
                                  ({"contract_version": "other"}, None),
                                  ({"event_count": True}, None), ({"event_count": -1}, None)):
            opener = Mock()
            opener.open.return_value = io.BytesIO(json.dumps({**valid, **changes}).encode())
            with patch.object(playtest.urllib.request, "build_opener", return_value=opener):
                self.assertEqual(playtest.live_event_count("http://local"), expected)

    def test_readiness_waits_for_advancing_events_not_just_http(self) -> None:
        child = Mock()
        child.poll.return_value = None
        with (
            patch.object(playtest, "live_event_count", side_effect=[None, 5, 5, 8]) as health,
            patch.object(playtest.time, "sleep"),
        ):
            playtest.wait_until_ready(child, "http://local")
        self.assertEqual(health.call_count, 4)

    def test_early_exit_is_reported_without_waiting_for_timeout(self) -> None:
        child = Mock(returncode=1)
        child.poll.return_value = 1
        with self.assertRaisesRegex(playtest.PlaytestError, "exited early"):
            playtest.wait_until_ready(child, "http://local")

    def test_frozen_producer_never_reports_ready(self) -> None:
        child = Mock()
        child.poll.return_value = None
        with (
            patch.object(playtest, "live_event_count", return_value=5),
            patch.object(playtest.time, "monotonic", side_effect=[0, 1, 2, 4]),
            patch.object(playtest.time, "sleep"),
            self.assertRaisesRegex(playtest.PlaytestError, "live demo events"),
        ):
            playtest.wait_until_ready(child, "http://local", timeout=3)

    def test_shutdown_signals_only_owned_child_and_escalates_if_needed(self) -> None:
        child = Mock()
        child.poll.return_value = None
        child.wait.side_effect = [subprocess.TimeoutExpired("server", 8), None]
        playtest.stop_server(child)
        child.send_signal.assert_called_once()
        child.terminate.assert_called_once()
        child.kill.assert_not_called()

    def test_check_uses_synthetic_controls_then_cleans_up_without_browser(self) -> None:
        self.bundled_files()
        child = Mock()
        with (
            patch.object(playtest, "choose_port", return_value=9999),
            patch.object(playtest.subprocess, "Popen", return_value=child) as spawn,
            patch.object(playtest, "wait_until_ready"),
            patch.object(playtest, "stop_server") as stop,
            patch.object(playtest.webbrowser, "open") as browser,
        ):
            result = playtest.launch(self.layout, Path("python"), profile="limited",
                                     port=None, no_browser=False, check=True)
        self.assertEqual(result, 0)
        arguments = spawn.call_args.args[0]
        self.assertIn("--showcase", arguments)
        self.assertIn("--enable-controls", arguments)
        self.assertIn("limited", arguments)
        self.assertEqual(spawn.call_args.kwargs["env"]["UMBRA_OBSERVER_UI_DIST"],
                         str(self.layout.ui / "dist"))
        browser.assert_not_called()
        stop.assert_called_once_with(child)

    def test_startup_failure_cleans_up_owned_child(self) -> None:
        child = Mock()
        with (
            patch.object(playtest, "choose_port", return_value=9999),
            patch.object(playtest.subprocess, "Popen", return_value=child),
            patch.object(playtest, "wait_until_ready", side_effect=playtest.PlaytestError("bad")),
            patch.object(playtest, "stop_server") as stop,
            self.assertRaisesRegex(playtest.PlaytestError, "bad"),
        ):
            playtest.launch(self.layout, Path("python"), profile="umbra",
                            port=None, no_browser=True, check=False)
        stop.assert_called_once_with(child)

    def test_missing_browser_does_not_stop_healthy_server(self) -> None:
        child = Mock()
        child.wait.return_value = 0
        with (
            patch.object(playtest, "choose_port", return_value=9999),
            patch.object(playtest.subprocess, "Popen", return_value=child),
            patch.object(playtest, "wait_until_ready"),
            patch.object(playtest, "stop_server"),
            patch.object(playtest.webbrowser, "open", side_effect=OSError("no browser")) as browser,
        ):
            result = playtest.launch(self.layout, Path("python"), profile="umbra",
                                    port=None, no_browser=False, check=False)
        self.assertEqual(result, 0)
        browser.assert_called_once_with("http://127.0.0.1:9999/")
        self.assertIn("Open http://127.0.0.1:9999/ in your browser.", self.output.getvalue())

    def test_invalid_port_is_rejected_before_setup(self) -> None:
        with self.assertRaises(SystemExit):
            playtest.parse_options(["--port", "65536"])


if __name__ == "__main__":
    unittest.main()
