# Saved observer workspaces

Saved workspaces remember how to investigate, not what the federation reported.
Open **Saved workspaces** in the dashboard header. Current choices are saved
automatically in this browser when storage permits. Give a configuration a name,
choose **Save current workspace**, then **Download saved workspace** to send it
to someone else. Miguel can import that JSON and explicitly apply it to his
own observer without a Python validation endpoint.

## What is remembered

The versioned configuration includes the selected screen and participant, the
30-second/5-minute/15-minute/one-hour chart window, and each investigation's
search, filters and selection. Global and federate event/declaration/time
settings are independent, and each federate keeps its own event/declaration
filters and time perspective in `federate_views`. Older v1 `federate_*` settings
remain accepted as templates for a participant not yet customized. Numeric
attribute choices use the exact object key
and perspective; choosing another perspective does not borrow its chart.
MOM counter, time perspective, topology join, region report, coordination
operation and FOM revision/element selections retain their exact keys.

Pagination, expanded payload details, uploads, command/alert-rule drafts,
recording bodies, captured events, snapshots and provider credentials are not
saved. Neither backend URLs, backend-instance IDs nor command authority have
configuration fields. Choosing Actions only opens that screen; applying a
workspace never submits a provider command or changes alert rules.

Selection/search text itself can identify a participant, object or operation;
anything you type into a filter is included. Review files before sharing them.
Browser storage is not a secret store. These profiles do not preserve evidence:
use [recordings](RECORDING.md) when you intentionally need captured data, and
the [alert configuration file](ALERTS.md) for rule definitions.

## Restore and limited RTIs

Import validates and saves a named configuration; it does not apply it. Select
**Apply saved workspace** to restore it. Replacement of an existing exact name
requires confirmation. Names are case-sensitive and Unicode is not normalized.
The current draft can also be downloaded without creating a named copy.

Unsupported features stay unavailable independently. Their stored choices stay
dormant rather than being discarded, and a choice absent from the current data
stays explicitly selected. No other participant, object, counter or model is
silently substituted. Empty results do not establish that an RTI has no such
objects or participants. Supported screens remain usable.

A valid URL fragment such as `#events` explicitly selects that screen when
opening a page; otherwise the saved screen is restored. Browser navigation
continues to work. Applying a profile resets temporary paging/detail state.
Apply, reload and reset are disabled during pause/replay; saving/downloading
configuration is still harmless. Return to live before restoring settings.

Some legacy filters still use reported participant names. A saved name is not
proof of the same participant or RTI incarnation after changing backends;
check current source/session provenance. Stable identity migration is OBS-127,
separate from the explicit identity-scoped topology and typed report keys.

## Storage, failures and bounds

One browser-origin key, `umbra.observer.workspace-library.v1`, stores the current
draft and at most 12 named profiles. A normalized profile is at most 64 KiB;
the whole library is at most 512 KiB. An imported file is at most 128 KiB before
normalization, allowing readable indentation. There are at most 64 exact object/
perspective chart preferences; adding a new target at that limit expires the
oldest preference with a notice, not an observation. At most 64 per-federate
settings entries are retained; editing a new perspective beyond that expires
the oldest settings entry with a notice, never federation evidence. Search fields allow 2,048
Unicode scalar values; filter/selection keys allow 8,192; names allow 128 and
must include a non-whitespace character. The models enforce encoded UTF-8
limits on complete documents as well as individual fields.

Storage is local to an origin, browser profile and device. Clearing browser
data, private-mode lifetime, quota limits or abrupt termination can lose it.
A backend restart does not erase a browser profile, but profiles do not migrate
automatically to another origin. Download files for portable backups.

Quota/permission failures leave the dashboard usable in memory and report the
problem. Invalid/future-version browser data is kept unchanged and automatic
writes are blocked. Export the active choices if needed before explicitly
resetting the application key; unrelated browser storage is never cleared.

A change observed from another tab blocks automatic writes and leaves the
current investigation in place. **Reload browser copy** explicitly adopts its
latest configuration; export your active settings first if you need both.
This is conflict detection, not atomic multi-tab transactions, locking, merging
or cross-device synchronization. A reset removes this application's key only.

## Frontend/backend contract

`workspace_models.py` owns strict named Pydantic models. Unknown fields,
coercible booleans/numbers, unsupported versions, duplicate chart targets and
duplicate saved names are rejected, not stripped. Optional fields receive
named defaults. Response types are generated from OpenAPI alongside the other
observer models.

`workspace_schema` exports two JSON Schema 2020-12 documents and defaults into
`static/workspace-v1.schema.json` and the frontend contract snapshot. This
bundle is a developer artifact, not an importable user profile. The trusted
schemas are compiled into standalone validators at build time; browser imports
do not compile schemas, evaluate code or fetch `$schema`/resource URLs. A shared
27-case corpus checks Python/offline normalization and rejection parity,
including Unicode names and default factories. Complete size and uniqueness
checks supplement structural schema validation.

`WorkspaceProvider` owns settings; screens consume typed preference hooks.
`codec.ts` validates before storage/import/export/apply. `WorkspacePanel` owns
explicit user operations, not RTI calls. The optional `workspace_validation`
backend capability advertises GET `workspace_defaults` and POST
`workspace_validate`; the latter normalizes and returns a document without
saving/applying it. `--no-workspace-validation` omits both independently of
local frontend support. A Java backend can omit them entirely or implement the
same named models. The read-only conformance checker verifies advertised GET
defaults/discovery only; it explicitly does not prove import normalization.

After intentionally changing models, run from the repository root:

```powershell
python -m umbra_rti_observer.workspace_schema --write --frontend packages/umbra-rti-observer-ui/src/api/workspace-contract.json --cases packages/umbra-rti-observer-ui/scripts/fixtures/workspaces.json
python -m umbra_rti_observer.conformance_schema --write --openapi .cache/observer-openapi.json
pnpm --dir packages/umbra-rti-observer-ui generate:api --schema ../../.cache/observer-openapi.json
pnpm --dir packages/umbra-rti-observer-ui generate:workspaces
```

Review the diffs; do not hand-edit generated validators. CI runs these checks
without `--write`, offline tests, Python models/endpoints and desktop/mobile
browser regressions, including a Java backend with no Python normalization
endpoint and a policy that disallows runtime code evaluation.
