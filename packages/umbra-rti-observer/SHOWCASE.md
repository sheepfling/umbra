# Observer showcase

This is a live, provider-neutral walkthrough of the observer. It uses the
same adapter, store, HTTP contract, stream, and dashboard path as a real RTI
integration, but it does not require an RTI installation.

## Easiest start (including first-time setup)

On Windows, double-click **Start-Observer.cmd** in the repository root. Or,
from the repository root on any platform, run:

```powershell
python tools/observer_playtest.py
```

The launcher builds the React dashboard, prepares a private Python environment,
starts live synthetic traffic with the Actions controls enabled, waits until
events are advancing, then opens your browser. No native RTI, compiler, FOM or
manual environment activation is needed. A source checkout needs Python 3.11+
and Node 22.13+; dependencies download on first launch. Later launches
reuse the environment and unchanged dashboard build.

Keep the terminal window open. **Ctrl+C in that window stops the demo**;
closing the webpage alone does not stop it. All demo data resets on restart.
If Windows asks `Terminate batch job (Y/N)?`, type `Y` and press Enter.
The printed URL is authoritative: if port 8765 is busy, another free port is
chosen without stopping any existing service. The server listens only on this
computer, not your network.

For the smaller RTI profile, double-click **Start-Observer-Limited.cmd**, or run:

```powershell
python tools/observer_playtest.py --profile limited
```

### Send Miguel a playtest ZIP

```powershell
python tools/observer_playtest.py --bundle
```

Send `dist/umbra-observer-playtest.zip`. It includes the built React dashboard,
observer Python sources, both Windows launchers, and a short `README.txt`.
Miguel extracts the ZIP and double-clicks `Start-Observer.cmd`. He needs only
Python 3.11+ and internet access for the first dependency install; **no Node,
repository checkout, C++ build or HLA installation**. macOS/Linux users can run
`python3 tools/observer_playtest.py` from the extracted folder instead.

The ZIP excludes native RTI code, bindings, development dependencies and local
environments. It is a disposable playtest snapshot, not a release package.

Troubleshooting: `--no-browser` prints the URL without opening it; `--port 8770`
selects a specific port. `--check` prepares everything, verifies the backend
and advancing demo events, then stops automatically. Startup failures show an
explanation; backend errors include the local server log path.

## Reproduce the Miguel checkpoint

The annotated Git tag `observer-showcase-2026-10-19` preserves the capability-aware
showcase through persistent object/time inspection and request/grant analysis.
It includes synthetic controls, rich/limited RTI profiles, generated frontend
types, Python/Java contract fixtures and the easy-start launchers. It does not
claim native RTI conformance; the optional native smoke remains a separate gate.

To get that fixed version in a separate folder without disturbing current work:

```powershell
git clone --branch observer-showcase-2026-10-19 --depth 1 https://github.com/sheepfling/umbra.git umbra-observer-miguel
cd umbra-observer-miguel
python tools/observer_playtest.py
```

The checkout is intentionally detached at the tag. On Windows you can also
double-click `Start-Observer.cmd`; use `Start-Observer-Limited.cmd` to demonstrate
missing MOM and grant-only time support. The source checkout needs Python 3.11+
and Node 22.13+ with first-run dependency downloads.

The later tag `observer-showcase-2026-10-19-declarations` additionally includes
the declaration explorer increment. Substitute it for the tag in the clone
command above for the expanded nine-screen showcase. The original checkpoint
tag is left unchanged.

Run `python tools/observer_playtest.py --bundle` from that checkout to recreate
the disposable ZIP. Built assets and environments are not stored in Git.
The tag fixes the source and frontend lockfile, not an indefinitely frozen
Python/browser environment or a binary release. Keep an already tested ZIP if
you need to send that exact playtest artifact later.

## Manual start (existing Python environment)

From the repository root:

```powershell
python -m pip install -e "packages/umbra-rti-observer[web]"
python -m umbra_rti_observer --showcase --profile umbra
```

Open <http://127.0.0.1:8765/>. The page will continue to receive object
updates, interactions, logical-time callbacks, and selected management
callbacks.

## Show the graceful-degradation profile

Stop the first process and run:

```powershell
python -m umbra_rti_observer --showcase --profile limited
```

The same dashboard remains useful. Timing, lifecycle, federates, objects,
attributes, and interactions continue to work. MOM, ownership, and regions are
clearly labelled as unsupported, while persistence is shown as disabled. No
fake data is generated for those unavailable features.

## Optional recovery walkthrough

The current dashboard offers **Inspect session** and confirmed **Reconcile
session** for its separate synthetic control session. Inspection alone never
restores permissions. This is newer than the historical checkpoint tag above;
use the current source/current playtest ZIP for this walkthrough.

To demonstrate an uncertain effect, after installing the web extra or preparing
the launcher environment, run with a Python 3.11+ interpreter:

```powershell
python -m umbra_rti_observer --showcase --enable-controls --showcase-control-fault-once connect --port 0
```

On Windows, the prepared interpreter is
`.cache/observer-playtest/venv/Scripts/python.exe`; substitute it for `python`
if your default interpreter/environment is different. Open the printed URL.
Connect deliberately takes effect in the synthetic driver, then reports a
failure. In Actions, inspect its actual connected/not-joined facts, review the
original failed record, then explicitly reconcile. The original failure stays
in history and no second connect is issued. Use `--showcase-control-fault-once
join` to demonstrate a join taking effect before its error, or add `--profile
limited` to keep MOM/ownership/DDM unavailable throughout.

`--showcase-control-inspection partial` makes membership inspection unavailable:
a query can return successfully while control remains unknown. `disabled` omits
recovery entirely while preserving normal known-state lifecycle commands. These
flags configure only the synthetic owned driver; they never inject faults into
or reconfigure a real RTI. Stop this demo with Ctrl+C in its terminal.

## Suggested walkthrough

To demonstrate the Actions tab, start with `--showcase --enable-controls`.
Select its labelled synthetic control session and connect, join as
`ManagedObserver`, confirm resign, then confirm disconnect. Observe the session
state and command result after each step. Refresh the page to show that backend
command history remains available. Pausing the view disables command submission.
This flow also works with `--profile limited`; it requires no MOM data.

In **Objects**, search for `ExampleClass`, choose `load` in the numeric selector
and switch from **Global observer** to **ShowcaseServer** and
**ShowcaseObserver**. The subscriber-like synthetic observations arrive less
frequently, so their latest snapshot and retained comparisons can differ.
Snapshots are explicitly attributed, not ownership or subscription evidence.
Use **Pause view** to inspect a comparison while collection continues. The
limited profile has the same object workspace without requiring MOM.

In **Logical time**, the rich showcase reports a fresh synthetic session,
pending requests, later correlated grants and measured observer delay. Inspect
lookahead/role fields and switch perspectives to the grant-only ShowcaseObserver.
GALT/LBTS stay unknown unless directly reported. The limited showcase reports
grants without request IDs or role state: its last-grant inspector still works,
while request-delay analysis clearly has no correlated samples. These inspector
snapshots are independent of the chart-time window and do not require MOM.

In **Declarations**, compare ShowcaseServer's explicit publications with
ShowcaseObserver's reported subscriptions, search `load`, then filter by
interaction versus object or by direction. The limited profile reports only
one object subscription with an unknown attribute set. These are last-reported
declarations, not a routing graph; the explorer deliberately draws no edges
between matching class names. Declarations remain useful without analytics.

The current working-tree showcase also adds **Topology**, **Interaction flow**,
**Synchronization**, **Save / restore**, **Ownership**, **Regions / DDM**,
**MOM reports** and **FOM browser**. These additions are not part of the older
fixed tags above. Topology and interaction pairing require explicit identities;
no matching names create edges. Save/restore separates local and collective
results. Ownership separates state from service tickets; DDM keeps exact bounds
without claiming that overlap proves delivery.

In **MOM reports**, inspect the partial member report and unresolved handle,
reported service failure, and the selectable counter history. The wide-integer
counter stays opaque text rather than becoming a rounded number. These reports
do not replace the ordinary Federates roster or grant command authority.
In **FOM browser**, inspect ExampleClass, follow its parent and datatype links,
and browse the array, enum and dimension metadata. Restart with the limited
profile to show that useful class metadata remains available even with unknown
ancestry/types and no MOM. The browser never parses FOM files or decodes bytes.

In **Recordings**, give the capture a label, start it, explore other tabs, then
return to stop/download it. Choose **Replay retained recording** or reopen its
JSON file. Show the purple replay banner, speed and time/exact-state sliders,
then visit Objects, MOM or FOM and seek backwards. Actions are unavailable in
replay even if captured controls were available. **Return to live** fetches fresh
state; the real/synthetic backend was not rewound or stopped. A recording lasts
only for the backend process unless downloaded. Its memory/frame limits stop
capture automatically without stopping traffic. Files contain actual captured
context/data; review them before sharing. The fixed tags above predate this
working-tree increment.

After updating sources, stop your own demo with Ctrl+C and launch it again.
Refreshing the browser does not reload the Python process. The launcher will
leave an already running older demo alone and choose another free port.

1. Open `/api/contract` and point out the stable contract version and the
   capability states.
2. Open the dashboard and show the capability matrix.
3. Point out the live event sequence, object inspector, interaction stream,
   and logical-time callback details.
4. Restart with `--profile limited`.
5. Show that the frontend is unchanged and only the affected feature states
   change.
6. Explain that a Java federate or backend can use the same contract without
   changing the dashboard.

## Useful inspection URLs

```text
/api/contract
/api/state?include_events=false
/api/events?after=0&limit=20
/api/metrics
```

The showcase is intentionally an adapter-shaped producer, not a simulation of
an HLA provider. It demonstrates the observer boundary and the maturity-aware
frontend behavior that a real Python, Java, or vendor-specific adapter plugs
into.
