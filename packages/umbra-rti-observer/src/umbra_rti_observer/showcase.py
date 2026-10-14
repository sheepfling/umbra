"""Live provider-neutral event producer used by the observer showcase.

The showcase is intentionally an adapter-shaped producer. It exercises the
same lifecycle and capability path as a real RTI adapter without importing an
RTI or pretending that the generated events came from one.
"""

from __future__ import annotations

import threading

from .core import EventSink
from .models import JsonValue

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
        )
    }
    if profile == "limited":
        return {
            **common,
            "mom": {
                "state": "unsupported",
                "reason": "This profile represents an RTI without MOM callbacks.",
            },
            "ownership": {
                "state": "unsupported",
                "reason": "This profile does not expose ownership callbacks.",
            },
            "regions": {
                "state": "unsupported",
                "reason": "This profile does not expose region callbacks.",
            },
            "persistence": {
                "state": "disabled",
                "reason": "Save and restore observation is disabled for this profile.",
            },
        }
    if profile == "umbra":
        return {
            **common,
            "mom": {"state": "available"},
            "ownership": {"state": "available"},
            "regions": {"state": "available"},
            "persistence": {"state": "available"},
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
        if sink is None:
            return
        event = {
            "event_type": event_type,
            "source": f"showcase-{self.profile}",
            "connection": f"showcase://{self.profile}",
            "federation": "ShowcaseFederation",
            **fields,
        }
        sink(event)

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

        while not self._stop.wait(self.interval):
            self._tick += 1
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
                operation="timeAdvanceGrant",
                details={"logical_time": logical_time, "profile": self.profile},
            )
            if self._tick % 3 == 0:
                self._emit(
                    "interaction.received",
                    federate="ShowcaseServer",
                    interaction_key="interaction-1",
                    interaction_class="ExampleInteraction",
                    parameters={"message": f"heartbeat-{self._tick}", "priority": "normal"},
                    logical_time=logical_time,
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
