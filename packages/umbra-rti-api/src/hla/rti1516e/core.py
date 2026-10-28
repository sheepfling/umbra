"""Discovery and compatibility façade for the IEEE 1516.1-2010 surface."""

from __future__ import annotations

import logging
import os
from importlib.metadata import EntryPoint, EntryPoints, entry_points
from typing import Literal, TypeAlias, cast

from .abstract import *
from .contracts import *
from .exceptions import RTIinternalError
from .values import *

_LOGGER = logging.getLogger(__name__)
_TimeFactoryAccessor: TypeAlias = Literal["getTimeFactory", "getFloat64TimeFactory"]
_TIME_FACTORY_ACCESSORS: tuple[_TimeFactoryAccessor, ...] = (
    "getTimeFactory",
    "getFloat64TimeFactory",
)


def _explicit_time_factory(entry_point: EntryPoint) -> LogicalTimeFactory | None:
    """Load one optional standalone time-factory plugin."""

    try:
        factory = entry_point.load()()
    except Exception:
        _LOGGER.debug(
            "Skipping optional time-factory entry point %s after a load failure",
            entry_point.name,
            exc_info=True,
        )
        return None
    if not isinstance(factory, LogicalTimeFactory):
        _LOGGER.debug(
            "Skipping optional time-factory entry point %s: it returned %s",
            entry_point.name,
            type(factory).__name__,
        )
        return None
    return factory


def _provider_time_factories(
    entry_point: EntryPoint,
) -> tuple[LogicalTimeFactory, ...]:
    """Read available time factories from one optional RTI provider."""

    try:
        provider = cast(RtiFactory, entry_point.load()())
        ambassador = provider.getRtiAmbassador()
    except Exception:
        _LOGGER.debug(
            "Skipping optional RTI provider %s during time-factory discovery",
            entry_point.name,
            exc_info=True,
        )
        return ()

    factories: list[LogicalTimeFactory] = []
    for accessor_name in _TIME_FACTORY_ACCESSORS:
        accessor = getattr(ambassador, accessor_name, None)
        if not callable(accessor):
            continue
        try:
            factory = accessor()
        except Exception:
            _LOGGER.debug(
                "Skipping %s from optional RTI provider %s",
                accessor_name,
                entry_point.name,
                exc_info=True,
            )
            continue
        if isinstance(factory, LogicalTimeFactory):
            factories.append(factory)
    return tuple(factories)


class RtiFactoryFactory:
    """Discover 2010 providers through the edition-specific entry-point group."""

    _ENTRY_POINT_GROUP = "hla.rti1516e.factories"

    @classmethod
    def getRtiFactory(cls, name: str | None = None) -> RtiFactory:
        requested = name if name is not None else os.getenv("HLA_RTI1516E_FACTORY_NAME")
        if requested is not None:
            aliased_factory = cls._factory_from_entry_point_alias(requested)
            if aliased_factory is not None:
                return aliased_factory
        factories = cls.getAvailableRtiFactories()
        if requested is None and factories:
            return factories[0]
        for factory in factories:
            if factory.rtiName() == requested:
                return factory
        if requested is None:
            raise RTIinternalError("Cannot find factory")
        raise RTIinternalError(f"Cannot find factory matching {requested}")

    @classmethod
    def getAvailableRtiFactories(cls) -> list[RtiFactory]:
        return [entry_point.load()() for entry_point in cls._entry_points()]

    @classmethod
    def _factory_from_entry_point_alias(cls, name: str) -> RtiFactory | None:
        """Select an installed transport alias without comparing vendor names."""

        matching = [
            entry_point
            for entry_point in cls._entry_points()
            if entry_point.name == name
        ]
        if not matching:
            return None
        if len(matching) != 1:
            raise RTIinternalError(
                f"More than one Python 2010 provider uses alias {name}"
            )
        return matching[0].load()()

    @classmethod
    def _entry_points(cls) -> EntryPoints:
        return entry_points(group=cls._ENTRY_POINT_GROUP)


class LogicalTimeFactoryFactory:
    """Discover logical-time providers using the Java 2010 default policy."""

    _ENTRY_POINT_GROUP = "hla.rti1516e.time_factories"

    @classmethod
    def getLogicalTimeFactory(
        cls, name: str | type[LogicalTimeFactory] | None = None
    ) -> LogicalTimeFactory | None:
        # The Java 2010 helper has both a String overload and a Class<T>
        # overload.  Python keeps one entry point and preserves both
        # selection modes without conflating an edition's time factories.
        factories = cls._available_factories()
        if isinstance(name, type):
            return next(
                (factory for factory in factories if isinstance(factory, name)), None
            )
        requested = "HLAfloat64Time" if name in (None, "") else name
        for factory in factories:
            if factory.getName() == requested:
                return factory
        return None

    @classmethod
    def getAvailableLogicalTimeFactories(cls) -> set[LogicalTimeFactory]:
        return set(cls._available_factories())

    @classmethod
    def _available_factories(cls) -> list[LogicalTimeFactory]:
        """Load explicit time providers, then the standard RTI-provider route.

        The Java 1516e helper discovers ``LogicalTimeFactory`` implementations
        with its service registry.  Python packages already have the edition's
        ``RtiFactoryFactory`` boundary, so an installed RTI provider can expose
        its standard ``getTimeFactory`` (and optional float64 companion) without
        requiring a second, unconfigured entry-point package.  Explicit time
        entry points remain first-class for providers that ship standalone time
        implementations.
        """

        discovered: list[LogicalTimeFactory] = []
        explicit = entry_points(group=cls._ENTRY_POINT_GROUP)
        for entry_point in explicit:
            factory = _explicit_time_factory(entry_point)
            if factory is not None:
                discovered.append(factory)
        if discovered:
            return discovered

        # Fall back to the edition-specific RTI factory providers.  A vendor
        # JPype provider may legitimately be unavailable until its optional
        # runtime/JAR is configured; skip that provider while retaining any
        # usable native or separately configured provider.
        for entry_point in RtiFactoryFactory._entry_points():
            for factory in _provider_time_factories(entry_point):
                if not any(
                    existing.getName() == factory.getName() for existing in discovered
                ):
                    discovered.append(factory)
        return discovered


__all__ = [
    "CPP_NAMESPACE",
    "FEDERATE_AMBASSADOR_METHODS",
    "FEDERATE_AMBASSADOR_OVERLOAD_COUNTS",
    "FEDERATE_AMBASSADOR_PARAMETER_TYPES",
    "FEDERATE_AMBASSADOR_RETURN_TYPES",
    "JAVA_PACKAGE",
    "RTIAMBASSADOR_METHODS",
    "RTIAMBASSADOR_OVERLOAD_COUNTS",
    "RTIAMBASSADOR_PARAMETER_TYPES",
    "RTIAMBASSADOR_RETURN_TYPES",
    "STANDARD_EDITION",
    "AttributeHandleFactory",
    "AttributeHandleSetFactory",
    "AttributeHandleValueMapFactory",
    "AttributeSetRegionSetPairListFactory",
    "DimensionHandleFactory",
    "DimensionHandleSetFactory",
    "FederateAmbassador",
    "FederateHandleFactory",
    "FederateHandleSetFactory",
    "InteractionClassHandleFactory",
    "LogicalTimeFactory",
    "LogicalTimeFactoryFactory",
    "MethodSpec",
    "NullFederateAmbassador",
    "ObjectClassHandleFactory",
    "ObjectInstanceHandleFactory",
    "ParameterHandleFactory",
    "ParameterHandleValueMapFactory",
    "RTIambassador",
    "RegionHandleSetFactory",
    "RtiFactory",
    "RtiFactoryFactory",
    "TransportationTypeHandleFactory",
]
