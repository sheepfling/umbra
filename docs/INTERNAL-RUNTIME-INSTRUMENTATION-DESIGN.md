# Internal Runtime Instrumentation Design

## Status

This is an internal, long-running design for measuring runtime activity across
the embedded RTI implementation. It is not a public RTI API and does not
change the IEEE 1516.1 ABI.

The first aggregate implementation is now present. It records implemented
`UmbraRtiAmbassador` service scopes, callback queue/execution timing,
caller-owned `FederateAmbassador` entry/exit timing, major embedded registry
operations, transport fault/control events, and service-report store events.
It now also has internal console, JSON, and one-shot CSV writers plus a
worker-thread CSV sampler with cumulative and per-interval counters. The
sampler performs no file I/O on RTI, registry, transport, or callback paths.
Rich callback method labels, cardinality dimensions, and bounded event tracing
remain later phases.

The first target is operation statistics. A later trace mode may use the same
boundaries to record a bounded sequence of correlated events.

## Scope

The instrumentation should account for the complete local runtime path:

```text
application thread
    |
    v
RTIambassador call
    |
    v
UmbraRtiAmbassador
    |  lifecycle, validation, public service boundary
    v
EmbeddedFederationRegistry
    |  shared federation state, DDM, ownership, time, save/restore
    v
callback route
    |
    v
CallbackDispatcher -> CallbackSession -> caller-owned FederateAmbassador

transport fault/control --------------------^
service-report planning/encoding/sink -------^
```

The caller-owned `FederateAmbassador` is not wrapped or replaced. The
`CallbackSession` is the boundary at which Umbra can observe actual entry into
and return from user callback code.

## What must be distinguishable

The implementation must not collapse these into one counter:

1. **RTI service calls**: calls made through the public `RTIambassador`
   interface, including calls that fail with an HLA exception.
2. **Internal federation operations**: logical registry transitions such as
   enqueueing an interaction, admitting a time advance, planning DDM
   recipients, or completing a retraction.
3. **Callback scheduling**: callback work accepted, queued, suppressed, or
   removed before user code runs.
4. **Federate callbacks**: actual invocation of a named method on the
   caller-owned `FederateAmbassador`.
5. **Transport events**: connect, graceful close, fault, and RTI-side
   membership control.
6. **Service-report events**: report eligibility, reservation, serialization,
   delivery, file append, and suppression.

For example, one `sendInteraction` call may produce one RTI service-call
record, one registry operation, several callback-scheduling records, and zero
or more actual federate callbacks. Those are related by correlation IDs but
are not the same statistic.

## Proposed internal components

### Runtime instrumentation sink

Add a private `RuntimeInstrumentation` component under `cpp/src/internal`.
It owns hot-path aggregation and exposes an internal snapshot operation.

The initial implementation should use fixed operation identifiers rather than
allocating arbitrary strings on every call. Human-readable names can be kept
in a static descriptor table generated from the binding inventory.

Conceptual shape:

```cpp
enum class InstrumentationLayer {
  rti_ambassador,
  federation_registry,
  callback_dispatch,
  federate_ambassador,
  transport,
  service_reporting,
};

struct OperationDescriptor {
  InstrumentationLayer layer;
  std::string_view name;
};

class RuntimeInstrumentation {
 public:
  [[nodiscard]] CallScope begin(OperationDescriptor operation,
                                CallContext context = {});
  [[nodiscard]] RuntimeInstrumentationSnapshot snapshot() const;
};
```

The exact names and storage strategy can change. The important properties are
that recording is thread-safe, does not take the RTI ambassador lock, and does
not perform file or console I/O on the operation path.

### Per-ambassador and shared-federation ownership

There are two useful scopes of ownership:

- Each `UmbraRtiAmbassador` owns or shares a statistics handle for its
  connection, public calls, callback delivery, and transport lifecycle.
- The process-local `EmbeddedFederationManagement` owns the shared registry
  instrumentation for federation-wide state. Its registry is currently a
  static process-local object, so this is the natural place to aggregate
  cross-federate activity.

An ambassador receives the shared federation instrumentation when it connects
or joins. Standalone registry unit tests may continue to construct an
uninstrumented registry by default, or inject a test sink explicitly.

This gives both views without duplicating global counters:

```text
UmbraRtiAmbassador A ---- per-ambassador view ----+
                                                  |
UmbraRtiAmbassador B ---- per-ambassador view ----+--> shared runtime snapshot
                                                  |
EmbeddedFederationRegistry -- federation view ---+
```

## Boundary design

### RTI ambassador calls

The public method implementation on `UmbraRtiAmbassador` is the authoritative
RTI service boundary. Each implemented public method should eventually begin a
small RAII scope identified by the exact standard service name.

The scope records:

- total calls;
- successful returns;
- HLA exception and internal-exception returns;
- explicitly returned negative/false outcomes where applicable;
- elapsed wall-clock time;
- minimum, maximum, and accumulated duration;
- active calls and peak concurrency;
- optional input-shape counters such as attribute, parameter, region, or
  recipient counts.

The scope must end after the public method has finished all synchronous work,
but before it returns to the caller. It must not hold `UmbraRtiAmbassador::mutex_`
while updating instrumentation.

The generated `RtiAmbassadorShell` is not itself a universal interception
point: implemented methods in `UmbraRtiAmbassador` override the generated
fallback methods directly. Therefore the first implementation will need
explicit scopes or generated source assistance at the real method definitions.
A later refactor could introduce generated public wrappers and protected
implementation methods if avoiding one-line scopes becomes important.

### Federate callbacks

`CallbackDispatcher` currently knows whether work is pending and whether a
callback is executing, while `CallbackSession` knows when user callback code is
actually entered and when it returns. The instrumentation should use both:

- dispatcher: submitted, queued, dequeued, suppressed, and pending-high-water
  counts;
- task metadata: enqueue time, originating call ID, callback operation ID, and
  recipient federate ID when known;
- session: actual callback start/end, queue wait duration, callback duration,
  exception count, and in-flight/peak-in-flight counts.

The current callback task and invocation types are unlabelled `std::function`s.
They should grow private metadata while retaining the same public callback
behavior. Callback labels can be introduced incrementally, beginning with the
central callback helpers and then covering all callback construction sites.

The caller may make a re-entrant RTI call from a callback. The active call
context must therefore support a parent/child relationship rather than
assuming a single flat call stack.

### Federation registry

The registry should record logical operations, not every helper or lookup.
Initial registry operation groups should be:

- federation create/destroy/join/resign;
- declaration and subscription changes;
- object registration, update, deletion, and discovery planning;
- interaction delivery planning and enqueueing;
- ownership transfer and callback admission;
- time advance request/grant/retraction;
- save/restore transitions;
- MOM/service-report routing.

For delivery-oriented operations, useful dimensions are recipient count,
accepted count, suppressed count, queue count, timestamped count, and
retraction count. These should be recorded from the registry's returned plans
and status values, not inferred later from callbacks.

The registry already has a strong lock boundary and returns callback plans
instead of invoking callbacks while locked. Instrumentation must preserve that
rule: record state-transition metrics while the registry lock is held only if
the update is lock-free/constant-time, and never invoke an exporter or user
callback from inside the registry lock.

### Transport

`EmbeddedTransportConnection` and `EmbeddedTransportHub` are the correct
transport-level hooks. Record connection identity and lifecycle events there,
including fault-versus-graceful close and forced resignation. Payload byte
metrics should wait until a real transport payload boundary exists; the current
loopback hub has no meaningful wire-byte count.

### Service reporting

Service-report statistics should remain separate from ordinary RTI service
statistics so reporting a call does not recursively count itself as another
public RTI call. The report path should eventually expose:

- eligible and suppressed reports;
- interaction versus file routing;
- serial reservations;
- encoding failures;
- writer append failures;
- records delivered to subscribers;
- records written to files.

The existing MOM report planner and `ServiceReportStore` are appropriate sinks
for these events, but they are not a replacement for the general runtime
instrumentation layer.

## Correlation model

Every top-level RTI service call receives a monotonically increasing call ID
from the per-runtime instrumentation object. The active context contains:

- runtime/ambassador identity;
- optional federation and federate identity;
- operation ID and layer;
- parent call ID for re-entrant calls;
- originating thread ID;
- start timestamp.

When an operation queues callback work, the callback task copies the relevant
context. When the task later enters `CallbackSession`, the callback event keeps
the originating call ID but receives its own event ID. This allows reports such
as:

```text
sendInteraction #104
  registry.enqueueInteraction #105
  callback queued #106 -> federate 2
  callback reflectAttributeValues #107
```

Correlation should be optional in the first aggregate-only build. It becomes
mandatory only for a bounded trace/ring-buffer mode, where event retention and
privacy limits must be specified.

## Collection and output

The hot path should aggregate in memory. No operation should synchronously
append to a file, emit a log line, or format a large JSON object.

The internal readout options are:

1. a non-installed test seam returning a typed snapshot;
2. console, JSON, and one-shot CSV writers invoked explicitly by internal code
   or test fixtures;
3. a worker-thread CSV sampler that records cumulative and per-interval
   counters for long-running collection;
4. a bounded in-memory event ring buffer added later for ordering and
   correlation investigations.

The sampler CSV can be plotted with `tools/plot_runtime_instrumentation.py`.
It produces throughput, average duration, and callback queue/execution timing
plots from the interval deltas. Matplotlib remains an optional tooling
dependency and is not linked into the RTI implementation.

An internal diagnostic fixture can collect the complete embedded view like
this:

```cpp
umbra::detail::RuntimeInstrumentationCsvSampler sampler(
    rti.runtimeInstrumentationSnapshotProviderForTesting(),
    "runtime-instrumentation.csv",
    std::chrono::seconds(1));
sampler.start();
// Run the federation workload.
sampler.stop();
```

The provider includes the ambassador totals and, in the embedded federation
management profile, the shared registry totals. One-shot console, JSON, and
CSV output can be selected with `writeRuntimeInstrumentation` when a periodic
sampler is unnecessary.

The compile-time switch should be separate from the public service-reporting
switch, for example `UMBRA_ENABLE_INTERNAL_INSTRUMENTATION`. Disabled builds
should compile the scopes down to minimal no-op code and should not add a
public symbol or ABI field.

## Phased implementation plan

### Phase 0: contracts and inventory

- Freeze the layer vocabulary and operation naming rules.
- Generate a complete RTI service descriptor table from
  `compliance/binding-inventory.json`.
- Decide which metrics are aggregate-only and which require correlation.
- Add invariants for no I/O, no callback invocation, and no public API change.

### Phase 1: statistics core

- Add `RuntimeInstrumentation`, operation counters, duration aggregation, and
  typed snapshots.
- Add unit tests for concurrency, exception completion, nested scopes, and
  disabled instrumentation.
- Keep the component unused by runtime code initially.

### Phase 2: one vertical slice

- Instrument `connect`, `disconnect`, `evokeCallback`, and one callback path.
- Add callback queue wait and callback execution duration.
- Add an internal `UmbraRtiAmbassador` test seam for snapshot inspection.
- Verify immediate and evoked callback models and re-entrant calls.

### Phase 3: complete RTI ambassador coverage

- Add scopes to all implemented `UmbraRtiAmbassador` public methods.
- Use a source-generation or audit script so new inventory members cannot be
  added without an instrumentation decision.
- Record operation-specific cardinalities only where they are cheap and
  semantically stable.

### Phase 4: shared federation coverage

- Inject optional instrumentation into `EmbeddedFederationRegistry`.
- Instrument the logical registry operation groups and returned plan/status
  dimensions.
- Attach federate and federation identity without copying large payloads.
- Add multi-ambassador tests proving one shared federation view and distinct
  per-ambassador views.

### Phase 5: callback, transport, and report completeness

- Label all callback construction sites.
- Add transport lifecycle/fault events.
- Add service-report routing and sink metrics.
- Verify that internal report emission does not recursively inflate public
  service-call counts.

### Phase 6: readout and performance validation

- Add an internal JSON/CSV snapshot writer.
- Add optional bounded trace mode only if aggregate statistics cannot answer
  the operational questions.
- Benchmark enabled and disabled builds under concurrent callback and update
  load.
- Set retention, cardinality, and sensitive-data rules before enabling any
  long-lived trace output.

## Testing strategy

The tests should prove both counts and boundaries:

- one public call produces exactly one public-call record even when it calls
  many registry helpers;
- one queued callback produces one scheduling record and one actual callback
  record if it is delivered;
- a suppressed or resigned callback never appears as an actual user callback;
- an exception increments the failure bucket and still closes the duration
  scope;
- immediate and evoked models report different queue latency but identical
  actual callback counts;
- two ambassadors share registry totals but not per-ambassador totals;
- transport failure and RTI-side resignation are distinct event outcomes;
- service-report routing does not recursively create public RTI call records;
- concurrent snapshots are consistent and do not deadlock with RTI or registry
  locks.

## Current implementation status

The private statistics core and the first end-to-end vertical slice are
implemented without modifying the public `RTIambassador` headers, the
caller-owned `FederateAmbassador`, or the existing MOM service-report format.
The complete implemented ambassador method inventory is scoped by operation
name, while callback timing is split between dispatcher queue/execute metrics
and the actual `CallbackSession` invocation boundary. Internal console, JSON,
and CSV snapshot writers plus the worker-thread CSV sampler are also present;
the sampler CSV is suitable for the optional plotting helper.

The next implementation increments are explicit callback method labels,
status/cardinality fields on registry operations, and bounded trace/correlation
support for investigations that need event ordering rather than aggregates.
