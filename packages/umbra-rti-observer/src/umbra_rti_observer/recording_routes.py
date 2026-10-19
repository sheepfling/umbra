"""Opt-in observer recording resources; imported data never reaches a provider."""

# pyright: strict
from __future__ import annotations

from collections.abc import Callable, Coroutine
from typing import Any

from fastapi import APIRouter, FastAPI, HTTPException, Query, Request
from fastapi.exceptions import RequestValidationError
from fastapi.routing import APIRoute
from pydantic import ValidationError
from starlette.responses import Response
from starlette.types import Message

from .analytics import build_analytics
from .capture import MAX_RECORDING_BYTES, CaptureConflict
from .core import ObserverStore
from .recording_codec import StorageFormat
from .recording_models import (
    CaptureStatus,
    CompactRecordingDocument,
    PortableRecordingDocument,
    RecordedWindowAnalysisRequest,
    RecordingAnalysisRequest,
    RecordingDocument,
    RecordingStartRequest,
    RecordingStatusResponse,
    RecordingTargetRequest,
)
from .schemas import AnalyticsResponse


class BoundedRecordingRoute(APIRoute):
    """Bound chunked as well as length-declared bodies before JSON/model parsing."""

    def get_route_handler(self) -> Callable[[Request], Coroutine[Any, Any, Response]]:
        handler = super().get_route_handler()

        async def bounded(request: Request) -> Response:
            length = request.headers.get("content-length")
            if length is not None:
                try:
                    size = int(length)
                except ValueError as error:
                    raise HTTPException(400, "Invalid recording request size.") from error
                if size < 0 or size > MAX_RECORDING_BYTES:
                    raise HTTPException(413, "Recording request exceeds 16 MiB.")
            received = 0

            async def receive() -> Message:
                nonlocal received
                message = await request.receive()
                received += len(message.get("body", b""))
                if received > MAX_RECORDING_BYTES:
                    raise HTTPException(413, "Recording request exceeds 16 MiB.")
                return message

            try:
                return await handler(Request(request.scope, receive=receive))
            except RequestValidationError as error:
                # Never echo a whole federation capture once for every invalid
                # field, or serialize a rejected NaN/credential in error output.
                raise HTTPException(422, "Recording request rejected by named model validation.") from error
            except RecursionError as error:
                # Extremely nested JSON can exceed the parser's stack before
                # named fields are available. Keep that a bounded input error.
                raise HTTPException(422, "Recording request exceeds supported nesting.") from error

        return bounded


def add_recording_routes(app: FastAPI, observer: ObserverStore) -> None:
    router = APIRouter(prefix="/api/recordings", route_class=BoundedRecordingRoute)

    def status() -> RecordingStatusResponse:
        info = observer.capture_info()
        return RecordingStatusResponse(recording=CaptureStatus.model_validate(info.to_json()) if info else None)

    def start(payload: RecordingStartRequest) -> CaptureStatus:
        try:
            info = observer.start_capture(payload.label, replace_id=payload.replace_id,
                                          max_frames=payload.max_frames, max_bytes=payload.max_bytes,
                                          storage_format=payload.storage_format)
        except CaptureConflict as error:
            raise HTTPException(409, str(error)) from error
        except ValueError as error:
            raise HTTPException(400, str(error)) from error
        return CaptureStatus.model_validate(info.to_json())

    def stop(payload: RecordingTargetRequest) -> CaptureStatus:
        try:
            return CaptureStatus.model_validate(observer.stop_capture(payload.recording_id).to_json())
        except CaptureConflict as error:
            raise HTTPException(409, str(error)) from error

    def export(recording_id: str = Query(min_length=1, max_length=128), storage_format: StorageFormat = "full-v1") -> Response:
        try:
            source = observer.capture_document(recording_id)
            compact = source.get("recording_version") == "umbra-observer-recording-v2"
            if compact != (storage_format == "compact-v2"):
                raise HTTPException(409, "This capture uses a different storage format. Request its explicitly advertised format.")
            document = CompactRecordingDocument.model_validate(source) if compact else RecordingDocument.model_validate(source)
        except CaptureConflict as error:
            raise HTTPException(409, str(error)) from error
        except (ValidationError, ValueError, TypeError) as error:
            raise HTTPException(422, "Captured content does not satisfy the recording contract.") from error
        encoded = document.model_dump_json(exclude_unset=True)
        if len(encoded.encode("utf-8")) > MAX_RECORDING_BYTES:
            raise HTTPException(413, "Recording export exceeds 16 MiB.")
        return Response(encoded, media_type="application/json", headers={
            "Content-Disposition": f'attachment; filename="observer-recording-{document.recording_id}.json"',
        })

    def validate(payload: PortableRecordingDocument) -> PortableRecordingDocument:
        # Validation canonicalizes named boundary models only. No path is opened,
        # no URL followed and no captured capability grants execution authority.
        return payload

    def analyze(payload: RecordingAnalysisRequest | RecordedWindowAnalysisRequest) -> AnalyticsResponse:
        snapshot = payload.frame.state if isinstance(payload, RecordingAnalysisRequest) else payload.window
        captured_at = payload.frame.captured_at if isinstance(payload, RecordingAnalysisRequest) else payload.captured_at
        return build_analytics(snapshot, now=captured_at,
                               window_seconds=payload.window_seconds, bucket_count=payload.bucket_count)

    router.add_api_route("", status, methods=["GET"], response_model=RecordingStatusResponse)
    router.add_api_route("/start", start, methods=["POST"], response_model=CaptureStatus)
    router.add_api_route("/stop", stop, methods=["POST"], response_model=CaptureStatus)
    router.add_api_route("/export", export, methods=["GET"], response_model=PortableRecordingDocument)
    # Models were fully validated on ingress; returning a Response avoids a
    # second expensive reconstruction pass by response model validation.
    def validated_response(payload: PortableRecordingDocument) -> Response:
        return Response(validate(payload).model_dump_json(exclude_unset=True), media_type="application/json")

    router.add_api_route("/validate", validated_response, methods=["POST"], response_model=PortableRecordingDocument)
    router.add_api_route("/analyze", analyze, methods=["POST"], response_model=AnalyticsResponse)
    app.include_router(router)
