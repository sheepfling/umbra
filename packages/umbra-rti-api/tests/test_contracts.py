import unittest
from unittest.mock import patch

import hla.rti1516_2025.exceptions as exceptions
from hla.rti1516_2025 import (
    AdditionalSettingsResultCode,
    CallbackModel,
    ConfigurationResult,
    RtiFactory,
    RtiFactoryFactory,
    RtiConfiguration,
)
from hla.rti1516_2025.auth import Credentials, HLAnoCredentials
from hla.rti1516_2025.exceptions import RTIinternalError, UnsupportedCallbackModel
from hla.rti1516_2025.core import _require_callback_model
from hla.rti1516_2025.exceptions import (
    ConnectionFailed,
    FederateInternalError,
    RTIexception,
    exceptionForName,
)


class _AliasFactory(RtiFactory):
    def getRtiAmbassador(self):
        raise NotImplementedError

    def getEncoderFactory(self):
        raise NotImplementedError

    def rtiName(self) -> str:
        return "A dynamically discovered vendor name"

    def rtiVersion(self) -> str:
        return "test"


class _EntryPoint:
    name = "java"

    @staticmethod
    def load():
        return _AliasFactory


class ContractsTest(unittest.TestCase):
    def test_foundation_values_are_stable_and_pythonic(self) -> None:
        result = ConfigurationResult(
            configurationUsed=False,
            addressUsed=False,
            additionalSettingsResultCode=AdditionalSettingsResultCode.SETTINGS_IGNORED,
        )

        self.assertEqual(CallbackModel.HLA_EVOKED.name, "HLA_EVOKED")
        self.assertFalse(result.configurationUsed)
        self.assertEqual(result.message, "")

    def test_entry_point_alias_can_select_a_lazy_transport(self) -> None:
        with patch("hla.rti1516_2025.core.entry_points", return_value=[_EntryPoint()]):
            factory = RtiFactoryFactory.getRtiFactory("java")

        self.assertEqual(factory.rtiName(), "A dynamically discovered vendor name")

    def test_duplicate_entry_point_aliases_are_rejected(self) -> None:
        with patch(
            "hla.rti1516_2025.core.entry_points",
            return_value=[_EntryPoint(), _EntryPoint()],
        ):
            with self.assertRaises(RTIinternalError):
                RtiFactoryFactory.getRtiFactory("java")

    def test_invalid_callback_models_have_a_standard_provider_error(self) -> None:
        self.assertEqual(_require_callback_model(CallbackModel.HLA_IMMEDIATE), CallbackModel.HLA_IMMEDIATE)
        with self.assertRaises(UnsupportedCallbackModel):
            _require_callback_model("HLA_IMMEDIATE")

    def test_connection_value_objects_keep_the_java_builder_and_data_shape(self) -> None:
        configuration = (
            RtiConfiguration.createConfiguration()
            .withConfigurationName("test")
            .withRtiAddress("rti.example")
            .withAdditionalSettings("mode=test")
        )
        credentials = Credentials("token", bytearray(b"secret"))

        self.assertEqual(configuration.configurationName(), "test")
        self.assertEqual(configuration.rtiAddress(), "rti.example")
        self.assertEqual(configuration.additionalSettings(), "mode=test")
        self.assertEqual(credentials.getType(), "token")
        self.assertEqual(credentials.getData(), b"secret")
        self.assertEqual(HLAnoCredentials().getData(), b"")

    def test_all_provider_edges_map_official_exception_names_to_specific_types(self) -> None:
        self.assertEqual(len(exceptions._EXCEPTION_TYPES), 100)
        self.assertIsInstance(exceptionForName("ConnectionFailed", "offline"), ConnectionFailed)
        self.assertIsInstance(
            exceptionForName("FederateInternalError", "callback failed"),
            FederateInternalError,
        )
        self.assertIsInstance(exceptionForName("vendor-only-error", "detail"), RTIexception)
