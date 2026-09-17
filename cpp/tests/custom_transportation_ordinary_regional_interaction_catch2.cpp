#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The custom transportation ordinary-regional test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"custom-transportation-ordinary-regional-interaction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

bool hasBytes(VariableLengthData const& value,
              std::vector<unsigned char> const& expected) {
  if (value.size() != expected.size()) {
    return false;
  }
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  return bytes != nullptr && std::equal(expected.begin(), expected.end(), bytes);
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions});
  }

  std::vector<InteractionReport> interactionReports;
};

}  // namespace

TEST_CASE(
    "Embedded ordinary regional delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management]"
    "[custom-transportation][custom-transportation-ordinary-regional-interaction]"
    "[fom][transportation-type-lookup][transportation][ddm][regional-interaction]"
    "[interaction-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.send-interaction-with-regions][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  std::vector<std::wstring> const fomModules{
      resourcePath("transportation-regional-reference-consumer-fom.xml").wstring(),
      resourcePath("transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(consumer->connect(consumerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle producerHandle;
  REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
      L"ordinary-regional-transport-producer",
      L"producer",
      federationName));
  REQUIRE_NOTHROW(consumer->joinFederationExecution(
      L"ordinary-regional-transport-consumer",
      L"consumer",
      federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  auto const interaction = producer->getInteractionClassHandle(
      fixture_hla::fom::transportation_regional_interaction);
  auto const consumerInteraction = consumer->getInteractionClassHandle(
      fixture_hla::fom::transportation_regional_interaction);
  REQUIRE(custom.isValid());
  REQUIRE(interaction.isValid());
  REQUIRE(consumerInteraction.isValid());
  REQUIRE_NOTHROW(producer->publishInteractionClass(interaction));

  DimensionHandle const producerX = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  DimensionHandle const producerY = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  DimensionHandle const consumerX = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  DimensionHandle const consumerY = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(producerX.isValid());
  REQUIRE(producerY.isValid());
  REQUIRE(consumerX.isValid());
  REQUIRE(consumerY.isValid());

  RegionHandle const sourceRegion = producer->createRegion(
      DimensionHandleSet{producerX, producerY});
  RegionHandle const receiverRegion = consumer->createRegion(
      DimensionHandleSet{consumerX, consumerY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(receiverRegion.isValid());
  REQUIRE_NOTHROW(producer->setRangeBounds(
      sourceRegion,
      producerX,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(producer->setRangeBounds(
      sourceRegion,
      producerY,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(consumer->setRangeBounds(
      receiverRegion,
      consumerX,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(consumer->setRangeBounds(
      receiverRegion,
      consumerY,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(producer->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(consumer->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(consumer->subscribeInteractionClassWithRegions(
      consumerInteraction,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(consumer->setConveyRegionDesignatorSetsSwitch(true));

  unsigned char const tagBytes[] = {'R', 'E', 'G'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(producer->sendInteractionWithRegions(
      interaction,
      ParameterHandleValueMap{},
      RegionHandleSet{sourceRegion},
      tag));
  drainCallbacks(*consumer);

  REQUIRE(consumerReports.interactionReports.size() == 1U);
  auto const& report = consumerReports.interactionReports.front();
  REQUIRE(report.interactionClass == consumerInteraction);
  REQUIRE(report.parameterValues.empty());
  REQUIRE(report.transportationType == custom);
  REQUIRE(report.producingFederate == producerHandle);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.contains(sourceRegion));
  REQUIRE(hasBytes(
      report.userSuppliedTag,
      std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes))));

  REQUIRE_NOTHROW(consumer->unsubscribeInteractionClassWithRegions(
      consumerInteraction,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(consumer->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(producer->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(consumer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}
