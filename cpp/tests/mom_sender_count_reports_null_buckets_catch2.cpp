#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "internal/handles/transportation_type_handle.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The MOM sender-count NULL-bucket tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLAfixedRecord;
using rti1516_2025::HLAinteger32BE;
using rti1516_2025::HLAoctet;
using rti1516_2025::HLAvariableArray;
using rti1516_2025::HLAvariableArrayT;
using rti1516_2025::FederateHandle;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::NO_ACTION;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

using rti1516_2025::umbra_binding_detail::decodeTransportationTypeHandle;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-mom-sender-count-reports-null-buckets-" +
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

  std::vector<InteractionReport> interactionReports;
};

void requireEmptyNestedCountArray(VariableLengthData const& encodedValue) {
  HLAvariableArrayT<HLAoctet> handlePrototype;
  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(handlePrototype)
      .appendElement(HLAinteger32BE{});
  HLAvariableArray counts{recordPrototype};
  counts.decode(encodedValue);
  REQUIRE(counts.size() == 0U);
}

}  // namespace

TEST_CASE(
    "Embedded MOM sender count reports emit NULL buckets for empty ledgers",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][object-management][interaction-management][directed]"
    "[transportation-management][callback-model]"
    "[mom-sender-count-reports-null-buckets]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador idleReports;
  auto requester = makeRti();
  auto idle = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(idle->connect(idleReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, restaurantFom().wstring(), standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"mom-sender-count-requester", L"requester", federationName));
  FederateHandle idleFederate;
  REQUIRE_NOTHROW(idleFederate = idle->joinFederationExecution(
      L"mom-sender-count-idle", L"idle", federationName));

  struct ReportFamily final {
    wchar_t const* requestName;
    wchar_t const* reportName;
    wchar_t const* countParameterName;
  };
  std::vector<ReportFamily> const families{
      {standard_hla::mom::request_updates_sent,
       standard_hla::mom::report_updates_sent,
       L"HLAupdateCounts"},
      {standard_hla::mom::request_interactions_sent,
       standard_hla::mom::report_interactions_sent,
       L"HLAinteractionCounts"},
      {standard_hla::mom::request_directed_interactions_sent,
       standard_hla::mom::report_directed_interactions_sent,
       L"HLAinteractionCounts"},
  };
  struct ResolvedFamily final {
    InteractionClassHandle requestClass;
    InteractionClassHandle reportClass;
    rti1516_2025::ParameterHandle requestFederate;
    rti1516_2025::ParameterHandle reportTransportation;
    rti1516_2025::ParameterHandle reportCounts;
  };
  std::vector<ResolvedFamily> resolved;
  resolved.reserve(families.size());
  for (auto const& family : families) {
    auto const requestClass = requester->getInteractionClassHandle(family.requestName);
    auto const reportClass = requester->getInteractionClassHandle(family.reportName);
    auto const requestFederate = requester->getParameterHandle(
        requestClass, standard_hla::mom::federate);
    auto const reportTransportation = requester->getParameterHandle(
        reportClass, standard_hla::mom::transportation);
    auto const reportCounts = requester->getParameterHandle(
        reportClass, family.countParameterName);
    REQUIRE(requestClass.isValid());
    REQUIRE(reportClass.isValid());
    REQUIRE(requestFederate.isValid());
    REQUIRE(reportTransportation.isValid());
    REQUIRE(reportCounts.isValid());
    REQUIRE_NOTHROW(requester->subscribeInteractionClass(reportClass));
    resolved.push_back({
        requestClass,
        reportClass,
        requestFederate,
        reportTransportation,
        reportCounts,
    });
  }

  auto const reliable = requester->getTransportationTypeHandle(standard_hla::mom::reliable);
  auto const bestEffort = requester->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());

  for (auto const& family : resolved) {
    auto const before = requesterReports.interactionReports.size();
    REQUIRE_NOTHROW(requester->sendInteraction(
        family.requestClass,
        ParameterHandleValueMap{{family.requestFederate, idleFederate.encode()}},
        VariableLengthData{}));
    REQUIRE(requesterReports.interactionReports.size() == before);
    drainCallbacks(*requester);
    REQUIRE(requesterReports.interactionReports.size() == before + 2U);

    for (std::size_t index = before;
         index != requesterReports.interactionReports.size();
         ++index) {
      auto const& report = requesterReports.interactionReports.at(index);
      REQUIRE(report.interactionClass == family.reportClass);
      REQUIRE(report.parameterValues.size() == 2U);
      REQUIRE(report.parameterValues.contains(family.reportTransportation));
      REQUIRE(report.parameterValues.contains(family.reportCounts));
      REQUIRE(report.transportationType == reliable);
      REQUIRE_FALSE(report.producingFederate.isValid());
      REQUIRE(report.userSuppliedTag.size() == 0U);
      REQUIRE_FALSE(report.sentRegionsSupplied);

      TransportationTypeHandle decodedTransportation;
      REQUIRE_NOTHROW(decodedTransportation = decodeTransportationTypeHandle(
          report.parameterValues.at(family.reportTransportation)));
      REQUIRE((decodedTransportation == reliable || decodedTransportation == bestEffort));
      REQUIRE_NOTHROW(requireEmptyNestedCountArray(
          report.parameterValues.at(family.reportCounts)));
    }
  }

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(idle->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(idle->disconnect());
}
