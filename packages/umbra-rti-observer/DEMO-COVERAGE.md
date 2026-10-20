# Demo coverage map

Start `Start-Observer.cmd` or `python tools/observer_playtest.py`.
**Demo walkthrough** lists all 19 workspaces with concrete exercises.
Its links navigate without submitting commands. All traffic and services in
this mode are synthetic.

| Workspace | Rich demo exercise |
| --- | --- |
| Federation | Traffic, participant activity, composition and selectable windows. |
| Federates | Scoped participant selection, attributed events, objects, time and declarations. |
| Objects | Global/attributed values, numeric load history, opaque representations and comparisons. |
| Declarations | Publications/subscriptions and selected/unknown attribute sets. |
| Topology | Ten joins and fifteen edges: fan-out, chain, cycle, self-loop, reciprocal/parallel edges, repeated names and disconnected group. Try three layouts, keyboard edge inspection and node focus. |
| Interaction flow | Explicit pairing, unpaired observations and observer intervals. |
| Ownership | Known/unknown state, completed acquire and failed divest tickets. |
| Regions / DDM | Exact large bounds, associations and explicit routing reports. |
| MOM reports | Partial/unresolved members, service failure and numeric/opaque counters. |
| FOM browser | Class ancestry, attributes, datatypes, enum/array and dimension references. |
| Logical time | Pending/correlated requests and grants alongside a grant-only perspective. |
| Synchronization | Paced barriers, participant progress and registration rejection. |
| Save / restore | Local/collective scopes, unknown peer progress and alternating success/failure. |
| Events | Search/filter, envelope expansion and JSON export. |
| Inspectors | Retained object, participant and interaction observations. |
| Recordings | Capture/stop/download, replay play/pause/speed/seek, return to live. |
| Alerts | Create rules, inspect states/history, acknowledge and export configuration. |
| Actions | Owned lifecycle, inspect/reconcile and seven coordination services. |
| Diagnostics | Transport/lifetime, retention, capabilities, adapters, storage and runtime warnings. |

Coordination advances over 40 traffic updates with fresh operation IDs each
cycle. The next cycle reverses which save/restore fails. At the launcher's
0.75-second interval, a cycle lasts about 30 seconds. **Follow operation** shows
separate scope/participant cards. Long-running evidence loss remains explicit.

## Other UI modes

- Use Pause / Resume while traffic continues.
- Use Saved workspaces to save/apply and download/import view choices.
- Use `Start-Observer-Limited.cmd` for grant-only time, partial declarations/FOM,
  unavailable topology/MOM/ownership/DDM/synchronization, disabled persistence,
  and lifecycle controls with coordination controls omitted.
- The manual showcase's `--no-analytics` preserves independent inspectors.
  Other optional feature switches are listed by `--help`.
- The [recovery walkthrough](SHOWCASE.md#optional-recovery-walkthrough) exercises
  uncertain effects, partial inspection and disabled recovery.
- Reconnect/offline, older/missing contracts, corrupt browser storage, truncation,
  stale responses and unsupported controls have isolated browser scenarios.
  They are not presented as live RTI facts in the default demo.

`coordination-actions.spec.ts` uses the actual synthetic service on desktop and
mobile for owned coordination, limited omission, complex layouts, walkthrough,
saved configuration, pause, capture, replay isolation and alert creation.
Existing workspace, streaming, storage and recording suites exercise detailed
failure and compatibility modes, including independent Java responses.

Graph layout and edge highlight are temporary display choices. Saved workspaces
retain the existing topology filters and participant focus. Graph positions
have no timing, distance or provider-routing meaning.
