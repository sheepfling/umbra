# Backend interoperability matrix

This is an internal observer application contract lane, not IEEE RTI normative
evidence. The same strict, named Pydantic wire models and semantic checks run
against recorded HTTP fixtures, the live FastAPI implementation and an
independent JDK HTTP fixture server. Missing RTI features are valid when the
manifest says so; missing or malformed **advertised** resources are failures.

| Profile | Expected behavior |
| --- | --- |
| `full` | Rich observation capabilities; HTTP-only and read-only. |
| `limited` | No MOM, ownership or regions; ordinary objects, time and roster remain usable. |
| `partial` | Partial logical-time support is explicit and includes a reason. |
| `empty` | Valid empty arrays and unknown RTI capabilities, not a broken backend. |
| `retention-loss` | Bounded history, expired sequences, a gap flag and exclusive ordered cursors. |
| `no-analytics` | Analytics is disabled and its path omitted; core observations still validate. |
| `reconnect` | First contract request fails with 503; the next run recovers without changing clients. |

These fixtures are synthetic and use a fixed wall clock. They are not session
recordings or a replay engine. A profile directory contains JSON responses and
an ordered `routes.tsv` manifest. The Java fixture reads those files directly;
it neither calls Python nor imports HLA APIs. Its intentionally finite query
table is not a production implementation of arbitrary event filtering.

## Check a backend without changing it

```powershell
python -m pip install -e "packages/umbra-rti-observer[conformance]"
python -m umbra_rti_observer.conformance http://127.0.0.1:8765
```

Only GET requests are sent. The JSON report includes individual checks,
`passed`, omitted optional features and explicitly unchecked features. Exit
status is nonzero on failures, including a backend that cannot be reached.
Discovery is same-origin; redirects and foreign resource URLs are not followed.
Each response is bounded to 8 MiB and each request has a finite timeout.

The checker validates nested payload types without coercing strings/booleans
into counters, rejects non-JSON non-finite extension values, verifies version
and media headers, required collections, retention, unique event sequences,
pagination/gaps, and aggregate consistency. A usable analytics/commands feature
must advertise an endpoint that returns its validated response model. Disabled,
unsupported or unknown features need not provide their optional endpoints.
Optional command catalogs/history are read, but no command is executed.

Optional object/time inspectors also require their named models when advertised
as available/partial. Time validation checks unique scoped request IDs, capacity,
ordered outcomes, snapshot cursors and duration/status consistency. The full
fixture includes correlated and pending requests; limited/no-analytics profiles
remain grant-only, and retention-loss preserves a pending request beyond event
eviction. Numeric time may be unknown without disabling explicit-ID correlation.

The declaration increment adds a named optional declaration model, unique scoped
snapshot keys, capacity/cursor checks and distinct selected/all/unknown/not-
applicable attribute semantics. The full Java/Python fixture includes object
publication, unknown-set subscription and interaction declarations. No fixture
or checker infers routing relationships from matching class names.

The full profile also carries explicit relationships, coordination phases,
interaction pairs, ownership state/tickets and exact decimal-string DDM ranges.
MOM reports include a partial member report with an unresolved handle, a service
error, numeric counter history and opaque wide-integer text. FOM metadata
includes explicit parent/type references, array/enum encodings and an exact
dimension upper bound. Named discriminated models enforce applicability,
identity uniqueness, replacement consistency, bounds and cursor rules. The
checker requires each advertised inspection model independently; legacy MOM,
ownership and regions flags do not promise these newer typed inspectors.
Malformed input tests cover each family. None implies native RTI behavior.

The optional `recording` capability advertises named process-local capture
status. HTTP-read conformance checks only that GET model and discovery; it never
starts/stops a capture or uploads a file. Its report explicitly lists capture/
import/replay as unchecked. Dedicated tests cover recording versions, finite
JSON, bounds, concurrency, batch/cursor continuity, clear boundaries, idle
checkpoints, import error/size safety and exact replay clock/command isolation.
Java fixture profiles currently disable recording resources; this does not
disable their independent observation workspaces. The portable full-model corpus
also validates and displays as a recording without reconstructing inspectors.

The old dependency-free `umbra_rti_observer.probe` remains a quick smoke check;
use `conformance` for these stronger acceptance checks. Passing this HTTP-read
scope does not validate stream delivery, lifecycle effects or RTI semantics.
`python -m umbra_rti_observer.probe URL --strict` is an equivalent entry point
for callers already using the quick probe.

## Repeat and evolve the contract

Install the reference implementation's `web,test` extras and JDK 17+, then run:

```powershell
python -m pytest packages/umbra-rti-observer/tests/test_conformance.py -q
python -m umbra_rti_observer.conformance_schema --openapi .cache/observer-openapi.json
```

`src/umbra_rti_observer/static/observer-wire-v1.schema.json` is the portable
JSON Schema 2020-12 bundle (`$defs` contains named response/stream models).
It is included in the Python distribution and playtest ZIP. Required wire
collections are explicit even where Python constructors offer defaults.
`normalized_events` must additionally be present for `include_events=true`;
it may be omitted for compact snapshots.

After reviewing an intentional model/contract change, explicitly update the
schema with `conformance_schema --write` and HTTP corpus with
`conformance_profiles --write`. Ordinary checks never rewrite them. Preserve
extension fields and document compatibility/version implications.

In the frontend package:

```powershell
pnpm generate:api --check --schema ../../.cache/observer-openapi.json
pnpm test:codegen
pnpm test:e2e
```

The drift check compares freshly generated types without touching the checked-in
file. Its own tests prove that both changed schemas and stale output fail.
CI runs the schema/corpus checks, Python/Java matrix and browser portability
tests. If no JDK is installed locally, the seven Java HTTP tests explicitly
skip; CI installs JDK 17 and runs them. Java browser cases require a JDK.
