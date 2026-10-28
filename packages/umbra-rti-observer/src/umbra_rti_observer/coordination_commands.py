"""Opt-in, provider-neutral service definitions for an explicitly owned federate."""

# pyright: strict
from __future__ import annotations

from .command_models import (
    CommandDefinition,
    CommandField,
    CoordinationAction,
    CoordinationCompletionParameters,
    CoordinationLabelParameters,
    EmptyParameters,
    SynchronizationAchieveParameters,
    SynchronizationRegisterParameters,
)
from .schemas import JsonObjectResponse, ObserverModel


def coordination_definitions(
    actions: tuple[CoordinationAction, ...],
) -> tuple[CommandDefinition, ...]:
    label = CommandField(
        name="label",
        label="Operation label",
        input="text",
        required=True,
        default_value="Observer checkpoint",
    )
    result = CommandField(
        name="result",
        label="Local result",
        input="select",
        required=True,
        choices=("completed", "failed"),
        default_value="completed",
    )
    specs: tuple[
        tuple[
            CoordinationAction, str, str, type[ObserverModel], tuple[CommandField, ...]
        ],
        ...,
    ] = (
        (
            "sync.register",
            "Register synchronization",
            "Request a federation-wide synchronization point. A service return does not prove registration or synchronization.",
            SynchronizationRegisterParameters,
            (
                label,
                CommandField(
                    name="tag_text", label="Tag text", input="text", required=False
                ),
            ),
        ),
        (
            "sync.achieve",
            "Report synchronization achievement",
            "Report this owned federate's result for an announced label. Other participants and collective completion remain separate.",
            SynchronizationAchieveParameters,
            (
                label,
                CommandField(
                    name="result",
                    label="Local result",
                    input="select",
                    required=True,
                    choices=("achieved", "failed"),
                    default_value="achieved",
                ),
            ),
        ),
        (
            "save.request",
            "Request save",
            "Request an immediate federation save. The backend owns provider configuration and persistence storage.",
            CoordinationLabelParameters,
            (label,),
        ),
        (
            "save.begun",
            "Report save begun",
            "Report that this owned federate has begun its pending save.",
            EmptyParameters,
            (),
        ),
        (
            "save.complete",
            "Report save result",
            "Report this owned federate's completed or failed save. This does not complete every participant's save.",
            CoordinationCompletionParameters,
            (result,),
        ),
        (
            "restore.request",
            "Request restore",
            "Request a federation restore by label. This can affect all participants; only an explicitly enabled adapter may offer it.",
            CoordinationLabelParameters,
            (label,),
        ),
        (
            "restore.complete",
            "Report restore result",
            "Report this owned federate's completed or failed restore. Collective outcome requires an explicit report.",
            CoordinationCompletionParameters,
            (result,),
        ),
    )
    return tuple(
        CommandDefinition(
            action=action,
            title=title,
            description=description,
            state="available",
            requires_confirmation=True,
            parameter_model=model.__name__,
            parameter_schema=JsonObjectResponse.model_validate(
                model.model_json_schema()
            ).root,
            fields=fields,
        )
        for action, title, description, model, fields in specs
        if action in actions
    )
