"""Compatibility façade for the 2025 Python RTI contracts.

Concrete value objects are defined in :mod:`.values`; abstract RTI,
ambassador, callback, and provider contracts are defined in :mod:`.abstract`.
This module keeps the historical ``hla.rti1516_2025.core`` imports stable and
retains the Java ``RtiFactoryFactory``/Python entry-point discovery helper.
"""

from __future__ import annotations

from importlib.metadata import entry_points
import os
from typing import Any

from .abstract import *
from .exceptions import RTIinternalError, UnsupportedCallbackModel
from .values import *


def _require_callback_model(value: object) -> CallbackModel:
    """Validate the Java ``CallbackModel``/C++ callback-mode contract."""

    if not isinstance(value, CallbackModel):
        raise UnsupportedCallbackModel(f"Unsupported callback model: {value!r}")
    return value


def _resolve_connect_arguments(
    configuration: RtiConfiguration | object | None,
    credentials: object | None,
) -> tuple[RtiConfiguration | None, object | None]:
    """Adapt the Java ``connect`` overloads to Python optional arguments."""

    from .auth import Credentials

    if isinstance(configuration, Credentials):
        if credentials is not None:
            raise TypeError("credentials were supplied twice")
        return None, configuration
    if configuration is not None and not isinstance(configuration, RtiConfiguration):
        raise TypeError("configuration must be RtiConfiguration or Credentials")
    if credentials is not None and not isinstance(credentials, Credentials):
        raise TypeError("credentials must be Credentials")
    return configuration, credentials


class RtiFactoryFactory:
    """Discover providers like Java ``RtiFactoryFactory`` and ``ServiceLoader``.

    Java surface: ``hla.rti1516_2025.RtiFactoryFactory``.
    C++ surface: ``RTI::RTIambassadorFactory``/provider factory discovery.
    """

    _ENTRY_POINT_GROUP = "hla.rti1516_2025.factories"

    @classmethod
    def getRtiFactory(cls, name: str | None = None) -> RtiFactory:
        requested_name = name if name is not None else os.getenv("HLA_RTI_FACTORY_NAME")
        if requested_name is not None:
            aliased_factory = cls._factory_from_entry_point_alias(requested_name)
            if aliased_factory is not None:
                return aliased_factory
        factories = cls.getAvailableRtiFactories()
        if requested_name is None and factories:
            return factories[0]
        for factory in factories:
            if factory.rtiName() == requested_name:
                return factory
        if requested_name is None:
            raise RTIinternalError("Cannot find factory")
        raise RTIinternalError(f"Cannot find factory matching {requested_name}")

    @classmethod
    def getAvailableRtiFactories(cls) -> list[RtiFactory]:
        return [entry_point.load()() for entry_point in cls._entry_points()]

    @classmethod
    def _factory_from_entry_point_alias(cls, name: str) -> RtiFactory | None:
        """Select an installed provider before it initializes its transport."""

        matching_entry_points = [
            entry_point for entry_point in cls._entry_points() if entry_point.name == name
        ]
        if not matching_entry_points:
            return None
        if len(matching_entry_points) != 1:
            raise RTIinternalError(f"More than one Python RTI provider uses alias {name}")
        return matching_entry_points[0].load()()

    @classmethod
    def _entry_points(cls) -> Any:
        return entry_points(group=cls._ENTRY_POINT_GROUP)
