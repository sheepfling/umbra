# Umbra observer adapter

This is the Umbra-specific integration package for the standalone
`umbra-rti-observer` dashboard. The generic observer package has no HLA or
Umbra dependency; this package owns the 1516.1-2025 Python callback and
connection shapes used by Umbra's native provider.

The dashboard/backend boundary is defined by the observer package's
[`CONTRACT.md`](../umbra-rti-observer/CONTRACT.md), not by this adapter. A Java
provider adapter can publish the same normalized events and use the same
frontend. This adapter also declares its observed capability surface so the
dashboard can distinguish full, partial, and unsupported RTI services.

The adapter deliberately stops at the provider boundary. It can connect an
already-created Umbra `RTIambassador` and translate callbacks, but it does not
invent FOM subscriptions or domain-specific object semantics. The application
chooses the FOM, joins the federation, and subscribes to the classes it wants
to observe.

Install the packages from this checkout:

```powershell
python -m pip install -e packages/umbra-rti-observer
python -m pip install -e packages/umbra-rti-api
python -m pip install -e packages/umbra-rti-native
python -m pip install -e packages/umbra-rti-observer-umbra
```

Attach the adapter to an observer store and pass its standard callback object
to Umbra's RTI ambassador:

```python
from hla.rti1516_2025 import CallbackModel
from umbra._native.rti1516_2025 import UmbraRtiFactory
from umbra_rti_observer import ObserverStore
from umbra_rti_observer_umbra import UmbraRtiAdapter

store = ObserverStore()
rti = UmbraRtiFactory().getRtiAmbassador()
adapter = UmbraRtiAdapter(
    rti,
    callback_model=CallbackModel.HLA_EVOKED,
    source="umbra-native",
)
store.attach_adapter(adapter)

# The application still owns its normal FOM loading, federation join, and
# publish/subscribe setup through `rti`.
```

The callback translator emits only generic events such as
`object.discovered`, `object.updated`, `object.removed`,
`interaction.received`, `runtime.error`, and `callback.invoked`. Other RTI
providers should get their own adapter package when their callback or session
shape differs.

## Optional owned-session controls

The observation adapter above remains application-driven. To expose lifecycle
commands, register a separate controller for a session the application owns:

```python
from umbra._native.rti1516_2025 import UmbraRtiFactory
from umbra_rti_observer import ObserverStore
from umbra_rti_observer.commands import CommandDispatcher
from umbra_rti_observer.server import create_app
from umbra_rti_observer_umbra.commands import create_umbra_command_session

store = ObserverStore()
commands = CommandDispatcher(sink=store.publish)
commands.register(create_umbra_command_session(
    lambda: UmbraRtiFactory().getRtiAmbassador(),
    target_id="umbra-observer-session",
    federations=("ExampleFederation",),
    sink=store.publish,
))
app = create_app(store, commands=commands)
```

Install `umbra-rti-observer[web]` and `umbra-rti-observer-umbra[control]` for this
path. Importing the ordinary observation adapter does not import Pydantic.
`commands.py` is checked with the package's strict `pyrightconfig.json`.

The controller lazily creates its RTI ambassador, connects with HLA_EVOKED,
and pumps callbacks on the same dedicated execution thread. Connect, join,
resign and disconnect all use that owner. Do not also attach the same RTI
ambassador through `UmbraRtiAdapter` or call it from HTTP/browser threads.
The application provides configuration and credentials server-side and chooses
the allowed federation names. FOM loading and federation creation remain
application setup; this command catalog does not expose arbitrary FOM URLs or
create/destroy operations.

The HTTP contract, command IDs, confirmations, timeout/late-result behavior
and history belong to the generic observer package. A different RTI can supply
its own `CommandSession` or `SessionDriver`, with explicit supported actions,
instead of adapting its API shape in React. The existing read-only
`UmbraRtiAdapter` does not automatically grant command authority.

Strict fake-ambassador tests cover the lifecycle and owner thread. Real native
verification is an opt-in internal smoke test; it requires the built 2025
provider and a matching Python interpreter on the environment's import path:

```powershell
$env:UMBRA_OBSERVER_NATIVE_SMOKE = "1"
python -m pytest packages/umbra-rti-observer-umbra/tests/test_native_control_smoke.py -q
```

That test creates an isolated source-checkout federation, executes the four
lifecycle commands, and cleans it up. It is not normative IEEE test evidence.
The normal observer regression run leaves this native-only test explicitly
skipped. Server deployments exposing real commands must provide their normal
session authorization around the HTTP app.
