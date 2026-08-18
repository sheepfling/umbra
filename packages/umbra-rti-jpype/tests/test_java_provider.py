from __future__ import annotations

import os
import unittest

from hla.rti1516_2025 import (
    CallbackModel,
    FederateAmbassador,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    RtiConfiguration,
)
from hla.rti1516_2025.auth import HLAnoCredentials
from hla.rti1516_2025.exceptions import AlreadyConnected, ConnectionFailed, RTIinternalError
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory
from umbra._java.rti1516_2025._runtime import JavaCallbackBinding


class _FakeJavaError(Exception):
    def __init__(self, name: str, message: str) -> None:
        super().__init__(message)
        self.name = name


class _FakeJavaEnum:
    def __init__(self, name: str) -> None:
        self._name = name

    def name(self) -> str:
        return self._name


class _FakeJavaConfigurationResult:
    configurationUsed = True
    addressUsed = False
    additionalSettingsResultCode = _FakeJavaEnum("SETTINGS_APPLIED")
    message = "connected through Java"


class _FakeCallbackProxy:
    def __init__(self, target: FederateAmbassador) -> None:
        self._target = target

    def connectionLost(self, description: str) -> None:
        self._target.connectionLost(description)

    def reportFederationExecutions(self, report: object) -> None:
        self._target.reportFederationExecutions(
            FederationExecutionInformationSet(
                FederationExecutionInformation(
                    information.federationExecutionName,
                    information.logicalTimeImplementationName,
                )
                for information in report  # type: ignore[union-attr]
            )
        )


class _FakeJavaFederationExecutionInformation:
    federationExecutionName = "Fake Federation"
    logicalTimeImplementationName = "HLAinteger64Time"


class _FakeJavaAmbassador:
    def __init__(self) -> None:
        self.connected = False
        self.callback_proxy: _FakeCallbackProxy | None = None
        self.callback_model: object | None = None
        self.connect_arguments: tuple[object, ...] = ()
        self.connect_error_name: str | None = None
        self.callbacks_enabled = True
        self.federation_executions: dict[str, str] = {"Fake Federation": "HLAinteger64Time"}

    def connect(self, callback_proxy: _FakeCallbackProxy, callback_model: object, *arguments: object) -> _FakeJavaConfigurationResult:
        if self.connect_error_name is not None:
            raise _FakeJavaError(self.connect_error_name, "Java provider error")
        if self.connected:
            raise _FakeJavaError("AlreadyConnected", "already connected")
        self.connected = True
        self.callback_proxy = callback_proxy
        self.callback_model = callback_model
        self.connect_arguments = arguments
        return _FakeJavaConfigurationResult()

    def disconnect(self) -> None:
        self.connected = False

    def evokeCallback(self, seconds: float) -> bool:
        return seconds >= 0.0

    def evokeMultipleCallbacks(self, minimum: float, maximum: float) -> bool:
        return minimum <= maximum

    def enableCallbacks(self) -> None:
        self.callbacks_enabled = True

    def disableCallbacks(self) -> None:
        self.callbacks_enabled = False

    def listFederationExecutions(self) -> None:
        if not self.connected:
            raise _FakeJavaError("NotConnected", "not connected")
        self.callback_proxy.reportFederationExecutions(
            [
                type(
                    "Information",
                    (),
                    {
                        "federationExecutionName": name,
                        "logicalTimeImplementationName": time_name,
                    },
                )()
                for name, time_name in self.federation_executions.items()
            ]
        )  # type: ignore[union-attr]

    def createFederationExecution(self, name: str, fom: str, time_name: str) -> None:
        self.federation_executions[name] = time_name

    def destroyFederationExecution(self, name: str) -> None:
        self.federation_executions.pop(name, None)


class _FakeJavaFactory:
    def __init__(self, ambassador: _FakeJavaAmbassador) -> None:
        self.ambassador = ambassador
        self.encoder_factory = object()

    def getRtiAmbassador(self) -> _FakeJavaAmbassador:
        return self.ambassador

    def getEncoderFactory(self) -> object:
        return self.encoder_factory

    def rtiName(self) -> str:
        return "Fake Java RTI"

    def rtiVersion(self) -> str:
        return "2025.test"


class _FakeJavaRuntime:
    def __init__(self) -> None:
        self.ambassador = _FakeJavaAmbassador()
        self.factory = _FakeJavaFactory(self.ambassador)
        self.configuration: JavaProviderConfiguration | None = None

    def get_rti_factory(self, configuration: JavaProviderConfiguration) -> _FakeJavaFactory:
        self.configuration = configuration
        return self.factory

    def callback_model(self, callback_model: CallbackModel) -> str:
        return f"java:{callback_model.name}"

    def bind_federate_ambassador(self, federate_ambassador: FederateAmbassador) -> JavaCallbackBinding:
        proxy = _FakeCallbackProxy(federate_ambassador)
        return JavaCallbackBinding(proxy=proxy, target=proxy)

    def rti_configuration(self, configuration: RtiConfiguration) -> tuple[str, str, str, str]:
        return (
            "java-configuration",
            configuration.configurationName(),
            configuration.rtiAddress(),
            configuration.additionalSettings(),
        )

    def credentials(self, credentials: HLAnoCredentials) -> tuple[str, str, bytes]:
        return ("java-credentials", credentials.getType(), credentials.getData())

    def exception_name(self, error: BaseException) -> str | None:
        return getattr(error, "name", None)


class _RecordingFederateAmbassador(FederateAmbassador):
    def __init__(self) -> None:
        self.connection_losses: list[str] = []
        self.federation_execution_reports: list[FederationExecutionInformationSet] = []

    def connectionLost(self, faultDescription: str) -> None:
        self.connection_losses.append(faultDescription)

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        self.federation_execution_reports.append(report)


class JavaProviderTest(unittest.TestCase):
    def setUp(self) -> None:
        self.runtime = _FakeJavaRuntime()
        self.configuration = JavaProviderConfiguration(
            classpath=("vendor-rti.jar",),
            rti_factory_name="Fake Java RTI",
        )
        self.factory = JavaRtiFactory(self.configuration, runtime=self.runtime)

    def test_java_provider_adapts_to_the_shared_python_contract(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()

        result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        self.assertEqual(self.factory.rtiName(), "Fake Java RTI")
        self.assertEqual(self.factory.rtiVersion(), "2025.test")
        self.assertTrue(result.configurationUsed)
        self.assertEqual(result.additionalSettingsResultCode.name, "SETTINGS_APPLIED")
        self.assertEqual(self.runtime.ambassador.callback_model, "java:HLA_EVOKED")
        self.runtime.ambassador.callback_proxy.connectionLost("network unavailable")  # type: ignore[union-attr]
        self.assertEqual(callbacks.connection_losses, ["network unavailable"])
        ambassador.disconnect()

    def test_java_exceptions_are_translated_at_the_provider_edge(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_IMMEDIATE)

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callbacks, CallbackModel.HLA_IMMEDIATE)

    def test_java_exception_names_outside_the_initial_service_slice_remain_typed(self) -> None:
        self.runtime.ambassador.connect_error_name = "ConnectionFailed"

        with self.assertRaises(ConnectionFailed):
            self.factory.getRtiAmbassador().connect(
                _RecordingFederateAmbassador(),
                CallbackModel.HLA_EVOKED,
            )

    def test_java_provider_selects_each_standard_connect_overload(self) -> None:
        configuration = RtiConfiguration.createConfiguration().withConfigurationName("fake")
        credentials = HLAnoCredentials()
        expected_arguments = (
            ((), ()),
            ((configuration,), (("java-configuration", "fake", "", ""),)),
            ((credentials,), (("java-credentials", "HLAnoCredentials", b""),)),
            (
                (configuration, credentials),
                (
                    ("java-configuration", "fake", "", ""),
                    ("java-credentials", "HLAnoCredentials", b""),
                ),
            ),
        )
        for supplied, expected in expected_arguments:
            with self.subTest(supplied=supplied):
                ambassador = self.factory.getRtiAmbassador()
                ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED, *supplied)
                self.assertEqual(self.runtime.ambassador.connect_arguments, expected)
                ambassador.disconnect()

    def test_java_provider_converts_federation_execution_report_callbacks(self) -> None:
        callback = _RecordingFederateAmbassador()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        ambassador.listFederationExecutions()

        self.assertEqual(
            callback.federation_execution_reports,
            [
                FederationExecutionInformationSet(
                    [FederationExecutionInformation("Fake Federation", "HLAinteger64Time")]
                )
            ],
        )
        ambassador.disconnect()

    def test_java_provider_forwards_federation_create_and_destroy_calls(self) -> None:
        callback = _RecordingFederateAmbassador()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        ambassador.createFederationExecution("Created Federation", "example.xml", "HLAinteger64Time")
        ambassador.listFederationExecutions()
        self.assertIn(
            FederationExecutionInformation("Created Federation", "HLAinteger64Time"),
            callback.federation_execution_reports[-1],
        )
        ambassador.destroyFederationExecution("Created Federation")
        ambassador.disconnect()

    def test_raw_java_access_is_explicit_and_not_the_public_factory_result(self) -> None:
        self.assertIs(self.factory.unwrap_java_factory(), self.runtime.factory)
        self.assertIs(self.factory.unwrap_java_encoder_factory(), self.runtime.factory.encoder_factory)
        with self.assertRaises(RTIinternalError):
            self.factory.getEncoderFactory()

    def test_environment_configuration_does_not_reuse_the_standard_factory_name(self) -> None:
        configuration = JavaProviderConfiguration.from_environment(
            {
                "UMBRA_JAVA_RTI_CLASSPATH": os.pathsep.join(("first.jar", "second.jar")),
                "UMBRA_JAVA_RTI_FACTORY_NAME": "Vendor RTI",
                "UMBRA_JAVA_RTI_JVM_PATH": "java.dll",
                "HLA_RTI_FACTORY_NAME": "must-not-be-read-here",
            }
        )

        self.assertEqual(configuration.classpath, ("first.jar", "second.jar"))
        self.assertEqual(configuration.rti_factory_name, "Vendor RTI")
        self.assertEqual(configuration.jvm_path, "java.dll")
