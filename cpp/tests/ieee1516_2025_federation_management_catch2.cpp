#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "internal/embedded_transport.hpp"
#include "internal/mom_service_report_encoding.hpp"
#include "internal/service_report_store.hpp"
#include "internal/umbra_rti_ambassador.hpp"
#include "internal/utf8_string.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The embedded federation-management tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederateHandle;
using rti1516_2025::FederateHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::ParameterHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::NO_ACTION;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RegionHandle;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::ResignAction;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::RestoreFailureReason;
using rti1516_2025::SaveFailureReason;
using rti1516_2025::SaveStatus;
using rti1516_2025::ServiceGroup;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

using TestFederateAmbassador = NullFederateAmbassador;

class FederationEventFederateAmbassador final : public NullFederateAmbassador {
 public:
  void connectionLost(std::wstring const& faultDescription) override {
    faultDescriptions.push_back(faultDescription);
  }

  void federateResigned(std::wstring const& reasonForResignDescription) override {
    resignationDescriptions.push_back(reasonForResignDescription);
  }

  std::vector<std::wstring> faultDescriptions;
  std::vector<std::wstring> resignationDescriptions;
};

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct MemberReport {
    std::wstring federationName;
    rti1516_2025::FederationExecutionMemberInformationVector members;
  };

  struct TimeAdvanceGrantReport {
    std::wstring implementationName;
    std::wstring value;
  };

  struct InteractionReport {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct TimestampedInteractionReport {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct RequestRetractionReport {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  struct DirectedInteractionReport {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct ObjectDiscoveryReport {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ObjectRemovalReport {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct AttributeReflectionReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct AttributeTransportationTypeChangeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    TransportationTypeHandle transportationType;
  };

  struct AttributeTransportationTypeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandle attribute;
    TransportationTypeHandle transportationType;
  };

  struct InteractionTransportationTypeChangeReport {
    InteractionClassHandle interactionClass;
    TransportationTypeHandle transportationType;
  };

  struct InteractionTransportationTypeReport {
    FederateHandle federate;
    InteractionClassHandle interactionClass;
    TransportationTypeHandle transportationType;
  };

  struct AttributeScopeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct AttributeRelevanceReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct AttributeRelevanceRateReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    std::wstring updateRateDesignator;
  };

  struct ObjectClassRelevanceReport {
    ObjectClassHandle objectClass;
  };

  struct InteractionRelevanceReport {
    InteractionClassHandle interactionClass;
  };

  struct AttributeValueUpdateRequestReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipReport {
    enum class Kind {
      federate,
      unowned,
      rti,
    };

    Kind kind = Kind::unowned;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    FederateHandle owner;
  };

  struct AttributeOwnershipAcquisitionReport {
    enum class Kind {
      notification,
      unavailable,
    };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipReleaseRequestReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionCancellationReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct FlushQueueGrantReport {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  struct SynchronizationPointRegistrationReport {
    std::wstring label;
    bool succeeded = false;
    rti1516_2025::SynchronizationPointFailureReason failureReason =
        rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE;
  };

  struct SynchronizationPointAnnouncementReport {
    std::wstring label;
    VariableLengthData userSuppliedTag;
  };

  struct FederationSynchronizedReport {
    std::wstring label;
    FederateHandleSet failedToSyncSet;
  };

  struct FederationSaveStatusReport {
    rti1516_2025::FederateHandleSaveStatusPairVector statuses;
  };

  struct InitiateFederateRestoreReport {
    std::wstring label;
    std::wstring federateName;
    FederateHandle postRestoreFederateHandle;
  };

  struct FederationRestoreStatusReport {
    rti1516_2025::FederateRestoreStatusVector statuses;
  };

  struct AttributeOwnershipAssumptionReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct DivestitureConfirmationReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct ObjectInstanceNameReservationReport {
    std::wstring objectInstanceName;
  };

  struct MultipleObjectInstanceNameReservationReport {
    std::set<std::wstring> objectInstanceNames;
  };

  void connectionLost(std::wstring const& faultDescription) override {
    faultDescriptions.push_back(faultDescription);
  }

  void reportFederationExecutions(
      rti1516_2025::FederationExecutionInformationVector const& report) override {
    federationExecutionReports.push_back(report);
  }

  void reportFederationExecutionMembers(
      std::wstring const& federationName,
      rti1516_2025::FederationExecutionMemberInformationVector const& report) override {
    federationExecutionMemberReports.push_back({federationName, report});
  }

  void reportFederationExecutionDoesNotExist(std::wstring const& federationName) override {
    missingFederationReports.push_back(federationName);
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  void synchronizationPointRegistrationSucceeded(
      std::wstring const& label) override {
    synchronizationPointRegistrationReports.push_back({label, true});
    callbackOrder.push_back("sync-registration-succeeded");
  }

  void synchronizationPointRegistrationFailed(
      std::wstring const& label,
      rti1516_2025::SynchronizationPointFailureReason reason) override {
    synchronizationPointRegistrationReports.push_back({label, false, reason});
    callbackOrder.push_back("sync-registration-failed");
  }

  void announceSynchronizationPoint(
      std::wstring const& label,
      VariableLengthData const& userSuppliedTag) override {
    synchronizationPointAnnouncementReports.push_back({label, userSuppliedTag});
    callbackOrder.push_back("sync-announce");
  }

  void federationSynchronized(
      std::wstring const& label,
      FederateHandleSet const& failedToSyncSet) override {
    federationSynchronizedReports.push_back({label, failedToSyncSet});
    callbackOrder.push_back("sync-complete");
  }

  void initiateFederateSave(std::wstring const& label) override {
    initiateFederateSaveReports.push_back(label);
    callbackOrder.push_back("save-initiate");
    if (onInitiateFederateSave) {
      onInitiateFederateSave();
    }
  }

  void federationSaved() override {
    ++federationSavedReportCount;
    callbackOrder.push_back("save-complete");
  }

  void federationNotSaved(SaveFailureReason reason) override {
    federationNotSavedReasons.push_back(reason);
    callbackOrder.push_back("save-failed");
  }

  void federationSaveStatusResponse(
      rti1516_2025::FederateHandleSaveStatusPairVector const& response) override {
    federationSaveStatusReports.push_back({response});
    callbackOrder.push_back("save-status");
  }

  void requestFederationRestoreSucceeded(std::wstring const& label) override {
    requestFederationRestoreSucceededReports.push_back(label);
    callbackOrder.push_back("restore-request-succeeded");
  }

  void requestFederationRestoreFailed(std::wstring const& label) override {
    requestFederationRestoreFailedReports.push_back(label);
    callbackOrder.push_back("restore-request-failed");
  }

  void federationRestoreBegun() override {
    ++federationRestoreBegunReportCount;
    callbackOrder.push_back("restore-begun");
  }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const& federateName,
      FederateHandle const& postRestoreFederateHandle) override {
    initiateFederateRestoreReports.push_back({
        label,
        federateName,
        postRestoreFederateHandle,
    });
    callbackOrder.push_back("restore-initiate");
  }

  void federationRestored() override {
    ++federationRestoredReportCount;
    callbackOrder.push_back("restore-complete");
  }

  void federationNotRestored(RestoreFailureReason reason) override {
    federationNotRestoredReasons.push_back(reason);
    callbackOrder.push_back("restore-failed");
  }

  void federationRestoreStatusResponse(
      rti1516_2025::FederateRestoreStatusVector const& response) override {
    federationRestoreStatusReports.push_back({response});
    callbackOrder.push_back("restore-status");
  }

  void timeRegulationEnabled(rti1516_2025::LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    objectInstanceNameReservationSucceededReports.push_back({objectInstanceName});
  }

  void objectInstanceNameReservationFailed(
      std::wstring const& objectInstanceName) override {
    objectInstanceNameReservationFailedReports.push_back({objectInstanceName});
  }

  void multipleObjectInstanceNameReservationSucceeded(
      std::set<std::wstring> const& objectInstanceNames) override {
    multipleObjectInstanceNameReservationSucceededReports.push_back({objectInstanceNames});
  }

  void multipleObjectInstanceNameReservationFailed(
      std::set<std::wstring> const& objectInstanceNames) override {
    multipleObjectInstanceNameReservationFailedReports.push_back({objectInstanceNames});
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
        optionalSentRegions != nullptr,
        optionalSentRegions != nullptr ? *optionalSentRegions : RegionHandleSet{},
    });
    callbackOrder.push_back("interaction");
    if (onTimestampedInteraction) {
      onTimestampedInteraction();
    }
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({
        retraction.isValid(),
        retraction.encode(),
    });
    callbackOrder.push_back("request-retraction");
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("directed");
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
    });
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void attributesInScope(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributesInScopeReports.push_back({objectInstance, attributes});
  }

  void attributesOutOfScope(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributesOutOfScopeReports.push_back({objectInstance, attributes});
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOnForObjectInstanceReports.push_back({objectInstance, attributes});
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      std::wstring const& updateRateDesignator) override {
    turnUpdatesOnForObjectInstanceRateReports.push_back({
        objectInstance,
        attributes,
        updateRateDesignator,
    });
  }

  void turnUpdatesOffForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOffForObjectInstanceReports.push_back({objectInstance, attributes});
  }

  void startRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    startRegistrationForObjectClassReports.push_back({objectClass});
  }

  void stopRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    stopRegistrationForObjectClassReports.push_back({objectClass});
  }

  void turnInteractionsOn(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOnReports.push_back({interactionClass});
  }

  void turnInteractionsOff(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOffReports.push_back({interactionClass});
  }

  void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      TransportationTypeHandle const& transportationType) override {
    attributeTransportationTypeChangeReports.push_back({
        objectInstance,
        attributes,
        transportationType,
    });
  }

  void reportAttributeTransportationType(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute,
      TransportationTypeHandle const& transportationType) override {
    attributeTransportationTypeReports.push_back({
        objectInstance,
        attribute,
        transportationType,
    });
  }

  void confirmInteractionTransportationTypeChange(
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    interactionTransportationTypeChangeReports.push_back({
        interactionClass,
        transportationType,
    });
  }

  void reportInteractionTransportationType(
      FederateHandle const& federate,
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    interactionTransportationTypeReports.push_back({
        federate,
        interactionClass,
        transportationType,
    });
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeValueUpdateRequestReports.push_back({
        objectInstance,
        attributes,
        userSuppliedTag,
    });
    if (provideAttributeValueUpdateHandler) {
      provideAttributeValueUpdateHandler(objectInstance, attributes, userSuppliedTag);
    }
  }

  void informAttributeOwnership(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      FederateHandle const& owner) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::federate,
        objectInstance,
        attributes,
        owner,
    });
  }

  void attributeIsNotOwned(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::unowned,
        objectInstance,
        attributes,
        FederateHandle(),
    });
  }

  void attributeIsOwnedByRTI(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::rti,
        objectInstance,
        attributes,
        FederateHandle(),
    });
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipUnavailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::unavailable,
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipReleaseRequestReports.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
    });
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipAcquisitionCancellationReports.push_back({
        objectInstance,
        attributes,
    });
  }

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAssumptionReports.push_back({
        objectInstance,
        offeredAttributes,
        userSuppliedTag,
    });
  }

  void requestDivestitureConfirmation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& releasedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    divestitureConfirmationReports.push_back({
        objectInstance,
        releasedAttributes,
        userSuppliedTag,
    });
  }

  std::vector<rti1516_2025::FederationExecutionInformationVector> federationExecutionReports;
  std::vector<MemberReport> federationExecutionMemberReports;
  std::vector<std::wstring> missingFederationReports;
  std::vector<std::wstring> faultDescriptions;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
  std::vector<SynchronizationPointRegistrationReport>
      synchronizationPointRegistrationReports;
  std::vector<SynchronizationPointAnnouncementReport>
      synchronizationPointAnnouncementReports;
  std::vector<FederationSynchronizedReport> federationSynchronizedReports;
  std::vector<std::wstring> initiateFederateSaveReports;
  std::function<void()> onInitiateFederateSave;
  std::size_t federationSavedReportCount = 0;
  std::vector<SaveFailureReason> federationNotSavedReasons;
  std::vector<FederationSaveStatusReport> federationSaveStatusReports;
  std::vector<std::wstring> requestFederationRestoreSucceededReports;
  std::vector<std::wstring> requestFederationRestoreFailedReports;
  std::size_t federationRestoreBegunReportCount = 0;
  std::vector<InitiateFederateRestoreReport> initiateFederateRestoreReports;
  std::size_t federationRestoredReportCount = 0;
  std::vector<RestoreFailureReason> federationNotRestoredReasons;
  std::vector<FederationRestoreStatusReport> federationRestoreStatusReports;
  std::vector<TimeAdvanceGrantReport> timeRegulationEnabledReports;
  std::vector<TimeAdvanceGrantReport> timeConstrainedEnabledReports;
  std::vector<std::string> callbackOrder;
  std::vector<ObjectInstanceNameReservationReport>
      objectInstanceNameReservationSucceededReports;
  std::vector<ObjectInstanceNameReservationReport>
      objectInstanceNameReservationFailedReports;
  std::vector<MultipleObjectInstanceNameReservationReport>
      multipleObjectInstanceNameReservationSucceededReports;
  std::vector<MultipleObjectInstanceNameReservationReport>
      multipleObjectInstanceNameReservationFailedReports;
  std::vector<InteractionReport> interactionReports;
  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::function<void()> onTimestampedInteraction;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<AttributeTransportationTypeChangeReport>
      attributeTransportationTypeChangeReports;
  std::vector<AttributeTransportationTypeReport> attributeTransportationTypeReports;
  std::vector<InteractionTransportationTypeChangeReport>
      interactionTransportationTypeChangeReports;
  std::vector<InteractionTransportationTypeReport> interactionTransportationTypeReports;
  std::vector<AttributeScopeReport> attributesInScopeReports;
  std::vector<AttributeScopeReport> attributesOutOfScopeReports;
  std::vector<AttributeRelevanceReport> turnUpdatesOnForObjectInstanceReports;
  std::vector<AttributeRelevanceRateReport> turnUpdatesOnForObjectInstanceRateReports;
  std::vector<AttributeRelevanceReport> turnUpdatesOffForObjectInstanceReports;
  std::vector<ObjectClassRelevanceReport> startRegistrationForObjectClassReports;
  std::vector<ObjectClassRelevanceReport> stopRegistrationForObjectClassReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOnReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOffReports;
  std::vector<AttributeValueUpdateRequestReport> attributeValueUpdateRequestReports;
  std::function<void(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&,
      VariableLengthData const&)>
      provideAttributeValueUpdateHandler;
  std::vector<AttributeOwnershipReport> attributeOwnershipReports;
  std::vector<AttributeOwnershipAcquisitionReport> attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipReleaseRequestReport> attributeOwnershipReleaseRequestReports;
  std::vector<AttributeOwnershipAcquisitionCancellationReport>
      attributeOwnershipAcquisitionCancellationReports;
  std::vector<AttributeOwnershipAssumptionReport> attributeOwnershipAssumptionReports;
  std::vector<DivestitureConfirmationReport> divestitureConfirmationReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
         "third_party" /
         "ieee1516.2-2025" /
         "resources" /
         relativePath;
}

class ScopedTemporaryFile final {
 public:
  explicit ScopedTemporaryFile(std::filesystem::path path) : path_(std::move(path)) {}

  ScopedTemporaryFile(ScopedTemporaryFile const&) = delete;
  ScopedTemporaryFile& operator=(ScopedTemporaryFile const&) = delete;

  ~ScopedTemporaryFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

class ScopedTemporaryDirectory final {
 public:
  explicit ScopedTemporaryDirectory(std::filesystem::path path) : path_(std::move(path)) {}

  ScopedTemporaryDirectory(ScopedTemporaryDirectory const&) = delete;
  ScopedTemporaryDirectory& operator=(ScopedTemporaryDirectory const&) = delete;

  ~ScopedTemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

ScopedTemporaryFile nrgEnabledRestaurantModule() {
  static std::atomic_uint64_t counter{0};
  std::ifstream input(resourcePath("examples/RestaurantFOMmodule-2025.xml"), std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  std::string const marker = "</switches>";
  auto const switchesPosition = fomText.find(marker);
  REQUIRE(switchesPosition != std::string::npos);
  fomText.insert(
      switchesPosition,
      "        <nonRegulatedGrant isEnabled=\"true\"/>\n    ");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-nrg-scheduler-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-catch2-federation-" + std::to_wstring(++counter);
}

ScopedTemporaryDirectory temporaryServiceReportDirectory() {
  static std::atomic_uint64_t counter{0};
  return ScopedTemporaryDirectory(
      std::filesystem::temp_directory_path() /
      ("umbra-service-report-lifecycle-" + std::to_string(++counter)));
}

[[nodiscard]] RtiConfiguration configurationForServiceReportDirectory(
    std::filesystem::path const& directory) {
  return umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
}

class FailingServiceReportStore final : public umbra::detail::ServiceReportStore {
 public:
  [[nodiscard]] std::unique_ptr<umbra::detail::ServiceReportWriter> createForJoinedFederate(
      umbra::detail::JoinedFederateReportDescriptor const& descriptor) override {
    static_cast<void>(descriptor);
    ++createCalls;
    throw std::runtime_error("intentional service-report store failure");
  }

  std::size_t createCalls = 0U;
};

std::vector<std::filesystem::path> serviceReportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> files;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

std::string readTextFile(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
}

std::vector<unsigned char> variableLengthDataBytes(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::optional<std::vector<std::wstring>> decodeHlaUnicodeStringList(
    VariableLengthData const& encoded) {
  try {
    auto const rawBytes = variableLengthDataBytes(encoded);
    std::vector<rti1516_2025::Octet> bytes(rawBytes.begin(), rawBytes.end());
    rti1516_2025::HLAinteger32BE count;
    auto index = count.decodeFrom(bytes, 0U);
    auto const elementCount = count.get();
    if (elementCount < 0) {
      return std::nullopt;
    }
    std::vector<std::wstring> result;
    result.reserve(static_cast<std::size_t>(elementCount));
    for (rti1516_2025::Integer32 element = 0; element < elementCount; ++element) {
      auto const remainder = index % 4U;
      if (remainder != 0U) {
        index += 4U - remainder;
      }
      rti1516_2025::HLAunicodeString designator;
      index = designator.decodeFrom(bytes, index);
      result.push_back(designator.get());
    }
    if (index != bytes.size()) {
      return std::nullopt;
    }
    return result;
  } catch (rti1516_2025::EncoderException const&) {
    return std::nullopt;
  }
}

}  // namespace

TEST_CASE(
    "Embedded asynchronous delivery gates receive-order callbacks by temporal state",
    "[integration][development-profile][time-management][asynchronous-delivery]"
    "[rti.service.enable-asynchronous-delivery][rti.service.disable-asynchronous-delivery]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_THROWS_AS(
      receiver->enableAsynchronousDelivery(),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      receiver->disableAsynchronousDelivery(),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      receiver->enableAsynchronousDelivery(),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      receiver->disableAsynchronousDelivery(),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      publisher->createFederationExecution(
          federationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"async-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"async-receiver",
      L"subscriber",
      federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);

  // The default switch is disabled. A receive-order message submitted while
  // the constrained federate is Time Granted is retained, not discarded.
  REQUIRE_NOTHROW(
      publisher->sendInteraction(interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.interactionReports.empty());

  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_THROWS_AS(
      receiver->enableAsynchronousDelivery(),
      rti1516_2025::AsynchronousDeliveryAlreadyEnabled);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.interactionReports.size() == 1);

  REQUIRE_NOTHROW(receiver->disableAsynchronousDelivery());
  REQUIRE_THROWS_AS(
      receiver->disableAsynchronousDelivery(),
      rti1516_2025::AsynchronousDeliveryAlreadyDisabled);

  REQUIRE_NOTHROW(
      publisher->sendInteraction(interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.interactionReports.size() == 1);

  // Entering Time Advancing makes the retained RO message eligible. It is
  // queued before any matching grant, so the next evoke observes the
  // interaction even though this isolated constrained federate has no
  // regulator available to release its TAR.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.interactionReports.size() == 2);
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary receive-order interaction eligibility",
    "[integration][development-profile][interaction-management][delay-subscription-evaluation]"
    "[rti.service.send-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const interactionFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                 "cpp" / "tests" / "data" /
                                 "parameter-handle-provider-fom.xml")
                                    .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{interactionFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-ro-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-ro-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
    REQUIRE(interactionClass.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

    // The receiver is joined but has no subscription when the RO message is
    // generated.  Only the Enabled federation keeps it as a candidate until
    // the HLA_EVOKED callback boundary.
    REQUIRE_NOTHROW(publisher->sendInteraction(
        interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    static_cast<void>(receiver->evokeCallback(0.0));
    REQUIRE(receiverReports.interactionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    if (delaySubscriptionEvaluationEnabled) {
      REQUIRE(receiverReports.interactionReports.front().interactionClass == interactionClass);
    }

    // Clause 8.1.8 requires an actual-delivery decision from the receiver's
    // current subscriptions in both modes.  An accepted recipient that
    // unsubscribes before its HLA_EVOKED boundary must therefore be suppressed.
    REQUIRE_NOTHROW(publisher->sendInteraction(
        interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
    static_cast<void>(receiver->evokeCallback(0.0));
    REQUIRE(receiverReports.interactionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled") {
    runScenario(true);
  }
  SECTION("the omitted switch uses the Disabled default") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary timestamped interaction eligibility",
    "[integration][development-profile][interaction-management][time-management]"
    "[delay-subscription-evaluation][tso]"
    "[rti.service.send-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const interactionFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                 "cpp" / "tests" / "data" /
                                 "parameter-handle-provider-fom.xml")
                                    .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{interactionFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-tso-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-tso-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
    REQUIRE(interactionClass.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    static_cast<void>(receiver->evokeCallback(0.0));
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    static_cast<void>(publisher->evokeCallback(0.0));

    // The timestamped message is generated while the receiver is unsubscribed.
    // In the Enabled case it waits in the federation-owned TSO queue and is
    // projected only when the recipient becomes eligible at its grant.
    auto const firstRetraction = publisher->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{},
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    static_cast<void>(publisher->evokeCallback(0.0));
    static_cast<void>(receiver->evokeCallback(0.0));
    REQUIRE(receiverReports.timestampedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& report = receiverReports.timestampedInteractionReports.front();
      REQUIRE(report.interactionClass == interactionClass);
      REQUIRE(report.timeValue == L"2");
      REQUIRE(report.sentOrderType == TIMESTAMP);
      REQUIRE(report.receivedOrderType == TIMESTAMP);
    }

    // A subscription existing at generation is not a promise of delivery:
    // current state at the second grant suppresses this otherwise queued TSO
    // interaction under both switch settings.
    auto const secondRetraction = publisher->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{},
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    static_cast<void>(publisher->evokeCallback(0.0));
    static_cast<void>(receiver->evokeCallback(0.0));
    REQUIRE(receiverReports.timestampedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled") {
    runScenario(true);
  }
  SECTION("the omitted switch uses the Disabled default") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary receive-order attribute-update eligibility",
    "[integration][development-profile][object-management][delay-subscription-evaluation]"
    "[rti.service.update-attribute-values]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.evoke-callback]"
    "[federate.callback.reflect-attribute-values]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const attributeFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "attribute-update-passel-fom.xml")
                                  .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{attributeFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-ro-attribute-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-ro-attribute-receiver", L"subscriber", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
    auto const attribute = publisher->getAttributeHandle(objectClass, L"ReliableBaseA");
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    AttributeHandleSet const attributes{attribute};
    unsigned char const valueBytes[] = {0xD5, 0x45};
    AttributeHandleValueMap values;
    values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));

    // The receiver first obtains a durable known-object record, then drops
    // the attribute declaration. This isolates delayed subscription
    // evaluation from discovery semantics.
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
    REQUIRE(receiver->getKnownObjectClassHandle(objectInstance) == objectClass);
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));

    // The receiver has no qualifying attribute subscription at generation.
    // Only the Enabled federation keeps a route until its HLA_EVOKED delivery
    // fence and can therefore deliver after the late re-subscription.
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance, values, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    if (delaySubscriptionEvaluationEnabled) {
      auto const& reflection = receiverReports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1);
      REQUIRE(reflection.attributeValues.contains(attribute));
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(attribute)) ==
              std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
      REQUIRE_FALSE(reflection.sentRegionsSupplied);
    }

    // A recipient accepted while subscribed is still re-evaluated at the
    // callback fence. Removing that current declaration suppresses a second
    // ordinary reflection in both switch settings.
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance, values, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled") {
    runScenario(true);
  }
  SECTION("the omitted switch uses the Disabled default") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary timestamped attribute-update eligibility",
    "[integration][development-profile][object-management][time-management]"
    "[delay-subscription-evaluation][tso]"
    "[rti.service.update-attribute-values]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const attributeFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "attribute-update-passel-fom.xml")
                                  .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{attributeFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-tso-attribute-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-tso-attribute-receiver", L"subscriber", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
    auto const attribute = publisher->getAttributeHandle(objectClass, L"ReliableBaseA");
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    AttributeHandleSet const attributes{attribute};
    unsigned char const valueBytes[] = {0xD5, 0x54};
    AttributeHandleValueMap values;
    values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));

    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(
        publisher->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    static_cast<void>(receiver->evokeCallback(0.0));
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    while (publisher->evokeCallback(0.0)) {
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1);

    // A TSO passel generated while unsubscribed remains associated with the
    // joined receiver only under the Enabled switch. The actual projection is
    // evaluated immediately before the callback at the recipient's grant.
    auto const firstRetraction = publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
    rti1516_2025::HLAinteger64Time publisherTime;
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherTime));
    REQUIRE(publisherTime.getTime() == 2);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& reflection = receiverReports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1);
      REQUIRE(reflection.attributeValues.contains(attribute));
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(attribute)) ==
              std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
      REQUIRE(reflection.timeValue == L"2");
      REQUIRE(reflection.sentOrderType == TIMESTAMP);
      REQUIRE(reflection.receivedOrderType == TIMESTAMP);
    }

    // Generation-time eligibility never promises a TSO reflection. The
    // second passel is accepted while subscribed, then suppressed after the
    // declaration is removed before the next time grant in both settings.
    auto const secondRetraction = publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled") {
    runScenario(true);
  }
  SECTION("the omitted switch uses the Disabled default") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded Request Retraction notifies delivered interaction recipients and suppresses queued fanout",
    "[integration][development-profile][interaction-management][time-management]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x52, 0x54, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"retraction-constrained", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(constrained->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The nonconstrained recipient sees the original timestamped interaction
  // immediately; the constrained recipient remains in the federation-owned
  // TSO queue until a later grant.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 1);
  REQUIRE(immediateReports.timestampedInteractionReports.front().retractionValid);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"interaction", "request-retraction"});

  // The still-pending fanout is removed before it can cross the recipient's
  // grant boundary. Advancing the regulator past its initial position releases
  // the recipient's TAR to 2 without a stale Receive Interaction callback.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  // Once the constrained recipient resigns, the next timestamped interaction
  // has no temporal-queue fanout at all. It must still retain a federation
  // ledger record so its already-delivered nonconstrained recipient can receive
  // Request Retraction.
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));

  auto const immediateOnlyRetraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(immediateOnlyRetraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 2);
  REQUIRE(immediateReports.timestampedInteractionReports.back().retractionValid);

  REQUIRE_NOTHROW(publisher->retract(immediateOnlyRetraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 2);
  REQUIRE(immediateReports.requestRetractionReports.back().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.back().encodedRetraction) ==
          variableLengthDataBytes(immediateOnlyRetraction.encode()));

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation-management services require an RTI connection",
    "[integration][federation-management][connection]") {
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->destroyFederationExecution(federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->joinFederationExecution(L"observer", federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->resignFederationExecution(NO_ACTION),
      rti1516_2025::NotConnected);
}

TEST_CASE(
    "Embedded transport loss forces the official connection-lost transition",
    "[integration][development-profile][federation-management][transport]"
    "[rti.service.connection-lost][federate.callback.connection-lost]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"transport-lost",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"transport-survivor",
      L"observer",
      federationName));

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"embedded loopback transport closed"));
  REQUIRE_FALSE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"duplicate fault"));

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"embedded loopback transport closed"});
  REQUIRE(lostReports.resignationDescriptions.empty());
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      lost->joinFederationExecution(L"observer", federationName),
      rti1516_2025::NotConnected);

  // The forced membership cleanup is observable from a surviving federate:
  // the lost endpoint no longer appears in the execution member report.
  REQUIRE_NOTHROW(surviving->listFederationExecutionMembers(federationName));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.federationExecutionMemberReports.size() == 1);
  REQUIRE(survivingReports.federationExecutionMemberReports.front().members.size() == 1);
  REQUIRE(
      survivingReports.federationExecutionMemberReports.front().members.front().federateName ==
      L"transport-survivor");

  // The forced transition returns the RTI ambassador to the ordinary
  // disconnected lifecycle, so the same object can establish a fresh
  // official connection after the fault callback has been evoked.
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic delete resign directive",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.remove-object-instance]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-delete-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = lost->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.empty());

  // Do not rely on the current FDD default: verify that the per-federate
  // directive selected through the official support service controls the
  // forced-resignation disposition.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"automatic delete transport fault"});
  REQUIRE(lostReports.resignationDescriptions.empty());
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  auto const& removal = survivingReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == lostFederate);
  // Connection Lost performs a resignation on behalf of the lost federate.
  // The returned identity remains a valid designator even though the member
  // report below no longer lists it as joined.
  REQUIRE(surviving->getFederateName(lostFederate) == L"automatic-delete-lost");
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(surviving->listFederationExecutionMembers(federationName));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.federationExecutionMemberReports.size() == 1);
  REQUIRE(survivingReports.federationExecutionMemberReports.front().members.size() == 1);
  REQUIRE(
      survivingReports.federationExecutionMemberReports.front().members.front().federateName ==
      L"automatic-delete-survivor");

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic unconditional-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-divest-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-divest-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = lost->getAttributeHandle(server, L"Efficiency");
  auto const privilegeToDelete = lost->getAttributeHandle(
      server,
      L"HLAprivilegeToDeleteObject");
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);

  // IEEE 1516.1-2025 §4.1.5 requires loss cleanup to use the member's
  // Automatic Resign Directive. Unlike the delete branch above, directive 1
  // retains the object while unconditionally divesting its owned attributes.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic unconditional-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic unconditional-divest transport fault"});
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == expectedAssumption);
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(surviving->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic delete-then-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-delete-then-divest-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-then-divest-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = lost->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));

  // The survivor owns this first object and deliberately transfers only its
  // Efficiency attribute to the federate that will be lost. It stays alive
  // after directive 4 because the lost federate never owns its delete
  // privilege.
  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = surviving->registerObjectInstance(server));
  auto const retainedObjectName = surviving->getObjectInstanceName(retainedObject);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  unsigned char const manualDivestitureTagBytes[] = {0xD4, 0x25};
  VariableLengthData const manualDivestitureTag(
      manualDivestitureTagBytes,
      sizeof(manualDivestitureTagBytes));
  REQUIRE_NOTHROW(surviving->unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      efficiencyOnly,
      manualDivestitureTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().objectInstance == retainedObject);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().attributes == efficiencyOnly);

  unsigned char const acquisitionTagBytes[] = {0xD5, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      efficiencyOnly,
      acquisitionTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(lost->isAttributeOwnedByFederate(retainedObject, efficiency));

  // The lost federate owns delete privilege for this separately registered
  // object. Directive 4 must delete it before divesting the transferred
  // attribute on retainedObject.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance == deletedObject);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete-then-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic delete-then-divest transport fault"});
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance == deletedObject);
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE(surviving->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(retainedObject, efficiency));

  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss cancels the lost federate's pending ownership acquisition",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto lost = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"automatic-cancel-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-cancel-lost",
      L"candidate",
      federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"automatic-cancel-survivor",
      L"candidate",
      federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  while (lost->evokeCallback(0.0)) {
  }
  while (candidate->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1);

  unsigned char const acquisitionTagBytes[] = {0xD6, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic pending-acquisition cancellation transport fault"));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic pending-acquisition cancellation transport fault"});

  // The old requester is gone before the owner can receive its queued release
  // request. Delivery must recheck registry state and suppress that stale
  // work, then the actual surviving candidate becomes eligible for a later
  // unconditional divestiture offer.
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  unsigned char const divestitureTagBytes[] = {0xD7, 0x25};
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      efficiencyOnly,
      divestitureTag));
  while (candidate->evokeCallback(0.0)) {
  }
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = candidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE_FALSE(candidate->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic cancel-then-delete-then-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.connection-lost]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-combined-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-combined-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = lost->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));

  // The survivor transfers one non-delete attribute to the federate that will
  // be lost. Directive 5 must later offer it back only after cancellation and
  // deletion have been handled.
  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = surviving->registerObjectInstance(server));
  auto const retainedObjectName = surviving->getObjectInstanceName(retainedObject);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  unsigned char const divestitureTagBytes[] = {0xD8, 0x25};
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(surviving->unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      efficiencyOnly,
      divestitureTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.size() == 1);
  unsigned char const retainedAcquisitionTagBytes[] = {0xD9, 0x25};
  VariableLengthData const retainedAcquisitionTag(
      retainedAcquisitionTagBytes,
      sizeof(retainedAcquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      efficiencyOnly,
      retainedAcquisitionTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(lost->isAttributeOwnedByFederate(retainedObject, efficiency));

  // The lost federate owns delete privilege for this final object, so the
  // deletion phase has an observable Remove Object Instance callback.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance == deletedObject);

  // The pending regular acquisition queues an owner-release callback at the
  // survivor. Directive 5's cancellation phase must make that queued work
  // stale before delivery, rather than leave it aimed at the departed member.
  ObjectInstanceHandle acquisitionTarget;
  REQUIRE_NOTHROW(acquisitionTarget = surviving->registerObjectInstance(server));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 2);
  unsigned char const pendingAcquisitionTagBytes[] = {0xDA, 0x25};
  VariableLengthData const pendingAcquisitionTag(
      pendingAcquisitionTagBytes,
      sizeof(pendingAcquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      acquisitionTarget,
      efficiencyOnly,
      pendingAcquisitionTag));
  REQUIRE(survivingReports.attributeOwnershipReleaseRequestReports.empty());

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic cancel-delete-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic cancel-delete-divest transport fault"});
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance == deletedObject);
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE(surviving->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(retainedObject, efficiency));

  REQUIRE_NOTHROW(
      surviving->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded RTI control forces the official federate-resigned transition",
    "[integration][development-profile][federation-management][transport]"
    "[federate.callback.federate-resigned]") {
  FederationEventFederateAmbassador forcedReports;
  FederationEventFederateAmbassador selfResignedReports;
  auto forced = makeRti();
  auto selfResigned = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(forced->connect(forcedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(selfResigned->connect(selfResignedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(forced->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(forced->joinFederationExecution(
      L"forced-resignation-target",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(selfResigned->joinFederationExecution(
      L"self-resignation-control",
      L"observer",
      federationName));

  // A federate-initiated resignation must not produce Federate Resigned at
  // that same federate. The RTI-originated control path below is distinct.
  REQUIRE_NOTHROW(selfResigned->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(selfResigned->evokeCallback(0.0));
  REQUIRE(selfResignedReports.resignationDescriptions.empty());

  REQUIRE(umbra::detail::forceEmbeddedFederateResignationForTesting(
      *forced,
      L"embedded RTI membership control removed this federate"));
  REQUIRE_FALSE(umbra::detail::forceEmbeddedFederateResignationForTesting(
      *forced,
      L"duplicate RTI membership control"));
  REQUIRE(forcedReports.resignationDescriptions.empty());
  REQUIRE(forcedReports.faultDescriptions.empty());

  REQUIRE_FALSE(forced->evokeCallback(0.0));
  REQUIRE(forcedReports.resignationDescriptions == std::vector<std::wstring>{
      L"embedded RTI membership control removed this federate"});
  REQUIRE(forcedReports.faultDescriptions.empty());

  // Federate Resigned leaves the connection alive but ends membership, so the
  // same RTI ambassador can immediately join the execution again.
  REQUIRE_NOTHROW(forced->joinFederationExecution(
      L"forced-resignation-rejoined",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(forced->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(forced->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(forced->disconnect());
  REQUIRE_NOTHROW(selfResigned->disconnect());
}

TEST_CASE(
    "Embedded federation-management services commit only a prevalidated federation definition",
    "[integration][development-profile][federation-management][rti.service.create-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.join-federation-execution]"
    "[rti.service.resign-federation-execution][m16.transition.federate-resign]") {
  TestFederateAmbassador creatorFederate;
  TestFederateAmbassador joiningFederate;
  TestFederateAmbassador duplicateNameFederate;
  auto creator = makeRti();
  auto joiner = makeRti();
  auto duplicateName = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(creator->connect(creatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(joiner->connect(joiningFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(duplicateName->connect(duplicateNameFederate, HLA_EVOKED));

  SECTION("Create rejects unresolved logical-time or standard-MIM designators") {
    REQUIRE_THROWS_AS(
        creator->createFederationExecution(federationName, fomModule),
        rti1516_2025::InconsistentFOM);
    REQUIRE_THROWS_AS(
        creator->createFederationExecution(federationName, fomModule, L"ExampleCustomTime"),
        rti1516_2025::CouldNotCreateLogicalTimeFactory);
    REQUIRE_THROWS_AS(
        creator->createFederationExecutionWithMIM(
            federationName,
            std::vector<std::wstring>{fomModule},
            L"HLAstandardMIM",
            L"HLAinteger64Time"),
        rti1516_2025::DesignatorIsHLAstandardMIM);
  }

  REQUIRE_NOTHROW(
      creator->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_THROWS_AS(
      creator->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"),
      rti1516_2025::FederationExecutionAlreadyExists);

  FederateHandle creatorHandle;
  REQUIRE_NOTHROW(
      creatorHandle = creator->joinFederationExecution(L"creator", L"owner", federationName));
  REQUIRE(creatorHandle.isValid());
  REQUIRE_THROWS_AS(
      creator->joinFederationExecution(L"creator", federationName),
      rti1516_2025::FederateAlreadyExecutionMember);
  REQUIRE_THROWS_AS(creator->disconnect(), rti1516_2025::FederateIsExecutionMember);

  FederateHandle joinerHandle;
  REQUIRE_NOTHROW(
      joinerHandle = joiner->joinFederationExecution(
          L"joiner",
          L"observer",
          federationName,
          std::vector<std::wstring>{fomModule}));
  REQUIRE(joinerHandle.isValid());
  REQUIRE(creatorHandle != joinerHandle);
  REQUIRE_THROWS_AS(
      duplicateName->joinFederationExecution(L"creator", L"observer", federationName),
      rti1516_2025::FederateNameAlreadyInUse);
  REQUIRE_THROWS_AS(
      creator->destroyFederationExecution(federationName),
      rti1516_2025::FederatesCurrentlyJoined);

  REQUIRE_THROWS_AS(
      creator->resignFederationExecution(static_cast<ResignAction>(-1)),
      rti1516_2025::InvalidResignAction);
  REQUIRE_NOTHROW(creator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(joiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));

  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(joiner->disconnect());
  REQUIRE_NOTHROW(duplicateName->disconnect());
}

TEST_CASE(
    "Embedded resign action unconditionally divests attributes for the 2025 ownership model",
    "[integration][development-profile][federation-management][ownership-management]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[rti.service.attribute-ownership-acquisition]") {
  TestFederateAmbassador ownerFederate;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"resigning-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      peer->joinFederationExecution(L"resigning-peer", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      L"HLAprivilegeToDeleteObject");
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const divestedAttributes{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);

  // Directive 6 cannot strand attributes owned by the resigning federate.
  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(NO_ACTION),
      rti1516_2025::FederateOwnsAttributes);

  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = peerReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == divestedAttributes);

  unsigned char const acquisitionTagBytes[] = {0xD1, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(peer->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(
      peerReports.attributeOwnershipAcquisitionReports.front().kind ==
      ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(peer->isAttributeOwnedByFederate(objectInstance, efficiency));

  // The remaining federate can divest the acquired attribute before the
  // federation is destroyed; the object itself was not deleted by directive 1.
  REQUIRE_NOTHROW(
      peer->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded resign action deletes delete-privileged objects and reports removal",
    "[integration][development-profile][federation-management][object-management]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.remove-object-instance]") {
  TestFederateAmbassador ownerFederate;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"deleting-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      peer->joinFederationExecution(L"deleting-peer", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  auto const objectInstanceName = owner->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);
  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(NO_ACTION),
      rti1516_2025::FederateOwnsAttributes);

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(peerReports.objectRemovalReports.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectRemovalReports.size() == 1);
  REQUIRE(peerReports.objectRemovalReports.front().objectInstance == objectInstance);
  REQUIRE_THROWS_AS(
      peer->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded assumption search continues after a later eligible publication",
    "[integration][development-profile][ownership-management][federation-management]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  TestFederateAmbassador ownerFederate;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"assumption-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"assumption-candidate", L"candidate", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      L"HLAprivilegeToDeleteObject");
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1);

  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.empty());

  // The candidate was known but not yet publishing when the owner resigned.
  // Publishing later makes it eligible and must continue the prior search.
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.front().objectInstance ==
          objectInstance);
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.front().attributes ==
          expectedAssumption);

  REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(candidate->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
}

TEST_CASE(
    "Embedded assumption search continues after a later join and discovery",
    "[integration][development-profile][ownership-management][federation-management]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador observerFederate;
  ReportingFederateAmbassador lateReports;
  auto owner = makeRti();
  auto observer = makeRti();
  auto late = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"search-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"search-observer", L"observer", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      L"HLAprivilegeToDeleteObject");
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));

  // Keep the execution alive with an unrelated member while the owner
  // resigns. The object remains, but no assumption recipient is yet known.
  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));

  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"search-late", L"candidate", federationName));
  REQUIRE_NOTHROW(late->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(late->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(lateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(lateReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.empty());

  REQUIRE_NOTHROW(late->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(late->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.front().objectInstance ==
          objectInstance);
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.front().attributes ==
          expectedAssumption);

  REQUIRE_NOTHROW(late->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(late->disconnect());
}

TEST_CASE(
    "Embedded resign action rejects pending ownership acquisition work",
    "[integration][development-profile][federation-management][ownership-management]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.attribute-ownership-acquisition]") {
  TestFederateAmbassador ownerFederate;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"pending-owner", L"subscriber", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"pending-requester", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = requester->registerObjectInstance(server));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  unsigned char const acquisitionTagBytes[] = {0xD2, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES),
      rti1516_2025::OwnershipAcquisitionPending);

  // Directive 5 explicitly resolves the pending request before the owner
  // leaves; the remaining producer can then delete its own object.
  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded final-federate resignation applies directive two regardless of the supplied action",
    "[integration][development-profile][federation-management][object-management]"
    "[rti.service.resign-federation-execution][federate.callback.object-instance-name-reservation-succeeded]") {
  ReportingFederateAmbassador ownerReports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"last-federate", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle originalObject;
  REQUIRE_NOTHROW(originalObject = owner->registerObjectInstance(server));
  auto const reusableObjectName = owner->getObjectInstanceName(originalObject);

  // 4.12.4 forces the delete-object pass for the last joined federate even
  // when the caller supplies NO_ACTION.  The deleted name must be reusable
  // after a new member joins the still-existing federation execution.
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"rejoined-federate", L"publisher", federationName));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(reusableObjectName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      ownerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
      reusableObjectName);

  ObjectInstanceHandle replacementObject;
  REQUIRE_NOTHROW(
      replacementObject = owner->registerObjectInstance(server, reusableObjectName));
  REQUIRE(replacementObject.isValid());
  REQUIRE(replacementObject != originalObject);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded federation-management preparation failures leave shared federation state unchanged",
    "[integration][development-profile][federation-management][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.destroy-federation-execution]") {
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador applicantFederate;
  auto owner = makeRti();
  auto applicant = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = resourcePath("examples/RestaurantExtensionFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(applicant->connect(applicantFederate, HLA_EVOKED));

  // The base module documents HLAinteger64Time, so the no-name Create default
  // must fail without reserving the federation name.
  REQUIRE_THROWS_AS(
      owner->createFederationExecution(federationName, baseFom),
      rti1516_2025::InconsistentFOM);
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));

  // The supplied extension cannot be materialized by the official FDD XSD.
  // Its failed addition must leave the valid base definition joinable.
  REQUIRE_THROWS_AS(
      applicant->joinFederationExecution(
          L"applicant",
          L"observer",
          federationName,
          std::vector<std::wstring>{extensionFom}),
      rti1516_2025::InconsistentFOM);
  FederateHandle applicantHandle;
  REQUIRE_NOTHROW(
      applicantHandle = applicant->joinFederationExecution(L"applicant", L"observer", federationName));
  REQUIRE(applicantHandle.isValid());

  REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(applicant->disconnect());
}

TEST_CASE(
    "Embedded federation save control coordinates initiation, status, completion, and abort",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.federate-save-not-complete]"
    "[rti.service.abort-federation-save][rti.service.query-federation-save-status]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-not-saved][federate.callback.federation-save-status-response]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"save-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"save-peer", L"observer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationSave(L"control-save-1"));
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{L"control-save-1"});
  REQUIRE(peerReports.initiateFederateSaveReports == std::vector<std::wstring>{L"control-save-1"});
  REQUIRE_THROWS_AS(owner->requestFederationSave(L"overlapping"), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->federateSaveComplete(), rti1516_2025::FederateHasNotBegunSave);

  REQUIRE_NOTHROW(owner->queryFederationSaveStatus());
  REQUIRE(ownerReports.federationSaveStatusReports.size() == 1);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].first == ownerHandle);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].second ==
          rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].first == peerHandle);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].second ==
          rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->queryFederationSaveStatus());
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].second ==
          rti1516_2025::FEDERATE_SAVING);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].second ==
          rti1516_2025::FEDERATE_SAVING);

  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 0);
  REQUIRE(peerReports.federationSavedReportCount == 0);
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(peerReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(owner->queryFederationSaveStatus());
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[0].second ==
          rti1516_2025::NO_SAVE_IN_PROGRESS);
  REQUIRE(ownerReports.federationSaveStatusReports.back().statuses[1].second ==
          rti1516_2025::NO_SAVE_IN_PROGRESS);

  REQUIRE_NOTHROW(owner->requestFederationSave(L"control-save-2"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveNotComplete());
  REQUIRE(ownerReports.federationNotSavedReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE);
  REQUIRE(peerReports.federationNotSavedReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE);

  REQUIRE_NOTHROW(owner->requestFederationSave(L"control-save-3"));
  REQUIRE_NOTHROW(owner->abortFederationSave());
  REQUIRE(ownerReports.federationNotSavedReasons.back() == rti1516_2025::SAVE_ABORTED);
  REQUIRE(peerReports.federationNotSavedReasons.back() == rti1516_2025::SAVE_ABORTED);
  REQUIRE_THROWS_AS(owner->abortFederationSave(), rti1516_2025::SaveNotInProgress);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded timed federation save waits for constrained TSO delivery and replaces pending requests",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-save-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timed-save-receiver", L"subscriber", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = owner->getParameterHandle(interactionClass, L"Identifier");
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0xA5, 0x25};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  std::size_t receiverTsoReportsAtSaveInitiate = 0;
  receiverReports.onInitiateFederateSave = [&receiverReports, &receiverTsoReportsAtSaveInitiate] {
    receiverTsoReportsAtSaveInitiate = receiverReports.timestampedInteractionReports.size();
  };

  REQUIRE_THROWS_AS(
      owner->requestFederationSave(L"timed-save-too-early", rti1516_2025::HLAinteger64Time(0)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_THROWS_AS(
      receiver->requestFederationSave(L"timed-save-at-galt", rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  // The second request replaces the first while neither has reached the
  // Initiate Federate Save boundary.
  REQUIRE_NOTHROW(owner->requestFederationSave(
      L"timed-save-replaced",
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->requestFederationSave(
      L"timed-save-final",
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(receiverReports.initiateFederateSaveReports.empty());

  // A timestamped interaction at the save boundary must be received before
  // the constrained federate can be instructed to save.
  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  // The receiver first receives the TSO payload at the scheduled-save
  // boundary, then receives Initiate Federate Save while still Time
  // Advancing, and only then receives its grant.
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"timed-save-final"});
  REQUIRE(receiverTsoReportsAtSaveInitiate == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  // The non-time-constrained regulator is queued only after the constrained
  // recipient has been admitted at its own pre-grant boundary.
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"timed-save-final"});
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  static_cast<void>(owner->evokeCallback(0.0));
  static_cast<void>(receiver->evokeCallback(0.0));
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded untimed federation save admits constrained members at time-advance boundaries",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"untimed-time-advance-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"untimed-save-owner", L"non-constrained-regulator", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"untimed-save-first", L"time-constrained", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"untimed-save-second", L"time-constrained", federationName));

  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE(firstReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE(secondReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(10)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1);

  ownerReports.callbackOrder.clear();
  firstReports.callbackOrder.clear();
  secondReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(firstReports.initiateFederateSaveReports.empty());
  REQUIRE(secondReports.initiateFederateSaveReports.empty());

  // Both constrained federates are now Time Advancing before either grant
  // callback is dispatched. The first callback must occur before its own
  // Time Advance Grant, but the non-constrained member must still wait for
  // the second constrained member to reach the same boundary.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(second->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  static_cast<void>(first->evokeCallback(0.0));
  REQUIRE(firstReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(firstReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(firstReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(secondReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  static_cast<void>(second->evokeCallback(0.0));
  REQUIRE(secondReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(secondReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(secondReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(first->federateSaveBegun());
  REQUIRE_NOTHROW(second->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(first->federateSaveComplete());
  REQUIRE_NOTHROW(second->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (first->evokeCallback(0.0)) {
  }
  while (second->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(firstReports.federationSavedReportCount == 1);
  REQUIRE(secondReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save admits every constrained member before non-constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"timed-time-advance-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-save-owner", L"non-constrained-regulator", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"timed-save-first", L"time-constrained", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"timed-save-second", L"time-constrained", federationName));

  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  firstReports.callbackOrder.clear();
  secondReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(firstReports.initiateFederateSaveReports.empty());
  REQUIRE(secondReports.initiateFederateSaveReports.empty());

  // TAR reaches the timestamp inclusively. Both constrained federates must
  // already be Time Advancing before either direct pre-grant initiation can
  // start; the ordinary regulator remains queued until the second one is
  // admitted at its own boundary.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(second->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  static_cast<void>(first->evokeCallback(0.0));
  REQUIRE(firstReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(firstReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(firstReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(secondReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  static_cast<void>(second->evokeCallback(0.0));
  REQUIRE(secondReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(secondReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(secondReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(first->federateSaveBegun());
  REQUIRE_NOTHROW(second->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(first->federateSaveComplete());
  REQUIRE_NOTHROW(second->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (first->evokeCallback(0.0)) {
  }
  while (second->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(firstReports.federationSavedReportCount == 1);
  REQUIRE(secondReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save uses an exclusive available-advance boundary",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.time-advance-request-available][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"available-time-advance-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"available-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"available-save-receiver", L"time-constrained", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));

  // TARA's next grant at exactly the save timestamp is not a qualifying
  // boundary. The save request remains pending after the recipient is granted
  // time 5 without an Initiate Federate Save callback.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(receiverReports.initiateFederateSaveReports.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant"});

  // Once the available request's next grant is strictly greater than the
  // scheduled save time, it is a qualifying boundary and direct initiation
  // precedes the grant.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "save-initiate", "grant"});

  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save covers inclusive and exclusive next-message boundaries",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.next-message-request][rti.service.next-message-request-available]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const inclusiveLabel = std::wstring{L"next-message-save-inclusive"};
  auto const exclusiveLabel = std::wstring{L"next-message-save-exclusive"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"next-message-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"next-message-save-receiver", L"time-constrained", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      inclusiveLabel,
      rti1516_2025::HLAinteger64Time(5)));

  // NMR is inclusive: its next grant at the scheduled save timestamp admits
  // the save and invokes the constrained recipient directly before that grant.
  REQUIRE_NOTHROW(receiver->nextMessageRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      exclusiveLabel,
      rti1516_2025::HLAinteger64Time(7)));

  // NMRA is exclusive. An equal next grant leaves the replacement request
  // pending; a later next grant performs the direct pre-grant admission.
  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel});
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel, exclusiveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"8");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate", "grant"});
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{inclusiveLabel, exclusiveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 2);
  REQUIRE(receiverReports.federationSavedReportCount == 2);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save admits Available and next-message constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador availableReports;
  ReportingFederateAmbassador nextMessageReports;
  auto owner = makeRti();
  auto available = makeRti();
  auto nextMessage = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"available-next-message-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(available->connect(availableReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nextMessage->connect(nextMessageReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"available-next-message-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(available->joinFederationExecution(
      L"available-next-message-save-available", L"time-constrained", federationName));
  REQUIRE_NOTHROW(nextMessage->joinFederationExecution(
      L"available-next-message-save-next-message", L"time-constrained", federationName));

  REQUIRE_NOTHROW(available->enableTimeConstrained());
  REQUIRE_FALSE(available->evokeCallback(0.0));
  REQUIRE_NOTHROW(nextMessage->enableTimeConstrained());
  REQUIRE_FALSE(nextMessage->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ownerReports.callbackOrder.clear();
  availableReports.callbackOrder.clear();
  nextMessageReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));

  // Both Available forms cross the strict clause-4.19 boundary at 6. The
  // save cannot start until each constrained member already has its own
  // queued grant, and the regulator must remain uninstructed until both
  // direct pre-grant callbacks have occurred.
  REQUIRE_NOTHROW(available->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(nextMessage->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_FALSE(available->evokeCallback(0.0));
  REQUIRE(availableReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(availableReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(availableReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(availableReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(nextMessageReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(nextMessage->evokeCallback(0.0));
  REQUIRE(nextMessageReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(nextMessageReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(nextMessageReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(nextMessageReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(available->federateSaveBegun());
  REQUIRE_NOTHROW(nextMessage->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(available->federateSaveComplete());
  REQUIRE_NOTHROW(nextMessage->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (available->evokeCallback(0.0)) {
  }
  while (nextMessage->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(availableReports.federationSavedReportCount == 1);
  REQUIRE(nextMessageReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(nextMessage->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(available->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nextMessage->disconnect());
  REQUIRE_NOTHROW(available->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save admits every advance mode before non-constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.next-message-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request-available][rti.service.flush-queue-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.flush-queue-grant][federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador tarReports;
  ReportingFederateAmbassador nmrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  ReportingFederateAmbassador fqrReports;
  auto owner = makeRti();
  auto tar = makeRti();
  auto nmr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto fqr = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"all-advance-modes-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tar->connect(tarReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmr->connect(nmrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"all-advance-modes-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(tar->joinFederationExecution(
      L"all-advance-modes-tar", L"time-constrained", federationName));
  REQUIRE_NOTHROW(nmr->joinFederationExecution(
      L"all-advance-modes-nmr", L"time-constrained", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"all-advance-modes-tara", L"time-constrained", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"all-advance-modes-nmra", L"time-constrained", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"all-advance-modes-fqr", L"time-constrained", federationName));

  REQUIRE_NOTHROW(tar->enableTimeConstrained());
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_NOTHROW(nmr->enableTimeConstrained());
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE_NOTHROW(tara->enableTimeConstrained());
  REQUIRE_FALSE(tara->evokeCallback(0.0));
  REQUIRE_NOTHROW(nmra->enableTimeConstrained());
  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE_NOTHROW(fqr->enableTimeConstrained());
  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  // TAR and NMR are inclusive at 5. TARA, NMRA, and FQR are strict, so each
  // uses the known value 6; FQR derives that same actual grant from the
  // regulator's pending request. The first TAR callback may begin the save
  // only because every other constrained member is already dispatchable.
  REQUIRE_NOTHROW(tar->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(nmr->nextMessageRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE(tarReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(tarReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(tarReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(tarReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(nmrReports.initiateFederateSaveReports.empty());
  REQUIRE(taraReports.initiateFederateSaveReports.empty());
  REQUIRE(nmraReports.initiateFederateSaveReports.empty());
  REQUIRE(fqrReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE(nmrReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(nmrReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(nmrReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(nmrReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(tara->evokeCallback(0.0));
  REQUIRE(taraReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(taraReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE(nmraReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(nmraReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE(fqrReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1);
  REQUIRE(fqrReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(fqrReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(tar->federateSaveBegun());
  REQUIRE_NOTHROW(nmr->federateSaveBegun());
  REQUIRE_NOTHROW(tara->federateSaveBegun());
  REQUIRE_NOTHROW(nmra->federateSaveBegun());
  REQUIRE_NOTHROW(fqr->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(tar->federateSaveComplete());
  REQUIRE_NOTHROW(nmr->federateSaveComplete());
  REQUIRE_NOTHROW(tara->federateSaveComplete());
  REQUIRE_NOTHROW(nmra->federateSaveComplete());
  REQUIRE_NOTHROW(fqr->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (tar->evokeCallback(0.0)) {
  }
  while (nmr->evokeCallback(0.0)) {
  }
  while (tara->evokeCallback(0.0)) {
  }
  while (nmra->evokeCallback(0.0)) {
  }
  while (fqr->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(tarReports.federationSavedReportCount == 1);
  REQUIRE(nmrReports.federationSavedReportCount == 1);
  REQUIRE(taraReports.federationSavedReportCount == 1);
  REQUIRE(nmraReports.federationSavedReportCount == 1);
  REQUIRE(fqrReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(nmr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tar->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(nmr->disconnect());
  REQUIRE_NOTHROW(tar->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save defers each constrained initiation for its queued TSO",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.send-interaction][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.receive-interaction]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador readyReports;
  ReportingFederateAmbassador delayedReports;
  auto owner = makeRti();
  auto ready = makeRti();
  auto delayed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"per-member-tso-save"};
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ready->connect(readyReports, HLA_EVOKED));
  REQUIRE_NOTHROW(delayed->connect(delayedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"per-member-tso-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(ready->joinFederationExecution(
      L"per-member-tso-ready", L"time-constrained", federationName));
  REQUIRE_NOTHROW(delayed->joinFederationExecution(
      L"per-member-tso-delayed", L"time-constrained", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = owner->getParameterHandle(interactionClass, L"Identifier");
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0x50, 0x45, 0x52};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(delayed->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(ready->enableTimeConstrained());
  REQUIRE_FALSE(ready->evokeCallback(0.0));
  REQUIRE_NOTHROW(delayed->enableTimeConstrained());
  REQUIRE_FALSE(delayed->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(ready->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(delayed->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  // The first member can enter the save operation at its clean inclusive
  // boundary. The delayed member's initiation remains local to its grant: it
  // must not happen until the TSO payload at the scheduled time is delivered.
  REQUIRE_FALSE(ready->evokeCallback(0.0));
  REQUIRE(readyReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(readyReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(readyReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(readyReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(delayedReports.timestampedInteractionReports.empty());
  REQUIRE(delayedReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(delayed->evokeCallback(0.0));
  REQUIRE(delayedReports.timestampedInteractionReports.size() == 1);
  REQUIRE(delayedReports.timestampedInteractionReports.back().timeValue == L"5");
  REQUIRE(delayedReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(delayedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(delayedReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(delayedReports.callbackOrder ==
          std::vector<std::string>{"interaction", "save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE(owner->evokeCallback(0.0));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(ready->federateSaveBegun());
  REQUIRE_NOTHROW(delayed->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(ready->federateSaveComplete());
  REQUIRE_NOTHROW(delayed->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (ready->evokeCallback(0.0)) {
  }
  while (delayed->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(readyReports.federationSavedReportCount == 1);
  REQUIRE(delayedReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(delayed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ready->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(delayed->disconnect());
  REQUIRE_NOTHROW(ready->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save requested during a TSO callback waits for in-transit delivery",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.send-interaction][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.receive-interaction]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"in-transit-tso-save"};
  unsigned char const tagBytes[] = {0x49, 0x4E, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"in-transit-tso-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"in-transit-tso-receiver", L"time-constrained", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = owner->getParameterHandle(interactionClass, L"Identifier");
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0x49, 0x4E, 0x54};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  bool reentrantSaveRequestAccepted = false;
  bool saveInitiatedBeforeTsoCallbackReturned = false;
  receiverReports.onTimestampedInteraction = [&] {
    saveInitiatedBeforeTsoCallbackReturned =
        !receiverReports.initiateFederateSaveReports.empty();
    try {
      owner->requestFederationSave(saveLabel, rti1516_2025::HLAinteger64Time(5));
      reentrantSaveRequestAccepted = true;
    } catch (...) {
      reentrantSaveRequestAccepted = false;
    }
  };

  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"4");

  // The timestamped request is made while this interaction is in transit.
  // It becomes eligible only after the callback returns and the runtime
  // completes that delivery, so the direct save callback follows Receive
  // Interaction and precedes the matching grant.
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(reentrantSaveRequestAccepted);
  REQUIRE_FALSE(saveInitiatedBeforeTsoCallbackReturned);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"4");
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save uses an exclusive Flush Queue grant boundary",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.receive-interaction][federate.callback.initiate-federate-save]"
    "[federate.callback.flush-queue-grant][federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"flush-queue-save"};
  unsigned char const tagBytes[] = {0x46, 0x51, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"flush-queue-save-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"flush-queue-save-receiver", L"time-constrained", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = owner->getParameterHandle(interactionClass, L"Identifier");
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0xF0, 0x51};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  // A TSO payload at the save time is flushed before the first actual grant.
  // That actual grant is equal to the scheduled save time, so FQR's strict
  // timestamped-save condition must leave the request pending.
  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"5");
  REQUIRE(receiverReports.initiateFederateSaveReports.empty());
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1);
  REQUIRE(receiverReports.flushQueueGrantReports.back().value == L"5");
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "flush-grant"});

  // The owner first finishes its advance to establish a later GALT, then its
  // next request permits an actual FQR grant of 6. That strictly-later grant
  // must instruct the constrained recipient before state changes to 6.
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"4");
  std::size_t flushQueueGrantsAtSaveInitiate = 0;
  receiverReports.onInitiateFederateSave =
      [&receiverReports, &flushQueueGrantsAtSaveInitiate] {
        flushQueueGrantsAtSaveInitiate = receiverReports.flushQueueGrantReports.size();
      };
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(flushQueueGrantsAtSaveInitiate == 1);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 2);
  REQUIRE(receiverReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant", "save-initiate", "flush-grant"});

  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save admits mixed Flush Queue and ordinary constrained members",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.flush-queue-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador thirdReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto third = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"mixed-flush-queue-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(third->connect(thirdReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mixed-flush-queue-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"mixed-flush-queue-first", L"time-constrained", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"mixed-flush-queue-second", L"time-constrained", federationName));
  REQUIRE_NOTHROW(third->joinFederationExecution(
      L"mixed-flush-queue-third", L"time-constrained", federationName));

  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE_NOTHROW(third->enableTimeConstrained());
  REQUIRE_FALSE(third->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  // The regulator's pending request produces GALT 6. Both FQRs therefore
  // have actual grants strictly beyond the save time, while TAR reaches the
  // inclusive boundary at 5. All three requests must be known before the
  // first direct admission starts the save operation.
  REQUIRE_NOTHROW(first->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(second->flushQueueRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(third->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE(firstReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(firstReports.flushQueueGrantReports.size() == 1);
  REQUIRE(firstReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(firstReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(secondReports.initiateFederateSaveReports.empty());
  REQUIRE(thirdReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE(secondReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReports.flushQueueGrantReports.size() == 1);
  REQUIRE(secondReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(secondReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(thirdReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(third->evokeCallback(0.0));
  REQUIRE(thirdReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(thirdReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(thirdReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(thirdReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(first->federateSaveBegun());
  REQUIRE_NOTHROW(second->federateSaveBegun());
  REQUIRE_NOTHROW(third->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(first->federateSaveComplete());
  REQUIRE_NOTHROW(second->federateSaveComplete());
  REQUIRE_NOTHROW(third->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (first->evokeCallback(0.0)) {
  }
  while (second->evokeCallback(0.0)) {
  }
  while (third->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(firstReports.federationSavedReportCount == 1);
  REQUIRE(secondReports.federationSavedReportCount == 1);
  REQUIRE(thirdReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(third->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(third->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed federation save can begin at an ordinary boundary before Flush Queue",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.flush-queue-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador ordinaryReports;
  ReportingFederateAmbassador flushQueueReports;
  auto owner = makeRti();
  auto ordinary = makeRti();
  auto flushQueue = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  auto const saveLabel = std::wstring{L"ordinary-before-flush-queue-save"};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ordinary->connect(ordinaryReports, HLA_EVOKED));
  REQUIRE_NOTHROW(flushQueue->connect(flushQueueReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ordinary-before-flush-queue-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(ordinary->joinFederationExecution(
      L"ordinary-before-flush-queue-tar", L"time-constrained", federationName));
  REQUIRE_NOTHROW(flushQueue->joinFederationExecution(
      L"ordinary-before-flush-queue-fqr", L"time-constrained", federationName));

  REQUIRE_NOTHROW(ordinary->enableTimeConstrained());
  REQUIRE_FALSE(ordinary->evokeCallback(0.0));
  REQUIRE_NOTHROW(flushQueue->enableTimeConstrained());
  REQUIRE_FALSE(flushQueue->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(flushQueue->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(ordinary->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  // The TAR callback starts the operation only because the registry has
  // precomputed that the pending FQR will also have actual grant 6 > 5.
  REQUIRE_FALSE(ordinary->evokeCallback(0.0));
  REQUIRE(ordinaryReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(ordinaryReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(ordinaryReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ordinaryReports.callbackOrder == std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(flushQueueReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(flushQueue->evokeCallback(0.0));
  REQUIRE(flushQueueReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(flushQueueReports.flushQueueGrantReports.size() == 1);
  REQUIRE(flushQueueReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(flushQueueReports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());

  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(ordinary->federateSaveBegun());
  REQUIRE_NOTHROW(flushQueue->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(ordinary->federateSaveComplete());
  REQUIRE_NOTHROW(flushQueue->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (ordinary->evokeCallback(0.0)) {
  }
  while (flushQueue->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(ordinaryReports.federationSavedReportCount == 1);
  REQUIRE(flushQueueReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(flushQueue->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ordinary->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(flushQueue->disconnect());
  REQUIRE_NOTHROW(ordinary->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded federation save and restore interlock representative services",
    "[integration][development-profile][federation-management][save-restore][interlocks]"
    "[rti.service.publish-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.create-region][rti.service.update-attribute-values]"
    "[rti.service.send-interaction][rti.service.attribute-ownership-acquisition]"
    "[rti.service.time-advance-request]"
    "[rti.service.object-class-declaration-interlocks]"
    "[rti.service.interaction-declaration-interlocks]"
    "[rti.service.directed-declaration-interlocks]"
    "[rti.service.order-type-interlocks]"
    "[rti.service.transportation-type-interlocks]"
    "[rti.service.ownership-disposition-interlocks]"
    "[rti.service.time-role-query-interlocks]"
    "[rti.service.scope-advisory-interlocks]"
    "[rti.service.regional-service-interlocks]"
    "[rti.service.synchronization-interlocks]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  VariableLengthData tag;

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"interlock-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"interlock-peer", L"peer", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  auto const takeOrder = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  auto const reliable = owner->getTransportationTypeHandle(L"HLAreliable");
  auto const ownerHandle = owner->getFederateHandle(L"interlock-owner");
  AttributeHandleSet const efficiencyOnly{efficiency};
  InteractionClassHandleSet const takeOrderOnly{takeOrder};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeInteractionClass(takeOrder));
  REQUIRE_NOTHROW(owner->publishObjectClassDirectedInteractions(server, takeOrderOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassDirectedInteractions(server, takeOrderOnly));
  auto objectInstance = owner->registerObjectInstance(server);
  AttributeHandleSet divestedAttributes;
  rti1516_2025::HLAinteger64Time interlockTime;
  rti1516_2025::HLAinteger64Interval interlockLookahead(1);
  auto const interlockRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      interlockRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{interlockRegion}));
  AttributeHandleSetRegionHandleSetPairVector const interlockAttributeRegions{{
      efficiencyOnly,
      RegionHandleSet{interlockRegion},
  }};
  RegionHandleSet const interlockRegions{interlockRegion};

  REQUIRE_NOTHROW(owner->requestFederationSave(L"interlock-save"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());

  REQUIRE_THROWS_AS(
      owner->publishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClass(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClass(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->publishInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->publishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeAttributeOrderType(objectInstance, efficiencyOnly, RECEIVE),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeOrderType(server, efficiencyOnly, RECEIVE),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeInteractionOrderType(takeOrder, RECEIVE),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeTransportationTypeChange(objectInstance, efficiencyOnly, reliable),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeTransportationType(server, efficiencyOnly, reliable),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeTransportationType(objectInstance, efficiency),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->requestInteractionTransportationTypeChange(takeOrder, reliable),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->queryInteractionTransportationType(ownerHandle, takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeOwnership(objectInstance, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->isAttributeOwnedByFederate(objectInstance, efficiency),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance, efficiencyOnly, tag, divestedAttributes),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelAttributeOwnershipAcquisition(objectInstance, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{sodaFlavor}),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->updateAttributeValues(objectInstance, AttributeHandleValueMap{}, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteraction(takeOrder, ParameterHandleValueMap{}, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipAcquisition(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->enableTimeRegulation(interlockLookahead),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeRegulation(), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->enableTimeConstrained(), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeConstrained(), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryLogicalTime(interlockTime), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryGALT(interlockTime), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryLITS(interlockTime), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryLookahead(interlockLookahead), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->modifyLookahead(interlockLookahead), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->getAttributeScopeAdvisorySwitch(),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->setAttributeScopeAdvisorySwitch(false),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstanceWithRegions(server, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unassociateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeValueUpdateWithRegions(server, interlockAttributeRegions, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteractionWithRegions(
          takeOrder, ParameterHandleValueMap{}, interlockRegions, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(L"interlock-sync", tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(
          L"interlock-sync-set", tag, FederateHandleSet{ownerHandle}),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->synchronizationPointAchieved(L"interlock-sync"),
      rti1516_2025::SaveInProgress);

  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE_NOTHROW(owner->requestFederationRestore(L"interlock-save"));

  REQUIRE_THROWS_AS(
      owner->publishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClass(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClass(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->publishInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->publishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeAttributeOrderType(objectInstance, efficiencyOnly, RECEIVE),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeOrderType(server, efficiencyOnly, RECEIVE),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeInteractionOrderType(takeOrder, RECEIVE),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeTransportationTypeChange(objectInstance, efficiencyOnly, reliable),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeTransportationType(server, efficiencyOnly, reliable),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeTransportationType(objectInstance, efficiency),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->requestInteractionTransportationTypeChange(takeOrder, reliable),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->queryInteractionTransportationType(ownerHandle, takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeOwnership(objectInstance, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->isAttributeOwnedByFederate(objectInstance, efficiency),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance, efficiencyOnly, tag, divestedAttributes),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelAttributeOwnershipAcquisition(objectInstance, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{sodaFlavor}),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->updateAttributeValues(objectInstance, AttributeHandleValueMap{}, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteraction(takeOrder, ParameterHandleValueMap{}, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipAcquisition(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->enableTimeRegulation(interlockLookahead),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeRegulation(), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->enableTimeConstrained(), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeConstrained(), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryLogicalTime(interlockTime), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryGALT(interlockTime), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryLITS(interlockTime), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryLookahead(interlockLookahead), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->modifyLookahead(interlockLookahead), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->getAttributeScopeAdvisorySwitch(),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->setAttributeScopeAdvisorySwitch(false),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstanceWithRegions(server, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unassociateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeValueUpdateWithRegions(server, interlockAttributeRegions, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteractionWithRegions(
          takeOrder, ParameterHandleValueMap{}, interlockRegions, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(L"interlock-sync", tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(
          L"interlock-sync-set", tag, FederateHandleSet{ownerHandle}),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->synchronizationPointAchieved(L"interlock-sync"),
      rti1516_2025::RestoreInProgress);

  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE_NOTHROW(peer->federateRestoreComplete());
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded federation resignation fails an outstanding save for remaining participants",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.resign-federation-execution][federate.callback.federation-not-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador resigningReports;
  auto owner = makeRti();
  auto resigning = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(resigning->connect(resigningReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"save-owner-resign", L"owner", federationName));
  REQUIRE_NOTHROW(
      resigning->joinFederationExecution(L"save-resigning", L"observer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationSave(L"resignation-save"));
  REQUIRE_NOTHROW(resigning->resignFederationExecution(NO_ACTION));
  REQUIRE(ownerReports.federationNotSavedReasons.size() == 1);
  REQUIRE(ownerReports.federationNotSavedReasons.back() ==
          rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE);
  REQUIRE_THROWS_AS(owner->federateSaveBegun(), rti1516_2025::SaveNotInitiated);

  // The failed operation is cleared, so the remaining member can start a
  // subsequent control-plane save rather than being left permanently stuck.
  REQUIRE_NOTHROW(owner->requestFederationSave(L"post-resignation-save"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(resigning->disconnect());
}

TEST_CASE(
    "Embedded federation restore rolls back a saved object-management snapshot",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.federate-restore-not-complete][rti.service.abort-federation-restore]"
    "[rti.service.query-federation-restore-status]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]"
    "[federate.callback.federation-restore-status-response]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"restore-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"restore-peer", L"peer", federationName));

  // Establish a completed, restorable image before adding any object state.
  REQUIRE_NOTHROW(owner->requestFederationSave(L"object-baseline"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(peerReports.federationSavedReportCount == 1);

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle mutatedObject;
  REQUIRE_NOTHROW(mutatedObject = owner->registerObjectInstance(server));
  REQUIRE(mutatedObject.isValid());

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"object-baseline"));
  REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{L"object-baseline"});
  REQUIRE(ownerReports.federationRestoreBegunReportCount == 1);
  REQUIRE(peerReports.federationRestoreBegunReportCount == 1);
  REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1);
  REQUIRE(peerReports.initiateFederateRestoreReports.size() == 1);
  REQUIRE(ownerReports.initiateFederateRestoreReports.front().federateName == L"restore-owner");
  REQUIRE(peerReports.initiateFederateRestoreReports.front().federateName == L"restore-peer");
  REQUIRE(ownerReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
          ownerHandle);
  REQUIRE(peerReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
          peerHandle);

  REQUIRE_NOTHROW(owner->queryFederationRestoreStatus());
  REQUIRE(ownerReports.federationRestoreStatusReports.size() == 1);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses[0].status ==
          rti1516_2025::FEDERATE_RESTORING);

  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE(ownerReports.federationRestoredReportCount == 0);
  REQUIRE(peerReports.federationRestoredReportCount == 0);
  REQUIRE_NOTHROW(peer->federateRestoreComplete());
  REQUIRE(ownerReports.federationRestoredReportCount == 1);
  REQUIRE(peerReports.federationRestoredReportCount == 1);
  REQUIRE(ownerReports.federationNotRestoredReasons.empty());
  REQUIRE(peerReports.federationNotRestoredReasons.empty());

  // The public ambassador still owns the same joined time state and callback
  // route, but the saved federation image no longer contains the post-save
  // object or its declarations.
  REQUIRE_THROWS_AS(
      owner->getKnownObjectClassHandle(mutatedObject),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(owner->queryFederationRestoreStatus());
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses[0].status ==
          rti1516_2025::NO_RESTORE_IN_PROGRESS);
  REQUIRE_FALSE(ownerReports.federationRestoreStatusReports.back().statuses[0]
                   .postRestoreHandle
                   .isValid());

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded federation restore reports missing labels and participant failures",
    "[integration][development-profile][federation-management][save-restore]"
    "[federate.callback.request-federation-restore-failed]"
    "[federate.callback.federation-not-restored]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"restore-failure-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"restore-failure-peer", L"peer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"missing-label"));
  REQUIRE(ownerReports.requestFederationRestoreFailedReports ==
          std::vector<std::wstring>{L"missing-label"});

  REQUIRE_NOTHROW(owner->requestFederationSave(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->federateRestoreNotComplete());
  REQUIRE(ownerReports.federationNotRestoredReasons.size() == 1);
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE);
  REQUIRE(peerReports.federationNotRestoredReasons.size() == 1);
  REQUIRE(peerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE);

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->abortFederationRestore());
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::RESTORE_ABORTED);
  REQUIRE(peerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::RESTORE_ABORTED);

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_RESIGNED_DURING_RESTORE);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded getTimeFactory returns the joined federation's selected time factory",
    "[integration][development-profile][federation-management][rti.service.get-time-factory]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"time-client", L"observer", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == L"HLAinteger64Time");
  auto initial = factory->makeInitial();
  auto* integerInitial = dynamic_cast<rti1516_2025::HLAinteger64Time*>(initial.get());
  REQUIRE(integerInitial != nullptr);
  REQUIRE(integerInitial->isInitial());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federate lookup services preserve departed designator identities within the joined federation",
    "[integration][development-profile][federation-management][rti.service.get-federate-handle]"
    "[rti.service.get-federate-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador peerFederate;
  TestFederateAmbassador foreignCreatorFederate;
  TestFederateAmbassador foreignMemberFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto foreignCreator = makeRti();
  auto foreignMember = makeRti();
  auto const federationName = nextFederationName();
  auto const foreignFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  FederateHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getFederateName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getFederateName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignCreator->connect(foreignCreatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignMember->connect(foreignMemberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(foreignCreator->createFederationExecution(
      foreignFederationName,
      fomModule,
      L"HLAinteger64Time"));

  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  FederateHandle foreignHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"lookup-peer", L"observer", federationName));
  REQUIRE_NOTHROW(foreignHandle = foreignMember->joinFederationExecution(
      L"lookup-foreign",
      L"observer",
      foreignFederationName));

  REQUIRE(owner->getFederateHandle(L"lookup-owner") == ownerHandle);
  REQUIRE(owner->getFederateHandle(L"lookup-peer") == peerHandle);
  REQUIRE(peer->getFederateName(ownerHandle) == L"lookup-owner");
  REQUIRE(peer->getFederateName(peerHandle) == L"lookup-peer");
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"missing"), rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(owner->getFederateName(invalid), rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->getFederateName(foreignHandle),
      rti1516_2025::FederateHandleNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"lookup-peer"), rti1516_2025::NameNotFound);
  // Get Federate Handle applies only to joined names.  The handle returned by
  // Join remains a valid federate designator after resignation, however, so
  // Get Federate Name must retain its immutable identity for the execution.
  REQUIRE(owner->getFederateName(peerHandle) == L"lookup-peer");

  REQUIRE_NOTHROW(foreignMember->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(foreignCreator->destroyFederationExecution(foreignFederationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(foreignCreator->disconnect());
  REQUIRE_NOTHROW(foreignMember->disconnect());
}

TEST_CASE(
    "Embedded object-class lookup services use stable handles from the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-object-class-handle]"
    "[rti.service.get-object-class-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "reference-data-class-provider-fom.xml";
  ObjectClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(L"HLAobjectRoot.Employee"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getObjectClassName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(L"HLAobjectRoot.Employee"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(L"HLAobjectRoot.Employee");
  REQUIRE(employee.isValid());
  REQUIRE(owner->getObjectClassHandle(L"HLAobjectRoot.Employee") == employee);
  REQUIRE(owner->getObjectClassName(employee) == L"HLAobjectRoot.Employee");
  REQUIRE_THROWS_AS(
      owner->getObjectClassHandle(L"HLAobjectRoot.Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getObjectClassName(invalid),
      rti1516_2025::InvalidObjectClassHandle);

  // A compatible additional-FOM join extends the catalog. The previously
  // returned class handle must remain stable while the new class becomes
  // discoverable from the same joined federation.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"lookup-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getObjectClassHandle(L"HLAobjectRoot.Employee") == employee);
  auto const extension = owner->getObjectClassHandle(L"HLAobjectRoot.UmbraReferenceFixtureClass");
  REQUIRE(extension.isValid());
  REQUIRE(extension != employee);
  REQUIRE(owner->getObjectClassName(extension) == L"HLAobjectRoot.UmbraReferenceFixtureClass");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded attribute lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-attribute-handle]"
    "[rti.service.get-attribute-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "reference-data-attribute-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, L"Name"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, L"Name"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"attribute-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const customer = owner->getObjectClassHandle(L"HLAobjectRoot.Customer");
  auto const employeeName = owner->getAttributeHandle(employee, L"Name");
  REQUIRE(employeeName.isValid());
  REQUIRE(owner->getAttributeHandle(employee, L"Name") == employeeName);
  REQUIRE(owner->getAttributeHandle(server, L"Name") == employeeName);
  REQUIRE(owner->getAttributeName(employee, employeeName) == L"Name");
  REQUIRE(owner->getAttributeName(server, employeeName) == L"Name");
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(server, L"Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(invalidObjectClass, L"Name"),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(invalidObjectClass, employeeName),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(employee, invalidAttribute),
      rti1516_2025::InvalidAttributeHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(customer, employeeName),
      rti1516_2025::AttributeNotDefined);

  // A compatible additional-FOM join adds a new inheritance chain. The
  // original Employee::Name handle stays stable, while the extension's
  // defining attribute has the same handle through its child class.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"attribute-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getAttributeHandle(employee, L"Name") == employeeName);
  auto const extensionBase = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const extensionChild = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureClass");
  auto const identifier = owner->getAttributeHandle(extensionBase, L"Identifier");
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getAttributeHandle(extensionChild, L"Identifier") == identifier);
  REQUIRE(owner->getAttributeName(extensionChild, identifier) == L"Identifier");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded interaction-class lookup services use stable handles from the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-interaction-class-handle]"
    "[rti.service.get-interaction-class-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "directed-interaction-interaction-provider-fom.xml";
  InteractionClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassName(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));

  auto const takeOrder = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.isValid());
  REQUIRE(
      owner->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder") == takeOrder);
  REQUIRE(owner->getInteractionClassName(takeOrder) == L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE_THROWS_AS(
      owner->getInteractionClassHandle(L"HLAinteractionRoot.Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getInteractionClassName(invalid),
      rti1516_2025::InvalidInteractionClassHandle);

  // A compatible additional-FOM join extends the catalog. The previously
  // returned interaction handle must remain stable while the new interaction
  // becomes discoverable from the same joined federation.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"lookup-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(
      owner->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder") == takeOrder);
  auto const extension = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(extension.isValid());
  REQUIRE(extension != takeOrder);
  REQUIRE(
      owner->getInteractionClassName(extension) ==
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded parameter lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-parameter-handle]"
    "[rti.service.get-parameter-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "parameter-handle-provider-fom.xml";
  InteractionClassHandle invalidInteractionClass;
  ParameterHandle invalidParameter;

  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, L"TemperatureOk"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, L"TemperatureOk"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"parameter-owner", L"owner", federationName));

  auto const mainCourseServed = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const takeOrder = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const temperatureOk = owner->getParameterHandle(mainCourseServed, L"TemperatureOk");
  REQUIRE(temperatureOk.isValid());
  REQUIRE(owner->getParameterHandle(mainCourseServed, L"TemperatureOk") == temperatureOk);
  REQUIRE(owner->getParameterName(mainCourseServed, temperatureOk) == L"TemperatureOk");
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(mainCourseServed, L"Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(invalidInteractionClass, L"TemperatureOk"),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(invalidInteractionClass, temperatureOk),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(mainCourseServed, invalidParameter),
      rti1516_2025::InvalidParameterHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(takeOrder, temperatureOk),
      rti1516_2025::InteractionParameterNotDefined);

  // A compatible additional-FOM join adds a new interaction inheritance
  // chain. The original Restaurant parameter handle remains stable, while the
  // extension's defining parameter has the same handle through its child.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"parameter-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getParameterHandle(mainCourseServed, L"TemperatureOk") == temperatureOk);
  auto const extensionBase = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase");
  auto const extensionChild = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = owner->getParameterHandle(extensionBase, L"Identifier");
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getParameterHandle(extensionChild, L"Identifier") == identifier);
  REQUIRE(owner->getParameterName(extensionChild, identifier) == L"Identifier");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded 2025 dimension lookup follows FOM hierarchy and upper bounds",
    "[integration][development-profile][federation-management]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const dimensionConsumerFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-consumer-fom.xml";
  auto const dimensionProviderFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  InteractionClassHandle invalidInteractionClass;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandle(L"SodaFlavor"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionName(invalidDimension),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"dimension-owner", L"owner", federationName));

  auto const barQuantity = owner->getDimensionHandle(L"BarQuantity");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  auto const sweetener = owner->getDimensionHandle(L"Sweetener");
  auto const serverId = owner->getDimensionHandle(L"ServerId");
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(sweetener.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(owner->getDimensionHandle(L"SodaFlavor") == sodaFlavor);
  REQUIRE(owner->getDimensionName(sodaFlavor) == L"SodaFlavor");
  REQUIRE(owner->getDimensionUpperBound(barQuantity) == 25UL);
  REQUIRE(owner->getDimensionUpperBound(sodaFlavor) == 4UL);
  REQUIRE(owner->getDimensionUpperBound(sweetener) == 3UL);
  REQUIRE(owner->getDimensionUpperBound(serverId) == 20UL);

  auto const drink = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink");
  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const light = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda.Light");
  DimensionHandleSet drinkDimensions = owner->getAvailableDimensionsForObjectClass(drink);
  DimensionHandleSet sodaDimensions = owner->getAvailableDimensionsForObjectClass(soda);
  DimensionHandleSet lightDimensions = owner->getAvailableDimensionsForObjectClass(light);
  REQUIRE(drinkDimensions == DimensionHandleSet{barQuantity});
  REQUIRE(sodaDimensions == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(lightDimensions == DimensionHandleSet{barQuantity, sodaFlavor, sweetener});

  auto const foodServed = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed");
  auto const mainCourseServed = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  REQUIRE(owner->getAvailableDimensionsForInteractionClass(foodServed).empty());
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(mainCourseServed) ==
      DimensionHandleSet{serverId});

  // A later 2025 FOM join contributes a class, interaction, and dimension
  // from separate modules. Existing Restaurant handles stay stable while the
  // newly composed dimension is immediately available to lookup services.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"dimension-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{dimensionConsumerFom.wstring(), dimensionProviderFom.wstring()}));
  REQUIRE(owner->getDimensionHandle(L"SodaFlavor") == sodaFlavor);
  auto const fixtureDimension = owner->getDimensionHandle(L"UmbraDimensionFixture");
  REQUIRE(fixtureDimension.isValid());
  REQUIRE(owner->getDimensionUpperBound(fixtureDimension) == 100UL);
  auto const fixtureObjectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDimensionFixtureObject");
  REQUIRE(
      owner->getAvailableDimensionsForObjectClass(fixtureObjectClass) ==
      DimensionHandleSet{fixtureDimension});
  auto const fixtureInteractionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDimensionFixtureInteraction");
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(fixtureInteractionClass) ==
      DimensionHandleSet{fixtureDimension});

  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(L"MissingDimension"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::InvalidInteractionClassHandle);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded public handle decoders enforce lifecycle and preserve encoded identities",
    "[integration][development-profile][federation-management][support-services][handles]"
    "[rti.service.decode-federate-handle][rti.service.decode-object-class-handle]"
    "[rti.service.decode-interaction-class-handle][rti.service.decode-object-instance-handle]"
    "[rti.service.decode-attribute-handle][rti.service.decode-parameter-handle]"
    "[rti.service.decode-dimension-handle][rti.service.decode-message-retraction-handle]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const encodedRetractionBytes[] = {
      0x00, 0x00, 0x00, 0x08, 0x01, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  };
  VariableLengthData const encodedRetraction(
      encodedRetractionBytes, sizeof(encodedRetractionBytes));

  REQUIRE_THROWS_AS(
      unjoined->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle federate;
  REQUIRE_NOTHROW(federate = owner->joinFederationExecution(
      L"handle-decoder-owner", L"owner", federationName));

  auto const objectClass = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const attribute = owner->getAttributeHandle(objectClass, L"Flavor");
  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const parameter = owner->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const dimension = owner->getDimensionHandle(L"BarQuantity");
  REQUIRE(federate.isValid());
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass, AttributeHandleSet{attribute}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());

  REQUIRE(owner->decodeFederateHandle(federate.encode()) == federate);
  REQUIRE(owner->decodeObjectClassHandle(objectClass.encode()) == objectClass);
  REQUIRE(owner->decodeInteractionClassHandle(interactionClass.encode()) == interactionClass);
  REQUIRE(owner->decodeObjectInstanceHandle(objectInstance.encode()) == objectInstance);
  REQUIRE(owner->decodeAttributeHandle(attribute.encode()) == attribute);
  REQUIRE(owner->decodeParameterHandle(parameter.encode()) == parameter);
  REQUIRE(owner->decodeDimensionHandle(dimension.encode()) == dimension);
  auto const decodedRetraction = owner->decodeMessageRetractionHandle(encodedRetraction);
  REQUIRE(decodedRetraction.isValid());
  REQUIRE(decodedRetraction.toString() == L"MessageRetractionHandle(72623859790382856)");

  REQUIRE_THROWS_AS(
      owner->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded 2025 region templates preserve pending and committed range state",
    "[integration][development-profile][federation-management][ddm][region-lifecycle]"
    "[rti.service.create-region][rti.service.commit-region-modifications]"
    "[rti.service.delete-region][rti.service.get-dimension-handle-set]"
    "[rti.service.get-range-bounds][rti.service.set-range-bounds]"
    "[rti.service.decode-region-handle]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador foreignFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto foreign = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  RegionHandle invalidRegion;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandleSet(invalidRegion),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreign->connect(foreignFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"region-owner", L"owner", federationName));
  REQUIRE_NOTHROW(foreign->joinFederationExecution(L"region-foreign", L"foreign", federationName));

  auto const barQuantity = owner->getDimensionHandle(L"BarQuantity");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());

  auto const region = owner->createRegion(DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(region.isValid());
  REQUIRE(owner->getDimensionHandleSet(region) == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(owner->decodeRegionHandle(region.encode()) == region);
  REQUIRE_THROWS_AS(
      owner->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, invalidDimension, RangeBounds(0UL, 10UL)),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(10UL, 10UL)),
      rti1516_2025::InvalidRangeBound);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 26UL)),
      rti1516_2025::InvalidRangeBound);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  auto const pendingBar = owner->getRangeBounds(region, barQuantity);
  REQUIRE(pendingBar.getLowerBound() == 0UL);
  REQUIRE(pendingBar.getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE(owner->getRangeBounds(region, barQuantity).getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, sodaFlavor),
      rti1516_2025::InvalidRegion);

  REQUIRE_THROWS_AS(
      foreign->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->deleteRegion(region),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->setRangeBounds(region, barQuantity, RangeBounds(0UL, 5UL)),
      rti1516_2025::RegionNotCreatedByThisFederate);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const committedBar = owner->getRangeBounds(region, barQuantity);
  auto const committedSoda = owner->getRangeBounds(region, sodaFlavor);
  REQUIRE(committedBar.getLowerBound() == 0UL);
  REQUIRE(committedBar.getUpperBound() == 10UL);
  REQUIRE(committedSoda.getLowerBound() == 1UL);
  REQUIRE(committedSoda.getUpperBound() == 3UL);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(5UL, 15UL)));
  auto const replacement = owner->getRangeBounds(region, barQuantity);
  REQUIRE(replacement.getLowerBound() == 5UL);
  REQUIRE(replacement.getUpperBound() == 15UL);
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const recommitted = owner->getRangeBounds(region, barQuantity);
  REQUIRE(recommitted.getLowerBound() == 5UL);
  REQUIRE(recommitted.getUpperBound() == 15UL);

  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(foreign->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(foreign->disconnect());
}

TEST_CASE(
    "Embedded interaction declaration services retain valid 2025 lifecycle and FOM boundaries",
    "[integration][development-profile][federation-management]"
    "[rti.service.publish-interaction-class][rti.service.unpublish-interaction-class]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador publisherFederate;
  TestFederateAmbassador subscriberFederate;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  InteractionClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->publishInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->subscribeInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unpublishInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->unsubscribeInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      publisher->joinFederationExecution(L"declaration-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      subscriber->joinFederationExecution(L"declaration-subscriber", L"subscriber", federationName));

  REQUIRE_THROWS_AS(
      publisher->publishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->unpublishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->unsubscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);

  auto const takeOrder = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.isValid());

  // Declaration state remains independent and idempotent per federate.  The
  // dedicated Send Interaction test below consumes this same state for
  // hierarchy-aware callback routing.
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions",
    "[integration][development-profile][federation-management]"
    "[rti.service.start-registration-for-object-class][rti.service.stop-registration-for-object-class]"
    "[rti.service.turn-interactions-on][rti.service.turn-interactions-off]"
    "[rti.service.get-object-class-relevance-advisory-switch]"
    "[rti.service.set-object-class-relevance-advisory-switch]"
    "[rti.service.get-interaction-relevance-advisory-switch]"
    "[rti.service.set-interaction-relevance-advisory-switch]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      publisher->joinFederationExecution(L"relevance-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      subscriber->joinFederationExecution(L"relevance-subscriber", L"subscriber", federationName));

  auto const employee = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const name = publisher->getAttributeHandle(employee, L"Name");
  auto const takeOrder = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(employee.isValid());
  REQUIRE(name.isValid());
  REQUIRE(takeOrder.isValid());

  // The Restaurant FOM explicitly enables these switches.  Their values are
  // seeded independently for each joining federate, then remain mutable via
  // the official per-federate accessors.
  REQUIRE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(subscriber->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(publisher->getInteractionRelevanceAdvisorySwitch());
  REQUIRE(subscriber->getInteractionRelevanceAdvisorySwitch());
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(publisher->setInteractionRelevanceAdvisorySwitch(false));
  REQUIRE_FALSE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(publisher->getInteractionRelevanceAdvisorySwitch());
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(true));
  REQUIRE_NOTHROW(publisher->setInteractionRelevanceAdvisorySwitch(true));
  REQUIRE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(publisher->getInteractionRelevanceAdvisorySwitch());

  // Active object subscription followed by publication establishes one Start
  // advisory at the publisher. Repeating the declaration is idempotent.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.empty());
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1);
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.front().objectClass == employee);
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1);

  // Removing the only active subscription establishes one Stop advisory.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.empty());
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 1);
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.front().objectClass == employee);

  // Passive ordinary subscriptions do not establish relevance. Switching to
  // active and back to passive creates exactly one Start/Stop pair.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1);
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 2);
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 2);

  // Interaction relevance follows the same active/passive boundary and is
  // independent of object-class relevance.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 1);
  REQUIRE(publisherReports.turnInteractionsOnReports.front().interactionClass == takeOrder);
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 1);
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOffReports.size() == 1);

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 2);
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOffReports.size() == 2);

  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded joins seed advisory switches from the current composed FDD",
    "[integration][development-profile][federation-management][fom][switches]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-relevance-advisory-switch]"
    "[rti.service.get-interaction-relevance-advisory-switch]") {
  TestFederateAmbassador baseReports;
  TestFederateAmbassador extensionReports;
  auto base = makeRti();
  auto extension = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml").wstring();
  auto const advisoryFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(base->connect(baseReports, HLA_EVOKED));
  REQUIRE_NOTHROW(extension->connect(extensionReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      base->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      base->joinFederationExecution(L"switch-base", L"base", federationName));

  // The base FOM omits both advisory entries, so the standard Disabled
  // default is applied to the first member.
  REQUIRE_FALSE(base->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(base->getInteractionRelevanceAdvisorySwitch());

  // The additional module contributes explicit Enabled settings.  It seeds
  // only the new member; the existing member's independently owned values are
  // not reset by the composed-definition replacement.
  REQUIRE_NOTHROW(extension->joinFederationExecution(
      L"switch-extension",
      L"extension",
      federationName,
      std::vector<std::wstring>{advisoryFom}));
  REQUIRE(extension->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(extension->getInteractionRelevanceAdvisorySwitch());
  REQUIRE_FALSE(base->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(base->getInteractionRelevanceAdvisorySwitch());

  REQUIRE_NOTHROW(extension->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(base->disconnect());
  REQUIRE_NOTHROW(extension->disconnect());
}

TEST_CASE(
    "Embedded federation shares the static Advisories Use Known Class switch",
    "[integration][development-profile][federation-management][fom][switches]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-advisories-use-known-class-switch]") {
  TestFederateAmbassador baseReports;
  TestFederateAmbassador knownClassReports;
  auto base = makeRti();
  auto knownClass = makeRti();
  auto const federationName = nextFederationName();
  auto const knownClassFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-known-class-enabled-fom.xml").wstring();
  auto const disabledFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml").wstring();

  REQUIRE_NOTHROW(base->connect(baseReports, HLA_EVOKED));
  REQUIRE_NOTHROW(knownClass->connect(knownClassReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      base->createFederationExecution(federationName, knownClassFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      base->joinFederationExecution(L"known-class-base", L"base", federationName));
  REQUIRE(base->getAdvisoriesUseKnownClassSwitch());

  // The additional FOM contributes an explicit Disabled value, but the
  // creation-time static switch remains shared by both federates.
  REQUIRE_NOTHROW(knownClass->joinFederationExecution(
      L"known-class-enabled",
      L"known-class",
      federationName,
      std::vector<std::wstring>{disabledFom}));
  REQUIRE(knownClass->getAdvisoriesUseKnownClassSwitch());
  REQUIRE(base->getAdvisoriesUseKnownClassSwitch());

  REQUIRE_NOTHROW(knownClass->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(base->disconnect());
  REQUIRE_NOTHROW(knownClass->disconnect());
}

TEST_CASE(
    "Embedded support switches are seeded per federate and retain static FDD policy",
    "[integration][development-profile][federation-management][fom][switches]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-exception-reporting-switch]"
    "[rti.service.set-exception-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.get-allow-relaxed-ddm-switch]") {
  TestFederateAmbassador ownerReports;
  TestFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const supportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, supportFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"support-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"support-peer", L"peer", federationName));

  // The explicit FDD settings seed each new member independently.
  REQUIRE(owner->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(peer->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(owner->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(peer->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE(owner->getExceptionReportingSwitch());
  REQUIRE(owner->getSendServiceReportsToFileSwitch());
  REQUIRE(peer->getServiceReportingSwitch());
  REQUIRE(peer->getExceptionReportingSwitch());
  REQUIRE(peer->getSendServiceReportsToFileSwitch());

  // Static switches are seeded at federation creation and are visible to all
  // members; this profile exposes no setter for either official API.
  REQUIRE(owner->getDelaySubscriptionEvaluationSwitch());
  REQUIRE(peer->getDelaySubscriptionEvaluationSwitch());
  REQUIRE(owner->getAllowRelaxedDDMSwitch());
  REQUIRE(peer->getAllowRelaxedDDMSwitch());

  // Per-federate setters do not leak into the peer's membership state.
  REQUIRE_NOTHROW(owner->setConveyRegionDesignatorSetsSwitch(false));
  REQUIRE_NOTHROW(owner->setAutomaticResignDirective(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setExceptionReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_FALSE(owner->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(owner->getAutomaticResignDirective() ==
          rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
  REQUIRE_FALSE(owner->getServiceReportingSwitch());
  REQUIRE_FALSE(owner->getExceptionReportingSwitch());
  REQUIRE_FALSE(owner->getSendServiceReportsToFileSwitch());
  REQUIRE(peer->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(peer->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(peer->getServiceReportingSwitch());
  REQUIRE(peer->getExceptionReportingSwitch());
  REQUIRE(peer->getSendServiceReportsToFileSwitch());

  REQUIRE_THROWS_AS(
      owner->setAutomaticResignDirective(static_cast<rti1516_2025::ResignAction>(99)),
      rti1516_2025::InvalidResignAction);

  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded joins preserve an explicit FOM NoAction automatic-resign directive",
    "[integration][development-profile][federation-management][fom][switches]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-automatic-resign-directive]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-explicit-no-action-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"explicit-no-action", L"observer", federationName));

  // The configured NoAction value is not the omitted FDD default. It must
  // reach the joined federate's per-member support-switch state unchanged.
  REQUIRE(rti->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service-report files have one immutable joined-federate lifetime",
    "[integration][development-profile][federation-management][mom][service-report-file]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withConfigurationName(L"report-file-test").withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-subject", L"observer", federationName));
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());

  auto files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const firstFile = files.front();
  auto const initialText = readTextFile(firstFile);
  REQUIRE_FALSE(initialText.empty());
  REQUIRE(initialText.front() == '{');
  REQUIRE(initialText.find("\"CallbackModel\":\"HLA_EVOKED\"") != std::string::npos);
  REQUIRE(initialText.find("\"ConfigurationName\":\"report-file-test\"") != std::string::npos);
  REQUIRE(initialText.find("\"HLAfederationName\":\"umbra-catch2-federation-") !=
          std::string::npos);
  REQUIRE(initialText.find("\"HLAMIMDesignator\":\"HLAstandardMIM\"") != std::string::npos);
  REQUIRE(initialText.find("\"HLAfederateName\":\"service-report-subject\"") !=
          std::string::npos);
  REQUIRE(initialText.find("\"HLAserialNumber\"") == std::string::npos);

  // File reporting switches gate future report appends only. They cannot
  // rotate or replace the preallocated joined-federate file, even when both
  // switches were disabled at the join that created its initial record.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE(serviceReportFiles(directory.path()) ==
          std::vector<std::filesystem::path>{firstFile});
  REQUIRE(readTextFile(firstFile) == initialText);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-subject", L"observer", federationName));
  files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);
  REQUIRE(files.front() != files.back());
  REQUIRE(std::find(files.begin(), files.end(), firstFile) != files.end());
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded simultaneously joined federates receive independent service-report files",
    "[integration][development-profile][federation-management][mom][service-report-file]") {
  TestFederateAmbassador subjectReports;
  TestFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"independent-report-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"independent-report-observer", L"observer", federationName));

  // The configured directory is shared, but each joined-federate lifetime
  // owns one distinct report file and its own initial record.  This runs
  // through the production factory path rather than the in-memory test seam.
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);
  REQUIRE(files.front() != files.back());
  auto const firstText = readTextFile(files.front());
  auto const secondText = readTextFile(files.back());
  auto const firstHasSubject =
      firstText.find("\"HLAfederateName\":\"independent-report-subject\"") != std::string::npos;
  auto const secondHasSubject =
      secondText.find("\"HLAfederateName\":\"independent-report-subject\"") != std::string::npos;
  auto const firstHasObserver =
      firstText.find("\"HLAfederateName\":\"independent-report-observer\"") != std::string::npos;
  auto const secondHasObserver =
      secondText.find("\"HLAfederateName\":\"independent-report-observer\"") != std::string::npos;
  REQUIRE(firstHasSubject != secondHasSubject);
  REQUIRE(firstHasObserver != secondHasObserver);
  REQUIRE(firstHasSubject != firstHasObserver);
  REQUIRE(secondHasSubject != secondHasObserver);

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service-report configuration rejects unusable directories without fallback",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-report-store]") {
  TestFederateAmbassador emptySettingReports;
  auto emptySettingRti = makeRti();
  auto const emptyDirectorySetting = configurationForServiceReportDirectory({});

  // The typed profile configuration exposes a real directory, not a
  // selectable memory sink. An empty explicit directory therefore fails
  // before Connect changes lifecycle state, and a later ordinary Connect
  // remains possible.
  REQUIRE_THROWS_AS(
      emptySettingRti->connect(emptySettingReports, HLA_EVOKED, emptyDirectorySetting),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(emptySettingRti->connect(emptySettingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(emptySettingRti->disconnect());

  static std::atomic_uint64_t counter{0};
  auto const filePath = std::filesystem::temp_directory_path() /
      ("umbra-service-report-not-a-directory-" + std::to_string(++counter));
  {
    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    REQUIRE(output.good());
    output << "not a directory";
    REQUIRE(output.good());
  }
  ScopedTemporaryFile fileGuard(filePath);

  TestFederateAmbassador fileSettingReports;
  auto fileSettingRti = makeRti();
  auto const fileDirectorySetting = RtiConfiguration::createConfiguration()
      .withAdditionalSettings(L"serviceReportDirectory=" + filePath.wstring());
  REQUIRE_THROWS_AS(
      fileSettingRti->connect(fileSettingReports, HLA_EVOKED, fileDirectorySetting),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(fileSettingRti->connect(fileSettingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fileSettingRti->disconnect());
}

TEST_CASE(
    "Embedded joins retain an unpublished RTI-owned joined-federate MOM snapshot",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[mom-object-foundation]") {
  using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;

  TestFederateAmbassador reports;
  auto rti = std::make_unique<UmbraRtiAmbassador>();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  auto const firstFederate = rti->joinFederationExecution(
      L"mom-snapshot-subject", L"observer", federationName);
  auto firstSnapshot = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(firstSnapshot);
  REQUIRE(firstSnapshot->objectInstanceHandle != 0U);
  REQUIRE(firstSnapshot->joinedFederateId != 0U);
  REQUIRE(firstSnapshot->objectClassHandle != 0U);
  REQUIRE(firstSnapshot->immutableFederatePoint.specificationCommitted);
  REQUIRE(firstSnapshot->immutableFederatePoint.dimensionHandles.size() == 1U);
  REQUIRE(firstSnapshot->immutableFederatePoint.committedRangeBounds.size() == 1U);
  auto const& pointRange = firstSnapshot->immutableFederatePoint.committedRangeBounds.begin()->second;
  REQUIRE(pointRange.upperBound == pointRange.lowerBound + 1U);

  // The seven Table 8 direct initial values are present as official MIM wire
  // encodings, including the public FederateHandle encoding and the exact
  // filesystem pathname. The empty dynamic module-designator array is the
  // correct value because this Join did not contribute an additional FOM.
  REQUIRE(firstSnapshot->initialAttributeValues.size() == 7U);
  auto containsEncodedValue = [&](VariableLengthData const& expected) {
    auto const expectedBytes = variableLengthDataBytes(expected);
    return std::any_of(
        firstSnapshot->initialAttributeValues.begin(),
        firstSnapshot->initialAttributeValues.end(),
        [&](auto const& attribute) {
          return variableLengthDataBytes(attribute.second) == expectedBytes;
        });
  };
  auto files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  REQUIRE(files.front().is_absolute());
  REQUIRE(files.front().parent_path() ==
          std::filesystem::absolute(directory.path()).lexically_normal());
  REQUIRE(containsEncodedValue(firstFederate.encode()));
  REQUIRE(containsEncodedValue(
      rti1516_2025::HLAunicodeString{L"mom-snapshot-subject"}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAunicodeString{L"observer"}.encode()));
  REQUIRE(containsEncodedValue(
      rti1516_2025::HLAunicodeString{L"umbra-embedded"}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAunicodeString{L"Umbra 0.1.0"}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAunicodeString{files.front().wstring()}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAinteger32BE{0}.encode()));

  // Switch changes gate later reporting only. They cannot replace the object
  // identity or its already allocated report-file value.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  auto const afterEnable = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(afterEnable);
  REQUIRE(afterEnable->objectInstanceHandle == firstSnapshot->objectInstanceHandle);
  auto sameInitialValues = [](auto const& left, auto const& right) {
    if (left.size() != right.size()) {
      return false;
    }
    auto leftValue = left.begin();
    auto rightValue = right.begin();
    for (; leftValue != left.end(); ++leftValue, ++rightValue) {
      if (leftValue->first != rightValue->first ||
          variableLengthDataBytes(leftValue->second) !=
              variableLengthDataBytes(rightValue->second)) {
        return false;
      }
    }
    return true;
  };
  REQUIRE(sameInitialValues(
      afterEnable->initialAttributeValues,
      firstSnapshot->initialAttributeValues));

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(rti->joinedFederateMomObjectSnapshotForTesting());
  auto const secondFederate = rti->joinFederationExecution(
      L"mom-snapshot-subject", L"observer", federationName);
  auto const secondSnapshot = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(secondSnapshot);
  REQUIRE(secondFederate != firstFederate);
  REQUIRE(secondSnapshot->objectInstanceHandle != firstSnapshot->objectInstanceHandle);
  files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded joined-federate MOM snapshots retain only FOM modules supplied at Join",
    "[integration][development-profile][federation-management][mom][mom-object-foundation][fom]") {
  using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;

  TestFederateAmbassador reports;
  auto rti = std::make_unique<UmbraRtiAmbassador>();
  auto const federationName = nextFederationName();
  auto const baseFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-known-class-enabled-fom.xml")
          .wstring();
  auto const joinedFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"mom-snapshot-additional-fom",
      L"observer",
      federationName,
      std::vector<std::wstring>{joinedFom}));
  auto const snapshot = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(snapshot);

  bool foundJoinScopedModuleList = false;
  for (auto const& [attributeHandle, value] : snapshot->initialAttributeValues) {
    static_cast<void>(attributeHandle);
    auto const designators = decodeHlaUnicodeStringList(value);
    if (designators && *designators == std::vector<std::wstring>{joinedFom}) {
      foundJoinScopedModuleList = true;
      break;
    }
  }
  REQUIRE(foundJoinScopedModuleList);

  // The Table 5 initial report must use the same Join-scoped module list as
  // the private MOM object. It must not claim that the base FOM was supplied
  // by this federate at Join.
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const encodedJoinedFom =
      umbra::detail::utf8FromWide(umbra::detail::formatMomString(joinedFom));
  REQUIRE(encodedJoinedFom);
  auto const joinScopedModuleList =
      std::string{"\"HLAFOMmoduleDesignatorList\":["} +
      *encodedJoinedFom + "]";
  REQUIRE(readTextFile(files.front()).find(joinScopedModuleList) != std::string::npos);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Internal tests can inject an in-memory service-report store without changing runtime configuration",
    "[unit][development-profile][federation-management][mom][service-report-store]") {
  using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;

  TestFederateAmbassador reports;
  auto store = std::make_unique<umbra::detail::MemoryServiceReportStore>();
  auto* const observedStore = store.get();
  auto rti = std::make_unique<UmbraRtiAmbassador>(
      UmbraRtiAmbassador::ServiceReportStoreTestSeam{}, std::move(store));
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"memory-report-subject", L"observer", federationName));
  REQUIRE(observedStore->records().size() == 1U);
  REQUIRE(observedStore->records().front().find(
              L"\"HLAfederateName\":\"memory-report-subject\"") != std::wstring::npos);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE(observedStore->records().size() == 1U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"memory-report-subject", L"observer", federationName));
  REQUIRE(observedStore->records().size() == 2U);
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Service-report writer creation failure rejects the join without an in-memory fallback",
    "[integration][development-profile][federation-management][mom][service-report-store]") {
  using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;

  TestFederateAmbassador failingReports;
  TestFederateAmbassador peerReports;
  auto failingStore = std::make_unique<FailingServiceReportStore>();
  auto* const observedStore = failingStore.get();
  auto failingRti = std::make_unique<UmbraRtiAmbassador>(
      UmbraRtiAmbassador::ServiceReportStoreTestSeam{}, std::move(failingStore));
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());

  REQUIRE_NOTHROW(failingRti->connect(failingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(failingRti->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_THROWS_AS(
      failingRti->joinFederationExecution(
          L"failure-subject", L"observer", federationName),
      rti1516_2025::RTIinternalError);
  REQUIRE(observedStore->createCalls == 1U);

  // A successful peer join with the rejected name proves that the failed
  // local writer setup rolled the registry membership back instead of leaving
  // a partial joined-federate identity behind.
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"failure-subject", L"observer", federationName));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(failingRti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(failingRti->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded MOM service-reporting state excludes report-service subscriptions",
    "[integration][development-profile][federation-management][mom][switches]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]") {
  TestFederateAmbassador ownerReports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const supportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, supportFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mom-report-owner", L"owner", federationName));

  auto const reportServiceInvocation = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  auto const serviceGroup = owner->getDimensionHandle(L"HLAserviceGroup");
  REQUIRE(reportServiceInvocation.isValid());
  REQUIRE(serviceGroup.isValid());
  REQUIRE(owner->getServiceReportingSwitch());

  // The FDD-seeded enabled switch rejects both ordinary and regional attempts
  // to create the exact MOM report-service subscription. This does not widen
  // the rule to the report interaction's ancestor classes.
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClass(reportServiceInvocation, false),
      rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM);
  auto const reportRegion = owner->createRegion(DimensionHandleSet{serviceGroup});
  REQUIRE_NOTHROW(owner->setRangeBounds(reportRegion, serviceGroup, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{reportRegion}));
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(
          reportServiceInvocation,
          RegionHandleSet{reportRegion},
          false),
      rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM);

  // Disabling reporting permits either subscription representation. Re-enabling
  // then fails until the exact ordinary/regional declaration has been removed.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_FALSE(owner->getServiceReportingSwitch());
  REQUIRE_NOTHROW(owner->subscribeInteractionClass(reportServiceInvocation, false));
  REQUIRE_THROWS_AS(
      owner->setServiceReportingSwitch(true),
      rti1516_2025::ReportServiceInvocationsAreSubscribed);
  REQUIRE_FALSE(owner->getServiceReportingSwitch());
  REQUIRE_NOTHROW(owner->unsubscribeInteractionClass(reportServiceInvocation));

  REQUIRE_NOTHROW(owner->subscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{reportRegion},
      false));
  REQUIRE_THROWS_AS(
      owner->setServiceReportingSwitch(true),
      rti1516_2025::ReportServiceInvocationsAreSubscribed);
  REQUIRE_NOTHROW(owner->unsubscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{reportRegion}));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE(owner->getServiceReportingSwitch());

  REQUIRE_NOTHROW(owner->deleteRegion(reportRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded MOM HLAsetSwitches updates the sending federate's switch subset",
    "[integration][development-profile][federation-management][mom][switches]"
    "[rti.service.send-interaction]"
    "[rti.service.get-object-class-relevance-advisory-switch]"
    "[rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.get-attribute-scope-advisory-switch]"
    "[rti.service.get-interaction-relevance-advisory-switch]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.get-exception-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]") {
  TestFederateAmbassador ownerReports;
  TestFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const supportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto const momExtensionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "mom-set-switches-extension-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{supportFom, momExtensionFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mom-switch-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"mom-switch-peer", L"peer", federationName));

  auto const setSwitches = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const objectClassRelevance = owner->getParameterHandle(
      setSwitches, L"HLAobjectClassRelevanceAdvisory");
  auto const attributeRelevance = owner->getParameterHandle(
      setSwitches, L"HLAattributeRelevanceAdvisory");
  auto const attributeScope = owner->getParameterHandle(
      setSwitches, L"HLAattributeScopeAdvisory");
  auto const interactionRelevance = owner->getParameterHandle(
      setSwitches, L"HLAinteractionRelevanceAdvisory");
  auto const conveyRegions = owner->getParameterHandle(
      setSwitches, L"HLAconveyRegionDesignatorSets");
  auto const automaticResign = owner->getParameterHandle(
      setSwitches, L"HLAautomaticResignAction");
  auto const serviceReporting = owner->getParameterHandle(
      setSwitches, L"HLAserviceReporting");
  auto const exceptionReporting = owner->getParameterHandle(
      setSwitches, L"HLAexceptionReporting");
  auto const sendServiceReportsToFile = owner->getParameterHandle(
      setSwitches, L"HLAsendServiceReportsToFile");
  auto const extensionPayload = owner->getParameterHandle(
      setSwitches, L"UmbraExtensionSwitchPayload");
  REQUIRE(setSwitches.isValid());
  REQUIRE(objectClassRelevance.isValid());
  REQUIRE(attributeRelevance.isValid());
  REQUIRE(attributeScope.isValid());
  REQUIRE(interactionRelevance.isValid());
  REQUIRE(conveyRegions.isValid());
  REQUIRE(automaticResign.isValid());
  REQUIRE(serviceReporting.isValid());
  REQUIRE(exceptionReporting.isValid());
  REQUIRE(sendServiceReportsToFile.isValid());
  REQUIRE(extensionPayload.isValid());

  // The support FOM names the latter four values explicitly; the advisory
  // switches omitted from that FOM start at the standard Disabled default.
  REQUIRE_FALSE(owner->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(owner->getAttributeRelevanceAdvisorySwitch());
  REQUIRE_FALSE(owner->getAttributeScopeAdvisorySwitch());
  REQUIRE_FALSE(owner->getInteractionRelevanceAdvisorySwitch());
  REQUIRE(owner->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(owner->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE(owner->getExceptionReportingSwitch());
  REQUIRE(owner->getSendServiceReportsToFileSwitch());

  std::vector<unsigned char> const enabled{0, 0, 0, 1};
  std::vector<unsigned char> const disabled{0, 0, 0, 0};
  std::vector<unsigned char> const noAction{0, 0, 0, 5};
  ParameterHandleValueMap allSwitchValues{
      {objectClassRelevance, VariableLengthData(enabled.data(), enabled.size())},
      {attributeRelevance, VariableLengthData(enabled.data(), enabled.size())},
      {attributeScope, VariableLengthData(enabled.data(), enabled.size())},
      {interactionRelevance, VariableLengthData(enabled.data(), enabled.size())},
      {conveyRegions, VariableLengthData(disabled.data(), disabled.size())},
      {automaticResign, VariableLengthData(noAction.data(), noAction.size())},
      {serviceReporting, VariableLengthData(disabled.data(), disabled.size())},
      {exceptionReporting, VariableLengthData(disabled.data(), disabled.size())},
      {sendServiceReportsToFile, VariableLengthData(disabled.data(), disabled.size())},
  };
  REQUIRE_NOTHROW(
      owner->sendInteraction(setSwitches, allSwitchValues, VariableLengthData()));

  // The MIM target is the sending joined federate, not a federation-wide
  // setting.  Verify both the full optional parameter subset and peer
  // isolation rather than treating the adjustment as a regular interaction
  // publication route.
  REQUIRE(owner->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());
  REQUIRE(owner->getAttributeScopeAdvisorySwitch());
  REQUIRE(owner->getInteractionRelevanceAdvisorySwitch());
  REQUIRE_FALSE(owner->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(owner->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);
  REQUIRE_FALSE(owner->getServiceReportingSwitch());
  REQUIRE_FALSE(owner->getExceptionReportingSwitch());
  REQUIRE_FALSE(owner->getSendServiceReportsToFileSwitch());
  REQUIRE_FALSE(peer->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(peer->getAttributeRelevanceAdvisorySwitch());
  REQUIRE_FALSE(peer->getAttributeScopeAdvisorySwitch());
  REQUIRE_FALSE(peer->getInteractionRelevanceAdvisorySwitch());
  REQUIRE(peer->getConveyRegionDesignatorSetsSwitch());
  REQUIRE(peer->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);
  REQUIRE(peer->getServiceReportingSwitch());
  REQUIRE(peer->getExceptionReportingSwitch());
  REQUIRE(peer->getSendServiceReportsToFileSwitch());

  // Table 20 makes HLAsetSwitches an explicit exception to the ordinary
  // all-parameters rule, but §11.4.2 still requires at least one parameter.
  REQUIRE_THROWS_AS(
      owner->sendInteraction(setSwitches, ParameterHandleValueMap{}, VariableLengthData()),
      rti1516_2025::InteractionParameterNotDefined);

  std::vector<unsigned char> const invalidResignAction{0, 0, 0, 6};
  ParameterHandleValueMap invalidResignValue{
      {automaticResign,
       VariableLengthData(invalidResignAction.data(), invalidResignAction.size())},
  };
  REQUIRE_THROWS_AS(
      owner->sendInteraction(setSwitches, invalidResignValue, VariableLengthData()),
      rti1516_2025::RTIinternalError);
  REQUIRE(owner->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);

  // Enabling Service Reporting through the MOM interaction follows the same
  // exact report-service-subscription interlock as the public support
  // service.  A mixed update is rejected as a unit, preserving the unrelated
  // Exception Reporting value as well.
  auto const reportServiceInvocation = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  REQUIRE(reportServiceInvocation.isValid());
  REQUIRE_NOTHROW(owner->subscribeInteractionClass(reportServiceInvocation, false));
  ParameterHandleValueMap rejectedValues{
      {serviceReporting, VariableLengthData(enabled.data(), enabled.size())},
      {exceptionReporting, VariableLengthData(enabled.data(), enabled.size())},
  };
  REQUIRE_THROWS_AS(
      owner->sendInteraction(setSwitches, rejectedValues, VariableLengthData()),
      rti1516_2025::RTIinternalError);
  REQUIRE_FALSE(owner->getServiceReportingSwitch());
  REQUIRE_FALSE(owner->getExceptionReportingSwitch());
  REQUIRE_NOTHROW(owner->unsubscribeInteractionClass(reportServiceInvocation));

  ParameterHandleValueMap reenableValues{
      {serviceReporting, VariableLengthData(enabled.data(), enabled.size())},
      {exceptionReporting, VariableLengthData(enabled.data(), enabled.size())},
  };
  REQUIRE_NOTHROW(
      owner->sendInteraction(setSwitches, reenableValues, VariableLengthData()));
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE(owner->getExceptionReportingSwitch());

  // A FOM can add a parameter to the standard MOM interaction. The RTI must
  // receive the extension but process only the predefined subset, so its
  // opaque, deliberately non-HLAswitch payload does not block a valid switch.
  std::vector<unsigned char> const extensionBytes{0xA5, 0x5A, 0x01};
  ParameterHandleValueMap extensionValues{
      {extensionPayload, VariableLengthData(extensionBytes.data(), extensionBytes.size())},
      {exceptionReporting, VariableLengthData(disabled.data(), disabled.size())},
  };
  REQUIRE_NOTHROW(
      owner->sendInteraction(setSwitches, extensionValues, VariableLengthData()));
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE_FALSE(owner->getExceptionReportingSwitch());

  // An extension may alternatively subclass the predefined interaction. The
  // RTI must process that as the promoted standard form: inherited standard
  // parameters still apply, while the extension-specific payload remains
  // opaque and ignored.
  auto const extendedSetSwitches = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches."
      L"UmbraExtendedSetSwitches");
  auto const promotedExceptionReporting = owner->getParameterHandle(
      extendedSetSwitches, L"HLAexceptionReporting");
  auto const extendedPayload = owner->getParameterHandle(
      extendedSetSwitches, L"UmbraExtendedSwitchPayload");
  REQUIRE(extendedSetSwitches.isValid());
  REQUIRE(promotedExceptionReporting.isValid());
  REQUIRE(extendedPayload.isValid());
  ParameterHandleValueMap promotedSubclassValues{
      {extendedPayload, VariableLengthData(extensionBytes.data(), extensionBytes.size())},
      {promotedExceptionReporting, VariableLengthData(enabled.data(), enabled.size())},
  };
  REQUIRE_NOTHROW(owner->sendInteraction(
      extendedSetSwitches, promotedSubclassValues, VariableLengthData()));
  REQUIRE(owner->getExceptionReportingSwitch());

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded handle normalization supplies stable DDM point-range coordinates",
    "[integration][development-profile][support-services][ddm][mom]"
    "[rti.service.normalize-service-group]"
    "[rti.service.normalize-federate-handle]"
    "[rti.service.normalize-object-class-handle]"
    "[rti.service.normalize-interaction-class-handle]"
    "[rti.service.normalize-object-instance-handle]") {
  TestFederateAmbassador ownerReports;
  TestFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  // The support services preserve the ordinary connection and joined-member
  // preconditions; save/restore is deliberately not an extra interlock in
  // the official 10.29--10.33 exception sets.
  REQUIRE_THROWS_AS(
      owner->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      owner->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"normalization-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"normalization-peer", L"peer", federationName));

  auto const ownerHandle = owner->getFederateHandle(L"normalization-owner");
  auto const peerHandle = owner->getFederateHandle(L"normalization-peer");
  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  auto const takeOrder = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(ownerHandle.isValid());
  REQUIRE(peerHandle.isValid());
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(takeOrder.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, AttributeHandleSet{efficiency}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());

  FederateHandle const peerHandleCopy(peerHandle);
  ObjectClassHandle const serverCopy(server);
  InteractionClassHandle const takeOrderCopy(takeOrder);
  ObjectInstanceHandle const objectInstanceCopy(objectInstance);

  // The standard promises equality preservation, not a sequential, unique,
  // or cross-execution-stable value. Verify the useful contract from both
  // joined ambassadors without asserting a homegrown coordinate scheme.
  auto const normalizedPeerHandle = owner->normalizeFederateHandle(peerHandle);
  REQUIRE(normalizedPeerHandle < std::numeric_limits<unsigned long>::max());
  REQUIRE(normalizedPeerHandle == owner->normalizeFederateHandle(peerHandle));
  REQUIRE(normalizedPeerHandle == owner->normalizeFederateHandle(peerHandleCopy));
  REQUIRE(normalizedPeerHandle == peer->normalizeFederateHandle(peerHandle));
  REQUIRE(owner->normalizeFederateHandle(ownerHandle) ==
          peer->normalizeFederateHandle(ownerHandle));

  auto const normalizedServer = owner->normalizeObjectClassHandle(server);
  REQUIRE(normalizedServer < std::numeric_limits<unsigned long>::max());
  REQUIRE(normalizedServer == owner->normalizeObjectClassHandle(server));
  REQUIRE(normalizedServer == owner->normalizeObjectClassHandle(serverCopy));
  REQUIRE(normalizedServer == peer->normalizeObjectClassHandle(server));

  auto const normalizedTakeOrder = owner->normalizeInteractionClassHandle(takeOrder);
  REQUIRE(normalizedTakeOrder < std::numeric_limits<unsigned long>::max());
  REQUIRE(normalizedTakeOrder == owner->normalizeInteractionClassHandle(takeOrder));
  REQUIRE(normalizedTakeOrder == owner->normalizeInteractionClassHandle(takeOrderCopy));
  REQUIRE(normalizedTakeOrder == peer->normalizeInteractionClassHandle(takeOrder));

  auto const normalizedObject = owner->normalizeObjectInstanceHandle(objectInstance);
  REQUIRE(normalizedObject < std::numeric_limits<unsigned long>::max());
  REQUIRE(normalizedObject == owner->normalizeObjectInstanceHandle(objectInstance));
  REQUIRE(normalizedObject == owner->normalizeObjectInstanceHandle(objectInstanceCopy));

  std::vector<ServiceGroup> const standardServiceGroups{
      rti1516_2025::FEDERATION_MANAGEMENT,
      rti1516_2025::DECLARATION_MANAGEMENT,
      rti1516_2025::OBJECT_MANAGEMENT,
      rti1516_2025::OWNERSHIP_MANAGEMENT,
      rti1516_2025::TIME_MANAGEMENT,
      rti1516_2025::DATA_DISTRIBUTION_MANAGEMENT,
      rti1516_2025::SUPPORT_SERVICES,
  };
  for (auto const serviceGroup : standardServiceGroups) {
    auto const normalized = owner->normalizeServiceGroup(serviceGroup);
    REQUIRE(normalized < 7UL);
    REQUIRE(normalized == owner->normalizeServiceGroup(serviceGroup));
    REQUIRE(normalized == peer->normalizeServiceGroup(serviceGroup));
  }

  FederateHandle invalidFederate;
  ObjectClassHandle invalidObjectClass;
  InteractionClassHandle invalidInteractionClass;
  ObjectInstanceHandle invalidObjectInstance;
  REQUIRE_THROWS_AS(
      owner->normalizeFederateHandle(invalidFederate),
      rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectClassHandle(invalidObjectClass),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeInteractionClassHandle(invalidInteractionClass),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectInstanceHandle(invalidObjectInstance),
      rti1516_2025::InvalidObjectInstanceHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeServiceGroup(static_cast<ServiceGroup>(99)),
      rti1516_2025::InvalidServiceGroup);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  // A FederateHandle remains a valid designator after the federate leaves the
  // execution, so its point coordinate remains available to joined peers.
  REQUIRE(normalizedPeerHandle == owner->normalizeFederateHandle(peerHandle));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded attribute relevance advisories follow scope transitions",
    "[integration][development-profile][federation-management][object-management]"
    "[rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.set-attribute-relevance-advisory-switch]"
    "[rti.service.turn-updates-on-for-object-instance]"
    "[rti.service.turn-updates-off-for-object-instance]"
    "[federate.callback.turn-updates-on-for-object-instance]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador subscriberReports;
  auto owner = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"attribute-relevance-owner", L"owner", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"attribute-relevance-subscriber", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = owner->getAttributeHandle(server, L"Name");
  auto const payRate = owner->getAttributeHandle(server, L"PayRate");
  REQUIRE(server.isValid());
  REQUIRE(name.isValid());
  REQUIRE(payRate.isValid());

  // RestaurantFOMmodule-2025 explicitly enables Attribute Relevance. The
  // subscriber's Attribute Scope switch remains Disabled; the two advisories
  // are intentionally independent.
  REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());
  REQUIRE_FALSE(subscriber->getAttributeScopeAdvisorySwitch());
  REQUIRE_NOTHROW(owner->setAttributeRelevanceAdvisorySwitch(false));
  REQUIRE_FALSE(owner->getAttributeRelevanceAdvisorySwitch());
  REQUIRE_NOTHROW(owner->setAttributeRelevanceAdvisorySwitch(true));
  REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());
  REQUIRE_NOTHROW(owner->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(owner->setInteractionRelevanceAdvisorySwitch(false));

  // Establish knowledge of the instance through Name, then make PayRate
  // enter and leave scope. This isolates Turn Updates from the initial
  // discovery transition, whose dedicated advisory path is still separate.
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, AttributeHandleSet{name, payRate}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{name},
      true));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  // RestaurantFOMmodule-2025 enables Auto Provide. Drain the RTI-invoked
  // provider callback before isolating the later scope-transition advisory.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate},
      true));
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().attributes ==
          AttributeHandleSet{payRate});

  // An explicit FDD designator selects the official rate-bearing callback
  // overload. The no-designator subscription above intentionally used the
  // legacy two-argument callback.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate}));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1);
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate},
      true,
      L"High"));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.size() == 1);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().objectInstance ==
          objectInstance);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().attributes ==
          AttributeHandleSet{payRate});
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().updateRateDesignator ==
          L"High");

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate}));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 2);
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().attributes ==
           AttributeHandleSet{payRate});

  // A queued explicit-rate advisory is resolved again at callback entry. The
  // subscription remains in scope, but replacing High with the omitted/default
  // designator before evocation must select the no-rate overload.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate},
      true,
      L"High"));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate},
      true,
      L""));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.size() == 1);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 2);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate}));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 3);

  // Disabling the owner's switch before a queued advisory is evoked suppresses
  // that callback and does not replay it when the switch is re-enabled.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate},
      true));
  REQUIRE_NOTHROW(owner->setAttributeRelevanceAdvisorySwitch(false));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 2);
  REQUIRE_NOTHROW(owner->setAttributeRelevanceAdvisorySwitch(true));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate}));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 4);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      server,
      AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded regional attribute relevance advisories retain explicit update-rate designators",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.turn-updates-on-for-object-instance]"
    "[federate.callback.turn-updates-off-for-object-instance]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador subscriberReports;
  auto owner = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-rate-owner", L"regional-rate-owner", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-rate-subscriber", L"regional-rate-subscriber", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(
      subscriber->setRangeBounds(subscriberRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(
      subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  auto const disjointOwnerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(
      owner->setRangeBounds(disjointOwnerRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{disjointOwnerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const disjointOwnerPair{{
      flavorOnly,
      RegionHandleSet{disjointOwnerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair,
      true,
      L"High"));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.empty());
  // The enabled Restaurant FOM also queues Auto Provide at the owning
  // federate; consume it before testing region association advisories.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  // Removing the last explicit association restores the default region, so
  // this matching regional subscriber remains relevant. Move instead to a
  // committed disjoint explicit region to exercise the off/on transition.
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());

  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, disjointOwnerPair));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1);

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, disjointOwnerPair));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.size() == 1);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().objectInstance ==
          objectInstance);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().attributes ==
          flavorOnly);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().updateRateDesignator ==
          L"High");

  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1);
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(disjointOwnerRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded update-rate lookup exposes FDD values and the default attribute boundary",
    "[integration][development-profile][federation-management][object-management]"
    "[rti.service.get-update-rate-value]"
    "[rti.service.get-update-rate-value-for-attribute]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador memberFederate;
  auto unjoined = makeRti();
  auto member = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValue(L"High"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValueForAttribute(invalidObjectInstance, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValue(L"High"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValueForAttribute(invalidObjectInstance, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      member->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      member->joinFederationExecution(L"update-rate-member", L"member", federationName));

  REQUIRE(member->getUpdateRateValue(L"High") == Catch::Approx(30.0));
  REQUIRE(member->getUpdateRateValue(L"Medium") == Catch::Approx(5.0));
  REQUIRE(member->getUpdateRateValue(L"Low") == Catch::Approx(0.2));
  REQUIRE(member->getUpdateRateValue(L"HLAdefault") == Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValue(L"default") == Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValue(L"HLAdefaultUpdateRate") == Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValue(L"") == Catch::Approx(0.0));
  REQUIRE_THROWS_AS(
      member->getUpdateRateValue(L"MissingRate"),
      rti1516_2025::InvalidUpdateRateDesignator);

  auto const server = member->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = member->getAttributeHandle(server, L"Name");
  auto const payRate = member->getAttributeHandle(server, L"PayRate");
  REQUIRE_NOTHROW(
      member->publishObjectClassAttributes(server, AttributeHandleSet{name, payRate}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = member->registerObjectInstance(server));

  REQUIRE_THROWS_AS(
      member->subscribeObjectClassAttributes(
          server,
          AttributeHandleSet{name},
          true,
          L"MissingRate"),
      rti1516_2025::InvalidUpdateRateDesignator);
  REQUIRE_NOTHROW(member->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{name},
      true,
      L"High"));
  REQUIRE_NOTHROW(member->subscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate},
      true,
      L"Medium"));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) == Catch::Approx(30.0));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, payRate) == Catch::Approx(5.0));
  REQUIRE_NOTHROW(member->unsubscribeObjectClassAttributes(
      server,
      AttributeHandleSet{name}));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) == Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, payRate) == Catch::Approx(5.0));
  REQUIRE_NOTHROW(member->unsubscribeObjectClassAttributes(
      server,
      AttributeHandleSet{payRate}));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) == Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, payRate) == Catch::Approx(0.0));
  REQUIRE_THROWS_AS(
      member->getUpdateRateValueForAttribute(invalidObjectInstance, name),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      member->getUpdateRateValueForAttribute(objectInstance, invalidAttribute),
      rti1516_2025::AttributeNotDefined);

  REQUIRE_NOTHROW(member->unpublishObjectClassAttributes(
      server,
      AttributeHandleSet{name, payRate}));
  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

TEST_CASE(
    "Embedded transportation type lookup exposes the mandatory 2025 support pair",
    "[integration][development-profile][federation-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-transportation-type-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador memberFederate;
  auto unjoined = makeRti();
  auto member = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  TransportationTypeHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeHandle(L"HLAreliable"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeName(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeHandle(L"HLAreliable"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      member->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      member->joinFederationExecution(L"transport-member", L"member", federationName));

  auto const reliable = member->getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = member->getTransportationTypeHandle(L"HLAbestEffort");
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE(reliable != bestEffort);
  REQUIRE(member->getTransportationTypeHandle(L"HLAreliable") == reliable);
  REQUIRE(member->getTransportationTypeName(reliable) == L"HLAreliable");
  REQUIRE(member->getTransportationTypeName(bestEffort) == L"HLAbestEffort");
  REQUIRE_THROWS_AS(
      member->getTransportationTypeHandle(L"UmbraCustomTransport"),
      rti1516_2025::InvalidTransportationName);
  REQUIRE_THROWS_AS(
      member->getTransportationTypeName(invalid),
      rti1516_2025::InvalidTransportationTypeHandle);

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

TEST_CASE(
    "Embedded order type lookup exposes the mandatory 2025 Receive and TimeStamp pair",
    "[integration][development-profile][time-management]"
    "[rti.service.get-order-type]"
    "[rti.service.get-order-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador memberFederate;
  auto unjoined = makeRti();
  auto member = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      unjoined->getOrderType(L"Receive"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getOrderName(RECEIVE),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getOrderType(L"Receive"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getOrderName(RECEIVE),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      member->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      member->joinFederationExecution(L"order-member", L"member", federationName));

  REQUIRE(member->getOrderType(L"Receive") == RECEIVE);
  REQUIRE(member->getOrderType(L"TimeStamp") == TIMESTAMP);
  REQUIRE(member->getOrderName(RECEIVE) == L"Receive");
  REQUIRE(member->getOrderName(TIMESTAMP) == L"TimeStamp");
  REQUIRE_THROWS_AS(
      member->getOrderType(L"HLAcustomOrder"),
      rti1516_2025::InvalidOrderName);
  REQUIRE_THROWS_AS(
      member->getOrderName(static_cast<OrderType>(0x7f)),
      rti1516_2025::InvalidOrderType);

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

TEST_CASE(
    "Embedded transportation type control commits at callbacks and preserves FOM defaults",
    "[integration][development-profile][transportation-management]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.change-default-attribute-transportation-type]"
    "[rti.service.query-attribute-transportation-type]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]"
    "[federate.callback.confirm-attribute-transportation-type-change]"
    "[federate.callback.report-attribute-transportation-type]"
    "[federate.callback.confirm-interaction-transportation-type-change]"
    "[federate.callback.report-interaction-transportation-type]"
    "[rti.service.update-attribute-values][federate.callback.reflect-attribute-values]"
    "[rti.service.send-interaction][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"transport-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"transport-peer", L"peer", federationName));
  // This scenario is about transportation controls; suppress the separately
  // tested declaration-relevance callbacks so its evocation assertions remain
  // scoped to the service under test.
  REQUIRE_NOTHROW(owner->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(owner->setInteractionRelevanceAdvisorySwitch(false));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  auto const takeOrder = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const reliable = owner->getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = owner->getTransportationTypeHandle(L"HLAbestEffort");

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(peer->subscribeInteractionClass(takeOrder));

  // The first object captures the FDD default. A later class-default change
  // must not rewrite this already registered attribute.
  ObjectInstanceHandle firstObject;
  REQUIRE_NOTHROW(firstObject = owner->registerObjectInstance(server));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE_NOTHROW(owner->changeDefaultAttributeTransportationType(
      server,
      efficiencyOnly,
      bestEffort));
  ObjectInstanceHandle secondObject;
  REQUIRE_NOTHROW(secondObject = owner->registerObjectInstance(server));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 2);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE_NOTHROW(owner->queryAttributeTransportationType(firstObject, efficiency));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeTransportationTypeReports.size() == 1);
  REQUIRE(ownerReports.attributeTransportationTypeReports.front().objectInstance == firstObject);
  REQUIRE(ownerReports.attributeTransportationTypeReports.front().transportationType == reliable);

  REQUIRE_NOTHROW(owner->queryAttributeTransportationType(secondObject, efficiency));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeTransportationTypeReports.size() == 2);
  REQUIRE(ownerReports.attributeTransportationTypeReports.back().objectInstance == secondObject);
  REQUIRE(ownerReports.attributeTransportationTypeReports.back().transportationType == bestEffort);

  unsigned char const valueBytes[] = {0x2A};
  AttributeHandleValueMap values;
  values.emplace(efficiency, VariableLengthData(valueBytes, sizeof(valueBytes)));
  VariableLengthData const tag;

  // An accepted change remains pending until its confirmation callback. An
  // update submitted before that callback still uses the captured old type.
  REQUIRE_NOTHROW(owner->requestAttributeTransportationTypeChange(
      firstObject,
      efficiencyOnly,
      bestEffort));
  REQUIRE_THROWS_AS(
      owner->requestAttributeTransportationTypeChange(firstObject, efficiencyOnly, reliable),
      rti1516_2025::AttributeAlreadyBeingChanged);
  REQUIRE_NOTHROW(owner->updateAttributeValues(firstObject, values, tag));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.attributeReflectionReports.size() == 1);
  REQUIRE(peerReports.attributeReflectionReports.back().transportationType == reliable);
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.empty());

  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.size() == 1);
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.front().objectInstance == firstObject);
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.front().attributes == efficiencyOnly);
  REQUIRE(ownerReports.attributeTransportationTypeChangeReports.front().transportationType == bestEffort);

  REQUIRE_NOTHROW(owner->updateAttributeValues(firstObject, values, tag));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.attributeReflectionReports.size() == 2);
  REQUIRE(peerReports.attributeReflectionReports.back().transportationType == bestEffort);

  // Interaction changes apply to ordinary and regional future sends at the
  // same confirmation boundary. This test exercises the ordinary overload.
  REQUIRE_NOTHROW(owner->requestInteractionTransportationTypeChange(takeOrder, bestEffort));
  REQUIRE_NOTHROW(owner->sendInteraction(takeOrder, ParameterHandleValueMap{}, tag));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.interactionReports.size() == 1);
  REQUIRE(peerReports.interactionReports.back().transportationType == reliable);
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.size() == 1);
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.front().interactionClass == takeOrder);
  REQUIRE(ownerReports.interactionTransportationTypeChangeReports.front().transportationType == bestEffort);

  REQUIRE_NOTHROW(owner->sendInteraction(takeOrder, ParameterHandleValueMap{}, tag));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.interactionReports.size() == 2);
  REQUIRE(peerReports.interactionReports.back().transportationType == bestEffort);

  REQUIRE_NOTHROW(peer->queryInteractionTransportationType(ownerHandle, takeOrder));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.interactionTransportationTypeReports.size() == 1);
  REQUIRE(peerReports.interactionTransportationTypeReports.front().federate == ownerHandle);
  REQUIRE(peerReports.interactionTransportationTypeReports.front().interactionClass == takeOrder);
  REQUIRE(peerReports.interactionTransportationTypeReports.front().transportationType == bestEffort);

  REQUIRE_NOTHROW(peer->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded order type control captures defaults, instance overrides, and publisher interaction order",
    "[integration][development-profile][time-management][object-management]"
    "[rti.service.change-attribute-order-type]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.update-attribute-values][rti.service.send-interaction]"
    "[federate.callback.reflect-attribute-values][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      publisher->joinFederationExecution(L"order-control-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      receiver->joinFederationExecution(L"order-control-receiver", L"receiver", federationName));
  // Keep declaration-relevance callbacks out of this order/defaults scenario;
  // their FOM-seeded defaults are covered by the dedicated advisory tests.
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(publisher->setInteractionRelevanceAdvisorySwitch(false));

  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  auto const takeOrder = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(takeOrder));
  REQUIRE_THROWS_AS(
      publisher->changeDefaultAttributeOrderType(
          server,
          efficiencyOnly,
          static_cast<OrderType>(0x7f)),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      publisher->changeInteractionOrderType(takeOrder, static_cast<OrderType>(0x7f)),
      rti1516_2025::RTIinternalError);

  ObjectInstanceHandle fomDefaultObject;
  REQUIRE_NOTHROW(fomDefaultObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // RestaurantFOMmodule-2025 declares this attribute TimeStamp.  The class
  // default change is prospective, so only later registrations capture Receive.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      server,
      efficiencyOnly,
      RECEIVE));
  ObjectInstanceHandle defaultReceiveObject;
  REQUIRE_NOTHROW(defaultReceiveObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // An explicit instance change affects future updates for that owned
  // attribute without rewriting the other instances' captured defaults.
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      defaultReceiveObject,
      efficiencyOnly,
      TIMESTAMP));
  ObjectInstanceHandle unchangedReceiveObject;
  REQUIRE_NOTHROW(unchangedReceiveObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  // The Receive-ordered instance is intentionally delivered while the
  // constrained receiver is idle, so opt into the official asynchronous mode.
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  unsigned char const valueBytes[] = {0x4A};
  AttributeHandleValueMap values;
  values.emplace(efficiency, VariableLengthData(valueBytes, sizeof(valueBytes)));
  VariableLengthData const tag;
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      fomDefaultObject,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      defaultReceiveObject,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      unchangedReceiveObject,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // The unchanged Receive instance is delivered immediately; the FOM-default
  // and explicitly changed instances wait for the receiver's grant.
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1);
  REQUIRE(receiverReports.attributeReflectionReports.front().sentOrderType == RECEIVE);
  REQUIRE(receiverReports.attributeReflectionReports.front().receivedOrderType == RECEIVE);
  REQUIRE_FALSE(receiverReports.attributeReflectionReports.front().retractionSupplied);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 3);
  std::size_t timestampedReflectionCount = 0;
  for (auto const& report : receiverReports.attributeReflectionReports) {
    if (report.sentOrderType == TIMESTAMP) {
      ++timestampedReflectionCount;
      REQUIRE(report.receivedOrderType == TIMESTAMP);
      REQUIRE(report.retractionSupplied);
      REQUIRE(report.retractionValid);
    }
  }
  REQUIRE(timestampedReflectionCount == 2);

  // Interaction order is scoped to the invoking publisher and changes future
  // timestamped sends without introducing a TSO queue for this class.
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(takeOrder, RECEIVE));
  auto const retraction = publisher->sendInteraction(
      takeOrder,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE_FALSE(retraction.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  auto const& interactionReport = receiverReports.timestampedInteractionReports.front();
  REQUIRE(interactionReport.sentOrderType == RECEIVE);
  REQUIRE(interactionReport.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(interactionReport.retractionSupplied);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries",
    "[integration][development-profile][federation-management][declaration-management]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.unpublish-object-class]"
    "[rti.service.unpublish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class]"
    "[rti.service.unsubscribe-object-class-attributes]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador publisherFederate;
  TestFederateAmbassador subscriberFederate;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;
  AttributeHandle invalidAttribute;
  AttributeHandleSet const noAttributes;

  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(invalidObjectClass, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      publisher->unpublishObjectClass(invalidObjectClass),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->connect(publisherFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->subscribeObjectClassAttributes(invalidObjectClass, noAttributes),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->unsubscribeObjectClass(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(L"subscriber", federationName));

  auto const employee = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = publisher->getAttributeHandle(employee, L"Name");
  auto const inheritedName = publisher->getAttributeHandle(server, L"Name");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  REQUIRE(name == inheritedName);

  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const invalidOnly{invalidAttribute};
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(invalidObjectClass, nameOnly),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->unpublishObjectClass(invalidObjectClass),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(employee, invalidOnly),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(employee, efficiencyOnly),
      rti1516_2025::AttributeNotDefined);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(employee, nameOnly));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(server, nameOnly, false));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(server, efficiencyOnly, true));
  // The whole-class form removes ordinary subscriptions at that exact class;
  // regional declarations remain an independent DDM state family.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClass(server));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(server, nameOnly, false));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(server, efficiencyOnly, true));
  // Whole-class unpublication removes every currently published attribute at
  // that class, while the inherited Name publication on Employee remains
  // until its own class is unpublished.
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(server));
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(employee));
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(server));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClass(server));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(server, nameOnly));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(employee, nameOnly));

  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle",
    "[integration][development-profile][object-management]"
    "[rti.service.register-object-instance]"
    "[rti.service.get-known-object-class-handle]"
    "[rti.service.get-object-instance-handle]"
    "[rti.service.get-object-instance-name]"
    "[federate.callback.discover-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador implicitPrivilegeReports;
  ReportingFederateAmbassador lateReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto cancelled = makeRti();
  auto implicitPrivilege = makeRti();
  auto late = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;
  ObjectInstanceHandle objectInstance;

  // The 2025 service preserves connection and membership preconditions ahead
  // of caller-supplied handle validation.
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(implicitPrivilege->connect(implicitPrivilegeReports, HLA_EVOKED));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"object-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"object-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"object-cancelled", L"subscriber", federationName));
  REQUIRE_NOTHROW(implicitPrivilege->joinFederationExecution(
      L"object-implicit-privilege", L"subscriber", federationName));
  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"object-late", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"object-immediate", L"subscriber", federationName));

  auto const employee = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = publisher->getAttributeHandle(employee, L"Name");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  auto const privilegeToDelete = publisher->getAttributeHandle(
      server,
      L"HLAprivilegeToDeleteObject");
  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const privilegeToDeleteOnly{privilegeToDelete};
  AttributeHandleSet const publishedAttributes{name, efficiency};

  REQUIRE_THROWS_AS(
      publisher->registerObjectInstance(invalidObjectClass),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->registerObjectInstance(server),
      rti1516_2025::ObjectClassNotPublished);

  // A registered Server is discoverable as Server to its closest exact
  // subscribers and as Employee to an active subscriber only at that
  // superclass.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(employee, nameOnly, true));
  REQUIRE_NOTHROW(cancelled->subscribeObjectClassAttributes(server, efficiencyOnly));
  // Publishing an ordinary attribute establishes implicit publication of the
  // standard HLAprivilegeToDeleteObject attribute for this class epoch. Its
  // subscriber therefore proves that registration snapshots the complete
  // currently published set, rather than only the explicit arguments.
  REQUIRE_NOTHROW(
      implicitPrivilege->subscribeObjectClassAttributes(server, privilegeToDeleteOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, publishedAttributes));

  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(objectInstanceName.empty());
  REQUIRE(publisher->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);

  // HLA_IMMEDIATE gets its induced discovery callback during registration;
  // HLA_EVOKED recipients remain unknown until their callback is evoked.
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exactReports.objectDiscoveryReports.empty());
  REQUIRE(promotedReports.objectDiscoveryReports.empty());
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE(implicitPrivilegeReports.objectDiscoveryReports.empty());
  REQUIRE(lateReports.objectDiscoveryReports.empty());
  REQUIRE_THROWS_AS(
      exact->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      late->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  // A queued callback must re-evaluate eligibility after an unsubscribe. A
  // later subscription to an already-registered instance must independently
  // plan the new discovery callback.
  REQUIRE_NOTHROW(cancelled->unsubscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(late->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(implicitPrivilege->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(late->evokeMultipleCallbacks(0.0, 0.0));

  auto requireDiscovery = [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report,
                              ObjectClassHandle const& expectedClass) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.objectClass == expectedClass);
    REQUIRE(report.objectInstanceName == objectInstanceName);
    REQUIRE(report.producingFederate == publisherHandle);
  };

  REQUIRE(exactReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(exactReports.objectDiscoveryReports.front(), server);
  REQUIRE(promotedReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(promotedReports.objectDiscoveryReports.front(), employee);
  REQUIRE(implicitPrivilegeReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(implicitPrivilegeReports.objectDiscoveryReports.front(), server);
  REQUIRE(lateReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(lateReports.objectDiscoveryReports.front(), server);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(immediateReports.objectDiscoveryReports.front(), server);
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE(publisherReports.objectDiscoveryReports.empty());

  REQUIRE(exact->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(promoted->getKnownObjectClassHandle(objectInstance) == employee);
  REQUIRE(implicitPrivilege->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(late->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(immediate->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(exact->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(promoted->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      cancelled->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  // Known-instance state is stable, so repeating a declaration does not
  // generate a duplicate discovery callback.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(exactReports.objectDiscoveryReports.size() == 1);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(late->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(implicitPrivilege->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(implicitPrivilege->disconnect());
  REQUIRE_NOTHROW(late->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded 2025 named object-instance registration consumes reservations",
    "[integration][development-profile][object-management]"
    "[rti.service.register-object-instance-named]"
    "[rti.service.register-object-instance-with-regions-named]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.release-object-instance-name]"
    "[federate.callback.discover-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;

  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass, L"Named-Not-Connected"),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass, L"Named-Not-Joined"),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"named-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"named-peer", L"peer", federationName));
  // Reservation callbacks are the subject here; disable the independent
  // declaration-relevance stream before making the declarations below.
  REQUIRE_NOTHROW(owner->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(owner->setInteractionRelevanceAdvisorySwitch(false));

  auto const employee = owner->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = owner->getAttributeHandle(employee, L"Name");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const publishedAttributes{name, efficiency};

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, publishedAttributes));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, nameOnly));

  std::wstring const firstName = L"UmbraNamedEmployee-1";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(firstName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
          firstName);

  // Reservation ownership is enforced at registration; the peer has
  // published the class but cannot consume the owner's reservation.
  REQUIRE_THROWS_AS(
      peer->registerObjectInstance(server, firstName),
      rti1516_2025::ObjectInstanceNameNotReserved);

  ObjectInstanceHandle firstInstance;
  REQUIRE_NOTHROW(firstInstance = owner->registerObjectInstance(server, firstName));
  REQUIRE(firstInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(firstInstance) == firstName);
  REQUIRE(owner->getObjectInstanceHandle(firstName) == firstInstance);
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(firstName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_THROWS_AS(
      peer->registerObjectInstance(server, firstName),
      rti1516_2025::ObjectInstanceNameInUse);

  REQUIRE(peerReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);
  REQUIRE(peerReports.objectDiscoveryReports.front().objectInstance == firstInstance);
  REQUIRE(peerReports.objectDiscoveryReports.front().objectClass == server);
  REQUIRE(peerReports.objectDiscoveryReports.front().objectInstanceName == firstName);
  // Registration discovery also solicits the owner under the official
  // Restaurant FOM Auto Provide switch. Drain that callback before the next
  // reservation assertion.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  // A failed registration does not consume a valid reservation. Once the
  // class is published, the same name can be used successfully.
  std::wstring const secondName = L"UmbraNamedEmployee-2";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(secondName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(employee, secondName),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(employee, nameOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(employee, nameOnly));

  ObjectInstanceHandle secondInstance;
  REQUIRE_NOTHROW(secondInstance = owner->registerObjectInstance(employee, secondName));
  REQUIRE(secondInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(secondInstance) == secondName);
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 2);
  REQUIRE(peerReports.objectDiscoveryReports.back().objectInstance == secondInstance);
  REQUIRE(peerReports.objectDiscoveryReports.back().objectClass == employee);
  REQUIRE(peerReports.objectDiscoveryReports.back().objectInstanceName == secondName);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  // A regional named registration consumes the same reservation state after
  // the existing 2025 regional validation path has accepted its association.
  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      ownerRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  REQUIRE_THROWS_AS(
      owner->registerObjectInstanceWithRegions(
          soda,
          regionalPair,
          L"UmbraRegionalUnreserved"),
      rti1516_2025::ObjectInstanceNameNotReserved);

  std::wstring const regionalName = L"UmbraRegional-1";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(regionalName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  ObjectInstanceHandle regionalInstance;
  REQUIRE_NOTHROW(regionalInstance = owner->registerObjectInstanceWithRegions(
      soda,
      regionalPair,
      regionalName));
  REQUIRE(regionalInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(regionalInstance) == regionalName);
  REQUIRE(owner->getObjectInstanceHandle(regionalName) == regionalInstance);
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(regionalName),
      rti1516_2025::ObjectInstanceNameNotReserved);

  // Release before registration returns the name to the federation-wide
  // pool, allowing a different joined federate to reserve and consume it.
  std::wstring const reusableName = L"UmbraNamedReusable";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(reusableName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(reusableName));
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(reusableName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  ObjectInstanceHandle peerInstance;
  REQUIRE_NOTHROW(peerInstance = peer->registerObjectInstance(server, reusableName));
  REQUIRE(peerInstance.isValid());
  REQUIRE(peer->getObjectInstanceName(peerInstance) == reusableName);
  REQUIRE_THROWS_AS(
      peer->releaseObjectInstanceName(reusableName),
      rti1516_2025::ObjectInstanceNameNotReserved);

  REQUIRE_NOTHROW(peer->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded receive-order Delete Object Instance honors 2025 removal lifecycle",
    "[integration][development-profile][object-management]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;

  // The 2025 service preserves connection and membership preconditions ahead
  // of caller-supplied handle validation.
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"delete-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(evoked->joinFederationExecution(
      L"delete-evoked", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"delete-immediate", L"subscriber", federationName));

  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(server));
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evokedReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evoked->getKnownObjectClassHandle(objectInstance) == server);

  unsigned char const deleteTagBytes[] = {0xD3, 0x1E, 0x7E};
  VariableLengthData const deleteTag(deleteTagBytes, sizeof(deleteTagBytes));
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(invalidObjectInstance, deleteTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      evoked->deleteObjectInstance(objectInstance, deleteTag),
      rti1516_2025::DeletePrivilegeNotHeld);

  // The deleting producer is immediately unknown and never receives its own
  // induced removal. An evoked recipient remains known until that callback
  // begins; an immediate recipient observes removal during the service call.
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(objectInstance, deleteTag));
  REQUIRE(publisherReports.objectRemovalReports.empty());
  REQUIRE(immediateReports.objectRemovalReports.size() == 1);
  REQUIRE(evokedReports.objectRemovalReports.empty());
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(evoked->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      immediate->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  auto requireRemoval = [&](ReportingFederateAmbassador::ObjectRemovalReport const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>{deleteTagBytes[0], deleteTagBytes[1], deleteTagBytes[2]});
    REQUIRE(report.producingFederate == publisherHandle);
  };
  requireRemoval(immediateReports.objectRemovalReports.front());

  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.objectRemovalReports.size() == 1);
  requireRemoval(evokedReports.objectRemovalReports.front());
  REQUIRE_THROWS_AS(
      evoked->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(objectInstance, deleteTag),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded Local Delete Object Instance preserves 2025 federation state",
    "[integration][development-profile][object-management]"
    "[rti.service.local-delete-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;

  REQUIRE_THROWS_AS(
      owner->localDeleteObjectInstance(invalidObjectInstance),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->localDeleteObjectInstance(invalidObjectInstance),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"local-delete-owner", L"owner", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"local-delete-requester", L"requester", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == server);

  // The producer owns the registration-established attribute and therefore
  // cannot use Local Delete Object Instance until ownership has moved away.
  REQUIRE_THROWS_AS(
      owner->localDeleteObjectInstance(objectInstance),
      rti1516_2025::FederateOwnsAttributes);

  unsigned char const tagBytes[] = {0x51, 0x18};
  VariableLengthData const acquisitionTag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->localDeleteObjectInstance(objectInstance),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_NOTHROW(requester->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));

  // Rejoin a fresh requester so the local-delete transition itself is tested
  // without carrying an acquisition reservation across the teardown boundary.
  requester = makeRti();
  requesterReports = ReportingFederateAmbassador{};
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"local-delete-requester-2", L"requester", federationName));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(requester->localDeleteObjectInstance(objectInstance));
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  // Local deletion does not remove the federation-wide object. Repeating the
  // eligible subscription permits a new discovery, and the owner remains able
  // to update the object for the rediscovered requester.
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 2);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == server);

  unsigned char const valueBytes[] = {0x04, 0x20};
  unsigned char const updateTagBytes[] = {0x7A};
  AttributeHandleValueMap values;
  values.emplace(efficiency, VariableLengthData(valueBytes, sizeof(valueBytes)));
  VariableLengthData const updateTag(updateTagBytes, sizeof(updateTagBytes));
  REQUIRE_NOTHROW(owner->updateAttributeValues(objectInstance, values, updateTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1);
  REQUIRE(requesterReports.attributeReflectionReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded receive-order Update Attribute Values honors 2025 passel and callback lifecycle",
    "[integration][development-profile][object-management]"
    "[rti.service.update-attribute-values][federate.callback.reflect-attribute-values]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto cancelled = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xC0, 0x25, 0xA4};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleValueMap noAttributeValues;

  // The service retains its connection and membership preconditions ahead of
  // validation of its object handle and value map.
  REQUIRE_THROWS_AS(
      unjoined->updateAttributeValues(invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->updateAttributeValues(invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"attribute-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"attribute-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"attribute-cancelled", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"attribute-immediate", L"subscriber", federationName));

  auto const base = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = publisher->getAttributeHandle(child, L"ReliableBaseB");
  auto const bestEffortBase = publisher->getAttributeHandle(child, L"BestEffortBase");
  auto const reliableChild = publisher->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = publisher->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(bestEffortBase.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const allOwned{
      reliableBaseA,
      reliableBaseB,
      bestEffortBase,
      reliableChild,
  };
  AttributeHandleSet const baseAttributes{
      reliableBaseA,
      reliableBaseB,
      bestEffortBase,
  };
  AttributeHandleSet const reliableChildOnly{reliableChild};

  // The promoted receiver uses an active superclass subscription so it is
  // eligible for both discovery and the projected base-attribute reflections.
  // The cancelled receiver starts eligible so that its later unsubscribe
  // exercises callback-time suppression.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(base, baseAttributes, true));
  REQUIRE_NOTHROW(cancelled->subscribeObjectClassAttributes(child, reliableChildOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, allOwned));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exactReports.objectDiscoveryReports.empty());
  REQUIRE(promotedReports.objectDiscoveryReports.empty());
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(exactReports.objectDiscoveryReports.size() == 1);
  REQUIRE(promotedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(cancelledReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exact->getKnownObjectClassHandle(objectInstance) == child);
  REQUIRE(promoted->getKnownObjectClassHandle(objectInstance) == base);
  REQUIRE(cancelled->getKnownObjectClassHandle(objectInstance) == child);

  unsigned char const reliableBaseABytes[] = {0x01, 0x02};
  unsigned char const reliableBaseBBytes[] = {0x03, 0x04};
  unsigned char const bestEffortBaseBytes[] = {0x05, 0x06};
  unsigned char const reliableChildBytes[] = {0x07, 0x08};
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      reliableBaseA,
      VariableLengthData(reliableBaseABytes, sizeof(reliableBaseABytes)));
  attributeValues.emplace(
      reliableBaseB,
      VariableLengthData(reliableBaseBBytes, sizeof(reliableBaseBBytes)));
  attributeValues.emplace(
      bestEffortBase,
      VariableLengthData(bestEffortBaseBytes, sizeof(bestEffortBaseBytes)));
  attributeValues.emplace(
      reliableChild,
      VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));

  AttributeHandleValueMap unownedValues;
  unownedValues.emplace(unownedChild, VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(invalidObjectInstance, attributeValues, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandle invalidAttribute;
  AttributeHandleValueMap invalidAttributeValues;
  invalidAttributeValues.emplace(invalidAttribute, VariableLengthData());
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, invalidAttributeValues, tag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, unownedValues, tag),
      rti1516_2025::AttributeNotOwned);
  AttributeHandleValueMap exactValues;
  exactValues.emplace(reliableChild, VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));
  REQUIRE_THROWS_AS(
      exact->updateAttributeValues(objectInstance, exactValues, tag),
      rti1516_2025::AttributeNotOwned);

  // One no-time request contains two immutable passels: the three reliable
  // values stay together, and the best-effort value remains separate. The
  // child-only attribute is deliberately absent from the promoted receiver's
  // known superclass projection.
  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, attributeValues, tag));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  REQUIRE(exactReports.attributeReflectionReports.empty());
  REQUIRE(promotedReports.attributeReflectionReports.empty());
  REQUIRE(cancelledReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(cancelled->unsubscribeObjectClassAttributes(child, reliableChildOnly));
  // With a zero maximum interval, the callback model processes one queued
  // callback per invocation and reports whether another passel remains.
  REQUIRE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));

  auto const reliableTransportation = publisher->getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffortTransportation = publisher->getTransportationTypeHandle(L"HLAbestEffort");
  auto reflectionFor = [](
                           std::vector<ReportingFederateAmbassador::AttributeReflectionReport> const& reports,
                           TransportationTypeHandle const& transportationType) {
    auto const found = std::find_if(
        reports.begin(),
        reports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.transportationType == transportationType;
        });
    REQUIRE(found != reports.end());
    return &*found;
  };
  auto requireReflection = [&](ReportingFederateAmbassador::AttributeReflectionReport const& report,
                               TransportationTypeHandle const& transportationType,
                               std::vector<AttributeHandle> const& expectedAttributes) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == expectedAttributes.size());
    for (AttributeHandle const& attribute : expectedAttributes) {
      auto const expected = attributeValues.find(attribute);
      REQUIRE(expected != attributeValues.end());
      auto const received = report.attributeValues.find(attribute);
      REQUIRE(received != report.attributeValues.end());
      REQUIRE(variableLengthDataBytes(received->second) == variableLengthDataBytes(expected->second));
    }
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == transportationType);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  };

  REQUIRE(exactReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(exactReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB, reliableChild});
  requireReflection(
      *reflectionFor(exactReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(promotedReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(promotedReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB});
  requireReflection(
      *reflectionFor(promotedReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(immediateReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB, reliableChild});
  requireReflection(
      *reflectionFor(immediateReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(cancelledReports.attributeReflectionReports.empty());
  REQUIRE(publisherReports.attributeReflectionReports.empty());

  // Unpublishing the whole class removes the producer's ownership of every
  // corresponding instance attribute, so a later update is rejected at the
  // official AttributeNotOwned boundary rather than using stale state.
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(child));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, attributeValues, tag),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded regional object attributes filter 2025 no-time updates by overlap",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-object-subscriber", L"subscriber", federationName));
  REQUIRE_FALSE(subscriber->getConveyRegionDesignatorSetsSwitch());

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const emptyRegionPair{{
      flavorOnly,
      RegionHandleSet{},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      regionalPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(
      objectInstance,
      emptyRegionPair));
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());

  unsigned char const firstValueBytes[] = {0x10, 0x25};
  AttributeHandleValueMap firstValue;
  firstValue.emplace(
      flavor,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      firstValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  // Association is additive and idempotent. The disjoint subscriber remains
  // undiscoverable even after the producer repeats the association explicitly.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == soda);

  unsigned char const secondValueBytes[] = {0x20, 0x25};
  AttributeHandleValueMap secondValue;
  secondValue.emplace(
      flavor,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      secondValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);
  REQUIRE_FALSE(subscriberReports.attributeReflectionReports.front().sentRegionsSupplied);
  REQUIRE(
      subscriberReports.attributeReflectionReports.front().attributeValues.contains(flavor));

  // The switch is recipient-local and may be changed after a federation has
  // joined.  Enabling it exposes the same update-region realization on the
  // next reflection without changing regional overlap routing.
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));
  unsigned char const conveyedValueBytes[] = {0x25, 0x20};
  AttributeHandleValueMap conveyedValue;
  conveyedValue.emplace(
      flavor,
      VariableLengthData(conveyedValueBytes, sizeof(conveyedValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      conveyedValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegions.contains(publisherRegion));

  // Changing the committed subscriber region to a disjoint range removes the
  // regional reflection route without changing ordinary known-instance state.
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  unsigned char const disjointValueBytes[] = {0x30, 0x25};
  AttributeHandleValueMap disjointValue;
  disjointValue.emplace(
      flavor,
      VariableLengthData(disjointValueBytes, sizeof(disjointValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      disjointValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, emptyRegionPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded Allow Relaxed DDM expands only touching regional object-attribute ranges",
    "[integration][development-profile][object-management][ddm][allow-relaxed-ddm]"
    "[rti.service.get-allow-relaxed-ddm-switch]"
    "[rti.service.create-region]"
    "[rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  auto runScenario = [](bool const relaxedDdmEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador subscriberReports;
    auto publisher = makeRti();
    auto subscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const restaurantFom =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const relaxedDdmFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                "cpp" / "tests" / "data" /
                                "allow-relaxed-ddm-enabled-fom.xml")
                                   .wstring();
    std::vector<std::wstring> fomModules{restaurantFom};
    if (relaxedDdmEnabled) {
      fomModules.push_back(relaxedDdmFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"relaxed-ddm-object-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"relaxed-ddm-object-subscriber", L"subscriber", federationName));
    REQUIRE(publisher->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);
    REQUIRE(subscriber->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);

    auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
    auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
    auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
    REQUIRE(soda.isValid());
    REQUIRE(flavor.isValid());
    REQUIRE(sodaFlavor.isValid());
    AttributeHandleSet const flavorOnly{flavor};
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

    auto const sourceRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
    auto const subscriptionRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        sodaFlavor,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        sodaFlavor,
        RangeBounds(2UL, 3UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
        flavorOnly,
        RegionHandleSet{sourceRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{{
        flavorOnly,
        RegionHandleSet{subscriptionRegion},
    }};
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
        soda,
        sourcePair));
    REQUIRE(objectInstance.isValid());
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        soda,
        subscriptionPair));
    while (subscriber->evokeCallback(0.0)) {
    }

    // The exact-boundary pair can establish a known instance and reflection
    // route only with Umbra's enabled Relaxed DDM policy.
    std::size_t expectedDiscoveries = relaxedDdmEnabled ? 1U : 0U;
    std::size_t expectedReflections = relaxedDdmEnabled ? 1U : 0U;
    REQUIRE(subscriberReports.objectDiscoveryReports.size() == expectedDiscoveries);
    unsigned char const touchingValueBytes[] = {0x31, 0x42};
    AttributeHandleValueMap touchingValue;
    touchingValue.emplace(
        flavor,
        VariableLengthData(touchingValueBytes, sizeof(touchingValueBytes)));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        touchingValue,
        VariableLengthData()));
    while (subscriber->evokeCallback(0.0)) {
    }
    REQUIRE(subscriberReports.attributeReflectionReports.size() == expectedReflections);

    // A positive gap removes the route even when the object is already known
    // from a previous relaxed discovery.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        sodaFlavor,
        RangeBounds(3UL, 4UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    unsigned char const gapValueBytes[] = {0x53, 0x64};
    AttributeHandleValueMap gapValue;
    gapValue.emplace(flavor, VariableLengthData(gapValueBytes, sizeof(gapValueBytes)));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        gapValue,
        VariableLengthData()));
    while (subscriber->evokeCallback(0.0)) {
    }
    REQUIRE(subscriberReports.attributeReflectionReports.size() == expectedReflections);

    // Restoring strict overlap must discover the disabled-profile recipient
    // and preserve delivery in the enabled profile; it must never be removed
    // by the relaxation policy.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        sodaFlavor,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        soda,
        subscriptionPair));
    while (subscriber->evokeCallback(0.0)) {
    }
    if (!relaxedDdmEnabled) {
      ++expectedDiscoveries;
    }
    REQUIRE(subscriberReports.objectDiscoveryReports.size() == expectedDiscoveries);
    unsigned char const strictValueBytes[] = {0x75, 0x86};
    AttributeHandleValueMap strictValue;
    strictValue.emplace(
        flavor,
        VariableLengthData(strictValueBytes, sizeof(strictValueBytes)));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        strictValue,
        VariableLengthData()));
    while (subscriber->evokeCallback(0.0)) {
    }
    ++expectedReflections;
    REQUIRE(subscriberReports.attributeReflectionReports.size() == expectedReflections);

    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        soda,
        subscriptionPair));
    REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the FDD enables Relaxed DDM") {
    runScenario(true);
  }
  SECTION("the FDD leaves Relaxed DDM disabled") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded default-region object routing derives 2025 ordinary and regional effectiveness",
    "[integration][development-profile][object-management][ddm][default-region]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador regionalReports;
  ReportingFederateAmbassador mixedReports;
  auto publisher = makeRti();
  auto regional = makeRti();
  auto mixed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regional->connect(regionalReports, HLA_EVOKED));
  REQUIRE_NOTHROW(mixed->connect(mixedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"default-region-object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(regional->joinFederationExecution(
      L"default-region-object-regional", L"subscriber", federationName));
  REQUIRE_NOTHROW(mixed->joinFederationExecution(
      L"default-region-object-mixed", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const regionalRegion = regional->createRegion(DimensionHandleSet{sodaFlavor});
  auto const mixedRegion = mixed->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(regional->setRangeBounds(
      regionalRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(regional->commitRegionModifications(RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->setRangeBounds(
      mixedRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(mixed->commitRegionModifications(RegionHandleSet{mixedRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{regionalRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const mixedPair{{
      flavorOnly,
      RegionHandleSet{mixedRegion},
  }};

  REQUIRE_NOTHROW(regional->subscribeObjectClassAttributesWithRegions(soda, regionalPair));
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributes(soda, flavorOnly, true));
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributesWithRegions(soda, mixedPair));

  // An explicit source region makes the mixed subscriber's explicit regional
  // declaration effective. Its retained ordinary declaration must not route
  // through the default region while that disjoint declaration exists.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      sourcePair));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.objectDiscoveryReports.size() == 1);
  REQUIRE(regionalReports.objectDiscoveryReports.back().objectInstance == objectInstance);
  REQUIRE(mixedReports.objectDiscoveryReports.empty());

  // Removing the non-default realization lets the retained ordinary
  // declaration use the default region again. Reapplying that declaration
  // asks the RTI to reconsider already registered but previously undiscovered
  // instances.
  REQUIRE_NOTHROW(mixed->unsubscribeObjectClassAttributesWithRegions(soda, mixedPair));
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributes(soda, flavorOnly, true));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(mixedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(mixedReports.objectDiscoveryReports.back().objectInstance == objectInstance);

  // Restore the disjoint regional declaration and remove the source's only
  // explicit association. Both regional subscribers now overlap the invisible
  // default region; no public RegionHandle is synthesized for it.
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributesWithRegions(soda, mixedPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(regional->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(mixed->setConveyRegionDesignatorSetsSwitch(true));

  unsigned char const defaultValueBytes[] = {0xD0, 0x01};
  AttributeHandleValueMap defaultValues;
  defaultValues.emplace(
      flavor,
      VariableLengthData(defaultValueBytes, sizeof(defaultValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      defaultValues,
      VariableLengthData()));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.attributeReflectionReports.size() == 1);
  REQUIRE(mixedReports.attributeReflectionReports.size() == 1);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegions.empty());
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegions.empty());

  // A later explicit association replaces the source default for this
  // attribute. The matching regional subscriber receives it; the mixed
  // subscriber's disjoint regional declaration continues to suppress its
  // retained ordinary declaration.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, sourcePair));
  unsigned char const explicitValueBytes[] = {0xE0, 0x02};
  AttributeHandleValueMap explicitValues;
  explicitValues.emplace(
      flavor,
      VariableLengthData(explicitValueBytes, sizeof(explicitValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      explicitValues,
      VariableLengthData()));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.attributeReflectionReports.size() == 2);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegions.contains(sourceRegion));
  REQUIRE(mixedReports.attributeReflectionReports.size() == 1);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
  unsigned char const restoredDefaultBytes[] = {0xD0, 0x03};
  AttributeHandleValueMap restoredDefaultValues;
  restoredDefaultValues.emplace(
      flavor,
      VariableLengthData(restoredDefaultBytes, sizeof(restoredDefaultBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      restoredDefaultValues,
      VariableLengthData()));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.attributeReflectionReports.size() == 3);
  REQUIRE(mixedReports.attributeReflectionReports.size() == 2);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegions.empty());
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegions.empty());

  REQUIRE_NOTHROW(mixed->unsubscribeObjectClassAttributesWithRegions(soda, mixedPair));
  REQUIRE_NOTHROW(mixed->unsubscribeObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(regional->unsubscribeObjectClassAttributesWithRegions(soda, regionalPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(regional->deleteRegion(regionalRegion));
  REQUIRE_NOTHROW(mixed->deleteRegion(mixedRegion));
  REQUIRE_NOTHROW(mixed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regional->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(mixed->disconnect());
  REQUIRE_NOTHROW(regional->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped default-region attribute update preserves 2025 regional callback metadata",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[default-region][tso][rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0xB1, 0x6E};
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F, 0x2D, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-attribute-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  // Ordinary registration and updates have no public source RegionHandle.
  // The receiver nevertheless discovers the dimensional object through the
  // derived default region before the timestamped update is accepted.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(soda));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1);

  auto const& report = receiverReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1);
  REQUIRE(report.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded passive object-attribute subscriptions do not arrange ordinary or regional delivery",
    "[integration][development-profile][object-management][ddm][passive-subscription]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"passive-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"passive-attribute-subscriber", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};

  // Passive ordinary declarations are retained, but do not arrange instance
  // discovery. Replacing the same declaration with an active one does.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, false));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());
  REQUIRE_THROWS_AS(
      subscriber->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, true));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(subscriberReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == soda);

  unsigned char const passiveOrdinaryBytes[] = {0x10, 0x20};
  AttributeHandleValueMap passiveOrdinaryValues;
  passiveOrdinaryValues.emplace(
      flavor,
      VariableLengthData(passiveOrdinaryBytes, sizeof(passiveOrdinaryBytes)));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, false));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      passiveOrdinaryValues,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  unsigned char const activeOrdinaryBytes[] = {0x30, 0x40};
  AttributeHandleValueMap activeOrdinaryValues;
  activeOrdinaryValues.emplace(
      flavor,
      VariableLengthData(activeOrdinaryBytes, sizeof(activeOrdinaryBytes)));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, true));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      activeOrdinaryValues,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);
  REQUIRE(subscriberReports.attributeReflectionReports.front().objectInstance == objectInstance);
  REQUIRE(
      subscriberReports.attributeReflectionReports.front().attributeValues.contains(flavor));

  // The same active/passive rule applies to an overlapping regional pair.
  // Ordinary and regional declarations are independent, so remove the
  // ordinary declaration before exercising the regional state.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(soda, flavorOnly));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair,
      false));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));

  ObjectInstanceHandle passiveRegionalObject;
  REQUIRE_NOTHROW(passiveRegionalObject = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE_THROWS_AS(
      subscriber->getKnownObjectClassHandle(passiveRegionalObject),
      rti1516_2025::ObjectInstanceNotKnown);

  unsigned char const passiveRegionalBytes[] = {0x50, 0x60};
  AttributeHandleValueMap passiveRegionalValues;
  passiveRegionalValues.emplace(
      flavor,
      VariableLengthData(passiveRegionalBytes, sizeof(passiveRegionalBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      passiveRegionalValues,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);

  unsigned char const activeRegionalBytes[] = {0x70, 0x80};
  AttributeHandleValueMap activeRegionalValues;
  activeRegionalValues.emplace(
      flavor,
      VariableLengthData(activeRegionalBytes, sizeof(activeRegionalBytes)));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair,
      true));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 2);
  REQUIRE(subscriberReports.objectDiscoveryReports.back().objectInstance == passiveRegionalObject);
  REQUIRE(subscriber->getKnownObjectClassHandle(passiveRegionalObject) == soda);
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      activeRegionalValues,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);
  REQUIRE(subscriberReports.attributeReflectionReports.back().objectInstance == objectInstance);
  REQUIRE(subscriberReports.attributeReflectionReports.back().attributeValues.contains(flavor));

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(passiveRegionalObject, publisherPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded ownership transfer clears the former owner's 2025 update-region association",
    "[integration][development-profile][federation-management][ddm][ownership-management]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador acquirerReports;
  ReportingFederateAmbassador regionalSubscriberReports;
  auto owner = makeRti();
  auto acquirer = makeRti();
  auto regionalSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(acquirer->connect(acquirerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regionalSubscriber->connect(regionalSubscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-ownership-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(acquirer->joinFederationExecution(
      L"regional-ownership-acquirer", L"publisher", federationName));
  REQUIRE_NOTHROW(regionalSubscriber->joinFederationExecution(
      L"regional-ownership-subscriber", L"subscriber", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const formerOwnerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      formerOwnerRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{formerOwnerRegion}));

  auto const subscriberRegion =
      regionalSubscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(regionalSubscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(
      regionalSubscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  AttributeHandleSetRegionHandleSetPairVector const formerOwnerPair{{
      flavorOnly,
      RegionHandleSet{formerOwnerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};
  REQUIRE_NOTHROW(regionalSubscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      formerOwnerPair));
  REQUIRE_FALSE(regionalSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalSubscriberReports.objectDiscoveryReports.size() == 1);

  // The acquirer is known through an ordinary subscription and publishes the
  // selected attribute before entering the bounded If Available transfer.
  REQUIRE_NOTHROW(acquirer->subscribeObjectClassAttributes(soda, flavorOnly));
  REQUIRE_FALSE(acquirer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(acquirerReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(acquirer->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(acquirer->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      flavorOnly,
      VariableLengthData()));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      flavorOnly,
      VariableLengthData(),
      divestedAttributes));
  REQUIRE(divestedAttributes == flavorOnly);
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, flavor));
  while (acquirer->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(acquirerReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(acquirer->isAttributeOwnedByFederate(objectInstance, flavor));

  unsigned char const staleAssociationValueBytes[] = {0x71, 0x25};
  AttributeHandleValueMap staleAssociationValue;
  staleAssociationValue.emplace(
      flavor,
      VariableLengthData(
          staleAssociationValueBytes,
          sizeof(staleAssociationValueBytes)));
  REQUIRE_NOTHROW(acquirer->updateAttributeValues(
      objectInstance,
      staleAssociationValue,
      VariableLengthData()));
  REQUIRE_FALSE(regionalSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalSubscriberReports.attributeReflectionReports.size() == 1);
  REQUIRE_FALSE(regionalSubscriberReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(regionalSubscriberReports.attributeReflectionReports.back()
              .attributeValues.contains(flavor));

  // The former association is gone at the ownership boundary, restoring the
  // default region. A new owner can then associate its own explicit region;
  // that replaces the default source realization for future reflections.
  auto const acquiringRegion = acquirer->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(acquirer->setRangeBounds(
      acquiringRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(acquirer->commitRegionModifications(RegionHandleSet{acquiringRegion}));
  AttributeHandleSetRegionHandleSetPairVector const acquiringPair{{
      flavorOnly,
      RegionHandleSet{acquiringRegion},
  }};
  REQUIRE_NOTHROW(acquirer->associateRegionsForUpdates(objectInstance, acquiringPair));

  unsigned char const currentAssociationValueBytes[] = {0x72, 0x25};
  AttributeHandleValueMap currentAssociationValue;
  currentAssociationValue.emplace(
      flavor,
      VariableLengthData(
          currentAssociationValueBytes,
          sizeof(currentAssociationValueBytes)));
  REQUIRE_NOTHROW(acquirer->updateAttributeValues(
      objectInstance,
      currentAssociationValue,
      VariableLengthData()));
  REQUIRE_FALSE(regionalSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalSubscriberReports.attributeReflectionReports.size() == 2);
  REQUIRE(regionalSubscriberReports.attributeReflectionReports.back()
              .attributeValues.contains(flavor));

  REQUIRE_NOTHROW(acquirer->unassociateRegionsForUpdates(objectInstance, acquiringPair));
  REQUIRE_NOTHROW(regionalSubscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(acquirer->unpublishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(acquirer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regionalSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(acquirer->disconnect());
  REQUIRE_NOTHROW(regionalSubscriber->disconnect());
}

TEST_CASE(
    "Embedded regional object scope callbacks follow 2025 region, association, and subscription changes",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[federate.callback.attributes-in-scope]"
    "[federate.callback.attributes-out-of-scope]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto owner = makeRti();
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"scope-owner", L"scope-owner", federationName));
  REQUIRE_NOTHROW(evoked->joinFederationExecution(
      L"scope-evoked", L"scope-evoked", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"scope-immediate", L"scope-immediate", federationName));
  REQUIRE_FALSE(evoked->getAttributeScopeAdvisorySwitch());
  REQUIRE_FALSE(immediate->getAttributeScopeAdvisorySwitch());
  REQUIRE_NOTHROW(evoked->setAttributeScopeAdvisorySwitch(true));
  REQUIRE_NOTHROW(immediate->setAttributeScopeAdvisorySwitch(true));
  REQUIRE(evoked->getAttributeScopeAdvisorySwitch());
  REQUIRE(immediate->getAttributeScopeAdvisorySwitch());

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const evokedRegion = evoked->createRegion(DimensionHandleSet{sodaFlavor});
  auto const immediateRegion = immediate->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const evokedPair{{
      flavorOnly,
      RegionHandleSet{evokedRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const immediatePair{{
      flavorOnly,
      RegionHandleSet{immediateRegion},
  }};
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributesWithRegions(soda, immediatePair));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evokedReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evoked->getKnownObjectClassHandle(objectInstance) == soda);
  REQUIRE(immediate->getKnownObjectClassHandle(objectInstance) == soda);
  REQUIRE(evokedReports.attributesInScopeReports.empty());
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(immediateReports.attributesInScopeReports.empty());
  REQUIRE(immediateReports.attributesOutOfScopeReports.empty());

  // Removing the last explicit association restores the default region. It
  // spans every available FDD dimension, so these matching regional
  // subscribers remain in scope and no scope transition is generated.
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(immediateReports.attributesOutOfScopeReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());

  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(immediateReports.attributesInScopeReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.empty());
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // Replacing a matching explicit association with the default and back again
  // also leaves scope unchanged: default-region effectiveness is derived, not
  // represented by a user-visible region association.
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(immediateReports.attributesOutOfScopeReports.empty());
  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(immediateReports.attributesInScopeReports.empty());
  static_cast<void>(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(evokedReports.attributesInScopeReports.empty());
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // Subscription declarations remain independently retained. Their effective
  // default and regional realizations determine the official in/out callbacks
  // and callback-time recheck.
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributesWithRegions(soda, immediatePair));
  REQUIRE(immediateReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().attributes == flavorOnly);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributesWithRegions(soda, immediatePair));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // Subscription work is stale-checked too: an evoked out-of-scope
  // transition is suppressed when the same regional subscription is restored
  // before callback dispatch.
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();

  // Ordinary subscription state remains retained separately from regional
  // state. With this explicit source region, the evoked federate crosses out
  // of regional scope, into ordinary scope, back out, and finally into
  // regional scope again.
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  evokedReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributes(soda, flavorOnly, true, L""));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  evokedReports.attributesInScopeReports.clear();
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributes(soda, flavorOnly));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  evokedReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  evokedReports.attributesInScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // A committed disjoint subscriber region produces one evoked out-of-scope
  // callback, while the immediate subscriber is entered synchronously.
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE(immediateReports.attributesOutOfScopeReports.front().objectInstance == objectInstance);
  REQUIRE(immediateReports.attributesOutOfScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE(immediateReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(immediateReports.attributesInScopeReports.front().attributes == flavorOnly);

  // Two queued transitions collapse at callback time: the stale out-of-scope
  // work is suppressed after the region has returned to its committed overlap.
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);

  // The advisory switch gates notification generation. Re-enabling it does
  // not manufacture a callback until a subsequent committed transition.
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(immediate->setAttributeScopeAdvisorySwitch(false));
  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesOutOfScopeReports.empty());
  REQUIRE_NOTHROW(immediate->setAttributeScopeAdvisorySwitch(true));
  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE(immediateReports.attributesInScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributesWithRegions(soda, immediatePair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(evoked->deleteRegion(evokedRegion));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded object-instance Request Attribute Value Update solicits 2025 owners",
    "[integration][development-profile][object-management]"
    "[rti.service.request-attribute-value-update][federate.callback.provide-attribute-value-update]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD7, 0x4E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectInstance, noAttributes, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectInstance, noAttributes, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"provide-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"provide-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(invalidObjectInstance, ownedAttributes, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandle invalidAttribute;
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(objectInstance, invalidAttributes, tag),
      rti1516_2025::AttributeNotDefined);

  // Defined-but-unowned attributes do not solicit a Provide callback. The
  // requester knows this instance as Child, so the no-callback outcome is not
  // a class-availability failure.
  AttributeHandleSet const unownedAttributes{unownedChild};
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, unownedAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  // One request with two attributes owned by the same federate induces at
  // most one Provide Attribute Value Update callback at that owner. The
  // requester itself never receives a corresponding callback.
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, ownedAttributes, tag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  auto const& provide = ownerReports.attributeValueUpdateRequestReports.front();
  REQUIRE(provide.objectInstance == objectInstance);
  REQUIRE(provide.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(provide.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());

  // Requesting an attribute already owned by the requester is an implicit
  // local provide and must not call the public provider callback on itself.
  REQUIRE_NOTHROW(owner->requestAttributeValueUpdate(objectInstance, ownedAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);

  // A queued owner callback is rechecked at delivery. Resignation removes the
  // provider's federation route, so stale work cannot invoke user code after
  // the membership transition.
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, ownedAttributes, tag));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded object-class Request Attribute Value Update solicits 2025 subclass owners",
    "[integration][development-profile][object-management]"
    "[rti.service.request-attribute-value-update][federate.callback.provide-attribute-value-update]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xC1, 0x25};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectClassHandle invalidObjectClass;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectClass, noAttributes, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectClass, noAttributes, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"class-provide-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"class-provide-requester", L"subscriber", federationName));

  auto const base = owner->getObjectClassHandle(L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = owner->getAttributeHandle(child, L"ReliableBaseB");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const providerAttributes{reliableBaseA, reliableBaseB, reliableChild};
  AttributeHandleSet const baseAttributes{reliableBaseA, reliableBaseB};
  AttributeHandleSet const childOnlyAttributes{reliableChild};
  AttributeHandleSet const unownedChildAttributes{unownedChild};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, providerAttributes));

  ObjectInstanceHandle objectInstance;
  ObjectInstanceHandle secondObjectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_NOTHROW(secondObjectInstance = owner->registerObjectInstance(child));
  // The class-designator form has no requester-known-instance precondition.
  // This requester has not subscribed, so discovery has not made the child
  // instance known locally before it asks for the base-class attribute.
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(secondObjectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(invalidObjectClass, baseAttributes, tag),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(base, childOnlyAttributes, tag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(child, unownedChildAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  // The base-class request expands over every registered Child instance and
  // invokes one owner callback for each particular object instance. Both base
  // attributes stay together in that one callback for each owner/instance.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(base, baseAttributes, tag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 2);
  auto const& firstProvide = ownerReports.attributeValueUpdateRequestReports[0];
  auto const& secondProvide = ownerReports.attributeValueUpdateRequestReports[1];
  REQUIRE(firstProvide.objectInstance == objectInstance);
  REQUIRE(secondProvide.objectInstance == secondObjectInstance);
  REQUIRE(firstProvide.attributes == baseAttributes);
  REQUIRE(secondProvide.attributes == baseAttributes);
  REQUIRE(variableLengthDataBytes(firstProvide.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(variableLengthDataBytes(secondProvide.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // Requester-owned attributes are an implicit local Provide, including when
  // the request expands from a class to its registered subclass instance.
  REQUIRE_NOTHROW(owner->requestAttributeValueUpdate(base, baseAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 2);

  // Class expansion is rechecked at delivery just like the instance form.
  // A queued owner callback cannot outlive the provider's resignation.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(base, baseAttributes, tag));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 2);
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded regional Request Attribute Value Update filters 2025 owner solicitations",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const tagBytes[] = {0x91, 0x25};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-request-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-requester", L"requester", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const requestRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const disjointRequestPair{{
      flavorOnly,
      RegionHandleSet{requestRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const emptyRequestPair{{
      flavorOnly,
      RegionHandleSet{},
  }};
  auto const serverId = requester->getDimensionHandle(L"ServerId");
  auto const wrongContextRegion = requester->createRegion(DimensionHandleSet{serverId});
  auto const uncommittedRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(requester->setRangeBounds(
      wrongContextRegion,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{wrongContextRegion}));
  AttributeHandleSetRegionHandleSetPairVector const foreignRequestPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const wrongContextRequestPair{{
      flavorOnly,
      RegionHandleSet{wrongContextRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const uncommittedRequestPair{{
      flavorOnly,
      RegionHandleSet{uncommittedRegion},
  }};

  ObjectInstanceHandle regionalObject;
  ObjectInstanceHandle defaultObject;
  REQUIRE_NOTHROW(regionalObject = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  REQUIRE_NOTHROW(defaultObject = owner->registerObjectInstance(soda));
  REQUIRE(regionalObject.isValid());
  REQUIRE(defaultObject.isValid());

  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, foreignRequestPair, tag),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, wrongContextRequestPair, tag),
      rti1516_2025::InvalidRegionContext);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, uncommittedRequestPair, tag),
      rti1516_2025::InvalidRegion);

  // An empty request-region set is a no-op for that class attribute.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      emptyRequestPair,
      tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  // The explicit [0,1) update association is disjoint from [2,3), while the
  // ordinary/default-region object remains eligible for a regional request.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().objectInstance == defaultObject);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().attributes == flavorOnly);
  REQUIRE(variableLengthDataBytes(
              ownerReports.attributeValueUpdateRequestReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // Recommitting the requester region into [0,1) makes both the explicit and
  // default-region instances eligible.
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 3);

  // Queue the same overlapping request, then move the committed request
  // region away before delivery. The explicit provider callback is suppressed
  // at callback entry while the default-region callback remains eligible.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 4);
  std::set<ObjectInstanceHandle> reportedObjects;
  for (auto const& report : ownerReports.attributeValueUpdateRequestReports) {
    REQUIRE(report.attributes == flavorOnly);
    reportedObjects.insert(report.objectInstance);
  }
  REQUIRE(reportedObjects == std::set<ObjectInstanceHandle>{regionalObject, defaultObject});

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(regionalObject, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(wrongContextRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(uncommittedRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(requestRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Request Attribute Value Update supports a 2025 provider response",
    "[integration][development-profile][object-management]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const requestTagBytes[] = {0xD7, 0x4E};
  unsigned char const responseTagBytes[] = {0x5A, 0x25};
  unsigned char const responseValueBytes[] = {0x42, 0x24, 0x07};
  VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleSet noAttributes;

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"response-provider", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"response-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableChild.isValid());

  AttributeHandleSet const requestedAttributes{reliableChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requestedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, requestedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  auto const ownerHandle = owner->getFederateHandle(L"response-provider");
  std::vector<unsigned char> observedRequestTag;
  std::vector<unsigned char> responseValue(
      responseValueBytes,
      responseValueBytes + sizeof(responseValueBytes));
  std::vector<unsigned char> responseTag(
      responseTagBytes,
      responseTagBytes + sizeof(responseTagBytes));
  ownerReports.provideAttributeValueUpdateHandler = [
      &owner,
      &observedRequestTag,
      responseValue,
      responseTag](
      ObjectInstanceHandle const& callbackObject,
      AttributeHandleSet const& callbackAttributes,
      VariableLengthData const& callbackTag) {
    observedRequestTag = variableLengthDataBytes(callbackTag);
    AttributeHandleValueMap values;
    for (AttributeHandle const& attribute : callbackAttributes) {
      values.emplace(
          attribute,
          VariableLengthData(responseValue.data(), responseValue.size()));
    }
    VariableLengthData responseUserTag(responseTag.data(), responseTag.size());
    owner->updateAttributeValues(callbackObject, values, responseUserTag);
  };

  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, requestedAttributes, requestTag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE(requesterReports.attributeReflectionReports.empty());

  // The provider callback is the response point: its update is submitted
  // while the official Provide Attribute Value Update callback is executing.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE(observedRequestTag ==
          std::vector<unsigned char>(
              requestTagBytes,
              requestTagBytes + sizeof(requestTagBytes)));
  REQUIRE(requesterReports.attributeReflectionReports.empty());

  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1);
  auto const& reflection = requesterReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1);
  auto const reflected = reflection.attributeValues.find(reliableChild);
  REQUIRE(reflected != reflection.attributeValues.end());
  REQUIRE(variableLengthDataBytes(reflected->second) == responseValue);
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == responseTag);
  REQUIRE(
      reflection.transportationType == owner->getTransportationTypeHandle(L"HLAreliable"));
  REQUIRE(reflection.producingFederate == ownerHandle);
  REQUIRE_FALSE(reflection.sentRegionsSupplied);

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Auto Provide solicits in-scope owners after discovery",
    "[integration][development-profile][object-management][auto-provide]"
    "[rti.service.get-auto-provide-switch]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const objectFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  auto const switchFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "switch-nrg-enabled-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectFom, switchFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"auto-provide-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"auto-provide-requester", L"subscriber", federationName));

  REQUIRE(owner->getAutoProvideSwitch());
  REQUIRE(requester->getAutoProvideSwitch());
  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());

  // The late subscription discovers an already registered object. Auto
  // Provide is evaluated after the Discover callback and therefore queues a
  // separate provider callback on the owner's dispatcher.
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  auto const& request = ownerReports.attributeValueUpdateRequestReports.front();
  REQUIRE(request.objectInstance == objectInstance);
  REQUIRE(request.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(request.userSuppliedTag).empty());

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded disabled Auto Provide leaves discovery without a provider callback",
    "[integration][development-profile][object-management][auto-provide]"
    "[rti.service.get-auto-provide-switch]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"auto-provide-disabled-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"auto-provide-disabled-requester", L"subscriber", federationName));

  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());
  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded MOM HLAsetSwitches adjusts federation-wide Auto Provide",
    "[integration][development-profile][federation-management][mom][auto-provide]"
    "[rti.service.send-interaction]"
    "[rti.service.get-auto-provide-switch]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"auto-provide-mom-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"auto-provide-mom-requester", L"subscriber", federationName));

  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());
  auto const setSwitches = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches");
  auto const autoProvideParameter = owner->getParameterHandle(
      setSwitches,
      L"HLAautoProvide");
  REQUIRE(setSwitches.isValid());
  REQUIRE(autoProvideParameter.isValid());

  std::vector<unsigned char> invalidSwitchBytes{0, 0, 0, 2};
  ParameterHandleValueMap invalidSwitchValues{
      {autoProvideParameter,
       VariableLengthData(invalidSwitchBytes.data(), invalidSwitchBytes.size())}};
  REQUIRE_THROWS_AS(
      owner->sendInteraction(setSwitches, invalidSwitchValues, VariableLengthData()),
      rti1516_2025::RTIinternalError);
  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle firstObject;
  REQUIRE_NOTHROW(firstObject = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  std::vector<unsigned char> enabledSwitchBytes{0, 0, 0, 1};
  ParameterHandleValueMap enabledSwitchValues{
      {autoProvideParameter,
       VariableLengthData(enabledSwitchBytes.data(), enabledSwitchBytes.size())}};
  REQUIRE_NOTHROW(
      owner->sendInteraction(setSwitches, enabledSwitchValues, VariableLengthData()));
  REQUIRE(owner->getAutoProvideSwitch());
  REQUIRE(requester->getAutoProvideSwitch());

  ObjectInstanceHandle secondObject;
  REQUIRE_NOTHROW(secondObject = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 2);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  auto const& enabledRequest = ownerReports.attributeValueUpdateRequestReports.front();
  REQUIRE(enabledRequest.objectInstance == secondObject);
  REQUIRE(enabledRequest.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(enabledRequest.userSuppliedTag).empty());

  std::vector<unsigned char> disabledSwitchBytes{0, 0, 0, 0};
  ParameterHandleValueMap disabledSwitchValues{
      {autoProvideParameter,
       VariableLengthData(disabledSwitchBytes.data(), disabledSwitchBytes.size())}};
  REQUIRE_NOTHROW(
      requester->sendInteraction(setSwitches, disabledSwitchValues, VariableLengthData()));
  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());

  ObjectInstanceHandle thirdObject;
  REQUIRE_NOTHROW(thirdObject = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 3);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded Query Attribute Ownership reports 2025 federate and unowned attributes",
    "[integration][development-profile][ownership-management]"
    "[rti.service.query-attribute-ownership]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"query-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"query-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  AttributeHandleSet const queriedAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  AttributeHandle invalidAttribute;
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(invalidObjectInstance, queriedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(objectInstance, invalidAttributes),
      rti1516_2025::AttributeNotDefined);

  // One successful query groups attributes by the standard ownership result:
  // a concrete joined federate is identified through Inform Attribute
  // Ownership, while an available attribute reaches Attribute Is Not Owned.
  REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE(requesterReports.attributeOwnershipReports.empty());
  REQUIRE(ownerReports.attributeOwnershipReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2);

  auto const federateReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate;
      });
  auto const unownedReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned;
      });
  REQUIRE(federateReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(unownedReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(federateReport->objectInstance == objectInstance);
  REQUIRE(federateReport->attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(federateReport->owner == owner->getFederateHandle(L"query-owner"));
  REQUIRE(unownedReport->objectInstance == objectInstance);
  REQUIRE(unownedReport->attributes == AttributeHandleSet{unownedChild});
  REQUIRE(std::none_of(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti;
      }));

  // Remove Object Instance nullifies reports that were queued by an earlier
  // query. The queued removal still reaches the requester, but no stale
  // ownership result may enter its FederateAmbassador afterward.
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Is Attribute Owned By Federate reads 2025 ownership state",
    "[integration][development-profile][ownership-management]"
    "[rti.service.is-attribute-owned-by-federate]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->isAttributeOwnedByFederate(invalidObjectInstance, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->isAttributeOwnedByFederate(invalidObjectInstance, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"ownership-check-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"ownership-check-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA};
  AttributeHandleSet const requesterSubscriptions{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  // The same shared 2025 ownership snapshot returns true only to its current
  // joined owner. A known remote owner and a defined-but-unowned attribute are
  // both false for the requester without triggering a callback.
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(invalidObjectInstance, reliableBaseA),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(objectInstance, invalidAttribute),
      rti1516_2025::AttributeNotDefined);

  // Once receive-order removal starts, the instance is no longer queryable
  // through this read-only ownership service before the removal callback runs.
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Acquisition If Available resolves 2025 ownership callbacks",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x51, 0xA7, 0x0C};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          noAttributes,
          tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          noAttributes,
          tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"acquisition-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"acquisition-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownerPublishedAttributes{reliableBaseA};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownerPublishedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  // The requester must publish at its known class before it can enter the
  // 2025 Willing to Acquire state. Publishing only one requested attribute
  // distinguishes class publication from per-attribute publication.
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{reliableBaseA},
          tag),
      rti1516_2025::AttributeNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA, reliableChild}));

  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          tag),
      rti1516_2025::AttributeNotDefined);

  AttributeHandleSet const mixedAvailabilityAttributes{unownedChild, reliableBaseA};
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      mixedAvailabilityAttributes,
      tag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  // Ownership transfers only when the matching callback begins. Until then,
  // 2025 requires a repeated If Available request for the same WTA attributes
  // to leave them unchanged, not fail or queue another terminal callback.
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag));
  // A mixed repeat preserves the already-pending attribute and independently
  // admits the new eligible attribute into Willing to Acquire.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild, reliableChild},
          tag));
  // The coupled declaration-management precondition prevents the requester
  // from withdrawing a publication that the pending 7.9 request still needs.
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3);
  auto const notification = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification;
      });
  auto const unavailable = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable;
      });
  REQUIRE(notification != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(unavailable != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(notification->objectInstance == objectInstance);
  REQUIRE(notification->attributes == AttributeHandleSet{unownedChild});
  REQUIRE(variableLengthDataBytes(notification->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(unavailable->objectInstance == objectInstance);
  REQUIRE(unavailable->attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(unavailable->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  auto const additionalNotification = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [&reliableChild](
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
                   ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification &&
               report.attributes == AttributeHandleSet{reliableChild};
      });
  REQUIRE(additionalNotification != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(additionalNotification->objectInstance == objectInstance);
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::FederateOwnsAttributes);
  REQUIRE_NOTHROW(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}));

  // Remove Object Instance nullifies a queued If Available callback before it
  // can establish ownership of a deleted instance. Its queued removal still
  // reaches the requester afterward.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      tag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Acquisition honors 2025 release and denial callbacks",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]"
    "[rti.service.attribute-ownership-release-denied]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const willingToAcquireTagBytes[] = {0x19, 0x53};
  unsigned char const acquisitionTagBytes[] = {0xA5, 0x70, 0xE1};
  unsigned char const denialTagBytes[] = {0xD3, 0x1A, 0x1E, 0xD0};
  VariableLengthData const willingToAcquireTag(
      willingToAcquireTagBytes,
      sizeof(willingToAcquireTagBytes));
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes,
          acquisitionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes,
          acquisitionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"regular-acquisition-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regular-acquisition-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const requesterAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, requesterAttributes));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          acquisitionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(
          objectInstance,
          AttributeHandleSet{unownedChild},
          denialTag),
      rti1516_2025::AttributeNotOwned);

  // The regular service overrides this requester's still-pending WTA state.
  // The earlier queued If Available work therefore becomes a no-delivery
  // callback rather than reporting the remote-owned attribute unavailable.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      willingToAcquireTag));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requesterAttributes,
      acquisitionTag));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{reliableBaseA},
          willingToAcquireTag),
      rti1516_2025::AttributeAlreadyBeingAcquired);
  // A repeated regular request preserves the Acquisition Pending state and
  // must not cause a duplicate Request Attribute Ownership Release callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  auto const& releaseRequest = ownerReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(releaseRequest.objectInstance == objectInstance);
  REQUIRE(releaseRequest.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(releaseRequest.userSuppliedTag) ==
          std::vector<unsigned char>(acquisitionTagBytes,
                                     acquisitionTagBytes + sizeof(acquisitionTagBytes)));

  // The WTA work was queued first and is now stale; the second callback is
  // the regular unowned-acquisition notification.
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& notification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == AttributeHandleSet{unownedChild});
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(acquisitionTagBytes,
                                     acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  // Release Denied preserves the owner's attribute ownership, ends every
  // matching regular acquisition, and supplies its own tag to Unavailable.
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 2);
  auto const& unavailable = requesterReports.attributeOwnershipAcquisitionReports.back();
  REQUIRE(unavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(unavailable.objectInstance == objectInstance);
  REQUIRE(unavailable.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(unavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));

  // Receive-order removal invalidates a queued regular owner-release
  // callback before it can enter user code or create a terminal report.
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 2);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Release Denied reaches all 2025 regular acquirers",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]"
    "[rti.service.attribute-ownership-release-denied]"
    "[federate.callback.attribute-ownership-unavailable]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstRequesterReports;
  ReportingFederateAmbassador secondRequesterReports;
  auto owner = makeRti();
  auto firstRequester = makeRti();
  auto secondRequester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const firstAcquisitionTagBytes[] = {0x01, 0x02, 0x03};
  unsigned char const secondAcquisitionTagBytes[] = {0x04, 0x05, 0x06};
  unsigned char const denialTagBytes[] = {0xD3, 0x1E, 0xD0};
  VariableLengthData const firstAcquisitionTag(
      firstAcquisitionTagBytes,
      sizeof(firstAcquisitionTagBytes));
  VariableLengthData const secondAcquisitionTag(
      secondAcquisitionTagBytes,
      sizeof(secondAcquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstRequester->connect(firstRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondRequester->connect(secondRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"denial-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstRequester->joinFederationExecution(
      L"denial-first-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(secondRequester->joinFederationExecution(
      L"denial-second-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA};
  REQUIRE_NOTHROW(firstRequester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(firstRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(secondRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(firstRequester->publishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->publishObjectClassAttributes(child, ownedAttributes));

  REQUIRE_NOTHROW(firstRequester->attributeOwnershipAcquisition(
      objectInstance,
      ownedAttributes,
      firstAcquisitionTag));
  REQUIRE_NOTHROW(secondRequester->attributeOwnershipAcquisition(
      objectInstance,
      ownedAttributes,
      secondAcquisitionTag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 2);
  auto const firstRelease = std::find_if(
      ownerReports.attributeOwnershipReleaseRequestReports.begin(),
      ownerReports.attributeOwnershipReleaseRequestReports.end(),
      [&firstAcquisitionTagBytes](
          ReportingFederateAmbassador::AttributeOwnershipReleaseRequestReport const& report) {
        return variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(
                firstAcquisitionTagBytes,
                firstAcquisitionTagBytes + sizeof(firstAcquisitionTagBytes));
      });
  auto const secondRelease = std::find_if(
      ownerReports.attributeOwnershipReleaseRequestReports.begin(),
      ownerReports.attributeOwnershipReleaseRequestReports.end(),
      [&secondAcquisitionTagBytes](
          ReportingFederateAmbassador::AttributeOwnershipReleaseRequestReport const& report) {
        return variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(
                secondAcquisitionTagBytes,
                secondAcquisitionTagBytes + sizeof(secondAcquisitionTagBytes));
      });
  REQUIRE(firstRelease != ownerReports.attributeOwnershipReleaseRequestReports.end());
  REQUIRE(secondRelease != ownerReports.attributeOwnershipReleaseRequestReports.end());
  REQUIRE(firstRelease->objectInstance == objectInstance);
  REQUIRE(secondRelease->objectInstance == objectInstance);
  REQUIRE(firstRelease->attributes == ownedAttributes);
  REQUIRE(secondRelease->attributes == ownedAttributes);

  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      ownedAttributes,
      denialTag));
  REQUIRE_FALSE(firstRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(secondRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& firstUnavailable = firstRequesterReports.attributeOwnershipAcquisitionReports.front();
  auto const& secondUnavailable = secondRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(firstUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(secondUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(firstUnavailable.attributes == ownedAttributes);
  REQUIRE(secondUnavailable.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(firstUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE(variableLengthDataBytes(secondUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(firstRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(secondRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(firstRequester->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->unpublishObjectClassAttributes(child, ownedAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(firstRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstRequester->disconnect());
  REQUIRE_NOTHROW(secondRequester->disconnect());
}

TEST_CASE(
    "Embedded Unconditional Attribute Ownership Divestiture offers eligible 2025 federates",
    "[integration][development-profile][ownership-management]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador regularRequesterReports;
  ReportingFederateAmbassador ifAvailableRequesterReports;
  ReportingFederateAmbassador invitedCandidateReports;
  ReportingFederateAmbassador staleCandidateReports;
  ReportingFederateAmbassador unpublishedCandidateReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto regularRequester = makeRti();
  auto ifAvailableRequester = makeRti();
  auto invitedCandidate = makeRti();
  auto staleCandidate = makeRti();
  auto unpublishedCandidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const regularAcquisitionTagBytes[] = {0xA2, 0x07, 0x20};
  unsigned char const pendingIfAvailableTagBytes[] = {0xB2, 0x07, 0x20};
  unsigned char const assumptionTagBytes[] = {0xD2, 0x07, 0x20};
  unsigned char const ifAvailableAcquisitionTagBytes[] = {0xC2, 0x07, 0x20};
  VariableLengthData const regularAcquisitionTag(
      regularAcquisitionTagBytes,
      sizeof(regularAcquisitionTagBytes));
  VariableLengthData const pendingIfAvailableTag(
      pendingIfAvailableTagBytes,
      sizeof(pendingIfAvailableTagBytes));
  VariableLengthData const assumptionTag(assumptionTagBytes, sizeof(assumptionTagBytes));
  VariableLengthData const ifAvailableAcquisitionTag(
      ifAvailableAcquisitionTagBytes,
      sizeof(ifAvailableAcquisitionTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          assumptionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          assumptionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regularRequester->connect(regularRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ifAvailableRequester->connect(ifAvailableRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(invitedCandidate->connect(invitedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(staleCandidate->connect(staleCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(unpublishedCandidate->connect(unpublishedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"unconditional-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(regularRequester->joinFederationExecution(
      L"unconditional-divestiture-regular-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(ifAvailableRequester->joinFederationExecution(
      L"unconditional-divestiture-if-available-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(invitedCandidate->joinFederationExecution(
      L"unconditional-divestiture-invited-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(staleCandidate->joinFederationExecution(
      L"unconditional-divestiture-stale-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(unpublishedCandidate->joinFederationExecution(
      L"unconditional-divestiture-unpublished-candidate",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = owner->getAttributeHandle(child, L"ReliableBaseB");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());
  AttributeHandleSet const regularAttributes{reliableBaseA};
  AttributeHandleSet const ifAvailableAttributes{reliableBaseB};
  AttributeHandleSet const candidateAttributes{reliableChild, unownedChild};
  AttributeHandleSet const ownedAttributes{
      reliableBaseA,
      reliableBaseB,
      reliableChild,
      unownedChild,
  };

  // Discovery gives every eventual candidate a known class. Only the two
  // selected candidates publish the offered attributes; a known but
  // unpublished federate must not receive the §7.4 callback.
  REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->subscribeObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(
      unpublishedCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(invitedCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(staleCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(unpublishedCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->publishObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->publishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->publishObjectClassAttributes(child, candidateAttributes));

  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          ownedAttributes,
          assumptionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{reliableBaseA, invalidAttribute},
          assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      regularRequester->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          regularAttributes,
          assumptionTag),
      rti1516_2025::AttributeNotOwned);

  // A regular acquirer is not offered the same attribute. Its existing
  // Acquisition Pending request instead becomes regular notification work
  // when unconditional divestiture makes the attribute unowned.
  REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
      objectInstance,
      regularAttributes,
      regularAcquisitionTag));
  REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      ifAvailableAttributes,
      pendingIfAvailableTag));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      ownedAttributes,
      assumptionTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));

  // The old owner-side release work is stale. A candidate that stops
  // publishing before its queued callback also receives no stale offer.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(staleCandidate->unpublishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(staleCandidateReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(unpublishedCandidateReports.attributeOwnershipAssumptionReports.empty());

  // A pre-existing If Available request keeps its own terminal callback and
  // is not duplicated as an assumption offer. It establishes ownership only
  // when that original callback begins after the unconditional transition.
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& ifAvailableNotification =
      ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(ifAvailableNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(ifAvailableNotification.objectInstance == objectInstance);
  REQUIRE(ifAvailableNotification.attributes == ifAvailableAttributes);
  REQUIRE(variableLengthDataBytes(ifAvailableNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              pendingIfAvailableTagBytes,
              pendingIfAvailableTagBytes + sizeof(pendingIfAvailableTagBytes)));
  REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_NOTHROW(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes));

  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& regularNotification =
      regularRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(regularNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(regularNotification.objectInstance == objectInstance);
  REQUIRE(regularNotification.attributes == regularAttributes);
  REQUIRE(variableLengthDataBytes(regularNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              regularAcquisitionTagBytes,
              regularAcquisitionTagBytes + sizeof(regularAcquisitionTagBytes)));
  REQUIRE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(
      regularRequester->unpublishObjectClassAttributes(child, regularAttributes));

  // The still-eligible, non-pending candidate receives a single grouped offer
  // with the unconditional-divestiture tag. The offer changes no ownership;
  // only its later acquisition request can do so.
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = invitedCandidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              assumptionTagBytes,
              assumptionTagBytes + sizeof(assumptionTagBytes)));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_NOTHROW(invitedCandidate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      candidateAttributes,
      ifAvailableAcquisitionTag));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& candidateNotification =
      invitedCandidateReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(candidateNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(candidateNotification.objectInstance == objectInstance);
  REQUIRE(candidateNotification.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(candidateNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              ifAvailableAcquisitionTagBytes,
              ifAvailableAcquisitionTagBytes + sizeof(ifAvailableAcquisitionTagBytes)));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(invitedCandidate->unpublishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regularRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(invitedCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(staleCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unpublishedCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(regularRequester->disconnect());
  REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
  REQUIRE_NOTHROW(invitedCandidate->disconnect());
  REQUIRE_NOTHROW(staleCandidate->disconnect());
  REQUIRE_NOTHROW(unpublishedCandidate->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Divestiture If Wanted transfers only to 2025 pending acquirers",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador regularRequesterReports;
  ReportingFederateAmbassador ifAvailableRequesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto regularRequester = makeRti();
  auto ifAvailableRequester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const regularAcquisitionTagBytes[] = {0xA1, 0x01, 0x25};
  unsigned char const ifAvailableAcquisitionTagBytes[] = {0xA2, 0x02, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD1, 0x56, 0x25};
  VariableLengthData const regularAcquisitionTag(
      regularAcquisitionTagBytes,
      sizeof(regularAcquisitionTagBytes));
  VariableLengthData const ifAvailableAcquisitionTag(
      ifAvailableAcquisitionTagBytes,
      sizeof(ifAvailableAcquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          noAttributes,
          divestitureTag,
          noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          noAttributes,
          divestitureTag,
          noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regularRequester->connect(regularRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ifAvailableRequester->connect(ifAvailableRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(regularRequester->joinFederationExecution(
      L"divestiture-regular-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(ifAvailableRequester->joinFederationExecution(
      L"divestiture-if-available-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  AttributeHandleSet const regularAttributes{reliableBaseA};
  AttributeHandleSet const ifAvailableAttributes{reliableChild};
  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};

  REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->subscribeObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->publishObjectClassAttributes(child, ifAvailableAttributes));

  AttributeHandleSet divestedAttributes{reliableBaseA};
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      regularRequester->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotOwned);

  // Without a pending acquirer, the service succeeds but returns an empty
  // result and retains the current owner. The output set is replaced rather
  // than appended to the caller's old contents.
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      regularAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));

  REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
      objectInstance,
      regularAttributes,
      regularAcquisitionTag));
  REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      ifAvailableAttributes,
      ifAvailableAcquisitionTag));

  // The owner accepts both forms as genuine pending acquirers. The regular
  // request's already queued owner-release callback is invalidated by the
  // synchronous ownership transfer.
  divestedAttributes = AttributeHandleSet{reliableBaseA};
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      ownedAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == ownedAttributes);
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_THROWS_AS(
      regularRequester->unpublishObjectClassAttributes(child, regularAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());

  // The regular acquirer receives the divestiture tag, not the older regular
  // acquisition tag, and its publication guard ends exactly at callback entry.
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& regularNotification =
      regularRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(regularNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(regularNotification.objectInstance == objectInstance);
  REQUIRE(regularNotification.attributes == regularAttributes);
  REQUIRE(variableLengthDataBytes(regularNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_NOTHROW(regularRequester->unpublishObjectClassAttributes(child, regularAttributes));

  // The stale If Available report is consumed first; its direct divestiture
  // notification follows with the same mandatory divestiture tag.
  REQUIRE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& ifAvailableNotification =
      ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(ifAvailableNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(ifAvailableNotification.objectInstance == objectInstance);
  REQUIRE(ifAvailableNotification.attributes == ifAvailableAttributes);
  REQUIRE(variableLengthDataBytes(ifAvailableNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_NOTHROW(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regularRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(regularRequester->disconnect());
  REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Divestiture If Wanted orders mixed 2025 acquirers by request",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.request-attribute-ownership-release]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstIfAvailableReports;
  ReportingFederateAmbassador laterRegularReports;
  auto owner = makeRti();
  auto firstIfAvailable = makeRti();
  auto laterRegular = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const ifAvailableTagBytes[] = {0x11, 0x25};
  unsigned char const regularTagBytes[] = {0x22, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD2, 0x25};
  unsigned char const denialTagBytes[] = {0xD3, 0x25};
  VariableLengthData const ifAvailableTag(ifAvailableTagBytes, sizeof(ifAvailableTagBytes));
  VariableLengthData const regularTag(regularTagBytes, sizeof(regularTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstIfAvailable->connect(firstIfAvailableReports, HLA_EVOKED));
  REQUIRE_NOTHROW(laterRegular->connect(laterRegularReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mixed-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstIfAvailable->joinFederationExecution(
      L"mixed-divestiture-first-if-available", L"publisher", federationName));
  REQUIRE_NOTHROW(laterRegular->joinFederationExecution(
      L"mixed-divestiture-later-regular", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const attributes{reliableBaseA};
  REQUIRE_NOTHROW(firstIfAvailable->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(laterRegular->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.objectDiscoveryReports.size() == 1);
  REQUIRE(laterRegularReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(firstIfAvailable->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->publishObjectClassAttributes(child, attributes));

  // The standard leaves multi-acquirer arbitration to the RTI. The bounded
  // serial profile makes its policy explicit: select the earliest accepted
  // request across regular and If Available forms, then retain later regular
  // requests for the selected owner to answer.
  REQUIRE_NOTHROW(firstIfAvailable->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      attributes,
      ifAvailableTag));
  REQUIRE_NOTHROW(laterRegular->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      regularTag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      attributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == attributes);
  REQUIRE(firstIfAvailable->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(laterRegular->isAttributeOwnedByFederate(objectInstance, reliableBaseA));

  // The old owner's release request is stale. The first queued If Available
  // terminal report is stale too, followed by the direct notification and a
  // follow-up regular release request addressed to the new owner.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& divestitureNotification =
      firstIfAvailableReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(divestitureNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(divestitureNotification.attributes == attributes);
  REQUIRE(variableLengthDataBytes(divestitureNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_FALSE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipReleaseRequestReports.size() == 1);
  auto const& laterRelease =
      firstIfAvailableReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(laterRelease.objectInstance == objectInstance);
  REQUIRE(laterRelease.attributes == attributes);
  REQUIRE(variableLengthDataBytes(laterRelease.userSuppliedTag) ==
          std::vector<unsigned char>(regularTagBytes, regularTagBytes + sizeof(regularTagBytes)));

  REQUIRE_NOTHROW(firstIfAvailable->attributeOwnershipReleaseDenied(
      objectInstance,
      attributes,
      denialTag));
  REQUIRE_FALSE(laterRegular->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(laterRegularReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& laterUnavailable = laterRegularReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(laterUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(laterUnavailable.attributes == attributes);
  REQUIRE(variableLengthDataBytes(laterUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(firstIfAvailable->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->unpublishObjectClassAttributes(child, attributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(firstIfAvailable->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(laterRegular->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstIfAvailable->disconnect());
  REQUIRE_NOTHROW(laterRegular->disconnect());
}

TEST_CASE(
    "Embedded Cancel Attribute Ownership Acquisition honors the 2025 confirmation boundary",
    "[integration][development-profile][ownership-management]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.confirm-attribute-ownership-acquisition]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const acquisitionTagBytes[] = {0xC0, 0xDE, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->cancelAttributeOwnershipAcquisition(invalidObjectInstance, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->cancelAttributeOwnershipAcquisition(invalidObjectInstance, noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"cancellation-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"cancellation-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());
  AttributeHandleSet const requestedAttributes{reliableBaseA, unownedChild};

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requestedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, requestedAttributes));

  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild}),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{invalidAttribute}),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{unownedChild}),
      rti1516_2025::AttributeAcquisitionWasNotRequested);
  REQUIRE_THROWS_AS(
      owner->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{reliableBaseA}),
      rti1516_2025::AttributeAlreadyOwned);

  // An If Available request is not a cancelable regular acquisition. The
  // following regular request overrides it, preserving the official state
  // boundary without synthesizing an explicit WTA cancellation callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{unownedChild},
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{unownedChild}),
      rti1516_2025::AttributeAcquisitionWasNotRequested);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes));

  // Cancellation is terminal only when confirmation begins. Until then it
  // retains the same publication fence and blocks another acquisition mode.
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::AttributeAlreadyBeingAcquired);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes,
      acquisitionTag));

  // The queued owner-release request is now stale, and neither it nor the
  // stale notification may reach user code after successful cancellation.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1);
  auto const& confirmation =
      requesterReports.attributeOwnershipAcquisitionCancellationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == requestedAttributes);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, requestedAttributes));
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(objectInstance, requestedAttributes),
      rti1516_2025::AttributeAcquisitionWasNotRequested);

  // Receive-order removal invalidates a queued cancellation confirmation just
  // as it invalidates other ownership callbacks.
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA}));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Negotiated Attribute Ownership Divestiture confirms a pending 2025 acquirer",
    "[integration][development-profile][ownership-management][callbacks]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador observerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const acquisitionTagBytes[] = {0x49, 0xA7, 0x11};
  unsigned char const divestitureTagBytes[] = {0x52, 0xA7, 0x11};
  unsigned char const confirmationTagBytes[] = {0x63, 0xA7, 0x11};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          divestitureTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->confirmDivestiture(invalidObjectInstance, noAttributes, confirmationTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->cancelNegotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          divestitureTag),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->confirmDivestiture(invalidObjectInstance, noAttributes, confirmationTag),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->cancelNegotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"negotiated-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"negotiated-divestiture-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"negotiated-divestiture-observer", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const transferredAttribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const cancelledAttribute = owner->getAttributeHandle(child, L"ReliableBaseB");
  auto const laterAcquirerAttribute = owner->getAttributeHandle(child, L"BestEffortBase");
  auto const noAcquirerAttribute = owner->getAttributeHandle(child, L"ReliableChild");
  auto const cancellationBeforeDeliveryAttribute = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(transferredAttribute.isValid());
  REQUIRE(cancelledAttribute.isValid());
  REQUIRE(laterAcquirerAttribute.isValid());
  REQUIRE(noAcquirerAttribute.isValid());
  REQUIRE(cancellationBeforeDeliveryAttribute.isValid());
  AttributeHandleSet const transferredAttributes{transferredAttribute};
  AttributeHandleSet const cancelledAttributes{cancelledAttribute};
  AttributeHandleSet const laterAcquirerAttributes{laterAcquirerAttribute};
  AttributeHandleSet const noAcquirerAttributes{noAcquirerAttribute};
  AttributeHandleSet const cancellationBeforeDeliveryAttributes{
      cancellationBeforeDeliveryAttribute};
  AttributeHandleSet const ownedAttributes{
      transferredAttribute,
      cancelledAttribute,
      laterAcquirerAttribute,
      noAcquirerAttribute,
      cancellationBeforeDeliveryAttribute,
  };

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_FALSE(observer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(observerReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, ownedAttributes));

  // Prepare the transfer boundary with a time-constrained observer and a
  // regulating requester. The old owner deliberately overrides this
  // instance to TimeStamp; after confirmation, the requester still has the
  // FOM/default Receive order, so a timestamped update must be delivered as
  // receive-order if ownership transfer correctly resets the captured state.
  REQUIRE_NOTHROW(observer->enableTimeConstrained());
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  // Receive-order reflections to an idle constrained federate require the
  // explicit 2025 asynchronous-delivery switch.
  REQUIRE_NOTHROW(observer->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(requester->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->changeAttributeOrderType(
      objectInstance,
      transferredAttributes,
      TIMESTAMP));

  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          invalidAttributes,
          divestitureTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      requester->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::AttributeNotOwned);
  REQUIRE_THROWS_AS(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, transferredAttributes),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, transferredAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);

  // An ordinary regular acquisition initially queues Request Attribute
  // Ownership Release. Starting negotiated divestiture before its callback
  // boundary makes that work stale and replaces it with the §7.5 confirmation
  // callback carrying the original acquisition tag.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, transferredAttributes, acquisitionTag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_NOTHROW(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::AttributeAlreadyBeingDivested);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, transferredAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  // The formerly queued ordinary release work is consumed without calling the
  // federate while the negotiated state is active; the real §7.5 callback is
  // then delivered on the next callback pass.
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1);
  auto const& firstConfirmation = ownerReports.divestitureConfirmationReports.front();
  REQUIRE(firstConfirmation.objectInstance == objectInstance);
  REQUIRE(firstConfirmation.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(firstConfirmation.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));

  REQUIRE_NOTHROW(owner->confirmDivestiture(
      objectInstance,
      transferredAttributes,
      confirmationTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, transferredAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& transferNotification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(transferNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(transferNotification.objectInstance == objectInstance);
  REQUIRE(transferNotification.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(transferNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              confirmationTagBytes,
              confirmationTagBytes + sizeof(confirmationTagBytes)));

  unsigned char const transferredValueBytes[] = {0x7A, 0x11};
  AttributeHandleValueMap transferredValues;
  transferredValues.emplace(
      transferredAttribute,
      VariableLengthData(transferredValueBytes, sizeof(transferredValueBytes)));
  auto const transferredRetraction = requester->updateAttributeValues(
      objectInstance,
      transferredValues,
      confirmationTag,
      rti1516_2025::HLAinteger64Time(2));
  // The transferred attribute reverts to the FOM's Receive-order default.
  // Clause 6.10 requires a returned designator only when at least one input
  // attribute has TSO preferred order, so this timestamped call is correctly
  // not retractable.
  REQUIRE_FALSE(transferredRetraction.isValid());
  REQUIRE_FALSE(observerReports.attributeReflectionReports.size());
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.attributeReflectionReports.size() == 1);
  auto const& transferredReflection = observerReports.attributeReflectionReports.front();
  REQUIRE(transferredReflection.objectInstance == objectInstance);
  REQUIRE(transferredReflection.attributeValues.size() == 1);
  REQUIRE(transferredReflection.attributeValues.begin()->first == transferredAttribute);
  REQUIRE(variableLengthDataBytes(transferredReflection.attributeValues.begin()->second) ==
          std::vector<unsigned char>(
              transferredValueBytes,
              transferredValueBytes + sizeof(transferredValueBytes)));
  REQUIRE(transferredReflection.sentOrderType == RECEIVE);
  REQUIRE(transferredReflection.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(transferredReflection.retractionSupplied);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, transferredAttributes));

  // A regular acquisition that arrives after the owner has entered Waiting
  // still takes the §7.5 confirmation route, rather than the ordinary release
  // route. Cancelling after that callback re-plans exactly one normal release.
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      laterAcquirerAttributes,
      divestitureTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      laterAcquirerAttributes,
      acquisitionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 2);
  REQUIRE(ownerReports.divestitureConfirmationReports.back().attributes == laterAcquirerAttributes);
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      laterAcquirerAttributes));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes ==
          laterAcquirerAttributes);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      laterAcquirerAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          laterAcquirerAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, laterAcquirerAttributes));

  // If cancellation happens before the older ordinary release work reaches
  // the owner, the preserved queued reservation lets that original release
  // callback run once. The queued confirmation work is stale and suppressed,
  // so cancellation does not create a duplicate owner callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      cancellationBeforeDeliveryAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      cancellationBeforeDeliveryAttributes,
      divestitureTag));
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      cancellationBeforeDeliveryAttributes));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 2);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes ==
          cancellationBeforeDeliveryAttributes);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 2);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      cancellationBeforeDeliveryAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 2);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          cancellationBeforeDeliveryAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      cancellationBeforeDeliveryAttributes));

  // Cancellation after a real §7.5 callback restores the ordinary release
  // route without changing ownership or allowing a stale confirmation to
  // complete the divestiture.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, cancelledAttributes, acquisitionTag));
  REQUIRE_NOTHROW(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          cancelledAttributes,
          divestitureTag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 3);
  REQUIRE(ownerReports.divestitureConfirmationReports.back().attributes == cancelledAttributes);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_NOTHROW(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, cancelledAttributes));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 3);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes == cancelledAttributes);
  REQUIRE(variableLengthDataBytes(
              ownerReports.attributeOwnershipReleaseRequestReports.back().userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, cancelledAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(objectInstance, cancelledAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 3);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          cancelledAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, cancelledAttributes));

  // If the selected regular acquirer cancels after the confirmation request,
  // Confirm Divestiture returns the official NoAcquisitionPending exception,
  // leaves ownership intact, and returns the private state to Waiting so the
  // owner can explicitly cancel the negotiated request.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, noAcquirerAttributes, acquisitionTag));
  REQUIRE_NOTHROW(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          noAcquirerAttributes,
          divestitureTag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 4);
  REQUIRE(ownerReports.divestitureConfirmationReports.back().attributes == noAcquirerAttributes);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      noAcquirerAttributes));
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, noAcquirerAttributes, confirmationTag),
      rti1516_2025::NoAcquisitionPending);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, noAcquirerAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, noAcquirerAttribute));
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      noAcquirerAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 4);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          noAcquirerAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, noAcquirerAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded receive-order Send Interaction honors 2025 promotion and callback lifecycle",
    "[integration][development-profile][interaction-management]"
    "[rti.service.send-interaction][federate.callback.receive-interaction]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador dualSubscriptionReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador unsubscribedReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto dualSubscription = makeRti();
  auto cancelled = makeRti();
  auto unsubscribed = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x71, 0x2A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap noParameters;
  InteractionClassHandle invalidInteractionClass;

  REQUIRE_THROWS_AS(
      unjoined->sendInteraction(invalidInteractionClass, noParameters, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->sendInteraction(invalidInteractionClass, noParameters, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(dualSubscription->connect(dualSubscriptionReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(unsubscribed->connect(unsubscribedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"interaction-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"interaction-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(dualSubscription->joinFederationExecution(
      L"interaction-dual", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"interaction-cancelled", L"subscriber", federationName));
  REQUIRE_NOTHROW(unsubscribed->joinFederationExecution(
      L"interaction-unsubscribed", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"interaction-immediate", L"subscriber", federationName));

  auto const base = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase");
  auto const child = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(child, L"Identifier");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(identifier.isValid());
  unsigned char const identifierBytes[] = {0xC4, 0x19, 0x02};
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_THROWS_AS(
      publisher->sendInteraction(child, parameterValues, tag),
      rti1516_2025::InteractionClassNotPublished);
  REQUIRE_NOTHROW(publisher->publishInteractionClass(child));
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(invalidInteractionClass, parameterValues, tag),
      rti1516_2025::InteractionClassNotDefined);
  ParameterHandle invalidParameter;
  ParameterHandleValueMap invalidParameterValues;
  invalidParameterValues.emplace(invalidParameter, VariableLengthData());
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(child, invalidParameterValues, tag),
      rti1516_2025::InteractionParameterNotDefined);

  // A passive superclass subscription does not arrange delivery. An active
  // child subscription still selects the closest received class, so a
  // child-and-parent subscription yields exactly one child callback.
  REQUIRE_NOTHROW(publisher->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(exact->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(promoted->subscribeInteractionClass(base, false));
  REQUIRE_NOTHROW(dualSubscription->subscribeInteractionClass(base, false));
  REQUIRE_NOTHROW(dualSubscription->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(cancelled->subscribeInteractionClass(base));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClass(child));

  REQUIRE_NOTHROW(publisher->sendInteraction(child, parameterValues, tag));
  REQUIRE(exactReports.interactionReports.empty());
  REQUIRE(promotedReports.interactionReports.empty());
  REQUIRE(dualSubscriptionReports.interactionReports.empty());
  REQUIRE(cancelledReports.interactionReports.empty());
  REQUIRE(immediateReports.interactionReports.size() == 1);

  // The standard suppresses delivery if a receiver unsubscribes after Send
  // Interaction and before its induced callback is invoked.
  REQUIRE_NOTHROW(cancelled->unsubscribeInteractionClass(base));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(dualSubscription->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unsubscribed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));

  auto requireDelivery = [&](ReportingFederateAmbassador::InteractionReport const& report,
                             InteractionClassHandle const& expectedClass) {
    REQUIRE(report.interactionClass == expectedClass);
    REQUIRE(report.parameterValues.size() == 1);
    auto const value = report.parameterValues.find(identifier);
    REQUIRE(value != report.parameterValues.end());
    REQUIRE(variableLengthDataBytes(value->second) ==
            std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == publisher->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  };

  REQUIRE(exactReports.interactionReports.size() == 1);
  requireDelivery(exactReports.interactionReports.front(), child);
  REQUIRE(promotedReports.interactionReports.empty());
  REQUIRE(dualSubscriptionReports.interactionReports.size() == 1);
  requireDelivery(dualSubscriptionReports.interactionReports.front(), child);
  REQUIRE(immediateReports.interactionReports.size() == 1);
  requireDelivery(immediateReports.interactionReports.front(), child);
  REQUIRE(cancelledReports.interactionReports.empty());
  REQUIRE(unsubscribedReports.interactionReports.empty());
  REQUIRE(publisherReports.interactionReports.empty());

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unsubscribed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(dualSubscription->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(dualSubscription->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(unsubscribed->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded timestamped Send Interaction queues TSO and requests retraction after delivery",
    "[integration][development-profile][interaction-management][time-management]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x31, 0x42};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA1, 0xB2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The sender's lower TSO bound is current time 0 plus lookahead 5.
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(
          interactionClass,
          parameterValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Retracting before the receiver's grant removes the pending TSO entry.
  // Advancing the regulator beyond its lookahead bound then releases the
  // receiver's TAR without invoking a stale interaction callback.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The next message is delivered at its exact TSO boundary. The callback is
  // entered before the matching Time Advance Grant and carries its handle.
  auto const secondHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant", "interaction", "grant"});
  auto const& secondReport = receiverReports.timestampedInteractionReports.front();
  REQUIRE(secondReport.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(secondReport.timeValue == L"7");
  REQUIRE(secondReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(secondReport.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(secondReport.retractionSupplied);
  REQUIRE(secondReport.retractionValid);
  REQUIRE(secondReport.parameterValues.size() == 1);
  // The sender is at time 2 with lookahead 5, so timestamp 7 is exactly its
  // Retract boundary and is not strictly later as 8.22.3 requires.
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE(receiverReports.requestRetractionReports.empty());

  // A producer advance can make a designator terminal before a constrained
  // recipient crosses the original delivery boundary. The tombstone must
  // preserve the public MessageCanNoLongerBeRetracted result while retaining
  // the typed payload long enough for the receiver's original callback.
  auto const terminalHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(terminalHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"8");
  REQUIRE(receiverReports.timestampedInteractionReports.back().retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(terminalHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Flush Queue Request can deliver a TSO message that remains retractable by
  // its originator. At sender time 3 and lookahead 5, timestamp 9 is legal for
  // Retract even though the receiver has already received the interaction.
  auto const thirdHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(thirdHandle.isValid());
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(9)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 3);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"9");
  REQUIRE(receiverReports.timestampedInteractionReports.back().retractionValid);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1);

  REQUIRE_NOTHROW(publisher->retract(thirdHandle));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(thirdHandle.encode()));

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded synchronization points announce, track achievement, and complete",
    "[integration][development-profile][federation-management][synchronization]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[federate.callback.synchronization-point-registration]"
    "[federate.callback.announce-synchronization-point]"
    "[federate.callback.federation-synchronized]") {
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador lateReports;
  auto first = makeRti();
  auto second = makeRti();
  auto late = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const syncTagBytes[] = {0x53, 0x59, 0x4E, 0x43};
  VariableLengthData const syncTag(syncTagBytes, sizeof(syncTagBytes));

  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      first->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  auto const firstHandle = first->joinFederationExecution(
      L"sync-first", L"sync", federationName);
  auto const secondHandle = second->joinFederationExecution(
      L"sync-second", L"sync", federationName);

  FederateHandleSet synchronizationSet;
  synchronizationSet.insert(firstHandle);
  synchronizationSet.insert(secondHandle);
  REQUIRE_NOTHROW(first->registerFederationSynchronizationPoint(
      L"startup", syncTag, synchronizationSet));
  REQUIRE(first->evokeCallback(0.0));
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE(firstReports.synchronizationPointRegistrationReports.size() == 1);
  REQUIRE(firstReports.synchronizationPointRegistrationReports.front().succeeded);
  REQUIRE(firstReports.synchronizationPointAnnouncementReports.size() == 1);
  REQUIRE(firstReports.synchronizationPointAnnouncementReports.front().label == L"startup");
  REQUIRE(firstReports.synchronizationPointAnnouncementReports.front().userSuppliedTag.size() ==
          sizeof(syncTagBytes));

  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE(secondReports.synchronizationPointAnnouncementReports.size() == 1);

  // A federate joining after registration is added to the outstanding
  // synchronization set and receives the same announced tag.
  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"sync-late", L"sync", federationName));
  REQUIRE_FALSE(late->evokeCallback(0.0));
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 1);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.front().label == L"startup");

  // A duplicate label is reported asynchronously to the requesting federate;
  // it does not disturb the already registered point.
  REQUIRE_NOTHROW(second->registerFederationSynchronizationPoint(L"startup", syncTag));
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE(secondReports.synchronizationPointRegistrationReports.size() == 1);
  REQUIRE_FALSE(secondReports.synchronizationPointRegistrationReports.front().succeeded);
  REQUIRE(
      secondReports.synchronizationPointRegistrationReports.front().failureReason ==
      rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE);

  REQUIRE_NOTHROW(first->synchronizationPointAchieved(L"startup"));
  REQUIRE_NOTHROW(second->synchronizationPointAchieved(L"startup", false));
  REQUIRE_NOTHROW(late->synchronizationPointAchieved(L"startup"));

  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE_FALSE(late->evokeCallback(0.0));
  REQUIRE(firstReports.federationSynchronizedReports.size() == 1);
  REQUIRE(secondReports.federationSynchronizedReports.size() == 1);
  REQUIRE(lateReports.federationSynchronizedReports.size() == 1);
  REQUIRE(firstReports.federationSynchronizedReports.front().label == L"startup");
  REQUIRE(secondReports.federationSynchronizedReports.front().label == L"startup");
  REQUIRE(lateReports.federationSynchronizedReports.front().label == L"startup");
  REQUIRE(firstReports.federationSynchronizedReports.front().failedToSyncSet.contains(secondHandle));
  REQUIRE(secondReports.federationSynchronizedReports.front().failedToSyncSet.contains(secondHandle));
  REQUIRE(lateReports.federationSynchronizedReports.front().failedToSyncSet.contains(secondHandle));
  REQUIRE_THROWS_AS(
      first->synchronizationPointAchieved(L"startup"),
      rti1516_2025::SynchronizationPointLabelNotAnnounced);

  REQUIRE_NOTHROW(late->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(late->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
}

TEST_CASE(
    "Embedded Next Message Request grants at the next queued TSO timestamp",
    "[integration][development-profile][interaction-management][time-management]"
    "[rti.service.next-message-request][rti.service.send-interaction]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4E, 0x4D, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmr-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xC1, 0xD2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const handle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(handle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The queued message at 7 is the NMR target even though the request asks
  // for 10. The request initially waits because the regulator's GALT is 5.
  REQUIRE_NOTHROW(receiver->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(
      receiverReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.front().sentOrderType ==
          rti1516_2025::TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.front().receivedOrderType ==
          rti1516_2025::TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.front().retractionValid);

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Available time advances use inclusive GALT and queued TSO delivery",
    "[integration][development-profile][interaction-management][time-management]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x41, 0x56, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"available-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"available-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA1, 0xB2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const first = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(first.isValid());

  // TARA reaches the exact defined GALT boundary after the regulator moves
  // to logical time 2. The message at 7 is delivered before the grant.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});

  // NMRA also selects the next queued timestamp, but its inclusive GALT rule
  // is exercised without relying on the message itself to mark the boundary:
  // the regulator advances from 2 to 4, making GALT exactly 9.
  auto const second = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(second.isValid());
  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"interaction", "grant", "interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"9");

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 9);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Flush Queue Request flushes queued TSO and reports optimistic time",
    "[integration][development-profile][interaction-management][time-management]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x46, 0x51, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"flush-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"flush-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xF1, 0xF2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const first = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  auto const second = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(12));
  REQUIRE(first.isValid());
  REQUIRE(second.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Flush Queue Request does not wait for the regulator. It delivers every
  // currently queued TSO message, even the one beyond the requested time, and
  // grants min(request=10, GALT=5, earliest-delivered=7) with OLT 7.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.flushQueueGrantReports.empty());
  REQUIRE_NOTHROW(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1);
  REQUIRE(receiverReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(receiverReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"interaction", "interaction", "flush-grant"});
  REQUIRE(receiverReports.timestampedInteractionReports[0].timeValue == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports[1].timeValue == L"12");

  REQUIRE_THROWS_AS(
      receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 2);
  REQUIRE(receiverReports.flushQueueGrantReports.back().value == L"7");
  REQUIRE(receiverReports.flushQueueGrantReports.back().optimisticValue == L"7");

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded producer advance requests terminalize expired TSO designators",
    "[integration][development-profile][interaction-management][time-management][tso]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request][rti.service.next-message-request-available]"
    "[rti.service.flush-queue-request]") {
  ReportingFederateAmbassador reports;
  ReportingFederateAmbassador receiverReports;
  auto rti = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"terminal-producer", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"terminal-receiver", L"subscriber", federationName));

  auto const interactionClass = rti->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = rti->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xE1, 0xE2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto sendAtBoundary = [&](std::int64_t timestamp) {
    auto const handle = rti->sendInteraction(
        interactionClass,
        parameterValues,
        tag,
        rti1516_2025::HLAinteger64Time(timestamp));
    REQUIRE(handle.isValid());
    return handle;
  };
  auto requireTerminal = [&](rti1516_2025::MessageRetractionHandle const& handle) {
    REQUIRE_THROWS_AS(
        rti->retract(handle),
        rti1516_2025::MessageCanNoLongerBeRetracted);
  };

  // Every accepted producer advance below moves its request boundary by one;
  // with actual lookahead five, the just-sent timestamp is exactly the strict
  // 8.22.3 cut-off. The idle constrained recipient retains each original
  // payload in its TSO queue while the producer's terminal record preserves
  // the public classification.
  auto const tar = sendAtBoundary(6);
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  requireTerminal(tar);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const tara = sendAtBoundary(7);
  REQUIRE_NOTHROW(rti->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(2)));
  requireTerminal(tara);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const nmr = sendAtBoundary(8);
  REQUIRE_NOTHROW(rti->nextMessageRequest(rti1516_2025::HLAinteger64Time(3)));
  requireTerminal(nmr);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const nmra = sendAtBoundary(9);
  REQUIRE_NOTHROW(rti->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(4)));
  requireTerminal(nmra);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const fqr = sendAtBoundary(10);
  REQUIRE_NOTHROW(rti->flushQueueRequest(rti1516_2025::HLAinteger64Time(5)));
  requireTerminal(fqr);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded timestamped Send Interaction returns a retraction designator without recipient fanout",
    "[integration][development-profile][interaction-management][time-management][tso]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4E, 0x4F, 0x4E, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"no-fanout-producer", L"publisher", federationName));

  auto const interactionClass = rti->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = rti->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA4, 0xA5};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  // Clause 8.22.3 identifies the designator returned from a timestamped
  // Send Interaction; the service must return one even when no recipient is
  // eligible for fanout. The ledger alone supports both legal retract and
  // eventual terminal classification without retaining typed payload.
  auto const retractable = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE_NOTHROW(rti->retract(retractable));
  REQUIRE_THROWS_AS(
      rti->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      rti->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout",
    "[integration][development-profile][object-management][time-management][tso]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4E, 0x4F, 0x4E, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"no-fanout-attribute-producer", L"publisher", federationName));

  auto const objectClass = rti->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = rti->getAttributeHandle(objectClass, L"ReliableBaseA");
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap values;
  unsigned char const valueBytes[] = {0xA4, 0xA5};
  values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));

  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(rti->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  // Clause 6.10 requires a designator when a time-regulating publisher sends
  // at least one TSO-preferred attribute with a timestamp; fanout is not an
  // additional precondition. The lightweight ledger supports legal retract
  // and eventual terminal classification without retaining typed passels.
  auto const retractable = rti->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE_NOTHROW(rti->retract(retractable));
  REQUIRE_THROWS_AS(
      rti->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = rti->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      rti->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable",
    "[integration][development-profile][interaction-management][time-management][tso]"
    "[rti.service.disable-time-regulation][rti.service.enable-time-regulation]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const parameterBytes[] = {0xD1, 0x5A};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x45, 0x4E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"re-enable-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"re-enable-retraction-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1);

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Disabling regulation removes the present authority to Retract but must
  // not discard or terminalize the live designator. Once regulation is
  // enabled again at the same lower boundary, the original queued fanout is
  // still legally retractable.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2);
  REQUIRE_NOTHROW(publisher->retract(retraction));

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Update Attribute Values queues passels before the grant and supports retraction",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xA4, 0x15};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-attribute-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  AttributeHandleValueMap attributeValues;
  unsigned char const reliableBytes[] = {0x11, 0x22};
  unsigned char const bestEffortBytes[] = {0x33, 0x44, 0x55};
  attributeValues.emplace(
      reliable,
      VariableLengthData(reliableBytes, sizeof(reliableBytes)));
  attributeValues.emplace(
      bestEffort,
      VariableLengthData(bestEffortBytes, sizeof(bestEffortBytes)));

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  // This fixture deliberately declares Receive order.  Opt the publisher's
  // class default into TimeStamp so this scenario continues to exercise the
  // timestamped queue while the order-control tranche honors FOM defaults.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(child, attributes, TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          attributeValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());

  auto const secondHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 2);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "reflect", "reflect", "grant"});

  bool reliableReported = false;
  bool bestEffortReported = false;
  for (auto const& report : receiverReports.attributeReflectionReports) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.attributeValues.size() == 1);
    if (report.transportationType == publisher->getTransportationTypeHandle(L"HLAreliable")) {
      reliableReported = report.attributeValues.contains(reliable);
      REQUIRE(variableLengthDataBytes(report.attributeValues.at(reliable)) ==
              std::vector<unsigned char>(
                  reliableBytes,
                  reliableBytes + sizeof(reliableBytes)));
    } else if (
        report.transportationType == publisher->getTransportationTypeHandle(L"HLAbestEffort")) {
      bestEffortReported = report.attributeValues.contains(bestEffort);
      REQUIRE(variableLengthDataBytes(report.attributeValues.at(bestEffort)) ==
              std::vector<unsigned char>(
                  bestEffortBytes,
                  bestEffortBytes + sizeof(bestEffortBytes)));
    } else {
      FAIL("Unexpected timestamped attribute transportation type");
    }
  }
  REQUIRE(reliableReported);
  REQUIRE(bestEffortReported);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Update Attribute Values requests retraction for an immediate-only recipient",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x52, 0x41, 0x56};
  unsigned char const reliableBytes[] = {0x6A, 0x17};
  unsigned char const bestEffortBytes[] = {0xC2, 0x4D, 0x19};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"attribute-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"attribute-retraction-immediate", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      reliable,
      VariableLengthData(reliableBytes, sizeof(reliableBytes)));
  attributeValues.emplace(
      bestEffort,
      VariableLengthData(bestEffortBytes, sizeof(bestEffortBytes)));

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(child, attributes, TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // No recipient is time-constrained, so this invocation has zero temporal
  // queue fanout. It must nevertheless retain the returned retraction
  // designator and the recipient's delivery state.
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  bool reliableReported = false;
  bool bestEffortReported = false;
  for (auto const& reflection : immediateReports.attributeReflectionReports) {
    REQUIRE(reflection.objectInstance == objectInstance);
    REQUIRE(reflection.producingFederate == publisherHandle);
    REQUIRE(reflection.timeValue == L"6");
    REQUIRE(reflection.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(reflection.receivedOrderType == rti1516_2025::RECEIVE);
    REQUIRE(reflection.retractionSupplied);
    REQUIRE(reflection.retractionValid);
    REQUIRE(reflection.attributeValues.size() == 1);
    if (reflection.attributeValues.contains(reliable)) {
      reliableReported = true;
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(reliable)) ==
              std::vector<unsigned char>(
                  reliableBytes,
                  reliableBytes + sizeof(reliableBytes)));
    } else if (reflection.attributeValues.contains(bestEffort)) {
      bestEffortReported = true;
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(bestEffort)) ==
              std::vector<unsigned char>(
                  bestEffortBytes,
                  bestEffortBytes + sizeof(bestEffortBytes)));
    } else {
      FAIL("Unexpected immediate timestamped attribute passel");
    }
  }
  REQUIRE(reliableReported);
  REQUIRE(bestEffortReported);
  REQUIRE(
      immediateReports.callbackOrder ==
      std::vector<std::string>{"reflect", "reflect"});

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(
      immediateReports.callbackOrder ==
      std::vector<std::string>{"reflect", "reflect", "request-retraction"});

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped regional Update Attribute Values carries recipient-gated regions across mixed fanout",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[mixed-fanout]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador immediateReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x5A, 0x25};
  unsigned char const tagBytes[] = {0x72, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-attribute-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"timestamped-regional-attribute-immediate", L"subscriber", federationName));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_FALSE(immediate->getConveyRegionDesignatorSetsSwitch());

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  auto const immediateRegion = immediate->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(immediate->setRangeBounds(
      immediateRegion, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const immediatePair{{
      flavorOnly,
      RegionHandleSet{immediateRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributesWithRegions(
      soda,
      immediatePair));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      flavor,
      VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          attributeValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(immediateReports.attributeReflectionReports.size() == 1);
  auto const& immediateFirstReport = immediateReports.attributeReflectionReports.back();
  REQUIRE(immediateFirstReport.objectInstance == objectInstance);
  REQUIRE(immediateFirstReport.timeValue == L"6");
  REQUIRE(immediateFirstReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(immediateFirstReport.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(immediateFirstReport.retractionSupplied);
  REQUIRE(immediateFirstReport.retractionValid);
  REQUIRE_FALSE(immediateFirstReport.sentRegionsSupplied);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(firstHandle.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"reflect", "request-retraction"});
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(immediateReports.attributeReflectionReports.size() == 1);

  auto const secondHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  auto const& immediateSecondReport = immediateReports.attributeReflectionReports.back();
  REQUIRE(immediateSecondReport.timeValue == L"7");
  REQUIRE(immediateSecondReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(immediateSecondReport.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(immediateSecondReport.retractionSupplied);
  REQUIRE(immediateSecondReport.retractionValid);
  REQUIRE_FALSE(immediateSecondReport.sentRegionsSupplied);
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1);
  auto const& suppressedRegionReport = receiverReports.attributeReflectionReports.front();
  REQUIRE(suppressedRegionReport.objectInstance == objectInstance);
  REQUIRE(suppressedRegionReport.producingFederate == publisherHandle);
  REQUIRE(suppressedRegionReport.timeValue == L"7");
  REQUIRE(suppressedRegionReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(suppressedRegionReport.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(suppressedRegionReport.retractionSupplied);
  REQUIRE(suppressedRegionReport.retractionValid);
  REQUIRE_FALSE(suppressedRegionReport.sentRegionsSupplied);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  auto const thirdHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(thirdHandle.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 3);
  auto const& immediateThirdReport = immediateReports.attributeReflectionReports.back();
  REQUIRE(immediateThirdReport.timeValue == L"8");
  REQUIRE(immediateThirdReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(immediateThirdReport.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(immediateThirdReport.retractionSupplied);
  REQUIRE(immediateThirdReport.retractionValid);
  REQUIRE_FALSE(immediateThirdReport.sentRegionsSupplied);
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 2);
  auto const& conveyedRegionReport = receiverReports.attributeReflectionReports.back();
  REQUIRE(conveyedRegionReport.objectInstance == objectInstance);
  REQUIRE(conveyedRegionReport.producingFederate == publisherHandle);
  REQUIRE(conveyedRegionReport.timeValue == L"8");
  REQUIRE(conveyedRegionReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(conveyedRegionReport.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(conveyedRegionReport.retractionSupplied);
  REQUIRE(conveyedRegionReport.retractionValid);
  REQUIRE(conveyedRegionReport.sentRegionsSupplied);
  REQUIRE(conveyedRegionReport.sentRegions.contains(publisherRegion));
  REQUIRE_THROWS_AS(
      publisher->retract(thirdHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributesWithRegions(
      soda,
      immediatePair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped regional Update Attribute Values returns a retraction designator without overlap-qualified recipients",
    "[integration][development-profile][object-management][ddm][time-management][tso]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x5A, 0x25};
  unsigned char const tagBytes[] = {0x4F, 0x56, 0x45, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-attribute-no-overlap-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-attribute-no-overlap-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));

  // Clause 6.10's TSO-preferred-attribute condition holds despite the
  // disjoint subscription. The public result therefore has a designator
  // while the regional planner suppresses all callback fanout.
  auto const retractable = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(retractable));
  REQUIRE_THROWS_AS(
      publisher->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      publisher->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Delete Object Instance reconstitutes on retraction and removes before grant",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD4, 0x16, 0x2A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-publisher", L"publisher", federationName));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->getFederateHandle(L"timestamped-delete-publisher"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-delete-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(
          objectInstance,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  auto const secondHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.objectRemovalReports.size() == 1);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "remove", "grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(removal.timeValue == L"7");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded terminal timestamped deletion tombstone releases its object name",
    "[integration][development-profile][object-management][time-management][tso]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.time-advance-request][rti.service.reserve-object-instance-name]") {
  ReportingFederateAmbassador ownerReports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                          "data" / "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x4F, 0x4D};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const objectName = L"Umbra.TerminalTimestampedDeletion";

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"terminal-deletion-owner", L"owner", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = owner->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ObjectInstanceHandle original;
  REQUIRE_NOTHROW(original = owner->registerObjectInstance(child, objectName));
  REQUIRE(original.isValid());
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  auto const deletion = owner->deleteObjectInstance(
      original,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(deletion.isValid());

  // The request boundary is 1 + the actual lookahead 5, exactly the sent
  // timestamp. Clause 8.22.3's strict comparison makes the designator
  // terminal, allowing the retained deletion state and name to be released.
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_THROWS_AS(
      owner->retract(deletion),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  ObjectInstanceHandle replacement;
  REQUIRE_NOTHROW(replacement = owner->registerObjectInstance(child, objectName));
  REQUIRE(replacement.isValid());

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded Request Retraction reconstitutes a delivered timestamped object deletion",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.remove-object-instance]"
    "[federate.callback.request-retraction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD5, 0x37, 0x2B};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"timestamped-delete-retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"timestamped-delete-retraction-constrained", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.objectDiscoveryReports.size() == 1);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  // Give the immediate recipient one ordinary attribute before deletion.  A
  // post-delivery retraction must restore this split invocation-time
  // ownership, not merely revive the deleting federate's privilege.
  AttributeHandleSet const immediateOwnedAttribute{bestEffort};
  REQUIRE_NOTHROW(immediate->publishObjectClassAttributes(child, immediateOwnedAttribute));
  REQUIRE_NOTHROW(immediate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      immediateOwnedAttribute,
      tag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(publisher->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      immediateOwnedAttribute,
      tag,
      divestedAttributes));
  REQUIRE(divestedAttributes == immediateOwnedAttribute);
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffort));
  REQUIRE(immediate->isAttributeOwnedByFederate(objectInstance, bestEffort));

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The nonconstrained recipient crosses its delivery boundary when it evokes
  // the accepted no-temporal-queue callback, while the constrained recipient
  // remains in the TSO queue. That is the only legal shape in which the
  // sender can still Retract and must both reconstitute the object and request
  // retraction from an already-delivered recipient.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(immediateReports.objectRemovalReports.size() == 1);
  REQUIRE(immediateReports.objectRemovalReports.front().retractionSupplied);
  REQUIRE(immediateReports.objectRemovalReports.front().retractionValid);
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      immediate->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(retraction));
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder.size() >= 2);
  REQUIRE(immediateReports.callbackOrder[immediateReports.callbackOrder.size() - 2] == "remove");
  REQUIRE(immediateReports.callbackOrder.back() == "request-retraction");

  // Reconstitution is committed before the Request Retraction callback.  The
  // original owner reassumes its attributes and every still-joined recipient
  // can resolve the restored instance immediately.
  REQUIRE(publisher->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(immediate->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(constrained->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffort));
  REQUIRE(immediate->isAttributeOwnedByFederate(objectInstance, bestEffort));

  // The queue's still-pending constrained fanout is suppressed, rather than
  // receiving Remove Object Instance or Request Retraction for a message it
  // never observed.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Request Retraction reconstitutes timestamped deletion without recipient fanout",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  auto publisher = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD6, 0x48, 0x3C};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-no-fanout-publisher", L"publisher", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  AttributeHandleSet const attributes{reliable};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // Equality at current time plus actual lookahead is not retractable under
  // 8.22.3's strict condition, even though the outgoing deletion itself is
  // a valid timestamped message.
  ObjectInstanceHandle boundaryObject;
  REQUIRE_NOTHROW(boundaryObject = publisher->registerObjectInstance(child));
  auto const boundaryRetraction = publisher->deleteObjectInstance(
      boundaryObject,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(boundaryRetraction.isValid());
  REQUIRE_THROWS_AS(
      publisher->retract(boundaryRetraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // With no other recipient, no typed payload enters the temporal queue.  The
  // execution still owns the designator and invocation snapshot, so a legal
  // later Retract must reconstitute the object without emitting a callback.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE(publisher->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE(publisherReports.requestRetractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Request Retraction reconstitutes timestamped deletion only for joined owners",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.remove-object-instance][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador formerOwnerReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto formerOwner = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD7, 0x59, 0x4D};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(formerOwner->connect(formerOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-joined-owner-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(formerOwner->joinFederationExecution(
      L"timestamped-delete-joined-owner-former", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"timestamped-delete-joined-owner-constrained", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(formerOwner->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(formerOwner->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  // At the deletion invocation the former owner owns BestEffortBase.  After
  // it resigns, 8.22.3 permits re-assumption only by owners still joined.
  AttributeHandleSet const formerOwnerAttribute{bestEffort};
  REQUIRE_NOTHROW(formerOwner->publishObjectClassAttributes(child, formerOwnerAttribute));
  REQUIRE_NOTHROW(formerOwner->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      formerOwnerAttribute,
      tag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(publisher->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      formerOwnerAttribute,
      tag,
      divestedAttributes));
  REQUIRE(divestedAttributes == formerOwnerAttribute);
  while (formerOwner->evokeCallback(0.0)) {
  }
  REQUIRE(formerOwner->isAttributeOwnedByFederate(objectInstance, bestEffort));

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  while (formerOwner->evokeCallback(0.0)) {
  }
  REQUIRE(formerOwnerReports.objectRemovalReports.size() == 1);
  REQUIRE_NOTHROW(
      formerOwner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE(formerOwnerReports.requestRetractionReports.empty());
  REQUIRE(publisher->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(constrained->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffort));

  AttributeHandleSet const queriedFormerOwnerAttribute{bestEffort};
  REQUIRE_NOTHROW(
      publisher->queryAttributeOwnership(objectInstance, queriedFormerOwnerAttribute));
  while (publisher->evokeCallback(0.0)) {
  }
  auto const unownedReport = std::find_if(
      publisherReports.attributeOwnershipReports.begin(),
      publisherReports.attributeOwnershipReports.end(),
      [&](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
                   ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned &&
               report.objectInstance == objectInstance &&
               report.attributes == queriedFormerOwnerAttribute;
      });
  REQUIRE(unownedReport != publisherReports.attributeOwnershipReports.end());

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(formerOwner->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded directed interactions route to known object-class subscribers",
    "[integration][development-profile][interaction-management][directed]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.unpublish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador unsubscribedReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto immediate = makeRti();
  auto unsubscribed = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" /
                               "tests" /
                               "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" /
                                    "tests" /
                                    "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x43, 0x11, 0x9A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(unsubscribed->connect(unsubscribedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"directed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"directed-subscriber", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"directed-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(unsubscribed->joinFederationExecution(
      L"directed-unsubscribed", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(unsubscribed->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(
      subscriber->subscribeObjectClassDirectedInteractions(objectClass, directedClasses, true));
  REQUIRE_NOTHROW(
      immediate->subscribeObjectClassDirectedInteractions(objectClass, directedClasses, true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unsubscribed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(unsubscribedReports.objectDiscoveryReports.size() == 1);

  // The target object and interaction are both valid, but only the two
  // declared directed subscribers receive the receive-order callback. The
  // sender is not an induced recipient.
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE(immediateReports.directedInteractionReports.size() == 1);
  REQUIRE(unsubscribedReports.directedInteractionReports.empty());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.size() == 1);
  auto const& first = subscriberReports.directedInteractionReports.front();
  REQUIRE(first.interactionClass == interactionClass);
  REQUIRE(first.objectInstance == target);
  REQUIRE(first.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(first.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(first.transportationType == publisher->getTransportationTypeHandle(L"HLAreliable"));

  // A queued callback is stale when the recipient unsubscribes before
  // evocation. Re-subscribing plans a fresh callback rather than reviving the
  // old one.
  subscriberReports.directedInteractionReports.clear();
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));

  // The source publication is another callback-time fence. Unpublishing the
  // pair before evocation suppresses the already accepted send.
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass,
          target,
          ParameterHandleValueMap{},
          tag),
      rti1516_2025::InteractionClassNotPublished);

  // Re-publication restores the route for a new send.
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.size() == 1);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unsubscribed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(unsubscribed->disconnect());
}

TEST_CASE(
    "Embedded directed interactions distinguish ownership and universal subscriptions",
    "[integration][development-profile][interaction-management][directed][ownership]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador ownershipSubscriberReports;
  ReportingFederateAmbassador universalSubscriberReports;
  auto owner = makeRti();
  auto sender = makeRti();
  auto ownershipSubscriber = makeRti();
  auto universalSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" /
                               "tests" /
                               "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" /
                                    "tests" /
                                    "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x55, 0x4E, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ownershipSubscriber->connect(ownershipSubscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(universalSubscriber->connect(universalSubscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"directed-subscription-owner", L"owner", federationName));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"directed-subscription-sender", L"sender", federationName));
  REQUIRE_NOTHROW(ownershipSubscriber->joinFederationExecution(
      L"directed-subscription-by-owner", L"subscriber", federationName));
  REQUIRE_NOTHROW(universalSubscriber->joinFederationExecution(
      L"directed-subscription-universal", L"subscriber", federationName));

  auto const objectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = owner->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(ownershipSubscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(owner->subscribeObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(
      ownershipSubscriber->subscribeObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = owner->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (sender->evokeCallback(0.0)) {
  }
  while (ownershipSubscriber->evokeCallback(0.0)) {
  }
  while (universalSubscriber->evokeCallback(0.0)) {
  }
  REQUIRE(senderReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ownershipSubscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(universalSubscriberReports.objectDiscoveryReports.size() == 1);

  // The default form is by ownership: the target owner receives this
  // directed interaction, the non-owning known subscriber does not, and the
  // universal known subscriber does.
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  while (owner->evokeCallback(0.0)) {
  }
  while (ownershipSubscriber->evokeCallback(0.0)) {
  }
  while (universalSubscriber->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.directedInteractionReports.size() == 1);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 1);
  REQUIRE(ownerReports.directedInteractionReports.front().objectInstance == target);
  REQUIRE(universalSubscriberReports.directedInteractionReports.front().objectInstance == target);

  // An empty class set must leave the universal kind unchanged, irrespective
  // of the supplied boolean. The next send therefore still reaches it.
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      InteractionClassHandleSet{},
      false));
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  while (owner->evokeCallback(0.0)) {
  }
  while (ownershipSubscriber->evokeCallback(0.0)) {
  }
  while (universalSubscriber->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.directedInteractionReports.size() == 2);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 2);

  // Re-subscribing a supplied class changes only that class's mode. Once the
  // universal subscriber becomes by-ownership, it no longer receives this
  // non-owned target; changing the other known subscriber to universal makes
  // it eligible on the following send.
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      false));
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  while (owner->evokeCallback(0.0)) {
  }
  while (ownershipSubscriber->evokeCallback(0.0)) {
  }
  while (universalSubscriber->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.directedInteractionReports.size() == 3);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 2);

  REQUIRE_NOTHROW(ownershipSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  while (owner->evokeCallback(0.0)) {
  }
  while (ownershipSubscriber->evokeCallback(0.0)) {
  }
  while (universalSubscriber->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.directedInteractionReports.size() == 4);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.size() == 1);
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 2);

  REQUIRE_NOTHROW(universalSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ownershipSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(universalSubscriber->disconnect());
  REQUIRE_NOTHROW(ownershipSubscriber->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timestamped directed interaction queues TSO before the grant and supports retraction",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[rti.service.send-directed-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x52, 0xA1, 0x0C};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-directed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-directed-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass,
          target,
          ParameterHandleValueMap{},
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.directedInteractionReports.empty());

  auto const secondHandle = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.directedInteractionReports.size() == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "directed", "grant"});

  auto const& report = receiverReports.directedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.objectInstance == target);
  REQUIRE(report.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.transportationType ==
          publisher->getTransportationTypeHandle(L"HLAreliable"));
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves TSO retraction-designator uniqueness",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][directed][time-management]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.send-directed-interaction][rti.service.retract]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                               "data" / "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-interaction-interaction-provider-fom.xml")
          .wstring();
  unsigned char const tagBytes[] = {0x52, 0x53, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-designator-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-designator-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The constrained recipient must be Time Advancing before an untimed save
  // can initiate.  Keep its grant pending while Request Federation Save is
  // accepted so the save callback can run at that boundary.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));

  // Save the complete directed-declaration and time-management baseline.
  REQUIRE_NOTHROW(publisher->requestFederationSave(L"directed-designator-baseline"));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"directed-designator-baseline"});
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"directed-designator-baseline"});
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  // This message is intentionally outside the saved image.  Its designator
  // must never identify any new message after the restore.
  auto const staleHandle = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(staleHandle.isValid());

  REQUIRE_NOTHROW(publisher->requestFederationRestore(L"directed-designator-baseline"));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1);
  REQUIRE(receiverReports.federationRestoredReportCount == 1);
  REQUIRE_THROWS_AS(
      publisher->retract(staleHandle),
      rti1516_2025::InvalidMessageRetractionHandle);

  auto const freshHandle = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(freshHandle.isValid());

  // Without a monotonic allocator, staleHandle would alias freshHandle here
  // and incorrectly retract the post-restore message.
  REQUIRE_THROWS_AS(
      publisher->retract(staleHandle),
      rti1516_2025::InvalidMessageRetractionHandle);
  REQUIRE_NOTHROW(publisher->retract(freshHandle));

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore restores a saved live TSO retraction record",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][time-management][tso]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.send-interaction]"
    "[rti.service.retract][rti.service.flush-queue-request]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const parameterBytes[] = {0x5A, 0x52};
  unsigned char const tagBytes[] = {0x53, 0x4E, 0x50};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"live-tso-retraction-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-live-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-live-retraction-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The constrained recipient must be Time Advancing before the untimed save
  // can initiate. Its target stays below the active regulator's GALT and well
  // below the live payload timestamp, so this does not deliver or alter the
  // payload that the snapshot must retain.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));

  // The completed snapshot contains both the queued interaction and its
  // live ledger entry.  Retract it after saving to ensure restore must replace
  // terminal post-save state rather than merely preserve an allocator floor.
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1);
  REQUIRE(receiverReports.federationRestoredReportCount == 1);

  // Flush Queue Request reaches the restored callback boundary without moving
  // the producer's strict retraction lower boundary. Both the original payload
  // and the same public designator must therefore be restored and remain
  // legally retractable.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"6");
  REQUIRE(receiverReports.timestampedInteractionReports.front().retractionValid);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a terminal TSO retraction classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][time-management][tso][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.send-interaction]"
    "[rti.service.retract]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const parameterBytes[] = {0x54, 0x4D};
  unsigned char const tagBytes[] = {0x54, 0x4F, 0x4D};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-tso-retraction-baseline";

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-terminal-retraction-publisher", L"publisher", federationName));

  auto const interactionClass = rti->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = rti->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const terminal = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(terminal.isValid());
  REQUIRE_NOTHROW(rti->retract(terminal));
  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Save the lightweight terminal record, then create later live traffic so
  // restore must replace the retraction index rather than merely accept the
  // same process-local handle again.
  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1);

  auto const postSave = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1);

  // Terminal classification is state in the saved ledger, not an inference
  // from current queue contents. The post-save handle, by contrast, belongs
  // to discarded traffic and must not alias that tombstone.
  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      rti->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore restores saved logical time and actual lookahead",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][lookahead]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[rti.service.query-lookahead][rti.service.modify-lookahead]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"time-window-baseline";
  rti1516_2025::HLAinteger64Time logicalTime;
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-time-window-member", L"publisher", federationName));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 0);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);

  // The completed image includes the federation's authoritative time
  // coordinator state.  Mutate both parts after saving so restore has to
  // replace, rather than merely retain, the post-save time window.
  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1);

  // The post-save time advance and lookahead increase are discarded.  This is
  // a narrow process-local rollback proof, not a claim of timed or durable
  // restore semantics.
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a deferred lookahead decrease",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][lookahead]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[rti.service.query-lookahead][rti.service.modify-lookahead]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"deferred-lookahead-baseline";
  rti1516_2025::HLAinteger64Time logicalTime;
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-deferred-lookahead-member", L"publisher", federationName));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (rti->evokeCallback(0.0)) {
  }

  // Clause 8.20 makes a decreasing request prospective: the actual
  // lookahead is still five before the next grant, while the requested value
  // is private future state that a completed save must retain.
  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1);

  // Consume the post-save decrease. Restore must put both the actual value
  // and its deferred target back, rather than just rewind the current time.
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1);
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 0);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  // Advance the restored member through the same elapsed interval.  The
  // second result proves that the saved deferred target, not only the visible
  // actual lookahead, returned with the snapshot.
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded Request Retraction notifies delivered directed-interaction recipients and suppresses queued fanout",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[rti.service.send-directed-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-directed-interaction]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x52, 0x44, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"directed-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"directed-retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"directed-retraction-constrained", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(constrainedReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The nonconstrained recipient has received the timestamped directed
  // interaction before the constrained recipient can cross its grant
  // boundary. The returned designator consequently distinguishes a delivered
  // recipient from the still-pending temporal fanout.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.directedInteractionReports.size() == 1);
  auto const& first = immediateReports.directedInteractionReports.front();
  REQUIRE(first.interactionClass == interactionClass);
  REQUIRE(first.objectInstance == target);
  REQUIRE(first.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(first.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(first.timeValue == L"2");
  REQUIRE(first.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(first.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(first.retractionSupplied);
  REQUIRE(first.retractionValid);
  REQUIRE(constrainedReports.directedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"directed", "request-retraction"});

  // The remaining recipient has not received the original callback, so the
  // same legal Retract removes its TSO queue entry rather than issuing a
  // Request Retraction. Advancing its TAR proves the stale delivery cannot
  // cross the grant boundary.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.directedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  // With the constrained member gone, this is an immediate-only timestamped
  // directed interaction: it has no temporal queue fanout but must retain a
  // valid recipient ledger for Request Retraction.
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  auto const immediateOnlyRetraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(immediateOnlyRetraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.directedInteractionReports.size() == 2);
  REQUIRE(immediateReports.directedInteractionReports.back().retractionValid);
  REQUIRE_NOTHROW(publisher->retract(immediateOnlyRetraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 2);
  REQUIRE(immediateReports.requestRetractionReports.back().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.back().encodedRetraction) ==
          variableLengthDataBytes(immediateOnlyRetraction.encode()));

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded regional interaction subscriptions filter 2025 receive-order sends",
    "[integration][development-profile][interaction-management][ddm]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x2C, 0x01};
  unsigned char const tagBytes[] = {0x7A, 0x19};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"regional-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-interaction-subscriber", L"subscriber", federationName));
  REQUIRE_FALSE(subscriber->getConveyRegionDesignatorSetsSwitch());

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  auto const barQuantity = publisher->getDimensionHandle(L"BarQuantity");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(barQuantity.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));

  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  // Regional declarations are independent from the ordinary subscription;
  // an empty region set does not create or remove a default subscription.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(interactionClass, {}));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(interactionClass, {}));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(subscriberReports.interactionReports.back().interactionClass == interactionClass);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);

  // Remove only the non-region subscription. The committed regional
  // subscription then controls whether the send is eligible.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);

  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 3);
  REQUIRE(subscriberReports.interactionReports.back().sentRegionsSupplied);

  // A queued regional send is rechecked at callback entry. Changing the
  // subscription region to a disjoint committed range suppresses the stale
  // delivery without changing the accepted send.
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(15UL, 20UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 3);

  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 4);
  REQUIRE(subscriberReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(variableLengthDataBytes(subscriberReports.interactionReports.back().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // An empty regional send is a no-send operation, while foreign, invalid,
  // uncommitted, and incompatible region specifications fail at the service
  // boundary with the 2025 exception vocabulary.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{},
      tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 4);
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          parameterValues,
          RegionHandleSet{subscriberRegion},
          tag),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{publisherRegion}),
      rti1516_2025::RegionNotCreatedByThisFederate);

  auto const uncommittedRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{uncommittedRegion}),
      rti1516_2025::InvalidRegion);
  auto const wrongContextRegion = subscriber->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      wrongContextRegion,
      barQuantity,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(
      subscriber->commitRegionModifications(RegionHandleSet{wrongContextRegion}));
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{wrongContextRegion}),
      rti1516_2025::InvalidRegionContext);
  auto const publisherWrongContextRegion = publisher->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherWrongContextRegion,
      barQuantity,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherWrongContextRegion}));
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          parameterValues,
          RegionHandleSet{publisherWrongContextRegion},
          tag),
      rti1516_2025::InvalidRegionContext);

  REQUIRE_THROWS_AS(
      subscriber->deleteRegion(subscriberRegion),
      rti1516_2025::RegionInUseForUpdateOrSubscription);
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(uncommittedRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(wrongContextRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherWrongContextRegion));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded Allow Relaxed DDM expands only touching regional interaction ranges",
    "[integration][development-profile][interaction-management][ddm][allow-relaxed-ddm]"
    "[rti.service.get-allow-relaxed-ddm-switch]"
    "[rti.service.create-region]"
    "[rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  auto runScenario = [](bool const relaxedDdmEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador subscriberReports;
    auto publisher = makeRti();
    auto subscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const restaurantFom =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const relaxedDdmFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                "cpp" / "tests" / "data" /
                                "allow-relaxed-ddm-enabled-fom.xml")
                                   .wstring();
    std::vector<std::wstring> fomModules{restaurantFom};
    if (relaxedDdmEnabled) {
      fomModules.push_back(relaxedDdmFom);
    }
    unsigned char const parameterBytes[] = {0x6B, 0x51};
    ParameterHandleValueMap parameterValues;
    VariableLengthData const tag;

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"relaxed-ddm-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"relaxed-ddm-subscriber", L"subscriber", federationName));
    REQUIRE(publisher->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);
    REQUIRE(subscriber->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);

    auto const interactionClass = publisher->getInteractionClassHandle(
        L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
    auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
    auto const serverId = publisher->getDimensionHandle(L"ServerId");
    REQUIRE(interactionClass.isValid());
    REQUIRE(temperatureOk.isValid());
    REQUIRE(serverId.isValid());
    parameterValues.emplace(
        temperatureOk,
        VariableLengthData(parameterBytes, sizeof(parameterBytes)));
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

    auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
    auto const subscriptionRegion = subscriber->createRegion(DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(0UL, 10UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        serverId,
        RangeBounds(10UL, 20UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{subscriptionRegion}));

    // [0, 10) and [10, 20) do not strictly overlap.  Umbra's explicit
    // Relaxed DDM policy admits this exact-boundary pair only when the
    // federation-wide switch is enabled.
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag));
    static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
    std::size_t expectedCallbacks = relaxedDdmEnabled ? 1U : 0U;
    REQUIRE(subscriberReports.interactionReports.size() == expectedCallbacks);

    // A nonzero gap is never treated as relaxed overlap.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        serverId,
        RangeBounds(11UL, 20UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag));
    static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(subscriberReports.interactionReports.size() == expectedCallbacks);

    // Relaxation is monotonic: an already strict overlap remains eligible in
    // both federation configurations.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        serverId,
        RangeBounds(5UL, 15UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag));
    static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
    ++expectedCallbacks;
    REQUIRE(subscriberReports.interactionReports.size() == expectedCallbacks);

    REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the FDD enables Relaxed DDM") {
    runScenario(true);
  }
  SECTION("the FDD leaves Relaxed DDM disabled") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded default-region interaction routing derives 2025 ordinary and regional effectiveness",
    "[integration][development-profile][interaction-management][ddm][default-region]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador regionalReports;
  ReportingFederateAmbassador mixedReports;
  auto publisher = makeRti();
  auto regional = makeRti();
  auto mixed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0xD1, 0x04};
  ParameterHandleValueMap parameters;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regional->connect(regionalReports, HLA_EVOKED));
  REQUIRE_NOTHROW(mixed->connect(mixedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"default-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(regional->joinFederationExecution(
      L"default-region-interaction-regional", L"subscriber", federationName));
  REQUIRE_NOTHROW(mixed->joinFederationExecution(
      L"default-region-interaction-mixed", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const regionalRegion = regional->createRegion(DimensionHandleSet{serverId});
  auto const mixedRegion = mixed->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(regional->setRangeBounds(
      regionalRegion,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(regional->commitRegionModifications(RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->setRangeBounds(
      mixedRegion,
      serverId,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(mixed->commitRegionModifications(RegionHandleSet{mixedRegion}));

  REQUIRE_NOTHROW(regional->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(mixed->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));

  VariableLengthData const tag;
  // The mixed federate retains an ordinary subscription, but the explicit,
  // disjoint regional declaration is its effective realization for this
  // class. It must not receive an explicit source-region interaction through
  // the ordinary default region.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.interactionReports.size() == 1);
  REQUIRE(mixedReports.interactionReports.empty());

  // Once its explicit subscription is removed, the retained ordinary
  // subscription again uses the default region and qualifies for any valid
  // explicit source-region interaction.
  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.interactionReports.size() == 2);
  REQUIRE(mixedReports.interactionReports.size() == 1);

  // Restore the disjoint regional realization. An ordinary Send Interaction
  // uses the invisible RTI-provided default region, which overlaps each
  // committed non-empty subscription region. An enabled convey switch must
  // expose that fact as a supplied, empty RegionHandleSet.
  REQUIRE_NOTHROW(mixed->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(regional->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(mixed->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameters, tag));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.interactionReports.size() == 3);
  REQUIRE(mixedReports.interactionReports.size() == 2);
  REQUIRE(regionalReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.interactionReports.back().sentRegions.empty());
  REQUIRE(mixedReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(mixedReports.interactionReports.back().sentRegions.empty());

  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regional->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(regional->deleteRegion(regionalRegion));
  REQUIRE_NOTHROW(mixed->deleteRegion(mixedRegion));
  REQUIRE_NOTHROW(mixed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regional->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(mixed->disconnect());
  REQUIRE_NOTHROW(regional->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped default-region interaction preserves 2025 regional callback metadata",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[default-region][tso][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0xD3, 0x0F};
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  ParameterHandleValueMap parameters;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-interaction-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // No source RegionHandle is supplied. For this dimensional interaction,
  // the 2025 RTI-provided default region overlaps the receiver's committed,
  // non-empty explicit subscription region and must retain that fact through
  // the federation-owned TSO queue.
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded passive interaction subscriptions do not arrange ordinary or regional delivery",
    "[integration][development-profile][interaction-management][ddm][passive-subscription]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x50, 0x41, 0x53};
  unsigned char const tagBytes[] = {0x53, 0x55, 0x42};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap parameterValues;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"passive-subscription-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"passive-subscription-subscriber", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  // An ordinary passive subscription is retained as declaration state but is
  // not eligible for a Receive Interaction callback. Replacing it with an
  // active subscription makes the next send eligible.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass, false));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.empty());

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass, true));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(interactionClass));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  // The same eligibility rule applies to a regional pair: the region remains
  // subscribed and in use while passive, but its overlap cannot arrange
  // delivery until the pair is made active.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion},
      false));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion},
      true));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped regional interaction queues TSO before the grant and supports retraction",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[rti.service.send-interaction-with-regions][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x4D, 0x0A};
  unsigned char const tagBytes[] = {0x6B, 0x22};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-receiver", L"subscriber", federationName));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, serverId, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          parameterValues,
          RegionHandleSet{publisherRegion},
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const secondHandle = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "interaction", "grant"});

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1);
  REQUIRE(report.parameterValues.find(temperatureOk) != report.parameterValues.end());
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_FALSE(report.sentRegionsSupplied);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  auto const thirdHandle = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(thirdHandle.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2);
  auto const& conveyedReport = receiverReports.timestampedInteractionReports.back();
  REQUIRE(conveyedReport.timeValue == L"8");
  REQUIRE(conveyedReport.sentRegionsSupplied);
  REQUIRE(conveyedReport.sentRegions.contains(publisherRegion));
  REQUIRE_THROWS_AS(
      publisher->retract(thirdHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Send Interaction With Regions returns a retraction designator without overlap-qualified recipients",
    "[integration][development-profile][interaction-management][ddm][time-management][tso]"
    "[rti.service.send-interaction-with-regions][rti.service.retract]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x4E, 0x4F};
  unsigned char const tagBytes[] = {0x4F, 0x56, 0x45, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-no-overlap-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-no-overlap-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, serverId, RangeBounds(11UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The regional send remains a qualifying TSO interaction even though the
  // sole subscription does not overlap the publisher's region. Clause 8.22.3
  // requires its federation-unique designator independently of fanout.
  auto const retractable = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(retractable));
  REQUIRE_THROWS_AS(
      publisher->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      publisher->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Request Retraction notifies delivered regional-interaction recipients and suppresses queued fanout",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[rti.service.send-interaction-with-regions][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x52, 0x52};
  unsigned char const tagBytes[] = {0x52, 0x47, 0x49};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"regional-retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"regional-retraction-constrained", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const immediateRegion = immediate->createRegion(DimensionHandleSet{serverId});
  auto const constrainedRegion = constrained->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(immediate->setRangeBounds(
      immediateRegion, serverId, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(constrained->setRangeBounds(
      constrainedRegion, serverId, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(constrained->commitRegionModifications(RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(constrained->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The immediate recipient's overlap-qualified callback begins before the
  // constrained recipient can reach its grant boundary. The recipient ledger
  // still owns both states even though only one entered the temporal queue.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 1);
  auto const& first = immediateReports.timestampedInteractionReports.front();
  REQUIRE(first.interactionClass == interactionClass);
  REQUIRE(first.parameterValues.size() == 1);
  REQUIRE(first.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(first.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(first.timeValue == L"2");
  REQUIRE(first.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(first.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(first.retractionSupplied);
  REQUIRE(first.retractionValid);
  REQUIRE_FALSE(first.sentRegionsSupplied);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"interaction", "request-retraction"});

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  // A regional send with no constrained recipient still receives an official
  // designator and retains the delivered immediate recipient for a later
  // Request Retraction callback.
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  auto const immediateOnlyRetraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(immediateOnlyRetraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 2);
  REQUIRE(immediateReports.timestampedInteractionReports.back().retractionValid);
  REQUIRE_NOTHROW(publisher->retract(immediateOnlyRetraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 2);
  REQUIRE(immediateReports.requestRetractionReports.back().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.back().encodedRetraction) ==
          variableLengthDataBytes(immediateOnlyRetraction.encode()));

  REQUIRE_NOTHROW(immediate->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation-list services dispatch standards reports in both callback models",
    "[integration][development-profile][federation-management][callbacks]"
    "[rti.service.list-federation-executions][rti.service.list-federation-execution-members]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador disconnectedReports;
  TestFederateAmbassador creatorFederate;
  TestFederateAmbassador memberFederate;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto disconnected = makeRti();
  auto creator = makeRti();
  auto member = makeRti();
  auto const firstFederationName = nextFederationName();
  auto const secondFederationName = nextFederationName();
  auto const missingFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(evoked->listFederationExecutions(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->listFederationExecutionMembers(firstFederationName),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(disconnected->connect(disconnectedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(creator->connect(creatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(firstFederationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(secondFederationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(member->joinFederationExecution(L"listed-member", L"observer", firstFederationName));

  REQUIRE_NOTHROW(evoked->listFederationExecutions());
  REQUIRE(evokedReports.federationExecutionReports.empty());
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionReports.size() == 1);
  auto const& listedFederations = evokedReports.federationExecutionReports.front();
  REQUIRE(listedFederations.size() == 2);
  auto const first = std::find_if(
      listedFederations.begin(),
      listedFederations.end(),
      [&firstFederationName](auto const& federation) {
        return federation.federationExecutionName == firstFederationName;
      });
  REQUIRE(first != listedFederations.end());
  REQUIRE(first->logicalTimeImplementationName == L"HLAinteger64Time");

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(firstFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionMemberReports.size() == 1);
  auto const& memberReport = evokedReports.federationExecutionMemberReports.front();
  REQUIRE(memberReport.federationName == firstFederationName);
  REQUIRE(memberReport.members.size() == 1);
  REQUIRE(memberReport.members.front().federateName == L"listed-member");
  REQUIRE(memberReport.members.front().federateType == L"observer");

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(missingFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.missingFederationReports == std::vector<std::wstring>{missingFederationName});

  REQUIRE_NOTHROW(immediate->listFederationExecutions());
  REQUIRE(immediateReports.federationExecutionReports.size() == 1);
  REQUIRE(immediateReports.federationExecutionReports.front().size() == 2);

  // Disconnect must discard a report that was queued against the prior
  // callback session; a later Evoke cannot dereference that stale recipient.
  REQUIRE_NOTHROW(disconnected->listFederationExecutions());
  REQUIRE_NOTHROW(disconnected->disconnect());
  REQUIRE_FALSE(disconnected->evokeCallback(0.0));
  REQUIRE(disconnectedReports.federationExecutionReports.empty());

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(firstFederationName));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(secondFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

TEST_CASE(
    "Embedded Time Advance Request changes logical time only at Time Advance Grant dispatch",
    "[integration][development-profile][time-management][callbacks]"
    "[rti.service.time-advance-request][rti.service.query-logical-time]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const evokedFederationName = nextFederationName();
  auto const immediateFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_THROWS_AS(evoked->queryLogicalTime(queriedTime), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      evoked->queryLogicalTime(queriedTime),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      evoked->createFederationExecution(
          evokedFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(evoked->joinFederationExecution(L"evoked-time-client", evokedFederationName));
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  rti1516_2025::HLAfloat64Time mismatchedQueryTime;
  REQUIRE_THROWS_AS(
      evoked->queryLogicalTime(mismatchedQueryTime),
      rti1516_2025::RTIinternalError);

  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAfloat64Time(7.0)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  REQUIRE(evokedReports.timeAdvanceGrantReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)),
      rti1516_2025::InTimeAdvancingState);

  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(evokedReports.timeAdvanceGrantReports.front().implementationName == L"HLAinteger64Time");
  REQUIRE(evokedReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  // Resignation deactivates the per-federate state. Its already-queued grant
  // is harmlessly consumed without advancing state or invoking a callback.
  REQUIRE_NOTHROW(evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeAdvanceGrantReports.size() == 1);

  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      immediate->createFederationExecution(
          immediateFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      immediate->joinFederationExecution(L"immediate-time-client", immediateFederationName));
  REQUIRE_NOTHROW(immediate->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(immediateReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(immediateReports.timeAdvanceGrantReports.front().value == L"5");
  rti1516_2025::HLAinteger64Time immediateTime;
  REQUIRE_NOTHROW(immediate->queryLogicalTime(immediateTime));
  REQUIRE(immediateTime.getTime() == 5);

  REQUIRE_NOTHROW(evoked->destroyFederationExecution(evokedFederationName));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->destroyFederationExecution(immediateFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded time-role services keep enable requests callback-gated before TSO support",
    "[integration][development-profile][time-management][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.disable-time-constrained]"
    "[rti.service.query-lookahead]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-constrained-enabled]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const evokedFederationName = nextFederationName();
  auto const immediateFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  rti1516_2025::HLAinteger64Interval queriedLookahead;
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeRegulation(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->queryLookahead(queriedLookahead), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      evoked->createFederationExecution(
          evokedFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      evoked->joinFederationExecution(L"evoked-time-role-client", evokedFederationName));
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAfloat64Interval(2.0)),
      rti1516_2025::InvalidLookahead);
  // The official reference interval constructors reject a negative value
  // before a caller can invoke the service; the mismatched reference type
  // above exercises Umbra's service-boundary InvalidLookahead mapping.

  REQUIRE_NOTHROW(evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE(evokedReports.timeRegulationEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().implementationName == L"HLAinteger64Time");
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == 2);
  rti1516_2025::HLAfloat64Interval mismatchedQueryLookahead;
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(mismatchedQueryLookahead),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(evoked->disableTimeRegulation());
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->disableTimeRegulation(),
      rti1516_2025::TimeRegulationIsNotEnabled);

  REQUIRE_NOTHROW(evoked->enableTimeConstrained());
  REQUIRE(evokedReports.timeConstrainedEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeConstrained(),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeConstrainedEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->disableTimeConstrained());
  REQUIRE_THROWS_AS(
      evoked->disableTimeConstrained(),
      rti1516_2025::TimeConstrainedIsNotEnabled);

  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      immediate->createFederationExecution(
          immediateFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      immediate->joinFederationExecution(L"immediate-time-role-client", immediateFederationName));
  REQUIRE_NOTHROW(immediate->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE(immediateReports.timeRegulationEnabledReports.size() == 1);
  rti1516_2025::HLAinteger64Interval immediateLookahead;
  REQUIRE_NOTHROW(immediate->queryLookahead(immediateLookahead));
  REQUIRE(immediateLookahead.getInterval() == 5);
  REQUIRE_NOTHROW(immediate->enableTimeConstrained());
  REQUIRE(immediateReports.timeConstrainedEnabledReports.size() == 1);

  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(evoked->destroyFederationExecution(evokedFederationName));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->destroyFederationExecution(immediateFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded Modify Lookahead applies increases immediately and decreases gradually",
    "[integration][development-profile][time-management][lookahead]"
    "[rti.service.modify-lookahead][rti.service.query-lookahead]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"lookahead-client", federationName));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::TimeRegulationIsNotEnabled);

  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  // A decrease is announced immediately but the actual lookahead remains at
  // five until logical time advances.
  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::InTimeAdvancingState);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 1);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded Query GALT and Query LITS observe other regulator time and pending advances",
    "[integration][development-profile][time-management][galt][lits]"
    "[rti.service.query-galt][rti.service.query-lits]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time galt;
  rti1516_2025::HLAinteger64Time lits;

  REQUIRE_THROWS_AS(receiver->queryGALT(galt), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(receiver->queryLITS(lits), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(receiver->queryGALT(galt), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(receiver->queryLITS(lits), rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  // A regulator is not active until the Time Regulation Enabled callback; no
  // other regulator means both no-TSO bounds are correctly undefined.
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(receiver->queryLITS(lits));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 2);
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 2);
  rti1516_2025::HLAfloat64Time mismatchedOutput;
  REQUIRE_THROWS_AS(receiver->queryGALT(mismatchedOutput), rti1516_2025::RTIinternalError);

  // While the regulator is Time Advancing, its requested time rather than its
  // prior granted time constrains its earliest possible future TSO timestamp.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 7);
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 7);
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 7);

  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(receiver->queryLITS(lits));

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded three-federate GALT tracks the minimum regulator and resignation",
    "[integration][development-profile][time-management][galt][lits][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.query-galt]"
    "[rti.service.query-lits][rti.service.resign-federation-execution]") {
  ReportingFederateAmbassador observerReports;
  ReportingFederateAmbassador leftRegulatorReports;
  ReportingFederateAmbassador rightRegulatorReports;
  auto observer = makeRti();
  auto leftRegulator = makeRti();
  auto rightRegulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time galt;
  rti1516_2025::HLAinteger64Time lits;
  rti1516_2025::HLAinteger64Time leftTime;

  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      observer->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(observer->joinFederationExecution(L"observer", federationName));
  REQUIRE_NOTHROW(leftRegulator->connect(leftRegulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(leftRegulator->joinFederationExecution(L"left-regulator", federationName));
  REQUIRE_NOTHROW(rightRegulator->connect(rightRegulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(rightRegulator->joinFederationExecution(L"right-regulator", federationName));

  REQUIRE_NOTHROW(observer->enableTimeConstrained());
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(leftRegulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(leftRegulator->evokeCallback(0.0));
  REQUIRE(leftRegulatorReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE_NOTHROW(rightRegulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)));
  REQUIRE_FALSE(rightRegulator->evokeCallback(0.0));
  REQUIRE(rightRegulatorReports.timeRegulationEnabledReports.size() == 1);

  // At the initial logical time, the left regulator (0 + 1) supplies the
  // minimum GALT/LITS candidate rather than the right regulator (0 + 3).
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 1);
  REQUIRE(observer->queryLITS(lits));
  REQUIRE(lits.getTime() == 1);

  // A pending and then granted TAR changes the left regulator's candidate to
  // 5 + 1.  The active right regulator continues to set the minimum at 3.
  REQUIRE_NOTHROW(leftRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 3);
  REQUIRE_FALSE(leftRegulator->evokeCallback(0.0));
  REQUIRE(leftRegulatorReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE_NOTHROW(leftRegulator->queryLogicalTime(leftTime));
  REQUIRE(leftTime.getTime() == 5);
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 3);

  // Resigning the regulator that supplied the minimum removes it from the
  // federation-owned snapshot, leaving the left regulator's 5 + 1 candidate.
  REQUIRE_NOTHROW(rightRegulator->resignFederationExecution(NO_ACTION));
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 6);
  REQUIRE(observer->queryLITS(lits));
  REQUIRE(lits.getTime() == 6);

  REQUIRE_NOTHROW(leftRegulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rightRegulator->disconnect());
  REQUIRE_NOTHROW(leftRegulator->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded constrained TAR waits for GALT and is released by a regulator advance",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1);

  // GALT is 2, so TAR is strict and TAR(2) must remain pending.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  rti1516_2025::HLAinteger64Time receiverTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.isInitial());

  // While the regulator is Time Advancing to 1, its pending time plus
  // lookahead raises GALT to 3 and releases the receiver's TAR(2), before the
  // regulator receives its own grant.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.getTime() == 2);

  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-disabled constrained TAR waits until a regulator becomes active",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  // The supplied FOM omits NRG, so its standard default is Disabled. Once a
  // regulator becomes active at time 0 with lookahead 2, TAR(1) is below the
  // newly defined GALT and the role callback wakes the deferred receiver.
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded constrained TAR is released when time-constrained mode is disabled",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // No other regulator means GALT is undefined, and the supplied FOM's NRG
  // switch defaults to Disabled. The constrained TAR therefore remains
  // pending until the role transition removes its GALT restriction.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-enabled constrained TAR is released when its only regulator disables",
    "[integration][development-profile][time-management][galt][non-regulated-grant][callbacks]"
    "[rti.service.disable-time-regulation][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->createFederationExecution(
      federationName,
      nrgFom.path().wstring(),
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE(receiver->getNonRegulatedGrantSwitch());
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));
  REQUIRE(regulator->getNonRegulatedGrantSwitch());

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  // Defined GALT is 2, so the strict TAR(2) waits while the regulator is
  // active. Disabling the sole regulator makes GALT undefined; the FDD's
  // enabled NRG switch then lets the receiver advance without waiting.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-enabled constrained TAR is released when its only regulator resigns",
    "[integration][development-profile][time-management][galt][non-regulated-grant][callbacks]"
    "[rti.service.resign-federation-execution][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->createFederationExecution(
      federationName,
      nrgFom.path().wstring(),
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  // The defined GALT is 2, so strict TAR(2) waits. Resigning the sole
  // regulator removes the bound; enabled NRG then releases the pending TAR.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded additional FOM NRG metadata cannot change the static switch or TAR",
    "[integration][development-profile][time-management][fom][non-regulated-grant][callbacks]"
    "[rti.service.join-federation-execution][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador applicantReports;
  auto receiver = makeRti();
  auto applicant = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // The official Restaurant FOM omits NRG, which means Disabled. With no
  // regulator, the constrained TAR must remain pending before the additional
  // module joins.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(applicant->connect(applicantReports, HLA_EVOKED));
  REQUIRE_NOTHROW(applicant->joinFederationExecution(
      L"metadata-applicant",
      L"observer",
      federationName,
      std::vector<std::wstring>{nrgFom.path().wstring()}));

  // NRG is a static federation-wide switch established at creation. The
  // additional module cannot change it or release the existing TAR.
  REQUIRE_FALSE(receiver->getNonRegulatedGrantSwitch());
  REQUIRE_FALSE(applicant->getNonRegulatedGrantSwitch());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(applicant->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded 2025 object-instance name reservation commits and reports asynchronously",
    "[integration][development-profile][federation-management]"
    "[rti.service.reserve-object-instance-name][rti.service.release-object-instance-name]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-multiple-object-instance-names]"
    "[federate.callback.object-instance-name-reservation-succeeded]"
    "[federate.callback.object-instance-name-reservation-failed]"
    "[federate.callback.multiple-object-instance-name-reservation-succeeded]"
    "[federate.callback.multiple-object-instance-name-reservation-failed]") {
  ReportingFederateAmbassador unjoinedReports;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      unjoined->reserveObjectInstanceName(L"Umbra.NotConnected"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->reserveMultipleObjectInstanceNames({L"Umbra.NotConnected"}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->reserveObjectInstanceName(L"Umbra.NotJoined"),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"reservation-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"reservation-peer", L"peer", federationName));

  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(L""),
      rti1516_2025::IllegalName);
  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(L"HLA.ReservedByTheRTI"),
      rti1516_2025::IllegalName);
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames({}),
      rti1516_2025::NameSetWasEmpty);
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames({L"Umbra.Valid", L"HLA.Invalid"}),
      rti1516_2025::IllegalName);

  auto const singleName = std::wstring{L"Umbra.SingleReservation"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(singleName));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.empty());
  REQUIRE(ownerReports.objectInstanceNameReservationFailedReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
          singleName);

  // Contention is an asynchronous failure, not ObjectInstanceNameInUse at
  // the service call boundary.
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(singleName));
  REQUIRE(peerReports.objectInstanceNameReservationFailedReports.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectInstanceNameReservationFailedReports.size() == 1);
  REQUIRE(peerReports.objectInstanceNameReservationFailedReports.front().objectInstanceName ==
          singleName);

  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(singleName));
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(singleName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(singleName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
          singleName);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(singleName));

  auto const multipleA = std::wstring{L"Umbra.MultipleA"};
  auto const multipleB = std::wstring{L"Umbra.MultipleB"};
  auto const multipleC = std::wstring{L"Umbra.MultipleC"};
  REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames({multipleA, multipleB}));
  REQUIRE(ownerReports.multipleObjectInstanceNameReservationSucceededReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.multipleObjectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      ownerReports.multipleObjectInstanceNameReservationSucceededReports.front().objectInstanceNames ==
      std::set<std::wstring>{multipleA, multipleB});

  REQUIRE_NOTHROW(peer->reserveMultipleObjectInstanceNames({multipleB, multipleC}));
  REQUIRE(peerReports.multipleObjectInstanceNameReservationSucceededReports.empty());
  REQUIRE(peerReports.multipleObjectInstanceNameReservationFailedReports.empty());
  REQUIRE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.multipleObjectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(peerReports.multipleObjectInstanceNameReservationFailedReports.size() == 1);
  REQUIRE(
      peerReports.multipleObjectInstanceNameReservationSucceededReports.front().objectInstanceNames ==
      std::set<std::wstring>{multipleC});
  REQUIRE(
      peerReports.multipleObjectInstanceNameReservationFailedReports.front().objectInstanceNames ==
      std::set<std::wstring>{multipleB});

  // Multiple release validates the entire set before mutating any name.
  REQUIRE_THROWS_AS(
      owner->releaseMultipleObjectInstanceNames({multipleA, multipleC}),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(multipleC));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames({multipleA, multipleB}));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames({}));

  // A reserved RTI-generated spelling cannot shadow an unnamed registration.
  // The standard does not prescribe a generated-name sequence; RTI-owned MOM
  // object instances may also legitimately occupy identities in that shared
  // object-instance namespace.
  auto const generatedName = std::wstring{L"UmbraObjectInstance-1"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(generatedName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, AttributeHandleSet{flavor}));
  auto const generatedObject = owner->registerObjectInstance(soda);
  auto const actualGeneratedName = owner->getObjectInstanceName(generatedObject);
  REQUIRE(actualGeneratedName != generatedName);
  REQUIRE(actualGeneratedName.rfind(L"UmbraObjectInstance-", 0U) == 0U);
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(generatedName));

  // Resignation returns reservations to the federation-wide pool.
  auto const resignedName = std::wstring{L"Umbra.ReleasedOnResign"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(resignedName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(resignedName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.size() == 2);
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.back().objectInstanceName ==
          resignedName);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(resignedName));

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
