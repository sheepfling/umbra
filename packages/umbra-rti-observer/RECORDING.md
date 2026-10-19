# Portable observer recordings

Recording is an optional observer-server feature, never an RTI requirement.
Missing MOM, ownership, DDM or analytics does not prevent a capture of the data
that actually exists. Files include federation data and may include secrets in
extensions. They are not sanitized workspace configurations.

## Two formats, one meaning

V1 (`umbra-observer-recording-v1`) stores a complete named `RecordingFrame` for
every accepted commit. It remains the default for an unqualified API start and
export. Existing files, clients and implementations remain supported.

V2 (`umbra-observer-recording-v2`, codec `observer-json-edits-v1`) stores one
complete `StateResponse` baseline plus named `RecordingDeltaFrame` entries.
Each has the same index, monotonic offset, captured wall time, change kind and
complete new arrivals as v1, plus explicit edits and a reconstructed-state byte
charge. The first frame is the zero-offset unedited baseline. Every later state
is produced by applying that frame's edits in order to the previous state.
This is lossless storage, not inferred inspector reconstruction or sampling.
Every batch, metadata change, clear, checkpoint and accepted stop remains a
distinct frame. Equal offsets and backward-moving wall clocks remain valid.

Only the **live backend's** `recording_compact` capability permits v2 requests.
The frontend sends `storage_format: "compact-v2"` on start and the same query
on export. Missing/unsupported capability uses the original v1 request, without
an extra field. Export rejects a format mismatch with 409; it never silently
converts a shared process capture or hands an old client a new format. New
clients read the retained capture's status before export. V2 import also
requires live-backend support. Captured capabilities/paths grant no authority.

## The edit contract

Edits are a discriminated union of three strict named models:

| Operation | Fields | Meaning |
| --- | --- | --- |
| `set` | `path`, `value` | Replace an existing array element or set an own object key; an empty path requires an object and replaces the state root. |
| `delete` | nonempty `path` | Delete an existing own object key, never an array element. |
| `splice` | `path`, `index`, `delete_count`, `items` | Replace exactly the stated in-bounds array interval with literal items. |

Paths are arrays of exact strings or nonnegative integer array indices. `"0"`
and `0` have different meanings. Boolean/coerced/negative indices, missing
intermediate keys, inherited properties, out-of-bounds deletions and unknown
operations are rejected. Parent objects/arrays are never created implicitly.
For example:

```json
{"op":"set","path":["context","label"],"value":null}
```

Null is a literal value, not deletion. Object keys such as `__proto__` are
ordinary own JSON keys; JavaScript uses own-key checks and explicit property
definition, never prototype traversal/assignment. The codec has no executable
expressions, references, copy/move operations, network URLs or external data.
Inserted values are detached before later edits can mutate the working tree.
Rolling arrays use retained-suffix/new-prefix splices; unchanged numeric and
opaque values are not reinterpreted. JSON object key order has no meaning.

## Limits and validation

Both formats keep the 16 MiB input/encoded-file limit and at most 2000 frames
(default 600). Capture stops before an over-budget state; it never evicts the
beginning or interrupts observation. V2 adds:

| Bound | Limit |
| --- | --- |
| Each reconstructed state | 8 MiB UTF-8 JSON, depth 96, 750,000 values/containers |
| Total reconstructed-state work | 1 GiB charged across every frame, including baseline/checkpoints |
| Edits | 20,000 per frame, 100,000 per file; paths at most 96 segments |

`state_bytes` is an upper bound on that frame's serialized reconstructed JSON,
not compressed bytes or RSS. `decoded_state_bytes` must equal the sum of those
charges, and `edit_count` the actual operation total. Fixed maximum fields
identify the supported work envelope; they cannot be raised by an imported
file. Stop reasons `state-limit`, `decode-limit` and `edit-limit` distinguish
these gates from encoded `byte-limit` and `frame-limit`. The process capture
status exposes format and work accounting; older absent metadata stays unknown.

The API validates every reconstructed state through `StateResponse` and
`RecordingFrame`, including strict optional inspector models, before accepting
the file. Both versions share cursor/arrival/window/transition checks: no lost
batch arrivals, rewritten/resurrected envelopes, reset lifetime cursor or
unannounced history changes. Manifest/capabilities, finite JSON, stop boundaries
and duration are validated too. Malformed edits cannot bypass named models.
Required context/adapter/capability/roster and object/interaction collections
must actually be present in the file and every reconstructed state. Python
constructor defaults cannot stand in for captured evidence. The same shared
wire requirements drive HTTP conformance and the portable schema.
Import errors are bounded and do not echo federation payloads. Validation does
not save a file or call a provider. It returns the compact document, not a huge
expanded timeline. The API allows clients up to 60 seconds for bounded v2
export/import processing; this is a timeout, not a latency guarantee.

The backend keeps an encoded baseline/frames and one working diff state while
recording; stopping releases the working state. Validation retains only the
current/previous named state and one working tree. These serialized work bounds
are not exact allocator/RSS limits. They prevent unlimited expansion, but capture
and validation still have CPU/memory cost; see [SCALE.md](SCALE.md).

## Frontend replay

`recordingCodec.ts` is an independent TypeScript consumer of the generated
contract. A dedicated worker retains only the current decoded state and the
last 60 small metric samples. Forward seeks apply intervening edits; backward
seeks restart at the baseline. Work yields between states so a newer seek can
replace queued work; closing/changing the document terminates its worker.
Every posted state is detached from subsequent worker mutation. No complete
array of expanded states is retained on the page.

The banner identifies the actual displayed state while a requested seek is
being reconstructed. It never labels an earlier/later state as the target.
Pending/error reconstruction does not substitute live data. Recorded analytics
is disabled during a pending seek and keyed to the exact resolved frame/window.
Replay never follows captured URLs, opens live subscriptions or enables provider
commands. Returning to live reconciles a fresh snapshot; it does not rewind the
federation. Old v1 files use the same controls and safety rules.

## Verification and remaining scope

Core codec tests cover lossless type/value changes, rolling windows, literal
null, own/prototype-like keys, invalid paths, dense edits and state bounds.
Named-model/API tests cover full inspector provenance, complete arrivals,
clear/rollover, corrupt states/work accounting, explicit negotiation and legacy
defaults, including missing collections in v1, v2 baselines and v2 edits.
Independent TypeScript tests and desktop/mobile browser checks cover
seeks, old files, format fallback, worker isolation and command/URL safety.
These are internal application checks, not native RTI/IEEE evidence. The Java
HTTP fixture does not advertise recording; a Java server may implement these
same named JSON models without sharing Python implementation details.

Large full-state delivery and remaining persistent-index budgets are still
OBS-124 work. V2 deliberately rejects an initial state above its 8 MiB decoded
bound; compression must not be used to hide an arbitrarily huge state.
