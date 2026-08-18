import unittest
from pathlib import Path
from uuid import uuid4

from hla.rti1516_2025 import (
    CallbackModel,
    FederateAmbassador,
    FederationExecutionInformation,
    RtiFactoryFactory,
)
from hla.rti1516_2025.exceptions import AlreadyConnected, RTIinternalError
from hla.rti1516_2025.testing import (
    ConnectionFoundationConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    ConnectionOverloadConformanceMixin,
)
from umbra._native.rti1516_2025 import UmbraRtiFactory


class NativeProviderTest(unittest.TestCase):
    def test_factory_is_discoverable_from_the_standard_api_namespace(self) -> None:
        self.assertEqual(RtiFactoryFactory.getRtiFactory("Umbra").rtiName(), "Umbra")

    def test_unimplemented_encoder_factory_does_not_return_a_placeholder(self) -> None:
        with self.assertRaises(RTIinternalError):
            UmbraRtiFactory().getEncoderFactory()

    def test_native_connection_foundation(self) -> None:
        factory = UmbraRtiFactory()
        ambassador = factory.getRtiAmbassador()
        result = ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)

        self.assertEqual(factory.rtiName(), "Umbra")
        self.assertFalse(result.configurationUsed)
        ambassador.disconnect()

    def test_native_exception_is_translated_at_the_provider_edge(self) -> None:
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        callback = FederateAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_IMMEDIATE)

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callback, CallbackModel.HLA_IMMEDIATE)

        ambassador.disconnect()

    def test_native_federation_creation_populates_the_typed_listing_callback(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.reports = []

            def reportFederationExecutions(self, report) -> None:
                self.reports.append(report)

        fom_module = (
            Path(__file__).parents[3]
            / "third_party"
            / "ieee1516.2-2025"
            / "resources"
            / "examples"
            / "RestaurantFOMmodule-2025.xml"
        )
        federation_name = f"python-native-{uuid4()}"
        callback = RecordingFederateAmbassador()
        ambassador = UmbraRtiFactory().getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        created = False
        try:
            ambassador.createFederationExecution(
                federation_name,
                str(fom_module),
                "HLAinteger64Time",
            )
            created = True
            ambassador.listFederationExecutions()
            ambassador.evokeCallback(0.0)
            self.assertIn(
                FederationExecutionInformation(federation_name, "HLAinteger64Time"),
                callback.reports[-1],
            )
        finally:
            if created:
                ambassador.destroyFederationExecution(federation_name)
            ambassador.disconnect()


class NativeConnectionFoundationConformanceTest(
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    unittest.TestCase,
):
    def make_factory(self) -> UmbraRtiFactory:
        return UmbraRtiFactory()
