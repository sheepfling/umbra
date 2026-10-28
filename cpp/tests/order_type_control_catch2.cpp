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
  ObjectInstanceHandle object;
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
      ObjectInstanceHandle const& object,
      AttributeHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      RegionHandleSet const*) override {
    attributeReflectionReports.push_back({object, RECEIVE, RECEIVE, false, false});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& object,
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
        object,
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
    "Embedded order type control captures per-federate defaults, instance overrides, and publisher interaction order",
    "[integration][development-profile][federation-management][time-management]"
    "[object-management][order-type-control][change-attribute-order-type]"
    "[change-default-attribute-order-type][change-interaction-order-type]"
    "[rti.service.change-attribute-order-type]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.change-interaction-order-type]"
    "[requirement:hla-1516.1-2025:clause-8.1.4][2025]") {
  OrderControlFederateAmbassador publisherReports;
  OrderControlFederateAmbassador independentPublisherReports;
  OrderControlFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto independentPublisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(independentPublisher->connect(
      independentPublisherReports, HLA_EVOKED));
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
      independentPublisher->joinFederationExecution(
          L"order-control-independent-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      receiver->joinFederationExecution(
          L"order-control-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(publisher->setInteractionRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(
      independentPublisher->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(
      independentPublisher->setInteractionRelevanceAdvisorySwitch(false));

  auto const server = publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = publisher->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  auto const independentServer = independentPublisher->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const independentEfficiency = independentPublisher->getAttributeHandle(
      independentServer, fixture_hla::fixture::efficiency);
  REQUIRE(independentServer == server);
  REQUIRE(independentEfficiency == efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const independentEfficiencyOnly{independentEfficiency};
  auto const takeOrder = publisher->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(independentPublisher->publishObjectClassAttributes(
      independentServer, independentEfficiencyOnly));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(takeOrder));

  auto drainTurnUpdatesOn = [&](RTIambassador& ambassador,
                                OrderControlFederateAmbassador& reports) {
    REQUIRE(ambassador.evokeCallback(0.0));
    REQUIRE(reports.callbackNames ==
            std::vector<std::string>{"turnUpdatesOnForObjectInstance"});
    REQUIRE_FALSE(ambassador.evokeCallback(0.0));
    reports.callbackNames.clear();
  };
  auto evokeUntil = [](auto& ambassador, auto&& ready) {
    for (int pass = 0; pass != 128 && !ready(); ++pass) {
      static_cast<void>(ambassador.evokeCallback(0.01));
    }
    REQUIRE(ready());
  };

  REQUIRE_THROWS_AS(
      publisher->changeDefaultAttributeOrderType(
          server, efficiencyOnly, static_cast<OrderType>(0x7f)),
      rti1516_2025::RTIinternalError);
  ObjectClassHandle const undefinedObjectClass;
  REQUIRE_THROWS_AS(
      publisher->changeDefaultAttributeOrderType(
          undefinedObjectClass, efficiencyOnly, RECEIVE),
      rti1516_2025::ObjectClassNotDefined);
  auto const employeeClass =
      publisher->getObjectClassHandle(fixture_hla::fom::employee);
  REQUIRE_THROWS_AS(
      publisher->changeDefaultAttributeOrderType(
          employeeClass, efficiencyOnly, RECEIVE),
      rti1516_2025::AttributeNotDefined);
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
  drainTurnUpdatesOn(*publisher, publisherReports);

  // The Restaurant FOM declares Efficiency TimeStamp. A prospective class
  // default change is captured only by later object registrations.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      server, efficiencyOnly, RECEIVE));
  ObjectInstanceHandle defaultReceiveObject;
  REQUIRE_NOTHROW(defaultReceiveObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  drainTurnUpdatesOn(*publisher, publisherReports);

  // An explicit instance change affects future updates for that instance only.
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      defaultReceiveObject, efficiencyOnly, TIMESTAMP));
  // Keep a second publisher's RECEIVE default independent while this
  // publisher changes the same class attribute back to TimeStamp.
  REQUIRE_NOTHROW(independentPublisher->changeDefaultAttributeOrderType(
      independentServer, independentEfficiencyOnly, RECEIVE));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      server, efficiencyOnly, TIMESTAMP));
  ObjectInstanceHandle publisherTimestampObject;
  REQUIRE_NOTHROW(
      publisherTimestampObject = publisher->registerObjectInstance(server));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  drainTurnUpdatesOn(*publisher, publisherReports);
  ObjectInstanceHandle independentReceiveObject;
  REQUIRE_NOTHROW(independentReceiveObject =
                      independentPublisher->registerObjectInstance(
                          independentServer));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  drainTurnUpdatesOn(*independentPublisher, independentPublisherReports);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_NOTHROW(independentPublisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(independentPublisher->evokeCallback(0.0));

  unsigned char const valueBytes[] = {0x4A};
  AttributeHandleValueMap values;
  values.emplace(efficiency, VariableLengthData(valueBytes, sizeof(valueBytes)));
  VariableLengthData const tag;
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      fomDefaultObject, values, tag, rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      defaultReceiveObject, values, tag, rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      publisherTimestampObject, values, tag, rti1516_2025::HLAinteger64Time(6)));
  auto const independentRetraction = independentPublisher->updateAttributeValues(
      independentReceiveObject,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE_FALSE(independentRetraction.isValid());
  evokeUntil(*receiver, [&] {
    return receiverReports.attributeReflectionReports.size() >= 1U;
  });

  REQUIRE(receiverReports.attributeReflectionReports.size() == 1);
  REQUIRE(receiverReports.attributeReflectionReports.front().object ==
          independentReceiveObject);
  REQUIRE(receiverReports.attributeReflectionReports.front().sentOrderType == RECEIVE);
  REQUIRE(receiverReports.attributeReflectionReports.front().receivedOrderType == RECEIVE);
  REQUIRE_FALSE(receiverReports.attributeReflectionReports.front().retractionSupplied);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_NOTHROW(
      independentPublisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(independentPublisher->evokeCallback(0.0));
  evokeUntil(*receiver, [&] {
    return receiverReports.attributeReflectionReports.size() >= 4U;
  });
  REQUIRE(receiverReports.attributeReflectionReports.size() == 4);
  std::size_t timestampedReflectionCount = 0;
  for (auto const& report : receiverReports.attributeReflectionReports) {
    if (report.object == independentReceiveObject) {
      REQUIRE(report.sentOrderType == RECEIVE);
      REQUIRE(report.receivedOrderType == RECEIVE);
      REQUIRE_FALSE(report.retractionSupplied);
      REQUIRE_FALSE(report.retractionValid);
    } else {
      REQUIRE((report.object == fomDefaultObject ||
              report.object == defaultReceiveObject ||
              report.object == publisherTimestampObject));
      REQUIRE(report.sentOrderType == TIMESTAMP);
      ++timestampedReflectionCount;
      REQUIRE(report.receivedOrderType == TIMESTAMP);
      REQUIRE(report.retractionSupplied);
      REQUIRE(report.retractionValid);
    }
  }
  REQUIRE(timestampedReflectionCount == 3);

  // Interaction order is scoped to this publisher and changes future sends.
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(takeOrder, RECEIVE));
  auto const retraction = publisher->sendInteraction(
      takeOrder,
      rti1516_2025::ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE_FALSE(retraction.isValid());
  evokeUntil(*receiver, [&] {
    return receiverReports.timestampedInteractionReports.size() >= 1U;
  });
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  auto const& interactionReport = receiverReports.timestampedInteractionReports.front();
  REQUIRE(interactionReport.sentOrderType == RECEIVE);
  REQUIRE(interactionReport.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(interactionReport.retractionSupplied);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(
      independentPublisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(independentPublisher->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded ownership transfer initializes instance order from acquiring federate default",
    "[integration][development-profile][ownership-management][object-management]"
    "[order-type-control][ownership-transfer-default-order]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.update-attribute-values][supplemental][rl-374]") {
  OrderControlFederateAmbassador ownerReports;
  OrderControlFederateAmbassador acquirerReports;
  OrderControlFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto acquirer = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(acquirer->connect(acquirerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"order-transfer-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(acquirer->joinFederationExecution(
      L"order-transfer-acquirer", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"order-transfer-receiver", L"receiver", federationName));

  auto const objectClass = owner->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const ownerAttribute = owner->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::efficiency);
  auto const acquirerClass = acquirer->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const acquirerAttribute = acquirer->getAttributeHandle(
      acquirerClass,
      fixture_hla::fixture::efficiency);
  auto const receiverClass = receiver->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const receiverAttribute = receiver->getAttributeHandle(
      receiverClass,
      fixture_hla::fixture::efficiency);
  REQUIRE(acquirerClass == objectClass);
  REQUIRE(acquirerAttribute == ownerAttribute);
  REQUIRE(receiverAttribute == ownerAttribute);
  AttributeHandleSet const ownerAttributes{ownerAttribute};
  AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  AttributeHandleSet const receiverAttributes{receiverAttribute};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass, ownerAttributes));
  REQUIRE_NOTHROW(acquirer->publishObjectClassAttributes(
      acquirerClass, acquirerAttributes));
  REQUIRE_NOTHROW(acquirer->subscribeObjectClassAttributes(
      acquirerClass, acquirerAttributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverClass, receiverAttributes));
  REQUIRE_NOTHROW(owner->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(acquirer->setObjectClassRelevanceAdvisorySwitch(false));

  // The FOM initializes the old owner's Efficiency order to TimeStamp. The
  // acquiring federate's class default is deliberately different.
  REQUIRE_NOTHROW(acquirer->changeDefaultAttributeOrderType(
      acquirerClass, acquirerAttributes, RECEIVE));
  ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = owner->registerObjectInstance(objectClass));
  for (int pass = 0; pass != 128; ++pass) {
    if (!acquirer->evokeCallback(0.0)) {
      break;
    }
  }
  for (int pass = 0; pass != 128; ++pass) {
    if (!receiver->evokeCallback(0.0)) {
      break;
    }
  }

  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      object, ownerAttributes, VariableLengthData{}));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(object, ownerAttribute));
  REQUIRE_FALSE(acquirer->isAttributeOwnedByFederate(object, acquirerAttribute));
  REQUIRE_NOTHROW(acquirer->attributeOwnershipAcquisitionIfAvailable(
      object, acquirerAttributes, VariableLengthData{}));
  for (int pass = 0; pass != 128; ++pass) {
    if (!acquirer->evokeCallback(0.0)) {
      break;
    }
  }
  REQUIRE(acquirer->isAttributeOwnedByFederate(object, acquirerAttribute));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(object, ownerAttribute));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  for (int pass = 0; pass != 128; ++pass) {
    if (!receiver->evokeCallback(0.0)) {
      break;
    }
  }
  REQUIRE_NOTHROW(acquirer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  for (int pass = 0; pass != 128; ++pass) {
    if (!acquirer->evokeCallback(0.0)) {
      break;
    }
  }

  unsigned char const bytes[] = {0x71};
  AttributeHandleValueMap values;
  values.emplace(acquirerAttribute, VariableLengthData(bytes, sizeof(bytes)));
  auto const retraction = acquirer->updateAttributeValues(
      object,
      values,
      VariableLengthData{},
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE_FALSE(retraction.isValid());
  for (int pass = 0;
       pass != 128 && receiverReports.attributeReflectionReports.empty();
       ++pass) {
    static_cast<void>(receiver->evokeCallback(0.01));
  }
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.object == object);
  REQUIRE(reflection.sentOrderType == RECEIVE);
  REQUIRE(reflection.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(reflection.retractionSupplied);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(acquirer->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(acquirer->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded class-default order follows inherited attribute across invocation classes",
    "[integration][development-profile][time-management][object-management]"
    "[order-type-control][inherited-class-default-order]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.update-attribute-values][supplemental][rl-374]") {
  OrderControlFederateAmbassador publisherReports;
  OrderControlFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"order-inherited-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"order-inherited-receiver", L"receiver", federationName));

  auto const employeeClass =
      publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const serverClass =
      publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const receiverEmployeeClass =
      receiver->getObjectClassHandle(fixture_hla::fom::employee);
  auto const employeeName = publisher->getAttributeHandle(
      employeeClass,
      fixture_hla::fixture::name);
  auto const serverName = publisher->getAttributeHandle(
      serverClass,
      fixture_hla::fixture::name);
  auto const receiverName = receiver->getAttributeHandle(
      receiverEmployeeClass,
      fixture_hla::fixture::name);
  REQUIRE(serverName == employeeName);
  REQUIRE(receiverName == employeeName);
  AttributeHandleSet const employeeNames{employeeName};
  AttributeHandleSet const receiverNames{receiverName};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      employeeClass, employeeNames));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverEmployeeClass, receiverNames));
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(false));

  // Name is declared by Employee and inherited by Server. Changing its
  // default through the derived Server handle must also govern future base
  // Employee instances of that same class attribute.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      serverClass, employeeNames, RECEIVE));
  ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = publisher->registerObjectInstance(employeeClass));
  for (int pass = 0; pass != 128; ++pass) {
    if (!receiver->evokeCallback(0.0)) {
      break;
    }
  }
  for (int pass = 0; pass != 128; ++pass) {
    if (!publisher->evokeCallback(0.0)) {
      break;
    }
  }

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  for (int pass = 0; pass != 128; ++pass) {
    if (!receiver->evokeCallback(0.0)) {
      break;
    }
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  for (int pass = 0; pass != 128; ++pass) {
    if (!publisher->evokeCallback(0.0)) {
      break;
    }
  }

  unsigned char const bytes[] = {0x4E};
  AttributeHandleValueMap values;
  values.emplace(employeeName, VariableLengthData(bytes, sizeof(bytes)));
  auto const retraction = publisher->updateAttributeValues(
      object,
      values,
      VariableLengthData{},
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE_FALSE(retraction.isValid());
  for (int pass = 0;
       pass != 128 && receiverReports.attributeReflectionReports.empty();
       ++pass) {
    static_cast<void>(receiver->evokeCallback(0.01));
  }
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.object == object);
  REQUIRE(reflection.sentOrderType == RECEIVE);
  REQUIRE(reflection.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(reflection.retractionSupplied);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
