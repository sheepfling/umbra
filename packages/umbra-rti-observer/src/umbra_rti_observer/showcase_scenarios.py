"""Bounded deterministic examples for complex graphs and paced coordination."""

from __future__ import annotations

from .models import JsonObject, JsonValue


def topology_scenario(session: str) -> tuple[JsonObject, ...]:
    names = {"server": "ShowcaseServer", "observer": "ShowcaseObserver", "relay": "Relay",
        "processor": "Processor", "archive": "Archive", "monitor": "Monitor",
        "worker": "Worker", "worker-rejoin": "Worker", "island-a": "Remote producer", "island-b": "Remote consumer"}
    def identity(key: str) -> JsonObject:
        return {"participant_id": "worker" if key == "worker-rejoin" else key,
            "membership_id": f"{key}-join-2" if key == "worker-rejoin" else f"{key}-join-1", "display_name": names[key]}
    edges = (
        ("server", "relay", "object", "active"), ("relay", "processor", "object", "active"),
        ("processor", "archive", "object", "active"), ("processor", "observer", "interaction", "active"),
        ("observer", "server", "interaction", "active"), ("server", "monitor", "interaction", "active"),
        ("server", "worker", "object", "ended"), ("server", "worker-rejoin", "object", "active"),
        ("worker-rejoin", "relay", "interaction", "unknown"), ("monitor", "monitor", "interaction", "active"),
        ("relay", "server", "interaction", "active"), ("server", "relay", "interaction", "active"),
        ("island-a", "island-b", "object", "unknown"),
    )
    return tuple({"relationship_id": f"mesh-{index}", "session_id": session,
        "identity_namespace": f"showcase:{session}", "federation_id": "showcase-federation",
        "producer": identity(producer), "recipient": identity(recipient), "kind": kind,
        "evidence": "observed" if index % 3 == 0 else "declared", "status": status, "viewpoint": "observer",
        "class_key": "ExampleClass" if kind == "object" else "ExampleInteraction",
        "reason": "Explicit synthetic mesh: cycles, fan-out, parallel reports, a self-loop, rejoin and disconnected group."}
        for index, (producer, recipient, kind, status) in enumerate(edges))


def coordination_scenario(session: str, tick: int) -> tuple[JsonObject, ...]:
    cycle, phase = divmod(tick, 40)
    subject: JsonObject = {"participant_id": "server", "membership_id": "server-join-1", "display_name": "ShowcaseServer"}
    peer: JsonObject = {"participant_id": "peer", "membership_id": "peer-join-1", "display_name": "ShowcasePeer"}
    rows: list[JsonObject] = []
    def report(kind: str, operation: str, label: str, state: str, *, local: bool = False,
               participants: list[JsonValue] | None = None, reason: str | None = None) -> None:
        row: JsonObject = {"kind": kind, "operation_id": f"cycle-{cycle}-{operation}", "label": label,
            "session_id": session, "identity_namespace": f"showcase:{session}", "federation_id": "showcase-federation",
            "phase": state, "scope": "local" if local else "federation"}
        if local:
            row["subject"] = subject
        if participants is not None:
            row["participants"] = participants
        if reason:
            row["reason"] = reason
        rows.append(row)
    if phase == 0:
        report("synchronization", "sync", "Paced startup barrier", "requested", local=True)
    elif phase == 1:
        report("synchronization", "sync", "Paced startup barrier", "registered", local=True)
    elif phase == 3:
        report("synchronization", "sync", "Paced startup barrier", "announced",
            participants=[{"identity": subject, "status": "pending"}, {"identity": peer, "status": "unknown"}])
    elif phase == 6:
        report("synchronization", "sync", "Paced startup barrier", "achieved", local=True)
    elif phase == 12:
        report("synchronization", "sync", "Paced startup barrier", "completed",
            participants=[{"identity": subject, "status": "achieved"}, {"identity": peer, "status": "achieved"}])
    elif phase == 15:
        report("synchronization", "rejected", "Rejected duplicate barrier", "registration-failed", local=True,
            reason="Synthetic registration rejection; no announcement or achievement is invented.")
    elif phase == 18:
        report("save", "save", "Paced checkpoint", "requested")
    elif phase == 20:
        report("save", "save", "Paced checkpoint", "begun", local=True)
    elif phase == 24:
        report("save", "save", "Paced checkpoint", "completed", local=True)
    elif phase == 28:
        failed = cycle % 2 == 0
        report("save", "save", "Paced checkpoint", "failed" if failed else "completed",
            participants=[{"identity": subject, "status": "completed"}, {"identity": peer, "status": "failed" if failed else "completed"}],
            reason="Synthetic peer failure despite local completion." if failed else None)
    elif phase == 30:
        report("restore", "restore", "Paced restore", "requested")
    elif phase == 32:
        report("restore", "restore", "Paced restore", "begun",
            participants=[{"identity": subject, "status": "begun"}, {"identity": peer, "status": "unknown"}])
    elif phase == 34:
        report("restore", "restore", "Paced restore", "completed", local=True)
    elif phase == 36:
        failed = cycle % 2 == 1
        report("restore", "restore", "Paced restore", "failed" if failed else "completed",
            participants=[{"identity": subject, "status": "completed"}, {"identity": peer, "status": "failed" if failed else "completed"}])
    return tuple(rows)
