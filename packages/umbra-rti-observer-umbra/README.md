# Umbra observer adapter

This is the Umbra-specific integration package for the standalone
`umbra-rti-observer` dashboard. The generic observer package has no HLA or
Umbra dependency; this package owns the 1516.1-2025 Python callback and
connection shapes used by Umbra's native provider.

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
