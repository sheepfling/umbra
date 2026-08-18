"""Umbra's private implementation of the public 2025 HLA Python API."""

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
from hla.rti1516_2025.auth import HLAnoCredentials
from hla.rti1516_2025.core import _require_callback_model, _resolve_connect_arguments
from hla.rti1516_2025.exceptions import RTIinternalError, exceptionForName

from . import _native

_Result = TypeVar("_Result")


class _UmbraRTIambassador(RTIambassador):
    def __init__(self, implementation: _native.NativeAmbassador) -> None:
        self._implementation = implementation

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: RtiConfiguration | object | None = None,
        credentials: object | None = None,
    ) -> ConfigurationResult:
        callbackModel = _require_callback_model(callbackModel)
        configuration, credentials = _resolve_connect_arguments(configuration, credentials)
        if credentials is not None and not isinstance(credentials, HLAnoCredentials):
            raise RTIinternalError("Umbra currently supports only HLAnoCredentials")
        result = self._call(
            self._implementation.connect,
            federateAmbassador,
            {
                CallbackModel.HLA_IMMEDIATE: "immediate",
                CallbackModel.HLA_EVOKED: "evoked",
            }[callbackModel],
            configuration.configurationName() if configuration is not None else "",
            configuration.rtiAddress() if configuration is not None else "",
            configuration.additionalSettings() if configuration is not None else "",
            configuration is not None,
            credentials is not None,
        )
        return ConfigurationResult(
            configurationUsed=result.configuration_used,
            addressUsed=result.address_used,
            additionalSettingsResultCode={
                "ignored": AdditionalSettingsResultCode.SETTINGS_IGNORED,
                "failed_to_parse": AdditionalSettingsResultCode.SETTINGS_FAILED_TO_PARSE,
                "applied": AdditionalSettingsResultCode.SETTINGS_APPLIED,
            }[result.additional_settings_result],
            message=result.message,
        )

    def disconnect(self) -> None:
        self._call(self._implementation.disconnect)

    def evokeCallback(self, approximateMinimumTimeInSeconds: float) -> bool:
        return self._call(self._implementation.evoke_callback, approximateMinimumTimeInSeconds)

    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        return self._call(
            self._implementation.evoke_multiple_callbacks,
            approximateMinimumTimeInSeconds,
            approximateMaximumTimeInSeconds,
        )

    def enableCallbacks(self) -> None:
        self._call(self._implementation.enable_callbacks)

    def disableCallbacks(self) -> None:
        self._call(self._implementation.disable_callbacks)

    def listFederationExecutions(self) -> None:
        self._call(self._implementation.list_federation_executions)

    def createFederationExecution(
        self,
        federationName: str,
        fomModule: str,
        logicalTimeImplementationName: str = "",
    ) -> None:
        self._call(
            self._implementation.create_federation_execution,
            federationName,
            fomModule,
            logicalTimeImplementationName,
        )

    def destroyFederationExecution(self, federationName: str) -> None:
        self._call(self._implementation.destroy_federation_execution, federationName)

    @staticmethod
    def _call(function: Callable[..., _Result], *args: object) -> _Result:
        try:
            return function(*args)
        except _native.NativeRtiError as error:
            name, separator, message = str(error).partition(": ")
            raise exceptionForName(name, message if separator else name) from error


class UmbraRtiFactory(RtiFactory):
    """The discovered Umbra provider for ``hla.rti1516_2025``."""

    def getRtiAmbassador(self) -> RTIambassador:
        try:
            return _UmbraRTIambassador(_native.NativeAmbassador())
        except _native.NativeRtiError as error:
            name, separator, message = str(error).partition(": ")
            raise exceptionForName(name, message if separator else name) from error

    def getEncoderFactory(self) -> Any:
        raise RTIinternalError("The 2025 encoding API is not implemented")

    def rtiName(self) -> str:
        return _native.rti_name()

    def rtiVersion(self) -> str:
        return _native.rti_version()
