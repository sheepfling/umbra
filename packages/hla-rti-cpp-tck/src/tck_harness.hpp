#pragma once

#include <RTI/FederateAmbassador.h>
#include <RTI/Enums.h>
#include <RTI/Exception.h>
#include <RTI/Handle.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RangeBounds.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>
#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/auth/HLAplainTextPassword.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAlogicalTime.h>
#include <RTI/encoding/HLAlogicalTimeInterval.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeFactory.h>
#include <RTI/time/LogicalTimeInterval.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/libfedtime/LogicalTimeFactoryFactory.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "tck_options.hpp"

namespace hla_rti_cpp_tck::detail {
namespace rti = rti1516_2025;

inline std::size_t adoptedDataDeleteCount = 0U;

inline void deleteAdoptedData(void* data) {
  ++adoptedDataDeleteCount;
  delete[] static_cast<rti::Octet*>(data);
}


struct ScenarioResult {
  std::string id;
  std::string callbackModel;
  std::string status;
  std::string message;
  long long durationMilliseconds = 0;
};

struct DiscoveryRecord {
  bool present = false;
  rti::ObjectInstanceHandle object;
  rti::ObjectClassHandle objectClass;
  std::wstring name;
  rti::FederateHandle producer;
};

struct ReflectionRecord {
  bool present = false;
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleValueMap values;
  std::vector<std::uint8_t> tag;
  rti::TransportationTypeHandle transportation;
  rti::FederateHandle producer;
  std::optional<rti::RegionHandleSet> regions;
};

struct InteractionRecord {
  bool present = false;
  rti::InteractionClassHandle interaction;
  rti::ParameterHandleValueMap parameters;
  std::vector<std::uint8_t> tag;
  rti::TransportationTypeHandle transportation;
  rti::FederateHandle producer;
  std::optional<rti::RegionHandleSet> regions;
};

struct TimedInteractionRecord {
  rti::InteractionClassHandle interaction;
  rti::ParameterHandleValueMap parameters;
  std::vector<std::uint8_t> tag;
  rti::TransportationTypeHandle transportation;
  rti::FederateHandle producer;
  std::vector<std::uint8_t> time;
  std::wstring timeText;
  rti::OrderType sentOrder = rti::RECEIVE;
  rti::OrderType receivedOrder = rti::RECEIVE;
  bool retractionPresent = false;
  std::vector<std::uint8_t> retraction;
  std::optional<rti::RegionHandleSet> regions;
};

struct DirectedInteractionRecord {
  rti::InteractionClassHandle interaction;
  rti::ObjectInstanceHandle object;
  rti::ParameterHandleValueMap parameters;
  std::vector<std::uint8_t> tag;
  rti::TransportationTypeHandle transportation;
  rti::FederateHandle producer;
};

struct TimedDirectedInteractionRecord {
  rti::InteractionClassHandle interaction;
  rti::ObjectInstanceHandle object;
  rti::ParameterHandleValueMap parameters;
  std::vector<std::uint8_t> tag;
  rti::TransportationTypeHandle transportation;
  rti::FederateHandle producer;
  std::vector<std::uint8_t> time;
  std::wstring timeText;
  rti::OrderType sentOrder = rti::RECEIVE;
  rti::OrderType receivedOrder = rti::RECEIVE;
  bool retractionPresent = false;
  std::vector<std::uint8_t> retraction;
};

struct TimedReflectionRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleValueMap values;
  std::vector<std::uint8_t> tag;
  rti::TransportationTypeHandle transportation;
  rti::FederateHandle producer;
  std::vector<std::uint8_t> time;
  std::wstring timeText;
  rti::OrderType sentOrder = rti::RECEIVE;
  rti::OrderType receivedOrder = rti::RECEIVE;
  bool retractionPresent = false;
  std::vector<std::uint8_t> retraction;
  std::optional<rti::RegionHandleSet> regions;
};

struct LogicalTimeRecord {
  std::vector<std::uint8_t> encoded;
  std::wstring text;
};

struct RetractionRecord {
  bool valid = false;
  std::vector<std::uint8_t> encoded;
};

struct AttributeTransportationConfirmationRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
  rti::TransportationTypeHandle transportation;
};

struct AttributeTransportationReportRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandle attribute;
  rti::TransportationTypeHandle transportation;
};

struct InteractionTransportationConfirmationRecord {
  rti::InteractionClassHandle interaction;
  rti::TransportationTypeHandle transportation;
};

struct InteractionTransportationReportRecord {
  rti::FederateHandle federate;
  rti::InteractionClassHandle interaction;
  rti::TransportationTypeHandle transportation;
};

struct UpdateAdvisoryRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
  std::optional<std::wstring> updateRateDesignator;
};

struct OwnershipInformationRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
  rti::FederateHandle owner;
};

struct OwnershipEventRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
  std::vector<std::uint8_t> tag;
};

struct RemovalRecord {
  rti::ObjectInstanceHandle object;
  std::vector<std::uint8_t> tag;
  rti::FederateHandle producer;
};

struct ScopeAdvisoryRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
};

struct TimedRemovalRecord {
  rti::ObjectInstanceHandle object;
  std::vector<std::uint8_t> tag;
  rti::FederateHandle producer;
  std::vector<std::uint8_t> time;
  std::wstring timeText;
  rti::OrderType sentOrder = rti::RECEIVE;
  rti::OrderType receivedOrder = rti::RECEIVE;
  bool retractionPresent = false;
  std::vector<std::uint8_t> retraction;
};

struct FlushQueueGrantRecord {
  std::vector<std::uint8_t> time;
  std::wstring timeText;
  std::vector<std::uint8_t> optimisticTime;
  std::wstring optimisticTimeText;
};

struct AttributeValueRequestRecord {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
  std::vector<std::uint8_t> tag;
};

struct SynchronizationRegistrationRecord {
  std::wstring label;
  bool succeeded = false;
  std::optional<rti::SynchronizationPointFailureReason> failureReason;
};

struct SynchronizationAnnouncementRecord {
  std::wstring label;
  std::vector<std::uint8_t> tag;
};

struct FederationSynchronizedRecord {
  std::wstring label;
  rti::FederateHandleSet failedToSyncSet;
};

struct RestoreInitiationRecord {
  std::wstring label;
  std::wstring federateName;
  rti::FederateHandle postRestoreFederateHandle;
};

inline std::wstring toWide(std::string const& value) {
  return std::wstring(value.begin(), value.end());
}

inline std::string toNarrow(std::wstring const& value) {
  std::string result;
  result.reserve(value.size());
  for (auto const character : value) {
    result += character >= 0 && character <= 0x7f
        ? static_cast<char>(character)
        : '?';
  }
  return result;
}

inline std::vector<std::uint8_t> copyBytes(
    rti::VariableLengthData const& data) {
  if (data.size() == 0U) {
    return {};
  }
  auto const* first = static_cast<std::uint8_t const*>(data.data());
  return {first, first + data.size()};
}

class Recorder final : public rti::FederateAmbassador {
 public:
  ~Recorder() noexcept override = default;

  void connectionLost(std::wstring const& description) override {
    std::lock_guard<std::mutex> lock(mutex_);
    connectionLost_ = true;
    connectionLostDescription_ = description;
  }

  void reportFederationExecutions(
      rti::FederationExecutionInformationVector const& report) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationReports_.push_back(report);
  }

  void reportFederationExecutionMembers(
      std::wstring const& federationName,
      rti::FederationExecutionMemberInformationVector const& report) override {
    std::lock_guard<std::mutex> lock(mutex_);
    memberReports_[federationName].push_back(report);
  }

  void reportFederationExecutionDoesNotExist(
      std::wstring const& federationName) override {
    std::lock_guard<std::mutex> lock(mutex_);
    missingFederations_.push_back(federationName);
  }

  void federateResigned(std::wstring const& reason) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federateResignedReasons_.push_back(reason);
  }

  void synchronizationPointRegistrationSucceeded(
      std::wstring const& label) override {
    std::lock_guard<std::mutex> lock(mutex_);
    synchronizationPointRegistrations_.push_back(
        SynchronizationRegistrationRecord{label, true, std::nullopt});
    callbackOrder_.push_back("synchronization-registration-succeeded");
  }

  void synchronizationPointRegistrationFailed(
      std::wstring const& label,
      rti::SynchronizationPointFailureReason reason) override {
    std::lock_guard<std::mutex> lock(mutex_);
    synchronizationPointRegistrations_.push_back(
        SynchronizationRegistrationRecord{label, false, reason});
    callbackOrder_.push_back("synchronization-registration-failed");
  }

  void announceSynchronizationPoint(
      std::wstring const& label,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    synchronizationPointAnnouncements_.push_back(
        SynchronizationAnnouncementRecord{label, copyBytes(tag)});
    callbackOrder_.push_back("synchronization-announce");
  }

  void federationSynchronized(
      std::wstring const& label,
      rti::FederateHandleSet const& failedToSyncSet) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationSynchronized_.push_back(
        FederationSynchronizedRecord{label, failedToSyncSet});
    callbackOrder_.push_back("federation-synchronized");
  }

  void initiateFederateSave(std::wstring const& label) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federateSaveInitiations_.push_back(label);
    callbackOrder_.push_back("federate-save-initiate");
  }

  void initiateFederateSave(
      std::wstring const& label,
      rti::LogicalTime const& time) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federateSaveInitiations_.push_back(label);
    federateSaveTimes_.push_back(
        LogicalTimeRecord{copyBytes(time.encode()), time.toString()});
    callbackOrder_.push_back("federate-save-initiate");
  }

  void federationSaved() override {
    std::lock_guard<std::mutex> lock(mutex_);
    ++federationSavedCount_;
    callbackOrder_.push_back("federation-saved");
  }

  void federationNotSaved(rti::SaveFailureReason reason) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationNotSavedReasons_.push_back(reason);
    callbackOrder_.push_back("federation-not-saved");
  }

  void federationSaveStatusResponse(
      rti::FederateHandleSaveStatusPairVector const& response) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationSaveStatusResponses_.push_back(response);
    callbackOrder_.push_back("federation-save-status");
  }

  void requestFederationRestoreSucceeded(std::wstring const& label) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationRestoreRequestsSucceeded_.push_back(label);
    callbackOrder_.push_back("federation-restore-request-succeeded");
  }

  void requestFederationRestoreFailed(std::wstring const& label) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationRestoreRequestsFailed_.push_back(label);
    callbackOrder_.push_back("federation-restore-request-failed");
  }

  void federationRestoreBegun() override {
    std::lock_guard<std::mutex> lock(mutex_);
    ++federationRestoreBegunCount_;
    callbackOrder_.push_back("federation-restore-begun");
  }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const& federateName,
      rti::FederateHandle const& postRestoreFederateHandle) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federateRestoreInitiations_.push_back(
        RestoreInitiationRecord{label, federateName, postRestoreFederateHandle});
    callbackOrder_.push_back("federate-restore-initiate");
  }

  void federationRestored() override {
    std::lock_guard<std::mutex> lock(mutex_);
    ++federationRestoredCount_;
    callbackOrder_.push_back("federation-restored");
  }

  void federationNotRestored(rti::RestoreFailureReason reason) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationNotRestoredReasons_.push_back(reason);
    callbackOrder_.push_back("federation-not-restored");
  }

  void federationRestoreStatusResponse(
      rti::FederateRestoreStatusVector const& response) override {
    std::lock_guard<std::mutex> lock(mutex_);
    federationRestoreStatusResponses_.push_back(response);
    callbackOrder_.push_back("federation-restore-status");
  }

  void startRegistrationForObjectClass(
      rti::ObjectClassHandle const& objectClass) override {
    std::lock_guard<std::mutex> lock(mutex_);
    registrationStarts_.push_back(objectClass);
  }

  void stopRegistrationForObjectClass(
      rti::ObjectClassHandle const& objectClass) override {
    std::lock_guard<std::mutex> lock(mutex_);
    registrationStops_.push_back(objectClass);
  }

  void turnInteractionsOn(
      rti::InteractionClassHandle const& interaction) override {
    std::lock_guard<std::mutex> lock(mutex_);
    interactionsOn_.push_back(interaction);
  }

  void turnInteractionsOff(
      rti::InteractionClassHandle const& interaction) override {
    std::lock_guard<std::mutex> lock(mutex_);
    interactionsOff_.push_back(interaction);
  }

  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    std::lock_guard<std::mutex> lock(mutex_);
    reservationSucceeded_ = objectInstanceName;
  }

  void objectInstanceNameReservationFailed(
      std::wstring const& objectInstanceName) override {
    std::lock_guard<std::mutex> lock(mutex_);
    reservationFailed_ = objectInstanceName;
  }

  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::ObjectClassHandle const& objectClass,
      std::wstring const& name,
      rti::FederateHandle const& producer) override {
    std::lock_guard<std::mutex> lock(mutex_);
    discovery_.present = true;
    discovery_.object = object;
    discovery_.objectClass = objectClass;
    discovery_.name = name;
    discovery_.producer = producer;
    discoveries_.push_back(discovery_);
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions) override {
    std::lock_guard<std::mutex> lock(mutex_);
    reflection_.present = true;
    reflection_.object = object;
    reflection_.values = values;
    reflection_.tag = copyBytes(tag);
    reflection_.transportation = transportation;
    reflection_.producer = producer;
    reflection_.regions = regions == nullptr
        ? std::nullopt
        : std::optional<rti::RegionHandleSet>(*regions);
    reflections_.push_back(reflection_);
    ++reflectionCount_;
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions,
      rti::LogicalTime const& time,
      rti::OrderType sentOrder,
      rti::OrderType receivedOrder,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    std::lock_guard<std::mutex> lock(mutex_);
    timedReflections_.push_back(TimedReflectionRecord{
        object,
        values,
        copyBytes(tag),
        transportation,
        producer,
        copyBytes(time.encode()),
        time.toString(),
        sentOrder,
        receivedOrder,
        optionalRetraction != nullptr,
        optionalRetraction == nullptr ? std::vector<std::uint8_t>{}
                                      : copyBytes(optionalRetraction->encode()),
        regions == nullptr
            ? std::nullopt
            : std::optional<rti::RegionHandleSet>(*regions),
    });
    callbackOrder_.push_back("reflect");
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interaction,
      rti::ParameterHandleValueMap const& parameters,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions) override {
    std::lock_guard<std::mutex> lock(mutex_);
    interaction_.present = true;
    interaction_.interaction = interaction;
    interaction_.parameters = parameters;
    interaction_.tag = copyBytes(tag);
    interaction_.transportation = transportation;
    interaction_.producer = producer;
    interaction_.regions = regions == nullptr
        ? std::nullopt
        : std::optional<rti::RegionHandleSet>(*regions);
    interactions_.push_back(interaction_);
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interaction,
      rti::ParameterHandleValueMap const& parameters,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions,
      rti::LogicalTime const& time,
      rti::OrderType sentOrder,
      rti::OrderType receivedOrder,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    std::lock_guard<std::mutex> lock(mutex_);
    TimedInteractionRecord record{
        interaction,
        parameters,
        copyBytes(tag),
        transportation,
        producer,
        copyBytes(time.encode()),
        time.toString(),
        sentOrder,
        receivedOrder,
        optionalRetraction != nullptr,
        optionalRetraction == nullptr ? std::vector<std::uint8_t>{}
                                      : copyBytes(optionalRetraction->encode()),
        regions == nullptr
            ? std::nullopt
            : std::optional<rti::RegionHandleSet>(*regions),
    };
    timedInteractions_.push_back(std::move(record));
    callbackOrder_.push_back("interaction");
  }

  void multipleObjectInstanceNameReservationSucceeded(
      std::set<std::wstring> const& names) override {
    std::lock_guard<std::mutex> lock(mutex_);
    multipleReservationSucceeded_ = names;
  }

  void multipleObjectInstanceNameReservationFailed(
      std::set<std::wstring> const& names) override {
    std::lock_guard<std::mutex> lock(mutex_);
    multipleReservationFailed_ = names;
  }

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const& interaction,
      rti::ObjectInstanceHandle const& object,
      rti::ParameterHandleValueMap const& parameters,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer) override {
    std::lock_guard<std::mutex> lock(mutex_);
    directedInteractions_.push_back(
        DirectedInteractionRecord{
            interaction, object, parameters, copyBytes(tag), transportation, producer});
  }

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const& interaction,
      rti::ObjectInstanceHandle const& object,
      rti::ParameterHandleValueMap const& parameters,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::LogicalTime const& time,
      rti::OrderType sentOrder,
      rti::OrderType receivedOrder,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    std::lock_guard<std::mutex> lock(mutex_);
    TimedDirectedInteractionRecord record{
        interaction,
        object,
        parameters,
        copyBytes(tag),
        transportation,
        producer,
        copyBytes(time.encode()),
        time.toString(),
        sentOrder,
        receivedOrder,
        optionalRetraction != nullptr,
        optionalRetraction == nullptr ? std::vector<std::uint8_t>{}
                                      : copyBytes(optionalRetraction->encode()),
    };
    timedDirectedInteractions_.push_back(std::move(record));
    callbackOrder_.push_back("directed");
  }

  void removeObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::VariableLengthData const& tag,
      rti::FederateHandle const& producer) override {
    std::lock_guard<std::mutex> lock(mutex_);
    removals_.push_back(RemovalRecord{object, copyBytes(tag), producer});
  }

  void removeObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::VariableLengthData const& tag,
      rti::FederateHandle const& producer,
      rti::LogicalTime const& time,
      rti::OrderType sentOrder,
      rti::OrderType receivedOrder,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    std::lock_guard<std::mutex> lock(mutex_);
    timedRemovals_.push_back(TimedRemovalRecord{
        object,
        copyBytes(tag),
        producer,
        copyBytes(time.encode()),
        time.toString(),
        sentOrder,
        receivedOrder,
        optionalRetraction != nullptr,
        optionalRetraction == nullptr ? std::vector<std::uint8_t>{}
                                      : copyBytes(optionalRetraction->encode()),
    });
    callbackOrder_.push_back("remove");
  }

  void attributesInScope(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    attributesInScope_.push_back(ScopeAdvisoryRecord{object, attributes});
    callbackOrder_.push_back("attributes-in-scope");
  }

  void attributesOutOfScope(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    attributesOutOfScope_.push_back(ScopeAdvisoryRecord{object, attributes});
    callbackOrder_.push_back("attributes-out-of-scope");
  }

  void provideAttributeValueUpdate(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    providedUpdates_.push_back(
        AttributeValueRequestRecord{object, attributes, copyBytes(tag)});
  }

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    updatesOn_.push_back(UpdateAdvisoryRecord{object, attributes, std::nullopt});
  }

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      std::wstring const& updateRateDesignator) override {
    std::lock_guard<std::mutex> lock(mutex_);
    updatesOn_.push_back(UpdateAdvisoryRecord{
        object, attributes, std::optional<std::wstring>{updateRateDesignator}});
  }

  void turnUpdatesOffForObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    updatesOff_.push_back(UpdateAdvisoryRecord{object, attributes, std::nullopt});
  }

  void confirmAttributeTransportationTypeChange(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::TransportationTypeHandle const& transportation) override {
    std::lock_guard<std::mutex> lock(mutex_);
    attributeTransportationConfirmations_.push_back(
        AttributeTransportationConfirmationRecord{object, attributes, transportation});
  }

  void reportAttributeTransportationType(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandle const& attribute,
      rti::TransportationTypeHandle const& transportation) override {
    std::lock_guard<std::mutex> lock(mutex_);
    attributeTransportationReports_.push_back(
        AttributeTransportationReportRecord{object, attribute, transportation});
  }

  void confirmInteractionTransportationTypeChange(
      rti::InteractionClassHandle const& interaction,
      rti::TransportationTypeHandle const& transportation) override {
    std::lock_guard<std::mutex> lock(mutex_);
    interactionTransportationConfirmations_.push_back(
        InteractionTransportationConfirmationRecord{interaction, transportation});
  }

  void reportInteractionTransportationType(
      rti::FederateHandle const& federate,
      rti::InteractionClassHandle const& interaction,
      rti::TransportationTypeHandle const& transportation) override {
    std::lock_guard<std::mutex> lock(mutex_);
    interactionTransportationReports_.push_back(
        InteractionTransportationReportRecord{federate, interaction, transportation});
  }

  void requestAttributeOwnershipAssumption(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipAssumption_ = OwnershipEventRecord{object, attributes, copyBytes(tag)};
  }

  void requestDivestitureConfirmation(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    divestitureConfirmation_ = OwnershipEventRecord{object, attributes, copyBytes(tag)};
  }

  void attributeOwnershipAcquisitionNotification(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipAcquisition_ = OwnershipEventRecord{object, attributes, copyBytes(tag)};
  }

  void attributeOwnershipUnavailable(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto record = OwnershipEventRecord{object, attributes, copyBytes(tag)};
    ownershipUnavailable_ = record;
    ownershipUnavailableEvents_.push_back(std::move(record));
  }

  void requestAttributeOwnershipRelease(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::function<void(
        rti::ObjectInstanceHandle const&,
        rti::AttributeHandleSet const&,
        rti::VariableLengthData const&)> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      ownershipReleaseRequest_ = OwnershipEventRecord{object, attributes, copyBytes(tag)};
      hook = ownershipReleaseHook_;
    }
    if (hook) {
      hook(object, attributes, tag);
    }
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto record = OwnershipEventRecord{object, attributes, {}};
    ownershipAcquisitionCancellation_ = record;
    ownershipAcquisitionCancellationEvents_.push_back(std::move(record));
  }

  void informAttributeOwnership(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::FederateHandle const& owner) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto record = OwnershipInformationRecord{object, attributes, owner};
    ownershipInformation_ = record;
    ownershipInformationEvents_.push_back(std::move(record));
  }

  void attributeIsNotOwned(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto record = OwnershipEventRecord{object, attributes, {}};
    ownershipNotOwned_ = record;
    ownershipNotOwnedEvents_.push_back(std::move(record));
  }

  void attributeIsOwnedByRTI(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto record = OwnershipEventRecord{object, attributes, {}};
    ownershipOwnedByRti_ = record;
    ownershipOwnedByRtiEvents_.push_back(std::move(record));
  }

  void timeRegulationEnabled(rti::LogicalTime const& time) override {
    std::lock_guard<std::mutex> lock(mutex_);
    timeRegulationEnabled_.push_back(
        LogicalTimeRecord{copyBytes(time.encode()), time.toString()});
  }

  void timeConstrainedEnabled(rti::LogicalTime const& time) override {
    std::lock_guard<std::mutex> lock(mutex_);
    timeConstrainedEnabled_.push_back(
        LogicalTimeRecord{copyBytes(time.encode()), time.toString()});
  }

  void flushQueueGrant(
      rti::LogicalTime const& time,
      rti::LogicalTime const& optimisticTime) override {
    std::lock_guard<std::mutex> lock(mutex_);
    flushQueueGrants_.push_back(FlushQueueGrantRecord{
        copyBytes(time.encode()),
        time.toString(),
        copyBytes(optimisticTime.encode()),
        optimisticTime.toString(),
    });
    callbackOrder_.push_back("flush-grant");
  }

  void timeAdvanceGrant(rti::LogicalTime const& time) override {
    std::lock_guard<std::mutex> lock(mutex_);
    timeAdvanceGrants_.push_back(
        LogicalTimeRecord{copyBytes(time.encode()), time.toString()});
    callbackOrder_.push_back("grant");
  }

  void requestRetraction(rti::MessageRetractionHandle const& retraction) override {
    std::lock_guard<std::mutex> lock(mutex_);
    retractions_.push_back(
        RetractionRecord{retraction.isValid(), copyBytes(retraction.encode())});
    callbackOrder_.push_back("request-retraction");
  }

  bool hasConnectionLost() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connectionLost_;
  }

  std::wstring connectionLostDescription() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connectionLostDescription_;
  }

  std::vector<std::wstring> federateResignedReasons() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federateResignedReasons_;
  }

  bool listsFederation(
      std::wstring const& name,
      std::wstring const& logicalTimeImplementation = {}) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto const& report : federationReports_) {
      for (auto const& execution : report) {
        if (execution.federationExecutionName == name &&
            (logicalTimeImplementation.empty() ||
             execution.logicalTimeImplementationName == logicalTimeImplementation)) {
          return true;
        }
      }
    }
    return false;
  }

  bool listsMembers(
      std::wstring const& federation,
      std::set<std::wstring> const& expected,
      std::size_t exactCount = 0U) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto const found = memberReports_.find(federation);
    if (found == memberReports_.end()) {
      return false;
    }
    for (auto const& report : found->second) {
      std::set<std::wstring> names;
      for (auto const& member : report) {
        names.insert(member.federateName);
      }
      if (std::includes(names.begin(), names.end(), expected.begin(), expected.end()) &&
          (exactCount == 0U || names.size() == exactCount)) {
        return true;
      }
    }
    return false;
  }

  bool listsMember(
      std::wstring const& federation,
      std::wstring const& federateName,
      std::wstring const& federateType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto const found = memberReports_.find(federation);
    if (found == memberReports_.end()) {
      return false;
    }
    for (auto const& report : found->second) {
      for (auto const& member : report) {
        if (member.federateName == federateName && member.federateType == federateType) {
          return true;
        }
      }
    }
    return false;
  }

  bool reportsMissing(std::wstring const& federation) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::find(missingFederations_.begin(), missingFederations_.end(), federation) !=
        missingFederations_.end();
  }

  std::vector<SynchronizationRegistrationRecord>
  synchronizationPointRegistrations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return synchronizationPointRegistrations_;
  }

  std::vector<SynchronizationAnnouncementRecord>
  synchronizationPointAnnouncements() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return synchronizationPointAnnouncements_;
  }

  std::vector<FederationSynchronizedRecord> federationSynchronized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationSynchronized_;
  }

  std::vector<std::wstring> federateSaveInitiations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federateSaveInitiations_;
  }

  std::vector<LogicalTimeRecord> federateSaveTimes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federateSaveTimes_;
  }

  std::size_t federationSavedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationSavedCount_;
  }

  std::vector<rti::SaveFailureReason> federationNotSavedReasons() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationNotSavedReasons_;
  }

  std::vector<rti::FederateHandleSaveStatusPairVector>
  federationSaveStatusResponses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationSaveStatusResponses_;
  }

  std::vector<std::wstring> federationRestoreRequestsSucceeded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationRestoreRequestsSucceeded_;
  }

  std::vector<std::wstring> federationRestoreRequestsFailed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationRestoreRequestsFailed_;
  }

  std::size_t federationRestoreBegunCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationRestoreBegunCount_;
  }

  std::vector<RestoreInitiationRecord> federateRestoreInitiations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federateRestoreInitiations_;
  }

  std::size_t federationRestoredCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationRestoredCount_;
  }

  std::vector<rti::RestoreFailureReason> federationNotRestoredReasons() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationNotRestoredReasons_;
  }

  std::vector<rti::FederateRestoreStatusVector>
  federationRestoreStatusResponses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return federationRestoreStatusResponses_;
  }

  bool reservationSucceeded(std::wstring const& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reservationSucceeded_.has_value() && reservationSucceeded_.value() == name;
  }

  bool reservationFailed(std::wstring const& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reservationFailed_.has_value() && reservationFailed_.value() == name;
  }

  bool multipleReservationSucceeded(std::set<std::wstring> const& names) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return multipleReservationSucceeded_.has_value() &&
        multipleReservationSucceeded_.value() == names;
  }

  bool multipleReservationFailed(std::set<std::wstring> const& names) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return multipleReservationFailed_.has_value() &&
        multipleReservationFailed_.value() == names;
  }

  DiscoveryRecord discovery() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return discovery_;
  }

  bool hasDiscovery(rti::ObjectInstanceHandle const& object) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(discoveries_.begin(), discoveries_.end(), [&](auto const& discovery) {
      return discovery.object == object;
    });
  }

  std::size_t discoveryCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return discoveries_.size();
  }

  std::vector<DirectedInteractionRecord> directedInteractions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return directedInteractions_;
  }

  std::vector<AttributeTransportationConfirmationRecord>
  attributeTransportationConfirmations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return attributeTransportationConfirmations_;
  }

  std::vector<AttributeTransportationReportRecord> attributeTransportationReports() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return attributeTransportationReports_;
  }

  std::vector<InteractionTransportationConfirmationRecord>
  interactionTransportationConfirmations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interactionTransportationConfirmations_;
  }

  std::vector<InteractionTransportationReportRecord> interactionTransportationReports() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interactionTransportationReports_;
  }

  std::vector<rti::ObjectClassHandle> registrationStarts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registrationStarts_;
  }

  std::vector<rti::ObjectClassHandle> registrationStops() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registrationStops_;
  }

  std::vector<rti::InteractionClassHandle> interactionsOn() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interactionsOn_;
  }

  std::vector<rti::InteractionClassHandle> interactionsOff() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interactionsOff_;
  }

  std::vector<UpdateAdvisoryRecord> updatesOn() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return updatesOn_;
  }

  std::vector<UpdateAdvisoryRecord> updatesOff() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return updatesOff_;
  }

  std::vector<RemovalRecord> removals() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return removals_;
  }

  bool hasRemoval(
      rti::ObjectInstanceHandle const& object,
      std::vector<std::uint8_t> const& tag,
      rti::FederateHandle const& producer) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(removals_.begin(), removals_.end(), [&](auto const& removal) {
      return removal.object == object && removal.tag == tag &&
          removal.producer == producer;
    });
  }

  std::vector<AttributeValueRequestRecord> providedUpdates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return providedUpdates_;
  }

  ReflectionRecord reflection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reflection_;
  }

  std::size_t reflectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reflectionCount_;
  }

  std::vector<ReflectionRecord> reflections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reflections_;
  }

  InteractionRecord interaction() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interaction_;
  }

  std::vector<InteractionRecord> interactions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interactions_;
  }

  std::vector<TimedInteractionRecord> timedInteractions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timedInteractions_;
  }

  std::vector<TimedDirectedInteractionRecord> timedDirectedInteractions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timedDirectedInteractions_;
  }

  std::vector<TimedReflectionRecord> timedReflections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timedReflections_;
  }

  std::vector<TimedRemovalRecord> timedRemovals() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timedRemovals_;
  }

  std::vector<ScopeAdvisoryRecord> attributesInScope() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return attributesInScope_;
  }

  std::vector<ScopeAdvisoryRecord> attributesOutOfScope() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return attributesOutOfScope_;
  }

  std::vector<LogicalTimeRecord> timeRegulationEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeRegulationEnabled_;
  }

  std::vector<LogicalTimeRecord> timeConstrainedEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeConstrainedEnabled_;
  }

  std::vector<LogicalTimeRecord> timeAdvanceGrants() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeAdvanceGrants_;
  }

  std::vector<FlushQueueGrantRecord> flushQueueGrants() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return flushQueueGrants_;
  }

  std::vector<std::string> callbackOrder() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return callbackOrder_;
  }

  std::vector<RetractionRecord> retractions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return retractions_;
  }

  std::optional<OwnershipInformationRecord> ownershipInformation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipInformation_;
  }

  std::vector<OwnershipInformationRecord> ownershipInformationEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipInformationEvents_;
  }

  std::optional<OwnershipEventRecord> ownershipAssumption() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipAssumption_;
  }

  std::optional<OwnershipEventRecord> divestitureConfirmation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return divestitureConfirmation_;
  }

  std::optional<OwnershipEventRecord> ownershipAcquisition() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipAcquisition_;
  }

  std::optional<OwnershipEventRecord> ownershipUnavailable() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipUnavailable_;
  }

  std::vector<OwnershipEventRecord> ownershipUnavailableEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipUnavailableEvents_;
  }

  std::optional<OwnershipEventRecord> ownershipReleaseRequest() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipReleaseRequest_;
  }

  void setOwnershipReleaseHook(
      std::function<void(
          rti::ObjectInstanceHandle const&,
          rti::AttributeHandleSet const&,
          rti::VariableLengthData const&)> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipReleaseHook_ = std::move(hook);
  }

  std::optional<OwnershipEventRecord> ownershipAcquisitionCancellation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipAcquisitionCancellation_;
  }

  std::vector<OwnershipEventRecord> ownershipAcquisitionCancellationEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipAcquisitionCancellationEvents_;
  }

  std::optional<OwnershipEventRecord> ownershipNotOwned() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipNotOwned_;
  }

  std::vector<OwnershipEventRecord> ownershipNotOwnedEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipNotOwnedEvents_;
  }

  std::optional<OwnershipEventRecord> ownershipOwnedByRti() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipOwnedByRti_;
  }

  std::vector<OwnershipEventRecord> ownershipOwnedByRtiEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipOwnedByRtiEvents_;
  }

  void clearOwnershipRecords() {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipInformation_.reset();
    ownershipInformationEvents_.clear();
    ownershipAssumption_.reset();
    divestitureConfirmation_.reset();
    ownershipAcquisition_.reset();
    ownershipAcquisitionCancellationEvents_.clear();
    ownershipUnavailable_.reset();
    ownershipUnavailableEvents_.clear();
    ownershipReleaseRequest_.reset();
    ownershipAcquisitionCancellation_.reset();
    ownershipNotOwned_.reset();
    ownershipNotOwnedEvents_.clear();
    ownershipOwnedByRti_.reset();
    ownershipOwnedByRtiEvents_.clear();
  }

  void clearReservations() {
    std::lock_guard<std::mutex> lock(mutex_);
    reservationSucceeded_.reset();
    reservationFailed_.reset();
    multipleReservationSucceeded_.reset();
    multipleReservationFailed_.reset();
  }

  void clearDiscovery() {
    std::lock_guard<std::mutex> lock(mutex_);
    discovery_ = DiscoveryRecord{};
    discoveries_.clear();
  }

  void clearReflection() {
    std::lock_guard<std::mutex> lock(mutex_);
    reflection_ = ReflectionRecord{};
    reflections_.clear();
    reflectionCount_ = 0U;
  }

  void clearInteraction() {
    std::lock_guard<std::mutex> lock(mutex_);
    interaction_ = InteractionRecord{};
    interactions_.clear();
  }

  void clearTimedInteractions() {
    std::lock_guard<std::mutex> lock(mutex_);
    timedInteractions_.clear();
  }

  void clearDirectedInteractions() {
    std::lock_guard<std::mutex> lock(mutex_);
    directedInteractions_.clear();
  }

  void clearTimedDirectedInteractions() {
    std::lock_guard<std::mutex> lock(mutex_);
    timedDirectedInteractions_.clear();
  }

  void clearTimedReflections() {
    std::lock_guard<std::mutex> lock(mutex_);
    timedReflections_.clear();
  }

  void clearTimedRemovals() {
    std::lock_guard<std::mutex> lock(mutex_);
    timedRemovals_.clear();
  }

  void clearScopeAdvisories() {
    std::lock_guard<std::mutex> lock(mutex_);
    attributesInScope_.clear();
    attributesOutOfScope_.clear();
  }

  void clearTimeCallbacks() {
    std::lock_guard<std::mutex> lock(mutex_);
    timeRegulationEnabled_.clear();
    timeConstrainedEnabled_.clear();
    timeAdvanceGrants_.clear();
  }

  void clearCallbackOrder() {
    std::lock_guard<std::mutex> lock(mutex_);
    callbackOrder_.clear();
  }

  void clearFlushQueueGrants() {
    std::lock_guard<std::mutex> lock(mutex_);
    flushQueueGrants_.clear();
  }

  void clearRetractions() {
    std::lock_guard<std::mutex> lock(mutex_);
    retractions_.clear();
  }

  void clearTransportationRecords() {
    std::lock_guard<std::mutex> lock(mutex_);
    attributeTransportationConfirmations_.clear();
    attributeTransportationReports_.clear();
    interactionTransportationConfirmations_.clear();
    interactionTransportationReports_.clear();
  }

  void clearAdvisories() {
    std::lock_guard<std::mutex> lock(mutex_);
    registrationStarts_.clear();
    registrationStops_.clear();
    interactionsOn_.clear();
    interactionsOff_.clear();
    updatesOn_.clear();
    updatesOff_.clear();
  }

  void clearRemovals() {
    std::lock_guard<std::mutex> lock(mutex_);
    removals_.clear();
  }

  void clearProvidedUpdates() {
    std::lock_guard<std::mutex> lock(mutex_);
    providedUpdates_.clear();
  }

 private:
  mutable std::mutex mutex_;
  bool connectionLost_ = false;
  std::wstring connectionLostDescription_;
  std::vector<std::wstring> federateResignedReasons_;
  std::vector<rti::FederationExecutionInformationVector> federationReports_;
  std::map<std::wstring, std::vector<rti::FederationExecutionMemberInformationVector>>
      memberReports_;
  std::vector<std::wstring> missingFederations_;
  std::vector<SynchronizationRegistrationRecord> synchronizationPointRegistrations_;
  std::vector<SynchronizationAnnouncementRecord> synchronizationPointAnnouncements_;
  std::vector<FederationSynchronizedRecord> federationSynchronized_;
  std::vector<std::wstring> federateSaveInitiations_;
  std::vector<LogicalTimeRecord> federateSaveTimes_;
  std::size_t federationSavedCount_ = 0U;
  std::vector<rti::SaveFailureReason> federationNotSavedReasons_;
  std::vector<rti::FederateHandleSaveStatusPairVector> federationSaveStatusResponses_;
  std::vector<std::wstring> federationRestoreRequestsSucceeded_;
  std::vector<std::wstring> federationRestoreRequestsFailed_;
  std::size_t federationRestoreBegunCount_ = 0U;
  std::vector<RestoreInitiationRecord> federateRestoreInitiations_;
  std::size_t federationRestoredCount_ = 0U;
  std::vector<rti::RestoreFailureReason> federationNotRestoredReasons_;
  std::vector<rti::FederateRestoreStatusVector> federationRestoreStatusResponses_;
  std::optional<std::wstring> reservationSucceeded_;
  std::optional<std::wstring> reservationFailed_;
  std::optional<std::set<std::wstring>> multipleReservationSucceeded_;
  std::optional<std::set<std::wstring>> multipleReservationFailed_;
  DiscoveryRecord discovery_;
  std::vector<DiscoveryRecord> discoveries_;
  ReflectionRecord reflection_;
  std::vector<ReflectionRecord> reflections_;
  std::size_t reflectionCount_ = 0U;
  InteractionRecord interaction_;
  std::vector<InteractionRecord> interactions_;
  std::vector<TimedInteractionRecord> timedInteractions_;
  std::vector<DirectedInteractionRecord> directedInteractions_;
  std::vector<TimedDirectedInteractionRecord> timedDirectedInteractions_;
  std::vector<LogicalTimeRecord> timeRegulationEnabled_;
  std::vector<LogicalTimeRecord> timeConstrainedEnabled_;
  std::vector<LogicalTimeRecord> timeAdvanceGrants_;
  std::vector<RetractionRecord> retractions_;
  std::vector<AttributeTransportationConfirmationRecord>
      attributeTransportationConfirmations_;
  std::vector<AttributeTransportationReportRecord> attributeTransportationReports_;
  std::vector<InteractionTransportationConfirmationRecord>
      interactionTransportationConfirmations_;
  std::vector<InteractionTransportationReportRecord> interactionTransportationReports_;
  std::vector<rti::ObjectClassHandle> registrationStarts_;
  std::vector<rti::ObjectClassHandle> registrationStops_;
  std::vector<rti::InteractionClassHandle> interactionsOn_;
  std::vector<rti::InteractionClassHandle> interactionsOff_;
  std::vector<UpdateAdvisoryRecord> updatesOn_;
  std::vector<UpdateAdvisoryRecord> updatesOff_;
  std::vector<RemovalRecord> removals_;
  std::vector<TimedReflectionRecord> timedReflections_;
  std::vector<TimedRemovalRecord> timedRemovals_;
  std::vector<ScopeAdvisoryRecord> attributesInScope_;
  std::vector<ScopeAdvisoryRecord> attributesOutOfScope_;
  std::vector<FlushQueueGrantRecord> flushQueueGrants_;
  std::vector<std::string> callbackOrder_;
  std::vector<AttributeValueRequestRecord> providedUpdates_;
  std::optional<OwnershipInformationRecord> ownershipInformation_;
  std::vector<OwnershipInformationRecord> ownershipInformationEvents_;
  std::optional<OwnershipEventRecord> ownershipAssumption_;
  std::optional<OwnershipEventRecord> divestitureConfirmation_;
  std::optional<OwnershipEventRecord> ownershipAcquisition_;
  std::vector<OwnershipEventRecord> ownershipAcquisitionCancellationEvents_;
  std::optional<OwnershipEventRecord> ownershipUnavailable_;
  std::vector<OwnershipEventRecord> ownershipUnavailableEvents_;
  std::optional<OwnershipEventRecord> ownershipReleaseRequest_;
  std::function<void(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&,
      rti::VariableLengthData const&)> ownershipReleaseHook_;
  std::optional<OwnershipEventRecord> ownershipAcquisitionCancellation_;
  std::optional<OwnershipEventRecord> ownershipNotOwned_;
  std::vector<OwnershipEventRecord> ownershipNotOwnedEvents_;
  std::optional<OwnershipEventRecord> ownershipOwnedByRti_;
  std::vector<OwnershipEventRecord> ownershipOwnedByRtiEvents_;
};

class Session final {
 public:
  Session(Options const& options, rti::CallbackModel callbackModel, std::string role)
      : options_(options), callbackModel_(callbackModel), role_(std::move(role)) {
    rti::RTIambassadorFactory factory;
    rti_ = factory.createRTIambassador();
    if (!rti_) {
      throw std::runtime_error("RTIambassadorFactory returned no ambassador");
    }
  }

  ~Session() {
    // Do not issue a potentially blocking provider service while another
    // service has already failed. The process exits after a failed scenario,
    // so the RTIambassador destructor remains the bounded final cleanup path;
    // this is especially important for an adapter fixture that intentionally
    // holds a lost peer open until the test records its callback.
    if (std::uncaught_exceptions() != 0) {
      return;
    }
    if (joined_) {
      try {
        rti_->resignFederationExecution(rti::NO_ACTION);
      } catch (...) {
      }
    }
    if (connected_) {
      try {
        rti_->disconnect();
      } catch (...) {
      }
    }
  }

  rti::ConfigurationResult connect() {
    if (connected_) {
      throw std::runtime_error("Session::connect was called twice");
    }
    auto configurationName = options_.configurationName;
    if (role_ == "owner" && !options_.ownerConfigurationName.empty()) {
      configurationName = options_.ownerConfigurationName;
    } else if (role_ != "owner" && !options_.memberConfigurationName.empty()) {
      configurationName = options_.memberConfigurationName;
    }
    rti::ConfigurationResult result;
    if (configurationName.empty() && options_.rtiAddress.empty() &&
        options_.additionalSettings.empty()) {
      result = rti_->connect(ambassador_, callbackModel_);
    } else {
      auto configuration = rti::RtiConfiguration::createConfiguration();
      if (!configurationName.empty()) {
        configuration.withConfigurationName(toWide(configurationName));
      }
      if (!options_.rtiAddress.empty()) {
        configuration.withRtiAddress(toWide(options_.rtiAddress));
      }
      if (!options_.additionalSettings.empty()) {
        configuration.withAdditionalSettings(toWide(options_.additionalSettings));
      }
      result = rti_->connect(ambassador_, callbackModel_, configuration);
    }
    connected_ = true;
    return result;
  }

  rti::ConfigurationResult connectWithConfiguration(
      rti::RtiConfiguration const& configuration) {
    if (connected_) {
      throw std::runtime_error("Session::connectWithConfiguration was called twice");
    }
    auto result = rti_->connect(ambassador_, callbackModel_, configuration);
    connected_ = true;
    return result;
  }

  void join(
      std::wstring const& federateName,
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules = {}) {
    if (!connected_) {
      throw std::runtime_error("Join was requested before Connect");
    }
    federateHandle_ = rti_->joinFederationExecution(
        federateName,
        federateType,
        federationName,
        additionalFomModules);
    if (!federateHandle_.isValid()) {
      throw std::runtime_error("Join returned an invalid federate handle");
    }
    joined_ = true;
  }

  void resign(rti::ResignAction action) {
    if (!joined_) {
      return;
    }
    rti_->resignFederationExecution(action);
    joined_ = false;
  }

  void disconnect() {
    if (!connected_) {
      return;
    }
    rti_->disconnect();
    connected_ = false;
  }

  void pump() {
    // The process adapter is a synchronous standard-API transport.  Its
    // immediate callback path still needs a bounded official callback-service
    // call to observe unsolicited frames; a provider with an asynchronous
    // receive thread simply returns with the callback already delivered.
    if (connected_ &&
        (callbackModel_ == rti::HLA_EVOKED ||
         (callbackModel_ == rti::HLA_IMMEDIATE &&
          options_.connectionLossServerManaged))) {
      static_cast<void>(rti_->evokeCallback(0.0));
    }
  }

  bool evokeMultipleCallbacks(
      double approximateMinimumTimeInSeconds,
      double approximateMaximumTimeInSeconds) {
    return rti_->evokeMultipleCallbacks(
        approximateMinimumTimeInSeconds,
        approximateMaximumTimeInSeconds);
  }

  rti::RTIambassador& rtiAmbassador() { return *rti_; }
  Recorder& recorder() { return ambassador_; }
  Recorder const& recorder() const { return ambassador_; }
  std::vector<SynchronizationRegistrationRecord>
  synchronizationPointRegistrations() const {
    return ambassador_.synchronizationPointRegistrations();
  }
  std::vector<SynchronizationAnnouncementRecord>
  synchronizationPointAnnouncements() const {
    return ambassador_.synchronizationPointAnnouncements();
  }
  std::vector<FederationSynchronizedRecord> federationSynchronized() const {
    return ambassador_.federationSynchronized();
  }
  std::vector<std::wstring> federateSaveInitiations() const {
    return ambassador_.federateSaveInitiations();
  }
  std::vector<LogicalTimeRecord> federateSaveTimes() const {
    return ambassador_.federateSaveTimes();
  }
  std::size_t federationSavedCount() const {
    return ambassador_.federationSavedCount();
  }
  std::vector<rti::SaveFailureReason> federationNotSavedReasons() const {
    return ambassador_.federationNotSavedReasons();
  }
  std::vector<rti::FederateHandleSaveStatusPairVector>
  federationSaveStatusResponses() const {
    return ambassador_.federationSaveStatusResponses();
  }
  std::vector<std::wstring> federationRestoreRequestsSucceeded() const {
    return ambassador_.federationRestoreRequestsSucceeded();
  }
  std::vector<std::wstring> federationRestoreRequestsFailed() const {
    return ambassador_.federationRestoreRequestsFailed();
  }
  std::size_t federationRestoreBegunCount() const {
    return ambassador_.federationRestoreBegunCount();
  }
  std::vector<RestoreInitiationRecord> federateRestoreInitiations() const {
    return ambassador_.federateRestoreInitiations();
  }
  std::size_t federationRestoredCount() const {
    return ambassador_.federationRestoredCount();
  }
  std::vector<rti::RestoreFailureReason> federationNotRestoredReasons() const {
    return ambassador_.federationNotRestoredReasons();
  }
  std::vector<rti::FederateRestoreStatusVector>
  federationRestoreStatusResponses() const {
    return ambassador_.federationRestoreStatusResponses();
  }
  rti::FederateHandle const& federateHandle() const { return federateHandle_; }
  std::string const& role() const { return role_; }

 private:
  Options const& options_;
  rti::CallbackModel callbackModel_;
  std::string role_;
  std::unique_ptr<rti::RTIambassador> rti_;
  Recorder ambassador_;
  rti::FederateHandle federateHandle_;
  bool connected_ = false;
  bool joined_ = false;
};

inline std::wstring federationName(
    Options const& options,
    std::string const& scenario) {
  if (!options.federationOverride.empty()) {
    return options.federationOverride;
  }
  static std::uint64_t sequence = 0U;
  auto const now = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  ++sequence;
  return toWide(options.federationPrefix + "-" + scenario + "-" +
                std::to_string(now) + "-" + std::to_string(sequence));
}

inline void require(bool condition, std::string const& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Operation>
inline void requireException(
    Operation operation,
    std::wstring const& expectedName,
    std::string const& description) {
  try {
    operation();
  } catch (rti::Exception const& error) {
    require(
        error.name() == expectedName,
        description + " threw " + toNarrow(error.name()) +
            " instead of " + toNarrow(expectedName));
    return;
  } catch (...) {
    throw std::runtime_error(description + " threw a non-HLA exception");
  }
  throw std::runtime_error(
      description + " completed without " + toNarrow(expectedName));
}

template <typename Operation>
inline void requireExceptionOneOf(
    Operation operation,
    std::vector<std::wstring> const& expectedNames,
    std::string const& description) {
  try {
    operation();
  } catch (rti::Exception const& error) {
    if (std::find(expectedNames.begin(), expectedNames.end(), error.name()) !=
        expectedNames.end()) {
      return;
    }
    std::string expected;
    for (auto const& name : expectedNames) {
      if (!expected.empty()) {
        expected += ", ";
      }
      expected += toNarrow(name);
    }
    throw std::runtime_error(
        description + " threw " + toNarrow(error.name()) +
        " instead of one of " + expected);
  } catch (...) {
    throw std::runtime_error(description + " threw a non-HLA exception");
  }
  throw std::runtime_error(
      description + " completed without an expected exception");
}

inline std::vector<std::uint8_t> encodeDataElement(
    rti::DataElement const& value) {
  return copyBytes(value.encode());
}

inline void requireEncodedBytes(
    rti::DataElement const& value,
    std::vector<std::uint8_t> const& expected,
    std::string const& description) {
  require(
      encodeDataElement(value) == expected,
      description + " produced an unexpected encoding");
}

inline void requireEncodedInto(
    rti::DataElement const& value,
    std::vector<std::uint8_t> const& expected,
    std::string const& description) {
  std::vector<rti::Octet> buffer{static_cast<rti::Octet>(0x7eU)};
  value.encodeInto(buffer);
  require(
      buffer.size() == expected.size() + 1U &&
          std::equal(
              expected.begin(),
              expected.end(),
              buffer.begin() + 1,
              [](std::uint8_t expectedByte, rti::Octet actualByte) {
                return expectedByte == static_cast<std::uint8_t>(actualByte);
              }),
      description + " did not append its expected encoding");
}

template <typename Predicate>
inline void waitFor(
    Session& session,
    Predicate predicate,
    Options const& options,
    std::string const& description) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (std::chrono::steady_clock::now() < deadline) {
    session.pump();
    if (predicate()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error("Timed out waiting for " + description);
}

template <typename Predicate>
inline void waitFor(
    Session& first,
    Session& second,
    Predicate predicate,
    Options const& options,
    std::string const& description) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (std::chrono::steady_clock::now() < deadline) {
    first.pump();
    second.pump();
    if (predicate()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error("Timed out waiting for " + description);
}

inline void connectAndJoin(
    Session& owner,
    Session& member,
    Options const& options,
    std::wstring const& federation,
    std::filesystem::path const& fom) {
  owner.connect();
  member.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
}

struct TimeContext {
  std::unique_ptr<rti::LogicalTimeFactory> factory;
  std::unique_ptr<rti::LogicalTime> initial;
  std::unique_ptr<rti::LogicalTime> finalTime;
  std::unique_ptr<rti::LogicalTimeInterval> zero;
  std::unique_ptr<rti::LogicalTimeInterval> epsilon;
};

inline TimeContext makeTimeContext(Session& session) {
  TimeContext result;
  result.factory = session.rtiAmbassador().getTimeFactory();
  require(result.factory != nullptr,
          "RTIambassador returned no logical-time factory");
  require(!result.factory->getName().empty(),
          "logical-time factory returned an empty name");
  result.initial = result.factory->makeInitial();
  result.finalTime = result.factory->makeFinal();
  result.zero = result.factory->makeZero();
  result.epsilon = result.factory->makeEpsilon();
  require(result.initial != nullptr && result.finalTime != nullptr,
          "logical-time factory returned no boundary time");
  require(result.zero != nullptr && result.epsilon != nullptr,
          "logical-time factory returned no boundary interval");
  require(result.initial->isInitial(),
          "logical-time factory initial value is not initial");
  require(result.finalTime->isFinal(),
          "logical-time factory final value is not final");
  require(result.zero->isZero(),
          "logical-time factory zero interval is not zero");
  require(result.epsilon->isEpsilon(),
          "logical-time factory epsilon interval is not epsilon");
  return result;
}

inline std::unique_ptr<rti::LogicalTime> timeAfter(
    rti::LogicalTimeFactory& factory,
    rti::LogicalTime const& base,
    rti::LogicalTimeInterval const& interval,
    std::size_t steps) {
  auto result = factory.makeInitial();
  require(result != nullptr, "logical-time factory returned no working time");
  *result = base;
  for (std::size_t step = 0U; step != steps; ++step) {
    *result += interval;
  }
  return result;
}

inline std::vector<std::uint8_t> encodeTime(rti::LogicalTime const& time) {
  return copyBytes(time.encode());
}

template <typename TimeValue>
inline void requireDirectTimeEncoding(
    TimeValue const& value,
    std::string const& description) {
  auto const expected = copyBytes(value.encode());
  require(!expected.empty(), description + " produced an empty encoding");
  require(
      expected.size() == value.encodedLength(),
      description + " reported an encoded length different from its value");

  std::vector<rti::Octet> buffer(expected.size());
  auto const written = value.encode(buffer.data(), buffer.size());
  require(
      written == expected.size(),
      description + " direct encoding returned the wrong length");
  require(
      std::equal(
          expected.begin(),
          expected.end(),
          buffer.begin(),
          [](std::uint8_t expectedByte, rti::Octet actualByte) {
            return expectedByte == static_cast<std::uint8_t>(actualByte);
          }),
      description + " direct encoding differed from its variable-length encoding");

  std::vector<rti::Octet> tooShort(expected.size() - 1U);
  requireException(
      [&] { static_cast<void>(value.encode(tooShort.data(), tooShort.size())); },
      L"CouldNotEncode",
      description + " accepted an undersized output buffer");
}

template <typename Handle>
inline void requirePortableHandleEncoding(
    Handle const& handle,
    std::string const& description) {
  auto const copiedHandle = handle;
  require(
      handle == copiedHandle && !(handle != copiedHandle) &&
          handle.hash() == copiedHandle.hash() && !(handle < copiedHandle) &&
          !(copiedHandle < handle),
      description + " copy, equality, hash, or ordering semantics were not stable");

  auto const encoded = handle.encode();
  auto const expected = copyBytes(encoded);
  require(
      !expected.empty() && encoded.size() == handle.encodedLength(),
      description + " reported an empty or inconsistent encoded length");

  rti::VariableLengthData assigned;
  handle.encode(assigned);
  require(
      copyBytes(assigned) == expected,
      description + " did not preserve bytes through VariableLengthData encoding");

  std::vector<rti::Octet> direct(expected.size());
  auto const written = handle.encode(direct.data(), direct.size());
  require(
      written == expected.size() &&
          std::equal(
              expected.begin(),
              expected.end(),
              direct.begin(),
              [](std::uint8_t left, rti::Octet right) {
                return left == static_cast<std::uint8_t>(right);
              }),
      description + " did not preserve bytes through direct-buffer encoding");

  std::vector<rti::Octet> tooShort(expected.size() - 1U);
  requireException(
      [&] { static_cast<void>(handle.encode(tooShort.data(), tooShort.size())); },
      L"CouldNotEncode",
      description + " accepted an undersized direct buffer");
}

inline std::unique_ptr<rti::LogicalTime> decodeTime(
    rti::LogicalTimeFactory& factory,
    std::vector<std::uint8_t> const& encoded) {
  return factory.decodeLogicalTime(encoded.data(), encoded.size());
}

inline void requireTimeEquals(
    rti::LogicalTime const& actual,
    rti::LogicalTime const& expected,
    std::string const& description) {
  require(actual == expected, description + " returned the wrong logical time");
}

inline void verifyStandardLogicalTimeFactoryFactoryContract() {
  auto defaultFactory = rti::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"");
  require(
      defaultFactory != nullptr &&
          defaultFactory->getName() == L"HLAfloat64Time" &&
          dynamic_cast<rti::HLAfloat64TimeFactory*>(defaultFactory.get()) != nullptr,
      "HLAlogicalTimeFactoryFactory did not select the standard default factory");
  auto selectedIntegerFactory =
      rti::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"HLAinteger64Time");
  require(
      selectedIntegerFactory != nullptr &&
          selectedIntegerFactory->getName() == L"HLAinteger64Time" &&
          dynamic_cast<rti::HLAinteger64TimeFactory*>(selectedIntegerFactory.get()) != nullptr,
      "HLAlogicalTimeFactoryFactory did not select HLAinteger64Time");
  auto forwardedDefaultFactory =
      rti::LogicalTimeFactoryFactory::makeLogicalTimeFactory(L"");
  require(
      forwardedDefaultFactory != nullptr &&
          forwardedDefaultFactory->getName() == L"HLAfloat64Time",
      "LogicalTimeFactoryFactory did not forward the standard default selection");
  require(
      rti::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"missing") == nullptr &&
          rti::LogicalTimeFactoryFactory::makeLogicalTimeFactory(L"missing") == nullptr,
      "logical-time factory selection accepted an unknown implementation name");
  std::wostringstream defaultTimeText;
  defaultTimeText << *defaultFactory->makeInitial();
  require(
      defaultTimeText.str() == L"0",
      "the standard default logical-time factory did not produce the initial value");
}

}  // namespace hla_rti_cpp_tck::detail
