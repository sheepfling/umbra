#include <RTI/FederateAmbassador.h>
#include <RTI/Enums.h>
#include <RTI/RTIambassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/VariableLengthData.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace rti = rti1516_2025;

using Clock = std::chrono::steady_clock;

struct Options {
  std::filesystem::path fom;
  std::vector<std::string> scenarios;
  std::string callbackModel = "both";
  std::string providerId = "unspecified";
  std::string results;
  std::string junit;
  std::string connectionLossCommand;
  std::string connectionLossMarker;
  bool connectionLossServerManaged = false;
  std::string federationPrefix = "cpp-tck";
  std::string configurationName;
  std::string ownerConfigurationName;
  std::string memberConfigurationName;
  std::string rtiAddress;
  std::string additionalSettings;
  std::wstring federationOverride;
  std::wstring ownerFederateName = L"tck-owner";
  std::wstring memberFederateName = L"tck-member";
  std::wstring federateType = L"tck-type";
  std::wstring objectClassName = L"HLAobjectRoot.TckObject";
  std::wstring attributeName = L"Value";
  std::wstring interactionClassName = L"HLAinteractionRoot.TckInteraction";
  std::wstring parameterName = L"Payload";
  std::wstring logicalTimeImplementationName;
  int timeoutMilliseconds = 5000;
};

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
};

struct InteractionRecord {
  bool present = false;
  rti::InteractionClassHandle interaction;
  rti::ParameterHandleValueMap parameters;
  std::vector<std::uint8_t> tag;
  rti::TransportationTypeHandle transportation;
  rti::FederateHandle producer;
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

std::wstring toWide(std::string const& value) {
  return std::wstring(value.begin(), value.end());
}

std::string toNarrow(std::wstring const& value) {
  std::string result;
  result.reserve(value.size());
  for (auto const character : value) {
    result += character >= 0 && character <= 0x7f
        ? static_cast<char>(character)
        : '?';
  }
  return result;
}

std::vector<std::uint8_t> copyBytes(rti::VariableLengthData const& data) {
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

  void federateResigned(std::wstring const&) override {}

  void synchronizationPointRegistrationSucceeded(std::wstring const&) override {}

  void synchronizationPointRegistrationFailed(
      std::wstring const&,
      rti::SynchronizationPointFailureReason) override {}

  void announceSynchronizationPoint(
      std::wstring const&,
      rti::VariableLengthData const&) override {}

  void federationSynchronized(
      std::wstring const&,
      rti::FederateHandleSet const&) override {}

  void initiateFederateSave(std::wstring const&) override {}

  void initiateFederateSave(
      std::wstring const&,
      rti::LogicalTime const&) override {}

  void federationSaved() override {}

  void federationNotSaved(rti::SaveFailureReason) override {}

  void federationSaveStatusResponse(
      rti::FederateHandleSaveStatusPairVector const&) override {}

  void requestFederationRestoreSucceeded(std::wstring const&) override {}

  void requestFederationRestoreFailed(std::wstring const&) override {}

  void federationRestoreBegun() override {}

  void initiateFederateRestore(
      std::wstring const&,
      std::wstring const&,
      rti::FederateHandle const&) override {}

  void federationRestored() override {}

  void federationNotRestored(rti::RestoreFailureReason) override {}

  void federationRestoreStatusResponse(
      rti::FederateRestoreStatusVector const&) override {}

  void startRegistrationForObjectClass(
      rti::ObjectClassHandle const&) override {}

  void stopRegistrationForObjectClass(
      rti::ObjectClassHandle const&) override {}

  void turnInteractionsOn(
      rti::InteractionClassHandle const&) override {}

  void turnInteractionsOff(
      rti::InteractionClassHandle const&) override {}

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
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const*) override {
    std::lock_guard<std::mutex> lock(mutex_);
    reflection_.present = true;
    reflection_.object = object;
    reflection_.values = values;
    reflection_.tag = copyBytes(tag);
    reflection_.transportation = transportation;
    reflection_.producer = producer;
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions,
      rti::LogicalTime const&,
      rti::OrderType,
      rti::OrderType,
      rti::MessageRetractionHandle const*) override {
    reflectAttributeValues(object, values, tag, transportation, producer, regions);
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interaction,
      rti::ParameterHandleValueMap const& parameters,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const*) override {
    std::lock_guard<std::mutex> lock(mutex_);
    interaction_.present = true;
    interaction_.interaction = interaction;
    interaction_.parameters = parameters;
    interaction_.tag = copyBytes(tag);
    interaction_.transportation = transportation;
    interaction_.producer = producer;
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interaction,
      rti::ParameterHandleValueMap const& parameters,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions,
      rti::LogicalTime const&,
      rti::OrderType,
      rti::OrderType,
      rti::MessageRetractionHandle const*) override {
    receiveInteraction(interaction, parameters, tag, transportation, producer, regions);
  }

  void multipleObjectInstanceNameReservationSucceeded(
      std::set<std::wstring> const&) override {}

  void multipleObjectInstanceNameReservationFailed(
      std::set<std::wstring> const&) override {}

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const&,
      rti::ObjectInstanceHandle const&,
      rti::ParameterHandleValueMap const&,
      rti::VariableLengthData const&,
      rti::TransportationTypeHandle const&,
      rti::FederateHandle const&) override {}

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const&,
      rti::ObjectInstanceHandle const&,
      rti::ParameterHandleValueMap const&,
      rti::VariableLengthData const&,
      rti::TransportationTypeHandle const&,
      rti::FederateHandle const&,
      rti::LogicalTime const&,
      rti::OrderType,
      rti::OrderType,
      rti::MessageRetractionHandle const*) override {}

  void removeObjectInstance(
      rti::ObjectInstanceHandle const&,
      rti::VariableLengthData const&,
      rti::FederateHandle const&) override {}

  void removeObjectInstance(
      rti::ObjectInstanceHandle const&,
      rti::VariableLengthData const&,
      rti::FederateHandle const&,
      rti::LogicalTime const&,
      rti::OrderType,
      rti::OrderType,
      rti::MessageRetractionHandle const*) override {}

  void attributesInScope(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&) override {}

  void attributesOutOfScope(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&) override {}

  void provideAttributeValueUpdate(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&,
      rti::VariableLengthData const&) override {}

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&) override {}

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&,
      std::wstring const&) override {}

  void turnUpdatesOffForObjectInstance(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&) override {}

  void confirmAttributeTransportationTypeChange(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&,
      rti::TransportationTypeHandle const&) override {}

  void reportAttributeTransportationType(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandle const&,
      rti::TransportationTypeHandle const&) override {}

  void confirmInteractionTransportationTypeChange(
      rti::InteractionClassHandle const&,
      rti::TransportationTypeHandle const&) override {}

  void reportInteractionTransportationType(
      rti::FederateHandle const&,
      rti::InteractionClassHandle const&,
      rti::TransportationTypeHandle const&) override {}

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
    ownershipUnavailable_ = OwnershipEventRecord{object, attributes, copyBytes(tag)};
  }

  void requestAttributeOwnershipRelease(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipReleaseRequest_ = OwnershipEventRecord{object, attributes, copyBytes(tag)};
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipAcquisitionCancellation_ = OwnershipEventRecord{object, attributes, {}};
  }

  void informAttributeOwnership(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::FederateHandle const& owner) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipInformation_ = OwnershipInformationRecord{object, attributes, owner};
  }

  void attributeIsNotOwned(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipNotOwned_ = OwnershipEventRecord{object, attributes, {}};
  }

  void attributeIsOwnedByRTI(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipOwnedByRti_ = OwnershipEventRecord{object, attributes, {}};
  }

  void timeRegulationEnabled(rti::LogicalTime const&) override {}

  void timeConstrainedEnabled(rti::LogicalTime const&) override {}

  void flushQueueGrant(
      rti::LogicalTime const&,
      rti::LogicalTime const&) override {}

  void timeAdvanceGrant(rti::LogicalTime const&) override {}

  void requestRetraction(rti::MessageRetractionHandle const&) override {}

  bool hasConnectionLost() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connectionLost_;
  }

  std::wstring connectionLostDescription() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connectionLostDescription_;
  }

  bool listsFederation(std::wstring const& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto const& report : federationReports_) {
      for (auto const& execution : report) {
        if (execution.federationExecutionName == name) {
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

  bool reportsMissing(std::wstring const& federation) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::find(missingFederations_.begin(), missingFederations_.end(), federation) !=
        missingFederations_.end();
  }

  bool reservationSucceeded(std::wstring const& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reservationSucceeded_.has_value() && reservationSucceeded_.value() == name;
  }

  bool reservationFailed(std::wstring const& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reservationFailed_.has_value() && reservationFailed_.value() == name;
  }

  DiscoveryRecord discovery() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return discovery_;
  }

  ReflectionRecord reflection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reflection_;
  }

  InteractionRecord interaction() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interaction_;
  }

  std::optional<OwnershipInformationRecord> ownershipInformation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipInformation_;
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

  std::optional<OwnershipEventRecord> ownershipReleaseRequest() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipReleaseRequest_;
  }

  std::optional<OwnershipEventRecord> ownershipAcquisitionCancellation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipAcquisitionCancellation_;
  }

  std::optional<OwnershipEventRecord> ownershipNotOwned() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipNotOwned_;
  }

  std::optional<OwnershipEventRecord> ownershipOwnedByRti() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipOwnedByRti_;
  }

  void clearOwnershipRecords() {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipInformation_.reset();
    ownershipAssumption_.reset();
    divestitureConfirmation_.reset();
    ownershipAcquisition_.reset();
    ownershipUnavailable_.reset();
    ownershipReleaseRequest_.reset();
    ownershipAcquisitionCancellation_.reset();
    ownershipNotOwned_.reset();
    ownershipOwnedByRti_.reset();
  }

 private:
  mutable std::mutex mutex_;
  bool connectionLost_ = false;
  std::wstring connectionLostDescription_;
  std::vector<rti::FederationExecutionInformationVector> federationReports_;
  std::map<std::wstring, std::vector<rti::FederationExecutionMemberInformationVector>>
      memberReports_;
  std::vector<std::wstring> missingFederations_;
  std::optional<std::wstring> reservationSucceeded_;
  std::optional<std::wstring> reservationFailed_;
  DiscoveryRecord discovery_;
  ReflectionRecord reflection_;
  InteractionRecord interaction_;
  std::optional<OwnershipInformationRecord> ownershipInformation_;
  std::optional<OwnershipEventRecord> ownershipAssumption_;
  std::optional<OwnershipEventRecord> divestitureConfirmation_;
  std::optional<OwnershipEventRecord> ownershipAcquisition_;
  std::optional<OwnershipEventRecord> ownershipUnavailable_;
  std::optional<OwnershipEventRecord> ownershipReleaseRequest_;
  std::optional<OwnershipEventRecord> ownershipAcquisitionCancellation_;
  std::optional<OwnershipEventRecord> ownershipNotOwned_;
  std::optional<OwnershipEventRecord> ownershipOwnedByRti_;
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

  void connect() {
    if (connected_) {
      return;
    }
    auto configurationName = options_.configurationName;
    if (role_ == "owner" && !options_.ownerConfigurationName.empty()) {
      configurationName = options_.ownerConfigurationName;
    } else if (role_ == "member" && !options_.memberConfigurationName.empty()) {
      configurationName = options_.memberConfigurationName;
    }
    if (configurationName.empty() && options_.rtiAddress.empty() &&
        options_.additionalSettings.empty()) {
      rti_->connect(ambassador_, callbackModel_);
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
      rti_->connect(ambassador_, callbackModel_, configuration);
    }
    connected_ = true;
  }

  void join(
      std::wstring const& federateName,
      std::wstring const& federateType,
      std::wstring const& federationName) {
    if (!connected_) {
      throw std::runtime_error("Join was requested before Connect");
    }
    federateHandle_ = rti_->joinFederationExecution(
        federateName,
        federateType,
        federationName,
        {});
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
    if (callbackModel_ == rti::HLA_EVOKED && connected_) {
      static_cast<void>(rti_->evokeCallback(0.0));
    }
  }

  rti::RTIambassador& rtiAmbassador() { return *rti_; }
  Recorder& recorder() { return ambassador_; }
  Recorder const& recorder() const { return ambassador_; }
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

std::wstring federationName(Options const& options, std::string const& scenario) {
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

void require(bool condition, std::string const& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Predicate>
void waitFor(
    Session& session,
    Predicate predicate,
    Options const& options,
    std::string const& description) {
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    session.pump();
    if (predicate()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error("Timed out waiting for " + description);
}

template <typename Predicate>
void waitFor(
    Session& first,
    Session& second,
    Predicate predicate,
    Options const& options,
    std::string const& description) {
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    first.pump();
    second.pump();
    if (predicate()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error("Timed out waiting for " + description);
}

void connectAndJoin(
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

void handles(
    Session& session,
    Options const& options,
    rti::ObjectClassHandle& objectClass,
    rti::AttributeHandle& attribute,
    rti::InteractionClassHandle& interactionClass,
    rti::ParameterHandle& parameter) {
  auto& rtiAmbassador = session.rtiAmbassador();
  objectClass = rtiAmbassador.getObjectClassHandle(options.objectClassName);
  attribute = rtiAmbassador.getAttributeHandle(objectClass, options.attributeName);
  interactionClass = rtiAmbassador.getInteractionClassHandle(options.interactionClassName);
  parameter = rtiAmbassador.getParameterHandle(interactionClass, options.parameterName);
  require(objectClass.isValid(), "Object class lookup returned an invalid handle");
  require(attribute.isValid(), "Attribute lookup returned an invalid handle");
  require(interactionClass.isValid(), "Interaction class lookup returned an invalid handle");
  require(parameter.isValid(), "Parameter lookup returned an invalid handle");
}

void scenarioConnection(Options const& options, rti::CallbackModel model) {
  Session session(options, model, "connection");
  session.connect();
  session.rtiAmbassador().disableCallbacks();
  session.rtiAmbassador().enableCallbacks();
  session.disconnect();
}

void scenarioFederationLifecycle(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  Session observer(options, model, "observer");
  auto const federation = federationName(options, "lifecycle");
  owner.connect();
  member.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);

  require(owner.rtiAmbassador().getFederateHandle(options.ownerFederateName) ==
              owner.federateHandle(),
          "Federate name-to-handle lookup did not round-trip");
  require(owner.rtiAmbassador().getFederateName(owner.federateHandle()) ==
              options.ownerFederateName,
          "Federate handle-to-name lookup did not round-trip");

  observer.rtiAmbassador().listFederationExecutions();
  waitFor(observer, [&] { return observer.recorder().listsFederation(federation); },
          options, "the federation execution report");

  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(observer, [&] {
    return observer.recorder().listsMembers(
        federation, {options.ownerFederateName, options.memberFederateName}, 2U);
  }, options, "the complete federation member report");

  member.resign(rti::NO_ACTION);
  member.disconnect();
  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(observer, [&] {
    return observer.recorder().listsMembers(federation, {options.ownerFederateName}, 1U);
  }, options, "the post-resign member report");

  owner.resign(rti::NO_ACTION);
  owner.disconnect();
  observer.rtiAmbassador().destroyFederationExecution(federation);
  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(observer, [&] { return observer.recorder().reportsMissing(federation); },
          options, "the federation-does-not-exist report");
  observer.disconnect();
}

void scenarioHandleLookups(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "lookups");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  require(owner.rtiAmbassador().getObjectClassName(objectClass) == options.objectClassName,
          "Object class handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getAttributeName(objectClass, attribute) == options.attributeName,
          "Attribute handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getInteractionClassName(interactionClass) ==
              options.interactionClassName,
          "Interaction class handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getParameterName(interactionClass, parameter) ==
              options.parameterName,
          "Parameter handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE,
          "Receive order name lookup did not resolve");
  require(owner.rtiAmbassador().getOrderName(rti::RECEIVE) == L"Receive",
          "Receive order handle lookup did not round-trip");
  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(reliable.isValid(), "Reliable transportation lookup returned an invalid handle");
  require(owner.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
          "Reliable transportation lookup did not round-trip");
}

void scenarioDeclarations(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "declarations");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  owner.rtiAmbassador().publishInteractionClass(interactionClass);
  member.rtiAmbassador().subscribeInteractionClass(interactionClass, true);

  member.rtiAmbassador().unsubscribeObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().unsubscribeObjectClass(objectClass);
  member.rtiAmbassador().unsubscribeInteractionClass(interactionClass);
  owner.rtiAmbassador().unpublishObjectClassAttributes(objectClass, attributes);
  owner.rtiAmbassador().unpublishObjectClass(objectClass);
  owner.rtiAmbassador().unpublishInteractionClass(interactionClass);
}

void scenarioObjectRegistration(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "object-registration");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Object registration returned an invalid handle");
  auto const name = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!name.empty(), "Object registration returned an empty instance name");

  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "object discovery");
  auto const discovered = member.recorder().discovery();
  require(discovered.object == object, "Discovery returned the wrong object handle");
  require(discovered.objectClass == objectClass, "Discovery returned the wrong object class");
  require(discovered.name == name, "Discovery returned the wrong object name");
  require(discovered.producer == owner.federateHandle(),
          "Discovery returned the wrong producing federate");
  require(member.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Object instance name lookup did not round-trip");
  require(member.rtiAmbassador().getKnownObjectClassHandle(object) == objectClass,
          "Known object class lookup did not round-trip");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioAttributeUpdate(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "attribute-update");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Attribute update registration returned an invalid handle");
  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "attribute update object discovery");

  std::vector<std::uint8_t> value{0x10U, 0x20U, 0x30U};
  std::vector<std::uint8_t> tagBytes{0x54U, 0x43U, 0x4BU};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  rti::AttributeHandleValueMap values;
  values.emplace(attribute, rti::VariableLengthData(value.data(), value.size()));
  owner.rtiAmbassador().updateAttributeValues(object, values, tag);

  waitFor(member, [&] { return member.recorder().reflection().present; }, options,
          "ordinary attribute reflection");
  auto const reflected = member.recorder().reflection();
  require(reflected.object == object, "Reflection returned the wrong object handle");
  require(reflected.values.size() == 1U && reflected.values.count(attribute) == 1U,
          "Reflection returned the wrong attribute set");
  require(copyBytes(reflected.values.at(attribute)) == value,
          "Reflection returned the wrong attribute value");
  require(reflected.tag == tagBytes, "Reflection returned the wrong user tag");
  require(reflected.producer == owner.federateHandle(),
          "Reflection returned the wrong producing federate");
  require(reflected.transportation.isValid(),
          "Reflection returned an invalid transportation handle");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioInteraction(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "interaction");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  owner.rtiAmbassador().publishInteractionClass(interactionClass);
  member.rtiAmbassador().subscribeInteractionClass(interactionClass, true);

  std::vector<std::uint8_t> value{0x42U, 0x11U};
  std::vector<std::uint8_t> tagBytes{0x49U, 0x4EU, 0x54U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(parameter, rti::VariableLengthData(value.data(), value.size()));
  owner.rtiAmbassador().sendInteraction(interactionClass, parameters, tag);

  waitFor(member, [&] { return member.recorder().interaction().present; }, options,
          "ordinary interaction delivery");
  auto const received = member.recorder().interaction();
  require(received.interaction == interactionClass,
          "Interaction delivery returned the wrong class handle");
  require(received.parameters.size() == 1U && received.parameters.count(parameter) == 1U,
          "Interaction delivery returned the wrong parameter set");
  require(copyBytes(received.parameters.at(parameter)) == value,
          "Interaction delivery returned the wrong parameter value");
  require(received.tag == tagBytes, "Interaction delivery returned the wrong user tag");
  require(received.producer == owner.federateHandle(),
          "Interaction delivery returned the wrong producing federate");
  require(received.transportation.isValid(),
          "Interaction delivery returned an invalid transportation handle");

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
}

void scenarioNamedRegistration(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "named-registration");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  auto const name = federation + L"-named-object";
  owner.rtiAmbassador().reserveObjectInstanceName(name);
  waitFor(owner, [&] { return owner.recorder().reservationSucceeded(name); }, options,
          "named object reservation");
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass, name);
  require(object.isValid(), "Named registration returned an invalid handle");
  require(owner.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Named registration lookup did not round-trip at the owner");

  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "named object discovery");
  auto const discovered = member.recorder().discovery();
  require(discovered.object == object, "Named discovery returned the wrong object handle");
  require(discovered.name == name, "Named discovery returned the wrong instance name");
  require(member.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Named registration lookup did not round-trip at the member");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioObjectManagement(Options const& options, rti::CallbackModel model) {
  scenarioObjectRegistration(options, model);
  scenarioNamedRegistration(options, model);
}

void scenarioAttributeInteraction(Options const& options, rti::CallbackModel model) {
  scenarioAttributeUpdate(options, model);
  scenarioInteraction(options, model);
}

void scenarioOwnership(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session acquirer(options, model, "acquirer");
  auto const federation = federationName(options, "ownership");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const acquirerClass =
      acquirer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute =
      owner.rtiAmbassador().getAttributeHandle(ownerClass, options.attributeName);
  auto const acquirerAttribute =
      acquirer.rtiAmbassador().getAttributeHandle(acquirerClass, options.attributeName);
  require(ownerClass.isValid() && acquirerClass.isValid(),
          "Ownership object class lookup returned an invalid handle");
  require(ownerAttribute.isValid() && acquirerAttribute.isValid(),
          "Ownership attribute lookup returned an invalid handle");

  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(ownerAttribute);
  rti::AttributeHandleSet acquirerAttributes;
  acquirerAttributes.insert(acquirerAttribute);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(acquirerClass, acquirerAttributes);
  owner.rtiAmbassador().subscribeObjectClassAttributes(
      ownerClass, ownerAttributes, true, L"");
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass, acquirerAttributes, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Ownership registration returned an invalid object handle");
  waitFor(acquirer, [&] { return acquirer.recorder().discovery().present; }, options,
          "ownership object discovery");
  auto const discovered = acquirer.recorder().discovery();
  require(discovered.object == object,
          "Ownership discovery returned the wrong object handle");

  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner did not initially own the published attribute");
  require(!acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer initially reported ownership of the attribute");

  // Querying through the subscriber exercises the owner report and the
  // standard cross-federate handle identity.
  acquirer.recorder().clearOwnershipRecords();
  acquirer.rtiAmbassador().queryAttributeOwnership(object, acquirerAttributes);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipInformation().has_value();
  }, options, "the ownership query report");
  auto const information = acquirer.recorder().ownershipInformation();
  require(information->object == object && information->attributes == acquirerAttributes,
          "Ownership query returned the wrong object or attribute set");
  require(information->owner == owner.federateHandle(),
          "Ownership query did not identify the owning federate");

  // Negotiated acquisition: the acquirer's tag reaches the owner's request,
  // and the owner's confirmation tag reaches the acquisition notification.
  acquirer.recorder().clearOwnershipRecords();
  owner.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> divestitureTagBytes{0x10U};
  std::vector<std::uint8_t> acquisitionRequestTagBytes{0x20U};
  std::vector<std::uint8_t> confirmationTagBytes{0x30U};
  rti::VariableLengthData divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  rti::VariableLengthData acquisitionRequestTag(
      acquisitionRequestTagBytes.data(), acquisitionRequestTagBytes.size());
  rti::VariableLengthData confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, acquisitionRequestTag);
  waitFor(owner, [&] {
    return owner.recorder().divestitureConfirmation().has_value();
  }, options, "the negotiated divestiture confirmation request");
  auto const divestitureConfirmation = owner.recorder().divestitureConfirmation();
  require(divestitureConfirmation->object == object &&
              divestitureConfirmation->attributes == ownerAttributes,
          "Negotiated divestiture returned the wrong object or attributes");
  require(divestitureConfirmation->tag == acquisitionRequestTagBytes,
          "Negotiated divestiture did not preserve the acquisition tag");

  owner.rtiAmbassador().confirmDivestiture(object, ownerAttributes, confirmationTag);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipAcquisition().has_value();
  }, options, "the negotiated acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(acquisition->object == object && acquisition->attributes == acquirerAttributes,
          "Acquisition notification returned the wrong object or attributes");
  require(acquisition->tag == confirmationTagBytes,
          "Acquisition notification did not preserve the confirmation tag");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner retained ownership after confirmed divestiture");
  require(acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer did not gain ownership after confirmation");

  // The If Wanted service is valid for an owner even when there is no pending
  // acquisition. The returned set is provider-arbitrated by the standard API.
  rti::AttributeHandleSet wanted;
  std::vector<std::uint8_t> noPendingDivestitureTagBytes{0x31U};
  rti::VariableLengthData noPendingDivestitureTag(
      noPendingDivestitureTagBytes.data(), noPendingDivestitureTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      acquirerAttributes,
      noPendingDivestitureTag,
      wanted);
  require(wanted.empty(),
          "If Wanted unexpectedly divested an attribute without a pending acquirer");

  // An unconditional divestiture makes the attribute immediately available;
  // the owner can then reacquire it through the If Available service.
  acquirer.recorder().clearOwnershipRecords();
  owner.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> unconditionalTagBytes{0x40U};
  std::vector<std::uint8_t> ifAvailableTagBytes{0x50U};
  rti::VariableLengthData unconditionalTag(
      unconditionalTagBytes.data(), unconditionalTagBytes.size());
  rti::VariableLengthData ifAvailableTag(
      ifAvailableTagBytes.data(), ifAvailableTagBytes.size());
  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, acquirerAttributes, unconditionalTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAssumption().has_value();
  }, options, "the ownership assumption offer");
  auto const assumption = owner.recorder().ownershipAssumption();
  require(assumption->object == object && assumption->attributes == ownerAttributes,
          "Ownership assumption returned the wrong object or attributes");
  require(assumption->tag == unconditionalTagBytes,
          "Ownership assumption did not preserve the divestiture tag");

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, ownerAttributes);
  waitFor(owner, [&] {
    return owner.recorder().ownershipNotOwned().has_value();
  }, options, "the unowned ownership query report");
  auto const unowned = owner.recorder().ownershipNotOwned();
  require(unowned->object == object && unowned->attributes == ownerAttributes,
          "Unowned ownership query returned the wrong object or attributes");

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, ownerAttributes, ifAvailableTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAcquisition().has_value();
  }, options, "the If Available acquisition notification");
  auto const ifAvailableAcquisition = owner.recorder().ownershipAcquisition();
  require(ifAvailableAcquisition->object == object &&
              ifAvailableAcquisition->attributes == ownerAttributes,
          "If Available acquisition returned the wrong object or attributes");
  require(ifAvailableAcquisition->tag == ifAvailableTagBytes,
          "If Available acquisition did not preserve its user tag");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner did not regain ownership through If Available acquisition");
  require(!acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer retained ownership after unconditional divestiture");

  // A pending regular acquisition can also be satisfied directly by the
  // current owner through Divestiture If Wanted. The divestiture tag, rather
  // than the older acquisition tag, is delivered to the new owner.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> ifWantedAcquisitionTagBytes{0xA0U};
  std::vector<std::uint8_t> ifWantedDivestitureTagBytes{0xA1U};
  rti::VariableLengthData ifWantedAcquisitionTag(
      ifWantedAcquisitionTagBytes.data(), ifWantedAcquisitionTagBytes.size());
  rti::VariableLengthData ifWantedDivestitureTag(
      ifWantedDivestitureTagBytes.data(), ifWantedDivestitureTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, ifWantedAcquisitionTag);
  rti::AttributeHandleSet ifWantedDivestedAttributes;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      ifWantedDivestitureTag,
      ifWantedDivestedAttributes);
  require(ifWantedDivestedAttributes == ownerAttributes,
          "If Wanted did not return the transferred attribute set");
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipAcquisition().has_value();
  }, options, "the If Wanted acquisition notification");
  auto const ifWantedAcquisition = acquirer.recorder().ownershipAcquisition();
  require(ifWantedAcquisition->object == object &&
              ifWantedAcquisition->attributes == acquirerAttributes,
          "If Wanted acquisition returned the wrong object or attributes");
  require(ifWantedAcquisition->tag == ifWantedDivestitureTagBytes,
          "If Wanted acquisition did not preserve the divestiture tag");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner retained ownership after If Wanted transfer");
  require(acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer did not gain ownership through If Wanted transfer");

  // Return the attribute to the owner so the denial and cancellation checks
  // start from the same owner-held state as the Java parity scenario.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> returnAcquisitionTagBytes{0xB0U};
  std::vector<std::uint8_t> returnDivestitureTagBytes{0xB1U};
  rti::VariableLengthData returnAcquisitionTag(
      returnAcquisitionTagBytes.data(), returnAcquisitionTagBytes.size());
  rti::VariableLengthData returnDivestitureTag(
      returnDivestitureTagBytes.data(), returnDivestitureTagBytes.size());
  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      acquirerAttributes,
      returnDivestitureTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAssumption().has_value();
  }, options, "the return ownership assumption offer");
  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, ownerAttributes, returnAcquisitionTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAcquisition().has_value();
  }, options, "the return If Available acquisition notification");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner did not regain ownership before denial checks");

  // A regular acquisition asks the current owner to release. Denying it must
  // preserve ownership and deliver the denial tag as Unavailable.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> deniedAcquisitionTagBytes{0x60U};
  std::vector<std::uint8_t> denialTagBytes{0x70U};
  rti::VariableLengthData deniedAcquisitionTag(
      deniedAcquisitionTagBytes.data(), deniedAcquisitionTagBytes.size());
  rti::VariableLengthData denialTag(denialTagBytes.data(), denialTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, deniedAcquisitionTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipReleaseRequest().has_value();
  }, options, "the ownership release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(releaseRequest->object == object && releaseRequest->attributes == ownerAttributes,
          "Ownership release request returned the wrong object or attributes");
  require(releaseRequest->tag == deniedAcquisitionTagBytes,
          "Ownership release request did not preserve the acquisition tag");
  owner.rtiAmbassador().attributeOwnershipReleaseDenied(object, ownerAttributes, denialTag);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipUnavailable().has_value();
  }, options, "the ownership-unavailable denial callback");
  auto const unavailable = acquirer.recorder().ownershipUnavailable();
  require(unavailable->object == object && unavailable->attributes == acquirerAttributes,
          "Ownership-unavailable callback returned the wrong object or attributes");
  require(unavailable->tag == denialTagBytes,
          "Ownership-unavailable callback did not preserve the denial tag");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner lost ownership after denying the acquisition");

  // A second pending acquisition can be canceled, and the requester receives
  // the standard confirmation callback.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> cancellationAcquisitionTagBytes{0x80U};
  rti::VariableLengthData cancellationAcquisitionTag(
      cancellationAcquisitionTagBytes.data(), cancellationAcquisitionTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, cancellationAcquisitionTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipReleaseRequest().has_value();
  }, options, "the cancelable ownership release request");
  acquirer.rtiAmbassador().cancelAttributeOwnershipAcquisition(object, acquirerAttributes);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipAcquisitionCancellation().has_value();
  }, options, "the ownership acquisition cancellation confirmation");
  auto const cancellation = acquirer.recorder().ownershipAcquisitionCancellation();
  require(cancellation->object == object && cancellation->attributes == acquirerAttributes,
          "Ownership cancellation returned the wrong object or attributes");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner lost ownership after acquisition cancellation");

  std::vector<std::uint8_t> canceledDivestitureTagBytes{0x90U};
  rti::VariableLengthData canceledDivestitureTag(
      canceledDivestitureTagBytes.data(), canceledDivestitureTagBytes.size());
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, canceledDivestitureTag);
  owner.rtiAmbassador().cancelNegotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes);
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner lost ownership after negotiated divestiture cancellation");

  owner.resign(rti::DELETE_OBJECTS);
  acquirer.resign(rti::NO_ACTION);
}

std::string replaceAll(std::string value, std::string const& needle, std::string const& replacement) {
  std::size_t position = 0U;
  while ((position = value.find(needle, position)) != std::string::npos) {
    value.replace(position, needle.size(), replacement);
    position += replacement.size();
  }
  return value;
}

void scenarioConnectionLoss(Options const& options, rti::CallbackModel model) {
  require(options.connectionLossServerManaged || !options.connectionLossCommand.empty(),
          "Connection-loss cleanup requires an adapter-supplied trigger command");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "connection-loss");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto command = replaceAll(options.connectionLossCommand, "{federation}", toNarrow(federation));
  command = replaceAll(command, "{owner}", toNarrow(options.ownerFederateName));
  command = replaceAll(command, "{member}", toNarrow(options.memberFederateName));
  std::optional<std::future<int>> triggerStatus;
  if (!options.connectionLossServerManaged) {
    triggerStatus.emplace(std::async(
        std::launch::async,
        [command] { return std::system(command.c_str()); }));
  }

  auto const lossDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < lossDeadline && !member.recorder().hasConnectionLost()) {
    if (model == rti::HLA_EVOKED) {
      try {
        static_cast<void>(member.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
      } catch (...) {
        // A provider may surface the socket fault on the Evoke operation
        // before placing the normative callback on the next callback turn.
      }
    }
    if (!member.recorder().hasConnectionLost()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  require(member.recorder().hasConnectionLost(),
          "Timed out waiting for the connectionLost callback");
  require(!member.recorder().connectionLostDescription().empty(),
          "connectionLost did not provide a fault description");
  if (!options.connectionLossMarker.empty()) {
    std::ofstream marker(
        options.connectionLossMarker,
        std::ios::binary | std::ios::trunc);
    require(static_cast<bool>(marker), "Could not write the connection-loss completion marker");
    marker << "callback-ok\n";
    require(static_cast<bool>(marker), "Could not flush the connection-loss completion marker");
  }

  rti::InteractionClassHandle interactionClass =
      owner.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  require(interactionClass.isValid(), "The surviving member could not resolve an interaction class");
  owner.rtiAmbassador().sendInteraction(
      interactionClass, rti::ParameterHandleValueMap{}, rti::VariableLengthData{});
  try {
    member.disconnect();
  } catch (...) {
    // Some providers transition the lost member to a disconnected state
    // before delivering connectionLost. Cleanup is best effort after loss.
  }
  owner.resign(rti::NO_ACTION);
  owner.disconnect();
  if (triggerStatus.has_value()) {
    if (triggerStatus->wait_for(std::chrono::seconds(1)) != std::future_status::ready) {
      throw std::runtime_error("Connection-loss trigger command did not finish");
    }
    require(triggerStatus->get() == 0, "Connection-loss trigger command failed");
  }
}

std::vector<std::pair<std::string, rti::CallbackModel>> callbackModels(Options const& options) {
  if (options.callbackModel == "evoked") {
    return {{"evoked", rti::HLA_EVOKED}};
  }
  if (options.callbackModel == "immediate") {
    return {{"immediate", rti::HLA_IMMEDIATE}};
  }
  return {{"evoked", rti::HLA_EVOKED}, {"immediate", rti::HLA_IMMEDIATE}};
}

std::vector<std::string> allScenarioIds() {
  return {
      "java-tck.overloads-and-exceptions",
      "java-tck.federation-membership",
      "java-tck.support-services",
      "java-tck.declaration-management",
      "java-tck.object-management",
      "java-tck.attribute-interaction",
      "java-tck.ownership",
      "cpp-tck.connection-loss-cleanup",
  };
}

void printHelp() {
  std::cout <<
      "Usage: hla_rti_cpp_tck --fom FILE [options]\n"
      "  --scenario ID                 Repeat; default is every P0 scenario\n"
      "  --callback-model both|evoked|immediate\n"
      "  --provider-id ID              Result metadata only\n"
      "  --rti-address ADDRESS         Standard RtiConfiguration address\n"
      "  --configuration-name NAME     Standard RtiConfiguration name\n"
      "  --additional-settings VALUE   Standard RtiConfiguration settings\n"
      "  --federation-prefix PREFIX    Prefix for unique federation names\n"
      "  --federation-name NAME        Fixed federation name for an adapter fixture\n"
      "  --owner-name NAME             Owner federate name\n"
      "  --member-name NAME            Member federate name\n"
      "  --federate-type NAME          Federate type\n"
      "  --owner-configuration-name N  Owner-side standard configuration name\n"
      "  --member-configuration-name N Member-side standard configuration name\n"
      "  --object-class NAME           Object class handle lookup name\n"
      "  --attribute NAME              Attribute handle lookup name\n"
      "  --interaction-class NAME      Interaction class handle lookup name\n"
      "  --parameter NAME              Parameter handle lookup name\n"
      "  --time-implementation NAME    Optional Create Federation time name\n"
      "  --connection-loss-command CMD Adapter trigger; use {federation}, {owner}, {member}\n"
      "  --connection-loss-server-managed\n"
      "                                Adapter already controls the loss lifecycle\n"
      "  --connection-loss-marker FILE Write callback completion marker for an adapter\n"
      "  --timeout-ms N                Callback wait timeout, default 5000\n"
      "  --results FILE                Write machine-readable JSON results\n"
      "  --junit FILE                  Write JUnit XML results\n"
      "  --help                        Show this help\n";
}

void requireValue(int& index, int argc, char** argv, std::string const& option) {
  if (index + 1 >= argc) {
    throw std::runtime_error("Missing value for " + option);
  }
}

Options parseOptions(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    auto const argument = std::string(argv[index]);
    if (argument == "--help") {
      printHelp();
      std::exit(0);
    }
    if (argument == "--fom") {
      requireValue(index, argc, argv, argument);
      options.fom = argv[++index];
    } else if (argument == "--scenario") {
      requireValue(index, argc, argv, argument);
      options.scenarios.emplace_back(argv[++index]);
    } else if (argument == "--callback-model") {
      requireValue(index, argc, argv, argument);
      options.callbackModel = argv[++index];
    } else if (argument == "--provider-id") {
      requireValue(index, argc, argv, argument);
      options.providerId = argv[++index];
    } else if (argument == "--results") {
      requireValue(index, argc, argv, argument);
      options.results = argv[++index];
    } else if (argument == "--junit") {
      requireValue(index, argc, argv, argument);
      options.junit = argv[++index];
    } else if (argument == "--connection-loss-command") {
      requireValue(index, argc, argv, argument);
      options.connectionLossCommand = argv[++index];
    } else if (argument == "--connection-loss-server-managed") {
      options.connectionLossServerManaged = true;
    } else if (argument == "--connection-loss-marker") {
      requireValue(index, argc, argv, argument);
      options.connectionLossMarker = argv[++index];
    } else if (argument == "--federation-prefix") {
      requireValue(index, argc, argv, argument);
      options.federationPrefix = argv[++index];
    } else if (argument == "--federation-name") {
      requireValue(index, argc, argv, argument);
      options.federationOverride = toWide(argv[++index]);
    } else if (argument == "--owner-name") {
      requireValue(index, argc, argv, argument);
      options.ownerFederateName = toWide(argv[++index]);
    } else if (argument == "--member-name") {
      requireValue(index, argc, argv, argument);
      options.memberFederateName = toWide(argv[++index]);
    } else if (argument == "--federate-type") {
      requireValue(index, argc, argv, argument);
      options.federateType = toWide(argv[++index]);
    } else if (argument == "--configuration-name") {
      requireValue(index, argc, argv, argument);
      options.configurationName = argv[++index];
    } else if (argument == "--owner-configuration-name") {
      requireValue(index, argc, argv, argument);
      options.ownerConfigurationName = argv[++index];
    } else if (argument == "--member-configuration-name") {
      requireValue(index, argc, argv, argument);
      options.memberConfigurationName = argv[++index];
    } else if (argument == "--rti-address") {
      requireValue(index, argc, argv, argument);
      options.rtiAddress = argv[++index];
    } else if (argument == "--additional-settings") {
      requireValue(index, argc, argv, argument);
      options.additionalSettings = argv[++index];
    } else if (argument == "--object-class") {
      requireValue(index, argc, argv, argument);
      options.objectClassName = toWide(argv[++index]);
    } else if (argument == "--attribute") {
      requireValue(index, argc, argv, argument);
      options.attributeName = toWide(argv[++index]);
    } else if (argument == "--interaction-class") {
      requireValue(index, argc, argv, argument);
      options.interactionClassName = toWide(argv[++index]);
    } else if (argument == "--parameter") {
      requireValue(index, argc, argv, argument);
      options.parameterName = toWide(argv[++index]);
    } else if (argument == "--time-implementation") {
      requireValue(index, argc, argv, argument);
      options.logicalTimeImplementationName = toWide(argv[++index]);
    } else if (argument == "--timeout-ms") {
      requireValue(index, argc, argv, argument);
      options.timeoutMilliseconds = std::stoi(argv[++index]);
    } else {
      throw std::runtime_error("Unknown option: " + argument);
    }
  }
  if (options.fom.empty()) {
    throw std::runtime_error("--fom is required");
  }
  if (!std::filesystem::is_regular_file(options.fom)) {
    throw std::runtime_error("FOM file does not exist: " + options.fom.string());
  }
  if (options.callbackModel != "both" && options.callbackModel != "evoked" &&
      options.callbackModel != "immediate") {
    throw std::runtime_error("--callback-model must be both, evoked, or immediate");
  }
  if (options.timeoutMilliseconds <= 0) {
    throw std::runtime_error("--timeout-ms must be positive");
  }
  if (options.scenarios.empty()) {
    options.scenarios = allScenarioIds();
  }
  return options;
}

using ScenarioFunction = std::function<void(Options const&, rti::CallbackModel)>;

ScenarioFunction scenarioFunction(std::string const& id) {
  if (id == "java-tck.overloads-and-exceptions") return scenarioConnection;
  if (id == "java-tck.federation-membership") return scenarioFederationLifecycle;
  if (id == "java-tck.support-services") return scenarioHandleLookups;
  if (id == "java-tck.declaration-management") return scenarioDeclarations;
  if (id == "java-tck.object-management") return scenarioObjectManagement;
  if (id == "java-tck.attribute-interaction") return scenarioAttributeInteraction;
  if (id == "java-tck.ownership") return scenarioOwnership;
  if (id == "cpp-tck.connection-loss-cleanup") return scenarioConnectionLoss;
  throw std::runtime_error("Unknown scenario: " + id);
}

std::string jsonEscape(std::string const& value) {
  std::string escaped;
  escaped.reserve(value.size() + 8U);
  for (auto const character : value) {
    switch (character) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default: escaped += character; break;
    }
  }
  return escaped;
}

std::string xmlEscape(std::string const& value) {
  std::string escaped;
  escaped.reserve(value.size() + 8U);
  for (auto const character : value) {
    switch (character) {
      case '&': escaped += "&amp;"; break;
      case '<': escaped += "&lt;"; break;
      case '>': escaped += "&gt;"; break;
      case '"': escaped += "&quot;"; break;
      case '\'': escaped += "&apos;"; break;
      default: escaped += character; break;
    }
  }
  return escaped;
}

void writeResults(
    std::filesystem::path const& path,
    Options const& options,
    std::vector<ScenarioResult> const& results) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("Could not open results file: " + path.string());
  }
  output << "{\n  \"kind\": \"hla-rti-cpp-tck-evidence\",\n"
         << "  \"standard\": \"IEEE 1516.1-2025\",\n"
         << "  \"provider_id\": \"" << jsonEscape(options.providerId) << "\",\n"
         << "  \"fom\": \"" << jsonEscape(options.fom.string()) << "\",\n"
         << "  \"results\": [\n";
  for (std::size_t index = 0U; index < results.size(); ++index) {
    auto const& result = results[index];
    output << "    {\"id\": \"" << jsonEscape(result.id)
           << "\", \"callback_model\": \"" << jsonEscape(result.callbackModel)
           << "\", \"status\": \"" << jsonEscape(result.status)
           << "\", \"message\": \"" << jsonEscape(result.message)
           << "\", \"duration_ms\": " << result.durationMilliseconds << "}";
    if (index + 1U != results.size()) output << ',';
    output << '\n';
  }
  output << "  ]\n}\n";
}

void writeJUnit(std::filesystem::path const& path, std::vector<ScenarioResult> const& results) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("Could not open JUnit file: " + path.string());
  }
  auto const failures = std::count_if(results.begin(), results.end(), [](auto const& result) {
    return result.status == "failed";
  });
  output << "<testsuite name=\"hla-rti-cpp-tck\" tests=\"" << results.size()
         << "\" failures=\"" << failures << "\">\n";
  for (auto const& result : results) {
    output << "  <testcase name=\"" << xmlEscape(result.id + "/" + result.callbackModel)
           << "\" time=\"" << (result.durationMilliseconds / 1000.0) << "\">\n";
    if (result.status == "failed") {
      output << "    <failure message=\"" << xmlEscape(result.message) << "\"/>\n";
    } else if (result.status == "skipped") {
      output << "    <skipped message=\"" << xmlEscape(result.message) << "\"/>\n";
    }
    output << "  </testcase>\n";
  }
  output << "</testsuite>\n";
}

int run(Options const& options) {
  std::vector<ScenarioResult> results;
  for (auto const& scenario : options.scenarios) {
    auto const function = scenarioFunction(scenario);
    for (auto const& callback : callbackModels(options)) {
      ScenarioResult result{scenario, callback.first, "passed", "", 0};
      auto const started = Clock::now();
      if (scenario == "cpp-tck.connection-loss-cleanup" &&
          options.connectionLossCommand.empty() &&
          !options.connectionLossServerManaged) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied trigger or server-managed loss";
      } else {
        try {
          function(options, callback.second);
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " [" << result.callbackModel << "]";
      if (!result.message.empty()) std::cout << ": " << result.message;
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) writeResults(options.results, options, results);
  if (!options.junit.empty()) writeJUnit(options.junit, results);
  auto const failures = std::count_if(results.begin(), results.end(), [](auto const& result) {
    return result.status == "failed";
  });
  auto const skipped = std::count_if(results.begin(), results.end(), [](auto const& result) {
    return result.status == "skipped";
  });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    auto const options = parseOptions(argc, argv);
    return run(options);
  } catch (std::exception const& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}
