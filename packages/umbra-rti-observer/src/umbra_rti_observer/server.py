"""Optional FastAPI/WebSocket transport for :mod:`umbra_rti_observer`."""

from __future__ import annotations

import asyncio
import json
from collections.abc import AsyncIterator, Mapping
from pathlib import Path
from typing import Any

from .core import OBSERVER_SCHEMA_VERSION, ObserverStore, build_event_schema

try:  # Keep the core importable when the optional web extra is absent.
    from fastapi import (
        FastAPI as _FastAPI,
        HTTPException as _HTTPException,
        Request as _Request,
        WebSocket as _WebSocket,
        WebSocketDisconnect as _WebSocketDisconnect,
    )
    from fastapi.responses import (
        HTMLResponse as _HTMLResponse,
        JSONResponse as _JSONResponse,
        StreamingResponse as _StreamingResponse,
    )
except ImportError:  # pragma: no cover - depends on optional extra
    _FastAPI = None
    _HTTPException = None
    _Request = None
    _WebSocket = None
    _WebSocketDisconnect = None
    _HTMLResponse = None
    _JSONResponse = None
    _StreamingResponse = None


def _dashboard_html() -> str:
    path = Path(__file__).with_name("static") / "dashboard.html"
    return path.read_text(encoding="utf-8")


def _event_payload(value: Any) -> list[Mapping[str, Any]]:
    if isinstance(value, Mapping) and "events" in value:
        value = value["events"]
    if isinstance(value, Mapping):
        return [value]
    if isinstance(value, list) and all(isinstance(item, Mapping) for item in value):
        return value
    raise ValueError("request body must be an event object or an events array")


def _csv_filter(value: str | None) -> list[str] | None:
    if value is None:
        return None
    values = [item.strip() for item in value.split(",") if item.strip()]
    return values or None


def create_app(store: ObserverStore | None = None) -> Any:
    """Create the dashboard app without importing FastAPI at package import time."""

    if _FastAPI is None:  # pragma: no cover - depends on optional extra
        raise RuntimeError(
            "The web dashboard requires optional dependencies; install "
            "umbra-rti-observer[web]."
        )

    observer = store or ObserverStore()
    app = _FastAPI(
        title="Umbra Runtime Observer",
        version=OBSERVER_SCHEMA_VERSION,
        description="Provider-neutral runtime event observer.",
    )

    @app.get("/", response_class=_HTMLResponse)
    async def dashboard() -> str:
        return _dashboard_html()

    @app.get("/api/health")
    async def health() -> dict[str, Any]:
        state = observer.state(include_events=False)
        return {
            "service": "umbra-rti-observer",
            "status": state["status"],
            "schema_version": OBSERVER_SCHEMA_VERSION,
            "event_count": state["live_metrics"]["event_count"],
            "retention": state["retention"],
        }

    @app.get("/api/catalog")
    async def catalog() -> dict[str, Any]:
        return {
            "schema_version": OBSERVER_SCHEMA_VERSION,
            "event_schema": build_event_schema(),
            "features": [
                "bounded event history",
                "generic object and interaction inspectors",
                "federate roster",
                "cursor-based event pages with gap detection",
                "adapter lifecycle tracking",
                "server-sent events",
                "websocket events",
            ],
        }

    @app.get("/api/schema")
    async def schema() -> dict[str, Any]:
        return build_event_schema()

    @app.get("/api/state")
    async def state(include_events: bool = True) -> dict[str, Any]:
        return observer.state(include_events=include_events)

    @app.get("/api/metrics")
    async def metrics() -> dict[str, Any]:
        state = observer.state(include_events=False)
        return {
            "schema_version": OBSERVER_SCHEMA_VERSION,
            "status": state["status"],
            "live_metrics": state["live_metrics"],
            "retention": state["retention"],
            "adapters": state["adapters"],
        }

    @app.get("/api/inspectors/{kind}")
    async def inspectors(kind: str) -> dict[str, Any]:
        state = observer.state(include_events=False)
        values = {
            "objects": state["inspectors"]["objects"],
            "interactions": state["inspectors"]["interactions"],
            "federates": state["federate_roster"],
        }
        if kind not in values:
            raise _HTTPException(status_code=404, detail="unknown inspector")
        return {"kind": kind, "items": values[kind]}

    @app.get("/api/events")
    async def events(
        after: int = 0,
        limit: int = 500,
        event_type: str | None = None,
        family: str | None = None,
        source: str | None = None,
    ) -> dict[str, Any]:
        if after < 0 or limit < 1 or limit > 2_000:
            raise _HTTPException(status_code=400, detail="after must be non-negative and limit must be between 1 and 2000")
        try:
            return observer.event_page(
                after_sequence=after,
                limit=limit,
                event_types=_csv_filter(event_type),
                families=_csv_filter(family),
                sources=_csv_filter(source),
            )
        except ValueError as error:
            raise _HTTPException(status_code=400, detail=str(error)) from error

    @app.post("/api/events")
    async def ingest(request: _Request) -> _JSONResponse:
        try:
            payload = await request.json()
            rows = _event_payload(payload)
            count = observer.publish_many(rows)
        except (ValueError, TypeError, json.JSONDecodeError) as error:
            raise _HTTPException(status_code=400, detail=str(error)) from error
        return _JSONResponse({"accepted": count, "state": observer.state(include_events=False)})

    @app.put("/api/context")
    async def context(request: _Request) -> dict[str, Any]:
        try:
            payload = await request.json()
            if not isinstance(payload, Mapping):
                raise ValueError("context must be an object")
            return {"context": observer.set_context(**dict(payload))}
        except (ValueError, TypeError, json.JSONDecodeError) as error:
            raise _HTTPException(status_code=400, detail=str(error)) from error

    @app.post("/api/control/status")
    async def status(request: _Request) -> dict[str, Any]:
        try:
            payload = await request.json()
            if not isinstance(payload, Mapping) or "status" not in payload:
                raise ValueError("status body must contain a status field")
            values = dict(payload)
            status_value = values.pop("status")
            return observer.set_status(str(status_value), **values)
        except (ValueError, TypeError, json.JSONDecodeError) as error:
            raise _HTTPException(status_code=400, detail=str(error)) from error

    @app.delete("/api/events")
    async def clear_events() -> dict[str, Any]:
        observer.clear()
        return observer.state(include_events=False)

    @app.get("/api/events/stream")
    async def event_stream(after: int = 0) -> Any:
        if after < 0:
            raise _HTTPException(status_code=400, detail="after must be non-negative")

        async def generate() -> AsyncIterator[str]:
            cursor = max(after, 0)
            while True:
                page = observer.event_page(after_sequence=cursor, limit=500)
                if page["gap"]:
                    retention = {
                        key: page[key]
                        for key in (
                            "first_retained_sequence",
                            "last_sequence",
                            "retained_events",
                            "dropped_events",
                            "cleared_events",
                        )
                    }
                    yield f"data: {json.dumps({'type': 'gap', 'retention': retention}, sort_keys=True)}\n\n"
                    cursor = max(cursor, int(page["first_retained_sequence"]) - 1)
                    continue
                rows = page["events"]
                if not rows:
                    rows = await asyncio.to_thread(
                        observer.wait_for_events,
                        after_sequence=cursor,
                        timeout=15.0,
                    )
                if not rows:
                    yield ": keepalive\n\n"
                    continue
                for row in rows:
                    cursor = max(cursor, int(row["sequence"]))
                    yield f"data: {json.dumps(row, sort_keys=True)}\n\n"

        return _StreamingResponse(
            generate(),
            media_type="text/event-stream",
            headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"},
        )

    @app.websocket("/ws/events")
    async def websocket_events(socket: _WebSocket) -> None:
        await socket.accept()
        initial_state = observer.state()
        retention = initial_state["retention"]
        cursor = int(retention["last_sequence"])
        try:
            await socket.send_json({"type": "state", "state": initial_state})
            while True:
                page = observer.event_page(after_sequence=cursor, limit=500)
                if page["gap"]:
                    retention = {
                        key: page[key]
                        for key in (
                            "first_retained_sequence",
                            "last_sequence",
                            "retained_events",
                            "dropped_events",
                            "cleared_events",
                        )
                    }
                    await socket.send_json({"type": "gap", "retention": retention})
                    cursor = max(cursor, int(page["first_retained_sequence"]) - 1)
                    continue
                rows = page["events"]
                if not rows:
                    rows = await asyncio.to_thread(
                        observer.wait_for_events,
                        after_sequence=cursor,
                        timeout=15.0,
                    )
                if rows:
                    cursor = max(cursor, int(rows[-1]["sequence"]))
                    await socket.send_json(
                        {
                            "type": "events",
                            "events": rows,
                            "state": observer.state(include_events=False),
                        }
                    )
                else:
                    await socket.send_json({"type": "heartbeat"})
        except _WebSocketDisconnect:
            return

    app.state.observer_store = observer
    return app


__all__ = ["create_app"]
