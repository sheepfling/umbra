"""Translate Umbra's standard Python RTI callback shape into generic events."""

from __future__ import annotations

import json
import threading
from collections.abc import Callable, Iterable, Mapping
from typing import Literal, NotRequired, TypedDict, cast

from hla.rti1516_2025 import CallbackModel, FederateAmbassador, RTIambassador
from umbra_rti_observer import EventSink, JsonObject, JsonValue
from umbra_rti_observer.models import CapabilityState

_UNREADABLE_PROVIDER_VALUE = object()

ObserverEventType = Literal[
    "runtime.started",
    "runtime.stopped",
    "runtime.error",
    "federate.joined",
    "federate.resigned",
    "federate.updated",
    "object.discovered",
    "object.updated",
    "object.removed",
    "interaction.received",
    "callback.invoked",
]


class AdapterCapability(TypedDict):
    """One capability family in this adapter's normalized callback surface."""

    state: CapabilityState
    event_types: NotRequired[list[ObserverEventType]]
    reason: NotRequired[str]


class AdapterCapabilities(TypedDict):
    """Named observer capability families provided by the Umbra adapter."""

    lifecycle: AdapterCapability
    federates: AdapterCapability
    objects: AdapterCapability
    attributes: AdapterCapability
    interactions: AdapterCapability
    logical_time: AdapterCapability
    mom: AdapterCapability
    persistence: AdapterCapability
    ownership: AdapterCapability
    regions: AdapterCapability


class RtiConnectOptions(TypedDict, total=False):
    """Optional keyword arguments accepted by the RTI connect boundary."""

    configuration: object
    credentials: object


def _probe_provider_value(operation: Callable[[], object]) -> object:
    """Read a provider-owned proxy without letting its representation escape."""
    try:
        return operation()
    except Exception:  # noqa: BLE001 - proxy getters and __str__ are provider-defined
        return _UNREADABLE_PROVIDER_VALUE


def _best_effort(operation: Callable[[], object]) -> None:
    """Keep observer delivery and cleanup failures from replacing RTI behavior."""
    try:
        operation()
    except Exception:  # noqa: BLE001 - these are non-authoritative side effects
        return


def _jsonable(value: object) -> JsonValue:
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, memoryview):
        return {"encoding": "hex", "value": value.tobytes().hex()}
    if isinstance(value, (bytes, bytearray)):
        return {"encoding": "hex", "value": bytes(value).hex()}
    if isinstance(value, Mapping):
        mapping = cast(Mapping[object, object], value)
        return {str(key): _jsonable(item) for key, item in mapping.items()}
    if isinstance(value, (list, tuple, set, frozenset)):
        items = cast(Iterable[object], value)
        return [_jsonable(item) for item in items]
    try:
        json.dumps(value)
    except (TypeError, ValueError, OverflowError):
        return str(value)
    return cast(JsonValue, value)


def _handle(value: object) -> str | None:
    if value is None:
        return None
    for attribute in ("encodedValue", "value", "handle"):
        candidate = _probe_provider_value(
            lambda name=attribute: getattr(value, name, None)
        )
        if candidate is _UNREADABLE_PROVIDER_VALUE:
            continue
        if candidate is None or callable(candidate):
            continue
        if isinstance(candidate, memoryview):
            return f"0x{candidate.tobytes().hex()}"
        if isinstance(candidate, (bytes, bytearray)):
            return f"0x{bytes(candidate).hex()}"
        text = _safe_text(candidate)
        if text is not None:
            return text
    return _safe_text(value)


def _safe_text(value: object) -> str | None:
    result = _probe_provider_value(lambda: str(value).strip())
    if isinstance(result, str) and result:
        return result
    return None


def _mapping_pairs(value: object) -> list[tuple[object, object]] | None:
    if isinstance(value, Mapping):
        mapping = cast(Mapping[object, object], value)
        pairs = _probe_provider_value(lambda: list(mapping.items()))
    else:
        items = _probe_provider_value(lambda: getattr(value, "items", None))
        if not callable(items):
            return None
        typed_items = cast(Callable[[], Iterable[tuple[object, object]]], items)
        pairs = _probe_provider_value(lambda: list(typed_items()))
    if pairs is _UNREADABLE_PROVIDER_VALUE:
        return None
    return cast(list[tuple[object, object]], pairs)


def _mapping(value: object) -> JsonObject:
    item_pairs = _mapping_pairs(value)
    if item_pairs is None:
        return {}
    return {(_handle(key) or str(key)): _jsonable(item) for key, item in item_pairs}


def _handles(value: object) -> list[str | None]:
    if value is None:
        return []
    items = _probe_provider_value(lambda: list(cast(Iterable[object], value)))
    if items is _UNREADABLE_PROVIDER_VALUE:
        return []
    return [_handle(item) for item in cast(list[object], items)]


class UmbraFederateAmbassador(FederateAmbassador):
    """Standard callback object that emits provider-neutral event mappings."""

    def __init__(
        self,
        *,
        source: str = "umbra",
        connection: str | None = None,
        federation: str | None = None,
        federate: str | None = None,
        sink: EventSink | None = None,
    ) -> None:
        self.source = source
        self.connection = connection
        self.federation = federation
        self.federate = federate
        self._sink_lock = threading.RLock()
        self._sink = sink

    def set_sink(self, sink: EventSink | None) -> None:
        with self._sink_lock:
            self._sink = sink

    def _emit(self, event_type: str, **fields: object) -> None:
        with self._sink_lock:
            sink = self._sink
        if sink is None:
            return
        event: JsonObject = {
            "event_type": event_type,
            "source": self.source,
        }
        for key, value in (
            ("connection", self.connection),
            ("federation", self.federation),
            ("federate", self.federate),
        ):
            if value is not None:
                event[key] = value
        event.update(
            {
                key: _jsonable(value)
                for key, value in fields.items()
                if value is not None
            }
        )
        # Dashboard delivery is diagnostic and must not change callback
        # behavior in an RTI federate when its sink is unavailable.
        _best_effort(lambda: sink(event))

    def runtime_started(self, **details: object) -> None:
        self._emit("runtime.started", details=details)

    def runtime_stopped(self, **details: object) -> None:
        self._emit("runtime.stopped", details=details)

    def runtime_error(self, **details: object) -> None:
        self._emit("runtime.error", details=details)

    def connectionLost(self, faultDescription: str) -> None:
        self._emit("runtime.error", details={"fault": faultDescription})

    def federateResigned(self, reasonForResignDescription: str) -> None:
        self._emit(
            "federate.resigned",
            details={"reason": reasonForResignDescription},
        )

    def discoverObjectInstance(
        self,
        objectInstance: object,
        objectClass: object,
        objectInstanceName: str,
        producingFederate: object,
    ) -> None:
        self._emit(
            "object.discovered",
            object_handle=_handle(objectInstance),
            class_handle=_handle(objectClass),
            object_name=objectInstanceName,
            producing_federate=_handle(producingFederate),
        )

    def removeObjectInstance(
        self,
        objectInstance: object,
        userSuppliedTag: object,
        producingFederate: object,
        time: object = None,
        sentOrderType: object = None,
        receivedOrderType: object = None,
        retraction: object = None,
    ) -> None:
        self._emit(
            "object.removed",
            object_handle=_handle(objectInstance),
            tag=userSuppliedTag,
            producing_federate=_handle(producingFederate),
            logical_time=time,
            sent_order=sentOrderType,
            received_order=receivedOrderType,
            retraction=_handle(retraction),
        )

    def reflectAttributeValues(
        self,
        objectInstance: object,
        attributeValues: object,
        userSuppliedTag: object,
        transportationType: object,
        producingFederate: object,
        sentRegions: object = None,
        time: object = None,
        sentOrderType: object = None,
        receivedOrderType: object = None,
        retraction: object = None,
    ) -> None:
        self._emit(
            "object.updated",
            object_handle=_handle(objectInstance),
            attributes=_mapping(attributeValues),
            tag=userSuppliedTag,
            transportation_type=_handle(transportationType),
            producing_federate=_handle(producingFederate),
            logical_time=time,
            sent_order=sentOrderType,
            received_order=receivedOrderType,
            retraction=_handle(retraction),
        )

    def receiveInteraction(
        self,
        interactionClass: object,
        parameterValues: object,
        userSuppliedTag: object,
        transportationType: object,
        producingFederate: object,
        sentRegions: object = None,
        time: object = None,
        sentOrderType: object = None,
        receivedOrderType: object = None,
        retraction: object = None,
    ) -> None:
        interaction_handle = _handle(interactionClass)
        self._emit(
            "interaction.received",
            interaction_key=interaction_handle,
            interaction_handle=interaction_handle,
            parameters=_mapping(parameterValues),
            tag=userSuppliedTag,
            transportation_type=_handle(transportationType),
            producing_federate=_handle(producingFederate),
            logical_time=time,
            sent_order=sentOrderType,
            received_order=receivedOrderType,
            retraction=_handle(retraction),
        )

    def receiveDirectedInteraction(
        self,
        interactionClass: object,
        objectInstance: object,
        parameterValues: object,
        userSuppliedTag: object,
        transportationType: object,
        producingFederate: object,
        time: object = None,
        sentOrderType: object = None,
        receivedOrderType: object = None,
        retraction: object = None,
    ) -> None:
        interaction_handle = _handle(interactionClass)
        self._emit(
            "interaction.received",
            interaction_key=interaction_handle,
            interaction_handle=interaction_handle,
            object_handle=_handle(objectInstance),
            parameters=_mapping(parameterValues),
            tag=userSuppliedTag,
            transportation_type=_handle(transportationType),
            producing_federate=_handle(producingFederate),
            logical_time=time,
            sent_order=sentOrderType,
            received_order=receivedOrderType,
            retraction=_handle(retraction),
            directed=True,
        )

    def turnInteractionsOn(self, interactionClass: object) -> None:
        self._service_event(
            "turnInteractionsOn", {"interaction_handle": _handle(interactionClass)}
        )

    def turnInteractionsOff(self, interactionClass: object) -> None:
        self._service_event(
            "turnInteractionsOff", {"interaction_handle": _handle(interactionClass)}
        )

    def _service_event(self, operation: str, value: object = None) -> None:
        self._emit(
            "callback.invoked",
            operation=operation,
            details={"value": _jsonable(value)} if value is not None else {},
        )

    def provideAttributeValueUpdate(
        self, objectInstance: object, attributes: object, userSuppliedTag: object
    ) -> None:
        self._service_event(
            "provideAttributeValueUpdate",
            {
                "object_handle": _handle(objectInstance),
                "attributes": _handles(attributes),
                "tag": userSuppliedTag,
            },
        )

    def attributesInScope(self, objectInstance: object, attributes: object) -> None:
        self._service_event(
            "attributesInScope", {"object_handle": _handle(objectInstance)}
        )

    def attributesOutOfScope(self, objectInstance: object, attributes: object) -> None:
        self._service_event(
            "attributesOutOfScope", {"object_handle": _handle(objectInstance)}
        )

    def turnUpdatesOnForObjectInstance(
        self,
        objectInstance: object,
        attributes: object,
        updateRateDesignator: str | None = None,
    ) -> None:
        self._service_event(
            "turnUpdatesOnForObjectInstance",
            {
                "object_handle": _handle(objectInstance),
                "update_rate": updateRateDesignator,
            },
        )

    def turnUpdatesOffForObjectInstance(
        self, objectInstance: object, attributes: object
    ) -> None:
        self._service_event(
            "turnUpdatesOffForObjectInstance",
            {"object_handle": _handle(objectInstance)},
        )

    def reportFederationExecutions(self, report: object) -> None:
        self._service_event("reportFederationExecutions", _jsonable(report))

    def reportFederationExecutionMembers(
        self, federationExecutionName: str, report: object
    ) -> None:
        self._service_event(
            "reportFederationExecutionMembers",
            {"federation": federationExecutionName, "members": _jsonable(report)},
        )

    def reportFederationExecutionDoesNotExist(
        self, federationExecutionName: str
    ) -> None:
        self._service_event(
            "reportFederationExecutionDoesNotExist",
            {"federation": federationExecutionName},
        )

    def federationSynchronized(
        self, synchronizationPointLabel: str, failedToSyncSet: object
    ) -> None:
        self._service_event(
            "federationSynchronized",
            {
                "label": synchronizationPointLabel,
                "failed_federates": _handles(failedToSyncSet),
            },
        )

    def federationSaved(self) -> None:
        self._service_event("federationSaved")

    def federationNotSaved(self, reason: object) -> None:
        self._service_event("federationNotSaved", {"reason": _jsonable(reason)})

    def federationRestoreBegun(self) -> None:
        self._service_event("federationRestoreBegun")

    def federationRestored(self) -> None:
        self._service_event("federationRestored")

    def federationNotRestored(self, reason: object) -> None:
        self._service_event("federationNotRestored", {"reason": _jsonable(reason)})

    def timeRegulationEnabled(self, time: object) -> None:
        self._service_event("timeRegulationEnabled", {"logical_time": time})

    def timeConstrainedEnabled(self, time: object) -> None:
        self._service_event("timeConstrainedEnabled", {"logical_time": time})

    def timeAdvanceGrant(self, time: object) -> None:
        self._service_event("timeAdvanceGrant", {"logical_time": time})

    def requestRetraction(self, retraction: object) -> None:
        self._service_event("requestRetraction", {"retraction": _handle(retraction)})


class UmbraRtiAdapter:
    """Lifecycle adapter for an already-created Umbra Python RTI ambassador.

    Connecting and disconnecting are handled here. Joining a federation and
    declaring subscriptions remain application-owned because they require the
    caller's FOM and federation policy.
    """

    name = "umbra-python-rti"

    @property
    def capabilities(self) -> AdapterCapabilities:
        """Describe the callback surface normalized by this adapter.

        The manifest describes observer coverage, not the complete RTI API.
        ``partial`` is intentional for broad HLA service families such as MOM;
        individual providers can expose more or less through another adapter.
        """

        return {
            "lifecycle": {
                "state": "available",
                "event_types": ["runtime.started", "runtime.stopped", "runtime.error"],
            },
            "federates": {
                "state": "available",
                "event_types": [
                    "federate.joined",
                    "federate.resigned",
                    "federate.updated",
                ],
            },
            "objects": {
                "state": "available",
                "event_types": [
                    "object.discovered",
                    "object.updated",
                    "object.removed",
                ],
            },
            "attributes": {
                "state": "available",
                "event_types": ["object.updated"],
            },
            "interactions": {
                "state": "available",
                "event_types": ["interaction.received"],
            },
            "logical_time": {
                "state": "available",
                "event_types": [
                    "object.updated",
                    "object.removed",
                    "interaction.received",
                    "callback.invoked",
                ],
            },
            "mom": {
                "state": "partial",
                "event_types": ["callback.invoked"],
                "reason": "Selected management callbacks are normalized; unsupported services remain absent.",
            },
            "persistence": {
                "state": "partial",
                "event_types": ["callback.invoked"],
                "reason": "Save and restore lifecycle callbacks are normalized.",
            },
            "ownership": {
                "state": "unsupported",
                "reason": "Ownership callbacks are not normalized by this adapter yet.",
            },
            "regions": {
                "state": "unsupported",
                "reason": "Region callbacks are not normalized by this adapter yet.",
            },
        }

    def __init__(
        self,
        rti_ambassador: RTIambassador,
        *,
        callback_model: CallbackModel,
        configuration: object | None = None,
        credentials: object | None = None,
        source: str = "umbra",
        connection: str | None = None,
        federation: str | None = None,
        federate: str | None = None,
        callback: UmbraFederateAmbassador | None = None,
    ) -> None:
        self.rti_ambassador = rti_ambassador
        self.callback_model = callback_model
        self.configuration = configuration
        self.credentials = credentials
        self.callback = callback or UmbraFederateAmbassador(
            source=source,
            connection=connection,
            federation=federation,
            federate=federate,
        )
        self._sink: EventSink | None = None
        self._connected = False
        self._starting = False
        self._state_lock = threading.RLock()

    @property
    def connected(self) -> bool:
        with self._state_lock:
            return self._connected

    def start(self, sink: EventSink) -> None:
        with self._state_lock:
            if self._connected or self._starting:
                raise RuntimeError("UmbraRtiAdapter is already running")
            self._starting = True
            self._sink = sink
        try:
            self.callback.set_sink(sink)
            kwargs: RtiConnectOptions = {}
            if self.configuration is not None:
                kwargs["configuration"] = self.configuration
            if self.credentials is not None:
                kwargs["credentials"] = self.credentials
            self.rti_ambassador.connect(self.callback, self.callback_model, **kwargs)
            with self._state_lock:
                self._connected = True
            self.callback.runtime_started(adapter=self.name)
        except Exception as error:
            self.callback.runtime_error(operation="connect", error=str(error))
            # A provider may have allocated part of a session before reporting
            # a connect failure. The original connect exception stays primary.
            _best_effort(self.rti_ambassador.disconnect)
            self.callback.set_sink(None)
            with self._state_lock:
                self._sink = None
            raise
        finally:
            with self._state_lock:
                self._starting = False

    def stop(self) -> None:
        with self._state_lock:
            connected = self._connected
        if not connected:
            self.callback.set_sink(None)
            with self._state_lock:
                self._sink = None
            return
        try:
            self.rti_ambassador.disconnect()
        finally:
            with self._state_lock:
                self._connected = False
            self.callback.runtime_stopped(adapter=self.name)
            self.callback.set_sink(None)
            with self._state_lock:
                self._sink = None


__all__ = ["UmbraFederateAmbassador", "UmbraRtiAdapter"]
