"""Versioned view configuration only: never captured data or control authority."""

# pyright: strict
from __future__ import annotations

import json
from collections.abc import Sequence
from typing import Annotated, Literal, TypeAlias

from pydantic import ConfigDict, Field, StringConstraints, model_validator

from .schemas import ObserverModel

WORKSPACE_VERSION = "umbra-observer-workspace-v1"
LIBRARY_VERSION = "umbra-observer-workspace-library-v1"
MAX_WORKSPACE_BYTES, MAX_LIBRARY_BYTES, MAX_SAVED = 65536, 524288, 12
ViewName: TypeAlias = Literal["federation", "federates", "objects", "declarations", "topology", "interactions",
    "ownership", "regions", "mom", "fom", "time", "synchronization", "persistence", "events", "inspectors", "recordings", "alerts", "actions", "diagnostics"]
FilterKey = Annotated[str, StringConstraints(max_length=8192)]
SearchText = Annotated[str, StringConstraints(max_length=2048)]
WorkspaceName = Annotated[str, StringConstraints(min_length=1, max_length=128, pattern=r"\S")]


class WorkspaceModel(ObserverModel):
    # Imports can omit defaulted choices; normalized output has every field.
    model_config = ConfigDict(json_schema_serialization_defaults_required=True)


class EventViewSettings(WorkspaceModel):
    search: SearchText = ""
    family: FilterKey = ""
    type: FilterKey = ""
    source: FilterKey = ""
    federate: FilterKey = ""


class ObjectViewSettings(WorkspaceModel):
    scope: FilterKey | None = None
    search: SearchText = ""
    status: FilterKey = "all"
    selected: FilterKey = ""


class ObjectChartChoice(WorkspaceModel):
    scope: FilterKey | None = None
    object_key: Annotated[str, StringConstraints(min_length=1, max_length=8192)]
    attribute: Annotated[str, StringConstraints(min_length=1, max_length=8192)]


class DeclarationViewSettings(WorkspaceModel):
    search: SearchText = ""
    perspective: FilterKey = "all"
    kind: FilterKey = "all"
    direction: FilterKey = "all"
    status: FilterKey = "all"


class TimeViewSettings(WorkspaceModel):
    selected: FilterKey | None = None
    grants_only: bool = False


class TopologyViewSettings(WorkspaceModel):
    search: SearchText = ""
    source: FilterKey = "all"
    kind: FilterKey = "all"
    status: FilterKey = "all"
    evidence: FilterKey = "all"
    focused: FilterKey = "all"


class InteractionViewSettings(WorkspaceModel):
    search: SearchText = ""
    status: FilterKey = "all"
    order: FilterKey = "all"


class CoordinationViewSettings(WorkspaceModel):
    search: SearchText = ""
    kind: FilterKey = "all"
    scope: FilterKey = "all"
    phase: FilterKey = "all"
    selected: FilterKey = "all"


class OwnershipViewSettings(WorkspaceModel):
    search: SearchText = ""
    scope: FilterKey = "all"
    owner_state: FilterKey = "all"
    request_status: FilterKey = "all"


class RegionViewSettings(WorkspaceModel):
    search: SearchText = ""
    kind: FilterKey = "all"
    status: FilterKey = "all"
    evidence: FilterKey = "all"
    selected: FilterKey | None = None


class MomViewSettings(WorkspaceModel):
    search: SearchText = ""
    scope: FilterKey = "all"
    service_status: FilterKey = "all"
    counter: FilterKey | None = None


class FomViewSettings(WorkspaceModel):
    search: SearchText = ""
    model: FilterKey = "all"
    kind: FilterKey = "all"
    status: FilterKey = "all"
    selected: FilterKey | None = None


class AlertViewSettings(WorkspaceModel):
    search: SearchText = ""
    status: FilterKey = "all"


class FederationViewSettings(WorkspaceModel):
    distribution: Literal["event_types", "sources", "families"] = "event_types"


class WorkspaceSettings(WorkspaceModel):
    view: ViewName = "federation"
    window_seconds: Literal[30, 300, 900, 3600] = 300
    selected_federate: FilterKey = ""
    federation: FederationViewSettings = Field(default_factory=FederationViewSettings)
    events: EventViewSettings = Field(default_factory=EventViewSettings)
    federate_events: EventViewSettings = Field(default_factory=EventViewSettings)
    objects: ObjectViewSettings = Field(default_factory=ObjectViewSettings)
    object_charts: Annotated[Sequence[ObjectChartChoice], Field(max_length=64)] = ()
    declarations: DeclarationViewSettings = Field(default_factory=DeclarationViewSettings)
    federate_declarations: DeclarationViewSettings = Field(default_factory=DeclarationViewSettings)
    time: TimeViewSettings = Field(default_factory=TimeViewSettings)
    federate_time: TimeViewSettings = Field(default_factory=TimeViewSettings)
    topology: TopologyViewSettings = Field(default_factory=TopologyViewSettings)
    interactions: InteractionViewSettings = Field(default_factory=InteractionViewSettings)
    synchronization: CoordinationViewSettings = Field(default_factory=CoordinationViewSettings)
    persistence: CoordinationViewSettings = Field(default_factory=CoordinationViewSettings)
    ownership: OwnershipViewSettings = Field(default_factory=OwnershipViewSettings)
    regions: RegionViewSettings = Field(default_factory=RegionViewSettings)
    mom: MomViewSettings = Field(default_factory=MomViewSettings)
    fom: FomViewSettings = Field(default_factory=FomViewSettings)
    alerts: AlertViewSettings = Field(default_factory=AlertViewSettings)

    @model_validator(mode="after")
    def unique_chart_targets(self) -> WorkspaceSettings:
        keys = [(row.scope, row.object_key) for row in self.object_charts]
        if len(keys) != len(set(keys)):
            raise ValueError("numeric chart targets must be unique within a workspace")
        _bounded(self.model_dump(mode="json"), MAX_WORKSPACE_BYTES)
        return self


def _bounded(value: object, limit: int) -> None:
    if len(json.dumps(value, ensure_ascii=False, separators=(",", ":"), allow_nan=False).encode("utf-8")) > limit:
        raise ValueError("workspace configuration exceeds its encoded-byte limit")


class WorkspaceDocument(WorkspaceModel):
    workspace_version: Literal["umbra-observer-workspace-v1"]
    name: WorkspaceName
    settings: WorkspaceSettings = Field(default_factory=WorkspaceSettings)

    @model_validator(mode="after")
    def bounded_document(self) -> WorkspaceDocument:
        _bounded(self.model_dump(mode="json"), MAX_WORKSPACE_BYTES)
        return self


class WorkspaceLibrary(WorkspaceModel):
    library_version: Literal["umbra-observer-workspace-library-v1"]
    current: WorkspaceSettings = Field(default_factory=WorkspaceSettings)
    saved: Annotated[Sequence[WorkspaceDocument], Field(max_length=12)] = ()

    @model_validator(mode="after")
    def bounded_library(self) -> WorkspaceLibrary:
        names = [row.name for row in self.saved]
        if len(set(names)) != len(names):
            raise ValueError("saved workspace names must be unique")
        _bounded(self.model_dump(mode="json"), MAX_LIBRARY_BYTES)
        return self


def default_library() -> WorkspaceLibrary:
    return WorkspaceLibrary(library_version=LIBRARY_VERSION)


class WorkspaceDefaultsResponse(WorkspaceModel):
    contract_version: Literal["umbra-observer-contract-v1"] = "umbra-observer-contract-v1"
    library: WorkspaceLibrary
