# Alphus complaint ledger

This ledger records concrete findings from the Umbra adoption profile that
were not caught by the configured checks, along with cases where a TODO was
not a valid tool complaint. It separates likely reusable checker improvements
from gaps in this repository's profile. No upstream issue is filed unless an
entry explicitly says so.

## Open checker candidates (8)

- **AC-001 — C++ local constness.** `cpp/src/internal/time/federation_time_bounds.cpp`
  had an initialized `epsilon` local that was never reassigned. The configured
  C++ style scan reported only a low-confidence unused-include candidate for
  this file, not the mutable local. The source now declares it `const`. A
  reusable C++ check could report initialized locals that are never modified;
  an AST-based implementation would avoid the false positives of a text rule.
  **Status:** local issue fixed; upstream candidate not filed.
- **AC-002 — Positional aggregate initialization.**
  `FederateTimeState::snapshot()` initialized a 22-field snapshot by position,
  making the mapping depend on declaration order. It now uses C++20 designated
  initializers. The file's only C++ style finding was a regex complexity
  estimate; the configured profile has no rule for long positional aggregate
  initializers. A reusable advisory could flag aggregates above a
  configurable field count and suggest designated initialization where the
  language standard permits it. **Status:** local issue fixed; upstream
  candidate not filed.
- **AC-003 — Markdown formatting is outside the profile.** The contributing
  guide's build-profile table had no formatter checking it. The table is now
  aligned, but the Alphus profile includes Python and C++ only. This is a
  consumer-profile coverage request unless Alphus adds an optional Markdown
  formatter step. **Status:** document fixed; profile gap remains; not filed.
- **AC-008 — Pydantic 2 `Field` declarations are reported as legacy.** The
  observer workspace model uses Pydantic 2 (`ConfigDict`, `model_validator`;
  dependency floor `pydantic>=2.7`), but the `legacy-pydantic-fields` rule flags
  each annotated assignment whose value is a call named `Field`. Its current
  implementation does not distinguish supported Pydantic 2 declarations such
  as `Field(default_factory=...)` from Pydantic 1 migration patterns. The
  latest complete run (`20261024T224315.028900Z`) had 26 such findings
  overall, with 23 in `workspace_models.py`. **Status:** local model is already
  Pydantic 2; checker false positive confirmed; not filed upstream.

- **AC-009 — Static export inventories are treated as opaque by `PLE0604`.** Four
  1516.1 API exports compose `__all__` from the canonical `types.__all__`,
  `_EXCEPTION_TYPES` registry, or `values.__all__`. Those sources produce only
  string names, but Ruff cannot follow the imports or the generated exception
  inventory and reports invalid objects. The local exports retain a narrow
  `noqa` with the canonical source named in a comment. **Status:** behavior is
  verified from the canonical inventories; checker candidate not filed.
- **AC-010 — `TRY004` misclassifies smoke assertions as invalid-input handling.**
  Three 2010 smoke scenarios raise `AssertionError` when an adapter returns the
  wrong public type or exception hierarchy. These assertions become explicit
  scenario failures; converting them to `TypeError` would describe the wrong
  contract. The local rule suppressions explain that distinction. **Status:**
  local assertions retained; checker candidate not filed.
- **AC-011 — Anonymous mapping conversions hide typed event contracts.** The
  opaque-mapping count checks broad annotations, but it did not flag analytics,
  alerting, and Pydantic schema code that rebuilt anonymous event dictionaries
  from typed observer models before calling mapping-based helpers. Field names
  and agreement rules were implicit despite zero opaque-mapping findings.
  Analytics and alerting now use `ParticipantEventMetadata`; participant
  identity, attribution, interaction-flow, relationship, coordination,
  ownership, declaration, participant-reference, MOM, FOM, and region schema paths
  now convert directly to domain values. `ObjectContextInput` also validates named
  fields without a dictionary round trip. The raw flow payload has a named
  `TypedDict`. Provider maps remain at the ingress parser, and other schema
  bridges are still open. The HTTP/core request conversion now has a named
  `event_input_to_raw_event()` helper returning `RawObserverEvent`, but its
  generic `model_dump()` and the core raw-map contract still hide field groups.
  A reusable diagnostic could flag dictionary literals passed into mapping-based
  domain helpers for review. **Status:** these participant, flow, relationship,
  coordination, ownership, declaration, participant-reference, MOM, FOM, region, and
  object-context contracts are fixed locally; checker candidate not filed.

- **AC-012 — Required `TYPE_CHECKING` imports are treated as findings.** The
  observer `models.py` keeps type-only imports for `ParticipantPerspective`
  and `ObjectContext` to avoid runtime cycles; Alphus reports both as
  `type-checking-imports`. Shared structural projection contracts keep FOM/MOM
  types precise without another type-only import. This rule does not distinguish
  necessary cycle boundaries from avoidable imports. **Status:** safe type-only
  imports retained; checker candidate not filed.

## Positive and negative examples

### AC-001 — C++ local constness

**Positive example, before the local fix** (`federation_time_bounds.cpp`):

```cpp
auto epsilon = factory.makeEpsilon();
if (!epsilon || epsilon->implementationName() != factory.getName()) {
  return nullptr;
}
*candidate += *epsilon;
```

The local binding is never assigned, moved from, or passed as an output
argument. `auto const epsilon` preserves mutation through the pointed-to time
interval while preventing reassignment of the owning pointer.

**Negative examples from the same codebase:**

- `auto candidate` in `cloneAndAdvance` is moved into `shared`; making that
  `unique_ptr` binding `const` would prevent the move.
- `cursor` in `parseProfiles` is advanced on each line, and `requesterPresent`
  in `FederationTimeBoundsCalculator::calculate` changes when a match is
  found. Neither should be reported.
- Do not use `const` on by-value function parameters as the goal of this rule;
  that does not protect the caller's object and adds declaration noise.

**Rule boundary:** check local variable bindings, not whether objects reached
through pointers are mutated. Exclude bindings assigned later, moved from,
passed to non-const reference/output parameters, or declared as references.
The saved Alphus style report has only a possible-unused-include finding for
`federation_time_bounds.cpp`; it has no constness diagnostic.

### AC-002 — Positional aggregate initialization

**Positive example, before the local fix** (`FederateTimeState::snapshot`): a
22-field `FederateTimeSnapshot` was returned as a positional braced list. The
snapshot declares its fields in `federate_time_state.hpp`; a declaration-order
change could silently pair a value with the wrong field. The corrected source
uses `.field = value` designators for every member.

**Negative example** (`federation_time_coordinator.cpp`):

```cpp
return {FederationTsoDeliveryStatus::invalid_recipient, {}};
```

This initializes the two-field `FederationTsoDeliveryResult`; it is compact
and should stay positional. Already-designated initializers and braced
constructor calls are also outside this rule.

**Rule boundary:** restrict the advisory to aggregate initialization and make
the field-count threshold configurable. For long lists, suggest member
designators where the selected language standard supports them; do not require
default-initialized members to be spelled out, and do not flag constructor
calls or short status/result aggregates. The local evidence supports reporting
the 22-member case and leaving the two-member result alone; it does not
establish the best threshold for every consumer. The saved Alphus report for
`federate_time_state.cpp` has a regex complexity estimate, not an initializer
diagnostic.

### AC-003 — Markdown formatting

**Positive example, before the local fix** (`CONTRIBUTING.md`):

```markdown
| Goal | Profile | Dependency behavior |
| --- | --- | --- |
| Core build, headers, contracts, and package smoke | native | No dependency download by default. |
```

The pipe columns were not aligned. A Markdown formatter can make this stable
when the document is edited; the current Alphus file set contains no `.md`
extension, so the profile cannot report it.

**Negative example:** preserve fenced examples such as the `.ini` block in
`docs/design/AUTHORIZATION-DESIGN.md`; table-like text inside a code fence is
example content, not a Markdown table to reformat. A repository-wide formatter
pass would also create unrelated churn, so start with changed Markdown files
and review its diff.

**Rule boundary:** this is a formatting integration gap, not a C++ or Python
smell. Treat Markdown formatting as an optional consumer-profile step with a
chosen formatter and an explicit scope.

### AC-008 — Pydantic 2 `Field` declarations

**Positive example:** `workspace_models.py` defines nested defaults with
`Field(default_factory=EventViewSettings)` and other Pydantic 2 models. The
same module uses `ConfigDict` and `@model_validator`, and the observer package
requires Pydantic 2.7 or newer.

**Negative example:** a migration check can flag Pydantic 1-only constructs
such as `@validator` or an inner `class Config` when their behavior needs a
Pydantic 2 replacement. A `Field` call alone does not identify either pattern.

**Rule boundary:** classify the specific legacy API or configuration being
used; do not report the shared `Field` API as legacy based on the call name.
The scanner source confirms the current rule flags every annotated `Field`
assignment without examining its arguments or installed Pydantic version.

## Closed or not promoted (4)

- **AC-004 — Canonical authorizer name.** The RID parser compared a string
  literal with the HLA authorizer name even though the official API exposes
  `rti1516_2025::HLAauthorizerName`. The parser now uses that API constant and
  names its private `disabled` token. This is a useful AI review smell, but a
  generic scanner cannot infer which string literals duplicate an external
  API constant without project-supplied canonical-name data. **Status:** local
  issue fixed; no upstream complaint filed.
- **AC-005 — RID grammar documentation.** A TODO asked whether the file was
  TOML, but the supported profile syntax and security constraints are already
  documented in `docs/design/AUTHORIZATION-DESIGN.md`. The stale TODO was
  removed and the parser points to that document. **Status:** already
  documented; no tool defect.
- **AC-006 — Shared trim utility.** The TODO suggested moving the RID parser's
  trim helper to common utilities. The helper has one consumer and preserves a
  `string_view` while trimming only the three whitespace bytes accepted by
  this parser; the FOM helper has different whitespace and ownership
  semantics. The TODO was replaced with a comment explaining the local
  contract. **Status:** no extraction warranted; no tool defect.
- **AC-007 — `class` versus `struct`.** The TODO objected to `MaterializedFdd`
  changing from `struct` to `class`, but the type encapsulates private state
  behind public accessors. The declaration is intentional and does not define
  a reliable static-checking rule. **Status:** TODO removed; no tool defect.

## Evidence

The findings above come from the configured Alphus profile pinned in
`tools/requirements-alphus.txt` and described in `quality-gates.toml` on
2026-10-24. The latest completed local profile (`20261024T230203.188946Z`)
took 78.099 seconds and reported 4,367 Ruff diagnostics, 184 formatting
candidates, 608 structure candidates including 114 opaque mappings, and 47
file-size findings. Compared with the previous recorded profile, this is 2
fewer Ruff diagnostics, 2 fewer formatting candidates, 12 fewer structure
findings, and 8 fewer opaque mappings. The wrapper exited 1 because
repository-wide report-only findings remain. The process-package verifier and
observer FOM/MOM event fixtures now have zero Alphus findings. The verifier
uses named JSON input shapes normalized into slotted records; the observer
fixtures share the production `JsonValue` alias and declare named payload
TypedDicts. The observer adapter still has no Ruff or formatting findings;
workspace-model formatting is clean, with 23 of 26 Pydantic false positives
remaining there. The configured observer Pyright project covers `src` only;
an exploratory direct check of `test_fom.py` reports 38 diagnostics in other
response-union and invalid-capacity assertions, so test typing coverage is a
known gap. The report stays outside CI, and no test suites were run. The next
large extraction is `tools/query_rti_work_regression.py`: its 15,134-line file
contains a 13,822-line `main` with 2,552 top-level statements, so its suite
boundaries need to be identified before moving blocks. Re-evaluate open
candidates when the pinned Alphus revision or profile configuration changes.

## Additional source TODOs resolved

- **Annex C temporary directories:** extracted atomic create/retry and
  best-effort cleanup into `cpp/tests/scoped_temporary_directory.hpp`, then
  used it from all 11 Annex C FOM composition fixtures that duplicated this
  lifecycle. It retries collisions across concurrent test processes and throws
  on filesystem errors other than an existing candidate path.
- **2010 floating time constructors:** moved shared validation and zero
  canonicalization into `checkedTime` and `checkedInterval`. Time and interval
  constructors now initialize their implementation values directly rather
  than calling virtual setters; the public setters use the same helpers.
- **2010 fixed-record include:** retained `RTI/VariableLengthData.h` because
  the implementation directly names that type; the include now says why.
- **2010 `auto_ptr`:** retained the exact return type because the pinned
  2010 `DataElement::clone()` declaration requires it. Replacing it would
  break the standard API override and ABI.
- **Callback instrumentation labels:** centralized the four callback event
  labels as named constants. They are observable snapshot keys, and
  `callback_dispatcher_catch2.cpp` asserts their spelling.
- **FOM validation declarations:** removed redundant `inline` from the three
  `constexpr` header functions; C++ already makes constexpr functions inline.
- **Tool unit-test placement:** moved all six `tools/test_*.py` unit-test
  modules into `tools/tests/`, added the package marker, and updated the
  observer workflow and roadmap source pointer. This clears the misplaced
  `test_observer_playtest.py` TODO. A future path-policy check could catch
  tests added outside test directories before review.
- **JUnit merge dependency:** kept `tools/merge_junit_reports.py` on Python's
  standard XML library. Its consumers merge Catch2's flat `testsuite` entries;
  adding a dependency for that bounded format would increase setup without
  adding needed behavior.

The remaining source occurrence of the word `TODO` is the query tool's status
description for planned versus completed test handles, not an unresolved code
action. Other occurrences in this ledger describe resolved TODO notes.


## 2026-10-24 — query regression contracts and setup friction

The query regression command started as a 15,134-line script with most checks
inside one `main`. Its suite boundaries were implicit: four groups depended on
a small shared set of index, plan, lane, and test values. The first split used
named immutable input records and preserved the original CLI, but left a
4,200-line mapping suite. Alphus surfaced that as a new size candidate. A
second pass separated object/region, time, process, and MOM checks; the modules
are now 1,190, 818, 814, and 1,416 lines. The time/process/MOM checks receive
only `MappingLookupInputs(index, tests)`, making that dependency visible at the
call site.

The full Alphus run `20261024T231727.915292Z` took 73.509 seconds. It reported
4,364 Ruff findings, 184 formatting candidates, 608 structure findings
including 114 opaque mappings, and 47 file-size candidates. The split reduced
Ruff by three from the prior documented run and removed the temporary mapping
file-size candidate. The command driver remains 4,986 lines and is still an
Alphus size candidate; the repository-wide opaque-mapping count did not change
in this structural pass. Syntax compilation, formatting, import/unused-noqa
checks, and `git diff --check` passed for the changed files. Test suites were
not run, and GitHub CI is unchanged.

Pain point: preserving the old command while extracting suites took explicit
source-boundary analysis because the original function shared local variables
without documenting which suite owned them. The remaining command driver also
still uses generic decoded JSON objects in several paths. Next, give
`tools/query_rti_work.py` named command/input contracts, starting with its 73
Alphus concrete-input findings, then continue through the highest-value opaque
mapping boundaries.


## 2026-10-24 — observer fixtures and worktree inventory contracts

The observer fixture batch replaced anonymous ownership and region event maps,
command parameter maps, and callback event dictionaries with named
`TypedDict` records or a small showcase-event protocol. It also moved local
imports to module scope and formatted the touched fixtures. The adapter test
now has clean import order. The four changed observer tests and adapter test
have no current Alphus findings; observer production source and the Umbra
adapter source still have zero opaque mappings.

`quality_gates/bin/worktree_hygiene.py` had five opaque mappings and six
concrete-input candidates. Its registered worktrees, inspection results, and
unregistered checkouts now have separate `TypedDict` records; read-only list
inputs use `Sequence`. That kept the scan behavior stable while making the
boundary fields visible. The file now has no Alphus findings. One checker
friction point was that replacing broad maps exposed concrete-list inputs, so
a follow-up type pass was needed after the first TypedDict conversion.

The final local profile for this pass, `20261024T233540.246789Z`, took
65.333 seconds. It reported 4,360 Ruff findings, 179 formatting candidates,
564 structure findings including 95 opaque mappings, and 47 file-size
findings. Compared with the last documented run, this removed 19 opaque
mappings, 44 structure findings, 5 formatting candidates, and 4 Ruff findings;
file-size findings were unchanged. The observer test files and worktree
inventory changes have no report findings. C++ style/statistics passed; the
full wrapper remains non-clean on existing Python and C++ report candidates,
and Lizard. Test suites were not run; GitHub CI is unchanged.

The next high-count boundary in the current report is
`tools/check_source_file_sizes.py` with four opaque mappings. After that, return
to the 73 concrete-input findings in `tools/query_rti_work.py` and continue
through the remaining opaque-mapping paths.

## 2026-10-24 — source-size checker contracts

`tools/check_source_file_sizes.py` now names the JSON policy configuration,
optional legacy expanded-size exemption, and size-finding row with `TypedDict`
contracts. The scanner consumes read-only mappings, the file categories are a
`Literal` union, suffix inventories are immutable, the expansion cache is a
mutable mapping, and CLI arguments accept a `Sequence`. Its four opaque
mapping, four concrete-input, and two mutable-module-constant findings are all
clear. The JSON keys, size thresholds, and report columns are unchanged; the
pre-existing include-ownership and blocker behavior in this dirty file was
preserved.

The final profile for this batch, `20261024T234132.700436Z`, took 64.335
seconds. It reported 4,360 Ruff findings, 179 formatting candidates, 551
structure findings including 91 opaque mappings, and 47 file-size findings.
Compared with `20261024T233540.246789Z`, structure fell by 13 and opaque
mappings by four; the Ruff, formatting, and file-size totals stayed level.
The changed checker passes Ruff, formatting, syntax compilation, and
`git diff --check`. The full profile remains non-clean on existing Python
lint/format/structure/size, C++ ownership/size, and Lizard candidates; C++
style and statistics pass. The shell selected Python 3.10, which had no
matching Alphus launcher; the documented `out/alphus-env` environment ran the
profile successfully after selection. No tests ran and GitHub CI is unchanged.

Next is the 73 concrete-input findings in `tools/query_rti_work.py`, while
continuing to replace opaque mappings with contracts named for their actual
consumer boundary.

## 2026-10-24 — catalog, gate, and provider-result contracts

The Java TCK scaffold now parses its nested catalog into named scenario,
mapping, and exclusion records. It validates string fields and string lists at
the JSON boundary and reports the scenario/field for malformed data. The
catalog's output keys and generated Markdown columns remain the same; a local
generation to `out/quality` rendered 756 scenarios and 534 distinct
requirement IDs. The generator now has zero Alphus structure findings.

The portable C++ quality-gate fixture now reuses the gate package's existing
`GateReport`, `SourceState`, and recursive `JsonValue` contracts. It has zero
Alphus findings. The 2010 JPype smoke runner now returns named scenario-result
records with a literal status contract and uses the capability profile's
explicit modes. Its scenario JSON fields are unchanged; the four opaque
mapping findings were cleared. Two lazy imports remain in the optional JNI and
native-provider startup paths, and this pass kept that isolation. The runner's
import-order Ruff finding is also clear.

The latest full local profile, `20261024T235218.027877Z`, took 64.879 seconds
and ended with wrapper exit 1 due existing candidates: Ruff 4,358 (37
production, 3,847 tests/fixtures, 468 other, 6 adapters), 178 formatting
candidates, 530 structural findings including 79 opaque mappings, and 47
file-size findings. Compared with `20261024T233540.246789Z`, structure fell by
34, opaque mappings by 16, Ruff findings by two, and formatting candidates by
one; file-size findings stayed level. The source-size checker, Java scaffold,
and portable gate fixture have no Alphus structure findings; observer source
and adapter remain at zero opaque mappings. The 2010 smoke runner retains five
other structure findings. Its import-order and syntax checks pass; the full
profile still reports one formatting candidate, three concrete-input
findings, two lazy imports, and its 4,676-line size candidate. Ruff,
formatting, syntax checks, and the generated scaffold command pass for the
source-size checker, Java scaffold, and gate fixture. C++ style/statistics
passed; Python lint/format/structure/size, C++ ownership/size, and Lizard
remain non-clean. No test suites ran and GitHub CI is unchanged.

Next is `packages/umbra-rti-jpype/src/umbra/_java/rti1516e/provider.py`, with
three opaque mapping candidates and two mutable lookup-table candidates. Name
the Java handle result/cache contracts while preserving the 2010 provider's
boundary and conversion behavior.

## 2026-10-25 — 1516e Java provider lookup contracts

The 1516e Java provider now gives its lookup boundaries semantic `Literal`
keys, immutable `Mapping` tables, named handle descriptors, and explicit raw
handle cache key/carrier records. It also replaces the empty-string factory
sentinel with `None`. This clears the provider's three opaque-mapping and nine
mutable-module-constant findings. The provider has no remaining Alphus report
findings. Ruff, format, Python compilation, and `git diff --check` pass for the
file.

The full local profile, `20261024T235947.764179Z`, took 65.104 seconds and
ended with wrapper exit 1 on remaining report candidates. It reported 4,358
Ruff findings (37 production, 3,847 tests/fixtures, 468 other, 6 adapters),
177 formatting candidates, 518 structural findings across 119 files including
76 opaque mappings, and 47 file-size findings. Compared with
`20261024T235218.027877Z`, structural findings fell by 12, opaque mappings by
three, and formatting candidates by one; Ruff and file-size totals stayed
level. C++ style and statistics passed; Python lint, formatting, structure,
file-size, C++ ownership/size, and Lizard steps remain non-clean. Observer
production source and adapter still have zero opaque mappings.

An exploratory file-scoped Pyright run reported 49 diagnostics around dynamic
JPype proxies, API overrides, and encoded-value narrowing. It was not clean;
there was no baseline comparison, so this run does not establish whether any
diagnostic was introduced by this batch. Using the pinned `out/alphus-env`
caused no setup friction. No tests ran, and CI and Requirements Lab revisions
are unchanged.

Next is `packages/umbra-rti-jpype/src/umbra/_java/rti1516e/runtime.py`, which
has three opaque-mapping candidates. Name the runtime bridge payloads and
collection boundaries, then verify the full profile with observer source and
adapter included.

## 2026-10-25 — Java runtime and type-surface report contracts

The 1516e runtime now uses literal names for its handle and enum class tables,
immutable mappings with `EncodedHandle` and `EnumType` values, and a `TypedDict`
for JVM startup options. This clears its three opaque mappings. The Python
type-surface verifier now declares its module inventory and top-level report
with `TypedDict` records and literal module/alias keys; its three opaque
mappings are clear. That pass also exposed a `Path` value passed to a `str`
parameter, now corrected by typing the Java source path as `Path`. Both files
pass Ruff, formatting, and syntax checks; the verifier also passes focused
Pyright with zero diagnostics.

The full profile, `20261025T000643.050610Z`, took 64.993 seconds and ended
with wrapper exit 1 on remaining report candidates. It reported 4,357 Ruff
findings (37 production, 3,847 tests/fixtures, 467 other, 6 adapters), 176
formatting candidates, 510 structural findings across 118 files including 70
opaque mappings, and 47 file-size findings. Compared with
`20261025T000430.038631Z`, Ruff fell by one, formatting candidates by one,
and structural findings and opaque mappings each fell by three; file-size
findings stayed level. The Java provider has no Alphus findings, and the
type-surface verifier has none. The runtime retains two local-import findings:
the Java interval façade import and the intentionally lazy optional JPype
import. Observer source and adapter remain at zero opaque mappings. C++ style
and statistics passed; Python lint/format/structure/size, C++ ownership/size,
and Lizard remain non-clean. Used the pinned environment without setup issues;
no tests ran and CI and Requirements Lab revisions are unchanged.

An exploratory Pyright run on the runtime reports 48 diagnostics in its Java
proxy conversions and the optional JPype import. None point to the new table
or JVM-options contracts. No baseline comparison was made, so this does not
establish whether the other diagnostics predate this batch.

Next is `packages/hla-rti-cpp-tck/adapters/current-process/run_confirm_divestiture.py`,
which has three opaque-mapping candidates. Replace its unstructured payloads
with contracts named for the adapter boundary, then run the full local profile
with observer source and adapter coverage included.

## 2026-10-25 — shared current-process TCK evidence

The current-process Confirm Divestiture and Connection Loss adapters duplicated
their TCK JSON loading and merge behavior. Both now use the shared
`tck_evidence.py` boundary: it names the result fields, validates the document,
result array, scenario ID, callback model, status, and optional message/duration
fields, and preserves additional JSON fields when merging. This clears six
opaque mappings. Both merge functions accept read-only `Sequence` inputs,
clearing four concrete-input findings, and both adapter files are formatted.
The pre-existing Connection Loss fixture-startup diagnostic changes were
preserved.

The first full profile after the shared extraction, `20261025T001238.174028Z`,
took 64.844 seconds and reported 4,354 Ruff findings, 174 formatting
candidates, 504 structural findings including 64 opaque mappings, and 47
file-size findings. After changing four read-only merge inputs from `list` to
`Sequence`, the final profile, `20261025T001414.981875Z`, took 64.365 seconds
and ended with wrapper exit 1 on remaining repository candidates. It reported
the same 4,354 Ruff findings (37 production, 3,847 tests/fixtures, 467 other,
3 adapters) and 174 formatting candidates, 500 structural findings across 116
files including 64 opaque mappings, and 47 file-size findings. Relative to the
previous recorded profile, `20261025T000643.050610Z`, Ruff fell by three,
formatting candidates by two, structural findings by ten, and opaque mappings
by six; file-size findings stayed level. The two C++ process adapters and
shared evidence helper have no Alphus findings. Observer source and adapter
remain at zero opaque mappings. Focused Ruff, format, syntax compilation, and
Pyright pass for the three changed Python files. C++ style and statistics pass;
Python lint/format/structure/size, C++ ownership/size, and Lizard remain
non-clean. The pinned environment ran without setup friction. No test suites
ran and CI and Requirements Lab revisions are unchanged.

Next is `packages/umbra-rti-jpype/tests/test_jpype_2010_jni_type_roundtrip.py`,
with three opaque mappings at the decoded-handle record, callback capture
records, and owner result cache. Give those per-test outputs named record types
and verify with the full local profile.

## 2026-10-25 — JNI round-trip test contracts

The JNI round-trip fixture now keys decoded handle values by the corresponding
handle class, records reflect/receive/remove callback arguments under a
callback-specific `TypedDict`, and replaces the one-at-a-time owner result
dictionary with a named pending-return record. The callback capture helper
raises a clear assertion if one of the expected final overloads was not seen.
This clears the test's three opaque mappings. Ruff also sorted the support
imports and formatted the file, removing one Ruff and one format candidate.
Replacing a redundant method-local JPype import with the class setup's saved
JPype object clears one more local-import finding; the setup import stays lazy
because JPype is optional. Ruff, format, syntax compilation, and diff checks
pass. Test suites were not run.

The full profile, `20261025T002139.794972Z`, took 69.675 seconds and ended with
wrapper exit 1 on repository candidates. It reported 4,353 Ruff findings (37
production, 3,846 tests/fixtures, 467 other, 3 adapters), 173 formatting
candidates, 496 structural findings across 116 files including 61 opaque
mappings, and 47 file-size findings. Compared with
`20261025T001414.981875Z`, Ruff and formatting each fell by one, structural
findings fell by four, opaque mappings by three, and file-size findings stayed
level. Observer source and adapter remain at zero opaque mappings. C++ style
and statistics passed; Python lint/format/structure/size, C++ ownership/size,
and Lizard remain non-clean. The pinned profile environment ran without setup
friction; CI and Requirements Lab revisions are unchanged.

An exploratory whole-file Pyright run during the batch remained non-clean at
146 diagnostics, concentrated in JPype proxies, generated callback overrides,
and abstract RTI interfaces. The diagnostics specific to optional callback
captures were resolved with the required-key record and explicit missing-call
assertions. There was no baseline comparison for the remaining diagnostics.

## 2026-10-25 — periodic MIM survey contracts

`tools/audit_cpp_tck_mim_periodic.py` now gives the XML attributes, lexical
source hits, detailed rows, and both report modes named `TypedDict` contracts.
The source-use mapping states its value record explicitly; the scanner accepts
any path-like source and a read-only abstract set of attribute names. Its three
opaque mappings, concrete-input finding, mutable default-source list, Ruff
import finding, and format candidate are clear. The summary and `--details`
outputs retain 21 MIM attributes, four lexical references, and 41 source hits.
Focused Ruff, formatting, syntax, output-mode checks, and diff checks pass; no
test suites were run.

The full profile `20261025T002810.002428Z` took 64.289 seconds and ended with
wrapper exit 1 on repository candidates: 4,352 Ruff findings (37 production,
3,846 tests/fixtures, 466 other, 3 adapters), 172 formatting candidates, 491
structural findings across 115 files including 58 opaque mappings, and 47
file-size findings. Compared with `20261025T002139.794972Z`, Ruff and
formatting each fell by one, structure fell by five, opaque mappings fell by
three, and file-size findings stayed level. C++ style and statistics pass;
Python lint/format/structure/size, C++ ownership/size, and Lizard remain
non-clean. Observer source and adapter remain at zero opaque mappings. CI and
Requirements Lab revisions are unchanged.

Setup friction stayed low with the pinned profile environment. One reporting
pain point surfaced: `out/quality/alphus-python-style-current.json` is several
hours older than this profile and still shows findings already cleared in the
latest lint log. Use the current run's summary and detailed log for per-run
evidence until that stale artifact is fixed.

## 2026-10-25 — C++ TCK API catalog contract

`tools/audit_cpp_tck_api_surface.py` now validates the catalog root, scenario
array, and each `api_methods` string list into named `TypedDict` records before
coverage calculations. Its three structure findings (bare container, concrete
input, and opaque mapping) and one format candidate are clear. The default
audit remains valid with 1,530 scenarios, 164 RTI methods, and 56 callbacks
catalog-covered. Ruff, formatting, syntax compilation, Pyright, and diff checks
pass; Pyright reports zero diagnostics. No test suites were run.

The full profile `20261025T003226.989376Z` took 64.876 seconds and ended with
wrapper exit 1 on remaining repository candidates: 4,351 Ruff findings (37
production, 3,846 tests/fixtures, 465 other, 3 adapters), 171 formatting
candidates, 488 structural findings across 114 files including 57 opaque
mappings, and 47 file-size findings. Compared with
`20261025T002810.002428Z`, Ruff and formatting each fell by one, structure fell
by three, opaque mappings fell by one, and file-size findings stayed level.
Profile step timings were 3.029 seconds for Python lint, 3.288 for formatting,
5.840 for Python structure, 3.821 for Python file size, 8.953 for C++
ownership/size, 9.709 for C++ style, 23.583 for Lizard, and 0.467 for C++
statistics. The pinned environment had no setup friction. The style-current
JSON remained stale; the fresh run summary and detailed log were used.
Observer source and adapter remain clear of opaque mappings, with remaining
observer findings in test fixtures. CI and Requirements Lab revisions are
unchanged.

## 2026-10-25 — external SISO manifest contract

`tools/verify_external_siso_fom_corpus.py` now returns a named immutable
`CorpusManifest` record instead of `dict[str, Any]`. Raw entries remain typed as
`object` until the verifier checks each one, then `FixtureFields` names the five
fields consumed by the integrity checks. The existing per-fixture validation,
finding text, order, and exit behavior remain in place. Ruff, formatting,
syntax compilation, and focused Pyright pass; Pyright reports zero diagnostics.
The configured repository manifest loads as 22 fixtures and zero
schema-negative fixtures, with no manifest findings. No test suites or external
corpus verification ran in this batch.

The full profile `20261025T003608.637759Z` took 65.019 seconds and ended with
wrapper exit 1 on remaining repository candidates: 4,350 Ruff findings (37
production, 3,846 tests/fixtures, 464 other, 3 adapters), 170 formatting
candidates, 487 structural findings across 113 files including 56 opaque
mappings, and 47 file-size findings. Compared with
`20261025T003226.989376Z`, Ruff and formatting each fell by one, structure and
opaque mappings each fell by one, and file-size findings stayed level. Step
timings were 3.049 seconds for Python lint, 3.418 for formatting, 5.792 for
Python structure, 3.899 for Python file size, 9.329 for C++ ownership/size,
9.675 for C++ style, 23.201 for Lizard, and 0.463 for C++ statistics. The
pinned environment had no setup friction. The style-current JSON remains stale;
the current run summary and detailed log were used. Observer source and adapter
remain clear of opaque mappings; 15 current observer opaque findings are in
test fixtures. CI and Requirements Lab revisions are unchanged.

## 2026-10-25 — observer recording JSON contract

The `recording()` fixture helper in
`packages/umbra-rti-observer/tests/test_recording.py` now returns the existing
recursive `JsonObject` type used by `ObserverStore.capture_document()`. Its
default-store selection uses an explicit `None` check. This clears the file's
opaque mapping and format candidate; focused Ruff, formatting, syntax, and diff
checks pass. No tests ran. An exploratory direct Pyright pass reports 318
diagnostics in nested wire-document mutations. The configured observer
Pyright project excludes tests, and there is no baseline comparison, so that
run is not a supported gate.

The full profile `20261025T004000.725561Z` took 68.001 seconds and ended with
wrapper exit 1 on remaining repository candidates: 4,349 Ruff findings (37
production, 3,845 tests/fixtures, 464 other, 3 adapters), 169 formatting
candidates, 486 structural findings across 112 files including 55 opaque
mappings, and 47 file-size findings. Compared with
`20261025T003608.637759Z`, Ruff, formatting, structure, opaque mappings, and
files with structure findings each fell by one; file-size findings stayed
level. Step timings were 3.039 seconds for Python lint, 3.302 for formatting,
6.176 for Python structure, 4.102 for Python file size, 9.033 for C++
ownership/size, 9.771 for C++ style, 25.939 for Lizard, and 0.498 for C++
statistics. The pinned environment had no setup friction. The style-current
JSON remains stale; the current summary and detailed log were used. Observer
source and adapter remain clear of opaque mappings; 14 current observer
opaque findings remain in test fixtures. CI is unchanged.

## 2026-10-25 — observer probe response contract

`packages/umbra-rti-observer/tests/test_probe.py` now types the endpoint map as
`dict[str, JsonObject]`, matching the shape of each fake API response while
preserving its URL-to-payload lookup and malformed-response mutations. Import
ordering and formatting are clean. Focused Ruff, formatting, syntax, and direct
Pyright all pass; Pyright reports zero diagnostics. No tests ran.

The full profile `20261025T004224.163268Z` took 64.561 seconds and ended with
wrapper exit 1 on remaining repository candidates: 4,348 Ruff findings (37
production, 3,844 tests/fixtures, 464 other, 3 adapters), 168 formatting
candidates, 485 structural findings across 111 files including 54 opaque
mappings, and 47 file-size findings. Compared with
`20261025T004000.725561Z`, Ruff, formatting, structure, opaque mappings, and
files with structure findings each fell by one; file-size findings stayed
level. Step timings were 3.051 seconds for Python lint, 3.342 for formatting,
5.783 for Python structure, 3.812 for Python file size, 9.005 for C++
ownership/size, 9.574 for C++ style, 23.154 for Lizard, and 0.460 for C++
statistics. The pinned environment had no setup friction. The style-current
JSON remains stale; the current summary and detailed log were used. Observer
source and adapter remain clear of opaque mappings; 13 current observer opaque
findings remain in test fixtures. CI is unchanged.

## 2026-10-25 — observer time-observation payload

`packages/umbra-rti-observer/tests/test_time_views.py` now declares the mutable
time-observation payload as `JsonObject` and its time, identity, and state-patch
inputs as `JsonValue`. Optional time records and reported state are explicitly
checked before field access; the negative-capacity case marks its deliberate
runtime-invalid input with a cast. The file's opaque-mapping and format
candidates are clear. Focused Ruff, formatting, syntax, diff, and direct
Pyright checks pass with zero diagnostics. No tests ran.

The full profile `20261025T004543.906221Z` took 65.478 seconds and ended with
wrapper exit 1 on remaining repository candidates: 4,348 Ruff findings (37
production, 3,844 tests/fixtures, 464 other, 3 adapters), 167 formatting
candidates, 484 structural findings across 110 files including 53 opaque
mappings, and 47 file-size findings. Compared with
`20261025T004224.163268Z`, Ruff and file-size totals stayed level; formatting,
structure, opaque mappings, and files with structure findings each fell by
one. Step timings were 3.167 seconds for Python lint, 3.278 for formatting,
5.907 for Python structure, 4.079 for Python file size, 8.881 for C++
ownership/size, 9.558 for C++ style, 23.510 for Lizard, and 0.465 for C++
statistics. The pinned environment had no setup friction. The style-current
JSON remains stale; the current summary and detailed log were used. Observer
source and adapter remain clear of opaque mappings; 12 current observer
opaque findings remain in test fixtures. CI is unchanged.

## 2026-10-25 — observer scale workload input

The negative workload fixture in
`packages/umbra-rti-observer/tests/test_scale.py` now accepts a read-only
`Mapping[str, JsonValue]`. It hands a copied object to `Workload.model_validate`
so its boolean, out-of-range, and wrong-type examples remain runtime-invalid
inputs. Optional recording and allocation results are checked before access.
The file's opaque mapping, concrete-input, and formatting findings are clear.
Focused Ruff, formatting, syntax, diff, and direct Pyright checks pass with zero
diagnostics. No tests ran.

The full profile `20261025T004859.487117Z` took 64.784 seconds and ended with
wrapper exit 1 on remaining candidates: 4,349 Ruff findings (37 production,
3,845 tests/fixtures, 464 other, 3 adapters), 166 formatting candidates, 482
structural findings across 109 files including 52 opaque mappings, and 47
file-size findings. Compared with `20261025T004543.906221Z`, formatting fell
by one, structure fell by two, opaque mappings and files with structure
findings fell by one, and file-size findings stayed level. Ruff rose by one
in tests/fixtures although the changed file is clean; the current detailed log
does not identify a finding in the target. Step timings were 3.086 seconds for
Python lint, 3.315 for formatting, 5.826 for Python structure, 3.822 for
Python file size, 9.102 for C++ ownership/size, 9.579 for C++ style, 23.343
for Lizard, and 0.472 for C++ statistics. A transient Windows mapped-file
lock delayed formatting; after it cleared the file passed the format check.
The pinned environment had no setup friction. The style-current JSON remains
stale; the current summary and detailed log were used. Observer source and
adapter remain clear of opaque mappings; 11 current observer opaque findings
remain in test fixtures. CI is unchanged.

Next is `packages/umbra-rti-observer/tests/test_object_views.py:59`, where a
test helper accepts arbitrary attribute values through `**attributes: object`
and Alphus reports both opaque-mapping and concrete-input findings. Define a
precise test-side provider-value contract that still covers bytes and nested
values accepted at observer ingress; preserve the raw-byte normalization case
and verify the full profile with observer source and adapter coverage.

## 2026-10-25 — observer object, relationship, and interaction-flow test contracts

The object-view fixture now names the provider-side attribute values it accepts:
`JsonValue | bytes | bytearray | memoryview`. Its invalid snapshot-metadata
cases use `JsonObject`. Relationship and interaction-flow fixtures now declare
event overrides and invalid payloads as `JsonValue`/`JsonObject`; the
interaction-flow direction envelope is a read-only `Mapping[str, str]`. These
three files have no remaining Alphus findings. Observer production source and
the adapter still have zero opaque mappings; six opaque-mapping findings remain
in observer test fixtures.

Focused Ruff, formatting, observer Pyright, and syntax compilation pass. Pyright
reports zero diagnostics. No test suites ran, and CI is unchanged.

The final full profile `20261025T005814.805958Z` took 64.694 seconds and ended
with wrapper exit 1 on remaining repository candidates: 4,347 Ruff findings
(37 production, 3,843 tests/fixtures, 464 other, 3 adapters), 163 formatting
candidates, 473 structural findings across 106 files including 47 opaque
mappings, and 47 file-size findings. Compared with the last logged profile
`20261025T004859.487117Z`, Ruff fell by two, formatting by three, structure by
nine, opaque mappings by five, and files with structure findings by three;
file-size findings stayed level. Step timings were 3.071 seconds for Python
lint, 3.299 for formatting, 5.845 for Python structure, 3.792 for Python file
size, 9.141 for C++ ownership/size, 9.566 for C++ style, 23.380 for Lizard,
and 0.457 for C++ statistics. A first launcher attempt used the system Python
and failed because no adjacent `alphus.exe` was present; putting
`out/alphus-env/Scripts` first on `PATH` selected the pinned environment and
completed the Python/C++ scans. The Markdown steps could not start because
`rumdl` and `mdrepo` are absent from this environment, even though the documented
setup installs both. A requirements reinstall hit a permission failure in the
system temporary directory; redirecting temporary files into the workspace
then exposed blocked package-index DNS for the pinned rumdl install. The
adjacent `../mdrepo` checkout exists but is not installed in this venv. The
checked-in style-current JSON remains stale; current summary and log were used.

Next is `packages/umbra-rti-observer/tests/test_declarations.py:16`: replace
the helper's `object` declaration payload with the observer JSON contract and
type the invalid declaration fixtures precisely, while keeping the
non-JSON capacity rejection inputs explicit.

## 2026-10-25 — observer core and TCK report JSON contracts

The observer declaration and coordination fixtures now build `JsonObject`
events from `JsonValue` fields. In the core tests, wait results use the store's
`JsonObject` type; the callback failure-isolation case now records only the
validated event-type string instead of retaining an opaque provider mapping.
Adapter capability fixtures are marked as `ClassVar[JsonObject]`, and their
deliberate wrong-type checks raise `TypeError`. The Java TCK JPype handoff
replaces `Any` report maps with a recursive JSON type, validates the direct
report/scenario shapes, and keeps its output steps in a typed list. The
intentional broad catch persists provider/JVM failures as evidence.

Observer source, adapter, and test fixtures now have zero opaque-mapping
findings. The JPype smoke script has no opaque mapping, though Alphus still
reports its optional local `jpype` import. Ruff, formatting, observer Pyright,
and syntax checks pass. Direct Pyright on the JPype smoke script has one
environment-only diagnostic: the optional `jpype` module is not installed. No
test suites ran, and CI is unchanged.

The full profile `20261025T011141.818507Z` took 65.918 seconds and ended with
wrapper exit 1 on repository candidates: 4,339 Ruff findings (37 production,
3,836 tests/fixtures, 463 other, 3 adapters), 159 formatting candidates, 464
structural findings across 104 files including 40 opaque mappings, and 47
file-size findings. Compared with the last logged run
`20261025T005814.805958Z`, Ruff fell by eight, formatting by four, structure by
nine, opaque mappings by seven, and files with findings by two; file-size
findings stayed level. Step timings were 3.106 seconds for Python lint, 3.288
for formatting, 5.920 for Python structure, 4.328 for Python file size, 9.124
for C++ ownership/size, 9.926 for C++ style, 23.462 for Lizard, and 0.462 for
C++ statistics. The Windows mapped-file lock recurred during formatting of two
files and cleared after a three-second retry. The profile's Markdown steps
remain unavailable because `rumdl` and `mdrepo` are missing. Package-index DNS
blocked restoring the pinned rumdl package; a requirements reinstall also hit
a system-temporary-directory permission error. The adjacent mdrepo checkout
exists but has not been installed in this venv. The checked-in style-current
JSON remains stale; current summary and log were used.

Next is `packages/umbra-rti-api/tests/test_python_surface_report.py:40`, where
the package metadata loader returns `dict[str, object]`. Give TOML values and
the nested metadata fields explicit contracts while preserving the existing
distribution-boundary assertions.
## 2026-10-25 — precise Python report, metadata, and handle contracts

The Python surface-report test now models recursive TOML values, validates every nested metadata table, and checks string-list fields before use. The JPype 2025 JVM startup keyword mapping is restricted to bool | str | list[str]; its message-retraction cache stores a named Java-handle protocol with the exact encoding operations the runtime uses. The JPype 2010 overload matrix and native 2010 handle round-trip now use unions of supported public handle classes, with class-specific retrievals made explicit. The 2016e surface inventory and RTI shell generators now return named report/member shapes and validate recursive JSON input. The native shell generator keeps malformed inventory errors within its CLI error path.

Observer source, adapter, and test fixtures still have zero opaque-mapping findings. Focused Ruff and syntax checks pass for the touched files; the JPype 2010 provider test and both report generators pass formatting checks. Pyright reports zero diagnostics for the two standalone report generators. No test suites ran, CI is unchanged, and the Requirements Lab was not resynchronized. The two JPype 2025 source files retain existing whole-file format candidates; the 2010 test and report-generator formatting candidates were cleared.

The final local Alphus run 20261025T012814.198329Z took 64.313 seconds and ended with wrapper exit 1 on remaining report candidates: 4,326 Ruff findings (35 production, 3,829 tests/fixtures, 459 other, 3 adapters), 155 formatting candidates, 456 structural findings across 102 files including 32 opaque mappings, and 47 file-size findings. Compared with the last logged run 20261025T011141.818507Z, Ruff fell by 13, formatting by 4, structure by 8, opaque mappings by 8, and files with findings by 2; file-size findings stayed level. Step timings were 3.052 seconds for Python lint, 3.298 for formatting, 5.789 for Python structure, 3.836 for Python file size, 8.816 for C++ ownership/size, 9.498 for C++ style, 23.269 for Lizard, and 0.459 for C++ statistics. C++ style and statistics passed; source ownership/size and Lizard retain existing findings.

The Markdown profile steps remain unavailable because rumdl is not installed and mdrepo is not importable. Earlier restoration attempts hit temporary-directory permissions and package-index DNS failures; no new install attempt was made. The checked-in style-current JSON remains stale, so the current profile summary and log were used.

Next is tools/inspect_cpp_tck_link.py:33: replace the bare result mapping with named link-report and library-record contracts, preserving path containment, library counts, and hash reporting.
## 2026-10-25 — typed C++ link and Java 2010 evidence reports

The C++ TCK link inspector now returns named success and failure reports, including a typed library record for path, digest, reference count, and package-prefix membership. The Java 2010 TCK exporter now loads recursive JSON objects, validates scenario IDs, status values, metadata arrays, and result fields into frozen records, and returns a named evidence artifact. Original catalog scenario objects are preserved in the serialized export.

Focused Ruff, formatting, Pyright, syntax, and whitespace checks pass for both files; Pyright reports zero diagnostics. No test suites ran, CI is unchanged, and the Requirements Lab was not resynchronized. C++ code was not changed.

The full Alphus run 20261025T013611.473217Z took 66.223 seconds and ended with wrapper exit 1 on remaining repository candidates: 4,322 Ruff findings (35 production, 3,829 tests/fixtures, 455 other, 3 adapters), 154 formatting candidates, 455 structural findings across 101 files including 28 opaque mappings, and 47 file-size findings. Compared with the last logged profile 20261025T012814.198329Z, Ruff fell by 4, formatting by 1, structure by 1, opaque mappings by 4, and files with findings by 1; file-size findings stayed level. Step timings were 3.065 seconds for Python lint, 3.295 for formatting, 5.895 for Python structure, 3.952 for Python file size, 9.439 for C++ ownership/size, 9.783 for C++ style, 23.730 for Lizard, and 0.457 for C++ statistics. C++ style and statistics passed; ownership/size and Lizard candidates remain.

The Markdown profile steps remain unavailable because rumdl is not installed and mdrepo is not importable. Earlier restoration attempts hit temporary-directory permissions and package-index DNS failures; no new install attempt was made. The checked-in style-current JSON remains stale, so the current profile summary and log were used.

Next is tools/python_tck_2010.py:18: replace the exporter’s dict[str, Any] JSON inputs and output with validated Python 2010 catalog, result, evidence, and artifact contracts. Include tools/verify_1516e_python_tck_results.py where its JSON row types are still opaque; preserve the provider-neutral output fields and keep the 2010 route separate from the 2025 exporter.

## 2026-10-25 — explicit Alphus and focused-link report contracts

The Java 2010 exporter and result verifier now validate JSON into named row and
artifact contracts. The Alphus profile summarizer now has explicit Ruff,
structure, formatting, run-timing, and step-timing shapes; malformed Ruff
diagnostic filenames or rule codes fail with a field-specific error instead
of being stringified. The focused C++ TCK link helper now uses the existing
shared evidence loader and merger for child result files, removing its raw
result dictionaries. The focused-link helper also now passes the repository's
Ruff format check.

Two full profiles captured this work. Run
`20261025T014636.518106Z` took 57.063 seconds and reported 4,316 Ruff findings
(35 production, 3,829 tests/fixtures, 449 other, 3 adapters), 151 format
candidates, 449 structure findings across 97 files including 22 opaque
mappings, and 47 file-size findings. After the focused-link contract change,
run `20261025T014916.122774Z` took 56.279 seconds and reported 4,315 Ruff
findings (35 production, 3,829 tests/fixtures, 448 other, 3 adapters), 150
format candidates, 445 structure findings across 97 files including 20
opaque mappings, and 47 file-size findings. Compared with the first run in
this batch, the second reduced Ruff by one, formatting by one, structure by
four, and opaque mappings by two. The final step timings were 2.745 seconds
for Python lint, 2.964 for formatting, 4.986 for Python structure, 3.464 for
Python file size, 7.871 for C++ ownership/size, 9.366 for C++ style, 18.785
for Lizard, and 0.461 for C++ statistics. C++ style and statistics passed;
the other steps still report existing candidates.

Focused Ruff, formatting, Pyright, and syntax checks pass for the Alphus
profile helper and focused-link helper. Observer source, adapter, and tests
remain clear of opaque mappings. No test suites ran, CI was not changed, and
the Requirements Lab was not resynchronized. Markdown profile dependencies
remain unavailable (`rumdl` is missing and `mdrepo` is not importable); the
previous package-index DNS and temporary-directory permission failures were
not retried. The checked-in style-current JSON remains stale, so these counts
come from the generated summary and run log.

Next is `tools/run_java_tck.py:308-332`: replace the Java matrix's
`dict[str, Any]` entries and `Any`-returning value helpers with a named
provider-configuration contract and field-specific validators. Preserve its
string-or-non-empty-string-array inputs and its duplicate-ID check.

## 2026-10-25 — matrix, survey, and verifier contracts

The Java TCK matrix parser now converts each JSON entry into a frozen
`_ProviderConfiguration` with validated required strings, provider and
dependency JAR lists, optional paths, package prefix, boolean mode, JVM
arguments, and time implementation. Execution uses those named fields instead
of `Any` lookups and string coercion. Relative path helpers now resolve the
current working directory at call time rather than freezing it at import.

The C++ TCK regression helper now uses the runner's `ResolvedInputs` and
`CppTckScenario` contracts. Its fixture provides a complete resolved-input
shape and a namespace with `callback_model`; static checking exposed that the
old `object()` fixture could not support the runner's `vars()` and attribute
access. The regression executable itself was not run. The native-gap survey
now consumes the validated C++ catalog, names its scenario-match alias and
report records, and correctly types semantic matches as lists of scenario
IDs. The 2010 capability-profile verifier now returns a named report shape,
removes its unused mutable mode set, and raises `TypeError` for a catalog row
without a string ID.

The read-only native-gap survey reported 1,530 catalog scenarios, 956 native
Catch2 files, 9 semantic matches, 879 unmatched native files, and zero
portable candidates. It took about 52.7 seconds; a separate measurement put
full catalog validation at 1.164 seconds, so include-closure scanning accounts
for most of the survey time.

The full Alphus run `20261025T020226.044034Z` took 56.743 seconds and ended
with existing report candidates: 4,308 Ruff findings (35 production, 3,829
tests/fixtures, 441 other, 3 adapters), 146 formatting candidates, 430
structure findings across 96 files including 12 opaque mappings, and 47
file-size findings. Compared with `20261025T015415.949914Z`, Ruff fell by 3,
formatting by 3, structure by 10, opaque mappings by 5, and files with
findings by one; file-size findings stayed level. Step timings were 2.779
seconds for Python lint, 2.935 for formatting, 4.983 for Python structure,
3.476 for Python file size, 7.990 for C++ ownership/size, 9.365 for C++ style,
18.674 for Lizard, and 0.467 for C++ statistics. C++ style and statistics
passed; the other steps retain candidates.

Focused Ruff, formatting, Pyright, and syntax checks pass for the changed
tools. Observer source, adapter, and tests still have zero opaque mappings.
No test suites ran, CI is unchanged, and the Requirements Lab was not
resynchronized. Markdown checks remain unavailable because `rumdl` is missing
and `mdrepo` is not importable; prior DNS and temporary-directory permission
failures were not retried.

Next is `tools/verify_1516e_provider_boundaries.py:60`: replace the manifest
loader's `dict[str, Any]` result with a recursive JSON boundary and named
entry-point manifest contract, then validate the consumed native and JPype
factory groups before comparing package ownership.

## 2026-10-25 — close the remaining opaque JSON mappings

The 2010 JNI type-roundtrip verifier now loads a checked JSON object, validates
carrier category/probe/status fields into a named row, and validates expected
string arrays before set comparisons. A malformed array now gets a field-level
error instead of failing later on an unhashable value. The Catch2 tag checker
normalizes its listing into named test rows and checks names, tags, and source
locations. The CTest service-lane checker normalizes test names and labels and
validates the nested LABELS property shape. The external 2010 FOM checker now
loads resource and Target Radar records into named contracts and checks schema,
path, digest, namespace, and schema-location fields before verification.

Full Alphus profile `20261025T022151.493113Z` took 56.703 seconds and returned
exit 1 on remaining repository candidates. Ruff reports 4,299 findings (35
production, 3,829 tests/fixtures, 432 other, and 3 adapters); 138 files need
formatting; Python structure reports 416 findings across 92 files, with zero
opaque-mapping findings; and file-size reporting remains at 47. Compared with
`20261025T020226.044034Z`, Ruff fell by 9, formatting by 8, structure findings
by 14, structure files by 4, and opaque mappings by 12; file-size findings did
not change. Step timings were 2.783 seconds for Python lint, 2.963 for
formatting, 5.018 for Python structure, 3.473 for Python file size, 8.159 for
C++ source ownership/size, 9.549 for C++ style, 18.697 for Lizard, and 0.466
for C++ statistics. C++ style and statistics passed; the other report steps
retain candidates. Markdown checks still cannot start because `rumdl` is
missing and `mdrepo` is not importable.

Focused Ruff, formatting, Pyright, and syntax checks pass for all four changed
verifiers. Observer source, adapter, and tests continue to have zero opaque
mappings. No test suites ran, CI is unchanged, and the Requirements Lab was not
resynchronized. The setup complaint remains the missing Markdown tools; earlier
package restoration attempts hit temporary-directory permissions and package
index DNS failures, and were not retried here.

Next is `tools/verify_1516e_type_roundtrip.py:50`: replace its mutable
module-level expectation sets and nested category mapping with immutable named
contracts, then take the next Alphus measurement. Keep observer coverage, CI,
Requirements Lab revisions, and test-suite policy unchanged.

## 2026-10-25 — freeze round-trip expectations

The type-roundtrip verifier's expected carrier and requirement-reference sets
are now `frozenset` values, and its nested reference and category maps use
read-only mapping proxies. This keeps the verifier's own expected contract from
being changed while it checks the catalog.

Full Alphus profile `20261025T022532.301912Z` took 56.678 seconds and returned
exit 1 on remaining repository candidates. It reports 4,299 Ruff findings (35
production, 3,829 tests/fixtures, 432 other, and 3 adapters), 138 formatting
candidates, 398 structure findings across 91 files with zero opaque mappings,
and 47 file-size findings. Compared with `20261025T022151.493113Z`, structure
findings fell by 18 and files with findings fell by one; Ruff, formatting, and
file-size counts stayed level. Mutable-module-constant findings fell from 60
to 42. Step timings were 2.772 seconds for Python lint, 2.928 for formatting,
4.901 for Python structure, 3.446 for Python file size, 8.123 for C++ source
ownership/size, 9.540 for C++ style, 18.796 for Lizard, and 0.457 for C++
statistics. C++ style and statistics passed; remaining report steps retain
candidates.

Focused Ruff, formatting, Pyright, and syntax checks pass for the verifier. No
test suites ran, CI and Requirements Lab revisions are unchanged, and the
Markdown setup issue remains (`rumdl` missing; `mdrepo` unavailable). Observer
source, adapter, and tests still have zero opaque mappings.

Next is `packages/umbra-rti-observer/src/umbra_rti_observer/workspace_models.py:160`:
review the remaining Pydantic field declarations flagged by Alphus and replace
legacy field constraints with explicit collection and value contracts. Keep
behavior covered by the existing observer tests, but do not run test suites in
this cleanup turn.


## 2026-10-25 — make observer reporter metadata explicit

Observer event input and envelope fields now share the named
`ReportedParticipant = str | Mapping[str, JsonValue] | None` contract. This is
reporter display metadata; identity remains in its dedicated attribution
contract. Analytics converts a read-only mapping to a JSON object only at its
serialization boundary. The portable wire schema remains current. Ruff reports
no Python collection-contract or opaque-mapping findings, including in observer
source, adapter, or tests.

The 2010 JPype and native time wrappers each had four unsorted `__slots__`
entries and one unsorted export list. Their ten ordering findings are fixed;
the changed wrappers are formatted. Observer Pyright reports zero diagnostics,
and focused Ruff, syntax compilation, and schema consistency checks pass. No
test suites ran.

Full Alphus profile `20261025T023239.248739Z` took 61.365 seconds and returned
exit 1 on remaining report candidates. Ruff reports 4,289 findings (25
production, 3,829 tests/fixtures, 432 other, and 3 adapters), 135 files need
formatting, Python structure reports 394 findings across 91 files with zero
opaque mappings, and file-size reporting remains at 47. Compared with
`20261025T022943.748348Z`, Ruff fell by 10 and format candidates by 2; structure
and file-size counts stayed level. Step timings were 3.112 seconds for Python
lint, 3.169 for formatting, 5.518 for Python structure, 3.919 for Python file
size, 8.815 for C++ ownership/size, 10.103 for C++ style, 20.023 for Lizard,
and 0.520 for C++ statistics. C++ style and statistics passed. Markdown checks
still cannot start: `rumdl` is missing and `mdrepo` is not installed in the
local environment.

Alphus still reports 26 `legacy-pydantic-fields` findings (23 in
`workspace_models.py`, two in `alert_models.py`, and one in `conformance.py`).
Inspection confirms these are valid Pydantic 2 `Field(default_factory=...)`
declarations; the checker flags assignment-form `Field(...)` without examining
its arguments. This is AC-008's confirmed false positive; do not reshape valid
models to satisfy it.

CI enforcement and Requirements Lab revisions are unchanged. The next review is
`packages/umbra-rti-api/src/hla/rti1516e/core.py:98-135`: make optional time
provider discovery's skip/error contract explicit and observable, and replace
the dynamic export list if the public names permit it. Preserve the expected
behavior for unavailable optional providers and check the existing contract
before changing exception scope.


## 2026-10-25 — clarify provider and integration cleanup contracts

The 2010 logical-time provider discovery now types installed entry points as
`EntryPoints`, names the two allowed time-factory accessors, and logs optional
provider failures at debug level. Each accessor is isolated so one failure does
not hide the other. Its public export list is literal and preserves the same 34
unique names. The Java-backed 2010 encoder also records a failed Java class
probe before using its class-name fallback. Fourteen `RUF022` export-order
findings were fixed, and four dynamic-export warnings now point back to their
canonical string inventories (AC-009).

The opt-in JNI integration module had 3,774 Ruff findings, 1,864 of which came
from best-effort cleanup blocks that discarded every exception. All 1,864 were
inside teardown paths. They now use one named context manager that retains the
non-masking cleanup behavior and records failures at debug level. Four
re-entrant membership catches now record the exact
`CallNotAllowedFromWithinCallback` contract; two save requests catch
`RTIexception`. The module fell to 32 Ruff findings and is formatted. The
remaining findings are 24 unused locals, five nested-context suggestions, two
`dict()` suggestions, and one nested-if suggestion. At 80,235 lines, it remains
a strong candidate for a future split along behavior families; no large module
reorganization was attempted in this focused lint batch.

The 2010 JPype smoke driver had 213 Ruff findings. Its 101 best-effort cleanup
blocks now use a logging context manager. Startup and scenario failure boundaries
still write explicit failed evidence records. The three `TRY004` assertions are
retained as `AssertionError` and narrowly documented (AC-010); one dictionary
presence check now uses `.get`, and unused imports were removed. The file passes
Ruff, formatting, and syntax checks.

Full profile `20261025T025053.466856Z` took 55.960 seconds and returned exit 1
on remaining repository candidates: Ruff 522 (87 tests/fixtures, 432 other,
3 adapters, and zero production findings), formatting 129 files, structure 394
across 91 files with zero opaque mappings, and 47 file-size findings. Compared
with `20261025T024209.805730Z`, Ruff fell by 3,742 and formatting by one. Step
timings were Python lint 2.725s, formatting 2.872s, structure 4.865s, file size
3.718s, C++ ownership/size 8.147s, C++ style 9.484s, Lizard 21.115s, and C++
statistics 0.514s.

Full profile `20261025T025316.627265Z` took 56.453 seconds and returned exit 1
on remaining report candidates: Ruff 309 (87 tests/fixtures, 219 other,
3 adapters, and zero production findings), formatting 128 files, structure 394
across 91 files with zero opaque mappings, and 47 file-size findings. Compared
with `20261025T025053.466856Z`, Ruff fell by 213 and format candidates by one;
structure and file-size counts stayed level. Step timings were Python lint
2.767s, formatting 2.802s, structure 4.679s, file size 3.462s, C++
ownership/size 7.970s, C++ style 9.889s, Lizard 18.856s, and C++ statistics
0.464s. The 2010 JPype smoke driver is now at zero Ruff findings. Markdown
checks still cannot start (`rumdl` missing; `mdrepo` not installed). The 26
Pydantic 2 `Field(default_factory=...)` false positives remain (AC-008).

The Umbra observer adapter's capability manifest now uses named `TypedDict`
contracts for its ten capability families, state values, and normalized event
names. Provider callback values are annotated as `object` until conversion at
the JSON boundary; the RTI ambassador and callback model are typed, and the
optional connect keys have a dedicated `TypedDict`. This removes the adapter's
`Any` annotations and its opaque-boundary suppression without changing runtime
event shapes. Ruff, format, syntax compilation, and strict adapter Pyright pass
(zero diagnostics). Full profile `20261025T025803.306767Z` took 57.041s and
reports Ruff 309 (87 tests/fixtures, 219 other, 3 adapters, zero production),
formatting 128 files, structure 394 across 91 files with zero opaque-mapping
findings, and file-size 47. Timings: Python lint 2.773s, formatting 2.687s,
structure 4.714s, file size 3.451s, C++ ownership/size 8.109s, C++ style
9.543s, Lizard 19.005s, and C++ statistics 0.474s. Markdown setup remains
blocked by missing `rumdl` and `mdrepo`.

No test suites ran; CI enforcement and Requirements Lab revisions are
unchanged. Next define the accepted raw observer-event and nested observation
fields at the `core.py`/`attribution.py` boundary, including the two legal
session-observation field names, while preserving provider extensions. Then
review the 24 remaining `F841` findings in
`packages/umbra-rti-jpype/tests/test_jpype_jni_integration.py`, starting at the
unused `lost_joined` state around line 6788, and split that module by cohesive
behavior groups while preserving discovery and resource paths. Keep observer
source and adapter in the full profile.

The observer's analytics and alerting layers previously reconstructed anonymous
event dictionaries from typed Pydantic/domain objects before passing them to
mapping-based participant helpers. Alphus reported zero opaque mappings, so
that count did not reveal this contract gap. `ParticipantEventMetadata` now
names the fields shared by these consumers; normalized event records keep their
existing public constructor and JSON shape, and provider extensions remain
available at the raw ingress boundary. Observer core, analytics, alerting,
declaration, object, and time projections consume the named view. The strict
observer and adapter Pyright projects report zero diagnostics; Ruff, format,
syntax compilation, and scoped diff checks pass. No test suites ran.

Full profile `20261025T032221.152082Z` took 61.339s and returned exit 1 on
existing report candidates: Ruff 282 (60 tests/fixtures, 219 other, 3 adapters,
zero production), formatting 121 files, structure 394 across 91 files with
zero opaque-mapping findings, and file-size 47. Compared with
`20261025T025803.306767Z`, Ruff fell by 27 and formatting candidates by 7;
structure and file-size counts were unchanged. Timings: Python lint 3.036s,
formatting 3.123s, structure 5.220s, file size 3.940s, C++ ownership/size
9.298s, C++ style 10.342s, Lizard 19.601s, and C++ statistics 0.505s.
Markdown checks remain unavailable (`rumdl` missing; `mdrepo` not installed).
No test suites ran; CI and Requirements Lab revisions remain unchanged. The
follow-up below completes direct typed-domain conversions for participant
identity and attribution, preserving Pydantic's required and extra-field rules.

Participant identity and attribution schema validators now convert their
already validated Pydantic fields directly into domain values. Event and
envelope session agreement is checked from named session fields rather than a
temporary dictionary. The observer and adapter Pyright projects report zero
diagnostics; Ruff, format, syntax compilation, and scoped whitespace checks
pass. No test suites ran.

Full profile `20261025T032836.449490Z` took 55.833s and returned exit 1 on
existing report candidates: Ruff 282 (60 tests/fixtures, 219 other, 3 adapters,
zero production), formatting 121 files, structure 394 across 91 files with
zero opaque-mapping findings, and file-size 47. Counts matched
`20261025T032221.152082Z`. Timings: Python lint 2.731s, formatting 2.684s,
structure 4.636s, file size 3.717s, C++ ownership/size 7.974s, C++ style
9.330s, Lizard 18.728s, and C++ statistics 0.476s. Markdown checks remain
unavailable (`rumdl` missing; `mdrepo` not installed). No test suites ran; CI
and Requirements Lab revisions are unchanged. Next, replace the remaining
interaction-flow dictionary bridges in the Pydantic event validators with a
typed conversion, preserving explicit direction and event-type agreement.

The interaction-flow schema now converts its typed Pydantic fields directly to
`InteractionFlowData`; direction agreement uses a named validator. Raw provider
payloads have `RawInteractionFlowObservation`, and the normalized parser shares
the same typed construction path. Direction alias handling and validation order
are preserved. The observer and adapter Pyright projects report zero
diagnostics; Ruff, format, syntax compilation, and scoped whitespace checks
pass. No test suites ran.

Full profile `20261025T033305.171302Z` took 55.739s and returned exit 1 on
existing candidates: Ruff 282 (60 tests/fixtures, 219 other, 3 adapters, zero
production), formatting 120 files, structure 394 across 91 files with zero
opaque-mapping findings, and file-size 47. Compared with
`20261025T032836.449490Z`, formatting candidates fell by one; other counts
were unchanged. Timings: Python lint 2.711s, formatting 2.702s, structure
4.635s, file size 3.438s, C++ ownership/size 7.856s, C++ style 9.592s,
Lizard 18.806s, and C++ statistics 0.458s. Markdown tools remain unavailable
(`rumdl` missing; `mdrepo` not installed). No test suites ran; CI and
Requirements Lab revisions remain unchanged. Next, replace the anonymous
relationship dictionary bridge in `RelationshipObservationInput` with a direct
typed-domain conversion while retaining its identity and scope validation.

## 2026-10-25 — relationship and coordination schema contracts

`RelationshipObservationInput` and `CoordinationObservationBase` no longer
serialize typed Pydantic fields into anonymous dictionaries and send them back
through mapping parsers. Both expose `to_domain()` conversions backed by named
domain constructors; the raw event parsers remain the untrusted-input boundary.
This continues AC-011: the profile's zero opaque-mapping findings did not
surface these model-to-dictionary bridges, so the metric alone could not
describe the confusing contract.

The observer and adapter Pyright projects report zero diagnostics. Scoped Ruff,
the changed schema formatter check, and syntax compilation pass. The full local
profile `20261025T034227.476979Z` took 67.338 seconds and returned exit 1 on
existing candidates: Ruff 282 (60 tests/fixtures, 219 other, 3 adapters, zero
production), formatting 120, structure 394 across 91 files with zero
opaque-mapping findings, and file-size 47. Timings: Python lint 3.001s,
formatting 3.195s, structure 5.922s, file size 4.054s, C++ ownership/size
9.080s, C++ style 9.788s, Lizard 25.676s, and C++ statistics 0.495s. C++ style
and statistics passed. Markdown checks remain unavailable (`rumdl` missing;
`mdrepo` not installed). No test suites ran; CI and Requirements Lab revisions
remain unchanged. Next, replace the ownership observation schema's anonymous
mapping bridge with a direct typed-domain conversion.

## 2026-10-25 — ownership schema contracts and format cleanup

`OwnershipObservationBase` now dispatches its named state, request, and outcome
variants directly to typed domain factories. Scope, state consistency, request
action, and outcome validation remain explicit; the raw parser still accepts
provider mappings at ingress and shares the field factories. Formatting the
ownership module removed one whole-file format candidate.

Observer and adapter Pyright each report zero diagnostics. Ruff, formatting for
the changed schema and ownership sources, syntax compilation, and whitespace
checks pass. The full local profile `20261025T034838.639218Z` took 67.141
seconds and returned exit 1 on existing candidates: Ruff 282 (60
tests/fixtures, 219 other, 3 adapters, zero production), formatting 119,
structure 394 across 91 files with zero opaque-mapping findings, and file-size
47. Formatting fell by one from the previous profile; Ruff, structure, and
file-size counts were unchanged.
Timings: Python lint 3.432s, formatting 3.242s, structure 5.941s, file size
3.932s, C++ ownership/size 9.622s, C++ style 9.643s, Lizard 23.627s, and C++
statistics 0.473s. C++ style and statistics passed. Markdown tools remain
unavailable (`rumdl` missing; `mdrepo` not installed). No tests ran; CI and
Requirements Lab revisions remain unchanged. Next, replace the anonymous
region observation bridge in `RegionObservationBase` with a direct typed-domain
conversion.

## 2026-10-25 — region schemas and explicit object contexts

Region request and response models now convert their validated fields directly
to named region domain values. The region schemas and shared Pydantic/JSON types
were extracted from `schemas.py`; the module is now 2,364 lines. `ObjectContextInput`
also validates through an explicit field constructor, and the raw event parser
uses that same constructor after enforcing its exact wire-field set. Provider
mappings remain at raw ingress.

The observer and adapter Pyright projects report zero diagnostics. Scoped Ruff,
formatting, and syntax compilation pass. Full local Alphus run
`20261025T040214.030175Z` took 67.269 seconds and returned exit 1 on existing
repository candidates: Ruff 282 (60 tests/fixtures, 219 other, 3 adapters,
zero production), formatting 117 files, structure 393 across 90 files with
zero opaque-mapping findings, and file-size 47. Compared with the previous
profile, formatting fell by one file, structure findings by one, and file-size
findings by one. Timings: Python lint 3.325s, formatting 3.399s, structure
6.149s, file size 4.217s, C++ ownership/size 9.302s, C++ style 9.749s,
Lizard 23.681s, and C++ statistics 0.466s. C++ style and statistics passed.
Markdown tools remain unavailable (`rumdl` missing; `mdrepo` not installed).
No tests ran; CI and Requirements Lab revisions are unchanged. Next, replace
`ObjectDeclarationObservation`'s model-to-dictionary validation bridge with a
direct declaration constructor that preserves selected-attribute rules.

## 2026-10-25 — declaration schema contracts

`ObjectDeclarationObservation.to_domain()` now passes named fields to
`declaration_from_fields()` rather than serializing a Pydantic model and sending
the anonymous mapping back through the raw parser. `declaration_data()` keeps
the exact provider-field allowlist at ingress and delegates shared domain
validation to the same constructor. Kind/direction and status checks still run
before unknown-field rejection; selected attributes still require an explicit
array, allow an empty array, cap at 4,096 items, and reject duplicates.

The observer and adapter Pyright projects report zero diagnostics. Scoped Ruff,
formatting, and syntax compilation pass. Full local Alphus run
`20261025T040640.156696Z` took 68.328 seconds and returned exit 1 on existing
repository candidates: Ruff 282 (60 tests/fixtures, 219 other, 3 adapters,
zero production), formatting 117 files, structure 393 across 90 files with
zero opaque-mapping findings, and file-size 47. Counts match the prior run.
Timings: Python lint 3.348s, formatting 3.341s, structure 6.110s, file size
4.198s, C++ ownership/size 9.710s, C++ style 10.001s, Lizard 24.307s, and
C++ statistics 0.494s. C++ style and statistics passed. Markdown tools remain
unavailable (`rumdl` missing; `mdrepo` not installed). No tests ran; CI and
Requirements Lab revisions are unchanged. Next, replace
`ParticipantReferenceResponse`'s anonymous model-to-dictionary validator with
a direct field conversion while preserving reference-evidence validation.

## 2026-10-25 — participant-reference schema contract

`ParticipantReferenceResponse.to_domain()` now passes its optional identity,
display name, and handle to `participant_reference_from_fields()` directly.
`ParticipantIdentityResponse` supplies its domain identity without a mapping
round trip. The raw participant-reference parser still rejects unknown fields
at ingress, then delegates evidence validation to the same field constructor.

The observer and adapter Pyright projects report zero diagnostics. Scoped Ruff,
formatting, and syntax compilation pass. Full local Alphus run
`20261025T040946.302964Z` took 65.102 seconds and returned exit 1 on existing
repository candidates: Ruff 282 (60 tests/fixtures, 219 other, 3 adapters,
zero production), formatting 117 files, structure 393 across 90 files with
zero opaque-mapping findings, and file-size 47. Counts match the preceding
profile. Timings: Python lint 3.119s, formatting 3.138s, structure 5.825s,
file size 3.880s, C++ ownership/size 9.018s, C++ style 9.613s, Lizard 23.609s,
and C++ statistics 0.469s. C++ style and statistics passed. Markdown tools
remain unavailable (`rumdl` missing; `mdrepo` not installed). No tests ran; CI
and Requirements Lab revisions are unchanged. Next, replace
`MomObservationBase`'s model-to-dictionary validation bridge with a named-field
conversion while retaining its event-kind and required-field rules.

## 2026-10-25 — management observation schema contracts

The management-report variants now expose direct `to_domain()` conversions for
member rosters, service observations, and counters. Named factories validate
the shared report context and each variant's domain invariants. `mom_data()`
remains the raw mapping boundary and retains its per-kind allowed-field sets,
required coverage/status/semantics checks, complete-roster requirement,
identified-member uniqueness, and exclusive numeric-or-text counter value.

The observer and adapter Pyright projects report zero diagnostics. Scoped Ruff,
formatting, and syntax compilation pass. Full local Alphus run
`20261025T041939.823425Z` took 89.023 seconds and returned exit 1 on existing
repository candidates: Ruff 279 (60 tests/fixtures, 219 other, zero adapters,
zero production), formatting 114 files, structure 393 across 90 files with
zero opaque-mapping findings, and file-size 47. Formatting fell by one from
the preceding profile; other candidate counts are unchanged. Timings: Python
lint 4.084s, formatting 4.252s, structure 7.679s, file size 5.434s, C++
ownership/size 11.679s, C++ style 12.445s, Lizard 34.747s, and C++ statistics
0.553s. C++ style and statistics passed. Total time varied from 69.626 seconds
in the preceding run; Lizard varied from 24.093s to 34.747s. Markdown tools
remain unavailable (`rumdl` missing; `mdrepo` not installed). No tests ran; CI
and Requirements Lab revisions are unchanged. Next, replace the
`FomObservationBase` model-to-dictionary bridge with direct field conversion
while preserving FOM metadata scope and variant-specific required fields.

## 2026-10-25 — adapter Ruff and format cleanup

Ruff's three reported findings in the Java TCK connection-loss runner were
safe fixes: two import-block cleanups and removal of an unused `E402`
suppression. The import remains after the `sys.path` setup. Ruff also formatted
that runner and the Umbra observer adapter's `commands.py`, both of which were
listed in the full-profile format candidates.

The adapter command module's Ruff and formatter checks pass, and its Pyright
project reports zero diagnostics. Full local Alphus run
`20261025T041442.460370Z` took 69.626 seconds and returned exit 1 on remaining
repository candidates: Ruff 279 (60 tests/fixtures, 219 other, zero adapters,
zero production), formatting 115 files, structure 393 across 90 files with
zero opaque-mapping findings, and file-size 47. Compared with profile
`20261025T040946.302964Z`, Ruff fell by three and formatting by two files.
Timings: Python lint 3.297s, formatting 3.383s, structure 6.326s, file size
4.377s, C++ ownership/size 10.043s, C++ style 9.935s, Lizard 24.093s, and
C++ statistics 0.480s. C++ style and statistics passed. Total time varied
from 93.368 seconds in the preceding run; Lizard varied from 36.869s to
24.093s. Markdown tools remain unavailable (`rumdl` missing; `mdrepo` not
installed). No tests ran; CI and Requirements Lab revisions are unchanged.

## 2026-10-25 — FOM schema contracts and profile

FOM object, interaction, datatype, and dimension request variants now convert
directly to domain values. Named factories cover shared metadata, fields,
enumerators, class scope, dimensions, and datatypes. `fom_data()` remains the
raw wire parser and retains its variant-specific allowlists and required-field
checks. This closes another AC-011 bridge that the opaque-mapping metric did
not identify. The HTTP/core conversion is now named and typed as `RawObserverEvent`; its
generic `model_dump()` remains, and the broader core raw-map contract is the
next focused cleanup.

Focused Ruff and formatting checks pass; syntax compilation passes, and both
observer and adapter Pyright projects report zero diagnostics. Full local
Alphus run `20261025T042421.980688Z` took 68.610 seconds and returned exit 1
on existing candidates: Ruff 279 (60 tests/fixtures, 219 other, zero adapters,
zero production), formatting 113, structure 393 across 90 files with zero
opaque-mapping findings, and file-size 47. Timings: Python lint 3.501s,
formatting 3.412s, structure 6.246s, file size 4.285s, C++ ownership/size
9.669s, C++ style 9.928s, Lizard 24.396s, and C++ statistics 0.495s. C++
style and statistics passed. Markdown tools remain unavailable (`rumdl` missing;
`mdrepo` not installed). No tests ran; CI and Requirements Lab revisions are
unchanged. Total time varied from 89.023 seconds in the prior run; Lizard
varied from 34.747 to 24.396 seconds.

## 2026-10-25 — explicit HTTP event ingress type and fresh profile

The HTTP serializer is now named `event_input_to_raw_event()` and returns the
`RawObserverEvent` ingress type. `_event_payload()` carries that type instead
of presenting request data as a `JsonObject`. The remaining core contract still
uses a broad raw mapping and generic model serialization; the next step is to
separate common event metadata, known family payloads, and provider extensions
within that ingress contract.

Ruff, formatting, syntax compilation, and both observer Pyright projects pass.
Full Alphus run `20261025T042906.112366Z` took 69.888 seconds and exited 1 on
existing candidates. It reported Ruff 279 (60 tests/fixtures, 219 other, zero
adapters, zero production), formatting 112 files, structure 393 across 90
files with zero opaque-mapping findings, and file-size 47 across 24 files.
Timings: Python lint 3.481s, formatting 3.462s, structure 6.358s, file size
4.277s, C++ ownership/size 10.004s, C++ style 10.053s, Lizard 24.909s, and
C++ statistics 0.508s. C++ style and statistics passed. The default Python 3.10
environment could not launch Alphus because the adjacent launcher was missing;
the dedicated `out/alphus-env/Scripts/python.exe` environment completed the
profile. No tests ran; CI and Requirements Lab revisions are unchanged.

## 2026-10-25 — retain the typed FOM projection

`_normalise_event()` now stores the already-validated `FomData` on `EventRecord`;
`FomViews` consumes that value directly instead of reparsing the raw extension
map. Raw provider fields remain available for event serialization, and the
FOM parser retains its variant allowlists. That run temporarily added one `TYPE_CHECKING` finding; the shared structural
protocol introduced afterward removed the extra. The latest structure count is
back to 393, with two existing type-only import findings recorded under AC-012.

Full local Alphus run `20261025T043546.886575Z` took 76.060 seconds and exited
1 on existing candidates: Ruff 279 (60 tests/fixtures, 219 other, zero adapters,
zero production), formatting 112, structure 394 across 90 files, and file-size
47 across 24 files. Timings: Python lint 3.413s, formatting 3.494s, structure
6.574s, file size 4.269s, C++ ownership/size 13.810s, C++ style 10.870s,
Lizard 26.063s, and C++ statistics 0.517s. C++ style and statistics passed.
The default Python environment still lacks the Alphus launcher; the dedicated
environment completed the run. Ruff, formatting, syntax compilation, and both
observer Pyright projects pass. No tests ran; CI and Requirements Lab revisions
are unchanged. The MOM projection is now implemented.

## 2026-10-25 — typed FOM and MOM event projections

`_normalise_event()` now retains both parsed FOM and MOM domain records on
`EventRecord`. `FomViews` and `MomViews` consume those typed projections, so the
views do not parse the raw extension map a second time. Read-only structural
protocols specify the report context, kind, and JSON conversion at the shared
model boundary; no additional type-only imports were needed.

Full local Alphus run `20261025T044022.314092Z` took 70.121 seconds and exited
1 on existing candidates: Ruff 279 (60 tests/fixtures, 219 other, zero adapters,
zero production), formatting 112, structure 393 across 90 files with zero
opaque-mapping findings, and file-size 47 across 24 files. Two required
`TYPE_CHECKING` imports remain flagged in `models.py`. Timings: Python lint
3.772s, formatting 3.523s, structure 6.317s, file size 4.428s, C++
ownership/size 9.958s, C++ style 10.016s, Lizard 24.490s, and C++ statistics
0.471s. C++ style and statistics passed. The dedicated Alphus environment
completed the run; the default interpreter still lacks its launcher. Focused
Ruff, formatting, syntax compilation, and both observer Pyright projects pass.
No tests ran; CI and Requirements Lab revisions are unchanged. The declaration
projection is now implemented; relationship observations are the next family.

## 2026-10-25 — typed declaration and relationship projections

Normalization now stores parsed `DeclarationData` and `RelationshipData` on
`EventRecord`. `DeclarationViews` and `RelationshipViews` consume those values
directly, preserving the raw parser's named-field, uniqueness, scope, and
applicability checks while avoiding a second parse of the extension map.
Read-only structural contracts describe the fields each view uses.

Full local Alphus run `20261025T044454.363999Z` took 66.320 seconds and exited
1 on existing candidates: Ruff 279 (60 tests/fixtures, 219 other, zero adapters,
zero production), formatting 112, structure 393 across 90 files with zero
opaque-mapping findings, and file-size 47 across 24 files. Timings: Python lint
3.124s, formatting 3.052s, structure 5.737s, file size 3.888s, C++
ownership/size 9.244s, C++ style 9.788s, Lizard 24.333s, and C++ statistics
0.503s. C++ style and statistics passed. Focused Ruff, formatting, syntax
compilation, and both observer Pyright projects pass. The dedicated Alphus
environment completed the profile; the default interpreter still lacks the
launcher. No tests ran; CI and Requirements Lab revisions are unchanged. The coordination
projection is now implemented; the next family is interaction flow.

## 2026-10-25 — typed coordination projection and source format batch

`EventRecord` now retains the validated coordination domain value, and
`CoordinationViews` uses it directly. A read-only projection contract captures
the operation, scope, subject, and report fields used by the view. Ruff formatted
30 previously unformatted observer/adapter source files; all 55 files in those
two source packages now pass Ruff, format, syntax, and Pyright checks.

The full local Alphus run `20261025T044845.324947Z` took 90.618 seconds and
exited 1 on existing candidates: Ruff 279 (60 tests/fixtures, 219 other, zero
adapters, zero production), formatting 81, structure 393 across 90 files with
zero opaque-mapping findings, and file-size 47 across 24 files. Timings: Python
lint 4.718s, formatting 4.631s, structure 8.685s, file size 5.530s, C++
ownership/size 12.293s, C++ style 11.958s, Lizard 33.736s, and C++ statistics
0.561s. C++ style and statistics passed. The 23 observer findings in
`workspace_models.py` remain the known Pydantic 2 `Field(...)` false positives
logged under AC-008. No tests ran; CI and Requirements Lab revisions are
unchanged. The default interpreter lacks the Alphus launcher; the dedicated
environment completed the profile. Next, retain `InteractionFlowData` on
`EventRecord` and have `InteractionFlowViews` consume it while preserving the
per-kind validation and direction rules.

## 2026-10-25 — typed interaction-flow projection

Normalization stores the parsed interaction-flow record on `EventRecord`, and
`InteractionFlowViews` consumes it without reparsing the raw extension mapping.
The legacy behavior remains explicit: exact send/receive event types without a
flow payload create uncorrelated records and do not infer endpoints or IDs.

Full local Alphus run `20261025T045115.931568Z` took 90.716 seconds and exited
1 on existing candidates: Ruff 279 (60 tests/fixtures, 219 other, zero adapters,
zero production), formatting 81, structure 393 across 90 files with zero
opaque-mapping findings, and file-size 47 across 24 files. Timings: Python lint
4.696s, formatting 4.760s, structure 9.245s, file size 6.418s, C++
ownership/size 13.517s, C++ style 11.665s, Lizard 31.049s, and C++ statistics
0.513s. C++ style and statistics passed. Focused Ruff, formatting, syntax
compilation, and both observer Pyright projects pass. No tests ran; CI and
Requirements Lab revisions are unchanged. The dedicated environment completed
the profile. Next, retain `OwnershipData` on `EventRecord` and have
`OwnershipViews` consume it while preserving request/state variants and scope
rules.

## 2026-10-25 — typed ownership/region projections and observer cleanup

Normalization now retains validated ownership and region observations on
`EventRecord`; both views consume the cached variants. Ownership state,
request, and outcome tags remain explicit, and region keys retain their
scoped context. The observer probe now accepts sequence interfaces, its
immutable result records are slotted, and the observer CLI/object view accept
read-only `Sequence`/`Iterable` inputs. The strict conformance import remains
lazy because that dependency is optional.

The complete Alphus run `20261025T050357.142134Z` took 108.583 seconds. The
Markdown link/graph check passed; style reported 25 existing findings in the
complaint ledger and Requirements Lab open-issues document. Python results:
Ruff 279 (60 tests/fixtures, 219 other, zero production; 53 auto-fixable),
formatting 81, structure 382 across 90 files with zero opaque-mapping findings,
and file-size 47 across 24 files. The
observer/adapter source directories contain 55 files and pass Ruff, format,
syntax compilation, and their configured Pyright projects. C++ style and
statistics passed; source ownership/size and Lizard still report candidates.
Step timings were 4.286s for Python lint, 4.194s for formatting, 7.532s for
structure, 5.281s for file size, 11.531s for C++ ownership/size, 11.402s for
C++ style, 31.815s for Lizard, and 0.473s for C++ statistics.

The nine read-only collection annotations in the query-work source-location
and requirement-gap helpers now use `Sequence`/`Mapping`. The following full
run, `20261025T050718.322550Z`, took 96.996 seconds: Ruff remained 279, format
remained 81, file-size remained 47, and structure fell from 382 to 373 across
89 files. Concrete-input candidates fell from 186 to 177; the `query_rti_work.py`
path fell from 73 to 64 findings. Opaque mappings remain at zero. The updated
query script passes Ruff and syntax compilation; its existing whole-file
format candidate was left untouched to keep this change focused.
Step timings were 3.083s for Python lint, 3.096s for formatting, 6.328s for
structure, 4.137s for file size, 10.080s for C++ ownership/size, 10.104s for
C++ style, 24.451s for Lizard, and 0.475s for C++ statistics. The Markdown
link/graph check passed; style reported 25 existing findings across two files.

### AC-013 — stale local profile environments can skip Markdown checks

The existing `out/alphus-env` lacked the pinned `rumdl` executable and the
adjacent `mdrepo` checkout. The first profile therefore failed both Markdown
steps even though Python and C++ reports completed. Adding the venv's Scripts
directory to `PATH`, installing the pinned Markdown checker, and exposing the
read-only local mdrepo source restored both steps. The link/graph check passes;
style still reports 25 existing findings in two Markdown files. A no-build-isolation
editable install also exposed the stale environment's setuptools 65.5.0; the
installed profile now has setuptools 84.2.1. The Windows workflow guide now
spells out project-scoped Pyright and records complete-run times (97–109
seconds, with Lizard around 24–32 seconds). Running `pyright` on package paths
from the repository root without `--project` produced 476 missing-import
diagnostics; the documented configured project commands return zero.

No tests ran; hosted CI and Requirements Lab revisions are unchanged. The
largest remaining structural target is `tools/query_rti_work.py`: it has 64
structural findings and 15,555 physical lines, including a 4,314-line `main`
function and a 752-line parser. Next, split a cohesive CLI command family into
modules without changing its text/JSON output contracts.

## 2026-10-25 — recover handoffs and split query-work CLI

Git's tracked roadmap and reachable refs contain none of the 25 added
records. No verbatim source for their previous `next` text was found. From the
saved scopes and evidence, I reconstructed 20 structured successors and added
three missing `Next` notes to scalar records. Each is marked as a reconstruction;
two already concrete successors were retained.

The 15,555-line `tools/query_rti_work.py` was split along ownership boundaries:
its dispatcher, parser, and index validator now live in `query_rti_work_cli.py`,
`query_rti_work_parser.py`, and `query_rti_work_validation.py`. The core is 9,781
lines; the new modules are 4,667, 782, and 790 lines. Direct-script and
package-mode invocations execute through the compatibility entry points.

Full profile `20261025T052657.353388Z` completed in 100.253 seconds. Ruff
remains 279 (zero production; 60 tests/fixtures and 219 other), format remains
81, structure is 382 across 91 files (up from 373/89), and file-size findings
are 48 across 25 files (up from 47/24). Opaque-mapping findings remain zero.
The query core fell from 15,555 to 9,781 lines; the new 4,667-line CLI remains
above Alphus's 3,000-line advisory. Step times: Python lint 3.134s, format
3.189s, structure 5.882s, file size 4.754s, C++ ownership/size 9.710s, C++
style 10.330s, Lizard 25.297s, C++ statistics 0.536s. Markdown link/graph passed; style remains failed on 25 existing findings
across two files. C++ style and statistics passed, while existing ownership/size
and Lizard candidates remain.

Focused Ruff passes on all four query-work modules; the three extracted modules
are formatted. The core's whole-file formatter candidate remains. `resume`
produces its prior bounded summary. The read-only `check` command still returns
two existing missing anchors in `docs/planning/ROADMAP.md`:
`github-issue-5-resign-retraction-route-pruning` and
`restore-member-identity-rebinding`.

Setup pain recurred: this venv still has no `mdrepo` command. I generated a
temporary launcher from the adjacent read-only source using pip's vendored
`distlib`, set `PYTHONPATH`, and removed the launcher after the profile. Both
Markdown checks passed. AC-013 remains open; a durable install into the local
venv is still needed for an ordinary one-command run.

No test suites ran; hosted CI and Requirements Lab revisions are unchanged.
Next, inspect the CLI discovery/report commands and define a narrow typed
context before extracting a handler family. Preserve direct-script/package
behavior and text/JSON output. Then return to the remaining Python Ruff
candidates, starting with the 37 findings in
`tools/query_rti_work_regression_lane_search.py`.

## 2026-10-25 — share teardown diagnostics and clear JPype lint hotspots

Best-effort cleanup now lives in `umbra_rti_test_support.cleanup` and is shared by the JPype JNI and mock
integration tests. Eight `try/except Exception: pass` cleanup sites now log teardown failures at debug level
while preserving the original test failure. The JNI module's duplicate `@contextmanager` decorator was
removed. Its eight Ruff findings and three formatting locations were cleared; focused Ruff checks pass
for the shared helper and mock test module. The JNI integration module is still 76,091 lines with one
integration test class, so cohesive test-family extraction is the next structural task.

The full profile `20261025T054304.796074Z` took 122.207 seconds and reported 64 Ruff findings. After
clearing the JNI module's eight findings and formatting it, profile `20261025T054621.403238Z` took
120.566 seconds: Ruff 56 (35 tests/fixtures, 21 other, zero production), 80 formatting candidates,
382 structure findings across 91 files with zero opaque mappings, and 48 file-size findings across
25 files. Compared with `20261025T052657.353388Z`, Ruff fell from 279 to 56 and formatting from 81 to 80.

Latest step timings: Python lint 4.197s, formatting 4.038s, structure 7.344s, file size 4.879s,
C++ ownership/size 11.747s, C++ style 11.191s (pass), Lizard 29.471s (fail), and C++ statistics
0.586s (pass). Markdown links/graph passed; style still reports 25 existing findings across two files.
Existing C++ ownership/size and Lizard findings remain.

Profile setup still requires a temporary `mdrepo` launcher and the adjacent source on `PYTHONPATH`;
both Markdown steps completed and the launcher was removed. No test suites ran, and hosted CI plus
Requirements Lab revisions are unchanged. Next, map ownership boundaries in the 76,091-line JNI
integration suite and extract one cohesive family into a sibling test module while preserving discovery
and imports; then continue with the remaining Ruff candidates.

## 2026-10-25 — clear Java provider test Ruff batch

`test_java_provider.py` had six Ruff findings. Two definitions of
`receiveDirectedInteraction` in the same test class had identical bodies; the duplicate was removed.
The two raw-time mock constructors now use `float`, which accepts both integer and floating-point
inputs. Two nested `subTest`/`assertRaises` contexts were combined, and Ruff sorted the imports. The
file now passes Ruff and formatting checks.

Full Alphus profile `20261025T055205.839923Z` took 120.883 seconds and exited 1 on existing repository
findings. It reports Ruff 50 (29 tests/fixtures, 21 other, zero production), 79 format candidates,
382 structure findings across 91 files with zero opaque mappings, and 48 file-size findings across
25 files. Relative to `20261025T054621.403238Z`, Ruff fell from 56 to 50 and format from 80 to 79.

Step timings: Python lint 4.117s, formatting 4.083s, structure 7.575s, file size 5.320s,
C++ ownership/size 11.702s, C++ style 12.026s (pass), Lizard 27.978s (fail), and C++ statistics
0.637s (pass). Markdown links/graph passed; style reports 25 existing findings across two files.
Observer source and adapter remain in the configured scan; Ruff production findings are zero, and
observer `workspace_models.py` remains visible with 23 structural candidates.

The stale local profile still needs a temporary mdrepo launcher and the adjacent source on
`PYTHONPATH`; the launcher was removed after the run. No test suites ran; CI and Requirements Lab
revisions are unchanged. Next, map the 76,091-line JNI integration test class into cohesive families
and extract one family into a sibling module, then continue the remaining Ruff and format candidates.

## 2026-10-25 — extract JNI factory discovery tests

Four factory-discovery and lifecycle checks moved from the 80,196-line JNI integration module into
`_jni_factory_discovery.py`. The original `JPypeJniIntegrationTest` inherits this mixin, so the existing
class-level JVM fixture remains shared. AST comparison against the original methods confirms their
executable bodies are unchanged. The extracted helper now imports optional JPype at module scope under
a `ModuleNotFoundError` guard; the first profile had moved three local-import findings into the new
module, and the guarded import removes those warnings instead of relocating them.

The corrected full profile `20261025T060210.632560Z` took 162.952 seconds and exited 1 on existing
findings. It reports Ruff 50 (29 tests/fixtures, 21 other, zero production), 79 format candidates,
379 structure findings across 91 files with zero opaque mappings, and 48 file-size findings across
25 files. The prior extraction run had 382 structure findings across 92 files before the local-import
cleanup. The latest top Ruff target is `packages/umbra-rti-api/tests/test_contracts.py` with three
findings.

Step times: Python lint 5.366s, formatting 5.617s, structure 10.729s, file size 7.267s,
C++ ownership/size 16.079s, C++ style 14.547s (pass), Lizard 42.959s (fail), and C++ statistics
0.587s (pass). Markdown links/graph passed; style reports 25 existing findings across two files.
Observer source and adapter remain in the configured scan.

Setup still needs a temporary `mdrepo` launcher and the adjacent source on `PYTHONPATH`; the launcher
was removed. No test suites ran, and CI plus Requirements Lab revisions are unchanged. Next, resolve
the three Ruff findings in the API contract tests, then continue the remaining JNI family extraction.

## 2026-10-25 — clean API contract test lint and format

`packages/umbra-rti-api/tests/test_contracts.py` had three Ruff findings: import ordering, an unnecessary
module alias, and nested test/assertion contexts. Ruff fixed them, and formatting the file removed its
whole-file format candidate. The file now passes focused Ruff and format checks.

Full profile `20261025T060601.011560Z` took 145.128 seconds and exited 1 on existing repository findings.
It reports Ruff 47 (26 tests/fixtures, 21 other, zero production), 78 format candidates, 379 structure
findings across 91 files with zero opaque mappings, and 48 file-size findings across 25 files. Compared
with `20261025T060210.632560Z`, Ruff fell from 50 to 47 and formatting from 79 to 78.

Step times: Python lint 5.038s, formatting 4.565s, structure 8.880s, file size 6.840s,
C++ ownership/size 13.846s, C++ style 13.758s (pass), Lizard 33.049s (fail), and C++ statistics
0.502s (pass). Markdown links/graph passed; style still reports 25 existing findings across two files.
Observer source and adapter remain included in the profile.

Setup still requires a temporary `mdrepo` launcher plus the adjacent source on `PYTHONPATH`; the
launcher was removed. No test suites ran; CI and Requirements Lab revisions are unchanged. The next
measured Ruff targets are `tools/requirements_lab_observations_regression.py` and
`tools/requirements_lab_resync_regression.py`, with three findings each; the latter must not trigger
a Requirements Lab resync.

## 2026-10-25 — clear regression-script import lint

The observations and resync regression scripts each had three Ruff findings: two import-order
findings and an unused `noqa` directive. Those six findings are cleared. The observations regression
module also had one format candidate, which is now formatted. Both files pass focused Ruff and format
checks. Neither script was executed and no Requirements Lab resync was run.

The latest full profile `20261025T060914.603622Z` took 99.536 seconds and exited 1 on existing
repository findings. It reports Ruff 41 (26 tests/fixtures, 15 other, zero production), 77 format
candidates, 379 structure findings across 91 files with zero opaque mappings, and 48 file-size findings
across 25 files. Compared with `20261025T060601.011560Z`, Ruff fell from 47 to 41 and formatting from
78 to 77.

Step times: Python lint 3.387s, formatting 3.176s, structure 5.846s, file size 4.121s,
C++ ownership/size 9.685s, C++ style 10.089s (pass), Lizard 25.816s (fail), and C++ statistics
0.466s (pass). Markdown links/graph passed; style reports 25 existing findings across two files.
Observer source and adapter remain included in the profile.

The profile environment still needs a temporary `mdrepo` launcher and adjacent source on `PYTHONPATH`;
the launcher was removed. No tests ran, and CI plus Requirements Lab revisions are unchanged. The next
measured Ruff target is `packages/umbra-rti-api/tests/test_2010_contracts.py` with two findings.

## 2026-10-25 - finish repository Ruff and formatting cleanup

The completed profile sequence reduced Ruff from 39 findings and 76 formatting candidates in run
`20261025T061300.757470Z` to zero Ruff findings and a passing formatter in
`20261025T063029.623701Z`. Runs `20261025T061506.764876Z` and
`20261025T061647.575926Z` both measured 37 Ruff findings and 75 format candidates; the next profile
`20261025T061937.106927Z` measured 29 and 71. After those files were formatted, the intermediate
run `20261025T062231.906107Z` had zero Ruff findings and 47 format candidates. Runs
`20261025T062505.488301Z`, `20261025T062737.731290Z`, and `20261025T063029.623701Z` passed both
Python checks. Ruff reformatted 49 files; 428 were already formatted.

Ruff cleanup fixed 26 import blocks and three remaining rules, alongside the earlier focused JNI,
API, and observer test fixes. The final observer and compile-time report changes made five concrete
list inputs sequences, replaced a mutable terminal-state set with a frozenset, typed the baseline
record with a `TypedDict`, and moved path-key normalization to pathlib. Structure findings fell from
379 across 91 files to 369 across 88 files. Concrete-input findings fell from 180 to 173, mutable
module constants from 41 to 40, and the two `os-path` findings were cleared. Opaque-dictionary
findings remain at zero.

The latest full profile took 98.224 seconds and exited 1 on remaining structure, file-size, C++
ownership/size, and Lizard findings. Step times: Python lint 3.208s (pass), Python formatting
3.121s (pass), structure 6.045s, file size 4.130s, C++ ownership/size 9.496s, C++ style 10.574s
(pass), Lizard 25.030s, and C++ statistics 0.459s (pass). Markdown links/graph passed; Markdown
style still reports the existing 25 findings across two files. Observer source and adapter remain
included in `quality-gates.toml`.

Setup pain recurred: the environment still needs a temporary mdrepo launcher and the adjacent source
on `PYTHONPATH`. Direct `distlib` was unavailable, so the launcher was generated with Python's
vendored `distlib`; it was named `mdrepo-3.11.exe`, which exposed an earlier cleanup-name mismatch.
The leftover launchers were removed and the latest profile cleanup handles either generated name.
The six lazy imports in observer `server.py` preserve optional route and transport loading, so they
remain intentional Alphus false positives. The 23 Pydantic field candidates in `workspace_models.py`
remain known Pydantic 2 false positives.

No tests or builds ran. CI enforcement and Requirements Lab revisions are unchanged. The next measured
structural target is `packages/umbra-rti-jpype/tests/test_jpype_jni_integration.py` with 63 findings;
extract one cohesive JNI family into a focused module and preserve the observer scan scope.

## 2026-10-25 - make generated 2010 contract metadata read-only

The generated 2010 API contract exposes six overload, parameter, and return-type maps. Their runtime
consumers only read them, so the generator now emits `Mapping` annotations over `MappingProxyType`
values. The generated module and generator stay aligned, and Alphus reports six fewer mutable-module
constant candidates. The official Java 2010 archive is not checked in, so regeneration against that
source was unavailable; no tests or builds were run.

The full local Alphus run `20261025T065354.920700Z` took 95.053 seconds and exited 1 on existing
provisional findings. Ruff reported zero findings and formatting passed. Python structure fell from
291 to 285 across 85 files: concrete inputs 95, local imports 117, legacy Pydantic fields 26, and
mutable module constants 34. File size remains 48 findings across 25 files. C++ style and statistics
passed; ownership/size and Lizard still report findings. Markdown links/graph passed; Markdown style
still reports 25 findings across two files. Observer source and adapter remain in the profile.

Setup still needs the adjacent mdrepo source on `PYTHONPATH` and a temporary launcher made with
Python's vendored `distlib`; both generated launcher names were cleaned up after the run. The 25 local
roadmap additions now retain an actionable follow-up from saved evidence: 20 structured handoffs
explicitly mark reconstructed wording, four string-form handoffs already include a `Next` instruction,
and the Python handoff points to the 63-finding JNI integration module. Two section-symbol encoding
glitches in tracked handoffs were restored from Git, and the edited query-work validation helper is
now included in the Python handoff's source list. CI enforcement and Requirements Lab revisions are
unchanged.

## 2026-10-25 - hoist optional JPype import

The JNI integration file had 55 method-level `import jpype` findings. Its test class already skips
when JPype is unavailable, and the sibling factory-discovery mixin uses a guarded optional import.
The integration module now uses the same guarded import once, while retaining the opt-in test guard.
Ruff and formatting pass for the changed file.

Full profile `20261025T065912.286161Z` took 96.719 seconds and exited 1 on remaining provisional
findings. Ruff reported zero findings and formatting passed. Structure fell from 285 to 230 across
85 files; local imports fell from 117 to 62, and opaque mappings remain at zero. The JNI module's
structure findings fell from 63 to 8. It is still 80,023 lines, above the 10,000-line hard limit.
File size remains 48 findings across 25 files. C++ style and statistics passed; ownership/size and
Lizard still report findings. Observer source and adapter remain included.

The next structural slice is the cohesive transport-loss callback and cutoff family in
`test_jpype_jni_integration.py`. Move one bounded family into a focused mixin module with explicit
dependencies, preserving the single JVM setup/teardown and opt-in skip behavior. Continue splitting
by behavior until the module fits the source-size limit. No tests ran; CI enforcement and Requirements
Lab revisions are unchanged.

## 2026-10-25 - name JNI callback and input contracts

The remaining tuple findings exposed an implicit MOM interaction-report shape. `_ReceivedInteraction`
now names the five callback fields, the callback recorder stores that type, and four report consumers
use named fields. Read-only helper inputs now use `Sequence`, `Mapping`, and `AbstractSet`. This
preserves the tuple-compatible callback value while making its contract explicit. `inspect` moved to
the module imports; the two imports that depend on the JNI fixture or a private runtime type remain
local. Ruff and formatting pass for the changed file.

Full profile `20261025T070557.397042Z` took 109.104 seconds and exited 1 on remaining provisional
findings. Ruff reported zero findings and formatting passed. Structure fell from 230 to 224 across
85 files, with zero opaque mappings and zero bare-tuple findings. Concrete inputs fell from 95 to 92;
local imports fell from 62 to 61. The JNI integration file now has three local-import findings and
is 80,026 lines, still above the 10,000-line hard limit.
File size remains 48 findings across 25 files. Step timings: Python lint 3.863s, formatting 3.843s,
structure 6.312s, file size 4.034s, C++ ownership/size 9.149s, C++ style 9.641s (pass), Lizard
25.898s, and C++ statistics 0.448s (pass). Markdown links/graph passed; style remains at 25
findings across two files. Observer source and adapter remain included.

The next slice is the cohesive transport-loss callback and cutoff family in
`test_jpype_jni_integration.py`. Move one bounded family into a focused mixin module with explicit
dependencies, preserving the single JVM setup/teardown fixture and opt-in skip behavior. Continue
with behavior-based splits until the module fits the source-size limit. No tests ran; CI enforcement
and Requirements Lab revisions are unchanged.

A fresh report-only `python-style --report --json` scan with the profile configuration completed in
6.7 seconds and returned zero `opaque_mappings`; its remaining category counts total 224. The older
`alphus-python-style-current.json` is dated 2026-10-24, so the current scan is the source for this
count.
