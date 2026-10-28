"""Named policy, JSON, CMake, manifest, and report records."""

from __future__ import annotations

import re
from collections.abc import Mapping, Sequence
from typing import TypedDict

FRAGMENT_SUFFIXES = {".inc", ".inl", ".ipp", ".tpp"}
IMPLEMENTATION_SUFFIXES = {".c", ".cc", ".cpp", ".cxx"}
HEADER_SUFFIXES = {".h", ".hh", ".hpp"}
SOURCE_SUFFIXES = FRAGMENT_SUFFIXES | IMPLEMENTATION_SUFFIXES | HEADER_SUFFIXES
OWNER_SUFFIXES = SOURCE_SUFFIXES - FRAGMENT_SUFFIXES
INCLUDE_RE = re.compile(
    r'^\s*#\s*include\s*[<"]([^">]+\.(?:inc|inl|ipp|tpp|c|cc|cpp|cxx|h|hh|hpp))[>"]'
)
DEFAULT_SOURCE_INCLUDE = [f"*{suffix}" for suffix in sorted(SOURCE_SUFFIXES)]
DEFAULT_SOURCE_EXCLUDE = [".git/*", "out/*", "build/*", "vendor/*", "third_party/*"]

JsonValue = (
    None | bool | int | float | str | Sequence["JsonValue"] | Mapping[str, "JsonValue"]
)
JsonObject = dict[str, JsonValue]


class _GateReportBase(TypedDict):
    schema: int
    gate: str
    run_id: str | None
    passed: bool
    findings: list[dict[str, str]]
    metrics: dict[str, JsonValue]


class GateReport(_GateReportBase, total=False):
    manifest_sha256: str


class LegacySourceException(TypedDict):
    reason: str
    max_physical_lines: int


class LegacySourceLimit(LegacySourceException, total=False):
    max_expanded_lines: int
    max_expanded_bytes: int


class FragmentSourceException(TypedDict):
    reason: str
    max_lines: int


class SourcePolicy(TypedDict, total=False):
    include: list[str]
    exclude: list[str]
    include_roots: list[str]
    min_files: int
    soft_lines: int
    hard_lines: int
    hard_bytes: int
    single_consumer_header_lines: int
    legacy: dict[str, LegacySourceLimit]
    fragment_exceptions: dict[str, FragmentSourceException]


class GraphPolicy(TypedDict, total=False):
    configuration: str
    include_targets: list[str]
    exclude_targets: list[str]
    max_evidence_age_seconds: int
    min_executable_targets: int
    max_executable_targets: int
    max_executables_per_static_library: int
    max_sources_per_executable: int
    require_executable_artifacts: bool
    min_measured_translation_units: int
    max_header_fanout: int


class _TestDeclarationIdentity(TypedDict):
    id: str
    selector: str
    source: str


class TestDeclaration(_TestDeclarationIdentity, total=False):
    source_anchor: str
    mappings: list[str]
    classification: str


class TestsPolicy(TypedDict, total=False):
    cases: list[TestDeclaration]
    min_cases: int
    known_mapping_ids: list[str]
    require_mappings: bool
    allow_extra_tests: bool
    require_execution: bool
    require_source_anchor: bool
    max_evidence_age_seconds: int


class ConsumerPolicy(TypedDict, total=False):
    executable: str
    junit: str
    result_json: str
    expected_sha256: str
    min_cases: int
    max_junit_age_seconds: int
    max_result_age_seconds: int


class PackagePolicy(TypedDict, total=False):
    prefix: str
    cache: str
    cache_path_keys: list[str]
    binding_report: str
    binding_keys: list[str]
    max_evidence_age_seconds: int
    required_globs: list[str]
    require_consumer_smoke: bool
    consumer: ConsumerPolicy


class _VerdictReportRequired(TypedDict):
    name: str
    path: str


class VerdictReportPolicy(_VerdictReportRequired, total=False):
    required: bool
    pass_field: str
    expected: JsonValue
    require_run_id: bool
    expected_schema: int
    expected_gate: str
    require_manifest: bool


class VerdictComparisonPolicy(TypedDict):
    name: str
    left_report: str
    left_field: str
    right_report: str
    right_field: str


class VerdictPolicy(TypedDict, total=False):
    reports: list[VerdictReportPolicy]
    min_reports: int
    max_age_seconds: int
    set_checks: list[VerdictComparisonPolicy]
    value_checks: list[VerdictComparisonPolicy]
    require_manifest: bool
    manifest_source_root: str
    require_clean_source: bool


class SourceSizeRecord(TypedDict):
    path: str
    physical_lines: int
    expanded_lines: int
    bytes: int
    expanded_bytes: int


class GraphTarget(TypedDict):
    id: str
    name: str
    type: str
    compiled_source_count: int
    dependencies: list[str]
    artifacts: list[str]


class _CMakeJsonFile(TypedDict):
    jsonFile: str


class CMakeReplyEntry(_CMakeJsonFile, total=False):
    kind: str
    version: dict[str, int]


class CMakeIndex(TypedDict, total=False):
    reply: dict[str, CMakeReplyEntry]
    objects: list[CMakeReplyEntry]


class CMakeTargetReference(TypedDict):
    jsonFile: str


class CMakeConfiguration(TypedDict, total=False):
    name: str
    targets: list[CMakeTargetReference]


class CMakeCodemodel(TypedDict, total=False):
    configurations: list[CMakeConfiguration]


class CMakeSource(TypedDict, total=False):
    compileGroupIndex: int


class CMakeArtifact(TypedDict):
    path: str


class CMakeDependency(TypedDict):
    id: str


class _CMakeTargetIdentity(TypedDict):
    id: str
    name: str
    type: str


class CMakeTargetRecord(_CMakeTargetIdentity, total=False):
    sources: list[CMakeSource]
    artifacts: list[CMakeArtifact]
    dependencies: list[CMakeDependency]


class _NormalizedTargetIdentity(TypedDict):
    id: str
    name: str
    type: str


class NormalizedGraphTarget(_NormalizedTargetIdentity, total=False):
    compiled_source_count: int
    artifacts: list[str]
    dependencies: list[str]


class _NormalizedGraphRequired(TypedDict):
    schema: int
    targets: list[NormalizedGraphTarget]


class NormalizedGraphReport(_NormalizedGraphRequired, total=False):
    configuration: str


class HeaderFanout(TypedDict):
    path: str
    translation_units: int


class CompileReport(TypedDict, total=False):
    records: list[JsonObject]
    header_fanout: list[HeaderFanout]


class ExecutionCase(TypedDict):
    name: str
    status: str


class ExecutionReport(TypedDict, total=False):
    schema: int
    run_id: str
    cases: list[ExecutionCase]


class TestInventoryEntry(TypedDict):
    name: str


class TestInventory(TypedDict, total=False):
    run_id: str
    tests: list[TestInventoryEntry]


class PackageBindingReport(TypedDict, total=False):
    schema: int
    run_id: str
    bindings: dict[str, JsonValue]


class SourceState(TypedDict):
    root: str
    revision: str
    dirty: bool
    state_sha256: str


class ManifestSource(TypedDict):
    root: str
    revision: str
    dirty: bool
    state_sha256: str


class ManifestBuild(TypedDict):
    configuration: str
    toolchain: str
    selected_targets: list[str]
    expected_executables: list[str]
    artifact_root: str | None


class ManifestInput(TypedDict):
    path: str
    sha256: str


class BuildManifest(TypedDict):
    schema: int
    kind: str
    run_id: str
    created_at_utc: str
    source: ManifestSource
    build: ManifestBuild
    inputs: list[ManifestInput]


class ConsumerMetrics(TypedDict, total=False):
    checked: bool
    case_count: int
    sha256: str


class VerdictEvidence(TypedDict, total=False):
    name: str
    status: str
    observed: JsonValue
    expected: JsonValue
    report: str
    sha256: str


class SetComparison(TypedDict):
    name: str
    left_count: int
    right_count: int
    missing_from_right: list[str]
    unexpected_on_right: list[str]


class ValueComparison(TypedDict):
    name: str
    left: JsonValue
    right: JsonValue


SECTION_KEYS = {
    "source": {
        "include",
        "exclude",
        "include_roots",
        "min_files",
        "soft_lines",
        "hard_lines",
        "hard_bytes",
        "single_consumer_header_lines",
        "legacy",
        "fragment_exceptions",
    },
    "graph": {
        "configuration",
        "include_targets",
        "exclude_targets",
        "max_evidence_age_seconds",
        "min_executable_targets",
        "max_executable_targets",
        "max_executables_per_static_library",
        "max_sources_per_executable",
        "require_executable_artifacts",
        "min_measured_translation_units",
        "max_header_fanout",
    },
    "tests": {
        "cases",
        "min_cases",
        "known_mapping_ids",
        "require_mappings",
        "allow_extra_tests",
        "require_execution",
        "require_source_anchor",
        "max_evidence_age_seconds",
    },
    "package": {
        "prefix",
        "cache",
        "cache_path_keys",
        "binding_report",
        "binding_keys",
        "max_evidence_age_seconds",
        "required_globs",
        "require_consumer_smoke",
        "consumer",
    },
    "verdict": {
        "reports",
        "min_reports",
        "max_age_seconds",
        "set_checks",
        "value_checks",
        "require_manifest",
        "manifest_source_root",
        "require_clean_source",
    },
}
