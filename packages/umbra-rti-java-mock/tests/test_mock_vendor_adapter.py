from __future__ import annotations

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from hla.rti1516_2025 import RtiFactoryFactory
from hla.rti1516_2025.exceptions import RTIinternalError
from umbra._java.mock_rti1516_2025 import MockJavaRtiFactory
from umbra._java.rti1516_2025._runtime import JavaCallbackBinding


class _FakeJavaFactory:
    def rtiName(self) -> str:
        return "Umbra Mock Java RTI"

    def rtiVersion(self) -> str:
        return "2025.mock"

    def getRtiAmbassador(self) -> object:
        return object()

    def getEncoderFactory(self) -> object:
        return object()


class _FakeRuntime:
    def __init__(self) -> None:
        self.configuration = None
        self.factory = _FakeJavaFactory()

    def get_rti_factory(self, configuration):
        self.configuration = configuration
        return self.factory

    def callback_model(self, callback_model):
        return callback_model

    def bind_federate_ambassador(self, federate_ambassador):
        return JavaCallbackBinding(proxy=federate_ambassador, target=federate_ambassador)

    def exception_name(self, error: BaseException) -> str | None:
        return None


class _EntryPoint:
    name = MockJavaRtiFactory.ENTRY_POINT_ALIAS

    @staticmethod
    def load():
        return MockJavaRtiFactory


class MockVendorAdapterTest(unittest.TestCase):
    def test_named_alias_selects_the_vendor_adapter_without_starting_a_jvm(self) -> None:
        with patch("hla.rti1516_2025.core.entry_points", return_value=[_EntryPoint()]):
            factory = RtiFactoryFactory.getRtiFactory(MockJavaRtiFactory.ENTRY_POINT_ALIAS)

        self.assertIsInstance(factory, MockJavaRtiFactory)

    def test_adapter_passes_one_known_jar_and_factory_name_to_the_java_transport(self) -> None:
        runtime = _FakeRuntime()
        with tempfile.NamedTemporaryFile(suffix=".jar") as jar:
            factory = MockJavaRtiFactory(Path(jar.name), runtime=runtime)

            self.assertEqual(factory.rtiName(), MockJavaRtiFactory.JAVA_FACTORY_NAME)

        self.assertEqual(runtime.configuration.classpath, (jar.name,))
        self.assertEqual(runtime.configuration.rti_factory_name, MockJavaRtiFactory.JAVA_FACTORY_NAME)

    def test_missing_or_unknown_jar_is_reported_before_jvm_startup(self) -> None:
        runtime = _FakeRuntime()
        with self.assertRaisesRegex(RTIinternalError, "UMBRA_MOCK_JAVA_RTI_JAR"):
            MockJavaRtiFactory(runtime=runtime).rtiName()
        self.assertIsNone(runtime.configuration)

        with self.assertRaisesRegex(RTIinternalError, "does not exist"):
            MockJavaRtiFactory("missing-vendor.jar", runtime=runtime).rtiName()
