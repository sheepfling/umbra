# HLA Java RTI Contract TCK

This package is a vendor-neutral executable test set for an IEEE 1516.1-2025
Java RTI provider. The Java sources import only JDK classes and the official
`hla.rti1516_2025` API. Provider selection happens through the standard
`RtiFactoryFactory`/Java service-loader boundary.

Use the [Java TCK guide](../../docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) for
the evidence model and provider-matrix workflow.

## Scope

The catalog covers factory discovery,
basic and composite standard encoding, federation creation/join/resign/destroy/query, ordinary
publication/subscription, named and unnamed object registration and discovery,
single and multiple name-reservation callbacks, released-name reuse after object
deletion, instance- and class-scoped attribute value update requests, attribute
update/reflection, object deletion and removal callbacks,
ordinary and timestamped interaction delivery (including the RO-positive/TSO-gated asynchronous-delivery boundary),
support lookups, negative cases, relevance
advisories, directed interactions, derived-target subscription eligibility,
and transportation/order controls. The
`java-tck.federate-mom-object-instances-updated-mim-attribute` case mirrors
the paired C++ TCK's `HLAobjectInstancesUpdated` witness: it checks zero before
updates, one after repeated updates to one object, two after a second object,
and three through a periodic `HLAsetTiming` report after updating a third
object twice. A later direct request agrees, and the value remains three after
periodic reporting is disabled. It maps only the exact MIM definition in
IEEE 1516.1-2025 §11.4.1. Focused Java runs passed under both callback models;
the paired C++ primary/contract tests passed 4/4. This is local-provider
evidence, not independent-provider behavior or conformance.
The default-run `java-tck.federate-mom-updated-object-count-periodic` case
checks the registered-object, distinct-updated-object, and update-call counters
directly at each service boundary, then verifies the periodic `(2, 2, 3)` report.
It maps only the shared exact definition for these three HLAcount attributes to
IEEE 1516.1-2025 §11.4.1. Java passed under both callback models; the paired C++
primary passed focused CTest 2/2 and both direct callback-model runs. Results
are in `out/java-tck/federate-mom-updated-object-count-periodic-java-parity-20261115/`
and `out/mgw/cpp-tck-mom-updated-object-count-periodic-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The default-run `java-tck.federate-mom-object-instances-reflected-mim-attribute`
case checks the distinct-object count at zero, one, still one after a repeat, two,
and three after two reflections to a third object. A separate monitor requests the
subject MOM value; the direct count remains three after periodic reporting is enabled
and disabled. It maps only the HLAobjectInstancesReflected definition in IEEE
1516.1-2025 §11.4.1. Java passed under both callback models, and the paired C++
primary passed focused CTest 2/2 plus both direct callback-model runs. Results are
in `out/java-tck/federate-mom-object-instances-reflected-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-object-instances-reflected-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The default-run `java-tck.federate-mom-object-instances-discovered-mim-attribute`
case checks the settled `HLAobjectInstancesDiscovered` baseline, its increase after
two application discoveries, and the same object's rediscovery after local deletion.
It then confirms the periodic value agrees with a direct request and remains stable
after periodic reports are disabled. It maps only the standard MIM definition in
IEEE 1516.1-2025 §11.4.1. Java passed under both callback models; the paired C++
primary passed focused CTest 2/2 and direct runs in both models. Results are in
`out/java-tck/federate-mom-object-instances-discovered-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-object-instances-discovered-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The default-run `java-tck.federate-mom-object-instances-removed-mim-attribute`
case checks the zero baseline, counts three receive-order removals only after their
callbacks arrive, verifies each callback's tag and producer, and compares direct
counts with the periodic value before and after reporting is disabled. The Java
case also checks RTI ownership of its MOM attribute, retired object-name lookup, and
the `RequestAttributeValueUpdate` service report. It maps only the standard MIM
definition in IEEE 1516.1-2025 §11.4.1. Java passed under both callback models; the
paired C++ primary passed focused CTest 2/2 and direct runs in both models. Results
are in `out/java-tck/federate-mom-object-instances-removed-java-parity-20261115/`
and `out/mgw/cpp-tck-mom-object-instances-removed-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The default-run `java-tck.federate-mom-object-instances-deleted-mim-attribute`
case checks the zero baseline, increments after each of three successful receive-order
deletions, and correlates each deletion with its receiver callback. It compares direct
and periodic values after the second and third deletions, verifies object-name
retirement, and confirms the count stays at three after periodic reporting stops.
It maps only the standard MIM definition in IEEE 1516.1-2025 §11.4.1. Java passed
under both callback models; the paired C++ primary passed focused CTest 2/2 and
direct runs in both models. Results are in
`out/java-tck/federate-mom-object-instances-deleted-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-object-instances-deleted-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The default-run `java-tck.federate-mom-updates-sent-mim-attribute` case checks the
zero baseline, one `HLAupdatesSent` increment for each of three successful updates,
including a repeated update to one object, and the value of five after two additional
updates with periodic reports enabled. It compares direct and periodic values and
confirms the count remains stable after reports are disabled. It maps only the
standard MIM definition in IEEE 1516.1-2025 §11.4.1. Java passed under both callback
models; the paired C++ primary passed focused CTest 2/2 and direct runs in both
models. Results are in
`out/java-tck/federate-mom-updates-sent-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-updates-sent-java-parity-20261115.{json,xml}`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.
The default-run `java-tck.federate-mom-reflections-received-mim-attribute` case
checks one `HLAreflectionsReceived` increment for each of three separate reflection
callbacks, including two updates to the same object. It compares direct counter
requests with the periodic value and verifies the count after periodic reporting is
disabled. It maps only the standard MIM definition in IEEE 1516.1-2025 §11.4.1.
Java passed under both callback models, and the paired C++ primary passed focused
CTest 2/2 and direct runs in both models. Results are in
`out/java-tck/federate-mom-reflections-received-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-reflections-received-java-parity-20261115.{json,xml}`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.
The default-run `java-tck.federate-service-reporting-mim-attribute` case
observes the subject's `HLAserviceReporting` value directly, changes the public
switch, checks one reliable conditional reflection, and restores the original value.
Its six direct mappings cover the MIM value and public getter/setter, request,
subscription, and reflection services in IEEE 1516.1-2025 §§5.8, 6.9.3, 6.21,
10.46, 10.47, and 11.4.1. Java passed both callback models; the paired C++ primary
and contract passed all four direct runs. Results are in
`out/java-tck/federate-service-reporting-mim-attribute-java-parity-20261116/` and
`out/mgw/cpp-tck-service-reporting-mim-attribute-java-parity-20261116-pair.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-object-class-relevance-advisory-mim-attribute`
case observes the `HLAobjectClassRelevanceAdvisory` switch's bounded effect: it
checks the initial state, one opposite-state change and conditional reflection,
a direct value request, and restoration. Its API mappings cover IEEE 1516.1-2025
§§5.8, 6.9.3, 6.21, 10.34, 10.35, and 11.4.1; the paired C++ source-refinement
mapping to §11.6 is explicitly cross-cutting and limited to the exercised switch
effect. Java and the paired C++ primary/contract passed both callback models.
Results are in `out/java-tck/object-class-relevance-advisory-mim-attribute-java-parity-20261116/`
and `out/mgw/cpp-tck-object-class-relevance-advisory-mim-attribute-java-parity-20261116-pair.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-attribute-relevance-advisory-mim-attribute` case
checks the HLAattributeRelevanceAdvisory switch's initial value, one opposite-state
change, the conditional reflection, a direct value request, and restoration. It maps
the public MIM/API surfaces to IEEE 1516.1-2025 §§5.8, 6.9.3, 6.21, 10.36, 10.37,
and 11.4.1. The paired C++ source-refinement mapping to §11.6 is cross-cutting and
limited to the switch effect. Java and paired C++ primary/contract runs passed both
callback models; Java results are in
`out/java-tck/federate-attribute-relevance-advisory-mim-attribute-java-parity-20261116/{evoked,immediate}/`.

The default-run `java-tck.federate-attribute-scope-advisory-mim-attribute` case
checks the HLAattributeScopeAdvisory switch's initial value, one opposite-state
change, its conditional reflection, a direct value request, and restoration. It maps
the public MIM/API surfaces to IEEE 1516.1-2025 §§5.8, 6.9.3, 6.21, 10.38, 10.39,
and 11.4.1. Java and paired C++ primary/contract runs passed both callback models.
Java results are in
`out/java-tck/federate-attribute-scope-advisory-mim-attribute-java-parity-20261116/{evoked,immediate}/`.

The default-run `java-tck.federate-interaction-relevance-advisory-mim-attribute`
case checks the HLAinteractionRelevanceAdvisory switch's initial value, one
opposite-state change, its conditional reflection, a direct value request, and
restoration. It maps the public MIM/API surfaces to IEEE 1516.1-2025 §§5.8, 6.9.3,
6.21, 10.40, 10.41, and 11.4.1. Java and paired C++ primary/contract runs passed
both callback models. Java results are in
`out/java-tck/federate-interaction-relevance-advisory-mim-attribute-java-parity-20261116/{evoked,immediate}/`.

The default-run `java-tck.federate-exception-reporting-mim-attribute` case
observes the subject's `HLAexceptionReporting` value directly, changes the public
switch, checks one reliable conditional reflection, and restores the original value.
Its six direct mappings cover the MIM value and public getter/setter, request,
subscription, and reflection services in IEEE 1516.1-2025 §§5.8, 6.9.3, 6.21,
10.48, 10.49, and 11.4.1. It does not test exception-report delivery. Java passed
both callback models; the paired C++ primary and contract passed all four direct
runs. Results are in
`out/java-tck/exception-reporting-mim-attribute-java-parity-20261116/` and
`out/mgw/cpp-tck-exception-reporting-mim-attribute-java-parity-20261116-pair.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-reflection-counts` case distinguishes the
number of distinct application objects reflected from the number of reflection
callbacks. It checks the direct counts at zero, after two reflections of one object,
a reflection of a second object, and a timestamped reflection; periodic
`HLAsetTiming` reporting agrees at `(2, 4)`. The direct mapping is the standard
MOM statistics definition in IEEE 1516.1-2025 §11.4.1; callback-boundary and
count transitions remain bounded scenario evidence, with RL-107 recording the
generic candidate refinement gap. Java passed under both callback models, and the
paired C++ primary/contract passed all four direct runs. Results are in
`out/java-tck/federate-mom-reflection-counts-java-parity-20261116/` and
`.build/cpp-tck-encoding-helper-base-dispatch-mingw-20261114/portable-tck/tck-results/`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-interactions-received-mim-attribute` case
checks the zero baseline, one `HLAinteractionsReceived` increment for each of three
delivered receive-order interactions, the value of five after two more interactions
with periodic reports enabled, and the unchanged count after reporting is disabled.
It maps only the standard MIM definition in IEEE 1516.1-2025 §11.4.1. Java passed
under both callback models, and the paired C++ primary passed focused CTest 2/2 and
direct runs in both models. Results are in
`out/java-tck/federate-mom-interactions-received-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-interactions-received-java-parity-20261115.{json,xml}`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.
The default-run `java-tck.federate-mom-interactions-sent-mim-attribute` case checks
the zero baseline, captures the direct counter while periodic reports are enabled,
then verifies that three accepted receive-order application sends raise both the
direct and periodic `HLAinteractionsSent` values by exactly three. It maps only the
bounded ordinary-send claim to IEEE 1516.1-2025 §11.4.1; the MIM's separate
DDM-accounting semantic remains outside this case. Java passed under both callback
models, and the paired C++ primary passed focused CTest 2/2 and direct runs in both
models. Results are in
`out/java-tck/federate-mom-interactions-sent-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-interactions-sent-java-parity-20261115.{json,xml}`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.
The default-run
`java-tck.federate-mom-directed-interactions-received-mim-attribute` case checks
that an ordinary interaction increments only `HLAinteractionsReceived`, while three
directed callbacks increment the total and directed counters to four and three.
Two further directed callbacks raise their periodic values to six and five, and
direct reads confirm both values after reporting is disabled. It has one unique
direct mapping to the two standard MIM entries in IEEE 1516.1-2025 §11.4.1. Java
passed under both callback models, and the paired C++ primary passed focused CTest
2/2 and direct runs in both models. Results are in
`out/java-tck/federate-mom-directed-interactions-received-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-directed-interactions-received-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The default-run
`java-tck.federate-mom-directed-interactions-sent-mim-attribute` case checks the
zero counters, verifies an ordinary send increments only `HLAinteractionsSent`,
then checks that three directed sends raise the total and directed counters by four
and three. Two further directed sends raise both by two more, with matching direct
and periodic values after reporting is disabled. It maps the two standard MIM
entries through one direct requirement pair in IEEE 1516.1-2025 §11.4.1; DDM
accounting is outside this no-region case. Java passed under both callback models,
and the paired C++ primary passed focused CTest 2/2 and direct runs in both models.
Results are in
`out/java-tck/federate-mom-directed-interactions-sent-java-parity-20261115/` and
`out/mgw/cpp-tck-mom-directed-interactions-sent-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The default-run `java-tck.federate-mom-directed-interactions-received-counts` case
requests `HLAreportDirectedInteractionsReceived` after one ordinary and one directed
reliable interaction. It verifies ordinary-receive exclusion, the reliable count,
empty best-effort and idle-requester buckets, nested `HLAinteractionCounts` decoding,
and standard report metadata. It maps the public MOM request/report surface to
IEEE 1516.1-2025 §11.4.1; detailed bucket accounting remains scenario evidence.
Java and the paired C++ primary passed both callback models. Results are in
`out/java-tck/federate-mom-directed-interactions-received-counts-java-parity-20261115/`
and `out/mgw/cpp-tck-mom-directed-interactions-received-counts-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-interactions-received-counts` case requests
`HLAreportInteractionsReceived` after one reliable and two best-effort application
receives. It verifies the target federate's two transport buckets, empty buckets for
a requester with no receipts, nested `HLAinteractionCounts` decoding, and standard
report metadata. It maps the public MOM request/report surface to IEEE 1516.1-2025
§11.4.1; detailed counts and empty responses remain scenario evidence. Java passed
both callback models. Results are in
`out/java-tck/federate-mom-interactions-received-counts-java-parity-20261115/`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-interactions-sent-counts` case requests
`HLAreportInteractionsSent` after one reliable and two best-effort application sends.
It verifies the sender's two transport buckets, empty buckets for an idle federate,
nested `HLAinteractionCounts` decoding, and standard report metadata. It maps the
public MOM request/report surface to IEEE 1516.1-2025 §11.4.1; detailed counts and
empty responses remain scenario evidence. Java passed both callback models. Results
are in
`out/java-tck/federate-mom-interactions-sent-counts-java-parity-20261115/`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-updates-sent-counts` case requests
`HLAreportUpdatesSent` after one reliable and two best-effort attribute updates. It
checks the target's per-transport `HLAupdateCounts` for the registered object class,
the empty report pair for a requester with no updates, and standard callback
metadata. It maps the two paired MOM candidates to IEEE 1516.1-2025 §11.4.1; detailed
counts remain scenario assertions and RL-112 records the generic candidate
refinement gap. Java passed both callback models; the paired 98-assertion C++ Catch2
case passed focused CTest 1/1 in HLA_EVOKED. Results are in
`out/java-tck/federate-mom-updates-sent-counts-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-reflections-received-counts` case requests
`HLAreportReflectionsReceived` after one reliable and two best-effort attribute
reflections. It checks the target's per-transport `HLAreflectCounts` for the
registered object class, the two empty report buckets for a requester with no
reflections, and standard callback metadata. It maps the paired public MOM
request/report candidates to IEEE 1516.1-2025 §11.4.1; callback counts, transport
buckets, class grouping, and nested encoding remain scenario assertions, with RL-118
recording the generic candidate refinement gap. Java passed both callback models;
the paired 129-assertion C++ Catch2 case passed focused CTest 1/1 in HLA_EVOKED.
Results are in
`out/java-tck/federate-mom-reflections-received-counts-java-parity-20261115/`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-deletable-object-count` case requests
`HLAobjectInstancesThatCanBeDeleted` directly before registration, after one and two
application object registrations, through a periodic `HLAsetTiming` reflection, and
after each deletion. It maps only the standard MIM definition to IEEE 1516.1-2025
§11.4.1; count transitions and callback metadata remain scenario evidence. Java
passed both callback models; the paired 55-assertion C++ Catch2 case passed focused
CTest 1/1 in HLA_EVOKED. Results are in
`out/java-tck/federate-mom-deletable-object-count-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-deletable-object-report` case requests
`HLAreportObjectInstancesThatCanBeDeleted` for an owner with live objects in two
application classes. It decodes one count per class, then verifies the report omits
the class whose object was deleted while retaining the other class at count one. It
maps only the Table 7 P report leaf to IEEE 1516.1-2025 §11.4.1; RL-110 records the
generic candidate refinement gap. Java passed both callback models; the paired
53-assertion HLA_EVOKED C++ case and HLA_IMMEDIATE companion passed focused CTest
2/2. Results are in
`out/java-tck/federate-mom-deletable-object-report-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-reflected-object-report` case requests
`HLAreportObjectInstancesReflected` after registering one object in each of two
application classes. It observes two reflections of the first object and one of the
second, then checks the report counts one distinct instance for each class. It maps
the paired MOM candidates to IEEE 1516.1-2025 §11.4.1 for this Table 7 P report leaf;
RL-111 records the generic candidate refinement gap. Java passed in both callback
models; the paired 51-assertion HLA_EVOKED C++ case and 50-assertion HLA_IMMEDIATE
companion passed focused CTest 2/2. Results are in
`out/java-tck/federate-mom-reflected-object-report-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-reflected-timestamped-object-report` case
registers two Server/Efficiency objects and sends three timestamped updates at one
logical time, including two updates to the first object. It checks each reflection's
value, tag, order, producer, reliable transport, retraction handle, and logical time,
then verifies all reflections arrive before the target's grant. A distinct requester
decodes the standard `HLAreportObjectInstancesReflected` response and expects two
distinct instances. Its 13 direct section mappings cover subscription/discovery,
registration, timestamped update/reflection and time management through §11.4.1. Java
passed in both callback models against the paired C++ primary/contract evidence.
Results are in
`out/java-tck/federate-mom-reflected-timestamped-object-report-java-parity-20261116/`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-updated-timestamped-object-report` case
registers two Server/Efficiency objects and sends one timestamped update to each at
one logical time. It checks both reflection payloads and delivery metadata before the
target's grant, then has a separate monitor request `HLAreportObjectInstancesUpdated`
and verify one class row with a count of two. Its 10 direct mappings cover timestamped
update/reflection, time management, and the selected §11.4.1 MOM report leaf. Java
passed in both callback models against the paired C++ primary/contract evidence.
Results are in
`out/java-tck/federate-mom-updated-timestamped-object-report-java-parity-20261116/`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.

The default-run `java-tck.federate-mom-ro-length-periodic` case checks a zero
`HLAROlength` baseline, a positive direct value while three receive-order interactions
are queued, a positive periodic value while another interaction is queued, and zero
after delivery. It maps only the standard MIM `HLAROlength` definition to
IEEE 1516.1-2025 §11.4.1; queue timing and detailed values remain scenario evidence.
Java passed both callback models. Results are in
`out/java-tck/federate-mom-ro-length-periodic-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-tso-length-periodic` case queues one
timestamped interaction and verifies direct and periodic `HLATSOlength` values of
one before the target grant, then zero after delivery. It checks that the timestamped
receive callback precedes the target grant. It maps only the standard MIM
`HLATSOlength` definition to IEEE 1516.1-2025 §11.4.1; queue accounting and delivery
ordering remain scenario evidence. Java passed both callback models. Results are in
`out/java-tck/federate-mom-tso-length-periodic-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-galt-lits-periodic` case checks direct and
periodic GALT/LITS values of 2 with one time-regulating federate at time 0 and
lookahead 2. After that last regulator disables regulation, both direct queries are
undefined and both MOM attribute values are empty. It maps only the standard MIM
GALT/LITS definitions to IEEE 1516.1-2025 §11.4.1; the timing sequence and encodings
remain scenario evidence. Java passed both callback models. Results are in
`out/java-tck/federate-mom-galt-lits-periodic-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-time-state-durations` case follows the target
through time-constrained enablement, an advance to time 2, and the grant. It checks
direct and periodic `HLAtimeGrantedTime`/`HLAtimeAdvancingTime` encodings and
nonnegative monotonic values, including the smaller direct values after the periodic
report consumes its interval. The MIM attribute definitions map to IEEE 1516.1-2025
§11.4.1; duration accumulation and consumption remain scenario evidence. Java passed
both callback models, and the paired 192-assertion C++ Catch2 case passed 1/1. Results
are in `out/java-tck/federate-mom-time-state-durations-java-parity-20261115/`. This is
local-provider evidence only; independent-provider behavior and conformance are not
claimed.

The default-run `java-tck.federate-mom-directed-interactions-sent-counts` case requests
sent-count reports after one ordinary and two directed reliable sends, then changes
transportation to best effort and makes another ordinary send. It checks that only
the directed sends appear in the reliable count, that the best-effort and idle-federate
buckets are empty, and that each report carries the expected MIM metadata. Its direct
mapping is the public MOM request/report surface in IEEE 1516.1-2025 §11.4.1; detailed
bucket accounting is scenario evidence, with the candidate refinement recorded in
RL-114. Java and the paired C++ primary passed both callback models. Results are in
`out/java-tck/federate-mom-directed-interactions-sent-counts-java-parity-20261115/`
and `out/mgw/cpp-tck-mom-directed-interactions-sent-counts-java-parity-20261115.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and conformance
are not claimed.
The
`java-tck.federation-mom-service-actor-identity` case sends a failing
`HLAchangeAttributeOrderType` request from a federate that does not own the
target attribute. It checks `AttributeNotOwned` and verifies that two separate
`HLAreportMOMexception` subscribers identify the sending federate. It uses the
adapter-supplied FOM, standard MIM, logical-time implementation, object class,
and attribute, and maps only the actor-context slice at p. 289 l. 56 in
IEEE 1516.1-2025 §11.3; other `HLAservice` leaves are not claimed. Focused
local Umbra JNI runs passed in both callback models using `p0-tck.xml`, the
standard MIM, `HLAinteger64Time`, `HLAobjectRoot.TckObject`, and `Value`.
Independent-provider behavior and conformance are not claimed.
The `java-tck.federation-mom-change-attribute-order-type` case sends the
Table 7 S-leaf request with inherited `HLAfederate` encoded from the subject's
handle, plus `HLAobjectInstance`, `HLAattributeList`, and
`HLAsendOrder=Receive`. It then checks that the subject's timestamp-bearing
attribute update is reflected in RECEIVE order without a retraction designator.
It maps p. 289 l. 53 to §11.3 and p. 290 l. 156 to §11.4.1. Current-source
JNI runs passed both callback models; results are in
`out/java-tck/federation-mom-change-attribute-order-type-java-federate-target-20261114/`.
No independent-provider or conformance claim is made.
The `java-tck.federate-application-multiple-executions` case creates two
execution names that differ by one ASCII case change, lists both, and joins two
federates to each execution through four distinct ambassadors. It reuses the
same two exact federate names in both executions and checks each execution's
member report independently. Same-execution positive controls and bounded peer
observations cover object discovery/reflection, receive-order interactions, and
timestamped interaction isolation; the case also reuses an ambassador after
resignation. Focused runs against the installed Umbra JNI provider passed in
both callback models, and the paired C++ primary/contract CTests passed 4/4.
These are focused same-provider results, not independent-provider or conformance
evidence. The case runs by default with the adapter's FOM and time selection.
The `java-tck.unsubscribe-last-attribute-stops-discovery` and
`java-tck.attribute-unsubscribe-stops-reflection` cases now run by default.
They first establish positive discovery/reflection, then verify that removing
the selected attribute subscription suppresses later callbacks within a
bounded observation window. Both passed focused JNI runs in evoked and
immediate callback modes using the adapter's ordinary object FOM. The
attribute-reflection case also directly maps the bounded post-unsubscribe
suppression witness at p. 223 l. 40 to §9.1.5, alongside its §5.9.4 setup
mapping.
The `java-tck.callback-reentrancy` case first checks callback-service exception
boundaries before joining, then creates and joins a federation and requests the
listing callback again. From the joined callback, `getObjectClassHandle` must
succeed while `evokeCallback` and `evokeMultipleCallbacks` must throw
`CallNotAllowedFromWithinCallback`. Those joined-phase checks map directly to
the two existing §12.8 requirement candidates; the pre-join phase remains
supplementary API/exception tracing. The mapped witnesses cover one permitted
and two excluded services, not either complete service subset. It is
parity-linked to `cpp-tck.callback-reentrancy`, runs by default, and passed
focused JNI runs in both callback models. The callback recorder uses bounded
waits and thread-safe state; no independent-provider or overall conformance
claim is made.
The opt-in `java-tck.federate-initiated-concurrency-different-ambassadors`
case joins two distinct `RTIambassador` instances to one federation, holds the
owner's `reportFederationExecutions` callback while the member invokes
`getInteractionClassHandle("HLAinteractionRoot")` and
`getInteractionClassName`, then `listFederationExecutions`; it finally releases
and evokes both reports. The exact class-name round trip exercises public
request/response association while the owner's list response is pending. The
direct p. 358 requirements map to IEEE 1516.1-2025 §12.8; p. 397 l. 126-41
maps to §12.13.4.1. The test does not inspect numeric sequence values or claim
overall conformance. A fresh current-source JNI run passed under
`HLA_EVOKED`; `HLA_IMMEDIATE` is outside this case's scope.
The `java-tck.evoked-callback-no-concurrency` case holds one
`reportFederationExecutions` callback open while a second thread calls
`evokeCallback` on the same ambassador. It verifies the second callback does
not enter before release and that callback execution never overlaps. The two
direct p. 357 requirements map to IEEE 1516.1-2025 §12.8. Five focused
current-source JNI runs passed under `HLA_EVOKED`; `HLA_IMMEDIATE` is
unsupported for this case.
The `java-tck.null-federate-ambassador` case invokes every official
`FederateAmbassador` callback overload through `NullFederateAmbassador`,
including timestamped and directed callbacks, and passes independently of
federation/FOM setup on the local JNI provider.
The `java-tck.authorizer-factory-factory` case discovers the standard named
authorizer factory and authorizer and verifies unknown-name rejection; it also
passes without federation/FOM setup on the local JNI provider.
The `java-tck.configuration-authorization-contract` case covers Java builder
accessors, configuration results, information values, standard credentials,
password encoding/decoding, and authorizer dispatch. It passes without
federation/FOM setup. Unlike C++ `VariableLengthData`, Java `Credentials`
exposes its byte array directly, so the test does not impose C++ copy semantics
on the Java API.
The `java-tck.authorizer-connect-service-decision` and
`java-tck.authorizer-unrecognized-credentials-unauthorized` cases exercise the
Java Connect boundary with a configured `HLAauthorizer`: they check no
credentials, a wrong password, a matching password, and an unrecognized
credential type. `java-tck.authorizer-disabled-service-rejects-credentials`
checks the Connect outcome when RID authorization is disabled. These cases need
the protected RID profiles supplied by
`packages/hla-rti-cpp-tck/adapters/current-process/run_authorizer_connect_decision.py`
and `run_authorizer_disabled_credentials.py`; focused JNI runs passed in both
callback models. The direct mappings cover the bounded Connect outcomes in
IEEE 1516.1-2025 §§12.5 and 12.12.1.1 only.
The `java-tck.authorizer-federation-service-decision` case exercises the
public Java Connect, Create Federation Execution, Join, and Destroy services
with the adapter's custom native Authorizer. It requires the protected RID
profile and provider library supplied through
`run_authorizer_federation_decision.py`, plus an adapter FOM. The Java
`AuthorizerFactoryFactory` uses Java `ServiceLoader`; the native C++ fixture
factory is not exposed through that discovery API, so this case records the
public federation-service outcomes without claiming custom-factory discovery
or provider interoperability.
The `java-tck.authorizer-destroy-service-decision` case uses the same adapter
library with `TckDestroyGate`: it requires `Unauthorized` for the first Destroy,
joins to confirm the federation remains, resigns, and retries Destroy
successfully. It maps p. 356 l. 293-97 to IEEE 1516.1-2025 §12.5; the protected
RID profile, custom native provider library, adapter FOM, and time
implementation are required.
The `java-tck.authorizer-join-service-decision` case uses `TckJoinGate` to
require `Unauthorized` on the first Join and successful membership on a retry
with the same federate name, then resigns and destroys the federation. It maps
p. 356 l. 326-108-join to IEEE 1516.1-2025 §12.5 and uses the same adapter
profile, native provider library, FOM, and time implementation inputs.
The `java-tck.authorizer-no-credentials-connect` case calls Connect with the
standard `HLAnoCredentials` value. Its adapter Authorizer permits connection
only when the RTI forwards that type with empty data; it maps p. 356 l. 389-129
and l. 395-131 to IEEE 1516.1-2025 §12.5.
The `java-tck.authorizer-rid-configuration-decision` case uses one denying
service profile, one allowing service profile, and one `service=disabled`
profile with identical Java Connect/HLAnoCredentials calls. It maps p. 357
l. 55-17 to IEEE 1516.1-2025 §12.5 and requires the protected RID and native
provider fixtures from `run_authorizer_rid_configuration.py`.
The `java-tck.authorizer-single-service-per-rid-profile` case checks that a
profile with two otherwise valid authorization service directives is rejected
through Java Connect. It maps p. 357 l. 58-18 to IEEE 1516.1-2025 §12.5 and uses
the same RID adapter with its duplicate-service profile.
The C++ `VariableLengthData` ownership/pointer-adoption contract has no direct
Java equivalent: the standard Java API uses `byte[]` and does not define pointer
ownership or adoption. The Java catalog records this as not applicable instead
of mapping it to a broad API-inventory test.
The `java-tck.data-element-base-contract` case exercises the shared Java
`DataElement` contract for encoded length, octet boundary, full encoding,
positioned `ByteWrapper` encoding/decoding, and truncated input rejection. It
passes without federation/FOM setup. Java's `DataElement` interface does not
expose the C++ clone, type-comparison, hash, `encodeInto`, or `decodeFrom`
members, so those C++-specific methods are not claimed as Java parity.
The paired `java-tck.basic-data-elements` scenario separately checks the
equivalent nonterminal `HLAinteger64LE` cursor behavior with `ByteWrapper`:
the following encoded `HLAoctet` remains readable from the cursor after the
integer. This verifies the shared cursor behavior without claiming a Java
`decodeFrom` method.
The `java-tck.handle-collection-values` case exercises standard attribute
handle-set and attribute/parameter value-map factories, set/map clone
independence, attribute/region pair-list cloning, save/restore status value
records, and `RangeBounds` value equality. It passes without federation or FOM
setup. It covers the shared Java surface only; C++-specific handle default
construction, parameter-handle sets, and vector copy/assignment semantics are
not claimed as Java parity.
The `java-tck.standard-enumeration-contract` case mirrors the promoted C++ enum
contract across all eleven corresponding Java enum families. It checks each
official enum's exact standard constant-name set without requiring a federation,
FOM, or provider-specific behavior; this is API-contract evidence, not a claim
about provider runtime behavior.
The `java-tck.standard-exception-hierarchy` case translates the common subset
of the promoted C++ exception contract: all 100 shared exception types must
construct through the standard Java `RTIexception` base and preserve their
standard name and message. Java copy/assignment and stream behavior, plus C++
exception types without direct Java counterparts, remain explicitly unclaimed.
The `java-tck.exception-name-string` case calls `getObjectClassHandle` before
Connect, catches the public `RTIexception`, and checks that `name()` returns
exactly `NotConnected`. It ports the matching C++ exception-name case through
the Java binding and is limited to this one service exception.
The `java-tck.basic-data-elements` case independently covers the standard scalar
basic-data-element contract, including exact wire vectors, round trips, and
malformed scalar decoding; it passes on the local JNI provider. This is linked
to the promoted C++ basic-data-elements contract. The `java-tck.encoder-round-trip`
case covers fixed and variable arrays
(including the empty-array count prefix), aligned records, variant records,
and extendable variant records. Its wire
checks are paired with the promoted C++ composite-data-elements contract;
runtime validation currently exposes an unknown-discriminant variant getter
behavior difference in the available provider.
The `java-tck.complex-data-element-constituent-access` case complements that
wire coverage with direct Java generic-array, record, and variant constituent
access, including simple and nested-record alternatives and selected-value
round trips. It maps the six demonstrated p. 381 assertions in
IEEE 1516.1-2025 §12.12.4.2; C++-specific operators and template class names
remain outside the Java parity claim.
The `java-tck.handle-encoding-mim-data-types` case encodes all ten standard
Java handle kinds, checks each as an `HLAvariableArray<HLAbyte>`, and verifies
the corresponding Java `HandleFactory` round trip. It maps p. 382 l. 24-7 to
IEEE 1516.1-2025 §12.12.4.2 and runs with the adapter-supplied DDM FOM.
The `java-tck.handle-parameter-value-round-trip` case sends a registered
object's encoded handle through both an interaction parameter and an attribute
value, verifies byte-preserving delivery and `ObjectInstanceHandleFactory`
reconstitution, and checks the same object name. For each receive-order send,
the receiver holds callbacks until the service returns; the sender then mutates
the input map's `byte[]` and the receiver must still get the original bytes.
Eight other standard Java handle factories must reject the interaction
parameter bytes with `CouldNotDecode`. Its direct mappings are p. 353 l. 120,
123, and 132-43 to §12.2, p. 377 l. 210-68 to §12.11.7, and p. 396 l. 186-61
to §12.13.3.3. After rebuilding the JNI native library from the current
checkout with MinGW/Ninja and rebuilding the Java provider bridge, the focused
case passed under both callback models. An earlier failure used the stale
2026-10-21 DLL and is superseded. Results are under
`out/java-tck/handle-parameter-value-round-trip-map-copy-20261114/`; these
provider-local runs are execution evidence, not independent-provider
interoperability or conformance evidence.
The `java-tck.malformed-inputs` scenario now mirrors the promoted C++ case:
it rejects missing and malformed FOM inputs, adapter-supplied invalid FOM
modules, and truncated `HLAinteger32BE` data, then disconnects through the
standard Java API. The adapter supplies invalid modules through
`hla.rti.tck.invalidFomModules`. Both `java-tck.malformed-inputs` and
`java-tck.support-services` pass the adapter's MIM explicitly through the
standard `createFederationExecutionWithMIM` service.
The `java-tck.support-services` case also checks that public name-based
support lookups and service-group normalization reject calls before connect
and before federation join, using the adapter's callback model, and complements
those lifecycle gates with joined-state handle checks.
The `java-tck.handle-decoder-semantic-validity` case registers an object in one
federation and decodes its standard handle through a different joined
federation's `ObjectInstanceHandleFactory`. It verifies that decoding preserves
the handle value while the subsequent target-federation name lookup reports
`ObjectInstanceNotKnown`, directly mapping the §12.12.4.2 semantic-independence
rule for object-instance handles. The adapter supplies the DDM FOM, object
class, and attribute; focused JNI runs passed both callback models. Other handle
decoder types and independent-provider behavior are not claimed.
The default-run `java-tck.standard-order-transportation-lookups` case verifies that
each federate's standard transportation-handle factory defaults match the
handles returned by the standard `HLAreliable` and `HLAbestEffort` name lookups.
Java has no portable equivalent of C++'s default-invalid transportation handle
or invalid `OrderType` cast, so this case does not fabricate either input.
It now links directly to the C++ primary case and maps the order names and values,
case-sensitive aliases, and mandatory transportation types to their exact 2025
sections. Focused current-source JNI runs passed under both callback models;
results/JUnit are in
`out/java-tck/standard-order-transportation-lookups-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
Independent-provider behavior and conformance are not claimed.
The `java-tck.logical-time-factory` case now mirrors the C++ lifecycle gates
for `getTimeFactory` before connection and after connection but before join,
and uses the adapter-selected callback model. It round-trips initial/final time
values and zero/epsilon intervals through the provider-selected standard
factory, checks offset encoding boundaries, ordering and arithmetic, and rejects
empty encodings. C++ mutable boundary setters and `HLAlogicalTime` DataElement
wrappers have no direct Java equivalent; the catalog marks those C++ slices
non-applicable rather than claiming Java parity. Federation-backed checks still
require a provider that can create an execution with adapter-supplied FOM/time
inputs.
The `java-tck.logical-time-interval-nonnegative` case resolves both standard
time factories through `LogicalTimeFactoryFactory`, verifies zero and positive
interval controls, and requires negative integer64 and float64 intervals to be
rejected. The 2025 JNI adapter enforces this boundary for its service-loaded
factories. Focused runs passed both callback models. Java `makeInterval` does
not declare a checked interval exception, so the catalog records rejection
without asserting one exception class; independent-provider behavior and
conformance are not established.
The `java-tck.logical-time-factory-factory-contract` case ports the standard
factory-selection checks: an empty name selects float64, the two standardized
names select their matching factory types, an unknown name returns null, and
the default factory creates initial time zero. The Java API has a
`LogicalTimeFactoryFactory` in its official `time` package, so this replaces
the previous not-applicable classification. It does not claim the C++-specific
header-placement requirement at §12.12.1.1 p. 378 l. 339. Focused installed
JNI runs passed both callback models; independent-provider behavior and
conformance are not established.
The `java-tck.overloads-and-exceptions` connection slice now covers all four
standard Java `connect` overloads plus the adapter-configured
`RtiConfiguration` overload, validates the standard additional-settings result
codes, and checks pre-connect service exceptions, duplicate connect, callback
disable/enable and empty-queue servicing, and reconnecting the same ambassador.
It also attempts a unique unknown `RtiConfiguration` name without an RTI address
and requires Connect to succeed with an allowed additional-settings result; it
does not assert configuration-selection flags. The direct mapping covers the
two existing §4.2.4 Connect fallback candidates and retains the §10.58 mapping
for the false empty-queue `evokeMultipleCallbacks` result. It uses the
adapter-selected callback model and passes on the local JNI provider in both
callback modes. The C++ pre-connect `disconnect`
exception assertion is intentionally not translated because the Java standard
API does not declare `NotConnected` for `disconnect`.
Optional standard connection settings for this scenario come from adapter JVM
properties `hla.rti.tck.configurationName`, `hla.rti.tck.rtiAddress`, and
`hla.rti.tck.additionalSettings`; they default to empty and stay outside the
portable scenario implementation.
The opt-in `java-tck.connection-additional-settings-parse-failure` scenario
checks that Java Connect completes and returns
`SETTINGS_FAILED_TO_PARSE` when the adapter supplies a malformed optional
setting, then disconnects. On provider build `umbra-jni-20261106`, focused
EVOKED and IMMEDIATE runs used `fomEdition=invalid`; that input is specific to
Umbra and does not define portable settings syntax. Results are in
`out/java-tck/connection-additional-settings-parse-failure-java-port-20261113/`.
The direct mapping covers only the optional-settings parse-failure continuation
rule in IEEE 1516.1-2025 §4.2.
The opt-in `java-tck.connection-default-configuration-fallback` case compares
an unnamed Connect with a Connect using a unique unknown configuration name and
no RTI address or additional settings. It creates a federation through the
unnamed connection and checks that the second ambassador lists that same
federation through `reportFederationExecutions`, without interpreting
`ConfigurationResult` flags or provider configuration details. Focused EVOKED
and IMMEDIATE runs passed on provider build `umbra-jni-20261106` with the
adapter FOM; results are in
`out/java-tck/connection-default-configuration-fallback-java-port-20261113/`.
The direct mapping covers IEEE 1516.1-2025 §4.2.
The `java-tck.factory-discovery` slice also mirrors the promoted C++
four-octet `HLAinteger32BE` encode/decode check through the discovered standard
encoder factory, the stable non-empty runtime name/version contract, and
repeated standard ambassador construction; the expanded scenario passes on
the local JNI provider.
The `java-tck.transport-order` scenario uses the adapter-selected callback
model and checks the distinct immediate-versus-evoked transport-change commit
behavior for both attributes and interactions. It also checks the unowned
`Name` attribute's FOM-declared transport and queries the
`HLAreliable`/`HLAbestEffort` defaults in a second two-federate execution using
`-Dhla.rti.tck.transportOrderDefaultFom=...`; the Java TCK fixture is
`packages/hla-rti-java-tck/fom/transport-default-best-effort-tck.xml`. Java
omits C++ default-invalid-handle boundaries because the official Java API
provides no portable invalid handle values.
The default-run `java-tck.mom-transportation-type-change-request` scenario
ports the paired C++ standard-MIM requests for attribute and interaction
transportation changes. It checks the confirmation callbacks, subsequent
best-effort delivery, reliable-type isolation for an independent publisher, and
the standard service-invocation report. Focused runs passed under both callback
models against the current-source JNI provider; evidence is in
`out/java-tck/mom-transportation-type-change-request-java-parity-20261115/`.
The federation restore ownership-assumption scenario also preserves pending
object-discovery and ownership-assumption callbacks across a complete save and
restore under both callback models, and checks that ownership is not transferred
before the restored callback is serviced.
The default `java-tck.ownership-acquisition-publication-fence` scenario uses
the adapter-supplied multi-attribute FOM from
`hla.rti.tck.multiAttributeFom`. It directly checks the `ObjectClassNotPublished`
and `AttributeNotPublished` publication gates and the clause 7.12 repeated
Release Denied no-effect outcome. It also covers the supplemental mixed
acquisition flow: the If Available request is superseded by a regular request,
only the owned subset prompts release, and a pending attribute cannot be
unpublished before release denial. The Java port omits the C++ default-invalid
handle probe because the standard Java API has no invalid-handle sentinel.
Default-enabled Java passed 1/1 under each callback model against the
current-source JNI provider using `ddm-multi-attribute-tck.xml`,
`HLAinteger64Time`, `HLAobjectRoot.TckAttributeDimensionBaseObject.TckAttributeDimensionDerivedObject`, inherited `BaseValue`, and
`DerivedValue`. The paired installed-package C++ primary/contract slice passed
4/4 focused CTests and 4/4 direct executions across both models. Java evidence
is under
`out/java-tck/ownership-acquisition-publication-fence-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/ownership-acquisition-publication-fence-java-parity-20261116.{json,xml}`. These are bounded local-provider results, not
independent-provider behavior or an overall conformance claim.
The default `java-tck.acquisition-publication-guard-unavailable` case checks
the requester's publication guard through ownership-unavailable and
acquisition-notification terminal paths. It also exercises If Available on a
second object, checking callback timing and exact object, attribute, and tag
values while ownership remains with the original owner. Its direct mappings
cover IEEE 1516.1-2025 §§7.1.1, 7.1.4, and 7.8.4. Focused Java runs passed 1/1
under each callback model with `p0-tck.xml` and `HLAobjectRoot.TckObject.Value`;
the paired installed-package C++ primary/contract slice passed 4/4 focused
CTests and 4/4 direct runs. Java evidence is in
`out/java-tck/acquisition-publication-guard-unavailable-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/acquisition-publication-guard-unavailable-java-parity-20261116.{json,xml}`.
These local-provider results do not establish independent-provider behavior or
conformance.
The default `java-tck.selective-unpublication-pending-acquisition` case uses
the adapter's `ddm-multi-attribute-tck.xml` and checks that a pending request
blocks unpublishing its attribute while an unrelated published attribute can
be unpublished. A later acquisition of that withdrawn attribute fails with
`AttributeNotPublished`. It maps the source-refinement witness to IEEE
1516.1-2025 §5.3.3. Focused Java runs passed 1/1 under each callback model, and
the paired installed-package C++ primary/contract slice passed 4/4 focused
CTests and 4/4 direct runs. Java evidence is under
`out/java-tck/selective-unpublication-pending-acquisition-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/selective-unpublication-pending-acquisition-java-parity-20261116.{json,xml}`.
These local-provider results do not establish independent-provider behavior or
conformance.
The `java-tck.partial-attribute-ownership-transfer` case uses the
adapter-supplied multi-attribute FOM and logical-time implementation. It cancels
one attribute from a two-attribute acquisition, then verifies that If Wanted
divests and notifies only the remaining requested attribute. It is enabled by
default; focused local JNI runs passed under both callback models, and the paired
C++ primary and contract selectors passed all four callback-model runs. The
current C++ catalog mapping note describes negotiated-divestiture cancellation,
which this scenario does not perform, so the Java mirror makes no direct clause
claim while that local crosswalk remains unresolved.
The default-enabled `java-tck.ownership-repeat-acquisition-pending-preserves-state`
case covers two §7.8 states with the adapter's two-attribute FOM. A repeated
`{A,B}` acquisition starts only B while A is already acquiring, then again
while A's cancellation callback is pending. The Java case checks exact owner
release and cancellation callback subsets and unchanged ownership. It maps nine
direct page-162 witnesses to IEEE 1516.1-2025 §7.8. Java passed 1/1 under both
callback models; the paired C++ primary and contract passed 4/4 focused CTests
and 4/4 direct executions. Java evidence is in
`out/java-tck/ownership-repeat-acquisition-pending-preserves-state-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/ownership-repeat-acquisition-pending-preserves-state-java-parity-20261116.{json,xml}`.
These are local-provider results, not independent-provider interoperability
or conformance evidence.
The default-enabled `java-tck.negotiated-divestiture-partial-acquisition-cancellation`
case uses the adapter-supplied multi-attribute FOM. It cancels A from `{A,B}`,
checks the exact cancellation callback, then verifies evoked confirmation
contains only B and echoes the acquisition tag. Confirmation transfers only B;
the acquisition callback carries B and the confirmation tag, while A remains
with the owner. Its selected page-154, page-155, page-169, page-170, and page-171
witnesses map directly to IEEE 1516.1-2025 §§7.1.4, 7.3, 7.14, 7.15, and 7.16.
Java passed 1/1 under both callback models; the paired C++ primary and contract
passed 4/4 focused CTests and 4/4 direct executions. Java evidence is in
`out/java-tck/negotiated-divestiture-partial-acquisition-cancellation-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/negotiated-divestiture-partial-acquisition-cancellation-java-parity-20261116.{json,xml}`.
These are local-provider results, not independent-provider interoperability
or conformance evidence.
The default-enabled `java-tck.negotiated-divestiture-pre-delivery-cancellation`
case uses `p0-tck.xml`, HLAobjectRoot.TckObject, and Value. It cancels the
requester's acquisition before servicing callbacks, checks one exact
cancellation confirmation, and under HLA_EVOKED confirms neither the negotiated
confirmation nor ordinary release callback appears before or after callback
servicing. Ownership remains with the original owner. The paired C++ rows use
internal scenario identifiers and have no direct normative mapping; this Java
case preserves that disposition. Java passed 1/1 under both callback models, and
the C++ primary/contract passed 4/4 focused CTests and 4/4 direct runs. Java
evidence is in
`out/java-tck/negotiated-divestiture-pre-delivery-cancellation-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/negotiated-divestiture-pre-delivery-cancellation-java-parity-20261116.{json,xml}`.
These local-provider results do not establish independent-provider behavior or
conformance.
The `java-tck.negotiated-divestiture-cancellation` case follows the standard
two-federate acquisition path, checks the confirmation carries the acquisition
tag, cancels the negotiated divestiture, and verifies that the ordinary release
request is restored without changing ownership. It uses byte-array tags and
adapter-supplied federation inputs; provider runtime remains unverified.
The default-enabled `java-tck.negotiated-divestiture-no-accepter` case checks
the eligible publisher's exact assumption callback after the owner offers
negotiated divestiture, retained ownership while no acquirer exists, the exact
confirmation request after acquisition begins, and transfer only after the
owner confirms. Its direct mappings are page 156 line 50 to IEEE 1516.1-2025
§7.3, page 155 lines 164 and 167 to §7.3, and page 151 line 27 to §7.1.2.1.
Java passed 1/1 under both callback models; the paired C++ primary and contract
passed 4/4 focused CTests and 4/4 direct executions. Java evidence is in
`out/java-tck/negotiated-divestiture-no-accepter-java-parity-20261116/`; C++
evidence is in
`.build/cpp-tck-evidence-20261116/negotiated-divestiture-no-accepter-java-parity-20261116.{json,xml}`.
These are local-provider results, not independent-provider interoperability
or conformance evidence.
The default-enabled `java-tck.negotiated-divestiture-duplicate-request` case
verifies that a second divestiture request throws
`AttributeAlreadyBeingDivested`, the current owner can still send a tagged
update while the first request is pending, and the original request transfers
ownership only after `confirmDivestiture`. It maps the three selected
page-156 witnesses directly to IEEE 1516.1-2025 §7.3. Java passed 1/1 under
both callback models; the paired C++ primary passed 2/2 focused CTests and 2/2
direct executions. Java evidence is in
`out/java-tck/negotiated-divestiture-duplicate-request-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/negotiated-divestiture-duplicate-request-java-parity-20261116.{json,xml}`.
These are local-provider results, not independent-provider interoperability
or conformance evidence.
The default-enabled `java-tck.negotiated-divestiture-if-wanted-completion`
case exercises the alternate completion path: after the owner receives the
acquisition request and confirmation callback, it calls
`attributeOwnershipDivestitureIfWanted` instead of `confirmDivestiture`. It
checks the exact returned attribute set, ownership transfer, and acquisition
callback tag. It maps page 156 line 20 directly to IEEE 1516.1-2025 §7.3. Java
passed 1/1 under both callback models; the paired C++ primary passed 2/2
focused CTests and 2/2 direct executions. Java evidence is in
`out/java-tck/negotiated-divestiture-if-wanted-completion-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/negotiated-divestiture-if-wanted-completion-java-parity-20261116.{json,xml}`.
These are local-provider results, not independent-provider interoperability
or conformance evidence.
The default-enabled `java-tck.negotiated-willing-to-acquire-continuation` case
uses `p0-tck.xml`, HLAobjectRoot.TckObject, and Value. In HLA_EVOKED it checks
that canceling the first If Available candidate leaves the second candidate
selected; under HLA_IMMEDIATE it follows the regular pending-acquisition path
after the first request completes as unavailable. Both paths verify ownership
stays with the owner until confirmation and then transfers only to the second
candidate with the correct request and confirmation tags. The paired C++ rows
carry internal identifiers only, so no normative section is claimed. Java
passed 1/1 under both callback models; the C++ primary/contract passed 4/4
focused CTests and 4/4 direct runs. Java evidence is in
`out/java-tck/negotiated-willing-to-acquire-continuation-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/negotiated-willing-to-acquire-continuation-java-parity-20261116.{json,xml}`.
These local-provider results do not establish independent-provider behavior or
conformance.
The default-run `java-tck.resign-pending-acquisition-rejection` case verifies the
standard `OwnershipAcquisitionPending` failure for unconditional resign,
confirms the named requester remains joined, then cancels the request through
the specified resign action and checks that evoked cleanup produces no stale
owner-release callback. It uses the explicit-name Join overload, an
adapter-supplied ownership FOM, and the standard Java API. The three paired
`cpp-1516.1-*` references remain internal, with no direct normative section
mapping claimed. Focused runs passed both callback models on current-source
Umbra JNI; independent-provider behavior is unverified.
The default-run `java-tck.resign-cancel-if-available-pending` case resigns with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` while an If Available request is pending,
then confirms the current owner is unchanged and no unavailable/acquisition
callback remains queued after cancellation. Its three paired `cpp-1516.1-*`
references remain internal, with no direct normative section mapping claimed.
Focused runs passed both callback models on current-source Umbra JNI;
independent-provider behavior is unverified.
The default-run `java-tck.resign-cancel-negotiated-pending` case queues a
negotiated divestiture, resigns the requester with the cancellation action, and
checks after bounded owner callback servicing that neither
divestiture-confirmation nor release-request callbacks remain and ownership
stays with the original owner. Its three paired `cpp-1516.1-*` references remain
internal, with no direct normative section mapping claimed. Focused runs passed
both callback models on current-source Umbra JNI; independent-provider behavior
is unverified.
The `java-tck.divestiture-if-wanted-mixed-acquirers` scenario uses that same
adapter-supplied FOM. It verifies that If Wanted is a no-op without pending
acquirers, then transfers two attributes to independent requesters with the
exact divestiture tag. Under HLA_EVOKED, the second requester uses If Available;
under HLA_IMMEDIATE, a regular pending acquisition keeps both callback models at
the same pending-transfer boundary. It is enabled by default after focused local
JNI runs passed under both callback models and the paired C++ primary/contract
selectors passed all four callback-model runs.
The `java-tck.divestiture-if-wanted-return-set` case uses the same FOM and checks
that an empty If Wanted call transfers nothing, a partial return transfers only
the requested attribute, and a later acquisition requires a new If Wanted call.
It covers the selected direct §7.1.2.1 mapping and is enabled by default after
both Java callback-model runs and all four paired C++ primary/contract runs
passed.
The `java-tck.if-available-during-negotiated-divestiture` scenario is limited to
HLA_EVOKED. It checks that a repeated If Available request preserves the first
pending request, unpublishing is blocked until the terminal acquisition
notification, and ownership transfers only at Confirm Divestiture. It is enabled
for HLA_EVOKED; HLA_IMMEDIATE is recorded as unsupported because it cannot
preserve the queued-request ordering.
The `java-tck.if-available-deleted-object` scenario is also limited to
HLA_EVOKED. It queues an If Available request, deletes the object before that
request can be serviced, and checks for exactly one tagged removal callback
with no stale ownership callback. HLA_IMMEDIATE is recorded as unsupported
because it cannot preserve the queued-result ordering.
The `java-tck.ownership-after-fom-merge` scenario requires an adapter-supplied
extension FOM. It verifies that a join-added attribute on a preexisting object
starts unowned, publication alone does not transfer it, and the joiner's tagged
If Available acquisition notification establishes ownership. It is enabled for
HLA_EVOKED; HLA_IMMEDIATE is recorded as unsupported because it cannot preserve
the notification timing witness.
The default `java-tck.unconditional-attribute-ownership-divestiture` case uses
`fom-model-tck.xml` and five federates to exercise four typed attributes across
a regular acquisition, an initially rejected and later retried If Available
request, an invited publisher's assumption and acquisition, and an unpublished
subscriber that receives no assumption. It checks exact callback objects,
attribute sets, tags, and resulting ownership. Its direct mappings cover the
selected witnesses in §§7.1.2.2, 7.1.5, and 7.2; a same-attribute pending
candidate exclusion and the full acquisition matrix remain outside this case.
The contract twin reuses the primary witness. Current-source JNI runs passed
1/1 under each callback model; paired installed-package C++ primary/contract
selectors passed 4/4 focused CTests and 4/4 direct executions. Results are under
`out/java-tck/unconditional-attribute-ownership-divestiture-java-parity-20261116/`
and `.build/cpp-tck-evidence-20261116/unconditional-attribute-ownership-divestiture-java-parity-20261116.{json,xml}`.

The default-enabled `java-tck.unconditional-divestiture-assumption-coalescing`
case uses the configured two-attribute FOM to check that unconditional
divestiture produces exactly one full-set, correctly tagged assumption callback
at the eligible publisher. It also checks that the divesting owner and a
subscribed non-publisher receive no assumption callback. It maps four direct
IEEE 1516.1-2025 §7.4 witnesses (page 157, lines 79, 82, 97, and 103). Java
passed 1/1 under HLA_EVOKED and HLA_IMMEDIATE; the paired C++ primary and
contract passed 4/4 focused CTests and 4/4 direct executions across both
callback models. Java results are in
`out/java-tck/unconditional-divestiture-assumption-coalescing-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/unconditional-divestiture-assumption-coalescing-java-parity-20261116.{json,xml}`.
This local-provider evidence does not establish independent-provider behavior,
interoperability, or conformance.
The `java-tck.ownership-state-save-restore` case completes an ordinary negotiated
transfer before saving, restores that completed owner state, and confirms it
through both `isAttributeOwnedByFederate` and `queryAttributeOwnership`. Cleanup
unconditionally divests the acquirer and leaves the prior owner with `NO_ACTION`,
matching the portable C++ scenario's resign boundary.
The default-enabled `java-tck.attribute-ownership-acquisition-cancellation`
case checks ordinary acquisition cancellation, its grouped confirmation,
preserved ownership, rejection of unconditional divestiture while acquisition
is pending, and the race where If-Wanted divestiture completes before
cancellation. The race checks the exact tagged acquisition notification and
transferred ownership. It maps direct witnesses to IEEE 1516.1-2025
§§7.1.2.2, 7.15, 7.16, and the evoked publication guard in §7.8.4. Java passed
1/1 under each callback model; the paired C++ primary/contract passed 4/4
focused CTests and 4/4 direct executions. Results are in
`out/java-tck/attribute-ownership-acquisition-cancellation-java-parity-20261116/`
and `.build/cpp-tck-evidence-20261116/attribute-ownership-acquisition-cancellation-java-parity-20261116.{json,xml}`.
These are local-provider results, not independent-provider or conformance evidence.
The default-enabled `java-tck.attribute-ownership-release-denied` case checks
the owner's exact Release request, the denial tag and unavailable callback,
and unchanged ownership. Its multi-acquirer phase verifies that one denial
terminates both pending acquisitions with one exact unavailable callback each.
It maps direct witnesses to IEEE 1516.1-2025 §§7.1.2.2 and 7.12. Java passed
1/1 under each callback model with the typed model FOM and HLAfloat64Time; the
paired C++ primary/contract passed 4/4 focused CTests and 4/4 direct executions.
Results are in
`out/java-tck/attribute-ownership-release-denied-java-parity-20261116/` and
`.build/cpp-tck-evidence-20261116/attribute-ownership-release-denied-java-parity-20261116.{json,xml}`.
These local-provider results do not establish independent-provider behavior or
conformance.
The `java-tck.attribute-ownership-query-owner-report` case checks the exact
`informAttributeOwnership` object, attribute set, and federate handle, then
rechecks that querying did not change ownership.
The `java-tck.ownership-query-partition-cleanup` scenario uses the adapter's
multi-attribute FOM configured with `hla.rti.tck.multiAttributeFom` and the
multi-attribute class/name properties. It checks the owned/unowned callback
partition, invalid object/attribute failures, and that removal suppresses stale
query results.
The `java-tck.ownership-query-save-restore-results` scenario uses the
adapter-supplied multi-attribute FOM configured with
`hla.rti.tck.multiAttributeFom` and the existing multi-attribute class/attribute
name properties. It saves an owner/unowned split, changes the live owner,
restores the saved state, then checks federate-owned, unowned, and RTI-owned
MOM query results through the standard callbacks.
The `java-tck.ownership-acquisition-cancellation-transfer-race` case uses the
adapter's ordinary object class and attribute. With evoked callbacks, it
cancels while the owner's release callback is active, then checks the exact
transfer notification and absence of a stale cancellation confirmation.
Immediate callbacks verify the transfer result without a concurrent
cancellation. Both modes passed focused JNI runs; this is not an
independent-provider or conformance claim.
The separate `java-tck.ownership-acquisition-cancellation-denial-race` case
checks the exact unavailable reply when the owner denies release after the
requester's cancellation call completes. It runs with evoked callbacks only;
immediate mode reports unsupported because it cannot express this callback
boundary race. It does not claim the three-reply aggregate obligation.
The default-enabled, HLA_EVOKED-only
`java-tck.ownership-acquisition-cancellation-mixed-replies` case mirrors the
C++ aggregate: one cancellation spans three adapter-named attributes and
produces disjoint cancellation, acquisition, and unavailable callbacks. It
checks each exact subset and tag, full requested-set coverage, and final
ownership. Java passed 1/1 with `p0-tck.xml`, HLAinteger64Time, and
HLAobjectRoot.TckObject Value/Name/PayRate; the paired C++ primary/contract
passed 2/2 focused CTests and 2/2 direct executions under HLA_EVOKED. It maps
IEEE 1516.1-2025 §7.15, page 170 line 16. Results are in
`out/java-tck/ownership-acquisition-cancellation-mixed-replies-java-parity-20261116/`
and `.build/cpp-tck-evidence-20261116/ownership-acquisition-cancellation-mixed-replies-java-parity-20261116.{json,xml}`.
This local-provider evidence does not cover HLA_IMMEDIATE, independent-provider
behavior, or conformance.
The `java-tck.auto-provide` case verifies the configured standard Auto Provide
switch and two object discoveries, first with Attribute Scope Advisory disabled
and then enabled. Each discovery yields one provider callback carrying the same
two subscribed attributes and an empty tag under either callback model.
The `java-tck.auto-provide-disabled-explicit-request` case verifies discovery
continues with Auto Provide disabled, while explicit object- and class-scoped
value requests each produce one callback preserving their supplied tag.
The companion discovery-only case verifies that disabling Auto Provide
suppresses only the provider solicitation; normal object discovery still arrives.
The inherited-attribute projection case checks base and derived subscriptions
against a derived object update, including handle stability, identity round-trips,
callback metadata, and the exact inherited/derived attribute sets.
The `java-tck.delay-subscription-evaluation-interaction` case checks the standard
switch with a composed adapter FOM, callback-boundary delivery after subscription,
and suppression when an interaction is unsubscribed before queued callbacks are
serviced. Supply the switch declaration module with
`-Dhla.rti.tck.switchesFom=<adapter FOM>`; no provider-specific API is used.
The companion `java-tck.delay-subscription-evaluation-attribute-update` case
checks known-object discovery, late ordinary attribute subscription, callback
metadata, and suppression after unsubscribe under the same standard switch.
The timestamped attribute counterpart evaluates the same declaration change at
the constrained time-grant boundary and checks timestamp, order, retraction, and
delivery metadata. It uses no DDM regions.
The timestamped interaction counterpart checks delivery after late subscription
at the constrained grant and confirms unsubscribe suppresses a later queued
interaction, including its timestamp and retraction metadata.
The ordinary directed-interaction counterpart exercises by-ownership target
selection, late subscription, and callback-boundary unsubscribe suppression;
it uses no DDM regions.
Its timestamped counterpart applies the same selector checks at constrained
time grants and verifies target, timestamp, order, retraction, and producer
metadata, also without DDM.
The focused `java-tck.asynchronous-delivery` scenario gates receive-order
attribute reflection while asynchronous delivery is disabled, then verifies
release on re-enablement, callback servicing in evoked mode, and the repeated
disable/re-enable boundary. It uses only the 2025 Java API and is default-run
after installed-package passes in both callback models on 2026-10-22. Its
direct witnesses are §§8.15.3/8.16.5; it does not claim timestamped delivery
or cross-provider conformance. Provider, FOM, endpoint, time, and callback
configuration remain adapter inputs.
The default-run `java-tck.asynchronous-delivery-tso-exclusion` case separately
checks that RO interaction progress does not release TSO interactions before
their logical-time gate, including TAR's equal-time cutoff (§§8.1.5, 8.14.4).
It passed both callback models on the installed package on 2026-10-22;
interaction classes, parameters, time, and callback configuration remain
adapter inputs. It does not assert asynchronous TSO dispatch.
The companion `java-tck.asynchronous-delivery-reenable-tso-gate` is also
default-run after evoked and immediate installed-package passes. It checks
that disabling then re-enabling asynchronous delivery releases the RO control
without bypassing the TSO time gate, and that TAR releases the timestamped
event before its grant (§§8.1.5, 8.14.4, 8.15.3, 8.16.5).
Two further default-run Time Advancing RO cases check the pending-TAR
boundary: enabling asynchronous delivery releases an RO interaction before
the grant (§8.15.3), and disabling it during Time Advancing does not revoke
that RO callback's eligibility (§8.16.5). Both passed evoked and immediate
installed-package runs on 2026-10-22; neither asserts TSO delivery or
network-arrival timing.
The adapter-backed `java-tck.update-rate-queries` scenario checks named-rate
lookup, active and passive named subscriptions, default-rate behavior,
per-attribute rate reset, and invalid designator/object/attribute failures. Its
active `High` subscription is established before object registration, and the
subscriber's per-attribute rate is checked against the resolved FOM value.
Direct mappings cover page 251 line 74 to §10.11.6 for named-rate lookup, page
252 line 17 to §10.12 for the per-attribute query after named subscription, and
page 97 line 23 to §5.8 for the default empty designator's zero-rate boundary.
The separate page 97 line 20 to §5.8 mapping witnesses the paired C++
`named-update-rate-subscription` case. Five focused current-source Umbra JNI
runs passed per callback model; the paired C++ CTests and direct run passed both
models. Results are under
`out/java-tck/update-rate-queries-java-port-20261106/current-source/`;
other-provider behavior is unverified.
The `java-tck.fom-handle-uniqueness` case checks pairwise-distinct encoded values for representative object-class, attribute, interaction-class, parameter, dimension, and transportation-type handles, plus matching values across two federates. It then destroys the first execution, creates a second with the same FOM on the same connected RTI instance, and compares the same 11 FDD-derived encodings. Direct mappings cover IEEE 1516.1-2025 §10.1.1 for the bounded uniqueness witness and §12.2 for the named cross-execution values. `OrderType` stays a separate fixed-enum check, outside handle uniqueness. Focused current-source JNI runs passed under both callback models; results are under `out/java-tck/fom-handle-uniqueness-java-port-20261106/results/`.
The default-run `java-tck.fom-transportation-handle-stability` scenario checks
that `HLAreliable`, `HLAbestEffort`, and the base custom transportation have
distinct handles and stable names before and after an additional-module join.
It also verifies the extension transportation is unavailable before the join,
then has one shared, distinct handle and round-trips by name on both federates.
Supply the rich base and extension modules through `hla.rti.tck.fom` and
`hla.rti.tck.additionalFom`, with names in `hla.rti.tck.customTransportationType`
and `hla.rti.tck.fomAdditionalTransportationType`. Focused runs passed under
both callback models against current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-transportation-handle-stability-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
The direct mappings cover selected lookup and handle stability only; this does
not establish runtime reliability guarantees or conformance.
The default-run `java-tck.fom-module-composition` scenario composes the p0 and
extension modules from absolute file URLs at Create and at a later Join. It
checks the expected base and composed object/interaction/attribute/parameter
tables, name round-trips, and shared handles across members, plus standard MIM
handle stability and the attribute-scope advisory default. The Create path
also verifies extension attribute value, tag, and `HLAreliable` reflection,
ordinary interaction delivery, and a directed interaction; directed delivery
is checked at Create and after Join. The modules and extension declarations are
adapter inputs. Focused current-source JNI runs passed under both callback
models. Results/JUnit are in
`out/java-tck/fom-module-composition-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
The direct mappings remain limited to the selected fixture declarations and
module operations; they do not establish arbitrary FOM table completeness or
conformance.
The default-run `java-tck.fom-file-url-designator` case passes an absolute local
FOM URL with a percent-escaped first filename character to standard Create,
checks the exact `HLAFOMmoduleDesignatorList` value before and after an
additional-module Join, and verifies `HLAMIMdesignator` is
`HLAstandardMIM` when Create omits an explicit MIM. Its two direct mappings
cover IEEE 1516.1-2025 §4.1.4.2. Default runs passed 1/1 under both callback
models against current-source JNI with p0-tck.xml, HLAstandardMIM-2025.xml,
fom-extension-tck.xml, and HLAinteger64Time. Supply the extension with
`--jvm-argument=-Dhla.rti.tck.additionalFomModules=<path>`. Results/JUnit are
in `out/java-tck/fom-file-url-designator-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
The paired C++ primary and contract CTests passed 4/4 against the installed
package. Local-provider evidence does not establish independent-provider
interoperability or conformance.
The federate MOM publication-query cases check ordinary object and interaction
publication reports, including partial/full attribute withdrawal and whole-class
withdrawal. The opt-in partial interaction-class withdrawal case publishes two
adapter-named ordinary classes, unpublishes the secondary, and checks that the
fresh MIM report contains only the retained primary class. Supply the second
class with `--jvm-argument=-Dhla.rti.tck.secondaryInteractionClass=<class>`;
FOM, MIM, names, provider, and callback model remain adapter supplied.
The companion full-withdrawal case removes both classes and checks the exact
MIM NULL response with one empty interaction-class list.
The multi-directed-publication query publishes adapter-selected directed
interactions for two object classes and verifies both standard MIM report rows,
their object-class identities, interaction lists, and total class count. Supply
the second object class with
`--jvm-argument=-Dhla.rti.tck.secondaryDirectedObjectClass=<class>`; the second
directed interaction uses `hla.rti.tck.directedInteractionClass`.
The partial-unpublication companion removes the inherited interaction only from
the derived class and verifies that its class-specific interaction and the
base-class inherited publication remain in the two MIM report rows.
The full-unpublication case first verifies that partial state, then removes all
directed publications from both classes and checks the exact zero-count NULL
report while ordinary publications remain unchanged.
The matching directed-subscription query subscribes to one adapter-selected
interaction under each object class, verifies both inactive standard MIM rows,
then set-unsubscribes the primary pair and verifies only the secondary row
remains. It also checks the unchanged NULL reports for ordinary subscriptions.
Its full-unsubscribe companion preserves both earlier query states, removes the
remaining class subscription with the class-scoped standard API, and checks the
exact two-parameter directed NULL response (zero class count and empty list)
alongside unchanged ordinary subscription NULL reports.
The full interaction-unsubscription companion removes the remaining active
application class after the passive MIM class, then checks the empty interaction
list NULL report while object and directed subscription reports remain present.
The passive-to-active activation case checks the interaction-list state before
and after activation while preserving the active object-attribute report and
empty directed-subscription response. Supply the adapter's rate FOM with
`-Dhla.rti.tck.updateRateFom=<FOM>` and its designator with
`-Dhla.rti.tck.updateRateDesignator=<name>`.
The complete object-class subscription case requests three distinct
adapter-configured attributes and verifies that exact active set and rate in
the standard MIM report; configure the second and third attributes with
`hla.rti.tck.objectClassSecondAttribute` and
`hla.rti.tck.objectClassThirdAttribute`.
The default-run `java-tck.object-attribute-subscription-lifecycle` scenario
requires a distinct second attribute through
`hla.rti.tck.objectClassSecondAttribute`. It publishes and updates both
attributes while subscribing to only the primary one. Activation checks the
retained subscribed value without requiring the original update tag; later
ordinary reflections check the tag, producer, and transportation metadata.
Focused installed-provider evoked and immediate runs passed; independent-
provider conformance is not claimed.
The default-run `java-tck.attribute-update-service-boundaries` case checks
ordinary Update/Reflect payload, tag, and producing-federate metadata across
active and passive subscriptions using adapter-supplied FOM names. It is mapped
to IEEE 1516.1-2025 §§5.8, 6.10, and 6.11.1; passive delivery after active
relevance is established remains a supplemental assertion. Focused installed-
provider evoked and immediate runs passed, without an interoperability claim.
The ownership-acquisition-if-available scenario uses a locally deleted object
as its unknown-instance fixture and a valid attribute from another class for
its undefined-attribute boundary. The adapter may set
`hla.rti.tck.foreignAttributeClass` and `hla.rti.tck.foreignAttribute`; their
defaults are the standard MIM `HLAobjectRoot.HLAmanager.HLAfederate` class
and `HLAfederateHandle` attribute. The ownership-query partition/cleanup case
uses the same foreign-attribute settings for its undefined-attribute boundary
and queries the real object handle after removal for its unknown-object boundary.
The If Available case also covers the HLA_EVOKED pending transition when a
regular acquisition follows, then checks the three terminal outcomes: release
denial reports unavailable, Divestiture If Wanted transfers ownership, and
cancellation confirms without changing ownership. The immediate model still
checks owned-attribute unavailability and acquisition after unconditional
divestiture. Nine direct mappings cover §§7.9 and 7.1.2.2. Focused local JNI
runs passed in both callback models with the adapter FOM's logical-time type
matched to the provider; evidence is in
`.build/java-tck-20261117/ownership-acquisition-if-available-override-parity-`
`{evoked,immediate}.{json,xml}`. This is local-provider evidence only.
The ownership-query save/restore case uses the adapter's multi-attribute FOM,
logical-time implementation, and callback model. It joins with explicit
federate names so restored identity reports can be checked exactly.
The default `java-tck.attribute-subscription-invocations-accumulate` case
subscribes to the configured primary attribute, waits for discovery, then
subscribes to the distinct configured second attribute and verifies one update
reflects both exact values and tag using the subscriber's locally resolved
standard handles. Its FOM and names come from the adapter. Focused installed
JNI runs passed in both callback modes; active/passive mode replacement and
independent-provider conformance remain unverified.
The default `java-tck.attribute-subscription-per-attribute-mode-transitions`
case keeps both attribute subscriptions while switching their active/passive
modes, including empty active and passive calls. It checks each reflected
attribute set, value, and tag against receiver-local standard handles. The
adapter supplies the FOM, logical-time implementation, names, provider,
endpoint, and callback model. Focused installed JNI runs passed in both modes;
direct mappings cover IEEE 1516.1-2025 §5.1.2 p. 82 l. 80, §9.1.5 p. 223
l. 103, and §5.8 p. 96 l. 129. The regional rule at p. 223 l. 109 remains
outside this case. Independent-provider conformance remains unverified.
The default `java-tck.passive-subscription-no-start-registration` case enables
the publisher's standard object-class relevance advisory switch if needed,
then uses an active subscription as a positive control for Start Registration.
It clears that control and verifies that a passive subscription produces no new
Start Registration callback. It does not claim a Stop Registration effect.
Focused installed JNI runs passed in both callback models; independent-provider
conformance remains unverified.
The default `java-tck.object-publication-registration-fence` case rejects
registration while a class is unpublished, then republishes it and confirms
registration, discovery, and instance-name/class lookups. Discovery identity
uses the subscriber's own standard object handle, resolved from the publisher's
object name. Focused installed JNI runs passed in both callback models;
independent-provider conformance remains unverified.
The default `java-tck.selective-attribute-unsubscription` case first confirms
that two federates independently receive both attributes, removes only the
configured primary attribute for one, and verifies that its secondary value
continues while the other federate still receives both. It uses receiver-local
handles through only the standard Java API. Focused installed JNI runs passed
in both callback modes; independent-provider conformance remains unverified.
The default `java-tck.whole-class-attribute-unsubscription` case confirms both
configured values and the tag are initially reflected, invokes the standard
whole-class unsubscribe service, and verifies that later updates produce no
reflection. It resolves the receiver's local object and attribute handles and
passed focused installed JNI runs in both callback modes; independent-provider
conformance remains unverified. The default
`java-tck.empty-attribute-unsubscription-is-no-op` case submits an empty set
to the standard attribute-set unsubscribe service and verifies both previously
subscribed values continue to arrive using receiver-local handles. Focused
installed JNI runs passed in both callback modes; the passive-state branch
and independent-provider conformance remain unverified.
The default `java-tck.empty-attribute-subscription-is-no-op` case creates an
active non-empty attribute subscription, submits an empty active set, and
verifies the original subscription still delivers the next tagged update using
the subscriber's local standard handles. The adapter supplies the FOM, standard
MIM, object class/attribute names, provider, logical-time implementation,
endpoint, and callback model. Five focused current-source Umbra JNI runs passed
under each callback model; passive-state preservation and independent-provider
conformance are unverified.
The passive companion uses the standard passive attribute-set subscription
operation and verifies that the same complete attribute list and rate are
reported with the active flag false.
The complete-set activation case first verifies that passive report, then uses
the standard active subscription call and requires the next MIM report to retain
all three attributes and the rate designator with active=true.
The partial attribute-unsubscribe case starts with two active attributes, removes
the first through the standard attribute-set service, and verifies that only the
second remains with the same active state and update-rate designator.
Its full-unsubscribe companion removes both attributes in one standard set call,
then checks the zero-class object-subscription NULL response and unchanged
interaction and directed-subscription NULL reports.
The multiple-interaction-subscription case checks an active adapter-selected
application interaction and a passive standard MIM service-report interaction
in the same report, plus the unchanged empty object and directed subscription
NULL reports.
The companion staged-unsubscribe case removes the passive service-report class,
queries again, and verifies the active application interaction is the sole
remaining interaction subscription while the other two NULL reports remain.
The default-run `java-tck.fom-additional-module-join-atomicity` case mirrors
the C++ Join checks. It rejects the adapter's 3 invalid and 11 standard-invalid
FOM inputs without admitting the member or extension class, accepts an identical
duplicate attribute declaration, rejects attribute-order, interaction-order,
transportation, and dimension-bound conflicts, and then accepts the valid
extension Join. Supply the lists with `hla.rti.tck.invalidFomModules` and
`hla.rti.tck.standardInvalidFomModules`; pass the duplicate, conflict, and
dimension fixtures through their `hla.rti.tck.fomAdditionalJoin*` properties.
Focused default runs passed under both callback models against current-source
Umbra JNI. Results/JUnit are in
`out/java-tck/fom-additional-module-join-atomicity-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
The mappings cover these selected fixture outcomes only; they do not establish
exhaustive FOM validation or conformance.
The default-run `java-tck.fom-invalid-composite-join-atomicity` case joins
with the valid extension module combined with each of the three adapter invalid
FOM inputs. Each rejected attempt leaves the new federate and extension class
absent; a valid follow-up Join then yields shared class and member handles.
Direct mappings cover IEEE 1516.1-2025 §§4.1.4 and 4.1.4.1 for these selected
fixtures. Focused default runs passed under both callback models against
current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-invalid-composite-join-atomicity-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
Coverage is limited to the three adapter inputs.
The default `java-tck.fom-incomplete-class-definition` scenario mirrors the
public C++ TCK case for schema-valid object and interaction class scaffolds.
It rejects each unfinished module union at Create and additional-module Join,
then retries with its companion module and checks that the completed class and
member are visible. Supply the base FOM and four scaffold/provider modules via
`hla.rti.tck.fom`, `hla.rti.tck.incompleteObjectClassScaffoldFom`,
`hla.rti.tck.completeObjectClassProviderFom`,
`hla.rti.tck.incompleteInteractionClassScaffoldFom`, and
`hla.rti.tck.completeInteractionClassProviderFom`. Its bounded direct mapping is
IEEE 1516.1-2025 §4.1.4.1, covering these unfinished class-union outcomes only.
Focused default runs passed in both callback models on the current-source JNI
build; results are in
`out/java-tck/fom-incomplete-class-definition-current-source-20261115/`.
Independent-provider behavior and conformance remain unverified.
The default-run `java-tck.fom-attribute-table-nonblank-semantics` scenario mirrors
the public C++ check for an empty attribute-table Semantics element. Supply
`attribute-table-empty-semantics-tck.xml` through
`hla.rti.tck.attributeTableEmptySemanticsFom`; Create Federation Execution must
reject it as `InconsistentFOM`. Focused default runs passed under both callback
models on current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-attribute-table-nonblank-semantics-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
The earlier 2026-11-05 provider accepted the fixture; this result is specific
to the tested build and fixture.
The default-run `java-tck.fom-reference-data-type-name-convention` scenario accepts
the adapter's valid reference-data-type name, then requires `InconsistentFOM`
for the paired DIF-valid FOM whose name contains a period. Supply the files via
`hla.rti.tck.referenceDataTypeValidNameFom` and
`hla.rti.tck.referenceDataTypeInvalidNameFom`. Focused default runs passed
under both callback models against current-source Umbra JNI; results/JUnit are
in `out/java-tck/fom-reference-data-type-name-convention-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
The evidence covers this name restriction only; independent-provider behavior
and conformance are not claimed.
The default-run `java-tck.fom-simple-data-type-name-convention` scenario accepts
the valid control and rejects three selected invalid Simple Data Type names:
one containing a period, one beginning with a mixed-case `HLA` prefix, and one
matching mixed-case `NA`. Supply the control and invalid FOMs through
`hla.rti.tck.simpleDataTypeValidNameFom`,
`hla.rti.tck.simpleDataTypeInvalidNameFom`,
`hla.rti.tck.simpleDataTypeReservedPrefixNameFom`, and
`hla.rti.tck.simpleDataTypeReservedNaNameFom`. Focused default runs passed
under both callback models against current-source Umbra JNI; results/JUnit
are in `out/java-tck/fom-simple-data-type-name-convention-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
These examples do not establish exhaustive name-convention coverage,
independent-provider behavior, or conformance.
The default-run `java-tck.fom-model-identification-name-valid` scenario supplies a
schema-valid FOM with a nonempty HLA/XML-compatible Model Identification Name
to public Create and destroys the resulting federation. Supply
`model-identification-name-valid-tck.xml` through
`hla.rti.tck.modelIdentificationNameValidFom`. Focused runs passed under both
callback models against current-source JNI and the pinned Java API with
HLAinteger64Time. Results/JUnit are in
`out/java-tck/fom-model-identification-name-valid-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This covers the valid input path only; independent-provider behavior and
conformance are not claimed.
The default-run `java-tck.fom-enumerated-data-type-name-convention` scenario
accepts a valid Enumerated Data Type name and rejects its period-containing
counterpart as `InconsistentFOM`. Supply the paired fixtures through
`hla.rti.tck.enumeratedDataTypeValidNameFom` and
`hla.rti.tck.enumeratedDataTypeInvalidNameFom`. Focused default runs passed
under both callback models on current-source Umbra JNI; results/JUnit are in
`out/java-tck/fom-enumerated-data-type-name-convention-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
This covers the period restriction only; independent-provider behavior and
conformance are not claimed.
The default-run `java-tck.fom-enumerator-name-convention` scenario accepts the
valid enumerator-name control and rejects its period-containing counterpart
as `InconsistentFOM`. Supply the fixtures through
`hla.rti.tck.enumeratorValidNameFom` and
`hla.rti.tck.enumeratorInvalidNameFom`. Focused default runs passed under both
callback models on current-source Umbra JNI; results/JUnit are in
`out/java-tck/fom-enumerator-name-convention-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
Coverage is limited to this prohibited character and enumerator row;
independent-provider behavior and conformance are not claimed.
The default-run `java-tck.fom-array-data-type-name-convention` scenario accepts a
valid Array Data Type name and rejects its period-containing counterpart as
`InconsistentFOM`. Supply the paired fixtures through
`hla.rti.tck.arrayDataTypeValidNameFom` and
`hla.rti.tck.arrayDataTypeInvalidNameFom`. Focused default runs passed under
both callback models against current-source Umbra JNI; results/JUnit are in
`out/java-tck/fom-array-data-type-name-convention-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
Coverage is limited to this period restriction; independent-provider behavior
and conformance are not claimed.
The default-run `java-tck.fom-array-element-type-reference` scenario accepts
the control Array Data Type that names the standard `HLAbyte` type and rejects
the otherwise identical FOM with an undeclared Element Type as `InconsistentFOM`.
Supply the fixtures through `hla.rti.tck.arrayElementTypeValidFom` and
`hla.rti.tck.arrayElementTypeInvalidFom`. Focused default runs passed under
both callback models against current-source Umbra JNI; results/JUnit are in
`out/java-tck/fom-array-element-type-reference-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
This covers the single-module reference only; independent-provider behavior
and conformance are not claimed.
The default-run `java-tck.fom-variant-record-name-convention` scenario accepts
a valid Variant Record Name and rejects its period-containing counterpart as
`InconsistentFOM`. Supply the fixtures through
`hla.rti.tck.variantRecordValidNameFom` and
`hla.rti.tck.variantRecordInvalidNameFom`. Focused runs passed under both
callback models against current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-variant-record-name-convention-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This covers the tested period restriction only; independent-provider behavior
and conformance are not claimed.
The default-run `java-tck.fom-variant-discriminant-name-convention` scenario accepts
a valid Variant Record Discriminant Name and rejects its period-containing
counterpart as `InconsistentFOM`. Supply the fixtures through
`hla.rti.tck.variantDiscriminantValidNameFom` and
`hla.rti.tck.variantDiscriminantInvalidNameFom`. Focused runs passed under
both callback models against current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-variant-discriminant-name-convention-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This covers the tested period restriction only; independent-provider behavior
and conformance are not claimed.
The default-run `java-tck.fom-variant-alternative-name-convention` scenario accepts
a valid Variant Record Alternative Name and rejects its period-containing
counterpart as `InconsistentFOM`. Supply the fixtures through
`hla.rti.tck.variantAlternativeValidNameFom` and
`hla.rti.tck.variantAlternativeInvalidNameFom`. Focused runs passed under
both callback models against current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-variant-alternative-name-convention-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This covers the tested period restriction only; independent-provider behavior
and conformance are not claimed.
The default-run `java-tck.fom-variant-alternative-type-category` scenario accepts
the enumerated `HLAboolean` Alternative Type control and rejects the otherwise
schema-valid alternative using basic `HLAinteger32BE` as `InconsistentFOM`.
Supply the fixtures through `hla.rti.tck.variantAlternativeTypeValidFom` and
`hla.rti.tck.variantAlternativeTypeInvalidFom`. Focused runs passed under both
callback models against current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-variant-alternative-type-category-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
Coverage is limited to this category boundary; independent-provider behavior
and conformance are not claimed.
The default-run `java-tck.fom-variant-record-encoding` scenario accepts a valid FOM
using `HLAvariantRecord` and `HLAextendableVariantRecord`, then rejects the
paired predefined `HLAfixedRecord` mismatch as `InconsistentFOM`. Supply the
fixtures through `hla.rti.tck.variantRecordEncodingValidFom` and
`hla.rti.tck.variantRecordEncodingInvalidFom`. Focused runs passed under both
callback models against current-source Umbra JNI. Results/JUnit are in
`out/java-tck/fom-variant-record-encoding-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This covers marker validation only, not detailed byte layout; independent-provider
behavior and conformance are not claimed.
The opt-in `java-tck.fom-synchronization-table-validation` scenario accepts a
FOM synchronization row with `Capability=NA`, rejects `Capability=Achieve` and
a user label beginning with the reserved `HLA` prefix, then retries the valid
FOM under the same federation name. Supply the fixtures through
`hla.rti.tck.synchronizationCapabilityNaFom`,
`hla.rti.tck.synchronizationCapabilityActionFom`, and
`hla.rti.tck.synchronizationLabelInvalidFom`. The current-source JNI provider
accepted the `Capability=Achieve` fixture in both callback models, so the
expected `InconsistentFOM` assertion failed and the label/retry checks were not
reached. Results/JUnit are in
`out/java-tck/fom-synchronization-table-validation-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
Earlier runs passed on provider build umbra-jni-20261106. The SOM Capability
rule is separate from these FOM assertions.
The default-run `java-tck.fom-transportation-description-ignored` scenario mirrors
the C++ Create-time union and additional-module Join case for two declarations
with the same transportation name and reliability but different `Semantics`.
Supply the fixtures through `hla.rti.tck.transportationDuplicateBaseFom` and
`hla.rti.tck.transportationDescriptionDifferentFom`. The Java test checks
public transportation handle/name lookup and does not assert which description
is retained. Focused runs passed under both callback models against current-source
Umbra JNI, including both Create and Join paths. Results/JUnit are in
`out/java-tck/fom-transportation-description-ignored-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
Runs against the older umbra-jni-20261106 build failed at Create; the current-source
evidence supersedes that result. No independent-provider or conformance claim is made.
The default `java-tck.fom-automatic-resign-action-setting` scenario rejects an
unsupported `resignAction` value with `InvalidFOM`, then accepts each of the six
defined settings and checks the joined federate's public
`getAutomaticResignDirective` value. Its direct mappings cover the Automatic
Resign Action switches-table row and its values in IEEE 1516.2-2025 §6.2.13 and
§4; they do not cover every FOM row or the SOM optional/NA rules. Supply the
seven FOM modules through the `hla.rti.tck.resignAction*Fom` properties and the
pinned MIM through `--mim-path`. Default focused runs passed in both callback
models on the current-source Umbra JNI build; results are in
`out/java-tck/fom-automatic-resign-action-setting-java-parity-20261115/`.
The default `java-tck.fom-root-hierarchy` case accepts standard object and
interaction roots, round-trips their child-class names through public handle
lookups, rejects nonstandard top-level object and interaction classes, and
creates a valid federation under the same name after each rejection. Its
bounded direct mappings are IEEE 1516.2-2025 §§4.5.1 and 4.6.1. The Java API
does not expose C++ `isValid()` on handles, so successful name/handle round
trips are the public validity witness. Focused default runs passed in both
callback models on current-source Umbra JNI; results are in
`out/java-tck/fom-root-hierarchy-java-parity-20261115/`. Independent-provider
behavior and conformance remain unverified.
The default `java-tck.fom-equivalent-class-conflict` case accepts an equivalent
duplicate object-class module through public handle/name lookup, rejects a
same-identity class with conflicting sharing, and confirms valid Create works
again under the same federation name. It directly covers IEEE 1516.2-2025
§C.2 for this class and conflict only. Focused default runs passed in both
callback models on current-source Umbra JNI; results are in
`out/java-tck/fom-equivalent-class-conflict-java-parity-20261115/`.
Independent-provider behavior and conformance remain unverified.
The default `java-tck.fom-inherited-member-names` case verifies stable
inherited attribute and parameter handles/names, rejects separate FOMs that
overload each inherited name, and retries valid federation creation under the
same federation name. It maps to IEEE 1516.2-2025 §§6.2.5 and 6.2.6 for one
attribute and one parameter witness. Supply the valid, duplicate-attribute,
and duplicate-parameter modules through the `hla.rti.tck.inheritedMemberNames*`
properties. Focused default runs passed in both callback models on
current-source Umbra JNI; results are in
`out/java-tck/fom-inherited-member-names-java-parity-20261115/`.
Independent-provider behavior and conformance remain unverified.
The default-run `java-tck.fom-attribute-value-required-sharing` case accepts
the paired valid FOM's explicit-false and omitted Value Required rows, rejects
`sharing=Neither` with `valueRequired=true` as `InconsistentFOM`, then recreates
the valid federation under the same name. Its direct mapping covers IEEE
1516.2-2025 §4 for these Create outcomes. Supply the fixtures through
`hla.rti.tck.attributeNeitherValueRequiredValidFom` and
`hla.rti.tck.attributeNeitherValueRequiredTrueFom`. Focused default runs passed
under both callback models on current-source Umbra JNI; results/JUnit are in
`out/java-tck/fom-attribute-value-required-sharing-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
Independent-provider behavior and conformance are not claimed.

The opt-in `java-tck.fom-tag-data-type-categories` case accepts the six
supported user-tag and synchronization-tag data-type categories, rejects raw
basic user and synchronization tag types, and rejects an XSD-valid FOM
synchronization point with `Register` capability. It retries the valid FOM
under the same federation name after each rejection. Its direct mappings cover
IEEE 1516.2-2025 §§4, 6.2.9, and 6.2.10 for these fixtures. Supply the valid,
two raw-basic, and Register-capability modules through
`hla.rti.tck.tagDataTypeValidFom`, `tagDataTypeBasicUserFom`,
`tagDataTypeBasicSynchronizationFom`, and
`tagSynchronizationCapabilityRegisterFom`. Explicit `--run-unverified`
executions in both callback models reached the third rejection and failed:
current-source Umbra JNI accepted the `Register` fixture where the C++ case
expects `InconsistentFOM`. The scenario stays opt-in until that provider
behavior is corrected; results/JUnit are in
`out/java-tck/fom-tag-data-type-categories-java-parity-20261115/{evoked,immediate}-unverified.{results.json,junit.xml}`.
The no-user-tag/no-tag Data type NA recommendations, runtime tag-byte
delivery, alternate capabilities, independent-provider behavior, and
conformance are outside this evidence.
The opt-in `java-tck.fom-interaction-class-hierarchy` case resolves the
three-level `TckTypedInteraction` hierarchy from `fom-model-tck.xml`, checks
qualified interaction names round-trip through distinct handles, and verifies
that inherited `IntegerParameter` and `DerivedParameter` handles and names
remain stable through the derived and grandchild classes. Both focused callback
models passed on provider build `umbra-jni-20261106`; results are in
`out/java-tck/fom-interaction-class-hierarchy-java-port-20261113/`. Its direct
mappings are IEEE 1516.2-2025 §§4.3.1 and 4.6.2; the checks cover this FOM
witness only.
The opt-in `java-tck.service-report-file-switch-lifecycle` scenario checks
distinct `HLAreportServiceFile` values for two joined federates, each file's
initial Connect record, independent file and Service Reporting switch gating,
and a fresh report file after resign and rejoin. Focused HLA_EVOKED and
HLA_IMMEDIATE runs passed on provider build `umbra-jni-20261106` with the
adapter-supplied report directories; results are in
`out/java-tck/service-report-file-switch-lifecycle-java-port-20261113/`.
The assertions cover only the public MOM values and resulting records observed
by this fixture, not filesystem failure handling or report-interaction
delivery while file logging is disabled.
The default-run `java-tck.fom-update-rate-value-stability` scenario checks the
selected base FOM entries `TckFast=25.0` and `TckSlow=0.5`, confirms
`TckAdditionalRate` is unavailable before the extension module is joined, then
checks `TckAdditionalRate=2.0` and the unchanged base values through both the
owner and extension joiner. Supply `fom-model-tck.xml` through
`hla.rti.tck.updateRateFom`, `fom-extension-tck.xml` through
`hla.rti.tck.additionalFom`, and the designators through
`hla.rti.tck.updateRateFastDesignator`, `hla.rti.tck.updateRateDesignator`, and
`hla.rti.tck.fomAdditionalUpdateRate`. Focused default runs passed 1/1 under
each callback model against current-source JNI; results/JUnit are in
`out/java-tck/fom-update-rate-value-stability-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This is evidence for these selected fixture entries; the public API does not
enumerate arbitrary FDD update-rate tables, and independent-provider behavior
and conformance remain unverified.
The default-run `java-tck.fom-invalid-create-atomicity` scenario retries
federation creation with each of the selected malformed, namespace, and
duplicate FOM inputs, then creates the valid p0 federation under the same name.
Two named federates join, and the creator resolves the member handle. Its
direct mapping covers these selected outcomes in IEEE 1516.1-2025 §4 only.
Focused default runs passed 1/1 under each callback model against current-source
JNI; results/JUnit are in
`out/java-tck/fom-invalid-create-atomicity-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This does not establish exhaustive FOM validation, independent-provider
behavior, or conformance.
The default `java-tck.fom-noncompliant-module` scenario tests a schema-valid
zero-update-rate FOM module at both Create and additional-module Join. It checks
that Create rejects the valid-plus-noncompliant module set and permits a valid
same-name retry; Join must reject the mixed class/rate module without adding a
member or class, then allow a valid join while the rejected class remains
absent. Supply the adapter's base FOM, zero-rate module, and mixed atomicity
module via `hla.rti.tck.fom`, `hla.rti.tck.noncompliantUpdateRateFom`, and
`hla.rti.tck.noncompliantModuleAtomicityFom`. Its direct mapping covers these
zero-update-rate rejection and atomicity outcomes in IEEE 1516.1-2025
§4.1.4.1 only. Focused default runs passed in both callback models on the
current-source JNI build; results are in
`out/java-tck/fom-noncompliant-module-java-parity-20261115/`.
Independent-provider behavior and conformance remain unverified.
The opt-in `java-tck.declaration-publish-before-send` scenario subscribes a
receiver before attempting an unpublished send, checks the standard rejection
and absence of a receive callback, then publishes and checks one callback with
the original class, parameter, payload, and tag. A focused run passed 1/1 using
the current-source JNI native build, cached Umbra Java adapter JAR, and
HLAinteger64Time; independent providers and conformance remain unverified.
The default-run `java-tck.fom-invalid-composite-join-atomicity` case passes each
invalid module together with the valid extension module, verifies atomic
rejection, then confirms the valid join works after all failures. Its focused
both-model evidence and fixture limits are recorded above.
The default-run `java-tck.fom-invalid-mim-create-atomicity` case first passes an
empty FOM module array with the valid pinned standard MIM and requires
`InvalidFOM`, then checks the selected invalid MIM-path inputs. It creates the
valid p0 FOM with `HLAstandardMIM-2025.xml` under the same federation name,
joins two named federates, verifies their shared `HLAobjectRoot.TckObject`
handle, and resolves the member handle. Its direct mapping covers the
nonempty-FOM rule in IEEE 1516.1-2025 §4.5; invalid-MIM path checks are
supplemental. Focused default runs passed 1/1 under each callback model against
current-source JNI; results/JUnit are in
`out/java-tck/fom-invalid-mim-create-atomicity-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
Independent-provider behavior and conformance remain unverified.
The opt-in `java-tck.fom-invalid-composite-mim-create-atomicity` scenario
combines each adapter-supplied invalid FOM module with the valid FOM/MIM inputs,
checks standard FOM rejection, and then proves a valid MIM-backed create under
the same federation name succeeds. The default-run
`java-tck.fom-empty-mim-create-atomicity` case requires an empty FOM module array
with the valid standard MIM to fail with `InvalidFOM`, then retries with valid
p0 FOM/MIM inputs under the same federation name. Two named federates join,
share the `HLAobjectRoot.TckObject` handle, and the creator resolves the member
handle. Its direct mapping covers IEEE 1516.1-2025 §4.5. Focused default runs
passed 1/1 under each callback model against current-source JNI; results/JUnit
are in
`out/java-tck/fom-empty-mim-create-atomicity-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
The opt-in `java-tck.fom-empty-module-validation` case performs the same
name-reuse check using standard Create Federation Execution without MIM.
The opt-in `java-tck.fom-standard-invalid-input-corpus` case accepts a
platform-path-separator-delimited adapter corpus of semantic-invalid FOM files;
each rejected input is followed by valid create/join/destroy using the same
federation name. It accepts only the official Java `CouldNotOpenFOM`,
`ErrorReadingFOM`, `InvalidFOM`, or `InconsistentFOM` exception types. Seven
adapter fixtures passed focused installed-provider evoked and immediate runs.
The corpus remains opt-in because its C++ contract is experimental and no
direct normative row mapping has been verified; runtime success alone is not
a conformance claim.
The opt-in `java-tck.fom-dimension-handle-stability` case checks base and
extension dimension metadata and object/interaction-class associations across
FOM composition. It uses no DDM region services; the FOMs, dimensions, and class
names are adapter-provided.
The default-run `java-tck.fom-reference-class-resolution` case uses three
adapter-supplied FOM modules to verify a later-module reference class, reject
an unresolved reference, and retry a valid create with the same federation name.
The default-run `java-tck.fom-enumerated-representation` case uses two
adapter-supplied modules to accept an omitted representation, reject a
non-basic representation, and retry a valid create. Both passed focused
installed-provider runs under evoked and immediate callbacks; neither is an
independent-provider conformance claim.
The default-run `java-tck.fom-array-element-category` case accepts named and
omitted Element Types, rejects raw `HLAinteger32BE`, rejects the schema-valid
but HLA-reserved `HLAUserArray` name with `InconsistentFOM` or `InvalidFOM`,
and retries the valid named FOM under the same federation name. Supply the
reserved-name module through `hla.rti.tck.arrayElementInvalidNameFom` alongside
the existing named, omitted, and raw-basic array FOM properties. Its direct
mapping is IEEE 1516.2-2025 §4.14.6 for these selected cases. Focused default
runs passed both callback models on current-source Umbra JNI; results/JUnit are
in `out/java-tck/fom-array-element-category-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
Independent-provider behavior and conformance are not claimed.

The default-run `java-tck.fom-record-member-category` case accepts named,
heterogeneous, and omitted record members; rejects raw basic Field and
Alternative types, a mismatched predefined record encoding marker, and the
tested mixed-case HLA/NA reserved Record Name and Field Name forms; then retries
the valid FOM. Its direct mappings cover selected IEEE 1516.2-2025 §6.2.19 and
§4.14.7 rows plus §4 rules for these examples. Supply the ten FOM modules
through the `hla.rti.tck.recordMember*Fom` properties. Focused default runs
passed both callback models on current-source Umbra JNI; results/JUnit are in
`out/java-tck/fom-record-member-category-java-parity-20261115/{evoked,immediate}-default.{results.json,junit.xml}`.
Independent-provider behavior and conformance are not claimed.

The default-run `java-tck.fom-variant-discriminant-category` case accepts named
and omitted type categories, rejects invalid raw-basic or non-enumerated types,
and retries a valid Create. Its FOM inputs remain adapter-owned. It passed
focused installed-provider evoked and immediate runs; independent-provider
behavior remains unverified.
The default-run `java-tck.fom-variant-enumerator-syntax`,
`java-tck.fom-variant-enumerator-membership`, and
`java-tck.fom-variant-range-semantics` cases cover enumerator forms, declared
membership and endpoints, later-module resolution, compatible merged ranges,
conflicting alternatives, and valid federation-name retry through standard
Create services. Their FOM modules are adapter-supplied. All three passed
focused installed-provider evoked and immediate runs; the membership and
range Lab mappings retain their recorded broad provenance limits.
The default-run `java-tck.fom-reference-attribute-resolution`,
`java-tck.fom-data-representation-resolution`, and
`java-tck.fom-transportation-reference-resolution` cases validate later-module
references, standard joined lookups where available, invalid reference
rejection, and valid federation-name retry. They consume adapter-supplied FOM
modules and passed focused installed-provider evoked and immediate runs. The
transportation case does not claim runtime delivery or class-to-transportation
introspection, and no independent-provider conformance is claimed.
The default-run `java-tck.fom-directed-reference-resolution` case verifies
ordinary directed delivery to a universally subscribed observer after target
discovery, then rejects a missing FOM reference and retries valid Create. Its
universal subscription matches the C++ parity case; owner-targeted subscription
does not make a receiver eligible for a sender-owned target. The default-run
`java-tck.fom-dimension-reference-resolution` case checks later-module object
and interaction dimension associations and upper-bound lookup, then rejects an
unresolved dimension and retries valid Create. Both passed installed-provider
evoked and immediate runs with adapter-supplied FOMs; independent-provider
behavior remains unverified.
The default-run `java-tck.fom-dimension-duplicate-union` and
`java-tck.fom-dimension-duplicate-conflict` cases check Annex C.4's identical
duplicate/unique-dimension acceptance and changed-upper-bound rejection through
public Create and dimension lookups. The default-run
`java-tck.fom-dimension-input-type` case checks raw-basic rejection and NA or
MIM-defined table-type acceptance. All three use adapter-supplied FOMs, MIM
where needed, logical time, and callback model, and passed focused
installed-provider evoked and immediate runs. Independent-provider conformance
is not claimed.

The default-run `java-tck.fom-dimension-input-description`,
`java-tck.fom-dimension-input-na-exclusivity`, and
`java-tck.fom-dimension-name-uniqueness` cases check description requirements,
exclusive NA input, and within-module name uniqueness through standard Create,
Join, and dimension lookups. All three passed installed-provider evoked and
immediate runs with adapter-supplied FOM modules, MIM, logical time, and
callback model. Their Requirements Lab mappings retain broad clause-4
provenance; independent-provider conformance is not claimed.

The default-run `java-tck.fom-object-class-name-conventions` case rejects
object-class local names containing a period, an unknown `HLA` prefix, or the
reserved `NA` marker, then accepts a hyphen-and-underscore name through public
Create and class-handle/name lookups. It passed installed-provider evoked and
immediate runs using four adapter-supplied FOM modules and the standard MIM.
Named tables and independent-provider conformance are not claimed.

The default-run `java-tck.fom-model-modification-date` case rejects a
timezone-suffixed model date and accepts the required `YYYY-MM-DD` form
through public Create and class-handle/name lookups. It passed installed-provider
evoked and immediate runs with adapter-supplied FOM modules and standard MIM;
independent-provider conformance is not claimed.

The default-run `java-tck.fom-positive-update-rate` case rejects a zero
update rate with `InconsistentFOM` and accepts a matched positive-rate module,
checking the value through the standard `getUpdateRateValue` service. It passed
installed-provider evoked and immediate runs using adapter-supplied FOM modules
and standard MIM; independent-provider conformance is not claimed.

The default-run `java-tck.fom-dimension-default-range` case rejects a FOM
default range extending beyond its dimension's upper bound and accepts the
exact boundary through public Create and dimension lookups. It passed
installed-provider evoked and immediate runs with adapter-supplied FOM modules
and standard MIM. This is the §6.2.7 FOM subset; the C++ case's separate
§9.1.1 region-boundary assertions are not claimed here.

The default-run `java-tck.fom-synchronization-duplicate-equal` and
`java-tck.fom-synchronization-duplicate-note-conflict` cases cover Annex C.5
composition of duplicate synchronization declarations: identical declarations
are accepted, while a changed note reference is rejected with `InconsistentFOM`
and the federation name can be reused. Both passed installed-provider evoked
and immediate runs with adapter-supplied FOM modules. Independent-provider
conformance is not claimed.

The default-run `java-tck.fom-transportation-duplicate-equal` and
`java-tck.fom-transportation-duplicate-note-conflict` cases cover Annex C.6
composition of user-defined transportation declarations. Equal declarations
are accepted and exposed by standard handle/name lookup; a changed note
reference is rejected with `InconsistentFOM`, followed by a valid retry.
Both passed installed-provider evoked and immediate runs with adapter-supplied
FOM modules. Independent-provider conformance is not claimed.

The default-run `java-tck.fom-update-rate-duplicate-equal` and
`java-tck.fom-update-rate-duplicate-note-conflict` cases cover Annex C.7
composition of same-name update-rate declarations. Equal declarations compose
and expose `30.0` through `getUpdateRateValue`; the equal case checks both
modules individually before composing them and joining a member. Its direct
Java mappings are requirement candidates p112-l70-9 and p112-l74-10 to Annex
C.7. The changed-note-reference case is rejected with `InconsistentFOM` and
retries the base module. It also covers IEEE 1516.1-2025 §4.1.4.1: the
individually accepted `TckMergeRate=30.0` and `TckMergeRate=60.0` modules reject
their combined Create, then the base module succeeds again. Its direct mappings
are requirement candidates p112-l70-9 and p112-l78-11 to Annex C.7, plus
`req-fom-module-reject-update-rate-conflict` to §4.1.4.1. Current-source JNI
runs passed for both Java cases in both callback models; the paired C++ primary
and contract cases passed 8/8 focused installed-package CTests. Evidence is
limited to these adapter fixtures; independent-provider behavior remains
unverified.

The default-run `java-tck.fom-switch-duplicate-equal`,
`java-tck.fom-switch-duplicate-order`, and
`java-tck.fom-switch-duplicate-omitted` cases cover Annex C.8 support-switch
composition: equivalent declarations agree, conflicting Disabled/Enabled
values are rejected in either Create order and as either Join extension, and
an omitted `isEnabled` defaults to Disabled. The conflict scenario retries
each rejected Create or Join with a valid same-name module and checks the
public switch getter. Its direct mappings are IEEE 1516.2-2025 Annex C.8 and
IEEE 1516.1-2025 §4.1.4.1. The conflict scenario passed current-source JNI
runs under HLA_EVOKED and HLA_IMMEDIATE; the paired C++ primary and contract
cases passed 4/4 focused CTests. All three scenarios passed installed-provider
evoked and immediate runs with adapter-supplied FOM modules. These results do
not claim independent-provider conformance.

The default-run `java-tck.fom-advisory-switch-duplicate-equal`,
`java-tck.fom-advisory-switch-duplicate-order`, and
`java-tck.fom-advisory-switch-duplicate-omitted` cases cover equivalent,
conflicting and omitted-default known-class advisory settings through the
standard public getter. The conflict case rejects the Disabled/Enabled modules
in either Create order, retries each federation name with one valid module, and
checks the retained value. Its direct mappings are IEEE 1516.2-2025 Annex C.8
and IEEE 1516.1-2025 §4.1.4.1. It passed current-source JNI runs under
HLA_EVOKED and HLA_IMMEDIATE; the paired C++ primary and contract cases passed
4/4 focused CTests. All three scenarios passed installed-provider evoked and
immediate runs with adapter-supplied FOM modules. Independent-provider
conformance is not claimed.

The default-run `java-tck.fom-service-usage-merge` case checks the logical OR
of service-use declarations from two adapter-supplied FOM modules. It reads
the standard federation MOM `HLAcurrentFDD` attribute through public callbacks
and parses its XML semantically, without depending on quote style, whitespace,
namespace prefixes, or attribute order. All four module combinations passed
installed-provider evoked and immediate runs with the adapter's standard MIM;
independent-provider conformance is not claimed.

The default-run `java-tck.fom-note-collision-remap` case composes two
adapter-supplied FOMs with the same source note label in either order. It
checks the public federation MOM `HLAcurrentFDD` for distinct labels that
retain both note meanings and are referenced by the composed service row.
The adapter supplies both FOM paths and the standard MIM; no generated label
spelling or XML serialization style is required. Focused installed-provider
evoked and immediate runs passed; independent-provider conformance is not claimed.

The
directed derived-object target scenario uses adapter-selected typed model-FOM
names and verifies typed parameter delivery only at the registered target,
excluding an observer without a directed subscription. It remains unsupported
by default pending execution against a Java provider.

The opt-in `java-tck.custom-transportation-directed-interaction-delivery` case
uses adapter-selected object/interaction/parameter names and a FOM-defined
transportation type to check targeted and universal directed delivery, including
target, value, tag, producer, and transportation metadata. It remains unsupported
by default until exercised against a Java provider.

The default-run `java-tck.support-lookups-valid-inputs-after-resignation` case first
proves adapter-supplied class/member handles round-trip while joined, then checks
that the corresponding eight handle/name lookups report
`FederateNotExecutionMember` after resignation. Focused local Java runs passed
under both callback models on 2026-10-22 with adapter-selected P0 names; the
existing C++ primary peer passed 2/2 without rebuilding. This is not
independent-provider conformance evidence.

The default-run `java-tck.update-rate-lookups-valid-inputs-after-resignation` case
uses adapter-supplied rate FOM inputs to verify both named and
object-attribute update-rate lookups while joined and their membership errors
after resignation. It passed focused local Java runs under both callback
models on 2026-10-22; the existing C++ primary peer passed 2/2 without a
rebuild. The provider supplies the rate FOM, class, attribute, and designator;
these local passes are not independent-provider conformance evidence.
Its named subscription and per-attribute rate equality also cover the smaller
`cpp-tck.named-update-rate-subscription` behavior without a duplicate Java case.

The opt-in `java-tck.timestamped-attribute-update-rate-reduction` case mirrors
the C++ sequence with timestamped updates at logical times 6, 8, 10, 12, and
14. The named-rate best-effort attribute is reflected at time 6 and suppressed
at time 8 while the reliable attribute is reflected for both updates. A
best-effort-only update at time 10 is suppressed and retracted before the
receiver's grant; a second retraction after that grant must raise
`MessageCanNoLongerBeRetracted`. The case then requests a change to
`HLAreliable`, checks the single confirmation for the expected object and
attribute, and verifies both later timestamped updates carry reliable transport
metadata.

Its adapter supplies `hla.rti.tck.updateRateFom`,
`hla.rti.tck.updateRateObjectClass`,
`hla.rti.tck.updateRateReliableAttribute`,
`hla.rti.tck.updateRateBestEffortAttribute`, and
`hla.rti.tck.updateRateDesignator`, along with the provider's standard
logical-time and callback configuration. Ten focused current-source JNI runs
passed, five in each callback model. Results and the joined traceability export
are under
`out/java-tck/timestamped-attribute-update-rate-reduction-java-port-20261106/current-source/`;
independent-provider behavior remains unverified.

The opt-in `java-tck.update-rate-wall-clock-spacing` case mirrors the C++

public API check for a named maximum update rate. It verifies the resolved

subscription rate, sends one receive-order update, then immediately submits a

second. The receiver records callback-entry times with `System.nanoTime()`

and checks every observed pair stays at least `1/x` seconds apart through

the full interval. It does not require the second update to arrive after that

interval and makes no minimum-rate or general delivery-cadence claim.

Its adapter supplies `hla.rti.tck.updateRateFom`,

`hla.rti.tck.updateRateObjectClass`,

`hla.rti.tck.updateRateBestEffortAttribute`, and

`hla.rti.tck.updateRateDesignator`. The focused fixture uses

`fom-model-tck.xml`, `HLAobjectRoot.TckRateObject.BestEffortValue`,

`TckSlow=0.5`, and `HLAinteger64Time`. Ten current-source JNI runs passed,

five in each callback model. Results and traceability export are under

`out/java-tck/update-rate-wall-clock-spacing-java-port-20261106/current-source/`;

independent-provider behavior remains unverified.

The opt-in `java-tck.synchronous-update-rate-message-drop` case mirrors the
C++ HLA_EVOKED scenario. It verifies that the receiver callback-service rate
`Re` and publisher update rate `Rp` both exceed the active named subscription
rate `Rs`, then counts unique receive-order updates delivered during a bounded
wall-clock window. It checks sent-minus-delivered against `(Rp-Rs) * window`
within two boundary updates. The case makes no HLA_IMMEDIATE, timestamped, or
general rate-combination claim.

Supply the adapter rate FOM, object class, best-effort attribute, and designator
with `hla.rti.tck.updateRateFom`,
`hla.rti.tck.updateRateObjectClass`,
`hla.rti.tck.updateRateBestEffortAttribute`, and
`hla.rti.tck.updateRateDesignator`, plus `HLA_EVOKED`. Five focused
current-source JNI runs passed with `HLAobjectRoot.TckRateObject`,
`BestEffortValue`, `TckSlow=1.0`, and `HLAinteger64Time`; the adapter FOM was
copied from `fom-model-tck.xml` with only `TckSlow` changed and is at
`out/java-tck/synchronous-update-rate-message-drop-java-port-20261106/tck-rate-1.0-fom.xml`.
The tracked
`TckSlow=0.5` fixture passed four of five exploratory runs, with one run over
the two-update tolerance by one update. The paired C++ CTest and direct evoked
run passed with the `1.0` adapter FOM. Results and traceability are under
`out/java-tck/synchronous-update-rate-message-drop-java-port-20261106/current-source-rate-1.0-park/`;
independent-provider behavior remains unverified.

The opt-in `java-tck.attribute-relevance-rate-reissue` peer checks the named
rate passed with owner-directed Turn Updates On callbacks. With the attribute
relevance advisory switch enabled, the initial active `TckFast` subscriber
produces an advisory; adding a lower-rate `TckSlow` subscriber does not reissue
it; changing and restoring the fast subscriber's rate reissues the new maximum;
removing that peer reissues the surviving slow rate; and removing the final
subscriber produces Turn Updates Off. The case checks the callback object,
attribute set, and exact designator, without a minimum-rate or throughput
claim.

The adapter uses `attribute-relevance-rate-tck.xml`, which sets the standard
advisory switch on and defines `TckFast=25.0` and `TckSlow=0.5` for
`HLAobjectRoot.TckRateObject.BestEffortValue`. Five focused current-source JNI
runs passed for each callback model. Results and the traceability export are
under `out/java-tck/attribute-relevance-rate-reissue-java-port-20261106/current-source/`;
the paired current-source C++ primary and contract CTests passed 4/4.

The default `java-tck.default-rate-subscription-no-reduction` case omits the
optional update-rate designator, sends three distinct ordinary updates before
servicing an evoked callback, and verifies that every original value and tag is
reflected in send order on the receiver's locally known object. Its FOM, object
class, attribute, endpoint, callback model, and logical time are adapter-
supplied. Five focused current-source JNI runs passed per callback mode;
independent-provider conformance remains unverified. Results and the joined
traceability export are under
`out/java-tck/default-rate-subscription-no-reduction-port-20261106/current-source/`.

The opt-in `java-tck.mixed-update-rate-subscriptions` case mirrors the C++
ordinary and regional branches. With an adapter-supplied rich rate FOM, it
checks named and default per-attribute rates, tagged receive-order reflections,
and the reliable/best-effort transportation types in the ordinary passel. The
next update retains the default-rate reliable attribute while suppressing the
named-rate best-effort attribute. A two-dimensional regional branch checks the
same named-rate suppression and default-rate delivery, including the producer
federate in callback metadata. The direct Java mappings cover §5.1.7
(named/default rates), §6.1.13 (owner updates), §6.11 (tag and transportation),
and §9.8 (regional delivery); the C++-binding-only §1.5 container type
assertions are excluded. Configure `hla.rti.tck.updateRateFom`,
`hla.rti.tck.updateRateObjectClass`,
`hla.rti.tck.updateRateReliableAttribute`,
`hla.rti.tck.updateRateBestEffortAttribute`,
`hla.rti.tck.updateRateDesignator`,
`hla.rti.tck.updateRateRegionalObjectClass`,
`hla.rti.tck.updateRateRegionalAttribute`,
`hla.rti.tck.updateRateDimensionFirst`, and
`hla.rti.tck.updateRateDimensionSecond`. Ten focused current-source JNI runs
passed, five per callback model. The joined mapping export is under
`out/java-tck/mixed-update-rate-subscriptions-java-port-20261106/current-source/`;
the scenario remains opt-in because these fixture inputs are not supplied by a
default adapter, and independent-provider behavior remains unverified.

The opt-in `java-tck.standard-order-transport-lookups-after-resignation` case
checks the mandatory `Receive`/`TimeStamp` orders and `HLAreliable`/
`HLAbestEffort` transportation types, including their membership errors after
resignation.

The default-run `java-tck.support-switch-getter-lifecycle-boundaries` case checks
all thirteen standard getters after resignation and disconnect. While joined,
it also matches the C++ value checks by toggling and restoring Service
Reporting, Exception Reporting, and Send Service Reports to File, then checking
Allow Relaxed DDM and Non Regulated Grant against the enabled adapter FOM. Its
direct mappings cover only the observed switch values/effects in §§10.46,
10.48–10.50, 10.55, and 10.56; the service-report directory configures the file
switch, with no report path/content assertions. Focused runs passed both
callback models on current-source Umbra JNI; independent-provider behavior is
unverified.

The default-run `java-tck.support-switch-setter-lifecycle-boundaries` case
matches the C++ false/true/restore round trips for all eight writable support
switches and checks each corresponding getter after every set. It also verifies
all eight setters reject use after resignation and disconnect. Direct mappings
cover the getter-return, setter-value, and Boolean argument/effect requirements
in §§10.34–10.43 and §§10.46–10.51. The adapter-configured service-report
directory enables the file switch; the case does not inspect report paths or
contents. Focused runs passed both callback models on current-source Umbra JNI;
independent-provider behavior is unverified.

The opt-in `java-tck.resign-unconditional-divestiture` case verifies that
`NO_ACTION` is rejected while attributes remain owned, then checks the standard
unconditional-divestiture assumption and the surviving peer's tagged acquisition,
including the implicit `HLAprivilegeToDeleteObject` attribute.

The default `java-tck.final-federate-resignation-cleanup` case verifies that a
final member's `NO_ACTION` resignation removes its registered object and frees
the former object name for reservation and named registration after rejoining.
It uses only the standard Java API and adapter-supplied class, attribute, FOM,
logical time, provider, endpoint, and callback model. Focused installed JNI
runs passed in both callback modes; independent-provider conformance is unverified.

The default `java-tck.object-name-reservation-lifecycle` case exercises legal
and illegal single-name reservations, contention and release, mixed multiple-name
success/failure callbacks, atomic release, and name reuse after resignation.
Its FOM, logical time, provider, endpoint, and callback model come from the
adapter. Focused installed JNI runs passed in both callback modes; this is not
independent-provider conformance evidence.

The `java-tck.federation-membership` scenario also covers the unnamed
two-argument join overload: it verifies that the RTI-assigned federate name
round-trips through handle lookup and appears with the requested federate type
in the standard member-report callback. It also verifies both immediate and
evoked federation-list callback behavior across two live executions, including
that disconnect discards an unserviced evoked report. It distinguishes a
never-created federation from a destroyed federation in member-list callbacks.

The focused `java-tck.duplicate-federate-name-join-boundary` scenario checks
that a second ambassador receives the standard `FederateNameAlreadyInUse`
exception for an occupied name, can then join under a distinct name, and can
resolve both federate handles and names through the standard API. Its ID is
paired with the equivalent portable C++ scenario.

The focused `java-tck.repeated-join-by-member-boundary` scenario checks that
an ambassador already joined to an execution receives
`FederateAlreadyExecutionMember` when it tries to join that execution again,
and that the original membership remains queryable afterward.

The focused `java-tck.foreign-federate-name-lookup-boundary` scenario creates
two executions and verifies that each member name resolves only within its
own execution, with `NameNotFound` for the foreign execution's name.

The focused `java-tck.foreign-object-instance-name-lookup-boundary` scenario
reserves and registers one named object in each execution, verifies local name
round-trips, and expects `ObjectInstanceNotKnown` when the local member looks
up the foreign execution's object name.

The focused `java-tck.foreign-object-instance-handle-lookup-boundary` case
checks execution scope in the reverse direction: it selects a foreign object
handle that cannot be confused with the local handle and expects
`ObjectInstanceNotKnown` when the local member queries it.

The focused `java-tck.local-delete-object-instance-name-lookup-boundary`
case checks that a member forgets a discovered object locally and can no longer
resolve its handle, while the publishing member retains its object name until
the normal deletion service is called.

The default `java-tck.local-delete-object-instance` case checks owner-attribute
protection, recipient-local forgetting and rediscovery, and continued ordinary
reflection. It resolves each recipient's object by name, without comparing
opaque handles across federates or fabricating invalid handles. The focused
installed JNI gate passed in both callback modes; provider, FOM, endpoint,
and callback model remain adapter-supplied. Independent-provider conformance
remains unverified.

The focused `java-tck.remote-delete-known-object-class-lookup-boundary` case
checks that the deleting member loses known-class lookup immediately, the
other member receives the standard removal callback, and that member then
also receives `ObjectInstanceNotKnown` for the removed handle.

The `java-tck.explicit-mim-creation` scenario runs by default. It checks an
adapter-supplied standard MIM through `createFederationExecutionWithMIM`, then
creates a separate federation without an explicit MIM and resolves the standard
`HLAfederation` class and `HLAfederationName` attribute. It also checks
rejection of the reserved standard-MIM designator, shared MOM handles/names
across federates, and continued lookup of the caller's declarations. The
adapter supplies the MIM file (`-Dhla.rti.tck.mim=...`), caller FOM, and the
FOM's object, attribute, interaction, and parameter names. These selected
paths map to IEEE 1516.1-2025 §§4.5 and 4.6.5; arbitrary custom-MIM content and
the separate MIM-before-FOM rule are not covered. The bundled P0 FOM uses
`Payload` as the `TckInteraction` parameter name. Both callback models passed
focused JNI runs; this is not independent-provider or conformance evidence.

The `java-tck.federation-management-requires-connection` case checks that
Create, Destroy, Join, and Resign reject calls before Connect. The companion
`java-tck.connect-before-federation-disconnect-after-resign` case checks Create
both before Connect and after a successful Join, Resign, and Disconnect.
The `java-tck.standard-exception-boundaries` case checks the portable Java
subset of standard service exceptions, including disconnected federation
operations, ownership-request boundaries, delete privilege, and duplicate
object names. Both callback models passed focused JNI runs. C++ invalid-enum
and cross-implementation logical-time inputs are not representable through the
standard Java API and are not claimed as Java parity.

The default-run `java-tck.federation-mom-current-fdd` scenario consumes the
adapter-supplied FOM, standard MIM, and an additional extension FOM
(`-Dhla.rti.tck.additionalFom=...`). It checks standard federation-MOM
discovery, reliable transportation and `HLAcurrentFDD` reflection, then
checks for the selected `HLAserviceReporting`, `HLAsetSwitches`, and
`HLAreportServiceInvocation` definitions while requesting only
`HLAfederation.HLAcurrentFDD`. After an additional-module join, it verifies the
refresh and compares it with an explicit request. After the supplying federate
resigns, it checks that the extension class and declarations remain in the FDD
and that standard class/attribute lookups still resolve. The paired C++ primary
maps these bounded witnesses to IEEE 1516.1-2025 §§4.1.4.1, 4.1.4.2, and 11.1.
Current-source JNI runs passed under both callback models; independent-provider
behavior and overall conformance remain unverified.

The default-run `java-tck.federation-mom-fom-module-designator-list` scenario uses
the adapter-supplied base FOM and MIM, additional FOM modules
(`-Dhla.rti.tck.additionalFomModules=...`), and data-type-only provider FOM
(`-Dhla.rti.tck.dataTypeMergeProviderFom=...`). It creates with a filesystem
path and a distinct file URL for the same base FOM content, then checks that
only the first qualifying designator appears. It compares the designator list
as an order-independent set after an additional-module join and verifies that
a data-type-only join adds `TckMergeValue` to `HLAcurrentFDD` without changing
the designator list. Callback checks include standard transport, tag, producer,
and DDM-region metadata. Current-source JNI runs passed under both callback
models; independent-provider behavior and overall conformance remain
unverified.

The default-run `java-tck.federate-mom-fom-module-content-report` scenario requests
one joined federate's FOM module through the standard federate MOM. The adapter
supplies the base FOM, standard MIM, additional FOM module, logical-time
implementation, callback model, and federate type. The test verifies the
standard request/report handles and parameter names, reliable report delivery,
the selected module indicator, and returned FOM XML content.
It passed installed-provider HLA_EVOKED and HLA_IMMEDIATE runs with an
adapter-supplied additional FOM module; independent-provider behavior is not
yet verified.

The default-run `java-tck.federate-mom-object-instance-information` scenario
requests reports for an owned object, a discovered object, and an object
removed from local knowledge. It checks the standard report parameters,
decoded handle and attribute lists, reliable delivery, and the owned/known/NULL
states. The adapter supplies the FOM, standard MIM, object-class and attribute
names, logical time, and callback model. It passed installed-provider runs in
both callback models; independent-provider behavior is not yet verified.

The default-run `java-tck.federate-mom-updated-null-object-report` scenario
requests HLArequestObjectInstancesUpdated for a joined subject with no successful
updates. It checks exactly one reliable HLAreportObjectInstancesUpdated response,
its single HLAobjectInstanceCounts parameter and empty metadata, and decodes the
value as an empty count array. Java passed in both callback models against the
paired C++ primary and contract. Results are in
`out/java-tck/federate-mom-updated-null-object-report-java-parity-20261116/`.
This is a bounded §11.4.1(d) witness and local-provider evidence only.

The default-run `java-tck.federate-mom-reflected-null-object-report` scenario
requests HLArequestObjectInstancesReflected for a joined subject with no
Reflect Attribute Values callbacks. It checks exactly one reliable
HLAreportObjectInstancesReflected response, its single HLAobjectInstanceCounts
parameter and empty metadata, then decodes an empty count list. Java passed in
both callback models against the paired C++ primary and contract. Results are
in `out/java-tck/federate-mom-reflected-null-java-parity-20261116/`. The direct
mapping covers §§6.12, 6.13, and 11.4.1; this is local-provider evidence only.

The default-run `java-tck.federate-mom-reflected-multi-class-object-report`
scenario registers two instances in each of two object classes, sends six
receive-order updates, and checks four discoveries plus six reflections at a
separate joined target. A requester then reads the target's standard reflected
object report and the test decodes exactly two distinct instances per class.
Java and the paired C++ primary/contract passed under both callback models using
`p0-tck.xml` with `ddm-multi-attribute-tck.xml`. The direct mappings cover
§§5.8, 6.8, 6.10, 6.11, and 11.4.1; this is bounded local-provider evidence.

The default-run `java-tck.federate-mom-updated-multi-class-object-report`
scenario uses the same two-class fixture and receive-order update sequence, then
requests HLAreportObjectInstancesUpdated for the subject. It checks the
standard request/report metadata and decodes exactly two distinct successfully
updated instances in each class. Java and the paired C++ primary/contract passed
under both callback models using `p0-tck.xml` with
`ddm-multi-attribute-tck.xml`. Direct mappings cover §§5.2, 5.8, 6.8, 6.10,
6.11, 6.12, and 11.4.1; evidence is bounded to this report and local provider.

The default-run `java-tck.federate-mom-updated-object-report` scenario
publishes one object class, registers two instances, updates one instance twice
and the other once, then requests HLAreportObjectInstancesUpdated for the
subject. It checks the standard report metadata and decodes one class/count
record with a distinct-instance count of two. Java and the paired C++
primary/contract passed under both callback models. Direct mappings cover
§§6.10, 6.12, and 11.4.1; this is bounded local-provider evidence.

The default-enabled `java-tck.mom-save-restore-interactions` case uses
`p0-tck.xml`, `HLAstandardMIM-2025.xml`, and the HLAinteractionRoot.TckInteraction
application interaction. Both federates publish the three standard
save/restore MOM interactions. During the active save it sends Save Begun and
Save Complete; during the active restore it sends Restore Complete. It checks
the matching completion callbacks and that application sends fail with
SaveInProgress or RestoreInProgress during those transitions. The three
page-291 witnesses map directly to IEEE 1516.1-2025 §11.4.2. Java passed 1/1
under both callback models; the C++ primary/contract passed 4/4 focused CTests
and 4/4 direct runs. Java evidence is in
`out/java-tck/mom-save-restore-interactions-java-parity-20261116/`; C++ evidence
is in
`.build/cpp-tck-evidence-20261116/mom-save-restore-interactions-java-parity-20261116.{json,xml}`.
This is one bounded positive path and local-provider evidence only.

The default-run `java-tck.federate-mom-publication-query` scenario requests
standard federate MOM publication reports after publishing an adapter-named
object attribute and interaction. It checks decoded object and interaction
handles, the reported publication sets, an empty directed-interaction set,
and reliable callback metadata. The installed provider passed both callback
models; independent-provider behavior is not yet verified.

The default-run `java-tck.federate-mom-complete-object-class-publication-query`
scenario reports the complete publication set for three distinct
adapter-named application attributes, including the inherited standard
delete privilege. It passed installed-provider evoked and immediate runs;
the provider, FOM, MIM, attribute names, and callback model remain adapter-owned.

The default-run `java-tck.federate-mom-object-class-attribute-unpublication-query`,
`java-tck.federate-mom-object-class-full-attribute-unpublication-query`, and
`java-tck.federate-mom-object-class-whole-unpublication-query` scenarios report
the publication set after one attribute, all application attributes, or the
whole class is unpublished. They use the same adapter-owned FOM and standard
MIM configuration and passed installed-provider runs in both callback models.

The default-run `java-tck.federate-mom-interaction-class-whole-unpublication-query`,
`java-tck.federate-mom-interaction-class-partial-unpublication-query`, and
`java-tck.federate-mom-multiple-interaction-class-full-unpublication-query`
scenarios check standard MOM publication reports after one or both ordinary
interaction classes are unpublished. The adapter supplies the interaction
names and runtime configuration; installed-provider runs passed in both
callback models.

The default-run `java-tck.federation-mom-static-identity` scenario consumes the
adapter-supplied FOM, standard MIM, logical-time implementation, and callback
model. It checks standard federation-MOM handle/name round trips and object
identity, then decodes the federation name, RTI version, MIM designator, and
logical-time implementation from the ordinary identity reflection. It also
resolves all 16 Table 8 P-designated federation attributes and requests their
complete report, inventories the 25 Table 7 P-leaf interaction names through
lookups only, and verifies that an acquisition-if-available attempt does not
change HLAfederationName ownership. Its two direct candidate mappings are to
IEEE 1516.1-2025 §11.4.1; Table 7 lookups do not claim publication, and the
NoTransfer probe has no direct mapping. Focused current-source JNI runs passed
in both callback models; independent-provider behavior remains unverified.

The default-run `java-tck.federation-mom-static-switches` scenario uses the same
adapter-supplied FOM, standard MIM, logical-time implementation, and callback
model. It checks the standard federation MOM's four switch attributes and
requires each decoded `HLAinteger32BE` value to be the standard Disabled (0) or
Enabled (1) enumerator; it does not assume a provider-specific default value.
It passed installed-provider evoked and immediate runs with standard
RTI-originated producer validation; independent-provider behavior remains
unverified.

The default-run `java-tck.federation-mom-federates-in-federation` scenario uses the
adapter-supplied FOM, standard MIM, logical-time implementation, and callback
model. It decodes the standard nested `HLAfederateReferenceList` encoding with
the official encoder API and federate-handle factory, treats roster order as
unspecified, and verifies requested membership plus conditional updates after
a federate joins and resigns.
This join/resign roster witness directly maps p. 290 l. 174 to §11.4.1; it
does not claim coverage of other Federation MOM attributes.
It passed installed-provider evoked and immediate runs after standard
RTI-originated producer validation; independent-provider behavior remains
unverified.

The default-run `java-tck.federation-mom-content-reports` scenario requests the
standard FOM-module and MIM content reports from one member and observes them
from another using the adapter-supplied FOM, MIM, logical-time implementation,
and callback model. It checks report parameters, RTI-originated producer
metadata, reliable transportation, and that the FOM report contains the
adapter's `TckObject` declaration. A malformed MIM request carrying the
FOM-only module indicator must raise `InteractionParameterNotDefined` and
produce no report. Runtime MIM-data names follow the bundled 1516.2 standard
MIM; the uppercase spelling in 1516.1 remains unresolved under RL-393, so no
alias is added. The scenario has a matching C++ portable TCK ID and passed
installed-provider evoked and immediate runs. This establishes API-level
parity on one provider, not independent-provider conformance.

The default `java-tck.object-registration-discovery-lifecycle` scenario checks
typed base/derived-class lookup, inherited attribute identity, registration
before and after empty publication, ownership of the published attributes,
unnamed registration with a positive encoded-length object handle,
exact/base/late discovery, and callback-time unsubscribe. It also rejects a
derived-only value request through the known
base class and an update by a non-owner, and confirms the rejected update
produces no reflection. Exact, base, and late subscribers then receive only
their subscribed attributes. A separate `TckRateObject` path registers an
instance with only `ReliableValue` published, verifies that the instance does
not own `BestEffortValue`, then registers another instance after publishing
both attributes. Unpublishing `BestEffortValue` removes only that instance
attribute’s ownership and makes a later update fail with `AttributeNotOwned`.
After the subscribers resign, it deletes the typed object, unpublishes the
whole derived class, and confirms that another registration fails with
`ObjectClassNotPublished`.
It uses only the standard IEEE 1516.1-2025 Java API; the adapter supplies its
provider, `fom-model-tck.xml`, endpoint, logical time, callback model, typed
class/attribute names, and the reliable/best-effort rate attributes. It
compares object and class handles only within each federate. Twenty-seven direct
mapping pairs now cover bounded witnesses in §§5.1, 5.1.1–5.1.3, 5.2, 5.2.4,
5.3, 5.3.4, 5.8, 6.1.2, and 6.8. The two §5.1 child mappings cover class-specific
attribute availability for update/reflection and exact/base/late discovered-class
views; this fixture does not exercise DDM regions.
callback timing/cancellation, duplicate subscriptions, unrelated-class
filtering, producer metadata, and cleanup remain supplemental. The focused Java case
passed 1/1 under both callback models; the paired C++ primary/contract gate passed
4/4. Results/JUnit are in
`.build/java-tck-20261117/object-registration-discovery-lifecycle-focused-`
`{evoked,immediate}.{json,xml}` and
`.build/cpp-tck-java-parity-named-registration/object-registration-discovery-lifecycle.{json,xml}`.
Independent-provider conformance remains unverified.

The default `java-tck.overlapping-object-class-subscriptions-single-discovery`
scenario registers a derived-class object while one federate actively subscribes
to both the base and derived classes. It requires exactly one discovery reporting
the registered class, then verifies receiver-local name and handle lookups. The
adapter supplies the typed FOM, logical time, provider, endpoint, and callback
model. Focused installed JNI runs passed in both callback modes; this is not an
independent-provider conformance result.

The default `java-tck.inherited-object-attribute-projection` case checks that
base and derived subscribers discover a derived registration under their
respective subscribed classes, then receive only their subscribed inherited and
derived attribute values. It resolves names and handles within each federate,
uses the adapter's typed FOM and logical time, and passed focused installed JNI
runs in both callback modes. Independent-provider conformance is unverified.

The default `java-tck.attribute-value-request-service-boundaries` case covers
ordinary instance- and class-scoped value requests, owner callbacks with request
tags, and the subsequent update/reflect responses. It uses each federate's own
object and attribute handles and the adapter's object class, attribute, FOM,
logical time, provider, endpoint, and callback model. Its opt-in companion,
`java-tck.attribute-value-request-service-boundaries-invalid-handle-contexts`,
checks the C++ supplemental `ObjectClassNotDefined` and two
`AttributeNotDefined` outcomes. It uses standard API handles from a separate
FOM execution and does not represent C++ default-invalid handle values. The
companion requires the adapter's additional FOM module. Both cases passed
focused installed JNI runs in both callback modes; independent-provider
conformance is unverified.

The default-run `java-tck.object-attribute-declarations-valid-inputs-after-resignation`
scenario uses adapter-supplied object-class and attribute names and checks all six
ordinary object-attribute publication/subscription calls while joined and after
resignation. The calls succeed while joined and report `FederateNotExecutionMember`
after resignation. It passed focused local Java runs in both callback models on
2026-10-22; the existing C++ primary peer passed 2/2 without rebuilding. These
are not independent-provider conformance results.

The default-run `java-tck.attribute-value-update-request-baseline` scenario uses an
adapter-supplied multi-attribute FOM plus the `hla.rti.tck.multiAttributeObjectClass`,
`hla.rti.tck.multiAttributeFirst`, and `hla.rti.tck.multiAttributeSecond` properties
when its names differ from the TCK defaults. It checks discovery, instance-scoped
requests, the provider callback's object/attribute set/tag, and suppression when
the requester owns or no federate owns the requested instance attributes. Provider,
endpoint, callback model, and FOM remain adapter-configured. It passed the focused
JNI gate in both `HLA_EVOKED` and `HLA_IMMEDIATE`; this does not establish
independent-provider conformance.

The default-run `java-tck.object-class-attribute-value-update-request-baseline` scenario
uses the adapter's typed base/derived FOM and the
`hla.rti.tck.typedIntegerAttribute` property when the integer attribute differs from
the default. It requests inherited identity and base integer attributes at the base
class, then checks one exact callback per derived instance, inherited-attribute
filtering, request-tag propagation, and suppression for the requester's own instance.
It uses no region services and passed the focused JNI gate in both `HLA_EVOKED`
and `HLA_IMMEDIATE`. This does not establish independent-provider conformance.

The default-run `java-tck.attribute-value-update-response` scenario uses the adapter's
ordinary object-class and attribute names (`hla.rti.tck.objectClass` and
`hla.rti.tck.attribute`). Before the request, the subscribed non-owner tries to
update the owner's instance attribute; Java must throw `AttributeNotOwned` and
deliver neither a provider callback nor a reflection. The case then verifies
request-tag delivery, the owner's response, and the reflected value/tag, reliable
transportation, producer, and absence of regions. The ownership rejection maps
directly to p. 148 l. 22 -> §7.1. Provider, FOM, endpoint, logical-time
implementation, and callback model remain adapter-configured. Current-source JNI
runs passed both `HLA_EVOKED` and `HLA_IMMEDIATE`; results are in
`out/java-tck/attribute-value-update-response-java-parity-20261114/`. This does
not establish independent-provider conformance.

The default-run `java-tck.attribute-value-update-request-multi-requester` scenario
uses the adapter's ordinary object-class and attribute names (`hla.rti.tck.objectClass`
and `hla.rti.tck.attribute`). Two independent
subscribers issue instance-scoped requests with distinct tags; the provider must
receive exactly one matching callback for each, and one ordinary update must be
reflected to both requesters with the expected value and standard receive-order
metadata. It passed the focused JNI gate in both `HLA_EVOKED` and `HLA_IMMEDIATE`;
this does not establish independent-provider conformance.

The default-run `java-tck.object-registration-discovery-multi-recipient` scenario
registers two ordinary objects and verifies that two active subscribers each
receive exactly one discovery per object, with matching producer/class metadata
and stable name/handle/known-class lookups. Provider, FOM, endpoint, and callback
model remain adapter-configured. The shared helper also supports default-run
`java-tck.named-registration-multi-recipient`, which reserves both names before
registration and reserves a second pair of explicit names that differ only by
ASCII case. It verifies both case-distinct reservations succeed and preserves
their exact spelling in the name/handle round-trips, a bounded §1.5 witness.
Both passed five focused current-source JNI runs in each callback model without
assuming discovery order. Each subscriber compares publisher-returned object
handles with its local name lookups using ObjectInstanceHandle.equals and checks
that each callback handle matches the local lookup. This does not establish
independent-provider conformance.

The opt-in `java-tck.object-class-abstraction-subscription` scenario uses the
adapter-supplied `hla.rti.tck.modelFom` and configured typed base/derived class
names with the inherited Identity attribute. It publishes and registers a
derived object, subscribes at the base class, verifies the subscriber's
discovery/known-class view, and checks reflection of the inherited value.
Five focused current-source Umbra JNI runs passed under each callback model;
behavior on other providers is unverified.

The adapter-backed `java-tck.per-federate-discovered-class`,
`java-tck.registered-object-class-immutability`, and
`java-tck.discovered-class-stability-after-specific-subscription` scenarios use
the typed hierarchy FOM to check subscriber-specific discovery views, stable
registered and known classes through subscription changes, and no duplicate
discovery after a more-specific subscription is added. Each passed five focused
current-source Umbra JNI runs under both callback models; this is local-provider
evidence only.

The default-run `java-tck.named-registration` case checks empty and illegal
multiple-name reservation requests, callback separation for single and multiple
reservations, atomic multiple-name release, cross-federate reuse, reservation
ownership, named discovery, and name/handle round-trips using only the standard
API. It checks that an unreserved named registration fails before the same name
is reserved and registered, and that two unnamed registrations receive
distinct names associated with their handles. Supplemental post-consumption and
reserved-name collision checks do not assume a provider-generated prefix.
Focused installed-provider evoked and immediate runs passed;
independent-provider conformance is not claimed.

The default-run `java-tck.ordinary-multi-attribute-subscription-projection` scenario
uses an adapter-supplied multi-attribute FOM and attribute names. It verifies a
complete two-attribute update and selective projection, then sends first-only
and second-only updates back-to-back without explicitly evoking a receiver
between calls. The fully subscribed receiver must get two distinct callbacks
with separate tags and one value each; selective receivers get only matching
updates. This is the bounded §6.1.12 sequential-pair witness, not a claim about
every concurrent interleaving or independent-provider conformance. Focused JNI
runs passed under both callback models.

The default-run `java-tck.subscription-initial-reflection-projection` scenario
joins both members before registration, updates both object attributes, then
subscribes to the first attribute. It checks one matching discovery and the
current initial value, then a later two-attribute update; both reflections
must contain only the subscribed value. The adapter supplies the provider,
FOM, names, and callback model. Both callback modes passed the focused JNI
gate. This is direct §5.8 parity evidence, not an independent-provider
conformance claim.

The default-run `java-tck.active-subscription-object-discovery` scenario checks
discovery of one object registered before subscription and one registered
afterward, each exactly once. It observes no discovery before subscription and
checks each callback's producer, name, class, and publisher-returned handle, plus
the subscriber's name and known-class lookups. Both members join before the first
registration. The adapter supplies the provider, FOM, class/attribute names, and
callback model. Both callback modes passed the focused JNI gate; only the
recorded §5.8 assertions are direct standard mappings.

The opt-in `java-tck.time-constrained-enable-galt-equality` scenario compiles
against the pinned IEEE 1516.1-2025 Java API. It checks that constrained-enable
notification follows the GALT equality boundary and that a later advance is
held at the regulating peer's GALT. Its paired C++ installed-package case
passes both callback models; Java provider-runtime execution remains pending.

The opt-in `java-tck.time-constrained-disable-queued-interaction-to-ro` scenario
uses only the standard API and adapter-supplied FOM, logical-time implementation,
and callback model. It checks the untimed receive-order callback for a queued
future interaction after disabling Time Constrained, then the receive-order
metadata of a subsequent timestamped send. The paired C++ installed-package
case passes both time implementations and callback models. Java compiles against
the official API jar but remains unsupported by default until provider-runtime
verification.

The opt-in `java-tck.time-constrained-disable-queued-attribute-to-ro` scenario
uses only the official API and adapter-supplied FOM, logical-time implementation,
and callback model. It checks untimed receive-order reflection of a queued
future update after disabling Time Constrained, then the receive-order metadata
of a subsequent timestamped update. The paired C++ installed-package case
passes both time implementations and callback models. Java compiles against
the official API jar but remains unsupported by default until provider-runtime
verification.

The default `java-tck.time-constrained-disable-queued-deletion-to-ro` scenario
uses two adapter-FOM objects to check the untimed receive-order removal of a
queued future deletion after disabling Time Constrained, followed by a separate
timestamped deletion whose received order is RECEIVE. Its C++ pair passes both
time implementations and callback models. Java compiles against the official
API jar and passes focused installed-provider runs in both callback models.
Other TSO families and independent-provider conformance remain unclaimed.

The opt-in `java-tck.time-constrained-disable-queued-directed-to-ro` scenario
checks that a queued future directed interaction arrives through the untimed
receive-order callback after disabling Time Constrained, then checks a later
timestamped directed send retains its time with receivedOrder=RECEIVE. Its C++
pair passes both time implementations and callback models. Java compiles against
the official API jar. An opt-in run with the directed-capable adapter FOM
timed out waiting for the queued callback under both Java callback models;
this case remains unsupported pending a focused provider/adapter investigation.

The default `java-tck.query-lits-source-resignation` scenario checks that
Query GALT becomes undefined after the regulating source resigns while Query
LITS remains defined at a queued timestamped interaction's time. The Java
case passes focused installed-provider runs in both callback models using
adapter-supplied FOM, interaction/parameter names, endpoint, and logical time.
This is a bounded §§8.18.1/8.19.3 witness, not a conformance claim.

The default `java-tck.next-message-request` scenario checks the §8.10.2
queued-message path: a timestamped interaction arrives before the receiver's
grant at that message time, with matching callback metadata, and cannot then
be retracted. Focused installed-provider runs pass under both callback models.
The provider JAR, FOM, interaction/parameter names, endpoint, time, and
callback model remain adapter inputs; other request variants are separate.

The default `java-tck.send-message-time-state-preservation-granted` case
checks the §8.1.5 ordinary Send Message rule: after a grant, the sender's
logical time and grant history are unchanged by a receive-order interaction,
and a later advance still grants. It passes focused installed-provider runs
in both callback models using adapter-supplied FOM, names, endpoint, and time.

The default `java-tck.tar-queued-tso-before-grant` case checks queued TSO(5)
delivery before TAR(6), then a sender lookahead floor after its TAR(2) grant:
timestamp 6 is rejected with `InvalidLogicalTime`, while timestamp 7 is
accepted and delivered before TAR(7). Direct mappings cover the bounded Clause
8 delivery witness and page-199-l91-29 -> §8.11 send-floor behavior for one
regulated producer. Current-source JNI runs passed both callback models with
HLAinteger64Time; results are in
`out/java-tck/tar-queued-tso-send-floor-java-port-20261114/`. This does not
claim a general future-message suppression rule or conformance.

The default `java-tck.timestamped-interaction-tar-strict-timestamp-cutoff`
case adds that §8.14.3 cohort check: known timestamps 5 and 6 arrive before
grant 6, while timestamp 11 waits for TAR(12) and precedes grant 12. It passes
focused installed-provider runs in both callback models with adapter inputs.

The default `java-tck.timestamped-interaction-tso-designator-terminalization`
case checks which grant callback each of TAR, TARA, NMR, NMRA, and FQR produces,
and checks its reported time. It also exercises retraction terminalization as
a supplemental assertion. Explicit callback waits pass in both models with
adapter-supplied provider, FOM, names, endpoint, and logical time.

The opt-in preferred-RECEIVE timestamped interaction conversion currently
fails against the installed Java provider in both callback models: the send
returns a valid retraction designator where its promoted C++ peer expects
none. It remains outside the default run pending a provider/adapter audit.

The adapter-required `java-tck.service-report-associate-regions-for-updates`
scenario directly maps Table 5 rows r007 and r010 to IEEE 1516.1-2025
§11.5.1. It decodes `HLAsuppliedArguments` as a variable array, checks its two
ordered `AttributeRegionAssociation` entries, and validates each singleton
attribute/region pair for one successful `AssociateRegionsForUpdates` call.
Supply `hla.rti.tck.fom`,
`hla.rti.tck.modelFom`, and `hla.rti.tck.mim` as adapter FOM/MIM paths, plus
the matching typed object, identity and integer attributes, dimension, time,
and callback settings. The Java P0 adapter input is
`packages/hla-rti-java-tck/fom/p0-tck-without-switches.xml`, paired with
`packages/hla-rti-cpp-tck/fom/fom-model-tck.xml`. It preserves the P0 model
declarations and omits the root switch block because its values conflict with
the typed model FOM; this scenario does not assert switch behavior. Its C++
primary and contract pass the focused installed-package gate with
`HLAinteger64Time`. The Java scenario passed current-source Umbra JNI runs
under both callback models with the same adapter FOM/MIM and time inputs;
results are in
`out/java-tck/service-report-associate-regions-java-parity-20261114/{evoked,immediate}.{results.json,junit.xml}`.
This is local provider evidence only.

The opt-in java-tck.service-report-commit-region-modifications scenario
commits two distinct regions with valid full-range bounds in one call. It
checks one type-43 Set of region designators report containing both region
handles, the successful report fields, and the committed bounds on both
regions. Configure the adapter FOM, TckDimension, standard MIM, and
HLAinteger64Time. The Umbra JNI profile has focused passing runs under both
callback models.

The opt-in java-tck.service-report-delete-region scenario deletes one
adapter-valid region. It checks one type-42 Region designator service report,
the successful report fields, and that getDimensionHandleSet rejects the
deleted region with InvalidRegion. Configure the adapter FOM, TckDimension,
standard MIM, and HLAinteger64Time. The Umbra JNI profile has focused passing
runs under both callback models.

The opt-in java-tck.service-report-create-region scenario creates one region
over TckDimension. It checks the type-11 Set of dimension designators supplied
argument and type-42 Region designator return value in one successful service
report, then deletes the region. Configure fom-model-tck.xml, the standard MIM,
and HLAinteger64Time. The Umbra JNI profile has focused passing runs under both
callback models.

The opt-in `java-tck.service-report-dimension-handle-set` scenario checks the
type-11 dimension-set result and successful report. It also enables conveyed
region designators and, inside `receiveInteraction`, checks that the report's
single callback region has exactly the `HLAfederate` and `HLAserviceGroup`
dimensions with normalized point ranges for the reporting federate and
`SUPPORT_SERVICES`. Configure the adapter FOM, TckDimension, standard MIM, and
HLAinteger64Time. The Umbra JNI profile has focused passing runs under both
callback models.

The opt-in `java-tck.service-report-range-bounds` scenario commits one valid
region, reads back its bounds, and checks the type-42 region and type-10
dimension arguments with the type-41 JSON range result. The
`java-tck.service-report-set-range-bounds` scenario checks the matching update
report with both type-35 bound arguments and verifies the updated range.
Configure the adapter FOM, TckDimension, standard MIM, and HLAinteger64Time.
The Umbra JNI profile has focused passing runs for both scenarios under both
callback models.

The opt-in `java-tck.service-report-interaction-dimensions` scenario checks
`GetAvailableDimensionsForInteractionClass` for the adapter's typed interaction
class. It verifies the single configured dimension and the empty
`HLAinteractionRoot` result, with successful reports at serials 0 and 1. The
reports check type-27 interaction-class and type-11 dimension-set arguments.
Handle text is checked by quoted shape because provider handle strings are not
portable. Configure `HLAinteractionRoot.TckTypedInteraction`, TckDimension, the
standard MIM, and HLAinteger64Time. The Umbra JNI profile has focused passing
runs under both callback models.

The opt-in
`java-tck.service-report-interaction-dimensions-invalid-handle-contexts`
companion repeats those success and empty results, then uses the standard handle
for `HLAinteractionRoot.TckExtensionInteraction` from a separate FOM execution.
It checks `InvalidInteractionClassHandle` and the failed report at serial 2.
The foreign handle reproduces the C++ failure outcome without representing its
default-invalid handle or adding a normative mapping. Configure the additional
FOM module, for example `fom-extension-tck.xml`; focused local Umbra JNI runs
passed under both callback models.

The opt-in `java-tck.service-report-object-dimensions` scenario checks
`GetAvailableDimensionsForObjectClass` for the adapter's typed object class. It
verifies the single configured dimension, the empty `HLAobjectRoot` result,
and the type-36 object-class and type-11 dimension-set report arguments for
both calls. Handle text is checked by its quoted form; provider handle strings
are not compared as portable values. Configure
`HLAobjectRoot.TckTypedObject`, TckDimension, the standard MIM, and
HLAinteger64Time. Focused local Umbra JNI runs passed under both callback models.

The opt-in
`java-tck.service-report-object-dimensions-invalid-handle-contexts` companion
repeats the positive and empty results, then uses the standard object-class
handle for `HLAobjectRoot.TckExtensionObject` from a separate FOM execution. It
checks `InvalidObjectClassHandle` and the failed report at serial 2. This
reproduces the C++ failure outcome without claiming the default-invalid C++
handle representation or adding a normative mapping. It requires the adapter's
additional FOM module; focused local Umbra JNI runs passed under both callback
models.

The opt-in `java-tck.service-report-dimension-upper-bound` scenario checks
`GetDimensionUpperBound` for TckDimension and verifies the type-10 dimension
argument and type-35 bound result. The adapter FOM must define its upper bound
as 64. Configure the standard MIM and HLAinteger64Time. The Umbra JNI profile
has focused passing runs under both callback models.

The opt-in `java-tck.service-report-dimension-name` scenario checks
`GetDimensionName` for TckDimension and verifies the type-10 dimension argument
and type-53 quoted name result. Configure the adapter FOM, standard MIM, and
HLAinteger64Time. The Umbra JNI profile has focused passing runs under both
callback models.

The opt-in `java-tck.service-report-dimension-handle` scenario checks
`GetDimensionHandle` for TckDimension and verifies the type-53 quoted name
argument and type-10 quoted handle result. Configure the adapter FOM, standard
MIM, and HLAinteger64Time. The Umbra JNI profile has focused passing runs under
both callback models.

The opt-in `java-tck.service-report-order-transportation-lookups` scenario
checks successful `GetOrderType`, `GetOrderName`, `GetTransportationTypeHandle`,
and `GetTransportationTypeName` calls, followed by `GetTimeFactory`. It verifies
all five ordered reports, including the zero-argument type-12 time-factory
result. Configure the standard MIM and `HLAinteger64Time`; the Umbra JNI profile
has focused passing runs under both callback models.

The opt-in `java-tck.service-report-order-transportation-lookup-failures`
scenario checks the ordered failure reports for unsupported order and
transportation lookups, an invalid order value, and an invalid transportation
handle, followed by a successful `Receive` lookup. Java's closed `OrderType`
enum uses `null` for the invalid value; the Umbra JNI adapter maps it to the
invalid C++ enum so the RTI emits the same exception and report. Its
zero-valued invalid transportation handle is decoded through the public Java
handle factory. Focused Umbra JNI runs pass under both callback models; the
scenario remains disabled by default for other providers because that handle
encoding is provider-specific.

The opt-in `java-tck.service-report-interaction-parameter-lookups` scenario
checks the ordered `GetInteractionClassHandle`, `GetInteractionClassName`,
`GetParameterHandle`, and `GetParameterName` reports, including their supplied
and returned arguments, serials, status, and callback metadata. With an
adapter-supplied `serviceReportDirectory`, it also subscribes to the Federate
MOM report-file attribute and verifies that a file-backed handle lookup appends
the Table 5 type-27 result. Configure the adapter FOM, standard MIM, and
`HLAinteger64Time`; focused Umbra JNI runs with file reporting pass under both
callback models.

The opt-in `java-tck.federate-mom-report-service-file` peer reads the public
Federate MOM `HLAreportServiceFile` value and checks Table 5 p. 295 row r013's
`CallbackModel` enumerant in the initial record. Focused HLA_EVOKED and
HLA_IMMEDIATE runs passed against the current-source Umbra JNI provider with
`HLAinteger64Time`; results are in
`out/java-tck/service-report-callback-model-java-port-20261114/`. The check does
not cover file identity, other record contents, or persistence.

The opt-in
`java-tck.service-report-interaction-parameter-lookup-failures` scenario checks
the ordered reports for a missing interaction class, invalid interaction-class
handle, missing parameter, invalid parameter handle, and successful interaction
class recovery. It verifies the exception classes, Table 5 argument names and
types, serials 0–4, status, federate identity, and callback metadata. Java
invalid handles are provider-neutral opaque sentinels; the test checks their
reported values for quoted form without assuming provider-specific text. The
Umbra JNI bridge maps empty sentinel encodings to default invalid C++ handles so
the RTI emits the standard API errors. Configure the adapter FOM, standard MIM,
and `HLAinteger64Time`; focused Umbra JNI runs pass under both callback models.

The opt-in `java-tck.service-report-object-attribute-update-rate-lookups`
scenario checks seven ordered reports for object-class, object-instance,
attribute, and update-rate lookups. With an adapter-supplied
`serviceReportDirectory`, it also checks that `GetUpdateRateValue` appends a
successful, unquoted Table 5 type-35 Number equal to the Java API result. Supply
the update-rate FOM, object class, best-effort attribute, and designator with
`hla.rti.tck.updateRateFom`, `hla.rti.tck.updateRateObjectClass`,
`hla.rti.tck.updateRateBestEffortAttribute`, and
`hla.rti.tck.updateRateDesignator`; focused Umbra JNI runs with file reporting
pass under both callback models.

The opt-in
`java-tck.service-report-object-attribute-update-rate-lookup-failures` scenario
checks seven ordered failed lookups for object instances, object classes,
attributes, and update rates, followed by a successful object-class recovery.
It verifies the exception classes, Table 5 argument names and types, serials
0–7, null failure results, status, federate identity, and callback metadata.
The three unknown-object reports use an API-issued object handle from a separate
execution and match its quoted public `toString()` value within that Java run;
the C++ default-invalid handle text is not compared across providers. Synthetic
invalid object-class and attribute handle text remains a quoted-shape check.
Supply the update-rate FOM, object class, best-effort attribute, and designator with
`hla.rti.tck.updateRateFom`, `hla.rti.tck.updateRateObjectClass`,
`hla.rti.tck.updateRateBestEffortAttribute`, and
`hla.rti.tck.updateRateDesignator`; focused Umbra JNI runs pass under both
callback models.

The opt-in `java-tck.service-report-federate-object-class-lookups` scenario
checks the paired eight-report sequence: four successful federate/object-class
lookups followed by invalid object-class, unknown object-class, unknown
federate, and invalid federate failures. For the invalid object-class lookup,
it uses an API-issued `ObjectClassHandle` for `HLAobjectRoot.TckExtensionObject`
from a separate execution and matches the report argument to that Java
handle's `toString()` value. The invalid federate handle remains a synthetic
proxy checked only for quoted form. Select with `--run-unverified`; configure
`fom-model-tck.xml` for the subject and `fom-extension-tck.xml` as
`hla.rti.tck.additionalFom`, plus the standard MIM, `HLAinteger64Time`, and
`HLAobjectRoot.TckTypedObject`. Focused Umbra JNI runs pass under both callback
models.

The opt-in `java-tck.service-report-federate-object-class-lookup-failures`
scenario verifies the paired nine-report sequence: four failed lookups,
four successful federate/object-class recoveries, then a foreign-federation
`FederateHandleNotKnown` report. For the invalid object-class lookup, it uses
an API-issued `ObjectClassHandle` for `HLAobjectRoot.TckExtensionObject` from a
separate execution and matches the report argument to that Java handle's
`toString()` value. The invalid federate handle remains a synthetic proxy whose
report text is checked for quoted form only. Select it with
`--run-unverified`; configure `fom-model-tck.xml`,
`fom-extension-tck.xml` as `hla.rti.tck.additionalFom`, the standard MIM,
`HLAinteger64Time`, and `HLAobjectRoot.TckTypedObject`. Focused Umbra JNI runs
pass under both callback models; this local evidence is not cross-provider
handle-text equivalence.

The default-run `java-tck.ordinary-multi-attribute-value-update-request-response`
scenario uses the adapter-supplied multi-attribute FOM and names. It exercises
full, first-only, and second-only instance requests, checks publisher/subscriber
object identity and the provider callback attribute sets and request tags, and
verifies each ordinary response's values,
tag, producer, reliable transportation, and receive-order metadata. Both callback
modes passed the focused JNI gate with adapter-supplied FOM/names. The direct
standard claims are limited to §§6.21, 6.21.5, 6.22, and 6.10.

The `java-tck.regional-multi-attribute-update` scenario uses an
adapter-supplied multi-attribute FOM configured with
`hla.rti.tck.multiAttributeFom`, the existing multi-attribute object/attribute
name properties, and two dimensional names. It independently associates each
published attribute with a different source region, verifies projection to two
subscribers, mutates source ranges to test independent filtering, and confirms
delivery resumes after the ranges are restored. It also registers both
attributes against both disjoint source regions and verifies that each
subscriber receives each attribute once through its own region subscription.
Callback-scoped copies of the conveyed regions are queried for their dimensions
and bounds while the reflection callbacks are running.

The `java-tck.allow-relaxed-ddm` scenario composes the adapter-supplied
dimensional DDM FOM with and without its switch-declaration FOM. Across both
object and interaction paths, it verifies that regions touching at their
boundary are filtered in strict mode and admitted when the standard
`getAllowRelaxedDDMSwitch` reports enabled, including source-region callback
metadata, and confirms that strictly overlapping regions are delivered with
the switch either disabled or enabled. Configure the object, attribute,
interaction, parameter, and two dimension names through the adapter.

The `java-tck.regional-three-dimensional-overlap` scenario uses an
adapter-supplied three-dimensional FOM configured with
`hla.rti.tck.threeDimensionalFom`, the existing DDM object/attribute names, and
the first, second, and `hla.rti.tck.ddmThirdDimension` dimension names. It
verifies discovery and reflection with complete three-axis overlap, suppresses
reflection when each source axis is made disjoint in turn, then confirms
delivery resumes after restoring overlap. The source-region copies conveyed
with reflection are queried for their dimension set and exact bounds during
callback dispatch. This internal case has no direct normative subsection
mapping.

The `java-tck.regional-declaration-relevance-advisories` scenario uses
the adapter-supplied DDM FOM and the configured object class, attribute,
interaction class, and two dimensions. It verifies passive regional object and
interaction subscriptions produce no relevance advisory, active declarations
produce the corresponding start-registration and turn-interactions-on callbacks,
and unsubscription produces the matching stop/off callbacks. It also checks
region-specific interaction advisory transitions, callback data for sends
through separate regions, default-active subscription behavior, and stale-region
rejection.

The default `java-tck.ownership-transfer-regional-update` scenario uses the
adapter-supplied dimensional DDM FOM and configured object, attribute, two
object dimensions, and unrelated dimension. It rejects an unrelated-dimension
registration with `InvalidRegionContext`, transfers and reacquires the
attribute, checks default-region delivery until explicit reassociation, and
checks the new owner's source-region bounds from the callback's copied region.
With Attribute Scope Advisory enabled, it also verifies no transient scope
callbacks during Divestiture If Wanted or confirmed Negotiated Divestiture,
then checks one out-of-scope and one in-scope callback in live controls. Default runs passed 1/1 in both callback models against the current-source
JNI provider using `ddm-tck.xml`, `HLAinteger64Time`,
`HLAobjectRoot.TckObject`, `Value`, dimensions `TckDimensionX` and
`TckDimensionY`, and unrelated dimension `TckUnrelatedDimension`. The
paired installed-package C++ primary and contract runs passed 4/4 focused
CTest cases and 4/4 direct executions across both callback models. Java
evidence is under
`out/java-tck/ownership-transfer-regional-update-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/ownership-transfer-regional-update-java-parity-20261116.{json,xml}`.
This is bounded local-provider evidence, not independent-provider
behavior or an overall conformance claim.

The default `java-tck.ownership-transfer-deferred-regional-update` scenario
stages the new owner's region association before transfer. It checks the current
owner's update carries its distinct `[1,3]` source bounds while the staged
association has `[6,8]` bounds, then transfers the attribute and verifies the
first new-owner update carries the staged bounds without another association
call. The receiver queries dimensions and bounds from each copied region during
the reflection callback. The default Java case passed 1/1 under each
callback model against the current-source JNI provider using `ddm-tck.xml`,
`HLAinteger64Time`, `HLAobjectRoot.TckObject`, `Value`, `TckDimensionX`,
and `TckDimensionY`. The paired installed-package C++ primary/contract
slice passed 4/4 focused CTests and 4/4 direct executions across both
models. Java evidence is under
`out/java-tck/ownership-transfer-deferred-regional-update-java-parity-20261116/`;
C++ evidence is in
`.build/cpp-tck-evidence-20261116/ownership-transfer-deferred-regional-update-java-parity-20261116.{json,xml}`. These are bounded local-provider results, not independent-provider
behavior or an overall conformance claim.

The opt-in `java-tck.attribute-scope-advisories` scenario checks regional
scope-in/scope-out callbacks caused by source reassociation and active
subscribe/unsubscribe transitions, confirms the standard attribute-scope switch
suppresses both directions, and tests stale in-scope callback suppression when
the region becomes disjoint before EVOKED callbacks are serviced. It uses the
adapter-supplied DDM FOM and names; provider runtime verification remains
pending.

The default `java-tck.object-removal-multi-recipient-fifo` scenario deletes two
ordinary objects and verifies that each of two subscribers receives exactly one
removal per locally known object with the matching tag and producer metadata.
The historical ID preserves C++ parity; Java does not require removal order.
Provider, FOM, endpoint, callback model, and logical time remain adapter-configured.
It passed the focused JNI gate in both callback modes through the standard
1516.1-2025 API. This does not establish independent-provider conformance.

The default `java-tck.object-registration-service-boundaries` scenario checks
ordinary registration, active-subscriber discovery, name and known-class
lookups, recipient-local deletion and rediscovery, then ordinary reflection.
Reservation and local-delete negative boundaries use standard API arguments;
the case does not fabricate invalid provider handles or compare object handles
across ambassadors. It passed focused JNI execution in both callback modes
with provider, FOM, endpoint, and callback model supplied by the adapter.
Independent-provider conformance remains unverified.

The default `java-tck.object-deletion-service-boundaries` scenario checks
pre-connection and pre-membership lookup failures, object name/handle round
trips, local deletion without remote callback, ordinary removal tag/producer
metadata, post-deletion lookup and former-owner ownership-status failures, and
DeleteObjects resignation cleanup.
Its C++ base/contract pair passed two independent fresh GNU 15.2/Ninja
installed-package builds with all four evoked/immediate CTest cases passing in
each build. The Java case also passed focused installed JNI execution in both
callback modes. It uses receiver-local object handles and avoids fabricated
unknown-object handles; independent-provider conformance remains unverified.

The default `java-tck.attribute-multi-recipient-fifo` scenario sends two ordinary
updates and verifies that both subscribers receive each value and matching tag,
with their locally known object, producing federate, and transportation metadata.
The publisher receives no reflection. The historical ID preserves C++ parity;
Java imposes no sender-order assertion without a direct standard mapping.
Provider, FOM, endpoint, and callback model remain adapter-configured. Focused
installed JNI execution passed in both modes; independent-provider conformance
remains unverified.

The default `java-tck.receive-order-attribute-update` scenario checks adapter-name
handle round trips, the evoked callback-servicing boundary, and two distinct
ordinary updates with exact receiver-local object, attribute, value, tag,
producer, receive-order, and transportation metadata. It does not require
sender-order callback delivery without a direct standard mapping. The publisher
receives no reflection. It passed focused installed JNI execution in both
callback modes; independent-provider conformance remains unverified.

The default-run `java-tck.interaction-declarations-valid-inputs-after-resignation`
scenario confirms a valid adapter-supplied interaction handle/name round trip,
exercises ordinary publication and subscription while joined, then verifies the
four membership-sensitive services report `FederateNotExecutionMember` after
resignation. It is paired with the promoted C++ scenario; focused local Java
and existing C++ runs passed in both callback models on 2026-10-22 using the
adapter-selected P0 interaction class. This is not independent-provider
conformance evidence.

The default-run `java-tck.interaction-transportation-query-valid-inputs-after-resignation`
scenario queries a valid active publisher for the adapter-configured interaction
class, verifies the reported federate/class and resolvable transportation name,
then checks `FederateNotExecutionMember` for the same query after requester
resignation. It is paired with the promoted C++ scenario; focused local Java
and existing C++ runs passed in both callback models on 2026-10-22. This is not
independent-provider conformance evidence.

The default `java-tck.directed-interaction-declarations-valid-inputs-after-resignation`
scenario uses the adapter's valid object-class/directed-interaction FOM
association and checks successful set and whole-class publication/subscription
overloads while joined, then `FederateNotExecutionMember` for all six standard
overloads after resignation. It is paired with the promoted C++ scenario and
passed focused installed JNI runs in both callback models. FOM association and
joined-state setup are supplemental; independent-provider conformance remains
unverified.

The opt-in `java-tck.service-report-interaction` scenario uses the adapter's
ordinary FOM, standard MIM, logical-time implementation, and callback model.
It enables standard service reporting only for one ordinary `sendInteraction`,
then checks the received `HLAreportServiceInvocation` identity, all standard
parameters, the reporting federate, `SendInteraction` service name and type,
success indicator, empty exception, initial serial number, reliable transport,
empty tag, a null Java producer field for the RTI-originated MOM interaction,
and absence of regions. The Java TCK compiled against the pinned official API
JAR, and focused runs with a matching Umbra JNI JAR/DLL pair passed in both
callback models. The paired installed-package C++ CTest passed in both models.
These are local Umbra runtime results, without an independent-provider or
conformance claim.

The opt-in `java-tck.service-report-receive-order-file-routing` scenario pairs
that callback check with the C++ receive-order file-routing case. It reflects
the publisher and receiver `HLAreportServiceFile` values, checks their initial
configuration records and distinct file identities, verifies that the file
switch gates appends, and checks two adjacent SendInteraction records with the
type-40 parameter map. It also verifies that file reporting suppresses MOM
callbacks and routes each federate's GetParameterHandle record to its own file.
The adapter supplies a separate report directory for each callback-model run.
Focused current-source JNI runs and paired C++ direct runs passed under both
callback models. The files are checked while their federates are joined and
removed during runtime cleanup. The scenario remains unsupported by default
until other provider profiles opt in after their own runtime checks; these
local results do not establish independent-provider behavior or conformance.

The opt-in `java-tck.service-report-query-galt` scenario uses the adapter's FOM,
standard MIM, logical-time implementation, callback model, and service-report
directory. It reflects the subject's `HLAreportServiceFile`, checks the defined
GALT and its complete `HLAreportServiceInvocation`, then verifies that both
Query GALT and Query Lookahead append the standard valid return records to the
file without delivering callback reports. The shared 2025 service-report writer
now uses the execution-scoped `HLAreliable` handle, matching public handle
lookup. Strict current-source JNI runs passed under `HLA_EVOKED` and
`HLA_IMMEDIATE`; the paired current-source C++ direct scenario also passed both
models. The Java case remains unsupported by default until additional Java
provider profiles opt in after their own verification. Results are under
`out/java-tck/query-galt-java-port-20261104/fixed-java-results2/` and
`out/mgw/cpp-tck-query-galt-handle-fix-20261104/`; no independent-provider or
conformance claim is made.

The opt-in `java-tck.service-report-next-message-request-available-boundary`
scenario translates the promoted C++ boundary case. It queues a timestamped
interaction, invokes standard `nextMessageRequestAvailable`, checks the MOM
report's encoded logical-time argument, and then checks delivery/grant at the
queued message time. It uses only the standard Java API and adapter-supplied
FOM, MIM, interaction/parameter names, logical-time implementation, and
callback model. It remains unsupported by default until a compatible provider
passes runtime verification.

The opt-in `java-tck.service-report-resign-federation-execution` scenario uses
the same adapter inputs and checks that the observer receives the standard
`HLAreportServiceInvocation` after the subject successfully resigns. It
validates the reported federate, resignation service name/type, success,
exception, serial number, and ordinary interaction metadata. The C++ base and
contract cases passed independent installed-package runs in both callback
models; Java provider execution remains unsupported by default pending
verification with a Java provider.

Ownership, synchronization, DDM, other time-managed delivery (including timestamped
attribute reflection, mixed-advance attribute and interaction
delivery, interaction retraction, cross-producer timestamp ordering, no-fanout
interaction retraction, retraction fan-out, and Time Regulation/Time Constrained
re-enable), save/restore,
and MOM are separate
capability-profile lanes. Timestamped object removal checks the receive-order
asynchronous-delivery gate, checks release both by re-enabling asynchronous
delivery and by a time advance while delivery is disabled, then validates the
timestamped callback metadata and delivery-before-grant ordering. Its callback model is adapter-selectable through
`hla.rti.tck.callbackModel` (`HLA_EVOKED` or `HLA_IMMEDIATE`). A provider profile
can enable a lane only when the provider and supplied FOM support it. The
object-removal case is default-run after installed-package passes in both
callback models on 2026-10-22. It directly witnesses only §§8.1.4 and 8.15.3;
FOM, endpoint, callback model, and logical-time implementation remain adapter
inputs, and no cross-provider conformance is claimed.
The attribute-update order-classification case is also default-run after
installed-package passes under both callback models on 2026-10-22. It checks
one receive-order reflection while time constrained with asynchronous delivery
enabled and a separate timestamp-order reflection with its retraction handle;
this is only a selected §§8.1.4/8.15.3 branch, not the full order matrix.
The interaction-order classification case is likewise default-run after
HLA_EVOKED and HLA_IMMEDIATE installed-package passes. It witnesses an ordinary
RO interaction and a separate timestamped TSO interaction at one receiver
under §8.1.4, with adapter-owned FOM and logical-time inputs.
Timestamped reflection checks that the constrained receiver gets no reflection
and does not advance logical time before requesting an advance, then checks the
callback's exact logical time and order metadata, reflection-before-grant
ordering, and the post-grant logical-time query. It remains unsupported by
default until exercised against an installed 2025 provider.

The Java time-advance scenario also verifies that all three alternate advance
requests and a second TAR fail with `InTimeAdvancingState` while a TAR is
pending, without prematurely completing the original request. It takes the
callback model from the adapter configuration rather than hard-coding one.

The opt-in `java-tck.timestamped-default-region-attribute-alternate-advances`
case delivers one ordinary timestamped update to three constrained regional
subscribers, each using Flush Queue, TARA, or NMRA. It verifies empty source-
region metadata, exact value/time/producer/order/retraction/transport data,
reflection-before-grant ordering, each grant and logical-time query, and
past-time rejection for all three advance services. The adapter supplies the
dimensional FOM, names, provider, and logical-time implementation; runtime
verification remains pending.

The `java-tck.timestamped-default-region-attribute-reenable` case uses ordinary
object registration and a timestamped update with a dimensional regional
subscription. It disables and re-enables Time Constrained while delivery is
queued, then verifies the reflection's supplied-empty source-region metadata,
callback-before-grant ordering, timestamp/value/retraction data, and the
post-delivery retraction boundary. The provider, dimensional FOM, names,
logical-time implementation, and callback model are adapter supplied; runtime
verification remains pending.

The `java-tck.timestamped-default-region-attribute-regulation-reenable` case
queues an ordinary timestamped update for regional subscribers, disables and
re-enables Time Regulation with a changed lookahead, and checks that the queued
reflection remains withheld until advance. It verifies empty source-region
metadata, the changed lookahead, reflection-before-grant order, grant/query
times, and the post-delivery retraction boundary. Adapter and provider runtime
verification remain pending.

The `java-tck.timestamped-default-region-attribute-mixed-fanout` case sends
one ordinary timestamped update to immediate and time-constrained regional
subscribers. It verifies the delivered copy's value, tag, producer, time,
orders, transport, retraction handle, and empty source-region set; retraction
then reaches only the immediate recipient, while the queued copy is suppressed
before the constrained grant. Runtime verification remains pending on an
adapter-supplied provider and dimensional FOM.

The `java-tck.delay-subscription-evaluation-timestamped-regional-attribute-update`
case repeats the timestamped DSE enabled/disabled comparison with regional
source association and regional subscriptions. It unsubscribes before sending,
restores the declaration before the grant, and checks that the enabled switch
reevaluates the queued update, including its source-region metadata; a later
update is withheld after unsubscribe. DDM FOM, switch FOM, logical-time
implementation, names, provider, and callback model are adapter supplied.

The `java-tck.delay-subscription-evaluation-timestamped-regional-interaction`
case applies the same enabled/disabled grant-boundary comparison to regional
timestamped interactions. It checks parameter/tag/time/order/producer/transport
metadata and the exact source-region set when the enabled switch makes the
late declaration eligible, then confirms a later send is suppressed after
unsubscribe. DDM and switch FOMs, names, logical time, provider, and callback
model are adapter supplied; runtime verification remains pending.

The `java-tck.timestamped-attribute-order-cohort` scenario queues updates at
timestamps 5, 5, and 7 for two constrained recipients. It accepts either order
for the equal-time pair while requiring both updates before the time-5 grant,
then verifies the later update before the time-7 grant, including exact values,
tags, producer, timestamp, timestamp order, retraction handles, and resolvable
transportation. Its direct mapping is IEEE 1516.1-2025 §8.1.4 for timestamp
ordering and delivery of the complete equal-time cohort; tie order remains
unspecified. Default-enabled current-source Umbra JNI runs passed in evoked and
immediate callback modes with both HLAinteger64Time and HLAfloat64Time. Results
are in `out/java-tck/timestamped-attribute-order-cohort-review-20261104/`.
Callback mode, FOM, and logical-time implementation remain adapter supplied;
local execution does not establish independent-provider conformance.

The `java-tck.timestamped-attribute-update-queued-passel-retraction` scenario
sends two reliable attributes together at one timestamp, retracts an earlier
queued update, and requires that update to produce neither a reflection nor a
Request Retraction callback. After a later update, the receiver must get exactly
one regionless reflection containing both attribute values with TIMESTAMP
sent/received order, the second retraction handle, and `HLAreliable`
transportation. Three direct mappings cover the bounded two-attribute passel
requirements in IEEE 1516.1-2025 §6.1.12; the invalid-time check and retraction
suppression are supplemental. Default-enabled current-source Umbra JNI runs
passed in evoked and immediate callback modes with both HLAinteger64Time and
HLAfloat64Time using `ddm-multi-attribute-tck.xml`. Results are in
`out/java-tck/timestamped-attribute-queued-passel-retraction-review-20261104/`.
The FOM, callback model, logical-time implementation, and provider are adapter
supplied; local execution does not establish independent-provider conformance.

The `java-tck.order-type-controls` scenario mirrors the C++ primary case: it
changes class defaults around successive registrations, applies a per-instance
timestamp override, and transfers that attribute to a second publisher. The
new owner timestamp-sends an update and verifies its class-default RECEIVE order,
including the absent retraction handle and timestamp-bearing callback metadata.
It also checks the FDD RECEIVE interaction default, per-federate interaction-order
changes, and regular plus regional timestamp-bearing sends. Direct mappings cover
IEEE 1516.1-2025 §§8.24.4, 8.25, and 8.26.4 for the bounded behaviors stated in
the scenario catalog; the test does not claim every §8.25.3 obligation. Focused
current-source Umbra JNI runs passed both callback models with HLAinteger64Time.
Results are in `out/java-tck/order-type-controls-java-port-20261114/`. The DDM
FOM, provider, endpoint, logical-time implementation, names, dimension, and
callback model are adapter-supplied through `hla.rti.tck.ddmFom`,
`hla.rti.tck.ddmObjectClass`, `hla.rti.tck.ddmAttribute`,
`hla.rti.tck.interactionClass`, `hla.rti.tck.parameter`, and
`hla.rti.tck.dimension`. The scenario remains opt-in; these local runs do not
establish independent-provider conformance.

The opt-in `java-tck.mixed-order-attribute-passels-separate-messages` scenario
puts one receive-order and one timestamp-order attribute in a single timestamped
update. It verifies the receiver gets two separate, single-attribute reflection
callbacks with the expected values, order metadata, tag, timestamp, and grants.
This covers only the mapped §6.1.12 and §8.1.3 passel witnesses. Current-source
Umbra JNI runs passed under HLA_EVOKED and HLA_IMMEDIATE with
`ddm-multi-attribute-tck.xml` and `HLAinteger64Time`; results are in
`out/java-tck/mixed-order-attribute-passels-java-port-20261114/`. These local
runs do not establish independent-provider interoperability or conformance.

The `java-tck.receive-order-attribute-update-callback-cancellation` scenario
branches on the adapter-selected callback model. In evoked mode it withdraws
one subscriber before servicing callbacks and verifies that only the still-active
subscriber receives the update. In immediate mode it waits until both subscribers
have received the update before withdrawing one subscription. It verifies the
object, attribute, value, tag, producer, and transport callback data; it remains
unsupported by default pending Java-provider execution.

The opt-in `java-tck.receive-order-attribute-update-receiver-resignation-fence`
scenario queues an update for a receiver, resigns and rejoins that receiver
before callback delivery, and verifies the old tagged reflection does not
cross the membership boundary while one later update reaches the new
membership. It is parity-linked to the C++ public-API contract candidate and
uses only `hla.rti1516_2025` types plus adapter-supplied FOM, endpoint,
logical-time implementation, and callback model. All 388 Java source files
compile against the pinned API JAR; Java provider-runtime execution remains
unverified.

The `java-tck.interaction-subscription-lifecycle` scenario exercises passive
and active interaction subscriptions, activation without replay, delivery
while active, downgrade back to passive, and unsubscription. A passive-only
declaration does not establish delivery relevance; the test confirms it
suppresses delivery before activation and after downgrade. Callback model, FOM,
and names are adapter-supplied; it remains unsupported by default pending
Java-provider execution.

The opt-in `java-tck.interaction-service-boundaries` scenario verifies that both
active and passive subscribers receive the same ordinary interaction with the
adapter's parameter handle, value, tag, producer handle, and resolvable
transportation metadata. The case is paired with the promoted C++ scenario and
remains unsupported by default pending Java-provider execution.

The opt-in `java-tck.interaction-order-classification` scenario sends a
receive-order interaction and then a timestamp-order interaction to the same
receiver. It verifies distinct callback paths, exact payload and metadata,
timestamp/retraction/order fields, and timestamped delivery before the matching
time-advance grant. FOM and logical-time implementation remain adapter-supplied;
provider runtime verification is still required.

The opt-in `java-tck.attribute-update-order-classification` scenario registers
one instance under the FDD's RECEIVE default, then changes the class default to
TIMESTAMP and registers a second instance. It verifies ordinary receive-order
and timestamp-order reflection paths, exact payload and metadata,
timestamp/retraction/order fields, and timestamped delivery before the matching
time-advance grant. Its direct §8.1.4 mapping is limited to those selected
default-order branches; it does not cover the full order matrix. FOM and
logical-time implementation remain adapter-supplied.

The opt-in `java-tck.custom-transportation-interaction-delivery` scenario uses
the adapter-supplied rich FOM, typed interaction/parameter names, and
`hla.rti.tck.customTransportationType` to verify custom transportation handle
name round trips, ordinary interaction callback metadata, and the standard
interaction transportation query report. The case is paired with the promoted
C++ scenario and remains unsupported by default pending Java-provider execution.

The `java-tck.interaction-publication-send-fence` scenario verifies that sending
after unpublication fails with `InteractionClassNotPublished` and produces no
subscriber callback, then republishes and verifies delivery with exact parameter,
tag, producer, reliable transport, and region metadata. FOM, names, and callback
mode are adapter-supplied; the scenario remains unsupported by default pending
Java-provider execution.

The `java-tck.interaction-publication-send-transition` scenario verifies
successful delivery while published, exact `InteractionClassNotPublished`
rejection after withdrawal with no callback, and successful delivery recovery
after republication. It checks payload, tag, producer, reliable transportation,
empty region metadata, evoked callback timing, and publisher exclusion. It runs
by default after focused installed-provider passes in both callback models on
2026-10-22. For the P0 FOM, the adapter supplies
`hla.rti.tck.interactionClass=HLAinteractionRoot.TckInteraction` and
`hla.rti.tck.parameter=Payload`; another provider supplies its own FOM names.
FOM, names, endpoint, and callback model remain adapter-supplied. These local
passes are not an independent-provider conformance claim.

The default-run `java-tck.interaction-full-unpublication` case translates the
promoted C++ two-class declaration test. After baseline delivery from both
classes, it requires `InteractionClassNotPublished` for the withdrawn class
while the other still delivers, then verifies republish recovery and rejection
after withdrawing the second class. The adapter supplies both class and
parameter names. For the P0 FOM, the first pair is
`HLAinteractionRoot.TckInteraction` / `Payload`, and the second is
`HLAinteractionRoot.TckSecondaryDirectedInteraction` / `SecondaryPayload`.
The Java and existing C++ peers passed focused installed-provider runs under
both callback models on 2026-10-22; this is not independent-provider
conformance evidence.

The `java-tck.interaction-multi-recipient-fifo` scenario sends two tagged
interactions to two active subscribers and verifies each independently receives
the same FIFO sequence with exact class/parameter/value/tag/producer metadata,
resolvable transportation, no region routing, and no publisher loopback. FOM,
interaction and parameter names, and callback model are adapter-supplied; it
remains unsupported by default pending Java-provider execution.

The opt-in `java-tck.custom-transportation-attribute-delivery` scenario
registers and discovers an object from the adapter's rich FOM, then verifies an
ordinary reflected attribute update retains the FOM-declared custom
transportation type. Configure the names with
`hla.rti.tck.customTransportationObjectClass`,
`hla.rti.tck.customTransportationAttribute`, and
`hla.rti.tck.customTransportationType`; defaults match the repository's TCK
rich FOM. The scenario is paired with its promoted C++ case and remains
unsupported by default pending Java-provider execution.

The opt-in `java-tck.custom-transportation-regional-attribute-delivery` case
reuses the ordinary regional-object routing oracle with adapter-supplied custom
FOM declarations. It checks named regional registration and discovery, overlap
filtering, ordinary reflection and regional value-request response, source
region designators, reassociation, and the declared custom transportation name
on each delivery. Configure the DDM FOM, object class, attribute, two dimensions,
and transportation type through the adapter; provider runtime remains pending.

The opt-in `java-tck.custom-transportation-timestamped-attribute-delivery`
scenario uses the same adapter FOM and names across a publisher, an unconstrained
receiver, and a time-constrained receiver. It checks timestamp-order metadata
and custom transportation identity, retracts one queued update before its
constrained delivery, then verifies a later update reaches its timestamp and
cannot be retracted after delivery. Configure the object class, attribute, and
transportation type as above; configure the logical-time implementation through
the adapter. It is paired with the promoted C++ case and remains unsupported by
default pending Java-provider execution.

The opt-in
`java-tck.custom-transportation-timestamped-attribute-alternate-advances` case
uses the existing three-recipient timestamped-attribute oracle with an
adapter-declared custom FOM transportation type. It checks custom class,
attribute, and transport name round trips, then verifies all three alternate
advance callbacks preserve that transport along with the timestamp, order,
payload, tag, and retraction handle. Supply the DDM FOM, logical-time
implementation, names, provider, and callback model through the adapter; runtime
verification remains pending.

The opt-in
`java-tck.custom-transportation-timestamped-interaction-delivery` case covers
ordinary timestamped interaction delivery with the adapter-declared custom
transportation type. It checks name round trips and callback transport identity,
payload, tag, producer, logical time, order, and retraction metadata. The adapter
supplies the FOM, interaction/parameter names, logical-time implementation,
provider, and callback model; runtime verification remains pending.

The opt-in
`java-tck.custom-transportation-timestamped-interaction-alternate-advances`
case reuses the three-recipient timestamped-interaction oracle and verifies the
adapter-declared custom transportation name round-trips through publisher and
receivers and is preserved by Flush Queue, TARA, and NMRA delivery. It checks
timestamp/order, parameter payload, tag, producer, and retraction identity too.
The adapter supplies the FOM, interaction/parameter names, logical-time
implementation, provider, and callback model; runtime verification remains
pending.

The opt-in
`java-tck.custom-transportation-timestamped-directed-interaction-alternate-advances`
case extends the timestamped directed-delivery oracle across Flush Queue, TARA,
and NMRA. It verifies adapter-supplied object, attribute, interaction,
parameter, and transportation names; target name/handle round trip; and custom
transport identity on all three timestamped callbacks. The adapter supplies the
FOM, logical-time implementation, names, provider, and callback model; runtime
verification remains pending.

The opt-in `java-tck.custom-transportation-timestamped-directed-delivery` case
translates the standard single-TAR directed timestamped route with an
adapter-declared custom transport. It checks object/attribute/interaction/
parameter and transport name round trips, discovery, timestamp/order/tag/
retraction metadata, and the custom transport reported in the delivery callback.
The adapter supplies the rich FOM, logical-time implementation, provider, and
callback model; provider runtime verification remains pending.

The `java-tck.directed-interaction-target-lifecycle` scenario verifies directed
delivery to a discovered object target, cancellation/restoration across
selective subscription and publication changes, callback metadata and delivery
ordering across target deletion, the standard removal callback, and rejection
of a later send to the deleted target. An independent `HLA_IMMEDIATE` observer
checks target discovery and directed deliveries even when the adapter-selected
receiver uses evoked callbacks. The provider, typed FOM, time implementation,
endpoint, names, selected interaction transportation, and primary callback model
are adapter inputs. This default-run case imports only the IEEE 1516.1-2025
Java API and passed focused installed JNI runs in both callback models. Queued
callback cutoffs are supplemental regressions, not independent-provider
conformance claims.

The default `java-tck.directed-interaction-publication-send-fence` scenario
checks that a directed send to an already discovered object fails with
`InteractionClassNotPublished` before publication and produces no callback.
After publication, one callback preserves the target and interaction data; a
whole-class unpublication rejects a later send, and republication restores
delivery with the expected parameter, tag, producer, and adapter-selected
transportation. It passed focused installed JNI runs in both callback models.
Provider, typed FOM, endpoint, names, time implementation, transportation,
and callback model are adapter inputs; independent-provider conformance is
unverified.

The default `java-tck.directed-interaction-subscription-kind` scenario distinguishes
by-ownership from universal delivery, checks that an empty by-ownership set
does not disturb a universal declaration, and resubscribes both subscribers
to different kinds while the federation is active. It uses adapter-supplied
provider, FOM, logical-time implementation, endpoint, names, and callback
model, and receiver-local handles. Focused installed JNI runs passed in both
callback models. Ownership/universal eligibility remains supplemental under
RL-028; independent-provider conformance is unverified.

The default `java-tck.timestamped-directed-interaction-subscription-kind`
scenario sends a timestamped directed interaction to a registered target
through by-ownership and universal subscriptions. It checks owner and
universal delivery, suppression for a non-owner with only a by-ownership
subscription, callback metadata before Time Advance Grants, and distinct valid
retraction handles from regulating senders. A non-regulating sender returns no
valid handle. Direct mappings are page 101 lines 104 and 116 -> §5.12, page 126
line 179 -> §6.14, page 127 line 7 -> §6.14.3, and pages 184 line 91 and 185
line 17 -> §8.1.7. The recipient-eligibility matrix and positive-lookahead
boundary are supplemental assertions. Its Java source imports only the
official IEEE 1516.1-2025 API; provider, FOM, logical time, endpoint, names,
and callback model remain adapter-supplied. Independent-provider conformance
is unverified. The Java TCK compiled against the pinned API JAR and passed
focused current-source JNI runs with both callback models and both time
factories (4/4). Results are in
`out/java-tck/timestamped-directed-interaction-subscription-kind-review-20261104/`;
those local-provider runs do not establish independent-provider conformance.

The default `java-tck.directed-derived-object-target-eligibility` scenario runs the
same ownership/universal distinction with an adapter-selected derived object
class and inherited identity attribute. It checks target ownership and
universal delivery while excluding subscribers that are not eligible for the
target. Provider, compatible typed FOM, logical time, endpoint, names, and
callback model are adapter-supplied. It uses receiver-local object and member
handles and passed focused installed JNI runs in both callback models.
Derived-class applicability and the recipient partition are supplemental
regression checks, not independent-provider conformance claims.

The default `java-tck.directed-interaction-multi-recipient-fifo` scenario
registers one target, then sends two ordinary directed interactions to it. Both
universal subscribers independently receive exact class, receiver-local
target/parameter/value/tag/producer metadata and resolvable transportation,
while the sender receives no loopback. FIFO is an additional regression check,
not a standalone conformance claim. The adapter supplies the FOM, names,
logical-time implementation, provider, endpoint, and callback model. Focused
installed JNI runs passed in both modes; independent-provider conformance
remains unverified.

The default `java-tck.directed-interaction-mixed-subscription-fanout` scenario
contrasts a by-ownership subscriber, two universal subscribers, and an
unsubscribed member across two targets. It verifies exact target, parameter,
tag, producer, and resolvable transport metadata, producer order, and that
only the universal subscribers receive an interaction addressed to another
member's object. Provider, FOM, logical-time implementation, names, endpoint,
and callback model are adapter-supplied. It uses receiver-local object and
member handles and passed focused installed JNI runs in both callback modes.
The by-ownership/universal partition, sender exclusion, and ordering are
supplemental regression checks; independent-provider conformance remains
unverified.

The default `java-tck.directed-interaction-no-applicable-subscription` case
first discovers a target by its standard object name, then sends an untimed
directed interaction to a receiver without a matching directed-class
subscription. It checks that neither directed nor ordinary interaction
delivery occurs during bounded callback servicing. It uses receiver-local
handles and passed focused installed JNI runs in both callback models;
independent-provider conformance remains unverified.

The default `java-tck.directed-interaction-subscription-accumulation` case
uses two sequential universal subscriptions for sibling directed classes on
the same object class. Both untimed sends must arrive through the receive-order
directed callback with their own parameters and tags; a timed overload is a
failure. The adapter can override both class and parameter names and supplies
the compatible FOM. Focused installed JNI runs passed in both callback models;
the separate timestamped witness and independent-provider conformance are not
established by this case alone.

The default `java-tck.directed-known-object-class-fallback` case discovers a
derived target only as its subscribed base class, then verifies that one
untimed directed send is received as the base interaction with only the base
parameter. The inapplicable child-class subscription does not change that
projection. It then uses standard time services to queue an NMR and send a
timestamped directed interaction: the same base-class projection must arrive
at message time before the grant, with timestamp, order, tag, and retraction
metadata intact. It uses receiver-local standard handles and passed focused
installed JNI runs in both callback models. Independent-provider conformance
remains unverified.

The default `java-tck.directed-interaction-received-class-projection` case
rejects a base-class send containing a derived-only parameter, then sends a
valid derived interaction. The publisher gets no loopback; the superclass
subscriber receives only the inherited parameter, while the exact-class
subscriber receives both. Its universal subscriptions and receiver-local
handles match the C++ standard-API case. Focused installed JNI runs passed in
both callback models; independent-provider conformance remains unverified.

The `java-tck.receive-order-interaction` scenario sends two tagged interactions
in succession and verifies callback FIFO order, exact parameter values, standard
handle/name round-trips, producer and reliable-transport metadata, no region
routing, and no publisher loopback. FOM, interaction and parameter names, and
callback mode are adapter-supplied; it remains unsupported by default pending
Java-provider execution.

The `java-tck.receive-order-interaction-callback-cancellation` scenario sends
one ordinary reliable interaction to two subscribers. In evoked mode it
withdraws one subscription before callback servicing and confirms that only the
active subscriber receives the interaction; in immediate mode it waits for both
deliveries before withdrawal. It verifies the interaction and parameter handles,
payload, tag, producer, reliable transportation, and absent region routing. Its
FOM, names, and callback model are adapter-supplied; it remains unsupported by
default pending Java-provider execution.

The `java-tck.receive-order-multi-attribute-update-callback-cancellation`
scenario projects a two-attribute update onto a subscriber interested in only
the second attribute. It checks both evoked cancellation and immediate delivery,
exact callback attribute sets and metadata, and that a later first-attribute-only
update reaches only the still-subscribed receiver. The provider-compatible FOM,
class and attribute names, and callback model are adapter-supplied; it remains
unsupported by default pending Java-provider execution.

The `java-tck.modify-lookahead-contract` scenario checks Query Lookahead,
immediate increase, invalid zero rejection without mutation, deferred decrease
until a Time Advance Grant, and the evoked-mode `InTimeAdvancingState` boundary
while a TAR is pending. It uses the adapter's FOM, logical-time implementation,
and callback model, and only the official IEEE 1516.1-2025 Java API. It is
unsupported by default until executed against a compatible 2025 provider.

The `java-tck.modify-lookahead-service-report` scenario composes the adapter's
FOM and standard MIM, requests a lower lookahead while time regulating, verifies
that the change remains deferred, and checks the resulting standard
`HLAreportServiceInvocation`: service identity/type, success, argument type/name
and requested interval, null return, serial number, federate identity, and
callback metadata. Provider, FOM/MIM, logical-time implementation, and callback
model remain adapter inputs; runtime verification is pending.

The `java-tck.timestamped-interaction-mixed-advances` scenario sends the same
timestamped interaction to three constrained receivers, advanced respectively
by Flush Queue, Time Advance Request Available, and Next Message Request
Available. The publisher advances to one lookahead before the interaction
timestamp, making the message time the inclusive GALT boundary for the two
available-advance receivers. It checks callback-before-grant ordering, payload
and tag, producer, time/order/retraction metadata, grant times, and logical-time
queries. It remains unsupported by default until run through a compatible
provider adapter.

The `java-tck.time-coordinated-interaction` scenario mirrors the focused C++
Clause 8.1.3 case: a timestamped interaction remains queued when its constrained
receiver advances only to time 2, then arrives with its original time and
standard delivery metadata before the receiver's matching time-5 grant. The
provider, FOM, logical-time implementation, interaction/parameter names, and
callback model are adapter inputs; Java provider-runtime verification remains
pending.

The `java-tck.timestamped-regional-interaction-alternate-advances` scenario
additionally checks the distinct Flush Queue actual/optimistic frontier (5/7),
then sends a timestamped regional interaction at actual time plus queried
lookahead (6) without changing the receiver's logical time. The C++ and Java
scenarios retain matching parity links and adapter-supplied DDM FOM/time
configuration; Java runtime verification remains adapter-dependent.

The `java-tck.timestamped-directed-alternate-advances` scenario applies the
same Flush Queue, TARA, and NMRA comparison to directed interactions addressed
to a registered object. It verifies target discovery, no delivery before an
advance, callback-before-grant ordering for all three services, exact grant
distinctions, payload/tag/time/order/retraction metadata, and resolvable
transportation. It remains unsupported by default until run through a
compatible provider adapter. A focused installed-adapter run reached the
publisher and Flush Queue grants but not the TARA/NMRA grants or directed
deliveries. Its recorder now reads the producer handle from the official
discovery callback's fourth argument.

The default-run `java-tck.timestamped-interaction-no-fanout` scenario verifies
that a timestamped interaction retains a usable retraction designator even
without subscribers, delivers no interaction or retraction callbacks, and
rejects that designator after retraction or after advancing past its timestamp.
It passed focused installed-provider runs under HLA_EVOKED and HLA_IMMEDIATE.
The direct standard mapping covers Retract use only; no-fanout behavior and
designator terminalization are supplemental observations. Provider, FOM,
endpoint, time implementation, interaction/parameter names, and callback model
remain adapter inputs.

The default-run `java-tck.timestamped-interaction-publication-send-transition`
scenario checks that a published timestamped Send Interaction is accepted with
a retraction handle, then that sending after unpublication raises
`InteractionClassNotPublished`. It passed focused installed-provider runs under
HLA_EVOKED and HLA_IMMEDIATE. Its direct mapping is limited to the §5.1.3
published-class precondition; time-role setup and absence of a pre-advance
receiver callback are supplemental. Provider, FOM, endpoint, logical-time
implementation, interaction/parameter names, and callback model are adapter
inputs.

The `java-tck.timestamped-interaction-retraction` scenario follows the promoted
C++ sequence: an unconstrained subscriber receives a timestamped interaction,
then receives the matching Request Retraction callback; a constrained copy is
suppressed when retracted before its grant. A second interaction reaches the
constrained subscriber before its grant, after which retracting its handle must
report `MessageCanNoLongerBeRetracted`. The scenario uses the adapter's callback
model, FOM, and time implementation and remains unsupported by default pending
Java provider execution.

The `java-tck.timestamped-attribute-ownership-transfer` scenario queues a
timestamped attribute update while the original owner is regulating, then
divests and transfers that attribute before the constrained recipient advances.
It checks both ownership callbacks and tags, the unowned interval, successful
acquisition, and exact update value/tag/producer/time/order/retraction metadata
at the constrained grant. The case is default-enabled after passing current-source
Umbra JNI runs with both callback models and both HLAinteger64Time and
HLAfloat64Time. Evidence is in
`out/java-tck/timestamped-attribute-ownership-transfer-review-20261104/`.

The focused service-report scenarios decode standard
`HLAreportServiceInvocation` callbacks through the public encoding API. The
ordinary attribute-update case verifies the invoking federate, service type,
success result, empty exception, and initial report serial. The
timestamped attribute-update and object-deletion cases additionally verify
retraction metadata and delay the corresponding reflection/removal until the
constrained receiver and regulating sender complete the required time advances.
The timestamped deletion failure case verifies reports for an already removed
object and for a timestamp below current time plus lookahead, including typed
exception, supplied-argument, null-return, and serial-number fields. These are
adapter-dependent and remain unsupported by default until exercised against an
installed Java provider. The timestamped attribute-update failure case adds
unknown-object, undefined-attribute, and invalid-time reports and confirms that
none of those rejected calls reflects to a subscribed receiver. The timestamped
interaction failure case checks invalid interaction class, invalid parameter,
and invalid-time reports with no application interaction delivery.

The `java-tck.timestamped-attribute-source-resignation` scenario queues an
attribute update, then resigns its producer with unconditional divestiture.
The survivor assumes and acquires the attribute; retraction by the departed
producer must fail with `FederateNotExecutionMember`, while a separate
regulating clock permits the queued update to reach the survivor before its
grant. It checks the original producer and complete timestamped reflection
metadata, including nonempty logical-time text and reflection before the matching grant. The case is default-enabled
after current-source Umbra JNI runs passed with both callback models and both logical-time implementations. The four
result files are in `out/java-tck/timestamped-attribute-source-resignation-review-20261104/`. Direct mappings cover
resignation (§4.12), reflection metadata (§6.11), reflection before grant (§8.1.3), and queued passel delivery (§8.1.7);
the post-resignation Retract failure is a supplemental boundary check.

The `java-tck.timestamped-attribute-immediate-recipient-multi-transport-retraction`
scenario sends one timestamped attribute update from a regulating publisher to
an unconstrained subscriber. Its adapter FOM declares two attributes using
`HLAreliable` and `HLAbestEffort`; the subscriber must receive exactly one
reflection per transportation passel with the shared timestamp and retraction
handle. Retract must then produce exactly one recipient-local
`requestRetraction`, and a second Retract must fail with
`MessageCanNoLongerBeRetracted`. The adapter supplies the FOM through
`--fom-path` and may override `hla.rti.tck.timestampedAttributePasselObjectClass`,
`hla.rti.tck.timestampedAttributePasselReliableAttribute`, and
`hla.rti.tck.timestampedAttributePasselBestEffortAttribute` with
`--jvm-argument`. The case is default-enabled after current-source Umbra JNI
runs passed in evoked and immediate callback modes with both
`HLAinteger64Time` and `HLAfloat64Time`. Results are in
`out/java-tck/timestamped-attribute-immediate-recipient-multi-transport-retraction-review-20261104/`.
Ten direct mappings cover update/reflection services, timestamped reflection
metadata and passelization, and the recipient-local retraction callback. The
page-184-l91 condition is intentionally unmapped because this scenario does not
enter the Time Advancing state; exact sent/received order, callback order, and
the second-Retract failure are supplemental. Local JNI runs do not establish
independent-provider conformance.

The `java-tck.timestamped-attribute-update-alternate-advances` scenario sends
one timestamped attribute update to constrained receivers using Flush Queue,
Time Advance Request Available, Next Message Request Available, and Next Message
Request. It checks reflection-before-grant ordering and, after the early NMR
grant at time five, sends from an owned object at time seven with lookahead one
while confirming logical time remains five. It also checks payload/tag,
producer metadata, timestamp/order/retraction handles, grant/query consistency,
Flush Queue actual and optimistic bounds, and terminal retraction. It remains
unsupported by default pending Java-provider runtime verification.

The `java-tck.timestamped-object-deletion-mixed-advances` scenario queues one
timestamped object deletion for three constrained recipients using Flush Queue,
Time Advance Request Available, and Next Message Request Available. It checks
removal-before-grant ordering, each service's distinct grant and logical-time
result, exact removal metadata, Flush Queue time bounds, and the
`MessageCanNoLongerBeRetracted` terminal boundary. Current-source runs passed
under both HLA_EVOKED and HLA_IMMEDIATE with the local JNI provider and the
adapter-supplied FOM, object class, attribute, and HLAinteger64Time. This is
provider-specific evidence, not an interoperability or conformance claim.

The `java-tck.timestamped-object-deletion-no-fanout` scenario checks a
timestamped deletion exactly at the producer lookahead boundary, retraction of
a later deletion when no recipient is eligible, restoration of the object-name
mapping and attribute ownership, absence of a request-retraction callback, and
the already-retracted terminal exception. It leaves the owner's object-name
lookup between scheduling and the deletion timestamp unasserted because that
pre-timestamp state is not prescribed. Current-source runs passed under both
HLA_EVOKED and HLA_IMMEDIATE with the local JNI provider and adapter-supplied
FOM, object class, attribute, and HLAinteger64Time. This is provider-specific
evidence, not an interoperability or conformance claim.

The `java-tck.timestamped-object-deletion-tombstone` scenario reserves and
registers a named instance, deletes it at a timestamp, advances to its terminal
boundary and verifies that retraction is no longer possible. It then advances
to the effective deletion timestamp before reserving and registering the same
name again and checking the resulting handle lookup. Current-source runs passed
under both HLA_EVOKED and HLA_IMMEDIATE against the local Umbra JNI provider
with the adapter-supplied FOM, object class, attribute, and HLAinteger64Time;
the paired C++ primary and contract selectors passed both callback models. This
scenario is enabled by default. These results are provider-local evidence, not
independent-provider interoperability or conformance.

The `java-tck.timestamped-object-deletion-retraction` scenario sends two
uses an unconstrained and a constrained recipient. The first deletion is
retracted after the unconstrained recipient observes provisional removal; the
case checks the Request Retraction callback, restored object identity and
producer attribute ownership, and no constrained removal after its time
advance. The second deletion is observed by both recipients after the
constrained member requests a time advance; terminal retraction and a later
timestamped delete using the removed handle must fail with
`MessageCanNoLongerBeRetracted` and `ObjectInstanceNotKnown`, respectively.
Current-source runs passed under both HLA_EVOKED and HLA_IMMEDIATE against the
local Umbra JNI provider; the paired C++ primary and contract selectors passed
both callback models. The Java case does not claim an exact removal-to-grant
callback order or exact grant time, and it cannot represent the default-invalid
object handle used by the C++ boundary check. This scenario is enabled by
default. The evidence is provider-local, not independent-provider
interoperability or conformance.

The `java-tck.timestamped-object-deletion-retraction-joined-owners` scenario
transfers one attribute to a second owner, queues a timestamped deletion, and
then has that owner resign before the producer retracts the deletion. It checks
that joined members regain the object identity, the departed owner's attribute
remains unowned, the constrained member receives no stale deletion or
retraction callback, and both time advances complete. The adapter supplies a
multi-attribute FOM plus `hla.rti.tck.multiAttributeObjectClass`,
`hla.rti.tck.multiAttributeFirst`, and `hla.rti.tck.multiAttributeSecond`
properties when its names differ from the TCK defaults. It is enabled by
default. It checks grant completion but does not assert exact post-retraction
grant times, matching the paired C++ scenario. The evidence is provider-local,
not independent-provider interoperability or conformance. Focused local JNI
runs passed under both callback models with the adapter-supplied multi-attribute
FOM and `HLAinteger64Time`; paired C++ primary and contract runs passed both
callback models.

The `java-tck.timestamped-object-deletion-source-resignation-fanout` scenario
queues one timestamped deletion for two constrained recipients using different
advance services. It resigns the producer before advancing a regulating clock,
then checks each recipient's exact grant and callback-before-grant removal, the
clock's safe frontier, and the `FederateNotExecutionMember` retraction boundary.
Provider, FOM, logical-time implementation, endpoint, and callback model remain
adapter supplied. This scenario is enabled by default. Focused local JNI runs
passed under both callback models with the P0 TCK FOM and `HLAinteger64Time`; the
paired C++ primary and contract runs passed both callback models. The evidence is
provider-local, not independent-provider interoperability or conformance.

The `java-tck.timestamped-object-deletion-regulation-reenable` scenario queues
a timestamped removal, disables and re-enables producer regulation with a
changed lookahead, verifies Query Lookahead, and checks that removal metadata
and callback-before-grant ordering survive until the constrained receiver's
grant. It also verifies both logical-time queries and the terminal retraction
exception, then confirms the removed object is no longer known. The publisher
advances to the same three-epsilon target as the paired C++ scenario. Current
Java runs passed under both HLA_EVOKED and HLA_IMMEDIATE against the local Umbra
JNI provider; the paired C++ primary and contract selectors passed both callback
models. Query Lookahead is exercised but remains unmapped because the available
candidate is for Modify Lookahead. This scenario is enabled by default. The
evidence is provider-local, not independent-provider interoperability or
conformance.

The default-run `java-tck.timestamped-interaction-cross-producer-order` scenario sends
timestamp-five and timestamp-seven interactions from two regulating producers
to two constrained receivers. It requires increasing timestamp order and
producer/tag identity, while allowing either ordering between the two equal-time
events. It also checks federation-unique retraction handles across interaction,
attribute-update, and object-deletion sends, followed by time-8 reflection and
time-9 removal before their respective grants. It passed focused installed-
provider runs in both callback models after aligning the strict sender-frontier
sequence with its promoted C++ peer. Provider, FOM, endpoint, callback model,
logical-time implementation, and object/attribute and interaction/parameter
names remain adapter inputs.

The `java-tck.timestamped-interaction-source-resignation` scenario holds a
timestamped interaction behind a constrained receiver's time frontier, resigns
the producer with `NO_ACTION`, and verifies that the accepted interaction is
delivered exactly once before the receiver's grant. It checks parameter, tag,
producer, timestamp, order, transportation, and retraction metadata, including
that the former producer cannot retract after resignation. It remains
unsupported by default: an opt-in installed-provider run passed under
HLA_EVOKED, but under HLA_IMMEDIATE one interaction arrived after producer
resignation while the clock was still at its blocking frontier. Its recorder
uses bounded callback waits and thread-safe observations for this comparison.

The `java-tck.timestamped-interaction-retraction-fanout` scenario has both an
immediate and a constrained subscriber. It checks delivery metadata, the
delivered-copy Request Retraction callback, suppression of the still-queued
copy through a time-advance grant, then repeats delivery and retraction after
the constrained member resigns. Time Regulation disable is also checked. It
remains unsupported by default until run against a Java provider adapter.

The default-run `java-tck.timestamped-interaction-regulation-reenable` scenario sends a
timestamped interaction, disables the producer's Time Regulation while the
interaction remains queued, then re-enables with a three-epsilon lookahead.
It checks Query Lookahead, interaction delivery at the GALT boundary without a
premature receiver grant, the safe grant after a second publisher advance,
callback metadata, grant/query times, and terminal retraction after delivery.
It passes focused installed-provider runs under both callback models. The
official Java API has no Time Regulation Disabled callback; the test checks
the disable service transition and second enable callback instead. Provider,
FOM, endpoint, time implementation, interaction/parameter names, and callback
model remain adapter inputs.

The `java-tck.timestamped-interaction-reenable` scenario keeps a timestamped
interaction queued while the receiver disables and re-enables Time Constrained.
It checks suppression during both boundaries, delivery before the later grant,
the original payload/tag/producer/time/order/transport/retraction metadata, and
the terminal retraction exception. It remains unsupported by default: focused
installed-provider opt-in runs in both callback models observed the publisher
grant, but neither receiver delivery nor grant after the re-enable. Bounded
callback waits and thread-safe observations rule out a one-shot drain.

The `java-tck.timestamped-attribute-update-regulation-reenable` peer also
remains unsupported. After bounded waits, its evoked run reflected the queued
value and granted the publisher, but withheld the receiver grant at the time-5
boundary. Its immediate run reflected the value while the producer advance was
still pending. The promoted C++ peer currently expects a time-5 receiver grant
after publisher TAR(2) with three-epsilon lookahead; compare that expectation
with the adjacent interaction peer's explicit second-advance boundary before
promoting Java or attributing the difference to the provider.

The `java-tck.timestamped-directed-interaction-reenable` scenario sends a
timestamped directed interaction while its registered receiver is time
constrained. Disabling Time Constrained releases the queued interaction through
the receive-order directed callback, preserving its target, payload, tag,
producer, and transport. Re-enabling Time Constrained does not redeliver it;
after both federates request the interaction time, the receiver sees only its
grant, and retracting the already-delivered message fails. Its callback model,
FOM, logical-time implementation, and provider are adapter-supplied. Its
portable defaults target the parameter-bearing P0 declarations; adapters can
override the object class, attribute, directed interaction, and parameter with
`hla.rti.tck.timestampedDirectedObjectClass`,
`hla.rti.tck.timestampedDirectedAttribute`,
`hla.rti.tck.timestampedDirectedInteractionClass`, and
`hla.rti.tck.timestampedDirectedParameter`.

The `java-tck.timestamped-directed-interaction-regulation-reenable` scenario
queues a timestamped directed message while the receiver is constrained, then
disables and re-enables publisher Time Regulation with a three-epsilon
lookahead. It checks Query Lookahead, no early delivery, one timestamped
callback before the receiver's grant, callback metadata, both grant/query times,
and `MessageCanNoLongerBeRetracted` after delivery. The publisher requests TAR
at initial time +3ε and the receiver requests TAR at initial time +5ε. The
scenario passed both callback models with both logical-time factories against
the current-source JNI provider, using the same adapter-supplied directed FOM
names as the Time Constrained re-enable case.

The `java-tck.timestamped-directed-interaction-tar-nmr` scenario sends one
timestamped directed interaction to two constrained receivers. One requests a
TAR at the message time; the other requests NMR beyond it. It verifies that
both receive the directed payload before their grants, that TAR grants at its
requested time while NMR grants at the message time, and that target, producer,
timestamp/order, transport, and retraction metadata are preserved. It uses the
adapter-configured callback model and directed FOM names and remains unsupported
by default pending Java-provider execution.

The `java-tck.timestamped-directed-interaction-source-resignation` scenario
keeps the target object with a separate owner, resigns the time-regulated
producer while its directed message is queued, and uses an independent clock
federate to advance the frontier. It verifies that resignation does not release
the callback prematurely, that retraction through the resigned producer fails
with `FederateNotExecutionMember`, and that the receiver eventually gets the
original target, payload, tag, producer, timestamp/order, transport, and
retraction metadata before its requested-time grant. The C++ universal directed
subscription maps to the Java universal-subscription API. Direct mappings are
page 185 line 17 -> §8.1.7, pages 194 lines 21 and 69 -> §8.8, and page 210
line 21 -> §8.22.3. The case uses adapter-selected FOM names and callback
model; its P0 default object class is
`HLAobjectRoot.TckObject.TckSecondaryObject`. Default-enabled focused
current-source JNI runs passed both callback models with HLAinteger64Time and
HLAfloat64Time (4/4). Results are in
`out/java-tck/timestamped-directed-interaction-source-resignation-review-20261104/`.
Local JNI results do not establish independent-provider conformance.

The `java-tck.timestamped-directed-interaction-source-resignation-fanout`
scenario extends that lifecycle to two independently constrained recipients.
Both queue a TAR at the directed message time; the independent clock first
releases exactly one receiver, while the other remains pending until a second
clock advance. It checks per-recipient delivery-before-grant ordering, no
duplicate delivery, target and payload identity, producer, timestamp/order,
transport, tag, retraction metadata, and the post-resignation retraction
exception. Direct mappings are page 185 line 17 -> §8.1.7, pages 194 lines 21
and 69 -> §8.8, and page 210 line 21 -> §8.22.3. It uses adapter-supplied FOM
names and callback model. Default-enabled focused current-source JNI runs
passed both callback models with HLAinteger64Time and HLAfloat64Time (4/4).
Results are in
`out/java-tck/timestamped-directed-interaction-source-resignation-fanout-review-20261104/`.
Local JNI results do not establish independent-provider conformance.

The `java-tck.timestamped-directed-interaction-immediate-source-resignation`
scenario sends an accepted timestamped directed interaction to an unconstrained
receiver, then immediately resigns the regulated producer. It checks exactly
one callback with the owner-held target, payload, tag, producer, timestamp, sent
and received order, transport, and retraction metadata intact, and rejects an
unexpected retraction callback. Direct mappings are page 58 line 112 -> §4.12
and pages 194 lines 45 and 69 -> §8.8. It uses adapter-supplied FOM names,
logical-time implementation, and callback model. Default-enabled focused
current-source JNI runs passed both callback models with HLAinteger64Time and
HLAfloat64Time (4/4). Results are in
`out/java-tck/timestamped-directed-interaction-immediate-source-resignation-review-20261104/`.
Local JNI results do not establish independent-provider conformance.

The default `java-tck.timestamped-attribute-update` case ports the portable C++
ordinary timestamped-update scenario. A publisher sends one update that reaches
an unconstrained receiver immediately and is retracted before the constrained
receiver's grant, then sends a second update that the constrained receiver gets
before its grant and that can no longer be retracted. It also checks unknown
object and undefined-attribute update boundaries. Direct coverage is limited to
page 194 line 45 -> §8.8 Reflect Attribute Values callback routing after TAR;
exact grant time and callback order against Time Advance Grant are not claimed.
Java uses a deleted public object handle and a standard-MIM attribute from a
foreign class because the official Java API has no public invalid-handle
constructor. Default-enabled current-source JNI runs passed both callback
models with HLAinteger64Time and HLAfloat64Time (4/4), using the P0 FOM and
HLAstandardMIM-2025.xml. Results are in
`out/java-tck/timestamped-attribute-update-review-20261104/`. Local JNI results
do not establish independent-provider conformance.

The `java-tck.timestamped-directed-interaction-retraction-fanout` scenario
queues a timestamped directed message for two constrained receivers, retracts
it before either recipient advances, and verifies both reach that time without
delivery or retraction callbacks. It then sends a later message and checks one
delivery to each recipient before each grant, full target/payload/tag/producer/
time/order/transport/retraction metadata, and the
`MessageCanNoLongerBeRetracted` boundary for the delivered handle. It remains
unsupported by default pending Java-provider execution.

The default `java-tck.directed-retraction-mixed-fanout` scenario checks the
standard §8.23.3 mixed-recipient boundary: a nonconstrained recipient receives
one timestamped directed copy and a matching Request Retraction, while a
constrained recipient's queued copy is suppressed through its grant. It passed
focused installed JNI runs in both callback models. Provider, typed FOM,
logical-time implementation, endpoint, names, and callback model remain adapter
inputs; independent-provider conformance is unverified.

The separate default `java-tck.directed-retraction-immediate-only` case has
one nonconstrained recipient and no queued peer. After a publisher grant, it
checks a timestamped directed callback with TIMESTAMP/RECEIVE order and its
valid designator, then one matching Request Retraction. It passed focused
installed JNI runs in both callback models with adapter-supplied provider,
typed FOM, time, endpoint, and names; independent-provider conformance is
unverified.

The default `java-tck.nmr-received-tso-directed-interaction-floor` case uses
adapter-supplied typed FOM names and logical time. After NMR(10) receives a
timestamped interaction at 5 and grants at 5, it checks that a timestamped
directed send at 5 throws `InvalidLogicalTime` and a send at 6 does not throw.
The direct mapping is limited to the §8.10.2 received-TSO floor. Focused
installed JNI runs passed in both callback models; independent-provider
conformance is unverified.

The separate default `java-tck.nmr-received-tso-attribute-update-floor` case
uses the same NMR(10) / received TSO(5) grant setup, then rejects a timestamped
attribute update at 5 and accepts one at 6. It has its own direct §8.10.2
mapping and passed focused installed JNI runs in both callback models. The
provider, typed FOM names, time implementation, and callback model remain
adapter inputs; independent-provider conformance is unverified.

The default `java-tck.nmr-empty-message-set-grant-time` case checks the
separate §8.10.2 no-delivered-TSO branch: NMR(10) grants exactly once at 10,
and the queried logical time is 10. It passed focused installed JNI runs in
both callback models. Provider, FOM, endpoint, time implementation, and
callback model remain adapter inputs; independent-provider conformance is
unverified.

The separate default `java-tck.nmr-request-below-galt-immediate-grant` case
checks the strict-below-GALT branch of §8.10.2: with GALT at 5, NMR(3) grants
at 3 without a regulator advance. It passed focused installed JNI runs in
both callback models. It does not establish equality-at-GALT, concurrent
future-message arrival, or independent-provider conformance.

The default `java-tck.nmr-lits-below-galt-immediate-grant` case checks the
separate queued-message branch: a TSO interaction at 3 is held while the
regulator advances to 4 with lookahead 1; NMR(10) delivers that interaction
before granting at 3 without another regulator advance. Its interaction and
parameter names are adapter inputs. It passed focused installed JNI runs in
both callback models. This case does not assert a public Query GALT value;
the regulator's time-plus-lookahead bound of 5 is not treated as that query's
result when a message remains queued.

The default `java-tck.nmra-no-pregrant-tso-floor` case covers the separate
§8.11.3 available-request branch: NMRA(10) grants at 10 with no TSO delivered,
a second request at the current time 10 grants at 10, and a request at 9 throws
`LogicalTimeAlreadyPassed`. A timestamped interaction send at 10 throws
`InvalidLogicalTime`, while one at 11 is accepted with the queried lookahead
of 1. Its direct mappings cover the two page-199 requirements plus the
§8.11.3 current-time precondition and §8.11.5 past-time exception. Focused
installed JNI runs passed in both callback models; independent-provider
conformance and the received-TSO branch remain unverified.

The default `java-tck.nmra-no-tso-waits-for-galt` case ports the two-member
§8.11 sequence: an empty-input NMRA(5) remains pending while GALT is 1 and 4,
then receives exactly one grant at 5 after the regulator advances to 4 with
lookahead 1. The case checks both GALT boundaries and the final logical times.
Focused current-source JNI runs passed in evoked and immediate callback models;
independent-provider behavior and conformance are not claimed.

The separate default `java-tck.nmr-no-pregrant-tso-floor` case exercises the
NMR §8.10.2 no-pregrant-input branch: NMR(10) grants at 10 with no TSO
delivered, rejects a timestamped Send Interaction at 10, and accepts one at
11 with actual lookahead 1. Its page-197-l77 direct mapping and focused
installed JNI passes are independent of the NMRA case. The provider and FOM
names remain adapter inputs; no independent-provider conformance is claimed.

The default `java-tck.nmr-received-tso-floor` case covers the received-input
§8.10.2 branch: NMR(10) delivers a queued TSO interaction at 5 and grants at
5, then rejects a timestamped Send Interaction at 5 while accepting one at 6
with the queried lookahead of 1. It has separate direct page-197-l89/l98
mappings and passed focused installed JNI runs in both callback models. The
no-pregrant branch and independent-provider conformance remain separate.

The separate default `java-tck.nmra-received-tso-actual-lookahead-floor`
case checks the §8.11.3 received-input branch: NMRA(10) delivers timestamp-5
input and grants at 5; a post-grant Send Interaction at 5 is rejected and one
at 6 is accepted with queried lookahead 1. It has its own direct page-199-l73
mapping and passed focused installed JNI runs in both callback models.
Adapter-supplied provider/FOM/time/names remain inputs; the no-pregrant branch
and independent-provider conformance remain separate.

The default `java-tck.nmra-ro-earliest-tso-cohort` case is HLA_EVOKED-only.
It checks two queued receive-order interactions, a complete timestamp-5
cohort before the first NMRA grant at 5, then only the remaining timestamp-8
interaction before the second grant at 8. The adapter supplies the provider,
interaction/parameter names, FOM, endpoint, and time implementation.
HLA_IMMEDIATE is explicitly unsupported for this request-boundary assertion;
independent-provider conformance is not claimed.

The default `java-tck.nmra-future-tso-arrival-pending-request` case ports the
C++ future-arrival sequence: NMRA(10) is pending before a TSO interaction at 5
arrives, and the interaction is delivered before the grant at 5; a second
pending NMRA(10) receives a later interaction at exactly 10 before granting
at 10. Its three direct page-198/199 mappings are limited to §8.11. Focused
current-source JNI runs passed both callback models with HLAinteger64Time and
HLAfloat64Time. The provider, FOM, interaction/parameter names, and logical-
time implementation remain adapter inputs; interoperability and conformance
are not claimed.

The default `java-tck.nmr-future-tso-arrival-while-pending` case ports the
same-time TSO cohort branch: NMR(10) is pending before two timestamp-5
interactions arrive; it remains pending through bounded callback windows both
before and after the send, then delivers both interactions before exactly one
grant at 5 after the publisher advances. It checks payloads, tags, producer,
timestamps, sent and received
orders, retraction handles, callback order, and final requester time. Its four
direct page-197 mappings are limited to §8.10.2. Focused current-source JNI
runs passed HLA_EVOKED with HLAinteger64Time and HLAfloat64Time. This Java case
uses two ambassadors in one JVM and does not reproduce the C++ process-topology
check; HLA_IMMEDIATE, interoperability, and provider conformance are not
claimed.

The default `java-tck.timestamped-attribute-update-future-nmr-arrival` case
checks one pending NMR path for timestamped Update Attribute Values: the
publisher advances to 2 with lookahead 5, NMR(10) remains pending, and an
update at 7 is reflected before the grant at 7. It checks object and attribute
identity, payload, tag, producer, timestamp, order, retraction handle,
transportation, pre-request GALT, and queried grant time. Its direct
page-197-l20 mapping is limited to §8.10.2. The Java port passed both callback
models with HLAinteger64Time and HLAfloat64Time on the current-source JNI
provider. The Java callback API has no separate time-text argument, so the
C++ time-text assertion is not represented. Provider, FOM, class, attribute,
and logical-time inputs come from the adapter; independent-provider behavior
and conformance are not claimed.

The default `java-tck.timestamped-attribute-update-future-tara-arrival` case
requests TARA(6) while GALT is 3, then sends a timestamped attribute update at
5 and advances the regulator to 6. It verifies one reflection at 5 before one
grant at 6, including object and attribute identity, payload, tag, producer,
sent and received order, retraction handle, transport, callback order, and
queried time. Its direct mappings cover §§8.12.3 and 8.14.4. Focused
current-source JNI runs passed both callback models with HLAinteger64Time and
HLAfloat64Time; installed-package C++ primary and contract runs passed the same
callback-model and time-factory matrix. The Java callback API has no separate
time-text argument, so that C++ assertion is not represented. Adapter supplies
provider, FOM, class, attribute, and time; independent-provider behavior and
conformance are not claimed.

The default `java-tck.timestamped-object-deletion-future-nmr-arrival` case
checks that NMR(10) remains pending until a timestamped Delete Object Instance
at 7 arrives, then delivers one Remove Object Instance callback before one
grant at 7. It validates object, producer, tag, timestamp, sent and received
order, retraction handle, and queried receiver time. Its direct mappings cover
§8.1.3 for the timestamped deletion sending and receiving relationship, §8.10.2,
and the exercised deletion, removal, and retraction clauses in §§6.16, 6.16.4,
and 6.17.1. Focused current-source JNI runs passed both callback
models with HLAinteger64Time and HLAfloat64Time. The Java callback API has no
separate time-text argument, so the corresponding C++-specific assertion is
not represented. Adapter supplies provider, FOM, class, attribute, and time;
independent-provider behavior and conformance are not claimed.

The default `java-tck.timestamped-object-deletion-future-nmra-arrival` case
checks that NMRA(10) remains pending until a timestamped Delete Object Instance
at 7 arrives, then delivers one Remove Object Instance callback before one
grant at 7. It validates object, producer, tag, timestamp, sent and received
order, retraction handle, callback order, and queried receiver time. Its direct
mappings cover §8.1.3 for the timestamped deletion sending and receiving relationship, §8.11,
and the exercised deletion, removal, and retraction clauses in §§6.16, 6.16.4, and 6.17.1. Focused current-source JNI runs passed
both callback models with HLAinteger64Time and HLAfloat64Time; installed-package
C++ primary and contract runs passed the same callback-model and time-factory
matrix. The Java callback API has no separate time-text argument, so the
corresponding C++ assertion is not represented. Adapter supplies provider,
FOM, class, attribute, and time; independent-provider behavior and conformance
are not claimed.

The default `java-tck.timestamped-object-deletion-future-tara-arrival` case
requests TARA(6) while GALT is 3, then sends a timestamped deletion at 5 and
advances the regulator to 6. It verifies one Remove Object Instance callback
at 5 before one grant at 6, including object, producer, tag, sent and received
order, retraction handle, callback order, pre-request GALT, and queried time.
Its direct mappings cover §§8.1.3 and 8.14.4, the Lab's chapter-level §8
mapping, and the exercised deletion, removal, and retraction clauses in
§§6.16, 6.16.4, and 6.17.1. Focused current-source JNI runs passed both
callback models with HLAinteger64Time and HLAfloat64Time. The Java callback API
has no separate time-text argument, so the corresponding C++ assertion is not
represented. Adapter supplies provider, FOM, class, attribute, and time;
independent-provider behavior and conformance are not claimed.

The default `java-tck.timestamped-interaction-tara-equal-boundary` case checks
the equal-time TARA boundary sequence. The receiver queries GALT=5 and is
granted TARA(5) while the publisher remains at 4 and before a timestamp-5
interaction exists. After that grant, the publisher advances and sends at 5;
the receiver gets the interaction after its first grant and before a later
TARA(6) grant. Direct mappings cover chapter §8, §8.1.7, and §8.9, including
the stated equal-time message and GALT boundary rules. Java current-source JNI
runs passed both callback models with HLAinteger64Time and HLAfloat64Time (4/4);
installed-package C++ primary and contract runs passed both callback models
with both time factories (8/8). The mapping covers this controlled arrival
sequence, not an interaction already queued before the first TARA request or
other advance services. Adapter supplies provider, FOM, interaction class,
parameter, callback model, and time; independent-provider behavior and
conformance are not claimed.

The default `java-tck.timestamped-interaction-tara-queued-equal-boundary` case
sends a timestamped interaction at 2 before the receiver requests TARA(2). It
checks that the interaction was not delivered before the request, then verifies
exactly one interaction callback before exactly one grant, with the expected
publisher and receiver grant times, handles, payload, tag, sent and received
orders, and retraction handle. Its direct §8.9 mappings use the complete
reconstructed clause text because RL-316 records that the pinned page-195
candidate omits the enumerated delivery set. Java current-source JNI runs
passed both callback models with HLAinteger64Time and HLAfloat64Time (4/4);
installed-package C++ primary and contract runs passed both callback models
with both time factories (8/8). The focused CTest query had no registrations;
the direct TCK runner supplied the C++ evidence. This covers an equal-time TSO
already queued before TARA, not a message arriving after the request or other
advance services. Adapter supplies provider, FOM, interaction class, parameter,
callback model, and time; independent-provider behavior and conformance are
not claimed.

The default `java-tck.timestamped-interaction-tara-strict-timestamp-cutoff`
case sends timestamped interactions at 5 and 6. With publisher TAR(5) and
receiver TARA(6), it verifies that timestamp 5 arrives before the first grant
while timestamp 6 remains queued. After the receiver requests TARA(7) and the
publisher requests TAR(6), it verifies timestamp 6 before the later grant.
Its direct mapping is §8.14.4(e). Java current-source JNI runs passed both
callback models with HLAinteger64Time and HLAfloat64Time (4/4); installed-
package C++ primary runs passed the same matrix (4/4). The focused CTest query
had no registrations. The Java test follows the C++ assertion surface and
does not require the receiver grant time to equal its requested cutoff; the
current-source JNI provider returned 5 for TARA(6). Adapter supplies provider,
FOM, interaction class, parameter, callback model, and time; independent-
provider behavior and conformance are not claimed.

The `java-tck.timestamped-interaction-future-tara-arrival` case starts receiver
TARA(6), confirms it remains pending, then sends a timestamp-5 interaction.
After the publisher advances, it checks the interaction payload, tag, producer,
timestamp order, transportation and retraction metadata, delivery-before-grant
callback order, grant time, and queried time. Direct mappings are
page-201-l52-16 -> §8.12.3 and page-204-l86-25 -> §8.14.4(e); this covers only
the bounded TARA future-interaction path. Java current-source JNI runs passed
HLA_EVOKED and HLA_IMMEDIATE with HLAinteger64Time (2/2). The installed-package
C++ consumer passed focused CTest and direct runs in both callback models (2/2
each). Results are in
`out/java-tck/timestamped-interaction-future-tara-arrival-java-port-20261113/`
and `.build/cpp-tck-timestamped-interaction-future-tara-arrival-java-port-20261113/`.
Adapter supplies provider, FOM, interaction class, parameter, callback model,
and time; independent-provider behavior and conformance are not claimed.

The `java-tck.tar-multi-regulator-acknowledged-tso-progress` case uses two
regulating senders and one constrained receiver. The senders queue timestamped
interactions at 3 and 5, then advance to 7 and 8. The receiver's TAR(4) delivers
only the first interaction before grant 4; after that callback returns, TAR(6)
delivers the second once before grant 6. It checks each producer, payload, tag,
timestamp order, valid send retraction, and grant time. Direct mappings are
page-182-l127-40 -> §8.1.6 and page-194-l21-3 -> §8.8. Java current-source JNI
runs passed both callback models with HLAinteger64Time (2/2). The installed-
package C++ consumer passed focused CTest and direct runs in both models (2/2
each). Results are in
`out/java-tck/tar-multi-regulator-acknowledged-tso-progress-java-port-20261113/`
and `.build/cpp-tck-tar-multi-regulator-acknowledged-tso-progress-java-port-20261113/`.
This checks public delivery and grant outcomes without prescribing a GALT
formula. Adapter supplies provider, FOM, interaction class, parameter, callback
model, and time; independent-provider behavior and conformance are not claimed.

The `java-tck.timestamped-interaction-tar-equal-boundary` case retains the
TAR(5) equal-time boundary sequence and also runs a TAR(6) case with queued
timestamp-5 and timestamp-6 interactions. It checks both exact callbacks arrive
in timestamp order before TAG(6), including payload, tag, producer, order,
retraction handles, and the granted logical time. The added direct clause 8
mappings cover the before-target and equal-target deliveries. Current-source
JNI runs passed both callback models with HLAinteger64Time; results are in
`out/java-tck/timestamped-interaction-tar-inclusive-target-java-port-20261114/`.
This does not claim beyond-target suppression, a general GALT formula, or
independent-provider conformance.

The default `java-tck.timestamped-interaction-zero-lookahead-send-at-grant`
case enables regulation with zero lookahead, grants publisher TARA(5), then
sends a timestamped interaction at 5. It verifies there is no early callback,
then checks the interaction, timestamp order, tag, retraction handle, and
callback order before the receiver's TAR(5) grant. The direct mapping is to
§8.9 through the internal timestamped-interaction delivery requirement. RL-417
tracks the missing Requirements Lab permission extraction; the nearby
page-195-l119-34 candidate is not used as send-permission evidence. Java
current-source JNI runs passed both callback models with both time factories
(4/4); installed-package C++ primary and contract runs passed the same matrix
(8/8). The focused CTest query had no registrations. Adapter supplies provider,
FOM, interaction class, parameter, callback model, and time; this evidence does
not establish independent-provider behavior or conformance.

The default `java-tck.timestamped-interaction-zero-lookahead-tar-send-boundary`
case starts a zero-lookahead publisher's TAR(5). While its grant callback is
pending, sending at 5 raises `InvalidLogicalTime`, while sending at 6 is
accepted. It verifies the TAR(5) grant and that the receiver has not received
the later interaction before advancing. Direct mappings are §6.12.3 and
§8.1.2 Table 1; the Java case does not assert delivery at 6 or the adjacent
post-grant rule. RL-312 tracks the pinned Lab extraction artifact. Focused Java
current-source JNI runs passed HLA_EVOKED with both time factories (2/2), and
installed-package C++ primary and contract runs passed the same time matrix
(4/4). The focused Catch2 CTest selector had no registration. The adapter
supplies provider, FOM, interaction class, parameter, time, and HLA_EVOKED;
this does not establish independent-provider behavior or conformance.

The default `java-tck.timestamped-directed-zero-lookahead-send-at-grant`
case enables a constrained receiver and a zero-lookahead regulated sender,
then verifies that the sender can issue a directed timestamped interaction at
the time it just received from NMRA. A second timestamped send receives a
distinct retraction designator and is retracted. The receiver then advances to
the admitted send time; the test checks the directed callback's target,
parameter, tag, producer, timestamp, TSO orders, and retraction handle before
the matching grant. It also checks the sender's zero lookahead after its grant
and verifies that the sender receives no copy of the directed interaction.
Direct mappings are §8.11 for the post-NMRA send permission, §6.14 for distinct
retraction designators, and §6.15.1 for the timestamped directed callback
properties. The separate receive-order directed-callback rules and region
behavior are outside this case. Java current-source JNI runs passed both
callback models with HLAinteger64Time and HLAfloat64Time (4/4); installed-
package C++ runs passed the same matrix (4/4). The focused CTest selector had
no registrations. Both runs used the P0 TckObject/Value/TckInteraction/Payload
declarations in `p0-tck.xml`; adapter supplies provider, FOM, callback model,
and time. Independent-provider behavior and conformance are not claimed.

The default `java-tck.timestamped-directed-zero-lookahead-tar-send-boundary`
case starts TAR(5) with zero lookahead, verifies that a directed send at 5
raises `InvalidLogicalTime`, and accepts a send at 6 before explicitly
servicing the grant callback. The constrained owner stays unadvanced, and the
test confirms it has not received the later send. Its selected requirement
mapping is to §8; the direct Table 1 witness is §8.1.2 for timestamped Send
Directed Interaction. RL-312 records the pinned Lab extraction artifact. The
case uses HLA_EVOKED only. Java current-source JNI runs passed with
HLAinteger64Time and HLAfloat64Time (2/2); installed-package C++ direct runs
passed the same matrix (2/2). The focused CTest selector had no registrations.
Both runs used the P0 TckObject/Value/TckInteraction/Payload declarations in
`p0-tck.xml`. The test does not claim delivery of the send at 6, HLA_IMMEDIATE
behavior, independent-provider behavior, or conformance.

The default `java-tck.zero-lookahead-tar-tso-equality-rejection` case enables
zero-lookahead regulation, advances the sender by three epsilons, and verifies
the TAR grant and Query Logical Time. A timestamped interaction at the grant
time raises `InvalidLogicalTime`; a strictly later timestamp returns a valid
retraction handle. Its direct mappings are §8.1.5. It covers the TAR equality
boundary only, with no NMR or continuation-rule claim. Java current-source JNI
runs passed both callback models with HLAinteger64Time and HLAfloat64Time
(4/4); installed-package C++ primary and contract runs passed the same matrix
(8/8). The focused CTest query had no registrations. The adapter supplies the
provider, root-hierarchy FOM with a parameter-free interaction class, time,
and callback model; independent-provider behavior and conformance are not
claimed.

The default `java-tck.zero-lookahead-nmr-tso-equality-rejection` case enables
zero-lookahead regulation, requests NMR to three epsilons beyond the initial
time, and verifies the grant and Query Logical Time. Sending at the grant
time raises `InvalidLogicalTime`; a strictly later timestamp returns a valid
retraction handle. Its direct mapping is §8.1.5 and covers the NMR equality
boundary only, with no TAR or continuation-rule claim. Java current-source JNI
runs passed both callback models with HLAinteger64Time and HLAfloat64Time
(4/4); installed-package C++ primary and contract runs passed the same matrix
(8/8). The focused CTest query had no registrations. The adapter supplies the
provider, root-hierarchy FOM with a parameter-free interaction class, time,
and callback model; independent-provider behavior and conformance are not
claimed.

The default `java-tck.zero-lookahead-nmr-continuation-tara-equality-acceptance`
case rejects a timestamp equal to a forward NMR grant, then requests a later
TARA grant and accepts a timestamp equal to that grant. Its direct mapping is
to the §8.1.5 continuation sentence. RL-417 tracks the missing precise Lab
requirement for the §8.9 equal-time TARA permission, so this case makes no
separate §8.9 claim. Java current-source JNI runs passed both callback models
with HLAinteger64Time and HLAfloat64Time (4/4); installed-package C++ primary
and contract runs passed the same matrix (8/8). The focused CTest query had no
registrations. It does not generalize to other subsequent advance services or
establish independent-provider behavior or conformance.

The default `java-tck.zero-lookahead-nmra-tso-equality-acceptance` case
requests NMRA with zero lookahead, then accepts a timestamped interaction at
the grant time and checks the valid retraction return. Its direct mapping is
the §8.11 post-grant permission only; it does not test NMRA delivery-set
ordering or other advance services. Java current-source JNI runs passed both
callback models with HLAinteger64Time and HLAfloat64Time (4/4); installed-
package C++ primary and contract runs passed the same matrix (8/8). The
focused CTest query had no registrations. This evidence does not establish
independent-provider behavior or conformance.

The default `java-tck.time-bounds-queries` case checks standard Query GALT and
Query LITS bounds before, during, and after time regulation, including pending
advances. It has six direct §8.18.1/§8.19.3 mappings and passed focused
installed-provider runs in both HLA_EVOKED and HLA_IMMEDIATE. The adapter
supplies the provider JAR, FOM, endpoint, logical-time implementation, and
callback model; independent-provider conformance is not claimed.

The default `java-tck.time-constrained-enable-galt-equality` case checks
delayed Time Constrained Enabled delivery at GALT equality, matching queried
logical time, and a TAR held at the regulating peer's GALT until that peer
advances. Its direct §8.1.2/§8.6/§8.6.4 mappings passed focused installed-
provider runs in both callback models. Provider, FOM, endpoint, time, and
callback configuration remain adapter inputs; this is not an independent-
provider conformance claim.

The default `java-tck.time-constrained-enable-regulating-federate-current-time`
case checks that Time Constrained Enabled reports the already-regulating
federate's current logical time after an advance. Its three direct §8.6/§8.6.3
mappings and focused installed-provider runs pass under both callback models.
The provider, FOM, endpoint, time, and callback model come from the adapter;
this does not establish an independent-provider conformance result.

The default `java-tck.time-regulation-enable-actual-lookahead-tso-send`
case checks the §8.3.4 post-enable send floor: a timestamped interaction
below the assigned actual lookahead is rejected, while one at the floor is
accepted. Focused installed-provider runs passed in both callback models
with the adapter supplying a FOM-matching interaction, parameter, and
HLAinteger64Time. The case uses only the official Java API; no independent-
provider conformance or float64 result is claimed.

The default `java-tck.disable-time-regulation-send-order` case is
HLA_IMMEDIATE-only. After disabling Time Regulation, it observes that a
timestamped Send Interaction arrives in receive order, allowing either
standard receive callback overload. Its direct §8.4 mapping passed a fresh
installed-provider integer64 run; existing focused evidence also covers
float64 with a matching adapter FOM. HLA_EVOKED is reported as unsupported,
not a test failure. Provider, FOM, interaction class, endpoint, time, and
callback configuration remain adapter inputs; no cross-provider conformance
is claimed.

The default `java-tck.time-constrained-enable-retains-queued-tso` case
checks that one timestamped interaction stays queued while Time Constrained
enablement is pending, then arrives as TIMESTAMP/TIMESTAMP before TAR(6)'s
grant. The Java translation now decodes the official callback's sent-order
field and advances the regulator to six before the receiver's TAR(6), as in
its promoted C++ peer. Focused installed-provider runs pass in both callback
models with adapter-supplied FOM, interaction/parameter names, endpoint, and
time implementation. This is a bounded §8.6 result, not conformance.

The default `java-tck.time-constrained-enable-retains-queued-tso-attribute`
case applies the same §8.6 boundary to a timestamped attribute update. It
checks that no receive-order reflection escapes while enablement is pending
and that the queued passel reflects as TIMESTAMP/TIMESTAMP before TAR(6)'s
grant. Focused installed-provider runs pass in both callback models after
the publisher advances to six. The promoted C++ peer's current source omits
that advance and needs a separate fresh runtime audit; no exact-sequence
parity or independent-provider conformance is claimed. FOM, object/attribute
names, endpoint, time, and callback model remain adapter inputs.

The adapter-required
`java-tck.time-constrained-enable-retains-queued-tso-object-deletion` case
checks the §8.6 stored-message branch for a timestamped object deletion while
Time Constrained enablement is pending. It accepts the allowed RECEIVE-order
removal before Time Constrained Enabled; otherwise, the retained removal must
arrive with TIMESTAMP/TIMESTAMP order before TAR(6)'s grant. Focused current-
source JNI and paired installed-package C++ runs passed in both callback
models. This is local-provider evidence only.

The default `java-tck.time-constrained-enable-receive-order-boundary` case
checks two timestamped interactions delivered with RECEIVE order before
Time Constrained Enabled (including one at the receiver's current time),
then a timestamp-7 interaction delivered with TIMESTAMP order before TAR(8)'s
grant. The Java recorder now reads sent order from the official timed
callback field. Focused installed-provider runs pass under both callback
models with adapter-supplied FOM, interaction/parameter names, endpoint, and
time implementation. This is a bounded §8.6 result, not conformance.

The default `java-tck.time-constrained-disable-queued-interaction-to-ro`
case checks the §8.7 interaction transition: a queued timestamped message
arrives through the untimed receive callback after disabling Time
Constrained, and a later timestamped send retains its time while reporting
RECEIVE as received order. The Java recorder reads sent order from the
official timed callback field. Focused installed-provider runs pass under
both callback models with adapter-supplied FOM, interaction/parameter names,
endpoint, and time implementation. Other message families and independent-
provider conformance remain unclaimed.

The default `java-tck.time-constrained-disable-queued-attribute-to-ro`
case checks the same §8.7 transition for attribute updates: a queued
timestamped passel reflects through the untimed callback after disabling
Time Constrained, and a later timestamped update retains its time with
RECEIVE as received order. Focused installed-provider runs pass under both
callback models. Provider, FOM, object/attribute names, endpoint, time, and
callback model remain adapter inputs; other TSO families and independent-
provider conformance are not claimed.

The `java-tck.timestamped-directed-interaction-retraction` scenario is the
single-recipient counterpart: it retracts one queued directed interaction,
advances the receiver without a stale delivery, then sends a later interaction
and verifies its callback metadata, grant ordering, and
`MessageCanNoLongerBeRetracted` terminal boundary. It remains unsupported by
default pending Java-provider execution.

The `java-tck.timestamped-directed-interactions` scenario translates the
combined portable C++ case: it distinguishes target-owner, universal, and
non-owner subscriptions; checks invalid interaction, target, and retraction
handles; observes a receive-order callback and its retraction while the target
is unconstrained; then verifies constrained timestamp-order delivery only to
the target and universal subscriber. It checks callback metadata and the
post-delivery retraction boundary. Provider, FOM, time implementation,
timestamped-directed names, and callback model remain adapter inputs; it is
unsupported by default until exercised against an external Java 1516.1-2025
provider.

The `java-tck.directed-interaction-derived-object-target` scenario registers an
adapter-selected derived object at the publisher, then verifies that a distinct
federate with a universal directed-interaction subscription receives the send
while an attribute-only observer does not. It checks target discovery and
interaction callback metadata through receiver-local standard handles. The
provider, typed FOM, time implementation, endpoint, names, and callback model
remain adapter inputs. This default-run case passed focused installed JNI runs
in both callback models. Derived-target identity and observer exclusion are
supplemental checks, not independent-provider conformance claims.

The `java-tck.timestamped-directed-interaction-derived-object-target` scenario
registers an adapter-selected derived object class with an inherited identity
attribute, directs a timestamped typed interaction to that target, and verifies
that an interested constrained federate receives it before TAR grant while a
second federate without a directed subscription receives none. The FOM/model,
derived class, identity attribute, interaction class, integer parameter, time
implementation, and callback model are adapter inputs. The default names use
the repository's TckTyped model; override them with
`hla.rti.tck.typedDerivedObjectClass`,
`hla.rti.tck.typedIdentityAttribute`,
`hla.rti.tck.typedInteractionClass`, and
`hla.rti.tck.typedIntegerParameter`. The case remains unsupported by default
pending Java-provider execution.

The ordinary tests use the IEEE Restaurant example names by default, including
`HLAobjectRoot.Employee.Server`/`Efficiency`,
`HLAobjectRoot.Food.Drink`/`NumberCups`, and
`HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed`.
Directed/relevance scenarios default to
`HLAinteractionRoot.ServerAction.TakeOrder`. Supply a compatible FOM or
override these names with the `hla.rti.tck.*` system properties.

The provider-independent API-surface inventory checks the public signatures of
the factory, ambassador, encoder, and logical-time interfaces. Every exposed
parameter, result, and exception type must belong to the official
`hla.rti1516_2025` namespace or the JDK. This check can run with the API JAR
alone and does not claim behavioral conformance by an RTI provider.

The ordinary `java-tck.receive-order-object-removal` case is default-run after
installed-package passes in both callback models on 2026-10-22. It witnesses
removal delivery to a known subscriber with the supplied tag and producing
federate (§§6.16, 6.16.4, 6.17.1). The FOM, endpoint, callback model, and
logical-time implementation remain adapter inputs; this is not a cross-provider
conformance claim.

The opt-in `java-tck.receive-order-object-removal-subscription-withdrawal`
scenario deletes an object after two subscribers discover it. In evoked mode,
one subscriber withdraws before callback servicing; both the still-subscribed
and withdrawn subscriber must receive the already-queued terminal removal. In
immediate mode it checks callback delivery before withdrawal. Both paths verify
object/tag/producer identity and suppress owner loopback. Its C++ base/contract
pair passed two independent fresh GNU 15.2/Ninja installed-package builds,
with 4/4 evoked/immediate CTest cases passing in each build. Java diagnostic
runs also passed both callback models on 2026-10-22. The case stays opt-in:
its API-only contract does not pin a standard requirement for queued terminal
removal delivery after subscription withdrawal, so adapter passes are not a
portable conformance claim.

The default `java-tck.resign-delete-objects-multi-recipient-fifo` scenario
registers two objects, then resigns the producer with standard `DELETE_OBJECTS`.
Both active subscribers must receive the two empty-tag removals in producer
order with the correct object and producer handles, and the producer must not
receive loopback. Its C++ base/contract pair passed two fresh GNU 15.2/Ninja
installed-package builds, with 4/4 focused evoked/immediate tests passing in
each. It passed a focused Java run against a fresh GNU/Ninja build of the local
JNI provider using the adapter-selected FOM and logical-time implementation.
Its behavior is observable entirely through the standard 1516.1-2025 API.

The default `java-tck.resign-delete-objects` scenario checks that
`NO_ACTION` resignation fails while the federate owns the standard
delete-privilege attribute, then verifies `DELETE_OBJECTS` delivers one empty-
tag removal with the correct object and producer. It also checks discovery
identity/name metadata and that the removed instance name no longer resolves.
The adapter supplies the FOM, logical-time implementation, provider, and
callback model. Its C++ base/contract pair passed two fresh GNU 15.2/Ninja
installed-package builds, each with all four evoked/immediate focused tests
passing. It passed a focused Java run against a fresh GNU/Ninja build of the
local JNI provider. Its behavior is observable entirely through the standard
1516.1-2025 API.

The default `java-tck.resign-delete-objects-then-divest-cross-object-effects`
scenario gives one federate an attribute on another federate's retained object,
then has it register a separate object and resign with
`DELETE_OBJECTS_THEN_DIVEST`. It checks removal delivery and name lookup for the
departing object's deletion, preservation of the retained object, and
successful acquisition of the released attribute by a third federate. Provider,
FOM, logical-time implementation, and callback model are adapter-supplied. Its
C++ base/contract pair passed two independent GNU 15.2/Ninja installed-package
builds with all four evoked/immediate focused CTest cases passing in each; the
Java scenario passed focused execution against a fresh GNU/Ninja JNI provider.
All assertions use the standard 1516.1-2025 API.

The default `java-tck.resign-delete-then-divest-privilege-epoch` case registers
an object, withdraws and republishes the standard delete privilege, then
registers a second object. On `DELETE_OBJECTS_THEN_DIVEST`, the earlier object
is retained for attribute assumption and only the later one is removed. It
compares discovered and removed objects using the receiver's own handles.
Focused installed JNI runs passed in both callback modes with adapter-supplied
FOM, logical time, provider, endpoint, and callback model; independent-provider
conformance remains unverified.

The default `java-tck.resign-assumption-later-publication` case verifies that
resigning an owner with `UNCONDITIONALLY_DIVEST_ATTRIBUTES` does not offer its
attributes prematurely to a known, nonpublishing candidate. Once that
candidate publishes, it receives one assumption offer with its local object
handle, the ordinary attribute and standard delete privilege, and an empty
tag. The case passed focused installed JNI runs in both callback modes using
adapter-supplied inputs; independent-provider conformance is unverified.

The default `java-tck.resign-assumption-later-discovery` case keeps the
federation alive after the owner resigns, then joins a new candidate. That
candidate discovers the retained object before publication, receives no early
offer, and receives one grouped empty-tag assumption offer after publishing.
It checks the candidate's local object handle and passed focused installed JNI
runs in both callback modes; independent-provider conformance is unverified.

The default `java-tck.ownership-assumption-search-after-decline` case holds a
first candidate's assumption callback while a second candidate receives its
acquisition-cancellation confirmation. It verifies that the unowned search
continues only after the first callback returns, with one tagged offer to the
second candidate. Immediate delivery uses only the standard callback
disable/enable services. Receiver-local handles, adapter-supplied inputs, and
focused installed JNI passes in both callback modes keep this portable;
independent-provider conformance is unverified.

The default `java-tck.ownership-assumption-transfer-epoch` case verifies that
both candidates receive the original tagged ownership offer, one acquires the
attributes, and the previously offered candidate receives a fresh grouped
offer with the new tag after transfer and republication. Object and attribute
handles are resolved per federate through standard API lookups. Focused
installed JNI runs passed in both callback modes; independent-provider
conformance remains unverified.

The default `java-tck.resign-multi-survivor-assumption-fanout` case uses
HLA_EVOKED callbacks to verify that servicing one surviving federate's queued
assumption offer leaves the other's queue untouched. Each empty-tag offer is
checked against receiver-local object and attribute handles. A focused
installed JNI run passed in HLA_EVOKED; HLA_IMMEDIATE is explicitly unsupported
for this queue-ordering assertion. Independent-provider conformance is unverified.

The default `java-tck.negotiated-divestiture-concurrent-assumption` case keeps
a third candidate's tagged assumption visible while owner confirmation is
pending, then checks the tagged acquisition and ownership transfer after
confirmation. It uses receiver-local standard API handles and excludes C++
process transport mechanics. Focused installed JNI runs passed in both callback
modes; independent-provider conformance is unverified.

The default `java-tck.confirm-divestiture-from-callback` case checks the
tagged owner confirmation request, calls Confirm Divestiture from that callback,
and verifies one tagged acquisition and ownership transfer using each
federate's locally resolved standard handles. Callback re-entry is supplemental
public-API evidence; C++ process transport is excluded. Focused installed JNI
runs passed in both callback modes; independent-provider conformance is unverified.

The default `java-tck.confirm-divestiture-mixed-set-atomicity` case submits
one requested and one unrequested attribute together to Confirm Divestiture.
It checks the typed rejection, unchanged ownership and acquisition callbacks,
then successful confirmation of the requested attribute alone. The two-attribute
FOM and names come from the adapter; the requester uses its local standard
handles. Focused installed JNI runs passed in both callback modes; process and
MOM mechanics and independent-provider conformance remain outside this evidence.

The default `java-tck.resign-unconditionally-divest-preserves-objects` scenario
establishes foreign ownership of a surviving federate's object, then resigns
the owner with `UNCONDITIONALLY_DIVEST_ATTRIBUTES`. It verifies that the object
remains registered and discoverable, no removal is delivered by that resignation,
and a third federate can acquire the released attribute with the requested tag.
Only a later `DELETE_OBJECTS` resignation removes the object. The Java scenario
passed both callback models against the adapter-selected 1516.1-2025 API and FOM;
its C++ base/contract pair passed four focused tests in each of two independent
GNU 15.2/Ninja installed-package builds. The Java source uses only the official
API and JDK, with provider, FOM, time implementation, and callback model supplied
by the runner.

The default `java-tck.resign-cancel-pending-ownership-acquisitions-action`
scenario queues a standard ownership acquisition, resigns the requester with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and verifies the registered object and
current owner are unchanged. The owner then divests and a successor successfully
acquires the attribute with the expected callback tag before ordinary cleanup;
the successor must not receive an `attributeOwnershipUnavailable` callback.
The Java scenario passes both callback models, and its C++ base/contract pair
passes all four focused evoked/immediate cases in each of two independent
GNU 15.2/Ninja installed-package builds. The test uses only the official
1516.1-2025 Java API; provider, FOM, time implementation, and callback model are
adapter-supplied. Its callback wait helper services callbacks only for the
evoked model, avoiding an illegal evoke call under immediate delivery.

The default `java-tck.resign-unconditional-divestiture` case verifies that
`resign(NO_ACTION)` is rejected while the departing federate still owns its
attribute, and that `UNCONDITIONALLY_DIVEST_ATTRIBUTES` produces an assumption
without transferring ownership until the surviving federate invokes
`attributeOwnershipAcquisition`. It checks the acquisition callback tag and
resulting ownership query. Its one direct mapping is limited to the resignation
exception/consent path in IEEE 1516.1-2025 §7.1. Focused current-source Umbra JNI
runs passed both callback modes using `known-class-advisory-tck.xml`,
`HLAinteger64Time`, and `HLAobjectRoot.TckKnownClassObject.TckKnownClassDerivedObject`;
results are under
`out/java-tck/resign-unconditional-divestiture-java-parity-20261115/`. No other
§7.1 exception path, independent-provider behavior, or conformance is claimed.

The default `java-tck.resign-divests-all-instance-attributes` case publishes two
attributes on each of two objects, then verifies unconditional resignation
retains both objects and releases both attributes. Each survivor ownership query
must produce exactly one `attributeIsNotOwned` callback containing the requested
pair, with no ownership-information or RTI-owned callback. The direct mapping is
limited to the two-object/two-attribute witness in §7.1.2.1, not the complete
resignation-action matrix. Focused current-source Umbra JNI runs passed 1/1
under each callback mode using `p0-tck.xml`, `HLAinteger64Time`,
`HLAobjectRoot.TckObject`, `Value`, and `Name`. Results/JUnit are under
`out/java-tck/resign-divests-all-instance-attributes-java-parity-20261116/`.
The paired installed-package C++ primary/contract selectors passed 4/4 focused
CTests and 4/4 direct executions across both callback models; evidence is under
`.build/cpp-tck-evidence-20261116/resign-divests-all-instance-attributes-java-parity-20261116.{json,xml}`.

The default `java-tck.automatic-resign-directive-lifecycle-boundaries` scenario
checks that the standard automatic-resign directive getter/setter work while
joined, report `FederateNotExecutionMember` after resignation, and report
`NotConnected` after disconnect. The Java scenario passes with both configured
callback models; its C++ base/contract pair passes all four focused tests in
each of two GNU 15.2/Ninja installed-package builds. Only the standard Java API
is used; provider, FOM, logical-time implementation, and callback model remain
adapter inputs.

The default `java-tck.automatic-resign-directive-standard-enumerator-values`
scenario sets and reads back all six standard Java `ResignAction` constants.
It passes with both callback configurations, and the corresponding C++ base/
contract pair passes all four focused tests in each of two GNU 15.2/Ninja
installed-package builds. The Java enum cannot represent the invalid integer
cast tested by C++; that negative boundary remains explicitly C++-only, while
the shared parity slice covers the standard enumerator values.

The opt-in, adapter-backed `java-tck.federate-lost-mom-report` scenario follows
the C++ loss case. It selects `NO_ACTION`, checks the lost-federate report and
cutoff timestamp, confirms the queued TSO arrives before the survivor grant,
verifies the post-loss `HLAfederatesInFederation` roster and public member list,
then completes a synchronization point with the single survivor. The roster
update maps directly to §4.1.6; synchronization completion is supplemental and
has no separate direct mapping. Current-source Umbra JNI runs passed in both
callback models with the `public-server-loss` fixture, `p0-tck.xml`, and the
standard MIM. Independent-provider behavior is not claimed.

The default `java-tck.advisory-switch-value-round-trips` scenario toggles each
of the four standard relevance/scope advisory switches away from its current
value and restores it, checking both getter results. It passes with evoked and
immediate callback configuration, as does the C++ base/contract pair in each of
two independent GNU 15.2/Ninja installed-package builds. The scenario uses only
the official 1516.1-2025 API and adapter-supplied federation inputs.

The default `java-tck.resign-cancel-then-delete-then-divest-cross-object-effects`
scenario exercises the compound `CANCEL_THEN_DELETE_THEN_DIVEST` action across
three distinct objects: cancel the departing federate's pending acquisition,
remove its registered object, and release its ownership on a retained object.
It verifies removal metadata and object-name lookups, then checks that a
successor can acquire both released attributes before the original owner deletes
its two objects. Java passes in both callback models; the C++ base/contract pair
passes the four focused tests in each of two GNU 15.2/Ninja installed-package
builds. The scenario depends only on the official Java API and adapter inputs.
Its bounded callback waits service callbacks only in the evoked model; immediate
delivery never invokes `evokeCallback`.

The `java-tck.synchronization` scenario now uses the adapter-selected callback
model for every participant. Evoked mode explicitly services callbacks;
immediate mode checks the same registration, announcement, and barrier results
without calling the evoked callback service. The scenario remains provider-
unverified until run against an installed Java 1516.1-2025 provider.

The focused `java-tck.synchronization-points-independent-tracking` twin registers
two labels with distinct tags, checks both registration results and announcements,
then completes one label while the other remains pending before completing the
second. Profile-enabled local JNI runs passed under `HLA_EVOKED` and
`HLA_IMMEDIATE` with the adapter-supplied `p0-tck.xml` and
`HLAinteger64Time`; the direct mapping is limited to §4.1.3.

## Build

From the repository root, use the shell-free Python runner. These commands are
single lines so they work in any shell:

```text
python tools/run_java_tck.py build --api-jar PATH/TO/hla-1516.1-2025-api.jar --output-directory out/java-tck/classes
```

Only the API JAR is needed at compile time.

## Run

At runtime, place the provider JAR and any provider dependencies on the class
path. The FOM module is required for lifecycle and ordinary data scenarios.

```text
python tools/run_java_tck.py run --api-jar PATH/TO/hla-1516.1-2025-api.jar --provider-jar PATH/TO/provider-rti.jar --fom-path PATH/TO/fom.xml --classes-directory out/java-tck/classes --results-path out/java-tck/provider.results.json --junit-path out/java-tck/provider.junit.xml
```

Use `--dependency-jar` for additional provider dependencies and
`--jvm-argument` for provider-owned JVM flags. The reusable runner does not
interpret those flags. By default, scenarios without Java provider-runtime
evidence remain `unsupported`, even when selected with `--scenario`.
Add `--run-unverified` to attempt those cases against the supplied provider;
this is an execution opt-in, not a conformance or promotion claim. An explicit
`unsupported` or `not_applicable` entry in an optional `--capability-profile`
still wins. Add `--scenario java-tck.<id>` to focus a run on one catalog
scenario; other entries are marked `filtered` in the result report.
For diagnostic exception stacks, pass
`--jvm-argument=-Dhla.rti.tck.stacktrace=true`.

`java-tck.fom-matching-data-types` is a default-run pure-standard case mapped
to IEEE 1516.2-2025 §C.3. Supply two valid matching-declaration modules via
`hla.rti.tck.matchingDataTypesFirstFom` and
`hla.rti.tck.matchingDataTypesSecondFom` as adapter-owned JVM arguments. The
installed JNI adapter passed both callback models, matching the
C++ primary and contract evidence, but this is not cross-provider conformance.

`java-tck.fom-data-type-forward-reference` and
`java-tck.fom-data-type-conflicting-definition` are default-run, API-only
§C.3 cases. Supply the consumer and provider FOM paths through
`hla.rti.tck.dataTypeMergeConsumerFom` and
`hla.rti.tck.dataTypeMergeProviderFom`; the conflict case also requires
`hla.rti.tck.dataTypeMergeConflictingProviderFom`. The first case rejects an
unresolved array element type and accepts the later module that defines it.
The second accepts each compatible pair separately, rejects their conflicting
combination, and checks valid federation-name reuse. The installed Java
adapter passed both callback models, matching the C++ primary and contract
evidence; independent-provider conformance is not claimed.

`java-tck.fom-variant-alternative-overlap` is a bounded default-run §C.3
case. Supply `hla.rti.tck.variantAlternativeBaseFom`,
`hla.rti.tck.variantAlternativeDisjointFom`, and
`hla.rti.tck.variantAlternativeOverlapFom` through the adapter. It accepts
disjoint named alternatives, rejects an overlap, and checks valid
federation-name reuse. The installed Java adapter passed both callback
models, as did the C++ primary and contract. This relies on the intact
general named-data-type conflict rule; the variant-specific continuation is
clipped in the Requirements Lab (RL-319), so full variant merging is not
claimed.

`java-tck.fom-interaction-parameter-merge-conflict` is default-run for a
bounded §C.3 parameter-set merge. Supply
`hla.rti.tck.interactionParameterBaseFom`,
`hla.rti.tck.interactionParameterDisjointFom`, and
`hla.rti.tck.interactionParameterConflictFom` through the adapter. The case
resolves both disjoint parameters after join, rejects a conflicting type,
and checks valid federation-name reuse. The installed Java adapter and
C++ primary/contract each passed both callback models; this does not cover
every parameter sub-element or establish cross-provider conformance.

`java-tck.fom-object-attribute-merge-conflict` is default-run for bounded
§C.3 object-attribute union and one semantics-field conflict. Supply
`hla.rti.tck.objectAttributeBaseFom`,
`hla.rti.tck.objectAttributeDisjointFom`, and
`hla.rti.tck.objectAttributeConflictFom` through the adapter. The adjacent
default-run `java-tck.fom-object-class-semantics-conflict` uses the same
base/disjoint modules plus `hla.rti.tck.objectClassSemanticsConflictFom` to
reject one same-named class-semantics conflict and check a valid retry.
Both passed the installed Java adapter in both callback models, matching
their C++ primary/contract evidence. Neither claims all class or attribute
fields, nor independent-provider conformance.

`java-tck.fom-object-class-dimension-merge` and
`java-tck.fom-interaction-class-dimension-merge` are default-run, API-only
Clause 7 cases. Supply the first and second FOM modules through
`hla.rti.tck.objectDimensionMergeFirstFom` and
`hla.rti.tck.objectDimensionMergeSecondFom` for the object case, or
`hla.rti.tck.interactionDimensionMergeFirstFom` and
`hla.rti.tck.interactionDimensionMergeSecondFom` for the interaction case.
Each checks that both modules work separately and that a joined member sees
exactly their two available dimensions after composition. The installed
Java adapter and matching C++ primary/contract cases passed both callback
models. Region operations, conflict branches, and cross-provider
conformance are not claimed.

The opt-in `java-tck.fom-interaction-inherited-available-dimensions` case
uses the adapter DDM FOM through `hla.rti.tck.ddmFom` and checks that the base
interaction class exposes exactly dimension X while its derived class exposes
inherited X plus Y. It tests the public available-dimensions query only; send
routing and region filtering are outside its claim. Focused local Umbra JNI
runs passed with both callback models; behavior on other providers is
unverified.

`java-tck.fom-named-variant-alternative-merge` is a default-run positive
§C.3 case: separately valid modules with disjoint enumerators on one named
extendable-variant alternative also create together. Supply them through
`hla.rti.tck.namedVariantAlternativeFirstFom` and
`hla.rti.tck.namedVariantAlternativeSecondFom`. The default-run
`java-tck.fom-extendable-variant-hlaother` instead checks the explicit
page-111 HLAother rejection and a valid retry; supply
`hla.rti.tck.extendableHlaOtherValidFom` and
`hla.rti.tck.extendableHlaOtherInvalidFom`. Both passed the installed Java
adapter and matching C++ primary/contract cases in both callback modes.
RL-319 clips the separate variant conflict continuation: neither case
claims complete variant-record merging or independent-provider conformance.

`java-tck.fom-enumerated-value-collision` and
`java-tck.fom-nonextendable-variant-alternative-conflict` are default-run
§C.3 rejection cases. The adapter supplies the respective
`hla.rti.tck.enumeratedCollisionFirstFom`/`enumeratedCollisionSecondFom` or
`hla.rti.tck.nonextendableVariantFirstFom`/`nonextendableVariantSecondFom`
modules. Each accepts its modules separately, rejects their conflicting
combination, and checks valid federation-name reuse. The installed Java
adapter and matching C++ primary/contract cases passed both callback modes;
no extendable-variant or cross-provider conformance claim is made.

`java-tck.federate-asynchronous-delivery-mim-attribute` is default-run with an
adapter-supplied application FOM and standard MIM. It reads the initial MOM
`HLAasynchronousDelivery` value, toggles the standard services, and verifies
conditional reflections. A non-null producer handle must be rejected as
`InvalidFederateHandle` by the standard `getFederateName` service; a null handle
also denotes no producer. This avoids assuming that all Java providers encode
an invalid handle as null. The installed JNI adapter passed both callback
models after that API-only oracle was applied.

The adjacent default-run `java-tck.federate-time-constrained-mim-attribute`
and `java-tck.federate-time-regulating-mim-attribute` cases use the same
adapter-supplied MIM and Boolean reflection oracle. They verify conditional
MOM values, direct value requests, and the corresponding enable callback;
the regulation case obtains epsilon lookahead from the standard time factory.
Both passed the installed JNI adapter in evoked and immediate callback modes,
matching their C++ primary and contract evidence.

`java-tck.federate-time-manager-state-mim-attribute` is also default-run with
the adapter-supplied standard MIM. It checks TimeAdvancing and TimeGranted MOM
reflections for TAR, TARA, NMR, NMRA, and Flush Queue Request, including the
distinct Flush Queue Grant callback. Its RTI-originated producer check uses
only the standard handle lookup API. The installed JNI adapter and matching
C++ primary/contract passed both callback models.

The default-run periodic `java-tck.federate-logical-time-mim-periodic` and
`java-tck.federate-lookahead-mim-periodic` cases use standard `HLAsetTiming`
reports and the adapter-selected logical-time factory. They compare decoded
MOM values with the corresponding Query Logical Time or Query Lookahead
services, including successful advances or a changed lookahead. Both passed
the installed JNI adapter in evoked and immediate modes, with matching green
C++ primary/contract evidence. Provider, FOM, MIM, endpoint, time factory, and
callback model remain adapter configuration.

`java-tck.federate-automatic-resign-action-mim-attribute` is default-run with
the adapter-supplied standard MIM. It checks the initial automatic resign
action, a different standard action, and restoration through both direct MOM
requests and conditional reflections. The producer-handle check uses only the
standard Get Federate Name service. The installed JNI adapter and the C++
primary/contract passed both callback models.

`java-tck.federate-automatic-resign-action-mom-switch` is a separate
default-run standard-MIM interaction case. It sends individual
`HLAsetSwitches.HLAautomaticResignAction` values, verifies the public getter
and unchanged neighboring switches, then restores the starting directive.
The installed JNI adapter and C++ primary/contract passed both callback
models; the case does not claim resignation side effects.

Three separate default-run `HLAreportMOMexception` cases cover malformed
`HLAsetSwitches` enumerators, an omitted required synchronization-status
parameter, and the Service Reporting subscription precondition. They verify
standard report identity and parameter-error values without prescribing a
synchronous exception type or report wording. Their RTI-originated producer
check uses only standard handle lookup. Each passed the installed JNI adapter
in both callback modes, matching its C++ primary and contract evidence.

`java-tck.federation-mom-exception-missing-fom-module-indicator` is a separate
default-run federation-level request case. It omits the standard
`HLAFOMmoduleIndicator`, then verifies the resulting `HLAreportMOMexception`
service identity, joined-federate identity, and parameter-error value. The
installed JNI adapter and C++ primary/contract passed both callback models.

The focused time/FOM parity cases take their fixtures from adapter-supplied
Java properties, passed through `--jvm-argument=-D...`:

- `hla.rti.tck.defaultLogicalTimeFom`: a valid FOM with no declared time
  representation;
- `hla.rti.tck.timeRepresentationValidFom`: a valid FOM documenting
  `HLAinteger64Time`;
- `hla.rti.tck.timeRepresentationBasicDataFom` and
  `hla.rti.tck.timeRepresentationReferenceDataFom`: schema-valid FOMs with
  disallowed time-representation data type categories; and
- `hla.rti.tck.unsupportedTimeImplementation`: a factory name the selected
  provider adapter knows is unavailable.

These inputs enable `java-tck.fom-default-logical-time-selection`,
`java-tck.fom-unavailable-logical-time-factory`,
`java-tck.fom-time-representation-valid`, and
`java-tck.fom-time-representation-rejections` as default-run cases when the
provider adapter supplies and attests the required inputs. The valid case
accepts an integer64 time-representation FOM; the rejection case rejects
raw basic and reference data types and checks valid federation-name reuse.
The default-time case observes `HLAfloat64Time` through the standard time
factory; the unavailable-factory case requires an adapter-attested absent
factory name. The installed Java adapter and matching C++ primary/contract
cases passed both callback models. Float64 representation acceptance, other
factories, and independent-provider conformance are not claimed.

`java-tck.fom-member-data-type-categories` is also default-run. Supply
`hla.rti.tck.memberDataTypeValidFom`,
`hla.rti.tck.memberDataTypeAttributeBasicFom`, and
`hla.rti.tck.memberDataTypeParameterBasicFom` through the adapter. It accepts
simple-data members, rejects raw basic attribute and parameter data types,
and checks valid federation-name reuse. The installed Java adapter and
matching C++ primary/contract passed both callback models; the parameter
source mapping has only Clause 4 granularity, and cross-provider conformance
is not claimed.

The NA attribute-companion cases are default-run when the adapter supplies
`hla.rti.tck.attributeNaValidCompanionsFom`,
`hla.rti.tck.attributeNaOmittedCompanionsFom`,
`hla.rti.tck.attributeNaOmittedCompanionsExtensionFom`,
`hla.rti.tck.attributeNaInvalidUpdateTypeFom`,
`hla.rti.tck.attributeNaInvalidUpdateConditionFom`, and
`hla.rti.tck.attributeNaInvalidTransportationFom`. The completion case
accepts an explicit NA companion row and a partial row completed by a second
module. The rejection case rejects three incompatible companions and verifies
that the federation name remains reusable. The installed Java adapter and
matching C++ primary/contract cases passed both callback models; independent-
provider conformance is not claimed.

The dynamic update-condition cases are also default-run with adapter-supplied
`hla.rti.tck.dynamicUpdateConditionValidFom`,
`hla.rti.tck.conditionalUpdateConditionNaFom`,
`hla.rti.tck.periodicUpdateConditionNaFom`, and
`hla.rti.tck.conditionalUpdateConditionEmptyFom`. They accept supplied
Conditional/Periodic conditions and an omitted partial condition, reject
NA/empty supplied conditions, and verify valid federation-name reuse. The
installed Java adapter and matching C++ primary/contract cases passed both
callback models. The pinned source candidate maps only to Clause 4;
independent-provider conformance is not claimed.

The default-run array-cardinality cases and adjacent dimension-duplicate
cases use adapter-supplied FOM paths. `arrayCardinalityValidFom` supplies the
accepted scalar/list/range/Dynamic/mixed forms;
`arrayCardinalityReversedRangeFom`,
`arrayCardinalityFixedMismatchFom`, and
`arrayCardinalityVariableMismatchFom` supply the three independent rejection
branches. `dimensionDuplicateBaseFom` pairs with
`dimensionDuplicateEqualUniqueFom` for the accepted union or with
`dimensionDuplicateConflictFom` for the upper-bound conflict. Prefix each
name with `hla.rti.tck.` when passing it as a Java property. These fixtures
enable the four corresponding `java-tck.fom-array-cardinality-*` and
`java-tck.fom-dimension-duplicate-*` scenarios only for adapters that supply
and attest the relevant 2025 modules. The two array-cardinality cases passed
the installed Java adapter in both callback models, matching green C++
primary/contract evidence; no independent-provider conformance is claimed.

The dimension-input cases additionally require the standard MIM via
`--mim-path`. Configure `dimensionInputRawBasicFom`, `dimensionInputNaFom`,
and `dimensionInputTableTypeFom` for the input-type case;
`dimensionDescriptionNaInvalidFom`, `dimensionDescriptionTextFom`,
`dimensionDescriptionNamedNaFom`, and `dimensionDescriptionNamedTextFom`
for the description case; and `dimensionInputMixedNaFom` plus the accepted
NA, described-empty, and table-type FOM paths for the NA-exclusivity case.
All names in this paragraph are `hla.rti.tck.`-prefixed Java properties,
and each provider adapter must supply schema-valid 2025 modules.

Further FOM parity fixtures are also adapter properties: use
`dimensionDuplicateWithinModuleFom` with the two legal duplicate-dimension
modules for the name-uniqueness case; the four `objectClassName*Fom` paths
for local-name conventions; `modelModificationDateTimezoneFom` and
`modelModificationDateExactFom` for date presentation; and
`updateRateZeroPreflightFom` with `updateRatePositivePreflightFom` for
rate positivity. Equal and conflicting cross-module update rates use
`updateRateDuplicateBaseFom` paired respectively with
`updateRateDuplicateEqualFom` or `updateRateDuplicateNoteConflictFom`.
Prefix each property with `hla.rti.tck.` and supply `--mim-path` to the
cases that request the standard MIM. These cases remain default-disabled
until a provider adapter attests its fixtures.

The dimension-default-range case uses `dimensionDefaultRangeOutsideFom`
and `dimensionDefaultRangeBoundedFom`, with the standard MIM. Duplicate
synchronization and transportation rows use a `*DuplicateBaseFom` paired
with either `*DuplicateEqualFom` or `*DuplicateNoteConflictFom`; use the
`synchronization` or `transportation` prefix as appropriate. Pass these as
`hla.rti.tck.`-prefixed Java properties backed by adapter-supplied 2025
modules. The default-disabled status remains until that provider supplies
and attests the fixtures.

The FOM switch-duplicate parity cases use adapter-owned modules for explicit
Disabled, equivalent, Enabled, and omitted values. Supply the properties
`switchDuplicateDisabledFom` paired with `switchDuplicateEqualFom`,
`switchDuplicateEnabledFom`, or `switchDuplicateOmittedFom`; the advisory
variants use the analogous `advisorySwitchDuplicate*Fom` names. Prefix each
name with `hla.rti.tck.` when passing it as a Java property. These cases
remain default-disabled until the selected provider supplies and attests
schema-valid 2025 modules.

`matrix` runs the same compiled classes against two or more provider
configurations. `matrix.example.json` shows the portable configuration shape;
its paths are placeholders relative to that JSON file. Each entry may set
`runUnverified: true` and may omit `capabilityProfile`, or point to a provider
profile whose explicit exclusions take precedence:

```text
python tools/run_java_tck.py matrix --configuration packages/hla-rti-java-tck/matrix.example.json --classes-directory out/java-tck/classes --output-directory out/java-tck/matrix
```

The optional JPype handoff is available directly as
`python packages/hla-rti-java-tck/jpype_smoke.py`.

Scenario/API/requirement traceability is maintained in
`compliance/catalogs/java-tck-scenario-catalog.json` and validated by
`tools/java_tck.py`.

The focused `java-tck.remote-delete-object-instance-name-lookup-boundary` case
checks that object-name/handle lookups round-trip before deletion, then become
unknown to both the deleting federate and a subscriber after remote removal.

The focused `java-tck.resign-delete-object-lookup-boundary` case checks the
standard `DELETE_OBJECTS` resignation callback and verifies that the surviving
member can no longer resolve the removed object's name, handle, or known class.

The focused `java-tck.resign-no-action-owned-attributes-rejection-boundary`
case verifies that `NO_ACTION` is rejected while the federate still owns an
object attribute and that the object's identity remains queryable afterward.

The opt-in `java-tck.timestamped-attribute-update-no-fanout` case publishes and
registers an object without subscribing any federate, then verifies timestamped
updates still return retraction handles, a successful first retraction becomes
non-retractable, and a second queued update becomes non-retractable after the
publisher advances through its timestamp. No reflection or retraction callback
may loop back to the publisher. FOM, logical-time implementation, provider, and
callback model are adapter-supplied; the case is paired with the promoted C++
scenario and remains unsupported pending Java-provider runtime evidence.

The opt-in `java-tck.attribute-update-service-boundaries` case registers an
object with one active and one passive subscriber, then verifies discovery and
ordinary attribute reflection reach both after active relevance is established.
It checks object/attribute/value/tag/producer and resolvable transportation
metadata, and confirms the update does not reflect back to its producer. The
adapter supplies the FOM, names, provider, endpoint, and callback model; this
scenario is paired with the promoted C++ case and remains unsupported pending
Java-provider runtime evidence.

The default `java-tck.dimension-lookup-valid-handles-after-resignation` case
resolves object, interaction, and dimension handles from `fom-model-tck.xml`
using `HLAinteger64Time`, then verifies their names and a positive dimension
bound while joined. After resignation, the same valid handles must fail
dimension and class-available-dimensions queries with
`FederateNotExecutionMember`. It performs no DDM region operations. Focused
runs passed under both callback models on the current-source Umbra JNI provider.

The opt-in `java-tck.interaction-class-lookup-lifecycle` case checks lookup
connection and membership boundaries, stable base-class handle/name round trips,
unknown-name and invalid-handle failures, and extension interaction visibility
before and after a federate joins with an adapter-supplied additional FOM module.
It binds only to the official Java API; Java's null handle is used for the
invalid-handle input because the API exposes no public invalid-handle
constructor. Provider runtime remains unverified until run against an installed
Java 1516.1-2025 provider.

The opt-in `java-tck.object-class-lookup-lifecycle` case checks object-class
lookup connection and membership boundaries, stable base-class handle/name
round trips, unknown-name and invalid-handle failures, and extension object
class visibility before and after an additional-FOM join. It uses only the
official Java API and adapter-provided FOM/configuration. Java's null handle is
used for the invalid-handle input because the API exposes no public invalid-
handle constructor; provider runtime remains unverified.

`java-tck.attribute-lookup-lifecycle` directly checks Get Attribute Handle
and Get Attribute Name results for inherited and derived attributes across two
joined federates. Unknown-name and wrong-class errors remain supplemental.
The adapter supplies `fom-model-tck.xml` and `HLAinteger64Time`; focused runs
passed under both callback models on the current-source Umbra JNI provider.
Java exposes no standard default-invalid object-class or attribute handle, so
the C++ invalid-handle and handle-based pre-connect/pre-membership checks are
not mirrored. This is local-provider evidence, not independent-provider
interoperability or conformance evidence.

`java-tck.parameter-lookup-lifecycle` checks parameter handle results and exact
parameter-name round-trips across base/derived interaction classes and two
joined federates. Unknown-name and valid wrong-class errors are supplemental.
It uses the adapter-supplied `fom-model-tck.xml` and `HLAinteger64Time`; focused
current-source Umbra JNI runs passed under both callback models. Java exposes no
standard default-invalid interaction-class or parameter handle, so the C++
invalid-handle and handle-based pre-connect/pre-membership checks are not
mirrored. This is local-provider evidence, not independent-provider
interoperability or conformance evidence.

The opt-in `java-tck.regional-attribute-value-request-filtering` case exercises
regional `requestAttributeValueUpdateWithRegions` overlap filtering, empty
request sets, default-region object eligibility, invalid foreign/context/
uncommitted request regions, and callback-time re-evaluation for the configured
callback model. In addition to the adapter-supplied DDM FOM, object and
attribute names, and two object dimensions, it requires the unrelated dimension
name through `hla.rti.tck.ddmUnrelatedDimension`; provider runtime remains
unverified.

The default-run `java-tck.regional-object-update` case registers a named object
with an explicit source region and checks overlapping discovery and reflection,
the regional provide/update response, and source-region callback metadata. It
rejects deleted region handles for subscription and update association, verifies
that empty association sets do not create default routing, checks additive and
repeated source associations and selective removal, then removes the final
explicit association and verifies default-region delivery with an empty source
designator set. A later update is suppressed after the receiver unsubscribes.

The same scenario now runs the C++ helper cases in sequence. The hierarchy case
requests `Name` at `TckKnownClassObject` and verifies provider callbacks and
receive-order responses for two base objects and one derived object in
overlapping regions. It excludes a fourth object in a disjoint region and an
object owned by the requester. Its timestamped subcase requests a regional
update, queues a timestamped response, confirms there is no early reflection,
then advances both federates and checks the response time, orders, producer,
value, and tag. The empty-request case sends an empty regional pair followed by
an ordinary class request and verifies that only the ordinary request produces
a callback with its own tag. Java has no portable default-invalid `RegionHandle`
value, so this case uses a deleted API-created region handle to verify
`InvalidRegion`; it does not claim the exact C++ sentinel representation.

Supply `ddm-tck.xml` through `hla.rti.tck.ddmFom`, `HLAobjectRoot.TckObject` and
`Value` through the adapter object/attribute properties, `TckDimensionX` and
`TckDimensionY` through `hla.rti.tck.dimension` and
`hla.rti.tck.ddmSecondDimension`, and `HLAinteger64Time` through the logical-time
setting. The known-class hierarchy and `Name` attribute are also supplied by
`ddm-tck.xml`. Direct mappings are limited to the exercised paths at IEEE
1516.1-2025 §§6.1.2, 9.1.1, 9.2, 9.5, 9.6, 9.6.3, 9.7, 9.7.3, 9.7.4, 9.9,
9.13, and 9.13.3; the `cpp-1516.1-*` IDs remain provenance only. The Java case
passed under both callback models against local Umbra JNI. The rebuilt C++
primary and contract passed 4/4 direct executions across both callback models;
these adapter-backed scenarios have no matching CTest entries in the focused
build. Results/JUnit are in
`out/java-tck/regional-object-update-java-parity-20261116/{hla_evoked-full,hla_immediate-full}/`
and `.build/cpp-tck-regional-object-update-java-parity-20261116-built.{json,xml}`.
Other providers remain unverified; these runs do not establish conformance.

The default `java-tck.regional-attribute-value-update-response-recheck` case
requests an attribute update while source and requester regions overlap, then
moves the requester from `[0,1)` to `[1,2)`. These semi-open ranges touch but do
not overlap, so the response is suppressed. After overlap is restored, the
fresh response preserves the value, tag, producer, reliable transportation,
and one source-region designator.
The DDM FOM, object/attribute names, and two dimensions remain adapter-supplied.
The page-216 l66-19 fragment maps collectively to IEEE 1516.1-2025 §9.1.1 under
RL-381; RL-380 tracks the page-level Clause 8/Clause 9 mismatch. Focused Umbra
JNI runs passed under both callback models, and the installed-package C++
primary/contract passed under both callback models. Results are in
`out/java-tck/regional-attribute-value-update-response-recheck-java-parity-20261116/`
and `out/cpp-tck-evidence-20261116/`. Behavior on other providers remains
unverified; these runs do not establish conformance.

The default `java-tck.regional-attribute-update-callback-ddm-recheck` case
mirrors the C++ callback-time DDM scenario. It rejects a subscription region
with a dimension unavailable to the object class, then checks overlap,
disjoint, and restored-overlap delivery after committing receiver-region
changes. The evoked path changes the region before servicing the queued
reflection; the immediate path records delivery before the mutation. The case
also checks source-region designator metadata, an empty subscriber with no
discovery or reflection across registration and all nine updates, source-region
unassociation, receiver unsubscription, and empty regional Subscribe and
Unsubscribe controls. Supply `ddm-tck.xml` through
`hla.rti.tck.ddmFom`, `HLAobjectRoot.TckObject` and `Value` through
`hla.rti.tck.ddmObjectClass` and `hla.rti.tck.ddmAttribute`, and the X, Y, and
unrelated dimensions through `hla.rti.tck.dimension`,
`hla.rti.tck.ddmSecondDimension`, and
`hla.rti.tck.ddmUnrelatedDimension`. The direct mappings follow the C++
primary at IEEE 1516.1-2025 §§9.1.3.2, 9.3, and 9.8. Focused Java runs passed
with both callback models against local Umbra JNI. A C++ direct rerun of the
primary/contract pair passed 4/4 against the installed MinGW package, using the
existing portable TCK executable; results are in
`.build/cpp-tck-evidence-20261116/regional-attribute-update-callback-ddm-recheck-java-parity-20261116.{json,xml}`.
The existing focused CTest record passed 4/4 at
`out/mgw/tck-dse-ro-regional-20261016/regional-attribute-update-callback-ddm-recheck-verified-20261016.json`.
A fresh C++ adapter rebuild could not link because the current GCC 15 toolchain
conflicts with the installed GCC 12 package library, so it produced no fresh
CTest matrix. Other providers remain unverified; these runs do not establish
conformance. Java results are in
`out/java-tck/regional-attribute-update-callback-ddm-recheck-20261116/`.

The opt-in `java-tck.ddm-unsubscribe-preserves-other-class-subscription` case
registers one ordinary and one regional object class before subscription, then
checks their independent discovery and reflection paths as each final attribute
subscription is removed. It uses the adapter DDM FOM through
`hla.rti.tck.ddmFom` and the configured `hla.rti.tck.ddmObjectClass`,
`hla.rti.tck.ddmAttribute`,
`hla.rti.tck.knownClassObjectClass`, `hla.rti.tck.knownClassAttribute`,
`hla.rti.tck.dimension`, and `hla.rti.tck.ddmSecondDimension` values. Focused
Umbra JNI runs passed with both callback models; behavior on other providers is
unverified.

The opt-in `java-tck.ddm-available-attribute-services` case checks the
available-attribute rule on ordinary Publish/Unpublish/Subscribe/Unsubscribe
and regional Subscribe/Unsubscribe calls. Its adapter DDM FOM must provide the
configured base and derived classes, base and derived attributes, and two
shared dimensions using the `hla.rti.tck.knownClass*`,
`hla.rti.tck.ddmFom`, `hla.rti.tck.dimension`, and
`hla.rti.tck.ddmSecondDimension` properties. Focused Umbra JNI runs passed
with both callback models; the exception type is checked as Java API behavior,
and behavior on other providers is unverified.

The default-run `java-tck.regional-interaction-empty-subscription-sets` case
checks that an empty regional subscription creates no delivery route, an empty
regional unsubscription leaves an existing route intact, and unsubscribing a
committed region absent from that route is a no-op. It also checks that
unsubscribing a deleted region fails with `InvalidRegion` without disturbing the
active route, then verifies that removing the active region suppresses later
delivery. Callback checks cover payload, producer, transport, tag, and one
receiver-local source-region designator copy with its dimensions and `[0,2)`
bounds. Direct mappings are limited to the three §9.11 unsubscription outcomes
and the deleted-region §9.11.3 exception outcome; empty-subscription filtering,
fixture checks, handles, and callback metadata are supplemental. The pinned
Java API, DDM FOM, object class, attribute, interaction, parameter, dimensions,
and callback model are adapter supplied.

The focused run passed under HLA_EVOKED and HLA_IMMEDIATE against current-source
Umbra JNI using `ddm-tck.xml`; results and JUnit reports are in
`out/java-tck/regional-interaction-empty-subscription-sets-java-parity-20261116/`.
Independent Java providers have not been tested.

The opt-in `java-tck.regional-interaction-source-region-snapshot` case queues
an ordinary regional interaction, mutates its source region to in-range
disjoint bounds before callback servicing in evoked mode, and verifies delivery
retains the source-region designator captured at send time. It also confirms
subsequent disjoint sends are filtered and delivery resumes after restoring
overlap. The callback designator is inspected through the receiver's own
RTIambassador, comparing dimension names and bounds instead of region handles
across federates. Its direct mapping matches the paired C++ case:
requirement-candidate-content-clauses-09-data-distribution-management-page-225-l57-17
-> IEEE 1516.1-2025 §9.1.7. The DDM FOM, interaction class, parameter,
dimensions, and callback model are adapter supplied. Focused Umbra JNI runs
passed under HLA_EVOKED and HLA_IMMEDIATE; results/JUnit are in
`.build/java-tck-20261117/source-region-snapshot-{evoked,immediate}.{json,xml}`.
Independent-provider behavior remains unverified.

The opt-in `java-tck.regional-interaction-subscription-filtering` case ports the
standard regional interaction filter sequence: ordinary delivery, empty
regional declarations, overlap and disjoint filtering at callback servicing,
source-region designator delivery, and foreign/uncommitted/context-invalid
region rejection. Its DDM FOM, interaction and parameter names, two interaction
dimensions, and unrelated dimension are adapter supplied; the latter uses
`hla.rti.tck.ddmUnrelatedDimension`. Provider runtime remains unverified.

The default-run `java-tck.regional-interaction-region-validation` case verifies
atomic rejection when a regional interaction send or subscription includes a
region on a dimension unavailable to the interaction class. It proves rejected
operations leave no partial delivery/declaration and that a valid region set
still delivers the expected payload, one source-region designator, and callback
metadata afterward. Its
only direct mapping is the invalid regional subscription requirement in IEEE
1516.1-2025 §9.1.3.3; invalid-send, handle-validity, fixture, and delivery
metadata checks are supplemental. The FOM and interaction, parameter, two valid
dimensions, and unrelated dimension are adapter supplied; the unrelated
dimension uses `hla.rti.tck.ddmUnrelatedDimension`. Java passed under both
callback models with `ddm-tck.xml`, HLAinteractionRoot.TckInteraction/Payload,
TckDimensionX/TckDimensionY, and TckUnrelatedDimension. The paired C++ primary
passed under both callback models using the existing installed-package adapter
executable; no CTest entry is registered for this scenario. Results are in
`out/java-tck/regional-interaction-region-validation-java-parity-20261116/` and
`.build/cpp-tck-regional-request-java-parity-20261116-ninja/`. This is local
provider/package evidence only; independent-provider behavior and conformance
are not claimed.

The default `java-tck.regional-unpublish-region-release` case checks both
region-use paths from the C++ scenario. A region associated with a registered
object's attribute cannot be deleted until the attribute is unpublished; it is
then deleted immediately, and the stale handle is rejected. A second region
used by a passive class-attribute subscription cannot be deleted until
unsubscribe, then is deleted and its stale handle is rejected. Its direct
mapping is limited to §9.4. The adapter supplies `ddm-tck.xml`,
HLAobjectRoot.TckObject/Value, TckDimensionX, TckDimensionY, and callback model.
Focused Java runs passed under both callback models against local Umbra JNI;
the paired C++ direct scenario passed both models using the existing installed
package adapter executable. Java results/JUnit are in
`out/java-tck/regional-unpublish-region-release-java-parity-20261116/`; C++
results/JUnit are in
`.build/cpp-tck-regional-request-java-parity-20261116-ninja/`. Other providers
remain unverified; these runs do not establish conformance.

The opt-in `java-tck.region-lifecycle` case checks DDM services before connect
and before join, timestamped regional-send boundaries, cross-federate dimension
metadata, region range round-trips and committed changes, absent-dimension
errors, invalid range errors, and invalidation after deletion. Its FOM and
object/interaction classes, attribute, parameter, dimensions, logical-time
implementation, and callback model are adapter supplied. `decodeRegionHandle`
is intentionally not represented because the official Java 2025
`RTIambassador` has no corresponding service. The focused scenario passed under
both callback models against local Umbra JNI; other providers remain
unverified. Membership-boundary checks use valid handles created through the
standard API by a joined owner federate. Java has no portable default-invalid
handle representation, so those exact C++ invalid-handle arguments are not
represented.

The opt-in `java-tck.region-range-validation` case accepts a committed range
from zero through the adapter dimension's upper bound, then verifies that
equal and reversed lower/upper bounds fail with `InvalidRangeBound`. The
application FOM, standard MIM, dimension name, logical-time implementation,
and callback model are adapter supplied; provider runtime remains unverified.

The opt-in `java-tck.region-range-dimension-validation` case creates a committed
region containing one adapter dimension and verifies both `getRangeBounds` and
`setRangeBounds` reject a different invalid dimension handle with
`RegionDoesNotContainSpecifiedDimension`. The FOM, standard MIM, dimension
name, logical-time implementation, and callback model are adapter supplied;
provider runtime remains unverified.

The default `java-tck.regional-boundaries` case verifies zero-dimensional
region lifecycle; rejects reads/commits of an incomplete three-dimension
region; rejects object registration and interaction sends using an unrelated
dimension; rejects foreign-region deletion/commit; and checks that passive
subscriptions and registered-object update associations keep regions in use
until unsubscribed or the object is deleted. Direct mappings are limited to
§§9.1.3.3 and 9.4; the incomplete-range, foreign-region, and object-registration
context checks remain supplemental. The adapter supplies `ddm-tck.xml`,
HLAobjectRoot.TckObject/Value, HLAinteractionRoot.TckInteraction/Payload,
TckDimensionX, TckDimensionY, TckUnrelatedDimension, and callback model.
Focused Java runs passed under both callback models against local Umbra JNI;
the paired C++ direct scenario passed both models using the existing installed
package adapter executable. Java results/JUnit are in
`out/java-tck/regional-boundaries-java-parity-20261116/`; C++ results/JUnit are
in `.build/cpp-tck-regional-request-java-parity-20261116-ninja/`. Other
providers remain unverified; these runs do not establish conformance.

The opt-in `java-tck.regional-object-attribute-dimension-inheritance` case uses
the adapter-supplied multi-attribute FOM configured with
`hla.rti.tck.multiAttributeFom`. It checks the base class's X dimension and the
derived class's X/Y dimensions, rejects Y-only regional registration,
subscription, value request, and update association for inherited `BaseValue`,
then verifies X and X/Y receivers get only their matching attribute while a
disjoint receiver gets neither. Focused local JNI runs passed under both
callback models; its direct mappings are limited to §9.1.2 for this fixture.

The opt-in `java-tck.regional-object-attribute-default-region-fdd-dimensions`
case uses the same adapter-supplied multi-attribute FOM. It checks the declared
X/Y upper bounds, rejects a Y-only subscription for inherited `BaseValue`,
registers the derived object without explicit regions, then verifies that
near-upper-bound X and Y subscriptions receive only inherited `BaseValue` and
derived `DerivedValue`, respectively, through FDD default-region routing.
Focused local JNI runs passed under both callback models; its direct mapping is
limited to §9.1.2.

The opt-in
`java-tck.regional-object-attribute-unspecified-dimension-default-range` case
also uses the adapter multi-attribute FOM. It explicitly checks that a
`DerivedValue` source region specifies X only, then registers that attribute
and verifies a Y-only receiver at the FDD default-range upper edge receives it
while the inherited `BaseValue` X path remains independent. Focused local JNI
runs passed under both callback models; the direct mapping is limited to the
omitted-dimension default-range alternative in §9.1.2.

The opt-in `java-tck.regional-object-attribute-excluded-dimension-not-realized`
case uses an adapter FOM where X and Y are available to the object class but
both are marked `Excluded`. It verifies the X-only positive delivery and checks
that a Y-only receiver gets neither discovery nor reflection when an X-only
source region is registered. Its direct mapping is limited to this §9.1.2
realization behavior.

The opt-in `java-tck.regional-subscription-triple-isolation` case uses the
adapter multi-attribute DDM FOM to keep subscriptions independent by class,
region, and attribute. It checks a two-attribute passel at a matching receiver,
per-attribute filtering at a second receiver, repeated and additive regional
subscriptions, empty-region-set unsubscription, and selective/repeated
unsubscription while the other region/attribute triples remain active. Invalid
subscribe and unsubscribe calls use a deleted standard region handle, since
the Java API has no default-invalid `RegionHandle` value. The exact 11 direct
mappings match the paired C++ case. Focused local Umbra JNI runs passed under
both callback models; results/JUnit are in
`.build/java-tck-20261117/regional-subscription-triple-isolation-{evoked,immediate}.{json,xml}`.
The case stays opt-in because it requires the adapter FOM and names.

The opt-in `java-tck.associate-regions-for-updates-replaces-default-region`
case first registers an object without an explicit update association and
confirms its default source reaches both overlapping and disjoint regional
subscribers with empty source-region sets. After associating one explicit
source region, only the overlapping subscriber receives the next update, with
one source-region designator. Its direct mapping is limited to §9.6 p. 230
l. 148-151. Focused local Umbra JNI runs passed under both callback models;
results/JUnit are in
`.build/java-tck-20261117/associate-regions-replaces-default-region-{evoked,immediate}.{json,xml}`.
The case remains opt-in because it requires an adapter DDM FOM and names.

The opt-in `java-tck.register-object-instance-with-regions-atomicity` case
registers once with a committed source region, confirms the overlapping
regional subscriber discovers the object, and services callbacks to confirm
the disjoint subscriber never discovers it. Its direct mapping is limited to
§9.5 p. 228 l. 167-169. Focused local Umbra JNI runs passed under both callback
models; results/JUnit are in
`.build/java-tck-20261117/register-object-instance-with-regions-atomicity-{evoked,immediate}.{json,xml}`.
The case remains opt-in because it requires an adapter DDM FOM and names.

The opt-in `java-tck.register-object-instance-with-regions-omitted-attributes-default-region`
case publishes two attributes, associates a source region only with the first
in the region-qualified registration, and subscribes to the omitted second
attribute through a disjoint region. It confirms discovery and one update with
the expected value, tag, and producer, exercising the default-region path for
that omitted attribute. Its direct mapping is limited to §9.5 p. 228
l. 176-178. Focused local Umbra JNI runs passed under both callback models;
results/JUnit are in
`.build/java-tck-20261117/register-object-instance-with-regions-omitted-attribute-default-region-{evoked,immediate}.{json,xml}`.
The case remains opt-in because it requires an adapter multi-attribute DDM FOM
and names.

The opt-in `java-tck.register-object-instance-with-regions-empty-region-set-default-region`
case pairs the published attribute with an empty region set during
region-qualified registration. A disjoint regional subscriber still receives
one discovery and one update with the expected value, tag, and producer,
exercising the empty-set default-region path. Its direct mapping is limited to
§9.5 p. 229 l. 11. Focused local Umbra JNI runs passed under both callback
models; results/JUnit are in
`.build/java-tck-20261117/register-object-instance-with-regions-empty-region-set-default-region-{evoked,immediate}.{json,xml}`.
The case remains opt-in because it requires an adapter multi-attribute DDM FOM
and names.

The opt-in `java-tck.register-object-instance-with-regions-invalid-region` case
creates and deletes a one-dimensional region, then supplies its stale handle to
`Register Object Instance With Regions` and requires `InvalidRegion`. The Java
API has no portable default-invalid `RegionHandle` value, so this case covers
the deleted-handle form. Its direct mapping is limited to §9.5.3 p. 229
l. 146. Focused local Umbra JNI runs passed under both callback models;
results/JUnit are in
`.build/java-tck-20261117/register-object-instance-with-regions-invalid-region-{evoked,immediate}.{json,xml}`.
The case remains opt-in because it requires an adapter multi-attribute DDM FOM
and names.

The opt-in `java-tck.regional-interaction-partial-default-range` case uses the
adapter restricted default-range FOM. It sends an interaction through a
non-empty X-only source region and verifies the omitted Y dimension receives
the FDD range `[0,5)`: an overlapping `[1,4)` subscriber receives once, while
a `[6,9)` subscriber receives nothing. Focused local JNI runs passed under
both callback models; its direct mappings are limited to §9.1.2.

The opt-in `java-tck.passive-regional-interaction-transition` case starts two
overlapping active regional subscriptions, transitions one to passive and
confirms it remains a delivery target while the other active route exists,
then removes the final active route and confirms both go quiet. An empty
regional subscription does not reactivate the passive route; converting it
back to active restores delivery only to that receiver. Delivered callbacks
are checked for payload, tag, producer, transportation, and source-region
metadata. Its DDM FOM, interaction, parameter, dimensions, and callback model
are adapter supplied; provider runtime remains unverified.

The opt-in `java-tck.default-region-interaction-routing` case checks explicit
regional delivery to an overlapping receiver while an ordinary subscription
still routes around a disjoint regional declaration, then checks delivery after
that declaration is removed. An ordinary send reaches both regional-only and
mixed receivers with present but empty source-region metadata. The case also
checks sender no-loopback before and after sender subscriptions, queries the
callback-scoped source-region copy through the receiving ambassador, and
verifies that disabling region-designator callbacks for one receiver omits only
its region set; re-enabling restores it. Focused local Umbra JNI runs passed
under both callback models with `ddm-tck.xml`; results are in
`.build/java-tck-20261117/default-region-interaction-routing-{evoked,immediate}.{json,xml}`.
The adapter supplies the interaction FOM, interaction, parameter, two
dimensions, and callback model, so default execution remains disabled.

The opt-in `java-tck.zero-dimensional-regional-interaction` case checks both
branches of the paired C++ scenario. With an excluded-dimension interaction
FOM, it checks an ordinary-send baseline and confirms that an explicit
zero-dimensional source region does not overlap the default realization. With
a second FOM whose available Y dimension has default range `[0,5)`, it sends
with an empty source specification and verifies that a Y `[6,9)` subscription
gets no callback while Y `[1,4)` receives once. Both FOMs, the interaction,
parameter, dimensions, provider, and callback model are adapter supplied. Set
`hla.rti.tck.ddmFom` to the excluded-dimension FOM and
`hla.rti.tck.zeroDimensionalDefaultRangeFom` to the restricted-default FOM.
Its two direct mappings are limited to IEEE 1516.1-2025 §9.1.3.1; it does not
generalize excluded-dimension or default-realization behavior beyond those
fixtures. The callback checks also require a positive encoded length for each
`TransportationTypeHandle` before resolving its name. Focused
`--run-unverified` runs passed 1/1 under HLA_EVOKED and HLA_IMMEDIATE with the
local Umbra JNI provider when both FOM properties were supplied explicitly.
Results/JUnit are in
`.build/java-tck-20261117/zero-dimensional-transport-handle-parity-{evoked,immediate}.{json,xml}`;
the traceability export is
`.build/java-tck-20261117/zero-dimensional-transport-handle-parity-traceability.json`.
The paired promoted C++ primary and contract passed both callback models and
validate without findings in the focused evidence slice derived from
`.build/cpp-tck-section-9.1.3.1-20261113/results.json`. The Java case remains
opt-in because its two FOMs and DDM names are adapter supplied; independent-
provider behavior and conformance remain unverified.

The opt-in `java-tck.multi-region-interaction-routing` case sends from two
source regions to separate single-region subscribers, a combined subscriber,
and a disjoint subscriber. It checks union delivery without duplicate callbacks,
strict touching-boundary suppression and interior-overlap delivery, and that an
X-only source does not route to a Y-only subscription under the excluded-
dimension FOM. For each received callback it snapshots source-region dimensions
and bounds through that receiver's own RTIambassador, avoiding cross-federate
region-handle comparisons. Direct mappings are limited to IEEE 1516.1-2025
§§9.1.3.3 and 9.1.4, including the paired C++ scenario's strict-boundary and
no-common-dimension evidence. Supply the excluded-dimension FOM through
`hla.rti.tck.ddmFom`, plus the interaction, parameter, X/Y dimension names,
provider, and callback model through adapter properties. Focused local Umbra JNI
runs passed under HLA_EVOKED and HLA_IMMEDIATE. Results/JUnit are in
`.build/java-tck-20261117/multi-region-{evoked,immediate}.{json,xml}`; independent
provider behavior remains unverified.

The default-run `java-tck.base-subscription-rejects-derived-only-attribute` case
uses the adapter known-class FOM to accept valid base and derived attribute
subscriptions, then reject a derived-only attribute in a base-class subscription.
Its direct mapping is limited to the attribute-set subset rule in IEEE
1516.1-2025 §5.8; `AttributeNotDefined` is supplemental. Focused runs passed
under both callback models on the current-source Umbra JNI provider.

The default-run `java-tck.base-publication-rejects-derived-only-attribute` case
uses the same adapter FOM to accept valid base and derived publications, then
reject a derived-only attribute when publishing the base class. Its direct
mapping is limited to the available-attribute rule in IEEE 1516.1-2025
§5.1.2(a); `AttributeNotDefined` is supplemental. Focused runs passed under both
callback models on the current-source Umbra JNI provider.

The default-run `java-tck.inherited-interaction-class-delivery` case sends a derived
interaction to both a base-class and a derived-class subscription. It verifies
base-subscription projection to the inherited parameter, full inherited and
derived parameter delivery to the derived subscriber, and that a base-class
interaction does not flow to a derived-only subscription. The adapter supplies
the inherited FOM class/parameter names, logical-time implementation, and
callback model; values use standard HLA integer and Unicode data elements.
Absent nonregional source regions may be null or empty. Focused installed-
provider evoked and immediate runs passed; independent-provider conformance
is not claimed.

The default-run `java-tck.interaction-closest-ancestor-transition` case uses the
adapter's three-level interaction FOM. As a receiver subscribes to base,
middle, and exact grandchild classes in sequence, it checks one callback per
send at the closest subscribed class. The grandchild name can be supplied with
`hla.rti.tck.typedGrandchildInteractionClass`; the packaged rich FOM defines
`HLAinteractionRoot.TckTypedInteraction.TckTypedDerivedInteraction.TckTypedGrandchildInteraction`.
The adapter also supplies logical time and callback model. Focused installed-
provider evoked and immediate runs passed; independent-provider conformance is
not claimed.

The opt-in `java-tck.timestamped-inherited-interaction-class-delivery` case
repeats the derived-to-base projection through timestamp-order delivery. It
checks that constrained subscribers do not receive the interaction before
advancing, then verifies both projected callback payloads, timestamp/order and
retraction metadata, and the aligned time-advance grants. The adapter supplies
the inherited FOM names and logical-time implementation; provider runtime
remains unverified.

The opt-in `java-tck.timed-regional-interaction-save-restore` case queues a
timestamped regional interaction beyond a timed federation-save boundary,
retracts it in the live branch, restores the federation, and uses Flush Queue to
verify that the saved interaction and its original retraction handle are live
again. It checks callback payload, source-region metadata, restore identities,
grant ordering, and the post-delivery retraction callback. The adapter supplies
the DDM FOM, names, provider, and logical-time implementation; provider runtime
remains unverified.

The opt-in `java-tck.timed-default-region-interaction-save-restore` case covers
the same saved-queue/retraction lifecycle for an ordinary timestamped send
routed to an adapter-declared regional subscription. After restore and
Flush Queue, it verifies empty source-region metadata, the restored producer
identity and retraction handle, and delivery-before-grant ordering. The adapter
supplies the dimensional FOM, names, provider, and logical-time implementation;
provider runtime remains unverified.

The opt-in `java-tck.timed-default-region-attribute-save-restore` case registers
an object, queues a timestamped default-region attribute update beyond a timed
save frontier, retracts it in the live branch, then restores and flushes the
saved reflection. It verifies restored object/producer identity, payload,
timestamp, empty source-region metadata, the saved/optimistic Flush Queue
frontier, callback-before-grant order, and reuse of the original retraction
handle. The adapter supplies the DDM FOM, names, provider, and logical-time
implementation; provider runtime remains unverified.

The opt-in `java-tck.timed-regional-attribute-ownership-save-restore` case
restores a queued timestamped update and continues the surviving regular
ownership candidate. It checks regional discovery and source metadata, save
and restore identity, the evoked candidate continuation and immediate
If-Available boundary, Flush Queue delivery-before-grant, and the resulting
ownership transfer. The adapter supplies the dimensional object FOM, object
class/attribute/dimension names, provider, and logical-time implementation;
provider runtime remains unverified.

The opt-in `java-tck.timed-regional-attribute-ownership-confirmation-cancel-save-restore`
case uses the same restored multi-recipient timed update, then verifies that
canceling the owner's negotiated divestiture after its confirmation request
does not transfer ownership, a subsequent confirmation is rejected, and the
remaining acquirer's cancellation callback reports the correct object and
attributes. In immediate-callback mode it exercises the standard
If-Available/If-Wanted path instead. Provider runtime remains unverified.

The opt-in `java-tck.timed-regional-attribute-ownership-pre-delivery-cancel-save-restore`
case cancels the negotiated divestiture before servicing its confirmation
callback. It verifies ownership is unchanged, neither stale confirmation nor
acquisition callback is delivered, and cancellation of the remaining request
reports the expected object and attributes. Immediate-callback mode follows
the standard If-Available/If-Wanted boundary instead. Provider runtime remains
unverified.

`java-tck.dimension-lookup-lifecycle` checks dimension handle/name round-trips,
the exact FDD-declared upper bound of 10 for `TckDimensionX` and
`TckDimensionY`, and their object/interaction class associations across the
owner and joining member. It also checks dimension-name lookup before connect,
before join, and after resignation using valid inputs. The focused case passed
under both callback models on the current-source Umbra JNI provider using
`fom-model-tck.xml` and `HLAinteger64Time`. Its direct mapping is limited to
Get Dimension Upper Bound at §10.25; RL-442 records the Requirements Lab's
crosswalk to §10.24.6, which is not claimed. C++ default-invalid handle probes
are omitted because Java has no standard default-invalid dimension or class
handle. The test creates no regions and calls no DDM region services. This is
local-provider evidence, not independent-provider interoperability or
conformance evidence.

`java-tck.handle-normalization-membership-boundaries` checks service-group,
federate, object-class, interaction-class, and object-instance normalization
before connection, while connected but unjoined, and after the probe joins and
resigns. It uses the regular typed TCK FOM, registers one object, and passed the
focused current-source Umbra JNI runner under both callback models. Its five
requirement IDs are internal scenario references; no direct normative mapping
is claimed. This is local-provider evidence, not independent-provider
interoperability or conformance evidence.

The opt-in `java-tck.timestamped-interaction-source-resignation-fanout` case
queues one timestamped interaction for two constrained recipients, advances one
with TAR and the other with NMR, resigns the producer, and verifies independent
delivery, payload/source/order/retraction metadata, callback-before-grant order,
and each recipient's grant time. Provider, time-managed FOM, logical-time
implementation, and callback model remain adapter-supplied; provider runtime is
unverified.

The opt-in `java-tck.joined-federate-mom-added-attribute-same-rti-instance`
case discovers the target federate's RTI-registered `HLAfederate` MOM object,
verifies a FOM-added attribute is not RTI-owned, acquires and updates it on that
same object, and checks a third federate's reflection, including value, tag,
class, and object-name identity. The extension FOM, standard MIM, logical-time
implementation, and callback model are adapter-supplied. Both focused local
Umbra JNI runs fail at the pre-acquisition ownership query because the provider
reports `attributeIsOwnedByRTI`; the matching installed-package C++ primary and
contract pass under both callback models. Other-provider behavior remains
unverified.

The default-run `java-tck.federation-save-timestamp-eligibility` case checks
that a timestamped save request is rejected with `FederateUnableToUseTime`
before time regulation, with no save callbacks, then accepted after enabling
regulation with the requested label and time. It maps the two direct
IEEE 1516.1-2025 §4.19 requirement rows for Request Federation Save. Java runs
passed in evoked and immediate modes; the paired installed-package C++ selectors
passed focused CTest and direct runs in both modes. These are local-provider
results, not independent-provider or conformance evidence.

The default-run `java-tck.federation-save-request-replacement` case issues two
timestamped save requests before either member advances. Both members receive
their first grants at the superseded save time without initiating a save; at
the replacement time, each receives one initiation with the replacement label
and time. Both members then complete the save successfully. It maps the direct
IEEE 1516.1-2025 §4.19 requirement for replacing an outstanding save request.
Java evoked and immediate runs passed, as did the paired installed-package C++
selectors. These are local-provider results, not independent-provider or
conformance evidence.

The default-run `java-tck.save-restore-queued-ro-interaction` case leaves one
receive-order interaction pending while both members save. After federation
destruction/recreation and public restore completion, the evoked subscriber
receives the interaction exactly once with its original parameter and tag and
the restored publisher identity. It maps the direct IEEE 1516.1-2025 §4.1.2
requirement for preservation of queued messages through federation save. The
focused Java HLA_EVOKED run and paired installed-package C++ consumer each
passed 1/1. This case does not cover HLA_IMMEDIATE, timestamp-order delivery,
attribute updates, all queued-message forms, independent-provider behavior, or
conformance.

The default-run `java-tck.restore-federate-type-count-mismatch` case saves two
members with the same federate type, then resigns and rejoins one member under a
different type while keeping the current total at two. The other member requests
restore and receives one failure callback for that save; neither member receives
Federation Restore Begun or Initiate Federate Restore. It maps the direct IEEE
1516.1-2025 §4.1.2 requirement for federate-type counts to match saved state.
Java evoked and immediate runs passed, as did the paired installed-package C++
selectors. This is one mismatch witness, not exhaustive restore-state
verification or conformance evidence.

The default-run `java-tck.restore-federate-type-identity-rebinding` case saves
two members of the same type, resigns both, and joins two differently named
replacements under that same type. After restore is requested, it checks that
each Initiate Federate Restore callback reports the saved name and the
replacement member's current handle, then completes restore for both members.
It maps the direct IEEE 1516.1-2025 §4.1.2 identity-rebinding requirement.
Java evoked and immediate runs passed, as did the paired installed-package C++
selectors. This is a bounded same-type witness, not exhaustive restore-state or
membership-permutation verification, independent-provider behavior, or
conformance.

The default-run `java-tck.fom-interaction-qualified-class-name-identity` loads the
adapter's `fom-model-tck.xml` and resolves the two
`TckClassNameProbe` interactions beneath different parents. It verifies that
their handles differ and that each handle returns its complete qualified name.
The Java case uses the same named Join overload as the paired C++ scenario.
Current-source JNI runs passed 1/1 under each callback model against the pinned
Java API and HLAinteger64Time. Results/JUnit are in
`out/java-tck/fom-interaction-qualified-class-name-identity-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
Direct mappings are IEEE 1516.2-2025 §§4.3.2, 5.3, and 6.2.3. This is a
qualified-identity witness only; it does not cover valid-name conventions,
malformed-name rejection, exhaustive §3.3 coverage, independent-provider
behavior, or conformance.

The default-run `java-tck.fom-duplicate-object-class-attribute-name` supplies
`invalid-duplicate-tck.xml` as the sole FOM module and checks that federation
creation rejects the repeated `DuplicateValue` attribute name with a standard
FOM error. It maps IEEE 1516.2-2025 §4.5.2. Current-source JNI runs passed
1/1 under each callback model against the pinned official Java API and
HLAinteger64Time. Results/JUnit are in
`out/java-tck/fom-duplicate-object-class-attribute-name-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
The paired C++ case passed both callback-model selectors with the same fixture.
This is one same-class duplicate-name witness; inherited-name collisions,
naming conventions, the table's NA entry rule, independent-provider behavior,
and conformance remain outside its scope.

The default-run `java-tck.fom-basic-data-representation-endian-values` creates and
joins one federation for each adapter-supplied basicData FOM declaring
`Endian=Big` and `Endian=Little`, then resigns and destroys both executions.
It uses the same single-module Create and named Join path as the paired C++
case, and maps IEEE 1516.2-2025 clauses 4 and 6.2.14. Current-source JNI runs
passed 1/1 under each callback model with HLAinteger64Time and the pinned Java
API. Supply the fixtures through
`hla.rti.tck.basicDataRepresentationEndianBigFom` and
`hla.rti.tck.basicDataRepresentationEndianLittleFom`. Results/JUnit are in
`out/java-tck/fom-basic-data-representation-endian-values-java-parity-20261115/{evoked,immediate}.{results.json,junit.xml}`.
This confirms acceptance of those two FOM values only; it does not test payload
byte order, other data representations, independent-provider behavior, or
conformance.

The opt-in `java-tck.federate-selected-update-thresholds` uses two publishers
that send an attribute at application-selected sample thresholds of two and
three. The receiver verifies exactly five reflections, the sample values,
threshold/sample tags, object identities, and producing federates. It maps
Summary Rule 9 and Detailed Rule 9 (§6.4). Java evoked and immediate runs
passed, as did the paired installed-package C++ selectors. The RTI does not
evaluate these application thresholds; SOM documentation and conformance are
outside the case.

The adapter-required `java-tck.transport-public-new-session-interoperability`
uses `tools/run_java_tck_transport_interop.py` to run the Java public
`RTIambassador.connect(..., RtiConfiguration)` and `disconnect` calls against
the independent literal-frame loopback peer. Java checks that the configured
heartbeat settings were applied; the peer checks the New Session request and
status, federate-originated heartbeat sequence and response correlation, and
Close/Acknowledgement/EOF ordering. Both callback models are exercised. Its
direct mappings cover IEEE 1516.1-2025 §§12.13.4.5, 12.13.4.5.1,
12.13.4.5.2, 12.13.4.5.3, and 13.2.
This is bounded Java API and wire-protocol evidence, not complete provider
interoperability or conformance.

The default-run java-tck.federate-asynchronous-delivery-mim-attribute case checks the MIM-derived initial HLAasynchronousDelivery value, one opposite-state transition, a direct request, conditional reflection metadata, and restoration. Its direct mappings cover IEEE 1516.1-2025 §§5.8, 6.9.3, 6.21, 8.15–8.16, and 11.4.1. A non-null producer handle must be rejected by getFederateName with InvalidFederateHandle; null represents no producer. Java and the paired current-source C++ primary/contract passed both callback models. Results are in out/java-tck/federate-asynchronous-delivery-mim-attribute-java-parity-20261116/{evoked,immediate}/ and out/mgw/cpp-tck-federate-asynchronous-delivery-mim-attribute-java-parity-20261116-pair.{json,xml}.

The default-run `java-tck.timestamped-local-delete-attribute` and
`java-tck.timestamped-local-delete-object` cases verify that local deletion
suppresses a queued timestamped reflection or removal for one recipient while
the surviving recipient receives the expected event and metadata. Each Java
case passed under both callback models against the current-source JNI provider;
the paired C++ primary/contract selectors passed focused CTest and direct runs
in both models. Their `cpp-1516.1-*` references are repository-internal IDs and
make no direct IEEE section claim. The two default-run
`java-tck.timestamped-delete-privilege-*` cases check the RECEIVE-preferred and
TIMESTAMP-preferred removal branches, including sent/received order, message
metadata, and retraction behavior. Both Java cases passed under both callback
models, and the paired C++ primary/contract selectors passed 8/8 focused CTests
and 8/8 direct runs. Their direct mapping remains limited to the exercised
§8.1.4 branches. Results are in
`out/java-tck/timestamped-local-delete-scenarios-java-parity-20261116/` and
`.build/cpp-tck-evidence-20261116/{timestamped-local-delete-attribute,timestamped-local-delete-object,timestamped-delete-privilege}-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.regional-attribute-value-request-filtering` case
checks regional Request Attribute Value Update failures for a foreign,
unrelated-dimension, or uncommitted region; suppression for an empty region
set; default-region selection when the explicit request region is disjoint;
and both regional and default objects when it overlaps. It preserves the
requested attributes and tag. Under HLA_EVOKED it re-evaluates queued work
after the request region moves; under HLA_IMMEDIATE it delivers before the
move and does not replay. The two direct mappings are limited to IEEE
1516.1-2025 §9.1.1 and cover committed-region use and deletion. Java passed
under both callback models with `ddm-tck.xml`, HLAobjectRoot.TckObject,
Value, TckDimensionX, TckDimensionY, and TckUnrelatedDimension. The paired C++
primary/contract selectors passed 4/4 focused CTests and 4/4 direct runs.
Results are in
`out/java-tck/regional-attribute-value-request-filtering-java-parity-20261116/{evoked,immediate}/{results-promoted.json,junit-promoted.xml}`
and
`out/cpp-tck-evidence-20261116/regional-attribute-value-request-filtering-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.object-class-abstraction-subscription` case verifies
that a derived registration is discovered through a base-class subscription
with the subscriber's class view, and that the inherited `Identity` value is
reflected once. Its direct mappings remain limited to IEEE 1516.2-2025 §4 and
the selected IEEE 1516.1-2025 §6.1.1 requirements. Java passed under both
callback models with `fom-model-tck.xml` and HLAinteger64Time; the installed-
package C++ primary/contract selectors passed 4/4 focused CTests and 4/4 direct
runs. Results are in
`out/java-tck/object-class-abstraction-subscription-java-parity-20261116/{evoked,immediate}/`
and
`out/cpp-tck-evidence-20261116/object-class-abstraction-subscription-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.attribute-relevance-known-class` and
`java-tck.attribute-relevance-known-class-disabled` cases exercise the same
inherited class hierarchy with the `Advisories Use Known Class` FOM switch on
and off. Both verify discovery through the closest subscribed superclass,
cross-member class and attribute handle identity, and the expected base/derived
attribute advisory sequences. They also check that disabling Attribute
Relevance Advisory suppresses later update-on and update-off callbacks. Only
the superclass discovery assertion maps directly to IEEE 1516.1-2025 §5.8;
the policy and advisory assertions remain supplemental. Both Java cases passed
under HLA_EVOKED and HLA_IMMEDIATE. Their paired C++ primary/contract cases
passed 8/8 focused CTests and 8/8 direct runs across both policies and callback
models. Results are in
`out/java-tck/attribute-relevance-known-class-java-parity-20261116/{evoked,immediate}/`,
`out/java-tck/attribute-relevance-known-class-disabled-java-parity-20261116/{evoked,immediate}/`,
and `out/cpp-tck-evidence-20261116/attribute-relevance-known-class-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.mixed-order-attribute-passel-retraction` case checks
that the receive-order attribute from one mixed-order update is delivered once
before `Retract` and remains delivered once after retraction, while the queued
timestamped attribute stays suppressed through the receiver's grant. Its direct
mapping remains limited to §8.2. The default-run
`java-tck.mixed-order-attribute-passels-separate-messages` case checks that one
update containing both attributes is delivered as two attribute-only reflection
messages with their respective receive and timestamp orders. Its five direct
mappings remain in §§6.1.12 and 8.1.3. Both Java cases passed under both callback
models with `ddm-multi-attribute-tck.xml` and `HLAinteger64Time`; each paired C++
primary/contract selector passed 4/4 focused CTests and direct runs. Results are
in `out/java-tck/mixed-order-attribute-passel-retraction-java-parity-20261116/{evoked,immediate}/`,
`out/java-tck/mixed-order-attribute-passels-separate-messages-java-parity-20261116/{evoked,immediate}/`,
and `.build/cpp-tck-evidence-20261116/{mixed-order-attribute-passel-retraction,mixed-order-attribute-passels-separate-messages}-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.ownership-state-save-restore` case verifies that a completed negotiated ownership transfer survives a successful federation save and restore, including post-restore federate handles and the ownership query. It uses the explicit federate-name join overload so the restore callback identities match the joined owner and acquirer. Its `cpp-1516.1-*` identifiers are repository-internal and have no direct IEEE clause mapping. Java passed under both callback models; the paired C++ primary/contract selectors passed 4/4 focused CTests and direct runs. Results are in `out/java-tck/ownership-state-save-restore-java-parity-20261116/{evoked,immediate}/` and `out/cpp-tck-evidence-20261116/ownership-state-save-restore-java-parity-20261116.{json,xml}`. This is local-provider evidence only; independent-provider behavior and conformance are not claimed.

The default-run `java-tck.pending-negotiated-ownership-save-restore` case preserves a pending negotiated transfer through federation save and restore. HLA_EVOKED uses Attribute Ownership Acquisition If Available; HLA_IMMEDIATE uses regular acquisition with callbacks deferred until after restore. Both paths verify the pending owner/acquirer state, post-restore confirmation, transferred ownership, and ownership query. The `cpp-1516.1-*` references are internal IDs with no direct IEEE clause mapping. Java passed under both callback models; the paired C++ primary/contract selectors passed 4/4 focused CTests and direct runs. Results are in `out/java-tck/pending-negotiated-ownership-save-restore-java-parity-20261116/{evoked,immediate}/` and `out/cpp-tck-evidence-20261116/pending-negotiated-ownership-save-restore-java-parity-20261116.{json,xml}`. This is local-provider evidence only; independent-provider behavior and conformance are not claimed.

The default-run `java-tck.ownership-acquisition-cancellation-save-restore` case preserves a pending acquisition cancellation across federation save and restore. It checks one post-restore cancellation confirmation, no acquisition notification, preserved ownership, restored handles, and the ownership query. Its repository-internal `cpp-1516.1-*` IDs have no direct IEEE clause mapping. Java passed under both callback models; the paired C++ primary/contract selectors passed 4/4 focused CTests and direct runs. Results are in `out/java-tck/ownership-acquisition-cancellation-save-restore-java-parity-20261116/{evoked,immediate}/` and `out/cpp-tck-evidence-20261116/ownership-acquisition-cancellation-save-restore-java-parity-20261116.{json,xml}`. This is local-provider evidence only; independent-provider behavior and conformance are not claimed.

The default-run `java-tck.if-available-ownership-restore-unavailable-callback` case preserves a pending If Available request across federation save and restore, then checks one unavailable callback with the original tag, preserved ownership, restored handles, and the ownership query. Its repository-internal `cpp-1516.1-*` IDs have no direct IEEE clause mapping. Java passed under both callback models; the paired C++ primary/contract selectors passed 4/4 focused CTests and 4/4 direct runs. Results are in `out/java-tck/if-available-ownership-restore-unavailable-java-parity-20261116/{evoked,immediate}/` and `out/cpp-tck-evidence-20261116/if-available-ownership-restore-unavailable-java-parity-20261116.{json,xml}`. This is local-provider evidence only; independent-provider behavior and conformance are not claimed.

The default-run `java-tck.ownership-cancellation-idle` case checks the exact
tag on the owner's release request, one cancellation confirmation, no acquisition
notification, and retained ownership. In `HLA_IMMEDIATE`, the requester makes no
RTI call between cancellation and callback observation; `HLA_EVOKED` services its
callback queue. Its direct mappings cover IEEE 1516.1-2025 §§7.8, 7.15, and 7.16.
The Java cancellation-confirmation method has no tag parameter, so the C++-only
empty confirmation-tag assertion has no Java-visible counterpart. Java passed
under both callback models; the paired C++ primary/contract selectors passed 4/4
focused CTests and 4/4 direct runs. Results are in
`out/java-tck/ownership-cancellation-idle-java-parity-20261116/{evoked,immediate}/`
and `out/cpp-tck-evidence-20261116/ownership-cancellation-idle-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.attribute-scope-ownership-acquisition-no-out-of-scope`
case verifies that the acquiring federate receives the ownership notification
and owns the attribute, while that same acquisition generates no
`attributesOutOfScope` callback during the 10-second observation window. Its
direct mappings are limited to the acting-acquirer and no-notification
requirements in IEEE 1516.1-2025 §10.1.2.1. Java passed under HLA_EVOKED and
HLA_IMMEDIATE with the DDM TCK FOM and HLAinteger64Time. The paired installed-
package C++ primary passed 2/2 focused CTests and 2/2 direct runs across both
callback models. Results are in
`out/java-tck/attribute-scope-ownership-acquisition-no-out-of-scope-java-parity-20261116/{evoked,immediate}/`
and
`out/cpp-tck-evidence-20261116/attribute-scope-ownership-acquisition-no-out-of-scope-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.per-federate-discovered-class` case verifies one
discovery per subscriber with the class view selected by that federate's
subscription. `java-tck.registered-object-class-immutability` then changes both
subscriber class views and confirms each discovered and registered class stays
stable. Both cases round-trip each member's own object handle from the shared
object name; they never compare opaque handles across ambassadors. Their direct
mappings remain limited to the selected IEEE 1516.1-2025 §5.1.3 requirements.
Java passed both cases under HLA_EVOKED and HLA_IMMEDIATE with the known-class
hierarchy FOM and HLAinteger64Time. The installed-package C++ primary/contract
selectors passed 8/8 focused CTests and 8/8 direct runs across both callback
models. Results are in
`out/java-tck/per-federate-discovered-class-java-parity-20261116/{evoked,immediate}/`,
`out/java-tck/registered-object-class-immutability-java-parity-20261116/{evoked,immediate}/`,
and `out/cpp-tck-evidence-20261116/object-class-view-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.discovered-class-stability-after-specific-subscription`
case records a discovery through a base-class subscription, adds the more-
specific subscription, and confirms the discovered class stays stable with no
duplicate callback. Its direct mapping is limited to the selected
IEEE 1516.1-2025 §5.1.3 requirement. Java passed under both callback models;
the installed-package C++ primary/contract selectors passed 4/4 focused CTests
and 4/4 direct runs. Results are in
`out/java-tck/discovered-class-stability-after-specific-subscription-java-parity-20261116/{evoked,immediate}/`
and
`out/cpp-tck-evidence-20261116/discovered-class-stability-after-specific-subscription-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.attribute-scope-advisories` case verifies grouped
in-scope and out-of-scope callbacks as update regions move, no advisory when a
member removes its final regional subscription, switch suppression, and stale
in-scope callback cancellation after a region change. Its sole direct mapping
is the final self-unsubscription rule in IEEE 1516.1-2025 §5.9; the other
transitions and callback details remain scenario assertions. Java passed under
both callback models with `ddm-tck.xml`, HLAobjectRoot.TckObject, Value,
TckDimensionX, and TckDimensionY. The installed-package C++ primary/contract
selectors passed 4/4 focused CTests and 4/4 direct runs. Results are in
`out/java-tck/attribute-scope-advisories-java-parity-20261116/{evoked,immediate}/`
and
`out/cpp-tck-evidence-20261116/attribute-scope-advisories-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.default-region-object-routing` case verifies the
default source region, explicit source associations, regional callback
designators, and the independent ordinary and regional subscription routes.
It queries dimension metadata and checks each callback source region through
the receiving ambassador, without comparing opaque handles across members.
Each reflection also checks that its transportation handle has a positive
encoded length, matching the C++ validity assertion.
Its 15 direct requirement mappings are limited to IEEE 1516.1-2025 §§9.1.1,
9.1.3.2, 9.1.5, and 9.8; the full subscription-tuple matrix is not claimed.
Java passed under HLA_EVOKED and HLA_IMMEDIATE with `ddm-tck.xml`,
HLAobjectRoot.TckObject, Value, TckDimensionX, and TckDimensionY. The
installed-package C++ primary/contract selectors passed 4/4 focused CTests
and 4/4 direct runs. Results are in
`.build/java-tck-20261117/default-region-object-routing-transportation-{evoked,immediate}.{json,xml}`
and
`out/cpp-tck-evidence-20261116/default-region-object-routing-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.default-region-registration-names` case registers
three objects through regional registration with an empty region set, regional
registration with an empty pair list, and ordinary registration. It checks
unique generated names and name/handle round trips on both members, discovery
and known class per receiver-local handle, default-source reflection values,
tags, producer name, and an empty region-designator set. Its two direct
mappings are limited to IEEE 1516.1-2025 §9.1.6. Java passed under
HLA_EVOKED and HLA_IMMEDIATE with `ddm-tck.xml`, HLAobjectRoot.TckObject,
Value, TckDimensionX, and TckDimensionY. The Java callback also verifies that
the returned transportation handle has a positive encoded length, matching the
C++ valid-handle assertion. Latest results/JUnit are in
`.build/java-tck-20261117/default-region-registration-names-{evoked,immediate}.{json,xml}`.
The installed-package C++
primary/contract selectors passed 4/4 focused CTests and 4/4 direct runs.
Results are in
`out/java-tck/default-region-registration-names-java-parity-20261116/{evoked,immediate}/{results-promoted.json,junit-promoted.xml}`
and
`out/cpp-tck-evidence-20261116/default-region-registration-names-java-parity-20261116.{json,xml}`.
This is local-provider evidence only; independent-provider behavior and
conformance are not claimed.

The default-run `java-tck.regional-object-attribute-dimension-subset` case
checks the adapter's object and interaction dimension names, availability, and
bounds; rejects registration and later update association when a region includes
a dimension unavailable to the object attribute; and confirms that registration
with the available dimensions succeeds. Its only direct mapping is IEEE
1516.1-2025 §9.1.3.2; fixture and handle-validity checks are supplemental. Java
passed under HLA_EVOKED and HLA_IMMEDIATE with `ddm-tck.xml`,
HLAobjectRoot.TckObject/Value, HLAinteractionRoot.TckInteraction,
TckDimensionX/TckDimensionY, and TckUnrelatedDimension. The paired C++ direct
scenario passed under both callback models using the existing installed-package
adapter executable; no CTest entry is registered for this scenario. Results are
in `out/java-tck/regional-object-attribute-dimension-subset-java-parity-20261116/`
and `.build/cpp-tck-regional-request-java-parity-20261116-ninja/`. This is
local-provider/package evidence only; independent-provider behavior and
conformance are not claimed.
