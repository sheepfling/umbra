from __future__ import annotations

import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from hla.rti1516_2025 import (
    CallbackModel,
    FederateAmbassador,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
)
from hla.rti1516_2025.exceptions import AlreadyConnected
from hla.rti1516_2025.testing import (
    ConnectionFoundationConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    ConnectionOverloadConformanceMixin,
)
from umbra._java.mock_rti1516_2025 import MockJavaRtiFactory


FIXTURE_ROOT = (
    Path(__file__).parents[2]
    / "umbra-rti-jpype"
    / "test-fixtures"
    / "mock-java-rti"
)
JPYPE_AVAILABLE = importlib.util.find_spec("jpype") is not None


def _java_toolchain_available() -> bool:
    return all(shutil.which(tool) is not None for tool in ("java", "javac", "jar"))


def _build_fixture(output_directory: Path) -> Path:
    classes = output_directory / "classes"
    classes.mkdir(parents=True, exist_ok=True)
    sources = sorted(str(path) for path in (FIXTURE_ROOT / "src" / "main" / "java").rglob("*.java"))
    subprocess.run(["javac", "-d", str(classes), *sources], check=True)
    shutil.copytree(FIXTURE_ROOT / "src" / "main" / "resources", classes, dirs_exist_ok=True)
    jar_path = output_directory / "umbra-mock-java-rti.jar"
    subprocess.run(["jar", "--create", "--file", str(jar_path), "-C", str(classes), "."], check=True)
    return jar_path


class _RecordingFederateAmbassador(FederateAmbassador):
    def __init__(self) -> None:
        self.connection_losses: list[str] = []
        self.federation_execution_reports: list[FederationExecutionInformationSet] = []

    def connectionLost(self, faultDescription: str) -> None:
        self.connection_losses.append(faultDescription)

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        self.federation_execution_reports.append(report)


@unittest.skipUnless(JPYPE_AVAILABLE and _java_toolchain_available(), "requires JPype and a JDK")
class MockVendorJPypeIntegrationTest(
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    unittest.TestCase,
):
    @classmethod
    def setUpClass(cls) -> None:
        import jpype

        if jpype.isJVMStarted():
            raise unittest.SkipTest("the fixture classpath must be present before the JVM starts")
        cls._temporary_directory = Path(tempfile.mkdtemp(prefix="umbra-mock-vendor-rti-"))
        cls._jar_path = _build_fixture(cls._temporary_directory)

    def make_factory(self) -> MockJavaRtiFactory:
        return MockJavaRtiFactory(self._jar_path)

    def test_vendor_adapter_uses_the_mock_jar_and_real_java_factory_name(self) -> None:
        factory = MockJavaRtiFactory(self._jar_path)
        ambassador = factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()

        result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        self.assertEqual(factory.rtiName(), MockJavaRtiFactory.JAVA_FACTORY_NAME)
        self.assertEqual(result.message, "connected through the mock Java RTI using HLA_EVOKED")

        java_ambassador = ambassador.unwrap_java_object()
        java_ambassador.queueConnectionLost("vendor-adapter callback")
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertEqual(callbacks.connection_losses, ["vendor-adapter callback"])

        ambassador.createFederationExecution(
            "Vendor adapter federation",
            "fixture-does-not-parse-fom.xml",
            "HLAinteger64Time",
        )
        ambassador.listFederationExecutions()
        self.assertTrue(ambassador.evokeCallback(0.0))
        self.assertIn(
            FederationExecutionInformation("Vendor adapter federation", "HLAinteger64Time"),
            callbacks.federation_execution_reports[-1],
        )
        ambassador.destroyFederationExecution("Vendor adapter federation")

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        ambassador.disconnect()
