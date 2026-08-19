"""Provider-neutral test support for the implemented Python API slice.

This module is not an IEEE API type. It is a small reusable conformance suite
that provider packages can mix into their own ``unittest.TestCase`` classes.
"""

from __future__ import annotations

from abc import abstractmethod

from .core import (
    AdditionalSettingsResultCode,
    CallbackModel,
    ConfigurationResult,
    FederateAmbassador,
    FederationExecutionInformationSet,
    FederationExecutionMemberInformationSet,
    RtiFactory,
    RtiConfiguration,
)
from .auth import HLAnoCredentials
from .exceptions import AlreadyConnected, NotConnected, UnsupportedCallbackModel


class ConnectionFoundationConformanceMixin:
    """Mirror the supported state-machine assertions in the C++ connection tests.

    Combine this mixin with ``unittest.TestCase`` and implement
    :meth:`make_factory`. Each test gets its own factory/ambassador, so a
    provider must return independently usable ambassador instances.
    """

    @abstractmethod
    def make_factory(self) -> RtiFactory:
        """Return the configured provider factory under test."""

    def make_federate_ambassador(self) -> FederateAmbassador:
        return FederateAmbassador()

    def assert_valid_configuration_result(self, result: ConfigurationResult) -> None:
        self.assertIsInstance(result, ConfigurationResult)
        self.assertIsInstance(result.configurationUsed, bool)
        self.assertIsInstance(result.addressUsed, bool)
        self.assertIn(
            result.additionalSettingsResultCode,
            tuple(AdditionalSettingsResultCode),
        )
        self.assertIsInstance(result.message, str)

    def test_factory_reports_nonempty_identity_and_version(self) -> None:
        factory = self.make_factory()

        self.assertIsInstance(factory.rtiName(), str)
        self.assertTrue(factory.rtiName())
        self.assertIsInstance(factory.rtiVersion(), str)
        self.assertTrue(factory.rtiVersion())

    def test_each_callback_model_connects_and_returns_a_standard_result(self) -> None:
        for callback_model in CallbackModel:
            with self.subTest(callback_model=callback_model):
                ambassador = self.make_factory().getRtiAmbassador()
                result = ambassador.connect(self.make_federate_ambassador(), callback_model)

                self.assert_valid_configuration_result(result)
                ambassador.disconnect()

    def test_duplicate_connect_is_translated_to_already_connected(self) -> None:
        ambassador = self.make_factory().getRtiAmbassador()
        callback = self.make_federate_ambassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callback, CallbackModel.HLA_EVOKED)

        ambassador.disconnect()

    def test_disconnect_without_a_connection_is_translated_to_not_connected(self) -> None:
        ambassador = self.make_factory().getRtiAmbassador()

        with self.assertRaises(NotConnected):
            ambassador.disconnect()

    def test_invalid_callback_model_is_rejected_without_connecting(self) -> None:
        ambassador = self.make_factory().getRtiAmbassador()
        callback = self.make_federate_ambassador()

        with self.assertRaises(UnsupportedCallbackModel):
            ambassador.connect(callback, object())

        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        ambassador.disconnect()

    def test_callback_controls_are_safe_with_an_empty_queue(self) -> None:
        for callback_model in CallbackModel:
            with self.subTest(callback_model=callback_model):
                ambassador = self.make_factory().getRtiAmbassador()
                ambassador.connect(self.make_federate_ambassador(), callback_model)

                self.assertFalse(ambassador.evokeCallback(0.0))
                self.assertFalse(ambassador.evokeMultipleCallbacks(0.0, 0.0))
                ambassador.disableCallbacks()
                self.assertFalse(ambassador.evokeCallback(0.0))
                ambassador.enableCallbacks()
                self.assertFalse(ambassador.evokeMultipleCallbacks(0.0, 0.0))
                ambassador.disconnect()

    def test_disconnect_allows_a_fresh_connection(self) -> None:
        ambassador = self.make_factory().getRtiAmbassador()
        callback = self.make_federate_ambassador()

        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        ambassador.disconnect()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        ambassador.disconnect()


class ConnectionOverloadConformanceMixin:
    """Verify the four Java ``connect`` overloads exposed by Python.

    The overload where credentials is the third Java argument is represented
    naturally in Python as the third positional argument; the shared adapter
    disambiguates it from :class:`RtiConfiguration` by type.
    """

    @abstractmethod
    def make_factory(self) -> RtiFactory:
        """Return the configured provider factory under test."""

    def make_federate_ambassador(self) -> FederateAmbassador:
        return FederateAmbassador()

    def assert_valid_configuration_result(self, result: ConfigurationResult) -> None:
        self.assertIsInstance(result, ConfigurationResult)
        self.assertIsInstance(result.configurationUsed, bool)
        self.assertIsInstance(result.addressUsed, bool)
        self.assertIn(result.additionalSettingsResultCode, tuple(AdditionalSettingsResultCode))

    def test_each_standard_connect_overload_returns_a_standard_result(self) -> None:
        configuration = (
            RtiConfiguration.createConfiguration()
            .withConfigurationName("python-conformance")
            .withRtiAddress("localhost")
            # This contract checks the standard overload shape, not a
            # provider-specific additional-settings grammar.
            .withAdditionalSettings("")
        )
        credentials = HLAnoCredentials()
        overload_arguments = (
            ("base", ()),
            ("configuration", (configuration,)),
            ("credentials", (credentials,)),
            ("configuration-and-credentials", (configuration, credentials)),
        )
        for name, arguments in overload_arguments:
            with self.subTest(overload=name):
                ambassador = self.make_factory().getRtiAmbassador()
                result = ambassador.connect(
                    self.make_federate_ambassador(),
                    CallbackModel.HLA_EVOKED,
                    *arguments,
                )
                self.assert_valid_configuration_result(result)
                ambassador.disconnect()


class FederationExecutionDiscoveryConformanceMixin:
    """Verify the list/report federation-execution callback service pair."""

    @abstractmethod
    def make_factory(self) -> RtiFactory:
        """Return the configured provider factory under test."""

    def test_list_federation_executions_requires_a_connection(self) -> None:
        with self.assertRaises(NotConnected):
            self.make_factory().getRtiAmbassador().listFederationExecutions()

    def test_list_federation_executions_dispatches_a_typed_callback(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.reports: list[FederationExecutionInformationSet] = []

            def reportFederationExecutions(
                self,
                report: FederationExecutionInformationSet,
            ) -> None:
                self.reports.append(report)

        for callback_model in CallbackModel:
            with self.subTest(callback_model=callback_model):
                callback = RecordingFederateAmbassador()
                ambassador = self.make_factory().getRtiAmbassador()
                ambassador.connect(callback, callback_model)
                ambassador.listFederationExecutions()
                if callback_model is CallbackModel.HLA_EVOKED:
                    ambassador.evokeMultipleCallbacks(0.0, 0.1)
                self.assertEqual(len(callback.reports), 1)
                self.assertIsInstance(callback.reports[0], FederationExecutionInformationSet)
                ambassador.disconnect()


class FederationExecutionMemberDiscoveryConformanceMixin:
    """Verify the missing-federation half of the member-reporting service."""

    @abstractmethod
    def make_factory(self) -> RtiFactory:
        """Return the configured provider factory under test."""

    def test_list_federation_execution_members_requires_a_connection(self) -> None:
        with self.assertRaises(NotConnected):
            self.make_factory().getRtiAmbassador().listFederationExecutionMembers("missing")

    def test_missing_federation_execution_is_reported_through_the_callback_model(self) -> None:
        class RecordingFederateAmbassador(FederateAmbassador):
            def __init__(self) -> None:
                self.member_reports: list[tuple[str, FederationExecutionMemberInformationSet]] = []
                self.missing_federations: list[str] = []

            def reportFederationExecutionMembers(
                self,
                federationExecutionName: str,
                report: FederationExecutionMemberInformationSet,
            ) -> None:
                self.member_reports.append((federationExecutionName, report))

            def reportFederationExecutionDoesNotExist(self, federationExecutionName: str) -> None:
                self.missing_federations.append(federationExecutionName)

        for callback_model in CallbackModel:
            with self.subTest(callback_model=callback_model):
                callback = RecordingFederateAmbassador()
                ambassador = self.make_factory().getRtiAmbassador()
                missing_name = "python-conformance-missing-federation"
                ambassador.connect(callback, callback_model)
                ambassador.listFederationExecutionMembers(missing_name)
                if callback_model is CallbackModel.HLA_EVOKED:
                    ambassador.evokeMultipleCallbacks(0.0, 0.1)
                self.assertEqual(callback.member_reports, [])
                self.assertEqual(callback.missing_federations, [missing_name])
                ambassador.disconnect()
