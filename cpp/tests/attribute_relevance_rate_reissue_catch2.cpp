#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The attribute relevance rate-reissue tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CallbackModel;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;

class RecordingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct RateReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    std::wstring updateRateDesignator;
  };

  struct AttributeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveredObjectInstances.push_back(objectInstance);
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOnForObjectInstanceReports.push_back({objectInstance, attributes, {}});
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      std::wstring const& updateRateDesignator) override {
    turnUpdatesOnForObjectInstanceReports.push_back(
        {objectInstance, attributes, updateRateDesignator});
  }

  void turnUpdatesOffForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOffForObjectInstanceReports.push_back({objectInstance, attributes});
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<RateReport> turnUpdatesOnForObjectInstanceReports;
  std::vector<AttributeReport> turnUpdatesOffForObjectInstanceReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"embedded-rate-reissue-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

void suppressDeclarationRelevanceAdvisories(RTIambassador& rti) {
  if (rti.getObjectClassRelevanceAdvisorySwitch()) {
    rti.setObjectClassRelevanceAdvisorySwitch(false);
  }
  if (rti.getInteractionRelevanceAdvisorySwitch()) {
    rti.setInteractionRelevanceAdvisorySwitch(false);
  }
}

TEST_CASE(
    "Embedded attribute relevance advisories reissue turn-on when the active update rate changes",
    "[integration][development-profile][federation-management][object-management]"
    "[callbacks][attribute-relevance-advisory][attribute-relevance-rate-reissue]"
    "[update-rate-reissue][2025]"
    "[rti.service.subscribe-object-class-attributes]"
    "[federate.callback.turn-updates-on-for-object-instance]"
    "[federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    RecordingFederateAmbassador ownerReports;
    RecordingFederateAmbassador highRateReports;
    RecordingFederateAmbassador lowRateReports;
    auto owner = makeRti();
    auto highRateSubscriber = makeRti();
    auto lowRateSubscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(highRateSubscriber->connect(highRateReports, callbackModel));
    REQUIRE_NOTHROW(lowRateSubscriber->connect(lowRateReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"rate-reissue-owner", L"rate-reissue-owner", federationName));
    REQUIRE_NOTHROW(highRateSubscriber->joinFederationExecution(
        L"rate-reissue-high", L"rate-reissue-high", federationName));
    REQUIRE_NOTHROW(lowRateSubscriber->joinFederationExecution(
        L"rate-reissue-low", L"rate-reissue-low", federationName));
    suppressDeclarationRelevanceAdvisories(*owner);

    auto const objectClass =
        owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const attribute =
        owner->getAttributeHandle(objectClass, fixture_hla::fixture::flavor);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());
    REQUIRE(highRateSubscriber->getAttributeRelevanceAdvisorySwitch());
    REQUIRE(lowRateSubscriber->getAttributeRelevanceAdvisorySwitch());

    AttributeHandleSet const attributeSet{attribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributeSet));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
    REQUIRE(objectInstance.isValid());

    auto drain = [](RTIambassador& rti, CallbackModel model) {
      if (model == HLA_EVOKED) {
        while (rti.evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };
    auto drainSubscriber = [&](RTIambassador& rti) { drain(rti, callbackModel); };
    auto drainOwner = [&] { drain(*owner, callbackModel); };

    // The first active declaration establishes the maximum rate and emits the
    // initial owner-directed, rate-bearing advisory after discovery commits.
    REQUIRE_NOTHROW(highRateSubscriber->subscribeObjectClassAttributes(
        objectClass,
        attributeSet,
        true,
        L"High"));
    drainSubscriber(*highRateSubscriber);
    REQUIRE(highRateReports.discoveredObjectInstances.size() == 1U);
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().attributes ==
            attributeSet);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().updateRateDesignator ==
            L"High");
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());

    // A lower-rate peer does not change the active maximum and therefore does
    // not reissue an owner callback.  Replacing that peer with an even lower
    // rate remains silent for the same reason.
    REQUIRE_NOTHROW(lowRateSubscriber->subscribeObjectClassAttributes(
        objectClass,
        attributeSet,
        true,
        L"Medium"));
    drainSubscriber(*lowRateSubscriber);
    REQUIRE(lowRateReports.discoveredObjectInstances.size() == 1U);
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());

    REQUIRE_NOTHROW(lowRateSubscriber->subscribeObjectClassAttributes(
        objectClass,
        attributeSet,
        true,
        L"Low"));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());

    // Lowering the current maximum reissues Turn Updates On with the new
    // maximum, and raising it again reissues with the restored designator.
    REQUIRE_NOTHROW(highRateSubscriber->subscribeObjectClassAttributes(
        objectClass,
        attributeSet,
        true,
        L"Medium"));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 2U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().attributes ==
            attributeSet);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().updateRateDesignator ==
            L"Medium");

    REQUIRE_NOTHROW(highRateSubscriber->subscribeObjectClassAttributes(
        objectClass,
        attributeSet,
        true,
        L"High"));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 3U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().updateRateDesignator ==
            L"High");

    // Removing the higher-rate peer leaves the Low declaration relevant, so
    // the boundary carries a Low-rate refresh rather than Turn Updates Off.
    REQUIRE_NOTHROW(highRateSubscriber->unsubscribeObjectClassAttributes(
        objectClass,
        attributeSet));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 4U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().attributes ==
            attributeSet);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().updateRateDesignator ==
            L"Low");

    REQUIRE_NOTHROW(lowRateSubscriber->unsubscribeObjectClassAttributes(
        objectClass,
        attributeSet));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().attributes ==
            attributeSet);

    REQUIRE_NOTHROW(highRateSubscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(lowRateSubscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(highRateSubscriber->disconnect());
    REQUIRE_NOTHROW(lowRateSubscriber->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

}  // namespace
