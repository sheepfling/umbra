#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped ownership-transfer test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-attribute-ownership-transfer-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
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

  struct AttributeOwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionReport final {
    enum class Kind { notification, unavailable };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    timeConstrainedEnabledReports.push_back(0);
  }

  void timeRegulationEnabled(LogicalTime const&) override {
    timeRegulationEnabledReports.push_back(0);
  }

  void timeAdvanceGrant(LogicalTime const&) override {
    timeAdvanceGrantReports.push_back(0);
    callbackOrder.push_back("grant");
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
    callbackOrder.push_back("assumption");
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
    callbackOrder.push_back("acquisition");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
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
    callbackOrder.push_back("reflect");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<int> timeConstrainedEnabledReports;
  std::vector<int> timeRegulationEnabledReports;
  std::vector<int> timeAdvanceGrantReports;
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

}  // namespace

TEST_CASE(
    "Embedded accepted timestamped attribute update survives ownership transfer before constrained grant",
    "[integration][development-profile][ownership-management][object-management][time-management]"
    "[timestamped-attribute-update][timestamped-attribute-update-ownership-transfer]"
    "[tso][in-flight-ownership]"
    "[rti.service.update-attribute-values][rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador acquirerReports;
  auto owner = makeRti();
  auto acquirer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  unsigned char const valueBytes[] = {0xBE, 0xEF};
  unsigned char const updateTagBytes[] = {0x54, 0x53, 0x4F, 0x2D, 0x4F, 0x57, 0x4E};
  unsigned char const divestitureTagBytes[] = {0x44, 0x49, 0x56};
  unsigned char const acquisitionTagBytes[] = {0x41, 0x43, 0x51};
  VariableLengthData const updateTag(updateTagBytes, sizeof(updateTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(acquirer->connect(acquirerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule.wstring(),
      standard_hla::mom::integer64_time));
  FederateHandle ownerFederate;
  REQUIRE_NOTHROW(ownerFederate = owner->joinFederationExecution(
      L"tso-ownership-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(acquirer->joinFederationExecution(
      L"tso-ownership-acquirer",
      L"publisher",
      federationName));

  auto const objectClass = owner->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const ownerAttribute = owner->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::efficiency);
  auto const acquirerAttribute = acquirer->getAttributeHandle(
      acquirer->getObjectClassHandle(fixture_hla::fom::employee_server),
      fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(acquirerAttribute.isValid());
  AttributeHandleSet const ownerAttributes{ownerAttribute};
  AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, ownerAttributes));
  REQUIRE_NOTHROW(acquirer->publishObjectClassAttributes(objectClass, acquirerAttributes));
  REQUIRE_NOTHROW(acquirer->subscribeObjectClassAttributes(objectClass, acquirerAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE_FALSE(acquirer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(acquirerReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(acquirer->enableTimeConstrained());
  REQUIRE_FALSE(acquirer->evokeCallback(0.0));
  REQUIRE(acquirerReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (ownerReports.timeRegulationEnabledReports.empty() &&
         owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(ownerAttribute, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = owner->updateAttributeValues(
      objectInstance,
      values,
      updateTag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(acquirerReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      ownerAttributes,
      divestitureTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, ownerAttribute));
  REQUIRE_FALSE(acquirer->isAttributeOwnedByFederate(objectInstance, acquirerAttribute));
  REQUIRE_FALSE(acquirer->evokeCallback(0.0));
  REQUIRE(acquirerReports.attributeOwnershipAssumptionReports.size() == 1U);
  auto const& assumption = acquirerReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == acquirerAttributes);
  REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));

  REQUIRE_NOTHROW(acquirer->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      acquirerAttributes,
      acquisitionTag));
  REQUIRE_FALSE(acquirer->evokeCallback(0.0));
  REQUIRE(acquirerReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& acquisition = acquirerReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(acquisition.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(acquisition.objectInstance == objectInstance);
  REQUIRE(acquisition.attributes == acquirerAttributes);
  REQUIRE(variableLengthDataBytes(acquisition.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(acquirer->isAttributeOwnedByFederate(objectInstance, acquirerAttribute));

  REQUIRE_NOTHROW(acquirer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (ownerReports.timeAdvanceGrantReports.empty() &&
         owner->evokeCallback(0.0)) {
  }
  while (acquirerReports.timeAdvanceGrantReports.empty() &&
         acquirer->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(acquirerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(acquirerReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = acquirerReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(acquirerAttribute));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(acquirerAttribute)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(
              updateTagBytes,
              updateTagBytes + sizeof(updateTagBytes)));
  REQUIRE(reflection.producingFederate == ownerFederate);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"5");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(variableLengthDataBytes(reflection.encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(acquirer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(acquirer->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
