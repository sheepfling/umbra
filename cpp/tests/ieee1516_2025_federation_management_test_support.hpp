#pragma once
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "internal/federation/embedded_transport.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_save_commit_store.hpp"
#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/observability/service_report_store.hpp"
#include "internal/time/update_rate_gate.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The embedded federation-management tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::FederateHandleSet;
using rti1516_2025::CallbackModel;
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

std::string formatFederateHandleSetForReport(
    std::vector<FederateHandle> const& handles) {
  std::string result{"["};
  for (std::size_t index = 0; index < handles.size(); ++index) {
    auto const handleText = umbra::detail::utf8FromWide(handles[index].toString());
    if (!handleText) {
      return {};
    }
    if (index != 0U) {
      result += ",";
    }
    result += "\"" + *handleText + "\"";
  }
  result += "]";
  return result;
}

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

// HLA_IMMEDIATE periodic MOM delivery is produced by the RTI scheduler
// thread, so this focused observer protects only the callback evidence it
// owns. The broad ReportingFederateAmbassador remains intentionally simple
// for the caller-gated HLA_EVOKED matrix.
class ImmediatePeriodicFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct Reflection final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    static_cast<void>(objectClass);
    static_cast<void>(objectInstanceName);
    static_cast<void>(producingFederate);
    std::scoped_lock lock(mutex_);
    discoveredObjects.push_back(objectInstance);
    callbacks_.notify_all();
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    static_cast<void>(userSuppliedTag);
    std::scoped_lock lock(mutex_);
    reflections.push_back({
        objectInstance,
        attributeValues,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    });
    callbacks_.notify_all();
  }

  [[nodiscard]] bool waitForReflectionCount(
      std::size_t expected,
      std::chrono::milliseconds timeout) const {
    std::unique_lock lock(mutex_);
    return callbacks_.wait_for(lock, timeout, [this, expected] {
      return reflections.size() >= expected;
    });
  }

  [[nodiscard]] std::vector<Reflection> reflectionSnapshot() const {
    std::scoped_lock lock(mutex_);
    return reflections;
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<Reflection> reflections;

 private:
  mutable std::mutex mutex_;
  mutable std::condition_variable callbacks_;
};

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  void federateResigned(std::wstring const& reasonForResignDescription) override {
    resignationDescriptions.push_back(reasonForResignDescription);
  }

  struct MemberReport {
    std::wstring federationName;
    rti1516_2025::FederationExecutionMemberInformationVector members;
  };

  struct TimeAdvanceGrantReport {
    std::wstring implementationName;
    std::wstring value;
  };

  struct TimestampedSaveInitiationReport {
    std::wstring label;
    std::wstring timeImplementationName;
    std::wstring timeValue;
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
    VariableLengthData encodedRetraction;
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

  void initiateFederateSave(
      std::wstring const& label,
      rti1516_2025::LogicalTime const& time) override {
    initiateFederateSaveReports.push_back(label);
    timestampedSaveInitiationReports.push_back({
        label,
        time.implementationName(),
        time.toString(),
    });
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
    if (onDiscoverObjectInstance) {
      onDiscoverObjectInstance();
    }
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
    if (onRemoveObjectInstance) {
      onRemoveObjectInstance();
    }
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
    });
    if (recordReceiveOrderObjectRemovalInCallbackOrder) {
      callbackOrder.push_back("remove");
    }
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    if (onTimestampedObjectRemoval) {
      onTimestampedObjectRemoval();
    }
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
    if (onAttributeReflection) {
      onAttributeReflection();
    }
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
        optionalRetraction != nullptr ? optionalRetraction->encode() : VariableLengthData{},
    });
    callbackOrder.push_back("reflect");
    if (onAttributeReflection) {
      onAttributeReflection();
    }
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
    if (onProvideAttributeValueUpdate) {
      onProvideAttributeValueUpdate();
    }
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
    if (onRequestAttributeOwnershipRelease) {
      onRequestAttributeOwnershipRelease();
    }
    attributeOwnershipReleaseRequestReports.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
    });
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    if (onConfirmAttributeOwnershipAcquisitionCancellation) {
      onConfirmAttributeOwnershipAcquisitionCancellation();
    }
    attributeOwnershipAcquisitionCancellationReports.push_back({
        objectInstance,
        attributes,
    });
  }

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    if (onRequestAttributeOwnershipAssumption) {
      onRequestAttributeOwnershipAssumption();
    }
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
    if (onRequestDivestitureConfirmation) {
      onRequestDivestitureConfirmation();
    }
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
  std::vector<TimestampedSaveInitiationReport> timestampedSaveInitiationReports;
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
  std::function<void()> onAttributeReflection;
  std::vector<std::wstring> resignationDescriptions;
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
  std::function<void()> onDiscoverObjectInstance;
  std::function<void()> onRemoveObjectInstance;
  std::function<void()> onTimestampedObjectRemoval;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  bool recordReceiveOrderObjectRemovalInCallbackOrder = false;
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
  std::function<void()> onProvideAttributeValueUpdate;
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
  std::function<void()> onRequestAttributeOwnershipRelease;
  std::function<void()> onRequestDivestitureConfirmation;
  std::function<void()> onConfirmAttributeOwnershipAcquisitionCancellation;
  std::function<void()> onRequestAttributeOwnershipAssumption;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

// DDM/time-management scenarios keep their callback assertions focused on
// delivery and grant ordering.  Regional subscriptions are also declaration
// relevance inputs in the 2025 profile, so those scenarios must explicitly
// suppress the independent Start/Stop and Turn-On/Off callback stream unless
// declaration relevance is the subject under test.
void suppressDeclarationRelevanceAdvisories(RTIambassador& rti) {
  if (rti.getObjectClassRelevanceAdvisorySwitch()) {
    rti.setObjectClassRelevanceAdvisorySwitch(false);
  }
  if (rti.getInteractionRelevanceAdvisorySwitch()) {
    rti.setInteractionRelevanceAdvisorySwitch(false);
  }
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath);
std::wstring nextFederationName();
std::vector<unsigned char> variableLengthDataBytes(VariableLengthData const& value);

}  // namespace

void runTimedMultiRecipientRegionalResignationAfterRestore(
    rti1516_2025::ResignAction resignationAction,
    bool useIfAvailableAcquisition = false,
    bool useNegotiatedDivestiture = false,
    bool useTwoCandidateNegotiatedContinuation = false,
    bool cancelRetainedNegotiatedConfirmation = false,
    bool useMixedRegularIfAvailableCandidateContinuation = false,
    bool cancelRetainedNegotiatedConfirmationBeforeDelivery = false,
    bool useRegularRetainedCandidate = false);
