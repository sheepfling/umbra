"""Implemented foundation of the IEEE 1516.1-2025 Java-shaped API."""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from importlib.metadata import entry_points
import os
from typing import Any

from .exceptions import RTIinternalError, UnsupportedCallbackModel


class AdditionalSettingsResultCode(Enum):
    SETTINGS_IGNORED = "SETTINGS_IGNORED"
    SETTINGS_FAILED_TO_PARSE = "SETTINGS_FAILED_TO_PARSE"
    SETTINGS_APPLIED = "SETTINGS_APPLIED"


class CallbackModel(Enum):
    HLA_IMMEDIATE = "HLA_IMMEDIATE"
    HLA_EVOKED = "HLA_EVOKED"


def _require_callback_model(value: object) -> CallbackModel:
    """Reject values outside the two standard callback-model enum members.

    Python does not enforce annotations at a call boundary. Providers therefore
    share this guard so an invalid value has the same observable outcome as the
    C++ binding's ``UnsupportedCallbackModel`` service error.
    """

    if not isinstance(value, CallbackModel):
        raise UnsupportedCallbackModel(f"Unsupported callback model: {value!r}")
    return value


@dataclass(frozen=True, slots=True)
class ConfigurationResult:
    """The standard Java API result object, represented as an immutable value."""

    configurationUsed: bool
    addressUsed: bool
    additionalSettingsResultCode: AdditionalSettingsResultCode
    message: str = ""


@dataclass(frozen=True, slots=True)
class FederationExecutionInformation:
    """The Java callback record identifying one federation execution."""

    federationExecutionName: str
    logicalTimeImplementationName: str


class FederationExecutionInformationSet(frozenset[FederationExecutionInformation]):
    """Immutable Python callback snapshot of the Java information set."""


class RtiConfiguration:
    """Java-shaped mutable connection configuration value."""

    def __init__(self) -> None:
        self._configuration_name = ""
        self._rti_address = ""
        self._additional_settings = ""

    @classmethod
    def createConfiguration(cls) -> "RtiConfiguration":
        return cls()

    def withConfigurationName(self, configurationName: str) -> "RtiConfiguration":
        self._configuration_name = str(configurationName)
        return self

    def withRtiAddress(self, rtiAddress: str) -> "RtiConfiguration":
        self._rti_address = str(rtiAddress)
        return self

    def withAdditionalSettings(self, additionalSettings: str) -> "RtiConfiguration":
        self._additional_settings = str(additionalSettings)
        return self

    def configurationName(self) -> str:
        return self._configuration_name

    def rtiAddress(self) -> str:
        return self._rti_address

    def additionalSettings(self) -> str:
        return self._additional_settings


def _resolve_connect_arguments(
    configuration: RtiConfiguration | object | None,
    credentials: object | None,
) -> tuple[RtiConfiguration | None, object | None]:
    """Adapt Java's connect overloads to Python's optional arguments."""

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


class FederateAmbassador(ABC):
    """Receives RTI callbacks; callback families arrive with native services."""

    def connectionLost(self, faultDescription: str) -> None:
        """Report connection loss when the provider supports that service."""

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        """Report the federation executions requested through the RTI."""


class RTIambassador(ABC):
    """The currently implemented connected/unjoined subset of the standard API."""

    @abstractmethod
    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: RtiConfiguration | object | None = None,
        credentials: object | None = None,
    ) -> ConfigurationResult:
        """Connect using the Java overload represented by the supplied values."""

    @abstractmethod
    def disconnect(self) -> None:
        """Disconnect this ambassador."""

    @abstractmethod
    def evokeCallback(self, approximateMinimumTimeInSeconds: float) -> bool:
        """Deliver at most one queued callback."""

    @abstractmethod
    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        """Deliver queued callbacks during the requested interval."""

    @abstractmethod
    def enableCallbacks(self) -> None:
        """Enable callback delivery."""

    @abstractmethod
    def disableCallbacks(self) -> None:
        """Disable callback delivery without discarding queued callbacks."""

    @abstractmethod
    def listFederationExecutions(self) -> None:
        """Request ``reportFederationExecutions`` through the callback model."""

    @abstractmethod
    def createFederationExecution(
        self,
        federationName: str,
        fomModule: str,
        logicalTimeImplementationName: str = "",
    ) -> None:
        """Create a federation from the scalar Java/C++ FOM overload."""

    @abstractmethod
    def destroyFederationExecution(self, federationName: str) -> None:
        """Destroy an unjoined federation execution."""


class RtiFactory(ABC):
    """Python analogue of the standard Java service-provider factory."""

    @abstractmethod
    def getRtiAmbassador(self) -> RTIambassador:
        """Return a new RTI ambassador."""

    @abstractmethod
    def getEncoderFactory(self) -> Any:
        """Return the provider encoder factory when that capability is available."""

    @abstractmethod
    def rtiName(self) -> str:
        """Return the provider name."""

    @abstractmethod
    def rtiVersion(self) -> str:
        """Return the provider version."""


class RtiFactoryFactory:
    """Discover providers like the Java API's ``ServiceLoader``-based helper."""

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
        factories: list[RtiFactory] = []
        for entry_point in cls._entry_points():
            factory_type = entry_point.load()
            factories.append(factory_type())
        return factories

    @classmethod
    def _factory_from_entry_point_alias(cls, name: str) -> RtiFactory | None:
        """Select an installed transport before it needs to initialize itself.

        Most entry-point names equal ``RtiFactory.rtiName()``. A Java bridge is
        different: its actual standard factory name is learned only after its
        JVM and vendor JAR have been selected. An entry-point alias therefore
        lets callers choose the bridge (for example ``"java"``) without
        forcing every installed Java RTI to start during unrelated lookup.
        """

        matching_entry_points = [
            entry_point
            for entry_point in cls._entry_points()
            if entry_point.name == name
        ]
        if not matching_entry_points:
            return None
        if len(matching_entry_points) != 1:
            raise RTIinternalError(f"More than one Python RTI provider uses alias {name}")
        factory_type = matching_entry_points[0].load()
        return factory_type()

    @classmethod
    def _entry_points(cls) -> Any:
        return entry_points(group=cls._ENTRY_POINT_GROUP)
