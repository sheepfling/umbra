# Developer tools

Run these utilities from the repository root. They keep generation, standards
verification, test-lane checks, and compliance-sidecar work out of the runtime
libraries.

For command arguments, use:

    python tools/<tool-name>.py --help

## Common recipes

### Check a native change

Use the repeatable baseline first. CI services delegate to this exact command;
it can also rerun one named stage after an initial pass:

    python tools/ci.py native
    python tools/ci.py native --stage test
    python tools/ci.py --list-profiles

For a focused Catch2 domain test, configure the Catch2 profile once, then
filter by its test name:

    python tools/ci.py native-catch2 --stage configure
    python tools/ci.py native-catch2 --stage build
    ctest --test-dir out/cmake/catch2 -C Debug -R federation_registry --output-on-failure

### Inspect or run one tool

Do this before using a tool that can write files:

    python tools/tool_name.py --help

The command help is the authority for arguments. Send ordinary reports and
plots to out/; do not write generated output into a source directory.

### Regenerate a checked-in source artifact

The private fallback ambassador is the only generated source artifact in the
native tree:

    python tools/generate_rti_ambassador_shell.py --help

Run it only when the pinned IEEE binding inventory changes, then review the
generated diff and run the native baseline.

### Work with Requirements Lab inputs

The Lab is optional for ordinary source work. When a requirement mapping or
source-path contract changes, start with:

    python tools/requirements_lab.py --help
    python tools/requirements_lab_sidecar.py --help

Keep exports and unreviewed evidence under .compliance/. Read the
[compliance workflow](../docs/testing/COMPLIANCE-WORKFLOW.md) before promoting
or describing any evidence.

### Validate a Java TCK result

The Java runner produces provider output; this tool joins that output to the
portable catalog and Requirements Lab contracts:

    python tools/java_tck.py validate
    python tools/java_tck.py export --help

See the [Java TCK guide](../docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) for
compile, run, matrix, and JPype-handoff commands.

## Generators and inventories

| Tool | Purpose |
| --- | --- |
| generate_binding_inventory.py | Builds the reviewable abstract-member inventory from the pinned C++ headers. |
| generate_cpp_conformance_worklist.py | Produces a non-evidentiary C++ worklist from a Requirements Lab bundle. |
| generate_rti_ambassador_shell.py | Regenerates the checked-in private fallback ambassador shell. |
| ieee_1516_2_resources.py | Imports or verifies the pinned IEEE 1516.2 resource set and digests. |

## Compliance and provider evidence

| Tool | Purpose |
| --- | --- |
| requirements_lab.py | Exports and validates Umbra-owned baselines against the adjacent Requirements Lab. |
| requirements_lab_sidecar.py | Prepares, records, and verifies sidecar evidence without importing Lab code at runtime. |
| java_tck.py | Validates and exports portable Java TCK traceability and provider evidence. |
| verify_external_2025_fom_corpus.py | Verifies a configured external 2025-native FOM snapshot against its manifest. |
| verify_external_siso_fom_corpus.py | Verifies a configured external SISO FOM snapshot against its manifest. |

## Native test checks

| Tool | Purpose |
| --- | --- |
| verify_catch2_test_tags.py | Ensures native Catch2 tests belong to focused development lanes. |
| verify_ctest_service_lanes.py | Checks that configured CTest service lanes remain complete. |
| verify_ieee_exception_binding.py | Confirms every official exception has a binding definition. |
| verify_ieee_headers.py | Confirms the vendored IEEE 1516.1 header file set and hashes. |

## Reporting

plot_runtime_instrumentation.py turns a runtime instrumentation CSV into a
local image. Send its output to out/ rather than a source directory.

## Write boundaries

Most verifiers read inputs and report success or failure. The generator and
Requirements Lab tools can write outputs. Generated source remains under
cpp/generated/, ordinary temporary output belongs under out/, and local
Requirements Lab requests or raw evidence belong under .compliance/. Review a
generator diff before committing it.
