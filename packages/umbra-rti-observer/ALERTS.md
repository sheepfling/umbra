# Observer alerts

Alerts are an observer feature, not an HLA service. An RTI without MOM, DDM,
ownership or provider commands still supports observation/error/history rules.
Each threshold describes the evidence this observer has, not the health or
complete membership of the federation.

## Frontend and backend ownership

Committed ingress → dependency-free `AlertEngine` → named validated HTTP models
→ generated TypeScript → `AlertsView`.

| Owner | Responsibility |
| --- | --- |
| `alerting.py` | Pure Python evaluation, monotonic observation ages, bounded error detail, versioned baselines and evidence. No RTI, Pydantic or file dependency. |
| `core.py` | Serialize configuration/evaluation under the store lock; observe accepted events even when they immediately expire from raw history. Rejected batches do not contribute. |
| `commands.py` | Supply immutable age/ID facts from dispatcher caches; never inspect a provider from an HTTP/evaluation thread. |
| `alert_models.py` / `alert_routes.py` | Strict discriminated rules, finite thresholds, cross-checked evidence and conditional configuration/reset/acknowledgement. |
| `useAlerts` / `AlertsView` | Discover resources, render rule-state counts and searchable evidence, edit/download/import configuration and respect read-only/pause/replay states. |

The core remains usable on Python 3.11 without web or RTI dependencies:

```python
from umbra_rti_observer import ObserverStore
from umbra_rti_observer.alerting import AlertRuleSpec

store = ObserverStore()
store.configure_alerts(
    [AlertRuleSpec("errors", "Observed errors", "errors",
                   threshold_count=2, window_seconds=30)],
    expected_revision=0,
)
store.publish({"event_type": "runtime.error", "source": "synthetic-example"})
report = store.alert_report()
```

## Rules and their evidence

| Kind | Threshold / evidence | Important distinction |
| --- | --- | --- |
| `stale-observations` | 0.1–86400 observer-monotonic seconds since a matching observation. | Never observed means unknown. Idle is not a disconnection. |
| `errors` | 1–1,000,000,000 matching `runtime.error` observations in a 1–3600-second window. | Starts at activation; does not invent earlier history or depend on the raw-event cache. |
| `dropped-history` | 1–1,000,000,000 expired raw envelopes since activation/reset. | Keeps exact baseline/current lifetime counters; explicit clearing is separate from expiry. |
| `pending-commands` | 0.1–86400 seconds for the oldest matching accepted/running cached request, including a timed-out call still in flight. | No dispatcher or an unknown target is unavailable, not zero pending work. No provider query/command is performed. |

Observation filters are optional exact root-envelope `source`, `federation`,
`federate` and `session_id` fields. Blank means any. Names remain legacy filters,
not stable cross-RTI identities; migration is OBS-127. Nested time/ownership/
relationship session IDs are not silently promoted into a root identity.
Adapters can explicitly supply a root session ID through event extensions.

Only an exact source/session filter can classify an explicit
`session.disconnected` as ended. `federate.resigned` also requires an exact
federate filter; resignation does not disconnect an entire session. Another
source or join cannot close the rule. A name/threshold/reset edit keeps a known
end for the same filter; late packets cannot silently reopen it. Reconnects
need fresh producer session IDs and an appropriate new filter. Deleting a rule
discards its watch, not a permanent federation tombstone.

Results are `disabled`, `unknown`, `clear`, `firing` or `ended`. Reasons and
coverage are separate typed values. `clear` means the particular upper bound
is below its threshold, not that a provider is healthy. Known zero, unknown,
missing and unavailable are not interchangeable.

## Exact participant filters

On a backend advertising `participant_provenance`, errors and stale-observation
rules can instead select a named exact `scope.perspective` with explicit join
IDs. The server validates its canonical key and full metadata. Choose that or
the legacy text filters, not both. Matching ignores mutable display names and
handles. Other namespaces, reporting sources and memberships never contribute
or close the exact monitor. Root-only disconnects cannot close an identified
domain without its namespace/federation evidence. A name-filtered rule that
has observed identified joins is not ended by a single join's lifecycle event;
threshold/name/reset edits cannot erase that uncertainty. Legacy RTIs still
support ordinary text/error/history rules without inventing join IDs.

## Bounded error windows and history

At most 64 rules and 64 KiB of canonical configuration are admitted. Each error
rule retains at most 512 monotonic time marks; same-clock bursts use exact
counts. Overflow is summarized by count and first/last time, not sampled away.
While that entire span is in the window its count is exact. When the moving
window cuts through it, results disclose lower/upper bounds:

- A lower bound meeting the threshold proves `firing`.
- An upper bound below it proves `clear`.
- Otherwise the result is `unknown` with partial coverage, never a healthy zero.

Old spans disappear only when certainly outside the window. These bounds are
independent of the main [evidence budget](STORAGE.md). A bounded watch cannot
grow an unlimited copy of provider payloads.

The independent alert history retains 500 state/reason/coverage transitions,
with an explicit eviction counter. Repeated polling or metric growth within
the same state does not flood it. Each entry retains its actual rule and
generation, threshold, measurement/bounds, last relevant event sequence when
applicable, wall observation time and acknowledgement. Pending evidence also
identifies the oldest command/target and unknown-effect flag; history loss
includes its baseline/current counter proof. Event payloads, command parameters
and provider credentials are not copied into evidence.

Clearing event history does not reset rule baselines or erase alert evidence.
Unchanged rules keep baselines through configuration updates; changed/reset
rules get new generations. Old evidence is not relabeled with a new rule.
Acknowledging an exact evidence ID does not resolve, reset, cancel anything or
acknowledge a provider callback.

Configuration/evidence are shared **for the server process**, surviving browser
refresh/navigation, not restart. Download configuration to keep it and
explicitly import it into a later process. Downloads contain only versioned
rules, including chosen participant identity metadata and operator-entered
filters—not event payloads, evidence, backend-instance IDs/URLs or server
credentials. Review identifying configuration
before sharing. There is no hidden database, credential scrubber or filesystem
durability claim.

## Portable HTTP contract

`alerts` advertises reads. Separate `alert_controls` advertises observer
configuration mutations; it does not depend on `commands` or grant RTI authority.
Missing/disabled/unsupported features leave unrelated screens usable. A backend
can serve existing rules/evidence read-only.

| Discovered resource | Method | Named request / response |
| --- | --- | --- |
| `alerts` | GET | `AlertReport` |
| `alert_configuration` | PUT | `AlertConfigurationUpdate` → `AlertReport` |
| `alert_reset` | POST | `AlertResetRequest` → `AlertReport` |
| `alert_acknowledge` | POST | `AlertAcknowledgeRequest` → `AlertReport` |

All mutations require the report's `expected_instance_id`. Configuration/reset
also require `expected_revision`. A restart or competing edit returns 409
without applying a stale request. Acknowledgement uses its exact evidence ID
and instance, never an index from a previous process. The UI does not retry a
rejected edit automatically; an open draft becomes read-only if its revision/
instance changes. Older concurrent polls cannot overwrite a mutation result.
Imported files cannot select endpoints or bypass these guards.

`AlertConfiguration` requires `configuration_version: "umbra-observer-alerts-v1"`
and a bounded array of unique discriminated rules. Unknown fields, inapplicable
parameters, boolean/string counters, non-finite numbers and oversized rulesets
are rejected. Every response uses named models; threshold/unit/enable state,
metric bounds, counter/command proofs, history order and generations are checked.
Identifiers are opaque data, never paths or executable code.

`--no-alerts` omits HTTP resources and the periodic clock; `--read-only-alerts`
keeps reads without configuration/reset/ack endpoints. Embedding applications
use `create_app(..., enable_alerts=False)` or `enable_alert_controls=False` and
still own any rules explicitly configured on their core store. The web lifespan
evaluates once a second without constructing the full inspector/history tree;
ingress commits and reads also evaluate. This is not a real-time/SLA guarantee.

These endpoints share the deployment's security boundary. Capability discovery
is a feature contract, not authentication; protect remotely exposed observers.
All clients of the process see the same rules.

## Pause, replay and portability evidence

Pause freezes the local view and disables mutations, not collection/evaluation.
If no alert report was loaded before pausing, it remains unavailable until
resume; current data is not substituted. Alert rules and their separate evidence
are not captured by state-recording formats. Replay therefore neither fetches
current alerts nor applies rules to future data; return to live to manage them.

Java fixtures serve the same read model with controls disabled. The HTTP checker
validates advertised reads/discovery but lists evaluation and mutation as
unchecked. Static responses are not evidence of a Java evaluator, provider
behavior or IEEE conformance. Python tests exercise burst/overflow bounds,
lifecycle scope, rejected batches, baselines, versions, acknowledgement and HTTP
mutations. Desktop/mobile tests cover the workflow, stale drafts, unsafe imports
and older/disabled/read-only backends.
