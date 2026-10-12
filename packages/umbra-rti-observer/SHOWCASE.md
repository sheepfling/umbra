# Observer showcase

This is a live, provider-neutral walkthrough of the observer. It uses the
same adapter, store, HTTP contract, stream, and dashboard path as a real RTI
integration, but it does not require an RTI installation.

## Start the rich profile

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

## Suggested walkthrough

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
