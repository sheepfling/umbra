#include "pending_negotiated_restore_tck.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck {
namespace {

namespace rti = rti1516_2025;

struct OwnershipCallback final {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
  std::vector<std::uint8_t> tag;
};

struct AttributeReflection final {
  rti::ObjectInstanceHandle object;
  std::vector<std::pair<rti::AttributeHandle, std::vector<std::uint8_t>>> values;
};

struct PublicMomReport final {
  rti::ObjectInstanceHandle object;
  std::filesystem::path path;
};

class CallbackRecorder final : public rti::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::ObjectClassHandle const&,
      std::wstring const&,
      rti::FederateHandle const&) override {
    std::scoped_lock lock(mutex_);
    discoveredObjects_.push_back(object);
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const&,
      rti::TransportationTypeHandle const&,
      rti::FederateHandle const&,
      rti::RegionHandleSet const*) override {
    std::scoped_lock lock(mutex_);
    AttributeReflection reflection{object, {}};
    reflection.values.reserve(values.size());
    for (auto const& [attribute, value] : values) {
      auto const* bytes = static_cast<std::uint8_t const*>(value.data());
      reflection.values.emplace_back(
          attribute,
          bytes == nullptr
              ? std::vector<std::uint8_t>{}
              : std::vector<std::uint8_t>{bytes, bytes + value.size()});
    }
    reflections_.push_back(std::move(reflection));
  }

  void initiateFederateSave(std::wstring const&) override {
    std::scoped_lock lock(mutex_);
    saveInitiated_ = true;
  }

  void federationSaved() override {
    std::scoped_lock lock(mutex_);
    saved_ = true;
  }

  void federationNotSaved(rti::SaveFailureReason) override {
    std::scoped_lock lock(mutex_);
    saveFailed_ = true;
  }

  void requestFederationRestoreSucceeded(std::wstring const&) override {
    std::scoped_lock lock(mutex_);
    restoreRequested_ = true;
  }

  void requestFederationRestoreFailed(std::wstring const&) override {
    std::scoped_lock lock(mutex_);
    restoreFailed_ = true;
  }

  void federationRestoreBegun() override {
    std::scoped_lock lock(mutex_);
    restoreBegun_ = true;
  }

  void initiateFederateRestore(
      std::wstring const&,
      std::wstring const&,
      rti::FederateHandle const&) override {
    std::scoped_lock lock(mutex_);
    restoreInitiated_ = true;
  }

  void federationRestored() override {
    std::scoped_lock lock(mutex_);
    restored_ = true;
  }

  void federationNotRestored(rti::RestoreFailureReason) override {
    std::scoped_lock lock(mutex_);
    restoreFailed_ = true;
  }

  void requestDivestitureConfirmation(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::scoped_lock lock(mutex_);
    divestitureConfirmation_ = OwnershipCallback{
        object, attributes, copyTag(tag)};
  }

  void attributeOwnershipAcquisitionNotification(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::scoped_lock lock(mutex_);
    acquisitionNotification_ = OwnershipCallback{
        object, attributes, copyTag(tag)};
  }

  [[nodiscard]] bool saveInitiated() const { return flag(&CallbackRecorder::saveInitiated_); }
  [[nodiscard]] bool saved() const { return flag(&CallbackRecorder::saved_); }
  [[nodiscard]] bool saveFailed() const { return flag(&CallbackRecorder::saveFailed_); }
  [[nodiscard]] bool restoreRequested() const { return flag(&CallbackRecorder::restoreRequested_); }
  [[nodiscard]] bool restoreBegun() const { return flag(&CallbackRecorder::restoreBegun_); }
  [[nodiscard]] bool restoreInitiated() const { return flag(&CallbackRecorder::restoreInitiated_); }
  [[nodiscard]] bool restored() const { return flag(&CallbackRecorder::restored_); }
  [[nodiscard]] bool restoreFailed() const { return flag(&CallbackRecorder::restoreFailed_); }

  [[nodiscard]] std::optional<OwnershipCallback> divestitureConfirmation() const {
    std::scoped_lock lock(mutex_);
    return divestitureConfirmation_;
  }

  [[nodiscard]] std::optional<OwnershipCallback> acquisitionNotification() const {
    std::scoped_lock lock(mutex_);
    return acquisitionNotification_;
  }

  [[nodiscard]] std::vector<AttributeReflection> reflections() const {
    std::scoped_lock lock(mutex_);
    return reflections_;
  }

  [[nodiscard]] bool discovered(rti::ObjectInstanceHandle const& object) const {
    std::scoped_lock lock(mutex_);
    return std::find(discoveredObjects_.begin(), discoveredObjects_.end(), object) !=
        discoveredObjects_.end();
  }

  void clearReflections() {
    std::scoped_lock lock(mutex_);
    reflections_.clear();
  }

 private:
  static std::vector<std::uint8_t> copyTag(rti::VariableLengthData const& tag) {
    auto const* bytes = static_cast<std::uint8_t const*>(tag.data());
    return bytes == nullptr ? std::vector<std::uint8_t>{}
                            : std::vector<std::uint8_t>{bytes, bytes + tag.size()};
  }

  bool flag(bool CallbackRecorder::*member) const {
    std::scoped_lock lock(mutex_);
    return this->*member;
  }

  mutable std::mutex mutex_;
  bool saveInitiated_ = false;
  bool saved_ = false;
  bool saveFailed_ = false;
  bool restoreRequested_ = false;
  bool restoreBegun_ = false;
  bool restoreInitiated_ = false;
  bool restored_ = false;
  bool restoreFailed_ = false;
  std::optional<OwnershipCallback> divestitureConfirmation_;
  std::optional<OwnershipCallback> acquisitionNotification_;
  std::vector<rti::ObjectInstanceHandle> discoveredObjects_;
  std::vector<AttributeReflection> reflections_;
};

class Participant final {
 public:
  Participant(rti::CallbackModel model, rti::RtiConfiguration const& configuration)
      : ambassador_(factory_.createRTIambassador()) {
    if (!ambassador_) {
      throw std::runtime_error("RTIambassadorFactory returned no ambassador");
    }
    static_cast<void>(ambassador_->connect(recorder_, model, configuration));
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

  rti::FederateHandle join(
      std::wstring const& federateName,
      std::wstring const& federateType,
      std::wstring const& federationName) {
    auto const handle = ambassador_->joinFederationExecution(
        federateName, federateType, federationName);
    if (!handle.isValid()) {
      throw std::runtime_error("Join Federation Execution returned an invalid handle");
    }
    joined_ = true;
    return handle;
  }

  void resign(rti::ResignAction action) {
    if (joined_) {
      ambassador_->resignFederationExecution(action);
      joined_ = false;
    }
  }

  void disconnect() {
    if (connected_) {
      ambassador_->disconnect();
      connected_ = false;
    }
  }

  [[nodiscard]] rti::RTIambassador& rti() { return *ambassador_; }
  [[nodiscard]] CallbackRecorder& callbacks() { return recorder_; }

 private:
  rti::RTIambassadorFactory factory_;
  std::unique_ptr<rti::RTIambassador> ambassador_;
  CallbackRecorder recorder_;
  bool connected_ = false;
  bool joined_ = false;
};

std::vector<std::filesystem::path> reportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> result;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    result.push_back(std::filesystem::absolute(entry.path()).lexically_normal());
  }
  std::sort(result.begin(), result.end());
  return result;
}

std::string initialReportCallbackModel(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error(
        "The public HLAreportServiceFile path could not be opened");
  }
  std::string const contents(
      std::istreambuf_iterator<char>{input},
      std::istreambuf_iterator<char>{});
  auto const fieldName = std::string{"\"CallbackModel\":"};
  auto const fieldPosition = contents.find(fieldName);
  if (fieldPosition == std::string::npos) {
    throw std::runtime_error(
        "The initial service-report record omitted CallbackModel");
  }
  auto const valueStart = contents.find('"', fieldPosition + fieldName.size());
  if (valueStart == std::string::npos) {
    throw std::runtime_error(
        "The initial service-report CallbackModel was not a string value");
  }
  auto const valueEnd = contents.find('"', valueStart + 1U);
  if (valueEnd == std::string::npos) {
    throw std::runtime_error(
        "The initial service-report CallbackModel string was incomplete");
  }
  return contents.substr(valueStart + 1U, valueEnd - valueStart - 1U);
}

std::vector<std::uint8_t> copyBytes(rti::VariableLengthData const& value) {
  auto const* bytes = static_cast<std::uint8_t const*>(value.data());
  return bytes == nullptr ? std::vector<std::uint8_t>{}
                          : std::vector<std::uint8_t>{bytes, bytes + value.size()};
}

std::vector<std::uint8_t> encodeUnicode(std::wstring const& value) {
  return copyBytes(rti::HLAunicodeString{value}.encode());
}

std::optional<PublicMomReport> reflectedMomReport(
    CallbackRecorder const& recorder,
    rti::AttributeHandle const& nameAttribute,
    rti::AttributeHandle const& reportFileAttribute,
    std::wstring const& federateName,
    std::vector<std::filesystem::path> const& expectedFiles) {
  auto const expectedName = encodeUnicode(federateName);
  auto const reflections = recorder.reflections();
  for (auto reflection = reflections.rbegin(); reflection != reflections.rend(); ++reflection) {
    auto const name = std::find_if(
        reflection->values.begin(), reflection->values.end(),
        [&](auto const& value) { return value.first == nameAttribute; });
    auto const reportFile = std::find_if(
        reflection->values.begin(), reflection->values.end(),
        [&](auto const& value) { return value.first == reportFileAttribute; });
    if (name == reflection->values.end() || reportFile == reflection->values.end() ||
        name->second != expectedName) {
      continue;
    }
    auto const& advertisedPath = reportFile->second;
    for (auto const& expectedFile : expectedFiles) {
      if (advertisedPath == encodeUnicode(expectedFile.wstring())) {
        return PublicMomReport{reflection->object, expectedFile};
      }
    }
    throw std::runtime_error(
        "Public Federate MOM HLAreportServiceFile did not match a joined-federate file");
  }
  return std::nullopt;
}

bool reflectedMomReportMatches(
    CallbackRecorder const& recorder,
    rti::ObjectInstanceHandle const& object,
    rti::AttributeHandle const& reportFileAttribute,
    std::filesystem::path const& expectedFile) {
  auto const expectedPath = encodeUnicode(expectedFile.wstring());
  for (auto const& reflection : recorder.reflections()) {
    auto const reportFile = std::find_if(
        reflection.values.begin(), reflection.values.end(),
        [&](auto const& value) { return value.first == reportFileAttribute; });
    if (reflection.object == object && reportFile != reflection.values.end() &&
        reportFile->second == expectedPath) {
      return true;
    }
  }
  return false;
}

template <typename Predicate>
void waitFor(
    Participant& first,
    Participant& second,
    rti::CallbackModel model,
    std::chrono::milliseconds timeout,
    Predicate predicate,
    char const* description) {
  auto const deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(first.rti().evokeMultipleCallbacks(0.0, 0.0));
      static_cast<void>(second.rti().evokeMultipleCallbacks(0.0, 0.0));
    }
    if (first.callbacks().saveFailed() || second.callbacks().saveFailed() ||
        first.callbacks().restoreFailed() || second.callbacks().restoreFailed()) {
      throw std::runtime_error(std::string(description) + " received a failure callback");
    }
    if (predicate()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error(std::string("Timed out waiting for ") + description);
}

std::wstring makeFederationName() {
  static std::atomic_uint64_t counter{0U};
  auto const tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return L"cpp-tck-pending-negotiated-restore-" + std::to_wstring(tick) + L"-" +
      std::to_wstring(++counter);
}

}  // namespace

void runPendingNegotiatedRestore(
    PendingNegotiatedRestoreConfiguration const& configuration) {
  if (configuration.applicationFom.empty() || configuration.standardMim.empty() ||
      configuration.logicalTimeImplementation.empty() ||
      configuration.serviceReportDirectory.empty()) {
    throw std::runtime_error(
        "Pending negotiated restore requires adapter FOM/MIM, time, and report directory");
  }
  std::error_code error;
  std::filesystem::create_directories(configuration.serviceReportDirectory, error);
  if (error) {
    throw std::filesystem::filesystem_error(
        "Unable to create the package-TCK service-report directory",
        configuration.serviceReportDirectory,
        error);
  }

  auto const federation = makeFederationName();
  auto const reportDirectory = configuration.serviceReportDirectory /
      std::filesystem::path(std::string(federation.begin(), federation.end()));
  std::filesystem::create_directories(reportDirectory, error);
  if (error) {
    throw std::filesystem::filesystem_error(
        "Unable to create the isolated package-TCK service-report directory",
        reportDirectory,
        error);
  }
  auto const ownerName = L"cpp-tck-negotiated-restore-owner";
  auto const requesterName = L"cpp-tck-negotiated-restore-requester";
  auto const federateType = L"cpp-tck";
  auto const saveLabel = L"cpp-tck-pending-negotiated-ownership-save";
  std::vector<std::uint8_t> const acquisitionTagBytes{0x54U, 0x43U, 0x4BU, 0x41U};
  std::vector<std::uint8_t> const divestitureTagBytes{0x54U, 0x43U, 0x4BU, 0x44U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());

  auto configurationValue = rti::RtiConfiguration::createConfiguration();
  if (!configuration.configurationName.empty()) {
    configurationValue.withConfigurationName(std::wstring(
        configuration.configurationName.begin(),
        configuration.configurationName.end()));
  }
  if (!configuration.rtiAddress.empty()) {
    configurationValue.withRtiAddress(
        std::wstring(configuration.rtiAddress.begin(), configuration.rtiAddress.end()));
  } else {
    configurationValue.withRtiAddress(L"in-process");
  }
  configurationValue.withAdditionalSettings(
      L"serviceReportDirectory=" + reportDirectory.wstring());

  Participant owner(configuration.callbackModel, configurationValue);
  Participant requester(configuration.callbackModel, configurationValue);
  owner.rti().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{configuration.applicationFom.wstring()},
      configuration.standardMim.wstring(),
      configuration.logicalTimeImplementation);
  static_cast<void>(owner.join(ownerName, federateType, federation));
  static_cast<void>(requester.join(requesterName, federateType, federation));

  auto const momClass = owner.rti().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const momNameAttribute = owner.rti().getAttributeHandle(
      momClass, L"HLAfederateName");
  auto const momReportFileAttribute = owner.rti().getAttributeHandle(
      momClass, L"HLAreportServiceFile");
  if (!momClass.isValid() || !momNameAttribute.isValid() ||
      !momReportFileAttribute.isValid()) {
    throw std::runtime_error("The installed package omitted Federate MOM report-file metadata");
  }
  rti::AttributeHandleSet const momAttributes{momNameAttribute, momReportFileAttribute};
  owner.rti().subscribeObjectClassAttributes(momClass, momAttributes, true, L"");
  requester.rti().subscribeObjectClassAttributes(momClass, momAttributes, true, L"");

  auto const objectClass = owner.rti().getObjectClassHandle(
      configuration.objectClassName);
  auto const attribute = owner.rti().getAttributeHandle(
      objectClass, configuration.attributeName);
  if (!objectClass.isValid() || !attribute.isValid()) {
    throw std::runtime_error("Adapter FOM omitted the configured ownership test surface");
  }
  rti::AttributeHandleSet const attributes{attribute};
  owner.rti().publishObjectClassAttributes(objectClass, attributes);
  requester.rti().subscribeObjectClassAttributes(objectClass, attributes);
  requester.rti().publishObjectClassAttributes(objectClass, attributes);
  auto const object = owner.rti().registerObjectInstance(objectClass);
  if (!object.isValid()) {
    throw std::runtime_error("Register Object Instance returned an invalid handle");
  }
  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] { return requester.callbacks().discovered(object); },
      "the requester's public discovery of the ownership test object");

  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] { return !reportFiles(reportDirectory).empty(); },
      "the joined federate's configured service-report file");
  auto const filesAtJoin = reportFiles(reportDirectory);
  for (auto const& path : filesAtJoin) {
    if (std::filesystem::file_size(path) == 0U) {
      throw std::runtime_error("A joined federate's initial service-report file is empty");
    }
  }
  if (filesAtJoin.size() != 2U) {
    throw std::runtime_error(
        "The two joined federates did not receive exactly two service-report files");
  }
  std::optional<PublicMomReport> ownerObservedRequester;
  std::optional<PublicMomReport> requesterObservedOwner;
  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] {
        ownerObservedRequester = reflectedMomReport(
            owner.callbacks(),
            momNameAttribute,
            momReportFileAttribute,
            requesterName,
            filesAtJoin);
        requesterObservedOwner = reflectedMomReport(
            requester.callbacks(),
            momNameAttribute,
            momReportFileAttribute,
            ownerName,
            filesAtJoin);
        return ownerObservedRequester.has_value() && requesterObservedOwner.has_value();
      },
      "each joined federate's public HLAreportServiceFile MOM value");
  if (ownerObservedRequester->path == requesterObservedOwner->path ||
      std::find(filesAtJoin.begin(), filesAtJoin.end(), ownerObservedRequester->path) ==
          filesAtJoin.end() ||
      std::find(filesAtJoin.begin(), filesAtJoin.end(), requesterObservedOwner->path) ==
          filesAtJoin.end()) {
    throw std::runtime_error(
        "Public Federate MOM HLAreportServiceFile values did not identify distinct retained files");
  }
  auto const expectedCallbackModel = configuration.callbackModel == rti::HLA_EVOKED
      ? "HLA_EVOKED"
      : "HLA_IMMEDIATE";
  if (initialReportCallbackModel(ownerObservedRequester->path) != expectedCallbackModel ||
      initialReportCallbackModel(requesterObservedOwner->path) != expectedCallbackModel) {
    throw std::runtime_error(
        "A joined federate's initial service-report record encoded the wrong CallbackModel");
  }

  if (configuration.callbackModel == rti::HLA_IMMEDIATE) {
    owner.rti().disableCallbacks();
    requester.rti().disableCallbacks();
  }
  requester.rti().attributeOwnershipAcquisitionIfAvailable(
      object, attributes, acquisitionTag);
  owner.rti().negotiatedAttributeOwnershipDivestiture(
      object, attributes, divestitureTag);
  if (owner.callbacks().divestitureConfirmation().has_value() ||
      requester.callbacks().acquisitionNotification().has_value()) {
    throw std::runtime_error(
        "Ownership work was delivered before the federation save boundary");
  }

  owner.rti().requestFederationSave(saveLabel);
  if (configuration.callbackModel == rti::HLA_EVOKED) {
    waitFor(
        owner,
        requester,
        configuration.callbackModel,
        configuration.timeout,
        [&] {
          return owner.callbacks().saveInitiated() &&
              requester.callbacks().saveInitiated();
        },
        "both federate save initiation callbacks");
  }
  owner.rti().federateSaveBegun();
  requester.rti().federateSaveBegun();
  owner.rti().federateSaveComplete();
  requester.rti().federateSaveComplete();
  if (configuration.callbackModel == rti::HLA_EVOKED) {
    waitFor(
        owner,
        requester,
        configuration.callbackModel,
        configuration.timeout,
        [&] { return owner.callbacks().saved() && requester.callbacks().saved(); },
        "both federation saved callbacks");
  }

  owner.rti().requestFederationRestore(saveLabel);
  if (configuration.callbackModel == rti::HLA_EVOKED) {
    waitFor(
        owner,
        requester,
        configuration.callbackModel,
        configuration.timeout,
        [&] {
          return owner.callbacks().restoreRequested() &&
              owner.callbacks().restoreBegun() &&
              requester.callbacks().restoreBegun() &&
              owner.callbacks().restoreInitiated() &&
              requester.callbacks().restoreInitiated();
        },
        "federation restore initiation callbacks");
  }
  owner.rti().federateRestoreComplete();
  requester.rti().federateRestoreComplete();
  if (configuration.callbackModel == rti::HLA_IMMEDIATE) {
    owner.rti().enableCallbacks();
    requester.rti().enableCallbacks();
  }
  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] { return owner.callbacks().restored() && requester.callbacks().restored(); },
      "both federation restored callbacks");

  owner.callbacks().clearReflections();
  requester.callbacks().clearReflections();
  rti::AttributeHandleSet const reportFileOnly{momReportFileAttribute};
  rti::VariableLengthData const noUserTag;
  owner.rti().requestAttributeValueUpdate(
      ownerObservedRequester->object, reportFileOnly, noUserTag);
  requester.rti().requestAttributeValueUpdate(
      requesterObservedOwner->object, reportFileOnly, noUserTag);
  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] {
        auto const ownerObservedRequesterAgain = reflectedMomReportMatches(
            owner.callbacks(),
            ownerObservedRequester->object,
            momReportFileAttribute,
            ownerObservedRequester->path);
        auto const requesterObservedOwnerAgain = reflectedMomReportMatches(
            requester.callbacks(),
            requesterObservedOwner->object,
            momReportFileAttribute,
            requesterObservedOwner->path);
        return ownerObservedRequesterAgain && requesterObservedOwnerAgain;
      },
      "restored public HLAreportServiceFile values after attribute-value update");

  auto verifyOwnerMomPath = [&](char const* description) {
    requester.callbacks().clearReflections();
    requester.rti().requestAttributeValueUpdate(
        requesterObservedOwner->object, reportFileOnly, noUserTag);
    waitFor(
        owner,
        requester,
        configuration.callbackModel,
        configuration.timeout,
        [&] {
          return reflectedMomReportMatches(
              requester.callbacks(),
              requesterObservedOwner->object,
              momReportFileAttribute,
              requesterObservedOwner->path);
        },
        description);
  };

  auto const ownerReportFile = requesterObservedOwner->path;
  owner.rti().setServiceReportingSwitch(true);
  owner.rti().setSendServiceReportsToFileSwitch(true);
  auto const beforeEnabledAppend = std::filesystem::file_size(ownerReportFile);
  static_cast<void>(owner.rti().getObjectClassHandle(configuration.objectClassName));
  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] { return std::filesystem::file_size(ownerReportFile) > beforeEnabledAppend; },
      "an enabled service invocation appended to the advertised report file");
  verifyOwnerMomPath("the report-file path remained advertised while file reporting was enabled");

  owner.rti().setSendServiceReportsToFileSwitch(false);
  auto const whileDisabled = std::filesystem::file_size(ownerReportFile);
  static_cast<void>(owner.rti().getObjectClassHandle(configuration.objectClassName));
  if (std::filesystem::file_size(ownerReportFile) != whileDisabled) {
    throw std::runtime_error(
        "A service invocation appended to the report file while file reporting was disabled");
  }
  verifyOwnerMomPath("the static report-file path changed while file reporting was disabled");

  owner.rti().setSendServiceReportsToFileSwitch(true);
  auto const beforeReenabledAppend = std::filesystem::file_size(ownerReportFile);
  static_cast<void>(owner.rti().getObjectClassHandle(configuration.objectClassName));
  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] { return std::filesystem::file_size(ownerReportFile) > beforeReenabledAppend; },
      "file reporting resumed appends to the same advertised report file");
  verifyOwnerMomPath("the static report-file path changed after file reporting was re-enabled");
  if (reportFiles(reportDirectory) != filesAtJoin) {
    throw std::runtime_error(
        "The service-report switch cycle replaced or added a joined-federate file");
  }

  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] { return owner.callbacks().divestitureConfirmation().has_value(); },
      "restored negotiated divestiture confirmation");
  auto const confirmation = owner.callbacks().divestitureConfirmation();
  if (!confirmation || confirmation->object != object ||
      confirmation->attributes != attributes ||
      confirmation->tag != acquisitionTagBytes) {
    throw std::runtime_error(
        "Restored divestiture confirmation returned different ownership metadata");
  }
  owner.rti().confirmDivestiture(object, attributes, divestitureTag);
  waitFor(
      owner,
      requester,
      configuration.callbackModel,
      configuration.timeout,
      [&] { return requester.callbacks().acquisitionNotification().has_value(); },
      "If Available acquisition notification following restored confirmation");
  auto const acquisition = requester.callbacks().acquisitionNotification();
  if (!acquisition || acquisition->object != object ||
      acquisition->attributes != attributes ||
      acquisition->tag != divestitureTagBytes) {
    throw std::runtime_error(
        "Restored If Available acquisition returned different ownership metadata");
  }
  if (owner.rti().isAttributeOwnedByFederate(object, attribute) ||
      !requester.rti().isAttributeOwnedByFederate(object, attribute)) {
    throw std::runtime_error(
        "Confirmed restored divestiture did not transfer ownership to the requester");
  }

  if (reportFiles(reportDirectory) != filesAtJoin) {
    throw std::runtime_error(
        "Federation restore replaced or added a joined federate's service-report file");
  }
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  requester.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rti().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

}  // namespace hla_rti_cpp_tck
