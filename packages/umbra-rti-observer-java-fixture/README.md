# Observer Java contract fixture

A small JDK 17+ HTTP server for checking that the observer frontend and contract
checker do not depend on Python. This is **not a Java RTI adapter**, a production
observer, or HLA conformance evidence. It serves deterministic, read-only JSON
fixtures; it does not collect callbacks, implement commands, or fake WebSockets.

It has no third-party Java dependencies and does not start a Python process.
The shared corpus is in `../umbra-rti-observer/conformance/fixtures`. Each
profile's `routes.tsv` maps exact GET request targets to HTTP status and JSON
files. A repeated route can describe a transient failure followed by recovery.
Queries are order-insensitive; requests not in the fixture return 404. Other
methods return 405, so this service cannot alter a federation.

From the repository root, after building the React UI:

```powershell
java --add-modules jdk.httpserver packages/umbra-rti-observer-java-fixture/src/ObserverFixtureServer.java packages/umbra-rti-observer/conformance/fixtures/limited packages/umbra-rti-observer-ui/dist
```

Open the printed loopback URL. The unchanged frontend shows HTTP polling,
object/time observations and unavailable controls/MOM. Only this computer can
access the service. Data is static and labelled `ContractFixture`, not live RTI
traffic. Stop it with Ctrl+C. The UI directory argument is optional when only
HTTP conformance is being checked.

Run the strict checker against the printed URL from a Python environment with
the observer's `conformance` extra:

```powershell
python -m umbra_rti_observer.conformance http://127.0.0.1:PORT
```

The Python tests compile this source with `--release 17` and run the same checks
against seven Java profiles and the live Python reference. Browser tests also
serve the production React build directly from Java for limited, partial,
empty, no-analytics, full and retention-loss profiles. Full demonstrates the
same named persistent time models with matched and pending requests; grant-only
profiles need no request-ID support. No provider-specific library is involved.

Before replacing this test double with a real Java backend, implement the
versioned resources and truthful capability manifest, then run these checks
and any appropriate command/stream suites. A passing HTTP-read report alone
does not prove command execution, stream delivery, or RTI semantics.
