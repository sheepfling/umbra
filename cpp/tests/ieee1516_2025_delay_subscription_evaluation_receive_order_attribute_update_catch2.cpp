#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary receive-order attribute-update eligibility",
    "[integration][development-profile][object-management][delay-subscription-evaluation]"
    "[rti.service.update-attribute-values]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.evoke-callback]"
    "[rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks]"
    "[callback-immediate]"
    "[federate.callback.reflect-attribute-values]") {
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
        L"delay-ro-attribute-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-ro-attribute-receiver", L"subscriber", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(objectClass, fixture_hla::fixture::reliable_base_a);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    AttributeHandleSet const attributes{attribute};
    unsigned char const valueBytes[] = {0xD5, 0x45};
    AttributeHandleValueMap values;
    values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));

    // The receiver first obtains a durable known-object record, then drops
    // the attribute declaration. This isolates delayed subscription
    // evaluation from discovery semantics.
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
    REQUIRE(receiver->getKnownObjectClassHandle(objectInstance) == objectClass);
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));

    // The receiver has no qualifying attribute subscription at generation.
    // Only the Enabled federation keeps a route until the receiver's actual
    // callback boundary and can therefore deliver after the late
    // re-subscription. HLA_IMMEDIATE establishes that boundary by temporarily
    // suspending callback dispatch.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance, values, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      while (receiver->evokeCallback(0.0)) {
      }
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    if (delaySubscriptionEvaluationEnabled) {
      auto const& reflection = receiverReports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1);
      REQUIRE(reflection.attributeValues.contains(attribute));
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(attribute)) ==
              std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
      REQUIRE_FALSE(reflection.sentRegionsSupplied);
    }

    // A recipient accepted while subscribed is still re-evaluated at the
    // callback boundary. Removing that current declaration suppresses a second
    // ordinary reflection in both switch settings.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance, values, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      while (receiver->evokeCallback(0.0)) {
      }
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));

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
