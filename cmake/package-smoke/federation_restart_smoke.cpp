#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace rti1516_2025;

constexpr wchar_t federationName[] = L"UmbraPackageRestartSaveRestore";
constexpr wchar_t ownerName[] = L"UmbraPackageRestartOwner";
constexpr wchar_t peerName[] = L"UmbraPackageRestartPeer";
constexpr wchar_t objectName[] = L"UmbraPackageRestartEmployee";
constexpr wchar_t saveLabel[] = L"pending-attribute-transportation-restart";

void require(bool condition, std::string const& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

class RestartFederateAmbassador final : public NullFederateAmbassador {
 public:
  void objectInstanceNameReservationSucceeded(
      std::wstring const& reservedName) override {
    if (reservedName == objectName) {
      objectNameReserved = true;
    }
  }

  void initiateFederateRestore(
      std::wstring const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++restoreInitiations;
  }

  void initiateFederateSave(std::wstring const& label) override {
    if (label == saveLabel) {
      ++saveInitiations;
    }
  }

  void federationSaved() override {
    ++saveCompletions;
  }

  void federationNotSaved(SaveFailureReason reason) override {
    ++saveFailures;
    saveFailureReason = static_cast<int>(reason);
  }

  void federationRestored() override {
    ++restoreCompletions;
  }

  void federationNotRestored(RestoreFailureReason reason) override {
    ++restoreFailures;
    restoreFailureReason = static_cast<int>(reason);
  }

  void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&,
      TransportationTypeHandle const& transportationType) override {
    ++transportationConfirmations;
    confirmedTransportation = transportationType;
  }

  void reportAttributeTransportationType(
      ObjectInstanceHandle const&,
      AttributeHandle const&,
      TransportationTypeHandle const& transportationType) override {
    queriedTransportation = transportationType;
  }

  void confirmInteractionTransportationTypeChange(
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    ++interactionTransportationConfirmations;
    confirmedInteraction = interactionClass;
    confirmedInteractionTransportation = transportationType;
  }

  void reportInteractionTransportationType(
      FederateHandle const& federate,
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    queriedInteractionFederate = federate;
    queriedInteraction = interactionClass;
    queriedInteractionTransportation = transportationType;
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const& transportationType,
      FederateHandle const&,
      RegionHandleSet const*) override {
    ++interactionReceives;
    receivedInteraction = interactionClass;
    receivedInteractionTransportation = transportationType;
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const& values,
      VariableLengthData const&,
      TransportationTypeHandle const& transportationType,
      FederateHandle const&,
      RegionHandleSet const*) override {
    if (!values.empty()) {
      auto const& encoded = values.begin()->second;
      auto const* bytes = static_cast<std::uint8_t const*>(encoded.data());
      reflectedValue.assign(bytes, bytes + encoded.size());
    }
    reflectedTransportation = transportationType;
    ++reflections;
  }

  std::size_t restoreInitiations = 0U;
  std::size_t restoreCompletions = 0U;
  std::size_t restoreFailures = 0U;
  std::size_t saveInitiations = 0U;
  std::size_t saveCompletions = 0U;
  std::size_t saveFailures = 0U;
  std::size_t transportationConfirmations = 0U;
  std::size_t interactionTransportationConfirmations = 0U;
  std::size_t interactionReceives = 0U;
  std::size_t reflections = 0U;
  bool objectNameReserved = false;
  TransportationTypeHandle confirmedTransportation;
  InteractionClassHandle confirmedInteraction;
  TransportationTypeHandle confirmedInteractionTransportation;
  FederateHandle queriedInteractionFederate;
  InteractionClassHandle queriedInteraction;
  TransportationTypeHandle queriedInteractionTransportation;
  InteractionClassHandle receivedInteraction;
  TransportationTypeHandle receivedInteractionTransportation;
  TransportationTypeHandle queriedTransportation;
  TransportationTypeHandle reflectedTransportation;
  int saveFailureReason = 0;
  int restoreFailureReason = 0;
  std::vector<std::uint8_t> reflectedValue;
};

RtiConfiguration configurationFor(
    std::filesystem::path const& saveDirectory,
    std::filesystem::path const& reportDirectory) {
  std::wstring const settings =
      L"serviceReportDirectory=" + reportDirectory.wstring() +
      L";federationSaveDirectory=" + saveDirectory.wstring();
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(L"package-restart-save-restore")
      .withRtiAddress(L"in-process")
      .withAdditionalSettings(settings);
}

std::unique_ptr<RTIambassador> connect(
    RTIambassadorFactory& factory,
    RestartFederateAmbassador& federateAmbassador,
    std::filesystem::path const& saveDirectory,
    std::filesystem::path const& reportDirectory) {
  auto ambassador = factory.createRTIambassador();
  require(static_cast<bool>(ambassador), "installed package factory returned no RTIambassador");
  static_cast<void>(ambassador->connect(
      federateAmbassador,
      HLA_EVOKED,
      configurationFor(saveDirectory, reportDirectory)));
  return ambassador;
}

void pump(RTIambassador& owner, RTIambassador& peer) {
  for (int pass = 0; pass != 32; ++pass) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 0.0));
    static_cast<void>(peer.evokeMultipleCallbacks(0.0, 0.0));
  }
}

template <typename Predicate>
void pumpUntil(
    RTIambassador& owner,
    RTIambassador& peer,
    Predicate predicate,
    char const* description) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(8);
  while (std::chrono::steady_clock::now() < deadline) {
    pump(owner, peer);
    if (predicate()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error(std::string("timed out waiting for ") + description);
}

std::vector<std::filesystem::path> reportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> result;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      result.push_back(entry.path());
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

void createAndJoin(
    RTIambassador& owner,
    RTIambassador& peer,
    bool createFederationExecution) {
  if (createFederationExecution) {
    owner.createFederationExecution(
        federationName,
        std::vector<std::wstring>{
            std::filesystem::path(UMBRA_PACKAGE_RESTAURANT_FOM).wstring(),
            std::filesystem::path(UMBRA_PACKAGE_SWITCH_FOM).wstring()},
        L"HLAinteger64Time");
  }
  static_cast<void>(owner.joinFederationExecution(ownerName, L"publisher", federationName));
  static_cast<void>(peer.joinFederationExecution(peerName, L"subscriber", federationName));
}

void runSavePhase(
    std::filesystem::path const& saveDirectory,
    std::filesystem::path const& reportDirectory) {
  RestartFederateAmbassador ownerReports;
  RestartFederateAmbassador peerReports;
  RTIambassadorFactory factory;
  auto owner = connect(factory, ownerReports, saveDirectory, reportDirectory);
  auto peer = connect(factory, peerReports, saveDirectory, reportDirectory);
  createAndJoin(*owner, *peer, true);

  auto const objectClass = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = owner->getAttributeHandle(objectClass, L"Efficiency");
  auto const bestEffort = owner->getTransportationTypeHandle(L"HLAbestEffort");
  require(objectClass.isValid() && attribute.isValid() && bestEffort.isValid(),
          "installed package could not resolve transportation-save handles");
  AttributeHandleSet attributes;
  attributes.insert(attribute);
  owner->publishObjectClassAttributes(objectClass, attributes);
  auto const peerClass = peer->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const peerAttribute = peer->getAttributeHandle(peerClass, L"Efficiency");
  AttributeHandleSet peerAttributes;
  peerAttributes.insert(peerAttribute);
  peer->subscribeObjectClassAttributes(peerClass, peerAttributes, true, L"");
  owner->reserveObjectInstanceName(objectName);
  pumpUntil(
      *owner,
      *peer,
      [&] { return ownerReports.objectNameReserved; },
      "named object reservation");
  auto const object = owner->registerObjectInstance(objectClass, objectName);
  require(object.isValid(), "installed package registered an invalid object handle");

  std::vector<std::uint8_t> initialValue{0x4aU, 0x06U};
  AttributeHandleValueMap initialValues;
  initialValues.emplace(
      attribute,
      VariableLengthData(initialValue.data(), initialValue.size()));
  owner->updateAttributeValues(object, initialValues, VariableLengthData{});
  pump(*owner, *peer);

  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const peerInteractionClass = peer->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  require(interactionClass.isValid() && peerInteractionClass.isValid(),
          "installed package could not resolve the interaction transport-save handle");
  owner->publishInteractionClass(interactionClass);
  peer->subscribeInteractionClass(peerInteractionClass);
  owner->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      VariableLengthData{});
  pumpUntil(
      *owner,
      *peer,
      [&] { return peerReports.interactionReceives == 1U; },
      "initial reliable interaction delivery");
  require(
      peerReports.receivedInteraction == peerInteractionClass &&
          peer->getTransportationTypeName(
              peerReports.receivedInteractionTransportation) == L"HLAreliable",
      "installed package did not deliver the initial interaction reliably");
  owner->requestAttributeTransportationTypeChange(object, attributes, bestEffort);
  owner->requestInteractionTransportationTypeChange(
      interactionClass,
      bestEffort);

  owner->requestFederationSave(saveLabel);
  owner->federateSaveBegun();
  peer->federateSaveBegun();
  owner->federateSaveComplete();
  peer->federateSaveComplete();
  pumpUntil(
      *owner,
      *peer,
      [&] {
        return ownerReports.saveCompletions + ownerReports.saveFailures == 1U &&
            peerReports.saveCompletions + peerReports.saveFailures == 1U;
      },
      "save completion for both joined federates");
  require(
      ownerReports.saveFailures == 0U && peerReports.saveFailures == 0U,
      "installed package reported federation save failure reason " +
          std::to_string(ownerReports.saveFailureReason) + "/" +
          std::to_string(peerReports.saveFailureReason));

  // The persisted image is the evidence under test. Resigning and destroying
  // the source execution ensures the restore child cannot use live registry
  // state to satisfy its queries.
  peer->resignFederationExecution(NO_ACTION);
  owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federationName);
  peer->disconnect();
  owner->disconnect();
}

void runRestorePhase(
    std::filesystem::path const& saveDirectory,
    std::filesystem::path const& reportDirectory) {
  RestartFederateAmbassador ownerReports;
  RestartFederateAmbassador peerReports;
  RTIambassadorFactory factory;
  auto owner = connect(factory, ownerReports, saveDirectory, reportDirectory);
  auto peer = connect(factory, peerReports, saveDirectory, reportDirectory);
  createAndJoin(*owner, *peer, true);

  owner->requestFederationRestore(saveLabel);
  auto const restoreInitiationDeadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(8);
  while (std::chrono::steady_clock::now() < restoreInitiationDeadline &&
         (ownerReports.restoreInitiations != 1U ||
          peerReports.restoreInitiations != 1U) &&
         ownerReports.restoreFailures == 0U && peerReports.restoreFailures == 0U) {
    pump(*owner, *peer);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      ownerReports.restoreFailures == 0U && peerReports.restoreFailures == 0U,
      "installed package reported restore failure reason " +
          std::to_string(ownerReports.restoreFailureReason) + "/" +
          std::to_string(peerReports.restoreFailureReason));
  require(
      ownerReports.restoreInitiations == 1U && peerReports.restoreInitiations == 1U,
      "timed out waiting for restore initiation for both rejoined federates");
  owner->federateRestoreComplete();
  peer->federateRestoreComplete();
  pumpUntil(
      *owner,
      *peer,
      [&] {
        return ownerReports.restoreCompletions == 1U &&
            peerReports.restoreCompletions == 1U &&
            ownerReports.transportationConfirmations == 1U &&
            ownerReports.interactionTransportationConfirmations == 1U;
      },
      "restored pending transportation confirmation");

  auto const object = owner->getObjectInstanceHandle(objectName);
  auto const objectClass = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = owner->getAttributeHandle(objectClass, L"Efficiency");
  auto const bestEffort = owner->getTransportationTypeHandle(L"HLAbestEffort");
  require(object.isValid() && attribute.isValid() && bestEffort.isValid(),
          "restored object or transportation handles are invalid");
  require(owner->getTransportationTypeName(ownerReports.confirmedTransportation) ==
              L"HLAbestEffort",
          "restore did not confirm the saved pending best-effort change");
  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  require(
      interactionClass.isValid() &&
          ownerReports.confirmedInteraction == interactionClass &&
          owner->getTransportationTypeName(
              ownerReports.confirmedInteractionTransportation) == L"HLAbestEffort",
      "restore did not confirm the saved pending interaction best-effort change");

  owner->queryAttributeTransportationType(object, attribute);
  pumpUntil(
      *owner,
      *peer,
      [&] { return ownerReports.queriedTransportation.isValid(); },
      "restored attribute transportation query");
  require(owner->getTransportationTypeName(ownerReports.queriedTransportation) ==
              L"HLAbestEffort",
          "restored attribute transportation query did not report best effort");

  auto const ownerFederate = peer->getFederateHandle(ownerName);
  peer->queryInteractionTransportationType(ownerFederate, interactionClass);
  pumpUntil(
      *owner,
      *peer,
      [&] { return peerReports.queriedInteractionTransportation.isValid(); },
      "restored interaction transportation query");
  require(
      peerReports.queriedInteractionFederate == ownerFederate &&
          peerReports.queriedInteraction == interactionClass &&
          peer->getTransportationTypeName(
              peerReports.queriedInteractionTransportation) == L"HLAbestEffort",
      "restored interaction transportation query did not report best effort");

  std::vector<std::uint8_t> updatedValue{0x4bU, 0x07U};
  AttributeHandleValueMap updatedValues;
  updatedValues.emplace(
      attribute,
      VariableLengthData(updatedValue.data(), updatedValue.size()));
  owner->updateAttributeValues(object, updatedValues, VariableLengthData{});
  pumpUntil(
      *owner,
      *peer,
      [&] { return peerReports.reflections == 1U; },
      "post-restore attribute reflection");
  require(peerReports.reflectedValue == updatedValue &&
              peer->getTransportationTypeName(peerReports.reflectedTransportation) ==
                  L"HLAbestEffort",
          "post-restore reflection did not preserve best-effort transport and value");

  owner->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      VariableLengthData{});
  pumpUntil(
      *owner,
      *peer,
      [&] { return peerReports.interactionReceives == 1U; },
      "post-restore best-effort interaction delivery");
  require(
      peerReports.receivedInteraction == interactionClass &&
          peer->getTransportationTypeName(
              peerReports.receivedInteractionTransportation) == L"HLAbestEffort",
      "post-restore interaction did not preserve best-effort transport");

  peer->resignFederationExecution(NO_ACTION);
  owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federationName);
  peer->disconnect();
  owner->disconnect();
}

std::string quote(std::string const& value) {
  return "\"" + value + "\"";
}

class TemporaryDirectory final {
 public:
  explicit TemporaryDirectory(std::filesystem::path path) : path_(std::move(path)) {
    std::filesystem::create_directories(path_);
  }
  ~TemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }
  std::filesystem::path const& path() const noexcept { return path_; }

 private:
  std::filesystem::path path_;
};

int runChild(
    std::string const& executable,
    char const* phase,
    std::filesystem::path const& root) {
  auto const command = quote(executable) + " --phase " + phase + " " +
      quote((root / "saves").string()) + " " +
      quote((root / "reports").string());
#if defined(_WIN32)
  auto const shellCommand = std::string("\"") + command + "\"";
#else
  auto const& shellCommand = command;
#endif
  return std::system(shellCommand.c_str());
}

void runProcessRestartScenario(std::string const& executable) {
  auto const tick = std::chrono::steady_clock::now().time_since_epoch().count();
  TemporaryDirectory root(
      std::filesystem::current_path() /
      ("umbra-package-restart-" + std::to_string(tick)));
  auto const sourceResult = runChild(executable, "save", root.path());
  if (sourceResult != 0) {
    std::string saveEntries;
    auto const saveDirectory = root.path() / "saves";
    if (std::filesystem::exists(saveDirectory)) {
      for (auto const& entry : std::filesystem::directory_iterator(saveDirectory)) {
        if (!saveEntries.empty()) {
          saveEntries += ", ";
        }
        saveEntries += entry.path().filename().string();
      }
    }
    require(
        false,
        "installed-package save process failed; filesystem entries: " +
            (saveEntries.empty() ? std::string("<none>") : saveEntries));
  }
  auto const sourceReports = reportFiles(root.path() / "reports");
  require(sourceReports.size() == 2U, "source process did not create two joined-federate report files");
  bool hasSaveManifest = false;
  for (auto const& entry : std::filesystem::directory_iterator(root.path() / "saves")) {
    hasSaveManifest = hasSaveManifest || entry.is_regular_file();
  }
  require(hasSaveManifest, "save process did not persist a filesystem commit manifest");

  auto const restoreResult = runChild(executable, "restore", root.path());
  require(restoreResult == 0, "installed-package restore process failed");
  auto const restoredReports = reportFiles(root.path() / "reports");
  require(restoredReports.size() == 4U, "fresh process did not receive new report files for its joins");
  for (auto const& sourceReport : sourceReports) {
    require(std::find(restoredReports.begin(), restoredReports.end(), sourceReport) !=
                restoredReports.end(),
            "fresh process replaced a source joined-federate report file");
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 5 && std::string(argv[1]) == "--phase") {
      auto const saveDirectory = std::filesystem::path(argv[3]);
      auto const reportDirectory = std::filesystem::path(argv[4]);
      if (std::string(argv[2]) == "save") {
        runSavePhase(saveDirectory, reportDirectory);
        return 0;
      }
      if (std::string(argv[2]) == "restore") {
        runRestorePhase(saveDirectory, reportDirectory);
        return 0;
      }
      return 2;
    }
    require(argc == 1, "unexpected package restart smoke arguments");
    runProcessRestartScenario(std::filesystem::absolute(argv[0]).string());
    std::cout << "installed-package federation save/restore survived provider process restart\n";
    return 0;
  } catch (Exception const& error) {
    std::wcerr << L"package restart smoke failed: " << error.name() << L": "
               << error.what() << L'\n';
    return 1;
  } catch (std::exception const& error) {
    std::cerr << "package restart smoke failed: " << error.what() << '\n';
    return 1;
  }
}
