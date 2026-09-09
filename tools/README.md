# Developer tools

Run these utilities from the repository root. They keep generation, standards
verification, test-lane checks, and compliance-sidecar work out of the runtime
libraries.

For command arguments, use:

    python tools/<tool-name>.py --help

## Common recipes

### Run a route-aware CI lane

The Python-first `tools.ci` module is the easiest path for a junior
developer or a hosted runner. It fans out by standard and transport:

    python -m tools.ci list
    python -m tools.ci test --standard 2010 --route cpp
    python -m tools.ci test --standard 2025 --route java
    python -m tools.ci test --standard 2010 --route java --api-jar C:\path\to\api.jar --provider-jar C:\path\to\vendor.jar --dependency-jar C:\path\to\vendor-dependency.jar --factory-name "Vendor RTI"
    python -m tools.ci test --standard 2010 --route jni --api-jar C:\path\to\api.jar
    python -m tools.ci test --standard all --route all

Run the repository hygiene commands from the same entry point:

    python -m tools.ci lint
    python -m tools.ci fix --scope changed
    python -m tools.ci clean --dry-run

The 2010 C++ lane includes the exact integer/float logical-time marshal gate.
The 2010 JNI lane runs the Java carrier matrix; after staging the optional
native Python extension, the Python-side boundary evidence is:

    python -m pytest -q packages/umbra-rti-native/tests/test_native_2010.py
    python -m unittest packages/umbra-rti-jpype/tests/test_jpype_2010_jni_type_roundtrip.py -v

When the corresponding `UMBRA_ENABLE_*` switch is set, the route-aware Python
command also includes that opt-in JVM/JNI suite automatically; with no switch,
the default route remains independent of optional toolchains and artifacts.

The 2010 Java API and provider archives are intentionally caller-supplied.
Use `--dependency-jar` for provider libraries that are not bundled with the
provider JAR. See the
[CI contract](../docs/development/CI.md) for provider JAR, FOM, MIM, and
capability-profile options.

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

### Run the portable C++ TCK

Use the standard-library-only Python orchestrator for an installed provider
package. It configures and builds the CMake adapter, runs the configured CTest
matrix, and can optionally invoke the executable for JSON/JUnit evidence. All
child processes receive argument lists directly; the runner does not invoke a
shell.

    python tools/run_cpp_tck.py --package-prefix .build/package-smoke-install --build-directory .build/cpp-tck-python --scenario-set verified --callback-model both

To run a focused scenario after the build gate, add `--scenario` and
`--skip-ctest`; the resulting file is intentionally a focused artifact:

    python tools/run_cpp_tck.py --package-prefix .build/package-smoke-install --build-directory .build/cpp-tck-python --scenario cpp-tck.regional-three-dimensional-overlap --skip-configure --skip-build --skip-ctest --results .build/cpp-tck-all/three-dimensional.json

For the catalog-wide evidence check, omit `--scenario` and validate the full
promoted artifact with:

python tools/cpp_tck.py --results .build/cpp-tck-all/api-surface-inventory-full.json --promotion promoted

The validator rejects failed or unapproved skipped cases; it permits only the
explicit `cpp-tck.connection-loss-cleanup` skip whose message states that an
adapter-managed connection-loss fixture is required.

Use the standard-library-only API-surface audit to compare the official IEEE
`RTIambassador` and `FederateAmbassador` headers with the portable catalog and
source. It does not load a provider or require a shell:

    python tools/audit_cpp_tck_api_surface.py

Use the standard-library-only native-gap survey to compare the native Catch2
inventory with the portable catalog. It records explicit semantic native-to-
portable mappings, while its remaining portable-candidate list is a review
queue, not promotion evidence. The private-boundary check follows local
quoted includes and reports transitive private marker files, so a wrapper
cannot hide a provider-specific harness. The survey does not compile or run
provider-specific tests:

    python tools/survey_cpp_tck_native_gaps.py

The same validator requires every Java catalog entry marked `run` to have
either a direct Java scenario ID or an explicit C++ parity mapping; entries
marked unsupported remain excluded.

The verified lane is 375 promoted scenario IDs (750 callback-model cases).
`--scenario-set all` configures all 381 available IDs (762 cases), including
six candidates. The candidate-inclusive evidence figures below are historical
artifacts from an earlier 344-ID, 688-case catalog and are not the promoted
gate. With `--connection-loss-fixture <path>`, the Python adapter
owns the connection-loss fixture and merges its two callback-model results
into the direct evidence; the no-fixture candidate-inclusive baseline records
676 direct passes plus 12 explicit skips, while the fixture-backed matrix
completes 686/686 ordinary CTest cases with no failures and records 678 direct
passes plus only the ten documented candidate immediate-model skips across all
688 callback-model cases.
The latest promoted object
management expansion includes independently selectable pure-standard contracts
for object-name reservation and object registration/discovery lifecycle.
The promoted `cpp-tck.named-registration-contract` runner exposes standard
object-instance name reservation/release and reuse, multiple-name lifecycle,
named registration and discovery identity, contention, and invalid-name
boundaries as an independently selectable pure C++ contract. The provider
package, FOM, endpoint, callback model, and logical-time configuration remain
adapter inputs.
The promoted `cpp-tck.object-attribute-subscription-lifecycle-contract` runner
exposes passive and active ordinary object-attribute subscriptions,
activation-time discovery, reflection, downgrade/reactivation, unsubscription,
and stable identity lookups as an independently selectable pure C++ contract.
The provider package, FOM, endpoint, callback model, and logical-time
configuration remain adapter inputs.
The promoted `cpp-tck.local-delete-object-instance-contract` runner exposes
the standard local-delete service boundaries, ownership and pending-acquisition
protection, fresh-requester deletion, rediscovery, stable identity lookups, and
continued ordinary reflection as an independently selectable pure C++ contract.
The provider package, FOM, endpoint, callback model, and logical-time
configuration remain adapter inputs.
The promoted `cpp-tck.timestamped-directed-interaction-tar-nmr-contract` and
`cpp-tck.timestamped-directed-interaction-immediate-source-resignation-contract`
runners expose the standard-only directed-interaction contracts as independently
selectable slices. Their focused four-scenario lane passed 8/8 callback-model
cases. The promoted `cpp-tck.query-lits-source-resignation-contract` and
`cpp-tck.partial-attribute-ownership-transfer-contract` runners add the same
standard-only boundary for Query LITS and partial ownership transfer; their
focused four-scenario lane also passed 8/8. The full promoted aggregate now
passes 662/662 CTest cases plus 662 direct passes with only the two expected
connection-loss skips. The promoted
`cpp-tck.federation-teardown-isolation-contract`,
`cpp-tck.mixed-update-rate-subscriptions-contract`, and
`cpp-tck.timestamped-attribute-update-rate-reduction-contract` runners add
the same standard-only boundary for update-rate isolation and reduction; their
focused six-scenario lane passed 12/12 callback-model cases. The promoted
`cpp-tck.explicit-mim-creation-contract` and
`cpp-tck.federation-mom-current-fdd-contract` runners add the same
standard-only boundary for adapter-supplied MIM composition and the federation
MOM current-FDD surface; their focused four-scenario lane passed 8/8
callback-model cases. The promoted
`cpp-tck.service-report-regional-interaction-contract` and
`cpp-tck.service-report-regional-interaction-subscription-contract` runners add
the same standard-only boundary for successful regional interaction service
reporting, typed MOM invocation metadata, and regional delivery or subscription
state; their focused four-scenario lane passed 8/8 callback-model cases. The
full promoted aggregate now passes 662/662 CTest cases plus 662 direct passes
with only the two expected connection-loss skips.
The promoted `cpp-tck.federation-mom-save-conditionals-contract` and
`cpp-tck.joined-federate-mom-federate-state-save-restore-contract` runners
add the standard save/restore MOM contract twins. Their focused four-scenario
lane passed 8/8 callback-model cases; the catalog-wide aggregate passed
666/666 CTest cases plus 666 direct passes with only the two expected
connection-loss skips.
The promoted `cpp-tck.divestiture-if-wanted-mixed-acquirers-contract` and
`cpp-tck.negotiated-divestiture-partial-acquisition-cancellation-contract`
runners add independently selectable pure-standard ownership contract twins.
Their focused four-case lane passed 4/4 callback-model cases. The latest
promoted aggregate is recorded in
`.build/cpp-tck-all/verified-evidence-ownership-contract-expansion.json` and
passed 670/670 ordinary CTest cases, with 670 direct passes and only the two
expected adapter-managed connection-loss skips; the strict catalog validator
reported `valid=true`.
The promoted `cpp-tck.region-lifecycle-contract` runner adds the independently
selectable standard region and dimension lifecycle contract. Its focused
base-and-contract lane passed 4/4 callback-model cases; the preceding promoted
aggregate is recorded in
`.build/cpp-tck-all/verified-evidence-regional-unpublish-region-release-contract.json` and
passed 674/674 ordinary CTest cases, with 674 direct passes and only the two
expected adapter-managed connection-loss skips. The strict catalog validator
reported `valid=true`.
The promoted `cpp-tck.regional-unpublish-region-release-contract` runner adds
the independently selectable standard regional publication and region-release
dependency contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the promoted aggregate above records the contract under
both callback models against the installed package. The source uses only
official IEEE C++ headers and the standard library; the provider, dimensional
FOM, endpoint, callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.regional-object-update-contract` runner adds the
independently selectable standard regional object publication, subscription,
discovery, Update/Reflect, value-request, and reassociation contract. Its
focused base-and-contract lane passed 4/4 callback-model cases; the promoted
aggregate records 676/676 CTest cases with 676 direct passes and only the two
expected adapter-managed connection-loss skips. The source uses only official
IEEE C++ headers and the standard library; the provider, dimensional FOM,
endpoint, callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.regional-attribute-value-request-filtering-contract`
runner adds the independently selectable standard regional Request Attribute
Value Update filtering contract. Its focused base-and-contract lane passed
4/4 callback-model cases; the promoted aggregate records 678/678 CTest cases
with 678 direct passes and only the two expected adapter-managed connection-
loss skips. The source uses only official IEEE C++ headers and the standard
library; the provider, dimensional FOM, endpoint, callback, and logical-time
configuration remain adapter-owned.
The promoted `cpp-tck.regional-attribute-value-update-response-recheck-contract`
runner adds the independently selectable standard regional attribute-value
response eligibility and reflection-metadata contract. Its focused
base-and-contract lane passed 4/4 callback-model cases; the promoted aggregate
records 680/680 CTest cases with 680 direct passes and only the two expected
adapter-managed connection-loss skips. The source uses only official IEEE C++
headers and the standard library; the provider, dimensional FOM, endpoint,
callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.default-region-object-routing-contract` runner adds the
independently selectable standard ordinary/default-region object routing and
association-replacement contract. Its focused base-and-contract lane passed
4/4 callback-model cases; the promoted aggregate records 682/682 CTest cases
with 682 direct passes and only the two expected adapter-managed connection-
loss skips. The source uses only official IEEE C++ headers and the standard
library; the provider, dimensional FOM, endpoint, callback, and logical-time
configuration remain adapter-owned.
The promoted `cpp-tck.passive-regional-subscription-contract` runner adds the
independently selectable standard passive regional subscription suppression and
activation contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the promoted aggregate records 684/684 CTest cases with
684 direct passes and only the two expected adapter-managed connection-loss
skips. The source uses only official IEEE C++ headers and the standard library;
the provider, dimensional FOM, endpoint, callback, and logical-time
configuration remain adapter-owned.
The promoted `cpp-tck.auto-provide-contract` runner adds the independently
selectable standard Auto Provide switch and grouped solicitation contract. Its
focused base-and-contract lane passed 4/4 callback-model cases; the promoted
aggregate records 686/686 CTest cases with 686 direct passes and only the two
expected adapter-managed connection-loss skips. The source uses only official
IEEE C++ headers and the standard library; the Auto Provide FOM, provider,
endpoint, callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.attribute-scope-advisories-contract` runner adds the
independently selectable standard attribute-scope switch, transition, and
stale-callback contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the promoted aggregate records 688/688 CTest cases with
688 direct passes and only the two expected adapter-managed connection-loss
skips. The source uses only official IEEE C++ headers and the standard library;
the dimensional FOM, provider, endpoint, callback, and logical-time
configuration remain adapter-owned.
The promoted `cpp-tck.regional-declaration-relevance-advisories-contract` runner
adds the independently selectable standard regional object and interaction
declaration-relevance contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the promoted aggregate records 690/690 CTest cases with
690 direct passes and only the two expected adapter-managed connection-loss
skips. The source uses only official IEEE C++ headers and the standard library;
the dimensional FOM, provider, endpoint, callback, and logical-time
configuration remain adapter-owned.
The promoted `cpp-tck.regional-interaction-routing-contract` runner adds the
independently selectable standard ordinary regional interaction routing
contract. Its focused base-and-contract lane passed 4/4 callback-model cases;
the promoted aggregate records 692/692 CTest cases with 692 direct passes and
only the two expected adapter-managed connection-loss skips. The source uses
only official IEEE C++ headers and the standard library; the dimensional FOM,
provider, endpoint, callback, and logical-time configuration remain
adapter-owned.
The promoted `cpp-tck.regional-interaction-source-region-snapshot-contract`
runner adds the independently selectable standard regional interaction
source-region snapshot contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the promoted aggregate records 694/694 CTest cases with
694 direct passes and only the two expected adapter-managed connection-loss
skips. The source uses only official IEEE C++ headers and the standard library;
the dimensional FOM, provider, endpoint, callback, and logical-time
configuration remain adapter-owned.
The promoted `cpp-tck.regional-interaction-subscription-filtering-contract`
runner adds the independently selectable standard regional interaction
subscription-filtering contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the promoted aggregate records 696/696 CTest cases with
696 direct passes and only the two expected adapter-managed connection-loss
skips. The source uses only official IEEE C++ headers and the standard library;
the dimensional FOM, provider, endpoint, callback, and logical-time
configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-interaction-contract` runner adds
the independently selectable standard timestamped regional interaction
delivery and retraction contract. Its focused base-and-contract lane passed
4/4 callback-model cases; the promoted aggregate records 698/698 CTest cases
with 698 direct passes and only the two expected adapter-managed
connection-loss skips. The source uses only official IEEE C++ headers and the
standard library; the dimensional FOM, provider, endpoint, callback, and
logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-interaction-regulation-reenable-contract`
runner adds the independently selectable standard timestamped regional
interaction Time Regulation re-enable contract. Its focused base-and-contract
lane passed 4/4 callback-model cases; the promoted aggregate records 702/702
CTest cases with 702 direct passes and only the two expected adapter-managed
connection-loss skips. The source uses only official IEEE C++ headers and the
standard library; the dimensional FOM, provider, endpoint, callback, and
logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-interaction-alternate-advances-contract`
runner adds the independently selectable standard timestamped regional
interaction alternate-advance contract. Its focused base-and-contract lane
passed 4/4 callback-model cases; the promoted aggregate records 702/702 CTest
cases with 702 direct passes and only the two expected adapter-managed
connection-loss skips. The source uses only official IEEE C++ headers and the
standard library; the dimensional FOM, provider, endpoint, callback, and
logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-interaction-no-overlap-contract`
runner adds the independently selectable standard timestamped regional
interaction no-overlap and retraction contract. Its focused base-and-contract
lane passed 4/4 callback-model cases; the promoted aggregate records 704/704
CTest cases with 704 direct passes and only the two expected adapter-managed
connection-loss skips. The source uses only official IEEE C++ headers and the
standard library; the dimensional FOM, provider, endpoint, callback, and
logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-interaction-subscription-replacement-contract`
runner adds the independently selectable standard timestamped regional
interaction subscription-replacement contract. Its focused base-and-contract
lane passed 4/4 callback-model cases; the promoted aggregate records 706/706
CTest cases with 706 direct passes and only the two expected adapter-managed
connection-loss skips. The source uses only official IEEE C++ headers and the
standard library; the dimensional FOM, provider, endpoint, callback, and
logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-interaction-source-resignation-contract`
and `cpp-tck.timestamped-regional-interaction-tar-nmr-contract` runners add
independently selectable standard contracts for queued delivery after producer
resignation and delivery before ordinary TAR/NMR grants. Their focused
base-and-contract lane passed 8/8 callback-model cases; the promoted aggregate
records 710/710 CTest cases with 710 direct passes and only the two expected
adapter-managed connection-loss skips. The source uses only official IEEE C++
headers and the standard library; dimensional FOM, provider, endpoint,
callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-attribute-update-contract` and
`cpp-tck.timestamped-regional-attribute-alternate-advances-contract` runners add
independently selectable standard contracts for timestamped regional
Update/Reflect and Flush Queue/TAR Available/NMR Available delivery. Their
focused base-and-contract lane passed 8/8 callback-model cases; the promoted
aggregate records 714/714 CTest cases with 714 direct passes and only the two
expected adapter-managed connection-loss skips. The source uses only official
IEEE C++ headers and the standard library; dimensional FOM, provider, endpoint,
callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-regional-attribute-association-replacement-contract`,
`cpp-tck.timestamped-regional-attribute-regulation-reenable-contract`, and
`cpp-tck.timestamped-regional-attribute-source-resignation-contract` runners add
independently selectable standard contracts for source-association replacement,
changed-lookahead Time Regulation re-enable, and post-resignation delivery. Their
focused base-and-contract lane passed 12/12 callback-model cases; the promoted
aggregate passed 720/720 CTest cases with 720 direct passes and only the two
expected adapter-managed connection-loss skips in
`.build\\cpp-tck-all\\verified-evidence-timestamped-regional-attribute-association-regulation-resignation-contracts.json`.
The source uses only official IEEE C++ headers and the standard library;
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.
The promoted `cpp-tck.timestamped-default-region-attribute-alternate-advances-contract`,
`cpp-tck.timestamped-default-region-attribute-reenable-contract`,
`cpp-tck.timestamped-default-region-attribute-regulation-reenable-contract`, and
`cpp-tck.timestamped-default-region-attribute-mixed-fanout-contract` runners add
independently selectable standard contracts for alternate advances, Time
Constrained and Time Regulation re-enable, and mixed fanout. Their focused
base-and-contract lane passed 16/16 callback-model cases; the promoted aggregate
passed 728/728 CTest cases with 728 direct passes and only the two expected
adapter-managed connection-loss skips in
`.build\\cpp-tck-all\\verified-evidence-timestamped-default-region-attribute-contracts.json`.
The source uses only official IEEE C++ headers and the standard library; FOM,
provider, endpoint, callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.timestamped-default-region-interaction-contract`,
`cpp-tck.timestamped-default-region-interaction-alternate-advances-contract`,
`cpp-tck.timestamped-default-region-interaction-mixed-fanout-contract`,
`cpp-tck.timestamped-default-region-interaction-source-resignation-contract`,
`cpp-tck.timestamped-default-region-interaction-reenable-contract`, and
`cpp-tck.timestamped-default-region-interaction-regulation-reenable-contract`
runners add independently selectable standard contracts for baseline delivery,
alternate advances, mixed fanout, source resignation, Time Constrained re-enable,
and Time Regulation re-enable. Their focused base-and-contract lane passed
24/24 callback-model cases; the promoted aggregate passed 740/740 CTest cases
with 740 direct passes and only the two expected adapter-managed connection-loss
skips in `.build\\cpp-tck-all\\verified-evidence-timestamped-default-region-interaction-contracts.json`.
The source uses only official IEEE C++ headers and the standard library; FOM,
provider, endpoint, callback, and logical-time configuration remain adapter-owned.
The promoted `cpp-tck.ownership-acquisition-cancellation-transfer-race` and
`cpp-tck.negotiated-willing-to-acquire-continuation` contract twins passed 8/8
callback-model cases in their focused four-scenario lane. The promoted aggregate
passed 748/748 CTest cases with 748 direct passes and only the two expected
adapter-managed connection-loss skips in
`.build\\cpp-tck-all\\verified-evidence-standard-ownership-cancellation-continuation.json`.
The timed regular-candidate contract twin recorded 2 evoked passes and 2
explicit immediate-model skips in its focused four-case lane; it remains a
candidate pending broader adapter coverage.
The timed pre-delivery cancellation contract twin recorded 2 evoked passes and
2 explicit immediate-model skips in its focused four-case lane; it remains a
candidate pending broader adapter coverage.
The timed confirmation-cancellation contract twin recorded 2 evoked passes and
2 explicit immediate-model skips in its focused four-case lane; it remains a
candidate pending broader adapter coverage.
The promoted `cpp-tck.mixed-update-rate-subscriptions` case uses the
adapter-supplied rich FOM to verify ordinary per-attribute update-rate gating.
Its focused portable lane passed 2/2 callback-model cases, and the matching
native oracle passed 37 assertions.
The promoted `cpp-tck.fom-empty-module-validation` case checks empty-FOM
rejection and same-name recovery with the adapter-supplied FOM using only the
official C++ API.
The promoted `cpp-tck.custom-transportation-interaction-delivery` case uses
the adapter-declared rich FOM to verify custom transportation lookup/name
round-trips, ordinary interaction delivery, received transportation identity,
and the standard transportation query report. Its focused portable artifact
passed 2/2 callback-model cases, and the matching native oracle passed 42
assertions.
The promoted `cpp-tck.custom-transportation-regional-attribute-delivery` case
uses the adapter-declared rich FOM and DDM dimensions to verify ordinary
regional attribute publication/subscription/update delivery, conveyed
source-region metadata, overlap filtering, and custom transportation identity.
Its focused portable artifact passed 2/2 callback-model cases, and the matching
native oracle passed 51 assertions.
The promoted `cpp-tck.custom-transportation-regional-interaction-delivery` case
uses the same adapter-declared rich FOM and DDM dimensions to verify ordinary
regional interaction publication/subscription/send delivery, parameter, tag,
producer, conveyed source-region, and custom transportation metadata. Its
focused portable artifact passed 2/2 callback-model cases, and the matching
native oracle passed 40 assertions.
The promoted `cpp-tck.custom-transportation-timestamped-delivery` case uses
the adapter-declared rich FOM to verify timestamped interaction delivery,
constrained grant timing, payload/tag/producer/time/order/retraction metadata,
and custom transportation identity without region metadata. Its focused
portable artifact passed 2/2 callback-model cases, and the matching native
oracle passed 48 assertions.
The promoted `cpp-tck.custom-transportation-timestamped-directed-delivery` case
uses the adapter-declared rich FOM to verify timestamped directed-interaction
delivery to a registered target, constrained grant timing,
payload/tag/target/producer/time/order/retraction metadata, and custom
transportation identity. Its focused portable artifact passed 2/2 callback-model
cases, and the matching native oracle passed 42 assertions.
The promoted `cpp-tck.custom-transportation-timestamped-regional-attribute-delivery`
case reuses the standard regional timestamped-attribute oracle with adapter-
declared FOM and DDM names to verify timestamped regional attribute delivery,
region metadata, retraction, and custom transportation identity. Its focused
portable artifact passed 2/2 callback-model cases, and the matching native oracle
passed 60 assertions.
The promoted regional-interaction source-region snapshot case also verifies
send-time source-region capture, disjoint suppression after source mutation,
and restored-overlap delivery through the standard API.
The promoted regional-interaction subscription-report case verifies standard
MOM reports for `SubscribeInteractionClassWithRegions` and
`UnsubscribeInteractionClassWithRegions`, including typed association
arguments and the passive-subscription indicator.
An adapter that supports fault injection can opt into the connection-loss case
with the CMake cache settings
`HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER` and
`HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED`; the Python runner maps
those options into the adapter configuration used by both CTest and direct
execution. The portable executable only observes the standard callback; the
adapter owns the fault fixture. The current-process adapter additionally provides the shell-free
`packages/hla-rti-cpp-tck/adapters/current-process/run_connection_loss.py`
harness; its evoked/immediate evidence is recorded in
`.build/cpp-tck-all/connection-loss-current-process-python.json` and is kept
adapter-specific because a generic fault-injection capability is outside the
IEEE API.

The promoted timestamped
Java-parity factory-discovery case checks that the standard C++
`RTIambassadorFactory` returns an ambassador and that the official
`HLAinteger32BE` encoder round-trips without provider-specific or private
headers.

The promoted Java-parity API-surface-inventory case checks the official C++
ambassador, callback, logical-time, and byte-container types, then verifies
factory creation and standard byte preservation without provider-specific
headers or FOM assumptions.

The promoted joined-federate MOM updates-sent-counts case uses only the adapter-supplied
ordinary FOM plus standard reliable/best-effort transportation. It requests
`HLArequestUpdatesSent` and verifies both `HLAreportUpdatesSent` transport
buckets, nested `HLAobjectClassBasedCounts`, standard report metadata, and the
empty response for a requester with no sent updates.
The promoted joined-federate MOM interactions-received-counts case delivers
one adapter interaction reliably and two after a standard best-effort change,
then verifies `HLAreportInteractionsReceived` buckets and nested
`HLAinteractionCounts`, including the empty response for a requester with no
received interactions.
The promoted joined-federate MOM interactions-sent-counts case sends one
adapter interaction reliably and two after a standard best-effort change, then
verifies `HLAreportInteractionsSent` buckets and nested `HLAinteractionCounts`,
including the empty response for an idle joined federate.
The promoted joined-federate MOM reflections-received-counts case delivers one
adapter attribute reflection reliably and two after a standard best-effort
change, then verifies `HLAreportReflectionsReceived` buckets and nested
`HLAobjectClassBasedCounts`, including the empty response for a requester with
no received reflections.
The promoted joined-federate MOM reflection-counts case observes the standard
`HLAobjectInstancesReflected` and `HLAreflectionsReceived` counters through
direct attribute-value requests and periodic `HLAsetTiming` reflections. It
distinguishes repeated reflections of one object from first reflections of
another, includes a timestamped reflection, and covers both ordinary and
timestamped callback paths using only adapter-supplied FOM/MIM and official
IEEE C++ API types.
The promoted joined-federate MOM directed-interactions-received case sends one
ordinary and one directed interaction of the same adapter-supplied class,
proves that ordinary receipt is excluded from
`HLAreportDirectedInteractionsReceived`, and verifies the reliable directed
bucket plus empty best-effort and idle buckets through nested standard
`HLAinteractionCounts` using adapter-supplied FOM/MIM and official IEEE C++ API
types.
The promoted joined-federate MOM directed-interactions-sent case sends one
ordinary and two directed interactions of the same adapter-supplied class,
changes the class to best effort for an ordinary send, and proves that only
the directed sends enter `HLAreportDirectedInteractionsSent`. It verifies the
reliable directed bucket plus empty best-effort and idle buckets through nested
standard `HLAinteractionCounts` using adapter-supplied FOM/MIM and official
IEEE C++ API types.
The promoted `cpp-tck.mom-transportation-type-change-request` case uses the
standard MIM request interactions for attribute and interaction transportation-
type changes, confirms both callbacks, verifies best-effort object and
interaction delivery plus reliable per-federate isolation, and checks the
RTI-originated `HLAreportServiceInvocation` report. Its request payloads use
only official standard encodings; the adapter supplies the FOM, MIM, endpoint,
callback model, and logical-time implementation.
The promoted timestamped
directed-interaction source-resignation fanout case verifies independent
recipient-local TAR frontiers after the producer resigns, including target,
payload, producer, timestamp/order, transport, retraction, and callback-before-
grant metadata. The promoted timestamped
directed-interaction TAR/NMR case sends one target-qualified message at time 7,
then proves that independent TAR(7) and NMR(10) requests each receive their
callback before their own grant, with the NMR grant returning at message time.
The promoted federation MOM save-conditionals case observes the standard
`HLAfederation` `HLAnextSaveName/Time` and `HLAlastSaveName/Time` attributes
through pending timestamped-save, admission-clearing, and successful-completion
reflections. It remains limited to the adapter-supplied standard MIM/FOM,
logical-time implementation, and official IEEE C++ API.
The promoted joined-federate MOM removed-object-count case first queries the
standard `HLAfederateHandle` attribute and verifies the
`FederateAmbassador::attributeIsOwnedByRTI` callback for the RTI-owned
joined-federate MOM object. It then observes the standard
`HLAobjectInstancesRemoved` counter after an ordinary receive-order object
deletion and verifies its reliable RTI-originated reflection metadata. It uses
only the adapter-supplied standard MIM/FOM and official IEEE C++ API.
The promoted joined-federate MOM time-state-duration case observes
`HLAtimeGrantedTime` and `HLAtimeAdvancingTime` through direct AVU and one
`HLAsetTiming` periodic reflection with official `HLAinteger32BE` encodings in
both callback models.
The promoted joined-federate MOM GALT/LITS periodic case observes standard
`HLAGALT` and `HLALITS` through direct AVU and `HLAsetTiming`, including the
undefined-value boundary after disabling the sole time regulator. The promoted
joined-federate MOM TSO-length periodic case observes standard `HLATSOlength`
directly and periodically around a queued timestamped adapter interaction,
then verifies that the count returns to zero after the grant. Both cases use
only adapter-supplied FOM/MIM and official IEEE C++ API types in both callback
models.
The promoted timestamped attribute-update no-fanout case verifies a valid
producer-side retraction handle, one legal retract, and the terminal
`MessageCanNoLongerBeRetracted` result when no recipient is eligible, with no
local delivery callbacks.
The promoted timestamped
attribute update-rate reduction case uses an adapter-supplied rich FOM to verify
reliable delivery, named best-effort rate suppression, and standard retraction
terminalization. FOM files,
dimensions, endpoint settings, callback model, and logical-time implementation
remain adapter inputs; the source stays on the official IEEE C++ API. The
promoted `cpp-tck.timestamped-object-deletion-tombstone` case verifies the
terminal timestamped deletion boundary and named object-instance reuse using
the same adapter-supplied FOM and logical-time inputs. The promoted
`cpp-tck.timestamped-object-deletion-no-fanout` case verifies the exact-lookahead
terminal boundary plus no-recipient retraction and restoration of object-name
lookup and local attribute ownership without Request Retraction fan-out. The
promoted `cpp-tck.receive-order-attribute-update-callback-cancellation` case
verifies that evoked queued reflection is suppressed by unsubscribe before
callback servicing while immediate delivery is observed before unsubscribe.
Both routes use only the adapter-selected ordinary FOM and official API
callbacks. The
promoted `cpp-tck.interaction-subscription-lifecycle` case verifies passive
ordinary interaction suppression, active replacement without replay,
downgrade back to passive, and final unsubscription under both callback
models. The
promoted `cpp-tck.interaction-subscription-lifecycle-contract` runner exposes
the same declaration boundary as an independently selectable pure standard C++
contract, with provider, FOM, endpoint, and callback configuration supplied by
the adapter. The
promoted `cpp-tck.object-attribute-subscription-lifecycle` case applies the same
portable declaration lifecycle to ordinary object attributes: passive
subscriptions suppress discovery and reflection, activation discovers the
existing object, downgrade suppresses later updates, reactivation restores
reflection, and unsubscribe removes delivery. The
promoted `cpp-tck.object-publication-registration-fence` case verifies that
whole-class unpublication fences ordinary object registration with
`ObjectClassNotPublished`, while republishing restores registration,
discovery, and stable object/class/name lookups. It uses only adapter-supplied
FOM values and the official C++ API callbacks. The
promoted `cpp-tck.object-publication-registration-fence-contract` runner
exposes the same boundary as an independently selectable pure standard C++
contract, with provider, FOM, endpoint, and callback configuration supplied by
the adapter. The
promoted `cpp-tck.interaction-publication-send-fence` case verifies that
whole-class unpublication fences ordinary `sendInteraction` with
`InteractionClassNotPublished`, while republication restores parameter
delivery and standard producer/tag/transport metadata. It uses only
adapter-supplied FOM values and official C++ API callbacks. The
promoted `cpp-tck.interaction-publication-send-fence-contract` runner exposes
the same publication boundary as an independently selectable pure standard C++
contract, with provider, FOM, endpoint, and callback configuration supplied by
the adapter. The
promoted `cpp-tck.service-report-interaction` scenario additionally exercises
the standard MOM `HLAreportServiceInvocation` interaction using an
adapter-supplied standard MIM. The adjacent promoted
`cpp-tck.service-report-attribute-update` scenario verifies the same standard
reporting contract for an ordinary `UpdateAttributeValues` service. The
promoted `cpp-tck.service-report-request-attribute-value-update` scenario
verifies both standard `RequestAttributeValueUpdate` overloads, their MOM
report serial progression, and the corresponding provider callbacks. The
promoted `cpp-tck.service-report-request-attribute-value-update-contract`,
`cpp-tck.service-report-release-multiple-object-instance-names-contract`,
`cpp-tck.service-report-release-object-instance-name-contract`, and
`cpp-tck.service-report-reserve-object-instance-name-contract` runners expose
the same ordinary MOM success routes as independently selectable pure standard
C++ contracts, with MIM, FOM, endpoint, and callback configuration supplied by
the adapter. The
promoted `cpp-tck.service-report-timestamped-interaction-contract` runner
exposes the successful timestamped interaction MOM route as an independently
selectable pure standard C++ contract, retaining typed report, constrained
delivery, logical-time-grant, payload, retraction, and callback assertions with
MIM, FOM, endpoint, callback, and logical-time configuration supplied by the
adapter. The
promoted `cpp-tck.timestamped-interactions-contract` runner exposes the
standard timestamped Send/Receive, time-role, retraction, and Request Retraction
surface as an independently selectable pure standard C++ contract, with FOM,
endpoint, callback, and logical-time configuration supplied by the adapter. The
promoted
`cpp-tck.timestamped-interaction-source-resignation-contract` and
`cpp-tck.timestamped-interaction-source-resignation-fanout-contract` runners
expose standard queued timestamped interaction source-resignation and
per-recipient fan-out delivery as independently selectable pure standard C++
contracts. They retain the post-resignation retraction boundary, TAR/NMR
servicing, payload/time/order/transport metadata, and callback ordering while
taking FOM, endpoint, callback, and logical-time configuration from the adapter.
The promoted `cpp-tck.directed-interaction-publication-send-fence-contract` and
`cpp-tck.directed-interaction-target-lifecycle-contract` runners expose the
standard targeted publication and target-lifecycle boundaries as independently
selectable pure standard C++ contracts, with provider, FOM, endpoint, and
callback configuration supplied by the adapter. The promoted
`cpp-tck.directed-interaction-subscription-kind-contract` runner exposes the
standard by-ownership and universal subscription-kind boundary as an
independently selectable pure standard C++ contract, with provider, FOM,
endpoint, and callback configuration supplied by the adapter. The promoted
`cpp-tck.unnamed-join-overload-contract`,
`cpp-tck.standard-order-and-transportation-lookups-contract`, and
`cpp-tck.callback-controls-contract` runners expose standard federation
membership, mandatory order/transportation lookup, and callback enable/disable
boundaries as independently selectable pure standard C++ contracts, with
provider, FOM, endpoint, and callback configuration supplied by the adapter.
The promoted
`cpp-tck.resign-delete-objects-contract`,
`cpp-tck.resign-unconditional-divestiture-contract`, and
`cpp-tck.final-federate-resignation-cleanup-contract` runners expose standard
resignation-time deletion, unconditional divestiture, final-federate cleanup,
ownership, object-name reuse, and identity boundaries as independently
selectable pure standard C++ contracts, with provider, FOM, endpoint, and
callback configuration supplied by the adapter.
The promoted
`cpp-tck.resign-pending-acquisition-rejection-contract`,
`cpp-tck.resign-cancel-pending-acquisition-contract`,
`cpp-tck.resign-cancel-if-available-pending-contract`, and
`cpp-tck.resign-cancel-negotiated-pending-contract` runners expose standard
pending-acquisition rejection and cancellation boundaries as independently
selectable pure standard C++ contracts, with provider, FOM, endpoint, and
callback configuration supplied by the adapter.
The promoted
`cpp-tck.negotiated-divestiture-cancellation-contract` and
`cpp-tck.negotiated-divestiture-pre-delivery-cancellation-contract` runners
expose negotiated ownership cancellation and pre-delivery callback suppression
as independently selectable pure standard C++ contracts, with provider, FOM,
endpoint, and callback configuration supplied by the adapter.
The promoted `cpp-tck.update-rate-queries-contract` and
`cpp-tck.handle-wire-formats-contract` runners expose named-rate state and
standard handle wire-format boundaries as independently selectable pure standard
C++ contracts, with provider, FOM, DDM, endpoint, logical-time, and callback
configuration supplied by the adapter.
The promoted `cpp-tck.timestamped-interaction-cross-producer-order-contract`,
`cpp-tck.timestamped-interaction-no-fanout-contract`, and
`cpp-tck.timestamped-interaction-retraction-fanout-contract` runners expose
standard multi-producer ordering, no-fan-out terminalization, and delivered/
queued retraction fan-out boundaries as independently selectable pure standard
C++ contracts, with provider, FOM, endpoint, logical-time, and callback
configuration supplied by the adapter.
The promoted `cpp-tck.timestamped-attribute-order-cohort-contract`,
`cpp-tck.timestamped-attribute-update-queued-passel-retraction-contract`, and
`cpp-tck.timestamped-attribute-update-no-fanout-contract` runners expose
timestamped attribute ordering, queued passel retraction, and no-recipient
terminalization as independently selectable pure standard C++ contracts, with
provider, FOM, endpoint, logical-time, and callback configuration supplied by
the adapter.
The promoted `cpp-tck.timestamped-attribute-update-alternate-advances-contract`,
`cpp-tck.timestamped-attribute-update-flush-queue-future-input-contract`, and
`cpp-tck.timestamped-attribute-update-reenable-contract` runners expose
alternate advance servicing, future-input Flush Queue behavior, and Time
Constrained re-enable as independently selectable pure standard C++ contracts,
with provider, FOM, endpoint, logical-time, and callback configuration supplied
by the adapter.
The promoted `cpp-tck.timestamped-attribute-source-resignation-contract`,
`cpp-tck.timestamped-attribute-source-resignation-fanout-contract`, and
`cpp-tck.timestamped-attribute-update-regulation-reenable-contract` runners
extend the same pure boundary to post-resignation ownership and queued
delivery, per-recipient timestamped attribute fan-out, and Time Regulation
re-enable with changed lookahead. They use only the standard C++ API and
standard library while taking provider, FOM, endpoint, callback, and
logical-time configuration from the adapter.
The promoted `cpp-tck.timestamped-object-deletion-no-fanout-contract`,
`cpp-tck.timestamped-object-deletion-tombstone-contract`, and
`cpp-tck.timestamped-object-deletion-regulation-reenable-contract` runners
extend that pure surface to no-recipient deletion retraction and
name/ownership restoration, terminal deletion tombstones and named
re-registration, and Time Regulation re-enable with changed lookahead. They
use only the official C++ API and standard library; provider package, FOM,
endpoint, callback, and logical-time configuration remains adapter-owned.
The promoted `cpp-tck.timestamped-object-deletion-source-resignation-fanout-contract`,
`cpp-tck.timestamped-object-deletion-retraction-joined-owners-contract`, and
`cpp-tck.timestamped-object-deletion-mixed-advances-contract` runners add
independent post-resignation fan-out, joined-owner retraction cleanup, and
Flush Queue/TAR-available/NMR-available delivery boundaries on the same pure
standard surface.
The promoted `cpp-tck.timestamped-interaction-regulation-reenable-contract`,
`cpp-tck.timestamped-interaction-reenable-contract`,
`cpp-tck.timestamped-directed-interaction-reenable-contract`, and
`cpp-tck.timestamped-directed-interaction-regulation-reenable-contract` runners
add ordinary and directed timestamped Time Constrained/Time Regulation
re-enable boundaries on the same adapter-owned standard surface.
The promoted `cpp-tck.timestamped-attribute-update-ownership-transfer-contract`
runner adds the corresponding pure standard ownership-transfer boundary for a
queued timestamped update, ownership callbacks, reflection metadata, time
advancement, and retraction.
The promoted `cpp-tck.federation-list-services-contract` and
`cpp-tck.federate-lookup-lifecycle-contract` runners expose standard federation
execution/member reports and federate identity lookup boundaries as independently
selectable pure standard C++ contracts, with provider, FOM, endpoint, and callback
configuration supplied by the adapter.
The promoted `cpp-tck.order-type-controls-contract`,
`cpp-tck.receive-order-attribute-update-callback-cancellation-contract`, and
`cpp-tck.receive-order-interaction-callback-cancellation-contract` runners expose
standard order-control and receive-order callback-cancellation boundaries as
independently selectable pure standard C++ contracts, with provider, FOM, endpoint,
logical-time, and callback configuration supplied by the adapter.
The promoted `cpp-tck.next-message-request-contract`,
`cpp-tck.available-time-advances-inclusive-galt-contract`, and
`cpp-tck.time-bounds-queries-contract` runners expose standard Next Message Request,
available time-advance, and Query GALT/Query LITS boundaries as independently
selectable pure standard C++ contracts, with provider, FOM, endpoint,
logical-time, and callback configuration supplied by the adapter.
The promoted `cpp-tck.timestamped-interaction-mixed-advances-contract`,
`cpp-tck.timestamped-interaction-flush-queue-future-input-contract`, and
`cpp-tck.timestamped-interaction-tso-designator-terminalization-contract`
runners expose standard timestamped-interaction alternate-advance, future-input,
and retraction-terminalization boundaries as independently selectable pure
standard C++ contracts, with provider, FOM, endpoint, logical-time, and callback
configuration supplied by the adapter.
The promoted `cpp-tck.attribute-value-update-request-baseline-contract`,
`cpp-tck.object-class-attribute-value-update-request-baseline-contract`, and
`cpp-tck.attribute-value-update-response-contract` runners expose standard
object-instance/class request and ordinary provider-response boundaries as
independently selectable pure standard C++ contracts, with provider, FOM,
endpoint, and callback configuration supplied by the adapter.
The promoted
`cpp-tck.timestamped-directed-interactions-contract` runner exposes the
standard timestamped directed-interaction target-routing, time-role, retraction,
and Request Retraction surface as an independently selectable pure standard C++
contract, with FOM, endpoint, callback, and logical-time configuration supplied
by the adapter. The promoted
`cpp-tck.timestamped-directed-interaction-source-resignation-contract` and
`cpp-tck.timestamped-directed-interaction-source-resignation-fanout-contract`
runners expose standard queued directed-interaction source-resignation,
post-resignation retraction, target routing, and independent per-recipient fan-out
delivery as independently selectable pure standard C++ contracts, with FOM,
endpoint, callback, and logical-time configuration supplied by the adapter. The
promoted
`cpp-tck.timestamped-directed-alternate-advances-contract` runner exposes the
standard directed-interaction Flush Queue, TAR-available, and NMR-available
delivery surface as an independently selectable pure standard C++ contract, with
FOM, endpoint, callback, and logical-time configuration supplied by the adapter.
The
promoted `cpp-tck.service-report-local-delete-object-instance-contract`,
`cpp-tck.service-report-local-delete-object-instance-failure-contract`, and
`cpp-tck.service-report-delete-object-instance-failure-contract` runners expose
ordinary and local object-deletion MOM success/failure routes as independently
selectable pure standard C++ contracts, with MIM, FOM, endpoint, and callback
configuration supplied by the adapter. The
promoted `cpp-tck.service-report-delete-object-instance-failure` scenario
verifies standard MOM failure reports for invalid and stale object deletion,
including failure status, exception text, returned-argument encoding, serial
progression, and removal cleanup. The promoted
`cpp-tck.service-report-local-delete-object-instance-failure` scenario applies
the same standard MOM contract to invalid and stale `localDeleteObjectInstance`
calls around a successful local deletion. The timestamped companion verifies
that report delivery remains receive-order while
the ordinary interaction follows timestamped delivery, retraction, and grant
ordering.
The promoted `cpp-tck.attribute-value-update-request-baseline` scenario isolates
the standard object-instance request overload, verifies exact current-owner
callbacks and request tags, and suppresses requester-owned or unowned attributes.
The promoted `cpp-tck.object-class-attribute-value-update-request-baseline`
scenario separately expands the class overload over concrete subclass instances,
checks one callback per owner with inherited-attribute filtering, and verifies
requester-owned suppression. Focused evidence is recorded in
`.build/cpp-tck-all/attribute-value-update-request-baseline-focused.json` and
`.build/cpp-tck-all/object-class-attribute-value-update-request-baseline-focused.json`.
Both scenarios use only the official API and adapter-supplied FOM inputs.

The promoted `cpp-tck.attribute-value-update-response` scenario completes the
ordinary attribute-value request/response route, including provider callback
metadata, pre-response reflection suppression, returned value/tag, reliable
transport, producer identity, and empty region metadata through the official API.

The promoted `cpp-tck.regional-attribute-value-update-response-recheck` scenario
adds the standard DDM response-delivery boundary: moving the committed
subscriber region out of overlap suppresses the pending provider response, and
restoring overlap allows a fresh response with the expected value, tag,
transport, and producer metadata. It uses only the official API and adapter
inputs.

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

For day-to-day implementation selection, start with the short
[roadmap query card](../docs/planning/QUERY-CARD.md); use the detailed
[roadmap and test query guide](../docs/planning/QUERY-GUIDE.md) only when the
bounded command output points to a deeper reference:

The fastest resume command is the bounded dashboard:

    python tools/query_rti_work.py dashboard --summary --compact

It prints live roadmap and Catch2/mapping counts, source health, snapshot
freshness, the latest completed slice, one next work handoff, and only the
first three open-family queue rows. Use `--limit N` for a different preview or
`--json` for automation. The resume card caps descriptive family prose at 240
characters and leaves exact `work`, `focus`, `trace`, and `matrix` commands as
the expansion points. It is read-only and does not reopen or rewrite the
Requirements Lab.

When the indexed source and planned-row queues are exhausted, the dashboard
does not present the completed active pointer as new work. It presents a
bounded family selector with requirement/canonical-2025-section counts and
`ready --family <id>`/`work <id>` handles. The family-scoped `ready` response
adds bounded ID previews and direct requirement-to-section pairs. This is the
intended handoff for selecting the next C++ slice; the text card prints the
first bounded family as a recommendation and JSON exposes the same
`recommended_family_id`.

`ready --summary --compact` is intentionally the active implementation handoff:
it reports the next plan/test/lane, mapping counts, and a bounded preview of
requirement and section ids. It labels the queued roadmap family as
`roadmap_owner` and, when the broad active pointer differs, also prints
`active_pointer`; this keeps cross-family handoffs legible. Use `ready --json`
only when a script or review needs the complete mapping arrays; this keeps ordinary implementation resumes from expanding the
unchanged Requirements Lab into the working context. `work` remains useful for
family context and completed baselines, but it may intentionally retain a
historical baseline while `ready` advances to the next queued source/test.
When its source and planned-row queues are exhausted, `ready` prints up to
three bounded open-family options with exact `work <family>` commands, while
`next --pointer` prints the bounded family-selector command. Neither path
requires reopening the unchanged Requirements Lab.
The default `next --summary`/`next --json` form is a compatibility alias for
that same `ready` handoff; it no longer reports the completed active pointer as
new work. Source-only declarations remain visible through `unplanned` and the
diagnostic source queue, but they do not displace an indexed 2025 family by
default. Use `ready --include-source-only` or
`next --include-source-only` only when deliberately reconciling that queue.
Use `next --pointer` only for the explicit historical source-pointer view.
Once a family is selected, `ready --family <id>` keeps the handoff scoped to
that exact roadmap family, including when its source/planned queues are
exhausted. This avoids repeating the global shortlist during focused work.
`status --summary --compact` and `queue --summary --compact` include live,
plan-derived family counts for mapped cases, explicit no-standalone-surface
dispositions, unclassified rows, source-unlocated rows, canonical 2025
sections, and official C++ API surfaces. When a family names a baseline, the
same row includes its derived assertion/requirement/2025-section/API counts and
an exact `trace` command. This makes the first-pass family decision
self-contained; use `trace` or `matrix` only when the individual requirement
statements or complete arrays are needed.
Queue rows additionally publish `action_state` (`implementation`, `mapping`,
`source-reconciliation`, `external-review`, `new-case-needed`, or
`evidence-complete`). The diagnostic `source_drift` total is paired with an
`actionable_source_drift` count, so disabled/reconciled historical artifacts
cannot become false implementation heads. JSON queue output includes aggregate
`action_counts` and queued/evidence-complete family totals for scripts.
Use `roadmap [<query>] --summary --compact` to search family ids, titles, tags,
anchors, next-action text, Requirements-Lab ids, canonical 2025 subsections, or
official C++ API surfaces. It returns one bounded row per matching open family
with live Catch2 counts, next-test requirement/subsection previews, an exact
`next_source` handoff when one is queued, and copyable `work`, `focus`, and
family `matrix` commands. Add `--status all` only when historical completed
families are required.
Each exhausted-family row also includes a collapsed `next` action in
compact/summary output, so the immediate implementation decision is available
without reopening the long roadmap.
The active restore slices are useful examples of the same handoff: query
`timestamped-default-region-attribute-restore-multi-recipient` for its exact
123-assertion, 18-requirement, 14-section mapping or
`timestamped-regional-interaction-timed-restore` for its exact 55-assertion,
12-requirement, 15-section mapping, then use `ready` to select the next
unplanned source declaration. The source queue is authoritative; the historical
completion ledger may retain superseded rows for auditability.
Use `lanes --family <id> --unmapped --summary --compact` when a family needs a
new exact tag: it groups the indexed plan rows by lane, reports mapping state,
assertions, requirement/2025-section counts, and prints one next-test
`trace`/`focus`/CTest handle. Add `--disposition unclassified` to select only
rows that still need a Requirements-Lab mapping decision, or
`--disposition explicit` to review intentional no-standalone-surface
dispositions. In the combined view, `next` is reserved for an actionable
source-located implementation, mapping, or source-reconciliation handoff;
lanes containing only explicit dispositions expose `review_only` and a
separate `review_trace` handle instead. This is bounded discovery over the
checked-in index; it does not rescan the Requirements Lab. If a broad family has no single `next_lane`,
`work <family-id>` emits the same bounded lane-discovery command plus a scoped
`ready --family <family-id>` handoff so the next case is still selected locally.

The implementation plan is queryable by heading without printing its prose:

    python tools/query_rti_work.py plan --summary --compact
    python tools/query_rti_work.py plan "current indexed" --summary --compact

The filtered plan view returns heading line numbers and breadcrumb paths; use
the reported line to open only the relevant portion of
`docs/planning/IMPLEMENTATION-PLAN.md`.

Before editing a contract selector, run the bounded native drift guard:

    python tools/query_rti_work.py contract-drift --summary --compact

It checks every local `cpp/tests/...::Title` reference against the current
C++ declarations, reports exact path/title mismatches with a bounded sample,
and counts external portable-TCK symbols separately. It never resynchronizes
the Requirements Lab; a clean result is the fast prerequisite for focused
lane work.

When existing C++ rows are complete and a new mapped case is needed, use the
pinned 2025 requirement-gap card instead of scanning the Lab export:

    python tools/query_rti_work.py gaps --summary --compact --limit 8
    python tools/query_rti_work.py gaps hla-1516.1-2025:clause-7.2 --summary --compact --limit 8

The card reports total/mapped/uncovered requirement counts and the most
uncovered canonical document:clause subsections. Summary output omits full
statements; query one sample id with `requirement <id> --summary --compact`
when the normative text is needed. If no Catch2 row is mapped yet, that lookup
returns the bounded gap record (subsection, statement, and source) instead of
ending at an empty test result.

`focus` reports `needs-mapping` if any unclassified row remains, even when its
source case is implemented; `complete` is reserved for lanes with no executable
candidate, unclassified row, source drift, or indexed execution gate.
`execution-blocked` records a source/build-integrity gate without discarding
the trace handles. Explicit no-standalone-surface dispositions are still
queryable. For example, the declared-custom-transport forms use one bounded
taxonomy handle:

    python tools/query_rti_work.py focus custom-transportation --summary --compact
    python tools/query_rti_work.py unmapped --lane custom-transportation --disposition explicit --summary --compact

For one test-to-standard lookup, keep the query exact and bounded. The Auto
Provide service-report lane is the current example: `focus` prints its owner,
source, requirements, clauses, and executable filter; `trace`/`matrix` then
show the direct requirement-to-subsection rows. Both reverse views accept an
exact C++ API surface id when you need to find every mapped test exercising a
specific official method. Use the printed `ctest_filter`
instead of a broad label when traceability checks carry historical selectors.
The bounded `queue` and `next` views include the live indexed assertion total
for any pointed lane, so a stale prose snapshot cannot obscure the current
focused-test size.
The canonical Catch2 plan mapping field is
`selected_requirements_lab_requirement_ids`; the query check rejects the
legacy `requirements_lab_requirement_ids` spelling so a mapped case cannot be
silently counted as unclassified. The bounded filesystem/MOM lifecycle lane is
queryable with `focus service-report-file-lifecycle`, and `matrix` prints its
per-test source, assertion, requirement, subsection, and API counts. Matrix
JSON rows additionally expose `requirement_section_mappings`, preserving each
direct Lab-requirement → canonical 2025 subsection pair.
The public fresh-registry application-value companion is likewise directly
queryable with `focus public-process-restart-application-value-focused`; its
dashboard/recent entry carries the source line, 82 assertions, nine
Requirements-Lab anchors, five canonical sections, and 18 official C++ API
surfaces.

    python tools/query_rti_work.py work --summary --compact
    python tools/query_rti_work.py ready --summary --compact
    python tools/query_rti_work.py focus --summary --compact
    python tools/query_rti_work.py next --summary
    python tools/query_rti_work.py next --pointer
    python tools/query_rti_work.py recent --summary --compact --limit 20
    python tools/query_rti_work.py search timestamped-directed-retraction --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped Delete Object Instance reconstitutes on retraction and removes before grant" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped Delete Object Instance reconstitutes on retraction and removes before grant" --summary --compact
    python tools/query_rti_work.py matrix api.2025.cpp.rtiambassador.querylogicaltime.cb29c063787c --summary --compact
    python tools/query_rti_work.py matrix header-binding-shell --summary --compact
    python tools/query_rti_work.py matrix time-support --summary --compact
    python tools/query_rti_work.py matrix hla-1516.1-2025:clause-9.12 --summary --compact --limit 20
    python tools/query_rti_work.py trace "Embedded attribute relevance advisories use subscriptions when known-class policy is disabled" --summary --compact
    python tools/query_rti_work.py search known-class-disabled --summary --compact
    python tools/query_rti_work.py trace "Embedded attribute relevance advisories honor known class when the static policy is enabled" --summary --compact
    python tools/query_rti_work.py search known-class-enabled --summary --compact
    python tools/query_rti_work.py trace "Embedded update-rate lookup ignores passive regional subscriptions" --summary --compact
    python tools/query_rti_work.py search update-rate-reduction --summary --compact
    python tools/query_rti_work.py trace "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
    python tools/query_rti_work.py search transportation-handle-stability --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped Update Attribute Values invocations through MOM interaction (restored baseline copy)" --summary --compact
    python tools/query_rti_work.py search restored-baseline-copy --summary --compact
    python tools/query_rti_work.py trace "Embedded regional Provide Attribute Value Update reports before callback delivery (restored baseline copy)" --summary --compact
    python tools/query_rti_work.py trace "Embedded three-dimensional regional object attributes require complete overlap" --summary --compact
    python tools/query_rti_work.py search complete-overlap --summary --compact
    python tools/query_rti_work.py trace "Embedded regional Request Attribute Value Update filters 2025 owner solicitations (restored baseline copy)" --summary --compact
    python tools/query_rti_work.py search regional-request-filtering --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction (restored baseline copy)" --summary --compact
    python tools/query_rti_work.py search timestamped-regional-attribute-update-failure --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records failed timestamped Update Attribute Values invocations (restored baseline copy)" --summary --compact
    python tools/query_rti_work.py search timestamped-attribute-update-failure --summary --compact
    python tools/query_rti_work.py trace "RTIambassador removes a regional subscription through a configured process endpoint" --summary
    python tools/query_rti_work.py lane request-retraction-suppressed-interaction --summary --compact
    python tools/query_rti_work.py test "Embedded suppressed timestamped interaction callback does not request retraction" --summary --compact
    python tools/query_rti_work_regression.py
    python tools/query_rti_work.py lane transport --summary --compact
    python tools/query_rti_work.py test "Private process transport exchanges framed data after endpoint handshake" --summary --compact
    python tools/query_rti_work.py test "Private process service binds create join and receive-order interaction to the federation registry" --summary --compact
    python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
    python tools/query_rti_work.py lane process-boundary --summary --compact
    python tools/query_rti_work.py focus process-modify-lookahead-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-modify-lookahead-focused --summary --compact
    python tools/query_rti_work.py focus process-modify-lookahead-grant-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
    python tools/query_rti_work.py check --lane process-modify-lookahead-grant-focused --summary --compact

The process Available-form temporal lane has the same bounded handles:

    python tools/query_rti_work.py focus process-time-advance-available-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-time-advance-available-focused --summary --compact

The process NMR/NMRA temporal lane is independently selectable:

    python tools/query_rti_work.py focus process-time-advance-next-message-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassador requests next message advances through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador requests next message advances through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-time-advance-next-message-focused --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests next message advances through a configured process endpoint$" --output-on-failure

The queued-TSO companion keeps the multi-federate scheduler proof separate:

    python tools/query_rti_work.py focus process-time-advance-next-message-queued-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
    python tools/query_rti_work.py check --lane process-time-advance-next-message-queued-focused --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors select queued timestamped process messages for next-message advances$" --output-on-failure

The current public process object-instance lookup slice is independently
queryable and deliberately small:

    python tools/query_rti_work.py focus object-instance-lookup --summary --compact
    python tools/query_rti_work.py trace "RTIambassador resolves known object-instance names and handles through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix object-instance-lookup --summary --compact
    python tools/query_rti_work.py check --lane object-instance-lookup --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador resolves known object-instance names and handles through a configured process endpoint$" --output-on-failure

It records 18 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, five
Requirements-Lab anchors, two canonical 2025 subsections, and six official C++
API surfaces. Use the exact lane/title handles above; do not reopen the full
Requirements-Lab plan to locate this slice.

The current public process reverse-FOM lookup slice is independently queryable:

    python tools/query_rti_work.py focus reverse-fom-lookup --summary --compact
    python tools/query_rti_work.py trace "RTIambassador reports reverse FOM lookup errors through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix reverse-fom-lookup --summary --compact
    python tools/query_rti_work.py check --lane reverse-fom-lookup --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador reports reverse FOM lookup errors through a configured process endpoint$" --output-on-failure

The lane now records four source-located cases and 76 aggregate assertions
(m102 contributes 20; m103 contributes 24; m104 contributes 16; m105
contributes 16) under `HLA_EVOKED` and `HLA_IMMEDIATE`. The m105 case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13647` records 16 assertions,
maps six Requirements-Lab anchors to canonical 2025 subsections `9.1.2`,
`10.19`, and `10.20.4`, and exercises the unknown-name/invalid-handle error
fence across four official C++ API surfaces. Use these exact lane/title handles
for bounded declaration-management work; class, attribute, parameter,
dimension, and transportation reverse lookups remain individually traceable.

The newest process multi-recipient callback-ordering slice is independently
queryable:

    python tools/query_rti_work.py focus process-multi-recipient-callback-ordering --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix process-multi-recipient-callback-ordering --summary --compact
    python tools/query_rti_work.py check --lane process-multi-recipient-callback-ordering --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint$" --output-on-failure

The m107 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:13968`
records 79 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps ten Requirements-Lab anchors to six
canonical 2025 subsections and five official C++ API surfaces, and checks
per-recipient FIFO interaction delivery, preserved tags/parameters/producer
identity, one callback per Evoke Callback, and sender exclusion. Keep
immediate, callback-disable, timestamped/region/directed fanout, package/JUnit,
review, validation, interoperability, and conformance in separate lanes.

The newest process TSO/DDM slice is independently queryable:

    python tools/query_rti_work.py focus timestamped-process-regional-interaction --summary --compact
    python tools/query_rti_work.py trace m109.embedded-process-tso-regional-interaction --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-process-regional-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a timestamped regional interaction through a configured process endpoint$" --output-on-failure

The m109 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:15057`
records 71 `HLA_EVOKED` assertions, maps 28 Requirements-Lab anchors to 16
canonical 2025 subsections, and exercises 20 official C++ API surfaces. It
proves overlap-qualified timestamped regional process delivery, time-advance
gating, conveyed source-region metadata, and timestamp/order/retraction
reconstruction. Keep callback-disable, directed, relaxed-DDM, package/JUnit,
review, validation, interoperability, and conformance in separate lanes.

The preceding process DDM slice is independently queryable:

    python tools/query_rti_work.py focus process-multi-recipient-regional-interaction --summary --compact
    python tools/query_rti_work.py trace m108.embedded-process-multi-recipient-regional-interaction --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-multi-recipient-regional-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint$" --output-on-failure

The m108 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:14414`
records 126 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps thirteen
Requirements-Lab anchors to seven canonical 2025 subsections, and exercises
thirteen official C++ API surfaces. It proves overlap-filtered delivery to two
disjoint regional recipients, source-region metadata through Convey Region
Designator Sets, and sender exclusion. Timestamped/retraction, directed,
relaxed-DDM, callback-disable, package/JUnit, review, validation,
interoperability, and conformance remain separate lanes.

The preceding process available-dimensions hierarchy slice is independently queryable:

    python tools/query_rti_work.py focus process-available-dimensions-hierarchy --summary --compact
    python tools/query_rti_work.py trace process-available-dimensions-hierarchy --summary --compact
    python tools/query_rti_work.py matrix process-available-dimensions-hierarchy --summary --compact
    python tools/query_rti_work.py check --lane process-available-dimensions-hierarchy --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador resolves available FOM dimensions through a configured process endpoint$" --output-on-failure

The m106 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:13795`
records 28 assertions under both callback models. It exercises the public
object/interaction hierarchy queries through the process endpoint, including
inherited object dimensions, an empty interaction dimension set, unknown-class
results, and official invalid-handle mapping. The row maps four Lab anchors to
`hla-1516.1-2025:clause-9.1.2` and two official C++ API surfaces. Keep process
region behavior, multi-federate ordering, and conformance evidence as separate
follow-on lanes; query this exact lane before opening another implementation
slice.

    python tools/query_rti_work.py lane callback-controls --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador publishes object-class attributes and registers an object through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador reserves a name and registers a named object through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes ordinary interaction declarations through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "RTIambassador projects the 2025 region lifecycle and regional registration through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes a remote regional subscription and scoped update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador preserves a timestamped regional update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador removes a regional subscription through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador suppresses a disjoint regional update through a configured process endpoint" --summary --compact
     python tools/query_rti_work.py focus process-time-advance-malformed-encoding --summary --compact
     python tools/query_rti_work.py trace "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
     python tools/query_rti_work.py matrix "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
     ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects malformed logical-time encoding through a configured process endpoint$" --output-on-failure
     python tools/query_rti_work.py focus process-time-advance-federation-scheduler --summary --compact
     python tools/query_rti_work.py trace "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
     python tools/query_rti_work.py matrix "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
     ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors coordinate deferred process time advances through the federation scheduler$" --output-on-failure
     python tools/query_rti_work.py focus process-tso-interaction-before-grant --summary --compact
     python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
     python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
     ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a deferred timestamped process interaction before the grant$" --output-on-failure
     python tools/query_rti_work.py focus process-tso-attribute-before-grant --summary --compact
     python tools/query_rti_work.py trace "Private process service releases timestamped Update Attribute Values before a constrained grant" --summary --compact
     python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
     python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
     ctest --test-dir <build-dir> -C Debug -R "^(umbra\.process_boundary_private\.catch2\.Private process service releases timestamped Update Attribute Values before a constrained grant|umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a deferred timestamped process attribute update before the grant)$" --output-on-failure
    python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
    python tools/query_rti_work.py lane rti.service.subscribe-object-class-attributes --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Update Attribute Values through a configured process endpoint" --summary --compact
     python tools/query_rti_work.py test "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback" --summary --compact
     python tools/query_rti_work.py source cpp/tests/external_2010_fom_catch2.cpp --summary --limit 20
     python tools/query_rti_work.py requirement hla-1516.1-2025:clause-4.1.1 --summary --limit 20
     python tools/query_rti_work.py section hla-1516.1-2025:clause-4.1.1 --summary --limit 20
     python tools/query_rti_work.py unmapped --disposition unclassified --summary --limit 20
     python tools/query_rti_work.py coverage --lane process-boundary --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_test_installable_package
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-timestamped --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_parameterized_consumer$" --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-parameterized --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_connection_loss_consumer$" --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_object_registration_consumer$" --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-object-registration --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_named_registration_consumer$" --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-named-registration --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_attribute_update_consumer$" --output-on-failure
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-attribute-update --output-on-failure
    python tools/verify_process_package_lanes.py --ctest ctest --test-dir <build-dir>/package-smoke-consumer --index docs/planning/ROADMAP-INDEX.json --config Debug
    python tools/query_rti_work.py test "Embedded transport loss applies the bounded automatic NoAction forced-resign policy" --summary --compact
    python tools/query_rti_work.py next --pointer
    python tools/query_rti_work.py focus process-boundary --summary --compact
    python tools/query_rti_work.py test "Embedded terminal timestamped deletion tombstone releases its object name" --summary --compact
    python tools/query_rti_work.py test "Embedded Flush Queue Request admits TSO input queued after submission" --summary --compact
    python tools/query_rti_work.py test "Embedded timestamped Update Attribute Values flushes queued passels with optimistic time" --compact
    python tools/query_rti_work.py test "Embedded timestamped Update Attribute Values honors available and next-message available grants" --compact
    python tools/query_rti_work.py test "Embedded timestamped regional Update Attribute Values carries recipient-gated regions across mixed fanout" --compact
    python tools/query_rti_work.py test "Embedded regional timestamped attribute updates deliver before TAR and NMR grants" --compact
    python tools/query_rti_work.py focus timestamped-regional-attribute-timed-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-attribute-timed-restore --summary --compact
    python tools/query_rti_work.py focus timestamped-regional-attribute-timed-restore-multi-recipient --summary --compact
    python tools/query_rti_work.py trace "Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-attribute-timed-restore-multi-recipient --summary --compact
    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-multi-resignation-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-multi-resignation-state --summary --compact
    python tools/query_rti_work.py recent --lane regional-automatic-provision-switch-mutation --summary --compact --limit 5
    python tools/query_rti_work.py focus auto-provide-service-report --summary --compact
    python tools/query_rti_work.py trace "Embedded Auto Provide service reporting records empty tag before its callback" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Auto Provide service reporting records empty tag before its callback" --summary --compact
    python tools/query_rti_work.py focus timestamped-directed-interaction-alternate-advance --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants" --summary --compact
    python tools/query_rti_work.py focus joined-federate-mom-federate-state --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAfederateState follows save and restore callbacks" --summary --compact
    python tools/query_rti_work.py matrix "Embedded joined-federate MOM HLAfederateState follows save and restore callbacks" --summary --compact
    python tools/query_rti_work.py focus timestamped-delete-object-instance-failure-service-report --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records failed timestamped Delete Object Instance invocations" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records failed timestamped Delete Object Instance invocations" --summary --compact
    python tools/query_rti_work.py focus timestamped-delete-object-instance-sender-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records timestamped Delete Object Instance before removal callback" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records timestamped Delete Object Instance before removal callback" --summary --compact
    python tools/query_rti_work.py focus timestamped-delete-object-instance-time-regulated-sender-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-time-regulated-sender-file --summary --compact
    python tools/query_rti_work.py focus timestamped-delete-object-instance-service-report-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-service-report-interaction --summary --compact
    python tools/query_rti_work.py focus timestamped-delete-object-instance-failure-mom-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-failure-mom-interaction --summary --compact
    python tools/query_rti_work.py check --compact

Use `coverage --lane <exact-tag> --summary --compact` (or
`coverage --family <roadmap-family-id> --summary --compact`) to see bounded
requirement, subsection, direct-pair, and source-location counts for one focus
lane/family; the source-location count keeps retained historical rows distinct
from executable Catch2 cases. Pair totals are deduplicated by
`(lab_requirement_id, document_id:clause_id)` and JSON distinguishes resolved
from unresolved rows.

Use `matrix [<exact-handle>] --summary --compact` when the next question is
"which tests cover this lane, requirement, or 2025 subsection?" It prints one
bounded row per matching test with the exact source location, status, assertion
count, requirement IDs, canonical `document_id:clause_id` subsection keys, and
the direct `requirement_section_mappings` pairs. Omit the handle to use the
active indexed lane. Use `--json` when a script needs the complete arrays and
pairs; use `trace` for one direct test-to-requirement row.

`status --summary --compact` reports the live plan/source queues and splits
unmapped cases into explicit dispositions versus unclassified rows, so an
intentional internal-boundary decision is not mistaken for missing traceability.
It also reports the C++ source-index health; `attention` is a source-recovery
signal (use the bounded `git diff --check` hint), not a reason to rescan or
revise the unchanged Requirements Lab.

When an indexed source lane is exhausted, `work` and `next --pointer` expose
the global unplanned-source count and deterministic head when one exists. The
default `ready`/`next` implementation handoff does not select that source-only
head; pass `--include-source-only` when the explicit reconciliation task is
intended. If that queue is empty, they expose the overlapping planned-row head
with its plan id, requirement/section/API counts, and a copyable `trace`
command; no full queue dump or Requirements-Lab rescan is needed.

The Lab is optional for ordinary source work. When a requirement mapping or
source-path contract changes, start with:

    python tools/requirements_lab.py --help
    python tools/requirements_lab_sidecar.py --help

Keep exports and unreviewed evidence under .compliance/. Read the
[compliance workflow](../docs/testing/COMPLIANCE-WORKFLOW.md) before promoting
or describing any evidence.

Only when the pinned export changes, or when a numbering/semantic discrepancy
is under review, compare a fresh edition-scoped export with the pinned bundle
using the read-only re-sync report. Ordinary implementation work should stay
on the checked-in index and mappings above:

    python tools/requirements_lab.py resync \
      --baseline .compliance/corpus-bundle.json \
      --candidate .tmp/corpus-bundle-resync-2026-08-24.json \
      --edition 2025 --fail-on-diff

The report includes normalized document digests and separate content,
regenerated-ID, and ordinal-numbering deltas. An ordinal move is reported even
when the normative text and opaque record ID stay unchanged; this prevents a
requirements-numbering change from disappearing inside the semantic
fingerprint. Each selected document is also checked for malformed requirement
ordinals (missing, invalid, duplicate, or non-contiguous values), and those
findings are included in the report's `ordinal_integrity` fields. This catches
an exporter numbering defect even when there is no trustworthy pairwise record
to compare. `--fail-on-diff` is suitable for a review/CI gate; without it,
the command always prints the complete report and exits zero. When an
intentional working-tree overlay adds semantic records, use
`--fail-on-numbering-drift` to gate only regenerated IDs, ordinal moves, or
ordinal-integrity findings. The focused offline guard also exercises the
ordinal-only, combined ID/ordinal, and malformed-ordinal cases:

    python tools/requirements_lab_resync_regression.py

The active native Catch2 planning catalog has its own non-mutating reference
guard. It resolves every supplied requirement/API ID across the 2025 bundle
and checks each human-readable selector against the C++ test sources:

    python tools/requirements_lab.py check-plan \
      --bundle .compliance/corpus-bundle.json

The observation ledger has a separate guard. It freezes the RL-001..RL-157
historical section blocks and requires every post-RL-157 recurrence (including
an old issue found still unresolved/not fixed, or a failed expected mitigation)
to use the next identifier and cite an existing earlier observation. The
command prints that next local identifier:

    python tools/requirements_lab.py check-observations

An unchanged export or additive test slice is recorded as an audit/local
coverage note and does not consume the next recurrence identifier. Only a
reproduced Lab or Umbra-consumer go-back should append the next `RL-###`
entry.

### Query the implementation roadmap and C++ mappings

Use the read-only work index for a short next-step report and exact test-to-
standard lookup. It joins the pinned Catch2 plan to the pinned 2025 bundle's
clause/subsection and source metadata; it does not resync or modify the Lab:
`next --summary` is the compact `ready` handoff: it includes one exact planned
or source-backed slice (or bounded family choices), its lane/test selector,
canonical 2025 mapping counts, and copyable trace/focus/check handles;
`next --json` exposes the same bounded object to scripts. Use
`next --pointer` only for the historical parent-pointer view. Run `check --compact` before a focused build to
catch stale test/source, requirement, section, or lane handles. The default
check validates the live roadmap, plan, requirement, and source index; it
skips the append-only completion ledger so historical source splits do not
block current work. Use `check --historical` for the strict ledger audit. Its
text output prints only a small sample when the checkout has many drift rows;
use `check --json` for the complete diagnostic. If the selector is an existing
regression used as the starting point for a new slice, it is labeled
`baseline_test` and reports `test_pointer=complete`; it is not presented as a
test to add or rerun. For an architectural slice that still needs its first
test, it reports the explicit `next_work_id`, `next_work_status`, and
`next_work_query` instead of inventing a selector. A completed, unlabeled
selector is reported as a roadmap-index warning by `check`, so stale pointers
do not silently become the work queue. `recent --lane <exact-tag>` narrows the
completion ledger to one focus lane. `status --compact` and `item --compact`
keep broad roadmap items bounded by reporting tag/match counts rather than
expanding every tag.
The live check also rejects repeated requirement, API, section, or tag
selectors within one Catch2 plan row, keeping reverse mappings deterministic.

Follow `work` with `focus [<exact-catch2-tag>] --summary --compact` to get a
bounded lane decision card: implemented cases, source-located executable
candidates, historical source drift, requirement/section counts, and the
copyable Catch2/CTest/JUnit handles. With no tag, `focus` follows the active
indexed lane. Use `focus --json` for automation; it never triggers a Lab scan.

For an iteration-local integrity gate, add `check --lane <exact-tag>`; it
limits live test/source validation to that lane. Add `--historical` when the
lane's completion-ledger rows should be audited too; unlocated historical rows
then remain visible as source drift. Historical line-number drift within the
same source file is tolerated because query output derives the live `TEST_CASE`
location; missing declarations, wrong-file pointers, and unknown sections still
fail. The unscoped `check` remains the whole-live-plan reconciliation check;
`check --historical` is the slower append-only ledger audit.

`test "<exact TEST_CASE title>" --summary --compact` is the bounded traceability
view: it prints the source location, API-surface IDs, service/callback tags,
Requirements-Lab IDs, and canonical 2025 section handles without expanding the
full plan; long API-surface prose is abbreviated in this view. Use `--json` (or
non-summary `--compact`) when the complete prose is required. For a direct
one-record relationship, use `trace`: it accepts an
exact plan id/title, Lab requirement id, canonical 2025 section key, or exact
Catch2 lane tag and prints `lab_requirement_id -> document_id:clause_id` rows
beside the C++ source location. It never falls back to fuzzy search; use
`search` explicitly for discovery. Use `requirement` or `section` for the
reverse lookup from a Lab requirement or standard subsection to its mapped
tests.

    python tools/query_rti_work.py trace umbra-cpp-process-endpoint-regional-unsubscribe-integration --json
    python tools/query_rti_work.py trace hla-1516.1-2025:clause-9.7.5 --summary --compact

    python tools/query_rti_work.py status --compact
    python tools/query_rti_work.py ready --summary --compact
    python tools/query_rti_work.py work --summary --compact
    python tools/query_rti_work.py next --compact
    python tools/query_rti_work.py next --summary
    python tools/query_rti_work.py next --json
    python tools/query_rti_work.py check --lane transport --summary --compact
    python tools/query_rti_work.py check --compact
    python tools/query_rti_work.py check --historical --compact
    python tools/query_rti_work.py item multi-federate-callback-ordering --compact
    python tools/query_rti_work.py plan
    python tools/query_rti_work.py coverage
    python tools/query_rti_work.py lane multi-federate-callback-ordering
    python tools/query_rti_work.py lane durable-save
    python tools/query_rti_work.py lane state-image --compact
    python tools/query_rti_work.py lane tso-directed-interaction-state --compact
    python tools/query_rti_work.py lane tso-attribute-update-state --compact
    python tools/query_rti_work.py lane tso-object-deletion-state --compact
    python tools/query_rti_work.py lane timestamped-regional-attribute-update --compact
    python tools/query_rti_work.py lane process-restart-regional-pending-attribute-value-update-provider-departure --summary
    python tools/query_rti_work.py lane public-process-restart-regional-pending-attribute-value-update-provider-departure --summary
    python tools/query_rti_work.py lane timestamped-regional-request-provider-response --compact
    python tools/query_rti_work.py test "Embedded regional Request Attribute Value Update supports a timestamped provider response" --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-live-restore-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-live-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-live-restore-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-live-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-multi-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-delete-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-if-available-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-continuation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-mixed-candidate-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-mixed-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-mixed-pre-delivery-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-regular-candidate-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-regular-retained-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-regular-retained-pre-delivery-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-resignation-matrix --summary
    python tools/query_rti_work.py lane timestamped-regional-attribute-timed-restore-multi-recipient --compact
    python tools/query_rti_work.py lane process-restart --summary
    python tools/query_rti_work.py lane process-restart-directed-interaction-declaration --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-declaration --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-routing --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-routing --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-ownership-handoff --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-ownership-handoff --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore follows directed by-ownership target handoff and receive-order delivery under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-callback --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-callback --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses stale directed TSO callback after by-ownership handoff under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-fanout-positive-negative --compact
    python tools/query_rti_work.py test "Filesystem fresh-registry directed TSO fan-out preserves an eligible recipient and suppresses an unsubscribed recipient" --compact
    python tools/query_rti_work.py lane process-restart-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane public-process-restart-pending-attribute-value-update --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending object-instance Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-class-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane public-process-restart-class-pending-attribute-value-update --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-regional-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane process-restart-regional-pending-attribute-value-update-negative --summary
    python tools/query_rti_work.py lane public-process-restart-regional-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane public-process-restart-regional-pending-attribute-value-update-negative --summary
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending regional object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py section hla-1516.1-2025:clause-9.13.1 --summary --limit 20
    python tools/query_rti_work.py lane timestamped-default-region-attribute-retract-alternate-advance --compact
    python tools/query_rti_work.py test "Embedded timestamped default-region attribute updates retract before TARA and NMRA grants" --compact
    python tools/query_rti_work.py lane time-advance-request-available --summary
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-parameter-projection --compact
    python tools/query_rti_work.py lane process-restart-regional-interaction-tso-ddm --compact
     python tools/query_rti_work.py test "Filesystem fresh-registry restore preserves one timestamped regional interaction payload and source-region snapshot" --compact
     python tools/query_rti_work.py lane public-process-restart-regional-interaction-tso-ddm --compact
     python tools/query_rti_work.py test "Embedded public restore rehydrates queued timestamped regional interaction and preserves filesystem service-report identity" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds queued timestamped regional interaction and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-regional-attribute-update-tso-ddm --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds queued timestamped regional attribute update and preserves report-file lifetimes" --compact
     python tools/query_rti_work.py lane durable-save-regional-pending-attribute-value-update-response-retraction --compact
     python tools/query_rti_work.py unlocated --summary --limit 20
     python tools/query_rti_work.py lane durable-save-regional-pending-attribute-value-update-regular-response --compact
     python tools/query_rti_work.py test "Embedded regional Request Attribute Value Update supports a 2025 provider response" --compact
     python tools/query_rti_work.py lane durable-save-regional-pending-attribute-value-update-regular-response-no-replay --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore does not replay a delivered receive-order regional provider response under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py test "Embedded object-class Request Attribute Value Update supports a timestamped provider response under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py test "Embedded transport loss drains a cutoff timestamped deletion to each pending survivor" --compact
     python tools/query_rti_work.py test "Embedded local deletion isolates queued timestamped attribute deliveries per recipient" --compact
     python tools/query_rti_work.py test "Embedded local deletion isolates queued timestamped object removals per recipient" --compact
     python tools/query_rti_work.py lane regional-automatic-provision --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide solicits overlap-qualified owners and suppresses stale disjoint work" --compact
     python tools/query_rti_work.py lane regional-automatic-provision-immediate --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide preserves synchronous discovery-before-provider ordering under HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane regional-automatic-provision-response --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide provider response reflects one scoped value under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane regional-automatic-provision-timestamped-response --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide timestamped response reflects once and exposes valid retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-multi-provider --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide fans out one request across two provider owners under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-switch-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide suppresses queued work after switch mutation and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-timestamped-switch-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide preserves a queued timestamped response across switch mutation and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-switch-admission-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide consumes queued discovery work when switch changes before provider admission and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-timestamped-switch-admission-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide fences timestamped discovery work before provider admission and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-multi-source-region --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide separates independent source regions and provider callbacks under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-relaxed-ddm --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide applies Allow Relaxed DDM to touching source projections and suppresses positive gaps under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-object-deletion-tso --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds queued timestamped object deletion and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-object-deletion-retraction --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore retracts delivered timestamped object deletion for one recipient under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-declaration --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rehydrates mixed interaction declaration and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-fanout --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds directed TSO fan-out and retracts only the delivered recipient under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-parameter-projection --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry parameterized directed TSO preserves parameter projection and timestamped order metadata under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-attribute-ownership-acquisition --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending regular ownership release callback and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-attribute-ownership-acquisition-if-available --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending If Available ownership callback and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-pending-attribute-ownership-query --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds a pending Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-pending-attribute-ownership-query-unowned --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds a pending unowned Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-pending-rti-owned-attribute-ownership-query --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds a pending RTI-owned Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-attribute-ownership-negotiated-owner-confirmation --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending negotiated owner confirmation and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-attribute-ownership-negotiated-if-available-owner-confirmation --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending negotiated If Available owner confirmation and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-attribute-ownership-mixed-negotiated-ownership --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds mixed negotiated ownership confirmations and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-negotiated-confirmation-delivered --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves delivered negotiated owner confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-negotiated-if-available-confirmation-delivered --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves delivered negotiated If Available confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-attribute-ownership-negotiated-owner-confirmation --compact
    python tools/query_rti_work.py test "Filesystem state image restores pending negotiated owner confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-attribute-ownership-negotiated-if-available-owner-confirmation --compact
    python tools/query_rti_work.py test "Filesystem state image restores pending negotiated If Available owner confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-attribute-ownership-mixed-negotiated-ownership --compact
    python tools/query_rti_work.py test "Filesystem state image restores a mixed negotiated ownership ledger in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-negotiated-confirmation-delivered --compact
    python tools/query_rti_work.py test "Filesystem state image preserves delivered negotiated owner confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-negotiated-if-available-confirmation-delivered --compact
    python tools/query_rti_work.py test "Filesystem state image preserves delivered negotiated If Available confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-mixed-confirmation-delivered --compact
    python tools/query_rti_work.py test "Filesystem state image preserves mixed delivered negotiated confirmations in a fresh registry" --compact
    python tools/query_rti_work.py lane public-process-restart-mixed-confirmation-delivered --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves mixed delivered negotiated confirmations and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-asymmetric-mixed-confirmation --compact
    python tools/query_rti_work.py test "Filesystem state image restores an asymmetric mixed negotiated confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane public-process-restart-asymmetric-mixed-confirmation --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves asymmetric mixed negotiated confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-asymmetric-mixed-confirmation-reverse --compact
     python tools/query_rti_work.py test "Filesystem state image restores the reverse asymmetric mixed negotiated confirmation in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-asymmetric-mixed-confirmation-reverse --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves reverse asymmetric mixed negotiated confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-malformed-mixed-confirmation --compact
     python tools/query_rti_work.py test "Federation restore rejects malformed mixed negotiated confirmation images" --compact
     python tools/query_rti_work.py lane process-restart-attribute-transportation-type-change --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending attribute transportation-type change in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-attribute-transportation-type-change --compact
      python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending attribute transportation-type change and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-interaction-transportation-type-change --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending interaction transportation-type change in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-transportation-type-change --compact
      python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending interaction transportation-type change and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-interaction-declaration --compact
     python tools/query_rti_work.py test "Filesystem state image restores a published interaction declaration in a fresh registry" --compact
     python tools/query_rti_work.py lane process-restart-interaction-transportation-type-override --compact
     python tools/query_rti_work.py test "Filesystem state image restores a committed interaction transportation-type override in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-transportation-type-override --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves committed interaction transportation-type override and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-interaction-mixed-override --compact
     python tools/query_rti_work.py test "Filesystem state image restores a mixed interaction override with a subscription for multiple federates" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-mixed-override --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves mixed interaction override and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-declaration --compact
     python tools/query_rti_work.py test "Filesystem state image restores a directed interaction publication and subscription for multiple federates" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-declaration --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rehydrates directed interaction declaration and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-routing --compact
     python tools/query_rti_work.py test "Filesystem state image restores a directed interaction target and receive-order route" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-routing --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds directed target routing and receive-order delivery under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-ownership-handoff --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore follows directed by-ownership target handoff and receive-order delivery under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-callback --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-callback --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses stale directed TSO callback after by-ownership handoff under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-tso-fanout-positive-negative --compact
     python tools/query_rti_work.py test "Filesystem fresh-registry directed TSO fan-out preserves an eligible recipient and suppresses an unsubscribed recipient" --compact
     python tools/query_rti_work.py lane tso-queue-state --compact
    python tools/query_rti_work.py lane ownership-ledger-state --compact
    python tools/query_rti_work.py lane process-restart-ownership-assumption-search --compact
    python tools/query_rti_work.py test "Filesystem state image restores ownership assumption search state and continues with a newly eligible federate" --compact
    python tools/query_rti_work.py lane public-process-restart-ownership-assumption-search --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending Request Attribute Ownership Assumption through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-ownership-acquisition-cancellation --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending ownership-acquisition cancellation in a fresh registry" --compact
     python tools/query_rti_work.py lane process-restart-divestiture-if-wanted --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending Divestiture If Wanted notification in a fresh registry" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending Divestiture If Wanted notification through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending Confirm Divestiture notification in a fresh registry" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending Confirm Divestiture notification through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane confirm-divestiture --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-fanout --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore fans out pending Confirm Divestiture notifications to three recipients through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-mixed-fanout --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore fans out mixed regular and If Available Confirm Divestiture notifications through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-resignation --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses a resigned Confirm Divestiture candidate and preserves the surviving recipient" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-post-confirmation-resignation --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses a post-confirmation resigned Confirm Divestiture recipient while preserving the surviving recipient" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses one post-confirmation resigned Confirm Divestiture recipient while preserving two surviving recipients" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds an eligible ownership-assumption recipient beside two Confirm Divestiture notifications" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py unlocated --summary --limit 20
    python tools/query_rti_work.py lane object-visibility-state --compact
    python tools/query_rti_work.py lane object-lifecycle-state --compact
    python tools/query_rti_work.py lane membership-update-state --compact
    python tools/query_rti_work.py lane membership-reflection-state --compact
    python tools/query_rti_work.py lane membership-lifecycle-state --compact
    python tools/query_rti_work.py lane membership-interaction-send-state --compact
    python tools/query_rti_work.py lane membership-interaction-receive-state --compact
    python tools/query_rti_work.py lane object-name-reservation-state --compact
    python tools/query_rti_work.py lane object-class-declaration-state --compact
    python tools/query_rti_work.py lane synchronization-state --compact
    python tools/query_rti_work.py lane region-state --compact
    python tools/query_rti_work.py lane application-value-state --compact
    python tools/query_rti_work.py lane pending-application-request-state --compact
    python tools/query_rti_work.py lanes --summary --limit 40
    python tools/query_rti_work.py search "regional timestamped" --summary
    python tools/query_rti_work.py unmapped --summary --limit 20
    python tools/query_rti_work.py test "federation save"
    python tools/query_rti_work.py requirement clause-6.12.4
    python tools/query_rti_work.py requirement hla-1516.1-2025:clause-8.22.3 --compact
    python tools/query_rti_work.py check

Append `--json` to any command for scripting (before or after the subcommand);
`next --pointer` reports the highest-priority open indexed parent pointer,
while default `next` joins the single ready handoff and `item` joins one
roadmap item to its tagged Catch2 cases and their standard mappings. `search`
is the forgiving one-query path across test IDs, names, semantic tags, API
surface IDs, Lab requirement IDs, and canonical 2025 clause/subsection keys.
For an installable profile, `work --summary --compact` also prints the exact
package target, CTest label, and installed profile-manifest path; this keeps
the package gate queryable beside the Catch2 lane without treating it as a
synthetic Catch2 requirement row.
It also reports the bounded JUnit target and artifact path for the active
process lane.
`source <source-path-substring>` is the bounded reverse lookup for planned
cases in a known C++ translation unit; each row keeps its source line, Lab IDs,
and canonical 2025 sections. `requirement` accepts a Lab identifier, contract,
or standard clause and returns the mapped tests; when the query matches an
uncovered pinned 2025 requirement, it returns that bounded gap record for
planning the next C++ case. `section` is the exact
standard-mapping path: it accepts a canonical
`document-id:clause-id`, the renderer's space-separated form, or a clause-only
key such as `clause-9.13.1`, and returns only tests mapped to that section.
`matrix <roadmap-family-id>` is the aggregate reverse path when the starting
handle is a roadmap family rather than a test or lane. Every summary row now
also exposes compact `lab_requirement_id -> document_id:clause_id` pairs, so a
caller does not need to zip separate requirement and section arrays.
Use `coverage --family <roadmap-family-id> --summary --compact` when only the
family totals are needed; `--lane` and `--family` are deliberately exclusive.
Use `lanes --summary --limit 40` to discover the most-used exact tags without
dumping the full inventory (use `--limit 0` when a complete list is intended),
and use `lanes --disposition unclassified` or
`unmapped --disposition unclassified` to make the remaining
requirement-less planning cases explicit. `unmapped --disposition explicit`
shows rows that intentionally document why no standalone Requirements-Lab API
surface exists; neither disposition is conformance evidence by itself.
Both `unmapped` and `unlocated` accept `--family <roadmap-family-id>` in
addition to an exact `--lane` for a bounded family backlog.
The compact family queue also prints a deduplicated `direct_pairs` total so
the family-to-standard mapping size is visible before selecting a test.
Test, lane, requirement, and item results include a derived C++ source
location (`cpp/tests/...cpp:line`). The location index is rebuilt from
`TEST_CASE` declarations for each query, so it cannot drift into a second
source of truth; `check` reports any plan entries that are not locatable.
lane/test/requirement/section/item/search/unmapped output is capped at 20 cases by default, with
`--limit 0` available when a complete export is needed. Add `--compact` to
retain each test's Lab requirement IDs and 2025 clause/subsection mappings
while omitting repeated contract-symbol provenance; this is the preferred
bounded traceability view. For roadmap selection, default `next --compact`
follows the ready handoff and keeps the output bounded by reporting only
actionable family/lane/test/ctest handles plus mapping counts; use
`next --json` or a focused `trace`/`matrix` query when the complete arrays are
needed. Add `--summary` when selecting work to
keep one short record per case (status, mapping id, counts, and focus tags)
without printing every requirement row. The canonical
bounded command set is duplicated in
`docs/planning/ROADMAP-INDEX.json` under `bounded_query_commands`.

The `application-value-state` lane is the focused object-value slice: it
selects the codec and state-image cases without pulling the entire
`object-ddm-ownership` item. Use
`python tools/query_rti_work.py lane application-value-state --compact` before
opening source; its output includes the exact mapped Lab IDs and standard
clause/subsection metadata. When a plan entry selects official C++ API
surfaces, `test` and `lane` also print those exact surface IDs beside the
requirements and clauses.

The `pending-application-request-state` lane is the focused temporal request
slice. It selects the same codec case with explicit pending time-advance,
time-role-enable, and deferred-lookahead request metadata, so the exact Lab
IDs and 2025 clause mapping can be inspected without opening the entire
save/restore item. The fresh-registry TAR/deferred-lookahead case is the
current private process-restart baseline; run it with
`python tools/query_rti_work.py test "Filesystem state image restores pending
time advance and deferred lookahead in a fresh registry" --compact`. Its
filesystem fixture names are process-unique, so the focused application and
pending lanes may be run in parallel without colliding.

The small roadmap index is [ROADMAP-INDEX.json](../docs/planning/ROADMAP-INDEX.json).
Keep roadmap anchors and Catch2 tags stable so the check command remains a
fast drift detector. The active item’s `next_task` plus optional
`next_work_id`, `next_work_status`, `next_work_query`, `next_lane`,
`next_test_role`, `next_test_query`, `next_ctest_filter`, `next_standard_sections`, and
`next_plan_ids` fields are the authoritative work handles; update those
alongside each bounded implementation slice. A planned architectural work
handle may intentionally have null lane/test/CTest fields until its first
focused C++ case is added.
`next_ctest_filter` is stored in Catch2 tag form (for example,
`[public-process-restart-regional-pending-attribute-value-update]`); pass that
form to the Catch2 executable or use the tag text as a CTest label with `-L`.

The JSON result for each test includes `lab_requirement_ids`,
`standard_sections` (canonical `document_id:clause_id` keys),
`selected_cpp_api_surface_ids`, `source_locations`, and
`requirements_lab_mapping_id`. These are
derived from the checked-in plan and pinned 2025 bundle, so scripts can join a
test to its C++ declaration, a requirement, or subsection without reparsing
the human roadmap.

For the official 2010 API artifacts and portable 1516e TCK catalog, run:

    python tools/verify_1516e_artifacts.py --help
    python tools/verify_1516e_tck_catalog.py
    python tools/verify_1516e_type_roundtrip.py
    python tools/verify_1516e_tck_parity.py
    python tools/verify_1516e_tck_results.py out/java-tck-2010/script-results.json
    python tools/java_tck_2010.py out/java-tck-2010/script-results.json \
      --provider "Umbra mock Java 1516e"
    python tools/verify_1516e_tck_catalog.py \
      --catalog compliance/catalogs/python-2010-tck-scenario-catalog.json
    python tools/verify_1516e_python_tck_results.py \
      out/java-tck-2010/python-jpype-smoke.json
    python tools/python_tck_2010.py out/java-tck-2010/python-jpype-smoke.json \
      --provider "Umbra mock Java 1516e"

For a real vendor Java provider, use the cross-platform Python route runner with
the API JAR, provider JAR, optional dependency JARs, and FOM path:

    python -m tools.ci test --standard 2010 --route java \
      --api-jar C:\\path\\to\\api.jar \
      --provider-jar C:\\path\\to\\vendor.jar \
      --fom-path C:\\path\\to\\RestaurantFOMmodule.xml

The same Python entry point can exercise the direct JNI/native route after the
native library has been staged:

    python -m tools.ci test --standard 2010 --route jni \
      --api-jar C:\\path\\to\\api.jar \
      --native-library C:\\path\\to\\umbra_rti_jni_2010.dll

The 2010 contract generators consume a locally staged official Java API source
tree and write only the checked-in contract destinations when explicitly
invoked:

    python tools/generate_1516e_python_contract.py --help
    python tools/generate_1516e_exception_contract.py --help
    python tools/verify_1516e_encoding_contract.py --help
    python tools/verify_1516e_python_surface.py --check-jpype
    python tools/verify_1516e_python_types.py --help
    python tools/verify_1516e_provider_boundaries.py
    python tools/generate_1516e_cpp_shell.py --check
    python tools/verify_1516e_provider_jar.py \
      --provider-jar C:\\path\\to\\vendor-rti-2010.jar \
      --api-jar C:\\path\\to\\ieee-1516.1-2010-java-api.jar
    python tools/generate_1516e_surface_inventory.py \
      --cpp-root C:\\path\\to\\cpp\\src \
      --java-root C:\\path\\to\\java\\src \
      --output out\\java-tck-2010\\1516e-surface-inventory.json

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
| generate_1516e_cpp_shell.py | Regenerates the bounded IEEE 1516e-2010 C++ null ambassador shell from the official header. |
| ieee_1516_2_resources.py | Imports or verifies the pinned IEEE 1516.2 resource set and digests. |

## Compliance and provider evidence

| Tool | Purpose |
| --- | --- |
| requirements_lab.py | Exports and validates Umbra-owned baselines, implementation contracts, and the native Catch2 plan against the adjacent Requirements Lab. |
| requirements_lab_sidecar.py | Prepares, records, and verifies sidecar evidence without importing Lab code at runtime. |
| java_tck.py | Validates and exports portable Java TCK traceability and provider evidence. |
| java_tck_2010.py | Validates and exports 2010 Java TCK results against the exact 1516e scenario/Requirements Lab catalog. |
| python_tck_2010.py | Validates and exports 2010 provider-neutral Python smoke results (JPype or native) with the same Requirements Lab linkage. |
| verify_external_2025_fom_corpus.py | Verifies a configured external 2025-native FOM snapshot against its manifest. |
| verify_external_siso_fom_corpus.py | Verifies a configured external SISO FOM snapshot against its manifest. |
| verify_external_2010_fom_resources.py | Verifies the reviewed external IEEE 1516.1/1516.2-2010 schemas and MIM against their manifest. |
| verify_1516e_artifacts.py | Verifies official IEEE 1516.1/1516.2-2010 archive digests, namespaces, and API counts. |
| verify_1516e_tck_catalog.py | Verifies every 2010 TCK requirement, mapping, transition, and provider-neutral API reference against the pinned Lab export and checked-in Python contract. |
| verify_1516e_type_roundtrip.py | Verifies the focused 2010 JNI carrier matrix covers raw JVM values, all standard data elements, handles, collections, logical times, and Java-only enum/record/callback/exception carriers. |
| verify_1516e_tck_parity.py | Verifies every Java 2010 provider scenario has a Python transplant and that Python retains its normative links. |
| verify_1516e_tck_results.py | Verifies a 2010 Java TCK result artifact has the catalog's exact scenarios, standard, and status vocabulary. |
| verify_1516e_python_tck_results.py | Verifies a 2010 Python result artifact (JPype or native) has the Python catalog's exact scenarios and explicit status counts. |
| verify_1516e_encoding_contract.py | Compares the official 2010 Java encoding interface method names with the Python contract namespace. |
| verify_1516e_python_surface.py | Audits the checked-in 2010 namespaces, contract methods, nested callback records, factories, encoders, and optional JPype adapter. |
| verify_1516e_python_types.py | Compares official Java type filenames with Python module attributes/exports and nested callback aliases. |
| generate_1516e_surface_inventory.py | Generates the C++/Java/Python method-name inventory and makes intentional C++/Java surface differences explicit. |
| verify_1516e_provider_boundaries.py | Proves that the 2010 JPype/native/JNI boundary is isolated from the 2025 provider and pins the bounded native target plus JNI null bridge. |
| verify_1516e_provider_jar.py | Performs a metadata-only 2010 `ServiceLoader`/provider-class/namespace preflight for provider and API JARs; it does not claim conformance. |
| verify_1516e_capability_profile.py | Validates shared 2010 Java/Python capability-profile keys against the transplantable scenario catalogs. |

## Native test checks

| Tool | Purpose |
| --- | --- |
| verify_catch2_test_tags.py | Ensures native Catch2 tests belong to focused development lanes. |
| verify_ctest_service_lanes.py | Checks that configured CTest service lanes remain complete. |
| verify_ieee_exception_binding.py | Confirms every official exception has a binding definition. |
| verify_ieee_headers.py | Confirms the vendored IEEE 1516.1 header file set and hashes. |
| verify_hla_symbolic_names.py | Rejects raw string literals at dynamic HLA handle-lookup boundaries. |

## Reporting

plot_runtime_instrumentation.py turns a runtime instrumentation CSV into a
local image. Send its output to out/ rather than a source directory.

## Write boundaries

Most verifiers read inputs and report success or failure. The generator and
Requirements Lab tools can write outputs. Generated source remains under
cpp/generated/, ordinary temporary output belongs under out/, and local
Requirements Lab requests or raw evidence belong under .compliance/. Review a
generator diff before committing it.
