"""Shared JSON types and strict Pydantic base models for observer schemas."""

from __future__ import annotations

from collections.abc import Mapping
from types import MappingProxyType
from typing import TYPE_CHECKING, Annotated, ClassVar, Literal, TypeAlias, cast

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    SerializerFunctionWrapHandler,
    StringConstraints,
    TypeAdapter,
    model_serializer,
    model_validator,
)
from typing_extensions import TypeAliasType

from .identities import ParticipantIdentity, participant_identity_from_fields

JsonText = Annotated[str, StringConstraints(strip_whitespace=False)]

if TYPE_CHECKING:
    JsonValue: TypeAlias = (
        None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"]
    )
else:
    # A named runtime alias gives OpenAPI a recursive schema; the static alias
    # above also works with checkers that cannot resolve TypeAliasType recursion.
    JsonValue = TypeAliasType(
        "JsonValue",
        None
        | bool
        | int
        | float
        | JsonText
        | list["JsonValue"]
        | dict[JsonText, "JsonValue"],
    )
JsonObject = dict[str, JsonValue]
# Pyright cannot express the recursive TypeForm in the TypeAdapter constructor.
_JSON_VALUE_ADAPTER = TypeAdapter(JsonValue)  # pyright: ignore[reportArgumentType]

CapabilityState = Literal[
    "available",
    "partial",
    "disabled",
    "unsupported",
    "unknown",
]
NonEmptyText = Annotated[str, Field(min_length=1)]
NonEmptyJsonText = Annotated[JsonText, Field(min_length=1)]
EventTypeText = Annotated[str, Field(min_length=1, max_length=256)]
SequenceNumber = Annotated[int, Field(ge=1)]
NonNegativeInt = Annotated[int, Field(ge=0)]
PositiveInt = Annotated[int, Field(ge=1)]
NonNegativeFloat = Annotated[float, Field(ge=0)]

# Constructor defaults aid Python callers, not HTTP/file producers. Consumers
# need these actual wire collections, even when they are explicitly empty.
WIRE_REQUIRED_FIELDS: Mapping[str, frozenset[str]] = MappingProxyType(
    {
        "ContractResponse": frozenset({"capabilities"}),
        "StateResponse": frozenset(
            {"context", "adapters", "capabilities", "federate_roster", "inspectors"}
        ),
        "InspectorsResponse": frozenset({"objects", "interactions"}),
        "HealthResponse": frozenset({"capabilities"}),
        "MetricsResponse": frozenset({"adapters"}),
        "EventPageResponse": frozenset({"events"}),
        "CommandCatalog": frozenset({"targets"}),
        "CommandHistory": frozenset({"records"}),
    }
)


class ObserverModel(BaseModel):
    """Strict, immutable Pydantic boundary model."""

    model_config = ConfigDict(
        extra="forbid",
        frozen=True,
        populate_by_name=True,
        strict=True,
        str_strip_whitespace=True,
    )


class WireObserverModel(ObserverModel):
    """Response model that preserves unknown wire fields at the top level."""

    model_config = ConfigDict(extra="allow")


class ExtensibleObserverModel(ObserverModel):
    """Strict model with one explicit provider-extension escape hatch."""

    _reject_batch_shape: ClassVar[bool] = False
    extensions: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]

    @model_validator(mode="before")
    @classmethod
    def collect_extensions(cls, value: object) -> object:
        """Collect unknown wire fields into the named extension mapping."""

        if not isinstance(value, Mapping):
            return value
        if cls._reject_batch_shape and "events" in value:
            raise ValueError("an object containing 'events' must be an event batch")
        fields = set(cls.model_fields) - {"extensions"}
        raw_extensions = value.get("extensions", {})
        if not isinstance(raw_extensions, Mapping):
            raise TypeError("extensions must be a JSON object")
        extensions = {
            str(key): _JSON_VALUE_ADAPTER.validate_python(item)
            for key, item in raw_extensions.items()
        }
        fields_in_extensions = {
            str(key): _JSON_VALUE_ADAPTER.validate_python(item)
            for key, item in value.items()
            if key not in fields and key != "extensions"
        }
        extensions.update(fields_in_extensions)
        return {
            **{key: item for key, item in value.items() if key in fields},
            "extensions": extensions,
        }

    @model_serializer(mode="wrap")
    def flatten_extensions(self, handler: SerializerFunctionWrapHandler) -> object:
        """Keep the existing top-level wire shape while storing extensions explicitly."""

        serialized = cast(dict[str, object], handler(self))
        extensions = serialized.pop("extensions", {})
        if not isinstance(extensions, Mapping):  # pragma: no cover - validated above
            raise TypeError("extensions must serialize as a JSON object")
        return {**dict(extensions), **serialized}


TimeId = Annotated[
    str,
    StringConstraints(
        strip_whitespace=False, min_length=1, max_length=256, pattern=r"\S"
    ),
]
TimeNumber = Annotated[int | float, Field(allow_inf_nan=False)]
DeclarationId = Annotated[
    str,
    StringConstraints(
        strip_whitespace=False, min_length=1, max_length=512, pattern=r"\S"
    ),
]


class ParticipantIdentityResponse(ObserverModel):
    """Opaque IDs identify a participant join; names and handles never do."""

    participant_id: DeclarationId
    membership_id: DeclarationId
    display_name: DeclarationId | None = None
    handle: DeclarationId | None = None

    def to_domain(self) -> ParticipantIdentity:
        """Return the validated domain identity without a JSON-map round trip."""
        return participant_identity_from_fields(
            self.participant_id,
            self.membership_id,
            self.display_name,
            self.handle,
        )
