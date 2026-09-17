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
#error "The joined-federate MOM interactions-received tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLAfixedRecord;
using rti1516_2025::HLAinteger32BE;
using rti1516_2025::HLAoctet;
using rti1516_2025::HLAvariableArray;
using rti1516_2025::HLAvariableArrayT;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::NO_ACTION;
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
  return L"federation-joined-mom-interactions-received-" +
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

  void confirmInteractionTransportationTypeChange(
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    transportationChanges.emplace_back(interactionClass, transportationType);
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

  std::vector<std::pair<InteractionClassHandle, TransportationTypeHandle>>
      transportationChanges;
  std::vector<InteractionReport> interactionReports;
};

struct DecodedInteractionCount final {
  InteractionClassHandle interactionClass;
  std::int32_t count = 0;
};

std::vector<DecodedInteractionCount> decodeInteractionCounts(
    VariableLengthData const& encodedValue) {
  // HLAinteractionCounts is the official 2025 MIM variable array of
  // HLAinteractionCount fixed records. Decode it with the standard data
  // elements and the published handle codec, never a private byte layout.
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
    "Embedded MOM requestInteractionsReceived reports class and transportation counts",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][interaction-management][transportation-management]"
    "[callback-model][joined-federate-mom-interactions-received]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
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
      L"mom-interactions-received-requester", L"requester", federationName));
  FederateHandle senderFederate;
  REQUIRE_NOTHROW(senderFederate = sender->joinFederationExecution(
      L"mom-interactions-received-sender", L"sender", federationName));
  FederateHandle idleFederate;
  REQUIRE_NOTHROW(idleFederate = idle->joinFederationExecution(
      L"mom-interactions-received-idle", L"idle", federationName));
  REQUIRE(requesterFederate.isValid());
  REQUIRE(senderFederate.isValid());
  REQUIRE(idleFederate.isValid());

  auto const reportClass = requester->getInteractionClassHandle(
      standard_hla::mom::report_interactions_received);
  auto const reportTransportation = requester->getParameterHandle(
      reportClass, standard_hla::mom::transportation);
  auto const reportInteractionCounts = requester->getParameterHandle(
      reportClass, L"HLAinteractionCounts");
  auto const requestClass = requester->getInteractionClassHandle(
      standard_hla::mom::request_interactions_received);
  auto const requestFederate = requester->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTransportation.isValid());
  REQUIRE(reportInteractionCounts.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederate.isValid());
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(reportClass));

  auto const takeOrder = sender->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const requesterTakeOrder = requester->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const reliable = sender->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const bestEffort = sender->getTransportationTypeHandle(
      standard_hla::mom::best_effort);
  REQUIRE(takeOrder.isValid());
  REQUIRE(requesterTakeOrder.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(requesterTakeOrder));

  // The receive ledger is defined at the accepted application callback
  // boundary. Produce one receive in each standard transportation bucket and
  // drain the requester before asking for its report.
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
  REQUIRE(requesterReports.interactionReports.front().userSuppliedTag.size() == 0U);
  REQUIRE_FALSE(requesterReports.interactionReports.front().sentRegionsSupplied);

  REQUIRE_NOTHROW(sender->requestInteractionTransportationTypeChange(
      takeOrder, bestEffort));
  REQUIRE(senderReports.transportationChanges.empty());
  drainCallbacks(*sender);
  REQUIRE(senderReports.transportationChanges.size() == 1U);
  REQUIRE(senderReports.transportationChanges.front().first == takeOrder);
  REQUIRE(senderReports.transportationChanges.front().second == bestEffort);
  REQUIRE_NOTHROW(sender->sendInteraction(
      takeOrder, ParameterHandleValueMap{}, VariableLengthData{}));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.interactionReports.size() == 2U);
  REQUIRE(requesterReports.interactionReports.back().interactionClass ==
      requesterTakeOrder);
  REQUIRE(requesterReports.interactionReports.back().transportationType == bestEffort);
  REQUIRE(requesterReports.interactionReports.back().producingFederate ==
      senderFederate);
  REQUIRE(requesterReports.interactionReports.back().userSuppliedTag.size() == 0U);
  REQUIRE_FALSE(requesterReports.interactionReports.back().sentRegionsSupplied);

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
      {{requesterTakeOrder, 1}});
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

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(idle->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(idle->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
