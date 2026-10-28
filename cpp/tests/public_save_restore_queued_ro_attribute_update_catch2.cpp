#include <catch2/catch_test_macros.hpp>
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/VariableLengthData.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#endif

namespace {

using namespace std::chrono_literals;
namespace rti = rti1516_2025;

class SaveRestoreRecorder final : public rti::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::ObjectClassHandle const&,
      std::wstring const&,
      rti::FederateHandle const&) override {
    std::scoped_lock lock(mutex_);
    discoveredObjects.push_back(object);
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const&,
      rti::FederateHandle const& producingFederate,
      rti::RegionHandleSet const* regions) override {
    std::scoped_lock lock(mutex_);
    Reflection reflection{object, {}, {}, producingFederate, regions != nullptr};
    for (auto const& [attribute, value] : values) {
      auto const* bytes = static_cast<std::uint8_t const*>(value.data());
      reflection.values.emplace_back(
          attribute,
          bytes == nullptr
              ? std::vector<std::uint8_t>{}
              : std::vector<std::uint8_t>{bytes, bytes + value.size()});
    }
    auto const* tagBytes = static_cast<std::uint8_t const*>(tag.data());
    if (tagBytes != nullptr && tag.size() != 0U) {
      reflection.tag.assign(tagBytes, tagBytes + tag.size());
    }
    reflections.push_back(std::move(reflection));
  }

  void initiateFederateSave(std::wstring const& label) override {
    std::scoped_lock lock(mutex_);
    saveLabels.push_back(label);
  }

  void federationSaved() override {
    std::scoped_lock lock(mutex_);
    ++savedCount;
  }

  void federationNotSaved(rti::SaveFailureReason) override {
    std::scoped_lock lock(mutex_);
    ++saveFailureCount;
  }

  void requestFederationRestoreSucceeded(std::wstring const& label) override {
    std::scoped_lock lock(mutex_);
    restoreRequestLabels.push_back(label);
  }

  void federationRestoreBegun() override {
    std::scoped_lock lock(mutex_);
    ++restoreBegunCount;
  }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const&,
      rti::FederateHandle const&) override {
    std::scoped_lock lock(mutex_);
    restoreLabels.push_back(label);
  }

  void federationRestored() override {
    std::scoped_lock lock(mutex_);
    ++restoredCount;
  }

  void federationNotRestored(rti::RestoreFailureReason) override {
    std::scoped_lock lock(mutex_);
    ++restoreFailureCount;
  }

  struct Reflection final {
    rti::ObjectInstanceHandle object;
    std::vector<std::pair<rti::AttributeHandle, std::vector<std::uint8_t>>> values;
    std::vector<std::uint8_t> tag;
    rti::FederateHandle producer;
    bool hasRegions = false;
  };

  std::mutex mutex_;
  std::vector<rti::ObjectInstanceHandle> discoveredObjects;
  std::vector<Reflection> reflections;
  std::vector<std::wstring> saveLabels;
  std::size_t savedCount = 0U;
  std::size_t saveFailureCount = 0U;
  std::vector<std::wstring> restoreRequestLabels;
  std::size_t restoreBegunCount = 0U;
  std::vector<std::wstring> restoreLabels;
  std::size_t restoredCount = 0U;
  std::size_t restoreFailureCount = 0U;
};

std::unique_ptr<rti::RTIambassador> makeRti() {
  rti::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

template <typename Predicate>
void evokeUntil(
    rti::RTIambassador& ambassador,
    Predicate&& predicate,
    char const* description) {
  auto const deadline = std::chrono::steady_clock::now() + 5s;
  while (!predicate() && std::chrono::steady_clock::now() < deadline) {
    static_cast<void>(ambassador.evokeCallback(0.01));
  }
  REQUIRE(predicate());
  INFO(description);
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::filesystem::path resource(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

umbra::detail::PrevalidatedFomModule validateModule(
    std::filesystem::path const& path,
    umbra::detail::FomModuleKind kind,
    std::wstring designator) {
  umbra::detail::LibXml2FomValidator validator;
  auto result = validator.validate({
      path,
      resource("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != umbra::detail::FomValidationStatus::valid ||
      !result.module) {
    throw std::runtime_error("The public save/restore FOM did not validate.");
  }
  return *result.module;
}

umbra::detail::FederationDefinition composedDefinition() {
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validateModule(
          resource("mim/HLAstandardMIM-2025.xml"),
          umbra::detail::FomModuleKind::mim,
          L"urn:umbra:test:queued-ro-attribute-restore-mim"),
      validateModule(
          resource("examples/RestaurantFOMmodule-2025.xml"),
          umbra::detail::FomModuleKind::fom,
          L"urn:umbra:test:queued-ro-attribute-restore-fom"),
  };
  umbra::detail::LibXml2FomModuleComposer composer(
      resource("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != umbra::detail::FomCompositionStatus::valid ||
      !result.catalog || !result.fdd) {
    throw std::runtime_error("The public save/restore FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}
#endif

}  // namespace

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "Public save/restore reconstitutes one queued receive-order attribute update",
    "[integration][federation-management][save-restore][receive-order]"
    "[process-boundary][public-api][2025]"
    "[public-save-queued-ro-attribute-update][rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[federate.callback.reflect-attribute-values]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationServiceHost;
  using umbra::detail::ProcessFederationServiceOptions;

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost service(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"queued-ro-attribute-restore-server", 0xA5U},
      ProcessFederationServiceOptions{false});
  service.start();
  auto const port = service.address().port;
  REQUIRE(port != 0U);

  SaveRestoreRecorder ownerCallbacks;
  SaveRestoreRecorder receiverCallbacks;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto configuration = rti::RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"queued-ro-attribute-restore-public")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientFailure;
  try {
    rti::ConfigurationResult ownerConnection;
    try {
      ownerConnection = owner->connect(
          ownerCallbacks, rti::HLA_EVOKED, configuration);
    } catch (rti::Exception const& exception) {
      auto const name = exception.name();
      auto const message = exception.what();
      throw std::runtime_error(
          "Owner Connect threw " + std::string(name.begin(), name.end()) +
          ": " + std::string(message.begin(), message.end()));
    } catch (std::exception const& exception) {
      throw std::runtime_error(
          "Owner Connect threw std::exception: " + std::string(exception.what()));
    } catch (...) {
      throw std::runtime_error("Owner Connect threw a non-standard exception.");
    }
    REQUIRE(ownerConnection.addressUsed);
    REQUIRE(receiver->connect(receiverCallbacks, rti::HLA_EVOKED, configuration)
                .addressUsed);
    auto const federation = L"queued-ro-attribute-restore-public-execution";
    owner->createFederationExecution(
        federation, L"server-owned-queued-ro-attribute-restore-fom.xml");
    auto const ownerFederate =
        owner->joinFederationExecution(L"queued-owner", federation);
    auto const receiverFederate =
        receiver->joinFederationExecution(L"queued-receiver", federation);
    REQUIRE(ownerFederate.isValid());
    REQUIRE(receiverFederate.isValid());

    auto const ownerClass = owner->getObjectClassHandle(umbra::test::hla::wide::fom::employee);
    auto const receiverClass =
        receiver->getObjectClassHandle(umbra::test::hla::wide::fom::employee);
    auto const ownerAttribute = owner->getAttributeHandle(ownerClass, umbra::test::hla::wide::fixture::name);
    auto const receiverAttribute =
        receiver->getAttributeHandle(receiverClass, umbra::test::hla::wide::fixture::name);
    REQUIRE(ownerClass.isValid());
    REQUIRE(receiverClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE(receiverAttribute.isValid());
    owner->publishObjectClassAttributes(ownerClass, {ownerAttribute});
    receiver->subscribeObjectClassAttributes(receiverClass, {receiverAttribute});
    auto const object = owner->registerObjectInstance(ownerClass);
    REQUIRE(object.isValid());
    evokeUntil(
        *receiver,
        [&] {
          std::scoped_lock lock(receiverCallbacks.mutex_);
          return std::find(
                     receiverCallbacks.discoveredObjects.begin(),
                     receiverCallbacks.discoveredObjects.end(),
                     object) != receiverCallbacks.discoveredObjects.end();
        },
        "The subscriber did not discover the published object.");

    // Queue an ordinary, nonregional update before Save. The process server's
    // receive-order poll prioritizes the Save notification over its separate
    // attribute-update queue, allowing the receiver to cross the initiation
    // callback boundary without consuming the reflection.
    constexpr auto saveLabel = L"queued-ro-attribute-restore-save";
    std::vector<std::uint8_t> const value{0x52U, 0x4FU, 0x2DU, 0x41U};
    std::vector<std::uint8_t> const tag{0x50U, 0x55U, 0x42U};
    rti::AttributeHandleValueMap values{{
        ownerAttribute,
        rti::VariableLengthData(value.data(), value.size()),
    }};
    rti::VariableLengthData userTag(tag.data(), tag.size());
    owner->updateAttributeValues(object, values, userTag);
    owner->requestFederationSave(saveLabel);
    evokeUntil(
        *owner,
        [&] {
          std::scoped_lock lock(ownerCallbacks.mutex_);
          return !ownerCallbacks.saveLabels.empty();
        },
        "The owner did not receive initiateFederateSave.");
    evokeUntil(
        *receiver,
        [&] {
          std::scoped_lock lock(receiverCallbacks.mutex_);
          return !receiverCallbacks.saveLabels.empty();
        },
        "The receiver did not receive initiateFederateSave.");
    {
      std::scoped_lock lock(receiverCallbacks.mutex_);
      REQUIRE(receiverCallbacks.reflections.empty());
    }
    owner->federateSaveBegun();
    receiver->federateSaveBegun();
    owner->federateSaveComplete();
    receiver->federateSaveComplete();
    evokeUntil(
        *owner,
        [&] {
          std::scoped_lock lock(ownerCallbacks.mutex_);
          return ownerCallbacks.savedCount == 1U ||
              ownerCallbacks.saveFailureCount != 0U;
        },
        "The owner save did not complete.");
    evokeUntil(
        *receiver,
        [&] {
          std::scoped_lock lock(receiverCallbacks.mutex_);
          return receiverCallbacks.savedCount == 1U ||
              receiverCallbacks.saveFailureCount != 0U;
        },
        "The receiver save did not complete.");
    {
      std::scoped_lock lock(receiverCallbacks.mutex_);
      REQUIRE(receiverCallbacks.saveFailureCount == 0U);
      REQUIRE(receiverCallbacks.reflections.empty());
    }

    constexpr auto restoreLabel = L"queued-ro-attribute-restore-save";
    owner->requestFederationRestore(restoreLabel);
    evokeUntil(
        *owner,
        [&] {
          std::scoped_lock lock(ownerCallbacks.mutex_);
          return !ownerCallbacks.restoreRequestLabels.empty();
        },
        "The owner restore request did not succeed.");
    for (auto* ambassador : {owner.get(), receiver.get()}) {
      auto* callbacks = ambassador == owner.get()
          ? &ownerCallbacks
          : &receiverCallbacks;
      evokeUntil(
          *ambassador,
          [&] {
            std::scoped_lock lock(callbacks->mutex_);
            return !callbacks->restoreLabels.empty();
          },
          "A joined federate did not receive initiateFederateRestore.");
    }
    owner->federateRestoreComplete();
    receiver->federateRestoreComplete();
    evokeUntil(
        *owner,
        [&] {
          std::scoped_lock lock(ownerCallbacks.mutex_);
          return ownerCallbacks.restoredCount == 1U ||
              ownerCallbacks.restoreFailureCount != 0U;
        },
        "The owner restore did not complete.");
    evokeUntil(
        *receiver,
        [&] {
          std::scoped_lock lock(receiverCallbacks.mutex_);
          return receiverCallbacks.restoredCount == 1U ||
              receiverCallbacks.restoreFailureCount != 0U;
        },
        "The receiver restore did not complete.");
    {
      std::scoped_lock lock(receiverCallbacks.mutex_);
      REQUIRE(receiverCallbacks.restoreFailureCount == 0U);
      REQUIRE(receiverCallbacks.reflections.empty());
    }

    evokeUntil(
        *receiver,
        [&] {
          std::scoped_lock lock(receiverCallbacks.mutex_);
          return !receiverCallbacks.reflections.empty();
        },
        "Restore did not reconstitute the pending receive-order update.");
    {
      std::scoped_lock lock(receiverCallbacks.mutex_);
      REQUIRE(receiverCallbacks.reflections.size() == 1U);
      auto const& reflection = receiverCallbacks.reflections.front();
      REQUIRE(reflection.object == object);
      REQUIRE(reflection.values.size() == 1U);
      REQUIRE(reflection.values.front().first == receiverAttribute);
      REQUIRE(reflection.values.front().second == value);
      REQUIRE(reflection.tag == tag);
      REQUIRE(reflection.producer == ownerFederate);
      REQUIRE_FALSE(reflection.hasRegions);
    }
    static_cast<void>(receiver->evokeCallback(0.05));
    {
      std::scoped_lock lock(receiverCallbacks.mutex_);
      REQUIRE(receiverCallbacks.reflections.size() == 1U);
    }

    receiver->resignFederationExecution(rti::NO_ACTION);
    owner->resignFederationExecution(rti::NO_ACTION);
    receiver->disconnect();
    owner->disconnect();
  } catch (...) {
    clientFailure = std::current_exception();
    receiver.reset();
    owner.reset();
  }
  service.stop();
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_NOTHROW(service.rethrowIfFailed());
}
#endif
