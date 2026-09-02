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

`--route all` expands the four routes for the selected standard. `--standard
all --route all` runs the complete matrix sequentially on a local machine;
hosted CI fans those same lanes out as independent jobs.

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
2. Provide Python 3.11+, CMake 3.23+, Visual Studio 2022 C++ build tools, and a
   Windows SDK.
3. Provide Java 11+ for Java/JNI lanes.
4. Install the small CI tools needed by the selected lane, such as `pytest`
   and `ruff`.
5. Invoke `python -m tools.ci` from the repository root.

The checked-in GitHub Actions workflow is an example of this contract, not a
second source of build behavior. An Azure DevOps or other Windows runner can
fan out the same `python -m tools.ci test ...` commands.
