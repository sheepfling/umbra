# Repository guide

Umbra is organized by ownership. Runtime code, public documentation,
compliance inputs, packages, vendored standards, and developer automation have
separate homes so generated evidence and external material do not become
implicit runtime dependencies.

## Start by intent

- Making a first change: read [CONTRIBUTING.md](../../CONTRIBUTING.md).
- Building or changing the native RTI: begin in [cpp/](../../cpp/README.md).
- Selecting or developing a Python provider: begin in the
  [package map](../../packages/README.md).
- Understanding current boundaries and planned work: read
  [architecture](../architecture/ARCHITECTURE.md) and
  [planning](../planning/ROADMAP.md).
- Adding tests or evaluating evidence: read
  [requirements and testing](../testing/REQUIREMENTS-AND-TESTING.md) and the
  [compliance input map](../../compliance/README.md).
- Running a generator or verifier: use [tools/](../../tools/README.md).

## Repository map

| Path | Owns | Entry point |
| --- | --- | --- |
| cpp/ | Native C++ API binding, implementation, generated fallback, and tests | [cpp/README.md](../../cpp/README.md) |
| packages/ | Python distributions plus Java/JNI test and bridge artifacts | [packages/README.md](../../packages/README.md) |
| docs/ | Architecture, plans, focused designs, and testing guidance | [docs/README.md](../README.md) |
| compliance/ | Reviewable Requirements Lab contracts, catalogs, and manifests | [compliance/README.md](../../compliance/README.md) |
| tools/ | Generators, verifiers, sidecar wrappers, and evidence utilities | [tools/README.md](../../tools/README.md) |
| cmake/ | Installed-package configuration and consumer smoke test | [cmake/README.md](../../cmake/README.md) |
| third_party/ | Pinned, immutable IEEE standards inputs | [third_party/README.md](../../third_party/README.md) |

## Native source layout

The native implementation follows the conventional public-header, source, and
test split:

- cpp/include/umbra/ contains Umbra-owned public helper headers.
- cpp/src/ contains concrete standard-binding definitions.
- cpp/src/internal/ separates private handles, FOM support, federation state,
  time coordination, callbacks, observability, and cross-cutting runtime
  helpers. The [private runtime map](../../cpp/src/internal/README.md) explains
  their ownership and dependency rules.
- cpp/generated/ contains checked-in output from a narrow generator. Do not
  hand-edit it.
- cpp/tests/ contains Catch2 and smoke tests; cpp/tests/data/ contains their
  XML fixtures.

The public IEEE header baseline is vendored rather than copied into cpp/include.
It is exposed by the CMake targets described in the root README and the
architecture document.

## Python and Java layout

Python packages use the src layout. Every Python distribution has its own
pyproject file, source directory, tests, and README. The native distribution
also has CMake configuration because it compiles an extension. Java/JNI
artifacts are not Python distributions, so their CMake or PowerShell build
entry points are deliberate exceptions to that pattern.

## Local output convention

Keep disposable work under out/ at the repository root:

- out/cmake/default is the ordinary native build.
- out/cmake/fom is the FOM-validator build.
- out/cmake/fom-services is the opt-in embedded federation-management build.
- out/java-tck contains compiled Java TCK classes, matrix results, and JPype
  handoff evidence.

The out/ tree is ignored and can be removed and regenerated. It is not an
input to source control or compliance review.

The ignored .compliance/ tree is intentionally different: it contains local
Requirements Lab exports, adapter requests, and unreviewed evidence. Keep it
separate from ordinary compiler and test output. Agent-local .codex paths and
other historic local build directories are also disposable; new commands
should use out/.
