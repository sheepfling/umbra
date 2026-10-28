"""Typed JSON evidence shared by current-process TCK adapters."""

from __future__ import annotations

import json
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path
from typing import Literal, TypeAlias, TypedDict, cast

JsonScalar: TypeAlias = str | int | float | bool | None
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]
CallbackModel: TypeAlias = Literal["evoked", "immediate"]
CallbackModelSelection: TypeAlias = Literal["both", "evoked", "immediate"]
ScenarioStatus: TypeAlias = Literal["passed", "skipped", "failed"]


class _TckEvidenceResultFields(TypedDict):
    id: str
    callback_model: CallbackModel
    status: ScenarioStatus


class TckEvidenceResult(_TckEvidenceResultFields, total=False):
    message: str | None
    duration_ms: int | None


@dataclass(frozen=True, slots=True)
class TckEvidencePayload:
    document: JsonObject
    results: tuple[TckEvidenceResult, ...]


def empty_tck_evidence() -> TckEvidencePayload:
    return TckEvidencePayload(document={}, results=())


def load_tck_evidence(path: Path) -> TckEvidencePayload:
    decoded = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(decoded, dict):
        raise TypeError(f"TCK evidence must be a JSON object: {path}")
    document = cast(JsonObject, decoded)
    raw_results = document.get("results", [])
    if not isinstance(raw_results, list):
        raise TypeError(f"TCK evidence results must be a JSON array: {path}")

    results: list[TckEvidenceResult] = []
    for index, raw_result in enumerate(raw_results):
        if not isinstance(raw_result, dict):
            raise TypeError(
                f"TCK evidence result {index} must be a JSON object: {path}"
            )
        scenario_id = raw_result.get("id")
        callback_model = raw_result.get("callback_model")
        status = raw_result.get("status")
        if not isinstance(scenario_id, str):
            raise TypeError(f"TCK evidence result {index} has no string id: {path}")
        if not isinstance(callback_model, str):
            raise TypeError(
                f"TCK evidence result {scenario_id} has no string callback model: {path}"
            )
        if callback_model not in {"evoked", "immediate"}:
            raise ValueError(
                f"TCK evidence result {scenario_id} has an unknown callback model: {path}"
            )
        if not isinstance(status, str):
            raise TypeError(
                f"TCK evidence result {scenario_id} has no string status: {path}"
            )
        if status not in {"passed", "skipped", "failed"}:
            raise ValueError(
                f"TCK evidence result {scenario_id} has an unknown status: {path}"
            )
        message = raw_result.get("message")
        if message is not None and not isinstance(message, str):
            raise TypeError(
                f"TCK evidence result {scenario_id} has a non-string message: {path}"
            )
        duration_ms = raw_result.get("duration_ms")
        if duration_ms is not None and (
            not isinstance(duration_ms, int) or isinstance(duration_ms, bool)
        ):
            raise TypeError(
                f"TCK evidence result {scenario_id} has a non-integer duration: {path}"
            )
        results.append(cast(TckEvidenceResult, raw_result))

    return TckEvidencePayload(document=document, results=tuple(results))


def combine_tck_evidence(
    payloads: Sequence[TckEvidencePayload], callback_model: CallbackModelSelection
) -> JsonObject:
    if not payloads:
        raise ValueError("cannot combine empty TCK evidence")
    combined = dict(payloads[0].document)
    combined["callback_model"] = callback_model
    combined["results"] = [
        cast(JsonValue, result) for payload in payloads for result in payload.results
    ]
    return combined
