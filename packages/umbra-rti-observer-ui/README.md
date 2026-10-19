# Umbra Runtime Observer UI

The React/TypeScript frontend for the provider-neutral Umbra runtime observer.
It consumes only the observer HTTP contract and can therefore be used with a
Python, Java, or vendor-specific backend that implements that contract.

The workspace includes Federation, Federates, Objects, Declarations, Logical time, Events, Inspectors,
Actions, and Diagnostics tabs. The optional `/api/analytics` resource supplies bounded
time-window aggregates and explicit participant attribution. Backends that do
not advertise analytics keep the events, inspectors, and basic metric charts.
See the [feature backlog](../umbra-rti-observer/FEATURE-BACKLOG.md) for delivered
features and the next increments. The Actions tab consumes the optional command
catalog, builds typed lifecycle requests and polls execution history. A backend
without session commands leaves the observation views usable.

Objects adds name/class/handle search, active/removed filters, global versus
explicitly attributed snapshots, previous-value comparisons, byte/representation
inspection and selectable finite-number charts. The optional
`object_observations` capability/model is independent of MOM and analytics.
Snapshots survive history eviction; charts never reconstruct missing history.
Older/disabled backends show an unavailable panel, not another federate's
global values. The same models work through the Java HTTP fixture.

Logical time adds an independent `time_inspection` capability/model: persistent
last-grant/state snapshots, explicit scoped request IDs, pending ages, terminal
outcomes and observer request-to-grant delay charts. It works without analytics
or MOM; grant-only sources stay useful and missing numeric time stays unknown.
Lookahead/roles/GALT/LBTS are reported facts only. The bounded request ledger
reports eviction separately from event retention. Reconnects need fresh session
IDs; delays measure one observer's monotonic ingress clock, not RTI service time.

Declarations adds independent scoped publication/subscription snapshots, class/
attribute search and kind/direction/status filters. Selected-empty, all, unknown
and interaction-not-applicable sets are distinct. Capacity loss and missing
features are explicit; no routing, ownership or delivery edges are inferred.

Pause view freezes the local view while collection continues. Event exports
contain the current filtered loaded rows; they are not full-session recordings.
Paused or stale control views cannot submit new commands.

## Development

For a complete synthetic playtest without separate terminals, run
`python tools/observer_playtest.py` from the repository root (or double-click
`Start-Observer.cmd` on Windows). It builds this dashboard, starts the dummy
backend with controls, and opens the browser. `--bundle` packages the built
dashboard for someone who does not have Node. See the
[showcase guide](../umbra-rti-observer/SHOWCASE.md).

Start an observer backend on `http://127.0.0.1:8765`, then run:

```powershell
pnpm install
pnpm generate:api
pnpm dev
```

The Vite development server proxies `/api` and `/ws` to the Python showcase.

`generate:api` uses the FastAPI-generated `/openapi.json` document to refresh
`src/api/generated.ts`. The generated file is a compile-time client contract;
the backend remains responsible for runtime validation with Pydantic or its
language-equivalent models.

Build the UI for an observer service with:

```powershell
pnpm build
```

Then point `UMBRA_OBSERVER_UI_DIST` at this package's `dist` directory when
starting the Python service. The service falls back to its zero-build HTML
dashboard when the React build is not present.

## Repeatable browser regressions

From the repository root, run `python tools/observer_playtest.py --check` once
to prepare the private Python web environment. Then, in this UI package:

```powershell
pnpm install --frozen-lockfile
pnpm exec playwright install chromium
pnpm test:e2e
```

The suite builds the production dashboard, starts a fresh synthetic Python or Java
backend on an OS-selected loopback port for **each test**, and cleans up only
that owned process tree. No native RTI, fixed port, manually running server,
or browser session is needed. Desktop and 390px mobile projects cover the nine
screens, keyboard/history navigation, charts, participant isolation, payload
filters/JSON downloads, bounded rendering/history clearing, reconnect,
connect/join/confirmed resign/disconnect, page refresh, pause safety, stale and
unsupported controls, and limited/read-only/no-analytics/empty backends.
Object regressions also cover shared-object perspective isolation, evicted
history, unknown/null comparisons, bytes, numeric selection, pause and filtering.
Time regressions cover persistent state, scoped matching, pending/granted/failed/
ambiguous outcomes, distinct sessions, pause/clear, Java and capability fallback.
Declaration regressions cover scope/session isolation, replacement/withdrawal,
pause/clear, history/capacity eviction, bounded rendering, Java and feature states.

If you already have Edge on Windows, avoid a browser download by setting
`$env:UMBRA_OBSERVER_BROWSER_CHANNEL='msedge'` before `pnpm test:e2e`.
`UMBRA_OBSERVER_TEST_PYTHON` can select another Python with the `web` extra
installed; otherwise the suite uses the playtest's private environment, then
`python` on PATH. HTTP seed payloads and intercepted catalogs use the generated
HTTP types, not provider-specific RTI bindings.

Use `pnpm test:e2e:headed` to watch the tests. For direct runner options after a
build, use `pnpm exec playwright test --project=desktop --repeat-each=3`.
Failures retain a screenshot, trace and backend log under ignored
`test-results/`; `pnpm test:e2e:report` opens the HTML report.

The separate `Observer dashboard` CI workflow runs Python tests plus both
browser projects in Chromium. These are internal application regressions, not
IEEE RTI conformance evidence. Cross-browser accessibility audits, large-scale
performance and real-provider Java integration remain separate backlog items.

## Backend portability checks

JDK 17+ is now needed for the Java fixture cases in the full browser suite; it
is not a prerequisite for the ordinary Python playtest or Miguel's ZIP.
`UMBRA_OBSERVER_TEST_JAVA` can select the Java executable. Twelve additional
desktop/mobile checks cover Java's limited/partial/empty/no-analytics shapes,
relocated REST paths and incompatible-version rejection (44 checks total).
The Java fixture serves the identical production build without a Python server.

The client follows the contract's REST/WebSocket paths, rejects foreign or
query-bearing discovered paths and incompatible versions, and uses polling
when WebSockets are not advertised. MOM and control authority remain optional.
See the [shared HTTP conformance matrix](../umbra-rti-observer/conformance/README.md)
for the strict checker and portable response schemas. To detect generated-type
drift without running or trusting an existing server, export fresh OpenAPI with
`python -m umbra_rti_observer.conformance_schema --openapi .cache/observer-openapi.json`
from the root, then run `pnpm generate:api --check --schema ../../.cache/observer-openapi.json`
here. `pnpm test:codegen` verifies the check fails on stale types without rewriting them.
