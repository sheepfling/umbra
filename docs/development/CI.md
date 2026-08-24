# CI contract

Umbra keeps pipeline configuration thin. The authoritative native CI commands
live in [tools/ci.py](../../tools/ci.py), so a developer checkout and every CI
provider execute the same configure, build, and test sequence.

## Provider contract

A provider needs only to:

1. Check out the repository.
2. Provide Python 3.11+, CMake 3.23+, Visual Studio 2022 C++ build tools, and
   a Windows SDK.
3. Run the selected profile from the repository root.

    python tools/ci.py native

The checked-in GitHub Actions workflow is an example of this contract, not the
source of build logic. An Azure DevOps or other Windows runner should prepare
the same prerequisites and invoke the same command.

## Profiles and reruns

Use `python tools/ci.py --list-profiles` to discover available profiles.

| Profile | Purpose |
| --- | --- |
| native | Default native build, smoke tests, requirements traceability, and package smoke. |
| native-catch2 | Focused native behavior tests; may fetch Catch2. |
| native-fom | XML/XSD and FOM validation; may fetch Catch2 and libxml2. |
| native-fom-services | Non-installable embedded federation-management development profile. |

The default runs configure, build, then test. A junior developer can repeat a
single completed stage without reproducing pipeline syntax:

    python tools/ci.py native --stage test
    python tools/ci.py native-fom --stage build

Use `--dry-run` to inspect the exact CMake/CTest command sequence. The named
CMake presets remain the low-level configuration authority; `tools/ci.py`
selects their matched configure, build, and test presets.
