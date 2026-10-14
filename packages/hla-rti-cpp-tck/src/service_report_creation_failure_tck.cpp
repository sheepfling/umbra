#include "service_report_creation_failure_tck.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTIambassadorFactory.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck {
namespace {

namespace rti = rti1516_2025;

struct Discovery final {
  rti::ObjectInstanceHandle object;
  rti::ObjectClassHandle objectClass;
  std::wstring name;
};

class DiscoveryRecorder final : public rti::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::ObjectClassHandle const& objectClass,
      std::wstring const& name,
      rti::FederateHandle const&) override {
    std::scoped_lock lock(mutex_);
    discoveries_.push_back(Discovery{object, objectClass, name});
  }

  [[nodiscard]] std::size_t countForClass(
      rti::ObjectClassHandle const& objectClass) const {
    std::scoped_lock lock(mutex_);
    std::size_t count = 0U;
    for (auto const& discovery : discoveries_) {
      if (discovery.objectClass == objectClass) {
        ++count;
      }
    }
    return count;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<Discovery> discoveries_;
};

class Participant final {
 public:
  explicit Participant(rti::CallbackModel callbackModel)
      : callbackModel_(callbackModel), ambassador_(factory_.createRTIambassador()) {
    if (!ambassador_) {
      throw std::runtime_error("RTIambassadorFactory returned no ambassador");
    }
  }

  Participant(rti::CallbackModel callbackModel, rti::RtiConfiguration const& configuration)
      : Participant(callbackModel) {
    connect(configuration);
  }

  void connect(rti::RtiConfiguration const& configuration) {
    if (connected_) {
      throw std::runtime_error("Test participant is already connected");
    }
    static_cast<void>(ambassador_->connect(recorder_, callbackModel_, configuration));
    connected_ = true;
  }

  Participant(Participant const&) = delete;
  Participant& operator=(Participant const&) = delete;

  ~Participant() {
    if (joined_) {
      try {
        ambassador_->resignFederationExecution(rti::NO_ACTION);
      } catch (...) {
      }
    }
    if (connected_) {
      try {
        ambassador_->disconnect();
      } catch (...) {
      }
    }
  }

  void createFederation(
      std::wstring const& name,
      std::filesystem::path const& fom,
      std::filesystem::path const& mim,
      std::wstring const& logicalTimeImplementation) {
    ambassador_->createFederationExecutionWithMIM(
        name,
        std::vector<std::wstring>{fom.wstring()},
        mim.wstring(),
        logicalTimeImplementation);
  }

  [[nodiscard]] rti::FederateHandle join(
      std::wstring const& name,
      std::wstring const& federation) {
    auto const handle = ambassador_->joinFederationExecution(
        name, L"service-report-file-failure-tck", federation);
    if (!handle.isValid()) {
      throw std::runtime_error("Join Federation Execution returned an invalid handle");
    }
    joined_ = true;
    return handle;
  }

  void resign() {
    if (joined_) {
      ambassador_->resignFederationExecution(rti::NO_ACTION);
      joined_ = false;
    }
  }

  void destroyFederation(std::wstring const& name) {
    ambassador_->destroyFederationExecution(name);
  }

  void disconnect() {
    if (connected_) {
      ambassador_->disconnect();
      connected_ = false;
    }
  }

  [[nodiscard]] rti::RTIambassador& rti() { return *ambassador_; }
  [[nodiscard]] DiscoveryRecorder& recorder() { return recorder_; }

 private:
  rti::CallbackModel callbackModel_;
  rti::RTIambassadorFactory factory_;
  std::unique_ptr<rti::RTIambassador> ambassador_;
  DiscoveryRecorder recorder_;
  bool connected_ = false;
  bool joined_ = false;
};

class TemporaryRoot final {
 public:
  explicit TemporaryRoot(std::filesystem::path path) : path_(std::move(path)) {}
  TemporaryRoot(TemporaryRoot const&) = delete;
  TemporaryRoot& operator=(TemporaryRoot const&) = delete;
  ~TemporaryRoot() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }
  [[nodiscard]] std::filesystem::path const& path() const { return path_; }

 private:
  std::filesystem::path path_;
};

std::wstring makeFederationName() {
  static std::atomic_uint64_t counter{0U};
  auto const tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return L"cpp-tck-service-report-creation-failure-" + std::to_wstring(tick) + L"-" +
      std::to_wstring(++counter);
}

rti::RtiConfiguration configurationForReportDirectory(
    std::filesystem::path const& directory,
    std::string const& configurationName) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withRtiAddress(L"in-process");
  if (!configurationName.empty()) {
    configuration.withConfigurationName(std::wstring(
        configurationName.begin(), configurationName.end()));
  }
  configuration.withAdditionalSettings(L"serviceReportDirectory=" + directory.wstring());
  return configuration;
}

void drainEvokedCallbacks(Participant& participant, rti::CallbackModel callbackModel) {
  if (callbackModel != rti::HLA_EVOKED) {
    return;
  }
  for (std::size_t attempt = 0U; attempt < 64U; ++attempt) {
    if (!participant.rti().evokeMultipleCallbacks(0.0, 0.0)) {
      break;
    }
  }
}

std::vector<std::filesystem::path> filesIn(std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> files;
  if (!std::filesystem::exists(directory)) {
    return files;
  }
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      files.push_back(std::filesystem::absolute(entry.path()).lexically_normal());
    }
  }
  return files;
}

}  // namespace

void runServiceReportConnectFailure(
    ServiceReportCreationFailureConfiguration const& configuration) try {
  if (configuration.applicationFom.empty() || configuration.standardMim.empty() ||
      configuration.serviceReportDirectory.empty() ||
      configuration.logicalTimeImplementation.empty()) {
    throw std::runtime_error(
        "Service-report Connect-failure scenario requires FOM, MIM, time, and directory");
  }

  auto const federation = makeFederationName();
  auto const root = std::filesystem::absolute(
      configuration.serviceReportDirectory / std::filesystem::path(
          std::string(federation.begin(), federation.end()))).lexically_normal();
  std::error_code error;
  std::filesystem::create_directories(root, error);
  if (error) {
    throw std::filesystem::filesystem_error(
        "Unable to create the service-report Connect-failure workspace", root, error);
  }
  TemporaryRoot cleanup(root);

  auto const blocker = root / "not-a-directory";
  auto const impossibleDirectory = blocker / "reports";
  {
    std::ofstream output(blocker, std::ios::binary | std::ios::trunc);
    if (!output) {
      throw std::runtime_error("Unable to create the Connect-failure blocker file");
    }
    output << "not a directory";
    output.close();
    if (!output) {
      throw std::runtime_error("Unable to finish creating the Connect-failure blocker file");
    }
  }

  auto const federateName = L"service-report-connect-retry-federate";
  auto const invalidConfiguration = configurationForReportDirectory(
      impossibleDirectory, configuration.configurationName);
  Participant participant(configuration.callbackModel);
  std::wstring failureName;
  try {
    participant.connect(invalidConfiguration);
  } catch (rti::Exception const& failure) {
    failureName = failure.name();
  } catch (std::exception const& failure) {
    throw std::runtime_error(
        std::string("Connect failed outside the RTI exception boundary: ") + failure.what());
  } catch (...) {
    throw std::runtime_error("Connect failed with a non-standard exception");
  }
  if (failureName != L"RTIinternalError") {
    throw std::runtime_error(
        failureName.empty()
            ? "Connect unexpectedly succeeded with an uncreatable service-report directory"
            : "Connect returned an unexpected RTI exception for service-report directory failure");
  }
  if (std::filesystem::exists(impossibleDirectory) ||
      !std::filesystem::is_regular_file(blocker)) {
    throw std::runtime_error(
        "Failed Connect created a substitute service-report directory or changed its blocker");
  }

  if (!std::filesystem::remove(blocker, error) || error) {
    throw std::filesystem::filesystem_error(
        "Unable to remove the Connect-failure blocker before retry", blocker, error);
  }
  auto const repairedDirectory = root / "repaired-reports";
  auto const repairedConfiguration = configurationForReportDirectory(
      repairedDirectory, configuration.configurationName);
  participant.connect(repairedConfiguration);
  participant.createFederation(
      federation,
      configuration.applicationFom,
      configuration.standardMim,
      configuration.logicalTimeImplementation);
  static_cast<void>(participant.join(federateName, federation));
  auto const reportFiles = filesIn(repairedDirectory);
  if (reportFiles.size() != 1U || std::filesystem::file_size(reportFiles.front()) == 0U) {
    throw std::runtime_error(
        "The same ambassador did not create one report file after Connect was repaired");
  }
  participant.resign();
  participant.destroyFederation(federation);
  participant.disconnect();
} catch (rti::Exception const& failure) {
  auto const name = failure.name();
  auto const message = failure.what();
  throw std::runtime_error(
      "Unexpected RTI exception outside the expected Connect failure: " +
      std::string(name.begin(), name.end()) + ": " +
      std::string(message.begin(), message.end()));
}

void runServiceReportCreationFailure(
    ServiceReportCreationFailureConfiguration const& configuration) try {
  if (configuration.applicationFom.empty() || configuration.standardMim.empty() ||
      configuration.serviceReportDirectory.empty() ||
      configuration.logicalTimeImplementation.empty()) {
    throw std::runtime_error("Service-report failure scenario requires FOM, MIM, time, and directory");
  }

  auto const federation = makeFederationName();
  auto const root = std::filesystem::absolute(
      configuration.serviceReportDirectory / std::filesystem::path(
          std::string(federation.begin(), federation.end()))).lexically_normal();
  std::error_code error;
  std::filesystem::create_directories(root, error);
  if (error) {
    throw std::filesystem::filesystem_error(
        "Unable to create the service-report failure scenario workspace", root, error);
  }
  TemporaryRoot cleanup(root);

  auto const validReportDirectory = root / "valid-reports";
  auto const reportDirectoryToBlock = root / "rejected-reports";
  std::filesystem::create_directories(reportDirectoryToBlock, error);
  if (error) {
    throw std::filesystem::filesystem_error(
        "Unable to prepare the report directory before the rejected join",
        reportDirectoryToBlock,
        error);
  }
  auto const blockingFile = reportDirectoryToBlock;

  auto const hostName = L"service-report-failure-host";
  auto const rejectedName = L"service-report-failure-rejected";
  auto hostConfiguration = configurationForReportDirectory(
      validReportDirectory, configuration.configurationName);
  auto rejectedConfiguration = configurationForReportDirectory(
      reportDirectoryToBlock, configuration.configurationName);
  Participant host(configuration.callbackModel, hostConfiguration);
  Participant rejected(configuration.callbackModel, rejectedConfiguration);
  host.createFederation(
      federation,
      configuration.applicationFom,
      configuration.standardMim,
      configuration.logicalTimeImplementation);
  static_cast<void>(host.join(hostName, federation));

  if (!std::filesystem::remove(reportDirectoryToBlock, error) || error) {
    throw std::filesystem::filesystem_error(
        "Unable to replace the connected participant's report directory",
        reportDirectoryToBlock,
        error);
  }
  {
    std::ofstream blocker(blockingFile, std::ios::binary | std::ios::trunc);
    if (!blocker) {
      throw std::runtime_error("Unable to replace the report directory with a regular file");
    }
    blocker << "not a directory";
    blocker.close();
    if (!blocker) {
      throw std::runtime_error("Unable to finish creating the report-directory blocker file");
    }
  }

  auto const momClass = host.rti().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const nameAttribute = host.rti().getAttributeHandle(momClass, L"HLAfederateName");
  auto const reportFileAttribute = host.rti().getAttributeHandle(
      momClass, L"HLAreportServiceFile");
  if (!momClass.isValid() || !nameAttribute.isValid() || !reportFileAttribute.isValid()) {
    throw std::runtime_error("The installed package omitted Federate MOM report-file metadata");
  }
  host.rti().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{nameAttribute, reportFileAttribute},
      true,
      L"");
  drainEvokedCallbacks(host, configuration.callbackModel);
  auto const discoveryCountBeforeRejectedJoin = host.recorder().countForClass(momClass);
  auto const validFilesBeforeRejectedJoin = filesIn(validReportDirectory);
  if (validFilesBeforeRejectedJoin.size() != 1U ||
      std::filesystem::file_size(validFilesBeforeRejectedJoin.front()) == 0U) {
    throw std::runtime_error(
        "The successful host join did not create exactly one initial report file");
  }

  std::wstring failureName;
  try {
    static_cast<void>(rejected.join(rejectedName, federation));
  } catch (rti::Exception const& failure) {
    failureName = failure.name();
  } catch (std::exception const& failure) {
    throw std::runtime_error(
        std::string("Join failed outside the RTI exception boundary: ") + failure.what());
  } catch (...) {
    throw std::runtime_error("Join failed with a non-standard exception");
  }
  if (failureName != L"RTIinternalError") {
    throw std::runtime_error(
        failureName.empty()
            ? "Join unexpectedly succeeded despite an uncreatable service-report directory"
            : "Join returned an unexpected RTI exception type for service-report creation failure");
  }
  drainEvokedCallbacks(host, configuration.callbackModel);

  if (host.recorder().countForClass(momClass) != discoveryCountBeforeRejectedJoin) {
    throw std::runtime_error(
        "A failed join published a joined-federate Federate MOM object");
  }
  if (filesIn(validReportDirectory) != validFilesBeforeRejectedJoin ||
      !std::filesystem::is_regular_file(blockingFile)) {
    throw std::runtime_error(
        "A failed join created a report file, altered a valid report set, or bypassed the blocker");
  }

  if (!std::filesystem::remove(blockingFile, error) || error) {
    throw std::filesystem::filesystem_error(
        "Unable to remove the report-directory blocker before retry",
        blockingFile,
        error);
  }
  std::filesystem::create_directories(reportDirectoryToBlock, error);
  if (error) {
    throw std::filesystem::filesystem_error(
        "Unable to restore the configured report directory before retry",
        reportDirectoryToBlock,
        error);
  }
  static_cast<void>(rejected.join(rejectedName, federation));
  drainEvokedCallbacks(host, configuration.callbackModel);
  auto const retryFiles = filesIn(reportDirectoryToBlock);
  if (retryFiles.size() != 1U || std::filesystem::file_size(retryFiles.front()) == 0U ||
      host.recorder().countForClass(momClass) != discoveryCountBeforeRejectedJoin + 1U) {
    throw std::runtime_error(
        "A corrected report directory could not retry Join with one public MOM/file identity");
  }

  rejected.resign();
  host.resign();
  host.destroyFederation(federation);
  rejected.disconnect();
  host.disconnect();
} catch (rti::Exception const& failure) {
  auto const name = failure.name();
  auto const message = failure.what();
  throw std::runtime_error(
      "Unexpected RTI exception outside the rejected join: " +
      std::string(name.begin(), name.end()) + ": " +
      std::string(message.begin(), message.end()));
}

}  // namespace hla_rti_cpp_tck
