#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The order-type control tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
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

struct AttributeReflectionReport {
  OrderType sentOrderType = RECEIVE;
  OrderType receivedOrderType = RECEIVE;
  bool retractionSupplied = false;
  bool retractionValid = false;
};

struct TimestampedInteractionReport {
  OrderType sentOrderType = RECEIVE;
  OrderType receivedOrderType = RECEIVE;
  bool retractionSupplied = false;
  bool retractionValid = false;
};

class OrderControlFederateAmbassador final : public NullFederateAmbassador {
 public:
  void provideAttributeValueUpdate(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&,
      VariableLengthData const&) override {
    callbackNames.push_back("provideAttributeValueUpdate");
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&) override {
    callbackNames.push_back("turnUpdatesOnForObjectInstance");
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&,
      std::wstring const&) override {
    callbackNames.push_back("turnUpdatesOnForObjectInstance(rate)");
  }

  void startRegistrationForObjectClass(ObjectClassHandle const&) override {
    callbackNames.push_back("startRegistrationForObjectClass");
  }

  void turnInteractionsOn(rti1516_2025::InteractionClassHandle const&) override {
    callbackNames.push_back("turnInteractionsOn");
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      rti1516_2025::ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      RegionHandleSet const*) override {
    callbackNames.push_back("receiveInteraction");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      RegionHandleSet const*) override {
    attributeReflectionReports.push_back({RECEIVE, RECEIVE, false, false});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      RegionHandleSet const*,
      rti1516_2025::LogicalTime const&,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      rti1516_2025::ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      RegionHandleSet const*,
      rti1516_2025::LogicalTime const&,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractionReports.push_back({
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
  }

  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<std::string> callbackNames;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-order-type-control-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded order type control captures defaults, instance overrides, and publisher interaction order",
    "[integration][development-profile][federation-management][time-management]"
    "[object-management][order-type-control][change-attribute-order-type]"
    "[change-default-attribute-order-type][change-interaction-order-type]"
    "[rti.service.change-attribute-order-type]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.change-interaction-order-type][2025]") {
  OrderControlFederateAmbassador publisherReports;
  OrderControlFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(
          federationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      publisher->joinFederationExecution(
          L"order-control-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      receiver->joinFederationExecution(
          L"order-control-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(publisher->setInteractionRelevanceAdvisorySwitch(false));

  auto const server = publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = publisher->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  auto const takeOrder = publisher->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(takeOrder));

  auto drainTurnUpdatesOn = [&] {
    REQUIRE(publisher->evokeCallback(0.0));
    REQUIRE(publisherReports.callbackNames ==
            std::vector<std::string>{"turnUpdatesOnForObjectInstance"});
    REQUIRE_FALSE(publisher->evokeCallback(0.0));
    publisherReports.callbackNames.clear();
  };

  REQUIRE_THROWS_AS(
      publisher->changeDefaultAttributeOrderType(
          server, efficiencyOnly, static_cast<OrderType>(0x7f)),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      publisher->changeInteractionOrderType(
          takeOrder, static_cast<OrderType>(0x7f)),
      rti1516_2025::RTIinternalError);

  ObjectInstanceHandle fomDefaultObject;
  REQUIRE_NOTHROW(fomDefaultObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  // Registration admits the active subscription and therefore queues the
  // official Turn Updates On advisory on the publishing federate. Drain it
  // before testing order-specific delivery.
  drainTurnUpdatesOn();

  // The Restaurant FOM declares Efficiency TimeStamp. A prospective class
  // default change is captured only by later object registrations.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      server, efficiencyOnly, RECEIVE));
  ObjectInstanceHandle defaultReceiveObject;
  REQUIRE_NOTHROW(defaultReceiveObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  drainTurnUpdatesOn();

  // An explicit instance change affects future updates for that instance only.
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      defaultReceiveObject, efficiencyOnly, TIMESTAMP));
  ObjectInstanceHandle unchangedReceiveObject;
  REQUIRE_NOTHROW(unchangedReceiveObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  drainTurnUpdatesOn();

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  unsigned char const valueBytes[] = {0x4A};
  AttributeHandleValueMap values;
  values.emplace(efficiency, VariableLengthData(valueBytes, sizeof(valueBytes)));
  VariableLengthData const tag;
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      fomDefaultObject, values, tag, rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      defaultReceiveObject, values, tag, rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      unchangedReceiveObject, values, tag, rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  REQUIRE(receiverReports.attributeReflectionReports.size() == 1);
  REQUIRE(receiverReports.attributeReflectionReports.front().sentOrderType == RECEIVE);
  REQUIRE(receiverReports.attributeReflectionReports.front().receivedOrderType == RECEIVE);
  REQUIRE_FALSE(receiverReports.attributeReflectionReports.front().retractionSupplied);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 3);
  std::size_t timestampedReflectionCount = 0;
  for (auto const& report : receiverReports.attributeReflectionReports) {
    if (report.sentOrderType == TIMESTAMP) {
      ++timestampedReflectionCount;
      REQUIRE(report.receivedOrderType == TIMESTAMP);
      REQUIRE(report.retractionSupplied);
      REQUIRE(report.retractionValid);
    }
  }
  REQUIRE(timestampedReflectionCount == 2);

  // Interaction order is scoped to this publisher and changes future sends.
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(takeOrder, RECEIVE));
  auto const retraction = publisher->sendInteraction(
      takeOrder,
      rti1516_2025::ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE_FALSE(retraction.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  auto const& interactionReport = receiverReports.timestampedInteractionReports.front();
  REQUIRE(interactionReport.sentOrderType == RECEIVE);
  REQUIRE(interactionReport.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(interactionReport.retractionSupplied);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
