"""Generated IEEE 1516.1-2010 provider-neutral Python contracts.

This file is generated from the authoritative Java API source archive.
It contains names and overload metadata only; it is not an RTI.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import ClassVar

STANDARD_EDITION = "IEEE 1516.1-2010"
JAVA_PACKAGE = "hla.rti1516e"
CPP_NAMESPACE = "rti1516e"

@dataclass(frozen=True, slots=True)
class MethodSpec:
    name: str
    overload_count: int

RTIAMBASSADOR_METHODS = ('connect', 'disconnect', 'createFederationExecution', 'destroyFederationExecution', 'listFederationExecutions', 'joinFederationExecution', 'resignFederationExecution', 'registerFederationSynchronizationPoint', 'synchronizationPointAchieved', 'requestFederationSave', 'federateSaveBegun', 'federateSaveComplete', 'federateSaveNotComplete', 'abortFederationSave', 'queryFederationSaveStatus', 'requestFederationRestore', 'federateRestoreComplete', 'federateRestoreNotComplete', 'abortFederationRestore', 'queryFederationRestoreStatus', 'publishObjectClassAttributes', 'unpublishObjectClass', 'unpublishObjectClassAttributes', 'publishInteractionClass', 'unpublishInteractionClass', 'subscribeObjectClassAttributes', 'subscribeObjectClassAttributesPassively', 'unsubscribeObjectClass', 'unsubscribeObjectClassAttributes', 'subscribeInteractionClass', 'subscribeInteractionClassPassively', 'unsubscribeInteractionClass', 'reserveObjectInstanceName', 'releaseObjectInstanceName', 'reserveMultipleObjectInstanceName', 'releaseMultipleObjectInstanceName', 'registerObjectInstance', 'updateAttributeValues', 'sendInteraction', 'deleteObjectInstance', 'localDeleteObjectInstance', 'requestAttributeValueUpdate', 'requestAttributeTransportationTypeChange', 'queryAttributeTransportationType', 'requestInteractionTransportationTypeChange', 'queryInteractionTransportationType', 'unconditionalAttributeOwnershipDivestiture', 'negotiatedAttributeOwnershipDivestiture', 'confirmDivestiture', 'attributeOwnershipAcquisition', 'attributeOwnershipAcquisitionIfAvailable', 'attributeOwnershipReleaseDenied', 'attributeOwnershipDivestitureIfWanted', 'cancelNegotiatedAttributeOwnershipDivestiture', 'cancelAttributeOwnershipAcquisition', 'queryAttributeOwnership', 'isAttributeOwnedByFederate', 'enableTimeRegulation', 'disableTimeRegulation', 'enableTimeConstrained', 'disableTimeConstrained', 'timeAdvanceRequest', 'timeAdvanceRequestAvailable', 'nextMessageRequest', 'nextMessageRequestAvailable', 'flushQueueRequest', 'enableAsynchronousDelivery', 'disableAsynchronousDelivery', 'queryGALT', 'queryLogicalTime', 'queryLITS', 'modifyLookahead', 'queryLookahead', 'retract', 'changeAttributeOrderType', 'changeInteractionOrderType', 'createRegion', 'commitRegionModifications', 'deleteRegion', 'registerObjectInstanceWithRegions', 'associateRegionsForUpdates', 'unassociateRegionsForUpdates', 'subscribeObjectClassAttributesWithRegions', 'subscribeObjectClassAttributesPassivelyWithRegions', 'unsubscribeObjectClassAttributesWithRegions', 'subscribeInteractionClassWithRegions', 'subscribeInteractionClassPassivelyWithRegions', 'unsubscribeInteractionClassWithRegions', 'sendInteractionWithRegions', 'requestAttributeValueUpdateWithRegions', 'getAutomaticResignDirective', 'setAutomaticResignDirective', 'getFederateHandle', 'getFederateName', 'getObjectClassHandle', 'getObjectClassName', 'getKnownObjectClassHandle', 'getObjectInstanceHandle', 'getObjectInstanceName', 'getAttributeHandle', 'getAttributeName', 'getUpdateRateValue', 'getUpdateRateValueForAttribute', 'getInteractionClassHandle', 'getInteractionClassName', 'getParameterHandle', 'getParameterName', 'getOrderType', 'getOrderName', 'getTransportationTypeHandle', 'getTransportationTypeName', 'getAvailableDimensionsForClassAttribute', 'getAvailableDimensionsForInteractionClass', 'getDimensionHandle', 'getDimensionName', 'getDimensionUpperBound', 'getDimensionHandleSet', 'getRangeBounds', 'setRangeBounds', 'normalizeFederateHandle', 'normalizeServiceGroup', 'enableObjectClassRelevanceAdvisorySwitch', 'disableObjectClassRelevanceAdvisorySwitch', 'enableAttributeRelevanceAdvisorySwitch', 'disableAttributeRelevanceAdvisorySwitch', 'enableAttributeScopeAdvisorySwitch', 'disableAttributeScopeAdvisorySwitch', 'enableInteractionRelevanceAdvisorySwitch', 'disableInteractionRelevanceAdvisorySwitch', 'evokeCallback', 'evokeMultipleCallbacks', 'enableCallbacks', 'disableCallbacks', 'getAttributeHandleFactory', 'getAttributeHandleSetFactory', 'getAttributeHandleValueMapFactory', 'getAttributeSetRegionSetPairListFactory', 'getDimensionHandleFactory', 'getDimensionHandleSetFactory', 'getFederateHandleFactory', 'getFederateHandleSetFactory', 'getInteractionClassHandleFactory', 'getObjectClassHandleFactory', 'getObjectInstanceHandleFactory', 'getParameterHandleFactory', 'getParameterHandleValueMapFactory', 'getRegionHandleSetFactory', 'getTransportationTypeHandleFactory', 'getHLAversion', 'getTimeFactory')
FEDERATE_AMBASSADOR_METHODS = ('connectionLost', 'reportFederationExecutions', 'synchronizationPointRegistrationSucceeded', 'synchronizationPointRegistrationFailed', 'announceSynchronizationPoint', 'federationSynchronized', 'initiateFederateSave', 'federationSaved', 'federationNotSaved', 'federationSaveStatusResponse', 'requestFederationRestoreSucceeded', 'requestFederationRestoreFailed', 'federationRestoreBegun', 'initiateFederateRestore', 'federationRestored', 'federationNotRestored', 'federationRestoreStatusResponse', 'startRegistrationForObjectClass', 'stopRegistrationForObjectClass', 'turnInteractionsOn', 'turnInteractionsOff', 'objectInstanceNameReservationSucceeded', 'objectInstanceNameReservationFailed', 'multipleObjectInstanceNameReservationSucceeded', 'multipleObjectInstanceNameReservationFailed', 'discoverObjectInstance', 'reflectAttributeValues', 'receiveInteraction', 'removeObjectInstance', 'attributesInScope', 'attributesOutOfScope', 'provideAttributeValueUpdate', 'turnUpdatesOnForObjectInstance', 'turnUpdatesOffForObjectInstance', 'confirmAttributeTransportationTypeChange', 'reportAttributeTransportationType', 'confirmInteractionTransportationTypeChange', 'reportInteractionTransportationType', 'requestAttributeOwnershipAssumption', 'requestDivestitureConfirmation', 'attributeOwnershipAcquisitionNotification', 'attributeOwnershipUnavailable', 'requestAttributeOwnershipRelease', 'confirmAttributeOwnershipAcquisitionCancellation', 'informAttributeOwnership', 'attributeIsNotOwned', 'attributeIsOwnedByRTI', 'timeRegulationEnabled', 'timeConstrainedEnabled', 'timeAdvanceGrant', 'requestRetraction')
RTIAMBASSADOR_OVERLOAD_COUNTS = {'connect': 2, 'disconnect': 1, 'createFederationExecution': 5, 'destroyFederationExecution': 1, 'listFederationExecutions': 1, 'joinFederationExecution': 4, 'resignFederationExecution': 1, 'registerFederationSynchronizationPoint': 2, 'synchronizationPointAchieved': 2, 'requestFederationSave': 2, 'federateSaveBegun': 1, 'federateSaveComplete': 1, 'federateSaveNotComplete': 1, 'abortFederationSave': 1, 'queryFederationSaveStatus': 1, 'requestFederationRestore': 1, 'federateRestoreComplete': 1, 'federateRestoreNotComplete': 1, 'abortFederationRestore': 1, 'queryFederationRestoreStatus': 1, 'publishObjectClassAttributes': 1, 'unpublishObjectClass': 1, 'unpublishObjectClassAttributes': 1, 'publishInteractionClass': 1, 'unpublishInteractionClass': 1, 'subscribeObjectClassAttributes': 2, 'subscribeObjectClassAttributesPassively': 2, 'unsubscribeObjectClass': 1, 'unsubscribeObjectClassAttributes': 1, 'subscribeInteractionClass': 1, 'subscribeInteractionClassPassively': 1, 'unsubscribeInteractionClass': 1, 'reserveObjectInstanceName': 1, 'releaseObjectInstanceName': 1, 'reserveMultipleObjectInstanceName': 1, 'releaseMultipleObjectInstanceName': 1, 'registerObjectInstance': 2, 'updateAttributeValues': 2, 'sendInteraction': 2, 'deleteObjectInstance': 2, 'localDeleteObjectInstance': 1, 'requestAttributeValueUpdate': 2, 'requestAttributeTransportationTypeChange': 1, 'queryAttributeTransportationType': 1, 'requestInteractionTransportationTypeChange': 1, 'queryInteractionTransportationType': 1, 'unconditionalAttributeOwnershipDivestiture': 1, 'negotiatedAttributeOwnershipDivestiture': 1, 'confirmDivestiture': 1, 'attributeOwnershipAcquisition': 1, 'attributeOwnershipAcquisitionIfAvailable': 1, 'attributeOwnershipReleaseDenied': 1, 'attributeOwnershipDivestitureIfWanted': 1, 'cancelNegotiatedAttributeOwnershipDivestiture': 1, 'cancelAttributeOwnershipAcquisition': 1, 'queryAttributeOwnership': 1, 'isAttributeOwnedByFederate': 1, 'enableTimeRegulation': 1, 'disableTimeRegulation': 1, 'enableTimeConstrained': 1, 'disableTimeConstrained': 1, 'timeAdvanceRequest': 1, 'timeAdvanceRequestAvailable': 1, 'nextMessageRequest': 1, 'nextMessageRequestAvailable': 1, 'flushQueueRequest': 1, 'enableAsynchronousDelivery': 1, 'disableAsynchronousDelivery': 1, 'queryGALT': 1, 'queryLogicalTime': 1, 'queryLITS': 1, 'modifyLookahead': 1, 'queryLookahead': 1, 'retract': 1, 'changeAttributeOrderType': 1, 'changeInteractionOrderType': 1, 'createRegion': 1, 'commitRegionModifications': 1, 'deleteRegion': 1, 'registerObjectInstanceWithRegions': 2, 'associateRegionsForUpdates': 1, 'unassociateRegionsForUpdates': 1, 'subscribeObjectClassAttributesWithRegions': 2, 'subscribeObjectClassAttributesPassivelyWithRegions': 2, 'unsubscribeObjectClassAttributesWithRegions': 1, 'subscribeInteractionClassWithRegions': 1, 'subscribeInteractionClassPassivelyWithRegions': 1, 'unsubscribeInteractionClassWithRegions': 1, 'sendInteractionWithRegions': 2, 'requestAttributeValueUpdateWithRegions': 1, 'getAutomaticResignDirective': 1, 'setAutomaticResignDirective': 1, 'getFederateHandle': 1, 'getFederateName': 1, 'getObjectClassHandle': 1, 'getObjectClassName': 1, 'getKnownObjectClassHandle': 1, 'getObjectInstanceHandle': 1, 'getObjectInstanceName': 1, 'getAttributeHandle': 1, 'getAttributeName': 1, 'getUpdateRateValue': 1, 'getUpdateRateValueForAttribute': 1, 'getInteractionClassHandle': 1, 'getInteractionClassName': 1, 'getParameterHandle': 1, 'getParameterName': 1, 'getOrderType': 1, 'getOrderName': 1, 'getTransportationTypeHandle': 1, 'getTransportationTypeName': 1, 'getAvailableDimensionsForClassAttribute': 1, 'getAvailableDimensionsForInteractionClass': 1, 'getDimensionHandle': 1, 'getDimensionName': 1, 'getDimensionUpperBound': 1, 'getDimensionHandleSet': 1, 'getRangeBounds': 1, 'setRangeBounds': 1, 'normalizeFederateHandle': 1, 'normalizeServiceGroup': 1, 'enableObjectClassRelevanceAdvisorySwitch': 1, 'disableObjectClassRelevanceAdvisorySwitch': 1, 'enableAttributeRelevanceAdvisorySwitch': 1, 'disableAttributeRelevanceAdvisorySwitch': 1, 'enableAttributeScopeAdvisorySwitch': 1, 'disableAttributeScopeAdvisorySwitch': 1, 'enableInteractionRelevanceAdvisorySwitch': 1, 'disableInteractionRelevanceAdvisorySwitch': 1, 'evokeCallback': 1, 'evokeMultipleCallbacks': 1, 'enableCallbacks': 1, 'disableCallbacks': 1, 'getAttributeHandleFactory': 1, 'getAttributeHandleSetFactory': 1, 'getAttributeHandleValueMapFactory': 1, 'getAttributeSetRegionSetPairListFactory': 1, 'getDimensionHandleFactory': 1, 'getDimensionHandleSetFactory': 1, 'getFederateHandleFactory': 1, 'getFederateHandleSetFactory': 1, 'getInteractionClassHandleFactory': 1, 'getObjectClassHandleFactory': 1, 'getObjectInstanceHandleFactory': 1, 'getParameterHandleFactory': 1, 'getParameterHandleValueMapFactory': 1, 'getRegionHandleSetFactory': 1, 'getTransportationTypeHandleFactory': 1, 'getHLAversion': 1, 'getTimeFactory': 1}
FEDERATE_AMBASSADOR_OVERLOAD_COUNTS = {'connectionLost': 1, 'reportFederationExecutions': 1, 'synchronizationPointRegistrationSucceeded': 1, 'synchronizationPointRegistrationFailed': 1, 'announceSynchronizationPoint': 1, 'federationSynchronized': 1, 'initiateFederateSave': 2, 'federationSaved': 1, 'federationNotSaved': 1, 'federationSaveStatusResponse': 1, 'requestFederationRestoreSucceeded': 1, 'requestFederationRestoreFailed': 1, 'federationRestoreBegun': 1, 'initiateFederateRestore': 1, 'federationRestored': 1, 'federationNotRestored': 1, 'federationRestoreStatusResponse': 1, 'startRegistrationForObjectClass': 1, 'stopRegistrationForObjectClass': 1, 'turnInteractionsOn': 1, 'turnInteractionsOff': 1, 'objectInstanceNameReservationSucceeded': 1, 'objectInstanceNameReservationFailed': 1, 'multipleObjectInstanceNameReservationSucceeded': 1, 'multipleObjectInstanceNameReservationFailed': 1, 'discoverObjectInstance': 2, 'reflectAttributeValues': 3, 'receiveInteraction': 3, 'removeObjectInstance': 3, 'attributesInScope': 1, 'attributesOutOfScope': 1, 'provideAttributeValueUpdate': 1, 'turnUpdatesOnForObjectInstance': 2, 'turnUpdatesOffForObjectInstance': 1, 'confirmAttributeTransportationTypeChange': 1, 'reportAttributeTransportationType': 1, 'confirmInteractionTransportationTypeChange': 1, 'reportInteractionTransportationType': 1, 'requestAttributeOwnershipAssumption': 1, 'requestDivestitureConfirmation': 1, 'attributeOwnershipAcquisitionNotification': 1, 'attributeOwnershipUnavailable': 1, 'requestAttributeOwnershipRelease': 1, 'confirmAttributeOwnershipAcquisitionCancellation': 1, 'informAttributeOwnership': 1, 'attributeIsNotOwned': 1, 'attributeIsOwnedByRTI': 1, 'timeRegulationEnabled': 1, 'timeConstrainedEnabled': 1, 'timeAdvanceGrant': 1, 'requestRetraction': 1}
RTIAMBASSADOR_PARAMETER_TYPES = {'connect': (('FederateAmbassador', 'CallbackModel', 'String'), ('FederateAmbassador', 'CallbackModel')), 'disconnect': ((),), 'createFederationExecution': (('String', 'URL[]', 'URL', 'String'), ('String', 'URL[]', 'String'), ('String', 'URL[]', 'URL'), ('String', 'URL[]'), ('String', 'URL')), 'destroyFederationExecution': (('String',),), 'listFederationExecutions': ((),), 'joinFederationExecution': (('String', 'String', 'String', 'URL[]'), ('String', 'String', 'URL[]'), ('String', 'String', 'String'), ('String', 'String')), 'resignFederationExecution': (('ResignAction',),), 'registerFederationSynchronizationPoint': (('String', 'byte[]'), ('String', 'byte[]', 'FederateHandleSet')), 'synchronizationPointAchieved': (('String',), ('String', 'boolean')), 'requestFederationSave': (('String',), ('String', 'LogicalTime')), 'federateSaveBegun': ((),), 'federateSaveComplete': ((),), 'federateSaveNotComplete': ((),), 'abortFederationSave': ((),), 'queryFederationSaveStatus': ((),), 'requestFederationRestore': (('String',),), 'federateRestoreComplete': ((),), 'federateRestoreNotComplete': ((),), 'abortFederationRestore': ((),), 'queryFederationRestoreStatus': ((),), 'publishObjectClassAttributes': (('ObjectClassHandle', 'AttributeHandleSet'),), 'unpublishObjectClass': (('ObjectClassHandle',),), 'unpublishObjectClassAttributes': (('ObjectClassHandle', 'AttributeHandleSet'),), 'publishInteractionClass': (('InteractionClassHandle',),), 'unpublishInteractionClass': (('InteractionClassHandle',),), 'subscribeObjectClassAttributes': (('ObjectClassHandle', 'AttributeHandleSet'), ('ObjectClassHandle', 'AttributeHandleSet', 'String')), 'subscribeObjectClassAttributesPassively': (('ObjectClassHandle', 'AttributeHandleSet'), ('ObjectClassHandle', 'AttributeHandleSet', 'String')), 'unsubscribeObjectClass': (('ObjectClassHandle',),), 'unsubscribeObjectClassAttributes': (('ObjectClassHandle', 'AttributeHandleSet'),), 'subscribeInteractionClass': (('InteractionClassHandle',),), 'subscribeInteractionClassPassively': (('InteractionClassHandle',),), 'unsubscribeInteractionClass': (('InteractionClassHandle',),), 'reserveObjectInstanceName': (('String',),), 'releaseObjectInstanceName': (('String',),), 'reserveMultipleObjectInstanceName': (('Set<String>',),), 'releaseMultipleObjectInstanceName': (('Set<String>',),), 'registerObjectInstance': (('ObjectClassHandle',), ('ObjectClassHandle', 'String')), 'updateAttributeValues': (('ObjectInstanceHandle', 'AttributeHandleValueMap', 'byte[]'), ('ObjectInstanceHandle', 'AttributeHandleValueMap', 'byte[]', 'LogicalTime')), 'sendInteraction': (('InteractionClassHandle', 'ParameterHandleValueMap', 'byte[]'), ('InteractionClassHandle', 'ParameterHandleValueMap', 'byte[]', 'LogicalTime')), 'deleteObjectInstance': (('ObjectInstanceHandle', 'byte[]'), ('ObjectInstanceHandle', 'byte[]', 'LogicalTime')), 'localDeleteObjectInstance': (('ObjectInstanceHandle',),), 'requestAttributeValueUpdate': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'), ('ObjectClassHandle', 'AttributeHandleSet', 'byte[]')), 'requestAttributeTransportationTypeChange': (('ObjectInstanceHandle', 'AttributeHandleSet', 'TransportationTypeHandle'),), 'queryAttributeTransportationType': (('ObjectInstanceHandle', 'AttributeHandle'),), 'requestInteractionTransportationTypeChange': (('InteractionClassHandle', 'TransportationTypeHandle'),), 'queryInteractionTransportationType': (('FederateHandle', 'InteractionClassHandle'),), 'unconditionalAttributeOwnershipDivestiture': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'negotiatedAttributeOwnershipDivestiture': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'),), 'confirmDivestiture': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'),), 'attributeOwnershipAcquisition': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'),), 'attributeOwnershipAcquisitionIfAvailable': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'attributeOwnershipReleaseDenied': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'attributeOwnershipDivestitureIfWanted': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'cancelNegotiatedAttributeOwnershipDivestiture': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'cancelAttributeOwnershipAcquisition': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'queryAttributeOwnership': (('ObjectInstanceHandle', 'AttributeHandle'),), 'isAttributeOwnedByFederate': (('ObjectInstanceHandle', 'AttributeHandle'),), 'enableTimeRegulation': (('LogicalTimeInterval',),), 'disableTimeRegulation': ((),), 'enableTimeConstrained': ((),), 'disableTimeConstrained': ((),), 'timeAdvanceRequest': (('LogicalTime',),), 'timeAdvanceRequestAvailable': (('LogicalTime',),), 'nextMessageRequest': (('LogicalTime',),), 'nextMessageRequestAvailable': (('LogicalTime',),), 'flushQueueRequest': (('LogicalTime',),), 'enableAsynchronousDelivery': ((),), 'disableAsynchronousDelivery': ((),), 'queryGALT': ((),), 'queryLogicalTime': ((),), 'queryLITS': ((),), 'modifyLookahead': (('LogicalTimeInterval',),), 'queryLookahead': ((),), 'retract': (('MessageRetractionHandle',),), 'changeAttributeOrderType': (('ObjectInstanceHandle', 'AttributeHandleSet', 'OrderType'),), 'changeInteractionOrderType': (('InteractionClassHandle', 'OrderType'),), 'createRegion': (('DimensionHandleSet',),), 'commitRegionModifications': (('RegionHandleSet',),), 'deleteRegion': (('RegionHandle',),), 'registerObjectInstanceWithRegions': (('ObjectClassHandle', 'AttributeSetRegionSetPairList'), ('ObjectClassHandle', 'AttributeSetRegionSetPairList', 'String')), 'associateRegionsForUpdates': (('ObjectInstanceHandle', 'AttributeSetRegionSetPairList'),), 'unassociateRegionsForUpdates': (('ObjectInstanceHandle', 'AttributeSetRegionSetPairList'),), 'subscribeObjectClassAttributesWithRegions': (('ObjectClassHandle', 'AttributeSetRegionSetPairList'), ('ObjectClassHandle', 'AttributeSetRegionSetPairList', 'String')), 'subscribeObjectClassAttributesPassivelyWithRegions': (('ObjectClassHandle', 'AttributeSetRegionSetPairList'), ('ObjectClassHandle', 'AttributeSetRegionSetPairList', 'String')), 'unsubscribeObjectClassAttributesWithRegions': (('ObjectClassHandle', 'AttributeSetRegionSetPairList'),), 'subscribeInteractionClassWithRegions': (('InteractionClassHandle', 'RegionHandleSet'),), 'subscribeInteractionClassPassivelyWithRegions': (('InteractionClassHandle', 'RegionHandleSet'),), 'unsubscribeInteractionClassWithRegions': (('InteractionClassHandle', 'RegionHandleSet'),), 'sendInteractionWithRegions': (('InteractionClassHandle', 'ParameterHandleValueMap', 'RegionHandleSet', 'byte[]'), ('InteractionClassHandle', 'ParameterHandleValueMap', 'RegionHandleSet', 'byte[]', 'LogicalTime')), 'requestAttributeValueUpdateWithRegions': (('ObjectClassHandle', 'AttributeSetRegionSetPairList', 'byte[]'),), 'getAutomaticResignDirective': ((),), 'setAutomaticResignDirective': (('ResignAction',),), 'getFederateHandle': (('String',),), 'getFederateName': (('FederateHandle',),), 'getObjectClassHandle': (('String',),), 'getObjectClassName': (('ObjectClassHandle',),), 'getKnownObjectClassHandle': (('ObjectInstanceHandle',),), 'getObjectInstanceHandle': (('String',),), 'getObjectInstanceName': (('ObjectInstanceHandle',),), 'getAttributeHandle': (('ObjectClassHandle', 'String'),), 'getAttributeName': (('ObjectClassHandle', 'AttributeHandle'),), 'getUpdateRateValue': (('String',),), 'getUpdateRateValueForAttribute': (('ObjectInstanceHandle', 'AttributeHandle'),), 'getInteractionClassHandle': (('String',),), 'getInteractionClassName': (('InteractionClassHandle',),), 'getParameterHandle': (('InteractionClassHandle', 'String'),), 'getParameterName': (('InteractionClassHandle', 'ParameterHandle'),), 'getOrderType': (('String',),), 'getOrderName': (('OrderType',),), 'getTransportationTypeHandle': (('String',),), 'getTransportationTypeName': (('TransportationTypeHandle',),), 'getAvailableDimensionsForClassAttribute': (('ObjectClassHandle', 'AttributeHandle'),), 'getAvailableDimensionsForInteractionClass': (('InteractionClassHandle',),), 'getDimensionHandle': (('String',),), 'getDimensionName': (('DimensionHandle',),), 'getDimensionUpperBound': (('DimensionHandle',),), 'getDimensionHandleSet': (('RegionHandle',),), 'getRangeBounds': (('RegionHandle', 'DimensionHandle'),), 'setRangeBounds': (('RegionHandle', 'DimensionHandle', 'RangeBounds'),), 'normalizeFederateHandle': (('FederateHandle',),), 'normalizeServiceGroup': (('ServiceGroup',),), 'enableObjectClassRelevanceAdvisorySwitch': ((),), 'disableObjectClassRelevanceAdvisorySwitch': ((),), 'enableAttributeRelevanceAdvisorySwitch': ((),), 'disableAttributeRelevanceAdvisorySwitch': ((),), 'enableAttributeScopeAdvisorySwitch': ((),), 'disableAttributeScopeAdvisorySwitch': ((),), 'enableInteractionRelevanceAdvisorySwitch': ((),), 'disableInteractionRelevanceAdvisorySwitch': ((),), 'evokeCallback': (('double',),), 'evokeMultipleCallbacks': (('double', 'double'),), 'enableCallbacks': ((),), 'disableCallbacks': ((),), 'getAttributeHandleFactory': ((),), 'getAttributeHandleSetFactory': ((),), 'getAttributeHandleValueMapFactory': ((),), 'getAttributeSetRegionSetPairListFactory': ((),), 'getDimensionHandleFactory': ((),), 'getDimensionHandleSetFactory': ((),), 'getFederateHandleFactory': ((),), 'getFederateHandleSetFactory': ((),), 'getInteractionClassHandleFactory': ((),), 'getObjectClassHandleFactory': ((),), 'getObjectInstanceHandleFactory': ((),), 'getParameterHandleFactory': ((),), 'getParameterHandleValueMapFactory': ((),), 'getRegionHandleSetFactory': ((),), 'getTransportationTypeHandleFactory': ((),), 'getHLAversion': ((),), 'getTimeFactory': ((),)}
FEDERATE_AMBASSADOR_PARAMETER_TYPES = {'connectionLost': (('String',),), 'reportFederationExecutions': (('FederationExecutionInformationSet',),), 'synchronizationPointRegistrationSucceeded': (('String',),), 'synchronizationPointRegistrationFailed': (('String', 'SynchronizationPointFailureReason'),), 'announceSynchronizationPoint': (('String', 'byte[]'),), 'federationSynchronized': (('String', 'FederateHandleSet'),), 'initiateFederateSave': (('String',), ('String', 'LogicalTime')), 'federationSaved': ((),), 'federationNotSaved': (('SaveFailureReason',),), 'federationSaveStatusResponse': (('FederateHandleSaveStatusPair[]',),), 'requestFederationRestoreSucceeded': (('String',),), 'requestFederationRestoreFailed': (('String',),), 'federationRestoreBegun': ((),), 'initiateFederateRestore': (('String', 'String', 'FederateHandle'),), 'federationRestored': ((),), 'federationNotRestored': (('RestoreFailureReason',),), 'federationRestoreStatusResponse': (('FederateRestoreStatus[]',),), 'startRegistrationForObjectClass': (('ObjectClassHandle',),), 'stopRegistrationForObjectClass': (('ObjectClassHandle',),), 'turnInteractionsOn': (('InteractionClassHandle',),), 'turnInteractionsOff': (('InteractionClassHandle',),), 'objectInstanceNameReservationSucceeded': (('String',),), 'objectInstanceNameReservationFailed': (('String',),), 'multipleObjectInstanceNameReservationSucceeded': (('Set<String>',),), 'multipleObjectInstanceNameReservationFailed': (('Set<String>',),), 'discoverObjectInstance': (('ObjectInstanceHandle', 'ObjectClassHandle', 'String'), ('ObjectInstanceHandle', 'ObjectClassHandle', 'String', 'FederateHandle')), 'reflectAttributeValues': (('ObjectInstanceHandle', 'AttributeHandleValueMap', 'byte[]', 'OrderType', 'TransportationTypeHandle', 'SupplementalReflectInfo'), ('ObjectInstanceHandle', 'AttributeHandleValueMap', 'byte[]', 'OrderType', 'TransportationTypeHandle', 'LogicalTime', 'OrderType', 'SupplementalReflectInfo'), ('ObjectInstanceHandle', 'AttributeHandleValueMap', 'byte[]', 'OrderType', 'TransportationTypeHandle', 'LogicalTime', 'OrderType', 'MessageRetractionHandle', 'SupplementalReflectInfo')), 'receiveInteraction': (('InteractionClassHandle', 'ParameterHandleValueMap', 'byte[]', 'OrderType', 'TransportationTypeHandle', 'SupplementalReceiveInfo'), ('InteractionClassHandle', 'ParameterHandleValueMap', 'byte[]', 'OrderType', 'TransportationTypeHandle', 'LogicalTime', 'OrderType', 'SupplementalReceiveInfo'), ('InteractionClassHandle', 'ParameterHandleValueMap', 'byte[]', 'OrderType', 'TransportationTypeHandle', 'LogicalTime', 'OrderType', 'MessageRetractionHandle', 'SupplementalReceiveInfo')), 'removeObjectInstance': (('ObjectInstanceHandle', 'byte[]', 'OrderType', 'SupplementalRemoveInfo'), ('ObjectInstanceHandle', 'byte[]', 'OrderType', 'LogicalTime', 'OrderType', 'SupplementalRemoveInfo'), ('ObjectInstanceHandle', 'byte[]', 'OrderType', 'LogicalTime', 'OrderType', 'MessageRetractionHandle', 'SupplementalRemoveInfo')), 'attributesInScope': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'attributesOutOfScope': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'provideAttributeValueUpdate': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'),), 'turnUpdatesOnForObjectInstance': (('ObjectInstanceHandle', 'AttributeHandleSet'), ('ObjectInstanceHandle', 'AttributeHandleSet', 'String')), 'turnUpdatesOffForObjectInstance': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'confirmAttributeTransportationTypeChange': (('ObjectInstanceHandle', 'AttributeHandleSet', 'TransportationTypeHandle'),), 'reportAttributeTransportationType': (('ObjectInstanceHandle', 'AttributeHandle', 'TransportationTypeHandle'),), 'confirmInteractionTransportationTypeChange': (('InteractionClassHandle', 'TransportationTypeHandle'),), 'reportInteractionTransportationType': (('FederateHandle', 'InteractionClassHandle', 'TransportationTypeHandle'),), 'requestAttributeOwnershipAssumption': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'),), 'requestDivestitureConfirmation': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'attributeOwnershipAcquisitionNotification': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'),), 'attributeOwnershipUnavailable': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'requestAttributeOwnershipRelease': (('ObjectInstanceHandle', 'AttributeHandleSet', 'byte[]'),), 'confirmAttributeOwnershipAcquisitionCancellation': (('ObjectInstanceHandle', 'AttributeHandleSet'),), 'informAttributeOwnership': (('ObjectInstanceHandle', 'AttributeHandle', 'FederateHandle'),), 'attributeIsNotOwned': (('ObjectInstanceHandle', 'AttributeHandle'),), 'attributeIsOwnedByRTI': (('ObjectInstanceHandle', 'AttributeHandle'),), 'timeRegulationEnabled': (('LogicalTime',),), 'timeConstrainedEnabled': (('LogicalTime',),), 'timeAdvanceGrant': (('LogicalTime',),), 'requestRetraction': (('MessageRetractionHandle',),)}
RTIAMBASSADOR_RETURN_TYPES = {'connect': ('void', 'void'), 'disconnect': ('void',), 'createFederationExecution': ('void', 'void', 'void', 'void', 'void'), 'destroyFederationExecution': ('void',), 'listFederationExecutions': ('void',), 'joinFederationExecution': ('FederateHandle', 'FederateHandle', 'FederateHandle', 'FederateHandle'), 'resignFederationExecution': ('void',), 'registerFederationSynchronizationPoint': ('void', 'void'), 'synchronizationPointAchieved': ('void', 'void'), 'requestFederationSave': ('void', 'void'), 'federateSaveBegun': ('void',), 'federateSaveComplete': ('void',), 'federateSaveNotComplete': ('void',), 'abortFederationSave': ('void',), 'queryFederationSaveStatus': ('void',), 'requestFederationRestore': ('void',), 'federateRestoreComplete': ('void',), 'federateRestoreNotComplete': ('void',), 'abortFederationRestore': ('void',), 'queryFederationRestoreStatus': ('void',), 'publishObjectClassAttributes': ('void',), 'unpublishObjectClass': ('void',), 'unpublishObjectClassAttributes': ('void',), 'publishInteractionClass': ('void',), 'unpublishInteractionClass': ('void',), 'subscribeObjectClassAttributes': ('void', 'void'), 'subscribeObjectClassAttributesPassively': ('void', 'void'), 'unsubscribeObjectClass': ('void',), 'unsubscribeObjectClassAttributes': ('void',), 'subscribeInteractionClass': ('void',), 'subscribeInteractionClassPassively': ('void',), 'unsubscribeInteractionClass': ('void',), 'reserveObjectInstanceName': ('void',), 'releaseObjectInstanceName': ('void',), 'reserveMultipleObjectInstanceName': ('void',), 'releaseMultipleObjectInstanceName': ('void',), 'registerObjectInstance': ('ObjectInstanceHandle', 'ObjectInstanceHandle'), 'updateAttributeValues': ('void', 'MessageRetractionReturn'), 'sendInteraction': ('void', 'MessageRetractionReturn'), 'deleteObjectInstance': ('void', 'MessageRetractionReturn'), 'localDeleteObjectInstance': ('void',), 'requestAttributeValueUpdate': ('void', 'void'), 'requestAttributeTransportationTypeChange': ('void',), 'queryAttributeTransportationType': ('void',), 'requestInteractionTransportationTypeChange': ('void',), 'queryInteractionTransportationType': ('void',), 'unconditionalAttributeOwnershipDivestiture': ('void',), 'negotiatedAttributeOwnershipDivestiture': ('void',), 'confirmDivestiture': ('void',), 'attributeOwnershipAcquisition': ('void',), 'attributeOwnershipAcquisitionIfAvailable': ('void',), 'attributeOwnershipReleaseDenied': ('void',), 'attributeOwnershipDivestitureIfWanted': ('AttributeHandleSet',), 'cancelNegotiatedAttributeOwnershipDivestiture': ('void',), 'cancelAttributeOwnershipAcquisition': ('void',), 'queryAttributeOwnership': ('void',), 'isAttributeOwnedByFederate': ('boolean',), 'enableTimeRegulation': ('void',), 'disableTimeRegulation': ('void',), 'enableTimeConstrained': ('void',), 'disableTimeConstrained': ('void',), 'timeAdvanceRequest': ('void',), 'timeAdvanceRequestAvailable': ('void',), 'nextMessageRequest': ('void',), 'nextMessageRequestAvailable': ('void',), 'flushQueueRequest': ('void',), 'enableAsynchronousDelivery': ('void',), 'disableAsynchronousDelivery': ('void',), 'queryGALT': ('TimeQueryReturn',), 'queryLogicalTime': ('LogicalTime',), 'queryLITS': ('TimeQueryReturn',), 'modifyLookahead': ('void',), 'queryLookahead': ('LogicalTimeInterval',), 'retract': ('void',), 'changeAttributeOrderType': ('void',), 'changeInteractionOrderType': ('void',), 'createRegion': ('RegionHandle',), 'commitRegionModifications': ('void',), 'deleteRegion': ('void',), 'registerObjectInstanceWithRegions': ('ObjectInstanceHandle', 'ObjectInstanceHandle'), 'associateRegionsForUpdates': ('void',), 'unassociateRegionsForUpdates': ('void',), 'subscribeObjectClassAttributesWithRegions': ('void', 'void'), 'subscribeObjectClassAttributesPassivelyWithRegions': ('void', 'void'), 'unsubscribeObjectClassAttributesWithRegions': ('void',), 'subscribeInteractionClassWithRegions': ('void',), 'subscribeInteractionClassPassivelyWithRegions': ('void',), 'unsubscribeInteractionClassWithRegions': ('void',), 'sendInteractionWithRegions': ('void', 'MessageRetractionReturn'), 'requestAttributeValueUpdateWithRegions': ('void',), 'getAutomaticResignDirective': ('ResignAction',), 'setAutomaticResignDirective': ('void',), 'getFederateHandle': ('FederateHandle',), 'getFederateName': ('String',), 'getObjectClassHandle': ('ObjectClassHandle',), 'getObjectClassName': ('String',), 'getKnownObjectClassHandle': ('ObjectClassHandle',), 'getObjectInstanceHandle': ('ObjectInstanceHandle',), 'getObjectInstanceName': ('String',), 'getAttributeHandle': ('AttributeHandle',), 'getAttributeName': ('String',), 'getUpdateRateValue': ('double',), 'getUpdateRateValueForAttribute': ('double',), 'getInteractionClassHandle': ('InteractionClassHandle',), 'getInteractionClassName': ('String',), 'getParameterHandle': ('ParameterHandle',), 'getParameterName': ('String',), 'getOrderType': ('OrderType',), 'getOrderName': ('String',), 'getTransportationTypeHandle': ('TransportationTypeHandle',), 'getTransportationTypeName': ('String',), 'getAvailableDimensionsForClassAttribute': ('DimensionHandleSet',), 'getAvailableDimensionsForInteractionClass': ('DimensionHandleSet',), 'getDimensionHandle': ('DimensionHandle',), 'getDimensionName': ('String',), 'getDimensionUpperBound': ('long',), 'getDimensionHandleSet': ('DimensionHandleSet',), 'getRangeBounds': ('RangeBounds',), 'setRangeBounds': ('void',), 'normalizeFederateHandle': ('long',), 'normalizeServiceGroup': ('long',), 'enableObjectClassRelevanceAdvisorySwitch': ('void',), 'disableObjectClassRelevanceAdvisorySwitch': ('void',), 'enableAttributeRelevanceAdvisorySwitch': ('void',), 'disableAttributeRelevanceAdvisorySwitch': ('void',), 'enableAttributeScopeAdvisorySwitch': ('void',), 'disableAttributeScopeAdvisorySwitch': ('void',), 'enableInteractionRelevanceAdvisorySwitch': ('void',), 'disableInteractionRelevanceAdvisorySwitch': ('void',), 'evokeCallback': ('boolean',), 'evokeMultipleCallbacks': ('boolean',), 'enableCallbacks': ('void',), 'disableCallbacks': ('void',), 'getAttributeHandleFactory': ('AttributeHandleFactory',), 'getAttributeHandleSetFactory': ('AttributeHandleSetFactory',), 'getAttributeHandleValueMapFactory': ('AttributeHandleValueMapFactory',), 'getAttributeSetRegionSetPairListFactory': ('AttributeSetRegionSetPairListFactory',), 'getDimensionHandleFactory': ('DimensionHandleFactory',), 'getDimensionHandleSetFactory': ('DimensionHandleSetFactory',), 'getFederateHandleFactory': ('FederateHandleFactory',), 'getFederateHandleSetFactory': ('FederateHandleSetFactory',), 'getInteractionClassHandleFactory': ('InteractionClassHandleFactory',), 'getObjectClassHandleFactory': ('ObjectClassHandleFactory',), 'getObjectInstanceHandleFactory': ('ObjectInstanceHandleFactory',), 'getParameterHandleFactory': ('ParameterHandleFactory',), 'getParameterHandleValueMapFactory': ('ParameterHandleValueMapFactory',), 'getRegionHandleSetFactory': ('RegionHandleSetFactory',), 'getTransportationTypeHandleFactory': ('TransportationTypeHandleFactory',), 'getHLAversion': ('String',), 'getTimeFactory': ('LogicalTimeFactory',)}
FEDERATE_AMBASSADOR_RETURN_TYPES = {'connectionLost': ('void',), 'reportFederationExecutions': ('void',), 'synchronizationPointRegistrationSucceeded': ('void',), 'synchronizationPointRegistrationFailed': ('void',), 'announceSynchronizationPoint': ('void',), 'federationSynchronized': ('void',), 'initiateFederateSave': ('void', 'void'), 'federationSaved': ('void',), 'federationNotSaved': ('void',), 'federationSaveStatusResponse': ('void',), 'requestFederationRestoreSucceeded': ('void',), 'requestFederationRestoreFailed': ('void',), 'federationRestoreBegun': ('void',), 'initiateFederateRestore': ('void',), 'federationRestored': ('void',), 'federationNotRestored': ('void',), 'federationRestoreStatusResponse': ('void',), 'startRegistrationForObjectClass': ('void',), 'stopRegistrationForObjectClass': ('void',), 'turnInteractionsOn': ('void',), 'turnInteractionsOff': ('void',), 'objectInstanceNameReservationSucceeded': ('void',), 'objectInstanceNameReservationFailed': ('void',), 'multipleObjectInstanceNameReservationSucceeded': ('void',), 'multipleObjectInstanceNameReservationFailed': ('void',), 'discoverObjectInstance': ('void', 'void'), 'reflectAttributeValues': ('void', 'void', 'void'), 'receiveInteraction': ('void', 'void', 'void'), 'removeObjectInstance': ('void', 'void', 'void'), 'attributesInScope': ('void',), 'attributesOutOfScope': ('void',), 'provideAttributeValueUpdate': ('void',), 'turnUpdatesOnForObjectInstance': ('void', 'void'), 'turnUpdatesOffForObjectInstance': ('void',), 'confirmAttributeTransportationTypeChange': ('void',), 'reportAttributeTransportationType': ('void',), 'confirmInteractionTransportationTypeChange': ('void',), 'reportInteractionTransportationType': ('void',), 'requestAttributeOwnershipAssumption': ('void',), 'requestDivestitureConfirmation': ('void',), 'attributeOwnershipAcquisitionNotification': ('void',), 'attributeOwnershipUnavailable': ('void',), 'requestAttributeOwnershipRelease': ('void',), 'confirmAttributeOwnershipAcquisitionCancellation': ('void',), 'informAttributeOwnership': ('void',), 'attributeIsNotOwned': ('void',), 'attributeIsOwnedByRTI': ('void',), 'timeRegulationEnabled': ('void',), 'timeConstrainedEnabled': ('void',), 'timeAdvanceGrant': ('void',), 'requestRetraction': ('void',)}

class FederateAmbassador(ABC):
    """Receive callbacks from a 2010 provider."""
    __java_package__: ClassVar[str] = JAVA_PACKAGE
    __cpp_namespace__: ClassVar[str] = CPP_NAMESPACE
    __overload_counts__: ClassVar[dict[str, int]] = FEDERATE_AMBASSADOR_OVERLOAD_COUNTS

    def connectionLost(self, *args: object, **kwargs: object) -> object:
        self_name = 'connectionLost'
        return None
    def reportFederationExecutions(self, *args: object, **kwargs: object) -> object:
        self_name = 'reportFederationExecutions'
        return None
    def synchronizationPointRegistrationSucceeded(self, *args: object, **kwargs: object) -> object:
        self_name = 'synchronizationPointRegistrationSucceeded'
        return None
    def synchronizationPointRegistrationFailed(self, *args: object, **kwargs: object) -> object:
        self_name = 'synchronizationPointRegistrationFailed'
        return None
    def announceSynchronizationPoint(self, *args: object, **kwargs: object) -> object:
        self_name = 'announceSynchronizationPoint'
        return None
    def federationSynchronized(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationSynchronized'
        return None
    def initiateFederateSave(self, *args: object, **kwargs: object) -> object:
        self_name = 'initiateFederateSave'
        return None
    def federationSaved(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationSaved'
        return None
    def federationNotSaved(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationNotSaved'
        return None
    def federationSaveStatusResponse(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationSaveStatusResponse'
        return None
    def requestFederationRestoreSucceeded(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestFederationRestoreSucceeded'
        return None
    def requestFederationRestoreFailed(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestFederationRestoreFailed'
        return None
    def federationRestoreBegun(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationRestoreBegun'
        return None
    def initiateFederateRestore(self, *args: object, **kwargs: object) -> object:
        self_name = 'initiateFederateRestore'
        return None
    def federationRestored(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationRestored'
        return None
    def federationNotRestored(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationNotRestored'
        return None
    def federationRestoreStatusResponse(self, *args: object, **kwargs: object) -> object:
        self_name = 'federationRestoreStatusResponse'
        return None
    def startRegistrationForObjectClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'startRegistrationForObjectClass'
        return None
    def stopRegistrationForObjectClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'stopRegistrationForObjectClass'
        return None
    def turnInteractionsOn(self, *args: object, **kwargs: object) -> object:
        self_name = 'turnInteractionsOn'
        return None
    def turnInteractionsOff(self, *args: object, **kwargs: object) -> object:
        self_name = 'turnInteractionsOff'
        return None
    def objectInstanceNameReservationSucceeded(self, *args: object, **kwargs: object) -> object:
        self_name = 'objectInstanceNameReservationSucceeded'
        return None
    def objectInstanceNameReservationFailed(self, *args: object, **kwargs: object) -> object:
        self_name = 'objectInstanceNameReservationFailed'
        return None
    def multipleObjectInstanceNameReservationSucceeded(self, *args: object, **kwargs: object) -> object:
        self_name = 'multipleObjectInstanceNameReservationSucceeded'
        return None
    def multipleObjectInstanceNameReservationFailed(self, *args: object, **kwargs: object) -> object:
        self_name = 'multipleObjectInstanceNameReservationFailed'
        return None
    def discoverObjectInstance(self, *args: object, **kwargs: object) -> object:
        self_name = 'discoverObjectInstance'
        return None
    def reflectAttributeValues(self, *args: object, **kwargs: object) -> object:
        self_name = 'reflectAttributeValues'
        return None
    def receiveInteraction(self, *args: object, **kwargs: object) -> object:
        self_name = 'receiveInteraction'
        return None
    def removeObjectInstance(self, *args: object, **kwargs: object) -> object:
        self_name = 'removeObjectInstance'
        return None
    def attributesInScope(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributesInScope'
        return None
    def attributesOutOfScope(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributesOutOfScope'
        return None
    def provideAttributeValueUpdate(self, *args: object, **kwargs: object) -> object:
        self_name = 'provideAttributeValueUpdate'
        return None
    def turnUpdatesOnForObjectInstance(self, *args: object, **kwargs: object) -> object:
        self_name = 'turnUpdatesOnForObjectInstance'
        return None
    def turnUpdatesOffForObjectInstance(self, *args: object, **kwargs: object) -> object:
        self_name = 'turnUpdatesOffForObjectInstance'
        return None
    def confirmAttributeTransportationTypeChange(self, *args: object, **kwargs: object) -> object:
        self_name = 'confirmAttributeTransportationTypeChange'
        return None
    def reportAttributeTransportationType(self, *args: object, **kwargs: object) -> object:
        self_name = 'reportAttributeTransportationType'
        return None
    def confirmInteractionTransportationTypeChange(self, *args: object, **kwargs: object) -> object:
        self_name = 'confirmInteractionTransportationTypeChange'
        return None
    def reportInteractionTransportationType(self, *args: object, **kwargs: object) -> object:
        self_name = 'reportInteractionTransportationType'
        return None
    def requestAttributeOwnershipAssumption(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestAttributeOwnershipAssumption'
        return None
    def requestDivestitureConfirmation(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestDivestitureConfirmation'
        return None
    def attributeOwnershipAcquisitionNotification(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeOwnershipAcquisitionNotification'
        return None
    def attributeOwnershipUnavailable(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeOwnershipUnavailable'
        return None
    def requestAttributeOwnershipRelease(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestAttributeOwnershipRelease'
        return None
    def confirmAttributeOwnershipAcquisitionCancellation(self, *args: object, **kwargs: object) -> object:
        self_name = 'confirmAttributeOwnershipAcquisitionCancellation'
        return None
    def informAttributeOwnership(self, *args: object, **kwargs: object) -> object:
        self_name = 'informAttributeOwnership'
        return None
    def attributeIsNotOwned(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeIsNotOwned'
        return None
    def attributeIsOwnedByRTI(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeIsOwnedByRTI'
        return None
    def timeRegulationEnabled(self, *args: object, **kwargs: object) -> object:
        self_name = 'timeRegulationEnabled'
        return None
    def timeConstrainedEnabled(self, *args: object, **kwargs: object) -> object:
        self_name = 'timeConstrainedEnabled'
        return None
    def timeAdvanceGrant(self, *args: object, **kwargs: object) -> object:
        self_name = 'timeAdvanceGrant'
        return None
    def requestRetraction(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestRetraction'
        return None

class NullFederateAmbassador(FederateAmbassador):
    """Convenience callback sink matching the standard Java helper."""
    pass

class RTIambassador(ABC):
    """Provider-facing IEEE 1516.1-2010 RTI service contract."""
    __java_package__: ClassVar[str] = JAVA_PACKAGE
    __cpp_namespace__: ClassVar[str] = CPP_NAMESPACE
    __overload_counts__: ClassVar[dict[str, int]] = RTIAMBASSADOR_OVERLOAD_COUNTS

    @abstractmethod
    def connect(self, *args: object, **kwargs: object) -> object:
        self_name = 'connect'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disconnect(self, *args: object, **kwargs: object) -> object:
        self_name = 'disconnect'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def createFederationExecution(self, *args: object, **kwargs: object) -> object:
        self_name = 'createFederationExecution'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def destroyFederationExecution(self, *args: object, **kwargs: object) -> object:
        self_name = 'destroyFederationExecution'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def listFederationExecutions(self, *args: object, **kwargs: object) -> object:
        self_name = 'listFederationExecutions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def joinFederationExecution(self, *args: object, **kwargs: object) -> object:
        self_name = 'joinFederationExecution'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def resignFederationExecution(self, *args: object, **kwargs: object) -> object:
        self_name = 'resignFederationExecution'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def registerFederationSynchronizationPoint(self, *args: object, **kwargs: object) -> object:
        self_name = 'registerFederationSynchronizationPoint'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def synchronizationPointAchieved(self, *args: object, **kwargs: object) -> object:
        self_name = 'synchronizationPointAchieved'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def requestFederationSave(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestFederationSave'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def federateSaveBegun(self, *args: object, **kwargs: object) -> object:
        self_name = 'federateSaveBegun'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def federateSaveComplete(self, *args: object, **kwargs: object) -> object:
        self_name = 'federateSaveComplete'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def federateSaveNotComplete(self, *args: object, **kwargs: object) -> object:
        self_name = 'federateSaveNotComplete'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def abortFederationSave(self, *args: object, **kwargs: object) -> object:
        self_name = 'abortFederationSave'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryFederationSaveStatus(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryFederationSaveStatus'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def requestFederationRestore(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestFederationRestore'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def federateRestoreComplete(self, *args: object, **kwargs: object) -> object:
        self_name = 'federateRestoreComplete'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def federateRestoreNotComplete(self, *args: object, **kwargs: object) -> object:
        self_name = 'federateRestoreNotComplete'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def abortFederationRestore(self, *args: object, **kwargs: object) -> object:
        self_name = 'abortFederationRestore'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryFederationRestoreStatus(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryFederationRestoreStatus'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def publishObjectClassAttributes(self, *args: object, **kwargs: object) -> object:
        self_name = 'publishObjectClassAttributes'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unpublishObjectClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'unpublishObjectClass'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unpublishObjectClassAttributes(self, *args: object, **kwargs: object) -> object:
        self_name = 'unpublishObjectClassAttributes'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def publishInteractionClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'publishInteractionClass'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unpublishInteractionClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'unpublishInteractionClass'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeObjectClassAttributes(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeObjectClassAttributes'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeObjectClassAttributesPassively(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeObjectClassAttributesPassively'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unsubscribeObjectClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'unsubscribeObjectClass'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unsubscribeObjectClassAttributes(self, *args: object, **kwargs: object) -> object:
        self_name = 'unsubscribeObjectClassAttributes'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeInteractionClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeInteractionClass'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeInteractionClassPassively(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeInteractionClassPassively'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unsubscribeInteractionClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'unsubscribeInteractionClass'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def reserveObjectInstanceName(self, *args: object, **kwargs: object) -> object:
        self_name = 'reserveObjectInstanceName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def releaseObjectInstanceName(self, *args: object, **kwargs: object) -> object:
        self_name = 'releaseObjectInstanceName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def reserveMultipleObjectInstanceName(self, *args: object, **kwargs: object) -> object:
        self_name = 'reserveMultipleObjectInstanceName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def releaseMultipleObjectInstanceName(self, *args: object, **kwargs: object) -> object:
        self_name = 'releaseMultipleObjectInstanceName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def registerObjectInstance(self, *args: object, **kwargs: object) -> object:
        self_name = 'registerObjectInstance'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def updateAttributeValues(self, *args: object, **kwargs: object) -> object:
        self_name = 'updateAttributeValues'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def sendInteraction(self, *args: object, **kwargs: object) -> object:
        self_name = 'sendInteraction'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def deleteObjectInstance(self, *args: object, **kwargs: object) -> object:
        self_name = 'deleteObjectInstance'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def localDeleteObjectInstance(self, *args: object, **kwargs: object) -> object:
        self_name = 'localDeleteObjectInstance'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def requestAttributeValueUpdate(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestAttributeValueUpdate'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def requestAttributeTransportationTypeChange(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestAttributeTransportationTypeChange'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryAttributeTransportationType(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryAttributeTransportationType'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def requestInteractionTransportationTypeChange(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestInteractionTransportationTypeChange'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryInteractionTransportationType(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryInteractionTransportationType'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unconditionalAttributeOwnershipDivestiture(self, *args: object, **kwargs: object) -> object:
        self_name = 'unconditionalAttributeOwnershipDivestiture'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def negotiatedAttributeOwnershipDivestiture(self, *args: object, **kwargs: object) -> object:
        self_name = 'negotiatedAttributeOwnershipDivestiture'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def confirmDivestiture(self, *args: object, **kwargs: object) -> object:
        self_name = 'confirmDivestiture'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def attributeOwnershipAcquisition(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeOwnershipAcquisition'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def attributeOwnershipAcquisitionIfAvailable(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeOwnershipAcquisitionIfAvailable'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def attributeOwnershipReleaseDenied(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeOwnershipReleaseDenied'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def attributeOwnershipDivestitureIfWanted(self, *args: object, **kwargs: object) -> object:
        self_name = 'attributeOwnershipDivestitureIfWanted'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def cancelNegotiatedAttributeOwnershipDivestiture(self, *args: object, **kwargs: object) -> object:
        self_name = 'cancelNegotiatedAttributeOwnershipDivestiture'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def cancelAttributeOwnershipAcquisition(self, *args: object, **kwargs: object) -> object:
        self_name = 'cancelAttributeOwnershipAcquisition'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryAttributeOwnership(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryAttributeOwnership'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def isAttributeOwnedByFederate(self, *args: object, **kwargs: object) -> object:
        self_name = 'isAttributeOwnedByFederate'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableTimeRegulation(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableTimeRegulation'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableTimeRegulation(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableTimeRegulation'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableTimeConstrained(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableTimeConstrained'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableTimeConstrained(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableTimeConstrained'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def timeAdvanceRequest(self, *args: object, **kwargs: object) -> object:
        self_name = 'timeAdvanceRequest'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def timeAdvanceRequestAvailable(self, *args: object, **kwargs: object) -> object:
        self_name = 'timeAdvanceRequestAvailable'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def nextMessageRequest(self, *args: object, **kwargs: object) -> object:
        self_name = 'nextMessageRequest'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def nextMessageRequestAvailable(self, *args: object, **kwargs: object) -> object:
        self_name = 'nextMessageRequestAvailable'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def flushQueueRequest(self, *args: object, **kwargs: object) -> object:
        self_name = 'flushQueueRequest'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableAsynchronousDelivery(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableAsynchronousDelivery'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableAsynchronousDelivery(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableAsynchronousDelivery'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryGALT(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryGALT'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryLogicalTime(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryLogicalTime'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryLITS(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryLITS'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def modifyLookahead(self, *args: object, **kwargs: object) -> object:
        self_name = 'modifyLookahead'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def queryLookahead(self, *args: object, **kwargs: object) -> object:
        self_name = 'queryLookahead'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def retract(self, *args: object, **kwargs: object) -> object:
        self_name = 'retract'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def changeAttributeOrderType(self, *args: object, **kwargs: object) -> object:
        self_name = 'changeAttributeOrderType'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def changeInteractionOrderType(self, *args: object, **kwargs: object) -> object:
        self_name = 'changeInteractionOrderType'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def createRegion(self, *args: object, **kwargs: object) -> object:
        self_name = 'createRegion'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def commitRegionModifications(self, *args: object, **kwargs: object) -> object:
        self_name = 'commitRegionModifications'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def deleteRegion(self, *args: object, **kwargs: object) -> object:
        self_name = 'deleteRegion'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def registerObjectInstanceWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'registerObjectInstanceWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def associateRegionsForUpdates(self, *args: object, **kwargs: object) -> object:
        self_name = 'associateRegionsForUpdates'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unassociateRegionsForUpdates(self, *args: object, **kwargs: object) -> object:
        self_name = 'unassociateRegionsForUpdates'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeObjectClassAttributesWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeObjectClassAttributesWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeObjectClassAttributesPassivelyWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeObjectClassAttributesPassivelyWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unsubscribeObjectClassAttributesWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'unsubscribeObjectClassAttributesWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeInteractionClassWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeInteractionClassWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def subscribeInteractionClassPassivelyWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'subscribeInteractionClassPassivelyWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def unsubscribeInteractionClassWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'unsubscribeInteractionClassWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def sendInteractionWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'sendInteractionWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def requestAttributeValueUpdateWithRegions(self, *args: object, **kwargs: object) -> object:
        self_name = 'requestAttributeValueUpdateWithRegions'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAutomaticResignDirective(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAutomaticResignDirective'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def setAutomaticResignDirective(self, *args: object, **kwargs: object) -> object:
        self_name = 'setAutomaticResignDirective'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getFederateHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getFederateHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getFederateName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getFederateName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getObjectClassHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getObjectClassHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getObjectClassName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getObjectClassName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getKnownObjectClassHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getKnownObjectClassHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getObjectInstanceHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getObjectInstanceHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getObjectInstanceName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getObjectInstanceName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAttributeHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAttributeHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAttributeName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAttributeName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getUpdateRateValue(self, *args: object, **kwargs: object) -> object:
        self_name = 'getUpdateRateValue'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getUpdateRateValueForAttribute(self, *args: object, **kwargs: object) -> object:
        self_name = 'getUpdateRateValueForAttribute'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getInteractionClassHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getInteractionClassHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getInteractionClassName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getInteractionClassName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getParameterHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getParameterHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getParameterName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getParameterName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getOrderType(self, *args: object, **kwargs: object) -> object:
        self_name = 'getOrderType'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getOrderName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getOrderName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getTransportationTypeHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getTransportationTypeHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getTransportationTypeName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getTransportationTypeName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAvailableDimensionsForClassAttribute(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAvailableDimensionsForClassAttribute'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAvailableDimensionsForInteractionClass(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAvailableDimensionsForInteractionClass'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getDimensionHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'getDimensionHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getDimensionName(self, *args: object, **kwargs: object) -> object:
        self_name = 'getDimensionName'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getDimensionUpperBound(self, *args: object, **kwargs: object) -> object:
        self_name = 'getDimensionUpperBound'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getDimensionHandleSet(self, *args: object, **kwargs: object) -> object:
        self_name = 'getDimensionHandleSet'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getRangeBounds(self, *args: object, **kwargs: object) -> object:
        self_name = 'getRangeBounds'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def setRangeBounds(self, *args: object, **kwargs: object) -> object:
        self_name = 'setRangeBounds'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def normalizeFederateHandle(self, *args: object, **kwargs: object) -> object:
        self_name = 'normalizeFederateHandle'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def normalizeServiceGroup(self, *args: object, **kwargs: object) -> object:
        self_name = 'normalizeServiceGroup'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableObjectClassRelevanceAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableObjectClassRelevanceAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableObjectClassRelevanceAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableObjectClassRelevanceAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableAttributeRelevanceAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableAttributeRelevanceAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableAttributeRelevanceAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableAttributeRelevanceAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableAttributeScopeAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableAttributeScopeAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableAttributeScopeAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableAttributeScopeAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableInteractionRelevanceAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableInteractionRelevanceAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableInteractionRelevanceAdvisorySwitch(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableInteractionRelevanceAdvisorySwitch'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def evokeCallback(self, *args: object, **kwargs: object) -> object:
        self_name = 'evokeCallback'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def evokeMultipleCallbacks(self, *args: object, **kwargs: object) -> object:
        self_name = 'evokeMultipleCallbacks'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def enableCallbacks(self, *args: object, **kwargs: object) -> object:
        self_name = 'enableCallbacks'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def disableCallbacks(self, *args: object, **kwargs: object) -> object:
        self_name = 'disableCallbacks'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAttributeHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAttributeHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAttributeHandleSetFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAttributeHandleSetFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAttributeHandleValueMapFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAttributeHandleValueMapFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getAttributeSetRegionSetPairListFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getAttributeSetRegionSetPairListFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getDimensionHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getDimensionHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getDimensionHandleSetFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getDimensionHandleSetFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getFederateHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getFederateHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getFederateHandleSetFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getFederateHandleSetFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getInteractionClassHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getInteractionClassHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getObjectClassHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getObjectClassHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getObjectInstanceHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getObjectInstanceHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getParameterHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getParameterHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getParameterHandleValueMapFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getParameterHandleValueMapFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getRegionHandleSetFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getRegionHandleSetFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getTransportationTypeHandleFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getTransportationTypeHandleFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getHLAversion(self, *args: object, **kwargs: object) -> object:
        self_name = 'getHLAversion'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )
    @abstractmethod
    def getTimeFactory(self, *args: object, **kwargs: object) -> object:
        self_name = 'getTimeFactory'
        raise NotImplementedError(
            f"IEEE 1516e provider has not implemented {type(self).__name__}.{self_name}"
        )

__all__ = [
    "STANDARD_EDITION",
    "JAVA_PACKAGE",
    "CPP_NAMESPACE",
    "MethodSpec",
    "RTIAMBASSADOR_METHODS",
    "FEDERATE_AMBASSADOR_METHODS",
    "RTIAMBASSADOR_OVERLOAD_COUNTS",
    "FEDERATE_AMBASSADOR_OVERLOAD_COUNTS",
    "RTIAMBASSADOR_PARAMETER_TYPES",
    "FEDERATE_AMBASSADOR_PARAMETER_TYPES",
    "RTIAMBASSADOR_RETURN_TYPES",
    "FEDERATE_AMBASSADOR_RETURN_TYPES",
    "FederateAmbassador",
    "NullFederateAmbassador",
    "RTIambassador",
]
