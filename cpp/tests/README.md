# Native test guide

This directory contains Umbra's native C++ tests. A test name identifies the
runtime domain or standard-binding surface it exercises; the file stays beside
similar focused tests instead of being grouped by framework.

## Test layers

| Test kind | Purpose | Typical file name |
| --- | --- | --- |
| Smoke test | Compiles or exercises a narrow installed/baseline surface without Catch2. | ieee1516_2025_headers_smoke.cpp |
| Focused Catch2 test | Tests one private domain or bounded service slice. | federation_registry_catch2.cpp |
| Integration Catch2 test | Tests a standard-facing workflow across internal domains. | ieee1516_2025_federation_management_catch2.cpp |
| Contract/traceability test | CMake invokes a tool to confirm source and requirement mappings. | Registered in the root CMake file. |

The default build profile runs smoke, standards-integrity, traceability, and
package tests. Catch2 tests are available through the native-catch2 or
native-fom profiles described in [CONTRIBUTING.md](../../CONTRIBUTING.md).

## Add or change a test

1. Start with the README for the owning internal domain.
2. Add a focused area_catch2.cpp test when a behavior needs Catch2.
3. Add the source to the root CMake test target near similar tests.
4. Use a descriptive fixture from [data/](data/README.md) only when the model
   input is part of the behavior.
5. Add or update a Requirements Lab contract only for an official,
   source-traceable behavior.

Run the narrowest test while iterating, then run the baseline:

    ctest --test-dir out/cmake/catch2 -C Debug -R federation_registry --output-on-failure
    python tools/ci.py native

## Roadmap lanes

Cross-cutting roadmap slices use Catch2 tags as CTest labels. The callback
kernel and multi-federate callback-ordering slices can be run without selecting
individual cases from the full integration executable:

    ctest --test-dir .build-fom-services -C Debug -L "^callbacks$" --output-on-failure
    ctest --test-dir .build-fom-services -C Debug -L "^multi-federate-callback-ordering$" --output-on-failure

The corresponding CMake targets are `umbra_test_callbacks` and
`umbra_test_multi_federate_callback_ordering`.
Keep new cases tagged with one execution scope, one owning domain, and the
cross-cutting roadmap tag when they belong to this slice.

FOM-composition lanes have an independent target so they do not wait on the
large federation-management translation unit. The current Annex C
directed-interaction guards are selected by the exact indexed query:

    python tools/query_rti_work.py focus schema-conflict --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_fom_composer_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.fom_composer\\.catch2\\.(The FDD materializer surfaces the multiple-directed-class schema conflict|The FDD materializer refuses the supplied extension when the official FDD schema cannot represent its directed-interaction merge)$" --output-on-failure

The exact trace joins each guard to its source line, RL-081 requirement, and
canonical IEEE 1516.2-2025 subsection; it does not imply runtime selector or
conformance coverage.

The active Annex C/reference-resolution pointer is the eight-case FOM-composer
lane. `focus reference-resolution --summary --compact` prints the exact CTest
regex; its current baseline is the transportation-name reference guard and
its trace maps to 2025 clauses 4.11.2, 6.2.5, 6.2.6, and Annex C. Run the
bounded lane only when all eight mapped guards are needed; the single baseline
CTest title from `work annex-c-and-reference-resolution --summary --compact`
keeps iteration to one test; copy the exact lane regex from `focus` when all
eight are required.

The adjacent support-switch composition guard is queryable independently with:

    python tools/query_rti_work.py trace "The FDD materializer retains the complete 2025 support-switch table" --summary --compact

and runs as the exact CTest title
`umbra.fom_composer.catch2.The FDD materializer retains the complete 2025 support-switch table`.

The current declaration-management source head is also directly traceable
without opening the full FOM-composer file:

    python tools/query_rti_work.py trace "The FDD materializer is repeatable for a fixed official module set" --summary --compact
    .build\Debug\umbra_fom_composer_catch2.exe "The FDD materializer is repeatable for a fixed official module set" --reporter compact

Its mapping is intentionally private reproducibility evidence anchored to the
existing FDD materialization candidates (IEEE 1516.2 clauses 4.14.2 and C.1).

The next source queue head is the enabled Non-Regulated-Grant switch guard;
`work fom-module-declaration-management --summary --compact` prints its exact
source location and the follow-on `unplanned` command.

After that guard, the queue advances to the Annex C.8 duplicate-switch case;
use the same `work`/`trace` commands rather than scanning the source file.

The next pointer after that is the omitted-switch-default case; the indexed
`work` result keeps its exact title, source line, and follow-on queue visible.

The following time-representation category guard is likewise available by its
exact `trace` query and remains a private FOM-composition slice.

The next source pointer after it is the tag datatype category guard; `work
fom-module-declaration-management --summary --compact` keeps that handoff
bounded to one exact `TEST_CASE`.

The inherited-member name guard is now the indexed follow-on slice; trace its
exact requirement and clause mappings before moving to the enumerated and
variant-record merge invariants at the next source pointer.

The enumerated/variant-record merge invariant case is now indexed against
Annex C.3 and Clause 7; the next source pointer is the notes-remapping and
service-usage merge case at line 3922.

The notes/service-usage merge case is now indexed against Annex C.9 and C.10;
the next source pointer is the reference logical-time selection case at line
3951.

The reference logical-time selection case is now indexed against IEEE 1516.1
§§4.5.5/4 and IEEE 1516.2 §4.8.3. The final federation-preparation case is
indexed against IEEE 1516.1 §§4.5.5/4.11.4; the FOM-composer source queue is
now exhausted, so select the next roadmap family rather than rescanning this
file.

For requirement-driven work, start with the bounded roadmap selector before
opening this directory:

    python tools/query_rti_work.py next --summary
    python tools/query_rti_work.py lane <exact-catch2-tag> --compact
    python tools/query_rti_work.py test "<exact TEST_CASE title>" --compact
    python tools/query_rti_work.py requirement <lab-id-or-clause> --summary
    python tools/query_rti_work.py section <document-id:clause-id> --summary
    python tools/query_rti_work.py check --lane <exact-catch2-tag> --compact
    python tools/query_rti_work.py unplanned --path <source-file> --summary

The exact test query resolves the C++ source line, selected Requirements-Lab
IDs, and canonical IEEE 1516.1-2025 clause/subsection keys. Use
`check --lane <exact-catch2-tag> --compact` as the iteration-local integrity
gate; it keeps unrelated historical source drift out of the active slice while
still listing unlocated rows. Use unscoped `check --compact` for a bounded
whole-plan drift sample and `check --json` when a tool or review needs the
complete mapping diagnostic.

`unplanned` is the inverse source check: it lists C++ `TEST_CASE` declarations
that are not exact plan rows, optionally narrowed to one source path. It is a
reconciliation queue only; it does not assign requirements or implementation
status.

The current process-boundary transport starting point is directly queryable:

    python tools/query_rti_work.py test "Private process transport exchanges framed data after endpoint handshake" --summary --compact
    .build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "Private process transport exchanges framed data after endpoint handshake" --reporter compact

The registry-bound process service slice is a bounded regression baseline:

    python tools/query_rti_work.py test "Private process service binds create join and receive-order interaction to the federation registry" --summary --compact
    .build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "Private process service binds create join and receive-order interaction to the federation registry" --reporter compact

The independently launched process baseline is also directly queryable:

    python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
    python tools/query_rti_work.py lane process-boundary --summary --compact
    ctest --test-dir .build-fom-services -C Debug -L process-boundary --output-on-failure
    ctest --test-dir .build-fom-services\package-smoke-consumer -C Debug -L package-process-connection-loss --output-on-failure
    .build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "Private registry-bound service exchanges federation traffic across independently launched processes" --reporter compact

It launches separate server, sender, and receiver helpers. The sender and
receiver use the private `ProcessFederationClient` seam; the receiver consumes
a pushed event frame and exercises the official C++
`FederateAmbassador::receiveInteraction` callback through the private bridge.
The public process-address/message slice is also directly queryable:

    python tools/query_rti_work.py test "RTIambassador routes public Create, Join, and Resign through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes ordinary interaction declarations through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py lane callback-controls --summary --compact
    python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
    python tools/query_rti_work.py test "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py trace "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py test "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py trace "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py trace "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py trace "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py trace "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Update Attribute Values through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback" --summary --compact
    python tools/query_rti_work.py test "Embedded transport loss applies the bounded automatic NoAction forced-resign policy" --summary --compact
    python tools/query_rti_work.py test "RTIambassador selects a configured tcp process endpoint through the official address field" --summary --compact
    python tools/query_rti_work.py test "RTIambassador rejects malformed tcp process addresses before connecting" --summary --compact

The next process-boundary case is deliberately recorded as a planned HLA_IMMEDIATE
companion for timestamped regional Update Attribute Values rather than a passing
test. Query it with `next --summary --compact` or `work --summary --compact`;
the output carries its source-file target, exact lane, requirement ids,
canonical 2025 sections, API surfaces, and focused CTest filter. Add the plan
row only when the C++ `TEST_CASE` is implemented.

The narrow `[process-boundary]` loop is 38 executable cases / 1617
focused-JUnit assertions, with every case mapped and source-located; the
local-delete codec contract contributes 9 direct assertions, the registry
service integration 44, and the public two-federate endpoint integration 18;
receive-order Delete Object Instance adds a 25-assertion codec contract and a
25-assertion public endpoint/removal-callback integration; the timestamped
public endpoint/removal-callback slice adds 64 assertions under both callback
models.
The 38-case query count is the unique mapped plan/test-declaration count; when
both aggregate and dedicated connection executables are present, the
`process-boundary` CTest label intentionally runs both registrations.
Each completed slice records its exact
assertion count in `ROADMAP-INDEX.json`. The bounded `coverage --lane transport` query
reports 57 plan entries (53 mapped, 52 source-located, and five historical
source-drift rows); use that query rather than a full-suite
scan when selecting transport work. The installed-profile ordinary,
timestamped, parameterized-envelope, connection-loss, object-registration,
and ordinary attribute-update/Reflect package checks are
separate CTest labels;
their exact names and the five-clause connection-loss baseline mapping are
available through `python tools/query_rti_work.py work --summary --compact`.
These remain process-boundary foundation evidence until protected review and
the broader interoperability gate are complete.

## Fixture and evidence boundaries

The [data/](data/README.md) directory contains small Umbra-owned XML fixtures.
It does not hold unreviewed external corpora. Requirements Lab contracts and
evidence inputs live under [compliance/](../../compliance/README.md), while
generated local evidence belongs under .compliance/ or out/.
