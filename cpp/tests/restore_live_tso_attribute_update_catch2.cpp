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
#error "The save/restore attribute tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
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
  return L"restore-live-tso-attribute-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  void federationSaved() override {
    ++federationSavedReportCount;
    callbackOrder.push_back("save-complete");
  }

  void federationRestored() override {
    ++federationRestoredReportCount;
    callbackOrder.push_back("restore-complete");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    reflections.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractions.push_back({retraction.isValid(), retraction.encode()});
    callbackOrder.push_back("request-retraction");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrants.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  std::vector<ReflectionReport> reflections;
  std::vector<RequestRetractionReport> requestRetractions;
  std::vector<FlushQueueGrantReport> flushQueueGrants;
  std::vector<std::string> callbackOrder;
  std::size_t federationSavedReportCount = 0U;
  std::size_t federationRestoredReportCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded federation restore restores a saved live timestamped attribute update",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][tso]"
    "[timestamped-attribute-update][timestamped-attribute-update-restore]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.register-object-instance]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.update-attribute-values]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x55, 0x4E, 0x52, 0x2D, 0x41, 0x54, 0x54};
  unsigned char const tagBytes[] = {0x55, 0x4E, 0x52, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"restore-live-tso-attribute-save";
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-live-tso-attribute-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-live-tso-attribute-receiver",
      L"subscriber",
      federationName));

  auto const publisherSoda = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const receiverSoda = receiver->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const publisherFlavor = publisher->getAttributeHandle(
      publisherSoda,
      fixture_hla::fixture::flavor);
  auto const receiverFlavor = receiver->getAttributeHandle(
      receiverSoda,
      fixture_hla::fixture::flavor);
  REQUIRE(publisherSoda.isValid());
  REQUIRE(receiverSoda.isValid());
  REQUIRE(publisherFlavor.isValid());
  REQUIRE(receiverFlavor.isValid());
  AttributeHandleSet const publisherAttributes{publisherFlavor};
  AttributeHandleSet const receiverAttributes{receiverFlavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherSoda,
      publisherAttributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      publisherSoda,
      publisherAttributes,
      TIMESTAMP));

  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverSoda,
      receiverAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(
      publisherSoda));
  REQUIRE(objectInstance.isValid());
  drain(*receiver);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*publisher);

  AttributeHandleValueMap values{{publisherFlavor, value}};
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.reflections.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  drain(*receiver);
  drain(*publisher);
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  drain(*publisher);
  drain(*receiver);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.reflections.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*receiver);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  drain(*publisher);
  drain(*receiver);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.reflections.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrants.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});

  auto const& reflection = receiverReports.reflections.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(receiverFlavor));
  REQUIRE(variableLengthDataBytes(
              reflection.attributeValues.at(receiverFlavor)) ==
          std::vector<unsigned char>(
              valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.producingFederate == publisherHandle);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"7");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE_FALSE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions.empty());
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(receiverReports.flushQueueGrants.front().timeImplementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(receiverReports.flushQueueGrants.front().value == L"5");
  REQUIRE(receiverReports.flushQueueGrants.front().optimisticValue == L"7");

  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractions.size() == 1U);
  REQUIRE(receiverReports.requestRetractions.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractions.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"request-retraction"});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      receiverSoda,
      receiverAttributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
