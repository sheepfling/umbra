#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary timestamped attribute-update eligibility",
    "[integration][development-profile][object-management][time-management]"
    "[delay-subscription-evaluation][tso]"
    "[rti.service.update-attribute-values]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[callback-immediate]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled,
                        auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const attributeFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "attribute-update-passel-fom.xml")
                                  .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{attributeFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-tso-attribute-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-tso-attribute-receiver", L"subscriber", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(objectClass, fixture_hla::fixture::reliable_base_a);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    AttributeHandleSet const attributes{attribute};
    unsigned char const valueBytes[] = {0xD5, 0x54};
    AttributeHandleValueMap values;
    values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));

    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(
        publisher->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
    if (!immediate) {
      while (receiver->evokeCallback(0.0)) {
      }
    }
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (!immediate) {
      while (publisher->evokeCallback(0.0)) {
      }
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1);

    // A TSO passel generated while unsubscribed remains associated with the
    // joined receiver only under the Enabled switch. The actual projection is
    // evaluated immediately before the callback at the recipient's grant.
    auto const firstRetraction = publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    if (!immediate) {
      while (publisher->evokeCallback(0.0)) {
      }
      while (receiver->evokeCallback(0.0)) {
      }
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
    rti1516_2025::HLAinteger64Time publisherTime;
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherTime));
    REQUIRE(publisherTime.getTime() == 2);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& reflection = receiverReports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1);
      REQUIRE(reflection.attributeValues.contains(attribute));
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(attribute)) ==
              std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
      REQUIRE(reflection.timeValue == L"2");
      REQUIRE(reflection.sentOrderType == TIMESTAMP);
      REQUIRE(reflection.receivedOrderType == TIMESTAMP);
    }

    // Generation-time eligibility never promises a TSO reflection. The
    // second passel is accepted while subscribed, then suppressed after the
    // declaration is removed before the next time grant in both settings.
    auto const secondRetraction = publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    if (!immediate) {
      while (publisher->evokeCallback(0.0)) {
      }
      while (receiver->evokeCallback(0.0)) {
      }
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled under HLA_EVOKED") {
    runScenario(true, HLA_EVOKED);
  }
  SECTION("the omitted switch uses the Disabled default under HLA_EVOKED") {
    runScenario(false, HLA_EVOKED);
  }
  SECTION("the creation-time switch is Enabled under HLA_IMMEDIATE") {
    runScenario(true, rti1516_2025::HLA_IMMEDIATE);
  }
  SECTION("the omitted switch uses the Disabled default under HLA_IMMEDIATE") {
    runScenario(false, rti1516_2025::HLA_IMMEDIATE);
  }
}
