#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using namespace rti1516_2025;
using namespace umbra::detail;

std::filesystem::path resource(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validateModule(
    std::filesystem::path const& path,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      path,
      resource("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The delayed regional process FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition delayedRegionalDefinition(bool enableDelay) {
  std::vector<PrevalidatedFomModule> modules{
      validateModule(
          resource("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:delayed-regional-process-mim"),
      validateModule(
          resource("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:delayed-regional-process-restaurant"),
  };
  if (enableDelay) {
    modules.push_back(validateModule(
        std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
            "data" / "delay-subscription-evaluation-enabled-fom.xml",
        FomModuleKind::fom,
        L"urn:umbra:test:delayed-regional-process-switches"));
  }
  LibXml2FomModuleComposer composer(resource("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The delayed regional process FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

class RegionalReceiver final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++discoveries;
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* sentRegions) override {
    ++reflections;
    receivedAttributes = values.size();
    receivedSentRegions = sentRegions == nullptr ? 0U : sentRegions->size();
    if (tag.size() != 0U) {
      auto const* bytes = static_cast<std::uint8_t const*>(tag.data());
      receivedTag.assign(bytes, bytes + tag.size());
    }
  }

  std::size_t discoveries = 0U;
  std::size_t reflections = 0U;
  std::size_t receivedAttributes = 0U;
  std::size_t receivedSentRegions = 0U;
  std::vector<std::uint8_t> receivedTag;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

RtiConfiguration configurationFor(std::uint16_t port, wchar_t const* name) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(name)
      .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
}

void drainCallbacks(RTIambassador& rti) {
  while (rti.evokeCallback(0.0)) {
  }
}

void drainCallbacksFor(RTIambassador& rti) {
  for (unsigned attempt = 0U; attempt < 20U; ++attempt) {
    drainCallbacks(rti);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}

template <typename Ready>
void pumpCallbacksUntil(RTIambassador& rti, Ready ready) {
  for (unsigned attempt = 0U; attempt < 50U && !ready(); ++attempt) {
    drainCallbacks(rti);
    if (!ready()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }
}

void runScenario(bool enableDelay) {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      delayedRegionalDefinition(enableDelay),
      {"127.0.0.1", 0U},
      {"umbra-delayed-regional-process-test", 0xD351U});
  host.start();

  constexpr wchar_t const* federationName = L"process-delayed-regional-attribute";
  NullFederateAmbassador senderFederate;
  RegionalReceiver receiverFederate;
  auto sender = makeRti();
  auto receiver = makeRti();
  REQUIRE(sender->connect(
      senderFederate,
      HLA_EVOKED,
      configurationFor(host.address().port, L"delayed-regional-sender")).addressUsed);
  REQUIRE(receiver->connect(
      receiverFederate,
      HLA_EVOKED,
      configurationFor(host.address().port, L"delayed-regional-receiver")).addressUsed);
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, L"server-owned-fom.xml"));
  REQUIRE_NOTHROW(sender->joinFederationExecution(L"sender", L"test", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", L"test", federationName));
  auto const receiverMembership = registry.memberByName(federationName, L"receiver");
  REQUIRE(receiverMembership);
  REQUIRE(registry.delaySubscriptionEvaluationSwitchFor(
              federationName, receiverMembership->id) == enableDelay);

  auto const senderClass = sender->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const senderAttribute = sender->getAttributeHandle(senderClass, L"Flavor");
  auto const senderDimension = sender->getDimensionHandle(L"SodaFlavor");
  auto const receiverClass = receiver->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const receiverAttribute = receiver->getAttributeHandle(receiverClass, L"Flavor");
  auto const receiverDimension = receiver->getDimensionHandle(L"SodaFlavor");
  REQUIRE(senderClass.isValid());
  REQUIRE(senderAttribute.isValid());
  REQUIRE(senderDimension.isValid());
  REQUIRE(receiverClass.isValid());
  REQUIRE(receiverAttribute.isValid());
  REQUIRE(receiverDimension.isValid());
  auto const senderRegion = sender->createRegion(DimensionHandleSet{senderDimension});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{receiverDimension});
  sender->setRangeBounds(senderRegion, senderDimension, RangeBounds(0UL, 1UL));
  sender->commitRegionModifications(RegionHandleSet{senderRegion});
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(0UL, 1UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  AttributeHandleSet const senderAttributes{senderAttribute};
  AttributeHandleSetRegionHandleSetPairVector const senderPairs{{
      senderAttributes,
      RegionHandleSet{senderRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
      AttributeHandleSet{receiverAttribute},
      RegionHandleSet{receiverRegion},
  }};
  sender->publishObjectClassAttributes(senderClass, senderAttributes);
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  auto const object = sender->registerObjectInstanceWithRegions(senderClass, senderPairs);
  REQUIRE(object.isValid());
  pumpCallbacksUntil(*receiver, [&] { return receiverFederate.discoveries == 1U; });
  REQUIRE(receiverFederate.discoveries == 1U);

  // The receiver is known, but has no attribute declaration when each update
  // is generated. The creation-time switch keeps its route until delivery.
  receiver->unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(2UL, 3UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  std::array<std::uint8_t, 1U> const value{0xD5U};
  AttributeHandleValueMap values;
  values.emplace(senderAttribute, VariableLengthData(value.data(), value.size()));
  std::array<std::uint8_t, 1U> const firstTag{0xD1U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(firstTag.data(), firstTag.size()));
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  drainCallbacksFor(*receiver);
  REQUIRE(receiverFederate.reflections == 0U);

  receiver->unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(0UL, 1UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  std::array<std::uint8_t, 1U> const secondTag{0xD2U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(secondTag.data(), secondTag.size()));
  // Eligibility at receive uses the source's committed range when Update was
  // issued, not a later mutation of the same region handle.
  sender->setRangeBounds(senderRegion, senderDimension, RangeBounds(2UL, 3UL));
  sender->commitRegionModifications(RegionHandleSet{senderRegion});
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  if (enableDelay) {
    pumpCallbacksUntil(*receiver, [&] { return receiverFederate.reflections == 1U; });
  } else {
    drainCallbacksFor(*receiver);
  }
  REQUIRE(receiverFederate.reflections == (enableDelay ? 1U : 0U));
  if (enableDelay) {
    REQUIRE(receiverFederate.receivedAttributes == 1U);
    REQUIRE(receiverFederate.receivedSentRegions == 1U);
    REQUIRE(receiverFederate.receivedTag ==
            std::vector<std::uint8_t>(secondTag.begin(), secondTag.end()));
  }

  // A passel initially eligible for this overlapping declaration must still
  // be suppressed when the committed receiving range becomes disjoint before
  // the evoked callback entry, irrespective of the switch setting.
  sender->setRangeBounds(senderRegion, senderDimension, RangeBounds(0UL, 1UL));
  sender->commitRegionModifications(RegionHandleSet{senderRegion});
  std::array<std::uint8_t, 1U> const thirdTag{0xD3U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(thirdTag.data(), thirdTag.size()));
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(2UL, 3UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  drainCallbacksFor(*receiver);
  REQUIRE(receiverFederate.reflections == (enableDelay ? 1U : 0U));

  receiver->resignFederationExecution(NO_ACTION);
  sender->resignFederationExecution(DELETE_OBJECTS);
  sender->destroyFederationExecution(federationName);
  receiver->disconnect();
  sender->disconnect();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

}  // namespace

TEST_CASE(
    "Process delayed regional attribute delivery rechecks the original sent region",
    "[integration][object-management][data-distribution-management]"
    "[delay-subscription-evaluation][process-boundary]"
    "[process-delayed-regional-attribute][2025]"
    "[rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values]") {
  SECTION("enabled delays eligibility to the HLA_EVOKED receive fence") {
    runScenario(true);
  }
  SECTION("disabled preserves generation-time exclusion") {
    runScenario(false);
  }
}
