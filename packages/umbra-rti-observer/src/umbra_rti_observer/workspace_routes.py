"""Optional configuration normalization; never persists or executes a workspace."""

# pyright: strict
from __future__ import annotations

from fastapi import FastAPI

from .workspace_models import (
    WorkspaceDefaultsResponse,
    WorkspaceDocument,
    default_library,
)


def add_workspace_routes(app: FastAPI) -> None:
    def defaults() -> WorkspaceDefaultsResponse:
        return WorkspaceDefaultsResponse(library=default_library())

    def validate_workspace(payload: WorkspaceDocument) -> WorkspaceDocument:
        return payload

    app.add_api_route(
        "/api/workspaces/defaults",
        defaults,
        methods=["GET"],
        response_model=WorkspaceDefaultsResponse,
    )
    app.add_api_route(
        "/api/workspaces/validate",
        validate_workspace,
        methods=["POST"],
        response_model=WorkspaceDocument,
    )
