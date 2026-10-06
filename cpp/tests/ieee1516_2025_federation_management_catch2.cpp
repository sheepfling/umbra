#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers accepted time-role transitions through MOM",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][time-management][time-role]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.modify-lookahead]"
    "[rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.disable-time-constrained]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{fomModule, switchModule},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"time-role-mom-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"time-role-mom-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::int32_t serial,
                                std::int32_t serviceType = 4) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == serviceType);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == serial);
  };
  auto const verifyReturnedArgument = [&](std::size_t index,
                                          std::int32_t type,
                                          std::wstring const& name,
                                          std::wstring const& value) {
    rti1516_2025::HLAfixedRecord returned;
    returned.appendElement(rti1516_2025::HLAinteger32BE{});
    returned.appendElement(rti1516_2025::HLAunicodeString{});
    returned.appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(returned.decode(
        observerReports.interactionReports.at(index).parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returned.get(0)).get() == type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(1)).get() == name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(2)).get() == value);
  };

  REQUIRE_NOTHROW(subject->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  verifyReport(0U, L"EnableTimeRegulation", 0);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE(subjectReports.timeRegulationEnabledReports.size() == 1U);

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(subject->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 0);
  REQUIRE(observerReports.interactionReports.size() == 2U);
  verifyReport(1U, L"QueryLogicalTime", 1);
  verifyReturnedArgument(1U, 31, L"Logical time", L"\"0\"");

  rti1516_2025::HLAinteger64Time queriedGalt;
  REQUIRE_FALSE(subject->queryGALT(queriedGalt));
  REQUIRE(observerReports.interactionReports.size() == 3U);
  verifyReport(2U, L"QueryGALT", 2);
  verifyReturnedArgument(2U, 34, L"", L"null");

  rti1516_2025::HLAinteger64Time queriedLits;
  REQUIRE_FALSE(subject->queryLITS(queriedLits));
  REQUIRE(observerReports.interactionReports.size() == 4U);
  verifyReport(3U, L"QueryLITS", 3);
  verifyReturnedArgument(3U, 34, L"", L"null");

  rti1516_2025::HLAinteger64Interval queriedLookahead;
  REQUIRE_NOTHROW(subject->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == 3);
  REQUIRE(observerReports.interactionReports.size() == 5U);
  verifyReport(4U, L"QueryLookahead", 4);
  verifyReturnedArgument(4U, 32, L"Lookahead", L"\"3\"");

  REQUIRE_NOTHROW(subject->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(observerReports.interactionReports.size() == 6U);
  verifyReport(5U, L"ModifyLookahead", 5);

  REQUIRE_NOTHROW(subject->enableTimeConstrained());
  REQUIRE(observerReports.interactionReports.size() == 7U);
  verifyReport(6U, L"EnableTimeConstrained", 6);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE(subjectReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(subject->disableTimeConstrained());
  REQUIRE(observerReports.interactionReports.size() == 8U);
  verifyReport(7U, L"DisableTimeConstrained", 7);
  REQUIRE_NOTHROW(subject->disableTimeRegulation());
  REQUIRE(observerReports.interactionReports.size() == 9U);
  verifyReport(8U, L"DisableTimeRegulation", 8);

  REQUIRE_NOTHROW(subject->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE(observerReports.interactionReports.size() == 10U);
  verifyReport(9U, L"TimeAdvanceRequest", 9);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE(subjectReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_NOTHROW(subject->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE(observerReports.interactionReports.size() == 11U);
  verifyReport(10U, L"TimeAdvanceRequestAvailable", 10);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE_NOTHROW(subject->nextMessageRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE(observerReports.interactionReports.size() == 12U);
  verifyReport(11U, L"NextMessageRequest", 11);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE_NOTHROW(subject->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(4)));
  REQUIRE(observerReports.interactionReports.size() == 13U);
  verifyReport(12U, L"NextMessageRequestAvailable", 12);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE_NOTHROW(subject->flushQueueRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(observerReports.interactionReports.size() == 14U);
  verifyReport(13U, L"FlushQueueRequest", 13);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE_NOTHROW(subject->enableAsynchronousDelivery());
  REQUIRE(observerReports.interactionReports.size() == 15U);
  verifyReport(14U, L"EnableAsynchronousDelivery", 14);
  REQUIRE_NOTHROW(subject->disableAsynchronousDelivery());
  REQUIRE(observerReports.interactionReports.size() == 16U);
  verifyReport(15U, L"DisableAsynchronousDelivery", 15);
  REQUIRE_NOTHROW(subject->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE(observerReports.interactionReports.size() == 17U);
  verifyReport(16U, L"SetObjectClassRelevanceAdvisorySwitch", 16, 6);
  REQUIRE_NOTHROW(subject->setAttributeRelevanceAdvisorySwitch(false));
  REQUIRE(observerReports.interactionReports.size() == 18U);
  verifyReport(17U, L"SetAttributeRelevanceAdvisorySwitch", 17, 6);
  REQUIRE_NOTHROW(subject->setAttributeScopeAdvisorySwitch(false));
  REQUIRE(observerReports.interactionReports.size() == 19U);
  verifyReport(18U, L"SetAttributeScopeAdvisorySwitch", 18, 6);
  REQUIRE_NOTHROW(subject->setInteractionRelevanceAdvisorySwitch(false));
  REQUIRE(observerReports.interactionReports.size() == 20U);
  verifyReport(19U, L"SetInteractionRelevanceAdvisorySwitch", 19, 6);
  REQUIRE_NOTHROW(subject->setConveyRegionDesignatorSetsSwitch(false));
  REQUIRE(observerReports.interactionReports.size() == 21U);
  verifyReport(20U, L"SetConveyRegionDesignatorSetsSwitch", 20, 6);
  REQUIRE_NOTHROW(subject->setAutomaticResignDirective(NO_ACTION));
  REQUIRE(observerReports.interactionReports.size() == 22U);
  verifyReport(21U, L"SetAutomaticResignDirective", 21, 6);
  REQUIRE_NOTHROW(subject->setExceptionReportingSwitch(false));
  REQUIRE(observerReports.interactionReports.size() == 23U);
  verifyReport(22U, L"SetExceptionReportingSwitch", 22, 6);

  // Federation-management synchronization services use the same public MOM
  // interaction route. Registration emits the initiating service and the
  // RTI-invoked confirmation and announcement before its evoked callbacks
  // are drained. The callback-originated report shares the serial stream.
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  auto const synchronizationLabel = std::wstring{L"mom-sync-public"};
  REQUIRE_NOTHROW(subject->registerFederationSynchronizationPoint(
      synchronizationLabel,
      VariableLengthData{}));
  REQUIRE(observerReports.interactionReports.size() == 26U);
  verifyReport(23U, L"RegisterFederationSynchronizationPoint", 23, 0);
  verifyReport(24U, L"ConfirmSynchronizationPointRegistration", 24, 0);
  verifyReport(25U, L"AnnounceSynchronizationPoint", 25, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->synchronizationPointAchieved(synchronizationLabel));
  REQUIRE(observerReports.interactionReports.size() == 27U);
  verifyReport(26U, L"SynchronizationPointAchieved", 26, 0);
  REQUIRE_NOTHROW(observer->synchronizationPointAchieved(synchronizationLabel));
  REQUIRE(observerReports.interactionReports.size() == 28U);
  verifyReport(27U, L"FederationSynchronized", 27, 0);
  while (subject->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(subject->requestFederationSave(L"mom-save-public"));
  REQUIRE(observerReports.interactionReports.size() == 30U);
  verifyReport(28U, L"RequestFederationSave", 28, 0);
  verifyReport(29U, L"InitiateFederateSave", 29, 0);
  REQUIRE_NOTHROW(subject->abortFederationSave());

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded timestamped directed interactions distinguish ownership and universal subscriptions",
    "[integration][development-profile][interaction-management][directed][ownership]"
    "[time-management][timestamped-directed-interaction][tso]"
    "[timestamped-directed-interaction-subscription-kind]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.receive-directed-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador ownershipSubscriberReports;
  ReportingFederateAmbassador universalSubscriberReports;
  auto owner = makeRti();
  auto sender = makeRti();
  auto ownershipSubscriber = makeRti();
  auto universalSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F, 0x2D, 0x53};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ownershipSubscriber->connect(ownershipSubscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(universalSubscriber->connect(universalSubscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timestamped-directed-subscription-owner", L"owner", federationName));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"timestamped-directed-subscription-sender", L"sender", federationName));
  REQUIRE_NOTHROW(ownershipSubscriber->joinFederationExecution(
      L"timestamped-directed-subscription-by-owner", L"subscriber", federationName));
  REQUIRE_NOTHROW(universalSubscriber->joinFederationExecution(
      L"timestamped-directed-subscription-universal", L"subscriber", federationName));

  auto const objectClass = owner->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = owner->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(ownershipSubscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(owner->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(ownershipSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = owner->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (sender->evokeCallback(0.0)) {
  }
  while (ownershipSubscriber->evokeCallback(0.0)) {
  }
  while (universalSubscriber->evokeCallback(0.0)) {
  }
  REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(ownershipSubscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(universalSubscriberReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(owner->enableTimeConstrained());
  REQUIRE_NOTHROW(ownershipSubscriber->enableTimeConstrained());
  REQUIRE_NOTHROW(universalSubscriber->enableTimeConstrained());
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (owner->evokeCallback(0.0)) {
  }
  while (sender->evokeCallback(0.0)) {
  }
  while (ownershipSubscriber->evokeCallback(0.0)) {
  }
  while (universalSubscriber->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(ownershipSubscriberReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(universalSubscriberReports.timeConstrainedEnabledReports.size() == 1U);

  auto advanceAll = [&](std::int64_t const value) {
    auto const time = rti1516_2025::HLAinteger64Time(value);
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(time));
    REQUIRE_NOTHROW(ownershipSubscriber->timeAdvanceRequest(time));
    REQUIRE_NOTHROW(universalSubscriber->timeAdvanceRequest(time));
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(time));
    while (owner->evokeCallback(0.0)) {
    }
    while (sender->evokeCallback(0.0)) {
    }
    while (ownershipSubscriber->evokeCallback(0.0)) {
    }
    while (universalSubscriber->evokeCallback(0.0)) {
    }
  };
  auto checkTimestampedReport = [&](auto const& report, std::wstring const& expectedTime) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.objectInstance == target);
    REQUIRE(report.parameterValues.empty());
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == expectedTime);
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };

  // By-ownership delivery reaches the target owner, but not a known
  // non-owner. The universal selector independently reaches that same target.
  auto const first = sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(first.isValid());
  REQUIRE(ownerReports.directedInteractionReports.empty());
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.empty());
  advanceAll(2);
  REQUIRE(ownerReports.directedInteractionReports.size() == 1U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(senderReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(ownershipSubscriberReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(universalSubscriberReports.timeAdvanceGrantReports.size() == 1U);
  checkTimestampedReport(ownerReports.directedInteractionReports.front(), L"2");
  checkTimestampedReport(universalSubscriberReports.directedInteractionReports.front(), L"2");

  // A queued universal candidate is re-evaluated at its TSO grant boundary.
  // Changing that selector to by-ownership suppresses delivery to the
  // non-owning subscriber while preserving the owner's eligibility.
  auto const second = sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(second.isValid());
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      false));
  advanceAll(4);
  REQUIRE(ownerReports.directedInteractionReports.size() == 2U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 1U);
  checkTimestampedReport(ownerReports.directedInteractionReports.back(), L"4");

  // Restore universal mode for a new accepted passel, then remove it before
  // the grant. The live unsubscribe suppresses only that non-owner route.
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));
  auto const third = sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(third.isValid());
  REQUIRE_NOTHROW(universalSubscriber->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  advanceAll(6);
  REQUIRE(ownerReports.directedInteractionReports.size() == 3U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 1U);
  checkTimestampedReport(ownerReports.directedInteractionReports.back(), L"6");

  REQUIRE_NOTHROW(universalSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ownershipSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(universalSubscriber->disconnect());
  REQUIRE_NOTHROW(ownershipSubscriber->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers save and restore requests through MOM",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][federation-save]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"timestamped-save-mom-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-save-mom-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::int32_t serial,
                                std::int32_t serviceType) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == serviceType);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == serial);
  };

  REQUIRE_NOTHROW(subject->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  verifyReport(0U, L"EnableTimeRegulation", 0, 4);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE(subjectReports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(subject->requestFederationSave(
      L"mom-timestamped-save",
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE(observerReports.interactionReports.size() == 3U);
  verifyReport(1U, L"RequestFederationSave", 1, 0);
  verifyReport(2U, L"InitiateFederateSave", 2, 0);
  REQUIRE_NOTHROW(subject->abortFederationSave());
  REQUIRE(observerReports.interactionReports.size() == 5U);
  verifyReport(3U, L"AbortFederationSave", 3, 0);
  verifyReport(4U, L"FederationSaved", 4, 0);

  auto const restoreLabel = std::wstring{L"mom-restore-public"};
  REQUIRE_NOTHROW(subject->requestFederationSave(restoreLabel));
  REQUIRE(observerReports.interactionReports.size() == 7U);
  verifyReport(5U, L"RequestFederationSave", 5, 0);
  verifyReport(6U, L"InitiateFederateSave", 6, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->federateSaveBegun());
  REQUIRE_NOTHROW(observer->federateSaveBegun());
  REQUIRE_NOTHROW(subject->federateSaveComplete());
  REQUIRE_NOTHROW(observer->federateSaveComplete());
  REQUIRE(observerReports.interactionReports.size() == 10U);
  verifyReport(7U, L"FederateSaveBegun", 7, 0);
  verifyReport(8U, L"FederateSaveComplete", 8, 0);
  verifyReport(9U, L"FederationSaved", 9, 0);
  while (subject->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(subject->requestFederationRestore(restoreLabel));
  REQUIRE(observerReports.interactionReports.size() == 14U);
  verifyReport(10U, L"RequestFederationRestore", 10, 0);
  verifyReport(11U, L"ConfirmFederationRestorationRequest", 11, 0);
  verifyReport(12U, L"FederationRestoreBegun", 12, 0);
  verifyReport(13U, L"InitiateFederateRestore", 13, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->federateRestoreComplete());
  REQUIRE(observerReports.interactionReports.size() == 15U);
  verifyReport(14U, L"FederateRestoreComplete", 14, 0);
  REQUIRE_NOTHROW(observer->federateRestoreComplete());
  while (subject->evokeCallback(0.0)) {
  }

  // The remaining accepted restore controls share the same public
  // federation-management interaction route and must be visible before their
  // status/failure callbacks are delivered.
  REQUIRE_NOTHROW(subject->requestFederationRestore(restoreLabel));
  REQUIRE(observerReports.interactionReports.size() == 19U);
  verifyReport(15U, L"RequestFederationRestore", 15, 0);
  verifyReport(16U, L"ConfirmFederationRestorationRequest", 16, 0);
  verifyReport(17U, L"FederationRestoreBegun", 17, 0);
  verifyReport(18U, L"InitiateFederateRestore", 18, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->queryFederationRestoreStatus());
  REQUIRE(observerReports.interactionReports.size() == 21U);
  verifyReport(19U, L"QueryFederationRestoreStatus", 19, 0);
  verifyReport(20U, L"FederationRestoreStatusResponse", 20, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->abortFederationRestore());
  REQUIRE(observerReports.interactionReports.size() == 22U);
  verifyReport(21U, L"AbortFederationRestore", 21, 0);
  while (subject->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Private update-rate gate spaces best-effort streams and never drops reliable delivery",
    "[update-rate-reduction][update-rate-gate][object-management][unit]") {
  using Gate = umbra::detail::UpdateRateGate;
  Gate::Clock::time_point now{};
  Gate gate([&now] { return now; });

  REQUIRE(gate.admit("receiver/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("receiver/attribute", 2.0, false));
  now += std::chrono::milliseconds(500);
  REQUIRE(gate.admit("receiver/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("receiver/attribute", 2.0, false));

  // A faster producer (three submissions in one half-second) is reduced to
  // the slower subscriber's two-per-second allowance.
  std::size_t delivered = 1;
  for (int submission = 0; submission < 2; ++submission) {
    now += std::chrono::milliseconds(100);
    delivered += gate.admit("producer/subscriber", 2.0, false) ? 1U : 0U;
  }
  REQUIRE(delivered == 2);
  now += std::chrono::milliseconds(400);
  REQUIRE(gate.admit("producer/subscriber", 2.0, false));

  // Distinct projected attributes keep independent admission histories.
  REQUIRE(gate.admit("receiver/fast-attribute", 10.0, false));
  REQUIRE(gate.admit("receiver/slow-attribute", 1.0, false));
  now += std::chrono::milliseconds(50);
  REQUIRE_FALSE(gate.admit("receiver/fast-attribute", 10.0, false));
  REQUIRE_FALSE(gate.admit("receiver/slow-attribute", 1.0, false));

  // A subscription mutation is encoded into the delivery key by the
  // registry.  A later generation therefore starts with a fresh admission
  // history even when the federate/object/attribute identity is unchanged.
  REQUIRE(gate.admit("receiver/object/generation-1/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("receiver/object/generation-1/attribute", 2.0, false));
  REQUIRE(gate.admit("receiver/object/generation-2/attribute", 2.0, false));

  REQUIRE(gate.admit("reliable", 0.001, true));
  REQUIRE(gate.admit("reliable", 0.001, true));
  REQUIRE(gate.admit("default", 0.0, false));
  REQUIRE(gate.admit("default", 0.0, false));
  gate.erase("receiver/attribute");
  REQUIRE(gate.admit("receiver/attribute", 2.0, false));

  // Federation teardown must only discard that execution's wall-clock
  // history.  A second live execution can have the same recipient/object
  // handles and still needs its reduction interval preserved.
  REQUIRE(gate.admit("federation-a/receiver/object/1/generation-1/attribute", 2.0, false));
  REQUIRE(gate.admit("federation-b/receiver/object/1/generation-1/attribute", 2.0, false));
  gate.erasePrefix("federation-a/");
  REQUIRE(gate.admit("federation-a/receiver/object/1/generation-1/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("federation-b/receiver/object/1/generation-1/attribute", 2.0, false));

  // Federate resignation is narrower than federation teardown.  Reclaiming
  // one recipient's history must leave a second recipient's live window
  // intact even when both share the same object and attribute handles.
  REQUIRE(gate.admit("federation-a/101/object/1/generation-1/attribute", 2.0, false));
  REQUIRE(gate.admit("federation-a/202/object/1/generation-1/attribute", 2.0, false));
  gate.erasePrefix("federation-a/101/");
  REQUIRE(gate.admit("federation-a/101/object/1/generation-1/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("federation-a/202/object/1/generation-1/attribute", 2.0, false));

  gate.clear();
  REQUIRE(gate.admit("receiver/attribute", 2.0, false));
}

}  // namespace

#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded asynchronous delivery gates receive-order callbacks by temporal state",
    "[integration][development-profile][time-management][asynchronous-delivery]"
    "[rti.service.enable-asynchronous-delivery][rti.service.disable-asynchronous-delivery]"
    "[callback-immediate]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" /
                            "tests" /
                            "data" /
                            "parameter-handle-provider-fom.xml")
                               .wstring();

    REQUIRE_THROWS_AS(
        receiver->enableAsynchronousDelivery(),
        rti1516_2025::NotConnected);
    REQUIRE_THROWS_AS(
        receiver->disableAsynchronousDelivery(),
        rti1516_2025::NotConnected);
    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_THROWS_AS(
        receiver->enableAsynchronousDelivery(),
        rti1516_2025::FederateNotExecutionMember);
    REQUIRE_THROWS_AS(
        receiver->disableAsynchronousDelivery(),
        rti1516_2025::FederateNotExecutionMember);

    REQUIRE_NOTHROW(
        publisher->createFederationExecution(
            federationName,
            fomModule,
            standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"async-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"async-receiver",
        L"subscriber",
        federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);

    // The default switch is disabled. A receive-order message submitted while
    // the constrained federate is Time Granted is retained, not discarded.
    REQUIRE_NOTHROW(
        publisher->sendInteraction(interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.empty());

    // Under HLA_IMMEDIATE this service flushes the deferred callback directly;
    // under HLA_EVOKED it makes the same callback available to Evoke.
    REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
    REQUIRE_THROWS_AS(
        receiver->enableAsynchronousDelivery(),
        rti1516_2025::AsynchronousDeliveryAlreadyEnabled);
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() == 1);

    REQUIRE_NOTHROW(receiver->disableAsynchronousDelivery());
    REQUIRE_THROWS_AS(
        receiver->disableAsynchronousDelivery(),
        rti1516_2025::AsynchronousDeliveryAlreadyDisabled);

    REQUIRE_NOTHROW(
        publisher->sendInteraction(interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() == 1);

    // Entering Time Advancing makes the retained RO message eligible. The
    // direct model observes it during this service call; the evoked model
    // observes it during the following Evoke. Neither path needs a grant from
    // another federate in this isolated scenario.
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() == 2);
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded timestamped Delete Object Instance reconstitutes on retraction and removes before grant",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD4, 0x16, 0x2A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-publisher", L"publisher", federationName));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->getFederateHandle(L"timestamped-delete-publisher"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-delete-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  // Discovery can enqueue the RTI-invoked Auto Provide request on the
  // publisher. Drain that setup callback before isolating time-regulation
  // enablement below; it is not part of the time-management assertion.
  while (publisher->evokeCallback(0.0)) {
  }
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(
          objectInstance,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  auto const secondHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.objectRemovalReports.size() == 1);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "remove", "grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(removal.timeValue == L"7");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting records failed timestamped regional Update Attribute Values invocations",
    "[integration][development-profile][federation-management][object-management][ddm][time-management]"
    "[mom][service-report-file][service-reporting][service-failure][tso]"
    "[timestamped-regional-attribute-update-failure]"
    "[rti.service.update-attribute-values][rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const valueBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'r', 'e', 'g'};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  rti1516_2025::HLAinteger64Time const validTimestamp(6);
  rti1516_2025::HLAinteger64Time const invalidTimestamp(4);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-regional-update-failure-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-update-failure-receiver", L"subscriber", federationName));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  AttributeHandleValueMap const values{{flavor, value}};
  AttributeHandleValueMap const invalidValues{{AttributeHandle{}, value}};
  auto asAscii = [](std::wstring const& text) {
    std::string result;
    result.reserve(text.size());
    for (wchar_t const character : text) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectValue = asAscii(objectInstance.toString());
  auto const invalidObjectValue = asAscii(ObjectInstanceHandle{}.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const invalidAttributeValue = asAscii(AttributeHandle{}.toString());
  auto const validMap = std::string{"{\""} + attributeValue + "\":\"AQI=\"}";
  auto const invalidMap = std::string{"{\""} + invalidAttributeValue + "\":\"AQI=\"}";
  auto const reportRecord = [](std::uint32_t serial,
                               std::string const& objectText,
                               std::string const& mapText,
                               std::string const& timestampText,
                               std::string const& exception) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"UpdateAttributeValues","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
        objectText + R"("},{"HLAargumentType":2,"HLAargumentName":"Constrained set of attribute designator and value pairs","HLAargumentValue":)" +
        mapText +
        R"(},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVn"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":)" +
        timestampText + R"(}],"HLAsuccessIndicator":false,"HLAexception":")" +
        exception + "\"}";
  };

  auto const unknownObjectFailure = reportRecord(
      0U,
      invalidObjectValue,
      validMap,
      "\"6\"",
      "ObjectInstanceNotKnown: Timestamped Update Attribute Values requires a known ObjectInstanceHandle.");
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          ObjectInstanceHandle{},
          values,
          tag,
          validTimestamp),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText + unknownObjectFailure);

  auto const invalidAttributeFailure = reportRecord(
      1U,
      objectValue,
      invalidMap,
      "\"6\"",
      "AttributeNotDefined: Timestamped Update Attribute Values requires defined AttributeHandle values.");
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          invalidValues,
          tag,
          validTimestamp),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText + unknownObjectFailure + invalidAttributeFailure);

  auto const invalidTimeFailure = reportRecord(
      2U,
      objectValue,
      validMap,
      "\"4\"",
      "InvalidLogicalTime: A timestamped service is earlier than the sender\\'s current logical time plus lookahead.");
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          values,
          tag,
          invalidTimestamp),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(readTextFile(reportFile) ==
          initialText + unknownObjectFailure + invalidAttributeFailure + invalidTimeFailure);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}



TEST_CASE(
    "Embedded receive-order Update Attribute Values honors 2025 passel and callback lifecycle",
    "[integration][development-profile][object-management]"
    "[rti.service.update-attribute-values][federate.callback.reflect-attribute-values]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto cancelled = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xC0, 0x25, 0xA4};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleValueMap noAttributeValues;

  // The service retains its connection and membership preconditions ahead of
  // validation of its object handle and value map.
  REQUIRE_THROWS_AS(
      unjoined->updateAttributeValues(invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->updateAttributeValues(invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"attribute-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"attribute-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"attribute-cancelled", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"attribute-immediate", L"subscriber", federationName));

  auto const base = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = publisher->getAttributeHandle(child, L"ReliableBaseB");
  auto const bestEffortBase = publisher->getAttributeHandle(child, L"BestEffortBase");
  auto const reliableChild = publisher->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = publisher->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(bestEffortBase.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const allOwned{
      reliableBaseA,
      reliableBaseB,
      bestEffortBase,
      reliableChild,
  };
  AttributeHandleSet const baseAttributes{
      reliableBaseA,
      reliableBaseB,
      bestEffortBase,
  };
  AttributeHandleSet const reliableChildOnly{reliableChild};

  // The promoted receiver uses an active superclass subscription so it is
  // eligible for both discovery and the projected base-attribute reflections.
  // The cancelled receiver starts eligible so that its later unsubscribe
  // exercises callback-time suppression.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(base, baseAttributes, true));
  REQUIRE_NOTHROW(cancelled->subscribeObjectClassAttributes(child, reliableChildOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, allOwned));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exactReports.objectDiscoveryReports.empty());
  REQUIRE(promotedReports.objectDiscoveryReports.empty());
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(exactReports.objectDiscoveryReports.size() == 1);
  REQUIRE(promotedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(cancelledReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exact->getKnownObjectClassHandle(objectInstance) == child);
  REQUIRE(promoted->getKnownObjectClassHandle(objectInstance) == base);
  REQUIRE(cancelled->getKnownObjectClassHandle(objectInstance) == child);

  unsigned char const reliableBaseABytes[] = {0x01, 0x02};
  unsigned char const reliableBaseBBytes[] = {0x03, 0x04};
  unsigned char const bestEffortBaseBytes[] = {0x05, 0x06};
  unsigned char const reliableChildBytes[] = {0x07, 0x08};
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      reliableBaseA,
      VariableLengthData(reliableBaseABytes, sizeof(reliableBaseABytes)));
  attributeValues.emplace(
      reliableBaseB,
      VariableLengthData(reliableBaseBBytes, sizeof(reliableBaseBBytes)));
  attributeValues.emplace(
      bestEffortBase,
      VariableLengthData(bestEffortBaseBytes, sizeof(bestEffortBaseBytes)));
  attributeValues.emplace(
      reliableChild,
      VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));

  AttributeHandleValueMap unownedValues;
  unownedValues.emplace(unownedChild, VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(invalidObjectInstance, attributeValues, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandle invalidAttribute;
  AttributeHandleValueMap invalidAttributeValues;
  invalidAttributeValues.emplace(invalidAttribute, VariableLengthData());
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, invalidAttributeValues, tag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, unownedValues, tag),
      rti1516_2025::AttributeNotOwned);
  AttributeHandleValueMap exactValues;
  exactValues.emplace(reliableChild, VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));
  REQUIRE_THROWS_AS(
      exact->updateAttributeValues(objectInstance, exactValues, tag),
      rti1516_2025::AttributeNotOwned);

  // One no-time request contains two immutable passels: the three reliable
  // values stay together, and the best-effort value remains separate. The
  // child-only attribute is deliberately absent from the promoted receiver's
  // known superclass projection.
  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, attributeValues, tag));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  REQUIRE(exactReports.attributeReflectionReports.empty());
  REQUIRE(promotedReports.attributeReflectionReports.empty());
  REQUIRE(cancelledReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(cancelled->unsubscribeObjectClassAttributes(child, reliableChildOnly));
  // With a zero maximum interval, the callback model processes one queued
  // callback per invocation and reports whether another passel remains.
  REQUIRE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));

  auto const reliableTransportation = publisher->getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffortTransportation = publisher->getTransportationTypeHandle(L"HLAbestEffort");
  auto reflectionFor = [](
                           std::vector<ReportingFederateAmbassador::AttributeReflectionReport> const& reports,
                           TransportationTypeHandle const& transportationType) {
    auto const found = std::find_if(
        reports.begin(),
        reports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.transportationType == transportationType;
        });
    REQUIRE(found != reports.end());
    return &*found;
  };
  auto requireReflection = [&](ReportingFederateAmbassador::AttributeReflectionReport const& report,
                               TransportationTypeHandle const& transportationType,
                               std::vector<AttributeHandle> const& expectedAttributes) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == expectedAttributes.size());
    for (AttributeHandle const& attribute : expectedAttributes) {
      auto const expected = attributeValues.find(attribute);
      REQUIRE(expected != attributeValues.end());
      auto const received = report.attributeValues.find(attribute);
      REQUIRE(received != report.attributeValues.end());
      REQUIRE(variableLengthDataBytes(received->second) == variableLengthDataBytes(expected->second));
    }
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == transportationType);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  };

  REQUIRE(exactReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(exactReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB, reliableChild});
  requireReflection(
      *reflectionFor(exactReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(promotedReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(promotedReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB});
  requireReflection(
      *reflectionFor(promotedReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(immediateReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB, reliableChild});
  requireReflection(
      *reflectionFor(immediateReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(cancelledReports.attributeReflectionReports.empty());
  REQUIRE(publisherReports.attributeReflectionReports.empty());

  // Unpublishing the whole class removes the producer's ownership of every
  // corresponding instance attribute, so a later update is rejected at the
  // official AttributeNotOwned boundary rather than using stale state.
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(child));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, attributeValues, tag),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded regional object attributes filter 2025 no-time updates by overlap",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-object-subscriber", L"subscriber", federationName));
  REQUIRE_FALSE(subscriber->getConveyRegionDesignatorSetsSwitch());

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const emptyRegionPair{{
      flavorOnly,
      RegionHandleSet{},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      regionalPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(
      objectInstance,
      emptyRegionPair));
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());

  unsigned char const firstValueBytes[] = {0x10, 0x25};
  AttributeHandleValueMap firstValue;
  firstValue.emplace(
      flavor,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      firstValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  // Association is additive and idempotent. The disjoint subscriber remains
  // undiscoverable even after the producer repeats the association explicitly.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == soda);

  unsigned char const secondValueBytes[] = {0x20, 0x25};
  AttributeHandleValueMap secondValue;
  secondValue.emplace(
      flavor,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      secondValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);
  REQUIRE_FALSE(subscriberReports.attributeReflectionReports.front().sentRegionsSupplied);
  REQUIRE(
      subscriberReports.attributeReflectionReports.front().attributeValues.contains(flavor));

  // The switch is recipient-local and may be changed after a federation has
  // joined.  Enabling it exposes the same update-region realization on the
  // next reflection without changing regional overlap routing.
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));
  unsigned char const conveyedValueBytes[] = {0x25, 0x20};
  AttributeHandleValueMap conveyedValue;
  conveyedValue.emplace(
      flavor,
      VariableLengthData(conveyedValueBytes, sizeof(conveyedValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      conveyedValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegions.contains(publisherRegion));

  // Changing the committed subscriber region to a disjoint range removes the
  // regional reflection route without changing ordinary known-instance state.
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  unsigned char const disjointValueBytes[] = {0x30, 0x25};
  AttributeHandleValueMap disjointValue;
  disjointValue.emplace(
      flavor,
      VariableLengthData(disjointValueBytes, sizeof(disjointValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      disjointValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, emptyRegionPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded timestamped regional attribute update survives time-constrained re-enable",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update][explicit-source][tso][re-enable]"
    "[timestamped-regional-attribute-reenable]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x47, 0x32};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x45, 0x4E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-attribute-reenable-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-attribute-reenable-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The source association is explicit and overlap-qualified. Its queued
  // callback must retain the original RegionHandle through the role change.
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 2U);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});

  auto const& report = receiverReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped regional Update Attribute Values returns a retraction designator without overlap-qualified recipients",
    "[integration][development-profile][object-management][ddm][time-management][tso]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x5A, 0x25};
  unsigned char const tagBytes[] = {0x4F, 0x56, 0x45, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-attribute-no-overlap-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-attribute-no-overlap-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  // Time regulation may report an advisory callback through the evoked
  // ambassador. Drain setup traffic before asserting the TSO/DDM behavior.
  while (publisher->evokeCallback(0.0)) {
  }
  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));

  // Clause 6.10's TSO-preferred-attribute condition holds despite the
  // disjoint subscription. The public result therefore has a designator
  // while the regional planner suppresses all callback fanout.
  auto const retractable = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(retractable));
  REQUIRE_THROWS_AS(
      publisher->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      publisher->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped regional attribute association replacement does not retarget a queued passel",
    "[integration][development-profile][object-management][ddm][time-management][tso]"
    "[timestamped-regional-attribute-update][explicit-source][association-replacement]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const firstValueBytes[] = {0x53, 0x52, 0x31};
  unsigned char const secondValueBytes[] = {0x53, 0x52, 0x32};
  unsigned char const firstTagBytes[] = {0x52, 0x45, 0x50, 0x31};
  unsigned char const secondTagBytes[] = {0x52, 0x45, 0x50, 0x32};
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-regional-replacement-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-replacement-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const sourceRegionA = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const sourceRegionB = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  for (auto const sourceRegion : {sourceRegionA, sourceRegionB}) {
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        sodaFlavor,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  }
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePairA{{
      flavorOnly,
      RegionHandleSet{sourceRegionA},
  }};
  AttributeHandleSetRegionHandleSetPairVector const sourcePairB{{
      flavorOnly,
      RegionHandleSet{sourceRegionB},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      sourcePairA));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap firstValues;
  firstValues.emplace(
      flavor,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  auto const firstRetraction = publisher->updateAttributeValues(
      objectInstance,
      firstValues,
      firstTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstRetraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The queued passel captured sourceRegionA. Replacing the association before
  // its callback boundary must not retarget that old payload to sourceRegionB.
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePairA));
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, sourcePairB));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());

  // The replacement applies to later sends. The new passel carries only
  // sourceRegionB and is delivered after the receiver crosses timestamp 7.
  AttributeHandleValueMap secondValues;
  secondValues.emplace(
      flavor,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  auto const secondRetraction = publisher->updateAttributeValues(
      objectInstance,
      secondValues,
      secondTag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondRetraction.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& report = receiverReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(secondValueBytes, secondValueBytes + sizeof(secondValueBytes)));
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.size() == 1U);
  REQUIRE(report.sentRegions.contains(sourceRegionB));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(secondTagBytes, secondTagBytes + sizeof(secondTagBytes)));
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePairB));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionB));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionA));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded suppressed timestamped regional attribute callback does not request retraction",
    "[integration][development-profile][object-management][ddm][time-management][tso][retract]"
    "[rti.service.update-attribute-values][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.retract][federate.callback.reflect-attribute-values]"
    "[federate.callback.request-retraction][suppressed-timestamped-regional-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x53, 0x55, 0x50, 0x41, 0x54, 0x54};
  AttributeHandleValueMap values;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"suppressed-regional-attribute-retraction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"suppressed-regional-attribute-retraction-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());

  // The instance/update region overlapped when the passel was accepted. A
  // later committed disjoint region suppresses the callback at its boundary;
  // it must consume the recipient ledger without classifying it delivered.
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(retraction));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded disjoint timestamped regional attribute callback remains suppressed through Time Advance Grant",
    "[integration][development-profile][object-management][ddm][time-management][tso][retract]"
    "[rti.service.update-attribute-values][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request][rti.service.retract]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]"
    "[federate.callback.request-retraction][suppressed-regional-attribute-through-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x54, 0x53, 0x4F, 0x44, 0x44, 0x4D};
  AttributeHandleValueMap values;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"suppressed-regional-attribute-grant-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"suppressed-regional-attribute-grant-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  // Regional declaration work may leave a publisher-side advisory callback in
  // the evoked queue. It is setup traffic, not part of the TSO boundary below.
  while (publisher->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The passel was eligible when accepted. Its receive-order projection is
  // intentionally reevaluated at the time-6 callback boundary.
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant"});

  // Advancing the regulator to 2 gives a timestamp-6 message its strict
  // terminal boundary (2 + lookahead 5). The suppressed callback remains a
  // non-delivery; it must not be revived by a later Retract attempt.
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Delete Object Instance delivers before TAR and NMR grants",
    "[integration][development-profile][object-management][time-management][tso]"
    "[timestamped-object-deletion][timestamped-delete-object-instance-tar-nmr]"
    "[time-advance-request][next-message-request]"
    "[rti.service.delete-object-instance][rti.service.time-advance-request]"
    "[rti.service.next-message-request]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador tarReports;
  ReportingFederateAmbassador nmrReports;
  auto publisher = makeRti();
  auto tar = makeRti();
  auto nmr = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x41, 0x52, 0x2D, 0x4E, 0x4D, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tar->connect(tarReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmr->connect(nmrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-delete-tar-nmr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(tar->joinFederationExecution(
      L"timestamped-delete-tar-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmr->joinFederationExecution(
      L"timestamped-delete-nmr-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(child, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(child, fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(tar->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(nmr->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE(tarReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(nmrReports.objectDiscoveryReports.size() == 1U);
  // Each discovery may solicit Auto Provide from the publisher. Complete
  // those RTI setup callbacks before testing time-regulation callbacks.
  while (publisher->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(tar->enableTimeConstrained());
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_NOTHROW(nmr->enableTimeConstrained());
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The deletion is accepted at timestamp 7. TAR reaches that timestamp
  // directly, while NMR requests 10 and selects the queued timestamp 7.
  // Both recipients must receive the removal before their own grant, and the
  // producer's time advance to 2 supplies the outgoing TSO lower bound.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(tarReports.objectRemovalReports.empty());
  REQUIRE(nmrReports.objectRemovalReports.empty());

  REQUIRE_NOTHROW(tar->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmr->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(tarReports.timeAdvanceGrantReports.empty());
  REQUIRE(nmrReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_FALSE(nmr->evokeCallback(0.0));

  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(tarReports.objectRemovalReports.size() == 1U);
  REQUIRE(nmrReports.objectRemovalReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmrReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(tarReports.callbackOrder == std::vector<std::string>{"remove", "grant"});
  REQUIRE(nmrReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto requireRemoval = [&](auto const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireRemoval(tarReports.objectRemovalReports.front());
  requireRemoval(nmrReports.objectRemovalReports.front());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  rti1516_2025::HLAinteger64Time tarTime;
  rti1516_2025::HLAinteger64Time nmrTime;
  REQUIRE_NOTHROW(tar->queryLogicalTime(tarTime));
  REQUIRE_NOTHROW(nmr->queryLogicalTime(nmrTime));
  REQUIRE(tarTime.getTime() == 7);
  REQUIRE(nmrTime.getTime() == 7);

  REQUIRE_NOTHROW(nmr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tar->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmr->disconnect());
  REQUIRE_NOTHROW(tar->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Delete Object Instance survives time-constrained re-enable",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-object-deletion][tso][re-enable]"
    "[timestamped-delete-survives-time-constrained-reenable]"
    "[rti.service.delete-object-instance][rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.time-constrained-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x52, 0x45, 0x45, 0x4E, 0x2D, 0x44};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-delete-reenable-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-delete-reenable-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  // The pending removal keeps its joined-federate identity while the
  // recipient leaves and re-enters the Time Constrained role. It must not be
  // delivered early, discarded, duplicated, or replaced.
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 2U);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Delete Object Instance delivers before FQR TARA and NMRA grants",
    "[integration][development-profile][object-management][time-management][tso]"
    "[timestamped-object-deletion]"
    "[timestamped-delete-fqr-tara-nmra]"
    "[flush-queue-request][time-advance-request-available][next-message-request-available]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.flush-queue-request]"
    "[rti.service.time-advance-request-available]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador fqrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto publisher = makeRti();
  auto fqr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD8, 0x61, 0x4E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-delete-alternate-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"timestamped-delete-alternate-fqr", L"subscriber", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"timestamped-delete-alternate-tara", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"timestamped-delete-alternate-nmra", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(fqr->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(tara->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(nmra->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  for (auto* receiver : {fqr.get(), tara.get(), nmra.get()}) {
    while (receiver->evokeCallback(0.0)) {
    }
  }
  REQUIRE(fqrReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(taraReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(nmraReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(fqr->enableTimeConstrained());
  while (fqr->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(tara->enableTimeConstrained());
  while (tara->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(nmra->enableTimeConstrained());
  while (nmra->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  // The deletion is accepted at timestamp 7. FQR, TARA, and NMRA use three
  // distinct grant frontiers over the same queued Remove Object Instance:
  // FQR requests 10, TARA reaches 7 inclusively, and NMRA selects the queued
  // timestamp 7 from its request-10 boundary.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(fqrReports.objectRemovalReports.empty());
  REQUIRE(taraReports.objectRemovalReports.empty());
  REQUIRE(nmraReports.objectRemovalReports.empty());

  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }

  for (auto* receiver : {fqr.get(), tara.get(), nmra.get()}) {
    while (receiver->evokeCallback(0.0)) {
    }
  }
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(fqrReports.objectRemovalReports.size() == 1U);
  REQUIRE(taraReports.objectRemovalReports.size() == 1U);
  REQUIRE(nmraReports.objectRemovalReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.front().value == L"7");
  REQUIRE(fqrReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(fqrReports.callbackOrder == std::vector<std::string>{"remove", "flush-grant"});
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"remove", "grant"});
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto requireRemoval = [&](auto const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireRemoval(fqrReports.objectRemovalReports.front());
  requireRemoval(taraReports.objectRemovalReports.front());
  requireRemoval(nmraReports.objectRemovalReports.front());

  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped regional interaction subscription replacement does not retarget a queued passel",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][explicit-source][tso][subscription-replacement]"
    "[timestamped-regional-interaction-subscription-replacement]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x49, 0x52, 0x31};
  unsigned char const secondParameterBytes[] = {0x49, 0x52, 0x32};
  unsigned char const firstTagBytes[] = {0x53, 0x55, 0x42, 0x31};
  unsigned char const secondTagBytes[] = {0x53, 0x55, 0x42, 0x32};
  ParameterHandleValueMap firstParameters;
  ParameterHandleValueMap secondParameters;
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-subscription-replacement-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-subscription-replacement-receiver",
      L"subscriber",
      federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  firstParameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  secondParameters.emplace(
      temperatureOk,
      VariableLengthData(secondParameterBytes, sizeof(secondParameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const sourceRegionA = publisher->createRegion(DimensionHandleSet{serverId});
  auto const sourceRegionB = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegionA = receiver->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegionB = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionA,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionB,
      serverId,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegionA,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegionB,
      serverId,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegionA, sourceRegionB}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{receiverRegionA, receiverRegionB}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionA}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  // The first passel captures receiverRegionA at acceptance. Replacing the
  // subscription before its callback boundary must suppress it rather than
  // retarget it to receiverRegionB or synthesize Request Retraction.
  auto const firstRetraction = publisher->sendInteractionWithRegions(
      interactionClass,
      firstParameters,
      RegionHandleSet{sourceRegionA},
      firstTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstRetraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionA}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionB}));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());

  // Later sends use the replacement subscription and source region. This
  // passel must deliver exactly once with receiverRegionB's source realization.
  auto const secondRetraction = publisher->sendInteractionWithRegions(
      interactionClass,
      secondParameters,
      RegionHandleSet{sourceRegionB},
      secondTag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondRetraction.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(secondParameterBytes,
                                     secondParameterBytes + sizeof(secondParameterBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(secondTagBytes,
                                     secondTagBytes + sizeof(secondTagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{sourceRegionB});
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "interaction", "grant"});
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionB}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegionB));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegionA));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionB));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionA));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded queued timestamped Delete Object Instance survives time-regulation disable and re-enable with changed lookahead",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-object-deletion][tso][re-enable][regulation-disable][changed-lookahead]"
    "[timestamped-object-deletion-regulation-reenable-changed-lookahead]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.query-lookahead][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x43, 0x48, 0x47, 0x2D, 0x4C, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-delete-changed-lookahead-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-delete-changed-lookahead-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  // The accepted deletion is queued at timestamp five under lookahead one.
  // Changing the producer's role and lookahead must not replace its payload or
  // recipient ledger.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == 3);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(removal.timeValue == L"5");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds a pending Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][public-process-restart-pending-attribute-ownership-query]"
    "[restore][pending-application-request]"
    "[pending-attribute-ownership-query][query-attribute-ownership]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.query-attribute-ownership][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.inform-attribute-ownership][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-saved][federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[callback-immediate][2025]") {
  auto runScenario = [](CallbackModel const callbackModel) {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
   auto const saveLabel = callbackModel == HLA_EVOKED
       ? std::wstring{L"pending-ownership-query-evoked"}
       : std::wstring{L"pending-ownership-query-immediate"};
  std::vector<unsigned char> const valueBytes{0x71U, 0x75U, 0x65U, 0x72U, 0x79U};

  auto const drain = [](RTIambassador& ambassador) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(ambassador.evokeCallback(0.0));
    }
  };
  auto const drainBoth = [&](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  ObjectInstanceHandle sourceObjectInstance;
  ObjectClassHandle sourceObjectClass;
  AttributeHandle sourceAttribute;
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceRequesterHandle;

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr,
                                                                     saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
                        L"pending-query-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(sourceRequesterHandle = requester->joinFederationExecution(
                        L"pending-query-requester", L"subscriber", federationName));

    sourceObjectClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    sourceAttribute = owner->getAttributeHandle(
        sourceObjectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(sourceObjectClass.isValid());
    REQUIRE(sourceAttribute.isValid());
    AttributeHandleSet const attributes{sourceAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        sourceObjectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        sourceObjectClass, attributes));
    REQUIRE_NOTHROW(sourceObjectInstance =
                        owner->registerObjectInstance(sourceObjectClass));
    REQUIRE(sourceObjectInstance.isValid());
    drain(*requester);
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);

    AttributeHandleValueMap values;
    values.emplace(
        sourceAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    VariableLengthData emptyTag;
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance, values, emptyTag));
    drain(*requester);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);

    // Leave the ownership-result callback evoked but undelivered. The save
    // image must retain the accepted request rather than only the queued
    // process-local closure.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->disableCallbacks());
    }
    REQUIRE_NOTHROW(requester->queryAttributeOwnership(
        sourceObjectInstance, attributes));
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());
    drain(*owner);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.pendingAttributeOwnershipQueriesPresent);
    REQUIRE(image.pendingAttributeOwnershipQueries.size() == 1U);
    auto const& query = image.pendingAttributeOwnershipQueries.front();
    auto const sourceRequesterValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceRequesterHandle);
    REQUIRE(sourceRequesterValue.has_value());
    auto const sourceObjectValue =
        rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
            sourceObjectInstance);
    REQUIRE(sourceObjectValue.has_value());
    REQUIRE(query.requestingFederateId == *sourceRequesterValue);
    REQUIRE(query.objectInstanceHandle == *sourceObjectValue);
    REQUIRE(query.reportKind == 0U);
    REQUIRE(query.requestedAttributeHandles.size() == 1U);

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->enableCallbacks());
    }
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr,
                                                                     saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshRequesterHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
                        L"pending-query-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(freshRequesterHandle = requester->joinFederationExecution(
                        L"pending-query-requester", L"subscriber", federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshRequesterHandle == sourceRequesterHandle);

    auto const freshObjectClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const freshAttribute = owner->getAttributeHandle(
        freshObjectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(freshObjectClass == sourceObjectClass);
    REQUIRE(freshAttribute == sourceAttribute);
    AttributeHandleSet const attributes{freshAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        freshObjectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        freshObjectClass, attributes));

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(requesterReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(requester->federateRestoreComplete());
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);
    auto const& ownership = requesterReports.attributeOwnershipReports.front();
    REQUIRE(ownership.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(ownership.objectInstance == sourceObjectInstance);
    REQUIRE(ownership.attributes == attributes);
    REQUIRE(ownership.owner == freshOwnerHandle);

    // The restored request is one-shot; a second evoke pass cannot replay it.
    drain(*requester);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);

    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds a pending unowned Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][public-process-restart-pending-attribute-ownership-query-unowned]"
    "[restore][pending-application-request]"
    "[pending-attribute-ownership-query-unowned][query-attribute-ownership]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.query-attribute-ownership][rti.service.send-interaction]"
    "[rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.attribute-is-not-owned][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-saved][federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
     "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
     "[callback-immediate][2025]") {
  auto runScenario = [](CallbackModel const callbackModel) {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  auto const saveLabel = callbackModel == HLA_EVOKED
      ? std::wstring{L"pending-unowned-ownership-query-evoked"}
      : std::wstring{L"pending-unowned-ownership-query-immediate"};
  std::vector<unsigned char> const valueBytes{0x71U, 0x75U, 0x65U, 0x72U, 0x79U};

  auto const drain = [](RTIambassador& ambassador) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(ambassador.evokeCallback(0.0));
    }
  };
  auto const drainBoth = [&](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  ObjectInstanceHandle sourceObjectInstance;
  ObjectClassHandle sourceObjectClass;
  AttributeHandle sourceAttribute;
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceRequesterHandle;

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr,
                                                                     saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
                        L"pending-query-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(sourceRequesterHandle = requester->joinFederationExecution(
                        L"pending-query-requester", L"subscriber", federationName));

    sourceObjectClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    sourceAttribute = owner->getAttributeHandle(
        sourceObjectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(sourceObjectClass.isValid());
    REQUIRE(sourceAttribute.isValid());
    AttributeHandleSet const attributes{sourceAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        sourceObjectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        sourceObjectClass, attributes));
    REQUIRE_NOTHROW(sourceObjectInstance =
                        owner->registerObjectInstance(sourceObjectClass));
    REQUIRE(sourceObjectInstance.isValid());
    drain(*requester);
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);

    AttributeHandleValueMap values;
    values.emplace(
        sourceAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    VariableLengthData emptyTag;
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance, values, emptyTag));
    drain(*requester);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);

    // Move the application attribute to the standard unowned state through
    // the official federate MOM adjustment interaction before accepting
    // the pending query. This keeps the request report kind distinct from
    // the completed federate-owner companion.
    auto const modifyAttributeState = owner->getInteractionClassHandle(
        standard_hla::mom::modify_attribute_state);
    auto const federateParameter = owner->getParameterHandle(
        modifyAttributeState, standard_hla::mom::federate);
    auto const objectParameter = owner->getParameterHandle(
        modifyAttributeState, standard_hla::mom::object_instance);
    auto const attributeParameter = owner->getParameterHandle(
        modifyAttributeState, standard_hla::mom::attribute);
    auto const stateParameter = owner->getParameterHandle(
        modifyAttributeState, standard_hla::mom::attribute_state);
    REQUIRE(modifyAttributeState.isValid());
    REQUIRE(federateParameter.isValid());
    REQUIRE(objectParameter.isValid());
    REQUIRE(attributeParameter.isValid());
    REQUIRE(stateParameter.isValid());
    REQUIRE_NOTHROW(owner->sendInteraction(
        modifyAttributeState,
        ParameterHandleValueMap{
            {federateParameter, sourceRequesterHandle.encode()},
            {objectParameter, sourceObjectInstance.encode()},
            {attributeParameter, sourceAttribute.encode()},
            {stateParameter, rti1516_2025::HLAinteger32BE{0}.encode()},
        },
        VariableLengthData()));
    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        sourceObjectInstance, sourceAttribute));

    // Leave the ownership-result callback evoked but undelivered. The save
    // image must retain the accepted request rather than only the queued
    // process-local closure.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->disableCallbacks());
    }
    REQUIRE_NOTHROW(requester->queryAttributeOwnership(
        sourceObjectInstance, attributes));
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());
    drain(*owner);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.pendingAttributeOwnershipQueriesPresent);
    REQUIRE(image.pendingAttributeOwnershipQueries.size() == 1U);
    auto const& query = image.pendingAttributeOwnershipQueries.front();
    auto const sourceRequesterValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceRequesterHandle);
    REQUIRE(sourceRequesterValue.has_value());
    auto const sourceObjectValue =
        rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
            sourceObjectInstance);
    REQUIRE(sourceObjectValue.has_value());
    REQUIRE(query.requestingFederateId == *sourceRequesterValue);
    REQUIRE(query.objectInstanceHandle == *sourceObjectValue);
    REQUIRE(query.reportKind == 1U);
    REQUIRE(query.requestedAttributeHandles.size() == 1U);

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->enableCallbacks());
    }
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr,
                                                                     saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshRequesterHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
                        L"pending-query-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(freshRequesterHandle = requester->joinFederationExecution(
                        L"pending-query-requester", L"subscriber", federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshRequesterHandle == sourceRequesterHandle);

    auto const freshObjectClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const freshAttribute = owner->getAttributeHandle(
        freshObjectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(freshObjectClass == sourceObjectClass);
    REQUIRE(freshAttribute == sourceAttribute);
    AttributeHandleSet const attributes{freshAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        freshObjectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        freshObjectClass, attributes));

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(requesterReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(requester->federateRestoreComplete());
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);
    auto const& ownership = requesterReports.attributeOwnershipReports.front();
    REQUIRE(ownership.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned);
    REQUIRE(ownership.objectInstance == sourceObjectInstance);
    REQUIRE(ownership.attributes == attributes);
    REQUIRE_FALSE(ownership.owner.isValid());

    // The restored request is one-shot; a second evoke pass cannot replay it.
    drain(*requester);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);

    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds a pending RTI-owned Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][public-process-restart-pending-rti-owned-attribute-ownership-query]"
    "[restore][pending-application-request][mom]"
    "[pending-attribute-ownership-query-rti-owned][query-attribute-ownership]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.query-attribute-ownership][rti.service.is-attribute-owned-by-federate]"
    "[rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.attribute-is-owned-by-rti][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-saved][federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
     "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
     "[callback-immediate][2025]") {
  auto runScenario = [](CallbackModel const callbackModel) {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-nrg-disabled-fom.xml")
          .wstring();
  auto const saveLabel = callbackModel == HLA_EVOKED
      ? std::wstring{L"pending-rti-owned-ownership-query-evoked"}
      : std::wstring{L"pending-rti-owned-ownership-query-immediate"};

  auto const drain = [](RTIambassador& ambassador) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(ambassador.evokeCallback(0.0));
    }
  };
  auto const drainBoth = [&](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  ObjectInstanceHandle sourceMomObject;
  ObjectClassHandle sourceMomClass;
  AttributeHandle sourceMomAttribute;
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceRequesterHandle;

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
                        L"pending-rti-owned-query-owner", L"observer",
                        federationName));
    REQUIRE_NOTHROW(sourceRequesterHandle = requester->joinFederationExecution(
                        L"pending-rti-owned-query-requester", L"observer",
                        federationName));

    sourceMomClass = requester->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    sourceMomAttribute = requester->getAttributeHandle(
        sourceMomClass, L"HLAfederateName");
    auto const sourceMomHandle =
        dynamic_cast<rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador*>(
            owner.get());
    REQUIRE(sourceMomHandle != nullptr);
    auto const sourceSnapshot =
        sourceMomHandle->joinedFederateMomObjectSnapshotForTesting();
    REQUIRE(sourceSnapshot);
    sourceMomObject =
        rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
            sourceSnapshot->objectInstanceHandle);
    REQUIRE(sourceMomObject.isValid());
    REQUIRE(sourceMomClass.isValid());
    REQUIRE(sourceMomAttribute.isValid());
    AttributeHandleSet const attributes{sourceMomAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        sourceMomClass, attributes));
    drain(*requester);
    REQUIRE_FALSE(requesterReports.objectDiscoveryReports.empty());

    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        sourceMomObject, sourceMomAttribute));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->disableCallbacks());
    }
    REQUIRE_NOTHROW(requester->queryAttributeOwnership(
        sourceMomObject, attributes));
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());
    // Deliver only the owner's save completion.  The requester callback queue
    // intentionally retains the accepted RTI-owned ownership result until the
    // fresh-registry restore boundary.
    drain(*owner);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.pendingAttributeOwnershipQueriesPresent);
    REQUIRE(image.pendingAttributeOwnershipQueries.size() == 1U);
    auto const& query = image.pendingAttributeOwnershipQueries.front();
    auto const sourceRequesterValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceRequesterHandle);
    REQUIRE(sourceRequesterValue.has_value());
    auto const sourceObjectValue =
        rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
            sourceMomObject);
    REQUIRE(sourceObjectValue.has_value());
    REQUIRE(query.requestingFederateId == *sourceRequesterValue);
    REQUIRE(query.objectInstanceHandle == *sourceObjectValue);
    REQUIRE(query.reportKind == 2U);
    REQUIRE(query.owningFederateId == 0U);
    REQUIRE(query.requestedAttributeHandles.size() == 1U);

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->enableCallbacks());
    }
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshRequesterHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
                        L"pending-rti-owned-query-owner", L"observer",
                        federationName));
    REQUIRE_NOTHROW(freshRequesterHandle = requester->joinFederationExecution(
                        L"pending-rti-owned-query-requester", L"observer",
                        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshRequesterHandle == sourceRequesterHandle);

    auto const freshMomClass = requester->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const freshMomAttribute = requester->getAttributeHandle(
        freshMomClass, L"HLAfederateName");
    REQUIRE(freshMomClass == sourceMomClass);
    REQUIRE(freshMomAttribute == sourceMomAttribute);
    AttributeHandleSet const attributes{freshMomAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        freshMomClass, attributes));
    drain(*requester);

    auto const freshMomHandle =
        dynamic_cast<rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador*>(
            owner.get());
    REQUIRE(freshMomHandle != nullptr);
    auto const freshSnapshot =
        freshMomHandle->joinedFederateMomObjectSnapshotForTesting();
    REQUIRE(freshSnapshot);
    auto const freshMomObject =
        rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
            freshSnapshot->objectInstanceHandle);
    REQUIRE(freshMomObject == sourceMomObject);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(requesterReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(requester->federateRestoreComplete());
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);
    auto const& ownership = requesterReports.attributeOwnershipReports.front();
    REQUIRE(ownership.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti);
    REQUIRE(ownership.objectInstance == freshMomObject);
    REQUIRE(ownership.attributes == attributes);
    REQUIRE_FALSE(ownership.owner.isValid());

    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        freshMomObject, freshMomAttribute));
    drain(*requester);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);

    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}


TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary receive-order interaction eligibility",
    "[integration][development-profile][interaction-management][delay-subscription-evaluation]"
    "[delay-subscription-evaluation-receive-order-interaction]"
    "[rti.service.send-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback]"
    "[rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks]"
    "[callback-immediate]"
    "[federate.callback.receive-interaction]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled,
                        auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const interactionFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                 "cpp" / "tests" / "data" /
                                 "parameter-handle-provider-fom.xml")
                                    .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{interactionFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-ro-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-ro-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    REQUIRE(interactionClass.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

    // The receiver is joined but has no subscription when the RO message is
    // generated.  Only the Enabled federation keeps it as a candidate until
    // the receiver's actual callback boundary. HLA_IMMEDIATE establishes the
    // same boundary by temporarily suspending callback dispatch.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->sendInteraction(
        interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    if (delaySubscriptionEvaluationEnabled) {
      REQUIRE(receiverReports.interactionReports.front().interactionClass == interactionClass);
    }

    // Clause 8.1.8 requires an actual-delivery decision from the receiver's
    // current subscriptions in both modes. An accepted recipient that
    // unsubscribes before its callback boundary must therefore be suppressed.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->sendInteraction(
        interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
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

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary timestamped interaction eligibility",
    "[integration][development-profile][interaction-management][time-management]"
    "[delay-subscription-evaluation][tso]"
    "[rti.service.send-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[callback-immediate]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled,
                        auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const interactionFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                 "cpp" / "tests" / "data" /
                                 "parameter-handle-provider-fom.xml")
                                    .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{interactionFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-tso-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-tso-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    REQUIRE(interactionClass.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (!immediate) {
      static_cast<void>(publisher->evokeCallback(0.0));
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1);

    // The timestamped message is generated while the receiver is unsubscribed.
    // In the Enabled case it waits in the federation-owned TSO queue and is
    // projected only when the recipient becomes eligible at its grant.
    auto const firstRetraction = publisher->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{},
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    if (!immediate) {
      static_cast<void>(publisher->evokeCallback(0.0));
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timestampedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& report = receiverReports.timestampedInteractionReports.front();
      REQUIRE(report.interactionClass == interactionClass);
      REQUIRE(report.timeValue == L"2");
      REQUIRE(report.sentOrderType == TIMESTAMP);
      REQUIRE(report.receivedOrderType == TIMESTAMP);
    }

    // A subscription existing at generation is not a promise of delivery:
    // current state at the second grant suppresses this otherwise queued TSO
    // interaction under both switch settings.
    auto const secondRetraction = publisher->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{},
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    if (!immediate) {
      static_cast<void>(publisher->evokeCallback(0.0));
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timestampedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
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

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers timestamped regional attribute eligibility",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[delay-subscription-evaluation][timestamped-regional-attribute-update][tso]"
    "[delay-subscription-evaluation-timestamped-regional-attribute]"
    "[rti.service.update-attribute-values]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{restaurantFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }
    unsigned char const firstValueBytes[] = {0x44, 0x53, 0x52, 0x31};
    unsigned char const secondValueBytes[] = {0x44, 0x53, 0x52, 0x32};
    AttributeHandleValueMap firstValues;
    AttributeHandleValueMap secondValues;

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-regional-attribute-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-regional-attribute-receiver",
        L"subscriber",
        federationName));

    auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
    auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
    REQUIRE(soda.isValid());
    REQUIRE(flavor.isValid());
    REQUIRE(sodaFlavor.isValid());
    AttributeHandleSet const flavorOnly{flavor};
    firstValues.emplace(
        flavor,
        VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
    secondValues.emplace(
        flavor,
        VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
    REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
        soda,
        flavorOnly,
        TIMESTAMP));

    auto const sourceRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
    auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        sodaFlavor,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(receiver->setRangeBounds(
        receiverRegion,
        sodaFlavor,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
    AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
        flavorOnly,
        RegionHandleSet{sourceRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
        flavorOnly,
        RegionHandleSet{receiverRegion},
    }};
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
        soda,
        sourcePair));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
    REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    REQUIRE_FALSE(receiver->evokeCallback(0.0));
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    while (publisher->evokeCallback(0.0)) {
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

    // The first passel is accepted while no regional declaration is active.
    // Enabled delay evaluation retains the joined recipient route so that the
    // declaration added below can make this passel eligible at its grant.
    auto const firstRetraction = publisher->updateAttributeValues(
        objectInstance,
        firstValues,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE(receiverReports.attributeReflectionReports.empty());
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    std::size_t const expectedReflections = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    REQUIRE(receiverReports.attributeReflectionReports.size() == expectedReflections);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& reflection = receiverReports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1U);
      REQUIRE(reflection.attributeValues.contains(flavor));
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) ==
              std::vector<unsigned char>(
                  firstValueBytes,
                  firstValueBytes + sizeof(firstValueBytes)));
      REQUIRE(reflection.timeValue == L"2");
      REQUIRE(reflection.sentOrderType == TIMESTAMP);
      REQUIRE(reflection.receivedOrderType == TIMESTAMP);
      REQUIRE(reflection.sentRegionsSupplied);
      REQUIRE(reflection.sentRegions.contains(sourceRegion));
    }

    // The second passel is generated while subscribed, then the declaration is
    // removed before its callback boundary. Current projection suppresses it
    // in both switch modes; delayed evaluation is not a promise to deliver a
    // stale callback after the declaration disappears.
    auto const secondRetraction = publisher->updateAttributeValues(
        objectInstance,
        secondValues,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() == expectedReflections);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);

    REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
    REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled") {
    runScenario(true);
  }
  SECTION("the omitted switch uses the Disabled default") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers timestamped regional interaction eligibility",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[delay-subscription-evaluation][timestamped-regional-interaction][tso]"
    "[delay-subscription-evaluation-timestamped-regional-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{restaurantFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }
    unsigned char const parameterBytes[] = {0x44, 0x53, 0x52, 0x49};
    unsigned char const tagBytes[] = {0x44, 0x53, 0x52, 0x54};
    ParameterHandleValueMap parameterValues;
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-regional-interaction-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-regional-interaction-receiver",
        L"subscriber",
        federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::main_course_served);
    auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
    auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
    REQUIRE(interactionClass.isValid());
    REQUIRE(temperatureOk.isValid());
    REQUIRE(serverId.isValid());
    parameterValues.emplace(
        temperatureOk,
        VariableLengthData(parameterBytes, sizeof(parameterBytes)));
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

    auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
    auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(0UL, 10UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(receiver->setRangeBounds(
        receiverRegion,
        serverId,
        RangeBounds(5UL, 15UL)));
    REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    while (publisher->evokeCallback(0.0)) {
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

    // The first passel is generated while no regional declaration is active.
    // Enabled delay evaluation keeps the joined recipient route so the
    // declaration added below can make the passel eligible at its grant;
    // Disabled evaluation has no recipient to recover.
    auto const firstRetraction = publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag,
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    std::size_t const expectedInteractions = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    REQUIRE(receiverReports.timestampedInteractionReports.size() == expectedInteractions);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& interaction = receiverReports.timestampedInteractionReports.front();
      REQUIRE(interaction.interactionClass == interactionClass);
      REQUIRE(interaction.parameterValues.size() == 1U);
      REQUIRE(interaction.parameterValues.contains(temperatureOk));
      REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(temperatureOk)) ==
              std::vector<unsigned char>(
                  parameterBytes,
                  parameterBytes + sizeof(parameterBytes)));
      REQUIRE(interaction.timeValue == L"2");
      REQUIRE(interaction.sentOrderType == TIMESTAMP);
      REQUIRE(interaction.receivedOrderType == TIMESTAMP);
      REQUIRE(interaction.sentRegionsSupplied);
      REQUIRE(interaction.sentRegions.contains(sourceRegion));
    }

    // The second passel is generated while the regional subscription is
    // active, then the declaration is removed before its callback boundary.
    // Current projection suppresses it in both switch modes.
    auto const secondRetraction = publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag,
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.timestampedInteractionReports.size() == expectedInteractions);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);

    REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled") {
    runScenario(true);
  }
  SECTION("the omitted switch uses the Disabled default") {
    runScenario(false);
  }
}

TEST_CASE(
    "Embedded Request Retraction notifies delivered interaction recipients and suppresses queued fanout",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction-request-retraction-fanout]"
    "[retract]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x52, 0x54, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"retraction-constrained", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(constrained->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The nonconstrained recipient sees the original timestamped interaction
  // immediately; the constrained recipient remains in the federation-owned
  // TSO queue until a later grant.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 1);
  REQUIRE(immediateReports.timestampedInteractionReports.front().retractionValid);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"interaction", "request-retraction"});

  // The still-pending fanout is removed before it can cross the recipient's
  // grant boundary. Advancing the regulator past its initial position releases
  // the recipient's TAR to 2 without a stale Receive Interaction callback.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  // Once the constrained recipient resigns, the next timestamped interaction
  // has no temporal-queue fanout at all. It must still retain a federation
  // ledger record so its already-delivered nonconstrained recipient can receive
  // Request Retraction.
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));

  auto const immediateOnlyRetraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(immediateOnlyRetraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 2);
  REQUIRE(immediateReports.timestampedInteractionReports.back().retractionValid);

  REQUIRE_NOTHROW(publisher->retract(immediateOnlyRetraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 2);
  REQUIRE(immediateReports.requestRetractionReports.back().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.back().encodedRetraction) ==
          variableLengthDataBytes(immediateOnlyRetraction.encode()));

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation-management services require an RTI connection",
    "[integration][federation-management][connection]") {
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->destroyFederationExecution(federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->joinFederationExecution(L"observer", federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->resignFederationExecution(NO_ACTION),
      rti1516_2025::NotConnected);
}

TEST_CASE(
    "Embedded transport loss forces the official connection-lost transition",
    "[integration][development-profile][federation-management][transport]"
    "[connection-lost-error-path]"
    "[rti.service.connection-lost][federate.callback.connection-lost]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"transport-lost",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"transport-survivor",
      L"observer",
      federationName));

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"embedded loopback transport closed"));
  REQUIRE_FALSE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"duplicate fault"));

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"embedded loopback transport closed"});
  REQUIRE(lostReports.resignationDescriptions.empty());
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      lost->joinFederationExecution(L"observer", federationName),
      rti1516_2025::NotConnected);

  // The forced membership cleanup is observable from a surviving federate:
  // the lost endpoint no longer appears in the execution member report.
  REQUIRE_NOTHROW(surviving->listFederationExecutionMembers(federationName));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.federationExecutionMemberReports.size() == 1);
  REQUIRE(survivingReports.federationExecutionMemberReports.front().members.size() == 1);
  REQUIRE(
      survivingReports.federationExecutionMemberReports.front().members.front().federateName ==
      L"transport-survivor");

  // The forced transition returns the RTI ambassador to the ordinary
  // disconnected lifecycle, so the same object can establish a fresh
  // official connection after the fault callback has been evoked.
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss reports HLAreportFederateLost to subscribed survivors",
    "[integration][development-profile][federation-management][transport][mom]"
    "[rti.service.connection-lost][federate.callback.connection-lost]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador observerReports;
  ReportingFederateAmbassador unsubscribedReports;
  auto lost = makeRti();
  auto observer = makeRti();
  auto unsubscribed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(unsubscribed->connect(unsubscribedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"lost-mom-federate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lost-mom-observer",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(unsubscribed->joinFederationExecution(
      L"lost-mom-unsubscribed",
      L"observer",
      federationName));

  auto const reportClass = observer->getInteractionClassHandle(reportClassName);
  auto const federateParameter = observer->getParameterHandle(reportClass, standard_hla::mom::federate);
  auto const federateNameParameter = observer->getParameterHandle(reportClass, standard_hla::mom::federate_name);
  auto const timestampParameter = observer->getParameterHandle(reportClass, standard_hla::mom::time_stamp);
  auto const faultDescriptionParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::fault_description);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(federateNameParameter.isValid());
  REQUIRE(timestampParameter.isValid());
  REQUIRE(faultDescriptionParameter.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  // The standard requires a last known time position when the lost federate
  // was time regulating. Its initial granted time is zero in this selected
  // logical-time implementation, which gives the report a concrete official
  // HLAlogicalTime value without manufacturing a private timestamp type.
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(lostReports.timeRegulationEnabledReports.front().value == L"0");

  std::wstring const faultDescription = L"loss-report transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE(unsubscribedReports.interactionReports.empty());

  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType == observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  // This is Umbra's deliberately narrow adapter representation for an
  // RTI-originated report, not a source claim that the standard assigns an
  // invalid handle to the producer argument. RL-065 retains that distinction.
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  auto const federateValue = report.parameterValues.find(federateParameter);
  auto const federateNameValue = report.parameterValues.find(federateNameParameter);
  auto const timestampValue = report.parameterValues.find(timestampParameter);
  auto const faultDescriptionValue = report.parameterValues.find(faultDescriptionParameter);
  REQUIRE(federateValue != report.parameterValues.end());
  REQUIRE(federateNameValue != report.parameterValues.end());
  REQUIRE(timestampValue != report.parameterValues.end());
  REQUIRE(faultDescriptionValue != report.parameterValues.end());
  REQUIRE(observer->decodeFederateHandle(federateValue->second) == lostFederate);
  rti1516_2025::HLAunicodeString decodedFederateName;
  REQUIRE_NOTHROW(decodedFederateName.decode(federateNameValue->second));
  REQUIRE(decodedFederateName.get() == L"lost-mom-federate");
  rti1516_2025::HLAinteger64Time decodedTimestamp;
  REQUIRE_NOTHROW(decodedTimestamp.decode(timestampValue->second));
  REQUIRE(decodedTimestamp.getTime() == 0);
  rti1516_2025::HLAunicodeString decodedFaultDescription;
  REQUIRE_NOTHROW(decodedFaultDescription.decode(faultDescriptionValue->second));
  REQUIRE(decodedFaultDescription.get() == faultDescription);

  REQUIRE_FALSE(unsubscribed->evokeCallback(0.0));
  REQUIRE(unsubscribedReports.interactionReports.empty());
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});

  // The faulted endpoint is disconnected after its callback, while both
  // surviving joined federates can leave normally and destroy the execution.
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unsubscribed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(unsubscribed->disconnect());
}

TEST_CASE(
    "Embedded transport loss delivers Federate Lost reports before automatic removals to every subscribed survivor",
    "[integration][development-profile][federation-management][transport][mom]"
    "[object-management][connection-lost-report-ordering]"
    "[rti.service.connection-lost][rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.remove-object-instance]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto lost = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"ordered-loss-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"ordered-loss-first-survivor",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"ordered-loss-second-survivor",
      L"subscriber",
      federationName));

  auto const firstReportClass = first->getInteractionClassHandle(reportClassName);
  auto const firstFaultDescriptionParameter = first->getParameterHandle(
      firstReportClass,
      standard_hla::mom::fault_description);
  auto const secondReportClass = second->getInteractionClassHandle(reportClassName);
  auto const secondFaultDescriptionParameter = second->getParameterHandle(
      secondReportClass,
      standard_hla::mom::fault_description);
  REQUIRE(firstReportClass.isValid());
  REQUIRE(firstFaultDescriptionParameter.isValid());
  REQUIRE(secondReportClass.isValid());
  REQUIRE(secondFaultDescriptionParameter.isValid());
  REQUIRE_NOTHROW(first->subscribeInteractionClass(firstReportClass));
  REQUIRE_NOTHROW(second->subscribeInteractionClass(secondReportClass));

  // The report planner requires a concrete last-known time for a lost
  // time-regulating federate. Establish that state before creating the object
  // whose automatic deletion will provide the second callback in each queue.
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions.empty());

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(first->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(second->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  REQUIRE_FALSE(first->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(second->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS);

  std::wstring const faultDescription = L"ordered multi-survivor transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(firstReports.interactionReports.empty());
  REQUIRE(firstReports.objectRemovalReports.empty());
  REQUIRE(secondReports.interactionReports.empty());
  REQUIRE(secondReports.objectRemovalReports.empty());

  // The RTI queues HLAreportFederateLost before the automatic DELETE_OBJECTS
  // removal for each recipient. One evoked callback at a time makes that
  // ordering observable independently on both surviving callback sessions.
  REQUIRE(first->evokeCallback(0.0));
  REQUIRE(firstReports.interactionReports.size() == 1U);
  REQUIRE(firstReports.objectRemovalReports.empty());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE(firstReports.objectRemovalReports.size() == 1U);
  REQUIRE(firstReports.objectRemovalReports.front().objectInstance == objectInstance);

  REQUIRE(second->evokeCallback(0.0));
  REQUIRE(secondReports.interactionReports.size() == 1U);
  REQUIRE(secondReports.objectRemovalReports.empty());
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE(secondReports.objectRemovalReports.size() == 1U);
  REQUIRE(secondReports.objectRemovalReports.front().objectInstance == objectInstance);

  auto const& firstReport = firstReports.interactionReports.front();
  auto const& secondReport = secondReports.interactionReports.front();
  REQUIRE(firstReport.interactionClass == firstReportClass);
  REQUIRE(secondReport.interactionClass == secondReportClass);
  REQUIRE(firstReport.transportationType == first->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(secondReport.transportationType == second->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(firstReport.producingFederate.isValid());
  REQUIRE_FALSE(secondReport.producingFederate.isValid());
  REQUIRE(firstReport.parameterValues.size() == 4U);
  REQUIRE(secondReport.parameterValues.size() == 4U);
  REQUIRE(
      variableLengthDataBytes(
          firstReport.parameterValues.at(firstFaultDescriptionParameter)) ==
      variableLengthDataBytes(
          secondReport.parameterValues.at(secondFaultDescriptionParameter)));

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
}

TEST_CASE(
    "Embedded transport loss delivers timestamped interactions through the lost federate's last-known time",
    "[integration][development-profile][federation-management][transport]"
    "[interaction-management][time-management][connection-lost-tso-cutoff]"
    "[rti.service.connection-lost][rti.service.send-interaction]"
    "[rti.service.time-advance-request]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4C, 0x4F, 0x53, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = lost->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xC0, 0xFF, 0xEE};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(lost->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions.empty());

  // Queue a message at time 6, then advance the time-regulating publisher to
  // that same time.  The subscriber's matching TAR remains pending until the
  // producer grant establishes its last-known time position.
  auto const retraction = lost->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE(survivingReports.timeAdvanceGrantReports.empty());

  // IEEE 1516.1-2025 4.4 requires delivery to remaining subscribers for all
  // TSO messages at or before the lost time-regulating federate's last known
  // time.  A message after that cutoff is deliberately not asserted because
  // the standard permits either outcome there.
  std::wstring const faultDescription = L"timestamped cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.timestampedInteractionReports.empty());

  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  auto const& report = survivingReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.producingFederate == lostFederate);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          variableLengthDataBytes(parameterValues.at(identifier)));
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  // The queued message is consumed exactly once; the faulted endpoint itself
  // receives the official Connection Lost callback before it can reconnect.
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss delivers timestamped interactions before the lost federate's last-known time",
    "[integration][development-profile][federation-management][transport]"
    "[interaction-management][time-management][connection-lost-tso-cutoff]"
    "[rti.service.connection-lost][rti.service.send-interaction]"
    "[rti.service.time-advance-request]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4C, 0x45, 0x53, 0x53};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"less-cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"less-cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = lost->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0x5A, 0x11};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(lost->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  // The TSO payload is strictly before the eventual loss boundary.  The
  // recipient nevertheless requests time 6, so it cannot receive the time-5
  // interaction before the regulating publisher reaches the boundary.
  auto const retraction = lost->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE(survivingReports.timeAdvanceGrantReports.empty());

  std::wstring const faultDescription = L"strictly-before cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.timestampedInteractionReports.empty());

  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  auto const& report = survivingReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.producingFederate == lostFederate);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"5");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          variableLengthDataBytes(parameterValues.at(identifier)));
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss schedules cutoff TSO delivery requested after the loss",
    "[integration][development-profile][federation-management][transport]"
    "[interaction-management][time-management][connection-lost-tso-cutoff]"
    "[rti.service.connection-lost][rti.service.send-interaction]"
    "[rti.service.time-advance-request]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x46, 0x55, 0x54, 0x55, 0x52, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"later-cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"later-cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = lost->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA6, 0xA7};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(lost->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (lost->evokeCallback(0.0)) {
  }

  auto const retraction = lost->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");

  // No subscriber advance exists when the publisher leaves. The same
  // at-or-before-loss obligation must remain until a later TAR establishes
  // the recipient's callback boundary.
  std::wstring const faultDescription = L"later timestamped cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));

  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  auto const& report = survivingReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.producingFederate == lostFederate);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          variableLengthDataBytes(parameterValues.at(identifier)));
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss reports the TSO cutoff through HLAreportFederateLost",
    "[integration][development-profile][federation-management][transport][mom]"
    "[interaction-management][time-management][asynchronous-delivery]"
    "[connection-lost-tso-cutoff]"
    "[rti.service.connection-lost][rti.service.send-interaction]"
    "[rti.service.time-advance-request][rti.service.enable-asynchronous-delivery]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;
  std::wstring const tsoInteractionClassName =
      fixture_hla::fom::customer_seated;

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"report-cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"report-cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const reportClass = surviving->getInteractionClassHandle(reportClassName);
  auto const reportTimestamp = surviving->getParameterHandle(reportClass, standard_hla::mom::time_stamp);
  auto const tsoInteractionClass = lost->getInteractionClassHandle(tsoInteractionClassName);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTimestamp.isValid());
  REQUIRE(tsoInteractionClass.isValid());
  REQUIRE_NOTHROW(lost->publishInteractionClass(tsoInteractionClass));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(tsoInteractionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(tsoInteractionClass));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  // HLAreportFederateLost is receive order.  §8.15 therefore makes this
  // explicitly enabled survivor able to receive it both while advancing and
  // after its matching grant; the test does not manufacture a MOM-specific
  // exception to the ordinary RO delivery rule.
  REQUIRE_NOTHROW(surviving->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  // The official Restaurant FOM also carries an interaction-relevance
  // advisory for this class.  Drain that unrelated setup callback alongside
  // Time Regulation Enabled rather than making it part of this fault test.
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  // Advance the time-regulating publisher to the timestamped interaction's
  // exact time before the fault.  The loss report's HLAtimeStamp and the
  // required queued TSO delivery must therefore describe one same boundary.
  auto const retraction = lost->sendInteraction(
      tsoInteractionClass,
      ParameterHandleValueMap{},
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(survivingReports.interactionReports.empty());
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE(survivingReports.timeAdvanceGrantReports.empty());

  std::wstring const faultDescription = L"report and cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.interactionReports.empty());
  REQUIRE(survivingReports.timestampedInteractionReports.empty());

  // The three independent callbacks (RO report, cutoff TSO interaction, and
  // grant) are permitted to require more than one zero-duration Evoke call.
  // Drain the callback queue without introducing an ordering claim between
  // the RO report and the timestamped interaction.
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.interactionReports.size() == 1U);
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});

  auto const& lossReport = survivingReports.interactionReports.front();
  REQUIRE(lossReport.interactionClass == reportClass);
  REQUIRE(lossReport.producingFederate.isValid() == false);
  auto const reportTimestampValue = lossReport.parameterValues.find(reportTimestamp);
  REQUIRE(reportTimestampValue != lossReport.parameterValues.end());
  rti1516_2025::HLAinteger64Time decodedReportTimestamp;
  REQUIRE_NOTHROW(decodedReportTimestamp.decode(reportTimestampValue->second));
  REQUIRE(decodedReportTimestamp.getTime() == 6);

  auto const& tsoReport = survivingReports.timestampedInteractionReports.front();
  REQUIRE(tsoReport.interactionClass == tsoInteractionClass);
  REQUIRE(tsoReport.producingFederate == lostFederate);
  REQUIRE(tsoReport.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(tsoReport.timeValue == L"6");
  REQUIRE(tsoReport.sentOrderType == TIMESTAMP);
  REQUIRE(tsoReport.receivedOrderType == TIMESTAMP);
  REQUIRE(tsoReport.retractionSupplied);
  REQUIRE(tsoReport.retractionValid);
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  // This case intentionally does not prescribe a callback order between the
  // receive-order MOM report and the timestamped application interaction; it
  // pins their common cutoff value, while the existing TSO cases pin delivery
  // before the matching grant.
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss delivers HLAreportFederateLost immediately to immediate subscribers",
    "[integration][development-profile][federation-management][transport][mom]"
    "[callback-immediate][rti.service.connection-lost]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador observerReports;
  auto lost = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"immediate-lost-mom-federate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"immediate-mom-observer",
      L"observer",
      federationName));

  auto const reportClass = observer->getInteractionClassHandle(reportClassName);
  auto const federateParameter = observer->getParameterHandle(reportClass, standard_hla::mom::federate);
  auto const faultDescriptionParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::fault_description);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(faultDescriptionParameter.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  // HLA_IMMEDIATE must enter the regulation-enabled callback during the
  // service call, leaving a real last-known logical time for the MOM report.
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(lostReports.timeRegulationEnabledReports.front().value == L"0");

  std::wstring const faultDescription = L"immediate loss-report transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));

  // Neither endpoint is evoked here.  The report must already have entered
  // the subscribed survivor's callback before the transport-fault source
  // returns, while the faulted endpoint receives its own Connection Lost.
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);
  auto const federateValue = report.parameterValues.find(federateParameter);
  auto const faultDescriptionValue = report.parameterValues.find(faultDescriptionParameter);
  REQUIRE(federateValue != report.parameterValues.end());
  REQUIRE(faultDescriptionValue != report.parameterValues.end());
  REQUIRE(observer->decodeFederateHandle(federateValue->second) == lostFederate);
  rti1516_2025::HLAunicodeString decodedFaultDescription;
  REQUIRE_NOTHROW(decodedFaultDescription.decode(faultDescriptionValue->second));
  REQUIRE(decodedFaultDescription.get() == faultDescription);

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded transport loss DDM-filters HLAreportFederateLost regional subscriptions",
    "[integration][development-profile][federation-management][transport][mom][ddm]"
    "[rti.service.connection-lost][rti.service.subscribe-interaction-class-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador matchingReports;
  ReportingFederateAmbassador disjointReports;
  auto lost = makeRti();
  auto matching = makeRti();
  auto disjoint = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(matching->connect(matchingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(disjoint->connect(disjointReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"lost-regional-report-federate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(matching->joinFederationExecution(
      L"matching-regional-report-observer",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(disjoint->joinFederationExecution(
      L"disjoint-regional-report-observer",
      L"observer",
      federationName));

  auto const reportClass = matching->getInteractionClassHandle(reportClassName);
  auto const federateDimension = matching->getDimensionHandle(standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateDimension.isValid());
  auto const normalizedLost = matching->normalizeFederateHandle(lostFederate);
  REQUIRE(normalizedLost < std::numeric_limits<unsigned long>::max());
  // Normalization does not promise unique or sequential values, so choose a
  // point immediately beside the lost federate's point rather than comparing
  // another federate's opaque coordinate.
  auto const disjointPoint = normalizedLost == 0UL ? 1UL : normalizedLost - 1UL;
  auto const matchingRegion = matching->createRegion(DimensionHandleSet{federateDimension});
  auto const disjointRegion = disjoint->createRegion(DimensionHandleSet{federateDimension});
  REQUIRE_NOTHROW(matching->setRangeBounds(
      matchingRegion,
      federateDimension,
      RangeBounds(normalizedLost, normalizedLost + 1UL)));
  REQUIRE_NOTHROW(disjoint->setRangeBounds(
      disjointRegion,
      federateDimension,
      RangeBounds(disjointPoint, disjointPoint + 1UL)));
  REQUIRE_NOTHROW(matching->commitRegionModifications(RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->commitRegionModifications(RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(matching->subscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->subscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{disjointRegion}));

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"regional loss-report transport fault"));
  REQUIRE_FALSE(matching->evokeCallback(0.0));
  REQUIRE(matchingReports.interactionReports.size() == 1U);
  REQUIRE(matchingReports.interactionReports.front().interactionClass == reportClass);
  REQUIRE_FALSE(matchingReports.interactionReports.front().sentRegionsSupplied);
  REQUIRE_FALSE(disjoint->evokeCallback(0.0));
  REQUIRE(disjointReports.interactionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"regional loss-report transport fault"});
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(matching->unsubscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->unsubscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(matching->deleteRegion(matchingRegion));
  REQUIRE_NOTHROW(disjoint->deleteRegion(disjointRegion));
  REQUIRE_NOTHROW(matching->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(disjoint->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(matching->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(matching->disconnect());
  REQUIRE_NOTHROW(disjoint->disconnect());
}

TEST_CASE(
    "Embedded transport loss DDM-delivers HLAreportFederateLost immediately to matching subscribers",
    "[integration][development-profile][federation-management][transport][mom][ddm]"
    "[callback-immediate][rti.service.connection-lost]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador matchingReports;
  ReportingFederateAmbassador disjointReports;
  auto lost = makeRti();
  auto matching = makeRti();
  auto disjoint = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(matching->connect(matchingReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(disjoint->connect(disjointReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"immediate-lost-regional-report-federate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(matching->joinFederationExecution(
      L"immediate-matching-regional-report-observer",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(disjoint->joinFederationExecution(
      L"immediate-disjoint-regional-report-observer",
      L"observer",
      federationName));

  auto const reportClass = matching->getInteractionClassHandle(reportClassName);
  auto const federateDimension = matching->getDimensionHandle(standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateDimension.isValid());
  auto const normalizedLost = matching->normalizeFederateHandle(lostFederate);
  REQUIRE(normalizedLost < std::numeric_limits<unsigned long>::max());
  auto const disjointPoint = normalizedLost == 0UL ? 1UL : normalizedLost - 1UL;
  auto const matchingRegion = matching->createRegion(DimensionHandleSet{federateDimension});
  auto const disjointRegion = disjoint->createRegion(DimensionHandleSet{federateDimension});
  REQUIRE_NOTHROW(matching->setRangeBounds(
      matchingRegion,
      federateDimension,
      RangeBounds(normalizedLost, normalizedLost + 1UL)));
  REQUIRE_NOTHROW(disjoint->setRangeBounds(
      disjointRegion,
      federateDimension,
      RangeBounds(disjointPoint, disjointPoint + 1UL)));
  REQUIRE_NOTHROW(matching->commitRegionModifications(RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->commitRegionModifications(RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(matching->subscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->subscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  std::wstring const faultDescription = L"immediate regional loss-report transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));

  // The route remains DDM-filtered in the immediate model. Neither survivor
  // is evoked: the matching point-range has already received the report while
  // the adjacent non-overlap remains silent.
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE(matchingReports.interactionReports.size() == 1U);
  REQUIRE(matchingReports.interactionReports.front().interactionClass == reportClass);
  REQUIRE(matchingReports.interactionReports.front().parameterValues.size() == 4U);
  REQUIRE_FALSE(matchingReports.interactionReports.front().producingFederate.isValid());
  REQUIRE_FALSE(matchingReports.interactionReports.front().sentRegionsSupplied);
  REQUIRE(disjointReports.interactionReports.empty());

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(matching->unsubscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->unsubscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(matching->deleteRegion(matchingRegion));
  REQUIRE_NOTHROW(disjoint->deleteRegion(disjointRegion));
  REQUIRE_NOTHROW(matching->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(disjoint->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(matching->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(matching->disconnect());
  REQUIRE_NOTHROW(disjoint->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic delete resign directive",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.remove-object-instance]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-delete-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.empty());

  // Do not rely on the current FDD default: verify that the per-federate
  // directive selected through the official support service controls the
  // forced-resignation disposition.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"automatic delete transport fault"});
  REQUIRE(lostReports.resignationDescriptions.empty());
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  auto const& removal = survivingReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == lostFederate);
  // Connection Lost performs a resignation on behalf of the lost federate.
  // The returned identity remains a valid designator even though the member
  // report below no longer lists it as joined.
  REQUIRE(surviving->getFederateName(lostFederate) == L"automatic-delete-lost");
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(surviving->listFederationExecutionMembers(federationName));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.federationExecutionMemberReports.size() == 1);
  REQUIRE(survivingReports.federationExecutionMemberReports.front().members.size() == 1);
  REQUIRE(
      survivingReports.federationExecutionMemberReports.front().members.front().federateName ==
      L"automatic-delete-survivor");

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic unconditional-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-divest-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-divest-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = lost->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);

  // IEEE 1516.1-2025 §4.1.5 requires loss cleanup to use the member's
  // Automatic Resign Directive. Unlike the delete branch above, directive 1
  // retains the object while unconditionally divesting its owned attributes.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic unconditional-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic unconditional-divest transport fault"});
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == expectedAssumption);
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(surviving->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the bounded automatic NoAction forced-resign policy",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[automatic-resign-no-action]"
    "[connection-lost-automatic-resign]"
    "[connection-lost-no-action-policy]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-no-action-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-no-action-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = lost->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);

  // NO_ACTION is the configured directive, but forced loss still cannot leave
  // a departed federate owning attributes.  The bounded policy is therefore
  // to retain the object and offer its formerly owned attributes, without an
  // automatic Remove Object Instance callback.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::NO_ACTION));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic NoAction transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"automatic NoAction transport fault"});
  REQUIRE_FALSE(surviving->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == expectedAssumption);
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(surviving->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(objectInstance, privilegeToDelete));
  REQUIRE(surviving->getFederateName(lostFederate) == L"automatic-no-action-lost");

  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded final-federate transport loss forces directive two",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][connection-lost-final-federate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.reserve-object-instance-name]"
    "[connection-lost-final-federate-directive-two]"
    "[federate.callback.connection-lost]"
    "[federate.callback.object-instance-name-reservation-succeeded]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = rti->joinFederationExecution(
      L"automatic-final-lost",
      L"publisher",
      federationName));

  auto const server = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle originalObject;
  REQUIRE_NOTHROW(originalObject = rti->registerObjectInstance(server));
  auto const reusableObjectName = rti->getObjectInstanceName(originalObject);

  // 4.12.4 applies directive two when the final joined federate leaves.  A
  // transport loss must use that same final-member rule even when the
  // configured automatic directive is NO_ACTION.
  REQUIRE_NOTHROW(rti->setAutomaticResignDirective(NO_ACTION));
  REQUIRE(rti->getAutomaticResignDirective() == NO_ACTION);
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *rti,
      L"final-federate transport fault"));
  REQUIRE(reports.faultDescriptions.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.faultDescriptions ==
          std::vector<std::wstring>{L"final-federate transport fault"});
  REQUIRE(reports.resignationDescriptions.empty());
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);

  // The federation execution remains available after its last member is
  // removed.  Rejoining with a fresh lifetime must be able to reserve the
  // deleted object's name, proving that the final-member cleanup used
  // directive two rather than the configured NO_ACTION policy.
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"automatic-final-rejoined",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(rti->reserveObjectInstanceName(reusableObjectName));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      reports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
      reusableObjectName);

  ObjectInstanceHandle replacementObject;
  REQUIRE_NOTHROW(
      replacementObject = rti->registerObjectInstance(server, reusableObjectName));
  REQUIRE(replacementObject.isValid());
  REQUIRE(replacementObject != originalObject);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Immediate callbacks apply the final-federate forced directive-two rule synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][connection-lost-final-federate-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.reserve-object-instance-name]"
    "[federate.callback.connection-lost]"
    "[federate.callback.object-instance-name-reservation-succeeded]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"automatic-final-immediate-lost",
      L"publisher",
      federationName));

  auto const server = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle originalObject;
  REQUIRE_NOTHROW(originalObject = rti->registerObjectInstance(server));
  auto const reusableObjectName = rti->getObjectInstanceName(originalObject);

  // The final-member rule is directive two even when the configured automatic
  // directive is NO_ACTION.  HLA_IMMEDIATE must deliver Connection Lost before
  // failEmbeddedTransportConnectionForTesting returns.
  REQUIRE_NOTHROW(rti->setAutomaticResignDirective(rti1516_2025::NO_ACTION));
  REQUIRE(rti->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *rti,
      L"final-federate immediate transport fault"));
  REQUIRE(reports.faultDescriptions ==
          std::vector<std::wstring>{L"final-federate immediate transport fault"});
  REQUIRE(reports.resignationDescriptions.empty());
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);

  // Rejoining begins a new lifetime.  The old object name must be reservable,
  // proving that final-member cleanup used directive two synchronously.
  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"automatic-final-immediate-rejoined",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(rti->reserveObjectInstanceName(reusableObjectName));
  REQUIRE(reports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      reports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
      reusableObjectName);

  ObjectInstanceHandle replacementObject;
  REQUIRE_NOTHROW(
      replacementObject = rti->registerObjectInstance(server, reusableObjectName));
  REQUIRE(replacementObject.isValid());
  REQUIRE(replacementObject != originalObject);

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Immediate callbacks apply the bounded automatic NoAction forced-resign policy synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][connection-lost-automatic-no-action-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[connection-lost-no-action-policy-immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      surviving->connect(survivingReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-no-action-immediate-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-no-action-immediate-survivor",
      L"subscriber",
      federationName));

  auto const server =
      lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency =
      lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = lost->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(
      surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(
      surviving->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::NO_ACTION));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);

  std::wstring const faultDescription =
      L"automatic NoAction immediate transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));

  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption =
      survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == expectedAssumption);
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(surviving->getObjectInstanceHandle(objectInstanceName) ==
          objectInstance);
  REQUIRE_FALSE(
      surviving->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(
      objectInstance,
      privilegeToDelete));
  REQUIRE(surviving->getFederateName(lostFederate) ==
          L"automatic-no-action-immediate-lost");

  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic delete-then-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[multi-federate-callback-ordering]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-delete-then-divest-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-then-divest-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  survivingReports.recordReceiveOrderObjectRemovalInCallbackOrder = true;
  survivingReports.onRequestAttributeOwnershipAssumption = [&survivingReports]() {
    survivingReports.callbackOrder.push_back("assumption");
  };

  // The survivor owns this first object and deliberately transfers only its
  // Efficiency attribute to the federate that will be lost. It stays alive
  // after directive 4 because the lost federate never owns its delete
  // privilege.
  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = surviving->registerObjectInstance(server));
  auto const retainedObjectName = surviving->getObjectInstanceName(retainedObject);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  unsigned char const manualDivestitureTagBytes[] = {0xD4, 0x25};
  VariableLengthData const manualDivestitureTag(
      manualDivestitureTagBytes,
      sizeof(manualDivestitureTagBytes));
  REQUIRE_NOTHROW(surviving->unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      efficiencyOnly,
      manualDivestitureTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().objectInstance == retainedObject);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().attributes == efficiencyOnly);

  unsigned char const acquisitionTagBytes[] = {0xD5, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      efficiencyOnly,
      acquisitionTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(lost->isAttributeOwnedByFederate(retainedObject, efficiency));

  // The lost federate owns delete privilege for this separately registered
  // object. Directive 4 must delete it before divesting the transferred
  // attribute on retainedObject.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance == deletedObject);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);

  survivingReports.callbackOrder.clear();
  REQUIRE(survivingReports.callbackOrder.empty());
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete-then-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic delete-then-divest transport fault"});
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance == deletedObject);
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  // Keep the embedded recipient-local dispatch sequence explicit as
  // implementation regression coverage, not as a cross-service ordering
  // claim from the standard.
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"assumption", "remove"});
  REQUIRE(surviving->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(retainedObject, efficiency));

  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Immediate callbacks apply the configured automatic delete-then-divest directive synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][connection-lost-automatic-delete-then-divest-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[multi-federate-callback-ordering]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      surviving->connect(survivingReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-delete-then-divest-immediate-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-then-divest-immediate-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency =
      lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(
      surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(
      surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  survivingReports.recordReceiveOrderObjectRemovalInCallbackOrder = true;
  survivingReports.onRequestAttributeOwnershipAssumption = [&survivingReports]() {
    survivingReports.callbackOrder.push_back("assumption");
  };

  // The survivor owns this first object and transfers only its Efficiency
  // attribute to the federate that will be lost. It stays alive after
  // DELETE_OBJECTS_THEN_DIVEST because the lost federate never owns delete
  // privilege for it.
  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = surviving->registerObjectInstance(server));
  auto const retainedObjectName = surviving->getObjectInstanceName(retainedObject);
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  unsigned char const manualDivestitureTagBytes[] = {0xD4, 0x25};
  VariableLengthData const manualDivestitureTag(
      manualDivestitureTagBytes,
      sizeof(manualDivestitureTagBytes));
  REQUIRE_NOTHROW(surviving->unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      efficiencyOnly,
      manualDivestitureTag));
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().objectInstance ==
          retainedObject);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().attributes ==
          efficiencyOnly);

  unsigned char const acquisitionTagBytes[] = {0xD5, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(lostReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(lost->isAttributeOwnedByFederate(retainedObject, efficiency));

  // The lost federate owns delete privilege for this separately registered
  // object. Directive 4 must delete it before divesting the transferred
  // attribute on retainedObject.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  REQUIRE(lost->getAutomaticResignDirective() ==
          rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);

  survivingReports.callbackOrder.clear();
  REQUIRE(survivingReports.callbackOrder.empty());
  // HLA_IMMEDIATE has already delivered the earlier ownership-assumption
  // callback from the setup transfer. Start a fresh report window for the
  // transport-fault transition itself.
  survivingReports.attributeOwnershipAssumptionReports.clear();
  survivingReports.objectRemovalReports.clear();
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete-then-divest immediate transport fault"));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic delete-then-divest immediate transport fault"});
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance ==
          deletedObject);
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption =
      survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  // Keep the embedded recipient-local dispatch sequence explicit as
  // implementation regression coverage, not as a cross-service ordering
  // claim from the standard.
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"assumption", "remove"});
  REQUIRE(surviving->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(retainedObject, efficiency));

  REQUIRE_NOTHROW(surviving->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Immediate callbacks apply the configured automatic delete-objects directive synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][connection-lost-automatic-delete-objects-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.remove-object-instance]"
    "[multi-federate-callback-ordering]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      surviving->connect(survivingReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-delete-objects-immediate-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-objects-immediate-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));

  // The lost federate owns this object and therefore gives directive 2 one
  // delete-privileged object to remove at the transport-fault boundary.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance ==
          deletedObject);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS);
  REQUIRE(survivingReports.objectRemovalReports.empty());

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete-objects immediate transport fault"));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic delete-objects immediate transport fault"});
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance ==
          deletedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss cancels the lost federate's pending ownership acquisition",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto lost = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"automatic-cancel-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-cancel-lost",
      L"candidate",
      federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"automatic-cancel-survivor",
      L"candidate",
      federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  while (lost->evokeCallback(0.0)) {
  }
  while (candidate->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1);

  unsigned char const acquisitionTagBytes[] = {0xD6, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic pending-acquisition cancellation transport fault"));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic pending-acquisition cancellation transport fault"});

  // The old requester is gone before the owner can receive its queued release
  // request. Delivery must recheck registry state and suppress that stale
  // work, then the actual surviving candidate becomes eligible for a later
  // unconditional divestiture offer.
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  unsigned char const divestitureTagBytes[] = {0xD7, 0x25};
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      efficiencyOnly,
      divestitureTag));
  while (candidate->evokeCallback(0.0)) {
  }
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = candidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE_FALSE(candidate->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded MOM HLAmodifyAttributeState changes application ownership without callbacks",
    "[integration][development-profile][federation-management][mom][ownership-management]"
    "[rti.service.send-interaction][rti.service.query-attribute-ownership]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador targetReports;
  auto owner = makeRti();
  auto target = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(target->connect(targetReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle targetFederate;
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mom-modify-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(targetFederate = target->joinFederationExecution(
      L"mom-modify-target", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const modifyAttributeState = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState");
  auto const federateParameter = owner->getParameterHandle(
      modifyAttributeState,
      L"HLAfederate");
  auto const objectParameter = owner->getParameterHandle(
      modifyAttributeState,
      L"HLAobjectInstance");
  auto const attributeParameter = owner->getParameterHandle(
      modifyAttributeState,
      L"HLAattribute");
  auto const stateParameter = owner->getParameterHandle(
      modifyAttributeState,
      L"HLAattributeState");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(modifyAttributeState.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(objectParameter.isValid());
  REQUIRE(attributeParameter.isValid());
  REQUIRE(stateParameter.isValid());

  AttributeHandleSet const attributeSet{reliableBaseA};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributeSet));
  REQUIRE_NOTHROW(owner->subscribeObjectClassAttributes(child, attributeSet));
  REQUIRE_NOTHROW(target->subscribeObjectClassAttributes(child, attributeSet));
  REQUIRE_NOTHROW(target->publishObjectClassAttributes(child, attributeSet));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(target->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(targetReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(target->getKnownObjectClassHandle(objectInstance) == child);

  // Establish that the discovering federate's known-instance route can
  // receive the ordinary query callback before the MOM control path changes
  // ownership.
  REQUIRE_NOTHROW(target->queryAttributeOwnership(objectInstance, attributeSet));
  REQUIRE(targetReports.attributeOwnershipReports.empty());
  REQUIRE_FALSE(target->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(targetReports.attributeOwnershipReports.size() == 1U);
  REQUIRE(targetReports.attributeOwnershipReports.front().kind ==
          ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
  REQUIRE(targetReports.attributeOwnershipReports.front().owner == owner->getFederateHandle(
      L"mom-modify-owner"));
  targetReports.attributeOwnershipReports.clear();

  auto makeModifyValues = [&](bool owned) {
    return ParameterHandleValueMap{
        {federateParameter, targetFederate.encode()},
        {objectParameter, objectInstance.encode()},
        {attributeParameter, reliableBaseA.encode()},
        {stateParameter, rti1516_2025::HLAinteger32BE{owned ? 1 : 0}.encode()},
    };
  };
  auto sendModify = [&](bool owned) {
    REQUIRE_NOTHROW(owner->sendInteraction(
        modifyAttributeState,
        makeModifyValues(owned),
        VariableLengthData()));
  };

  // The target must be publishing before the MOM interaction can assign
  // ownership.  No acquisition/release callback is generated by this direct
  // control path.
  sendModify(true);
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(target->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(targetReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_NOTHROW(target->queryAttributeOwnership(objectInstance, attributeSet));
  REQUIRE(targetReports.attributeOwnershipReports.empty());
  REQUIRE_FALSE(target->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(targetReports.attributeOwnershipReports.size() == 1U);
  REQUIRE(targetReports.attributeOwnershipReports.front().kind ==
          ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
  REQUIRE(targetReports.attributeOwnershipReports.front().owner == targetFederate);
  targetReports.attributeOwnershipReports.clear();

  // Unowned is a direct state transition and does not require the target to
  // retain publication.  The subsequent query therefore reports the standard
  // Attribute Is Not Owned callback.
  sendModify(false);
  REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(targetReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_NOTHROW(target->queryAttributeOwnership(objectInstance, attributeSet));
  REQUIRE(targetReports.attributeOwnershipReports.empty());
  REQUIRE_FALSE(target->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(targetReports.attributeOwnershipReports.size() == 1U);
  REQUIRE(targetReports.attributeOwnershipReports.front().kind ==
          ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned);
  targetReports.attributeOwnershipReports.clear();

  // Assignment to Owned is rejected without changing state when the target
  // is no longer publishing the corresponding class attribute. Restore the
  // declaration before exercising the successful re-assignment below.
  REQUIRE_NOTHROW(target->unpublishObjectClassAttributes(child, attributeSet));
  REQUIRE_THROWS_AS(
      owner->sendInteraction(
          modifyAttributeState,
          makeModifyValues(true),
          VariableLengthData()),
      rti1516_2025::AttributeNotPublished);
  REQUIRE_NOTHROW(target->publishObjectClassAttributes(child, attributeSet));

  // The same interaction can immediately assign the attribute again; no
  // ownership-notification callback is synthesized for either transition.
  sendModify(true);
  REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(targetReports.attributeOwnershipAcquisitionReports.empty());

  // A predefined MOM attribute is RTI-owned and must never be routed through
  // the application ownership ledger.  The private snapshot is used only to
  // obtain the internal object handle for this negative boundary test.
  auto* ownerUmbra = dynamic_cast<rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador*>(
      owner.get());
  REQUIRE(ownerUmbra != nullptr);
  auto const momSnapshot = ownerUmbra->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(momSnapshot);
  auto const momClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const momAttribute = owner->getAttributeHandle(momClass, L"HLAfederateName");
  REQUIRE(momClass.isValid());
  REQUIRE(momAttribute.isValid());
  REQUIRE_THROWS_AS(
      owner->sendInteraction(
          modifyAttributeState,
          ParameterHandleValueMap{
              {federateParameter, targetFederate.encode()},
              {objectParameter,
               rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
                   momSnapshot->objectInstanceHandle)
                   .encode()},
              {attributeParameter, momAttribute.encode()},
              {stateParameter, rti1516_2025::HLAinteger32BE{1}.encode()},
          },
          VariableLengthData()),
      rti1516_2025::RTIinternalError);

  REQUIRE_NOTHROW(target->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(target->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded queued timestamped regional interaction survives time-regulation disable and re-enable with changed lookahead",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][tso][re-enable][regulation-disable][changed-lookahead]"
    "[timestamped-regional-interaction-regulation-reenable-changed-lookahead]"
    "[rti.service.send-interaction-with-regions][rti.service.enable-time-regulation]"
    "[rti.service.disable-time-regulation][rti.service.query-lookahead]"
    "[rti.service.time-advance-request][rti.service.enable-time-constrained]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[federate.callback.time-regulation-enabled]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0xC7, 0x19};
  unsigned char const tagBytes[] = {0x52, 0x47, 0x43, 0x48};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-changed-lookahead-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-changed-lookahead-receiver", L"subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{sourceRegion},
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The accepted TSO message keeps the source region realization from send
  // time.  A later source mutation must not make the queued recipient lose
  // an interaction that already overlapped at admission.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(16UL, 20UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));

  // The accepted regional passel retains its invocation-time source-region
  // snapshot while the producer changes time-regulation role and lookahead.
  // Re-enabling with lookahead three must establish the new GALT boundary,
  // not replace or re-route the queued passel.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(3)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == 3);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"5");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded public restore rehydrates queued timestamped regional interaction and preserves filesystem service-report identity",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][ddm][filesystem][service-report-file][service-reporting]"
    "[process-restart-regional-interaction-tso-ddm]"
    "[public-process-restart-regional-interaction-tso-ddm]"
    "[tso-queue-state][tso-payload-state][tso-regional-interaction-state]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.get-range-bounds]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[federate.callback.receive-interaction][federate.callback.federation-restored]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto reportDirectory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const parameterBytes[] = {0xC7, 0x19};
  unsigned char const tagBytes[] = {0x52, 0x47, 0x43, 0x48};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  // Give the owner the public embedded-profile directory.  The receiver uses
  // the default profile directory; this keeps the assertion focused on one
  // joined federate's immutable advertised report file.
  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"public-regional-restore-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"public-regional-restore-receiver", L"subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*owner);
  suppressDeclarationRelevanceAdvisories(*receiver);

  auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
  REQUIRE(filesAtJoin.size() == 1U);
  auto const reportFile = filesAtJoin.front();
  auto const initialReportText = readTextFile(reportFile);
  REQUIRE(std::filesystem::absolute(reportFile).lexically_normal() ==
          reportFile.lexically_normal());

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = owner->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const sourceRegion = owner->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(2UL, 4UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(2UL, 4UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

  // The message is later than the timed-save boundary, so it remains in the
  // receiver's TSO queue when the save image is committed.
  auto const retraction = owner->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{sourceRegion},
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  std::wstring const saveLabel = L"public-regional-restore-queued";
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);
  auto const reportAfterSave = readTextFile(reportFile);
  REQUIRE(reportAfterSave.size() > initialReportText.size());
  REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
  REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

  // Change the live source to a disjoint range.  Restore must recover the
  // saved [2,4) region and the queued interaction's invocation snapshot,
  // rather than re-reading this post-save live range.
  REQUIRE_NOTHROW(owner->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(9UL, 11UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
  auto const mutatedBounds = owner->getRangeBounds(sourceRegion, serverId);
  REQUIRE(mutatedBounds.getLowerBound() == 9UL);
  REQUIRE(mutatedBounds.getUpperBound() == 11UL);

  REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
  REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
  REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);

  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const restoredBounds = owner->getRangeBounds(sourceRegion, serverId);
  REQUIRE(restoredBounds.getLowerBound() == 2UL);
  REQUIRE(restoredBounds.getUpperBound() == 4UL);
  auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
  REQUIRE(filesAfterRestore == std::vector<std::filesystem::path>{reportFile});
  auto const reportAfterRestore = readTextFile(reportFile);
  REQUIRE(reportAfterRestore.size() > reportAfterSave.size());
  REQUIRE(reportAfterRestore.find("RequestFederationRestore") != std::string::npos);
  REQUIRE(reportAfterRestore.find("FederateRestoreComplete") != std::string::npos);

  // Re-establish the GALT boundary and cross the restored queue entry.  The
  // restored callback carries the original source-region designator and
  // timestamp even though the live region was changed before restore.
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == ownerHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"9");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds queued timestamped regional interaction and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][ddm]"
    "[service-report-file][service-reporting][tso-queue-state][tso-payload-state]"
    "[tso-regional-interaction-state]"
    "[process-restart-regional-interaction-tso-ddm]"
    "[public-process-restart-regional-interaction-tso-ddm]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.get-range-bounds]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[federate.callback.receive-interaction][federate.callback.federation-restored]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[callback-immediate][2025]") {
  auto runScenario = [](CallbackModel const callbackModel) {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto sourceConfiguration = configurationForServiceReportDirectory(reportDirectory.path());
  sourceConfiguration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  std::wstring const saveLabel = L"public-fresh-regional-restore-queued";
  unsigned char const parameterBytes[] = {0xD1, 0x37};
  unsigned char const tagBytes[] = {0x50, 0x52, 0x46, 0x52};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  FederateHandle sourceOwnerHandle;
  RegionHandle sourceRegion;
  RegionHandle sourceReceiverARegion;
  RegionHandle sourceReceiverBRegion;
  InteractionClassHandle interactionClass;
  ParameterHandle temperatureOk;
  DimensionHandle serverId;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [&](RTIambassador& first,
                            RTIambassador& second,
                            RTIambassador& third) {
    // A callback on one route may submit the next callback to another route
    // after that route's turn in this pass (for example, the last constrained
    // grant submits the non-constrained owner's Initiate Federate Save).
    // EvokeCallback reports whether work remains *after* its one callback, so
    // a single boolean-driven pass can miss that cross-route enqueue.  Give
    // the three routes a bounded number of callback turns instead.
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
      static_cast<void>(third.evokeCallback(0.0));
    }
  };

  // The source registry is deliberately backed by the durable filesystem
  // store.  After the source ambassadors resign, the fresh registry below
  // can load the same route-free image without retaining source closures.
  {
    auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-regional-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-regional-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-regional-receiver-b",
        L"subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

    interactionClass = owner->getInteractionClassHandle(
        fixture_hla::fom::main_course_served);
    temperatureOk = owner->getParameterHandle(
        interactionClass,
        fixture_hla::fixture::temperature_ok);
    serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
    REQUIRE(interactionClass.isValid());
    REQUIRE(temperatureOk.isValid());
    REQUIRE(serverId.isValid());
    parameterValues.emplace(
        temperatureOk,
        VariableLengthData(parameterBytes, sizeof(parameterBytes)));
    REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));

    REQUIRE_NOTHROW(sourceRegion = owner->createRegion(DimensionHandleSet{serverId}));
    REQUIRE_NOTHROW(sourceReceiverARegion =
        receiverA->createRegion(DimensionHandleSet{serverId}));
    REQUIRE_NOTHROW(sourceReceiverBRegion =
        receiverB->createRegion(DimensionHandleSet{serverId}));
    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(receiverA->setRangeBounds(
        sourceReceiverARegion,
        serverId,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(receiverA->commitRegionModifications(
        RegionHandleSet{sourceReceiverARegion}));
    REQUIRE_NOTHROW(receiverB->setRangeBounds(
        sourceReceiverBRegion,
        serverId,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(receiverB->commitRegionModifications(
        RegionHandleSet{sourceReceiverBRegion}));
    REQUIRE_NOTHROW(receiverA->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{sourceReceiverARegion}));
    REQUIRE_NOTHROW(receiverB->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{sourceReceiverBRegion}));
    REQUIRE_NOTHROW(receiverA->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiverB->setConveyRegionDesignatorSetsSwitch(true));

    REQUIRE_NOTHROW(receiverA->enableTimeConstrained());
    REQUIRE_FALSE(receiverA->evokeCallback(0.0));
    REQUIRE(receiverAReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(receiverB->enableTimeConstrained());
    REQUIRE_FALSE(receiverB->evokeCallback(0.0));
    REQUIRE(receiverBReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

    auto const retraction = owner->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag,
        rti1516_2025::HLAinteger64Time(9));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverAReports.timestampedInteractionReports.empty());
    REQUIRE(receiverBReports.timestampedInteractionReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE executes an admitted grant synchronously.  Hold every
    // route while the three members cross the timed-save boundary so the
    // durable image is admitted with the same membership and queue frontier
    // as the evoked run.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(receiverA->disableCallbacks());
      REQUIRE_NOTHROW(receiverB->disableCallbacks());
    }
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
    }
    REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(receiverA->enableCallbacks());
      REQUIRE_NOTHROW(receiverB->enableCallbacks());
    }
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverAReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverBReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});

    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(receiverA->federateSaveBegun());
    REQUIRE_NOTHROW(receiverB->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(receiverA->federateSaveComplete());
    REQUIRE_NOTHROW(receiverB->federateSaveComplete());
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(receiverAReports.federationSavedReportCount == 1U);
    REQUIRE(receiverBReports.federationSavedReportCount == 1U);
    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    REQUIRE_FALSE(durable->stateImage.empty());
    auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    // The source has one publication plus two regional subscriptions; all
    // three declaration records are part of the durable image.
    REQUIRE(durableImage.interactionDeclarations.size() == 3U);
    REQUIRE(durableImage.regions.size() == 3U);
    REQUIRE(durableImage.tsoInteractionMessages.size() == 1U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 2U);

    // The source region mutation happens after the durable image is complete.
    // The fresh registry must restore [2,4) and the invocation snapshot even
    // though this source lifetime is then resigned.
    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(9UL, 11UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    auto const mutatedBounds = owner->getRangeBounds(sourceRegion, serverId);
    REQUIRE(mutatedBounds.getLowerBound() == 9UL);
    REQUIRE(mutatedBounds.getUpperBound() == 11UL);

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  // Recreate the federation through the public facade against a new registry
  // instance.  The public callback routes below are therefore fresh objects;
  // the filesystem commit is the only source of the queued payload state.
  {
    auto const freshRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();
    auto freshConfiguration = configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-regional-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-regional-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-regional-receiver-b",
        L"subscriber",
        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(), sourceReportFile) !=
            filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverAReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverBReports.initiateFederateRestoreReports.size() == 1U);

    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverA->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverB->federateRestoreComplete());
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.timestampedInteractionReports.empty());
    REQUIRE(receiverBReports.timestampedInteractionReports.empty());

    auto const restoredBounds = owner->getRangeBounds(sourceRegion, serverId);
    REQUIRE(restoredBounds.getLowerBound() == 2UL);
    REQUIRE(restoredBounds.getUpperBound() == 4UL);
    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshAfterRestoreReportText = readTextFile(freshReportFile);
    REQUIRE(freshAfterRestoreReportText.size() > freshInitialReportText.size());
    REQUIRE(freshAfterRestoreReportText.find("RequestFederationRestore") != std::string::npos);
    REQUIRE(freshAfterRestoreReportText.find("FederateRestoreComplete") != std::string::npos);

    // Cross the restored queue entry from the fresh callback routes.  The
    // saved [2,4) source snapshot must win over the resigned source's [9,11)
    // mutation, and both recipient-specific queue entries must deliver once.
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverAReports.timestampedInteractionReports.size() == 1U);
    REQUIRE(receiverBReports.timestampedInteractionReports.size() == 1U);
    for (auto const* report : {
             &receiverAReports.timestampedInteractionReports.front(),
             &receiverBReports.timestampedInteractionReports.front()}) {
      REQUIRE(report->interactionClass == interactionClass);
      REQUIRE(report->parameterValues.size() == 1U);
      REQUIRE(report->parameterValues.contains(temperatureOk));
      REQUIRE(variableLengthDataBytes(report->parameterValues.at(temperatureOk)) ==
              std::vector<unsigned char>(
                  parameterBytes,
                  parameterBytes + sizeof(parameterBytes)));
      REQUIRE(variableLengthDataBytes(report->userSuppliedTag) ==
              std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
      REQUIRE(report->producingFederate == freshOwnerHandle);
      REQUIRE(report->timeImplementationName == standard_hla::mom::integer64_time);
      REQUIRE(report->timeValue == L"9");
      REQUIRE(report->sentOrderType == TIMESTAMP);
      REQUIRE(report->receivedOrderType == TIMESTAMP);
      REQUIRE(report->sentRegionsSupplied);
      REQUIRE(report->sentRegions == RegionHandleSet{sourceRegion});
      REQUIRE(report->retractionSupplied);
      REQUIRE(report->retractionValid);
    }

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };
  runScenario(rti1516_2025::HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}


TEST_CASE(
    "Embedded timestamped default-region attribute update survives time-regulation disable and re-enable at the new lookahead",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-default-region-attribute-update][timestamped-default-region-attribute-regulation-reenable]"
    "[timestamped-default-region-attribute-regulation-reenable-changed-lookahead]"
    "[default-region][tso][re-enable][regulation-disable][changed-lookahead]"
    "[rti.service.register-object-instance]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.retract]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](std::int64_t reenabledLookahead,
                        std::int64_t producerAdvance) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x47, 0x52};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x47, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-attribute-regulation-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-attribute-regulation-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(soda));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // Producer role changes must not discard or retarget an already accepted
  // default-region passel. Re-enabling at either the same or a changed
  // lookahead establishes a new producer boundary while the recipient's
  // queued copy remains tied to this joined-federate lifetime.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(reenabledLookahead)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == reenabledLookahead);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(producerAdvance)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});

  auto const& report = receiverReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  };

  // Exercise both the unchanged-lookahead lifecycle and the changed-lookahead
  // boundary.  With lookahead three, TAR(3) is the first producer position
  // that raises GALT to the queued timestamp six.
  runScenario(5, 2);
  runScenario(3, 3);
}

TEST_CASE(
    "Embedded queued timestamped regional attribute update survives time-regulation disable and re-enable with changed lookahead",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update][explicit-source][tso][re-enable][regulation-disable][changed-lookahead]"
    "[timestamped-regional-attribute-update-regulation-reenable-changed-lookahead]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.query-lookahead][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x52, 0x47, 0x41, 0x43};
  unsigned char const tagBytes[] = {0x43, 0x48, 0x47, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-attribute-changed-lookahead-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-attribute-changed-lookahead-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The source realization is part of the accepted passel.  Mutating the
  // live region before the callback/grant boundary must not erase this
  // already-overlap-qualified reflection.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));

  // The accepted passel retains its explicit source-region snapshot while
  // the producer changes time-regulation role and lookahead. Re-enabling at
  // three must establish the new GALT boundary without replacing the queued
  // object/update or its retraction identity.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == 3);

  // With current time zero and lookahead three, the producer's advance to two
  // raises GALT to the queued timestamp five. Reflection must precede the
  // receiver's matching grant and preserve the original source region.
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});

  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.producingFederate == publisherHandle);
  REQUIRE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"5");
  REQUIRE(reflection.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded transport loss cancels a pending negotiated ownership transfer",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][connection-lost-negotiated-cancellation]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-divestiture-confirmation]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador lostReports;
  auto owner = makeRti();
  auto lost = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"automatic-negotiated-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-negotiated-lost",
      L"candidate",
      federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(lost->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);

  // The lost member is an eligible regular acquirer. The owner then starts
  // negotiated divestiture before its release callback is consumed, selecting
  // the same pending acquisition for Request Divestiture Confirmation.
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  unsigned char const acquisitionTagBytes[] = {0xE6, 0x25};
  unsigned char const divestitureTagBytes[] = {0xE7, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      efficiencyOnly,
      divestitureTag));
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(lost->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic negotiated-cancellation transport fault"));

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic negotiated-cancellation transport fault"});
  // Both owner-side callbacks were queued before the fault. Forced directive
  // three must remove their pending state before callback delivery, leaving
  // ownership with the surviving owner and no stale confirmation path.
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records Set Range Bounds arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[set-range-bounds-service-report]"
    "[rti.service.set-range-bounds]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"set-range-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const region = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(2UL, 8UL)));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const regionValue = asAscii(region.toString());
  auto const dimensionValue = asAscii(barQuantity.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      regionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      dimensionValue +
      R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":2},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":8}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(8UL, 8UL)),
      rti1516_2025::InvalidRangeBound);
  auto const invalidRangeRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      regionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      dimensionValue +
      R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":8},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":8}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRangeBound: Set Range Bounds requires 0 <= lowerBound < upperBound <= dimension upper bound."})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord + invalidRangeRecord);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records Create Region and Get Range Bounds return arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[create-region-service-report][get-range-bounds-service-report]"
    "[rti.service.create-region][rti.service.get-range-bounds]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"create-get-range-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const existingRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(
      owner->setRangeBounds(existingRegion, barQuantity, RangeBounds(2UL, 8UL)));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  auto const createdRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  auto const rangeBounds = owner->getRangeBounds(existingRegion, barQuantity);
  REQUIRE(rangeBounds.getLowerBound() == 2UL);
  REQUIRE(rangeBounds.getUpperBound() == 8UL);

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto const existingRegionValue = asAscii(existingRegion.toString());
  auto const createdRegionValue = asAscii(createdRegion.toString());
  auto const createRegionRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
      createdRegionValue +
      R"("}],"HLAservice":"CreateRegion","HLAsuppliedArguments":[{"HLAargumentType":11,"HLAargumentName":"Set of dimension designators","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  auto const getRangeBoundsRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":41,"HLAargumentName":"Range bounds","HLAargumentValue":{"lower":2,"upper":8}}],"HLAservice":"GetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + createRegionRecord + getRangeBoundsRecord);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(createdRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(existingRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records support dimension lookup return arguments",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-report-file][service-reporting][dimension-lookup-service-reports]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound][rti.service.get-dimension-handle-set]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"dimension-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const drink = owner->getObjectClassHandle(fixture_hla::fom::food_drink);
  auto const mainCourseServed = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const region = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(owner->getAvailableDimensionsForObjectClass(drink) ==
          DimensionHandleSet{barQuantity});
  REQUIRE(owner->getAvailableDimensionsForInteractionClass(mainCourseServed) ==
          DimensionHandleSet{serverId});
  REQUIRE(owner->getDimensionHandle(fixture_hla::fixture::bar_quantity) == barQuantity);
  REQUIRE(owner->getDimensionName(barQuantity) == fixture_hla::fixture::bar_quantity);
  REQUIRE(owner->getDimensionUpperBound(barQuantity) == 25UL);
  REQUIRE(owner->getDimensionHandleSet(region) == DimensionHandleSet{barQuantity});

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto const serverIdValue = asAscii(serverId.toString());
  auto const drinkValue = asAscii(drink.toString());
  auto const mainCourseServedValue = asAscii(mainCourseServed.toString());
  auto const regionValue = asAscii(region.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAservice":"GetAvailableDimensionsForObjectClass","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      drinkValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
      serverIdValue +
      R"("]}],"HLAservice":"GetAvailableDimensionsForInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      mainCourseServedValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAservice":"GetDimensionHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}],"HLAservice":"GetDimensionName","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Dimension upper bound","HLAargumentValue":25}],"HLAservice":"GetDimensionUpperBound","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimensions","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAservice":"GetDimensionHandleSet","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      regionValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records failed dimension and region lookups",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[rti.service.dimension-lookup-failure-matrix]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"failed-dimension-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(ObjectClassHandle{}),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(fixture_hla::fixture::missing_dimension),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(RegionHandle{}),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(ObjectClassHandle{}),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(fixture_hla::fixture::missing_dimension),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(RegionHandle{}),
      rti1516_2025::InvalidRegion);
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const invalidObjectClassValue = asAscii(ObjectClassHandle{}.toString());
  auto const invalidInteractionClassValue = asAscii(InteractionClassHandle{}.toString());
  auto const invalidDimensionValue = asAscii(DimensionHandle{}.toString());
  auto const invalidRegionValue = asAscii(RegionHandle{}.toString());
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto expectedRecords = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"GetAvailableDimensionsForObjectClass","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      invalidObjectClassValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidObjectClassHandle: Get Available Dimensions for Object Class requires a valid ObjectClassHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"GetAvailableDimensionsForInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      invalidInteractionClassValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidInteractionClassHandle: Get Available Dimensions for Interaction Class requires a valid InteractionClassHandle."})"};
  expectedRecords +=
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"MissingDimension"}],"HLAsuccessIndicator":false,"HLAexception":"NameNotFound: The supplied dimension name is not defined in this federation execution."})";
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionName","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      invalidDimensionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidDimensionHandle: Get Dimension Name requires a valid DimensionHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionUpperBound","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      invalidDimensionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidDimensionHandle: Get Dimension Upper Bound requires a valid DimensionHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionHandleSet","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      invalidRegionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRegion: Get Dimension Handle Set requires a valid RegionHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":6,"HLAreturnedArgument":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAservice":"GetDimensionHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records failed Create Region and Get Range Bounds invocations",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[ddm-nonvoid-failure][rti.service.ddm-nonvoid-failure-matrix]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"failed-ddm-nonvoid-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const existingRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(
      owner->setRangeBounds(existingRegion, barQuantity, RangeBounds(2UL, 8UL)));
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{DimensionHandle{}}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(existingRegion, DimensionHandle{}),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{DimensionHandle{}}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(existingRegion, DimensionHandle{}),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  auto const createdRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  auto const rangeBounds = owner->getRangeBounds(existingRegion, barQuantity);
  REQUIRE(rangeBounds.getLowerBound() == 2UL);
  REQUIRE(rangeBounds.getUpperBound() == 8UL);

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const invalidDimensionValue = asAscii(DimensionHandle{}.toString());
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto const existingRegionValue = asAscii(existingRegion.toString());
  auto const createdRegionValue = asAscii(createdRegion.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CreateRegion","HLAsuppliedArguments":[{"HLAargumentType":11,"HLAargumentName":"Set of dimension designators","HLAargumentValue":[")" +
      invalidDimensionValue +
      R"("]}],"HLAsuccessIndicator":false,"HLAexception":"InvalidDimensionHandle: Create Region requires valid DimensionHandle values."})"} +
      std::string{
          R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"GetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      invalidDimensionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"RegionDoesNotContainSpecifiedDimension: Get Range Bounds requires a dimension contained by the region."})"} +
      std::string{
          R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
      createdRegionValue +
      R"("}],"HLAservice":"CreateRegion","HLAsuppliedArguments":[{"HLAargumentType":11,"HLAargumentName":"Set of dimension designators","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"} +
      std::string{
          R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":41,"HLAargumentName":"Range bounds","HLAargumentValue":{"lower":2,"upper":8}}],"HLAservice":"GetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(createdRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(existingRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}





TEST_CASE(
    "Embedded timed federation save waits for constrained TSO delivery and replaces pending requests",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-save-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timed-save-receiver", L"subscriber", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = owner->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0xA5, 0x25};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  std::size_t receiverTsoReportsAtSaveInitiate = 0;
  receiverReports.onInitiateFederateSave = [&receiverReports, &receiverTsoReportsAtSaveInitiate] {
    receiverTsoReportsAtSaveInitiate = receiverReports.timestampedInteractionReports.size();
  };

  REQUIRE_THROWS_AS(
      owner->requestFederationSave(L"timed-save-too-early", rti1516_2025::HLAinteger64Time(0)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_THROWS_AS(
      receiver->requestFederationSave(L"timed-save-at-galt", rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  // The second request replaces the first while neither has reached the
  // Initiate Federate Save boundary.
  REQUIRE_NOTHROW(owner->requestFederationSave(
      L"timed-save-replaced",
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->requestFederationSave(
      L"timed-save-final",
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(receiverReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());
  REQUIRE(receiverReports.callbackOrder.empty());

  // A timestamped interaction at the save boundary must be received before
  // the constrained federate can be instructed to save.
  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  // The receiver first receives the TSO payload at the scheduled-save
  // boundary, then receives Initiate Federate Save while still Time
  // Advancing, and only then receives its grant.
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"timed-save-final"});
  REQUIRE(receiverReports.timestampedSaveInitiationReports.size() == 1);
  REQUIRE(receiverReports.timestampedSaveInitiationReports.back().label == L"timed-save-final");
  REQUIRE(receiverReports.timestampedSaveInitiationReports.back().timeImplementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(receiverReports.timestampedSaveInitiationReports.back().timeValue == L"7");
  REQUIRE(receiverTsoReportsAtSaveInitiate == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  // The non-time-constrained regulator is queued only after the constrained
  // recipient has been admitted at its own pre-grant boundary.
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"timed-save-final"});
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  static_cast<void>(owner->evokeCallback(0.0));
  static_cast<void>(receiver->evokeCallback(0.0));
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded float64 timed federation save preserves provider time scheduling",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[float-time][timed-save]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "time-representation-float64-fom.xml")
                             .wstring();
  rti1516_2025::HLAfloat64Time const saveTime(5.0);
  auto const saveLabel = std::wstring(L"float64-timed-save-boundary");
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::float64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"float64-timed-save-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"float64-timed-save-receiver", L"subscriber", federationName));

  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAfloat64Interval(1.0)));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*owner);
  drain(*receiver);
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(ownerReports.timeRegulationEnabledReports.front().implementationName ==
          standard_hla::mom::float64_time);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.front().implementationName ==
          standard_hla::mom::float64_time);

  REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel, saveTime));
  REQUIRE(ownerReports.timestampedSaveInitiationReports.empty());
  REQUIRE(receiverReports.timestampedSaveInitiationReports.empty());

  // The regulator reaches the requested boundary first, but the timed save
  // remains held until the constrained Java/C++ member reaches the same
  // floating boundary.  This is the native scheduling authority for the
  // external IEEE-JAR float64 save vectors.
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(saveTime));
  drain(*owner);
  REQUIRE(ownerReports.timestampedSaveInitiationReports.empty());
  REQUIRE(receiverReports.timestampedSaveInitiationReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(saveTime));
  drain(*receiver);
  drain(*owner);
  REQUIRE(ownerReports.timestampedSaveInitiationReports.size() == 1U);
  REQUIRE(receiverReports.timestampedSaveInitiationReports.size() == 1U);
  for (auto const* reports : {&ownerReports, &receiverReports}) {
    REQUIRE(reports->timestampedSaveInitiationReports.front().label == saveLabel);
    REQUIRE(
        reports->timestampedSaveInitiationReports.front().timeImplementationName ==
        standard_hla::mom::float64_time);
    REQUIRE(
        reports->timestampedSaveInitiationReports.front().timeValue ==
        saveTime.toString());
  }

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  drain(*owner);
  drain(*receiver);
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::float64_time);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::float64_time);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}


TEST_CASE(
    "Embedded federation resignation fails an outstanding save for remaining participants",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.resign-federation-execution][federate.callback.federation-not-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador resigningReports;
  auto owner = makeRti();
  auto resigning = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(resigning->connect(resigningReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"save-owner-resign", L"owner", federationName));
  REQUIRE_NOTHROW(
      resigning->joinFederationExecution(L"save-resigning", L"observer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationSave(L"resignation-save"));
  REQUIRE_NOTHROW(resigning->resignFederationExecution(NO_ACTION));
  REQUIRE(ownerReports.federationNotSavedReasons.size() == 1);
  REQUIRE(ownerReports.federationNotSavedReasons.back() ==
          rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE);
  REQUIRE_THROWS_AS(owner->federateSaveBegun(), rti1516_2025::SaveNotInitiated);

  // The failed operation is cleared, so the remaining member can start a
  // subsequent control-plane save rather than being left permanently stuck.
  REQUIRE_NOTHROW(owner->requestFederationSave(L"post-resignation-save"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(resigning->disconnect());
}

TEST_CASE(
    "Embedded federation restore rolls back a saved object-management snapshot",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.federate-restore-not-complete][rti.service.abort-federation-restore]"
    "[rti.service.query-federation-restore-status]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]"
    "[federate.callback.federation-restore-status-response]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"restore-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"restore-peer", L"peer", federationName));

  // Establish a completed, restorable image before adding any object state.
  REQUIRE_NOTHROW(owner->requestFederationSave(L"object-baseline"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(peerReports.federationSavedReportCount == 1);

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle mutatedObject;
  REQUIRE_NOTHROW(mutatedObject = owner->registerObjectInstance(server));
  REQUIRE(mutatedObject.isValid());

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"object-baseline"));
  REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{L"object-baseline"});
  REQUIRE(ownerReports.federationRestoreBegunReportCount == 1);
  REQUIRE(peerReports.federationRestoreBegunReportCount == 1);
  REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1);
  REQUIRE(peerReports.initiateFederateRestoreReports.size() == 1);
  REQUIRE(ownerReports.initiateFederateRestoreReports.front().federateName == L"restore-owner");
  REQUIRE(peerReports.initiateFederateRestoreReports.front().federateName == L"restore-peer");
  REQUIRE(ownerReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
          ownerHandle);
  REQUIRE(peerReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
          peerHandle);

  REQUIRE_NOTHROW(owner->queryFederationRestoreStatus());
  REQUIRE(ownerReports.federationRestoreStatusReports.size() == 1);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses[0].status ==
          rti1516_2025::FEDERATE_RESTORING);

  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE(ownerReports.federationRestoredReportCount == 0);
  REQUIRE(peerReports.federationRestoredReportCount == 0);
  REQUIRE_NOTHROW(peer->federateRestoreComplete());
  REQUIRE(ownerReports.federationRestoredReportCount == 1);
  REQUIRE(peerReports.federationRestoredReportCount == 1);
  REQUIRE(ownerReports.federationNotRestoredReasons.empty());
  REQUIRE(peerReports.federationNotRestoredReasons.empty());

  // The public ambassador still owns the same joined time state and callback
  // route, but the saved federation image no longer contains the post-save
  // object or its declarations.
  REQUIRE_THROWS_AS(
      owner->getKnownObjectClassHandle(mutatedObject),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(owner->queryFederationRestoreStatus());
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses[0].status ==
          rti1516_2025::NO_RESTORE_IN_PROGRESS);
  REQUIRE_FALSE(ownerReports.federationRestoreStatusReports.back().statuses[0]
                   .postRestoreHandle
                   .isValid());

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded federation restore reports missing labels and participant failures",
    "[integration][development-profile][federation-management][save-restore]"
    "[federate.callback.request-federation-restore-failed]"
    "[federate.callback.federation-not-restored]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"restore-failure-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"restore-failure-peer", L"peer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"missing-label"));
  REQUIRE(ownerReports.requestFederationRestoreFailedReports ==
          std::vector<std::wstring>{L"missing-label"});

  REQUIRE_NOTHROW(owner->requestFederationSave(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->federateRestoreNotComplete());
  REQUIRE(ownerReports.federationNotRestoredReasons.size() == 1);
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE);
  REQUIRE(peerReports.federationNotRestoredReasons.size() == 1);
  REQUIRE(peerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE);

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->abortFederationRestore());
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::RESTORE_ABORTED);
  REQUIRE(peerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::RESTORE_ABORTED);

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_RESIGNED_DURING_RESTORE);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves synchronization-point state from the saved image",
    "[integration][development-profile][federation-management][save-restore][synchronization]"
    "[synchronization-state][region-state]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.get-dimension-handle-set]"
    "[rti.service.get-range-bounds][rti.service.delete-region]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[rti.service.request-federation-save][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-synchronized][federate.callback.federation-restored]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const tagBytes[] = {0x53, 0x41, 0x56, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"synchronization-restore", L"sync", federationName));

  // Save while the point is announced but not yet achieved.  The saved image
  // must retain that point even if the live execution completes it afterward.
  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(L"restore-sync", tag));
  REQUIRE(reports.synchronizationPointAnnouncementReports.size() == 1U);
  REQUIRE(reports.synchronizationPointAnnouncementReports.front().label == L"restore-sync");

  auto const serverId = rti->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(serverId.isValid());
  auto const savedRegion = rti->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(rti->setRangeBounds(savedRegion, serverId, RangeBounds(2UL, 5UL)));
  REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{savedRegion}));
  REQUIRE_NOTHROW(rti->requestFederationSave(L"synchronization-baseline"));
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());

  // Complete and remove the live point so the restore has to reconstitute it
  // from the saved federation image rather than observing the current state.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(L"restore-sync"));
  REQUIRE(reports.federationSynchronizedReports.size() == 1U);
  REQUIRE_NOTHROW(rti->deleteRegion(savedRegion));

  REQUIRE_NOTHROW(rti->requestFederationRestore(L"synchronization-baseline"));
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  REQUIRE(reports.federationRestoredReportCount == 1U);

  // A second achievement is legal only if restore brought back the
  // announced, unachieved synchronization point from the saved image.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(L"restore-sync"));
  REQUIRE(reports.federationSynchronizedReports.size() == 2U);
  REQUIRE(rti->getDimensionHandleSet(savedRegion) == DimensionHandleSet{serverId});
  auto const restoredBounds = rti->getRangeBounds(savedRegion, serverId);
  REQUIRE(restoredBounds.getLowerBound() == 2UL);
  REQUIRE(restoredBounds.getUpperBound() == 5UL);
  REQUIRE_NOTHROW(rti->deleteRegion(savedRegion));

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded evoked federation restore drops queued callbacks for a resigning participant",
    "[integration][development-profile][federation-management][save-restore][callbacks]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-not-restored]") {
  ReportingFederateAmbassador survivorReports;
  ReportingFederateAmbassador resigningReports;
  auto survivor = makeRti();
  auto resigning = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(survivor->connect(survivorReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(resigning->connect(resigningReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(
      survivor->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      survivor->joinFederationExecution(L"restore-silence-survivor", L"survivor", federationName));
  REQUIRE_NOTHROW(
      resigning->joinFederationExecution(L"restore-silence-resigning", L"resigning", federationName));

  // Establish a completed image.  Drain the evoked save callbacks so the
  // restore case below starts with an empty callback queue on both members.
  REQUIRE_NOTHROW(survivor->requestFederationSave(L"restore-silence-baseline"));
  while (survivor->evokeCallback(0.0)) {
  }
  while (resigning->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(survivor->federateSaveBegun());
  REQUIRE_NOTHROW(resigning->federateSaveBegun());
  REQUIRE_NOTHROW(survivor->federateSaveComplete());
  REQUIRE_NOTHROW(resigning->federateSaveComplete());
  while (survivor->evokeCallback(0.0)) {
  }
  while (resigning->evokeCallback(0.0)) {
  }
  REQUIRE(survivorReports.federationSavedReportCount == 1U);
  REQUIRE(resigningReports.federationSavedReportCount == 1U);

  survivorReports.callbackOrder.clear();
  resigningReports.callbackOrder.clear();
  REQUIRE_NOTHROW(survivor->requestFederationRestore(L"restore-silence-baseline"));
  REQUIRE(survivorReports.callbackOrder.empty());
  REQUIRE(resigningReports.callbackOrder.empty());

  // The resigning member has restore-request-succeeded, restore-begun, and
  // initiate-federate-restore callbacks queued but not yet evoked.  Resign
  // must invalidate that session's queue; only surviving members receive the
  // Federation Restored failure caused by the resignation.
  REQUIRE_NOTHROW(resigning->resignFederationExecution(NO_ACTION));
  while (survivor->evokeCallback(0.0)) {
  }
  REQUIRE(survivorReports.callbackOrder ==
          std::vector<std::string>{"restore-request-succeeded",
                                   "restore-begun",
                                   "restore-initiate",
                                   "restore-failed"});
  REQUIRE(survivorReports.federationNotRestoredReasons ==
          std::vector<RestoreFailureReason>{
              rti1516_2025::FEDERATE_RESIGNED_DURING_RESTORE});
  // Resignation may leave a private no-op cleanup task (for example, a MOM
  // conditional update that rechecks membership at callback time).  Drain
  // that task, but require that no public restore callback reaches the
  // departed participant.
  while (resigning->evokeCallback(0.0)) {
  }
  REQUIRE(resigningReports.callbackOrder.empty());
  REQUIRE(resigningReports.requestFederationRestoreSucceededReports.empty());
  REQUIRE(resigningReports.federationRestoreBegunReportCount == 0U);
  REQUIRE(resigningReports.initiateFederateRestoreReports.empty());
  REQUIRE(resigningReports.federationNotRestoredReasons.empty());

  REQUIRE_NOTHROW(survivor->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(survivor->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(survivor->disconnect());
  REQUIRE_NOTHROW(resigning->disconnect());
}

TEST_CASE(
    "Embedded evoked federation restore reconstitutes a saved synchronization point",
    "[integration][development-profile][federation-management][save-restore][synchronization][callback-evoked]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.synchronization-point-registration-succeeded]"
    "[federate.callback.announce-synchronization-point]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]"
    "[federate.callback.federation-synchronized]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const tagBytes[] = {0x45, 0x56, 0x4F, 0x4B};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const synchronizationLabel = L"evoked-restore-sync";
  std::wstring const saveLabel = L"evoked-restore-sync-baseline";
  auto const drain = [&] {
    while (rti->evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"evoked-restore-sync-federate", L"sync", federationName));

  // The point is announced before the save.  The save image must retain this
  // unachieved state even though the live execution completes the point later.
  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(
      synchronizationLabel, tag));
  REQUIRE(reports.synchronizationPointRegistrationReports.empty());
  REQUIRE(reports.synchronizationPointAnnouncementReports.empty());
  drain();
  REQUIRE(reports.synchronizationPointRegistrationReports.size() == 1U);
  REQUIRE(reports.synchronizationPointRegistrationReports.front().label ==
          synchronizationLabel);
  REQUIRE(reports.synchronizationPointRegistrationReports.front().succeeded);
  REQUIRE(reports.synchronizationPointAnnouncementReports.size() == 1U);
  REQUIRE(reports.synchronizationPointAnnouncementReports.front().label ==
          synchronizationLabel);
  REQUIRE(variableLengthDataBytes(
              reports.synchronizationPointAnnouncementReports.front()
                  .userSuppliedTag) ==
          std::vector<unsigned char>{tagBytes, tagBytes + sizeof(tagBytes)});

  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  REQUIRE(reports.initiateFederateSaveReports.empty());
  drain();
  REQUIRE(reports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  REQUIRE(reports.federationSavedReportCount == 0U);
  drain();
  REQUIRE(reports.federationSavedReportCount == 1U);

  // Complete the live point after the save.  Restore must replace this live
  // completion with the announced-but-unachieved state from the image.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(synchronizationLabel));
  REQUIRE(reports.federationSynchronizedReports.empty());
  drain();
  REQUIRE(reports.federationSynchronizedReports.size() == 1U);

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  REQUIRE(reports.requestFederationRestoreSucceededReports.empty());
  drain();
  REQUIRE(reports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(reports.federationRestoreBegunReportCount == 1U);
  REQUIRE(reports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE(reports.initiateFederateRestoreReports.front().federateName ==
          L"evoked-restore-sync-federate");
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  REQUIRE(reports.federationRestoredReportCount == 0U);
  drain();
  REQUIRE(reports.federationRestoredReportCount == 1U);

  // A second achievement is legal only if restore reconstituted the saved
  // announced, unachieved synchronization point.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(synchronizationLabel));
  drain();
  REQUIRE(reports.federationSynchronizedReports.size() == 2U);
  REQUIRE(reports.federationSynchronizedReports.back().label ==
          synchronizationLabel);
  REQUIRE(reports.federationSynchronizedReports.back().failedToSyncSet.empty());
  REQUIRE(reports.synchronizationPointAnnouncementReports.size() == 1U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded getTimeFactory returns the joined federation's selected time factory",
    "[integration][development-profile][federation-management][rti.service.get-time-factory]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"time-client", L"observer", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == standard_hla::mom::integer64_time);
  auto initial = factory->makeInitial();
  auto* integerInitial = dynamic_cast<rti1516_2025::HLAinteger64Time*>(initial.get());
  REQUIRE(integerInitial != nullptr);
  REQUIRE(integerInitial->isInitial());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federate lookup services preserve departed designator identities within the joined federation",
    "[integration][development-profile][federation-management][rti.service.get-federate-handle]"
    "[rti.service.get-federate-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador peerFederate;
  TestFederateAmbassador foreignCreatorFederate;
  TestFederateAmbassador foreignMemberFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto foreignCreator = makeRti();
  auto foreignMember = makeRti();
  auto const federationName = nextFederationName();
  auto const foreignFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  FederateHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getFederateName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getFederateName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignCreator->connect(foreignCreatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignMember->connect(foreignMemberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(foreignCreator->createFederationExecution(
      foreignFederationName,
      fomModule,
      standard_hla::mom::integer64_time));

  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  FederateHandle foreignHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"lookup-peer", L"observer", federationName));
  REQUIRE_NOTHROW(foreignHandle = foreignMember->joinFederationExecution(
      L"lookup-foreign",
      L"observer",
      foreignFederationName));

  REQUIRE(owner->getFederateHandle(L"lookup-owner") == ownerHandle);
  REQUIRE(owner->getFederateHandle(L"lookup-peer") == peerHandle);
  REQUIRE(peer->getFederateName(ownerHandle) == L"lookup-owner");
  REQUIRE(peer->getFederateName(peerHandle) == L"lookup-peer");
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"missing"), rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(owner->getFederateName(invalid), rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->getFederateName(foreignHandle),
      rti1516_2025::FederateHandleNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"lookup-peer"), rti1516_2025::NameNotFound);
  // Get Federate Handle applies only to joined names.  The handle returned by
  // Join remains a valid federate designator after resignation, however, so
  // Get Federate Name must retain its immutable identity for the execution.
  REQUIRE(owner->getFederateName(peerHandle) == L"lookup-peer");

  REQUIRE_NOTHROW(foreignMember->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(foreignCreator->destroyFederationExecution(foreignFederationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(foreignCreator->disconnect());
  REQUIRE_NOTHROW(foreignMember->disconnect());
}

TEST_CASE(
    "Embedded object-class lookup services use stable handles from the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-object-class-handle]"
    "[rti.service.get-object-class-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "reference-data-class-provider-fom.xml";
  ObjectClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(fixture_hla::fom::employee),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getObjectClassName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(fixture_hla::fom::employee),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(fixture_hla::fom::employee);
  REQUIRE(employee.isValid());
  REQUIRE(owner->getObjectClassHandle(fixture_hla::fom::employee) == employee);
  REQUIRE(owner->getObjectClassName(employee) == fixture_hla::fom::employee);
  REQUIRE_THROWS_AS(
      owner->getObjectClassHandle(fixture_hla::fom::missing_object),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getObjectClassName(invalid),
      rti1516_2025::InvalidObjectClassHandle);

  // A compatible additional-FOM join extends the catalog. The previously
  // returned class handle must remain stable while the new class becomes
  // discoverable from the same joined federation.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"lookup-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getObjectClassHandle(fixture_hla::fom::employee) == employee);
  auto const extension = owner->getObjectClassHandle(fixture_hla::fom::reference_fixture_class);
  REQUIRE(extension.isValid());
  REQUIRE(extension != employee);
  REQUIRE(owner->getObjectClassName(extension) == fixture_hla::fom::reference_fixture_class);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded attribute lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-attribute-handle]"
    "[rti.service.get-attribute-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "reference-data-attribute-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, fixture_hla::fixture::name),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, fixture_hla::fixture::name),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"attribute-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const customer = owner->getObjectClassHandle(fixture_hla::fom::customer);
  auto const employeeName = owner->getAttributeHandle(employee, fixture_hla::fixture::name);
  REQUIRE(employeeName.isValid());
  REQUIRE(owner->getAttributeHandle(employee, fixture_hla::fixture::name) == employeeName);
  REQUIRE(owner->getAttributeHandle(server, fixture_hla::fixture::name) == employeeName);
  REQUIRE(owner->getAttributeName(employee, employeeName) == fixture_hla::fixture::name);
  REQUIRE(owner->getAttributeName(server, employeeName) == fixture_hla::fixture::name);
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(server, fixture_hla::fixture::missing),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(invalidObjectClass, fixture_hla::fixture::name),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(invalidObjectClass, employeeName),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(employee, invalidAttribute),
      rti1516_2025::InvalidAttributeHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(customer, employeeName),
      rti1516_2025::AttributeNotDefined);

  // A compatible additional-FOM join adds a new inheritance chain. The
  // original Employee::Name handle stays stable, while the extension's
  // defining attribute has the same handle through its child class.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"attribute-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getAttributeHandle(employee, fixture_hla::fixture::name) == employeeName);
  auto const extensionBase = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_base);
  auto const extensionChild = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_class);
  auto const identifier = owner->getAttributeHandle(extensionBase, fixture_hla::fixture::identifier);
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getAttributeHandle(extensionChild, fixture_hla::fixture::identifier) == identifier);
  REQUIRE(owner->getAttributeName(extensionChild, identifier) == fixture_hla::fixture::identifier);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded interaction-class lookup services use stable handles from the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-interaction-class-handle]"
    "[rti.service.get-interaction-class-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "directed-interaction-interaction-provider-fom.xml";
  InteractionClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassHandle(fixture_hla::fom::server_take_order),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassName(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassHandle(fixture_hla::fom::server_take_order),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));

  auto const takeOrder = owner->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(takeOrder.isValid());
  REQUIRE(
      owner->getInteractionClassHandle(fixture_hla::fom::server_take_order) == takeOrder);
  REQUIRE(owner->getInteractionClassName(takeOrder) == fixture_hla::fom::server_take_order);
  REQUIRE_THROWS_AS(
      owner->getInteractionClassHandle(fixture_hla::fom::missing_interaction_base),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getInteractionClassName(invalid),
      rti1516_2025::InvalidInteractionClassHandle);

  // A compatible additional-FOM join extends the catalog. The previously
  // returned interaction handle must remain stable while the new interaction
  // becomes discoverable from the same joined federation.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"lookup-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(
      owner->getInteractionClassHandle(fixture_hla::fom::server_take_order) == takeOrder);
  auto const extension = owner->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(extension.isValid());
  REQUIRE(extension != takeOrder);
  REQUIRE(
      owner->getInteractionClassName(extension) ==
      fixture_hla::fom::directed_fixture_interaction);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded parameter lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-parameter-handle]"
    "[rti.service.get-parameter-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "parameter-handle-provider-fom.xml";
  InteractionClassHandle invalidInteractionClass;
  ParameterHandle invalidParameter;

  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, fixture_hla::fixture::temperature_ok),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, fixture_hla::fixture::temperature_ok),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"parameter-owner", L"owner", federationName));

  auto const mainCourseServed = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const takeOrder = owner->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const temperatureOk = owner->getParameterHandle(mainCourseServed, fixture_hla::fixture::temperature_ok);
  REQUIRE(temperatureOk.isValid());
  REQUIRE(owner->getParameterHandle(mainCourseServed, fixture_hla::fixture::temperature_ok) == temperatureOk);
  REQUIRE(owner->getParameterName(mainCourseServed, temperatureOk) == fixture_hla::fixture::temperature_ok);
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(mainCourseServed, fixture_hla::fixture::missing),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(invalidInteractionClass, fixture_hla::fixture::temperature_ok),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(invalidInteractionClass, temperatureOk),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(mainCourseServed, invalidParameter),
      rti1516_2025::InvalidParameterHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(takeOrder, temperatureOk),
      rti1516_2025::InteractionParameterNotDefined);

  // A compatible additional-FOM join adds a new interaction inheritance
  // chain. The original Restaurant parameter handle remains stable, while the
  // extension's defining parameter has the same handle through its child.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"parameter-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getParameterHandle(mainCourseServed, fixture_hla::fixture::temperature_ok) == temperatureOk);
  auto const extensionBase = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_base);
  auto const extensionChild = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = owner->getParameterHandle(extensionBase, fixture_hla::fixture::identifier);
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getParameterHandle(extensionChild, fixture_hla::fixture::identifier) == identifier);
  REQUIRE(owner->getParameterName(extensionChild, identifier) == fixture_hla::fixture::identifier);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded 2025 dimension lookup follows FOM hierarchy and upper bounds",
    "[integration][development-profile][federation-management]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const dimensionConsumerFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-consumer-fom.xml";
  auto const dimensionProviderFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  InteractionClassHandle invalidInteractionClass;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandle(fixture_hla::fixture::soda_flavor),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionName(invalidDimension),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"dimension-owner", L"owner", federationName));

  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  auto const sweetener = owner->getDimensionHandle(fixture_hla::fixture::sweetener);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(sweetener.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(owner->getDimensionHandle(fixture_hla::fixture::soda_flavor) == sodaFlavor);
  REQUIRE(owner->getDimensionName(sodaFlavor) == fixture_hla::fixture::soda_flavor);
  REQUIRE(owner->getDimensionUpperBound(barQuantity) == 25UL);
  REQUIRE(owner->getDimensionUpperBound(sodaFlavor) == 4UL);
  REQUIRE(owner->getDimensionUpperBound(sweetener) == 3UL);
  REQUIRE(owner->getDimensionUpperBound(serverId) == 20UL);

  auto const drink = owner->getObjectClassHandle(fixture_hla::fom::food_drink);
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const light = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda_light);
  DimensionHandleSet drinkDimensions = owner->getAvailableDimensionsForObjectClass(drink);
  DimensionHandleSet sodaDimensions = owner->getAvailableDimensionsForObjectClass(soda);
  DimensionHandleSet lightDimensions = owner->getAvailableDimensionsForObjectClass(light);
  REQUIRE(drinkDimensions == DimensionHandleSet{barQuantity});
  REQUIRE(sodaDimensions == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(lightDimensions == DimensionHandleSet{barQuantity, sodaFlavor, sweetener});

  auto const foodServed = owner->getInteractionClassHandle(
      fixture_hla::fom::food_served);
  auto const mainCourseServed = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  REQUIRE(owner->getAvailableDimensionsForInteractionClass(foodServed).empty());
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(mainCourseServed) ==
      DimensionHandleSet{serverId});

  // A later 2025 FOM join contributes a class, interaction, and dimension
  // from separate modules. Existing Restaurant handles stay stable while the
  // newly composed dimension is immediately available to lookup services.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"dimension-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{dimensionConsumerFom.wstring(), dimensionProviderFom.wstring()}));
  REQUIRE(owner->getDimensionHandle(fixture_hla::fixture::soda_flavor) == sodaFlavor);
  auto const fixtureDimension = owner->getDimensionHandle(fixture_hla::fixture::umbra_dimension_fixture);
  REQUIRE(fixtureDimension.isValid());
  REQUIRE(owner->getDimensionUpperBound(fixtureDimension) == 100UL);
  auto const fixtureObjectClass = owner->getObjectClassHandle(
      fixture_hla::fom::dimension_fixture_object);
  REQUIRE(
      owner->getAvailableDimensionsForObjectClass(fixtureObjectClass) ==
      DimensionHandleSet{fixtureDimension});
  auto const fixtureInteractionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::dimension_fixture_interaction);
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(fixtureInteractionClass) ==
      DimensionHandleSet{fixtureDimension});

  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(fixture_hla::fixture::missing_dimension),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::InvalidInteractionClassHandle);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded public handle decoders enforce lifecycle and preserve encoded identities",
    "[integration][development-profile][federation-management][support-services][handles]"
    "[rti.service.decode-federate-handle][rti.service.decode-object-class-handle]"
    "[rti.service.decode-interaction-class-handle][rti.service.decode-object-instance-handle]"
    "[rti.service.decode-attribute-handle][rti.service.decode-parameter-handle]"
    "[rti.service.decode-dimension-handle][rti.service.decode-message-retraction-handle]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const encodedRetractionBytes[] = {
      0x00, 0x00, 0x00, 0x08, 0x01, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  };
  VariableLengthData const encodedRetraction(
      encodedRetractionBytes, sizeof(encodedRetractionBytes));

  REQUIRE_THROWS_AS(
      unjoined->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle federate;
  REQUIRE_NOTHROW(federate = owner->joinFederationExecution(
      L"handle-decoder-owner", L"owner", federationName));

  auto const objectClass = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const attribute = owner->getAttributeHandle(objectClass, fixture_hla::fixture::flavor);
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = owner->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const dimension = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(federate.isValid());
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass, AttributeHandleSet{attribute}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());

  REQUIRE(owner->decodeFederateHandle(federate.encode()) == federate);
  REQUIRE(owner->decodeObjectClassHandle(objectClass.encode()) == objectClass);
  REQUIRE(owner->decodeInteractionClassHandle(interactionClass.encode()) == interactionClass);
  REQUIRE(owner->decodeObjectInstanceHandle(objectInstance.encode()) == objectInstance);
  REQUIRE(owner->decodeAttributeHandle(attribute.encode()) == attribute);
  REQUIRE(owner->decodeParameterHandle(parameter.encode()) == parameter);
  REQUIRE(owner->decodeDimensionHandle(dimension.encode()) == dimension);
  auto const decodedRetraction = owner->decodeMessageRetractionHandle(encodedRetraction);
  REQUIRE(decodedRetraction.isValid());
  REQUIRE(decodedRetraction.toString() == L"MessageRetractionHandle(72623859790382856)");

  REQUIRE_THROWS_AS(
      owner->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded 2025 region templates preserve pending and committed range state",
    "[integration][development-profile][federation-management][ddm][region-lifecycle]"
    "[rti.service.create-region][rti.service.commit-region-modifications]"
    "[rti.service.delete-region][rti.service.get-dimension-handle-set]"
    "[rti.service.get-range-bounds][rti.service.set-range-bounds]"
    "[rti.service.decode-region-handle]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador foreignFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto foreign = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  RegionHandle invalidRegion;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandleSet(invalidRegion),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreign->connect(foreignFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"region-owner", L"owner", federationName));
  REQUIRE_NOTHROW(foreign->joinFederationExecution(L"region-foreign", L"foreign", federationName));

  // The official 2025 C++ createRegion surface accepts an empty set of
  // specified dimensions.  This is a valid zero-dimensional template and
  // specification; the sibling Python backend's InvalidRegionContext branch
  // is intentionally not copied because that exception is not declared by
  // the C++ API.
  auto const emptyRegion = owner->createRegion(DimensionHandleSet{});
  REQUIRE(emptyRegion.isValid());
  REQUIRE(owner->getDimensionHandleSet(emptyRegion).empty());
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{emptyRegion}));
  REQUIRE(owner->getDimensionHandleSet(emptyRegion).empty());
  REQUIRE_NOTHROW(owner->deleteRegion(emptyRegion));

  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());

  auto const region = owner->createRegion(DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(region.isValid());
  REQUIRE(owner->getDimensionHandleSet(region) == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(owner->decodeRegionHandle(region.encode()) == region);
  REQUIRE_THROWS_AS(
      owner->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, invalidDimension, RangeBounds(0UL, 10UL)),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(10UL, 10UL)),
      rti1516_2025::InvalidRangeBound);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 26UL)),
      rti1516_2025::InvalidRangeBound);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  auto const pendingBar = owner->getRangeBounds(region, barQuantity);
  REQUIRE(pendingBar.getLowerBound() == 0UL);
  REQUIRE(pendingBar.getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE(owner->getRangeBounds(region, barQuantity).getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, sodaFlavor),
      rti1516_2025::InvalidRegion);

  REQUIRE_THROWS_AS(
      foreign->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->deleteRegion(region),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->setRangeBounds(region, barQuantity, RangeBounds(0UL, 5UL)),
      rti1516_2025::RegionNotCreatedByThisFederate);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const committedBar = owner->getRangeBounds(region, barQuantity);
  auto const committedSoda = owner->getRangeBounds(region, sodaFlavor);
  REQUIRE(committedBar.getLowerBound() == 0UL);
  REQUIRE(committedBar.getUpperBound() == 10UL);
  REQUIRE(committedSoda.getLowerBound() == 1UL);
  REQUIRE(committedSoda.getUpperBound() == 3UL);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(5UL, 15UL)));
  auto const replacement = owner->getRangeBounds(region, barQuantity);
  REQUIRE(replacement.getLowerBound() == 5UL);
  REQUIRE(replacement.getUpperBound() == 15UL);
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const recommitted = owner->getRangeBounds(region, barQuantity);
  REQUIRE(recommitted.getLowerBound() == 5UL);
  REQUIRE(recommitted.getUpperBound() == 15UL);

  // A region remains in use for deletion purposes even when its regional
  // subscription is passive.  The declaration does not arrange delivery, but
  // the region is still a live subscription dependency until it is removed.
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(serverId.isValid());
  auto const subscriptionRegion = owner->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      subscriptionRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(owner->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriptionRegion},
      false));
  REQUIRE_THROWS_AS(
      owner->deleteRegion(subscriptionRegion),
      rti1516_2025::RegionInUseForUpdateOrSubscription);
  REQUIRE_NOTHROW(owner->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(owner->deleteRegion(subscriptionRegion));

  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(foreign->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(foreign->disconnect());
}

TEST_CASE(
    "Embedded unpublishing an associated regional attribute releases region use",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][publication]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.unpublish-object-class-attributes]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.delete-region]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-publication-owner", L"owner", federationName));

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      region,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      sourcePair);
  REQUIRE(objectInstance.isValid());

  // The registration's update-region association keeps the region in use.
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  // Unpublishing the attribute removes that association.  The region usage
  // ledger must be refreshed before the service returns so the same region
  // can be deleted immediately, without requiring an unrelated mutation.
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded receive-order deletion releases a sole object's update region",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][object-deletion]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.delete-object-instance]"
    "[rti.service.delete-region]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-deleting-owner", L"owner", federationName));

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      region,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      sourcePair);
  REQUIRE(objectInstance.isValid());

  // The registered object's explicit update-region association protects the
  // region while the object is alive.
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  // With no surviving recipient, receive-order deletion purges the object in
  // the same service call.  The region ledger must be refreshed before return
  // so the former object's last association no longer blocks deletion.
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded final object-removal callback releases the deleted object's update region",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][object-deletion][callback-ordering]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.delete-object-instance]"
    "[rti.service.delete-region]"
    "[federate.callback.remove-object-instance]") {
  TestFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-removal-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"regional-removal-peer", L"subscriber", federationName));

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      region,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      sourcePair);
  REQUIRE(objectInstance.isValid());

  // Complete discovery so the peer receives the later Remove callback.
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, VariableLengthData{}));
  // The object is retained until the peer consumes its pending removal, so
  // the source region remains protected at this boundary.
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectRemovalReports.size() == 1U);
  // The final removal callback purges the execution-wide object and must
  // refresh the region ledger before returning.
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded service reporting records object-instance name reservation arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting]"
    "[object-instance-name-reservation-service-report]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.release-object-instance-name]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"object-name-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // A selected report file exists for the whole joined-federate lifetime, but
  // both switches gate later appends.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const gatedName = std::wstring{L"Umbra.GatedReservation"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(gatedName));
  REQUIRE(readTextFile(reportFile) == initialText);
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  auto const activeName = std::wstring{L"Umbra.ReportedReservation"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(activeName));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const activeNameValue = asAscii(activeName);
  auto const reserveRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ReserveObjectInstanceName","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Name","HLAargumentValue":")" +
      activeNameValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord);

  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(activeName));
  auto const releaseRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ReleaseObjectInstanceName","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Name","HLAargumentValue":")" +
      activeNameValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord + releaseRecord);

  // The dedicated object-name failure matrix owns failed-service records;
  // keep this argument regression focused on accepted forms.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(activeName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord + releaseRecord);

  // Releasing the earlier gated reservation still produces no record.
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(gatedName));
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord + releaseRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records failed object-instance name reservations",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[object-name-reservation-failure][service-report-object-name-reservation-file-failures]"
    "[rti.service.object-name-reservation-failure-matrix]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"object-name-failure-subject", L"owner", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const quote = [](std::string const& value) {
    return std::string{"\""} + value + "\"";
  };
  auto const nameArgument = [&](std::wstring const& name) {
    return std::string{"{\"HLAargumentType\":53,\"HLAargumentName\":\"Name\",\"HLAargumentValue\":"} +
        quote(asAscii(name)) + "}";
  };
  auto const reportRecord = [&quote](std::uint32_t serial,
                                     std::string const& service,
                                     std::string const& argument,
                                     bool success,
                                     std::string const& exception) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        ",\"HLAreturnedArgument\":[null],\"HLAservice\":\"" + service +
        "\",\"HLAsuppliedArguments\":[" + argument +
        "],\"HLAsuccessIndicator\":" + (success ? "true" : "false") +
        ",\"HLAexception\":" +
        (exception.empty() ? std::string{"null"} : quote(exception)) + "}";
  };
  auto const emptyName = std::wstring{};
  auto const validName = std::wstring{L"Umbra.ObjectNameFailure"};

  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(emptyName),
      rti1516_2025::IllegalName);
  auto const failedReserve = reportRecord(
      0U,
      "ReserveObjectInstanceName",
      nameArgument(emptyName),
      false,
      "IllegalName: Reserve Object Instance Name received an illegal object instance name.");
  REQUIRE(readTextFile(reportFile) == initialText + failedReserve);

  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(validName));
  auto const successfulReserve = reportRecord(
      1U,
      "ReserveObjectInstanceName",
      nameArgument(validName),
      true,
      {});
  REQUIRE(readTextFile(reportFile) == initialText + failedReserve + successfulReserve);

  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(L"Umbra.NotReserved"),
      rti1516_2025::ObjectInstanceNameNotReserved);
  auto const failedRelease = reportRecord(
      2U,
      "ReleaseObjectInstanceName",
      nameArgument(L"Umbra.NotReserved"),
      false,
      "ObjectInstanceNameNotReserved: Release Object Instance Name requires a name reserved by this federate.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedReserve + successfulReserve + failedRelease);

  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(validName));
  auto const successfulRelease = reportRecord(
      3U,
      "ReleaseObjectInstanceName",
      nameArgument(validName),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedReserve + successfulReserve + failedRelease + successfulRelease);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers failed object-instance name reservations through MOM interaction",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-reporting][service-report-interaction][service-failure]"
    "[object-name-reservation-failure][service-report-object-name-reservation-mom-failures]"
    "[rti.service.object-name-reservation-failure-matrix-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"object-name-failure-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"object-name-failure-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::wstring const& value,
                                bool success,
                                std::wstring const& exception) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 2);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get() == success);
    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 1U);
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(0U));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == 53);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == fixture_hla::fixture::name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
            L"\"" + value + L"\"");
    rti1516_2025::HLAfixedRecord nullReturned;
    nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() == L"null");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get() == exception);
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  REQUIRE_THROWS_AS(subject->reserveObjectInstanceName(L""), rti1516_2025::IllegalName);
  verifyReport(
      0U,
      L"ReserveObjectInstanceName",
      L"",
      false,
      L"IllegalName: Reserve Object Instance Name received an illegal object instance name.");
  auto const validName = std::wstring{L"Umbra.ObjectNameFailure"};
  REQUIRE_NOTHROW(subject->reserveObjectInstanceName(validName));
  verifyReport(1U, L"ReserveObjectInstanceName", validName, true, L"");
  REQUIRE_THROWS_AS(
      subject->releaseObjectInstanceName(L"Umbra.NotReserved"),
      rti1516_2025::ObjectInstanceNameNotReserved);
  verifyReport(
      2U,
      L"ReleaseObjectInstanceName",
      L"Umbra.NotReserved",
      false,
      L"ObjectInstanceNameNotReserved: Release Object Instance Name requires a name reserved by this federate.");
  REQUIRE_NOTHROW(subject->releaseObjectInstanceName(validName));
  verifyReport(3U, L"ReleaseObjectInstanceName", validName, true, L"");
  REQUIRE(observerReports.interactionReports.size() == 4U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting records multiple object-instance name reservation arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting]"
    "[object-instance-name-reservation-service-report]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-multiple-object-instance-names]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"multiple-object-name-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };

  std::set<std::wstring> const gatedNames{
      L"Umbra.GatedMultipleA", L"Umbra.GatedMultipleB"};
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames(gatedNames));
  REQUIRE(readTextFile(reportFile) == initialText);
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames(gatedNames));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  std::set<std::wstring> const emptyNames;
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames(emptyNames),
      rti1516_2025::NameSetWasEmpty);
  auto const emptyReserveFailure = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ReserveMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name Set","HLAargumentValue":[]}],"HLAsuccessIndicator":false,"HLAexception":"NameSetWasEmpty: Reserve Multiple Object Instance Names requires a non-empty object instance name set."})"};
  REQUIRE(readTextFile(reportFile) == initialText + emptyReserveFailure);

  std::set<std::wstring> const activeNames{
      L"Umbra.MultipleReservationA", L"Umbra.MultipleReservationB"};
  REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames(activeNames));
  auto const activeNameA = asAscii(L"Umbra.MultipleReservationA");
  auto const activeNameB = asAscii(L"Umbra.MultipleReservationB");
  auto const reserveRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ReserveMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name Set","HLAargumentValue":[")" +
      activeNameA +
      R"(",")" + activeNameB +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + emptyReserveFailure + reserveRecord);
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));

  auto const invalidRelease = std::set<std::wstring>{
      L"Umbra.MultipleReservationA", L"Umbra.NeverReserved"};
  REQUIRE_THROWS_AS(
      owner->releaseMultipleObjectInstanceNames(invalidRelease),
      rti1516_2025::ObjectInstanceNameNotReserved);
  auto const invalidReleaseRecord = std::string{
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"ReleaseMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name set","HLAargumentValue":["Umbra.MultipleReservationA","Umbra.NeverReserved"]}],"HLAsuccessIndicator":false,"HLAexception":"ObjectInstanceNameNotReserved: Release Multiple Object Instance Names requires a name reserved by this federate."})"};
  REQUIRE(readTextFile(reportFile) ==
          initialText + emptyReserveFailure + reserveRecord + invalidReleaseRecord);

  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames(activeNames));
  auto const releaseRecord = std::string{
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"ReleaseMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name set","HLAargumentValue":[")" +
      activeNameA +
      R"(",")" + activeNameB +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) ==
          initialText + emptyReserveFailure + reserveRecord + invalidReleaseRecord +
              releaseRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers multiple object-instance name release through MOM",
    "[integration][development-profile][federation-management][object-management][mom]"
    "[service-reporting][service-report-interaction][object-instance-name-reservation-service-report]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-multiple-object-instance-names]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"multiple-object-name-mom-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"multiple-object-name-mom-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  // Keep the setup reservation out of the interaction assertion. The
  // accepted release is the only selected public MOM service in this case.
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  std::set<std::wstring> const activeNames{
      L"Umbra.MultipleMomA", L"Umbra.MultipleMomB"};
  REQUIRE_NOTHROW(subject->reserveMultipleObjectInstanceNames(activeNames));
  static_cast<void>(subject->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subject->releaseMultipleObjectInstanceNames(activeNames));

  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(decodedService.get() == L"ReleaseMultipleObjectInstanceNames");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 2);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord suppliedPrototype;
  suppliedPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{suppliedPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 1U);
  auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(0U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == 54);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() ==
          L"Name set");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
          L"[\"Umbra.MultipleMomA\",\"Umbra.MultipleMomB\"]");

  rti1516_2025::HLAfixedRecord nullReturned;
  nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() ==
          L"null");
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded committed receiver region discovers an existing regional object when it enters overlap",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][region-commit-discovery]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[federate.callback.discover-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherFederate;
  REQUIRE_NOTHROW(publisherFederate = publisher->joinFederationExecution(
      L"region-commit-discovery-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"region-commit-discovery-receiver",
      L"subscriber",
      federationName));

  // Keep declaration-relevance advisories out of this callback-focused slice;
  // the regional subscription remains active and is the only discovery input.
  suppressDeclarationRelevanceAdvisories(*publisher);
  suppressDeclarationRelevanceAdvisories(*receiver);

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      sourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  // The object is already registered, but the receiver's committed range is
  // disjoint. Mutating and committing that same region is the discovery
  // boundary; no new subscription or registration event is involved.
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const& discovery = receiverReports.objectDiscoveryReports.front();
  REQUIRE(discovery.objectInstance == objectInstance);
  REQUIRE(discovery.objectClass == soda);
  REQUIRE(discovery.objectInstanceName == publisher->getObjectInstanceName(objectInstance));
  REQUIRE(discovery.producingFederate == publisherFederate);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded existing regional object is discovered when an overlapping update region is associated",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][association-discovery]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[federate.callback.discover-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherFederate;
  REQUIRE_NOTHROW(publisherFederate = publisher->joinFederationExecution(
      L"association-discovery-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"association-discovery-receiver",
      L"subscriber",
      federationName));

  suppressDeclarationRelevanceAdvisories(*publisher);
  suppressDeclarationRelevanceAdvisories(*receiver);
  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const sourceRegionA = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const sourceRegionB = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionA,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionB,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegionA, sourceRegionB}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const initialSourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegionA},
  }};
  AttributeHandleSetRegionHandleSetPairVector const addedSourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegionB},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      initialSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  // Registration remains disjoint. Adding the second source realization is
  // the boundary that makes this already-registered object discoverable.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, addedSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const& discovery = receiverReports.objectDiscoveryReports.front();
  REQUIRE(discovery.objectInstance == objectInstance);
  REQUIRE(discovery.objectClass == soda);
  REQUIRE(discovery.objectInstanceName == publisher->getObjectInstanceName(objectInstance));
  REQUIRE(discovery.producingFederate == publisherFederate);

  // Removing the only source association restores the default source region.
  // A separately registered object therefore crosses the same discovery
  // boundary without a new receiver declaration.
  ObjectInstanceHandle defaultedObject;
  REQUIRE_NOTHROW(defaultedObject = publisher->registerObjectInstanceWithRegions(
      soda,
      initialSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      defaultedObject,
      initialSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 2U);
  auto const& defaultedDiscovery = receiverReports.objectDiscoveryReports.back();
  REQUIRE(defaultedDiscovery.objectInstance == defaultedObject);
  REQUIRE(defaultedDiscovery.objectClass == soda);
  REQUIRE(defaultedDiscovery.producingFederate == publisherFederate);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, addedSourcePair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, initialSourcePair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionB));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionA));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting records Commit Region Modifications arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[commit-region-modifications-service-report]"
    "[region-handle-set-array-encoding]"
    "[rti.service.commit-region-modifications]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"commit-region-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Setup mutations are intentionally outside the report lane. The selected
  // file and its serial sequence remain fixed while both switches are gated.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const region = owner->createRegion(DimensionHandleSet{barQuantity});
  auto const additionalRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE(region.isValid());
  REQUIRE(additionalRegion.isValid());
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(owner->setRangeBounds(
      additionalRegion, barQuantity, RangeBounds(0UL, 10UL)));
  RegionHandleSet const regions{region, additionalRegion};
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(owner->commitRegionModifications(regions));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  std::string regionValues{"["};
  bool firstRegion = true;
  for (auto const& regionHandle : regions) {
    if (!firstRegion) {
      regionValues.push_back(',');
    }
    firstRegion = false;
    regionValues.push_back('"');
    regionValues += asAscii(regionHandle.toString());
    regionValues.push_back('"');
  }
  regionValues.push_back(']');
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CommitRegionModifications","HLAsuppliedArguments":[{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":)" +
      regionValues +
      R"(}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{RegionHandle{}}),
      rti1516_2025::InvalidRegion);
  auto const invalidCommitRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"CommitRegionModifications","HLAsuppliedArguments":[{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
      asAscii(RegionHandle{}.toString()) +
      R"("]}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRegion: Commit Region Modifications requires valid RegionHandle values."})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord + invalidCommitRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records regional object-attribute association arguments",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[regional-object-attribute-association-service-report][associate-regions-for-updates-service-report-file]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-association-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Build the object/region state outside the reporting lane. This keeps the
  // first serial tied to AssociateRegionsForUpdates rather than setup calls.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));

  auto const disjointRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      disjointRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{disjointRegion}));
  AttributeHandleSetRegionHandleSetPairVector const disjointPair{{
      flavorOnly,
      RegionHandleSet{disjointRegion},
  }};

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, disjointPair));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const regionValue = asAscii(disjointRegion.toString());
  auto const associationArgumentName =
      std::string{"Collection of attribute designator set and region designator set pairs"};
  auto const associateRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"AssociateRegionsForUpdates","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + associateRecord);

  // This legacy success-path case suppresses the rejected call; the paired
  // failure matrix below verifies its Null/false/exception record.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(
          objectInstance,
          AttributeHandleSetRegionHandleSetPairVector{{
              flavorOnly,
              RegionHandleSet{RegionHandle{}},
          }}),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText + associateRecord);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, disjointPair));
  auto const unassociateRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"UnassociateRegionsForUpdates","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + associateRecord + unassociateRecord);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(disjointRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records all Register Object Instance overloads",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[register-object-instance-service-report]"
    "[rti.service.register-object-instance]"
    "[rti.service.register-object-instance-named]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.register-object-instance-with-regions-named]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"register-object-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Resolve all FOM handles, publication, reservations, and region state
  // while reporting is gated. The first selected serial is therefore the
  // first accepted Register Object Instance overload below.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(region, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const namedObjectName = std::wstring{L"Umbra.RegisteredNamed"};
  auto const namedRegionalObjectName = std::wstring{L"Umbra.RegisteredRegionalNamed"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(namedObjectName));
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(namedRegionalObjectName));

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  ObjectInstanceHandle unnamedObject;
  ObjectInstanceHandle namedObject;
  ObjectInstanceHandle regionalObject;
  ObjectInstanceHandle namedRegionalObject;
  REQUIRE_NOTHROW(unnamedObject = owner->registerObjectInstance(soda));
  REQUIRE_NOTHROW(namedObject = owner->registerObjectInstance(soda, namedObjectName));
  REQUIRE_NOTHROW(
      regionalObject = owner->registerObjectInstanceWithRegions(soda, regionalPair));
  REQUIRE_NOTHROW(namedRegionalObject = owner->registerObjectInstanceWithRegions(
      soda, regionalPair, namedRegionalObjectName));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const classValue = asAscii(soda.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const regionValue = asAscii(region.toString());
  auto const unnamedObjectValue = asAscii(unnamedObject.toString());
  auto const namedObjectValue = asAscii(namedObject.toString());
  auto const regionalObjectValue = asAscii(regionalObject.toString());
  auto const namedRegionalObjectValue = asAscii(namedRegionalObject.toString());
  auto const namedObjectNameValue = asAscii(namedObjectName);
  auto const namedRegionalObjectNameValue = asAscii(namedRegionalObjectName);

  auto const classArgument = [&classValue] {
    return std::string{
        R"({"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")"} +
        classValue + R"("})";
  };
  auto const regionalArgument = [&attributeValue, &regionValue] {
    return std::string{
        R"({"HLAargumentType":4,"HLAargumentName":"Collection of attribute designator set and region designator set pairs","HLAargumentValue":[{"attributeHandleSet":[")"} +
        attributeValue + R"("],"regionHandleSet":[")" + regionValue + R"("]}]})";
  };
  auto const namedArgument = [](std::string const& value) {
    return std::string{
        R"({"HLAargumentType":53,"HLAargumentName":"Object instance name","HLAargumentValue":")"} +
        value + R"("})";
  };
  auto const successRecord = [](std::uint32_t serial,
                                std::string const& service,
                                std::string const& supplied,
                                std::string const& objectValue) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        R"(,"HLAreturnedArgument":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
        objectValue + R"("}],"HLAservice":")" + service +
        R"(","HLAsuppliedArguments":[)" + supplied +
        R"(],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  auto const registerRecord = successRecord(
      0U, "RegisterObjectInstance", classArgument(), unnamedObjectValue);
  auto const namedRegisterRecord = successRecord(
      1U,
      "RegisterObjectInstance",
      classArgument() + "," + namedArgument(namedObjectNameValue),
      namedObjectValue);
  auto const regionalRegisterRecord = successRecord(
      2U,
      "RegisterObjectInstanceWithRegions",
      classArgument() + "," + regionalArgument(),
      regionalObjectValue);
  auto const namedRegionalRegisterRecord = successRecord(
      3U,
      "RegisterObjectInstanceWithRegions",
      classArgument() + "," + regionalArgument() + "," +
          namedArgument(namedRegionalObjectNameValue),
      namedRegionalObjectValue);
  REQUIRE(readTextFile(reportFile) ==
          initialText + registerRecord + namedRegisterRecord + regionalRegisterRecord +
              namedRegionalRegisterRecord);

  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(unknownObjectClass),
      rti1516_2025::ObjectClassNotDefined);
  auto const invalidClassValue = asAscii(unknownObjectClass.toString());
  auto const failedRecord = std::string{
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"RegisterObjectInstance","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")"} +
      invalidClassValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"ObjectClassNotDefined: Register Object Instance requires a defined ObjectClassHandle."})";
  REQUIRE(readTextFile(reportFile) ==
          initialText + registerRecord + namedRegisterRecord + regionalRegisterRecord +
              namedRegionalRegisterRecord + failedRecord);

  // Cleanup is deliberately gated so induced delete/resign records cannot
  // obscure the five records owned by this focused overload matrix.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(regionalObject, regionalPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
      namedRegionalObject, regionalPair));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(unnamedObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(namedObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(regionalObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(namedRegionalObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records regional object-class subscription arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[regional-object-attribute-subscription-service-report]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]") {
  TestFederateAmbassador reports;
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(subscriber->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      subscriber->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-subscription-report-subscriber", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Keep setup and the switch transition out of the selected service-report
  // sequence. The first report below is therefore the accepted §9.8 call.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  auto const soda = subscriber->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = subscriber->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = subscriber->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  auto const region = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      region, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{region},
  }};

  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda, regionalPair, true, L"High"));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectClassValue = asAscii(soda.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const regionValue = asAscii(region.toString());
  auto const associationArgumentName =
      std::string{"Collection of attribute designator set and region designator set pairs"};
  auto const subscribeRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SubscribeObjectClassAttributesWithRegions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":false},{"HLAargumentType":53,"HLAargumentName":"Optional update rate designator","HLAargumentValue":"High"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord);

  // Disable both report sinks for the suppressed invocation.  With the
  // successful §9.8 route now promoted to HLAreportServiceInvocation, turning
  // off only file output would correctly send the call through the interaction
  // sink and advance the joined-federate serial.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda, regionalPair, false));
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord);
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda, regionalPair, false));
  auto const passiveSubscribeRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SubscribeObjectClassAttributesWithRegions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":true},{"HLAargumentType":34,"HLAargumentName":"Optional update rate designator","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord + passiveSubscribeRecord);

  // Suppress the rejected invocation in this success-path regression.  The
  // dedicated failure matrix separately proves its Null/false/exception
  // report; with reporting enabled this call would correctly consume serial 2.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributesWithRegions(
          soda,
          AttributeHandleSetRegionHandleSetPairVector{{
              flavorOnly,
              RegionHandleSet{RegionHandle{}},
      }}),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord + passiveSubscribeRecord);
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(soda, regionalPair));
  auto const unsubscribeRecord = std::string{
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeObjectClassAttributesWithRegions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) ==
          initialText + subscribeRecord + passiveSubscribeRecord + unsubscribeRecord);

  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subscriber->deleteRegion(region));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subscriber->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded interaction declaration services retain valid 2025 lifecycle and FOM boundaries",
    "[integration][development-profile][federation-management]"
    "[declaration-management][publish-interaction-class][subscribe-interaction-class]"
    "[unpublish-interaction-class]"
    "[unsubscribe-interaction-class]"
    "[rti.service.publish-interaction-class][rti.service.unpublish-interaction-class]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador publisherFederate;
  TestFederateAmbassador subscriberFederate;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  InteractionClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->publishInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->subscribeInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unpublishInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->unsubscribeInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      publisher->joinFederationExecution(L"declaration-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      subscriber->joinFederationExecution(L"declaration-subscriber", L"subscriber", federationName));

  REQUIRE_THROWS_AS(
      publisher->publishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->unpublishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->unsubscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);

  auto const takeOrder = publisher->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(takeOrder.isValid());

  // Declaration state remains independent and idempotent per federate.  The
  // dedicated Send Interaction test below consumes this same state for
  // hierarchy-aware callback routing.
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Publish Interaction Class arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[publish-interaction-class]"
    "[rti.service.publish-interaction-class]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"publish-interaction-report-subject", L"publisher", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated declaration-failure matrix owns failed-service records;
  // keep this success-argument regression focused on the accepted call.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  InteractionClassHandle const unknownInteraction;
  REQUIRE_THROWS_AS(
      rti->publishInteractionClass(unknownInteraction),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  auto const interactionClassText = interactionClass.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"PublishInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Subscribe Interaction Class arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"subscribe-interaction-report-subject", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated declaration-subscription failure matrix owns failed-service
  // records; keep this argument regression focused on the accepted call.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  InteractionClassHandle const unknownInteraction;
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(unknownInteraction, false),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  // The C++ `active == false` selector maps to the service's Optional passive
  // subscription indicator == true, exercising the standards-facing inverse.
  REQUIRE_NOTHROW(rti->subscribeInteractionClass(interactionClass, false));
  auto const interactionClassText = interactionClass.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SubscribeInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}





TEST_CASE(
    "Embedded service reporting delivers HLAreportServiceInvocation to an eligible observer",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][service-report-observer-eligibility][interaction-management]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction]"
    "[federate.callback.receive-interaction]") {
  for (auto const callbackModel : {HLA_EVOKED, rti1516_2025::HLA_IMMEDIATE}) {
    DYNAMIC_SECTION((callbackModel == HLA_EVOKED ? "HLA_EVOKED" : "HLA_IMMEDIATE")) {
      ReportingFederateAmbassador publisherReports;
      ReportingFederateAmbassador receiverReports;
      ReportingFederateAmbassador observerReports;
      auto publisher = makeRti();
      auto receiver = makeRti();
      auto observer = makeRti();
      auto const federationName = nextFederationName();
      auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

      REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
      REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
      REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel));
      REQUIRE_NOTHROW(publisher->createFederationExecution(
          federationName,
          fomModule,
          standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(publisher->joinFederationExecution(
          L"mom-report-publisher",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(observer->joinFederationExecution(
          L"mom-report-observer",
          L"observer",
          federationName));
      REQUIRE_NOTHROW(receiver->joinFederationExecution(
          L"mom-report-receiver",
          L"receiver",
          federationName));

      auto const publisherInteraction = publisher->getInteractionClassHandle(
          fixture_hla::fom::main_course_served);
      auto const observerInteraction = observer->getInteractionClassHandle(
          fixture_hla::fom::main_course_served);
      auto const receiverInteraction = receiver->getInteractionClassHandle(
          fixture_hla::fom::main_course_served);
      auto const publisherParameter = publisher->getParameterHandle(
          publisherInteraction,
          fixture_hla::fixture::temperature_ok);
      auto const observerParameter = observer->getParameterHandle(
          observerInteraction,
          fixture_hla::fixture::temperature_ok);
      auto const receiverParameter = receiver->getParameterHandle(
          receiverInteraction,
          fixture_hla::fixture::temperature_ok);
      REQUIRE(publisherInteraction.isValid());
      REQUIRE(observerInteraction.isValid());
      REQUIRE(receiverInteraction.isValid());
      REQUIRE(publisherParameter.isValid());
      REQUIRE(observerParameter.isValid());
      REQUIRE(receiverParameter.isValid());
      REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));
      REQUIRE_NOTHROW(observer->subscribeInteractionClass(observerInteraction));
      REQUIRE_NOTHROW(receiver->subscribeInteractionClass(receiverInteraction));

      auto const reportClass = observer->getInteractionClassHandle(
          standard_hla::mom::report_service_invocation);
      REQUIRE(reportClass.isValid());
      std::vector<ParameterHandle> reportParameters;
      for (auto const& name : {
               standard_hla::mom::service,
               standard_hla::mom::service_type,
               standard_hla::mom::success_indicator,
               standard_hla::mom::supplied_arguments,
               standard_hla::mom::returned_argument,
               standard_hla::mom::exception,
               standard_hla::mom::serial_number}) {
        auto const parameter = observer->getParameterHandle(reportClass, name);
        REQUIRE(parameter.isValid());
        reportParameters.push_back(parameter);
      }

      // The observer's own switch remains disabled, so it may subscribe to the
      // RTI-originated report interaction. A non-subscriber is the negative
      // control for report routing.
      REQUIRE_FALSE(observer->getServiceReportingSwitch());
      REQUIRE_FALSE(receiver->getServiceReportingSwitch());
      REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
      REQUIRE_FALSE(publisher->getServiceReportingSwitch());

      unsigned char const payloadBytes[] = {0x52, 0x45, 0x50, 0x54};
      ParameterHandleValueMap payload{
          {publisherParameter, VariableLengthData(payloadBytes, sizeof(payloadBytes))}};
      unsigned char const tagBytes[] = {0x52, 0x54, 0x49};
      VariableLengthData const tag(tagBytes, sizeof(tagBytes));

      // With reporting disabled, the observer subscription must not create an
      // HLAreportServiceInvocation; both federates still get the application
      // interaction because both subscribed to that class.
      REQUIRE_NOTHROW(publisher->sendInteraction(publisherInteraction, payload, tag));
      if (callbackModel == HLA_EVOKED) {
        REQUIRE(observerReports.interactionReports.empty());
        REQUIRE(receiverReports.interactionReports.empty());

        // The report is queued before the ordinary receive-order delivery. Evoke
        // until both callbacks have crossed the observer's HLA_EVOKED boundary.
        for (int attempt = 0; attempt < 4 && observerReports.interactionReports.size() < 2U;
             ++attempt) {
          static_cast<void>(observer->evokeCallback(0.0));
        }
        for (int attempt = 0; attempt < 4 && receiverReports.interactionReports.empty();
             ++attempt) {
          static_cast<void>(receiver->evokeCallback(0.0));
        }
      } else {
        REQUIRE(observerReports.interactionReports.size() == 1U);
        REQUIRE(receiverReports.interactionReports.size() == 1U);
      }
      REQUIRE(observerReports.interactionReports.size() == 1U);
      REQUIRE(receiverReports.interactionReports.size() == 1U);
      REQUIRE(observerReports.interactionReports.front().interactionClass == observerInteraction);
      REQUIRE(receiverReports.interactionReports.front().interactionClass == receiverInteraction);
      REQUIRE(variableLengthDataBytes(observerReports.interactionReports.front().userSuppliedTag) ==
              variableLengthDataBytes(tag));
      REQUIRE(variableLengthDataBytes(receiverReports.interactionReports.front().userSuppliedTag) ==
              variableLengthDataBytes(tag));
      REQUIRE(variableLengthDataBytes(
                  observerReports.interactionReports.front().parameterValues.at(observerParameter)) ==
              variableLengthDataBytes(payload.at(publisherParameter)));
      REQUIRE(variableLengthDataBytes(
                  receiverReports.interactionReports.front().parameterValues.at(receiverParameter)) ==
              variableLengthDataBytes(payload.at(publisherParameter)));

      // Once enabled, only the report-subscribed observer receives the MOM
      // report, before the ordinary interaction for this second send.
      REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
      REQUIRE(publisher->getServiceReportingSwitch());
      REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());
      REQUIRE_FALSE(observer->getServiceReportingSwitch());
      REQUIRE_NOTHROW(publisher->sendInteraction(publisherInteraction, payload, tag));
      if (callbackModel == HLA_EVOKED) {
        REQUIRE(observerReports.interactionReports.size() == 1U);
        REQUIRE(receiverReports.interactionReports.size() == 1U);
        for (int attempt = 0; attempt < 4 && observerReports.interactionReports.size() < 3U;
             ++attempt) {
          static_cast<void>(observer->evokeCallback(0.0));
        }
        for (int attempt = 0; attempt < 4 && receiverReports.interactionReports.size() < 2U;
             ++attempt) {
          static_cast<void>(receiver->evokeCallback(0.0));
        }
      } else {
        // HLA_IMMEDIATE callbacks cross the callback boundary before the service
        // invocation returns to the publisher.
        REQUIRE(observerReports.interactionReports.size() == 3U);
        REQUIRE(receiverReports.interactionReports.size() == 2U);
      }
      REQUIRE(observerReports.interactionReports.size() == 3U);
      REQUIRE(receiverReports.interactionReports.size() == 2U);
      REQUIRE(observerReports.interactionReports[0].interactionClass == observerInteraction);
      REQUIRE(observerReports.interactionReports[1].interactionClass == reportClass);
      REQUIRE(observerReports.interactionReports.back().interactionClass == observerInteraction);
      REQUIRE(variableLengthDataBytes(observerReports.interactionReports.back().userSuppliedTag) ==
              variableLengthDataBytes(tag));
      REQUIRE(variableLengthDataBytes(
                  observerReports.interactionReports.back().parameterValues.at(observerParameter)) ==
              variableLengthDataBytes(payload.at(publisherParameter)));
      REQUIRE(receiverReports.interactionReports[1].interactionClass == receiverInteraction);

      auto const& report = observerReports.interactionReports[1];
      REQUIRE(report.parameterValues.size() == 8U);
      REQUIRE(report.userSuppliedTag.size() == 0U);
      REQUIRE(report.transportationType == observer->getTransportationTypeHandle(standard_hla::mom::reliable));
      // The Requirements Lab does not resolve a callback-visible producer handle
      // for RTI-originated MOM interactions; the embedded adapter keeps that
      // unresolved fact as its default-invalid public value (RL-043).
      REQUIRE_FALSE(report.producingFederate.isValid());
      REQUIRE_FALSE(report.sentRegionsSupplied);

      rti1516_2025::HLAunicodeString decodedService;
      REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
      REQUIRE(decodedService.get() == L"SendInteraction");
      rti1516_2025::HLAinteger16BE decodedServiceType;
      REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
      REQUIRE(decodedServiceType.get() == 2);
      rti1516_2025::HLAboolean decodedSuccess;
      REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
      REQUIRE(decodedSuccess.get());
      auto const suppliedArguments = variableLengthDataBytes(
          report.parameterValues.at(reportParameters[3]));
      REQUIRE(suppliedArguments.size() > 4U);
      REQUIRE(suppliedArguments[0] == 0U);
      REQUIRE(suppliedArguments[1] == 0U);
      REQUIRE(suppliedArguments[2] == 0U);
      REQUIRE(suppliedArguments[3] == 4U);
      REQUIRE(variableLengthDataBytes(report.parameterValues.at(reportParameters[4])).size() > 4U);
      rti1516_2025::HLAunicodeString decodedException;
      REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
      REQUIRE(decodedException.get().empty());
      rti1516_2025::HLAinteger32BE decodedSerial;
      REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
      REQUIRE(decodedSerial.get() == 0);

      REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
      REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(observer->disconnect());
      REQUIRE_NOTHROW(receiver->disconnect());
      REQUIRE_NOTHROW(publisher->disconnect());
    }
  }
}

TEST_CASE(
    "Embedded service reporting delivers federate and object-class lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[rti.service.get-federate-handle][rti.service.get-federate-name]"
    "[rti.service.get-object-class-handle][rti.service.get-object-class-name]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  auto const subjectFederate = subject->joinFederationExecution(
      L"lookup-success-subject", L"subject", federationName);
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  // Keep setup lookups out of the observed serial stream. The subject's
  // switch is enabled only after the observer has selected the public route.
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const subjectName = std::wstring{L"lookup-success-subject"};
  auto const serverName = std::wstring{fixture_hla::fom::employee_server};
  auto const subjectHandle = subject->getFederateHandle(subjectName);
  auto const serverHandle = subject->getObjectClassHandle(serverName);
  REQUIRE(subjectHandle == subjectFederate);
  REQUIRE(serverHandle.isValid());
  REQUIRE(subject->getFederateName(subjectHandle) == subjectName);
  REQUIRE(subject->getObjectClassName(serverHandle) == serverName);

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->getFederateHandle(subjectName));
  REQUIRE_NOTHROW(subject->getFederateName(subjectHandle));
  REQUIRE_NOTHROW(subject->getObjectClassHandle(serverName));
  REQUIRE_NOTHROW(subject->getObjectClassName(serverHandle));
  REQUIRE(observerReports.interactionReports.size() == 4U);

  auto quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto decodeReport = [&](std::size_t index,
                          std::wstring const& service,
                          rti1516_2025::Integer32 suppliedType,
                          std::wstring const& suppliedName,
                          std::wstring const& suppliedValue,
                          rti1516_2025::Integer32 returnedType,
                          std::wstring const& returnedName,
                          std::wstring const& returnedValue) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 1U);
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(0U));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() ==
            suppliedType);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() ==
            suppliedName);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
            suppliedValue);

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
            returnedType);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
            returnedName);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
            returnedValue);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"GetFederateHandle",
      53,
      L"Federate name",
      quoted(subjectName),
      15,
      L"Federate handle",
      quoted(subjectHandle.toString()));
  decodeReport(
      1U,
      L"GetFederateName",
      15,
      L"Federate handle",
      quoted(subjectHandle.toString()),
      53,
      L"Federate name",
      quoted(subjectName));
  decodeReport(
      2U,
      L"GetObjectClassHandle",
      53,
      L"Object class name",
      quoted(serverName),
      36,
      L"Object class handle",
      quoted(serverHandle.toString()));
  decodeReport(
      3U,
      L"GetObjectClassName",
      36,
      L"Object class handle",
      quoted(serverHandle.toString()),
      53,
      L"Object class name",
      quoted(serverName));

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers object, attribute, and update-rate lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[federate-object-attribute-update-rate-lookup-service-reports]"
    "[service-report-successful-lookup-matrix]"
    "[rti.service.get-known-object-class-handle][rti.service.get-object-instance-handle]"
    "[rti.service.get-object-instance-name][rti.service.get-attribute-handle]"
    "[rti.service.get-attribute-name][rti.service.get-update-rate-value]"
    "[rti.service.get-update-rate-value-for-attribute]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"lookup-success-object-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-object-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const serverName = std::wstring{fixture_hla::fom::employee_server};
  auto const objectName = std::wstring{L"lookup-success-object"};
  auto const attributeName = std::wstring{fixture_hla::fixture::efficiency};
  auto const server = subject->getObjectClassHandle(serverName);
  auto const attribute = subject->getAttributeHandle(server, attributeName);
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(server, AttributeHandleSet{attribute}));
  REQUIRE_NOTHROW(subject->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  auto const object = subject->registerObjectInstance(server, objectName);
  REQUIRE(object.isValid());
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const knownClass = subject->getKnownObjectClassHandle(object);
  auto const objectByName = subject->getObjectInstanceHandle(objectName);
  auto const resolvedObjectName = subject->getObjectInstanceName(object);
  auto const attributeByClass = subject->getAttributeHandle(server, attributeName);
  auto const resolvedAttributeName = subject->getAttributeName(server, attribute);
  auto const maximumRate = subject->getUpdateRateValue(L"High");
  auto const attributeMaximumRate = subject->getUpdateRateValueForAttribute(object, attribute);
  REQUIRE(knownClass == server);
  REQUIRE(objectByName == object);
  REQUIRE(resolvedObjectName == objectName);
  REQUIRE(attributeByClass == attribute);
  REQUIRE(resolvedAttributeName == attributeName);
  REQUIRE(maximumRate == Catch::Approx(30.0));
  REQUIRE(attributeMaximumRate == Catch::Approx(0.0));
  REQUIRE(observerReports.interactionReports.size() == 7U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  struct ExpectedArgument final {
    rti1516_2025::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti1516_2025::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(0U)).get() ==
            expected.type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(1U)).get() ==
            expected.name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(2U)).get() ==
            expected.value);
  };
  auto const decodeReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedSupplied.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedSupplied.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      decodeArgument(supplied, expectedSupplied[argumentIndex]);
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    decodeArgument(returnedArgument, expectedReturned);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"GetKnownObjectClassHandle",
      {{37, L"Object instance handle", quoted(object.toString())}},
      {36, L"Object class handle", quoted(knownClass.toString())});
  decodeReport(
      1U,
      L"GetObjectInstanceHandle",
      {{53, L"Object instance name", quoted(objectName)}},
      {37, L"Object instance handle", quoted(objectByName.toString())});
  decodeReport(
      2U,
      L"GetObjectInstanceName",
      {{37, L"Object instance handle", quoted(object.toString())}},
      {53, L"Object instance name", quoted(resolvedObjectName)});
  decodeReport(
      3U,
      L"GetAttributeHandle",
      {{36, L"Object class handle", quoted(server.toString())},
       {53, L"Class attribute name", quoted(attributeName)}},
      {0, L"Class attribute handle", quoted(attributeByClass.toString())});
  decodeReport(
      4U,
      L"GetAttributeName",
      {{36, L"Object class handle", quoted(server.toString())},
       {0, L"Class attribute handle", quoted(attribute.toString())}},
      {53, L"Class attribute name", quoted(resolvedAttributeName)});
  decodeReport(
      5U,
      L"GetUpdateRateValue",
      {{53, L"Update rate name", quoted(L"High")}},
      {35, L"Maximum update rate value", std::to_wstring(maximumRate)});
  decodeReport(
      6U,
      L"GetUpdateRateValueForAttribute",
      {{37, L"Object instance handle", quoted(object.toString())},
       {0, L"Attribute handle", quoted(attribute.toString())}},
      {35, L"Maximum update rate value", std::to_wstring(attributeMaximumRate)});

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers interaction-class and parameter lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[federation-interaction-parameter-lookup-service-reports]"
    "[service-report-successful-interaction-parameter-lookup-matrix]"
    "[rti.service.get-interaction-class-handle][rti.service.get-interaction-class-name]"
    "[rti.service.get-parameter-handle][rti.service.get-parameter-name]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"lookup-success-interaction-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-interaction-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const interactionName = std::wstring{fixture_hla::fom::main_course_served};
  auto const parameterName = std::wstring{fixture_hla::fixture::temperature_ok};
  auto const interaction = subject->getInteractionClassHandle(interactionName);
  auto const parameter = subject->getParameterHandle(interaction, parameterName);
  REQUIRE(interaction.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const resolvedInteraction = subject->getInteractionClassHandle(interactionName);
  auto const resolvedInteractionName = subject->getInteractionClassName(interaction);
  auto const resolvedParameter = subject->getParameterHandle(interaction, parameterName);
  auto const resolvedParameterName = subject->getParameterName(interaction, parameter);
  REQUIRE(resolvedInteraction == interaction);
  REQUIRE(resolvedInteractionName == interactionName);
  REQUIRE(resolvedParameter == parameter);
  REQUIRE(resolvedParameterName == parameterName);
  REQUIRE(observerReports.interactionReports.size() == 4U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  struct ExpectedArgument final {
    rti1516_2025::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti1516_2025::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(0U)).get() ==
            expected.type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(1U)).get() ==
            expected.name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(2U)).get() ==
            expected.value);
  };
  auto const decodeReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedSupplied.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedSupplied.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      decodeArgument(supplied, expectedSupplied[argumentIndex]);
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    decodeArgument(returnedArgument, expectedReturned);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"GetInteractionClassHandle",
      {{53, L"Interaction class name", quoted(interactionName)}},
      {27, L"Interaction class handle", quoted(resolvedInteraction.toString())});
  decodeReport(
      1U,
      L"GetInteractionClassName",
      {{27, L"Interaction class handle", quoted(interaction.toString())}},
      {53, L"Interaction class name", quoted(resolvedInteractionName)});
  decodeReport(
      2U,
      L"GetParameterHandle",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {53, L"Parameter name", quoted(parameterName)}},
      {39, L"Parameter handle", quoted(resolvedParameter.toString())});
  decodeReport(
      3U,
      L"GetParameterName",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {39, L"Parameter handle", quoted(parameter.toString())}},
      {53, L"Parameter name", quoted(resolvedParameterName)});

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers order and transportation lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction][transportation-management]"
    "[time-management][order-transportation-lookup-service-reports]"
    "[service-report-successful-order-transportation-lookup-matrix]"
    "[rti.service.get-order-type][rti.service.get-order-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-transportation-type-name]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"lookup-success-order-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-order-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const reliable = subject->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(reliable.isValid());
  REQUIRE(subject->getOrderType(L"Receive") == RECEIVE);
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const timestampOrder = subject->getOrderType(L"TimeStamp");
  auto const receiveOrderName = subject->getOrderName(RECEIVE);
  auto const reliableByName = subject->getTransportationTypeHandle(standard_hla::mom::reliable);
  auto const reliableName = subject->getTransportationTypeName(reliable);
  REQUIRE(timestampOrder == TIMESTAMP);
  REQUIRE(receiveOrderName == L"Receive");
  REQUIRE(reliableByName == reliable);
  REQUIRE(reliableName == standard_hla::mom::reliable);
  REQUIRE(observerReports.interactionReports.size() == 4U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  struct ExpectedArgument final {
    rti1516_2025::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti1516_2025::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(0U)).get() ==
            expected.type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(1U)).get() ==
            expected.name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(2U)).get() ==
            expected.value);
  };
  auto const decodeReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedSupplied.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedSupplied.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      decodeArgument(supplied, expectedSupplied[argumentIndex]);
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    decodeArgument(returnedArgument, expectedReturned);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"GetOrderType",
      {{53, L"Order name", quoted(L"TimeStamp")}},
      {38, L"Order type", quoted(L"TIMESTAMP")});
  decodeReport(
      1U,
      L"GetOrderName",
      {{38, L"Order type", quoted(L"RECEIVE")}},
      {53, L"Order name", quoted(receiveOrderName)});
  decodeReport(
      2U,
      L"GetTransportationTypeHandle",
      {{53, L"Transportation type name", quoted(standard_hla::mom::reliable)}},
      {59, L"Transportation type handle", quoted(reliableByName.toString())});
  decodeReport(
      3U,
      L"GetTransportationTypeName",
      {{59, L"Transportation type handle", quoted(reliable.toString())}},
      {53, L"Transportation type name", quoted(reliableName)});

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers dimension and region lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-reporting][service-report-interaction]"
    "[dimension-region-lookup-service-reports]"
    "[service-report-successful-dimension-region-lookup-matrix]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound][rti.service.get-dimension-handle-set]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"lookup-success-dimension-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-dimension-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const barQuantity = subject->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const serverId = subject->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const drink = subject->getObjectClassHandle(fixture_hla::fom::food_drink);
  auto const mainCourseServed = subject->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const region = subject->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE(barQuantity.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(drink.isValid());
  REQUIRE(mainCourseServed.isValid());
  REQUIRE(region.isValid());
  REQUIRE_NOTHROW(subject->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const availableObjectDimensions = subject->getAvailableDimensionsForObjectClass(drink);
  auto const availableInteractionDimensions =
      subject->getAvailableDimensionsForInteractionClass(mainCourseServed);
  auto const resolvedDimension =
      subject->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const resolvedDimensionName = subject->getDimensionName(barQuantity);
  auto const upperBound = subject->getDimensionUpperBound(barQuantity);
  auto const resolvedDimensionSet = subject->getDimensionHandleSet(region);
  REQUIRE(availableObjectDimensions == DimensionHandleSet{barQuantity});
  REQUIRE(availableInteractionDimensions == DimensionHandleSet{serverId});
  REQUIRE(resolvedDimension == barQuantity);
  REQUIRE(resolvedDimensionName == fixture_hla::fixture::bar_quantity);
  REQUIRE(upperBound == 25UL);
  REQUIRE(resolvedDimensionSet == DimensionHandleSet{barQuantity});
  REQUIRE(observerReports.interactionReports.size() == 6U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const dimensionSetText = [](DimensionHandle const& value) {
    return std::wstring{L"[\""} + value.toString() + L"\"]";
  };
  struct ExpectedArgument final {
    rti1516_2025::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti1516_2025::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(0U)).get() ==
            expected.type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(1U)).get() ==
            expected.name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(2U)).get() ==
            expected.value);
  };
  auto const decodeReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedSupplied.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedSupplied.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      decodeArgument(supplied, expectedSupplied[argumentIndex]);
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    decodeArgument(returnedArgument, expectedReturned);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"GetAvailableDimensionsForObjectClass",
      {{36, L"Object class handle", quoted(drink.toString())}},
      {11, L"A set of dimension handles", dimensionSetText(barQuantity)});
  decodeReport(
      1U,
      L"GetAvailableDimensionsForInteractionClass",
      {{27, L"Interaction class handle", quoted(mainCourseServed.toString())}},
      {11, L"A set of dimension handles", dimensionSetText(serverId)});
  decodeReport(
      2U,
      L"GetDimensionHandle",
      {{53, L"Dimension name", quoted(fixture_hla::fixture::bar_quantity)}},
      {10, L"Dimension handle", quoted(resolvedDimension.toString())});
  decodeReport(
      3U,
      L"GetDimensionName",
      {{10, L"Dimension handle", quoted(barQuantity.toString())}},
      {53, L"Dimension name", quoted(resolvedDimensionName)});
  decodeReport(
      4U,
      L"GetDimensionUpperBound",
      {{10, L"Dimension handle", quoted(barQuantity.toString())}},
      {35, L"Dimension upper bound", std::to_wstring(upperBound)});
  decodeReport(
      5U,
      L"GetDimensionHandleSet",
      {{42, L"Region handle", quoted(region.toString())}},
      {11, L"A set of dimensions", dimensionSetText(barQuantity)});

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->deleteRegion(region));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers Create Region and Get Range Bounds return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-reporting][service-report-interaction]"
    "[ddm-nonvoid-service-reports]"
    "[service-report-successful-ddm-nonvoid-matrix]"
    "[rti.service.create-region][rti.service.get-range-bounds]"
    "[rti.service.get-dimension-handle][rti.service.set-range-bounds]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"ddm-nonvoid-success-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"ddm-nonvoid-success-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const barQuantity = subject->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const existingRegion = subject->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE(existingRegion.isValid());
  REQUIRE_NOTHROW(
      subject->setRangeBounds(existingRegion, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const createdRegion = subject->createRegion(DimensionHandleSet{barQuantity});
  auto const rangeBounds = subject->getRangeBounds(existingRegion, barQuantity);
  REQUIRE(createdRegion.isValid());
  REQUIRE(rangeBounds.getLowerBound() == 0UL);
  REQUIRE(rangeBounds.getUpperBound() == 10UL);
  REQUIRE(observerReports.interactionReports.size() == 2U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const dimensionSetText = [](DimensionHandle const& value) {
    return std::wstring{L"[\""} + value.toString() + L"\"]";
  };
  struct ExpectedArgument final {
    rti1516_2025::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti1516_2025::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(0U)).get() ==
            expected.type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(1U)).get() ==
            expected.name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(2U)).get() ==
            expected.value);
  };
  auto const decodeReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 5);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedSupplied.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedSupplied.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      decodeArgument(supplied, expectedSupplied[argumentIndex]);
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    decodeArgument(returnedArgument, expectedReturned);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"CreateRegion",
      {{11, L"Set of dimension designators", dimensionSetText(barQuantity)}},
      {42, L"Region designator", quoted(createdRegion.toString())});
  decodeReport(
      1U,
      L"GetRangeBounds",
      {{42, L"Region handle", quoted(existingRegion.toString())},
       {10, L"Dimension handle", quoted(barQuantity.toString())}},
      {41, L"Range bounds", L"{\"lower\":0,\"upper\":10}"});

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->deleteRegion(createdRegion));
  REQUIRE_NOTHROW(subject->deleteRegion(existingRegion));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers handle normalization return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[handle-normalization-service-reports]"
    "[service-report-successful-handle-normalization-matrix]"
    "[rti.service.normalize-service-group][rti.service.normalize-federate-handle]"
    "[rti.service.normalize-object-class-handle]"
    "[rti.service.normalize-interaction-class-handle]"
    "[rti.service.normalize-object-instance-handle]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"handle-normalization-success-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"handle-normalization-success-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const server = subject->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = subject->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(server, {efficiency}));
  auto const takeOrder = subject->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const objectInstance = subject->registerObjectInstance(server);
  auto const ownerFederate = subject->getFederateHandle(
      L"handle-normalization-success-subject");
  auto const serviceGroupValue = subject->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES);
  auto const federateValue = subject->normalizeFederateHandle(ownerFederate);
  auto const objectClassValue = subject->normalizeObjectClassHandle(server);
  auto const interactionClassValue = subject->normalizeInteractionClassHandle(takeOrder);
  auto const objectInstanceValue = subject->normalizeObjectInstanceHandle(objectInstance);
  REQUIRE(serviceGroupValue == static_cast<unsigned long>(rti1516_2025::SUPPORT_SERVICES));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());
  REQUIRE(subject->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES) == serviceGroupValue);
  REQUIRE(subject->normalizeFederateHandle(ownerFederate) == federateValue);
  REQUIRE(subject->normalizeObjectClassHandle(server) == objectClassValue);
  REQUIRE(subject->normalizeInteractionClassHandle(takeOrder) == interactionClassValue);
  REQUIRE(subject->normalizeObjectInstanceHandle(objectInstance) == objectInstanceValue);
  REQUIRE(observerReports.interactionReports.size() == 5U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  struct ExpectedArgument final {
    rti1516_2025::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti1516_2025::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(0U)).get() ==
            expected.type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(1U)).get() ==
            expected.name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(2U)).get() ==
            expected.value);
  };
  auto const decodeReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedSupplied.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedSupplied.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      decodeArgument(supplied, expectedSupplied[argumentIndex]);
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    decodeArgument(returnedArgument, expectedReturned);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"NormalizeServiceGroup",
      {{50, L"Service group indicator", quoted(L"SUPPORT_SERVICES")}},
      {35, L"Normalized value", std::to_wstring(serviceGroupValue)});
  decodeReport(
      1U,
      L"NormalizeFederateHandle",
      {{15,
        L"Federate handle",
        quoted(ownerFederate.toString())}},
      {35, L"Normalized value", std::to_wstring(federateValue)});
  decodeReport(
      2U,
      L"NormalizeObjectClassHandle",
      {{36, L"Object class handle", quoted(server.toString())}},
      {35, L"Normalized value", std::to_wstring(objectClassValue)});
  decodeReport(
      3U,
      L"NormalizeInteractionClassHandle",
      {{27, L"Interaction class handle", quoted(takeOrder.toString())}},
      {35, L"Normalized value", std::to_wstring(interactionClassValue)});
  decodeReport(
      4U,
      L"NormalizeObjectInstanceHandle",
      {{37, L"Object instance handle", quoted(objectInstance.toString())}},
      {35, L"Normalized value", std::to_wstring(objectInstanceValue)});

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->deleteObjectInstance(objectInstance, VariableLengthData{}));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service-report files have one immutable joined-federate lifetime",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withConfigurationName(L"report-file-test").withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-subject", L"observer", federationName));
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());

  auto files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const firstFile = files.front();
  auto const initialText = readTextFile(firstFile);
  REQUIRE_FALSE(initialText.empty());
  REQUIRE(initialText.front() == '{');
  REQUIRE(initialText.find("\"CallbackModel\":\"HLA_EVOKED\"") != std::string::npos);
  REQUIRE(initialText.find("\"ConfigurationName\":\"report-file-test\"") != std::string::npos);
  REQUIRE(initialText.find("\"HLAfederationName\":\"umbra-catch2-federation-") !=
          std::string::npos);
  REQUIRE(initialText.find("\"HLAMIMDesignator\":\"HLAstandardMIM\"") != std::string::npos);
  REQUIRE(initialText.find("\"HLAfederateName\":\"service-report-subject\"") !=
          std::string::npos);
  REQUIRE(initialText.find("\"HLAserialNumber\"") == std::string::npos);

  // File reporting switches gate future report appends only. They cannot
  // rotate or replace the preallocated joined-federate file, even when both
  // switches were disabled at the join that created its initial record.
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  REQUIRE(readTextFile(firstFile) == initialText);
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(true));
  auto const firstRecord =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(firstFile) == initialText + firstRecord);
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  REQUIRE(readTextFile(firstFile) == initialText + firstRecord);
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  auto const secondRecord =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(serviceReportFiles(directory.path()) ==
          std::vector<std::filesystem::path>{firstFile});
  REQUIRE(readTextFile(firstFile) == initialText + firstRecord + secondRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-subject", L"observer", federationName));
  files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);
  REQUIRE(files.front() != files.back());
  REQUIRE(std::find(files.begin(), files.end(), firstFile) != files.end());
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}








#if 0
TEST_CASE(
      rti1516_2025::RTIinternalError);
  REQUIRE(observerReports.attributeReflectionReports.size() == beforeTiming);

  sendTiming(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  while (observer->evokeCallback(0.0)) {
  }

  auto const periodicReflection = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforeTiming),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == subjectObjectInstance &&
            report.attributeValues.contains(logicalTimeAttribute) &&
            report.attributeValues.contains(lookaheadAttribute);
      });
  REQUIRE(periodicReflection != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->attributeValues.size() == 2U);
  rti1516_2025::HLAinteger64Time periodicLogicalTime;
  REQUIRE_NOTHROW(periodicLogicalTime.decode(
      periodicReflection->attributeValues.at(logicalTimeAttribute)));
  REQUIRE(periodicLogicalTime.getTime() == 0);
  // The subject has not enabled time regulation, so the official undefined
  // HLAlookahead value remains the MIM's empty variable-array form.
  REQUIRE(periodicReflection->attributeValues.at(lookaheadAttribute).size() == 0U);
  REQUIRE(periodicReflection->transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
  REQUIRE_FALSE(periodicReflection->sentRegionsSupplied);

  auto const beforeDisable = observerReports.attributeReflectionReports.size();
  sendTiming(0);
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.attributeReflectionReports.size() == beforeDisable);

  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

#endif

#if 0
TEST_CASE(
    "Embedded federation MOM exposes static configuration values",
    "[integration][development-profile][federation-management][mom][mom-static]"
    "[rti.service.join-federation-execution][rti.service.subscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador observerReports;
  auto owner = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const knownClassFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-known-class-enabled-fom.xml")
          .wstring();
  auto const nonRegulatedGrantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-enabled-fom.xml")
          .wstring();
  auto const relaxedDdmFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "allow-relaxed-ddm-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{
      restaurantFom,
      knownClassFom,
      nonRegulatedGrantFom,
      relaxedDdmFom,
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mom-static-owner", L"owner", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-static-observer", L"observer", federationName));

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federation_object_class);
  auto attribute = [&](wchar_t const* name) {
    return observer->getAttributeHandle(momClass, name);
  };
  auto const federationNameAttribute = attribute(L"HLAfederationName");
  auto const rtiVersionAttribute = attribute(L"HLARTIversion");
  auto const mimDesignatorAttribute = attribute(L"HLAMIMdesignator");
  auto const timeImplementationAttribute = attribute(L"HLAtimeImplementationName");
  auto const knownClassAttribute = attribute(L"HLAadvisoriesUseKnownClass");
  auto const delayedSubscriptionAttribute = attribute(L"HLAdelaySubscriptionEvaluation");
  auto const nonRegulatedGrantAttribute = attribute(L"HLAnonRegulatedGrant");
  auto const relaxedDdmAttribute = attribute(L"HLAallowRelaxedDDM");
  AttributeHandleSet const staticAttributes{
      federationNameAttribute,
      rtiVersionAttribute,
      mimDesignatorAttribute,
      timeImplementationAttribute,
      knownClassAttribute,
      delayedSubscriptionAttribute,
      nonRegulatedGrantAttribute,
      relaxedDdmAttribute,
  };
  REQUIRE(momClass.isValid());
  REQUIRE(staticAttributes.size() == 8U);
  for (auto const handle : staticAttributes) {
    REQUIRE(handle.isValid());
  }

  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass, staticAttributes, true));
  while (observer->evokeCallback(0.0)) {
  }
  auto const discoveredFederation = std::find_if(
      observerReports.objectDiscoveryReports.begin(),
      observerReports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectClass == momClass &&
            report.objectInstanceName == standard_hla::mom::federation;
      });
  REQUIRE(discoveredFederation != observerReports.objectDiscoveryReports.end());
  auto const federationObjextTime.has_value());
  REQUIRE(pending.nextTime->getTime() == 7);
  REQUIRE(pending.lastName.empty());
  REQUIRE(pending.lastTimeBytes == 0U);

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  static_cast<void>(owner->evokeCallback(0.0));
  while (observer->evokeCallback(0.0)) {
  }
  auto admitted = readNextConditionals();
  REQUIRE(admitted.nextName.empty());
  REQUIRE(admitted.nextTimeBytes == 0U);
  REQUIRE(admitted.lastName.empty());
  REQUIRE(admitted.lastTimeBytes == 0U);

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(constrained->federateSaveBegun());
  REQUIRE_NOTHROW(observer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(constrained->federateSaveComplete());
  REQUIRE_NOTHROW(observer->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (constrained->evokeCallback(0.0)) {
  }
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(constrainedReports.federationSavedReportCount == 1U);
  auto completed = readLastConditionals();
  REQUIRE(completed.lastName == L"save-conditionals-pending");
  REQUIRE(completed.lastTime.has_value());
  REQUIRE(completed.lastTime->getTime() == 7);

  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(constrained->disconnect());
}

TEST_CASE(
    "Embedded MOM HLAsetTiming drives HLA_IMMEDIATE periodic values without Evoke",
    "[integration][development-profile][federation-management][mom][periodic-mom]"
    "[callback-model][immediate-callback][rti.service.send-interaction]"
    "[federate.callback.reflect-attribute-values]") {
  NullFederateAmbassador subjectReports;
  ImmediatePeriodicFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-immediate-observer", L"observer", federationName));

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const logicalTimeAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::logical_time);
  auto const lookaheadAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::lookahead);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(logicalTimeAttribute.isValid());
  REQUIRE(lookaheadAttribute.isValid());
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{
          federateHandleAttribute,
          logicalTimeAttribute,
          lookaheadAttribute},
      true));

  auto const subjectFederate = subject->joinFederationExecution(
      L"mom-immediate-subject", L"subject", federationName);
  REQUIRE(observerReports.waitForReflectionCount(1U, std::chrono::milliseconds(1000)));
  auto initialReflections = observerReports.reflectionSnapshot();
  auto const reflectedSubject = std::find_if(
      initialReflections.begin(),
      initialReflections.end(),
      [&](ImmediatePeriodicFederateAmbassador::Reflection const& reflection) {
        auto const value = reflection.attributeValues.find(federateHandleAttribute);
        return value != reflection.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(subjectFederate.encode());
      });
  REQUIRE(reflectedSubject != initialReflections.end());
  auto const subjectObjectInstance = reflectedSubject->objectInstance;
  auto const beforeTiming = initialReflections.size();

  auto const setTiming = observer->getInteractionClassHandle(
      standard_hla::mom::set_timing);
  auto const federateParameter = observer->getParameterHandle(setTiming, standard_hla::mom::federate);
  auto const periodParameter = observer->getParameterHandle(setTiming, standard_hla::mom::report_period);
  REQUIRE(setTiming.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(periodParameter.isValid());

  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, subjectFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));

  // No Evoke call is made on the HLA_IMMEDIATE observer.  The RTI-owned
  // scheduler must nevertheless enter the callback once the one-second
  // HLAsetTiming deadline elapses.
  REQUIRE(observerReports.waitForReflectionCount(
      beforeTiming + 1U,
      std::chrono::milliseconds(2500)));
  auto periodicReflections = observerReports.reflectionSnapshot();
  auto const periodicReflection = std::find_if(
      periodicReflections.begin() + static_cast<std::ptrdiff_t>(beforeTiming),
      periodicReflections.end(),
      [&](ImmediatePeriodicFederateAmbassador::Reflection const& reflection) {
        return reflection.objectInstance == subjectObjectInstance &&
            reflection.attributeValues.contains(logicalTimeAttribute) &&
            reflebsubscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{federateHandleAttribute, roLengthAttribute},
      true));

  auto const targetFederate = target->joinFederationExecution(
      L"mom-ro-length-target", L"target", federationName);
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"mom-ro-length-publisher", L"publisher", federationName));
  while (observer->evokeCallback(0.0)) {
  }

  auto const reflectedTarget = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(federateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(targetFederate.encode());
      });
  REQUIRE(reflectedTarget != observerReports.attributeReflectionReports.end());
  auto const targetObjectInstance = reflectedTarget->objectInstance;

  auto const requestRoLength = [&] {
    auto const before = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        targetObjectInstance,
        AttributeHandleSet{roLengthAttribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == before + 1U);
    auto const& reflection = observerReports.attributeReflectionReports.back();
    REQUIRE(reflection.objectInstance == targetObjectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    rti1516_2025::HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(reflection.attributeValues.at(roLengthAttribute)));
    REQUIRE(reflection.transportationType == observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    return decoded.get();
  };

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const targetInteraction = target->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const publisherParameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(targetInteraction.isValid());
  REQUIRE(publisherParameter.isValid());
  REQUIRE_NOTHROW(target->subscribeInteractionClass(targetInteraction));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  REQUIRE(requestRoLength() == 0);
  unsigned char const payloadBytes[] = {'q', 'u', 'e', 'u', 'e', 'd'};
  VariableLengthData const tag(
      reinterpret_cast<unsigned char const*>("queued-ro-tag"), 13U);
  REQUIRE_NOTHROW(publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{{
          publisherParameter,
          VariableLengthData(payloadBytes, sizeof(payloadBytes))}},
      tag));
  REQUIRE(targetReports.interactionReports.empty());

  // The target is HLA_EVOKED and has not crossed its callback boundary, so
  // its peer's MOM object must expose one pending receive-order task.
  REQUIRE(requestRoLength() == 1);

  auto const setTiming = observer->getInteractionClassHandle(
      standard_hla::mom::set_timing);
  auto const federateParameter = observer->getParameterHandle(setTiming, standard_hla::mom::federate);
  auto const periodParameter = observer->getParameterHandle(setTiming, standard_hla::mom::report_period);
  REQUIRE(setTiming.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(periodParameter.isValid());
  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, targetFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  auto const beforePeriodic = observerReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  while (observer->evokeCallback(0.0)) {
  }
  auto const periodicReflection = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePeriodic),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == targetObjectInstance &&
            report.attributeValues.contains(roLengthAttribute);
      });
  REQUIRE(periodicReflection != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->attributeValues.size() == 1U);
  rti1516_2025::HLAinteger32BE periodicCount;
  REQUIRE_NOTHROW(periodicCount.decode(
      periodicReflection->attributeValues.at(roLengthAttribute)));
  REQUIRE(periodicCount.get() == 1);
  REQUIRE(periodicReflection->transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
  REQUIRE_FALSE(periodicReflection->sentRegionsSupplied);

  REQUIRE_NOTHROW(target->evokeCallback(0.0));
  REQUIRE(targetReports.interactionReports.size() == 1U);
  REQUIRE(targetReports.interactionReports.front().interactionClass == targetInteraction);
  REQUIRE(variableLengthDataBytes(targetReports.interactionReports.front().userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(variableLengthDataBytes(
              targetReports.interactionReports.front().parameterValues.at(
                  target->getParameterHandle(targetInteraction, fixture_hla::fixture::identifier))) ==
          std::vector<unsigned char>(payloadBytes, payloadBytes + sizeof(payloadBytes)));

  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParamevattributeReflectionReports.size() == before + 1U);
    auto const& reflection = observerReports.attributeReflectionReports.back();
    REQUIRE(reflection.objectInstance == objectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    rti1516_2025::HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(reflection.attributeValues.at(attribute)));
    return decoded.get();
  };
  auto requestCount = [&](AttributeHandle attribute) {
    return requestCountFor(subjectObjectInstance, attribute);
  };

  // Direct Request Attribute Value Update exposes the MIM periodic value even
  // before HLAsetTiming is configured.
  REQUIRE(requestCount(updatesSentAttribute) == 0);
  REQUIRE(requestCount(updatedInstancesAttribute) == 0);
  REQUIRE(requestCount(registeredInstancesAttribute) == 0);
  REQUIRE(requestCount(deletedInstancesAttribute) == 0);
  REQUIRE(requestCountFor(observerObjectInstance, removedInstancesAttribute) == 0);
  // The discovery counter is recipient-owned: it is read from the MOM object
  // representing the federate that received the Discover Object Instance
  // callback, not from the producer's MOM object.
  REQUIRE(requestCountFor(observerObjectInstance, discoveredInstancesAttribute) == 0);

  auto const serverClass = subject->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiencyAttribute = subject->getAttributeHandle(
      serverClass, fixture_hla::fixture::efficiency);
  auto const observerServerClass = observer->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const observerEfficiencyAttribute = observer->getAttributeHandle(
      observerServerClass, fixture_hla::fixture::efficiency);
  REQUIRE(serverClass.isValid());
  REQUIRE(efficiencyAttribute.isValid());
  REQUIRE(observerServerClass.isValid());
  REQUIRE(observerEfficiencyAttribute.isValid());
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(
      serverClass,
      AttributeHandleSet{efficiencyAttribute}));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      observerServerClass,
      AttributeHandleSet{observerEfficiencyAttribute},
      true));
  auto const initialDiscoveryReports = observerReports.objectDiscoveryReports.size();
  auto const registeredObject = subject->registerObjectInstance(serverClass);
  REQUIRE(registeredObject.isValid());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(registeredInstancesAttribute) == 1);
  REQUIRE(requestCountFor(observerObjectInstance, discoveredInstancesAttribute) == 1);
  REQUIRE(observerReports.objectDiscoveryReports.size() == initialDiscoveryReports + 1U);

  unsigned char const firstValueBytes[] = {0x2A};
  unsigned char const secondValueBytes[] = {0x2B};
  REQUIRE_NOTHROW(subject->updateAttributeValues(
      registeredObject,
      AttributeHandleValueMap{
          {efficiencyAttribute,
           VariableLengthData(firstValueBytes, sizeof(firstValueBytes))}},
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(updatesSentAttribute) == 1);
  REQUIRE(requestCount(updatedInstancesAttribute) == 1);
  REQUIRE_NOTHROW(subject->updateAttributeValues(
      registeredObject,
      AttributeHandleValueMap{
          {efficiencyAttribute,
           VariableLengthData(secondValueBytes, sizeof(secondValueBytes))}},
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(updatesSentAttribute) == 2);
  // Repeated service invocations for one object do not inflate the distinct
  // HLAobjectInstancesUpdated count.
  REQUIRE(requestCount(updatedInstancesAttribute) == 1);

  auto const secondRegisteredObject = subject->registerObjectInstance(serverClass);
  REQUIRE(secondRegisteredObject.isValid());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(registeredInstancesAttribute) == 2);
  REQUIRE(requestCountFor(observerObjectInstance, discoveredInstancesAttribute) == 2);
  REQUIRE(observerReports.objectDiscoveryReports.size() == initialDiscoveryReports + 2U);
  REQUIRE_NOTHROW(subject->updateAttributeValues(
      secondRegisteredObject,
      AttributeHandleValueMap{
          {efficiencyAttribute,
           VariableLengthData(firstValueBytes, sizeof(firstValueBytes))}},
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(updatesSentAttribute) == 3);
  REQUIRE(requestCount(updatedInstancesAttribute) == 2);

  auto const setTiming = observer->getInteractionClassHandle(
      standard_hla::mom::set_timing);
  auto const federateParameter = observer->getParameterHandle(setTiming, standard_hla::mom::federate);
  auto const periodParameter = observer->getParameterHandle(setTiming, standard_hla::mom::report_period);
  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, subjectFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, observerFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  auto const beforePeriodic = observerReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  while (observer->evokeCallback(0.0)) {
  }
  auto const periodicReflection = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePeriodic),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == subjectObjectInstance &&
            report.attributeValues.contains(updatesSentAttribute);
      });
  REQUIRE(periodicReflection != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->ancesAttribute,
          reflectionsReceivedAttribute},
      true));

  auto const subjectFederate = subject->joinFederationExecution(
      L"mom-reflection-count-subject", L"subject", federationName);
  auto const subjectMomClass = subject->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const subjectFederateHandleAttribute = subject->getAttributeHandle(
      subjectMomClass, standard_hla::mom::federate_handle);
  REQUIRE(subjectMomClass.isValid());
  REQUIRE(subjectFederateHandleAttribute.isValid());
  REQUIRE_NOTHROW(subject->subscribeObjectClassAttributes(
      subjectMomClass,
      AttributeHandleSet{subjectFederateHandleAttribute},
      true));
  while (observer->evokeCallback(0.0)) {
  }
  while (subject->evokeCallback(0.0)) {
  }

  // The observer's MOM object is globally addressable, but its initial
  // reflection is delivered to the other joined federate. The public handle
  // remains valid for the observer's direct MOM request path.
  auto const reflectedObserver = std::find_if(
      subjectReports.attributeReflectionReports.begin(),
      subjectReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(subjectFederateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(observerFederate.encode());
      });
  REQUIRE(reflectedObserver != subjectReports.attributeReflectionReports.end());
  auto const observerObjectInstance = reflectedObserver->objectInstance;

  auto requestCount = [&](AttributeHandle attribute) {
    auto const before = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        observerObjectInstance,
        AttributeHandleSet{attribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == before + 1U);
    auto const& reflection = observerReports.attributeReflectionReports.back();
    REQUIRE(reflection.objectInstance == observerObjectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    rti1516_2025::HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(reflection.attributeValues.at(attribute)));
    return decoded.get();
  };

  // MOM-owned reflections do not feed the application-reflection counters.
  REQUIRE(requestCount(reflectedInstancesAttribute) == 0);
  REQUIRE(requestCount(reflectionsReceivedAttribute) == 0);

  auto const serverClass = subject->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiencyAttribute = subject->getAttributeHandle(
      serverClass, fixture_hla::fixture::efficiency);
  auto const observerServerClass = observer->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const observerEfficiencyAttribute = observer->getAttributeHandle(
      observerServerClass, fixture_hla::fixture::efficiency);
  REQUIRE(serverClass.isValid());
  REQUIRE(efficiencyAttribute.isValid());
  REQUIRE(observerServerClass.isValid());
  REQUIRE(observerEfficiencyAttribute.isValid());
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(
      serverClass, AttributeHandleSet{efficiencyAttribute}));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      observerServerClass, AttributeHandleSet{observerEfficiencyAttribute}, true));

  auto const firstObject = subject->registerObjectInstance(serverClass);
  REQUIRE(firstObject.isValid());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->updateAttributeValues(
      firstObject,
      AttributeHandleValueMap{{
          efficiencyAttribute,
          VariableLengthData(
              reinterpret_cast<unsigned char const*>("first"), 5U)}},
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(reflectionsReceivedAttribute) == 1);
  REQUIRE(requestCount(reflectedInstancesAttribute) == 1);

  // A second accepted reflection for the same application object increments
  // the total but not the distinct-object count.
  REQUIRE_NOTHROW(subject->updateAttributeValues(
      firstObject,
      AttributeHandleValueMap{{
          efficiencyAttribute,
          VariableLengthData(
              reinterpret_cast<unsigned char const*>("second"), 6U)}},
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(reflectionsReceivedAttribute) == 2);
  REQUIRE(requestCount(reflectedInstancesAttribute) == 1);

  auto const secondObject = subject->registerObjectInstance(serverClass);
  REQUIRE(secondObject.isValid());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->updateAttributeValues(
      secondObject,
      AttributeHandleValueMap{{
          efficiencyAttribute,
          VariableLengthData(
              reinterpret_cast<unsigned char const*>("third"), 5U)}},
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(requestCount(reflectionsReceivedAttribute) == 3);
  REQUIRE(requestCount(reflectedInstancesAttribute) == 2);

  // Exercise the queued timestamped callback boundary as well. The total
  // counter must advance once when the TSO reflection is admitted, while the
  // distinct-object counter remains unchanged for the already-seen object.
  AttributeHandleSet const efficiencyOnly{efficiencyAttribute};
  REQUIRE_NOTHROW(subject->changeAttributeOrderType(
      firstObject, efficiencyOnly, TIMESTAMP));
  REQUIRE_NOTHROW(observer->enableTimeConstrained());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (subject->evokeCallback(0.0)) {
  }
  unsigned char const timestampedValueBytes[] = {0x04};
  auto const timestampedRetraction = subject->updateAttributeValues(
   "  auto const bestEffortBytes = variableLengthDataBytes(
      observer->getTransportationTypeHandle(standard_hla::mom::best_effort).encode());
  auto const takeOrderBytes = variableLengthDataBytes(
      observerTakeOrder.encode());
  auto const regionalInteractionBytes = variableLengthDataBytes(
      observerRegionalInteraction.encode());
  std::map<std::vector<unsigned char>,
           std::map<std::vector<unsigned char>, std::int32_t>> decodedReports;
  for (auto const& report : observerReports.interactionReports) {
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 2U);
    REQUIRE(report.parameterValues.contains(reportTransportationParameter));
    REQUIRE(report.parameterValues.contains(reportCountsParameter));
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType == observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet> interactionClassPrototype;
    rti1516_2025::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(interactionClassPrototype)
        .appendElement(rti1516_2025::HLAinteger32BE{});
    rti1516_2025::HLAvariableArray counts{recordPrototype};
    REQUIRE_NOTHROW(counts.decode(report.parameterValues.at(reportCountsParameter)));
    REQUIRE(counts.size() > 0U);
    for (std::size_t index = 0; index < counts.size(); ++index) {
      auto const& record = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(counts.get(index));
      auto const& count = dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(1U));
      decodedReports[variableLengthDataBytes(
          report.parameterValues.at(reportTransportationParameter))][
          variableLengthDataBytes(record.get(0U).encode())] = count.get();
    }
  }
  REQUIRE(decodedReports.size() == 2U);
  REQUIRE(decodedReports.at(reliableBytes).at(takeOrderBytes) == 1);
  REQUIRE(decodedReports.at(reliableBytes).at(regionalInteractionBytes) == 1);
  REQUIRE(decodedReports.at(bestEffortBytes).at(takeOrderBytes) == 1);

  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

#endif
#if 0
#endif
#if 0
TEST_CASE(
    "Embedded MOM federation content reports FOM module and MIM data",
    "[integration][development-profile][federation-management][mom][mom-reserverTakeOrder.encode());
  std::map<std::vector<unsigned char>, std::size_t> decodedCounts;
  for (auto const& report : observerReports.interactionReports) {
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 2U);
    REQUIRE(report.parameterValues.contains(reportTransportationParameter));
    REQUIRE(report.parameterValues.contains(reportCountsParameter));
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType == observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet> interactionClassPrototype;
    rti1516_2025::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(interactionClassPrototype)
        .appendElement(rti1516_2025::HLAinteger32BE{});
    rti1516_2025::HLAvariableArray counts{recordPrototype};
    REQUIRE_NOTHROW(counts.decode(report.parameterValues.at(reportCountsParameter)));
    auto const transportationBytes = variableLengthDataBytes(
        report.parameterValues.at(reportTransportationParameter));
    decodedCounts[transportationBytes] = counts.size();
    if (counts.size() == 1U) {
      auto const& record = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(counts.get(0U));
      auto const& count = dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(1U));
      REQUIRE(variableLengthDataBytes(record.get(0U).encode()) == takeOrderBytes);
      REQUIRE(count.get() == 1);
    }
  }
  REQUIRE(decodedCounts.size() == 2U);
  REQUIRE(decodedCounts.at(reliableBytes) == 1U);
  REQUIRE(decodedCounts.at(bestEffortBytes) == 0U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded joined-federate MOM exposes interaction send counts",
    "[integration][development-profile][federation-management][mom][periodic-mom]"
    "[interaction-management][time-management][ddm]"
    "[rti.service.send-interaction][rti.service.send-directed-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.request-attribute-value-update][rti.service.enable-time-regulation]"
    "[federate.callback.receive-interaction][federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));

  auto const observerFederate = observer->joinFederationExecution(
      L"mom-interaction-count-observer", L"observer", federationName);

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const interactionsSentAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::interactions_sent);
  auto const directedInteractionsSentAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::directed_interactions_sent);
  auto const interactionsReceivedAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::interactions_received);
  auto const directedInteractionsReceivedAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::directed_interactions_received);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(interactionsSentAttribute.isValid());
  REQUIRE(directedInteractionsSentAttribute.isValid());
  REQUIRE(interactionsReceivedAttribute.isValid());
  REQUIRE(directedInteractionsReceivedAttribute.isValid());
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{
          federateHandleAttribute,
          interactionsSentAttribute,
          directedInteractionsSentAttribute,
          interactionsReceivedAttribute,
          directedInteractionsReceivedAttribute},
      true));

  auto const subjectFederate = subject->joinFederationExecution(
      L"mom-interaction-count-subject", L"subject", federationName);
  auto const subjectMomClass = subject->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const subjectFederateHandleAttribute = subject->getAttributeHandle(
      subjectMomClass, standard_hla::mom::federate_handle);
  auto const subjectInteractionsReceivedAttribute = subject->getAttributeHandle(
      subjectMomClass, standard_hla::mom::interactions_received);
  auto const subjectDirectedInteractionsReceivedAttribute = subject->getAttributeHandle(
      subjectMomClass, standard_hla::mom::directed_interactions_received);
  REQUIRE(subjectMomClass.isValid());
  REQUIRE(subjectFederateHandleAttribute.isValid());
  REQUIRE(subjectInteractionsReceivedAttribute.isValid());
  REQUIRE(subjectDirectedInteractionsReceivedAttribute.isValid());
  REQUIRE_NOTHROW(subject->subscribeObjectClassAttributes(
      subjectMomClass,
      AttributeHandleSet{
          subjectFederateHandleAttribute,
          subjectInteractionsReceivedAttribute,
          subjectDirectedInteractionsReceivedAttribute},
      true));
  while (observer->evokeCallback(0.0)) {
  }
  while (subject->evokeCallback(0.0)) {
  }
  auto const reflectedSubject = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeRede objectInstance;
  REQUIRE_NOTHROW(objectInstance = producer->registerObjectInstance(objectClass));
  while (consumer->evokeCallback(0.0)) {
  }
  REQUIRE(consumerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(consumerReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(consumer->enableTimeConstrained());
  static_cast<void>(consumer->evokeCallback(0.0));
  REQUIRE(consumerReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  static_cast<void>(producer->evokeCallback(0.0));
  REQUIRE(producerReports.timeRegulationEnabledReports.size() == 1U);

  auto const timestamp = rti1516_2025::HLAinteger64Time(2);
  auto const interactionRetraction = producer->sendInteraction(
      interaction,
      ParameterHandleValueMap{},
      VariableLengthData{},
      timestamp);
  REQUIRE(interactionRetraction.isValid());
  rti1516_2025::HLAunicodeString value{L"timestamped-custom-transportation-value"};
  AttributeHandleValueMap values{{attribute, value.encode()}};
  auto const attributeRetraction = producer->updateAttributeValues(
      objectInstance,
      values,
      VariableLengthData{},
      timestamp);
  REQUIRE(attributeRetraction.isValid());

  REQUIRE_NOTHROW(consumer->timeAdvanceRequest(timestamp));
  REQUIRE_NOTHROW(producer->timeAdvanceRequest(timestamp));
  while (producer->evokeCallback(0.0)) {
  }
  while (consumer->evokeCallback(0.0)) {
  }

  REQUIRE(consumerReports.timestampedInteractionReports.size() == 1U);
  auto const& interactionReport = consumerReports.timestampedInteractionReports.front();
  REQUIRE(interactionReport.interactionClass == consumerInteraction);
  REQUIRE(interactionReport.transportationType == custom);
  REQUIRE(interactionReport.timeValue == L"2");
  REQUIRE(interactionReport.sentOrderType == TIMESTAMP);
  REQUIRE(interactionReport.receivedOrderType == TIMESTAMP);

  REQUIRE(consumerReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = consumerReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.transportationType == custom);
  REQUIRE(reflection.timeValue == L"2");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(consumerAttribute));

  REQUIRE_NOTHROW(consumer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}

// The preceding disabled fragment is damaged; keep it out of this focused,
// independently reconstructed interaction regression.
#endif


#if 0
TEST_CASE(
    "Embedded directed delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[fom][transportation-type-lookup][transportation][directed]"
    "[interaction-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance]"
    "[rti.service.send-directed-interaction][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto const objectConsumer = (testData / "directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider =
      (testData / "transportation-directed-interaction-provider-fom.xml").wstring();
  auto const transportationProvider =
      (testData / "transportation-reference-provider-fom.xml").wstring();
  std::vector<std::wstring> const fomModules{
      objectConsumer,
      interactionProvider,
      transportationProvider,
  };

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(consumer->connect(consumerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      producer->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle producerHandle;
  REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
      L"custom-directed-producer", L"producer", federationName));
  FederateHandle consumerHandle;
  REQUIRE_NOTHROW(consumerHandle = consumer->joinFederationExecution(
      L"custom-directed-consumer", L"consumer", federationName));

  auto const custom = producer->getTransportationTypeHandle(fixture_hla::fixture::umbra_transportation_fixture);
  auto const objectClass = producer->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = producer->getAttributeHandle(objectClass, fixture_hla::fixture::directed_target_marker);
  autn  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      egional timestamped attribute update cancels mixed retained regular owner confirmation after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[cancel-pending-ownership-acquisitions][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[negotiated-willing-to-acquire-continuation][willing-to-acquire]"
    "[timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-state]"
    "[tso-regional-attribute-update-timed-negotiated-continuation-state]"
    "[tso-regional-attribute-update-timed-negotiated-mixed-candidate-state]"
    "[tso-regional-attribute-update-timed-negotiated-mixed-confirmation-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-state]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]" ) {
  runTimedMultiRecipientRegionalResignationAfterRestore(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
      true,
      true,
      true,
      true,
      false,
      false,
      true);
}

#endif















TEST_CASE(
    "Embedded passive interaction subscriptions do not arrange ordinary or regional delivery",
    "[integration][development-profile][interaction-management][ddm][passive-subscription]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x50, 0x41, 0x53};
  unsigned char const tagBytes[] = {0x53, 0x55, 0x42};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap parameterValues;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"passive-subscription-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"passive-subscription-subscriber", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  // An ordinary passive subscription is retained as declaration state but is
  // not eligible for a Receive Interaction callback. Replacing it with an
  // active subscription makes the next send eligible.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass, false));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.empty());

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass, true));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(interactionClass));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  // The same eligibility rule applies to a regional pair: the region remains
  // subscribed and in use while passive, but its overlap cannot arrange
  // delivery until the pair is made active.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion},
      false));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion},
      true));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation-list services dispatch standards reports in both callback models",
    "[integration][development-profile][federation-management][callbacks]"
    "[rti.service.list-federation-executions][rti.service.list-federation-execution-members]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador disconnectedReports;
  TestFederateAmbassador creatorFederate;
  TestFederateAmbassador memberFederate;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto disconnected = makeRti();
  auto creator = makeRti();
  auto member = makeRti();
  auto const firstFederationName = nextFederationName();
  auto const secondFederationName = nextFederationName();
  auto const missingFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(evoked->listFederationExecutions(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->listFederationExecutionMembers(firstFederationName),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(disconnected->connect(disconnectedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(creator->connect(creatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(firstFederationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(secondFederationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(member->joinFederationExecution(L"listed-member", L"observer", firstFederationName));

  REQUIRE_NOTHROW(evoked->listFederationExecutions());
  REQUIRE(evokedReports.federationExecutionReports.empty());
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionReports.size() == 1);
  auto const& listedFederations = evokedReports.federationExecutionReports.front();
  REQUIRE(listedFederations.size() == 2);
  auto const first = std::find_if(
      listedFederations.begin(),
      listedFederations.end(),
      [&firstFederationName](auto const& federation) {
        return federation.federationExecutionName == firstFederationName;
      });
  REQUIRE(first != listedFederations.end());
  REQUIRE(first->logicalTimeImplementationName == standard_hla::mom::integer64_time);

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(firstFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionMemberReports.size() == 1);
  auto const& memberReport = evokedReports.federationExecutionMemberReports.front();
  REQUIRE(memberReport.federationName == firstFederationName);
  REQUIRE(memberReport.members.size() == 1);
  REQUIRE(memberReport.members.front().federateName == L"listed-member");
  REQUIRE(memberReport.members.front().federateType == L"observer");

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(missingFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.missingFederationReports == std::vector<std::wstring>{missingFederationName});

  REQUIRE_NOTHROW(immediate->listFederationExecutions());
  REQUIRE(immediateReports.federationExecutionReports.size() == 1);
  REQUIRE(immediateReports.federationExecutionReports.front().size() == 2);

  // Disconnect must discard a report that was queued against the prior
  // callback session; a later Evoke cannot dereference that stale recipient.
  REQUIRE_NOTHROW(disconnected->listFederationExecutions());
  REQUIRE_NOTHROW(disconnected->disconnect());
  REQUIRE_FALSE(disconnected->evokeCallback(0.0));
  REQUIRE(disconnectedReports.federationExecutionReports.empty());

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(firstFederationName));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(secondFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}





TEST_CASE(
    "Embedded service reporting preserves Cancel Attribute Ownership Acquisition arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting]"
    "[cancel-attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                             "cpp" / "tests" / "data" /
                             "attribute-update-passel-fom.xml")
                                .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"cancellation-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"cancellation-report-requester",
      L"publisher",
      federationName));
  // This support FOM enables both reporting switches by default. Keep setup
  // outside the one-record assertion, then select file reporting only for the
  // accepted Section 7.15 cancellation below.
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  unsigned char const acquisitionTagBytes[] = {0xC0U, 0xDEU, 0x25U};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, attributes, acquisitionTag));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  // A rejected cancellation does not reserve a serial or append a
  // successful-void record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(unknownObject, attributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(objectInstance, attributes));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectInstanceValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(attribute.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CancelAttributeOwnershipAcquisition","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The accepted §7.15 cancellation is reported before its separately queued
  // confirmation callback.  Its supplied one-element set remains Table 5's
  // type-1 AttributeHandleSet form.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.empty());

  // The cancellation invalidates the pending owner-side acquisition work,
  // then the requester receives the confirmation after the record is durable.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  auto const& confirmation =
      requesterReports.attributeOwnershipAcquisitionCancellationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == attributes);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Attribute Ownership Acquisition arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting]"
    "[attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                             "cpp" / "tests" / "data" /
                             "attribute-update-passel-fom.xml")
                                .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"acquisition-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"acquisition-report-requester",
      L"publisher",
      federationName));
  // This support FOM enables both reporting switches by default. Keep setup
  // outside the one-record assertion, then select file reporting only for the
  // accepted Section 7.8 request below.
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  // A rejected Section 7.8 invocation has no successful-void service record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(unknownObject, attributes, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(objectInstance, attributes, tag));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectInstanceValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(attribute.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"AttributeOwnershipAcquisition","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"AP8QpQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The accepted §7.8 request is reported before its separately queued owner
  // release callback. Table 5's literal type-63 tag remains a file-text-only
  // assertion pending the companion MIM discrepancy's resolution (RL-077).
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());

  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1U);
  auto const& release = ownerReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(release.objectInstance == objectInstance);
  REQUIRE(release.attributes == attributes);
  REQUIRE(variableLengthDataBytes(release.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Attribute Ownership Acquisition If Available arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting]"
    "[attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                             "cpp" / "tests" / "data" /
                             "attribute-update-passel-fom.xml")
                                .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"if-available-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"if-available-report-requester",
      L"publisher",
      federationName));
  // This support FOM enables both reporting switches by default. Keep setup
  // outside the one-record assertion, then select file reporting only for the
  // accepted Section 7.9 request below.
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const ownerAttribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const requestedAttribute = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(requestedAttribute.isValid());
  AttributeHandleSet const requestedAttributes{requestedAttribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child,
      AttributeHandleSet{ownerAttribute, requestedAttribute}));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, {ownerAttribute}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, requestedAttributes));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  // A rejected Section 7.9 invocation has no successful-void service record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          unknownObject,
          requestedAttributes,
          tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      requestedAttributes,
      tag));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectInstanceValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(requestedAttribute.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"AttributeOwnershipAcquisitionIfAvailable","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"AP8QpQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The accepted §7.9 request is reported before its separately queued
  // acquisition notification. The Table 5 literal tag type remains confined
  // to this private file-text assertion (RL-077).
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& notification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == requestedAttributes);
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Attribute Ownership Release Denied arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting]"
    "[attribute-ownership-release-denied]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-release-denied]"
    "[federate.callback.attribute-ownership-unavailable]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                             "cpp" / "tests" / "data" /
                             "attribute-update-passel-fom.xml")
                                .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const acquisitionTagBytes[] = {0x01U, 0x02U, 0x03U};
  unsigned char const denialTagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"release-denied-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"release-denied-report-requester",
      L"publisher",
      federationName));
  // This support FOM enables both reporting switches by default. Keep setup
  // outside the one-record assertion, then select file reporting only for the
  // accepted Section 7.12 denial below.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      acquisitionTag));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1U);

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  // A rejected Section 7.12 invocation has no successful-void service record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(unknownObject, attributes, denialTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      attributes,
      denialTag));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectInstanceValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(attribute.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"AttributeOwnershipReleaseDenied","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators for which the joined federate is unwilling to divest ownership","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"AP8QpQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The accepted §7.12 denial is reported before its separately queued
  // Attribute Ownership Unavailable callback. Its long source-defined
  // attribute-set name is preserved verbatim in the Table 5 record.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& unavailable = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(unavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(unavailable.objectInstance == objectInstance);
  REQUIRE(unavailable.attributes == attributes);
  REQUIRE(variableLengthDataBytes(unavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Confirm Divestiture arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting][confirm-divestiture]"
    "[confirm-divestiture-mixed-set-atomicity][2025]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                             "cpp" / "tests" / "data" /
                             "attribute-update-passel-fom.xml")
                                .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const acquisitionTagBytes[] = {0x21U, 0x22U, 0x23U};
  unsigned char const divestitureTagBytes[] = {0x31U, 0x32U, 0x33U};
  unsigned char const confirmationTagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"confirm-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"confirm-report-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unrequestedAttribute = owner->getAttributeHandle(child, L"ReliableBaseB");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(unrequestedAttribute.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleSet const publishedAttributes{attribute, unrequestedAttribute};
  AttributeHandleSet const mixedConfirmationAttributes{
      attribute,
      unrequestedAttribute,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, publishedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      attributes,
      divestitureTag));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(unknownObject, attributes, confirmationTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  // Confirming one valid pending attribute together with a second attribute
  // that was never included in negotiated divestiture must reject the set as
  // a whole; neither ownership nor the successful-void report may partially
  // change.
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(
          objectInstance,
          mixedConfirmationAttributes,
          confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, attribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, attribute));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, unrequestedAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
      objectInstance,
      unrequestedAttribute));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->confirmDivestiture(objectInstance, attributes, confirmationTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, attribute));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, attribute));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ConfirmDivestiture","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      asAscii(objectInstance.toString()) +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      asAscii(attribute.toString()) +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"AP8QpQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& notification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == attributes);
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(confirmationTagBytes,
                                     confirmationTagBytes + sizeof(confirmationTagBytes)));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Negotiated Attribute Ownership Divestiture arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting]"
    "[negotiated-attribute-ownership-divestiture]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-divestiture-confirmation]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                             "cpp" / "tests" / "data" /
                             "attribute-update-passel-fom.xml")
                                .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const acquisitionTagBytes[] = {0x01U, 0x02U, 0x03U};
  unsigned char const divestitureTagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"negotiated-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"negotiated-report-requester",
      L"publisher",
      federationName));
  // The support FOM initially selects both reporting switches. Keep setup
  // outside the exact one-record assertion, then enable file reporting only
  // for the accepted Section 7.3 invocation below.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));
  // The pending regular request supplies the tag later delivered with the
  // confirmation callback; it is distinct from the Section 7.3 report tag.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      acquisitionTag));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  // A rejected Section 7.3 invocation has no successful-void service record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(unknownObject, attributes, divestitureTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      attributes,
      divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, attribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, attribute));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectInstanceValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(attribute.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"NegotiatedAttributeOwnershipDivestiture","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"AP8QpQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // §7.3's accepted request is recorded before its separately queued
  // Request Divestiture Confirmation callback begins.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());

  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  auto const& confirmation = ownerReports.divestitureConfirmationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == attributes);
  REQUIRE(variableLengthDataBytes(confirmation.userSuppliedTag) ==
          std::vector<unsigned char>(acquisitionTagBytes,
                                     acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  // Restore and reject the pending regular request with reporting disabled, so
  // teardown does not affect the one-record assertion above.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      attributes));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1U);
  VariableLengthData cleanupTag;
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      attributes,
      cleanupTag));
  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, attributes));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded federation restore restores one queued timestamped attribute update to multiple recipients",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][tso][timestamped-attribute-update]"
    "[mixed-fanout][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.update-attribute-values]"
    "[rti.service.register-object-instance][rti.service.subscribe-object-class-attributes]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" / "tests" / "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x4D, 0x55, 0x4C, 0x54, 0x49};
  unsigned char const tagBytes[] = {0x4D, 0x45, 0x4D, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"multi-recipient-tso-attribute-baseline";
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {}
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"multi-recipient-tso-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"multi-recipient-tso-attribute-first", L"subscriber", federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"multi-recipient-tso-attribute-second", L"subscriber", federationName));
  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(firstReceiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(secondReceiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(firstReceiver->enableTimeConstrained());
  drain(*firstReceiver);
  REQUIRE_NOTHROW(secondReceiver->enableTimeConstrained());
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*publisher);
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance, attributeValues, tag, rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(firstReceiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(secondReceiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);
  REQUIRE_NOTHROW(publisher->retract(retraction));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE_THROWS_AS(publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);
  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  auto const verifyReflection = [&](ReportingFederateAmbassador const& reports) {
    REQUIRE(reports.attributeReflectionReports.size() == 1U);
    REQUIRE(reports.flushQueueGrantReports.size() == 1U);
    auto const& report = reports.attributeReflectionReports.front();
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == 1U);
    REQUIRE(report.attributeValues.contains(attribute));
    REQUIRE(variableLengthDataBytes(report.attributeValues.at(attribute)) ==
            std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  REQUIRE_NOTHROW(firstReceiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  verifyReflection(firstReceiverReports);
  verifyReflection(secondReceiverReports);
  REQUIRE(firstReceiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});
  REQUIRE(secondReceiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(secondReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(firstReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(secondReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE_THROWS_AS(publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(firstReceiver->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(secondReceiver->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
TEST_CASE(
    "Embedded service reporting delivers receive-order Send Interaction through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management]"
    "[mom][service-reporting][service-report-interaction][send-interaction-service-report]"
    "[receive-order-send-interaction-service-report-interaction]"
    "[rti.service.send-interaction][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  unsigned char const tagBytes[] = {'r', 'o', 'i'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const identifierBytes[] = {0x01, 0x02};
  VariableLengthData const identifierValue(identifierBytes, sizeof(identifierBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  // An immediate observer makes the MOM report visible at the accepted-send
  // boundary while the ordinary recipient remains HLA_EVOKED.
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"receive-order-mom-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"receive-order-mom-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"receive-order-mom-observer", L"observer", federationName));

  // Keep join/setup calls out of the single accepted Send Interaction report.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  // The publisher's switch controls its own reports; the observer's switch
  // stays disabled because it is only selecting the public MOM interaction.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());
  ParameterHandleValueMap const values{{identifier, identifierValue}};
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, values, tag));

  // HLA_IMMEDIATE has already received the report, while the ordinary
  // receive-order callback is still queued for the HLA_EVOKED recipient.
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.interactionReports.empty());
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(decodedService.get() == L"SendInteraction");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 2);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 4U);
  auto const verifyArgument = [&](std::size_t argumentIndex,
                                  std::int32_t type,
                                  std::wstring const& name,
                                  std::wstring const& value) {
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(argumentIndex));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == value);
  };
  verifyArgument(
      0U,
      27,
      L"Interaction class designator",
      L"\"" + interactionClass.toString() + L"\"");
  verifyArgument(
      1U,
      40,
      L"Constrained set of interaction parameter designator and value pairs",
      L"{\"" + identifier.toString() + L"\":\"AQI=\"}");
  verifyArgument(2U, 60, L"User-supplied tag", L"\"cm9p\"");
  verifyArgument(3U, 34, L"Optional timestamp", L"null");

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(
      returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
          34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
          L"null");
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.interactionReports.size() == 1U);
  auto const& interaction = receiverReports.interactionReports.front();
  REQUIRE(interaction.interactionClass == interactionClass);
  REQUIRE(interaction.parameterValues.size() == 1U);
  REQUIRE(interaction.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(identifier)) ==
          std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
  REQUIRE(variableLengthDataBytes(interaction.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(interaction.transportationType ==
          receiver->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(interaction.producingFederate == publisherHandle);
  REQUIRE_FALSE(interaction.sentRegionsSupplied);
  REQUIRE(interaction.sentRegions.empty());

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting records timestamped Send Interaction before interaction callback",
    "[integration][development-profile][federation-management][interaction-management]"
    "[time-management][tso][mom][service-report-file][service-reporting]"
    "[timestamped-interaction][timestamped-send-interaction-service-report]"
    "[rti.service.send-interaction][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const identifierBytes[] = {0x01, 0x02};
  VariableLengthData const identifierValue(identifierBytes, sizeof(identifierBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-send-file-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-send-file-receiver", L"receiver", federationName));

  // Keep setup calls out of the accepted timestamped service-report record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  ParameterHandleValueMap const values{{identifier, identifierValue}};
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      values,
      tag,
      timestamp);
  REQUIRE_FALSE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto asAscii = [](std::wstring const& text) {
    std::string result;
    result.reserve(text.size());
    for (wchar_t const character : text) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const interactionValue = asAscii(interactionClass.toString());
  auto const parameterValue = asAscii(identifier.toString());
  auto const timestampValue = asAscii(timestamp.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":34,"HLAargumentName":"","HLAargumentValue":null}],"HLAservice":"SendInteraction","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionValue +
      R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":{")" +
      parameterValue +
      R"(":"AQI="}},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":")" +
      timestampValue + R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};

  // The accepted type-2 record is durable before HLA_EVOKED enters the
  // recipient's timestamped Receive Interaction callback.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(serviceReportFiles(directory.path()) == files);

  // Switches gate future appends but never replace the joined federate's
  // report file or alter its existing record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE(serviceReportFiles(directory.path()) == files);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(serviceReportFiles(directory.path()) == files);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  bool reportPresentAtCallbackEntry = false;
  receiverReports.onTimestampedInteraction = [&] {
    reportPresentAtCallbackEntry =
        readTextFile(reportFile) == initialText + expectedRecord;
  };
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& interaction = receiverReports.timestampedInteractionReports.front();
  REQUIRE(interaction.interactionClass == interactionClass);
  REQUIRE(interaction.parameterValues.size() == 1U);
  REQUIRE(interaction.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(identifier)) ==
          std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
  REQUIRE(variableLengthDataBytes(interaction.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(interaction.transportationType ==
          receiver->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(interaction.producingFederate == publisherHandle);
  REQUIRE_FALSE(interaction.sentRegionsSupplied);
  REQUIRE(interaction.sentRegions.empty());
  REQUIRE(interaction.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(interaction.timeValue == timestamp.toString());
  REQUIRE(interaction.sentOrderType == RECEIVE);
  REQUIRE(interaction.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(interaction.retractionSupplied);
  REQUIRE_FALSE(interaction.retractionValid);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers timestamped Send Interaction through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management]"
    "[time-management][tso][mom][service-reporting][service-report-interaction]"
    "[timestamped-send-interaction-service-report-interaction]"
    "[rti.service.timestamped-send-interaction-service-report-interaction]"
    "[rti.service.send-interaction][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const identifierBytes[] = {0x01, 0x02};
  VariableLengthData const identifierValue(identifierBytes, sizeof(identifierBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-send-mom-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-send-mom-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-send-mom-observer", L"observer", federationName));

  // Keep setup calls out of the accepted timestamped service-report record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());
  ParameterHandleValueMap const values{{identifier, identifierValue}};
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      values,
      tag,
      timestamp);
  REQUIRE_FALSE(retraction.isValid());
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(decodedService.get() == L"SendInteraction");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 2);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 4U);
  auto const verifyArgument = [&](std::size_t argumentIndex,
                                  std::int32_t type,
                                  std::wstring const& name,
                                  std::wstring const& value) {
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(argumentIndex));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == value);
  };
  verifyArgument(
      0U,
      27,
      L"Interaction class designator",
      L"\"" + interactionClass.toString() + L"\"");
  verifyArgument(
      1U,
      40,
      L"Constrained set of interaction parameter designator and value pairs",
      L"{\"" + identifier.toString() + L"\":\"AQI=\"}");
  verifyArgument(2U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(3U, 31, L"Optional timestamp", L"\"" + timestamp.toString() + L"\"");

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(
      returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
          34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
          L"null");
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& interaction = receiverReports.timestampedInteractionReports.front();
  REQUIRE(interaction.interactionClass == interactionClass);
  REQUIRE(interaction.parameterValues.size() == 1U);
  REQUIRE(interaction.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(identifier)) ==
          std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
  REQUIRE(variableLengthDataBytes(interaction.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(interaction.transportationType ==
          receiver->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(interaction.producingFederate == publisherHandle);
  REQUIRE_FALSE(interaction.sentRegionsSupplied);
  REQUIRE(interaction.sentRegions.empty());
  REQUIRE(interaction.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(interaction.timeValue == timestamp.toString());
  REQUIRE(interaction.sentOrderType == RECEIVE);
  REQUIRE(interaction.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(interaction.retractionSupplied);
  REQUIRE_FALSE(interaction.retractionValid);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
