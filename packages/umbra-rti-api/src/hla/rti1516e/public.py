"""Public exports for the IEEE 1516.1-2010 Java-shaped Python API."""

from __future__ import annotations

from .byte_types import BytesLike, WritableBytes
from .contracts import (
    CPP_NAMESPACE,
    FEDERATE_AMBASSADOR_METHODS,
    FEDERATE_AMBASSADOR_OVERLOAD_COUNTS,
    FEDERATE_AMBASSADOR_PARAMETER_TYPES,
    FEDERATE_AMBASSADOR_RETURN_TYPES,
    JAVA_PACKAGE,
    MethodSpec,
    NullFederateAmbassador,
    RTIAMBASSADOR_METHODS,
    RTIAMBASSADOR_OVERLOAD_COUNTS,
    RTIAMBASSADOR_PARAMETER_TYPES,
    RTIAMBASSADOR_RETURN_TYPES,
    STANDARD_EDITION,
    FederateAmbassador,
    RTIambassador,
)
from .core import LogicalTimeFactoryFactory, RtiFactoryFactory
from .abstract import (
    AttributeHandleFactory,
    AttributeHandleSetFactory,
    AttributeHandleValueMapFactory,
    AttributeSetRegionSetPairListFactory,
    DimensionHandleFactory,
    DimensionHandleSetFactory,
    FederateHandleFactory,
    FederateHandleSetFactory,
    InteractionClassHandleFactory,
    LogicalTimeFactory,
    ObjectClassHandleFactory,
    ObjectInstanceHandleFactory,
    ParameterHandleFactory,
    ParameterHandleValueMapFactory,
    RegionHandleSetFactory,
    RtiFactory,
    TransportationTypeHandleFactory,
)
from .values import *
from .values import __all__ as _value_all

# The official Java API declares these callback records as nested interfaces
# on ``FederateAmbassador``.  Python keeps the records importable as ordinary
# top-level value types while retaining the Java-shaped nested lookup for code
# that mirrors the standard package literally.
FederateAmbassador.SupplementalReflectInfo = SupplementalReflectInfo  # type: ignore[attr-defined]
FederateAmbassador.SupplementalReceiveInfo = SupplementalReceiveInfo  # type: ignore[attr-defined]
FederateAmbassador.SupplementalRemoveInfo = SupplementalRemoveInfo  # type: ignore[attr-defined]


__all__ = [
    "BytesLike", "WritableBytes", "STANDARD_EDITION", "JAVA_PACKAGE", "CPP_NAMESPACE",
    "MethodSpec", "RTIAMBASSADOR_METHODS", "RTIAMBASSADOR_OVERLOAD_COUNTS",
    "FEDERATE_AMBASSADOR_METHODS", "FEDERATE_AMBASSADOR_OVERLOAD_COUNTS",
    "RTIAMBASSADOR_PARAMETER_TYPES", "FEDERATE_AMBASSADOR_PARAMETER_TYPES",
    "RTIAMBASSADOR_RETURN_TYPES", "FEDERATE_AMBASSADOR_RETURN_TYPES",
    "FederateAmbassador", "NullFederateAmbassador", "RTIambassador", "RtiFactory",
    "RtiFactoryFactory", "LogicalTimeFactoryFactory", "LogicalTimeFactory",
    "AttributeHandleFactory", "AttributeHandleSetFactory", "AttributeHandleValueMapFactory",
    "AttributeSetRegionSetPairListFactory", "DimensionHandleFactory", "DimensionHandleSetFactory",
    "FederateHandleFactory", "FederateHandleSetFactory", "InteractionClassHandleFactory",
    "ObjectClassHandleFactory", "ObjectInstanceHandleFactory", "ParameterHandleFactory",
    "ParameterHandleValueMapFactory", "RegionHandleSetFactory", "TransportationTypeHandleFactory",
    *_value_all,
]
