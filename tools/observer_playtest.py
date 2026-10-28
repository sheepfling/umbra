"""Bootstrap and launch a local synthetic observer; also make a shareable ZIP.

Only the standard library is needed by this launcher. The observer's optional
web dependencies live in a private environment, never in the user's Python.
The same launcher works in a checkout and in the small prebuilt-UI bundle.
"""

# pyright: strict
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import signal
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
import webbrowser
import zipfile
from collections import deque
from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from pathlib import Path
from typing import cast

PNPM_VERSION = "11.25.0"
HOST = "127.0.0.1"
DEFAULT_PORT = 8765
CONTRACT_VERSION = "umbra-observer-contract-v1"
BUNDLE_README = """UMBRA OBSERVER - PLAYTEST

1. Extract this ZIP to a normal folder (do not run inside the ZIP).
2. Install Python 3.11 or newer if needed: https://www.python.org/downloads/
3. On Windows, double-click Start-Observer.cmd.
   On macOS/Linux, run: python3 tools/observer_playtest.py

First launch downloads Python web dependencies into this folder's private
.cache directory. Internet access is needed for that first setup. Later
launches reuse it. No Node, compiler, HLA installation or real RTI is needed.
Keep the terminal window open; press Ctrl+C in it to stop the playtest.
If Windows asks "Terminate batch job (Y/N)?", type Y and press Enter.
Closing the browser alone does not stop the server. Reopening the printed
URL returns to the dashboard. All demonstration data is in memory and resets
when you stop and restart.

The browser opens automatically once the dashboard is ready. If it does not,
copy the URL printed in the terminal. A busy default port is handled for you.
Only this computer can access the server; it never connects to a real RTI.

TRY THIS
- Open Demo walkthrough for links and exercises for all 19 workspaces.
- Federation: watch the charts and activity update automatically.
- Federates: select ShowcaseServer to inspect its activity.
- Objects: search ExampleClass, compare perspectives, and chart the load attribute.
- Declarations: compare explicit publication/subscription snapshots, not inferred routes.
- Topology: switch three layouts across cycles, parallel links, a self-loop and disconnected group; inspect edges and focus joins.
- Interaction flow: compare paired/uncorrelated observations and observer arrival intervals.
- Ownership: compare explicit owner reports with acquisition/divestiture tickets.
- Regions / DDM: inspect exact large bounds, scoped associations and routing evidence.
- MOM reports: inspect reported members, service errors and numeric/opaque counters.
- FOM browser: navigate explicit classes, fields, datatypes and model revisions.
- Recordings: start/stop/download a bounded capture, then replay and seek across screens.
- Alerts: define observer thresholds, review bounded evidence and download configuration without captured payloads.
- Saved workspaces (header): name/download your view choices, then import and explicitly apply them without captured observations or provider commands.
- Logical time: inspect persistent grants, pending requests and observed grant delays.
- Synchronization: follow paced registration/achievement and rejection reports; select an operation for scope/participant cards.
- Save / restore: compare local completion with collective success/failure over paced cycles.
- Events and Inspectors: explore the generated observations.
- Actions: select the synthetic session, connect, join as ManagedObserver,
  register/achieve a synchronization label, request save, report begun/result,
  then restore that exact saved label and report its result. Wait for explicit
  collective reports between phases. Finally confirm resign and disconnect.
  These controls affect only that dummy
  session; the showcase traffic continues independently.
- Pause view, filter events and export a snapshot.
- Diagnostics: inspect the supported and unavailable features.

Stop the rich demo, then double-click Start-Observer-Limited.cmd to try an
RTI-shaped profile without MOM, ownership, regions or coordination controls.
Lifecycle, timing, attributes and other supported views remain usable.
This is a synthetic observer demonstration, not a complete RTI simulation or
an HLA conformance claim.

Optional terminal commands (from the extracted folder):
  python tools/observer_playtest.py --profile limited
  python tools/observer_playtest.py --no-browser --port 8770
  python tools/observer_playtest.py --check

If startup fails, leave the terminal open and send its error message and the
server log path printed there. A managed proxy/firewall may need permission
for Python package downloads. Nothing changes your global Python installation.
"""


class PlaytestError(RuntimeError):
    """An actionable startup failure, displayed without a Python traceback."""


@dataclass(frozen=True)
class Layout:
    root: Path

    @property
    def observer(self) -> Path:
        return self.root / "packages" / "umbra-rti-observer"

    @property
    def ui(self) -> Path:
        return self.root / "packages" / "umbra-rti-observer-ui"

    @property
    def cache(self) -> Path:
        return self.root / ".cache" / "observer-playtest"

    @property
    def python(self) -> Path:
        environment = self.cache / "venv"
        return environment / ("Scripts/python.exe" if os.name == "nt" else "bin/python")


def compatible_python() -> Path | None:
    """Find a newer installed Python when an old PATH entry shadows it.

    Windows' Python launcher does not always see Scoop installations. Restrict
    discovery to named commands and the two normal per-user install locations,
    rather than searching drives or relying on a developer-specific runtime.
    """
    candidates: list[Path] = []
    for name in (
        "python3",
        "python3.14",
        "python3.13",
        "python3.12",
        "python3.11",
        "python",
    ):
        executable = shutil.which(name)
        if executable:
            candidates.append(Path(executable))
    if os.name == "nt":
        local = os.environ.get("LOCALAPPDATA")
        if local:
            candidates.extend(
                sorted(
                    (Path(local) / "Programs" / "Python").glob("Python3*/python.exe"),
                    reverse=True,
                )
            )
        scoop = Path(os.environ.get("SCOOP", str(Path.home() / "scoop")))
        candidates.extend(
            sorted((scoop / "apps").glob("python*/current/python.exe"), reverse=True)
        )
    for candidate in dict.fromkeys(candidates):
        if candidate.resolve() == Path(sys.executable).resolve():
            continue
        try:
            result = subprocess.run(
                [
                    str(candidate),
                    "-c",
                    "import sys; sys.exit(sys.version_info < (3, 11))",
                ],
                env=clean_environment(),
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
                timeout=5,
            )
            if result.returncode == 0:
                return candidate
        except (OSError, subprocess.TimeoutExpired):
            continue
    return None


def clean_environment() -> dict[str, str]:
    # An inherited PYTHONPATH must not make a half-installed venv look healthy.
    environment = dict(os.environ)
    environment.pop("PYTHONPATH", None)
    environment.pop("PYTHONHOME", None)
    return environment


def forward_to_python(interpreter: Path, argv: Sequence[str]) -> int:
    child = subprocess.Popen(
        [str(interpreter), str(Path(__file__).resolve()), *argv],
        env=clean_environment(),
    )
    try:
        return wait_for_exit(child)
    except KeyboardInterrupt:
        # Both processes share the console and receive Ctrl+C. Do not use
        # subprocess.call here: it kills the child on interruption before the
        # newer launcher has had time to shut its server down.
        try:
            return child.wait(timeout=15)
        except subprocess.TimeoutExpired as error:
            raise PlaytestError(
                "The newer Python launcher is still shutting down; check its window."
            ) from error


def wait_for_exit(child: subprocess.Popen[bytes]) -> int:
    # An infinite Windows process wait can defer Python's Ctrl+C handler until
    # the server exits -- but this launcher is responsible for making it exit.
    # Bounded waits let the main thread process the console interrupt promptly.
    while True:
        try:
            return child.wait(timeout=0.5)
        except subprocess.TimeoutExpired:
            continue


def run_step(arguments: Sequence[str], *, cwd: Path, label: str) -> None:
    print(label, flush=True)
    try:
        subprocess.run(list(arguments), cwd=cwd, env=clean_environment(), check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        raise PlaytestError(
            f"{label} failed. Check the output above; first setup needs internet "
            "access for package downloads. Correct the problem and run the launcher again."
        ) from error


def digest_files(base: Path, paths: Sequence[Path]) -> str:
    digest = hashlib.sha256()
    for path in sorted(paths):
        digest.update(path.relative_to(base).as_posix().encode())
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def ui_fingerprint(layout: Layout) -> str:
    sources = [path for path in layout.ui.iterdir() if path.is_file()]
    for directory in ("src", "public"):
        sources.extend(
            path for path in (layout.ui / directory).rglob("*") if path.is_file()
        )
    outputs = [path for path in (layout.ui / "dist").rglob("*") if path.is_file()]
    return digest_files(layout.ui, sources) + "\n" + digest_files(layout.ui, outputs)


def prepare_ui(layout: Layout) -> None:
    index = layout.ui / "dist" / "index.html"
    if not (layout.ui / "package.json").is_file():
        if not index.is_file():
            raise PlaytestError(
                "Dashboard files are missing. Extract the complete playtest ZIP again."
            )
        print("Using the bundled dashboard (no Node setup needed).", flush=True)
        return

    stamp = layout.cache / "ui-ready.txt"
    if (
        index.is_file()
        and stamp.is_file()
        and stamp.read_text() == ui_fingerprint(layout)
    ):
        print("Dashboard is already built.", flush=True)
        return
    node = shutil.which("node")
    if node is None:
        raise PlaytestError(
            "A source checkout needs Node 22.13+ to build the dashboard. "
            "Install Node, or use the prebuilt playtest ZIP, which needs only Python."
        )
    try:
        version = subprocess.check_output([node, "--version"], text=True).strip()
        major, minor, *_ = (int(part) for part in version.lstrip("v").split("."))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise PlaytestError(
            "Cannot read the Node version. Check your Node installation."
        ) from error
    if not ((major == 22 and minor >= 13) or major > 22):
        raise PlaytestError(
            f"Node {version} is too old for the build tools; use Node 22.13+."
        )

    pnpm = shutil.which("pnpm")
    if pnpm:
        package_manager = [pnpm]
    else:
        npm = shutil.which("npm.cmd" if os.name == "nt" else "npm")
        if npm is None:
            raise PlaytestError(
                "Node is present, but npm is missing. Reinstall Node with npm included."
            )
        package_manager = [
            npm,
            "exec",
            "--yes",
            f"--package=pnpm@{PNPM_VERSION}",
            "--",
            "pnpm",
        ]
    run_step(
        [*package_manager, "install", "--frozen-lockfile"],
        cwd=layout.ui,
        label="Preparing dashboard dependencies (first launch may take a minute)...",
    )
    run_step(
        [*package_manager, "build"], cwd=layout.ui, label="Building the dashboard..."
    )
    if not index.is_file():
        raise PlaytestError("The dashboard build did not produce dist/index.html.")
    layout.cache.mkdir(parents=True, exist_ok=True)
    stamp.write_text(ui_fingerprint(layout))


def prepare_python(layout: Layout) -> Path:
    stamp = layout.cache / "python-ready.txt"
    project = layout.observer / "pyproject.toml"
    if not project.is_file():
        raise PlaytestError(
            "Observer sources are missing. Extract the complete playtest ZIP again."
        )
    identity = hashlib.sha256(
        project.read_bytes()
        + str(layout.root.resolve()).encode()
        + sys.version.encode()
    ).hexdigest()
    layout.cache.mkdir(parents=True, exist_ok=True)
    has_pip = (
        layout.python.is_file()
        and subprocess.run(
            [str(layout.python), "-c", "import pip"],
            env=clean_environment(),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        ).returncode
        == 0
    )
    if not has_pip:
        run_step(
            [sys.executable, "-m", "venv", str(layout.cache / "venv")],
            cwd=layout.root,
            label="Creating a private Python environment...",
        )
    if stamp.is_file() and stamp.read_text() == identity:
        result = subprocess.run(
            [str(layout.python), "-c", "import umbra_rti_observer.server, uvicorn"],
            env=clean_environment(),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if result.returncode == 0:
            print("Python environment is ready.", flush=True)
            return layout.python
    run_step(
        [
            str(layout.python),
            "-m",
            "pip",
            "install",
            "--disable-pip-version-check",
            "-e",
            f"{layout.observer}[web]",
        ],
        cwd=layout.root,
        label="Installing Python web dependencies (first launch only)...",
    )
    stamp.write_text(identity)
    return layout.python


def choose_port(requested: int | None) -> int:
    candidate = DEFAULT_PORT if requested is None else requested
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        try:
            listener.bind((HOST, candidate))
        except OSError as error:
            if requested is not None:
                raise PlaytestError(
                    f"Port {requested} is busy or unavailable. Choose another --port, "
                    "or omit --port to select a free one. No existing server was stopped."
                ) from error
            listener.bind((HOST, 0))
        return int(listener.getsockname()[1])


def live_event_count(url: str) -> int | None:
    # Ignore HTTP proxy settings for our loopback-only readiness check.
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    try:
        with opener.open(url + "/api/health", timeout=1) as response:
            payload: object = json.load(response)
        if not isinstance(payload, dict):
            return None
        health = cast(Mapping[str, object], payload)
        count = health.get("event_count")
        if (
            health.get("service") == "umbra-rti-observer"
            and health.get("contract_version") == CONTRACT_VERSION
            and isinstance(count, int)
            and not isinstance(count, bool)
            and count >= 0
        ):
            return count
    except (OSError, ValueError, urllib.error.URLError):
        pass
    return None


def wait_until_ready(
    child: subprocess.Popen[bytes], url: str, *, timeout: float = 30
) -> None:
    deadline = time.monotonic() + timeout
    first_count: int | None = None
    while time.monotonic() < deadline:
        if child.poll() is not None:
            raise PlaytestError(
                f"The playtest server exited early (exit code {child.returncode})."
            )
        count = live_event_count(url)
        if count is not None:
            if first_count is not None and count > first_count and child.poll() is None:
                return
            first_count = count if first_count is None else first_count
        time.sleep(0.2)
    raise PlaytestError(
        "The server did not produce live demo events within 30 seconds."
    )


def stop_server(child: subprocess.Popen[bytes]) -> None:
    if child.poll() is not None:
        return
    try:
        child.send_signal(signal.CTRL_BREAK_EVENT if os.name == "nt" else signal.SIGINT)
        child.wait(timeout=8)
    except (OSError, subprocess.TimeoutExpired):
        if child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=3)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()


def launch(
    layout: Layout,
    python: Path,
    *,
    profile: str,
    port: int | None,
    no_browser: bool,
    check: bool,
) -> int:
    selected_port = choose_port(port)
    url = f"http://{HOST}:{selected_port}"
    environment = clean_environment()
    environment["UMBRA_OBSERVER_UI_DIST"] = str(layout.ui / "dist")
    log_path = layout.cache / f"server-{selected_port}.log"
    layout.cache.mkdir(parents=True, exist_ok=True)
    print(
        f"Starting the {profile} synthetic profile. Server log: {log_path}", flush=True
    )
    with log_path.open("wb") as log:
        child = subprocess.Popen(
            [
                str(python),
                "-u",
                "-m",
                "umbra_rti_observer",
                "--showcase",
                "--enable-controls",
                "--profile",
                profile,
                "--interval",
                "0.75",
                "--host",
                HOST,
                "--port",
                str(selected_port),
            ],
            cwd=layout.root,
            env=environment,
            stdout=log,
            stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0,
        )
        try:
            wait_until_ready(child, url)
            print(
                f"\nDashboard ready: {url}/\n"
                "Live dummy events and synthetic Actions are enabled. No real RTI is connected.\n"
                "Try Topology, Interaction flow, Synchronization and Save / restore alongside the federation and federate views.\n"
                "Keep this window open. Press Ctrl+C here to stop; closing the browser does not stop it.\n",
                flush=True,
            )
            if check:
                print(
                    "Playtest check passed: the server is healthy and live events are advancing."
                )
                return 0
            if not no_browser:
                try:
                    opened = webbrowser.open(url + "/")
                except (webbrowser.Error, OSError):
                    opened = False
                if not opened:
                    print(f"Open {url}/ in your browser.", flush=True)
            code = wait_for_exit(child)
            if code != 0:
                raise PlaytestError(
                    f"The server stopped unexpectedly (exit code {code})."
                )
            return code
        except PlaytestError:
            log.flush()
            with log_path.open(encoding="utf-8", errors="replace") as diagnostic:
                print("".join(deque(diagnostic, maxlen=25)), file=sys.stderr)
            raise
        finally:
            stop_server(child)


def make_bundle(layout: Layout, output: Path) -> None:
    output = output.resolve()
    files = [
        layout.root / name
        for name in (
            "Start-Observer.cmd",
            "Start-Observer-Limited.cmd",
            "tools/observer_playtest.py",
        )
    ]
    files.append(layout.observer / "pyproject.toml")
    # The packaged README links these guides; keep the extracted ZIP usable
    # without a checkout or a connection back to the repository.
    files.extend(path for path in layout.observer.glob("*.md") if path.is_file())
    files.extend(
        path
        for path in (layout.observer / "benchmarks").glob("*.json")
        if path.is_file()
    )
    files.extend(
        path
        for path in (layout.observer / "src").rglob("*")
        if path.is_file()
        and (
            path.suffix in {".py", ".html"}
            or path.name == "py.typed"
            or path.name.endswith(".schema.json")
        )
    )
    files.extend(path for path in (layout.ui / "dist").rglob("*") if path.is_file())
    if output in (path.resolve() for path in files):
        raise PlaytestError("Choose a ZIP path outside the source and dashboard files.")
    if not (layout.ui / "dist" / "index.html").is_file():
        raise PlaytestError("Build the dashboard before making a playtest bundle.")
    output.parent.mkdir(parents=True, exist_ok=True)
    prefix = "umbra-observer-playtest/"
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(prefix + "README.txt", BUNDLE_README)
        for path in sorted(files):
            archive.write(path, prefix + path.relative_to(layout.root).as_posix())
    print(
        f"\nReady to share: {output}\nExtract it and double-click Start-Observer.cmd. "
        "Only Python 3.11+ and first-run internet access are needed.",
        flush=True,
    )


def parse_options(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--profile",
        choices=("umbra", "limited"),
        default="umbra",
        help="rich demo (default) or an RTI-shaped demo without MOM/ownership/regions",
    )
    parser.add_argument(
        "--port", type=int, help="use this port (default: 8765, or a free port if busy)"
    )
    parser.add_argument(
        "--no-browser", action="store_true", help="print the URL without opening it"
    )
    parser.add_argument(
        "--check", action="store_true", help="set up, verify live events, then stop"
    )
    parser.add_argument(
        "--bundle",
        nargs="?",
        const="dist/umbra-observer-playtest.zip",
        metavar="ZIP",
        help="build a shareable ZIP with the React dashboard; no Python setup needed",
    )
    options = parser.parse_args(argv)
    if options.port is not None and not 0 <= options.port <= 65535:
        parser.error("--port must be between 0 (choose a free port) and 65535")
    return options


def main(argv: Sequence[str] | None = None) -> int:
    options = parse_options(argv)
    layout = Layout(Path(__file__).resolve().parents[1])
    try:
        if sys.version_info < (3, 11):
            interpreter = compatible_python()
            if interpreter is not None:
                print(f"Using the installed newer Python: {interpreter}", flush=True)
                return forward_to_python(
                    interpreter, list(argv) if argv is not None else sys.argv[1:]
                )
            raise PlaytestError(
                "Python 3.11 or newer is needed. Install a current Python and try again."
            )
        print(
            "Umbra Observer playtest - local synthetic data only. No native RTI required.",
            flush=True,
        )
        prepare_ui(layout)
        if options.bundle:
            make_bundle(layout, layout.root / options.bundle)
            return 0
        python = prepare_python(layout)
        return launch(
            layout,
            python,
            profile=options.profile,
            port=options.port,
            no_browser=options.no_browser,
            check=options.check,
        )
    except KeyboardInterrupt:
        print("\nPlaytest stopped. Relaunch any time for a fresh demo.", flush=True)
        return 0
    except (PlaytestError, OSError) as error:
        print(f"\nCould not start the playtest: {error}", file=sys.stderr, flush=True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
