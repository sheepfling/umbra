"""Optional FastAPI/WebSocket transport for :mod:`umbra_rti_observer`."""

from __future__ import annotations

import asyncio
import json
import os
from collections.abc import AsyncIterator, Awaitable, Callable, Mapping
from contextlib import asynccontextmanager, suppress
from pathlib import Path

from .contract import (
    OBSERVER_CONTRACT_VERSION,
    OBSERVER_JSON_MEDIA_TYPE,
    build_contract_document,
)
from .core import (
    EVENT_TYPES,
    OBSERVER_SCHEMA_VERSION,
    ObserverStore,
    build_event_schema,
)
from .models import JsonObject

try:  # The web extra is optional; only importing this transport requires it.
    from fastapi import (
        FastAPI,
        HTTPException,
        Query,
        Request,
        WebSocket,
        WebSocketDisconnect,
    )
    from fastapi.responses import HTMLResponse, StreamingResponse
    from fastapi.staticfiles import StaticFiles
    from pydantic import BaseModel
    from starlette.responses import Response

    from .analytics import build_analytics
    from .command_models import (
        CommandCatalog,
        CommandHistory,
        CommandRecord,
        CommandRequest,
    )
    from .commands import CommandConflict, CommandDispatcher
    from .schemas import (
        AnalyticsResponse,
        AnalyticsWindow,
        CapabilityResponse,
        CatalogResponse,
        ContextRequest,
        ContextResponse,
        ContractResponse,
        EventBatchRequest,
        EventIngestRequest,
        EventPageResponse,
        EventsMessage,
        GapMessage,
        HealthResponse,
        HeartbeatMessage,
        IngestResponse,
        InspectorResponse,
        JsonObjectResponse,
        MetricsResponse,
        RetentionGapResponse,
        RuntimeSummaryResponse,
        SnapshotMessage,
        StateResponse,
        StatusRequest,
        event_input_mapping,
        json_model,
    )
except ImportError as error:  # pragma: no cover - depends on optional extra
    raise RuntimeError(
        "The observer web transport requires optional dependencies; install "
        "umbra-rti-observer[web]."
    ) from error


def _react_ui_dist() -> Path | None:
    configured = os.environ.get("UMBRA_OBSERVER_UI_DIST")
    candidates = [Path(configured)] if configured else []
    repository_root = Path(__file__).resolve().parents[4]
    candidates.append(repository_root / "packages" / "umbra-rti-observer-ui" / "dist")
    for candidate in candidates:
        if candidate.is_dir() and (candidate / "index.html").is_file():
            return candidate
    return None


def _dashboard_html() -> str:
    react_index = _react_ui_dist()
    if react_index is not None:
        return (react_index / "index.html").read_text(encoding="utf-8")
    path = Path(__file__).with_name("static") / "dashboard.html"
    return path.read_text(encoding="utf-8")


def _event_payload(value: EventIngestRequest) -> list[Mapping[str, object]]:
    if isinstance(value, EventBatchRequest):
        return [event_input_mapping(event) for event in value.events]
    return [event_input_mapping(value)]


def _csv_filter(value: str | None) -> list[str] | None:
    if value is None:
        return None
    values = [item.strip() for item in value.split(",") if item.strip()]
    return values or None


def _validated_state(observer: ObserverStore, *, include_events: bool) -> StateResponse:
    return StateResponse.model_validate(observer.state(include_events=include_events))


def _validated_summary(observer: ObserverStore) -> RuntimeSummaryResponse:
    return RuntimeSummaryResponse.model_validate(observer.runtime_summary())


def _stream_json(message: BaseModel) -> str:
    return json.dumps(message.model_dump(mode="json", exclude_none=True), sort_keys=True)


def _validated_capabilities(observer: ObserverStore) -> dict[str, CapabilityResponse]:
    return {
        name: CapabilityResponse.model_validate(value)
        for name, value in observer.capabilities().items()
    }


def create_app(
    store: ObserverStore | None = None, *, enable_analytics: bool = True,
    commands: CommandDispatcher | None = None,
    enable_recording: bool = True,
    enable_alerts: bool = True,
    enable_alert_controls: bool = True,
    enable_workspace_validation: bool = True,
) -> FastAPI:
    """Create the optional FastAPI transport over an observer store."""

    observer = store or ObserverStore()
    observer.set_capabilities(
        {
            "workspace_validation": {
                "state": "available" if enable_workspace_validation else "disabled",
                "reason": "Optional named workspace configuration validation; local browser settings do not require an RTI or this resource.",
            },
            "alert_controls": {
                "state": "available" if enable_alerts and enable_alert_controls else "disabled",
                "reason": "Observer rule configuration only; no provider commands."
                if enable_alerts and enable_alert_controls else "Alert configuration is read-only or alerts are disabled.",
            },
            "alerts": {
                "state": "available" if enable_alerts else "disabled",
                "reason": "Typed thresholds with bounded process-local configuration/evidence; no RTI service dependency."
                if enable_alerts else "Alert resources and periodic evaluation are disabled by this backend.",
            },
            "recording_compact": {
                "state": "available" if enable_recording else "disabled",
                "reason": "Negotiated v2 baseline/edit files with bounded reconstructed state/work; v1 remains supported.",
            },
            "recording_analytics_window": {
                "state": "available" if enable_recording else "disabled",
                "reason": "Named validated retained-window requests; legacy complete-frame requests remain accepted.",
            },
            "recording": {
                "state": "available" if enable_recording else "disabled",
                "reason": "Bounded observer-state journal and validated read-only replay files."
                if enable_recording else "Recording/import resources are disabled by this backend.",
            },
            "http_api": {
                "state": "available",
                "reason": "Provided by the FastAPI backend.",
            },
            "websocket": {
                "state": "available",
                "reason": "Provided by the FastAPI backend.",
            },
            "sse": {
                "state": "available",
                "reason": "Provided by the FastAPI backend.",
            },
            "analytics": {
                "state": "available" if enable_analytics else "disabled",
                "reason": "Read-only analytics over retained normalized observations."
                if enable_analytics else "Analytics is disabled by this backend's configuration.",
            },
            "commands": {
                "state": "available" if commands is not None else "disabled",
                "reason": "Explicitly registered adapter-controlled sessions."
                if commands is not None else "No command dispatcher is configured; this backend is read-only.",
            },
        },
        owner="backend",
        source="fastapi",
    )

    def contract_document() -> JsonObject:
        return build_contract_document(
            EVENT_TYPES,
            event_schema_version=OBSERVER_SCHEMA_VERSION,
            capabilities=observer.capabilities(),
        )

    @asynccontextmanager
    async def lifespan(_app: FastAPI) -> AsyncIterator[None]:
        async def capture_clock() -> None:
            while True:
                await asyncio.sleep(1)
                await asyncio.to_thread(observer.checkpoint_capture)

        def evaluate_alerts() -> None:
            if commands is None:
                observer.evaluate_alerts()
            else:
                observer.evaluate_alerts(commands.alert_watch())

        async def alert_clock() -> None:
            while True:
                await asyncio.sleep(1)
                await asyncio.to_thread(evaluate_alerts)

        clock_task = asyncio.create_task(capture_clock()) if enable_recording else None
        alert_task = asyncio.create_task(alert_clock()) if enable_alerts else None
        try:
            yield
        finally:
            if alert_task is not None:
                alert_task.cancel()
                with suppress(asyncio.CancelledError):
                    await alert_task
            if clock_task is not None:
                clock_task.cancel()
                with suppress(asyncio.CancelledError):
                    await clock_task
            if commands is not None:
                await asyncio.to_thread(commands.close)

    app = FastAPI(
        title="Umbra Runtime Observer",
        version=OBSERVER_CONTRACT_VERSION,
        description="Provider-neutral runtime event observer.",
        lifespan=lifespan,
    )
    if enable_recording:
        from .recording_routes import add_recording_routes
        add_recording_routes(app, observer)
    if enable_alerts:
        from .alert_routes import add_alert_routes
        add_alert_routes(app, observer, commands, controls=enable_alert_controls)
    if enable_workspace_validation:
        from .workspace_routes import add_workspace_routes
        add_workspace_routes(app)

    react_ui_dist = _react_ui_dist()
    if react_ui_dist is not None and (react_ui_dist / "assets").is_dir():
        app.mount(
            "/assets",
            StaticFiles(directory=str(react_ui_dist / "assets")),
            name="observer-ui-assets",
        )

    @app.middleware("http")
    async def contract_headers(
        request: Request,
        call_next: Callable[[Request], Awaitable[Response]],
    ) -> Response:
        response = await call_next(request)
        response.headers["X-Umbra-Observer-Contract"] = OBSERVER_CONTRACT_VERSION
        response.headers["X-Umbra-Observer-Media-Type"] = OBSERVER_JSON_MEDIA_TYPE
        return response

    @app.get("/", response_class=HTMLResponse)
    async def dashboard() -> str:
        return _dashboard_html()

    @app.get("/api/health", response_model=HealthResponse)
    async def health() -> HealthResponse:
        state = _validated_summary(observer)
        return HealthResponse(
            service="umbra-rti-observer",
            status=state.status,
            contract_version=OBSERVER_CONTRACT_VERSION,
            schema_version=OBSERVER_SCHEMA_VERSION,
            event_count=state.live_metrics.event_count,
            retention=state.retention,
            capabilities=state.capabilities,
        )

    @app.get("/api/catalog", response_model=CatalogResponse)
    async def catalog() -> CatalogResponse:
        capabilities = _validated_capabilities(observer)
        return CatalogResponse(
            contract_version=OBSERVER_CONTRACT_VERSION,
            schema_version=OBSERVER_SCHEMA_VERSION,
            event_schema=build_event_schema(),
            contract=ContractResponse.model_validate(contract_document()),
            capabilities=capabilities,
            features=[
                capability.label
                for capability in capabilities.values()
                if capability.state in {"available", "partial"}
            ],
        )

    @app.get("/api/contract", response_model=ContractResponse)
    async def contract() -> JsonObject:
        return contract_document()

    @app.get("/api/schema", response_model=JsonObjectResponse)
    async def schema() -> JsonObject:
        return build_event_schema()

    @app.get("/api/state", response_model=StateResponse, response_model_exclude_none=True)
    async def state(include_events: bool = True) -> JsonObject:
        return observer.state(include_events=include_events)

    @app.get("/api/metrics", response_model=MetricsResponse)
    async def metrics() -> MetricsResponse:
        state = _validated_summary(observer)
        return MetricsResponse(
            contract_version=OBSERVER_CONTRACT_VERSION,
            schema_version=OBSERVER_SCHEMA_VERSION,
            status=state.status,
            live_metrics=state.live_metrics,
            retention=state.retention,
            adapters=state.adapters,
        )

    @app.get("/api/commands/catalog", response_model=CommandCatalog)
    def command_catalog() -> CommandCatalog:
        if commands is None:
            return CommandCatalog(contract_version=OBSERVER_CONTRACT_VERSION, enabled=False,
                                  timeout_seconds=0, history_limit=0, targets=())
        return commands.catalog()

    @app.get("/api/commands", response_model=CommandHistory)
    def command_history() -> CommandHistory:
        if commands is None:
            return CommandHistory(contract_version=OBSERVER_CONTRACT_VERSION, records=(), limit=0, evicted_records=0)
        return commands.history()

    @app.get("/api/commands/{command_id}", response_model=CommandRecord)
    def command_result(command_id: str) -> CommandRecord:
        try:
            if commands is None:
                raise KeyError(command_id)
            return commands.get(command_id)
        except KeyError as error:
            raise HTTPException(status_code=404, detail="command not found in retained history") from error

    @app.post("/api/commands", status_code=202, response_model=CommandRecord)
    def submit_command(payload: CommandRequest) -> CommandRecord:
        if commands is None:
            raise HTTPException(status_code=503, detail="command execution is disabled")
        try:
            return commands.submit(payload)
        except KeyError as error:
            raise HTTPException(status_code=404, detail="unknown controlled session") from error
        except CommandConflict as error:
            raise HTTPException(status_code=409, detail=str(error)) from error

    if enable_analytics:
        @app.get("/api/analytics", response_model=AnalyticsResponse)
        def analytics(
            window_seconds: int = Query(default=300, ge=30, le=3600),
            bucket_count: int = Query(default=30, ge=10, le=120),
            federate: str | None = Query(default=None, min_length=1, max_length=512),
            perspective: str | None = Query(default=None, min_length=1, max_length=8192),
        ) -> AnalyticsResponse:
            return build_analytics(
                AnalyticsWindow.model_validate(observer.analytics_window()),
                window_seconds=window_seconds, bucket_count=bucket_count, federate=federate, perspective=perspective,
            )

    @app.get("/api/inspectors/{kind}", response_model=InspectorResponse)
    async def inspectors(kind: str) -> InspectorResponse:
        state = _validated_state(observer, include_events=False)
        values = {
            "objects": state.inspectors.objects,
            "interactions": state.inspectors.interactions,
            "federates": state.federate_roster,
        }
        if kind not in values:
            raise HTTPException(status_code=404, detail="unknown inspector")
        return InspectorResponse.model_validate({
            "contract_version": OBSERVER_CONTRACT_VERSION,
            "kind": kind,
            "items": values[kind],
        })

    @app.get("/api/events", response_model=EventPageResponse)
    async def events(
        after: int = Query(default=0, ge=0),
        limit: int = Query(default=500, ge=1, le=2_000),
        event_type: str | None = None,
        family: str | None = None,
        source: str | None = None,
    ) -> JsonObject:
        try:
            return observer.event_page(
                after_sequence=after,
                limit=limit,
                event_types=_csv_filter(event_type),
                families=_csv_filter(family),
                sources=_csv_filter(source),
            )
        except ValueError as error:
            raise HTTPException(status_code=400, detail=str(error)) from error

    @app.post("/api/events", response_model=IngestResponse, response_model_exclude_none=True)
    async def ingest(payload: EventIngestRequest) -> IngestResponse:
        try:
            rows = _event_payload(payload)
            count = observer.publish_many(rows)
        except (ValueError, TypeError) as error:
            raise HTTPException(status_code=400, detail=str(error)) from error
        return IngestResponse(
            accepted=count,
            state=_validated_state(observer, include_events=False),
        )

    @app.put("/api/context", response_model=ContextResponse)
    async def context(payload: ContextRequest) -> ContextResponse:
        try:
            return ContextResponse(context=observer.set_context(**payload.root))
        except (ValueError, TypeError) as error:
            raise HTTPException(status_code=400, detail=str(error)) from error

    @app.post("/api/control/status", response_model=StateResponse, response_model_exclude_none=True)
    async def status(payload: StatusRequest) -> StateResponse:
        try:
            context_values = dict(payload.extensions)
            context_values.update(payload.context)
            return StateResponse.model_validate(
                observer.set_status(payload.status, **context_values)
            )
        except (ValueError, TypeError) as error:
            raise HTTPException(status_code=400, detail=str(error)) from error

    @app.delete("/api/events", response_model=StateResponse, response_model_exclude_none=True)
    async def clear_events() -> JsonObject:
        observer.clear()
        return observer.state(include_events=False)

    @app.get("/api/events/stream", response_model=None)
    async def event_stream(after: int = Query(default=0, ge=0)) -> StreamingResponse:

        async def generate() -> AsyncIterator[str]:
            cursor = max(after, 0)
            snapshot = SnapshotMessage(
                type="snapshot",
                state=_validated_state(observer, include_events=False),
            )
            yield f"data: {_stream_json(snapshot)}\n\n"
            while True:
                page = EventPageResponse.model_validate(observer.event_page(after_sequence=cursor, limit=500))
                if page.gap:
                    retention = RetentionGapResponse(
                        first_retained_sequence=page.first_retained_sequence,
                        last_sequence=page.last_sequence,
                        retained_events=page.retained_events,
                        dropped_events=page.dropped_events,
                        cleared_events=page.cleared_events,
                        next_sequence=page.next_sequence,
                    )
                    yield f"data: {_stream_json(GapMessage(type='gap', retention=retention))}\n\n"
                    cursor = max(cursor, page.first_retained_sequence - 1 if page.retained_events else page.next_after)
                    continue
                rows = page.events
                if not rows:
                    await asyncio.to_thread(
                        observer.wait_for_events,
                        after_sequence=cursor,
                        timeout=15.0,
                    )
                    fresh = EventPageResponse.model_validate(observer.event_page(after_sequence=cursor, limit=500))
                    if fresh.gap:
                        continue
                    rows = fresh.events
                if not rows:
                    yield f"data: {_stream_json(HeartbeatMessage(type='heartbeat'))}\n\n"
                    continue
                cursor = max(cursor, rows[-1].sequence)
                message = EventsMessage(
                    type="events",
                    events=rows,
                    state=_validated_state(observer, include_events=False),
                )
                yield f"data: {_stream_json(message)}\n\n"

        return StreamingResponse(
            generate(),
            media_type="text/event-stream",
            headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"},
        )

    @app.websocket("/ws/events")
    async def websocket_events(socket: WebSocket) -> None:
        await socket.accept()
        initial_state = _validated_state(observer, include_events=False)
        cursor = 0
        try:
            await socket.send_json(json_model(SnapshotMessage(type="snapshot", state=initial_state)))
            while True:
                page = EventPageResponse.model_validate(observer.event_page(after_sequence=cursor, limit=500))
                if page.gap:
                    retention = RetentionGapResponse(
                        first_retained_sequence=page.first_retained_sequence,
                        last_sequence=page.last_sequence,
                        retained_events=page.retained_events,
                        dropped_events=page.dropped_events,
                        cleared_events=page.cleared_events,
                        next_sequence=page.next_sequence,
                    )
                    await socket.send_json(json_model(GapMessage(type="gap", retention=retention)))
                    cursor = max(cursor, page.first_retained_sequence - 1 if page.retained_events else page.next_after)
                    continue
                rows = page.events
                if not rows:
                    await asyncio.to_thread(
                        observer.wait_for_events,
                        after_sequence=cursor,
                        timeout=15.0,
                    )
                    fresh = EventPageResponse.model_validate(observer.event_page(after_sequence=cursor, limit=500))
                    if fresh.gap:
                        continue
                    rows = fresh.events
                if rows:
                    cursor = max(cursor, rows[-1].sequence)
                    await socket.send_json(json_model(EventsMessage(
                        type="events",
                        events=rows,
                        state=_validated_state(observer, include_events=False),
                    )))
                else:
                    await socket.send_json(json_model(HeartbeatMessage(type="heartbeat")))
        except WebSocketDisconnect:
            return

    app.state.observer_store = observer
    return app


__all__ = ["create_app"]
