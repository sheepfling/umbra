#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::DELETE_OBJECTS;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::IllegalName;
using rti1516_2025::ObjectInstanceNameInUse;
using rti1516_2025::ObjectInstanceNotKnown;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::VariableLengthData;

constexpr wchar_t const* kFederationName = L"process-execution";
constexpr wchar_t const* kObjectClassName = L"HLAobjectRoot.Customer";
constexpr wchar_t const* kInteractionClassName =
    L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
constexpr wchar_t const* kDirectedObjectClassName =
    L"HLAobjectRoot.Employee.Server";
constexpr wchar_t const* kDirectedInteractionClassName =
    L"HLAinteractionRoot.ServerAction.TakeOrder";
constexpr wchar_t const* kParameterName = L"TimelinessOk";

class ReceiverFederateAmbassador final : public NullFederateAmbassador {
 public:
  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    reservationSucceeded = true;
    reservationName = objectInstanceName;
  }

  void objectInstanceNameReservationFailed(
      std::wstring const& objectInstanceName) override {
    reservationFailed = true;
    reservationName = objectInstanceName;
  }

  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      rti1516_2025::FederateHandle const& producingFederate) override {
    discovered = true;
    discoveredObjectInstance = objectInstance;
    discoveredObjectClass = objectClass;
    discoveredObjectInstanceName = objectInstanceName;
    discoveredProducingFederate = producingFederate;
  }

  void connectionLost(std::wstring const& faultDescription) override {
    connectionLostCalled = true;
    connectionLostDescription = faultDescription;
  }

  void initiateFederateSave(std::wstring const& label) override {
    ++saveInitiateCount;
    saveLabel = label;
  }

  void federationSaved() override { ++saveCompleteCount; }

  void federationNotSaved(rti1516_2025::SaveFailureReason reason) override {
    ++saveNotCompleteCount;
    saveFailureReason = reason;
  }

  void requestFederationRestoreSucceeded(
      std::wstring const& label) override {
    ++restoreSucceededCount;
    restoreLabel = label;
    callbackOrder.push_back("restore-request-succeeded");
  }

  void requestFederationRestoreFailed(std::wstring const& label) override {
    ++restoreFailedCount;
    restoreLabel = label;
    callbackOrder.push_back("restore-request-failed");
  }

  void federationRestoreBegun() override {
    ++restoreBegunCount;
    callbackOrder.push_back("restore-begun");
  }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const& federateName,
      rti1516_2025::FederateHandle const& postRestoreFederateHandle) override {
    ++restoreInitiateCount;
    restoreLabel = label;
    restoreFederateName = federateName;
    restorePostFederateHandle = postRestoreFederateHandle;
    callbackOrder.push_back("restore-initiate");
  }

  void federationRestored() override {
    ++restoreCompleteCount;
    callbackOrder.push_back("restore-complete");
  }

  void federationNotRestored(
      rti1516_2025::RestoreFailureReason reason) override {
    ++restoreNotCompleteCount;
    restoreFailureReason = reason;
    callbackOrder.push_back("restore-failed");
  }

  void federationRestoreStatusResponse(
      rti1516_2025::FederateRestoreStatusVector const& response) override {
    federationRestoreStatusReports.push_back(response);
    callbackOrder.push_back("restore-status");
  }

  void removeObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::FederateHandle const& producingFederate) override {
    ++objectRemovalCount;
    objectRemovalInstance = objectInstance;
    objectRemovalProducer = producingFederate;
    objectRemovalTag.clear();
    if (userSuppliedTag.size() != 0U) {
      auto const* first =
          static_cast<std::uint8_t const*>(userSuppliedTag.data());
      objectRemovalTag.assign(first, first + userSuppliedTag.size());
    }
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*) override {
    received = true;
    interactionClassHandle = interactionClass;
    producingFederateHandle = producingFederate;
    parameterCount = parameterValues.size();
    captureParameterValues(parameterValues);
    tag.clear();
    captureTag(userSuppliedTag);
  }

  void receiveDirectedInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate) override {
    ++directedReceivedCount;
    directedReceived = true;
    directedInteractionClassHandle = interactionClass;
    directedObjectInstance = objectInstance;
    directedProducingFederate = producingFederate;
    directedTransportationType = transportationType;
    directedParameterCount = parameterValues.size();
    directedTag.clear();
    captureTag(userSuppliedTag, directedTag);
  }

  void receiveDirectedInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType sentOrder,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    receiveDirectedInteraction(
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate);
    directedTimestamped = true;
    directedHasOptionalRetraction = optionalRetraction != nullptr;
    directedSentOrderType = sentOrder;
    directedReceivedOrderType = receivedOrder;
    directedTimestampImplementationName = time.implementationName();
    directedTimestampEncoding.clear();
    auto const encoded = time.encode();
    if (encoded.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(encoded.data());
      directedTimestampEncoding.assign(first, first + encoded.size());
    }
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    ++requestRetractionCount;
    requestRetractionHandle = retraction;
  }

  void reflectAttributeValues(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    reflected = true;
    reflectedObjectInstance = objectInstance;
    reflectedTransportationType = transportationType;
    reflectedProducingFederate = producingFederate;
    reflectedAttributeCount = attributeValues.size();
    reflectedHasOptionalSentRegions = optionalSentRegions != nullptr;
    reflectedValue.clear();
    if (!attributeValues.empty()) {
      auto const& value = attributeValues.begin()->second;
      if (value.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(value.data());
        reflectedValue.assign(first, first + value.size());
      }
    }
    reflectedTag.clear();
    captureTag(userSuppliedTag, reflectedTag);
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType sentOrder,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    received = true;
    timestampedReceived = true;
    interactionClassHandle = interactionClass;
    producingFederateHandle = producingFederate;
    parameterCount = parameterValues.size();
    captureParameterValues(parameterValues);
    hasOptionalRetraction = optionalRetraction != nullptr;
    sentOrderType = sentOrder;
    receivedOrderType = receivedOrder;
    timestampImplementationName = time.implementationName();
    timestampEncoding.clear();
    auto const encoded = time.encode();
    if (encoded.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(encoded.data());
      timestampEncoding.assign(first, first + encoded.size());
    }
    captureTag(userSuppliedTag);
  }

 private:
  void captureParameterValues(ParameterHandleValueMap const& parameterValues) {
    parameterHandle = rti1516_2025::ParameterHandle{};
    parameterValue.clear();
    if (parameterValues.size() != 1U) {
      return;
    }
    auto const& entry = *parameterValues.begin();
    parameterHandle = entry.first;
    if (entry.second.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(entry.second.data());
      parameterValue.assign(first, first + entry.second.size());
    }
  }

  static void captureTag(
      VariableLengthData const& userSuppliedTag,
      std::vector<std::uint8_t>& destination) {
    destination.clear();
    if (userSuppliedTag.size() != 0U) {
      auto const* first =
          static_cast<std::uint8_t const*>(userSuppliedTag.data());
      destination.assign(first, first + userSuppliedTag.size());
    }
  }

  void captureTag(VariableLengthData const& userSuppliedTag) {
    captureTag(userSuppliedTag, tag);
  }

 public:
  bool received = false;
  bool timestampedReceived = false;
  rti1516_2025::InteractionClassHandle interactionClassHandle;
  rti1516_2025::FederateHandle producingFederateHandle;
  std::size_t parameterCount = 0U;
  rti1516_2025::ParameterHandle parameterHandle;
  std::vector<std::uint8_t> parameterValue;
  std::vector<std::uint8_t> tag;
  bool hasOptionalRetraction = false;
  rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
  std::wstring timestampImplementationName;
  std::vector<std::uint8_t> timestampEncoding;
  bool directedReceived = false;
  std::size_t directedReceivedCount = 0U;
  rti1516_2025::InteractionClassHandle directedInteractionClassHandle;
  rti1516_2025::ObjectInstanceHandle directedObjectInstance;
  rti1516_2025::FederateHandle directedProducingFederate;
  rti1516_2025::TransportationTypeHandle directedTransportationType;
  std::size_t directedParameterCount = 0U;
  std::vector<std::uint8_t> directedTag;
  bool directedTimestamped = false;
  bool directedHasOptionalRetraction = false;
  rti1516_2025::OrderType directedSentOrderType = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType directedReceivedOrderType = rti1516_2025::RECEIVE;
  std::wstring directedTimestampImplementationName;
  std::vector<std::uint8_t> directedTimestampEncoding;
  std::size_t requestRetractionCount = 0U;
  rti1516_2025::MessageRetractionHandle requestRetractionHandle;
  bool connectionLostCalled = false;
  std::wstring connectionLostDescription;
  std::size_t saveInitiateCount = 0U;
  std::wstring saveLabel;
  std::size_t saveCompleteCount = 0U;
  std::size_t saveNotCompleteCount = 0U;
  rti1516_2025::SaveFailureReason saveFailureReason =
      rti1516_2025::SAVE_ABORTED;
  std::size_t restoreSucceededCount = 0U;
  std::size_t restoreFailedCount = 0U;
  std::size_t restoreBegunCount = 0U;
  std::size_t restoreInitiateCount = 0U;
  std::size_t restoreCompleteCount = 0U;
  std::size_t restoreNotCompleteCount = 0U;
  std::wstring restoreLabel;
  std::wstring restoreFederateName;
  rti1516_2025::FederateHandle restorePostFederateHandle;
  rti1516_2025::RestoreFailureReason restoreFailureReason =
      rti1516_2025::RESTORE_ABORTED;
  std::vector<rti1516_2025::FederateRestoreStatusVector>
      federationRestoreStatusReports;
  std::vector<std::string> callbackOrder;
  std::size_t objectRemovalCount = 0U;
  rti1516_2025::ObjectInstanceHandle objectRemovalInstance;
  rti1516_2025::FederateHandle objectRemovalProducer;
  std::vector<std::uint8_t> objectRemovalTag;
  bool reflected = false;
  bool discovered = false;
  rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
  rti1516_2025::ObjectClassHandle discoveredObjectClass;
  std::wstring discoveredObjectInstanceName;
  rti1516_2025::FederateHandle discoveredProducingFederate;
  rti1516_2025::ObjectInstanceHandle reflectedObjectInstance;
  rti1516_2025::TransportationTypeHandle reflectedTransportationType;
  rti1516_2025::FederateHandle reflectedProducingFederate;
  std::size_t reflectedAttributeCount = 0U;
  bool reflectedHasOptionalSentRegions = false;
  std::vector<std::uint8_t> reflectedValue;
  std::vector<std::uint8_t> reflectedTag;
  bool reservationSucceeded = false;
  bool reservationFailed = false;
  std::wstring reservationName;
};

std::filesystem::path temporaryDirectory() {
  auto const stamp = std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count();
  return std::filesystem::temp_directory_path() /
      ("umbra-package-process-" + std::to_string(stamp));
}

std::string quoted(std::filesystem::path const& path) {
  return "\"" + path.string() + "\"";
}

template <typename Reader>
auto waitForFile(std::filesystem::path const& path, Reader&& reader) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(30);
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(path)) {
      try {
        return reader(path);
      } catch (std::exception const&) {
        // The marker may still be flushing. Retry within the bounded window.
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error("The package process smoke timed out waiting for a marker.");
}

std::uint16_t readPort(std::filesystem::path const& path) {
  return waitForFile(path, [](std::filesystem::path const& marker) {
    std::ifstream input(marker, std::ios::binary);
    unsigned int port = 0U;
    input >> port;
    if (!input || port == 0U || port > 65535U) {
      throw std::runtime_error("The package process smoke port marker is invalid.");
    }
    return static_cast<std::uint16_t>(port);
  });
}

std::uint64_t readHandle(std::filesystem::path const& path) {
  return waitForFile(path, [](std::filesystem::path const& marker) {
    std::ifstream input(marker, std::ios::binary);
    std::uint64_t value = 0U;
    input >> value;
    if (!input || value == 0U) {
      throw std::runtime_error("The package process smoke handle marker is invalid.");
    }
    return value;
  });
}

void writeText(std::filesystem::path const& path, std::string const& text) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error(
        "The package process smoke could not write its status marker.");
  }
  output << text;
  if (!output) {
    throw std::runtime_error(
        "The package process smoke could not flush its status marker.");
  }
}

int run(
    bool timestamped,
    bool connectionLoss,
    bool automaticDeleteObjects,
    bool parameterized,
    bool objectRegistration,
    bool namedRegistration,
    bool attributeUpdate,
    bool directedRetraction,
    bool federationSaveRestore,
    bool federationSaveRestoreFailure,
    bool federationSaveRestoreAbort,
    bool federationSaveRestoreStatus) {
#ifndef UMBRA_PROCESS_SERVICE_PROBE_PATH
  throw std::runtime_error("The package process smoke probe path is not configured.");
#else
  bool const saveRestore =
      federationSaveRestore || federationSaveRestoreFailure ||
      federationSaveRestoreAbort || federationSaveRestoreStatus;
  auto const directory = temporaryDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  if (!std::filesystem::create_directories(directory)) {
    throw std::runtime_error("The package process smoke could not create its marker directory.");
  }

  std::filesystem::path const probe = UMBRA_PROCESS_SERVICE_PROBE_PATH;
  if (!std::filesystem::is_regular_file(probe)) {
    throw std::runtime_error("The package process smoke probe is not a regular file.");
  }

  if (automaticDeleteObjects) {
    writeText(directory / "automatic-delete-objects.mode", "enabled\n");
  }

  auto const launch = [&](std::string const& role) {
    auto command = quoted(probe) + " " + role + " " + quoted(directory);
#if defined(_WIN32)
    // std::system delegates to cmd.exe; the outer quotes preserve the full
    // command when either generated path contains spaces.
    command = std::string{"\""} + command + "\"";
#endif
    return std::async(
        std::launch::async,
        [command] { return std::system(command.c_str()); });
  };

  std::string const serverRole =
      saveRestore
          ? (federationSaveRestoreStatus
                 ? "public-server-save-restore-status"
                 : (federationSaveRestoreFailure
                        ? "public-server-save-restore-failure"
                        : (federationSaveRestoreAbort
                               ? "public-server-save-restore-abort"
                               : "public-server-save-restore")))
          : (directedRetraction
                 ? "public-server-directed-retraction"
                 : (namedRegistration
                        ? "public-server-named-registration"
                        : (objectRegistration
                               ? "public-server-object-registration"
                               : (connectionLoss
                                      ? "public-server-loss"
                                      : (attributeUpdate
                                             ? "public-server-attribute-update"
                                             : (parameterized
                                                    ? "public-server-parameterized"
                                                    : "public-server"))))));
  auto server = launch(serverRole);
  std::unique_ptr<RTIambassador> sender;
  std::unique_ptr<RTIambassador> receiver;
  ReceiverFederateAmbassador senderAmbassador;
  ReceiverFederateAmbassador receiverAmbassador;
  bool senderJoined = false;
  bool receiverJoined = false;
  std::exception_ptr clientError;
  try {
    auto const port = readPort(directory / "port.txt");
    auto const address = L"tcp://127.0.0.1:" + std::to_wstring(port);
    auto senderConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(L"package-process-sender")
                                   .withRtiAddress(address);
    auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"package-process-receiver")
                                     .withRtiAddress(address);

    RTIambassadorFactory factory;
    sender = factory.createRTIambassador();
    if (!saveRestore) {
      receiver = factory.createRTIambassador();
    }
    if (!sender || (!saveRestore && !receiver)) {
      throw std::runtime_error(
          "The package process smoke could not create the required ambassadors.");
    }

    auto const senderConnection =
        sender->connect(senderAmbassador, HLA_EVOKED, senderConfiguration);
    if (!senderConnection.addressUsed) {
      throw std::runtime_error("The installed sender did not select the process endpoint.");
    }
    if (!saveRestore) {
      auto receiverConnection =
          receiver->connect(receiverAmbassador, HLA_EVOKED, receiverConfiguration);
      if (!receiverConnection.addressUsed) {
        throw std::runtime_error(
            "The installed receiver did not select the process endpoint.");
      }
    }

    sender->createFederationExecution(kFederationName, L"server-owned-fom.xml");
    static_cast<void>(sender->joinFederationExecution(
        L"package-process-sender",
        L"package-process-type",
        kFederationName));
    senderJoined = true;
    if (!saveRestore) {
      auto const receiverHandle = receiver->joinFederationExecution(
          L"package-process-receiver",
          L"package-process-type",
          kFederationName);
      if (!receiverHandle.isValid()) {
        throw std::runtime_error(
            "The installed receiver Join returned an invalid handle.");
      }
      receiverJoined = true;
    }

    if (saveRestore) {
      constexpr wchar_t const* saveLabel = L"package-process-save-restore";
      sender->requestFederationSave(saveLabel);
      if (senderAmbassador.saveInitiateCount != 0U) {
        throw std::runtime_error(
            "The installed save/restore process smoke delivered save initiation too early.");
      }
      static_cast<void>(sender->evokeCallback(0.0));
      if (senderAmbassador.saveInitiateCount != 1U ||
          senderAmbassador.saveLabel != saveLabel ||
          senderAmbassador.saveNotCompleteCount != 0U) {
        throw std::runtime_error(
            "The installed save/restore process smoke did not preserve save initiation.");
      }
      sender->federateSaveBegun();
      sender->federateSaveComplete();
      static_cast<void>(sender->evokeCallback(0.0));
      if (senderAmbassador.saveCompleteCount != 1U ||
          senderAmbassador.saveNotCompleteCount != 0U) {
        throw std::runtime_error(
            "The installed save/restore process smoke did not preserve save completion.");
      }

      sender->requestFederationRestore(saveLabel);
      if (senderAmbassador.restoreSucceededCount != 0U) {
        throw std::runtime_error(
            "The installed save/restore process smoke delivered restore success too early.");
      }
      static_cast<void>(sender->evokeCallback(0.0));
      static_cast<void>(sender->evokeCallback(0.0));
      static_cast<void>(sender->evokeCallback(0.0));
      if (senderAmbassador.restoreSucceededCount != 1U ||
          senderAmbassador.restoreFailedCount != 0U ||
          senderAmbassador.restoreBegunCount != 1U ||
          senderAmbassador.restoreInitiateCount != 1U ||
          senderAmbassador.restoreLabel != saveLabel ||
          senderAmbassador.restoreFederateName.empty() ||
          !senderAmbassador.restorePostFederateHandle.isValid() ||
          senderAmbassador.callbackOrder !=
              std::vector<std::string>{
                  "restore-request-succeeded",
                  "restore-begun",
                  "restore-initiate"}) {
        throw std::runtime_error(
            "The installed save/restore process smoke did not preserve restore initiation.");
      }
      if (federationSaveRestoreStatus) {
        sender->queryFederationRestoreStatus();
        if (!senderAmbassador.federationRestoreStatusReports.empty()) {
          throw std::runtime_error(
              "The installed restore-status process smoke delivered status too early.");
        }
        static_cast<void>(sender->evokeCallback(0.0));
        if (senderAmbassador.federationRestoreStatusReports.size() != 1U ||
            senderAmbassador.federationRestoreStatusReports.front().size() != 1U ||
            !senderAmbassador.federationRestoreStatusReports.front().front()
                 .preRestoreHandle
                 .isValid() ||
            !senderAmbassador.federationRestoreStatusReports.front().front()
                 .postRestoreHandle
                 .isValid() ||
            senderAmbassador.federationRestoreStatusReports.front().front().status !=
                rti1516_2025::FEDERATE_RESTORING ||
            senderAmbassador.callbackOrder !=
                std::vector<std::string>{
                    "restore-request-succeeded",
                    "restore-begun",
                    "restore-initiate",
                    "restore-status"}) {
          throw std::runtime_error(
              "The installed restore-status process smoke did not preserve the in-progress status.");
        }
        sender->federateRestoreComplete();
        if (senderAmbassador.restoreCompleteCount != 0U) {
          throw std::runtime_error(
              "The installed restore-status process smoke delivered restore completion too early.");
        }
        static_cast<void>(sender->evokeCallback(0.0));
        if (senderAmbassador.restoreCompleteCount != 1U ||
            senderAmbassador.callbackOrder !=
                std::vector<std::string>{
                    "restore-request-succeeded",
                    "restore-begun",
                    "restore-initiate",
                    "restore-status",
                    "restore-complete"}) {
          throw std::runtime_error(
              "The installed restore-status process smoke did not preserve restore completion.");
        }
      } else if (federationSaveRestoreFailure || federationSaveRestoreAbort) {
        bool const restoreAbort = federationSaveRestoreAbort;
        if (restoreAbort) {
          sender->abortFederationRestore();
        } else {
          sender->federateRestoreNotComplete();
        }
        if (senderAmbassador.restoreNotCompleteCount != 0U) {
          throw std::runtime_error(
              "The installed save/restore-control process smoke delivered restore failure too early.");
        }
        static_cast<void>(sender->evokeCallback(0.0));
        if (senderAmbassador.restoreCompleteCount != 0U ||
            senderAmbassador.restoreNotCompleteCount != 1U ||
            senderAmbassador.restoreFailureReason !=
                (restoreAbort
                     ? rti1516_2025::RESTORE_ABORTED
                     : rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE) ||
            senderAmbassador.callbackOrder !=
                std::vector<std::string>{
                    "restore-request-succeeded",
                    "restore-begun",
                    "restore-initiate",
                    "restore-failed"}) {
          throw std::runtime_error(
              "The installed save/restore-control process smoke did not preserve restore failure.");
        }
      } else {
        sender->federateRestoreComplete();
        if (senderAmbassador.restoreCompleteCount != 0U) {
          throw std::runtime_error(
              "The installed save/restore process smoke delivered restore completion too early.");
        }
        static_cast<void>(sender->evokeCallback(0.0));
        if (senderAmbassador.restoreCompleteCount != 1U ||
            senderAmbassador.restoreNotCompleteCount != 0U ||
            senderAmbassador.callbackOrder !=
                std::vector<std::string>{
                    "restore-request-succeeded",
                    "restore-begun",
                    "restore-initiate",
                    "restore-complete"}) {
          throw std::runtime_error(
              "The installed save/restore process smoke did not preserve restore completion.");
        }
      }
      writeText(directory / "send.ok", "ok\n");
      sender->resignFederationExecution(NO_ACTION);
      senderJoined = false;
      sender->disconnect();
    } else if (directedRetraction) {
      // Keep this consumer on the installed public surface: the fixture owns
      // only the private process service, while every lookup, declaration,
      // registration, timestamped directed send, and Retract crosses the
      // official RTIambassador API.
      auto const objectClass =
          sender->getObjectClassHandle(kDirectedObjectClassName);
      auto const interactionClass =
          sender->getInteractionClassHandle(kDirectedInteractionClassName);
      auto const attribute = sender->getAttributeHandle(
          objectClass, L"HLAprivilegeToDeleteObject");
      if (!objectClass.isValid() || !interactionClass.isValid() ||
          !attribute.isValid()) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke returned invalid sender handles.");
      }
      rti1516_2025::AttributeHandleSet publishedAttributes;
      publishedAttributes.insert(attribute);
      sender->publishObjectClassAttributes(objectClass, publishedAttributes);

      rti1516_2025::InteractionClassHandleSet directedInteractions;
      directedInteractions.insert(interactionClass);
      sender->publishObjectClassDirectedInteractions(
          objectClass, directedInteractions);
      auto const objectInstance = sender->registerObjectInstance(objectClass);
      if (!objectInstance.isValid()) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke returned an invalid target object.");
      }

      auto const receiverObjectClass =
          receiver->getObjectClassHandle(kDirectedObjectClassName);
      auto const receiverAttribute = receiver->getAttributeHandle(
          receiverObjectClass, L"HLAprivilegeToDeleteObject");
      auto const receiverInteractionClass =
          receiver->getInteractionClassHandle(kDirectedInteractionClassName);
      if (!receiverObjectClass.isValid() || !receiverAttribute.isValid() ||
          !receiverInteractionClass.isValid() ||
          receiverInteractionClass != interactionClass) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke returned invalid receiver handles.");
      }
      rti1516_2025::AttributeHandleSet subscribedAttributes;
      subscribedAttributes.insert(receiverAttribute);
      receiver->subscribeObjectClassAttributes(
          receiverObjectClass, subscribedAttributes, true, L"");
      rti1516_2025::InteractionClassHandleSet subscribedInteractions;
      subscribedInteractions.insert(receiverInteractionClass);
      receiver->subscribeObjectClassDirectedInteractions(
          receiverObjectClass, subscribedInteractions, true);

      // The attribute subscription makes the already-registered target
      // discoverable.  Consume that callback before entering the directed
      // interaction slice so the next Evoke is reserved for the directed
      // receive-order event itself.
      static_cast<void>(receiver->evokeCallback(0.0));
      if (!receiverAmbassador.discovered ||
          receiverAmbassador.discoveredObjectClass != receiverObjectClass ||
          receiverAmbassador.discoveredObjectInstance != objectInstance ||
          !receiverAmbassador.discoveredProducingFederate.isValid()) {
        throw std::runtime_error(
            "The installed receiver did not discover the directed target before delivery.");
      }

      // Timestamped Retract is only legal while the producer is time
      // regulating. Establish that role after discovery has crossed its
      // HLA_EVOKED callback boundary.
      sender->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1));
      static_cast<void>(sender->evokeCallback(0.0));

      auto timeFactory =
          rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
              L"HLAinteger64Time");
      auto* integerFactory =
          dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(timeFactory.get());
      if (integerFactory == nullptr) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke could not create HLAinteger64Time.");
      }
      auto timestamp = integerFactory->makeLogicalTime(5);
      if (!timestamp) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke could not create its logical time.");
      }
      auto const encodedTimestamp = timestamp->encode();
      std::vector<std::uint8_t> expectedTimestampEncoding;
      if (encodedTimestamp.size() != 0U) {
        auto const* first =
            static_cast<std::uint8_t const*>(encodedTimestamp.data());
        expectedTimestampEncoding.assign(
            first, first + encodedTimestamp.size());
      }
      std::array<std::uint8_t, 4U> const encodedTag{
          0x44U, 0x52U, 0x54U, 0x31U};
      VariableLengthData tag(encodedTag.data(), encodedTag.size());
      ParameterHandleValueMap parameters;
      auto const firstRetraction = sender->sendDirectedInteraction(
          interactionClass, objectInstance, parameters, tag, *timestamp);
      if (!firstRetraction.isValid()) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke did not return a retraction handle.");
      }
      // First prove the positive path: the receiver crosses one Evoke boundary
      // before Retract and observes the official directed callback.
      static_cast<void>(receiver->evokeCallback(0.0));
      if (receiverAmbassador.directedReceivedCount != 1U ||
          !receiverAmbassador.directedTimestamped ||
          receiverAmbassador.directedInteractionClassHandle !=
              receiverInteractionClass ||
          receiverAmbassador.directedObjectInstance != objectInstance ||
          receiverAmbassador.directedParameterCount != 0U ||
          !receiverAmbassador.directedTransportationType.isValid() ||
          !receiverAmbassador.directedProducingFederate.isValid() ||
          receiverAmbassador.directedTag !=
              std::vector<std::uint8_t>{0x44U, 0x52U, 0x54U, 0x31U} ||
          receiverAmbassador.directedTimestampImplementationName !=
              L"HLAinteger64Time" ||
          receiverAmbassador.directedTimestampEncoding !=
              expectedTimestampEncoding ||
          receiverAmbassador.directedSentOrderType != rti1516_2025::RECEIVE ||
          receiverAmbassador.directedReceivedOrderType != rti1516_2025::RECEIVE ||
          !receiverAmbassador.directedHasOptionalRetraction) {
        throw std::runtime_error(
            "The installed receiver did not preserve the positive directed callback envelope.");
      }
      if (receiverAmbassador.requestRetractionCount != 0U) {
        throw std::runtime_error(
            "The installed receiver observed Request Retraction before Retract.");
      }

      sender->retract(firstRetraction);
      static_cast<void>(receiver->evokeCallback(0.0));
      if (receiverAmbassador.requestRetractionCount != 1U ||
          receiverAmbassador.requestRetractionHandle != firstRetraction) {
        throw std::runtime_error(
            "The installed receiver did not observe the post-delivery Request Retraction.");
      }

      // Then prove the negative path with a fresh message: Retract before the
      // receiver crosses its boundary must suppress the directed callback and
      // must not create a second Request Retraction callback.
      auto secondTimestamp = integerFactory->makeLogicalTime(6);
      if (!secondTimestamp) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke could not create its second logical time.");
      }
      auto const secondRetraction = sender->sendDirectedInteraction(
          interactionClass, objectInstance, parameters, tag, *secondTimestamp);
      if (!secondRetraction.isValid()) {
        throw std::runtime_error(
            "The installed directed-retraction process smoke did not return its second retraction handle.");
      }
      sender->retract(secondRetraction);
      static_cast<void>(receiver->evokeCallback(0.0));
      if (receiverAmbassador.directedReceivedCount != 1U ||
          receiverAmbassador.requestRetractionCount != 1U) {
        throw std::runtime_error(
            "The installed receiver delivered a directed interaction after a pre-delivery Retract.");
      }

      writeText(directory / "send.ok", "ok\n");
      sender->resignFederationExecution(DELETE_OBJECTS);
      senderJoined = false;
      receiver->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      receiver->disconnect();
      sender->disconnect();
    }

    if (namedRegistration) {
      auto const objectClass = sender->getObjectClassHandle(kObjectClassName);
      auto const attribute = sender->getAttributeHandle(
          objectClass, L"HLAprivilegeToDeleteObject");
      if (!objectClass.isValid() || !attribute.isValid()) {
        throw std::runtime_error(
            "The installed named-registration process smoke returned invalid handles.");
      }
      rti1516_2025::AttributeHandleSet attributes;
      attributes.insert(attribute);
      sender->publishObjectClassAttributes(objectClass, attributes);

      constexpr wchar_t const* requestedName =
          L"package-process-named-object";
      sender->reserveObjectInstanceName(requestedName);
      static_cast<void>(sender->evokeCallback(0.0));
      if (!senderAmbassador.reservationSucceeded ||
          senderAmbassador.reservationName != requestedName ||
          senderAmbassador.reservationFailed) {
        throw std::runtime_error(
            "The installed named-registration process smoke did not deliver the reservation callback.");
      }
      auto const objectInstance =
          sender->registerObjectInstance(objectClass, requestedName);
      if (!objectInstance.isValid()) {
        throw std::runtime_error(
            "The installed named-registration process smoke returned an invalid object-instance handle.");
      }
      try {
        static_cast<void>(sender->registerObjectInstance(objectClass, requestedName));
        throw std::runtime_error(
            "The installed named-registration process smoke accepted a duplicate name.");
      } catch (ObjectInstanceNameInUse const&) {
      }
      try {
        sender->reserveObjectInstanceName(L"HLA.package-process-illegal");
        throw std::runtime_error(
            "The installed named-registration process smoke accepted an illegal name.");
      } catch (IllegalName const&) {
      }

      writeText(directory / "send.ok", "ok\n");
      sender->resignFederationExecution(DELETE_OBJECTS);
      senderJoined = false;
      receiver->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      receiver->disconnect();
      sender->disconnect();
    } else if (objectRegistration) {
      auto const objectClass = sender->getObjectClassHandle(kObjectClassName);
      if (!objectClass.isValid()) {
        throw std::runtime_error(
            "The installed object-registration process smoke returned an invalid object-class handle.");
      }
      auto const attribute = sender->getAttributeHandle(
          objectClass, L"HLAprivilegeToDeleteObject");
      if (!attribute.isValid()) {
        throw std::runtime_error(
            "The installed object-registration process smoke returned an invalid attribute handle.");
      }
      rti1516_2025::AttributeHandleSet attributes;
      attributes.insert(attribute);
      sender->publishObjectClassAttributes(objectClass, attributes);
      auto const objectInstance = sender->registerObjectInstance(objectClass);
      if (!objectInstance.isValid()) {
        throw std::runtime_error(
            "The installed object-registration process smoke returned an invalid object-instance handle.");
      }
      // The sender owns the newly registered object's published attributes.
      // With a second joined federate present, NO_ACTION would correctly be
      // rejected by the registry; exercise the successful delete-on-resign
      // path explicitly for this package lane.
      sender->resignFederationExecution(rti1516_2025::DELETE_OBJECTS);
      senderJoined = false;
      receiver->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      receiver->disconnect();
      sender->disconnect();
    }

    if (attributeUpdate) {
      auto const receiverObjectClass =
          receiver->getObjectClassHandle(kObjectClassName);
      auto const receiverAttribute = receiver->getAttributeHandle(
          receiverObjectClass, L"HLAprivilegeToDeleteObject");
      if (!receiverObjectClass.isValid() || !receiverAttribute.isValid()) {
        throw std::runtime_error(
            "The installed attribute-update process smoke returned invalid handles.");
      }
      rti1516_2025::AttributeHandleSet receiverAttributes;
      receiverAttributes.insert(receiverAttribute);
      receiver->subscribeObjectClassAttributes(
          receiverObjectClass,
          receiverAttributes,
          true,
          L"");
      rti1516_2025::AttributeHandleSet attributes;
      attributes.insert(receiverAttribute);
      auto const objectClass = sender->getObjectClassHandle(kObjectClassName);
      auto const attribute = sender->getAttributeHandle(
          objectClass, L"HLAprivilegeToDeleteObject");
      if (!objectClass.isValid() || !attribute.isValid()) {
        throw std::runtime_error(
            "The installed attribute-update process smoke returned invalid sender handles.");
      }
      attributes.clear();
      attributes.insert(attribute);
      sender->publishObjectClassAttributes(objectClass, attributes);
      auto const objectInstance = sender->registerObjectInstance(objectClass);
      if (!objectInstance.isValid()) {
        throw std::runtime_error(
            "The installed attribute-update process smoke returned an invalid object-instance handle.");
      }
      std::array<std::uint8_t, 3U> const encodedValue{0x41U, 0x56U, 0x55U};
      std::array<std::uint8_t, 3U> const encodedTag{0x55U, 0x50U, 0x44U};
      AttributeHandleValueMap attributeValues;
      attributeValues.emplace(
          attribute,
          VariableLengthData(encodedValue.data(), encodedValue.size()));
      VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
      sender->updateAttributeValues(objectInstance, attributeValues, userSuppliedTag);

      auto const deadline = std::chrono::steady_clock::now() +
          std::chrono::seconds(30);
      while ((!receiverAmbassador.reflected || !receiverAmbassador.discovered) &&
             std::chrono::steady_clock::now() < deadline) {
        // The fixture answers one receive poll for this bounded event.  Use
        // the single-callback API; once one event is queued, the ambassador
        // drains it before polling again.
        static_cast<void>(receiver->evokeCallback(0.0));
        if (!receiverAmbassador.reflected || !receiverAmbassador.discovered) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      if (!receiverAmbassador.reflected ||
          !receiverAmbassador.discovered ||
          receiverAmbassador.discoveredObjectInstance != objectInstance ||
          receiverAmbassador.discoveredObjectClass != receiverObjectClass ||
          receiverAmbassador.discoveredObjectInstanceName.empty() ||
          !receiverAmbassador.discoveredProducingFederate.isValid() ||
          receiverAmbassador.reflectedObjectInstance != objectInstance ||
          receiverAmbassador.reflectedAttributeCount != 1U ||
          receiverAmbassador.reflectedValue !=
              std::vector<std::uint8_t>(encodedValue.begin(), encodedValue.end()) ||
          receiverAmbassador.reflectedTag !=
              std::vector<std::uint8_t>(encodedTag.begin(), encodedTag.end()) ||
          !receiverAmbassador.reflectedTransportationType.isValid() ||
          !receiverAmbassador.reflectedProducingFederate.isValid() ||
          receiverAmbassador.reflectedHasOptionalSentRegions) {
        throw std::runtime_error(
            "The installed receiver did not preserve the process attribute reflection callback.");
      }

      sender->resignFederationExecution(DELETE_OBJECTS);
      senderJoined = false;
      receiver->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      receiver->disconnect();
      sender->disconnect();
    } else if (!saveRestore && !objectRegistration &&
               !namedRegistration && !directedRetraction) {
    if (automaticDeleteObjects) {
      auto const lostObjectClass =
          receiver->getObjectClassHandle(kObjectClassName);
      auto const survivingObjectClass =
          sender->getObjectClassHandle(kObjectClassName);
      auto const lostAttribute = receiver->getAttributeHandle(
          lostObjectClass, L"HLAprivilegeToDeleteObject");
      auto const survivingAttribute = sender->getAttributeHandle(
          survivingObjectClass, L"HLAprivilegeToDeleteObject");
      if (!lostObjectClass.isValid() || !survivingObjectClass.isValid() ||
          !lostAttribute.isValid() || !survivingAttribute.isValid()) {
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke returned invalid handles.");
      }
      rti1516_2025::AttributeHandleSet lostAttributes;
      lostAttributes.insert(lostAttribute);
      receiver->publishObjectClassAttributes(lostObjectClass, lostAttributes);
      rti1516_2025::AttributeHandleSet survivingAttributes;
      survivingAttributes.insert(survivingAttribute);
      sender->subscribeObjectClassAttributes(
          survivingObjectClass, survivingAttributes, true, L"");

      auto const deletedObject = receiver->registerObjectInstance(lostObjectClass);
      if (!deletedObject.isValid()) {
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke returned an invalid object.");
      }
      auto const deletedObjectName = receiver->getObjectInstanceName(deletedObject);
      static_cast<void>(sender->evokeCallback(0.0));
      if (!senderAmbassador.discovered ||
          senderAmbassador.discoveredObjectInstance != deletedObject ||
          senderAmbassador.discoveredObjectClass != survivingObjectClass ||
          senderAmbassador.discoveredObjectInstanceName != deletedObjectName) {
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke did not deliver discovery.");
      }

      receiver->setAutomaticResignDirective(DELETE_OBJECTS);
      if (receiver->getAutomaticResignDirective() != DELETE_OBJECTS) {
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke did not preserve DELETE_OBJECTS.");
      }
      if (sender->getObjectInstanceHandle(deletedObjectName) != deletedObject) {
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke did not preserve object-name lookup.");
      }

      writeText(directory / "connection-loss-ready.ok", "ready\n");
      try {
        static_cast<void>(receiver->evokeMultipleCallbacks(0.0, 0.05));
      } catch (rti1516_2025::Exception const&) {
        // The transport failure is surfaced on the operation that detects it;
        // the official Connection Lost callback is delivered by the next drain.
      }
      auto const lossDeadline = std::chrono::steady_clock::now() +
          std::chrono::seconds(30);
      while (!receiverAmbassador.connectionLostCalled &&
             std::chrono::steady_clock::now() < lossDeadline) {
        try {
          static_cast<void>(receiver->evokeMultipleCallbacks(0.0, 0.05));
        } catch (rti1516_2025::Exception const&) {
        }
        if (!receiverAmbassador.connectionLostCalled) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      if (!receiverAmbassador.connectionLostCalled ||
          receiverAmbassador.connectionLostDescription.empty()) {
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke did not receive Connection Lost.");
      }
      receiverJoined = false;
      writeText(directory / "receiver-loss.ok", "callback-ok\n");

      auto const removalDeadline = std::chrono::steady_clock::now() +
          std::chrono::seconds(30);
      while (senderAmbassador.objectRemovalCount == 0U &&
             std::chrono::steady_clock::now() < removalDeadline) {
        static_cast<void>(sender->evokeMultipleCallbacks(0.0, 0.05));
        if (senderAmbassador.objectRemovalCount == 0U) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      if (senderAmbassador.objectRemovalCount != 1U ||
          senderAmbassador.objectRemovalInstance != deletedObject ||
          !senderAmbassador.objectRemovalProducer.isValid() ||
          !senderAmbassador.objectRemovalTag.empty()) {
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke did not preserve the removal callback.");
      }
      try {
        static_cast<void>(sender->getObjectInstanceHandle(deletedObjectName));
        throw std::runtime_error(
            "The installed automatic-delete-objects process smoke retained the deleted object name.");
      } catch (ObjectInstanceNotKnown const&) {
      }
      writeText(directory / "send.ok", "ok\n");
      sender->resignFederationExecution(NO_ACTION);
      senderJoined = false;
      sender->disconnect();
    } else if (connectionLoss) {
      // The server has closed this federate's socket and removed its remote
      // membership.  The first Evoke observes EOF and schedules Connection
      // Lost; the next Evoke drains that official callback from the shared
      // dispatcher.
      // Tell the fixture that both public clients have completed their
      // pre-loss setup.  The fixture waits for this handshake before it
      // applies Connection Lost; without it, the client and fixture would
      // wait on one another and the package smoke could only terminate via
      // its timeout path.
      writeText(directory / "connection-loss-ready.ok", "ready\n");
      try {
        static_cast<void>(receiver->evokeMultipleCallbacks(0.0, 0.05));
      } catch (rti1516_2025::Exception const&) {
        // The transport failure is surfaced on the operation that detects it;
        // the normative callback is delivered on the following Evoke.
      }
      auto const lossDeadline = std::chrono::steady_clock::now() +
          std::chrono::seconds(30);
      while (!receiverAmbassador.connectionLostCalled &&
             std::chrono::steady_clock::now() < lossDeadline) {
        try {
          static_cast<void>(receiver->evokeMultipleCallbacks(0.0, 0.05));
        } catch (rti1516_2025::Exception const&) {
          // A repeated drain remains bounded if the first operation exposed
          // the socket failure before the callback queue became visible.
        }
        if (!receiverAmbassador.connectionLostCalled) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      if (!receiverAmbassador.connectionLostCalled ||
          receiverAmbassador.connectionLostDescription.empty()) {
        throw std::runtime_error(
            "The installed receiver did not receive the process Connection Lost callback.");
      }
      receiverJoined = false;
      writeText(directory / "receiver-loss.ok", "callback-ok\n");
    }

    if (!automaticDeleteObjects) {
    auto const interactionClass =
        sender->getInteractionClassHandle(kInteractionClassName);
    if (!interactionClass.isValid()) {
      throw std::runtime_error("The installed sender lookup returned an invalid interaction handle.");
    }

    std::array<std::uint8_t, 4U> const encodedTag{0x50U, 0x4BU, 0x47U, 0x31U};
    VariableLengthData tag(encodedTag.data(), encodedTag.size());
    ParameterHandleValueMap parameterValues;
    rti1516_2025::ParameterHandle parameterHandle;
    std::vector<std::uint8_t> expectedParameterValue;
    if (parameterized) {
      auto const objectClass = sender->getObjectClassHandle(kObjectClassName);
      if (!objectClass.isValid() ||
          objectClass.toString() !=
              L"ObjectClassHandle(" +
                  std::to_wstring(readHandle(directory / "object-class.txt")) +
                  L")") {
        throw std::runtime_error(
            "The installed parameterized process smoke did not preserve the object-class handle.");
      }
      parameterHandle = sender->getParameterHandle(interactionClass, kParameterName);
      if (!parameterHandle.isValid()) {
        throw std::runtime_error(
            "The installed parameterized process smoke returned an invalid parameter handle.");
      }
      std::array<std::uint8_t, 2U> const encodedParameter{0x01U, 0x00U};
      expectedParameterValue.assign(
          encodedParameter.begin(), encodedParameter.end());
      parameterValues.emplace(
          parameterHandle,
          VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    }
    std::vector<std::uint8_t> expectedTimestampEncoding;
    if (timestamped) {
      auto timeFactory =
          rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
              L"HLAinteger64Time");
      auto* integerFactory =
          dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(timeFactory.get());
      if (integerFactory == nullptr) {
        throw std::runtime_error(
            "The installed timestamped package smoke could not create HLAinteger64Time.");
      }
      auto timestamp = integerFactory->makeLogicalTime(5);
      if (!timestamp) {
        throw std::runtime_error(
            "The installed timestamped package smoke could not create its logical time.");
      }
      auto const encodedTimestamp = timestamp->encode();
      if (encodedTimestamp.size() != 0U) {
        auto const* first =
            static_cast<std::uint8_t const*>(encodedTimestamp.data());
        expectedTimestampEncoding.assign(
            first, first + encodedTimestamp.size());
      }
      auto const retraction = sender->sendInteraction(
          interactionClass, parameterValues, tag, *timestamp);
      if (retraction.isValid()) {
        throw std::runtime_error(
            "The installed process timestamped smoke fabricated a retraction handle.");
      }
    } else {
      sender->sendInteraction(interactionClass, parameterValues, tag);
    }

    if (!connectionLoss) {
      auto const deadline = std::chrono::steady_clock::now() +
          std::chrono::seconds(30);
      while (!receiverAmbassador.received &&
             std::chrono::steady_clock::now() < deadline) {
        static_cast<void>(receiver->evokeMultipleCallbacks(0.0, 0.05));
        if (!receiverAmbassador.received) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      if (!receiverAmbassador.received ||
          (timestamped != receiverAmbassador.timestampedReceived) ||
          receiverAmbassador.interactionClassHandle != interactionClass ||
          !receiverAmbassador.producingFederateHandle.isValid() ||
          receiverAmbassador.parameterCount != (parameterized ? 1U : 0U) ||
          receiverAmbassador.tag !=
              std::vector<std::uint8_t>(encodedTag.begin(), encodedTag.end())) {
        throw std::runtime_error("The installed receiver did not preserve the process interaction callback.");
      }
      if (parameterized &&
          (receiverAmbassador.parameterHandle != parameterHandle ||
           receiverAmbassador.parameterValue != expectedParameterValue)) {
        throw std::runtime_error(
            "The installed receiver did not preserve the parameterized process interaction.");
      }
      if (timestamped &&
          (receiverAmbassador.timestampImplementationName != L"HLAinteger64Time" ||
           receiverAmbassador.timestampEncoding != expectedTimestampEncoding ||
           receiverAmbassador.hasOptionalRetraction ||
           receiverAmbassador.sentOrderType != rti1516_2025::RECEIVE ||
           receiverAmbassador.receivedOrderType != rti1516_2025::RECEIVE)) {
        throw std::runtime_error(
            "The installed receiver did not preserve the timestamped process interaction.");
      }
    } else if (receiverAmbassador.received) {
      throw std::runtime_error(
          "The installed receiver received an interaction after Connection Lost.");
    }

    sender->resignFederationExecution(NO_ACTION);
    senderJoined = false;
    if (!connectionLoss) {
      receiver->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      receiver->disconnect();
    }
    sender->disconnect();
    }
    }
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined && sender) {
      try {
        sender->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (receiverJoined && receiver) {
      try {
        receiver->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (receiver) {
      try {
        receiver->disconnect();
      } catch (...) {
      }
    }
    if (sender) {
      try {
        sender->disconnect();
      } catch (...) {
      }
    }
  }

  auto const serverStatus = server.get();
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  if (serverStatus != 0) {
    throw std::runtime_error("The package process smoke server returned a failure.");
  }
  if (!std::filesystem::exists(directory / "send.ok") ||
      !std::filesystem::exists(directory / "server.ok")) {
    throw std::runtime_error("The package process smoke server did not complete its markers.");
  }
  std::filesystem::remove_all(directory, ignored);
  return 0;
#endif
}

}  // namespace

int main(int argc, char** argv) {
  bool timestamped = false;
  bool connectionLoss = false;
  bool automaticDeleteObjects = false;
  bool parameterized = false;
  bool objectRegistration = false;
  bool namedRegistration = false;
  bool attributeUpdate = false;
  bool directedRetraction = false;
  bool federationSaveRestore = false;
  bool federationSaveRestoreFailure = false;
  bool federationSaveRestoreAbort = false;
  bool federationSaveRestoreStatus = false;
  if (argc > 2) {
    return 2;
  }
  if (argc == 2) {
    auto const mode = std::string(argv[1]);
    if (mode == "timestamped") {
      timestamped = true;
    } else if (mode == "connection-loss") {
      connectionLoss = true;
    } else if (mode == "connection-loss-delete-objects") {
      connectionLoss = true;
      automaticDeleteObjects = true;
    } else if (mode == "parameterized") {
      parameterized = true;
    } else if (mode == "object-registration") {
      objectRegistration = true;
    } else if (mode == "named-registration") {
      namedRegistration = true;
    } else if (mode == "attribute-update") {
      attributeUpdate = true;
    } else if (mode == "directed-retraction") {
      directedRetraction = true;
    } else if (mode == "federation-save-restore") {
      federationSaveRestore = true;
    } else if (mode == "federation-save-restore-failure") {
      federationSaveRestoreFailure = true;
    } else if (mode == "federation-save-restore-abort") {
      federationSaveRestoreAbort = true;
    } else if (mode == "federation-save-restore-status") {
      federationSaveRestoreStatus = true;
    } else {
      return 2;
    }
  }
  try {
    return run(
        timestamped,
        connectionLoss,
        automaticDeleteObjects,
        parameterized,
        objectRegistration,
        namedRegistration,
        attributeUpdate,
        directedRetraction,
        federationSaveRestore,
        federationSaveRestoreFailure,
        federationSaveRestoreAbort,
        federationSaveRestoreStatus);
  } catch (std::exception const& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
