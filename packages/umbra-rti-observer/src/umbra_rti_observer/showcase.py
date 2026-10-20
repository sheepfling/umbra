"""Live provider-neutral event producer used by the observer showcase.

The showcase is intentionally an adapter-shaped producer. It exercises the
same lifecycle and capability path as a real RTI adapter without importing an
RTI or pretending that the generated events came from one.
"""

from __future__ import annotations

import threading
from uuid import uuid4

from .core import EventSink
from .models import JsonValue
from .showcase_scenarios import coordination_scenario, topology_scenario

ShowcaseProfile = str


def _profile_capabilities(profile: ShowcaseProfile) -> dict[str, dict[str, JsonValue]]:
    common: dict[str, dict[str, JsonValue]] = {
        key: {"state": "available"}
        for key in (
            "lifecycle",
            "federates",
            "objects",
            "attributes",
            "interactions",
            "logical_time",
            "declarations",
        )
    }
    if profile == "limited":
        return {
            **common,
            "time_inspection": {"state": "partial", "reason": "Grants only; request IDs and time-role state are not reported."},
            "declarations": {"state": "partial", "reason": "Only an object subscription is reported; publication and interaction declarations are unknown."},
            "relationships": {"state": "unsupported", "reason": "This profile reports no explicit producer/recipient relationships or stable join IDs."},
            "synchronization": {"state": "unsupported", "reason": "This profile does not report synchronization phases."},
            "persistence_inspection": {"state": "disabled", "reason": "Save/restore inspection is disabled for this profile."},
            "interaction_flow": {"state": "partial", "reason": "Receive observations only; transmission and endpoint IDs are not reported."},
            "mom": {
                "state": "unsupported",
                "reason": "This profile represents an RTI without MOM callbacks.",
            },
            "mom_inspection": {"state": "unsupported", "reason": "This profile reports no typed management reports; ordinary observation and join controls remain available."},
            "fom_inspection": {"state": "partial", "reason": "Only object-class metadata is supplied; ancestry and datatype references may be unresolved."},
            "ownership": {
                "state": "unsupported",
                "reason": "This profile does not expose ownership callbacks.",
            },
            "ownership_inspection": {"state": "unsupported", "reason": "This profile reports no explicit owner or ownership request evidence."},
            "regions": {
                "state": "unsupported",
                "reason": "This profile does not expose region callbacks.",
            },
            "regions_inspection": {"state": "unsupported", "reason": "This profile reports no region extents, associations or routing evidence."},
            "persistence": {
                "state": "disabled",
                "reason": "Save and restore observation is disabled for this profile.",
            },
        }
    if profile == "umbra":
        return {
            **common,
            "relationships": {"state": "available", "reason": "Synthetic explicit edges; not inferred from declarations or delivery."},
            "mom": {"state": "available"},
            "mom_inspection": {"state": "available"},
            "fom_inspection": {"state": "available"},
            "ownership": {"state": "available"},
            "ownership_inspection": {"state": "available"},
            "regions": {"state": "available"},
            "regions_inspection": {"state": "available"},
            "persistence": {"state": "available"},
            "persistence_inspection": {"state": "available"},
        }
    raise ValueError("showcase profile must be 'umbra' or 'limited'")


class ShowcaseAdapter:
    """Emit a paced, repeatable event stream for a live dashboard walkthrough."""

    name = "showcase"

    def __init__(
        self,
        *,
        profile: ShowcaseProfile = "umbra",
        interval: float = 1.5,
    ) -> None:
        self.profile = profile
        self.interval = max(float(interval), 0.05)
        self._sink: EventSink | None = None
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._lock = threading.RLock()
        self._tick = 0
        self._session_id = uuid4().hex

        # Validate early so a typo cannot leave a dashboard that looks live but
        # has no meaningful capability declaration.
        _profile_capabilities(profile)

    @property
    def capabilities(self) -> dict[str, dict[str, JsonValue]]:
        return _profile_capabilities(self.profile)

    @property
    def running(self) -> bool:
        with self._lock:
            return self._thread is not None and self._thread.is_alive()

    def start(self, sink: EventSink) -> None:
        with self._lock:
            if self._thread is not None and self._thread.is_alive():
                raise RuntimeError("ShowcaseAdapter is already running")
            self._sink = sink
            self._stop.clear()
            self._tick = 0
            self._session_id = uuid4().hex
            self._thread = threading.Thread(target=self._run, name="umbra-observer-showcase", daemon=True)
            thread = self._thread
            thread.start()

    def stop(self) -> None:
        with self._lock:
            thread = self._thread
            self._stop.set()
            self._sink = None
        if thread is not None and thread is not threading.current_thread():
            thread.join(timeout=max(self.interval * 2, 1.0))
        with self._lock:
            self._thread = None

    def _emit(self, event_type: str, **fields: JsonValue) -> None:
        with self._lock:
            sink = self._sink
            session = self._session_id
        if sink is None:
            return
        event = {
            "event_type": event_type,
            "family": event_type.partition(".")[0],
            "source": f"showcase-{self.profile}",
            "session_id": session,
            "connection": f"showcase://{self.profile}",
            "federation": "ShowcaseFederation",
            **fields,
        }
        sink(event)

    def _ownership(self, kind: str, **fields: JsonValue) -> None:
        self._emit("ownership.observed", ownership={
            "kind": kind, "session_id": self._session_id, "identity_namespace": f"showcase:{self._session_id}",
            "federation_id": "showcase-federation", "object_id": "object-1-instance-1", "attribute_key": "load", **fields,
        })

    def _region(self, kind: str, **fields: JsonValue) -> None:
        self._emit("region.observed", region_observation={
            "kind": kind, "session_id": self._session_id, "identity_namespace": f"showcase:{self._session_id}",
            "federation_id": "showcase-federation", "evidence": "declared", "status": "active", **fields,
        })

    def _mom(self, kind: str, report_id: str, **fields: JsonValue) -> None:
        self._emit("mom.observed", mom_observation={
            "kind": kind, "report_id": report_id, "session_id": self._session_id, "identity_namespace": f"showcase:{self._session_id}",
            "federation_id": "showcase-federation", **fields,
        })

    def _fom(self, kind: str, element_id: str, **fields: JsonValue) -> None:
        self._emit("fom.observed", fom_observation={
            "kind": kind, "element_id": element_id, "model_id": "showcase-model", "revision": "1", "status": "present",
            "session_id": self._session_id, "identity_namespace": f"showcase:{self._session_id}", "federation_id": "showcase-federation", **fields,
        })

    def _run(self) -> None:
        self._emit("runtime.started", details={"profile": self.profile})
        self._emit(
            "federate.joined",
            federate="ShowcaseServer",
            role="producer",
            details={"profile": self.profile},
        )
        self._emit(
            "federate.joined",
            federate="ShowcaseObserver",
            role="observer",
            details={"profile": self.profile},
        )
        self._emit(
            "object.discovered",
            federate="ShowcaseServer",
            object_key="object-1",
            object_name="ExampleObject",
            class_name="ExampleClass",
            attributes={"state": "ready", "load": 0.42},
        )
        self._emit(
            "interaction.received",
            federate="ShowcaseServer",
            interaction_key="interaction-1",
            interaction_class="ExampleInteraction",
            parameters={"message": "showcase online", "priority": "normal"},
        )
        self._fom("object-class", "ExampleClass", name="ExampleClass", parent_state="parent" if self.profile == "umbra" else "unknown",
                  **({"parent_id": "ExampleBase"} if self.profile == "umbra" else {}), field_scope="declared",
                  attributes=[{"field_id": "load", "name": "load", "datatype_id": "ExampleNumber"}, {"field_id": "state", "name": "state", "datatype_id": "ExampleText"}])
        if self.profile == "umbra":
            self._fom("object-class", "ExampleBase", name="ExampleBase", parent_state="root", field_scope="declared", attributes=[])
            self._fom("interaction-class", "ExampleInteraction", name="ExampleInteraction", parent_state="root", field_scope="declared", parameters=[{"field_id": "message", "name": "message", "datatype_id": "ExampleText"}])
            self._fom("datatype", "ExampleNumber", name="ExampleNumber", encoding="primitive", representation="Provider-supplied floating-point representation")
            self._fom("datatype", "ExampleText", name="ExampleText", encoding="opaque", representation="Provider-supplied text representation")
            self._fom("datatype", "ExampleArray", name="ExampleArray", encoding="array", element_type_id="ExampleNumber", cardinality="4")
            self._fom("datatype", "ExampleEnum", name="ExampleEnum", encoding="enumeration", enumerators=[{"name": "Off", "value": "0"}, {"name": "On", "value": "1"}])
            self._fom("dimension", "sector", name="sector", upper_bound="1000", unit="provider-domain")
            self._mom("members", "reported-members", coverage="partial", members=[
                {"participant": {"identity": {"participant_id": "server", "membership_id": "server-join-1", "display_name": "ShowcaseServer"}}, "status": "joined"},
                {"participant": {"display_name": "Unresolved peer", "handle": "0x45"}, "status": "unknown"},
            ], reason="Synthetic partial management report, independent of the ordinary observed roster.")
            self._mom("service", "synthetic-service-1", service_name="ExampleService", invocation_id="invoke-1", status="failed", error_name="SyntheticServiceError", reason="Explicit synthetic service report; no command was dispatched.")
            self._mom("counter", "traffic-counter", counter_key="observer-reports", semantics="cumulative", value=0, unit="reports")
            self._mom("counter", "opaque-counter", counter_key="provider-wide-integer", semantics="unknown", value_text="18446744073709551615", unit="provider units")
            self._region("region", region_id="window-1-instance-1", extents=[{"extent_id": "e1", "dimensions": [
                {"dimension_id": "sector", "lower_bound": "0", "upper_bound": "250", "dimension_upper_bound": "1000"},
                {"dimension_id": "channel", "lower_bound": "9007199254740993", "upper_bound": "9007199254741093", "dimension_upper_bound": "18446744073709551615"},
            ]}])
            self._region("association", association_id="object-window", target_kind="object", target_id="object-1-instance-1", direction="update", region_ids=["window-1-instance-1"], attribute_keys=["load"])
            self._region("routing", routing_id="window-observation", evidence="observed", region_ids=["window-1-instance-1"], target_kind="object", target_id="object-1-instance-1",
                         producer={"participant_id": "server", "membership_id": "server-join-1", "display_name": "ShowcaseServer"},
                         recipient={"participant_id": "observer", "membership_id": "observer-join-1", "display_name": "ShowcaseObserver"}, reason="Explicit synthetic routing observation, not delivery proof or an inferred overlap.")
            self._ownership("state", owner_state="known", owner={"participant_id": "server", "membership_id": "server-join-1", "display_name": "ShowcaseServer"}, transition="reported")
            self._ownership("state", attribute_key="state", owner_state="unknown", transition="reported", reason="This attribute owner has not been queried.")
            self._ownership("request", request_id="transfer-1", action="acquire", requester={"participant_id": "observer", "membership_id": "observer-join-1", "display_name": "ShowcaseObserver"})
            self._emit("time.observed", federate="ShowcaseServer", time_observation={
                "kind": "state", "session_id": self._session_id, "lookahead": 0.5,
                "regulating": True, "constrained": True,
            })
            self._emit("time.observed", federate="ShowcaseServer", time_observation={
                "kind": "request", "session_id": self._session_id, "request_id": "advance-1", "logical_time": 0.5,
            })
            for kind, class_key in (("object", "ExampleClass"), ("interaction", "ExampleInteraction")):
                for direction, federate in (("publication", "ShowcaseServer"), ("subscription", "ShowcaseObserver")):
                    declaration: dict[str, JsonValue] = {
                        "kind": kind, "class_key": class_key, "class_name": class_key,
                        "direction": direction, "status": "declared", "session_id": self._session_id,
                    }
                    if kind == "object":
                        declaration.update(attribute_scope="selected", attributes=["state", "load"])
                    self._emit("declaration.observed", federate=federate, declaration=declaration)
            for kind, class_key in (("object", "ExampleClass"), ("interaction", "ExampleInteraction")):
                self._emit("relationship.observed", relationship={
                    "relationship_id": f"showcase-{kind}", "session_id": self._session_id,
                    "identity_namespace": f"showcase:{self._session_id}", "federation_id": "showcase-federation",
                    "producer": {"participant_id": "server", "membership_id": "server-join-1", "display_name": "ShowcaseServer"},
                    "recipient": {"participant_id": "observer", "membership_id": "observer-join-1", "display_name": "ShowcaseObserver"},
                    "kind": kind, "evidence": "declared", "status": "active", "viewpoint": "observer",
                    "class_key": class_key, "class_name": class_key, "reason": "Explicit synthetic adapter relationship, not a delivery receipt.",
                })
            for relationship in topology_scenario(self._session_id):
                self._emit("relationship.observed", relationship=relationship)
            for observation in coordination_scenario(self._session_id, 0):
                self._emit("coordination.observed", coordination=observation)
            subject: dict[str, JsonValue] = {"participant_id": "server", "membership_id": "server-join-1", "display_name": "ShowcaseServer"}
            phases: tuple[tuple[str, str, str, str, str, dict[str, JsonValue]], ...] = (
                ("synchronization", "startup-sync", "Ready to run", "requested", "local", {"subject": subject}),
                ("synchronization", "startup-sync", "Ready to run", "registered", "local", {"subject": subject}),
                ("synchronization", "startup-sync", "Ready to run", "announced", "federation", {}),
                ("synchronization", "startup-sync", "Ready to run", "achieved", "local", {"subject": subject}),
                ("synchronization", "startup-sync", "Ready to run", "completed", "federation", {"participants": [{"identity": subject, "status": "achieved"}]}),
                ("save", "checkpoint", "Demo checkpoint", "requested", "federation", {}),
                ("save", "checkpoint", "Demo checkpoint", "begun", "local", {"subject": subject}),
                ("save", "checkpoint", "Demo checkpoint", "completed", "local", {"subject": subject}),
                ("save", "checkpoint", "Demo checkpoint", "failed", "federation", {"reason": "Synthetic peer save failure; local completion is not collective completion.", "participants": [{"identity": {"participant_id": "peer", "membership_id": "peer-join-1", "display_name": "ShowcasePeer"}, "status": "failed", "reason": "Synthetic storage unavailable"}]}),
                ("restore", "restore-demo", "Demo restore", "requested", "federation", {}),
                ("restore", "restore-demo", "Demo restore", "begun", "federation", {}),
                ("restore", "restore-demo", "Demo restore", "completed", "federation", {}),
            )
            for kind, operation, label, phase, scope, fields in phases:
                self._emit("coordination.observed", coordination={
                    "kind": kind, "operation_id": operation, "label": label, "phase": phase, "scope": scope,
                    "session_id": self._session_id, "identity_namespace": f"showcase:{self._session_id}",
                    "federation_id": "showcase-federation", **fields,
                })
        else:
            self._emit("declaration.observed", federate="ShowcaseObserver", declaration={
                "kind": "object", "class_key": "ExampleClass", "class_name": "ExampleClass",
                "direction": "subscription", "status": "declared", "session_id": self._session_id,
                "attribute_scope": "unknown", "reason": "The selected attribute set is not exposed by this profile.",
            })

        while not self._stop.wait(self.interval):
            self._tick += 1
            if self.profile == "umbra":
                for observation in coordination_scenario(self._session_id, self._tick):
                    self._emit("coordination.observed", coordination=observation)
                self._mom("counter", "traffic-counter", counter_key="observer-reports", semantics="cumulative", value=self._tick, unit="reports")
                if self._tick % 8 == 0:
                    lower = (self._tick % 20) * 25
                    self._region("region", region_id="window-1-instance-1", extents=[{"extent_id": "e1", "dimensions": [
                        {"dimension_id": "sector", "lower_bound": str(lower), "upper_bound": str(lower + 250), "dimension_upper_bound": "1000"},
                    ]}])
                observer_identity: dict[str, JsonValue] = {"participant_id": "observer", "membership_id": "observer-join-1", "display_name": "ShowcaseObserver"}
                if self._tick == 3:
                    self._ownership("outcome", request_id="transfer-1", action="acquire", requester=observer_identity, outcome="completed")
                    self._ownership("state", owner_state="known", owner=observer_identity, transition="acquired")
                elif self._tick == 6:
                    self._ownership("request", request_id="release-1", action="divest", requester=observer_identity)
                elif self._tick == 9:
                    self._ownership("outcome", request_id="release-1", action="divest", requester=observer_identity, outcome="failed", reason="Synthetic rejection; last reported owner is unchanged.")
            if self._tick > 1 and self._tick % 17 == 1:
                self._emit("runtime.started", details={"synthetic": True, "message": "Showcase adapter recovered"})
            logical_time = round(self._tick * 0.5, 3)
            self._emit(
                "object.updated",
                federate="ShowcaseServer",
                object_key="object-1",
                object_name="ExampleObject",
                class_name="ExampleClass",
                attributes={
                    "state": "ready" if self._tick % 4 else "checkpoint",
                    "load": round(0.42 + ((self._tick % 5) * 0.07), 3),
                },
                logical_time=logical_time,
            )
            self._emit(
                "callback.invoked",
                federate="ShowcaseServer",
                operation="timeAdvanceGrant",
                details={"logical_time": logical_time, "profile": self.profile},
                **({"time_observation": {"kind": "grant", "session_id": self._session_id,
                    "request_id": f"advance-{self._tick}", "logical_time": logical_time}} if self.profile == "umbra" else {}),
            )
            if self.profile == "umbra":
                self._emit("time.observed", federate="ShowcaseServer", time_observation={
                    "kind": "request", "session_id": self._session_id,
                    "request_id": f"advance-{self._tick + 1}", "logical_time": logical_time + 0.5,
                })
            if self._tick % 3 == 0:
                flow: dict[str, JsonValue] = {
                    "order": "timestamped" if self._tick % 2 else "receive-order",
                    "message_id": f"heartbeat-{self._tick}", "identity_namespace": f"showcase:{self._session_id}",
                    "federation_id": "showcase-federation", "session_id": self._session_id, "class_key": "ExampleInteraction",
                    "producer": {"participant_id": "server", "membership_id": "server-join-1", "display_name": "ShowcaseServer"},
                    "recipient": {"participant_id": "observer", "membership_id": "observer-join-1", "display_name": "ShowcaseObserver"},
                }
                if self._tick % 2:
                    flow["logical_time"] = logical_time
                if self.profile == "umbra":
                    self._emit("interaction.sent", federate="ShowcaseServer", interaction_class="ExampleInteraction",
                               interaction_flow={**flow, "kind": "send"})
                self._emit(
                    "interaction.received",
                    federate="ShowcaseObserver",
                    interaction_key="interaction-1",
                    interaction_class="ExampleInteraction",
                    parameters={"message": f"heartbeat-{self._tick}", "priority": "normal"},
                    logical_time=logical_time,
                    **({"interaction_flow": {**flow, "kind": "receive"}} if self.profile == "umbra" else {}),
                )
            self._emit(
                "callback.invoked",
                federate="ShowcaseObserver",
                operation="timeAdvanceGrant",
                logical_time=round(max(0, logical_time - (self._tick % 4) * 0.15), 3),
            )
            if self._tick % 2 == 0:
                self._emit(
                    "object.updated", federate="ShowcaseObserver",
                    object_key="object-1", object_name="ExampleObject", class_name="ExampleClass",
                    attributes={"load": round(0.42 + ((self._tick % 5) * 0.07), 3)},
                    details={"observation": "subscriber reflection"},
                )
            if self._tick % 17 == 0:
                self._emit(
                    "runtime.error", federate="ShowcaseServer",
                    operation="publishTelemetry",
                    details={"message": "Synthetic showcase warning: retry succeeded", "synthetic": True},
                )
            if self.profile == "umbra" and self._tick % 5 == 0:
                self._emit(
                    "callback.invoked",
                    operation="reportFederationExecutionMembers",
                    details={
                        "federation": "ShowcaseFederation",
                        "members": ["ShowcaseServer", "ShowcaseObserver"],
                    },
                )
                self._emit(
                    "callback.invoked",
                    operation="federationSynchronized",
                    details={"label": "ShowcaseReady", "failed_federates": []},
                )


__all__ = ["ShowcaseAdapter", "ShowcaseProfile"]
