"""Translate Umbra's standard Python RTI callback shape into generic events."""

from __future__ import annotations

import json
import threading
from collections.abc import Callable, Mapping
from typing import Any

from hla.rti1516_2025 import FederateAmbassador
from umbra_rti_observer import EventSink


def _jsonable(value: Any) -> Any:
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, (bytes, bytearray, memoryview)):
        return {"encoding": "hex", "value": bytes(value).hex()}
    if isinstance(value, Mapping):
        return {str(key): _jsonable(item) for key, item in value.items()}
    if isinstance(value, (list, tuple, set, frozenset)):
        return [_jsonable(item) for item in value]
    try:
        json.dumps(value)
    except (TypeError, ValueError, OverflowError):
        return str(value)
    return value


def _handle(value: Any) -> str | None:
    if value is None:
        return None
    for attribute in ("encodedValue", "value", "handle"):
        try:
            candidate = getattr(value, attribute, None)
        except Exception:
            continue
        if candidate is None or callable(candidate):
            continue
        if isinstance(candidate, (bytes, bytearray, memoryview)):
            return f"0x{bytes(candidate).hex()}"
        try:
            text = str(candidate).strip()
        except Exception:
            continue
        if text:
            return text
    try:
        text = str(value).strip()
    except Exception:
        return None
    return text or None


def _mapping(value: Any) -> dict[str, Any]:
    if isinstance(value, Mapping):
        return {(_handle(key) or str(key)): _jsonable(item) for key, item in value.items()}
    items = getattr(value, "items", None)
    if callable(items):
        try:
            return {(_handle(key) or str(key)): _jsonable(item) for key, item in items()}
        except Exception:
            pass
    return {}


def _handles(value: Any) -> list[str | None]:
    if value is None:
        return []
    try:
        return [_handle(item) for item in value]
    except Exception:
        return []


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

    def _emit(self, event_type: str, **fields: Any) -> None:
        with self._sink_lock:
            sink = self._sink
        if sink is None:
            return
        event: dict[str, Any] = {
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
        event.update({key: _jsonable(value) for key, value in fields.items() if value is not None})
        try:
            sink(event)
        except Exception:
            # Dashboard delivery is diagnostic and must not change callback
            # behavior in an RTI federate when its sink is unavailable.
            return

    def runtime_started(self, **details: Any) -> None:
        self._emit("runtime.started", details=details)

    def runtime_stopped(self, **details: Any) -> None:
        self._emit("runtime.stopped", details=details)

    def runtime_error(self, **details: Any) -> None:
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
        objectInstance: Any,
        objectClass: Any,
        objectInstanceName: str,
        producingFederate: Any,
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
        objectInstance: Any,
        userSuppliedTag: Any,
        producingFederate: Any,
        time: Any = None,
        sentOrderType: Any = None,
        receivedOrderType: Any = None,
        retraction: Any = None,
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
        objectInstance: Any,
        attributeValues: Any,
        userSuppliedTag: Any,
        transportationType: Any,
        producingFederate: Any,
        sentRegions: Any = None,
        time: Any = None,
        sentOrderType: Any = None,
        receivedOrderType: Any = None,
        retraction: Any = None,
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
        interactionClass: Any,
        parameterValues: Any,
        userSuppliedTag: Any,
        transportationType: Any,
        producingFederate: Any,
        sentRegions: Any = None,
        time: Any = None,
        sentOrderType: Any = None,
        receivedOrderType: Any = None,
        retraction: Any = None,
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
        interactionClass: Any,
        objectInstance: Any,
        parameterValues: Any,
        userSuppliedTag: Any,
        transportationType: Any,
        producingFederate: Any,
        time: Any = None,
        sentOrderType: Any = None,
        receivedOrderType: Any = None,
        retraction: Any = None,
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

    def turnInteractionsOn(self, interactionClass: Any) -> None:
        self._service_event("turnInteractionsOn", {"interaction_handle": _handle(interactionClass)})

    def turnInteractionsOff(self, interactionClass: Any) -> None:
        self._service_event("turnInteractionsOff", {"interaction_handle": _handle(interactionClass)})

    def _service_event(self, operation: str, value: Any = None) -> None:
        self._emit(
            "callback.invoked",
            operation=operation,
            details={"value": _jsonable(value)} if value is not None else {},
        )

    def provideAttributeValueUpdate(self, objectInstance: Any, attributes: Any, userSuppliedTag: Any) -> None:
        self._service_event(
            "provideAttributeValueUpdate",
            {"object_handle": _handle(objectInstance), "attributes": _handles(attributes), "tag": userSuppliedTag},
        )

    def attributesInScope(self, objectInstance: Any, attributes: Any) -> None:
        self._service_event("attributesInScope", {"object_handle": _handle(objectInstance)})

    def attributesOutOfScope(self, objectInstance: Any, attributes: Any) -> None:
        self._service_event("attributesOutOfScope", {"object_handle": _handle(objectInstance)})

    def turnUpdatesOnForObjectInstance(self, objectInstance: Any, attributes: Any, updateRateDesignator: str | None = None) -> None:
        self._service_event(
            "turnUpdatesOnForObjectInstance",
            {"object_handle": _handle(objectInstance), "update_rate": updateRateDesignator},
        )

    def turnUpdatesOffForObjectInstance(self, objectInstance: Any, attributes: Any) -> None:
        self._service_event("turnUpdatesOffForObjectInstance", {"object_handle": _handle(objectInstance)})

    def reportFederationExecutions(self, report: Any) -> None:
        self._service_event("reportFederationExecutions", _jsonable(report))

    def reportFederationExecutionMembers(self, federationExecutionName: str, report: Any) -> None:
        self._service_event(
            "reportFederationExecutionMembers",
            {"federation": federationExecutionName, "members": _jsonable(report)},
        )

    def reportFederationExecutionDoesNotExist(self, federationExecutionName: str) -> None:
        self._service_event(
            "reportFederationExecutionDoesNotExist",
            {"federation": federationExecutionName},
        )

    def federationSynchronized(self, synchronizationPointLabel: str, failedToSyncSet: Any) -> None:
        self._service_event(
            "federationSynchronized",
            {
                "label": synchronizationPointLabel,
                "failed_federates": _handles(failedToSyncSet),
            },
        )

    def federationSaved(self) -> None:
        self._service_event("federationSaved")

    def federationNotSaved(self, reason: Any) -> None:
        self._service_event("federationNotSaved", {"reason": _jsonable(reason)})

    def federationRestoreBegun(self) -> None:
        self._service_event("federationRestoreBegun")

    def federationRestored(self) -> None:
        self._service_event("federationRestored")

    def federationNotRestored(self, reason: Any) -> None:
        self._service_event("federationNotRestored", {"reason": _jsonable(reason)})

    def timeRegulationEnabled(self, time: Any) -> None:
        self._service_event("timeRegulationEnabled", {"logical_time": time})

    def timeConstrainedEnabled(self, time: Any) -> None:
        self._service_event("timeConstrainedEnabled", {"logical_time": time})

    def timeAdvanceGrant(self, time: Any) -> None:
        self._service_event("timeAdvanceGrant", {"logical_time": time})

    def requestRetraction(self, retraction: Any) -> None:
        self._service_event("requestRetraction", {"retraction": _handle(retraction)})


class UmbraRtiAdapter:
    """Lifecycle adapter for an already-created Umbra Python RTI ambassador.

    Connecting and disconnecting are handled here. Joining a federation and
    declaring subscriptions remain application-owned because they require the
    caller's FOM and federation policy.
    """

    name = "umbra-python-rti"

    def __init__(
        self,
        rti_ambassador: Any,
        *,
        callback_model: Any,
        configuration: Any = None,
        credentials: Any = None,
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
            kwargs: dict[str, Any] = {}
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
            try:
                # A provider may have allocated part of a session before
                # reporting a connect failure. Disconnect is best-effort here;
                # the original connect exception remains authoritative.
                self.rti_ambassador.disconnect()
            except Exception:
                pass
            try:
                self.callback.set_sink(None)
            except Exception:
                pass
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
