# Python development workflow

Run the commands below from the repository root. Use the smallest check that
covers the change, then widen the check before review when the change crosses a
package or provider boundary.

## One-time local setup

Umbra's Python packages require Python 3.11 or newer. Create one local tool
environment and keep using it; the Alphus profile does not build the native
provider or replace package-specific test dependencies.

~~~powershell
# Set this to python when it is Python 3.11 or newer; otherwise use python311.
$basePython = 'python311'
& $basePython -m venv out\alphus-env
$python = '.\out\alphus-env\Scripts\python.exe'
& $python -m pip install -r tools\requirements-alphus.txt
& $python -m pip install -e ..\mdrepo
& $python -m pip install -e 'packages/umbra-rti-observer[dev]'
~~~

The requirements install Alphus, Ruff, Pyright, pytest, the C++ complexity
scanner, and rumdl. Install mdrepo from the adjacent `../mdrepo` checkout into
the same environment for the repository-local Markdown path checks. The
observer development extra supplies the dependencies for the two configured
Pyright projects. Package-specific extras and native/JVM build prerequisites
are listed in the [package map](../../packages/README.md) and the relevant
package README.

## After each Python edit

Run Ruff on the files you changed:

~~~powershell
$files = @(
    'packages/umbra-rti-api/src/hla/rti1516_2025/core.py'
)
& $python -m ruff check $files
& $python -m ruff format --check $files
~~~

Then run the nearest behavior test. Ruff does not run tests or check types.
Use the package's documented runner: the API and JPype suites use unittest;
the observer uses pytest. Examples and source-root setup are in
[Python conformance testing](PYTHON-CONFORMANCE-TESTING.md).

For the API and Java-adapter unit suites, the established source-root setup is:

~~~powershell
$env:PYTHONPATH = 'packages/umbra-rti-api/src;packages/umbra-rti-test-support/src;packages/umbra-rti-jpype/src'
& $python -m unittest discover -s packages/umbra-rti-api/tests
& $python -m unittest discover -s packages/umbra-rti-jpype/tests
~~~

These do not need a vendor JAR. The mock provider's real-JVM tests need JPype1
and a JDK; JNI and external-provider runs have additional opt-in setup described
in the conformance guide.

## Before review

Run the changed-file repository lint and the full test suite for each affected
package. For provider or standard-specific changes, run the corresponding
route-aware test command as well. The lint command discovers Ruff from PATH, so
prepend this environment's Scripts directory for the command:

~~~powershell
$env:Path = "$(Resolve-Path '.\out\alphus-env\Scripts');$env:Path"
& $python -m tools.ci lint --scope changed
& $python -m tools.ci test --standard 2025 --route python
# Use this route for changes in the 2010 Python surface:
& $python -m tools.ci test --standard 2010 --route python
~~~

Observer changes also need the observer's configured type check. Run only the
project that matches the files changed:

~~~powershell
& $python -m pip install -e 'packages/umbra-rti-observer[dev]'
& $python -m pytest packages/umbra-rti-observer/tests
& .\out\alphus-env\Scripts\pyright.exe --project packages\umbra-rti-observer\pyrightconfig.json
& .\out\alphus-env\Scripts\pyright.exe --project packages\umbra-rti-observer-umbra\pyrightconfig.json
~~~

Those are currently the only committed Pyright project configurations. From
the repository root, pass the matching `--project` file as shown; running
`pyright packages/<package>` without it does not load that package's config and
can produce misleading missing-import errors. The full Alphus profile does not
run Pyright, and a repository-wide ad-hoc Pyright command is not a supported
gate. Native-provider tests require the built
extension, and the 2025 Python route does not replace those tests. Follow the
[native provider README](../../packages/umbra-rti-native/README.md) and
[Windows MinGW first-build guide](../development/FIRST-BUILD-WINDOWS-MINGW.md).
After building and installing that provider, run its suite with the source
contracts and shared fixtures on the import path:

~~~powershell
$env:PYTHONPATH = 'packages/umbra-rti-api/src;packages/umbra-rti-test-support/src'
& $python -m unittest discover -s packages/umbra-rti-native/tests
~~~

Run the opt-in JNI suite when changing the Java/JNI/C++ boundary, using the
environment switches in the conformance guide.

## Full-repository review cadence

Run the full local profile after changing lint configuration or Alphus
integration, at the start and completion of a cross-package Python cleanup
batch, and before a release. If cleanup continues longer than a week, capture
one comparison each week. There is no scheduled full-profile job. It scans all
repository Python and C++ files, so there is little value in rerunning it after
every small edit.

~~~powershell
$env:Path = "$(Resolve-Path '.\out\alphus-env\Scripts');$env:Path"
& $python tools\run_alphus_profile.py --root .
~~~

The Alphus report has eight Python/C++ steps, and the full local profile also
runs two Markdown checks. Correctly configured recent full runs took about
97–109 seconds on this Windows checkout, including 24–32 seconds for the C++
Lizard scan. Runs that cannot resolve the Markdown tools are incomplete and
should not be used as a timing baseline. It writes summaries, timing records,
and the full transcript under out/quality/. It does not run tests or Pyright.
Its checks are report-only and are not wired into GitHub CI. A wrapper
exit code of zero means
the report was collected; individual steps can still say FAIL because they
found provisional candidates. Review the summary and changed paths instead of
treating those labels as a clean/unclean verdict.

For useful trend comparisons, keep the Alphus version and configuration hash
the same. Record the run ID and elapsed time from
out/quality/alphus-python-summary.json and
out/quality/alphus-run-timing.json. The detailed install notes and finding
history are in the [Alphus adoption guide](../development/ALPHUS-ADOPTION.md).

## Check cadence at a glance

| When | Run |
| --- | --- |
| Each edit | Ruff check and format check on changed files, plus the nearest behavior test |
| Before review | Changed-file lint, affected package tests, and Pyright for observer changes |
| Provider/API change | The matching 2010 or 2025 Python route in tools.ci |
| Cleanup batch or release | Full Alphus profile; compare its run ID, findings, and timing |

tools.ci fix changes generated files and may apply formatting fixes. Use it
only when you intend to apply those changes; use the lint commands above for
verification. See the [CI contract](../development/CI.md) for route contents
and optional external Java/JNI inputs.
