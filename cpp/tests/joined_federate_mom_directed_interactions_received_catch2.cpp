#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM directed-interactions-received tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLAfixedRecord;
using rti1516_2025::HLAinteger32BE;
using rti1516_2025::HLAoctet;
using rti1516_2025::HLAvariableArray;
using rti1516_2025::HLAvariableArrayT;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
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

using rti1516_2025::umbra_binding_detail::decodeInteractionClassHandle;
using rti1516_2025::umbra_binding_detail::decodeTransportationTypeHandle;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-joined-mom-directed-interactions-received-" +
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

  struct DirectedInteractionReport final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

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

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<InteractionReport> interactionReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
};

struct DecodedInteractionCount final {
  InteractionClassHandle interactionClass;
  std::int32_t count = 0;
};

std::vector<DecodedInteractionCount> decodeInteractionCounts(
    VariableLengthData const& encodedValue) {
  // HLAinteractionCounts is the official 2025 MIM variable array of
  // HLAinteractionCount fixed records. Decode it with standard data elements.
  HLAvariableArrayT<HLAoctet> interactionClassHandlePrototype;
  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(interactionClassHandlePrototype)
      .appendElement(HLAinteger32BE{});
  HLAvariableArray counts{recordPrototype};
  counts.decode(encodedValue);

  std::vector<DecodedInteractionCount> decoded;
  decoded.reserve(counts.size());
  for (std::size_t index = 0; index != counts.size(); ++index) {
    auto const& record = dynamic_cast<HLAfixedRecord const&>(counts.get(index));
    auto const& encodedInteractionClass =
        dynamic_cast<HLAvariableArray const&>(record.get(0));
    decoded.push_back({
        decodeInteractionClassHandle(encodedInteractionClass.encode()),
        dynamic_cast<HLAinteger32BE const&>(record.get(1)).get(),
    });
  }
  return decoded;
}

}  // namespace

TEST_CASE(
    "Embedded MOM requestDirectedInteractionsReceived reports directed counts and NULL buckets",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][interaction-management][directed][transportation-management]"
    "[callback-model][joined-federate-mom-directed-interactions-received]"
    "[rti.service.get-interaction-class-handle][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance]"
    "[rti.service.send-directed-interaction][rti.service.send-interaction]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-directed-interaction][federate.callback.receive-interaction]") {
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
      L"mom-directed-interactions-received-requester", L"requester", federationName));
  FederateHandle senderFederate;
  REQUIRE_NOTHROW(senderFederate = sender->joinFederationExecution(
      L"mom-directed-interactions-received-sender", L"sender", federationName));
  FederateHandle idleFederate;
  REQUIRE_NOTHROW(idleFederate = idle->joinFederationExecution(
      L"mom-directed-interactions-received-idle", L"idle", federationName));
  REQUIRE(requesterFederate.isValid());
  REQUIRE(senderFederate.isValid());
  REQUIRE(idleFederate.isValid());

  auto const reportClass = requester->getInteractionClassHandle(
      standard_hla::mom::report_directed_interactions_received);
  auto const reportTransportation = requester->getParameterHandle(
      reportClass, standard_hla::mom::transportation);
  auto const reportInteractionCounts = requester->getParameterHandle(
      reportClass, L"HLAinteractionCounts");
  auto const requestClass = requester->getInteractionClassHandle(
      standard_hla::mom::request_directed_interactions_received);
  auto const requestFederate = requester->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTransportation.isValid());
  REQUIRE(reportInteractionCounts.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederate.isValid());
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(reportClass));

  auto const requesterServerClass = requester->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const requesterEfficiency = requester->getAttributeHandle(
      requesterServerClass, L"Efficiency");
  auto const senderServerClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const senderEfficiency = sender->getAttributeHandle(
      senderServerClass, L"Efficiency");
  auto const takeOrder = sender->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const requesterTakeOrder = requester->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const reliable = sender->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const bestEffort = sender->getTransportationTypeHandle(
      standard_hla::mom::best_effort);
  REQUIRE(requesterServerClass.isValid());
  REQUIRE(requesterEfficiency.isValid());
  REQUIRE(senderServerClass.isValid());
  REQUIRE(senderEfficiency.isValid());
  REQUIRE(takeOrder.isValid());
  REQUIRE(requesterTakeOrder.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());

  InteractionClassHandleSet const directedClasses{takeOrder};
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      requesterServerClass, AttributeHandleSet{requesterEfficiency}));
  REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(
      senderServerClass, AttributeHandleSet{senderEfficiency}));
  REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
      senderServerClass, directedClasses));
  REQUIRE_NOTHROW(requester->subscribeObjectClassDirectedInteractions(
      requesterServerClass, InteractionClassHandleSet{requesterTakeOrder}));
  REQUIRE_NOTHROW(sender->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(requesterTakeOrder));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = requester->registerObjectInstance(requesterServerClass));
  REQUIRE(target.isValid());
  drainCallbacks(*sender);
  REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(senderReports.objectDiscoveryReports.front() == target);

  // An ordinary receive of the same interaction class belongs to the total
  // receive ledger, but must not enter the directed subset.
  REQUIRE_NOTHROW(sender->sendInteraction(
      takeOrder, ParameterHandleValueMap{}, VariableLengthData{}));
  REQUIRE(requesterReports.interactionReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.interactionReports.size() == 1U);
  REQUIRE(requesterReports.interactionReports.front().interactionClass ==
      requesterTakeOrder);
  REQUIRE(requesterReports.interactionReports.front().transportationType == reliable);
  REQUIRE(requesterReports.interactionReports.front().producingFederate ==
      senderFederate);
  REQUIRE(requesterReports.directedInteractionReports.empty());

  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      takeOrder,
      target,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  REQUIRE(requesterReports.directedInteractionReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.directedInteractionReports.size() == 1U);
  auto const& directed = requesterReports.directedInteractionReports.front();
  REQUIRE(directed.interactionClass == requesterTakeOrder);
  REQUIRE(directed.objectInstance == target);
  REQUIRE(directed.parameterValues.empty());
  REQUIRE(directed.userSuppliedTag.size() == 0U);
  REQUIRE(directed.transportationType == reliable);
  REQUIRE(directed.producingFederate == senderFederate);

  auto requestReportFor = [&](FederateHandle const& reportedFederate) {
    auto const before = requesterReports.interactionReports.size();
    REQUIRE_NOTHROW(requester->sendInteraction(
        requestClass,
        ParameterHandleValueMap{{requestFederate, reportedFederate.encode()}},
        VariableLengthData{}));
    REQUIRE(requesterReports.interactionReports.size() == before);
    drainCallbacks(*requester);
    return before;
  };

  auto const firstReport = requestReportFor(requesterFederate);
  REQUIRE(requesterReports.interactionReports.size() == firstReport + 2U);

  auto verifyReport = [&](std::size_t index,
                          TransportationTypeHandle const& expectedTransportation,
                          std::vector<DecodedInteractionCount> const& expectedCounts) {
    auto const& report = requesterReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 2U);
    REQUIRE(report.parameterValues.contains(reportTransportation));
    REQUIRE(report.parameterValues.contains(reportInteractionCounts));
    REQUIRE(report.transportationType == reliable);
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(report.sentRegionsSupplied);

    TransportationTypeHandle decodedTransportation;
    REQUIRE_NOTHROW(decodedTransportation = decodeTransportationTypeHandle(
        report.parameterValues.at(reportTransportation)));
    REQUIRE(decodedTransportation == expectedTransportation);
    std::vector<DecodedInteractionCount> decodedCounts;
    REQUIRE_NOTHROW(decodedCounts = decodeInteractionCounts(
        report.parameterValues.at(reportInteractionCounts)));
    REQUIRE(decodedCounts.size() == expectedCounts.size());
    for (auto const& expectedCount : expectedCounts) {
      auto const decoded = std::find_if(
          decodedCounts.begin(),
          decodedCounts.end(),
          [&](DecodedInteractionCount const& candidate) {
            return candidate.interactionClass == expectedCount.interactionClass;
          });
      REQUIRE(decoded != decodedCounts.end());
      REQUIRE(decoded->count == expectedCount.count);
    }
  };

  auto findReport = [&](std::size_t first,
                        TransportationTypeHandle const& expectedTransportation) {
    return std::find_if(
        requesterReports.interactionReports.begin() +
            static_cast<std::ptrdiff_t>(first),
        requesterReports.interactionReports.end(),
        [&](ReportingFederateAmbassador::InteractionReport const& report) {
          try {
            return decodeTransportationTypeHandle(
                       report.parameterValues.at(reportTransportation)) ==
                expectedTransportation;
          } catch (...) {
            return false;
          }
        });
  };

  auto const bestEffortReport = findReport(firstReport, bestEffort);
  auto const reliableReport = findReport(firstReport, reliable);
  REQUIRE(bestEffortReport != requesterReports.interactionReports.end());
  REQUIRE(reliableReport != requesterReports.interactionReports.end());
  verifyReport(
      static_cast<std::size_t>(bestEffortReport - requesterReports.interactionReports.begin()),
      bestEffort,
      {});
  verifyReport(
      static_cast<std::size_t>(reliableReport - requesterReports.interactionReports.begin()),
      reliable,
      {{requesterTakeOrder, 1}});

  auto const idleFirstReport = requestReportFor(idleFederate);
  REQUIRE(requesterReports.interactionReports.size() == idleFirstReport + 2U);
  for (std::size_t index = idleFirstReport;
       index != requesterReports.interactionReports.size();
       ++index) {
    TransportationTypeHandle decodedTransportation;
    REQUIRE_NOTHROW(decodedTransportation = decodeTransportationTypeHandle(
        requesterReports.interactionReports.at(index)
            .parameterValues.at(reportTransportation)));
    REQUIRE((decodedTransportation == reliable || decodedTransportation == bestEffort));
    verifyReport(index, decodedTransportation, {});
  }

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(idle->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(idle->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
