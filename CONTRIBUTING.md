# Contributing to Umbra

This guide is the safe first path for a source checkout. Umbra is a bounded,
standards-first RTI implementation; a passing test does not by itself create a
conformance claim. Start with a small, focused change and preserve the
existing public and compliance boundaries.

## First 30 minutes

Install:

- CMake 3.23 or newer.
- Visual Studio 2022 with the C++ desktop workload and a Windows SDK.
- Python 3.11 or newer for repository tools and Python packages.
- Git. Java is needed only for Java/JNI or Java-TCK work.

For Python work, create an isolated environment before installing a package:

    python -m venv .venv
    .\.venv\Scripts\Activate.ps1
    python -m pip install --upgrade pip

From the repository root, run the baseline route-aware gate:

    python -m tools.ci test --standard all --route cpp

For one standard and one transport, use (for example):

    python -m tools.ci test --standard 2010 --route cpp
    python -m tools.ci test --standard 2025 --route python

List routes and preview commands without changing anything:

    python -m tools.ci list
    python -m tools.ci test --standard 2010 --route cpp --dry-run
    python -m tools.ci doctor

Apply the repository's generated-artifact and available formatter fixes with:

    python -m tools.ci fix
    python -m tools.ci lint

The [CI contract](docs/development/CI.md) shows the route matrix, external Java
artifact inputs, safe cleanup boundary, and the small environment contract for
GitHub Actions, Azure DevOps, and other runners.

The preset uses one build job because parallel MSVC projects can contend for a
program database on Windows. All disposable output is written below out/.

If Visual Studio cannot discover its SDK, open a Developer PowerShell for
Visual Studio and rerun the preset. Do not place build output in cpp/, packages/,
or compliance/.

## Find the right place

Use [the repository guide](docs/development/REPOSITORY-GUIDE.md) first.

- Native C++ behavior belongs under [cpp/](cpp/README.md).
- Private runtime state belongs in one of the domains described by
  [cpp/src/internal/](cpp/src/internal/README.md).
- Python distribution boundaries are explained by the
  [package map](packages/README.md).
- Requirements Lab contracts and evidence inputs live under
  [compliance/](compliance/README.md), never in runtime source.

## Make a safe first change

1. Read the relevant local README and focused design document.
2. Make one behavior or documentation change at a time.
3. Add or update the nearest focused test.
4. Run the smallest affected test, then the baseline native gate.
5. Review the diff for accidental generated files, build output, or unrelated
   reformatting.

For a native behavior change, begin at [cpp/tests/](cpp/tests/README.md). When
a change affects an official requirement or a source path named by a contract,
also follow the traceability guidance in
[requirements and testing](docs/testing/REQUIREMENTS-AND-TESTING.md).

## Choose a build profile

| Goal | Profile | Dependency behavior |
| --- | --- | --- |
| Core build, headers, contracts, and package smoke | native | No dependency download by default. |
| Native Catch2 behavior tests | native-catch2 | Fetches pinned Catch2 if unavailable. |
| XML/XSD and FOM validation | native-fom | Fetches pinned Catch2 and libxml2 if unavailable. |
| Embedded federation-management development profile | native-fom-services | Extends the FOM profile; it is non-installable. |

Run any profile locally or from any CI service that provides Python, CMake, the
Visual Studio C++ toolchain, and a Windows SDK. For example:

    python tools/ci.py native-fom

## Python and Java work

Python package metadata requires Python 3.11 or newer. Start with the package
README and keep applications on the provider-neutral hla-rti-api contract.
Use the bundled or a local Python 3.11+ interpreter; Python 3.10 cannot import
the current API contract.

Java/JNI work is optional. Start with
[the Java TCK guide](docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) or the relevant
package README. Do not add a vendor JAR to the repository unless licensing and
provenance are explicitly approved.

## Requirements Lab boundary

Most first changes do not require the adjacent Requirements Lab. It becomes
relevant when you add an official requirement mapping, change a
source-path-backed contract, or create compliance evidence.

- Treat checked-in contracts under compliance/requirements-lab/ as reviewable
  inputs.
- Keep local exports, raw evidence, and protected material under .compliance/.
- Do not claim verified or conformant status from a local passing test.

Read [the compliance workflow](docs/testing/COMPLIANCE-WORKFLOW.md) before
changing this boundary.

## Style and review

The repository uses [.editorconfig](.editorconfig) for baseline whitespace and
indentation settings: two spaces for C++/CMake and four for Python/PowerShell.
Follow surrounding style, keep includes explicit, and do not perform unrelated
formatting changes. See [style and review](docs/development/STYLE-AND-REVIEW.md)
for the full checklist.

Before requesting review:

- Run the narrowest relevant test and the baseline native gate.
- Update local README or design/requirements links when folder ownership
  changes.
- Keep generated source reproducible through its documented tool.
- Explain any skipped test, optional dependency, or Requirements Lab impact.
