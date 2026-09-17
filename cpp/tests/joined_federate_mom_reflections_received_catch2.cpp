#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM reflections-received tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLAfixedRecord;
using rti1516_2025::HLAinteger32BE;
using rti1516_2025::HLAoctet;
using rti1516_2025::HLAvariableArray;
using rti1516_2025::HLAvariableArrayT;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::NO_ACTION;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

using rti1516_2025::umbra_binding_detail::decodeObjectClassHandle;
using rti1516_2025::umbra_binding_detail::decodeTransportationTypeHandle;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  auto const ticks = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  return L"federation-mom-refl-" + std::to_wstring(ticks) + L"-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path restaurantFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 256; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
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

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct AttributeTransportationTypeChangeReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    TransportationTypeHandle transportationType;
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
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
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
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      TransportationTypeHandle const& transportationType) override {
    attributeTransportationTypeChangeReports.push_back({
        objectInstance,
        attributes,
        transportationType,
    });
  }

  std::vector<InteractionReport> interactionReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<AttributeTransportationTypeChangeReport>
      attributeTransportationTypeChangeReports;
};

struct DecodedObjectClassCount final {
  ObjectClassHandle objectClass;
  std::int32_t count = 0;
};

std::vector<DecodedObjectClassCount> decodeObjectClassCounts(
    VariableLengthData const& encodedValue) {
  HLAvariableArrayT<HLAoctet> objectClassHandlePrototype;
  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(objectClassHandlePrototype)
      .appendElement(HLAinteger32BE{});
  HLAvariableArray counts{recordPrototype};
  counts.decode(encodedValue);

  std::vector<DecodedObjectClassCount> decoded;
  decoded.reserve(counts.size());
  for (std::size_t index = 0; index != counts.size(); ++index) {
    auto const& record = dynamic_cast<HLAfixedRecord const&>(counts.get(index));
    auto const& encodedObjectClass =
        dynamic_cast<HLAvariableArray const&>(record.get(0));
    decoded.push_back({
        decodeObjectClassHandle(encodedObjectClass.encode()),
        dynamic_cast<HLAinteger32BE const&>(record.get(1)).get(),
    });
  }
  return decoded;
}

}  // namespace

TEST_CASE(
    "Embedded MOM requestReflectionsReceived reports class and transportation counts",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][object-management][transportation-management]"
    "[callback-model][joined-federate-mom-reflections-received]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.update-attribute-values]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.subscribe-object-class-attributes][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.reflect-attribute-values][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador idleReports;
  auto requester = makeRti();
  auto sender = makeRti();
  auto idle = makeRti();
  auto const federationName = nextFederationName();
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(idle->connect(idleReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, restaurantFom().wstring(), standard_hla::mom::integer64_time));
  FederateHandle requesterFederate;
  REQUIRE_NOTHROW(requesterFederate = requester->joinFederationExecution(
      L"mom-reflections-received-requester", L"requester", federationName));
  FederateHandle senderFederate;
  REQUIRE_NOTHROW(senderFederate = sender->joinFederationExecution(
      L"mom-reflections-received-sender", L"sender", federationName));
  FederateHandle idleFederate;
  REQUIRE_NOTHROW(idleFederate = idle->joinFederationExecution(
      L"mom-reflections-received-idle", L"idle", federationName));

  auto const reportClass = requester->getInteractionClassHandle(
      standard_hla::mom::report_reflections_received);
  auto const reportTransportation = requester->getParameterHandle(
      reportClass, standard_hla::mom::transportation);
  auto const reportReflectionCounts = requester->getParameterHandle(
      reportClass, L"HLAreflectCounts");
  auto const requestClass = requester->getInteractionClassHandle(
      standard_hla::mom::request_reflections_received);
  auto const requestFederate = requester->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTransportation.isValid());
  REQUIRE(reportReflectionCounts.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederate.isValid());
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(reportClass));

  auto const serverClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const serverEfficiency = sender->getAttributeHandle(
      serverClass, L"Efficiency");
  auto const sodaClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const sodaFlavor = sender->getAttributeHandle(sodaClass, L"Flavor");
  auto const receiverServerClass = requester->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const receiverServerEfficiency = requester->getAttributeHandle(
      receiverServerClass, L"Efficiency");
  auto const receiverSodaClass = requester->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const receiverSodaFlavor = requester->getAttributeHandle(
      receiverSodaClass, L"Flavor");
  auto const reliable = sender->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const bestEffort = sender->getTransportationTypeHandle(
      standard_hla::mom::best_effort);
  REQUIRE(serverClass.isValid());
  REQUIRE(serverEfficiency.isValid());
  REQUIRE(sodaClass.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(receiverServerClass.isValid());
  REQUIRE(receiverServerEfficiency.isValid());
  REQUIRE(receiverSodaClass.isValid());
  REQUIRE(receiverSodaFlavor.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      serverClass, AttributeHandleSet{serverEfficiency}));
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      sodaClass, AttributeHandleSet{sodaFlavor}));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      receiverServerClass, AttributeHandleSet{receiverServerEfficiency}));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      receiverSodaClass, AttributeHandleSet{receiverSodaFlavor}));

  auto const serverObject = sender->registerObjectInstance(serverClass);
  auto const sodaObject = sender->registerObjectInstance(sodaClass);
  REQUIRE(serverObject.isValid());
  REQUIRE(sodaObject.isValid());
  drainCallbacks(*requester);
  requesterReports.attributeReflectionReports.clear();

  // Commit a per-instance transportation override through the official public
  // request path before accepting the two server updates.
  REQUIRE_NOTHROW(sender->requestAttributeTransportationTypeChange(
      serverObject, AttributeHandleSet{serverEfficiency}, bestEffort));
  REQUIRE(senderReports.attributeTransportationTypeChangeReports.empty());
  drainCallbacks(*sender);
  REQUIRE(senderReports.attributeTransportationTypeChangeReports.size() == 1U);
  REQUIRE(senderReports.attributeTransportationTypeChangeReports.front().objectInstance ==
      serverObject);
  REQUIRE(senderReports.attributeTransportationTypeChangeReports.front().attributes ==
      AttributeHandleSet{serverEfficiency});
  REQUIRE(senderReports.attributeTransportationTypeChangeReports.front().transportationType ==
      bestEffort);

  REQUIRE_NOTHROW(sender->updateAttributeValues(
      serverObject,
      AttributeHandleValueMap{{serverEfficiency, HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      serverObject,
      AttributeHandleValueMap{{serverEfficiency, HLAinteger32BE{2}.encode()}},
      VariableLengthData{}));
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      sodaObject,
      AttributeHandleValueMap{{sodaFlavor, HLAinteger32BE{101}.encode()}},
      VariableLengthData{}));
  drainCallbacks(*requester);

  REQUIRE(requesterReports.attributeReflectionReports.size() == 3U);
  REQUIRE(std::count_if(
      requesterReports.attributeReflectionReports.begin(),
      requesterReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == serverObject &&
            report.transportationType == bestEffort;
      }) == 2);
  REQUIRE(std::count_if(
      requesterReports.attributeReflectionReports.begin(),
      requesterReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == sodaObject &&
            report.transportationType == reliable;
      }) == 1);
  for (auto const& reflection : requesterReports.attributeReflectionReports) {
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    REQUIRE(reflection.producingFederate == senderFederate);
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
  }

  auto requestReportFor = [&](FederateHandle const& reportedFederate) {
    auto const before = requesterReports.interactionReports.size();
    REQUIRE_NOTHROW(requester->sendInteraction(
        requestClass,
        ParameterHandleValueMap{{requestFederate, reportedFederate.encode()}},
        VariableLengthData{}));
    REQUIRE(before == requesterReports.interactionReports.size());
    drainCallbacks(*requester);
    return before;
  };

  // HLArequestReflectionsReceived reports the callback ledger owned by the
  // referenced joined federate, so target the receiver rather than the
  // application sender whose own reflection ledger is empty.
  auto const receiverFirstReport = requestReportFor(requesterFederate);
  REQUIRE(requesterReports.interactionReports.size() == receiverFirstReport + 2U);
  auto verifyReport = [&](std::size_t index,
                          TransportationTypeHandle const& expectedTransportation,
                          ObjectClassHandle const* expectedObjectClass,
                          std::int32_t expectedCount) {
    auto const& report = requesterReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 2U);
    REQUIRE(report.parameterValues.contains(reportTransportation));
    REQUIRE(report.parameterValues.contains(reportReflectionCounts));
    REQUIRE(report.transportationType == reliable);
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(report.sentRegionsSupplied);

    TransportationTypeHandle decodedTransportation;
    REQUIRE_NOTHROW(decodedTransportation = decodeTransportationTypeHandle(
        report.parameterValues.at(reportTransportation)));
    REQUIRE(decodedTransportation == expectedTransportation);
    std::vector<DecodedObjectClassCount> decodedCounts;
    REQUIRE_NOTHROW(decodedCounts = decodeObjectClassCounts(
        report.parameterValues.at(reportReflectionCounts)));
    if (expectedObjectClass == nullptr) {
      REQUIRE(decodedCounts.empty());
    } else {
      REQUIRE(decodedCounts.size() == 1U);
      REQUIRE(decodedCounts.front().objectClass == *expectedObjectClass);
      REQUIRE(decodedCounts.front().count == expectedCount);
    }
  };

  auto const senderReportBegin = static_cast<std::size_t>(receiverFirstReport);
  bool sawBestEffortCounts = false;
  bool sawReliableCounts = false;
  for (std::size_t index = senderReportBegin;
       index != requesterReports.interactionReports.size(); ++index) {
    TransportationTypeHandle decodedTransportation;
    REQUIRE_NOTHROW(decodedTransportation = decodeTransportationTypeHandle(
        requesterReports.interactionReports.at(index)
            .parameterValues.at(reportTransportation)));
    std::vector<DecodedObjectClassCount> decodedCounts;
    REQUIRE_NOTHROW(decodedCounts = decodeObjectClassCounts(
        requesterReports.interactionReports.at(index)
            .parameterValues.at(reportReflectionCounts)));
    if (decodedTransportation == bestEffort) {
      verifyReport(index, bestEffort, &serverClass, 2);
      sawBestEffortCounts = true;
    } else {
      REQUIRE(decodedTransportation == reliable);
      verifyReport(index, reliable, &sodaClass, 1);
      sawReliableCounts = true;
    }
  }
  REQUIRE(sawBestEffortCounts);
  REQUIRE(sawReliableCounts);

  auto const idleFirstReport = requestReportFor(idleFederate);
  REQUIRE(requesterReports.interactionReports.size() == idleFirstReport + 2U);
  for (std::size_t index = static_cast<std::size_t>(idleFirstReport);
       index != requesterReports.interactionReports.size(); ++index) {
    TransportationTypeHandle decodedTransportation;
    REQUIRE_NOTHROW(decodedTransportation = decodeTransportationTypeHandle(
        requesterReports.interactionReports.at(index)
            .parameterValues.at(reportTransportation)));
    REQUIRE((decodedTransportation == reliable || decodedTransportation == bestEffort));
    verifyReport(index, decodedTransportation, nullptr, 0);
  }

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(idle->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(idle->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
