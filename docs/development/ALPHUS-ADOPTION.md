# Alphus Python and C++ lint on Umbra

Alphus is the entry point for Umbra's local Python and C++ lint profile. It is
a development-only Git dependency, not a runtime dependency of the RTI
packages. Alphus runs Ruff over every repository Python file, its own Python
smell checks, and its C++ source, style, and complexity checks. No CI workflow
is added or changed.
For what to run during an edit, before review, and during periodic cleanup, see
the [Python development workflow](../python/PYTHON-DEVELOPMENT-WORKFLOW.md).

## Install and run

Use Python 3.11 or newer. From the repository root, create a virtual
environment and install the immutable Alphus revision. Its `python` extra
requests Ruff, Pyright, and pytest; the installed environment did not contain
the Pyright executable, so Umbra pins Pyright 1.1.414 directly. Its
`cyclomatic-lizard` extra installs Lizard for the C++ complexity pass. Umbra
also pins Ruff to 0.16.1 so the existing baseline stays reproducible. The
observer's project type check covers its web and conformance-profile helpers,
so install that package's declared `dev` extra as well. If `python --version`
is older than 3.11, use your installed 3.11 interpreter, such as `python311`
from Scoop. These commands call the environment's executables directly, so
PowerShell activation settings do not need to change:

```powershell
$venvPath = 'out\alphus-env'
# If `python --version` is 3.11 or newer:
python -m venv $venvPath
# Otherwise, comment out the command above and use the installed 3.11 command:
# python311 -m venv $venvPath
$python = '.\out\alphus-env\Scripts\python.exe'
& $python -m pip install -r tools\requirements-alphus.txt
# Install the repository-policy checker from the adjacent mdrepo checkout.
& $python -m pip install -e ..\mdrepo
& $python -m pip install -e 'packages/umbra-rti-observer[dev]'
& .\out\alphus-env\Scripts\pyright.exe --project packages\umbra-rti-observer\pyrightconfig.json
& .\out\alphus-env\Scripts\pyright.exe --project packages\umbra-rti-observer-umbra\pyrightconfig.json
```

Preview the selected checks, then run the full lint profile. It can print many
candidate lines. The wrapper saves the full transcript and records per-step
timings and Python summary statistics under `out/quality/`:

```powershell
$env:Path = "$(Resolve-Path '.\out\alphus-env\Scripts');$env:Path"
.\out\alphus-env\Scripts\alphus.exe --root . --config quality-gates.toml --plan --profile lint
$python = '.\out\alphus-env\Scripts\python.exe'
& $python tools\run_alphus_profile.py --root .
$alphusExitCode = $LASTEXITCODE
if ($alphusExitCode -ne 0) { throw "Alphus failed; see out\quality\alphus-lint.txt" }
Write-Host "Alphus finished; review out\quality\alphus-lint.txt"
```

The `lint` profile includes the repository's Markdown files. Rumdl checks
document structure, formatting, local targets, and heading fragments; mdrepo
checks portable repository paths and exact-case targets. The repository keeps
mdrepo's optional reachability graph disabled until its documentation roots
are curated.

The full text report is saved as `out/quality/alphus-lint.txt`. Separate C++
reports are written to `alphus-cpp-source.json`, `alphus-cpp-style.json`, and
`alphus-cpp-complexity.txt` in that same ignored directory. The last profile
step combines their counts into `alphus-cpp-summary.json` and appends each run
to `alphus-cpp-history.jsonl`, so the counts can be compared over time. The
summary includes scanner scope, finding categories, duplicate-code counts,
Lizard complexity distributions, tool and config versions, and the Git
revision/dirty state. These files are local outputs under ignored `out/`.
The wrapper also saves Ruff's complete JSON report, a Python summary/history,
and per-step timing summary/history as `alphus-python-ruff.json`,
`alphus-python-summary.json`, `alphus-python-history.jsonl`,
`alphus-run-timing.json`, and `alphus-run-timing.jsonl`. The C++ summary also
includes the full profile's step timings. Every profile step is provisional,
so this baseline does not fail GitHub CI.

## Initial Umbra baseline — 2026-10-24

Alphus resolved and installed from commit
`b4b6055a6ae0286912b461cd2b4262b3318c5a0c` in a fresh Python 3.11 environment.
The baseline below is from the current working tree, which contains unrelated
uncommitted source and test changes; it is not a clean-main comparison. The
configured eight-step profile completed with exit code 0; its findings remain
provisional.

The profile selected all 249 Python files in the repository, including tests.
Ruff reported 4,944 lint diagnostics; its largest groups were blind exception
catches (`BLE001`, 1,994) and swallowed exceptions (`S110`, 1,976). The format
check found 197 files that Ruff would reformat. Nothing was auto-fixed. This is
a baseline, not a recommendation to reformat the repository wholesale.

Alphus's Python structural rule reported 1,367 category/line hits across 145
of those 249 files (1,147 in production paths and 220 in tests). Most are
`opaque_mappings` and `concrete_inputs`: they identify broad annotations and
concrete container inputs for review, not confirmed defects. The rule's JSON
is a path-to-category-and-line inventory, so the full report needs manual
interpretation.

The Python size pass recorded 42 candidates. Notable candidates include
`packages/umbra-rti-jpype/tests/test_jpype_jni_integration.py` at 83,728 lines,
`tools/query_rti_work.py` at 14,972 lines,
`tools/query_rti_work_regression.py` at 13,159 lines, and
`packages/umbra-rti-native/src/umbra/_native/rti1516e/__init__.py` at 634
lines. These need ownership review before any split; the checker does not know
which files are generated, fixture-heavy, or intentionally API façades.

Alphus's C++ source-ownership check scanned 1,771 owned C/C++ files and returned
23 candidates: 11 errors and 12 warnings, including `.cpp` files included as
implementation fragments and two TCK files over the configured 10,000-line
hard limit. Its separate style
scanner checked 1,771 files and listed 1,212 low-confidence candidates: 848
possible unused includes, 152 regex-based complexity estimates, and 12 large
files. It found 14,993 repeated-code groups but listed only 200 examples.

The profile also runs Lizard 1.24.2 over the C++ tree. At the deliberately high
initial threshold of CCN 50, it returned 113 functions across 91 files. Their
median CCN was 69 and the maximum was 996 in
`packages/hla-rti-cpp-tck/src/main.cpp`. The threshold focuses this first pass
on extreme cases; it is not a claim that lower-complexity functions are clean.
These C++ results are review signals, not confirmed bugs. Each run now records
the full summary and top ten functions in the local JSON outputs above.

## Python lint progress — 2026-10-24

The latest run, `20261024T193254.712477Z`, reports 4,396 Ruff diagnostics,
down 548 from the first 4,944. Production-path diagnostics fell from 560 to
50. The scope split is 50 production, 3,852 tests/fixtures, 488 other files,
and 6 adapters. The largest concentration remains the 83,728-line JPype
integration test at 3,774 findings; its provider and callback boundaries need
an ownership review before any broader cleanup. The leading rules are still
broad/swallowed exception handling (`BLE001` and `S110`), mostly in that test.

Two focused passes reduced product-code findings. A safe Ruff `B009` fix
replaced 325 dynamic attribute lookups with direct attribute access across the
five JPype bridge modules. Then the generated IEEE 1516e contract generator
was corrected so concrete callback stubs no longer emit unused method-name
locals. Its generated module lost 51 production `F841` findings; the same
focused Ruff pass also removed a redundant `pass` and sorted its generated
export list. The generator and generated artifact are both updated. The 2010
Java source archive used to regenerate the artifact is not checked into this
repository, so a full archive-based regeneration comparison remains a follow-up.

### Observer opaque-mapping review

The initial report contained 664 `opaque-mappings` candidates. Across
`umbra-rti-observer` and its Umbra adapter, all 57 candidates were removed:
schema builders and validated HTTP boundaries now use the recursive
`JsonObject`/`JsonValue` contract, post-normalization helpers and participant
projections consume `EventRecord` or JSON-valued mappings, and adapter event
and capability outputs use JSON object types. The latest repository-wide count
is 264; the observer production source and adapter still have zero
opaque-mapping candidates. Observer test helpers retain 37 candidates across
16 files, where the remaining values belong to test fixture boundaries.

The observer's configured Pyright check now covers 50 source files and passes
with zero diagnostics. The Umbra adapter project now includes `adapter.py` as
well as its command module; both files pass strict Pyright with zero
diagnostics. The adapter's dynamic `Any` callback arguments remain intentional
because the provider callback signatures are runtime-specific.

The 24 remaining broad annotations there are intentional dynamic boundaries:
23 raw event/capability inputs can carry runtime-specific provider values
before validation and normalization, and one conditional `connect(**kwargs)`
forwards configuration and credentials to the RTI. Each now has a reason
comment immediately above its declaration; the Alphus profile only permits
those comments in the observer source paths. New unmarked opaque mappings in
those files still show up. The adapter's broad callback catches also keep
optional observer failures from changing RTI callback behavior, so do not
remove them solely to quiet `BLE001`/`S110`.

The bounded JSON pass first updated
`tools/query_rti_work.py::cpp_tck_requirement_gap_inventory`: its nine broad
mapping annotations now use a recursive JSON type, and its internal coverage
group has a concrete shape. Follow-up typing covers JSON-loaded contracts,
issue records, contract links, and C++ source-location and health records.
Alphus now emits no opaque-mapping candidates from the query tool, down from
270. A whole-file Pyright run reports only the analyzer's complexity limit in
`main()`; the typed helper slices introduce no type errors.

The C++ TCK runner now uses explicit `ResolvedInputs`, catalog, scenario, and
direct-run summary records; both `tools/run_cpp_tck.py` and `tools/cpp_tck.py`
have zero opaque-mapping candidates. The JPype integration test now uses a
recursive JSON object type for report decoders, named overload groups for
reflection data, and a named callable registry for temporary provider
overrides; its opaque-mapping count fell from 31 to zero. The report decoders
also verify that decoded values are objects before using them.

The preceding full profile, run 20261024T213158.910796Z, completed all
eight steps in 62.741 seconds with wrapper exit 0. Python structure reported
648 candidates across 132 files, including 141 opaque mappings across 60 files,
down from 664 initially. The decoded surface verifier and the regression scripts
for query work and Requirements-Lab resynchronization had no opaque-mapping
candidates. Ruff reported 4,396 diagnostics: 50 production, 3,853 tests/fixtures,
487 other, and 6 adapters. The format scan marked 190 files, and the Python size
pass reported 45 candidates across 22 files. The query-work regression script
grew from 13,159 to 15,134 physical lines after formatting and remains over the
10,000-line hard cap.

The latest full profile, run 20261024T214207.898565Z, completed all eight
steps with wrapper exit 0 in 60.641 seconds. Ruff reports 4,396 diagnostics
(50 production, 3,853 tests/fixtures, 487 other, and 6 adapters). Alphus reports
649 structural candidates across 133 files, including 142 opaque mappings across
60 files; 190 files would be reformatted and 45 file-size findings remain across
22 files. The native 2025 provider, encoder, and native-call helper have zero
Ruff findings and zero Alphus structure findings. The overall structure total
rose by one since the prior recorded run, outside these three changed modules.

A bounded package review found the native 2025 provider mixed RTI service
implementation with 33 encoder/data-element classes. Those classes occupied
853 source lines and now live in rti1516_2025/encoding.py (979 lines including
imports and documentation). Native exception translation lives in
rti1516_2025/_native_calls.py, which both the provider and encoder use.
provider.py is now 1,960 lines, down from 2,889. The 2010 encoding module
remains separate to preserve the compatibility boundary. Ruff reports no findings in the three changed files. The new
encoding and
native-call modules pass Ruff formatting; provider.py remains one of the
repository-wide formatting candidates. Syntax compilation passes for all three.
A direct Pyright run still
reports 162 diagnostics across these files, including compiled-extension member
and API-signature issues; this remains an unclean, ad-hoc type-check result.
No tests or native build were run.
The C++ profile now uses a 20,000-line soft size threshold, a 30,000-line hard
cap, a 4 MiB expanded-byte cap, a 3,000-line single-consumer-header review
threshold, and a 20,000-line advisory large-file threshold. This follows the
existing Umbra growth guards for the large portable-TCK sources. The latest
source scan has eight findings: six `.cpp`-include ownership errors and two
soft size warnings for those TCK sources; no hard size or single-consumer
header findings remain. The C++ style report has two large-file advisories,
down from twelve.

The surface verifier passes its normal report, focused Ruff and format checks,
syntax compilation, and Pyright with zero diagnostics under the Python 3.11
environment and test-support import path. The two recursive JSON aliases in
the regression scripts remove their 14 opaque-mapping findings, but a focused
Pyright run over those legacy scripts reports 249 nested-access diagnostics;
their decoded records still need incremental narrowing before they can be
called type-clean. Tests were not run. Four query-tool `TRY004` findings
remain explicitly suppressed because they validate malformed serialized
content, where `ValueError` is the appropriate content error. Findings remain
provisional and CI is unchanged.

The remaining 50 production Ruff findings have been reviewed and assigned a
disposition:

| Finding | Count | Disposition |
| --- | ---: | --- |
| `BLE001`, `S110`, `S112` | 19 diagnostics across 11 broad-catch sites | Provider, proxy, callback-isolation, and best-effort-cleanup boundaries intentionally contain runtime-specific exceptions. Narrowing them without provider contracts could leak failures into RTI callbacks; the silent-fallback sites need a separate logging-policy review. |
| `PLE0604` | 5 | Public `__all__` lists include star-imported or generated exception names that Ruff cannot statically inspect. Verify the exported name set before replacing the dynamic construction. |
| `RUF022` | 16 | Keep public export ordering stable until its compatibility and documentation role is reviewed. |
| `RUF023` | 10 | Keep `__slots__` declaration ordering until class layout and serialization compatibility are reviewed. |

The latest full profile took 62.741 seconds, compared with 64.121 seconds in
the previous run. These timings vary with the workstation load; treat the
difference as run-time variation, not a code speedup. Step timings are stored
per run; this sample shows where the time went:

| Step | Seconds |
| --- | ---: |
| Python lint | 3.379 |
| Python format check | 3.416 |
| Python structure scan | 5.921 |
| Python file-size scan | 4.234 |
| C++ ownership and size | 10.053 |
| C++ style | 9.899 |
| C++ Lizard complexity | 24.740 |
| Summary and history write | 0.492 |
| **Total** | **62.741** |

Individual step timers omit a small amount of process startup and report-writing
overhead, so their rows do not sum exactly to the full wall time.

All profile steps remain provisional. The wrapper exits successfully when it
has completed and recorded the run; the per-step `FAIL` labels mean candidates
remain, not that GitHub CI is blocked. The latest Python and timing history
files contain the detailed scope split and run-to-run data.

## Setup friction log

- **Python version:** This Windows setup defaults to Python 3.10; the `py`
  launcher does not list 3.11, but `python311` provides 3.11.9. The install
  guide gives both interpreter choices.
- **Pyright entry point:** Running Pyright on the standalone surface verifier
  without project settings could not resolve `tomllib` or the local
  test-support package. Supplying the Python 3.11 interpreter and the two
  source roots fixed both. The old regression scripts also need typed
  narrowing around decoded JSON before a broad Pyright invocation is useful;
  the first recursive-alias pass exposed 249 diagnostics instead of giving a
  clean whole-script result.
- **Large-file formatting:** Formatting the already oversized query-work
  regression script expanded it from 13,159 to 15,134 physical lines and
  created a large review diff. Split that regression along cohesive check
  boundaries before formatting it again or formatting similarly large files.
- **Tool installation:** The first requirement listed Ruff and Lizard
  separately, which obscured that Alphus supplies them. It now selects
  Alphus's `python` and `cyclomatic-lizard` extras; only Ruff's exact version
  remains directly pinned to keep the existing baseline reproducible. A fresh,
  no-cache install brought in Ruff 0.16.1 and Lizard 1.24.2 through those
  extras. The pinned Git dependency needs Git and network access because
  Alphus has no package index release yet.
- **Running without activation:** Calling `alphus.exe` directly does not add
  the virtual environment's `Scripts` folder to `PATH`, so a plain `ruff`
  command could find a global copy or fail. The profile runs Ruff as
  `{python} -m ruff`; the full profile succeeded from the unactivated
  environment.
- **Global Python mismatch:** The machine's plain `python` is 3.10 and does
  not have Ruff installed. Always invoke the interpreter inside
  `out/alphus-env`; the quickstart now gives that exact command.
- **Profile launcher mismatch:** Running the timing wrapper with system
  Python looked for `alphus.exe` beside Python 3.10 and failed before any
  checks ran. Invoke the wrapper with
  `out/alphus-env/Scripts/python.exe`; the wrapper uses that interpreter's
  adjacent Alphus launcher.
- **Missing type checker (resolved 2026-10-24):** The installed Alphus extra
  advertised Pyright but did not provide its command. Umbra now pins Pyright
  directly, and both observer project configs point at `out/alphus-env` so
  imports resolve from the same interpreter that installed the tools.
- **JSON typing boundary:** A blanket recursive JSON alias caused direct
  Pyright runs to report 40 errors in `tools/java_tck.py` and 224 in
  `quality_gates/bin/cpp_common_quality.py`. Replacing it with typed catalog,
  evidence, and export records plus field checks brought Java TCK to zero
  opaque mappings and zero Pyright diagnostics. The Requirements Lab uses a
  recursive JSON value only inside its normalization boundary, with local
  narrowing; it also passes Pyright with zero opaque mappings. Artifact,
  build-quality, and compile-time reports now use named record types.
- **Single-file checker distribution (resolved 2026-10-24):** The checker was
  over 2,000 lines while the porting guide promised a one-file copy. It is now
  a standard-library-only package with a compatibility script for the current
  CLI and imports. Consumers copy that script with the sibling package folder,
  or copy the full `quality_gates/` tree. The split kept the structure finding
  count at 678 and the package at zero opaque mappings; its six concrete-input
  and six mutable-constant findings were already present before the split.
- **Checker JSON typing (resolved 2026-10-24):** Keeping the recursive JSON
  value type at the decoding boundary and casting into separate policy, CMake
  codemodel, manifest, and verdict records after validation cleared the
  checker's 30 opaque-mapping candidates with zero focused Pyright errors. A
  broad recursive alias used throughout the checker had previously produced
  224 errors; the narrow boundary plus local records kept the validation logic
  readable.
- **Optional observer dependencies:** The full observer type check includes
  its conformance-profile helper, which imports FastAPI and HTTPX. Install the
  observer `dev` extra into the local environment before running its project
  check; the extra is not needed for the Alphus lint profile itself.
- **Interpreter mix-up:** One install initially used the machine's default
  Python 3.10 and placed FastAPI/Uvicorn there. Those two packages were removed
  from that interpreter; subsequent installs used `out/alphus-env` directly.
- **Windows C++ complexity:** Alphus's structured Lizard adapter fails with
  `WinError 206` when it launches with about 1,500 file paths. The profile
  scans the repository root recursively and writes warnings-only output.
- **Report volume:** The Python structure rule emits more than 1,300 candidate
  lines, which buried useful run progress in the terminal. The wrapper now
  shows only step starts/results and the final timing summary, while preserving
  the full Python/C++ transcript and structured reports in ignored
  `out/quality/`.
- **Timing parser:** Alphus sometimes omits an exit-code suffix on a successful
  step. The first timing parser missed those rows; it now accepts either form
  and confirms all eight steps in each complete run.
- **Cache traversal:** The C++ advisory scanner does not exclude `.cache/` by
  default. The profile explicitly excludes local dependency caches.

## Alphus project review

The repository has no open GitHub issues. Its closed tracker is mostly project
work and adoption planning. See [the closeout record](https://github.com/sheepfling/alphus/issues/25)
and [the release and downstream-adoption record](https://github.com/sheepfling/alphus/issues/29).
Alphus's documented pre-registry install is pinned to a Git commit rather than
a package-index release.
The integration did uncover consumer-side concerns and one Windows-specific
Alphus limitation:

- The C++ advisory walk does not exclude `.cache/` by default. The Umbra
  profile adds that and the other local dependency caches explicitly.
- The broad Python rule produces many category/line candidates without
  per-hit explanation text. Keep it report-only until Umbra-specific examples
  establish which patterns are useful and which are intentional API design.
- Duplicate-window reporting can discover thousands of groups but displays
  only a bounded sample, which is too noisy to treat as a defect count.
- Alphus's structured Lizard rule passes nearly 1,500 paths to one Windows
  process and fails with `WinError 206` (command line too long). The profile
  works around this by asking Lizard to scan the repository root recursively,
  writing only functions over its review threshold. This is a useful issue to
  report upstream, but no issue or comment was posted.

The C++ style scanner is intentionally low-confidence. Its role and limits are
documented in [Alphus's C++ coverage guide](https://github.com/sheepfling/alphus/blob/main/docs/cpp/CPP-QUALITY-GATE-COVERAGE.md).

Consumer-observed coverage gaps and dispositions are tracked in the
[Alphus complaint ledger](ALPHUS-COMPLAINT-LEDGER.md). The ledger is a local
review record; entries marked as upstream candidates have not been filed.

GitHub currently reports no license for Alphus. Keep this pin development-only
until Alphus declares a license and distribution policy; do not bundle it into
an Umbra runtime package.

## Next review

Compare future profile runs with their Alphus version and configuration hashes. The latest full local
profile, `20261025T070557.397042Z`, completed in 109.104 seconds. Ruff reported zero findings and
formatting passed. A fresh `python-style --report --json` scan with the same configuration found zero
`opaque_mappings`; its 224 structural candidates across 85 files match the full-profile rule totals.
Remaining categories include 92 concrete inputs, 61 local imports, 26 legacy Pydantic fields, and 34
mutable module constants. The observer source and Umbra adapter remain in the profile.

The full profile still reports Python structure and file-size candidates, C++ source ownership/size
findings, and Lizard candidates. File size reports 48 findings across 25 files; the JNI integration
test remains 80,026 lines against the 10,000-line cap. Step times were 3.863s for Python lint,
3.843s for formatting, 6.312s for structure, 4.034s for file size, 9.149s for C++ ownership/size,
9.641s for C++ style, 25.898s for Lizard, and 0.448s for C++ statistics. C++ style and statistics
passed. Markdown links/graph passed; style reports 25 findings across two files.

The Windows setup still needs the adjacent mdrepo source on `PYTHONPATH` and a temporary launcher
generated through Python's vendored distlib. Both launcher names were removed after the run. The
local Alphus profile remains report-only and is not wired into GitHub CI. No test suites or builds ran.
