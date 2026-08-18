from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest

from hla.rti1516_2025 import (
    CallbackModel,
    FederateAmbassador,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
)
from hla.rti1516_2025.exceptions import AlreadyConnected
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory

from _mock_java_fixture import build_mock_java_rti, java_toolchain_available


JPYPE_AVAILABLE = importlib.util.find_spec("jpype") is not None


class _RecordingFederateAmbassador(FederateAmbassador):
    def __init__(self) -> None:
        self.connection_losses: list[str] = []
        self.federation_execution_reports: list[FederationExecutionInformationSet] = []

    def connectionLost(self, faultDescription: str) -> None:
        self.connection_losses.append(faultDescription)

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        self.federation_execution_reports.append(report)


@unittest.skipUnless(JPYPE_AVAILABLE and java_toolchain_available(), "requires JPype and a JDK")
class JPypeMockIntegrationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        import jpype

        if jpype.isJVMStarted():
            raise unittest.SkipTest("the fixture classpath must be present before the JVM starts")
        # JPype does not permit a safe JVM restart, so leave this temporary
        # directory for process cleanup after the one integration JVM exits.
        cls._temporary_directory = Path(tempfile.mkdtemp(prefix="umbra-mock-java-rti-"))
        jar_path = build_mock_java_rti(cls._temporary_directory)
        cls.factory = JavaRtiFactory(
            JavaProviderConfiguration(
                classpath=(str(jar_path),),
                rti_factory_name="Umbra Mock Java RTI",
            )
        )

    def test_real_jvm_service_loader_and_java_to_python_callback(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()

        result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        self.assertEqual(self.factory.rtiName(), "Umbra Mock Java RTI")
        self.assertEqual(self.factory.rtiVersion(), "2025.mock")
        self.assertFalse(result.configurationUsed)
        self.assertEqual(result.message, "connected through the mock Java RTI using HLA_EVOKED")

        java_ambassador = ambassador.unwrap_java_object()
        java_ambassador.queueConnectionLost("callback from the JVM")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.connection_losses, ["callback from the JVM"])

        ambassador.listFederationExecutions()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(
            callbacks.federation_execution_reports,
            [
                FederationExecutionInformationSet(
                    [FederationExecutionInformation("Umbra Mock Federation", "HLAinteger64Time")]
                )
            ],
        )

        ambassador.createFederationExecution(
            "Python-created mock federation",
            "fixture-does-not-parse-fom.xml",
            "HLAinteger64Time",
        )
        ambassador.listFederationExecutions()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertIn(
            FederationExecutionInformation("Python-created mock federation", "HLAinteger64Time"),
            callbacks.federation_execution_reports[-1],
        )
        ambassador.destroyFederationExecution("Python-created mock federation")

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        ambassador.disconnect()
