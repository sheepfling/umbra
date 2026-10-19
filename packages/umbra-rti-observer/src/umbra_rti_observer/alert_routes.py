"""Optional validated configuration/evidence routes; no provider operations."""

# pyright: strict
from __future__ import annotations

from fastapi import FastAPI, HTTPException

from .alert_models import (
    AlertAcknowledgeRequest,
    AlertConfigurationUpdate,
    AlertReport,
    AlertResetRequest,
)
from .alerting import NO_COMMAND_WATCH, AlertConflict, CommandWatch
from .commands import CommandDispatcher
from .core import ObserverStore


def add_alert_routes(app: FastAPI, observer: ObserverStore, commands: CommandDispatcher | None, *, controls: bool = True) -> None:
    def watch() -> CommandWatch:
        # Acquire/release the dispatcher cache lock before entering the store.
        return commands.alert_watch() if commands is not None else NO_COMMAND_WATCH

    def alerts() -> AlertReport:
        return AlertReport.model_validate(observer.alert_report(watch()))

    def configure_alerts(payload: AlertConfigurationUpdate) -> AlertReport:
        require_controls(payload.expected_instance_id)
        try:
            source = observer.configure_alerts(
                tuple(rule.spec() for rule in payload.configuration.rules), payload.expected_revision, watch())
        except AlertConflict as error:
            raise HTTPException(status_code=409, detail=str(error)) from error
        except ValueError as error:
            raise HTTPException(status_code=400, detail="Alert configuration exceeds its bounds.") from error
        return AlertReport.model_validate(source)

    def reset_alert(payload: AlertResetRequest) -> AlertReport:
        require_controls(payload.expected_instance_id)
        try:
            return AlertReport.model_validate(observer.reset_alert(payload.rule_id, payload.expected_revision, watch()))
        except AlertConflict as error:
            raise HTTPException(status_code=409, detail=str(error)) from error
        except KeyError as error:
            raise HTTPException(status_code=404, detail="Alert rule is no longer configured.") from error

    def acknowledge_alert(payload: AlertAcknowledgeRequest) -> AlertReport:
        require_controls(payload.expected_instance_id)
        try:
            return AlertReport.model_validate(observer.acknowledge_alert(payload.evidence_id, watch()))
        except KeyError as error:
            raise HTTPException(status_code=404, detail="Alert evidence is no longer retained.") from error

    def require_controls(instance_id: str) -> None:
        if instance_id != observer.alert_instance_id:
            raise HTTPException(status_code=409, detail="The alert backend restarted. Refresh before applying changes.")
        capability = observer.capabilities().get("alert_controls")
        if capability is None or capability.get("state") not in {"available", "partial"}:
            raise HTTPException(status_code=403, detail="Alert configuration controls are disabled.")

    app.add_api_route("/api/alerts", alerts, methods=["GET"], response_model=AlertReport)
    if controls:
        app.add_api_route("/api/alerts/configuration", configure_alerts, methods=["PUT"], response_model=AlertReport)
        app.add_api_route("/api/alerts/reset", reset_alert, methods=["POST"], response_model=AlertReport)
        app.add_api_route("/api/alerts/acknowledge", acknowledge_alert, methods=["POST"], response_model=AlertReport)
