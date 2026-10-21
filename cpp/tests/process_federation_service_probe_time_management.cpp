#include "process_federation_service_probe_time_management.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace umbra::test {
namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;

void writeText(
    std::filesystem::path const& path,
    std::string const& text) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output || !(output << text)) {
    throw std::runtime_error("Could not write no-TSO process marker.");
  }
}

std::string awaitMarker(
    std::filesystem::path const& directory,
    std::string const& markerName,
    std::string const& errorRole,
    std::chrono::seconds timeout = std::chrono::seconds(20)) {
  auto const marker = directory / markerName;
  auto const errorMarker = directory / (errorRole + ".error");
  auto const deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(errorMarker)) {
      std::ifstream errorInput(errorMarker, std::ios::binary);
      std::string message{
          std::istreambuf_iterator<char>(errorInput),
          std::istreambuf_iterator<char>()};
      throw std::runtime_error(errorRole + " process failed: " + message);
    }
    if (std::filesystem::exists(marker)) {
      std::ifstream input(marker, std::ios::binary);
      std::string value{
          std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
      if (!value.empty()) {
        return value;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error("Timed out waiting for process marker " + markerName);
}

std::uint16_t authorityPort(
    std::filesystem::path const& directory,
    std::string const& role = "no-tso-coordinator-authority") {
  std::ifstream input(directory / (role + ".port"));
  unsigned int port = 0U;
  if (!(input >> port) || port == 0U || port > 65535U) {
    throw std::runtime_error("The no-TSO coordinator published an invalid port.");
  }
  return static_cast<std::uint16_t>(port);
}

void publishReadyMarker(
    std::filesystem::path const& path,
    std::uint16_t port) {
  auto temporary = path;
  temporary += ".tmp";
  writeText(temporary, std::to_string(port));
  std::filesystem::rename(temporary, path);
}

RtiConfiguration configurationFor(
    std::uint16_t port,
    wchar_t const* identity) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(identity)
      .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
}

void evokeUntil(
    RTIambassador& ambassador,
    auto&& ready,
    char const* failureMessage) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(15);
  while (!ready() && std::chrono::steady_clock::now() < deadline) {
    static_cast<void>(ambassador.evokeCallback(0.01));
  }
  if (!ready()) {
    throw std::runtime_error(failureMessage);
  }
}

class NoTsoTimeAmbassador final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
    regulationEnabled = true;
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
    constrainedEnabled = true;
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    auto const* integerTime =
        dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
    if (integerTime == nullptr) {
      throw std::runtime_error("A no-TSO process grant used a different time type.");
    }
    grants.push_back(integerTime->getTime());
  }

  bool regulationEnabled = false;
  bool constrainedEnabled = false;
  std::vector<std::int64_t> grants;
};

class DeleteTsoAmbassador final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
    regulationEnabled = true;
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
    constrainedEnabled = true;
  }

  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& instance,
      rti1516_2025::ObjectClassHandle const& objectClass,
      std::wstring const&,
      rti1516_2025::FederateHandle const& producer) override {
    ++discoveries;
    discoveredInstance = instance;
    discoveredClass = objectClass;
    discoveredProducer = producer;
    callbackOrder.push_back('D');
  }

  void removeObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& instance,
      rti1516_2025::VariableLengthData const& tag,
      rti1516_2025::FederateHandle const& producer,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType sentOrder,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const*) override {
    ++removals;
    removedInstance = instance;
    removingProducer = producer;
    removalSentOrder = sentOrder;
    removalReceivedOrder = receivedOrder;
    if (auto const* bytes =
            static_cast<unsigned char const*>(tag.data());
        bytes != nullptr) {
      removalTag.assign(bytes, bytes + tag.size());
    }
    auto const* integerTime =
        dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
    if (integerTime != nullptr) {
      removalTime = integerTime->getTime();
    }
    callbackOrder.push_back('R');
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    auto const* integerTime =
        dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
    if (integerTime == nullptr) {
      throw std::runtime_error("A Delete Object Instance grant used a different time type.");
    }
    grants.push_back(integerTime->getTime());
    callbackOrder.push_back('G');
  }

  bool regulationEnabled = false;
  bool constrainedEnabled = false;
  std::size_t discoveries = 0U;
  std::size_t removals = 0U;
  rti1516_2025::ObjectInstanceHandle discoveredInstance;
  rti1516_2025::ObjectInstanceHandle removedInstance;
  rti1516_2025::ObjectClassHandle discoveredClass;
  rti1516_2025::FederateHandle discoveredProducer;
  rti1516_2025::FederateHandle removingProducer;
  rti1516_2025::OrderType removalSentOrder = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType removalReceivedOrder = rti1516_2025::RECEIVE;
  std::vector<unsigned char> removalTag;
  std::int64_t removalTime = -1;
  std::vector<std::int64_t> grants;
  std::vector<char> callbackOrder;
};

void stopAfterMarker(
    std::filesystem::path const& directory,
    std::string const& role) {
  static_cast<void>(awaitMarker(
      directory, role + ".stop", role, std::chrono::seconds(15)));
  writeText(directory / (role + ".ok"), "stopped");
}

int runRegulatingClient(std::filesystem::path const& directory) {
  constexpr wchar_t const* federationName =
      L"process-no-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"no-tso-authoritative-regulator";
  constexpr wchar_t const* federateType = L"no-tso-authoritative-type";
  constexpr char const* role = "no-tso-coordinator-regulator";

  NoTsoTimeAmbassador ambassador;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           HLA_EVOKED,
           configurationFor(authorityPort(directory), L"no-tso-regulator"))
           .addressUsed) {
    throw std::runtime_error("The regulator did not select the coordinator endpoint.");
  }
  rti->createFederationExecution(
      federationName, L"server-owned-authoritative-tso-fom.xml");
  auto const handle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!handle.isValid()) {
    throw std::runtime_error("The no-TSO regulator Join returned an invalid handle.");
  }
  rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5));
  evokeUntil(
      *rti,
      [&] { return ambassador.regulationEnabled; },
      "The no-TSO regulator did not receive Time Regulation Enabled.");
  publishReadyMarker(directory / (std::string(role) + ".port"), 1U);
  writeText(directory / (std::string(role) + ".ready"), "regulator-at-zero");

  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-requester.nmr-pending",
      "no-tso-coordinator-requester"));
  rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants.size() >= 1U; },
      "The no-TSO regulator did not receive its first grant.");
  if (ambassador.grants[0] != 1) {
    throw std::runtime_error("The no-TSO regulator's first grant was not time 1.");
  }
  writeText(
      directory / (std::string(role) + ".advanced1"),
      "regulator-granted-at-1");

  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-requester.nmra-pending",
      "no-tso-coordinator-requester"));
  rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants.size() >= 2U; },
      "The no-TSO regulator did not receive its second grant.");
  if (ambassador.grants[1] != 2) {
    throw std::runtime_error("The no-TSO regulator's second grant was not time 2.");
  }
  writeText(
      directory / (std::string(role) + ".advanced2"),
      "regulator-granted-at-2");
  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-requester.tara-pending",
      "no-tso-coordinator-requester"));
  rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants.size() >= 3U; },
      "The no-TSO regulator did not receive its third grant.");
  if (ambassador.grants[2] != 3) {
    throw std::runtime_error("The no-TSO regulator's third grant was not time 3.");
  }
  writeText(
      directory / (std::string(role) + ".advanced3"),
      "regulator-granted-at-3");
  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-requester.completed",
      "no-tso-coordinator-requester"));
  writeText(
      directory / (std::string(role) + ".completed"),
      "regulator-granted-at-1-2-and-3");

  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  stopAfterMarker(directory, role);
  return 0;
}

int runConstrainedClient(std::filesystem::path const& directory) {
  constexpr wchar_t const* federationName =
      L"process-no-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"no-tso-authoritative-requester";
  constexpr wchar_t const* federateType = L"no-tso-authoritative-type";
  constexpr char const* role = "no-tso-coordinator-requester";

  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-regulator.ready",
      "no-tso-coordinator-regulator"));
  NoTsoTimeAmbassador ambassador;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           HLA_EVOKED,
           configurationFor(authorityPort(directory), L"no-tso-requester"))
           .addressUsed) {
    throw std::runtime_error("The requester did not select the coordinator endpoint.");
  }
  auto const handle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!handle.isValid()) {
    throw std::runtime_error("The no-TSO requester Join returned an invalid handle.");
  }
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The no-TSO requester did not receive Time Constrained Enabled.");
  publishReadyMarker(directory / (std::string(role) + ".port"), 1U);

  rti->nextMessageRequest(rti1516_2025::HLAinteger64Time(5));
  static_cast<void>(rti->evokeCallback(0.0));
  if (!ambassador.grants.empty()) {
    throw std::runtime_error("NMR granted at the strict GALT boundary of time 5.");
  }
  writeText(
      directory / (std::string(role) + ".nmr-pending"),
      "NMR-5-pending-at-GALT-5");

  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-regulator.advanced1",
      "no-tso-coordinator-regulator"));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants.size() >= 1U; },
      "The no-TSO requester did not receive its NMR grant after GALT advanced.");
  if (ambassador.grants[0] != 5) {
    throw std::runtime_error("The no-TSO NMR grant was not at time 5.");
  }

  rti->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(7));
  static_cast<void>(rti->evokeCallback(0.0));
  if (ambassador.grants.size() != 1U) {
    throw std::runtime_error("NMRA granted above the current GALT of time 6.");
  }
  writeText(
      directory / (std::string(role) + ".nmra-pending"),
      "NMRA-7-pending-at-GALT-6");

  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-regulator.advanced2",
      "no-tso-coordinator-regulator"));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants.size() >= 2U; },
      "The no-TSO requester did not receive its NMRA grant at the inclusive boundary.");
  if (ambassador.grants != std::vector<std::int64_t>{5, 7}) {
    throw std::runtime_error("The no-TSO requester grants were not NMR(5), NMRA(7).");
  }

  rti->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(8));
  static_cast<void>(rti->evokeCallback(0.0));
  if (ambassador.grants.size() != 2U) {
    throw std::runtime_error("TARA granted above the current GALT of time 7.");
  }
  writeText(
      directory / (std::string(role) + ".tara-pending"),
      "TARA-8-pending-at-GALT-7");
  static_cast<void>(awaitMarker(
      directory,
      "no-tso-coordinator-regulator.advanced3",
      "no-tso-coordinator-regulator"));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants.size() >= 3U; },
      "The no-TSO requester did not receive its TARA grant at the inclusive boundary.");
  if (ambassador.grants != std::vector<std::int64_t>{5, 7, 8}) {
    throw std::runtime_error(
        "The no-TSO requester grants were not NMR(5), NMRA(7), TARA(8).");
  }
  writeText(
      directory / (std::string(role) + ".completed"),
      "NMR-5-NMRA-7-TARA-8");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  stopAfterMarker(directory, role);
  return 0;
}

int runDeleteTsoSender(
    std::filesystem::path const& directory,
    std::string const& role,
    std::string const& prefix) {
  constexpr wchar_t const* federationName =
      L"process-tso-delete-authoritative-coordinator";
  constexpr wchar_t const* federateType = L"delete-tso-authoritative-type";
  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* attributeName = L"Flavor";

  DeleteTsoAmbassador ambassador;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           HLA_EVOKED,
           configurationFor(
               authorityPort(directory, prefix + "-authority"),
               L"delete-tso-authoritative-sender"))
           .addressUsed) {
    throw std::runtime_error("The deletion sender did not select the coordinator endpoint.");
  }
  rti->createFederationExecution(
      federationName, L"server-owned-authoritative-tso-fom.xml");
  auto const senderHandle = rti->joinFederationExecution(
      L"delete-tso-authoritative-sender", federateType, federationName);
  if (!senderHandle.isValid()) {
    throw std::runtime_error("The deletion sender Join returned an invalid handle.");
  }
  auto const objectClass = rti->getObjectClassHandle(objectClassName);
  auto const attribute = rti->getAttributeHandle(objectClass, attributeName);
  rti->publishObjectClassAttributes(objectClass, {attribute});
  rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1));
  evokeUntil(
      *rti,
      [&] { return ambassador.regulationEnabled; },
      "The deletion sender did not receive Time Regulation Enabled.");
  publishReadyMarker(directory / (role + ".port"), 1U);
  writeText(directory / (role + ".ready"), "regulation-enabled");

  static_cast<void>(awaitMarker(
      directory, prefix + "-receiver.joined", prefix + "-receiver"));
  auto const objectInstance = rti->registerObjectInstance(objectClass);
  if (!objectInstance.isValid()) {
    throw std::runtime_error("The deletion sender could not register the Soda object.");
  }
  writeText(directory / (role + ".registered"), "soda-instance-registered");
  static_cast<void>(awaitMarker(
      directory, prefix + "-receiver.requested", prefix + "-receiver"));

  std::uint8_t const tagByte = 0x71U;
  rti->deleteObjectInstance(
      objectInstance,
      rti1516_2025::VariableLengthData(&tagByte, sizeof(tagByte)),
      rti1516_2025::HLAinteger64Time(5));
  rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5));
  writeText(directory / (role + ".sent"), "sent-delete-at-5");
  static_cast<void>(awaitMarker(
      directory,
      prefix + "-receiver.completed",
      prefix + "-receiver"));
  writeText(directory / (role + ".completed"), "sent-delete-at-5");

  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  stopAfterMarker(directory, role);
  return 0;
}

int runDeleteTsoReceiver(
    std::filesystem::path const& directory,
    std::string const& role,
    std::string const& prefix,
    std::string const& advanceMode) {
  constexpr wchar_t const* federationName =
      L"process-tso-delete-authoritative-coordinator";
  constexpr wchar_t const* federateType = L"delete-tso-authoritative-type";
  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* attributeName = L"Flavor";

  static_cast<void>(awaitMarker(
      directory, prefix + "-sender.ready", prefix + "-sender"));
  DeleteTsoAmbassador ambassador;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           HLA_EVOKED,
           configurationFor(
               authorityPort(directory, prefix + "-authority"),
               L"delete-tso-authoritative-receiver"))
           .addressUsed) {
    throw std::runtime_error("The deletion receiver did not select the coordinator endpoint.");
  }
  auto const receiverHandle = rti->joinFederationExecution(
      L"delete-tso-authoritative-receiver", federateType, federationName);
  if (!receiverHandle.isValid()) {
    throw std::runtime_error("The deletion receiver Join returned an invalid handle.");
  }
  auto const objectClass = rti->getObjectClassHandle(objectClassName);
  auto const attribute = rti->getAttributeHandle(objectClass, attributeName);
  rti->subscribeObjectClassAttributes(objectClass, {attribute}, true);
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The deletion receiver did not receive Time Constrained Enabled.");
  publishReadyMarker(directory / (role + ".port"), 1U);
  writeText(directory / (role + ".joined"), "time-constrained-and-subscribed");
  static_cast<void>(awaitMarker(
      directory, prefix + "-sender.registered", prefix + "-sender"));
  evokeUntil(
      *rti,
      [&] { return ambassador.discoveries == 1U; },
      "The deletion receiver did not discover the registered Soda object.");

  if (advanceMode == "nmr") {
    rti->nextMessageRequest(rti1516_2025::HLAinteger64Time(10));
  } else if (advanceMode == "nmra") {
    rti->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10));
  } else if (advanceMode == "tara") {
    rti->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(6));
  } else {
    throw std::runtime_error("Unknown timestamped deletion advance mode.");
  }
  std::string const advanceName = advanceMode == "nmr"
      ? "NMR"
      : advanceMode == "nmra" ? "NMRA" : "TARA";
  std::int64_t const expectedGrantTime = advanceMode == "tara" ? 6 : 5;
  writeText(
      directory / (role + ".requested"),
      advanceMode == "tara" ? "TARA-6" : advanceName + "-10");
  if (advanceMode != "tara") {
    static_cast<void>(awaitMarker(
        directory, prefix + "-sender.sent", prefix + "-sender"));
  }
  evokeUntil(
      *rti,
      [&] { return ambassador.grants.size() == 1U; },
      advanceMode == "nmr"
          ? "The deletion receiver did not receive its NMR grant."
          : advanceMode == "nmra"
              ? "The deletion receiver did not receive its NMRA grant."
              : "The deletion receiver did not receive its TARA grant.");
  if (ambassador.removals != 1U ||
      ambassador.callbackOrder != std::vector<char>{'D', 'R', 'G'} ||
      ambassador.discoveredClass != objectClass ||
      ambassador.removedInstance != ambassador.discoveredInstance ||
      !ambassador.discoveredProducer.isValid() ||
      ambassador.removingProducer != ambassador.discoveredProducer ||
      ambassador.removalTag != std::vector<unsigned char>{0x71U} ||
      ambassador.removalTime != 5 ||
      ambassador.removalSentOrder != rti1516_2025::TIMESTAMP ||
      ambassador.removalReceivedOrder != rti1516_2025::TIMESTAMP ||
      ambassador.grants[0] != expectedGrantTime) {
    throw std::runtime_error("The timestamped deletion did not precede its " + advanceName +
        " grant with metadata intact.");
  }
  writeText(
      directory / (role + ".completed"),
      "delete-before-" + advanceMode + "-grant-at-" +
          std::to_string(expectedGrantTime));

  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  stopAfterMarker(directory, role);
  return 0;
}

}  // namespace

int runNoTsoCoordinatorClientProbe(
    std::filesystem::path const& directory,
    std::string const& role) {
  if (role == "no-tso-coordinator-regulator") {
    return runRegulatingClient(directory);
  }
  if (role == "no-tso-coordinator-requester") {
    return runConstrainedClient(directory);
  }
  throw std::runtime_error("Unknown no-TSO coordinator client role: " + role);
}

int runDeleteTsoCoordinatorClientProbe(
    std::filesystem::path const& directory,
    std::string const& role) {
  if (role == "delete-tso-nmr-sender") {
    return runDeleteTsoSender(directory, role, "delete-tso-nmr");
  }
  if (role == "delete-tso-nmr-receiver") {
    return runDeleteTsoReceiver(directory, role, "delete-tso-nmr", "nmr");
  }
  if (role == "delete-tso-nmra-sender") {
    return runDeleteTsoSender(directory, role, "delete-tso-nmra");
  }
  if (role == "delete-tso-nmra-receiver") {
    return runDeleteTsoReceiver(directory, role, "delete-tso-nmra", "nmra");
  }
  if (role == "delete-tso-tara-sender") {
    return runDeleteTsoSender(directory, role, "delete-tso-tara");
  }
  if (role == "delete-tso-tara-receiver") {
    return runDeleteTsoReceiver(directory, role, "delete-tso-tara", "tara");
  }
  throw std::runtime_error("Unknown Delete Object Instance process role: " + role);
}

}  // namespace umbra::test
