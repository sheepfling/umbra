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
#error "The joined-federate MOM deletable-object report tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandleSet;
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
  return L"federation-joined-mom-object-instances-that-can-be-deleted-report-" +
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
  // HLAobjectClassBasedCount fixed records. Decode with the standard binding
  // data elements and the official nested ObjectClassHandle decoder.
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
    "Embedded MOM requestObjectInstancesThatCanBeDeleted reports live owner counts",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][object-management][interaction-management]"
    "[ownership-management][callback-model]"
    "[joined-federate-mom-object-instances-that-can-be-deleted-report]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.delete-object-instance]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador ownerReports;
  auto requester = makeRti();
  auto owner = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, restaurantFom().wstring(), standard_hla::mom::integer64_time));

  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"mom-object-instances-that-can-be-deleted-requester",
      L"requester",
      federationName));
  FederateHandle ownerFederate;
  REQUIRE_NOTHROW(ownerFederate = owner->joinFederationExecution(
      L"mom-object-instances-that-can-be-deleted-owner", L"owner", federationName));

  auto const reportClass = requester->getInteractionClassHandle(
      standard_hla::mom::report_object_instances_that_can_be_deleted);
  auto const reportCounts = requester->getParameterHandle(
      reportClass, standard_hla::mom::object_instance_counts);
  auto const requestClass = requester->getInteractionClassHandle(
      standard_hla::mom::request_object_instances_that_can_be_deleted);
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

  auto const serverClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const serverEfficiency = owner->getAttributeHandle(
      serverClass, L"Efficiency");
  auto const sodaClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const sodaFlavor = owner->getAttributeHandle(sodaClass, L"Flavor");
  REQUIRE(serverClass.isValid());
  REQUIRE(serverEfficiency.isValid());
  REQUIRE(sodaClass.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      serverClass, AttributeHandleSet{serverEfficiency}));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      sodaClass, AttributeHandleSet{sodaFlavor}));

  auto const serverObject = owner->registerObjectInstance(serverClass);
  auto const sodaObject = owner->registerObjectInstance(sodaClass);
  REQUIRE(serverObject.isValid());
  REQUIRE(sodaObject.isValid());

  auto requestReport = [&] {
    auto const before = requesterReports.interactionReports.size();
    REQUIRE_NOTHROW(requester->sendInteraction(
        requestClass,
        ParameterHandleValueMap{{requestFederate, ownerFederate.encode()}},
        VariableLengthData{}));
    // The report is RTI-originated and remains behind the requester's
    // HLA_EVOKED callback boundary until the explicit drain.
    REQUIRE(requesterReports.interactionReports.size() == before);
    drainCallbacks(*requester);
    REQUIRE(requesterReports.interactionReports.size() == before + 1U);
    auto const& report = requesterReports.interactionReports.back();
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(reportCounts));
    REQUIRE(report.transportationType == reliable);
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    return decodeObjectClassCounts(report.parameterValues.at(reportCounts));
  };

  // Both federate-owned objects are live and therefore retain the implicit
  // HLAprivilegeToDeleteObject ownership in the first report.
  auto counts = requestReport();
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

  // Accepted deletion removes only the deleted class from the live owner
  // ledger; the remaining Soda object stays countable.
  REQUIRE_NOTHROW(owner->deleteObjectInstance(serverObject, VariableLengthData{}));
  counts = requestReport();
  REQUIRE(counts.size() == 1U);
  REQUIRE(counts.front().objectClass == sodaClass);
  REQUIRE(counts.front().count == 1);

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
