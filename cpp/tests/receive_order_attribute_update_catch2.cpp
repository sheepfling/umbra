#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The receive-order attribute-update test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

struct ObjectDiscoveryReport final {
  ObjectInstanceHandle objectInstance;
  ObjectClassHandle objectClass;
};

struct AttributeReflectionReport final {
  ObjectInstanceHandle objectInstance;
  AttributeHandleValueMap attributeValues;
  VariableLengthData userSuppliedTag;
  TransportationTypeHandle transportationType;
  FederateHandle producingFederate;
  bool sentRegionsSupplied = false;
};

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back({objectInstance, objectClass});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"receive-order-attribute-update-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

std::vector<unsigned char> bytes(VariableLengthData const& value) {
  auto const* data = static_cast<unsigned char const*>(value.data());
  if (data == nullptr || value.size() == 0U) {
    return {};
  }
  return {data, data + value.size()};
}

}  // namespace

TEST_CASE(
    "Embedded receive-order Update Attribute Values honors 2025 passel and callback lifecycle",
    "[integration][development-profile][object-management][callbacks]"
    "[receive-order-attribute-update][callback-evoked][receive-order]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador cancelledReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto cancelled = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleValueMap noAttributeValues;
  unsigned char const tagBytes[] = {0xC0, 0x25, 0xA4};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->updateAttributeValues(
          invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"attribute-update-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"attribute-update-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"attribute-update-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"attribute-update-cancelled", L"subscriber", federationName));

  auto const base = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = publisher->getAttributeHandle(child, L"ReliableBaseB");
  auto const bestEffortBase = publisher->getAttributeHandle(child, L"BestEffortBase");
  auto const reliableChild = publisher->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = publisher->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(bestEffortBase.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const allOwned{
      reliableBaseA, reliableBaseB, bestEffortBase, reliableChild};
  AttributeHandleSet const baseAttributes{
      reliableBaseA, reliableBaseB, bestEffortBase};
  AttributeHandleSet const reliableChildOnly{reliableChild};
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(
      base, baseAttributes, true));
  REQUIRE_NOTHROW(cancelled->subscribeObjectClassAttributes(
      child, reliableChildOnly));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, allOwned));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  REQUIRE(exactReports.objectDiscoveryReports.empty());
  REQUIRE(promotedReports.objectDiscoveryReports.empty());
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  drainCallbacks(*exact);
  drainCallbacks(*promoted);
  drainCallbacks(*cancelled);
  REQUIRE(exactReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(promotedReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(cancelledReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(exactReports.objectDiscoveryReports.front().objectClass == child);
  REQUIRE(promotedReports.objectDiscoveryReports.front().objectClass == base);
  REQUIRE(cancelledReports.objectDiscoveryReports.front().objectClass == child);

  unsigned char const reliableBaseABytes[] = {0x01, 0x02};
  unsigned char const reliableBaseBBytes[] = {0x03, 0x04};
  unsigned char const bestEffortBaseBytes[] = {0x05, 0x06};
  unsigned char const reliableChildBytes[] = {0x07, 0x08};
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      reliableBaseA,
      VariableLengthData(reliableBaseABytes, sizeof(reliableBaseABytes)));
  attributeValues.emplace(
      reliableBaseB,
      VariableLengthData(reliableBaseBBytes, sizeof(reliableBaseBBytes)));
  attributeValues.emplace(
      bestEffortBase,
      VariableLengthData(bestEffortBaseBytes, sizeof(bestEffortBaseBytes)));
  attributeValues.emplace(
      reliableChild,
      VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));

  AttributeHandleValueMap unownedValues;
  unownedValues.emplace(
      unownedChild,
      VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          invalidObjectInstance, attributeValues, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandle invalidAttribute;
  AttributeHandleValueMap invalidAttributeValues;
  invalidAttributeValues.emplace(invalidAttribute, VariableLengthData());
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance, invalidAttributeValues, tag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, unownedValues, tag),
      rti1516_2025::AttributeNotOwned);
  REQUIRE_THROWS_AS(
      exact->updateAttributeValues(objectInstance, attributeValues, tag),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(
      publisher->updateAttributeValues(objectInstance, attributeValues, tag));
  REQUIRE(exactReports.attributeReflectionReports.empty());
  REQUIRE(promotedReports.attributeReflectionReports.empty());
  REQUIRE(cancelledReports.attributeReflectionReports.empty());

  // Removing the active subscription before callback delivery suppresses the
  // queued reflection. The other recipients retain the passel boundaries.
  REQUIRE_NOTHROW(cancelled->unsubscribeObjectClassAttributes(
      child, reliableChildOnly));
  drainCallbacks(*exact);
  drainCallbacks(*promoted);
  drainCallbacks(*cancelled);

  auto const reliableTransportation =
      publisher->getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffortTransportation =
      publisher->getTransportationTypeHandle(L"HLAbestEffort");
  REQUIRE(exactReports.attributeReflectionReports.size() == 2U);
  REQUIRE(promotedReports.attributeReflectionReports.size() == 2U);
  REQUIRE(cancelledReports.attributeReflectionReports.empty());
  REQUIRE(publisherReports.attributeReflectionReports.empty());

  auto findReflection = [](
                           std::vector<AttributeReflectionReport> const& reports,
                           TransportationTypeHandle const& transportationType)
      -> AttributeReflectionReport const* {
    auto const found = std::find_if(
        reports.begin(), reports.end(), [&](AttributeReflectionReport const& report) {
          return report.transportationType == transportationType;
        });
    return found == reports.end() ? nullptr : &*found;
  };
  auto requireReflection = [&](AttributeReflectionReport const& report,
                                TransportationTypeHandle const& transportationType,
                                std::vector<AttributeHandle> const& expectedAttributes) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == expectedAttributes.size());
    for (AttributeHandle const& attribute : expectedAttributes) {
      auto const expected = attributeValues.find(attribute);
      REQUIRE(expected != attributeValues.end());
      auto const received = report.attributeValues.find(attribute);
      REQUIRE(received != report.attributeValues.end());
      REQUIRE(bytes(received->second) == bytes(expected->second));
    }
    REQUIRE(bytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == transportationType);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  };

  auto const* exactReliable =
      findReflection(exactReports.attributeReflectionReports, reliableTransportation);
  auto const* exactBestEffort = findReflection(
      exactReports.attributeReflectionReports, bestEffortTransportation);
  auto const* promotedReliable = findReflection(
      promotedReports.attributeReflectionReports, reliableTransportation);
  auto const* promotedBestEffort = findReflection(
      promotedReports.attributeReflectionReports, bestEffortTransportation);
  REQUIRE(exactReliable != nullptr);
  REQUIRE(exactBestEffort != nullptr);
  REQUIRE(promotedReliable != nullptr);
  REQUIRE(promotedBestEffort != nullptr);
  requireReflection(
      *exactReliable,
      reliableTransportation,
      {reliableBaseA, reliableBaseB, reliableChild});
  requireReflection(
      *exactBestEffort,
      bestEffortTransportation,
      {bestEffortBase});
  requireReflection(
      *promotedReliable,
      reliableTransportation,
      {reliableBaseA, reliableBaseB});
  requireReflection(
      *promotedBestEffort,
      bestEffortTransportation,
      {bestEffortBase});

  REQUIRE_NOTHROW(publisher->unpublishObjectClass(child));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, attributeValues, tag),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(cancelled->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
