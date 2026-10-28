# CI contract

Umbra keeps hosted pipeline syntax thin. The authoritative CI entry point is
the Python-first [tools/ci.py](../../tools/ci.py) module.
GitHub Actions and another CI service should invoke the same route command
rather than growing separate build logic. The route orchestrator calls Python,
CMake, CTest, Java, `javac`, and `jar` directly; it does not require
PowerShell.

## Quick routes

Run these from the repository root with Python 3.11 or newer:

```text
python -m tools.ci list
python -m tools.ci test --standard 2010 --route cpp
python -m tools.ci test --standard 2025 --route cpp
python -m tools.ci test --standard 2025 --route java
python -m tools.ci test --standard 2025 --route jni
python -m tools.ci test --standard 2010 --route python
python -m tools.ci test --standard all --route all
```

The route matrix is:

| Standard | `cpp` | `java` | `jni` | `python` |
| --- | --- | --- | --- | --- |
| 2010 | C++ headers, encoder/time shell, exact integer/float time marshal round-trips, reference federation/declaration/object/interaction/ownership/synchronization proof slices, and focused CTest checks | 2010 Java TCK | 2010 null JNI build and Java smoke, including the exact Java/JNI/C++ carrier matrix | 2010 surface contracts, factory bindings, adapter unit checks, optional native-boundary tests, surface audit, catalog parity, and all capability-profile declarations |
| 2025 | C++ headers, binding shell, and lifecycle smoke | Java mock-fixture smoke by default; full TCK with caller-supplied API/provider JARs | JNI build and Java smoke against the checked-in mock fixture | 2025 contract and adapter unit checks |

Hosted GitHub Actions runs tooling and the C++ 2025 smoke route on pull
requests and pushes to `master`. It avoids a second push run for feature
branches that already have a pull request. Java, JNI, and Python 2025 routes
are available through manual dispatch: check `run_bindings` in the Run
workflow dialog to run all three. They are skipped by default so routine
changes do not wait on or pay for every binding lane.

The tooling command uses `lint --scope ci --skip-2010-checks`, which omits the
2010-only generated C++ shell check and dedicated 2010 surface, catalog, and
profile audits from hosted CI. Shared integrity checks—including the
two-edition Python surface report and provider-boundary verification—remain
active to guard edition separation. The `ci` lint scope also skips the
repository-wide source-size survey; run `python tools/check_source_file_sizes.py`
locally when reviewing source growth. The explicit 2010 route commands above
remain available for local use and are not merged into the 2025 routes.

`--route all` expands the four routes for the selected standard. `--standard
all --route all` runs the complete matrix sequentially on a local machine. The
hosted workflow keeps the core routes automatic; the 2010 routes remain
available as explicit local runs.

## Java API and provider inputs

The official Java API, provider, and provider-dependency archives are
caller-supplied and are never committed. `--provider-jar` may be repeated for
intentional multiple providers; `--dependency-jar` may be repeated for their
runtime libraries. The same inputs are accepted by the 2010 and 2025 Java
routes. The 2010 Java and 2010 JNI routes therefore require one of:

```text
python -m tools.ci test --standard 2010 --route java \
  --api-jar C:\vendor\ieee-1516.1-2010-java-api.jar `
  --provider-jar C:\vendor\vendor-rti.jar `
  --dependency-jar C:\vendor\vendor-support.jar `
  --factory-name 'Vendor RTI'

$env:UMBRA_2010_API_JAR = 'C:\vendor\ieee-1516.1-2010-java-api.jar'
python -m tools.ci test --standard 2010 --route jni
```

If no provider JAR is supplied, the Java route builds the checked-in mock
fixture. The 2025 JNI route uses its checked-in mock fixture by default. The
2025 Java route does the same Java-only fixture smoke; pass both `--api-jar`
and `--provider-jar` to compile and run the full portable Java TCK against an
external provider. Pass `--fom-path`, `--mim-path`, or
`--capability-profile` when exercising the corresponding external route.

## 2010 marshal baseline

The transport boundary is a gate before behavioral claims. The C++ route runs
`umbra.ieee1516e_2010.time_marshal`, which checks exact bytes, decoded values,
boundary values, and malformed encodings for both standard logical-time
families. The JNI route runs `NativeTypeRoundTripTest`, and the opt-in Python
handoff uses the same bridge artifacts:

```text
python -m tools.ci test --standard 2010 --route cpp
python -m tools.ci test --standard 2010 --route jni --api-jar C:\vendor\ieee-1516.1-2010-java-api.jar

$env:UMBRA_ENABLE_JNI_2010_TYPE_ROUNDTRIP_TESTS = '1'
$env:UMBRA_JNI_2010_BRIDGE_ARTIFACT_DIRECTORY = (Resolve-Path .\out\jni-2010)
python -m unittest packages/umbra-rti-jpype/tests/test_jpype_2010_jni_type_roundtrip.py -v
```

When the optional `_native_2010` extension is built, the Python route also
executes `packages/umbra-rti-native/tests/test_native_2010.py`; without that
artifact the provider-specific tests remain an explicit skip rather than a
false pass.

The route also appends the opt-in Python bridge suites when their documented
switches are set: `UMBRA_ENABLE_JNI_2010_TYPE_ROUNDTRIP_TESTS`,
`UMBRA_ENABLE_JPYPE_2010_MOCK_INTEGRATION`,
`UMBRA_ENABLE_JNI_INTEGRATION_TESTS`, and
`UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX`. This keeps the default route
source-checkout-safe while making an explicitly requested JVM/JNI run part of
the same route result.

## Hygiene, fixes, and cleanup

The CI tool also owns repeatable repository hygiene:

```text
python -m tools.ci lint                 # integrity + CI-tool formatting + diff whitespace
python -m tools.ci lint --scope changed # lint changed Python files too
python -m tools.ci fix                  # regenerate checked-in artifacts and fix CI tooling
python -m tools.ci fix --scope changed  # also apply Ruff fixes to changed Python files
python -m tools.ci clean --dry-run      # preview safe disposable-output cleanup
python -m tools.ci clean --target ci    # remove only out/ci
python -m tools.ci doctor               # inspect runner tools
```

`fix` regenerates the two checked-in C++ fallback shells and the binding
inventory before applying Ruff fixes/formatting. `clang-format` is used when
installed; it is optional until the repository adopts a committed C++
formatting configuration. `clean` only removes known children of `out/`; it
does not invoke `git clean` or touch source, compliance, or vendor trees.
Use `--strict` on `lint` when missing optional formatters should fail the lane.

## Routine MinGW C++ test builds and disk retention

Use the shared-library preset for broad Catch2 and installed-package C++ TCK
runs. It includes the embedded FOM-services profile needed by those suites.
The RTI and its debug information live in shared libraries once;
Catch2 still registers individual cases with CTest from grouped runners.
One-case Catch2 executables are available with `--target <name>` but are
excluded from a default all-target build.
The test preset exports private DLL symbols needed by white-box Catch2 cases;
the normal production shared-library option leaves those exports disabled.

```text
cmake --preset mingw-shared
cmake --build --preset mingw-shared-debug --parallel 4
ctest --preset mingw-shared-debug --output-on-failure --parallel 4 --timeout 120
```

For the installed-package C++ TCK, install this same shared build into a
disposable `out/` prefix, configure the current-package adapter against that
prefix, and build its one reusable TCK runner:

```powershell
$sharedPrefix = Join-Path (Get-Location) 'out/mgw/shared-package'
cmake --install out/mgw/shared --prefix $sharedPrefix
cmake -S packages/hla-rti-cpp-tck/adapters/current-package -B out/mgw/shared-tck -G Ninja "-DCMAKE_PREFIX_PATH=$sharedPrefix"
cmake --build out/mgw/shared-tck --target hla_rti_cpp_tck
ctest --test-dir out/mgw/shared-tck --output-on-failure
```

The adapter registers scenarios separately in CTest while linking the portable
TCK once; its executable receives the same 128 MiB/16 MiB budget.

## Parallel builds and language checks

Use four jobs for the routine MinGW build and CTest run. The Windows junior
guide uses the same setting. On a machine that runs short on memory, lower
`--parallel 4` to `--parallel 2`; do not run two builds in the same build
directory at the same time. A first JNI build has its own CMake tree and must
compile the RTI core for the bridge, so it takes longer than an incremental
build of an already configured tree.

The 2010 and 2025 Python routes can run side by side when they use separate
output roots. This shortens wall time without making either test run share
generated files:

```powershell
python -m tools.ci test --standard 2010 --route python --build-root out/ci/python-2010
python -m tools.ci test --standard 2025 --route python --build-root out/ci/python-2025
```

Run each command in its own terminal. For a complete local sequence, finish
the shared C++ build once, run CTest with `--parallel 4`, then run the Java/JNI
and Python routes. The Java mock-fixture route verifies the wiring; the full
Java TCK additionally requires caller-supplied official API and provider JARs.

All routine MinGW test executables, including the current-package portable
C++ TCK, link without embedded DWARF. The post-link gate fails above 128 MiB
total image or 16 MiB of `.debug_*` sections. For source-level investigation
of one statically linked case, use `mingw-focused-full-debug-static` and build
only the named target; that preset deliberately bypasses the routine image
budget and excludes Catch2 executables from `all`.

Generated `out/mgw/cpp-tck-named-*` build directories are disposable after
their test run. The local retention policy is 24 hours after the last write;
then preview and prune them with:

```powershell
./tools/prune_named_test_builds.ps1
./tools/prune_named_test_builds.ps1 -Apply
```

The script only targets direct named-test children of `out/mgw`, refuses
reparse points and active builds, and is a dry run without `-Apply`. Save any
desired JSON results or diagnostics outside a named build directory before
pruning. The generated binaries are rebuildable; pruning does not touch
source, compliance, or vendor directories.

Standalone CMake/Ninja scenario builds under `out/mgw` use the same 24-hour
retention window. Preview and clean stale generated outputs with:

```powershell
./tools/clean_stale_mingw_build_outputs.ps1
./tools/clean_stale_mingw_build_outputs.ps1 -Apply
```

This keeps the reusable `out/mgw/shared` build and skips trees configured from
other projects. Age is based on the newest file inside each tree, not just the
top-level directory timestamp. It runs Ninja's `clean`, removes only compiled
artifact extensions left orphaned in eligible CMake trees, and prunes stale
Umbra install/provider prefixes plus standalone TCK executables over 500 MiB.
It leaves CMake caches, dependency sources, test sources, and JSON evidence in
place. Export any desired diagnostics before cleanup; dry-run is the default.
For worktree closeout, archive Codex-managed trees through the app and preserve
detached commits before removing manually created trees; see
[worktree hygiene](../../quality_gates/docs/WORKTREE-HYGIENE.md). If the app blocks an archive because
the owning task or workspace is pinned, leave that checkout in place and record
the blocker; do not bypass it by deleting the managed directory from a shell.

## Compile-time observations

The MinGW/Ninja presets export `compile_commands.json`; Ninja records command
durations in `.ninja_log` and transitive compiler dependencies in its deps log.
The report helper combines these existing records to show per-translation-unit
compile time and source lines, project versus external include fanout, the
highest-fanout project headers, and link/archive actions. Use it after a build
to compare a focused source split:

```text
python tools/report_compile_times.py out/mgw/catch2 --top 25 --write-json out/compile-time/catch2-before.json
# Make the scoped change and rebuild the same target/preset.
python tools/report_compile_times.py out/mgw/catch2 --top 25 --baseline out/compile-time/catch2-before.json --write-json out/compile-time/catch2-after.json
```

The report uses each object's latest Ninja timing and full transitive include
set; it does not rerun preprocessing, and its include counts are not direct
`#include` counts. Summed compiler time is CPU time, not elapsed build wall
time. Compare runs made with the same compiler, build type, target, and machine;
use the source-size ratchet separately as the review gate. Timing is
intentionally observational, not a noisy CI threshold.

For a persistent local history that also records source-size and Requirements
Lab traceability health, capture and query through the quality dashboard:

```text
python tools/build_quality.py capture out/mgw/catch2 --label mingw-catch2
python tools/build_quality.py history --limit 10
python tools/build_quality.py history --label mingw-embedded --limit 10
python tools/build_quality.py history --status failed --limit 10
python tools/build_quality.py inspect --id latest --top 20
python tools/build_quality.py inspect --id latest --source federation_registry
python tools/build_quality.py inspect --id latest --header runtime_instrumentation
python tools/build_quality.py inspect --id latest --target umbra_rti
```

Annotate each capture with the result of the build/test run it represents. Failed
and partial builds are useful history too, and the status/note make that
distinction searchable in the same log:

```text
python tools/build_quality.py capture out/mgw/catch2 --label mingw-catch2 --status passed
python tools/build_quality.py capture out/mgw/catch2 --label mingw-catch2 --status failed --note "umbra_rti declaration mismatch"
```

Snapshots append to `out/build-metrics/history.jsonl` by default. This log is
local, queryable, and outside the directories removed by the safe CI cleanup
targets. Each snapshot also runs and records the roadmap/test mapping gate
separately from summary health counts, so stale indexes and unmapped lane tags
remain visible. Pass `--history <path>` to keep or query a different history
file.

## Legacy native profiles

The previous native-only interface remains supported for existing scripts:

```text
python tools/ci.py native
python tools/ci.py native --stage test
python tools/ci.py native-fom --dry-run
python tools/ci.py --list-profiles
```

Those profiles select the named CMake configure/build/test presets. The newer
route commands select only the standard and transport lane while still using
the same CMake presets and package source layout.

## Provider contract

A hosted provider needs only to:

1. Check out the repository.
2. Provide Python 3.11+, CMake 3.23+, Scoop MinGW GCC (`gcc`), and Ninja. On a
   Scoop setup, install the compiler and generator with `scoop install gcc ninja`.
   The native C++ presets use this MinGW toolchain rather than Visual Studio.
3. Provide Java 11+ for Java/JNI lanes.
4. Install the small CI tools needed by the selected lane, such as `pytest`
   and `ruff`.
5. Invoke `python -m tools.ci` from the repository root.

On a deeply nested Windows worktree, MinGW can compile a long-named test
translation unit but fail to open its generated `.obj.d` dependency file with
`No such file or directory`. This is a path-length failure, not a C++ source
diagnostic. If a build has already been configured, map the checkout to a
verified unused drive letter with `subst`, resume Ninja through that short
alias, and remove the mapping after validation. Keep the original build tree;
do not delete source or mask genuine compiler errors.

The checked-in GitHub Actions workflow is an example of this contract, not a
second source of build behavior. An Azure DevOps or other Windows runner can
fan out the same `python -m tools.ci test ...` commands.
