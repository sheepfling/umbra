#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped source-resignation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-attribute-source-resignation-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct AttributeOwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionReport final {
    enum class Kind {
      notification,
      unavailable,
    };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  void timeAdvanceGrant(LogicalTime const&) override {
    timeAdvanceGrantReports.push_back(0);
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(LogicalTime const&) override {
    timeRegulationEnabledReports.push_back(0);
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    timeConstrainedEnabledReports.push_back(0);
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAssumptionReports.push_back({
        objectInstance,
        offeredAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
        optionalRetraction != nullptr ? optionalRetraction->encode()
                                       : VariableLengthData{},
    });
  }

  std::vector<int> timeAdvanceGrantReports;
  std::vector<int> timeRegulationEnabledReports;
  std::vector<int> timeConstrainedEnabledReports;
  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeOwnershipAssumptionReport>
      attributeOwnershipAssumptionReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<std::string> callbackOrder;
};

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

}  // namespace

TEST_CASE(
    "Embedded queued timestamped attribute update survives source resignation",
    "[integration][development-profile][federation-management][ownership-management][object-management][time-management]"
    "[timestamped-attribute-update][tso][resignation][in-flight-ownership]"
    "[rti.service.update-attribute-values][rti.service.resign-federation-execution]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador survivorReports;
  ReportingFederateAmbassador clockReports;
  auto owner = makeRti();
  auto survivor = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::vector<unsigned char> const valueBytes{0xCAU, 0xFEU};
  std::vector<unsigned char> const updateTagBytes{
      0x52U, 0x45U, 0x53U, 0x49U, 0x47U, 0x4EU};
  std::vector<unsigned char> const acquisitionTagBytes{
      0x41U, 0x43U, 0x51U, 0x2DU, 0x52U};
  VariableLengthData const updateTag(updateTagBytes.data(), updateTagBytes.size());
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(survivor->connect(survivorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle ownerFederate;
  REQUIRE_NOTHROW(ownerFederate = owner->joinFederationExecution(
      L"tso-resigning-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(survivor->joinFederationExecution(
      L"tso-resigning-survivor",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"tso-resigning-clock",
      L"publisher",
      federationName));

  auto const objectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const ownerAttribute = owner->getAttributeHandle(objectClass, L"Efficiency");
  auto const survivorObjectClass = survivor->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const survivorAttribute = survivor->getAttributeHandle(
      survivorObjectClass,
      L"Efficiency");
  auto const survivorPrivilegeToDelete = survivor->getAttributeHandle(
      survivorObjectClass,
      standard_hla::mom::privilege_to_delete_object);
  REQUIRE(objectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(survivorAttribute.isValid());
  REQUIRE(survivorPrivilegeToDelete.isValid());
  AttributeHandleSet const ownerAttributes{ownerAttribute};
  AttributeHandleSet const survivorPublishedAttributes{
      survivorAttribute,
      survivorPrivilegeToDelete};
  AttributeHandleSet const survivorAttributes{
      survivorAttribute,
      survivorPrivilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, ownerAttributes));
  REQUIRE_NOTHROW(survivor->publishObjectClassAttributes(
      survivorObjectClass,
      survivorPublishedAttributes));
  REQUIRE_NOTHROW(survivor->subscribeObjectClassAttributes(
      survivorObjectClass,
      AttributeHandleSet{survivorAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  drainCallbacks(*survivor);
  REQUIRE(survivorReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(survivor->enableTimeConstrained());
  drainCallbacks(*survivor);
  REQUIRE(survivorReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*clock);
  REQUIRE(clockReports.timeRegulationEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(
      ownerAttribute,
      VariableLengthData(valueBytes.data(), valueBytes.size()));
  auto const retraction = owner->updateAttributeValues(
      objectInstance,
      values,
      updateTag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(survivorReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  drainCallbacks(*survivor);
  REQUIRE(survivorReports.attributeOwnershipAssumptionReports.size() == 1U);
  auto const& assumption = survivorReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == survivorAttributes);

  REQUIRE_NOTHROW(survivor->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      survivorAttributes,
      acquisitionTag));
  drainCallbacks(*survivor);
  REQUIRE(survivorReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& acquisition = survivorReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(acquisition.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(acquisition.objectInstance == objectInstance);
  REQUIRE(acquisition.attributes == survivorAttributes);
  REQUIRE(variableLengthDataBytes(acquisition.userSuppliedTag) == acquisitionTagBytes);
  REQUIRE(survivor->isAttributeOwnedByFederate(objectInstance, survivorAttribute));

  // The source regulator has resigned, so an independent surviving regulator
  // supplies the federation-wide temporal frontier for this queued passel.
  REQUIRE_NOTHROW(survivor->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*clock);
  drainCallbacks(*survivor);
  REQUIRE(clockReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(survivorReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(survivorReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = survivorReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(survivorAttribute));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(survivorAttribute)) ==
          valueBytes);
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == updateTagBytes);
  REQUIRE(reflection.producingFederate == ownerFederate);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"5");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(variableLengthDataBytes(reflection.encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));

  REQUIRE_NOTHROW(survivor->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(survivor->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
