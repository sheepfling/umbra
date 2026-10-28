"""Run the junior Python -> JPype -> Java -> JNI -> C++ capstone.

Run this from the repository root after building/staging the JNI product and
installing the API and JPype packages. See docs/development/FIRST-BUILD-WINDOWS-MINGW.md.
"""

from __future__ import annotations

import os
import sys
import time
from collections.abc import Callable
from pathlib import Path

from hla.rti1516_2025 import (
    AttributeHandleSet,
    AttributeHandleValueMap,
    CallbackModel,
    FederateAmbassador,
    InteractionClassHandle,
    LogicalTime,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ParameterHandleValueMap,
    ResignAction,
    RTIambassador,
    RtiConfiguration,
)
from hla.rti1516_2025.encoding import EncoderFactory
from hla.rti1516_2025.exceptions import RTIexception
from umbra._java.rti1516_2025 import UmbraJniRtiFactory

FEDERATION_NAME = "UmbraDeathStarCantina"
PUBLISHER_NAME = "DeathStarCommand"
OBSERVER_NAME = "RebelProbeDroid"
SERVER_CLASS_NAME = "HLAobjectRoot.Employee.Server"
SERVER_INSTANCE_NAME = "DeathStarCantinaServer"
SERVER_ATTRIBUTE_NAME = "Name"
SERVER_ATTRIBUTE_VALUE = "Grand Moff Tarkin"
SEATED_INTERACTION_NAME = "HLAinteractionRoot.CustomerTransactions.CustomerSeated"
MOM_FEDERATE_CLASS_NAME = "HLAobjectRoot.HLAmanager.HLAfederate"
MOM_FEDERATE_NAME_ATTRIBUTE = "HLAfederateName"
MOM_REPORT_FILE_ATTRIBUTE = "HLAreportServiceFile"


class ScenarioCallbacks(FederateAmbassador):
    """Keep the callbacks the exercise needs so the main flow stays readable."""

    def __init__(self) -> None:
        self.discovered_objects: list[
            tuple[ObjectInstanceHandle, ObjectClassHandle, str, object]
        ] = []
        self.reflections: list[
            tuple[ObjectInstanceHandle, AttributeHandleValueMap, object]
        ] = []
        self.received_interactions: list[
            tuple[InteractionClassHandle, LogicalTime | None]
        ] = []
        self.reserved_names: list[str] = []
        self.failed_names: list[str] = []
        self.time_regulation_enabled: list[LogicalTime] = []
        self.time_constrained_enabled: list[LogicalTime] = []
        self.time_advance_grants: list[LogicalTime] = []

    def discoverObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        objectClass: ObjectClassHandle,
        objectInstanceName: str,
        producingFederate: object,
    ) -> None:
        self.discovered_objects.append(
            (objectInstance, objectClass, objectInstanceName, producingFederate)
        )

    def reflectAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: object,
        producingFederate: object,
        sentRegions: object | None = None,
        time: LogicalTime | None = None,
        sentOrderType: object | None = None,
        receivedOrderType: object | None = None,
        retraction: object | None = None,
    ) -> None:
        self.reflections.append((objectInstance, attributeValues, time))

    def receiveInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: bytes,
        transportationType: object,
        producingFederate: object,
        sentRegions: object | None = None,
        time: LogicalTime | None = None,
        sentOrderType: object | None = None,
        receivedOrderType: object | None = None,
        retraction: object | None = None,
    ) -> None:
        self.received_interactions.append((interactionClass, time))

    def objectInstanceNameReservationSucceeded(self, objectInstanceName: str) -> None:
        self.reserved_names.append(objectInstanceName)

    def objectInstanceNameReservationFailed(self, objectInstanceName: str) -> None:
        self.failed_names.append(objectInstanceName)

    def timeRegulationEnabled(self, time: LogicalTime) -> None:
        self.time_regulation_enabled.append(time)

    def timeConstrainedEnabled(self, time: LogicalTime) -> None:
        self.time_constrained_enabled.append(time)

    def timeAdvanceGrant(self, time: LogicalTime) -> None:
        self.time_advance_grants.append(time)


def pump_callbacks(ambassadors: tuple[RTIambassador, ...]) -> None:
    """Deliver one queued HLA_EVOKED callback per connected ambassador."""
    for ambassador in ambassadors:
        ambassador.evokeCallback(0.0)


def wait_for(
    condition: Callable[[], bool],
    ambassadors: tuple[RTIambassador, ...],
    description: str,
    timeout_seconds: float = 10.0,
) -> None:
    """Avoid busy-waiting and give an unmet callback condition a clear error."""
    deadline = time.monotonic() + timeout_seconds
    while not condition() and time.monotonic() < deadline:
        pump_callbacks(ambassadors)
        time.sleep(0.005)
    if not condition():
        raise TimeoutError(f"Timed out waiting for {description}.")


def decode_unicode(encoder: EncoderFactory, value: bytes) -> str:
    """Decode an HLAunicodeString through the provider's Java encoder."""
    return encoder.createHLAunicodeString().decode(value).getValue()


def report_for_federate(
    callbacks: ScenarioCallbacks,
    mom_class: ObjectClassHandle,
    name_attribute: object,
    report_attribute: object,
    encoder: EncoderFactory,
) -> tuple[str, Path] | None:
    """Return the MOM identity and report path once both have been reflected."""
    mom_instances = {
        instance
        for instance, object_class, _, _ in callbacks.discovered_objects
        if object_class == mom_class
    }
    for instance, values, _ in callbacks.reflections:
        if instance not in mom_instances:
            continue
        if name_attribute not in values or report_attribute not in values:
            continue
        federate_name = decode_unicode(encoder, values[name_attribute])
        report_path = decode_unicode(encoder, values[report_attribute])
        if federate_name == PUBLISHER_NAME and report_path:
            return federate_name, Path(report_path)
    return None


def main() -> int:
    fom_value = os.environ.get("UMBRA_SCENARIO_FOM")
    reports_value = os.environ.get("UMBRA_MOM_REPORTS")
    if not fom_value or not reports_value:
        raise RuntimeError(
            "Set UMBRA_SCENARIO_FOM and UMBRA_MOM_REPORTS using the "
            "Final practice scenario setup in the junior build guide."
        )

    fom_file = Path(fom_value).resolve()
    reports_directory = Path(reports_value).resolve()
    if not fom_file.is_file():
        raise FileNotFoundError(f"Scenario FOM does not exist: {fom_file}")
    reports_directory.mkdir(parents=True, exist_ok=True)

    factory = UmbraJniRtiFactory()
    print(f"Provider: {factory.rtiName()} {factory.rtiVersion()}")
    publisher = factory.getRtiAmbassador()
    observer = factory.getRtiAmbassador()
    publisher_callbacks = ScenarioCallbacks()
    observer_callbacks = ScenarioCallbacks()
    ambassadors = (publisher, observer)

    publisher_connected = False
    observer_connected = False
    federation_created = False
    publisher_joined = False
    observer_joined = False

    try:
        for ambassador, callbacks, role in (
            (publisher, publisher_callbacks, "publisher"),
            (observer, observer_callbacks, "observer"),
        ):
            configuration = (
                RtiConfiguration.createConfiguration().withAdditionalSettings(
                    f"serviceReportDirectory={reports_directory}"
                )
            )
            ambassador.connect(
                callbacks, CallbackModel.HLA_EVOKED, configuration=configuration
            )
            if role == "publisher":
                publisher_connected = True
            else:
                observer_connected = True
        print("[PASS] Both Python ambassadors connected through Java/JNI.")

        publisher.createFederationExecution(
            FEDERATION_NAME, str(fom_file), "HLAinteger64Time"
        )
        federation_created = True
        publisher.joinFederationExecution(
            PUBLISHER_NAME, FEDERATION_NAME, federateName=PUBLISHER_NAME
        )
        publisher_joined = True
        observer.joinFederationExecution(
            OBSERVER_NAME, FEDERATION_NAME, federateName=OBSERVER_NAME
        )
        observer_joined = True
        print("[PASS] Both named federates joined the Restaurant FOM federation.")

        encoder = factory.getEncoderFactory()
        mom_class = observer.getObjectClassHandle(MOM_FEDERATE_CLASS_NAME)
        mom_name = observer.getAttributeHandle(mom_class, MOM_FEDERATE_NAME_ATTRIBUTE)
        mom_report = observer.getAttributeHandle(mom_class, MOM_REPORT_FILE_ATTRIBUTE)
        mom_attributes = AttributeHandleSet([mom_name, mom_report])
        observer.subscribeObjectClassAttributes(mom_class, mom_attributes)
        # Enable report output before the time services so the MOM probe can
        # verify those service calls in the publisher's report file.
        publisher.setServiceReportingSwitch(True)
        publisher.setSendServiceReportsToFileSwitch(True)

        server_class = publisher.getObjectClassHandle(SERVER_CLASS_NAME)
        observer_server_class = observer.getObjectClassHandle(SERVER_CLASS_NAME)
        server_attribute = publisher.getAttributeHandle(
            server_class, SERVER_ATTRIBUTE_NAME
        )
        observer_server_attribute = observer.getAttributeHandle(
            observer_server_class, SERVER_ATTRIBUTE_NAME
        )
        publisher_attributes = AttributeHandleSet([server_attribute])
        observer.subscribeObjectClassAttributes(
            observer_server_class, AttributeHandleSet([observer_server_attribute])
        )
        publisher.publishObjectClassAttributes(server_class, publisher_attributes)

        seated_interaction = publisher.getInteractionClassHandle(
            SEATED_INTERACTION_NAME
        )
        observer_seated_interaction = observer.getInteractionClassHandle(
            SEATED_INTERACTION_NAME
        )
        publisher.publishInteractionClass(seated_interaction)
        observer.subscribeInteractionClass(observer_seated_interaction)

        publisher.reserveObjectInstanceName(SERVER_INSTANCE_NAME)
        wait_for(
            lambda: (
                SERVER_INSTANCE_NAME in publisher_callbacks.reserved_names
                or SERVER_INSTANCE_NAME in publisher_callbacks.failed_names
            ),
            ambassadors,
            "the server object-name reservation callback",
        )
        if SERVER_INSTANCE_NAME in publisher_callbacks.failed_names:
            raise RuntimeError(
                f"Object name is already reserved: {SERVER_INSTANCE_NAME}"
            )
        server_instance = publisher.registerObjectInstance(
            server_class, objectInstanceName=SERVER_INSTANCE_NAME
        )
        wait_for(
            lambda: any(
                instance_name == SERVER_INSTANCE_NAME
                for _, _, instance_name, _ in observer_callbacks.discovered_objects
            ),
            ambassadors,
            "the observer to discover DeathStarCantinaServer",
        )
        print("[PASS] The observer discovered the named server object.")

        # The supplied FOM declares Employee.Name as timestamp ordered.
        observer.enableTimeConstrained()
        publisher.enableTimeRegulation(
            publisher.getTimeFactory().makeLogicalTimeInterval(1)
        )
        wait_for(
            lambda: (
                bool(observer_callbacks.time_constrained_enabled)
                and bool(publisher_callbacks.time_regulation_enabled)
            ),
            ambassadors,
            "time-constrained and time-regulation enabled callbacks",
        )

        name_value = bytes(
            encoder.createHLAunicodeString(SERVER_ATTRIBUTE_VALUE).toByteArray()
        )
        name_update_time = publisher.getTimeFactory().makeLogicalTime(1)
        publisher.updateAttributeValuesWithTime(
            server_instance,
            AttributeHandleValueMap({server_attribute: name_value}),
            name_update_time,
            b"cantina-server-name",
        )
        seated_time = publisher.getTimeFactory().makeLogicalTime(5)
        publisher.sendInteractionWithTime(
            seated_interaction,
            ParameterHandleValueMap(),
            seated_time,
            b"customer-seated",
        )
        observer.timeAdvanceRequest(observer.getTimeFactory().makeLogicalTime(5))
        publisher.timeAdvanceRequest(publisher.getTimeFactory().makeLogicalTime(5))
        wait_for(
            lambda: (
                bool(observer_callbacks.time_advance_grants)
                and bool(publisher_callbacks.time_advance_grants)
                and observer_callbacks.time_advance_grants[-1].getTime() == 5
                and publisher_callbacks.time_advance_grants[-1].getTime() == 5
                and any(
                    interaction == observer_seated_interaction
                    and timestamp is not None
                    and timestamp.getTime() == 5
                    for interaction, timestamp in observer_callbacks.received_interactions
                )
            ),
            ambassadors,
            "the time-5 CustomerSeated interaction and both time grants",
        )

        reflected_name = next(
            (
                decode_unicode(encoder, values[observer_server_attribute])
                for _, values, timestamp in observer_callbacks.reflections
                if observer_server_attribute in values
                and timestamp is not None
                and timestamp.getTime() == 1
            ),
            None,
        )
        if reflected_name != SERVER_ATTRIBUTE_VALUE:
            raise RuntimeError(
                f"Expected server Name {SERVER_ATTRIBUTE_VALUE!r} at time 1; "
                f"received {reflected_name!r}."
            )
        print("[PASS] The observer decoded Grand Moff Tarkin at logical time 1.")
        print("[PASS] The observer received CustomerSeated at logical time 5.")

        # MOM is ordinary object management: the RTI owns the instances and
        # the observer subscribed to them before either scenario federate ran.
        wait_for(
            lambda: any(
                object_class == mom_class
                for _, object_class, _, _ in observer_callbacks.discovered_objects
            ),
            ambassadors,
            "discovery of RTI-owned joined-federate MOM objects",
        )
        wait_for(
            lambda: (
                len(
                    {
                        object_instance
                        for (
                            object_instance,
                            object_class,
                            _,
                            _,
                        ) in observer_callbacks.discovered_objects
                        if object_class == mom_class
                    }
                )
                >= 2
            ),
            ambassadors,
            "MOM discovery for both joined federates",
        )
        for (
            object_instance,
            object_class,
            _,
            _,
        ) in observer_callbacks.discovered_objects:
            if object_class == mom_class:
                observer.requestAttributeValueUpdate(object_instance, mom_attributes)
        wait_for(
            lambda: (
                report_for_federate(
                    observer_callbacks, mom_class, mom_name, mom_report, encoder
                )
                is not None
            ),
            ambassadors,
            "DeathStarCommand's MOM name and service-report path",
        )
        mom_name_value, report_file = report_for_federate(
            observer_callbacks, mom_class, mom_name, mom_report, encoder
        ) or ("", Path())
        if mom_name_value != PUBLISHER_NAME or not report_file.is_file():
            raise RuntimeError(
                f"MOM returned an unusable service-report path: {report_file}"
            )
        report_contents = report_file.read_text(encoding="utf-8")
        if (
            FEDERATION_NAME not in report_contents
            or PUBLISHER_NAME not in report_contents
        ):
            raise RuntimeError(
                "The publisher MOM report does not contain the expected federation "
                "and federate names."
            )
        if "EnableTimeRegulation" not in report_contents:
            raise RuntimeError(
                "The publisher MOM report is missing the expected time-service record."
            )
        print(f"[PASS] MOM identified {mom_name_value} and its report file.")
        print(
            f"[PASS] Service-report file records the federation and time service: {report_file}"
        )
        print("All Python -> JPype -> Java -> JNI -> C++ checks passed.")
        return 0
    finally:
        for ambassador, joined, action, label in (
            (
                observer,
                observer_joined,
                ResignAction.NO_ACTION,
                "observer",
            ),
            (
                publisher,
                publisher_joined,
                ResignAction.DELETE_OBJECTS_THEN_DIVEST,
                "publisher",
            ),
        ):
            if joined:
                try:
                    ambassador.resignFederationExecution(action)
                except RTIexception as error:
                    print(f"[WARN] Could not resign {label}: {error}", file=sys.stderr)
        if federation_created:
            try:
                publisher.destroyFederationExecution(FEDERATION_NAME)
            except RTIexception as error:
                print(f"[WARN] Could not destroy federation: {error}", file=sys.stderr)
        for ambassador, connected, label in (
            (observer, observer_connected, "observer"),
            (publisher, publisher_connected, "publisher"),
        ):
            if connected:
                try:
                    ambassador.disconnect()
                except RTIexception as error:
                    print(
                        f"[WARN] Could not disconnect {label}: {error}", file=sys.stderr
                    )


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"[FAIL] {error}", file=sys.stderr)
        raise
