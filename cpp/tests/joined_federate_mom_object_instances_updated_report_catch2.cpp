#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "internal/handles/object_class_handle.hpp"

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
#error "The joined-federate MOM object-instances-updated report tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

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
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

using rti1516_2025::umbra_binding_detail::decodeObjectClassHandle;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-joined-mom-object-instances-updated-report-" +
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
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
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
  // HLAobjectClassBasedCount fixed records. Decode through the standard
  // binding data-element classes, then use the official handle decoder for
  // the nested ObjectClassHandle value.
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
    "Embedded MOM requestObjectInstancesUpdated reports class-grouped counts",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][object-management][interaction-management]"
    "[callback-model][joined-federate-mom-object-instances-updated-report]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador senderReports;
  auto requester = makeRti();
  auto sender = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, restaurantFom().wstring(), standard_hla::mom::integer64_time));

  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"mom-object-instances-updated-requester", L"requester", federationName));
  FederateHandle senderFederate;
  REQUIRE_NOTHROW(senderFederate = sender->joinFederationExecution(
      L"mom-object-instances-updated-sender", L"sender", federationName));

  auto const reportClass = requester->getInteractionClassHandle(
      standard_hla::mom::report_object_instances_updated);
  auto const reportCounts = requester->getParameterHandle(
      reportClass, standard_hla::mom::object_instance_counts);
  auto const requestClass = requester->getInteractionClassHandle(
      standard_hla::mom::request_object_instances_updated);
  auto const requestFederate = requester->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  auto const reliable = requester->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportCounts.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederate.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(reportClass));

  auto const serverClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const serverEfficiency = sender->getAttributeHandle(
      serverClass, L"Efficiency");
  auto const sodaClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const sodaFlavor = sender->getAttributeHandle(sodaClass, L"Flavor");
  REQUIRE(serverClass.isValid());
  REQUIRE(serverEfficiency.isValid());
  REQUIRE(sodaClass.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      serverClass, rti1516_2025::AttributeHandleSet{serverEfficiency}));
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      sodaClass, rti1516_2025::AttributeHandleSet{sodaFlavor}));

  auto const serverObject = sender->registerObjectInstance(serverClass);
  auto const sodaObject = sender->registerObjectInstance(sodaClass);
  REQUIRE(serverObject.isValid());
  REQUIRE(sodaObject.isValid());

  // The report is a joined-lifetime ledger grouped by registered object
  // class. Repeating one object's update must not create a second class
  // entry or inflate the distinct-object count.
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      serverObject,
      rti1516_2025::AttributeHandleValueMap{{
          serverEfficiency, HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      serverObject,
      rti1516_2025::AttributeHandleValueMap{{
          serverEfficiency, HLAinteger32BE{2}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      sodaObject,
      rti1516_2025::AttributeHandleValueMap{{
          sodaFlavor, HLAinteger32BE{101}.encode()}},
      VariableLengthData{}));

  auto const beforeRequest = requesterReports.interactionReports.size();
  REQUIRE_NOTHROW(requester->sendInteraction(
      requestClass,
      ParameterHandleValueMap{{requestFederate, senderFederate.encode()}},
      VariableLengthData{}));
  // HLA_EVOKED keeps the RTI-originated report behind the callback boundary.
  REQUIRE(requesterReports.interactionReports.size() == beforeRequest);
  drainCallbacks(*requester);
  REQUIRE(requesterReports.interactionReports.size() == beforeRequest + 1U);

  auto const& report = requesterReports.interactionReports.back();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(reportCounts));
  REQUIRE(report.transportationType == reliable);
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE_FALSE(report.sentRegionsSupplied);

  auto const counts = decodeObjectClassCounts(
      report.parameterValues.at(reportCounts));
  REQUIRE(counts.size() == 2U);
  auto const serverCount = std::find_if(
      counts.begin(), counts.end(), [&](DecodedObjectClassCount const& value) {
        return value.objectClass == serverClass;
      });
  auto const sodaCount = std::find_if(
      counts.begin(), counts.end(), [&](DecodedObjectClassCount const& value) {
        return value.objectClass == sodaClass;
      });
  REQUIRE(serverCount != counts.end());
  REQUIRE(sodaCount != counts.end());
  REQUIRE(serverCount->count == 1);
  REQUIRE(sodaCount->count == 1);

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
