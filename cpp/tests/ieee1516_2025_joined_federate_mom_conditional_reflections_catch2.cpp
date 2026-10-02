#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded joined-federate MOM conditional reflections track current state",
    "[integration][development-profile][federation-management][mom][object-management]"
    "[mom-switches][service-report-file][service-reporting]"
    "[rti.service.set-object-class-relevance-advisory-switch]"
    "[rti.service.set-attribute-relevance-advisory-switch]"
    "[rti.service.set-attribute-scope-advisory-switch]"
    "[rti.service.set-interaction-relevance-advisory-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-exception-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.disable-time-constrained]"
    "[rti.service.enable-asynchronous-delivery]"
    "[rti.service.disable-asynchronous-delivery]"
    "[rti.service.time-advance-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request][rti.service.next-message-request-available]"
    "[rti.service.flush-queue-request]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.send-interaction]"
    "[federate.callback.reflect-attribute-values]"
    "[joined-federate-mom-conditional-reflections]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador subjectReports;
    ReportingFederateAmbassador observerReports;
    auto subject = makeRti();
    auto observer = makeRti();
    auto subjectDirectory = temporaryServiceReportDirectory();
    auto observerDirectory = temporaryServiceReportDirectory();
    auto subjectConfiguration = configurationForServiceReportDirectory(subjectDirectory.path());
    auto observerConfiguration = configurationForServiceReportDirectory(observerDirectory.path());
    subjectConfiguration.withRtiAddress(L"in-process");
    observerConfiguration.withRtiAddress(L"in-process");
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
         "switch-nrg-disabled-fom.xml")
            .wstring();

    REQUIRE_NOTHROW(subject->connect(subjectReports, callbackModel, subjectConfiguration));
    REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel, observerConfiguration));
    REQUIRE_NOTHROW(
        subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
    auto const subjectFederate = subject->joinFederationExecution(
        L"conditional-mom-subject", L"subject", federationName);
    REQUIRE_NOTHROW(observer->joinFederationExecution(
        L"conditional-mom-observer", L"observer", federationName));

    auto const momClass = observer->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const federateHandleAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_handle);
    auto const federateNameAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_name);
    auto const federateTypeAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_type);
    auto const federateHostAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_host);
    auto const rtiVersionAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::rti_version);
    auto const fomModuleListAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::fom_module_designator_list);
    auto const reportFileAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::report_service_file);
    auto const objectClassRelevanceAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::object_class_relevance_advisory);
    auto const attributeRelevanceAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::attribute_relevance_advisory);
    auto const attributeScopeAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::attribute_scope_advisory);
    auto const interactionRelevanceAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::interaction_relevance_advisory);
    auto const conveyRegionsAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::convey_region_designator_sets);
    auto const automaticResignAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::automatic_resign_action);
    auto const serviceReportingAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::service_reporting);
    auto const exceptionReportingAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::exception_reporting);
    auto const sendServiceReportsToFileAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::send_service_reports_to_file);
    auto const timeConstrainedAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::time_constrained);
    auto const timeRegulatingAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::time_regulating);
    auto const asynchronousDeliveryAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::asynchronous_delivery);
    auto const timeManagerStateAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::time_manager_state);
    auto const logicalTimeAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::logical_time);
    auto const lookaheadAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::lookahead);
    auto const reliable = observer->getTransportationTypeHandle(standard_hla::mom::reliable);
    REQUIRE(momClass.isValid());
    REQUIRE(federateHandleAttribute.isValid());
    REQUIRE(federateNameAttribute.isValid());
    REQUIRE(federateTypeAttribute.isValid());
    REQUIRE(federateHostAttribute.isValid());
    REQUIRE(rtiVersionAttribute.isValid());
    REQUIRE(fomModuleListAttribute.isValid());
    REQUIRE(reportFileAttribute.isValid());
    REQUIRE(objectClassRelevanceAttribute.isValid());
    REQUIRE(attributeRelevanceAttribute.isValid());
    REQUIRE(attributeScopeAttribute.isValid());
    REQUIRE(interactionRelevanceAttribute.isValid());
    REQUIRE(conveyRegionsAttribute.isValid());
    REQUIRE(automaticResignAttribute.isValid());
    REQUIRE(serviceReportingAttribute.isValid());
    REQUIRE(exceptionReportingAttribute.isValid());
    REQUIRE(sendServiceReportsToFileAttribute.isValid());
    REQUIRE(timeConstrainedAttribute.isValid());
    REQUIRE(timeRegulatingAttribute.isValid());
    REQUIRE(asynchronousDeliveryAttribute.isValid());
    REQUIRE(timeManagerStateAttribute.isValid());
    REQUIRE(logicalTimeAttribute.isValid());
    REQUIRE(lookaheadAttribute.isValid());
    REQUIRE(reliable.isValid());

    AttributeHandleSet const subscribedAttributes{
        federateHandleAttribute,
        federateNameAttribute,
        federateTypeAttribute,
        federateHostAttribute,
        rtiVersionAttribute,
        fomModuleListAttribute,
        reportFileAttribute,
        objectClassRelevanceAttribute,
        attributeRelevanceAttribute,
        attributeScopeAttribute,
        interactionRelevanceAttribute,
        conveyRegionsAttribute,
        automaticResignAttribute,
        serviceReportingAttribute,
        exceptionReportingAttribute,
        sendServiceReportsToFileAttribute,
        timeConstrainedAttribute,
        timeRegulatingAttribute,
        asynchronousDeliveryAttribute,
        timeManagerStateAttribute,
        logicalTimeAttribute,
        lookaheadAttribute,
    };
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        subscribedAttributes,
        true));
    while (observer->evokeCallback(0.0)) {
    }

    auto const subjectFiles = serviceReportFiles(subjectDirectory.path());
    REQUIRE(subjectFiles.size() == 1U);
    auto const reflectedSubject = std::find_if(
        observerReports.attributeReflectionReports.begin(),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(federateHandleAttribute);
          return value != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) ==
                  variableLengthDataBytes(subjectFederate.encode());
        });
    REQUIRE(reflectedSubject != observerReports.attributeReflectionReports.end());
    auto const subjectObjectInstance = reflectedSubject->objectInstance;
    REQUIRE(reflectedSubject->attributeValues.size() == 7U);
    REQUIRE(reflectedSubject->attributeValues.find(objectClassRelevanceAttribute) ==
            reflectedSubject->attributeValues.end());

    AttributeHandleSet const conditionalAttributes{
        objectClassRelevanceAttribute,
        attributeRelevanceAttribute,
        attributeScopeAttribute,
        interactionRelevanceAttribute,
        conveyRegionsAttribute,
        automaticResignAttribute,
        serviceReportingAttribute,
        exceptionReportingAttribute,
        sendServiceReportsToFileAttribute,
    };
    auto expectConditionalReflection = [&](std::size_t const before,
                                           AttributeHandle const attribute,
                                           std::int32_t const expected) {
      auto const reflected = std::find_if(
          observerReports.attributeReflectionReports.begin() +
              static_cast<std::ptrdiff_t>(before),
          observerReports.attributeReflectionReports.end(),
          [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
            return report.objectInstance == subjectObjectInstance &&
                report.attributeValues.contains(attribute);
          });
      REQUIRE(reflected != observerReports.attributeReflectionReports.end());
      REQUIRE(reflected->attributeValues.size() == 1U);
      rti1516_2025::HLAinteger32BE decoded;
      REQUIRE_NOTHROW(decoded.decode(reflected->attributeValues.at(attribute)));
      REQUIRE(decoded.get() == expected);
      REQUIRE(reflected->transportationType == reliable);
      REQUIRE_FALSE(reflected->producingFederate.isValid());
      REQUIRE_FALSE(reflected->sentRegionsSupplied);
    };
    auto applySwitchAndCheck = [&](auto setter,
                                   AttributeHandle const attribute,
                                   std::int32_t const expected) {
      auto const before = observerReports.attributeReflectionReports.size();
      REQUIRE_NOTHROW(setter());
      while (observer->evokeCallback(0.0)) {
      }
      REQUIRE(observerReports.attributeReflectionReports.size() > before);
      expectConditionalReflection(before, attribute, expected);
    };
    auto expectBooleanConditionalReflection = [&](std::size_t const before,
                                                  AttributeHandle const attribute,
                                                  bool const expected) {
      auto const reflected = std::find_if(
          observerReports.attributeReflectionReports.begin() +
              static_cast<std::ptrdiff_t>(before),
          observerReports.attributeReflectionReports.end(),
          [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
            return report.objectInstance == subjectObjectInstance &&
                report.attributeValues.contains(attribute);
          });
      REQUIRE(reflected != observerReports.attributeReflectionReports.end());
      REQUIRE(reflected->attributeValues.size() == 1U);
      rti1516_2025::HLAboolean decoded;
      REQUIRE_NOTHROW(decoded.decode(reflected->attributeValues.at(attribute)));
      REQUIRE(decoded.get() == expected);
      REQUIRE(reflected->transportationType == reliable);
      REQUIRE_FALSE(reflected->producingFederate.isValid());
      REQUIRE_FALSE(reflected->sentRegionsSupplied);
    };
    auto applyTimeStateAndCheck = [&](auto service,
                                      AttributeHandle const attribute,
                                      bool const expected) {
      auto const before = observerReports.attributeReflectionReports.size();
      REQUIRE_NOTHROW(service());
      while (subject->evokeCallback(0.0)) {
      }
      while (observer->evokeCallback(0.0)) {
      }
      REQUIRE(observerReports.attributeReflectionReports.size() > before);
      expectBooleanConditionalReflection(before, attribute, expected);
    };

    // Before time regulation is enabled, HLAlookahead is undefined.  The
    // MIM represents that periodic value as an empty variable array, while
    // HLAlogicalTime is already defined at the provider's initial value.
    auto const beforeUndefinedPeriodicRequest =
        observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        AttributeHandleSet{logicalTimeAttribute, lookaheadAttribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() ==
            beforeUndefinedPeriodicRequest + 1U);
    auto const& undefinedPeriodicRequest = observerReports.attributeReflectionReports.back();
    REQUIRE(undefinedPeriodicRequest.objectInstance == subjectObjectInstance);
    REQUIRE(undefinedPeriodicRequest.attributeValues.size() == 2U);
    rti1516_2025::HLAinteger64Time undefinedLogicalTime;
    REQUIRE_NOTHROW(undefinedLogicalTime.decode(
        undefinedPeriodicRequest.attributeValues.at(logicalTimeAttribute)));
    REQUIRE(undefinedLogicalTime.getTime() == 0);
    REQUIRE(undefinedPeriodicRequest.attributeValues.at(lookaheadAttribute).size() == 0U);

    applyTimeStateAndCheck(
        [&] { subject->enableTimeConstrained(); },
        timeConstrainedAttribute,
        true);
    applyTimeStateAndCheck(
        [&] { subject->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)); },
        timeRegulatingAttribute,
        true);

    // The MIM marks these two values Periodic, but §11.4.1 requires their
    // values on a direct Request Attribute Value Update even before a future
    // HLAsetTiming wall-clock scheduler is enabled.  The current integer-time
    // provider therefore supplies its exact official encodings here.
    auto const beforePeriodicRequest = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        AttributeHandleSet{logicalTimeAttribute, lookaheadAttribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == beforePeriodicRequest + 1U);
    auto const& periodicRequest = observerReports.attributeReflectionReports.back();
    REQUIRE(periodicRequest.objectInstance == subjectObjectInstance);
    REQUIRE(periodicRequest.attributeValues.size() == 2U);
    rti1516_2025::HLAinteger64Time periodicLogicalTime;
    REQUIRE_NOTHROW(periodicLogicalTime.decode(
        periodicRequest.attributeValues.at(logicalTimeAttribute)));
    REQUIRE(periodicLogicalTime.getTime() == 0);
    rti1516_2025::HLAinteger64Interval periodicLookahead;
    REQUIRE_NOTHROW(periodicLookahead.decode(
        periodicRequest.attributeValues.at(lookaheadAttribute)));
    REQUIRE(periodicLookahead.getInterval() == 1);
    REQUIRE(periodicRequest.transportationType == reliable);
    REQUIRE_FALSE(periodicRequest.producingFederate.isValid());
    REQUIRE_FALSE(periodicRequest.sentRegionsSupplied);

    applyTimeStateAndCheck(
        [&] { subject->enableAsynchronousDelivery(); },
        asynchronousDeliveryAttribute,
        true);
    applyTimeStateAndCheck(
        [&] { subject->disableAsynchronousDelivery(); },
        asynchronousDeliveryAttribute,
        false);
    applyTimeStateAndCheck(
        [&] { subject->disableTimeRegulation(); },
        timeRegulatingAttribute,
        false);
    applyTimeStateAndCheck(
        [&] { subject->disableTimeConstrained(); },
        timeConstrainedAttribute,
        false);

    applySwitchAndCheck(
        [&] { subject->setObjectClassRelevanceAdvisorySwitch(true); },
        objectClassRelevanceAttribute,
        1);
    applySwitchAndCheck(
        [&] { subject->setAttributeRelevanceAdvisorySwitch(true); },
        attributeRelevanceAttribute,
        1);
    applySwitchAndCheck(
        [&] { subject->setAttributeScopeAdvisorySwitch(true); },
        attributeScopeAttribute,
        1);
    applySwitchAndCheck(
        [&] { subject->setInteractionRelevanceAdvisorySwitch(true); },
        interactionRelevanceAttribute,
        1);
    applySwitchAndCheck(
        [&] { subject->setConveyRegionDesignatorSetsSwitch(false); },
        conveyRegionsAttribute,
        0);
    applySwitchAndCheck(
        [&] { subject->setAutomaticResignDirective(rti1516_2025::NO_ACTION); },
        automaticResignAttribute,
        5);
    applySwitchAndCheck(
        [&] { subject->setServiceReportingSwitch(true); },
        serviceReportingAttribute,
        1);
    applySwitchAndCheck(
        [&] { subject->setExceptionReportingSwitch(false); },
        exceptionReportingAttribute,
        0);
    applySwitchAndCheck(
        [&] { subject->setSendServiceReportsToFileSwitch(true); },
        sendServiceReportsToFileAttribute,
        1);

    auto const beforeRequest = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        conditionalAttributes,
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == beforeRequest + 1U);
    auto const& requested = observerReports.attributeReflectionReports.back();
    REQUIRE(requested.objectInstance == subjectObjectInstance);
    REQUIRE(requested.attributeValues.size() == conditionalAttributes.size());
    for (auto const& [attribute, expected] : std::vector<std::pair<AttributeHandle, std::int32_t>>{
             {objectClassRelevanceAttribute, 1},
             {attributeRelevanceAttribute, 1},
             {attributeScopeAttribute, 1},
             {interactionRelevanceAttribute, 1},
             {conveyRegionsAttribute, 0},
             {automaticResignAttribute, 5},
             {serviceReportingAttribute, 1},
             {exceptionReportingAttribute, 0},
             {sendServiceReportsToFileAttribute, 1},
         }) {
      rti1516_2025::HLAinteger32BE decoded;
      REQUIRE_NOTHROW(decoded.decode(requested.attributeValues.at(attribute)));
      REQUIRE(decoded.get() == expected);
    }
    REQUIRE(requested.transportationType == reliable);
    REQUIRE_FALSE(requested.producingFederate.isValid());

    // The standard HLAsetSwitches MOM interaction is another state-changing
    // route.  Its accepted subset must drive the same current-value
    // projection as the individual support-service setters.
    auto const setSwitches = subject->getInteractionClassHandle(
        standard_hla::mom::set_switches_federate);
    auto const serviceReportingParameter = subject->getParameterHandle(
        setSwitches,
        standard_hla::mom::service_reporting);
    REQUIRE(setSwitches.isValid());
    REQUIRE(serviceReportingParameter.isValid());
    auto const disabledSwitch = rti1516_2025::HLAinteger32BE{0}.encode();
    auto const beforeMomAdjustment = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(subject->sendInteraction(
        setSwitches,
        ParameterHandleValueMap{{serviceReportingParameter, disabledSwitch}},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() > beforeMomAdjustment);
    expectConditionalReflection(beforeMomAdjustment, serviceReportingAttribute, 0);

    AttributeHandleSet const timeAttributes{
        timeConstrainedAttribute,
        timeRegulatingAttribute,
        asynchronousDeliveryAttribute,
    };
    auto const beforeTimeRequest = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        timeAttributes,
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == beforeTimeRequest + 1U);
    auto const& requestedTime = observerReports.attributeReflectionReports.back();
    REQUIRE(requestedTime.objectInstance == subjectObjectInstance);
    REQUIRE(requestedTime.attributeValues.size() == timeAttributes.size());
    for (auto const& attribute : timeAttributes) {
      rti1516_2025::HLAboolean decoded;
      REQUIRE_NOTHROW(decoded.decode(requestedTime.attributeValues.at(attribute)));
      REQUIRE_FALSE(decoded.get());
    }
    REQUIRE(requestedTime.transportationType == reliable);
    REQUIRE_FALSE(requestedTime.producingFederate.isValid());

    auto expectTimeManagerTransitions = [&](std::size_t const before) {
      std::set<std::int32_t> observed;
      for (auto iterator = observerReports.attributeReflectionReports.begin() +
                                static_cast<std::ptrdiff_t>(before);
           iterator != observerReports.attributeReflectionReports.end();
           ++iterator) {
        auto const value = iterator->attributeValues.find(timeManagerStateAttribute);
        if (iterator->objectInstance != subjectObjectInstance ||
            value == iterator->attributeValues.end()) {
          continue;
        }
        REQUIRE(iterator->attributeValues.size() == 1U);
        rti1516_2025::HLAinteger32BE decoded;
        REQUIRE_NOTHROW(decoded.decode(value->second));
        observed.insert(decoded.get());
        REQUIRE(iterator->transportationType == reliable);
        REQUIRE_FALSE(iterator->producingFederate.isValid());
        REQUIRE_FALSE(iterator->sentRegionsSupplied);
      }
      REQUIRE(observed.contains(0));
      REQUIRE(observed.contains(1));
    };
    auto applyTimeAdvanceAndCheck = [&](auto request) {
      auto const before = observerReports.attributeReflectionReports.size();
      REQUIRE_NOTHROW(request());
      while (subject->evokeCallback(0.0)) {
      }
      while (observer->evokeCallback(0.0)) {
      }
      expectTimeManagerTransitions(before);
    };

    // Each accepted request publishes TimeAdvancing (1), and its matching
    // grant publishes TimeGranted (0). Exercise all five request forms so
    // this conditional field cannot accidentally be coupled to one API.
    applyTimeAdvanceAndCheck(
        [&] { subject->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)); });
    applyTimeAdvanceAndCheck(
        [&] { subject->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(2)); });
    applyTimeAdvanceAndCheck(
        [&] { subject->nextMessageRequest(rti1516_2025::HLAinteger64Time(3)); });
    applyTimeAdvanceAndCheck(
        [&] { subject->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(4)); });
    applyTimeAdvanceAndCheck(
        [&] { subject->flushQueueRequest(rti1516_2025::HLAinteger64Time(5)); });

    auto const beforeTimeManagerRequest = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        AttributeHandleSet{timeManagerStateAttribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == beforeTimeManagerRequest + 1U);
    auto const& requestedTimeManager = observerReports.attributeReflectionReports.back();
    REQUIRE(requestedTimeManager.objectInstance == subjectObjectInstance);
    REQUIRE(requestedTimeManager.attributeValues.size() == 1U);
    rti1516_2025::HLAinteger32BE requestedTimeManagerState;
    REQUIRE_NOTHROW(requestedTimeManagerState.decode(
        requestedTimeManager.attributeValues.at(timeManagerStateAttribute)));
    REQUIRE(requestedTimeManagerState.get() == 0);
    REQUIRE(requestedTimeManager.transportationType == reliable);
    REQUIRE_FALSE(requestedTimeManager.producingFederate.isValid());

    REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subject->disconnect());
    REQUIRE_NOTHROW(observer->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

}
