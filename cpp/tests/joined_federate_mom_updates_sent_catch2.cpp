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
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM updates-sent tests require the Umbra source directory."
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
  return L"federation-joined-mom-updates-sent-" +
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

struct DecodedObjectClassCount final {
  ObjectClassHandle objectClass;
  std::int32_t count = 0;
};

std::vector<DecodedObjectClassCount> decodeObjectClassCounts(
    VariableLengthData const& encodedValue) {
  // HLAobjectClassBasedCounts is the official 2025 MIM variable array of
  // HLAobjectClassBasedCount fixed records.  Decode through the public
  // standard data-element classes, then use the binding helper only for the
  // nested official ObjectClassHandle value.
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
    auto const objectClass = decodeObjectClassHandle(encodedObjectClass.encode());
    auto const count = dynamic_cast<HLAinteger32BE const&>(record.get(1)).get();
    decoded.push_back({objectClass, count});
  }
  return decoded;
}

}  // namespace

TEST_CASE(
    "Embedded MOM requestUpdatesSent reports class and transportation counts",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][object-management][interaction-management]"
    "[transportation-management][callback-model]"
    "[joined-federate-mom-updates-sent]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.change-default-attribute-transportation-type]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]") {
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

  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"mom-updates-sent-requester", L"requester", federationName));
  FederateHandle senderFederate;
  REQUIRE_NOTHROW(senderFederate = sender->joinFederationExecution(
      L"mom-updates-sent-sender", L"sender", federationName));
  FederateHandle idleFederate;
  REQUIRE_NOTHROW(idleFederate = idle->joinFederationExecution(
      L"mom-updates-sent-idle", L"idle", federationName));

  auto const reportClass = requester->getInteractionClassHandle(
      standard_hla::mom::report_updates_sent);
  auto const reportTransportation = requester->getParameterHandle(
      reportClass, standard_hla::mom::transportation);
  auto const reportUpdateCounts = requester->getParameterHandle(
      reportClass, L"HLAupdateCounts");
  auto const requestClass = requester->getInteractionClassHandle(
      standard_hla::mom::request_updates_sent);
  auto const requestFederate = requester->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTransportation.isValid());
  REQUIRE(reportUpdateCounts.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederate.isValid());
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(reportClass));

  auto const serverClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const serverEfficiency = sender->getAttributeHandle(serverClass, L"Efficiency");
  auto const sodaClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const sodaFlavor = sender->getAttributeHandle(sodaClass, L"Flavor");
  auto const reliable = sender->getTransportationTypeHandle(standard_hla::mom::reliable);
  auto const bestEffort = sender->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(serverClass.isValid());
  REQUIRE(serverEfficiency.isValid());
  REQUIRE(sodaClass.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());

  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      serverClass, AttributeHandleSet{serverEfficiency}));
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      sodaClass, AttributeHandleSet{sodaFlavor}));
  REQUIRE_NOTHROW(sender->changeDefaultAttributeTransportationType(
      serverClass, AttributeHandleSet{serverEfficiency}, bestEffort));
  auto const serverObject = sender->registerObjectInstance(serverClass);
  auto const sodaObject = sender->registerObjectInstance(sodaClass);
  REQUIRE(serverObject.isValid());
  REQUIRE(sodaObject.isValid());

  // The sender ledger is counted at the accepted Update Attribute Values
  // boundary.  Server's default is changed to best effort, while Soda keeps
  // its Restaurant-FOM reliable default.
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      serverObject,
      AttributeHandleValueMap{{serverEfficiency, HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      serverObject,
      AttributeHandleValueMap{{serverEfficiency, HLAinteger32BE{2}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      sodaObject,
      AttributeHandleValueMap{{sodaFlavor, HLAinteger32BE{101}.encode()}},
      VariableLengthData{}));

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

  auto const firstReport = requestReportFor(senderFederate);
  REQUIRE(requesterReports.interactionReports.size() == firstReport + 2U);

  auto verifyReport = [&](std::size_t index,
                          TransportationTypeHandle const& expectedTransportation,
                          ObjectClassHandle const* expectedObjectClass,
                          std::int32_t expectedCount) {
    auto const& report = requesterReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 2U);
    REQUIRE(report.parameterValues.contains(reportTransportation));
    REQUIRE(report.parameterValues.contains(reportUpdateCounts));
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
        report.parameterValues.at(reportUpdateCounts)));
    if (expectedObjectClass == nullptr) {
      REQUIRE(decodedCounts.empty());
    } else {
      REQUIRE(decodedCounts.size() == 1U);
      REQUIRE(decodedCounts.front().objectClass == *expectedObjectClass);
      REQUIRE(decodedCounts.front().count == expectedCount);
    }
  };

  // Both supported transportation buckets are emitted even though the
  // accepted ledger contains two best-effort Server updates and one reliable
  // Soda update.  The RTI-originated report itself remains reliable.
  auto const bestEffortReport = std::find_if(
      requesterReports.interactionReports.begin() +
          static_cast<std::ptrdiff_t>(firstReport),
      requesterReports.interactionReports.end(),
      [&](ReportingFederateAmbassador::InteractionReport const& report) {
        TransportationTypeHandle decoded;
        try {
          decoded = decodeTransportationTypeHandle(report.parameterValues.at(reportTransportation));
        } catch (...) {
          return false;
        }
        return decoded == bestEffort;
      });
  auto const reliableReport = std::find_if(
      requesterReports.interactionReports.begin() +
          static_cast<std::ptrdiff_t>(firstReport),
      requesterReports.interactionReports.end(),
      [&](ReportingFederateAmbassador::InteractionReport const& report) {
        TransportationTypeHandle decoded;
        try {
          decoded = decodeTransportationTypeHandle(report.parameterValues.at(reportTransportation));
        } catch (...) {
          return false;
        }
        return decoded == reliable;
      });
  REQUIRE(bestEffortReport != requesterReports.interactionReports.end());
  REQUIRE(reliableReport != requesterReports.interactionReports.end());
  verifyReport(
      static_cast<std::size_t>(bestEffortReport - requesterReports.interactionReports.begin()),
      bestEffort,
      &serverClass,
      2);
  verifyReport(
      static_cast<std::size_t>(reliableReport - requesterReports.interactionReports.begin()),
      reliable,
      &sodaClass,
      1);

  // A second request against an idle joined federate proves the required
  // NULL response shape: one empty HLAobjectClassBasedCounts array per
  // supported transportation, with no invented class entries.
  auto const idleFirstReport = requestReportFor(idleFederate);
  REQUIRE(requesterReports.interactionReports.size() == idleFirstReport + 2U);
  for (std::size_t index = idleFirstReport;
       index != requesterReports.interactionReports.size();
       ++index) {
    TransportationTypeHandle expectedTransportation;
    REQUIRE_NOTHROW(expectedTransportation = decodeTransportationTypeHandle(
        requesterReports.interactionReports.at(index)
            .parameterValues.at(reportTransportation)));
    REQUIRE((expectedTransportation == reliable || expectedTransportation == bestEffort));
    verifyReport(index, expectedTransportation, nullptr, 0);
  }

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(idle->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(idle->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
