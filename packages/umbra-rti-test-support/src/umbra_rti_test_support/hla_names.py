"""Canonical HLA names used by the provider conformance fixtures.

The RTI resolves these names dynamically against a FOM, so keeping them as
immutable, named values makes misspellings visible during review and keeps the
Python, native, and JPype fixtures on the same vocabulary.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class HlaFomNames:
    OBJECT_ROOT: str = "HLAobjectRoot"
    INTERACTION_ROOT: str = "HLAinteractionRoot"

    CUSTOMER: str = "HLAobjectRoot.Customer"
    EMPLOYEE: str = "HLAobjectRoot.Employee"
    EMPLOYEE_SERVER: str = "HLAobjectRoot.Employee.Server"
    FOOD: str = "HLAobjectRoot.Food"
    FOOD_DRINK: str = "HLAobjectRoot.Food.Drink"
    FOOD_DRINK_SODA: str = "HLAobjectRoot.Food.Drink.Soda"
    FOOD_DRINK_SODA_LIGHT: str = "HLAobjectRoot.Food.Drink.Soda.Light"
    REGIONAL_THING: str = "HLAobjectRoot.RegionalThing"

    CUSTOMER_TRANSACTIONS: str = "HLAinteractionRoot.CustomerTransactions"
    CUSTOMER_SEATED: str = (
        "HLAinteractionRoot.CustomerTransactions.CustomerSeated"
    )
    FOOD_SERVED: str = "HLAinteractionRoot.CustomerTransactions.FoodServed"
    MAIN_COURSE_SERVED: str = (
        "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
    )
    SERVER_ACTION: str = "HLAinteractionRoot.ServerAction"
    TAKE_ORDER: str = "HLAinteractionRoot.ServerAction.TakeOrder"
    SODA_SERVED: str = "HLAinteractionRoot.Food.Drink.SodaServed"

    ATTRIBUTE_FIXTURE_BASE: str = "HLAobjectRoot.UmbraAttributeFixtureBase"
    ATTRIBUTE_FIXTURE_CHILD: str = (
        "HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild"
    )
    ATTRIBUTE_FIXTURE_CLASS: str = (
        "HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureClass"
    )
    DIMENSION_FIXTURE_OBJECT: str = "HLAobjectRoot.UmbraDimensionFixtureObject"
    DIRECTED_FIXTURE_OBJECT: str = "HLAobjectRoot.UmbraDirectedFixtureObject"
    PARAMETER_FIXTURE_BASE: str = (
        "HLAinteractionRoot.UmbraParameterFixtureBase"
    )
    PARAMETER_FIXTURE_CHILD: str = (
        "HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild"
    )
    REFERENCE_FIXTURE_CLASS: str = "HLAobjectRoot.UmbraReferenceFixtureClass"
    # Names used by the deterministic 2010 native reference provider.  These
    # are intentionally separate from the Restaurant/fixture FOM vocabulary.
    REFERENCE_2010_OBJECT_CLASS: str = "HLAobjectRoot.Reference"
    REFERENCE_2010_INTERACTION_CLASS: str = "HLAinteractionRoot.Reference"
    REFERENCE_2010_ATTRIBUTE: str = "Payload"
    REFERENCE_2010_PARAMETER: str = "Payload"
    TRANSPORTATION_FIXTURE_OBJECT: str = (
        "HLAobjectRoot.UmbraTransportationFixtureObject"
    )
    TRANSPORTATION_REGIONAL_OBJECT: str = (
        "HLAobjectRoot.UmbraTransportationRegionalObject"
    )
    TWO_DIMENSIONAL_REGION_OBJECT: str = (
        "HLAobjectRoot.UmbraTwoDimensionalRegionObject"
    )

    ATTRIBUTE_FIXTURE_INTERACTION: str = (
        "HLAinteractionRoot.UmbraAttributeFixtureInteraction"
    )
    DIMENSION_FIXTURE_INTERACTION: str = (
        "HLAinteractionRoot.UmbraDimensionFixtureInteraction"
    )
    DIRECTED_FIXTURE_INTERACTION: str = (
        "HLAinteractionRoot.UmbraDirectedFixtureInteraction"
    )
    TRANSPORTATION_FIXTURE_INTERACTION: str = (
        "HLAinteractionRoot.UmbraTransportationFixtureInteraction"
    )
    TRANSPORTATION_REGIONAL_INTERACTION: str = (
        "HLAinteractionRoot.UmbraTransportationRegionalInteraction"
    )
    PARAMETER_FIXTURE_CHILD_INTERACTION: str = (
        "HLAinteractionRoot.UmbraParameterFixtureBase."
        "UmbraParameterFixtureChild"
    )
    TWO_DIMENSIONAL_REGION_INTERACTION: str = (
        "HLAinteractionRoot.UmbraTwoDimensionalRegionInteraction"
    )

    MISSING_OBJECT_CLASS: str = "HLAobjectRoot.NoSuchClass"
    MISSING_OBJECT_CLASS_ALT: str = "HLAobjectRoot.DoesNotExist"
    MISSING_INTERACTION_CLASS: str = "HLAinteractionRoot.NoSuchInteraction"
    MISSING_ATTRIBUTE: str = "NoSuchAttribute"
    MISSING_PARAMETER: str = "NoSuchParameter"
    MISSING_DIMENSION: str = "NoSuchDimension"
    MISSING_DIMENSION_ALT: str = "MissingDimension"
    MISSING_TRANSPORTATION: str = "NoSuchTransportation"


@dataclass(frozen=True, slots=True)
class HlaMomNames:
    STANDARD_MIM: str = "HLAstandardMIM"
    STANDARD_MIM_FILE: str = "HLAstandardMIM-2025.xml"
    STANDARD_MIM_FILE_LEGACY: str = "HLAstandardMIM.xml"
    MIM_DESIGNATOR: str = "HLAMIMdesignator"
    SERVICE_REPORT_MIM_DESIGNATOR: str = "HLAMIMDesignator"

    FEDERATE_OBJECT_CLASS: str = "HLAobjectRoot.HLAmanager.HLAfederate"
    FEDERATION_OBJECT_CLASS: str = "HLAobjectRoot.HLAmanager.HLAfederation"

    REQUEST_OBJECT_INSTANCES_UPDATED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestObjectInstancesUpdated"
    )
    REQUEST_OBJECT_INSTANCES_THAT_CAN_BE_DELETED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestObjectInstancesThatCanBeDeleted"
    )
    REQUEST_OBJECT_INSTANCES_REFLECTED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestObjectInstancesReflected"
    )
    REQUEST_OBJECT_INSTANCE_INFORMATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestObjectInstanceInformation"
    )
    REQUEST_FOM_MODULE_DATA_FEDERATE: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestFOMmoduleData"
    )
    REQUEST_PUBLICATIONS: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestPublications"
    )
    REQUEST_SUBSCRIPTIONS: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestSubscriptions"
    )
    REQUEST_REFLECTIONS_RECEIVED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestReflectionsReceived"
    )
    REQUEST_UPDATES_SENT: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestUpdatesSent"
    )
    REQUEST_INTERACTIONS_SENT: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestInteractionsSent"
    )
    REQUEST_DIRECTED_INTERACTIONS_SENT: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestDirectedInteractionsSent"
    )
    REQUEST_INTERACTIONS_RECEIVED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestInteractionsReceived"
    )
    REQUEST_DIRECTED_INTERACTIONS_RECEIVED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
        "HLArequestDirectedInteractionsReceived"
    )
    REQUEST_SYNCHRONIZATION_POINTS: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
        "HLArequestSynchronizationPoints"
    )
    REQUEST_SYNCHRONIZATION_POINT_STATUS: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
        "HLArequestSynchronizationPointStatus"
    )
    REQUEST_FOM_MODULE_DATA_FEDERATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
        "HLArequestFOMmoduleData"
    )
    REQUEST_MIM_DATA: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
        "HLArequestMIMdata"
    )
    REQUEST_MIM_DATA_NAME: str = "HLArequestMIMdata"

    REPORT_SERVICE_INVOCATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportServiceInvocation"
    )
    REPORT_FEDERATE_LOST: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportFederateLost"
    )
    REPORT_EXCEPTION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException"
    )
    REPORT_MOM_EXCEPTION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportMOMexception"
    )
    REPORT_FOM_MODULE_DATA_FEDERATE: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportFOMmoduleData"
    )
    REPORT_OBJECT_INSTANCES_UPDATED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportObjectInstancesUpdated"
    )
    REPORT_OBJECT_INSTANCES_THAT_CAN_BE_DELETED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportObjectInstancesThatCanBeDeleted"
    )
    REPORT_OBJECT_INSTANCES_REFLECTED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportObjectInstancesReflected"
    )
    REPORT_OBJECT_INSTANCE_INFORMATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportObjectInstanceInformation"
    )
    REPORT_UPDATES_SENT: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent"
    )
    REPORT_INTERACTIONS_SENT: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportInteractionsSent"
    )
    REPORT_DIRECTED_INTERACTIONS_SENT: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportDirectedInteractionsSent"
    )
    REPORT_REFLECTIONS_RECEIVED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportReflectionsReceived"
    )
    REPORT_INTERACTIONS_RECEIVED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportInteractionsReceived"
    )
    REPORT_DIRECTED_INTERACTIONS_RECEIVED: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportDirectedInteractionsReceived"
    )
    REPORT_OBJECT_CLASS_PUBLICATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportObjectClassPublication"
    )
    REPORT_OBJECT_CLASS_SUBSCRIPTION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportObjectClassSubscription"
    )
    REPORT_INTERACTION_PUBLICATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportInteractionPublication"
    )
    REPORT_INTERACTION_SUBSCRIPTION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportInteractionSubscription"
    )
    REPORT_DIRECTED_INTERACTION_PUBLICATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportDirectedInteractionPublication"
    )
    REPORT_DIRECTED_INTERACTION_SUBSCRIPTION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
        "HLAreportDirectedInteractionSubscription"
    )
    REPORT_FOM_MODULE_DATA_FEDERATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
        "HLAreportFOMmoduleData"
    )
    REPORT_MIM_DATA: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata"
    )
    REPORT_SYNCHRONIZATION_POINTS: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
        "HLAreportSynchronizationPoints"
    )
    REPORT_SYNCHRONIZATION_POINT_STATUS: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
        "HLAreportSynchronizationPointStatus"
    )

    DIRECTED_INTERACTIONS_RECEIVED: str = "HLAdirectedInteractionsReceived"
    DIRECTED_INTERACTIONS_SENT: str = "HLAdirectedInteractionsSent"
    INTERACTIONS_RECEIVED: str = "HLAinteractionsReceived"
    INTERACTIONS_SENT: str = "HLAinteractionsSent"

    SET_TIMING: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming"
    )
    SET_SWITCHES_FEDERATE: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches"
    )
    SET_SWITCHES_FEDERATION: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches"
    )
    EXTENDED_SET_SWITCHES: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust."
        "HLAsetSwitches.UmbraExtendedSetSwitches"
    )
    MODIFY_ATTRIBUTE_STATE: str = (
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust."
        "HLAmodifyAttributeState"
    )

    FEDERATE: str = "HLAfederate"
    FEDERATION: str = "HLAfederation"
    FEDERATE_HANDLE: str = "HLAfederateHandle"
    FEDERATE_NAME: str = "HLAfederateName"
    FEDERATE_STATE: str = "HLAfederateState"
    FEDERATE_HOST: str = "HLAfederateHost"
    FEDERATE_TYPE: str = "HLAfederateType"
    FEDERATES_IN_FEDERATION: str = "HLAfederatesInFederation"
    FEDERATION_NAME: str = "HLAfederationName"
    FOM_MODULE_DATA: str = "HLAFOMmoduleData"
    FOM_MODULE_DESIGNATOR_LIST: str = "HLAFOMmoduleDesignatorList"
    FOM_MODULE_INDICATOR: str = "HLAFOMmoduleIndicator"
    MIM_DATA: str = "HLAMIMdata"
    ACTIVE: str = "HLAactive"
    ATTRIBUTE: str = "HLAattribute"
    ATTRIBUTE_LIST: str = "HLAattributeList"
    ATTRIBUTE_STATE: str = "HLAattributeState"
    ATTRIBUTE_RELEVANCE_ADVISORY: str = "HLAattributeRelevanceAdvisory"
    ATTRIBUTE_SCOPE_ADVISORY: str = "HLAattributeScopeAdvisory"
    AUTO_PROVIDE: str = "HLAautoProvide"
    AUTOMATIC_RESIGN_ACTION: str = "HLAautomaticResignAction"
    ASYNCHRONOUS_DELIVERY: str = "HLAasynchronousDelivery"
    ALLOW_RELAXED_DDM: str = "HLAallowRelaxedDDM"
    ADVISORIES_USE_KNOWN_CLASS: str = "HLAadvisoriesUseKnownClass"
    CONVEY_REGION_DESIGNATOR_SETS: str = "HLAconveyRegionDesignatorSets"
    DEFAULT_UPDATE_RATE: str = "HLAdefaultUpdateRate"
    DELAY_SUBSCRIPTION_EVALUATION: str = "HLAdelaySubscriptionEvaluation"
    EXCEPTION_REPORTING: str = "HLAexceptionReporting"
    INTERACTION_CLASS_LIST: str = "HLAinteractionClassList"
    INTERACTION_COUNTS: str = "HLAinteractionCounts"
    KNOWN_CLASS: str = "HLAknownClass"
    MAX_UPDATE_RATE: str = "HLAmaxUpdateRate"
    NUMBER_OF_CLASSES: str = "HLAnumberOfClasses"
    OBJECT_CLASS: str = "HLAobjectClass"
    OBJECT_CLASS_RELEVANCE_ADVISORY: str = "HLAobjectClassRelevanceAdvisory"
    OBJECT_INSTANCE: str = "HLAobjectInstance"
    OBJECT_INSTANCE_COUNTS: str = "HLAobjectInstanceCounts"
    OBJECT_INSTANCES_DELETED: str = "HLAobjectInstancesDeleted"
    OBJECT_INSTANCES_DISCOVERED: str = "HLAobjectInstancesDiscovered"
    OBJECT_INSTANCES_REFLECTED: str = "HLAobjectInstancesReflected"
    OBJECT_INSTANCES_REGISTERED: str = "HLAobjectInstancesRegistered"
    OBJECT_INSTANCES_REMOVED: str = "HLAobjectInstancesRemoved"
    OBJECT_INSTANCES_THAT_CAN_BE_DELETED: str = "HLAobjectInstancesThatCanBeDeleted"
    OBJECT_INSTANCES_UPDATED: str = "HLAobjectInstancesUpdated"
    OWNED_INSTANCE_ATTRIBUTE_LIST: str = "HLAownedInstanceAttributeList"
    PRIVILEGE_TO_DELETE_OBJECT: str = "HLAprivilegeToDeleteObject"
    REFLECT_COUNTS: str = "HLAreflectCounts"
    REFLECTIONS_RECEIVED: str = "HLAreflectionsReceived"
    REGISTERED_CLASS: str = "HLAregisteredClass"
    REPORT_PERIOD: str = "HLAreportPeriod"
    REPORT_SERVICE_FILE: str = "HLAreportServiceFile"
    RESIGN_ACTION: str = "HLAresignAction"
    SERVICE: str = "HLAservice"
    SERVICE_REPORTING: str = "HLAserviceReporting"
    SEND_SERVICE_REPORTS_TO_FILE: str = "HLAsendServiceReportsToFile"
    SYNC_POINT_FEDERATES: str = "HLAsyncPointFederates"
    SYNC_POINT_NAME: str = "HLAsyncPointName"
    SYNC_POINTS: str = "HLAsyncPoints"
    TRANSPORTATION: str = "HLAtransportation"
    UPDATE_COUNTS: str = "HLAupdateCounts"
    UPDATES_SENT: str = "HLAupdatesSent"
    CURRENT_FDD: str = "HLAcurrentFDD"
    TIME_ADVANCING_TIME: str = "HLAtimeAdvancingTime"
    TIME_GRANTED_TIME: str = "HLAtimeGrantedTime"
    TIME_STAMP: str = "HLAtimeStamp"
    LOGICAL_TIME: str = "HLAlogicalTime"
    LOOKAHEAD: str = "HLAlookahead"
    GALT: str = "HLAGALT"
    LITS: str = "HLALITS"
    RO_LENGTH: str = "HLAROlength"
    TSO_LENGTH: str = "HLATSOlength"
    HLA_FEDERATE_DIMENSION: str = "HLAfederate"
    HLA_SERVICE_GROUP_DIMENSION: str = "HLAserviceGroup"
    RELIABLE: str = "HLAreliable"
    BEST_EFFORT: str = "HLAbestEffort"

    ARGUMENT_TYPE: str = "HLAargumentType"
    ARGUMENT_NAME: str = "HLAargumentName"
    ARGUMENT_VALUE: str = "HLAargumentValue"
    RETURNED_ARGUMENT: str = "HLAreturnedArgument"
    SERIAL_NUMBER: str = "HLAserialNumber"
    SUPPLIED_ARGUMENTS: str = "HLAsuppliedArguments"
    SUCCESS_INDICATOR: str = "HLAsuccessIndicator"
    EXCEPTION: str = "HLAexception"
    FAULT_DESCRIPTION: str = "HLAfaultDescription"
    RTI_VERSION: str = "HLARTIversion"
    SERVICE_TYPE: str = "HLAserviceType"
    PARAMETER_ERROR: str = "HLAparameterError"
    INTERACTION_RELEVANCE_ADVISORY: str = "HLAinteractionRelevanceAdvisory"
    LAST_SAVE_NAME: str = "HLAlastSaveName"
    LAST_SAVE_TIME: str = "HLAlastSaveTime"
    NEXT_SAVE_NAME: str = "HLAnextSaveName"
    NEXT_SAVE_TIME: str = "HLAnextSaveTime"
    NON_REGULATED_GRANT: str = "HLAnonRegulatedGrant"
    TIME_CONSTRAINED: str = "HLAtimeConstrained"
    TIME_IMPLEMENTATION_NAME: str = "HLAtimeImplementationName"
    TIME_MANAGER_STATE: str = "HLAtimeManagerState"
    TIME_REGULATING: str = "HLAtimeRegulating"


@dataclass(frozen=True, slots=True)
class HlaFixtureNames:
    ATTRIBUTE_FIXTURE_ATTRIBUTE: str = "UmbraAttributeFixtureAttribute"
    BEST_EFFORT_BASE: str = "BestEffortBase"
    BAR_QUANTITY: str = "BarQuantity"
    COLOR: str = "Color"
    DIRECTED_TARGET_MARKER: str = "DirectedTargetMarker"
    EFFICIENCY: str = "Efficiency"
    FLAVOR: str = "Flavor"
    IDENTIFIER: str = "Identifier"
    NAME: str = "Name"
    ORGANIC: str = "Organic"
    PAY_RATE: str = "PayRate"
    RELIABLE_BASE_A: str = "ReliableBaseA"
    RELIABLE_CHILD: str = "ReliableChild"
    SERVER_ID: str = "ServerId"
    SODA_FLAVOR: str = "SodaFlavor"
    SWEETENER: str = "Sweetener"
    TEMPERATURE_OK: str = "TemperatureOk"
    TRANSPORTATION_FIXTURE_ATTRIBUTE: str = "UmbraTransportationFixtureAttribute"
    UMBRA_TRANSPORTATION_FIXTURE: str = "UmbraTransportationFixture"
    UMBRA_DIMENSION_FIXTURE: str = "UmbraDimensionFixture"
    UMBRA_EXTENSION_SWITCH_PAYLOAD: str = "UmbraExtensionSwitchPayload"
    UMBRA_EXTENDED_SWITCH_PAYLOAD: str = "UmbraExtendedSwitchPayload"
    UMBRA_REGION_X: str = "UmbraRegionX"
    UMBRA_REGION_Y: str = "UmbraRegionY"
    UMBRA_REGIONAL_DIMENSION: str = "UmbraRegionalDimension"
    UNOWNED_CHILD: str = "UnownedChild"
    VALUE: str = "Value"
    INVALID_OBJECT_INSTANCE_NAME: str = "HLA.Invalid"
    RESERVED_OBJECT_INSTANCE_NAME: str = "HLA.ReservedByTheRTI"


@dataclass(frozen=True, slots=True)
class HlaTypeNames:
    ASCII_CHAR: str = "HLAASCIIchar"
    ASCII_STRING: str = "HLAASCIIstring"
    AUTHORIZER: str = "HLAauthorizer"
    BOOLEAN: str = "HLAboolean"
    BYTE: str = "HLAbyte"
    FLOAT32_BE: str = "HLAfloat32BE"
    FLOAT32_LE: str = "HLAfloat32LE"
    FLOAT64_BE: str = "HLAfloat64BE"
    FLOAT64_LE: str = "HLAfloat64LE"
    INTEGER16_BE: str = "HLAinteger16BE"
    INTEGER16_LE: str = "HLAinteger16LE"
    INTEGER32_BE: str = "HLAinteger32BE"
    INTEGER32_LE: str = "HLAinteger32LE"
    INTEGER64_BE: str = "HLAinteger64BE"
    INTEGER64_LE: str = "HLAinteger64LE"
    INTEGER64_TIME: str = "HLAinteger64Time"
    FLOAT64_TIME: str = "HLAfloat64Time"
    NO_CREDENTIALS: str = "HLAnoCredentials"
    OCTET: str = "HLAoctet"
    OCTET_PAIR_BE: str = "HLAoctetPairBE"
    OCTET_PAIR_LE: str = "HLAoctetPairLE"
    OPAQUE_DATA: str = "HLAopaqueData"
    PLAIN_TEXT_PASSWORD: str = "HLAplainTextPassword"
    UNICODE_CHAR: str = "HLAunicodeChar"
    UNICODE_STRING: str = "HLAunicodeString"
    UNSIGNED_INTEGER16_BE: str = "HLAunsignedInteger16BE"
    UNSIGNED_INTEGER16_LE: str = "HLAunsignedInteger16LE"
    UNSIGNED_INTEGER32_BE: str = "HLAunsignedInteger32BE"
    UNSIGNED_INTEGER32_LE: str = "HLAunsignedInteger32LE"
    UNSIGNED_INTEGER64_BE: str = "HLAunsignedInteger64BE"
    UNSIGNED_INTEGER64_LE: str = "HLAunsignedInteger64LE"


HLA_FOM = HlaFomNames()
HLA_MOM = HlaMomNames()
HLA_FIXTURES = HlaFixtureNames()
HLA_TYPES = HlaTypeNames()


__all__ = [
    "HLA_FIXTURES",
    "HLA_FOM",
    "HLA_MOM",
    "HLA_TYPES",
    "HlaFixtureNames",
    "HlaFomNames",
    "HlaMomNames",
    "HlaTypeNames",
]
