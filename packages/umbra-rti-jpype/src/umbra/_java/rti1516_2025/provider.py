"""Transport-neutral Python adapters over the Java 2025 RTI binding."""

from __future__ import annotations

from typing import Any, Callable, TypeVar

from hla.rti1516_2025 import (
    AdditionalSettingsResultCode,
    CallbackModel,
    ConfigurationResult,
    FederateAmbassador,
    RTIambassador,
    RtiConfiguration,
    RtiFactory,
)
from hla.rti1516_2025.core import _require_callback_model, _resolve_connect_arguments
from hla.rti1516_2025.exceptions import RTIexception, RTIinternalError, exceptionForName

from ._runtime import JPypeJavaRuntime, JavaCallbackBinding, JavaRuntime
from .config import JavaProviderConfiguration

_Result = TypeVar("_Result")


def _enum_name(value: object) -> str:
    """Read either a Java enum's ``name()`` or a test-double string value."""

    name = getattr(value, "name", None)
    if callable(name):
        return str(name())
    if name is not None:
        return str(name)
    return str(value)


def _configuration_result(java_result: object) -> ConfigurationResult:
    setting_name = _enum_name(getattr(java_result, "additionalSettingsResultCode"))
    try:
        setting_code = AdditionalSettingsResultCode[setting_name]
    except KeyError as error:
        raise RTIinternalError(
            f"Java RTI returned an unknown AdditionalSettingsResultCode: {setting_name}"
        ) from error
    return ConfigurationResult(
        configurationUsed=bool(getattr(java_result, "configurationUsed")),
        addressUsed=bool(getattr(java_result, "addressUsed")),
        additionalSettingsResultCode=setting_code,
        message=str(getattr(java_result, "message")),
    )


class JavaRTIambassador(RTIambassador):
    """A public Python ambassador backed by a Java ``RTIambassador``."""

    def __init__(self, implementation: object, runtime: JavaRuntime) -> None:
        self._implementation = implementation
        self._runtime = runtime
        self._callback_binding: JavaCallbackBinding | None = None

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: RtiConfiguration | object | None = None,
        credentials: object | None = None,
    ) -> ConfigurationResult:
        callbackModel = _require_callback_model(callbackModel)
        configuration, credentials = _resolve_connect_arguments(configuration, credentials)
        callback_binding = self._runtime.bind_federate_ambassador(federateAmbassador)
        arguments: list[object] = [
            callback_binding.proxy,
            self._runtime.callback_model(callbackModel),
        ]
        if configuration is not None:
            arguments.append(self._runtime.rti_configuration(configuration))
        if credentials is not None:
            arguments.append(self._runtime.credentials(credentials))
        result = self._call(
            getattr(self._implementation, "connect"),
            *arguments,
        )
        # A Java proxy must outlive the connection, even if Java keeps only a
        # weak reference to it.
        self._callback_binding = callback_binding
        return _configuration_result(result)

    def disconnect(self) -> None:
        self._call(getattr(self._implementation, "disconnect"))
        self._callback_binding = None

    def evokeCallback(self, approximateMinimumTimeInSeconds: float) -> bool:
        return self._call(
            getattr(self._implementation, "evokeCallback"),
            approximateMinimumTimeInSeconds,
        )

    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        return self._call(
            getattr(self._implementation, "evokeMultipleCallbacks"),
            approximateMinimumTimeInSeconds,
            approximateMaximumTimeInSeconds,
        )

    def enableCallbacks(self) -> None:
        self._call(getattr(self._implementation, "enableCallbacks"))

    def disableCallbacks(self) -> None:
        self._call(getattr(self._implementation, "disableCallbacks"))

    def listFederationExecutions(self) -> None:
        self._call(getattr(self._implementation, "listFederationExecutions"))

    def createFederationExecution(
        self,
        federationName: str,
        fomModule: str,
        logicalTimeImplementationName: str = "",
    ) -> None:
        self._call(
            getattr(self._implementation, "createFederationExecution"),
            federationName,
            fomModule,
            logicalTimeImplementationName,
        )

    def destroyFederationExecution(self, federationName: str) -> None:
        self._call(getattr(self._implementation, "destroyFederationExecution"), federationName)

    def unwrap_java_object(self) -> object:
        """Return the underlying Java object for explicit migration-only use.

        This method is deliberately absent from the public ``RTIambassador``
        contract. Code that calls it is coupled to Java and cannot be swapped
        to the pybind11 provider unchanged.
        """

        return self._implementation

    def _call(self, function: Callable[..., _Result], *args: object) -> _Result:
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java RTI call failed: {error}") from error
            raise exceptionForName(name, str(error)) from error


class JavaRtiFactory(RtiFactory):
    """A discovered provider that delegates to a selected Java RTI factory."""

    def __init__(
        self,
        configuration: JavaProviderConfiguration | None = None,
        *,
        runtime: JavaRuntime | None = None,
    ) -> None:
        self._configuration = configuration or JavaProviderConfiguration.from_environment()
        self._runtime = runtime or JPypeJavaRuntime()
        self._implementation: object | None = None

    def getRtiAmbassador(self) -> RTIambassador:
        implementation = self._call(getattr(self._java_factory(), "getRtiAmbassador"))
        return JavaRTIambassador(implementation, self._runtime)

    def getEncoderFactory(self) -> Any:
        raise RTIinternalError(
            "The Java RTI has an EncoderFactory, but the shared Python encoding API is not implemented"
        )

    def rtiName(self) -> str:
        return str(self._call(getattr(self._java_factory(), "rtiName")))

    def rtiVersion(self) -> str:
        return str(self._call(getattr(self._java_factory(), "rtiVersion")))

    def unwrap_java_factory(self) -> object:
        """Return the underlying Java ``RtiFactory`` for explicit migration use."""

        return self._java_factory()

    def unwrap_java_encoder_factory(self) -> object:
        """Return the Java encoder only as an explicit non-portable escape hatch."""

        return self._call(getattr(self._java_factory(), "getEncoderFactory"))

    def _java_factory(self) -> object:
        if self._implementation is None:
            self._implementation = self._runtime.get_rti_factory(self._configuration)
        return self._implementation

    def _call(self, function: Callable[..., _Result], *args: object) -> _Result:
        try:
            return function(*args)
        except RTIexception:
            raise
        except Exception as error:
            name = self._runtime.exception_name(error)
            if name is None:
                raise RTIinternalError(f"Java RTI call failed: {error}") from error
            raise exceptionForName(name, str(error)) from error
