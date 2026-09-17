#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_save_commit_store.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The public application-value restore test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;
namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class ScopedTemporaryDirectory final {
 public:
  explicit ScopedTemporaryDirectory(std::filesystem::path path)
      : path_(std::move(path)) {}
  ScopedTemporaryDirectory(ScopedTemporaryDirectory const&) = delete;
  ScopedTemporaryDirectory& operator=(ScopedTemporaryDirectory const&) = delete;
  ~ScopedTemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept { return path_; }

 private:
  std::filesystem::path path_;
};

ScopedTemporaryDirectory reserveDirectory(std::string const& prefix) {
  static std::atomic_uint64_t sequence{0U};
  auto const parent = std::filesystem::temp_directory_path();
  auto const ticks = static_cast<std::uint64_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  for (std::size_t attempt = 0U; attempt != 256U; ++attempt) {
    auto const candidate = parent / (prefix + "-" + std::to_string(ticks) + "-" +
                                     std::to_string(sequence.fetch_add(1U) + attempt));
    std::error_code error;
    if (std::filesystem::create_directory(candidate, error)) {
      return ScopedTemporaryDirectory(candidate);
    }
    if (error && error != std::errc::file_exists) {
      throw std::system_error(error, "Unable to reserve a temporary Umbra directory");
    }
  }
  throw std::runtime_error("Unable to reserve a unique temporary Umbra directory.");
}

class ScopedEmbeddedFederationRegistry final {
 public:
  explicit ScopedEmbeddedFederationRegistry(
      std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> replacement)
      : previous_(
            rti1516_2025::umbra_binding_detail::replaceEmbeddedFederationRegistryForTesting(
                std::move(replacement))) {}
  ScopedEmbeddedFederationRegistry(ScopedEmbeddedFederationRegistry const&) = delete;
  ScopedEmbeddedFederationRegistry& operator=(ScopedEmbeddedFederationRegistry const&) = delete;
  ~ScopedEmbeddedFederationRegistry() noexcept {
    if (previous_) {
      static_cast<void>(
          rti1516_2025::umbra_binding_detail::replaceEmbeddedFederationRegistryForTesting(
              std::move(previous_)));
    }
  }

 private:
  std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> previous_;
};

class ApplicationValueObserver final : public rti1516_2025::NullFederateAmbassador {
 public:
  void federationSaved() override { ++federationSavedCount; }
  void federationRestored() override { ++federationRestoredCount; }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveredObjects.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    reflectedObject = objectInstance;
    reflectedValues = attributeValues;
  }

  std::size_t federationSavedCount = 0U;
  std::size_t federationRestoredCount = 0U;
  std::vector<ObjectInstanceHandle> discoveredObjects;
  ObjectInstanceHandle reflectedObject;
  AttributeHandleValueMap reflectedValues;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drain(RTIambassador& ambassador) {
  for (std::size_t pass = 0U; pass != 64U; ++pass) {
    if (!ambassador.evokeCallback(0.0)) {
      return;
    }
  }
}

void drainBoth(RTIambassador& first, RTIambassador& second) {
  for (std::size_t pass = 0U; pass != 64U; ++pass) {
    static_cast<void>(first.evokeCallback(0.0));
    static_cast<void>(second.evokeCallback(0.0));
  }
}

std::vector<std::filesystem::path> reportFiles(std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> result;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      result.push_back(std::filesystem::absolute(entry.path()).lexically_normal());
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

std::string readFile(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<unsigned char> bytes(VariableLengthData const& value) {
  auto const* data = static_cast<unsigned char const*>(value.data());
  if (data == nullptr) {
    return {};
  }
  return {data, data + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"umbra-public-application-value-focused-" +
      std::to_wstring(sequence.fetch_add(1U));
}

}  // namespace

TEST_CASE(
    "Embedded public fresh-registry restore rehydrates application value and retains report files",
    "[integration][development-profile][federation-management][object-management]"
    "[save-restore][durable-save][filesystem][process-restart][restore]"
    "[application-value-state][public-process-restart-application-value]"
    "[public-process-restart-application-value-focused]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  auto const saveDirectory = reserveDirectory("umbra-focused-application-save");
  auto const reportDirectory = reserveDirectory("umbra-focused-application-report");
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto configuration = umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{reportDirectory.path()});
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  std::wstring const saveLabel = L"application-value-focused-save";
  std::vector<unsigned char> const savedValue{0x61U, 0x70U, 0x70U, 0x01U};
  std::vector<unsigned char> const postRestoreValue{0x61U, 0x70U, 0x70U, 0x02U};

  FederateHandle ownerHandle;
  FederateHandle receiverHandle;
  ObjectInstanceHandle objectInstance;
  ObjectClassHandle objectClass;
  AttributeHandle attribute;
  std::vector<std::filesystem::path> sourceReportFiles;

  {
    auto const registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ApplicationValueObserver ownerObserver;
    ApplicationValueObserver receiverObserver;
    auto owner = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerObserver, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(receiverObserver, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"application-value-focused-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(receiverHandle = receiver->joinFederationExecution(
        L"application-value-focused-receiver", L"subscriber", federationName));
    REQUIRE(ownerHandle.isValid());
    REQUIRE(receiverHandle.isValid());

    sourceReportFiles = reportFiles(reportDirectory.path());
    REQUIRE(sourceReportFiles.size() == 2U);
    for (auto const& file : sourceReportFiles) {
      REQUIRE_FALSE(readFile(file).empty());
    }

    REQUIRE_NOTHROW(objectClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child));
    REQUIRE_NOTHROW(attribute = owner->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child));
    AttributeHandleSet const attributes{attribute};
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
    REQUIRE(objectInstance.isValid());
    drain(*receiver);
    REQUIRE(receiverObserver.discoveredObjects == std::vector<ObjectInstanceHandle>{objectInstance});

    AttributeHandleValueMap values;
    values.emplace(attribute, VariableLengthData(savedValue.data(), savedValue.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(objectInstance, values, VariableLengthData{}));
    drain(*receiver);
    REQUIRE(receiverObserver.reflectedObject == objectInstance);
    REQUIRE(bytes(receiverObserver.reflectedValues.at(attribute)) == savedValue);

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    drainBoth(*owner, *receiver);
    REQUIRE(ownerObserver.federationSavedCount == 0U);
    REQUIRE(receiverObserver.federationSavedCount == 0U);
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drainBoth(*owner, *receiver);
    REQUIRE(ownerObserver.federationSavedCount == 1U);
    REQUIRE(receiverObserver.federationSavedCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(image.objects.size() == 1U);
    REQUIRE(image.objects.front().attributeValuesPresent);
    REQUIRE(image.objects.front().attributeValues.size() == 1U);
    REQUIRE(image.objects.front().attributeValues.front().value ==
            std::string(savedValue.begin(), savedValue.end()));

    REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ApplicationValueObserver ownerObserver;
    ApplicationValueObserver receiverObserver;
    auto owner = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerObserver, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(receiverObserver, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshReceiverHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"application-value-focused-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(freshReceiverHandle = receiver->joinFederationExecution(
        L"application-value-focused-receiver", L"subscriber", federationName));
    REQUIRE(freshOwnerHandle == ownerHandle);
    REQUIRE(freshReceiverHandle == receiverHandle);

    auto const freshReportFiles = reportFiles(reportDirectory.path());
    REQUIRE(freshReportFiles.size() == 4U);
    REQUIRE(std::includes(
        freshReportFiles.begin(), freshReportFiles.end(),
        sourceReportFiles.begin(), sourceReportFiles.end()));
    std::vector<std::filesystem::path> freshOnly;
    std::set_difference(
        freshReportFiles.begin(), freshReportFiles.end(),
        sourceReportFiles.begin(), sourceReportFiles.end(),
        std::back_inserter(freshOnly));
    REQUIRE(freshOnly.size() == 2U);
    std::vector<std::size_t> initialSizes;
    for (auto const& file : freshOnly) {
      initialSizes.push_back(readFile(file).size());
    }

    objectClass = owner->getObjectClassHandle(fixture_hla::fom::attribute_fixture_child);
    attribute = owner->getAttributeHandle(objectClass, fixture_hla::fixture::reliable_child);
    AttributeHandleSet const attributes{attribute};
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainBoth(*owner, *receiver);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    drainBoth(*owner, *receiver);
    REQUIRE(ownerObserver.federationRestoredCount == 1U);
    REQUIRE(receiverObserver.federationRestoredCount == 1U);
    REQUIRE(reportFiles(reportDirectory.path()) == freshReportFiles);

    auto const afterRestoreLabel = saveLabel + L"-after-restore";
    REQUIRE_NOTHROW(owner->requestFederationSave(afterRestoreLabel));
    drainBoth(*owner, *receiver);
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drainBoth(*owner, *receiver);
    auto const afterRestore = saveStore->load(federationName, afterRestoreLabel);
    REQUIRE(afterRestore.has_value());
    auto const restoredImage = umbra::detail::FederationStateImageCodec::decode(
        afterRestore->stateImage);
    REQUIRE(restoredImage.objects.size() == 1U);
    REQUIRE(restoredImage.objects.front().attributeValuesPresent);
    REQUIRE(restoredImage.objects.front().attributeValues.front().value ==
            std::string(savedValue.begin(), savedValue.end()));

    receiverObserver.reflectedValues.clear();
    AttributeHandleValueMap nextValues;
    nextValues.emplace(
        attribute,
        VariableLengthData(postRestoreValue.data(), postRestoreValue.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(objectInstance, nextValues, VariableLengthData{}));
    drain(*receiver);
    REQUIRE(bytes(receiverObserver.reflectedValues.at(attribute)) == postRestoreValue);
    REQUIRE(reportFiles(reportDirectory.path()) == freshReportFiles);
    for (std::size_t index = 0U; index != freshOnly.size(); ++index) {
      REQUIRE(readFile(freshOnly[index]).size() >= initialSizes[index]);
    }

    REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
}
