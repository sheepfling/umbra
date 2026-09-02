"""Discovery and compatibility façade for the IEEE 1516.1-2010 surface."""

from __future__ import annotations

from importlib.metadata import entry_points
import os
from typing import Any

from . import abstract as _abstract
from . import contracts as _contracts
from .abstract import *
from .contracts import *
from .exceptions import RTIinternalError
from .values import *


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

        matching = [entry_point for entry_point in cls._entry_points() if entry_point.name == name]
        if not matching:
            return None
        if len(matching) != 1:
            raise RTIinternalError(f"More than one Python 2010 provider uses alias {name}")
        return matching[0].load()()

    @classmethod
    def _entry_points(cls) -> Any:
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
            return next((factory for factory in factories if isinstance(factory, name)), None)
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
            try:
                factory = entry_point.load()()
            except Exception:
                continue
            if isinstance(factory, LogicalTimeFactory):
                discovered.append(factory)
        if discovered:
            return discovered

        # Fall back to the edition-specific RTI factory providers.  A vendor
        # JPype provider may legitimately be unavailable until its optional
        # runtime/JAR is configured; skip that provider while retaining any
        # usable native or separately configured provider.
        for entry_point in RtiFactoryFactory._entry_points():
            try:
                provider = entry_point.load()()
                ambassador = provider.getRtiAmbassador()
                for accessor in ("getTimeFactory", "getFloat64TimeFactory"):
                    candidate = getattr(ambassador, accessor, None)
                    if not callable(candidate):
                        continue
                    value = candidate()
                    if isinstance(value, LogicalTimeFactory) and not any(
                        existing.getName() == value.getName() for existing in discovered
                    ):
                        discovered.append(value)
            except Exception:
                continue
        return discovered


__all__ = [
    *_abstract.__all__,
    *_contracts.__all__,
    "RtiFactoryFactory", "LogicalTimeFactoryFactory",
]
