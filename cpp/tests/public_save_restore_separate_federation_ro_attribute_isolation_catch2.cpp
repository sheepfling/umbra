#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/VariableLengthData.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
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

class IsolationRecorder final : public rti::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::ObjectClassHandle const&,
      std::wstring const&,
      rti::FederateHandle const&) override {
    std::scoped_lock lock(mutex);
    discovered.push_back(object);
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const&,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions) override {
    std::scoped_lock lock(mutex);
    Reflection reflection{object, {}, {}, producer, regions != nullptr};
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
    std::scoped_lock lock(mutex);
    saveLabels.push_back(label);
  }

  void federationSaved() override {
    std::scoped_lock lock(mutex);
    ++saved;
  }

  void federationNotSaved(rti::SaveFailureReason) override {
    std::scoped_lock lock(mutex);
    ++saveFailures;
  }

  void requestFederationRestoreSucceeded(std::wstring const& label) override {
    std::scoped_lock lock(mutex);
    restoreRequestLabels.push_back(label);
  }

  void federationRestoreBegun() override {
    std::scoped_lock lock(mutex);
    ++restoreBegun;
  }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const&,
      rti::FederateHandle const&) override {
    std::scoped_lock lock(mutex);
    restoreLabels.push_back(label);
  }

  void federationRestored() override {
    std::scoped_lock lock(mutex);
    ++restored;
  }

  void federationNotRestored(rti::RestoreFailureReason) override {
    std::scoped_lock lock(mutex);
    ++restoreFailures;
  }

  struct Reflection final {
    rti::ObjectInstanceHandle object;
    std::vector<std::pair<rti::AttributeHandle, std::vector<std::uint8_t>>> values;
    std::vector<std::uint8_t> tag;
    rti::FederateHandle producer;
    bool hasRegions = false;
  };

  std::mutex mutex;
  std::vector<rti::ObjectInstanceHandle> discovered;
  std::vector<Reflection> reflections;
  std::vector<std::wstring> saveLabels;
  std::size_t saved = 0U;
  std::size_t saveFailures = 0U;
  std::vector<std::wstring> restoreRequestLabels;
  std::size_t restoreBegun = 0U;
  std::vector<std::wstring> restoreLabels;
  std::size_t restored = 0U;
  std::size_t restoreFailures = 0U;
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
    throw std::runtime_error("The federation-isolation FOM did not validate.");
  }
  return *result.module;
}

umbra::detail::FederationDefinition composedDefinition() {
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validateModule(
          resource("mim/HLAstandardMIM-2025.xml"),
          umbra::detail::FomModuleKind::mim,
          L"urn:umbra:test:restore-federation-isolation-mim"),
      validateModule(
          resource("examples/RestaurantFOMmodule-2025.xml"),
          umbra::detail::FomModuleKind::fom,
          L"urn:umbra:test:restore-federation-isolation-fom"),
  };
  umbra::detail::LibXml2FomModuleComposer composer(
      resource("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != umbra::detail::FomCompositionStatus::valid ||
      !result.catalog || !result.fdd) {
    throw std::runtime_error("The federation-isolation FOM did not compose.");
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
    "Restoring one federation preserves another federation's queued receive-order attribute update",
    "[integration][internal-only][no-normative-mapping]"
    "[federation-management][save-restore][receive-order]"
    "[process-boundary][public-api][2025]"
    "[public-save-restore-separate-federation-ro-attribute-isolation]"
    "[rti.service.update-attribute-values]"
    "[rti.service.request-federation-save]"
    "[rti.service.request-federation-restore]"
    "[federate.callback.reflect-attribute-values]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationServiceHost;
  using umbra::detail::ProcessFederationServiceOptions;

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost service(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"restore-federation-isolation-server", 0xA5U},
      ProcessFederationServiceOptions{false});
  service.start();
  auto const port = service.address().port;
  REQUIRE(port != 0U);

  IsolationRecorder restoreCallbacks;
  IsolationRecorder publisherCallbacks;
  IsolationRecorder receiverCallbacks;
  auto restoreRti = makeRti();
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto configuration = rti::RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"restore-federation-isolation")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientFailure;
  try {
    REQUIRE(restoreRti->connect(
                restoreCallbacks, rti::HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE(publisher->connect(
                publisherCallbacks, rti::HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE(receiver->connect(
                receiverCallbacks, rti::HLA_EVOKED, configuration)
                .addressUsed);

    auto const restoreFederation = L"restore-isolation-execution-a";
    auto const liveFederation = L"restore-isolation-execution-b";
    restoreRti->createFederationExecution(
        restoreFederation, L"server-owned-restore-isolation-fom.xml");
    restoreRti->joinFederationExecution(L"restore-only-member", restoreFederation);
    publisher->createFederationExecution(
        liveFederation, L"server-owned-restore-isolation-fom.xml");
    auto const publisherFederate =
        publisher->joinFederationExecution(L"queued-update-publisher", liveFederation);
    auto const receiverFederate =
        receiver->joinFederationExecution(L"queued-update-receiver", liveFederation);
    REQUIRE(publisherFederate.isValid());
    REQUIRE(receiverFederate.isValid());

    auto const objectClass = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee");
    auto const receiverClass = receiver->getObjectClassHandle(L"HLAobjectRoot.Employee");
    auto const attribute = publisher->getAttributeHandle(objectClass, L"Name");
    auto const receiverAttribute = receiver->getAttributeHandle(receiverClass, L"Name");
    REQUIRE(objectClass.isValid());
    REQUIRE(receiverClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(receiverAttribute.isValid());
    publisher->publishObjectClassAttributes(objectClass, {attribute});
    receiver->subscribeObjectClassAttributes(receiverClass, {receiverAttribute});
    auto const object = publisher->registerObjectInstance(objectClass);
    REQUIRE(object.isValid());
    evokeUntil(
        *receiver,
        [&] {
          std::scoped_lock lock(receiverCallbacks.mutex);
          return std::find(
                     receiverCallbacks.discovered.begin(),
                     receiverCallbacks.discovered.end(),
                     object) != receiverCallbacks.discovered.end();
        },
        "The second federation's receiver did not discover the object.");

    std::vector<std::uint8_t> const value{0x49U, 0x53U, 0x4FU, 0x4CU};
    std::vector<std::uint8_t> const tag{0x53U, 0x45U, 0x50U};
    rti::AttributeHandleValueMap values{{
        attribute,
        rti::VariableLengthData(value.data(), value.size()),
    }};
    publisher->updateAttributeValues(
        object,
        values,
        rti::VariableLengthData(tag.data(), tag.size()));
    {
      std::scoped_lock lock(receiverCallbacks.mutex);
      REQUIRE(receiverCallbacks.reflections.empty());
    }

    constexpr auto saveLabel = L"restore-isolation-save-a";
    restoreRti->requestFederationSave(saveLabel);
    evokeUntil(
        *restoreRti,
        [&] {
          std::scoped_lock lock(restoreCallbacks.mutex);
          return !restoreCallbacks.saveLabels.empty();
        },
        "The first federation did not receive initiateFederateSave.");
    restoreRti->federateSaveBegun();
    restoreRti->federateSaveComplete();
    evokeUntil(
        *restoreRti,
        [&] {
          std::scoped_lock lock(restoreCallbacks.mutex);
          return restoreCallbacks.saved == 1U || restoreCallbacks.saveFailures != 0U;
        },
        "The first federation save did not complete.");
    {
      std::scoped_lock lock(restoreCallbacks.mutex);
      REQUIRE(restoreCallbacks.saveFailures == 0U);
    }

    constexpr auto restoreLabel = L"restore-isolation-save-a";
    restoreRti->requestFederationRestore(restoreLabel);
    evokeUntil(
        *restoreRti,
        [&] {
          std::scoped_lock lock(restoreCallbacks.mutex);
          return !restoreCallbacks.restoreRequestLabels.empty();
        },
        "The first federation restore request did not succeed.");
    evokeUntil(
        *restoreRti,
        [&] {
          std::scoped_lock lock(restoreCallbacks.mutex);
          return !restoreCallbacks.restoreLabels.empty();
        },
        "The first federation did not receive initiateFederateRestore.");
    restoreRti->federateRestoreComplete();
    evokeUntil(
        *restoreRti,
        [&] {
          std::scoped_lock lock(restoreCallbacks.mutex);
          return restoreCallbacks.restored == 1U || restoreCallbacks.restoreFailures != 0U;
        },
        "The first federation restore did not complete.");
    {
      std::scoped_lock lock(restoreCallbacks.mutex);
      REQUIRE(restoreCallbacks.restoreFailures == 0U);
    }

    evokeUntil(
        *receiver,
        [&] {
          std::scoped_lock lock(receiverCallbacks.mutex);
          return !receiverCallbacks.reflections.empty();
        },
        "Restoring the first federation discarded the other federation's queued update.");
    {
      std::scoped_lock lock(receiverCallbacks.mutex);
      REQUIRE(receiverCallbacks.reflections.size() == 1U);
      auto const& reflection = receiverCallbacks.reflections.front();
      REQUIRE(reflection.object == object);
      REQUIRE(reflection.values.size() == 1U);
      REQUIRE(reflection.values.front().first == receiverAttribute);
      REQUIRE(reflection.values.front().second == value);
      REQUIRE(reflection.tag == tag);
      REQUIRE(reflection.producer == publisherFederate);
      REQUIRE_FALSE(reflection.hasRegions);
    }
    static_cast<void>(receiver->evokeCallback(0.05));
    {
      std::scoped_lock lock(receiverCallbacks.mutex);
      REQUIRE(receiverCallbacks.reflections.size() == 1U);
    }

    receiver->resignFederationExecution(rti::NO_ACTION);
    publisher->resignFederationExecution(rti::NO_ACTION);
    restoreRti->resignFederationExecution(rti::NO_ACTION);
    receiver->disconnect();
    publisher->disconnect();
    restoreRti->disconnect();
  } catch (...) {
    clientFailure = std::current_exception();
    receiver.reset();
    publisher.reset();
    restoreRti.reset();
  }
  service.stop();
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_NOTHROW(service.rethrowIfFailed());
}
#endif
