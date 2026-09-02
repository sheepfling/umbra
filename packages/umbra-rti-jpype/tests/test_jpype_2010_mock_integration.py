"""Opt-in 2010 Java-provider callback/state evidence through JPype.

The test deliberately uses the standard 1516e ``RtiFactory`` ServiceLoader
route.  The fixture is a small provider-neutral state machine, not an RTI
claim; its purpose is to prove that the Python callback and carrier adapters
remain usable with a real Java object graph under the 2010 package names.
"""

from __future__ import annotations

import importlib.util
import os
import tempfile
import unittest
from contextlib import suppress
from pathlib import Path

from _mock_java_2010_fixture import (
    FACTORY_NAME,
    build_mock_java_rti_2010,
    java_toolchain_available,
)
from hla.rti1516e import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    CallbackModel,
    FederateHandle,
    HLAfloat64Time,
    HLAinteger64Time,
    NullFederateAmbassador,
    ParameterHandleValueMap,
    ResignAction,
    RestoreFailureReason,
    SaveFailureReason,
)
from umbra._java.rti1516e import (
    Java2010ProviderConfiguration,
    Java2010RtiFactory,
)
from umbra_rti_test_support import (
    HLA_FIXTURES,
    HLA_FOM,
    SurfaceEventTrace,
    assert_event_order,
    iter_surface_matrix,
)

JPYPE_AVAILABLE = importlib.util.find_spec("jpype") is not None
ENABLED = os.environ.get("UMBRA_ENABLE_JPYPE_2010_MOCK_INTEGRATION") == "1"


class _MatrixCallback(NullFederateAmbassador):
    """Record the standard 2010 callback carriers without provider logic."""

    def __init__(self, trace: SurfaceEventTrace) -> None:
        self.trace = trace
        self.discoveries: list[tuple[object, ...]] = []
        self.reflections: list[tuple[object, ...]] = []
        self.interactions: list[tuple[object, ...]] = []
        self.save_initiations: list[str] = []
        self.timestamped_save_initiations: list[tuple[str, object]] = []
        self.save_completions = 0
        self.save_failures: list[SaveFailureReason] = []
        self.restore_accepted: list[str] = []
        self.restore_request_failures: list[str] = []
        self.restore_begun = 0
        self.restore_initiations: list[tuple[str, str, FederateHandle]] = []
        self.restore_completions = 0
        self.restore_failures: list[RestoreFailureReason] = []

    def discoverObjectInstance(self, *args: object) -> None:
        self.trace.record("discoverObjectInstance")
        self.discoveries.append(args)

    def reflectAttributeValues(self, *args: object) -> None:
        self.trace.record("reflectAttributeValues")
        self.reflections.append(args)

    def receiveInteraction(self, *args: object) -> None:
        self.trace.record("receiveInteraction")
        self.interactions.append(args)

    def timeAdvanceGrant(self, time: object) -> None:
        del time
        self.trace.record("timeAdvanceGrant")

    def initiateFederateSave(self, label: str, *args: object) -> None:
        self.trace.record("initiateFederateSave")
        if args:
            self.timestamped_save_initiations.append((label, args[0]))
        else:
            self.save_initiations.append(label)

    def federationSaved(self) -> None:
        self.trace.record("federationSaved")
        self.save_completions += 1

    def federationNotSaved(self, reason: SaveFailureReason) -> None:
        self.trace.record("federationNotSaved")
        self.save_failures.append(reason)

    def requestFederationRestoreSucceeded(self, label: str) -> None:
        self.trace.record("requestFederationRestoreSucceeded")
        self.restore_accepted.append(label)

    def requestFederationRestoreFailed(self, label: str) -> None:
        self.trace.record("requestFederationRestoreFailed")
        self.restore_request_failures.append(label)

    def federationRestoreBegun(self) -> None:
        self.trace.record("federationRestoreBegun")
        self.restore_begun += 1

    def initiateFederateRestore(
        self, label: str, federate_name: str, post_restore_handle: FederateHandle
    ) -> None:
        self.trace.record("initiateFederateRestore")
        self.restore_initiations.append((label, federate_name, post_restore_handle))

    def federationRestored(self) -> None:
        self.trace.record("federationRestored")
        self.restore_completions += 1

    def federationNotRestored(self, reason: RestoreFailureReason) -> None:
        self.trace.record("federationNotRestored")
        self.restore_failures.append(reason)


@unittest.skipUnless(
    JPYPE_AVAILABLE and java_toolchain_available() and ENABLED,
    "set UMBRA_ENABLE_JPYPE_2010_MOCK_INTEGRATION=1 with JPype and a JDK to run the 2010 JVM lane",
)
class Jpype2010MockIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        import jpype

        if jpype.isJVMStarted():
            raise unittest.SkipTest(
                "the 2010 fixture classpath must be installed before the JVM starts"
            )
        cls._temporary_directory = Path(
            tempfile.mkdtemp(prefix="umbra-mock-java-2010-")
        )
        try:
            api_jar, provider_jar = build_mock_java_rti_2010(cls._temporary_directory)
        except FileNotFoundError as error:
            raise unittest.SkipTest(str(error)) from error
        cls.factory = Java2010RtiFactory(
            Java2010ProviderConfiguration(
                classpath=(str(api_jar), str(provider_jar)),
                rti_factory_name=FACTORY_NAME,
            )
        )

    def test_standard_2010_multifederate_callback_matrix(self) -> None:
        """Prove Java→Python callback ordering for one and two subscribers."""

        cases = iter_surface_matrix(
            time_implementations=("HLAinteger64Time", "HLAfloat64Time"),
            save_kinds=("scalar",),
            member_counts=(2, 3),
        )
        for case in cases:
            with self.subTest(case=case.case_id):
                federation_name = (
                    f"python-2010-object-matrix-{case.case_id.replace(':', '-')}"
                )
                publisher = self.factory.getRtiAmbassador()
                publisher_callback = _MatrixCallback(SurfaceEventTrace())
                subscribers: list[object] = []
                callbacks: list[_MatrixCallback] = []
                attributes: list[AttributeHandle] = []
                created = False

                def drain(ambassador: object) -> None:
                    for _ in range(32):
                        if not ambassador.evokeCallback(0.0):  # type: ignore[attr-defined]
                            return
                    self.fail("2010 callback queue did not drain")

                try:
                    callback_model = CallbackModel[case.callback_model]
                    expected_time = (
                        5.25
                        if case.time_implementation == "HLAfloat64Time"
                        else 5
                    )
                    publisher.connect(publisher_callback, callback_model)
                    # The Java 1516e signature consumes ``java.net.URL``.
                    # Passing a Path makes the adapter exercise that
                    # conversion while the fixture intentionally ignores
                    # the module contents.  A third argument selects the
                    # standard logical-time factory for the float64 cases;
                    # omitting it preserves the API's integer default.
                    module_argument: object = (
                        [Path(__file__).resolve()]
                        if case.time_implementation == "HLAfloat64Time"
                        else Path(__file__).resolve()
                    )
                    create_args: list[object] = [federation_name, module_argument]
                    if case.time_implementation == "HLAfloat64Time":
                        create_args.append(case.time_implementation)
                    publisher.createFederationExecution(*create_args)
                    created = True
                    publisher.joinFederationExecution(
                        "matrix-2010-publisher", federation_name
                    )
                    publisher_class = publisher.getObjectClassHandle(
                        HLA_FOM.FOOD_DRINK_SODA
                    )
                    publisher_attribute = publisher.getAttributeHandle(
                        publisher_class, HLA_FIXTURES.FLAVOR
                    )
                    publisher_interaction = publisher.getInteractionClassHandle(
                        HLA_FOM.SODA_SERVED
                    )
                    publisher.publishObjectClassAttributes(
                        publisher_class, AttributeHandleSet([publisher_attribute])
                    )
                    publisher.publishInteractionClass(publisher_interaction)

                    time_factory = publisher.getTimeFactory()
                    self.assertEqual(
                        time_factory.getName(), case.time_implementation
                    )
                    target = time_factory.makeLogicalTime(expected_time)
                    getattr(publisher, case.advance_service)(target)
                    drain(publisher)
                    if case.time_implementation == "HLAfloat64Time":
                        self.assertIsInstance(publisher.queryLogicalTime(), HLAfloat64Time)
                        self.assertAlmostEqual(
                            publisher.queryLogicalTime().getTime(), expected_time
                        )
                    else:
                        self.assertIsInstance(publisher.queryLogicalTime(), HLAinteger64Time)
                        self.assertEqual(publisher.queryLogicalTime().getTime(), expected_time)

                    for member in range(case.member_count - 1):
                        subscriber = self.factory.getRtiAmbassador()
                        callback = _MatrixCallback(SurfaceEventTrace())
                        subscriber.connect(callback, callback_model)
                        subscriber.joinFederationExecution(
                            f"matrix-2010-subscriber-{member}", federation_name
                        )
                        subscriber_class = subscriber.getObjectClassHandle(
                            HLA_FOM.FOOD_DRINK_SODA
                        )
                        subscriber_attribute = subscriber.getAttributeHandle(
                            subscriber_class, HLA_FIXTURES.FLAVOR
                        )
                        subscriber_interaction = subscriber.getInteractionClassHandle(
                            HLA_FOM.SODA_SERVED
                        )
                        subscriber.subscribeObjectClassAttributes(
                            subscriber_class,
                            AttributeHandleSet([subscriber_attribute]),
                        )
                        subscriber.subscribeInteractionClass(subscriber_interaction)
                        getattr(subscriber, case.advance_service)(target)
                        drain(subscriber)
                        if case.time_implementation == "HLAfloat64Time":
                            self.assertIsInstance(subscriber.queryLogicalTime(), HLAfloat64Time)
                            self.assertAlmostEqual(
                                subscriber.queryLogicalTime().getTime(), expected_time
                            )
                        else:
                            self.assertIsInstance(subscriber.queryLogicalTime(), HLAinteger64Time)
                            self.assertEqual(subscriber.queryLogicalTime().getTime(), expected_time)
                        subscribers.append(subscriber)
                        callbacks.append(callback)
                        attributes.append(subscriber_attribute)

                    object_instance = publisher.registerObjectInstance(publisher_class)
                    for subscriber in subscribers:
                        drain(subscriber)
                    for callback in callbacks:
                        self.assertEqual(len(callback.discoveries), 1)
                        self.assertEqual(callback.discoveries[0][0], object_instance)

                    publisher.updateAttributeValues(
                        object_instance,
                        AttributeHandleValueMap({publisher_attribute: b"2010-value"}),
                        b"2010-update-tag",
                    )
                    for subscriber in subscribers:
                        drain(subscriber)
                    for callback, attribute in zip(callbacks, attributes, strict=True):
                        self.assertEqual(len(callback.reflections), 1)
                        self.assertEqual(
                            callback.reflections[0][1][attribute], b"2010-value"
                        )
                        self.assertEqual(callback.reflections[0][2], b"2010-update-tag")

                    publisher.sendInteraction(
                        publisher_interaction,
                        ParameterHandleValueMap(),
                        b"2010-interaction-tag",
                    )
                    for subscriber in subscribers:
                        drain(subscriber)
                    for callback in callbacks:
                        self.assertEqual(len(callback.interactions), 1)
                        self.assertEqual(
                            callback.interactions[0][2], b"2010-interaction-tag"
                        )
                        assert_event_order(
                            callback.trace.events,
                            "timeAdvanceGrant",
                            "discoverObjectInstance",
                            "reflectAttributeValues",
                            "receiveInteraction",
                        )

                    # Exercise the timestamped object/interaction callback
                    # overloads as a second state-space point.  The fixture
                    # intentionally delivers immediately; this assertion is
                    # about exact Java carrier shape and Python conversion,
                    # not a claim about timestamp scheduling.
                    reflection_value = (
                        6.25
                        if case.time_implementation == "HLAfloat64Time"
                        else 6
                    )
                    interaction_value = (
                        7.5
                        if case.time_implementation == "HLAfloat64Time"
                        else 7
                    )
                    reflection_time = time_factory.makeLogicalTime(reflection_value)
                    interaction_time = time_factory.makeLogicalTime(interaction_value)
                    publisher.updateAttributeValues(
                        object_instance,
                        AttributeHandleValueMap({publisher_attribute: b"2010-tso-value"}),
                        b"2010-tso-update-tag",
                        reflection_time,
                    )
                    for subscriber in subscribers:
                        drain(subscriber)
                    publisher.sendInteraction(
                        publisher_interaction,
                        ParameterHandleValueMap(),
                        b"2010-tso-interaction-tag",
                        interaction_time,
                    )
                    for subscriber in subscribers:
                        drain(subscriber)
                    for callback, attribute in zip(callbacks, attributes, strict=True):
                        self.assertEqual(len(callback.reflections), 2)
                        timed_reflection = callback.reflections[-1]
                        self.assertEqual(len(timed_reflection), 8)
                        self.assertEqual(
                            timed_reflection[1][attribute], b"2010-tso-value"
                        )
                        self.assertEqual(timed_reflection[2], b"2010-tso-update-tag")
                        expected_time_type = (
                            HLAfloat64Time
                            if case.time_implementation == "HLAfloat64Time"
                            else HLAinteger64Time
                        )
                        self.assertIsInstance(timed_reflection[5], expected_time_type)
                        if case.time_implementation == "HLAfloat64Time":
                            self.assertAlmostEqual(
                                timed_reflection[5].getTime(), reflection_value
                            )
                        else:
                            self.assertEqual(timed_reflection[5].getTime(), reflection_value)
                        self.assertEqual(len(callback.interactions), 2)
                        timed_interaction = callback.interactions[-1]
                        self.assertEqual(len(timed_interaction), 8)
                        self.assertEqual(
                            timed_interaction[2], b"2010-tso-interaction-tag"
                        )
                        self.assertIsInstance(timed_interaction[5], expected_time_type)
                        if case.time_implementation == "HLAfloat64Time":
                            self.assertAlmostEqual(
                                timed_interaction[5].getTime(), interaction_value
                            )
                        else:
                            self.assertEqual(timed_interaction[5].getTime(), interaction_value)
                finally:
                    for subscriber in subscribers:
                        with suppress(Exception):
                            subscriber.resignFederationExecution(ResignAction.NO_ACTION)
                        with suppress(Exception):
                            subscriber.disconnect()
                    with suppress(Exception):
                        publisher.resignFederationExecution(ResignAction.NO_ACTION)
                    if created:
                        cleanup = self.factory.getRtiAmbassador()
                        cleanup.connect(
                            NullFederateAmbassador(), CallbackModel.HLA_IMMEDIATE
                        )
                        try:
                            cleanup.destroyFederationExecution(federation_name)
                        finally:
                            cleanup.disconnect()
                    with suppress(Exception):
                        publisher.disconnect()

    def test_standard_2010_live_save_restore_outcome_matrix(self) -> None:
        """Exercise scalar save/restore outcomes through the 1516e proxy.

        The reusable source-provider matrix covers timestamped overloads and
        every state-space point.  This opt-in fixture lane exercises both
        standard save-request overloads.  The compact fixture delivers the
        timed callback immediately (it does not model a scheduled boundary),
        which is sufficient to prove that the LogicalTime carrier survives
        Java -> JPype -> Python while the save/restore callbacks retain their
        standard ordering and outcome shapes.
        """

        outcomes = ("complete", "not-complete", "abort")
        expected_save_failure = {
            "not-complete": SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE,
            "abort": SaveFailureReason.SAVE_ABORTED,
        }
        expected_restore_failure = {
            "not-complete": RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
            "abort": RestoreFailureReason.RESTORE_ABORTED,
        }
        cases = iter_surface_matrix(
            time_implementations=("HLAinteger64Time", "HLAfloat64Time"),
            save_kinds=("scalar", "timestamped"),
            member_counts=(1, 2),
        )
        for case in cases:
            for save_outcome in outcomes:
                with self.subTest(case=case.case_id, save_outcome=save_outcome):
                    federation_name = (
                        f"python-2010-save-outcome-{case.case_id.replace(':', '-')}-"
                        f"{save_outcome}"
                    )
                    ambassadors: list[object] = []
                    callbacks: list[_MatrixCallback] = []
                    created = False

                    def drain(ambassador: object) -> None:
                        for _ in range(32):
                            if not ambassador.evokeCallback(0.0):  # type: ignore[attr-defined]
                                return
                        self.fail("2010 callback queue did not drain")

                    try:
                        callback_model = CallbackModel[case.callback_model]
                        save_time_value = (
                            7.5
                            if case.time_implementation == "HLAfloat64Time"
                            else 7
                        )
                        for member in range(case.member_count):
                            ambassador = self.factory.getRtiAmbassador()
                            callback = _MatrixCallback(SurfaceEventTrace())
                            ambassador.connect(callback, callback_model)
                            if member == 0:
                                module_argument: object = (
                                    [Path(__file__).resolve()]
                                    if case.time_implementation == "HLAfloat64Time"
                                    else Path(__file__).resolve()
                                )
                                create_args: list[object] = [
                                    federation_name,
                                    module_argument,
                                ]
                                if case.time_implementation == "HLAfloat64Time":
                                    create_args.append(case.time_implementation)
                                ambassador.createFederationExecution(*create_args)
                                created = True
                            ambassador.joinFederationExecution(
                                f"save-2010-{member}", federation_name
                            )
                            ambassadors.append(ambassador)
                            callbacks.append(callback)

                        save_label = f"save-{case.save_kind}-{save_outcome}"
                        time_factory = ambassadors[0].getTimeFactory()
                        self.assertEqual(
                            time_factory.getName(), case.time_implementation
                        )
                        save_time = (
                            time_factory.makeLogicalTime(save_time_value)
                            if case.save_kind == "timestamped"
                            else None
                        )
                        if save_time is None:
                            ambassadors[0].requestFederationSave(save_label)
                        else:
                            ambassadors[0].requestFederationSave(save_label, save_time)
                        for ambassador in ambassadors:
                            drain(ambassador)
                        for ambassador in ambassadors:
                            ambassador.federateSaveBegun()
                        if save_outcome == "complete":
                            for ambassador in ambassadors:
                                ambassador.federateSaveComplete()
                        else:
                            getattr(
                                ambassadors[0],
                                "federateSaveNotComplete"
                                if save_outcome == "not-complete"
                                else "abortFederationSave",
                            )()
                        for ambassador in ambassadors:
                            drain(ambassador)

                        for callback in callbacks:
                            if case.save_kind == "timestamped":
                                self.assertEqual(callback.save_initiations, [])
                                self.assertEqual(
                                    len(callback.timestamped_save_initiations), 1
                                )
                                timed_label, timed_time = (
                                    callback.timestamped_save_initiations[0]
                                )
                                self.assertEqual(timed_label, save_label)
                                expected_time_type = (
                                    HLAfloat64Time
                                    if case.time_implementation == "HLAfloat64Time"
                                    else HLAinteger64Time
                                )
                                self.assertIsInstance(timed_time, expected_time_type)
                                if case.time_implementation == "HLAfloat64Time":
                                    self.assertAlmostEqual(
                                        timed_time.getTime(), save_time_value
                                    )
                                else:
                                    self.assertEqual(timed_time.getTime(), save_time_value)
                            else:
                                self.assertEqual(
                                    callback.save_initiations, [save_label]
                                )
                                self.assertEqual(
                                    callback.timestamped_save_initiations, []
                                )
                            if save_outcome == "complete":
                                self.assertEqual(callback.save_completions, 1)
                                self.assertEqual(callback.save_failures, [])
                            else:
                                self.assertEqual(callback.save_completions, 0)
                                self.assertEqual(
                                    callback.save_failures,
                                    [expected_save_failure[save_outcome]],
                                )
                        if save_outcome != "complete":
                            # A failed/aborted save must not leave a restore
                            # snapshot behind.  The standard failure callback
                            # is delivered to the requesting federate and
                            # carries the exact label through Java -> JPype ->
                            # Python.
                            before_failures = [
                                len(callback.restore_request_failures)
                                for callback in callbacks
                            ]
                            ambassadors[0].requestFederationRestore(save_label)
                            for ambassador in ambassadors:
                                drain(ambassador)
                            self.assertEqual(
                                len(callbacks[0].restore_request_failures),
                                before_failures[0] + 1,
                            )
                            self.assertEqual(
                                callbacks[0].restore_request_failures[-1], save_label
                            )
                            for callback, failures in zip(
                                callbacks[1:], before_failures[1:], strict=True
                            ):
                                self.assertEqual(
                                    len(callback.restore_request_failures), failures
                                )
                            continue

                        for restore_outcome in outcomes:
                            with self.subTest(restore_outcome=restore_outcome):
                                # The fixture stores one snapshot under the
                                # save label, so each outcome replays that
                                # same standard restore request.
                                restore_label = save_label
                                before_accepted = [
                                    len(callback.restore_accepted)
                                    for callback in callbacks
                                ]
                                before_begun = [
                                    callback.restore_begun for callback in callbacks
                                ]
                                before_initiated = [
                                    len(callback.restore_initiations)
                                    for callback in callbacks
                                ]
                                before_completed = [
                                    callback.restore_completions
                                    for callback in callbacks
                                ]
                                before_failed = [
                                    len(callback.restore_failures)
                                    for callback in callbacks
                                ]
                                ambassadors[0].requestFederationRestore(restore_label)
                                for ambassador in ambassadors:
                                    drain(ambassador)
                                for callback, accepted, begun, initiated in zip(
                                    callbacks,
                                    before_accepted,
                                    before_begun,
                                    before_initiated,
                                    strict=True,
                                ):
                                    self.assertEqual(
                                        len(callback.restore_accepted), accepted + 1
                                    )
                                    self.assertEqual(
                                        callback.restore_accepted[-1], restore_label
                                    )
                                    self.assertEqual(callback.restore_begun, begun + 1)
                                    self.assertEqual(
                                        len(callback.restore_initiations), initiated + 1
                                    )
                                if restore_outcome == "complete":
                                    for ambassador in ambassadors:
                                        ambassador.federateRestoreComplete()
                                else:
                                    getattr(
                                        ambassadors[0],
                                        "federateRestoreNotComplete"
                                        if restore_outcome == "not-complete"
                                        else "abortFederationRestore",
                                    )()
                                for ambassador in ambassadors:
                                    drain(ambassador)
                                for callback, completed, failed in zip(
                                    callbacks,
                                    before_completed,
                                    before_failed,
                                    strict=True,
                                ):
                                    if restore_outcome == "complete":
                                        self.assertEqual(
                                            callback.restore_completions, completed + 1
                                        )
                                        self.assertEqual(
                                            len(callback.restore_failures), failed
                                        )
                                    else:
                                        self.assertEqual(
                                            callback.restore_completions, completed
                                        )
                                        self.assertEqual(
                                            callback.restore_failures[failed:],
                                            [expected_restore_failure[restore_outcome]],
                                        )
                    finally:
                        for ambassador in ambassadors:
                            with suppress(Exception):
                                ambassador.resignFederationExecution(
                                    ResignAction.NO_ACTION
                                )
                            with suppress(Exception):
                                ambassador.disconnect()
                        if created:
                            cleanup = self.factory.getRtiAmbassador()
                            cleanup.connect(
                                NullFederateAmbassador(), CallbackModel.HLA_IMMEDIATE
                            )
                            try:
                                cleanup.destroyFederationExecution(federation_name)
                            finally:
                                cleanup.disconnect()


if __name__ == "__main__":
    unittest.main()
