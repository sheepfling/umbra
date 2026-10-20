# Owned synchronization and save/restore controls

The Actions workspace can offer seven optional services for an explicitly owned
session. Its catalog controls availability independently of MOM, roster,
topology and read-only coordination inspection. Adapters implement only services
their RTI supports.

## Try the synthetic flow

Run the rich launcher and open **Demo walkthrough**, or go to Actions.
Connect and join the synthetic control session first. Each coordination call
requires a fresh owned target version and explicit confirmation.

1. Register synchronization with a label. Wait for its announced report in
   Synchronization, then report achievement with the exact label.
2. Request save with a checkpoint label. Once the request is reported, report
   save begun and then the local save result.
3. Wait for the explicit federation completed report, then request restore using
   that exact saved label. Report the local result after restore begins.
4. In Save / restore, filter the label and choose **Follow operation**. Separate
   cards show the last local/federation reports and any supplied participants.
   A completed local card can coexist with a failed federation card.

Drill-down initially draws 20 scopes and 25 supplied participants per scope,
with independent load-more controls. Missing or truncated reports stay explicit.

The demo pump delays reports after service return. Early, duplicate and wrong
label calls return unavailable without changing known membership. Synthetic
checkpoints are bounded to 32 labels and last for this driver's lifetime.
No provider filesystem or actual federation data is persisted by the demo.

Background showcase operations have separate source/session/operation identities
from owned commands. Their paced success, failure and unknown progress reports
cannot authorize actions.

## Portable requests and results

| Action | Named parameters | Local meaning |
| --- | --- | --- |
| `sync.register` | `SynchronizationRegisterParameters(label, tag_text)` | Request a federation-wide point. The adapter owns text-tag encoding. |
| `sync.achieve` | `SynchronizationAchieveParameters(label, result)` | Report achieved/failed for this owned participant and an announced label. |
| `save.request` | `CoordinationLabelParameters(label)` | Request immediate save; timed save is not included. |
| `save.begun` | `EmptyParameters` | Report this federate's pending save begun. |
| `save.complete` | `CoordinationCompletionParameters(result)` | Report completed/failed local save. |
| `restore.request` | `CoordinationLabelParameters(label)` | Request restore by exact label. |
| `restore.complete` | `CoordinationCompletionParameters(result)` | Report completed/failed local restore. |

All seven requests require `expected_version` and `confirmed: true`. Labels
preserve whitespace and case. Strict models reject unknown fields, paths,
arbitrary method names and coerced values. The bounded text tag is not a binary
upload.

Success retains a `CoordinationCommandReceipt`: kind, exact label, optional
adapter operation ID and `basis: service-return`. Registration, achievement,
persistence and collective completion remain separate explicit
`coordination.observed` reports. Unknown effects, timeouts, late outcomes and
retained-ID deduplication use the existing dispatcher. Provider operations are
never retried automatically.

## Implement another RTI

`SessionController` takes an explicit `coordination_actions` tuple; the default
is empty. Opt-in drivers implement
`CoordinatingSessionDriver.coordinate(CoordinationCommandRequest)`. Factory,
service calls and callback pumping run on the existing owner thread. The driver
translates named requests into its binding shape and returns typed receipts.

The driver enforces provider preconditions, maps proven no-effect rejections to
`CommandUnavailable`, and publishes actual callback evidence with scoped
operation IDs. The catalog identifies supported services for a joined session;
it does not query pending provider phases. A provider can reject a point that
is not announced or a save/restore that is not pending.

The separate Umbra adapter still defaults to its verified lifecycle services.
It does not opt in to these seven services yet. Native coordination, Java
execution, subset synchronization, timed saves, status queries and application
state serialization need explicit adapter work and provider evidence. Internal
observer tests are not IEEE conformance evidence.

Observation works without commands. Missing, disabled, unsupported and partial
capabilities keep independent meanings. Pause/replay cannot submit commands.
Drill-down never merges reporters, reused labels, joins or local/collective
results. See [the contract](CONTRACT.md#optional-session-commands) and
[demo coverage](DEMO-COVERAGE.md).
