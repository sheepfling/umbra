# Java RTI conformance TCK

`packages/hla-rti-java-tck` is a vendor-neutral executable test set for an
IEEE 1516.1-2025 Java RTI provider. Its Java sources import only JDK classes
and the official `hla.rti1516_2025` API. Provider selection happens outside
the test package:

```text
official API JAR + provider JAR(s) + factory name + FOM/MIM inputs
```

The compiled classes can therefore be copied to another product and run with
that product's API/provider JARs. Provider-specific JVM or native-loader flags
are supplied as launcher arguments and are never referenced by the test
sources.

## Scope

The catalog contains 149 scenarios:

- factory discovery, standard encoding, malformed input, and API/exception
  checks;
- complete federation execution create, join, query, resign, destroy, and
  disconnect lifecycle;
- ordinary object-class declaration, object registration/discovery, attribute
  update/reflection, interaction publication/delivery, callback control,
  support lookups, negative edges, relevance advisories, directed
  interactions, and transportation/order controls; and
- capability-profile lanes for DDM, time advance, save/restore, ownership,
  synchronization, and MOM.

All scenarios remain in the evidence output. A provider that does not expose a
required capability records `unsupported` or `not applicable`; the scenario is
not silently removed.

The ordinary scenarios use the IEEE Restaurant example names by default,
including `HLAobjectRoot.Employee.Server`/`Efficiency`,
`HLAobjectRoot.Food.Drink`/`NumberCups`, and
`HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed`.
Directed-interaction and relevance checks default to
`HLAinteractionRoot.ServerAction.TakeOrder`. Supply a compatible FOM or
override names with the `hla.rti.tck.*` system properties.

The `java-tck.federate-lookup-lifecycle` scenario covers lookup preconditions
before connection and before joining, round-trip member handle/name lookups,
unknown-name and invalid/foreign-handle failures, and the distinct post-resign
behavior: the departed name is no longer resolvable, while the old handle still
returns its designator name. Federation and participant names are unique per
run; the provider, FOM, and callback model are adapter inputs.

The `java-tck.callback-controls-transportation` scenario checks standard
attribute and interaction transportation reports and requested-change
confirmations across callback disable/enable. It verifies callback withholding,
release after enabling, and the committed transportation types.

The `java-tck.callback-controls-directed-interaction` scenario checks baseline
directed delivery, callback suppression while disabled, and exactly-once
delivery after re-enabling. It verifies target object, interaction and parameter
handles, payload, tag, reliable transportation, and producer through standard
Java callbacks. Provider, FOM, names, and callback model are adapter inputs.

The `java-tck.callback-controls-federation-reports` scenario checks callback
gating for federation execution lists, exact member lists, and
federation-does-not-exist reports. It validates federation/time-implementation
and member name/type data after callbacks are re-enabled.

The `java-tck.callback-controls-declaration-advisories` scenario gates start/
stop registration and interaction turn-on/turn-off callbacks while changing
ordinary active publication/subscription relevance. Each advisory is checked
for suppression while callbacks are disabled and correct handle identity after
re-enabling; the case does not use DDM regions.

The `java-tck.callback-controls-attribute-relevance-advisories` scenario gates
unnamed and named per-object turn-updates-on/off advisories. It checks object
and attribute handles, the update-rate designator, standard update-rate
metadata, and suppression/release behavior. The adapter FOM supplies distinct
name, rate, and configured test attribute names. Override the defaults with
`-Dhla.rti.tck.relevanceNameAttribute`,
`-Dhla.rti.tck.relevanceRateAttribute`, and
`-Dhla.rti.tck.updateRateName`; the scenario does not use DDM regions.

The `java-tck.ordinary-edges` scenario uses the configured object, interaction,
attribute, and parameter names. Its inheritance checks also need an adapter FOM
parent object class, inherited attribute, and parent interaction class; override
their defaults with `-Dhla.rti.tck.ordinaryEdgesParentObjectClass`,
`-Dhla.rti.tck.ordinaryEdgesInheritedAttribute`, and
`-Dhla.rti.tck.ordinaryEdgesParentInteractionClass`. The unrelated-class check
uses the existing `supportObjectClass` and `supportAttribute` adapter settings.

The `java-tck.object-class-lookup-lifecycle` scenario verifies that an
additional-module object class is absent before the adapter-supplied module
joins and becomes visible afterward. Its class name defaults to
`HLAobjectRoot.TckExtensionObject`; override it with
`-Dhla.rti.tck.objectClassLookupExtensionObjectClass` through
`--jvm-argument` to match the adapter's module.

The `java-tck.interaction-class-lookup-lifecycle` scenario applies the same
additional-module check to interaction classes. Its extension interaction
class defaults to `HLAinteractionRoot.TckExtensionInteraction`; set
`-Dhla.rti.tck.interactionLookupExtensionInteractionClass` through
`--jvm-argument` when the adapter's module uses a different name.

The `java-tck.relevance-advisories` scenario uses the adapter's object-class
name, primary attribute name, interaction-class name, callback model, and FOM.
Its FOM must also supply distinct names for the advisory fixture's name and
rate attributes; override them with `-Dhla.rti.tck.relevanceNameAttribute`
and `-Dhla.rti.tck.relevanceRateAttribute` through `--jvm-argument` when the
FOM uses different names. The adapter can supply the named update-rate
designator with `-Dhla.rti.tck.updateRateName`; this also configures the
ordinary attribute/interaction scenario.

The `java-tck.callback-controls-attribute-value-request` scenario requests the
configured attribute on a discovered instance while the owner has callbacks
disabled, checks that no `provideAttributeValueUpdate` callback escapes, then
re-enables callbacks and verifies exactly one callback preserves the object,
attribute set, and request tag. The provider, FOM, object-class and attribute
names, logical-time implementation, and callback model are adapter inputs.

The `java-tck.callback-controls-auto-provide` scenario uses the adapter's
Auto Provide FOM and configured object/attribute names. It verifies that the
standard switch is enabled, gates the registration-triggered
`provideAttributeValueUpdate` callback while disabled, and checks the callback's
object, exact published attribute set, and empty tag after callbacks are enabled.
Configure it with `-Dhla.rti.tck.autoProvideFom=...`,
`-Dhla.rti.tck.autoProvideObjectClass=...`,
`-Dhla.rti.tck.autoProvideFirstAttribute=...`, and
`-Dhla.rti.tck.autoProvideSecondAttribute=...`; the FOM must enable the standard
Auto Provide switch.

The `java-tck.callback-controls-ownership-assumption` scenario gates the
standard `requestAttributeOwnershipAssumption` callback across unconditional
divestiture on owner resignation. It then verifies the callback's object,
attribute set, and empty tag; acquires the attribute with an explicit tag; and
checks both the acquisition notification and standard ownership query.

The `java-tck.callback-controls-ownership-unavailable` scenario requests
if-available ownership while callbacks are disabled. After callbacks are
enabled, it verifies the unavailable callback's object, attribute set, and
request tag, then confirms through standard ownership queries that the original
owner still owns the attribute and the requester does not.

The `java-tck.callback-controls-ownership-release-request` scenario requests
ownership while the owner's callbacks are disabled, then checks the released
`requestAttributeOwnershipRelease` callback's object, attributes, and acquisition
tag. It divests the wanted attributes through the standard service, verifies the
returned divestiture set, and checks the requester's acquisition callback and
final ownership query.

The `java-tck.callback-controls-ownership-acquisition-cancellation` scenario
first observes the owner's standard release-request callback, then cancels the
pending acquisition with requester callbacks disabled. It verifies unchanged
ownership both before and after callback release, and checks the cancellation
completion's object and attribute set (the standard callback has no tag).

The `java-tck.callback-controls-ownership-release-denied` scenario requests
ownership, observes the owner's release request, and denies it with a distinct
tag while requester callbacks are disabled. After re-enabling, it verifies the
standard ownership-unavailable callback's object, attributes, denial tag, and
that ownership remains with the original owner.

The `java-tck.callback-controls-ownership-acquisition-notification` scenario
divests an attribute with `attributeOwnershipDivestitureIfWanted` while the
requester's callbacks are disabled. It verifies the service's returned attribute
set, then enables callbacks and checks the acquisition notification's object,
attribute set, tag, and resulting ownership.

The `java-tck.callback-controls-ownership-divestiture-confirmation` scenario
gates negotiated-divestiture confirmation while owner callbacks are disabled,
then verifies the confirmation payload and unchanged ownership. It cancels the
divestiture, checks the restored release-request payload, cancels the requester's
acquisition, and verifies the cancellation callback and final ownership state.

The `java-tck.callback-controls-ownership-query` scenario gates ownership-query
results for a published-but-partially-owned object, then checks the exact owned
and unowned attribute partitions. It also queries the adapter-configured standard
MOM `HLAfederateHandle` attribute and verifies the separate RTI-owned callback.
Configure it with `hla.rti.tck.additionalFom`, `hla.rti.tck.mim`,
`hla.rti.tck.timeImplementation`, and the `hla.rti.tck.multiAttribute*` name
properties; all are adapter inputs.

The `java-tck.callback-controls-object-removal` scenario first establishes normal
deletion delivery, then repeats removal while receiver callbacks are disabled.
After re-enabling, it checks the exact removed object, tag, and producing
federate handle.

The `java-tck.callback-controls-object-discovery` scenario registers a subscribed
object while receiver callbacks are disabled, verifies that discovery is withheld,
then checks the discovered object's handle, class, instance name, and producing
federate after re-enabling.

The `java-tck.callback-controls-object-name-reservation` scenario reserves an
adapter-derived unique name with callbacks disabled, checks that success is
withheld, then verifies the exact success callback after re-enabling and releases
the reservation through the standard service.

The `java-tck.callback-controls-object-name-reservation-failure` scenario has
one federate reserve a name, then gates the failure callback produced when a
second federate requests that contended name. It confirms the exact failure
result after callbacks are re-enabled and releases the owner's reservation.

The `java-tck.callback-controls-multiple-object-name-reservation` scenario first
gates a successful two-name reservation, then gates a second federate's mixed
request and checks the exact successful and failed name sets after re-enabling.
It releases each successfully reserved name through the standard services.

The `java-tck.callback-controls-flush-queue-grant` scenario sends a timestamped
interaction, requests Flush Queue, and advances the publisher while receiver
callbacks are disabled. It verifies that both the queued interaction and Flush
Queue grant remain withheld, then checks their order, payload, tag, producer,
timestamp, retraction handle, and grant-time bounds after callbacks are enabled.
FOM, logical-time implementation, interaction/parameter names, and callback
model are adapter inputs; provider runtime behavior remains unverified until run
against an installed provider.

The `java-tck.callback-controls-available-time-advance` scenario independently
gates both `timeAdvanceRequestAvailable` at the inclusive message-time boundary
and `nextMessageRequestAvailable` beyond the message time. Each path verifies
withheld callbacks, then checks the interaction-before-grant order, exact
timestamped interaction metadata and retraction handle, and matching grant time
after callbacks are re-enabled. FOM, logical-time implementation, names, and
callback model remain adapter inputs.

The `java-tck.callback-controls-time-role-enablement` scenario enables time
regulation and time constrained roles while callbacks are disabled, services
both federates, and verifies that neither enablement callback escaped. After
re-enabling callbacks, it checks exactly one callback per role and confirms
each reported initial time matches the standard logical-time query. FOM, time
implementation, object-class name, and callback model are adapter inputs.

The `java-tck.callback-controls-interaction` scenario checks baseline ordinary
interaction delivery, gates a second delivery while callbacks are disabled,
then verifies exactly-once delivery with the configured parameter bytes, tag,
producer, transportation, and receive-order metadata after callbacks are
re-enabled. FOM, interaction/parameter names, and callback model are adapter
inputs.

The `java-tck.service-report-interaction-failure` scenario subscribes an
observer to the standard `HLAreportServiceInvocation` MOM interaction and
checks reports for invalid interaction handles, invalid parameter handles, and
unpublished interactions. It verifies the standard service/type/success
metadata, supplied and returned argument records, exception names, serial
numbers, producer handle, and that failed sends do not reach the application
subscriber. It requires an adapter-supplied MIM and remains unverified against
a provider until run with its profile.

The `java-tck.service-report-timestamped-attribute-update` scenario checks a
timestamped attribute update's `HLAreportServiceInvocation` and its separate
timestamp-ordered reflection. It validates typed service-report arguments,
the returned retraction designator, invoking federate, withheld delivery
before the receiver's time advance, reflection metadata, and the matching
grant. FOM, MIM, logical-time implementation, object/attribute names, and
callback model are adapter inputs.

The successful `java-tck.service-report-interaction` case also verifies the
ordinary receive-order application interaction delivered to a separate
subscriber while the publisher's `HLAreportServiceInvocation` is delivered to
the MOM observer. Both callbacks are checked independently for exactly-once
delivery and their respective payload, tag, producer, transportation, and
report metadata.

## Build

The portable entry point is Python's standard-library runner; it starts
`javac` without a shell and works across supported host shells:

```text
python tools/run_java_tck.py build \
  --api-jar C:/deps/hla-1516.1-2025-api.jar \
  --output-directory out/java-tck/classes
```

Only the official API JAR is needed at compile time.

## Run

At runtime, put the provider JAR and any provider dependencies on the class
path. The FOM module is required for lifecycle and ordinary data scenarios.

```text
python tools/run_java_tck.py run \
  --api-jar C:/deps/hla-1516.1-2025-api.jar \
  --provider-jar C:/deps/provider-rti.jar \
  --factory-name "Provider RTI" \
  --fom-path C:/fom/RestaurantFOMmodule-2025.xml \
  --mim-path C:/fom/HLAstandardMIM-2025.xml \
  --capability-profile C:/fom/provider.properties \
  --classes-directory out/java-tck/classes \
  --results-path out/java-tck/provider.results.json \
  --junit-path out/java-tck/provider.junit.xml
```

Use `--dependency-jar` for additional provider JARs and `--jvm-argument` for
extra JVM settings or TCK fixture properties. Repeat it for each value; for
example, `-Dhla.rti.tck.additionalFom=...` supplies an additional scenario FOM.
The capability profile decides which scenarios run. It does not fill in the
FOM class names or modules those scenarios require.

### Current Umbra JNI TCK status

The official 2025 API/JNI run on 2026-10-28 used the repository's
`RestaurantFOMmodule-2025.xml`, `HLAstandardMIM-2025.xml`, and the Umbra JNI
profile. It reported **121 passed, 148 failed, and 487 unsupported** out of 756
scenarios. Of the failures, 84 asked for adapter-supplied TCK inputs, 47 failed
while resolving FOM names or time models, 12 reached behavioral assertions, and
5 ended in other exceptions. This is useful diagnostic evidence, not a clean
Java conformance result. Keep unsupported scenarios distinct from passing
scenarios, and check the result JSON for the exact property or service failure
before interpreting a broad run.

`matrix` runs the same compiled classes against two or more provider
configurations. `matrix.example.json` shows the portable configuration shape;
each provider supplies its own JAR paths, factory name, FOM, profile, and
optional JVM arguments:

```text
python tools/run_java_tck.py matrix \
  --configuration packages/hla-rti-java-tck/matrix.example.json \
  --classes-directory out/java-tck/classes \
  --output-directory out/java-tck/matrix
```

## Traceability

`compliance/catalogs/java-tck-scenario-catalog.json` is the source of truth for
scenario IDs, standard Java API methods, requirements IDs, and contract files.
Validate the source boundary and catalog with:

```text
python tools/java_tck.py validate
```

The export command joins provider result artifacts to the catalog:

```text
python tools/java_tck.py export \
  --results out/java-tck/provider.results.json
```

The result contains stable scenario IDs, provider identity, capability profile,
standard API linkage, JUnit/evidence paths, and explicit portable exclusions.

## Optional JPype handoff

After a direct Java run passes, the same API/provider JARs can be checked
through JPype:

```text
python packages/hla-rti-java-tck/jpype_smoke.py \
  --direct-results out/java-tck/provider.results.json \
  --api-jar C:/deps/hla-1516.1-2025-api.jar \
  --provider-jar C:/deps/provider-rti.jar \
  --factory-name "Provider RTI" \
  --output out/java-tck/jpype-evidence.json
```

This handoff is an integration check; it does not add provider code to the
portable Java source set.
