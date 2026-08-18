"""JPype-specific mechanics kept behind the Java provider boundary."""

from __future__ import annotations

from dataclasses import dataclass
from threading import Lock
from typing import Any, Protocol

from hla.rti1516_2025 import (
    CallbackModel,
    FederateAmbassador,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    RtiConfiguration,
)
from hla.rti1516_2025.auth import Credentials, HLAnoCredentials
from hla.rti1516_2025.exceptions import RTIinternalError

from .config import JavaProviderConfiguration


_JVM_CONFIGURATION_LOCK = Lock()
_STARTED_JVM_CONFIGURATION: JavaProviderConfiguration | None = None


@dataclass(frozen=True, slots=True)
class JavaCallbackBinding:
    """Keep both the Java proxy and its Python callback target alive."""

    proxy: object
    target: object


class JavaRuntime(Protocol):
    """Small seam that permits adapter tests without a JVM or vendor JAR."""

    def get_rti_factory(self, configuration: JavaProviderConfiguration) -> object:
        """Return the selected Java ``RtiFactory`` object."""

    def callback_model(self, callback_model: CallbackModel) -> object:
        """Return the matching Java ``CallbackModel`` enum member."""

    def bind_federate_ambassador(
        self,
        federate_ambassador: FederateAmbassador,
    ) -> JavaCallbackBinding:
        """Implement Java's callback interface with a retained Python proxy."""

    def rti_configuration(self, configuration: RtiConfiguration) -> object:
        """Create the matching Java configuration value."""

    def credentials(self, credentials: Credentials) -> object:
        """Create the matching Java credentials value."""

    def exception_name(self, error: BaseException) -> str | None:
        """Return the simple Java exception type name, when available."""


class _FederateAmbassadorCallback:
    """The implemented callback subset presented to Java through ``JProxy``."""

    def __init__(self, target: FederateAmbassador) -> None:
        self._target = target

    def connectionLost(self, fault_description: object) -> None:
        self._target.connectionLost(str(fault_description))

    def reportFederationExecutions(self, report: object) -> None:
        self._target.reportFederationExecutions(
            FederationExecutionInformationSet(
                FederationExecutionInformation(
                    str(getattr(information, "federationExecutionName")),
                    str(getattr(information, "logicalTimeImplementationName")),
                )
                for information in report  # type: ignore[union-attr]
            )
        )


class JPypeJavaRuntime:
    """Lazy production runtime; importing this module never starts a JVM."""

    def __init__(self) -> None:
        self._jpype: Any | None = None

    def get_rti_factory(self, configuration: JavaProviderConfiguration) -> object:
        jpype = self._ensure_jvm(configuration)
        factory_factory = jpype.JClass("hla.rti1516_2025.RtiFactoryFactory")
        try:
            if configuration.rti_factory_name is not None:
                return factory_factory.getRtiFactory(configuration.rti_factory_name)
            return factory_factory.getRtiFactory()
        except Exception as error:
            raise RTIinternalError(f"Java RtiFactoryFactory failed: {error}") from error

    def callback_model(self, callback_model: CallbackModel) -> object:
        jpype = self._require_started_jvm()
        callback_model_type = jpype.JClass("hla.rti1516_2025.CallbackModel")
        return getattr(callback_model_type, callback_model.name)

    def bind_federate_ambassador(
        self,
        federate_ambassador: FederateAmbassador,
    ) -> JavaCallbackBinding:
        jpype = self._require_started_jvm()
        callback_interface = jpype.JClass("hla.rti1516_2025.FederateAmbassador")
        target = _FederateAmbassadorCallback(federate_ambassador)
        return JavaCallbackBinding(
            proxy=jpype.JProxy(callback_interface, inst=target),
            target=target,
        )

    def rti_configuration(self, configuration: RtiConfiguration) -> object:
        jpype = self._require_started_jvm()
        result = jpype.JClass("hla.rti1516_2025.RtiConfiguration").createConfiguration()
        return result.withConfigurationName(configuration.configurationName()).withRtiAddress(
            configuration.rtiAddress()
        ).withAdditionalSettings(configuration.additionalSettings())

    def credentials(self, credentials: Credentials) -> object:
        jpype = self._require_started_jvm()
        if isinstance(credentials, HLAnoCredentials):
            return jpype.JClass("hla.rti1516_2025.auth.HLAnoCredentials")()
        byte_array = jpype.JArray(jpype.JByte)(credentials.getData())
        return jpype.JClass("hla.rti1516_2025.auth.Credentials")(credentials.getType(), byte_array)

    def exception_name(self, error: BaseException) -> str | None:
        try:
            return str(error.getClass().getSimpleName())  # type: ignore[attr-defined]
        except (AttributeError, TypeError):
            return None

    def _ensure_jvm(self, configuration: JavaProviderConfiguration) -> Any:
        global _STARTED_JVM_CONFIGURATION

        jpype = self._load_jpype()
        with _JVM_CONFIGURATION_LOCK:
            if jpype.isJVMStarted():
                if _STARTED_JVM_CONFIGURATION == configuration:
                    return jpype
                if configuration.classpath or configuration.jvm_path or configuration.jvm_options:
                    raise RTIinternalError(
                        "The JVM is already running with a different or unknown Java RTI configuration"
                    )
                return jpype

            arguments = list(configuration.jvm_options)
            keyword_arguments: dict[str, object] = {
                "classpath": list(configuration.classpath),
                "convertStrings": configuration.convert_strings,
            }
            if configuration.jvm_path is not None:
                keyword_arguments["jvmpath"] = configuration.jvm_path
            try:
                jpype.startJVM(*arguments, **keyword_arguments)
            except Exception as error:
                raise RTIinternalError(f"Could not start the JVM for the Java RTI: {error}") from error
            _STARTED_JVM_CONFIGURATION = configuration
        return jpype

    def _require_started_jvm(self) -> Any:
        jpype = self._load_jpype()
        if not jpype.isJVMStarted():
            raise RTIinternalError("The Java RTI JVM has not been started")
        return jpype

    def _load_jpype(self) -> Any:
        if self._jpype is None:
            try:
                import jpype
            except ImportError as error:
                raise RTIinternalError(
                    "Install the optional dependency with 'umbra-rti-jpype[jpype]' to use a Java RTI"
                ) from error
            self._jpype = jpype
        return self._jpype
