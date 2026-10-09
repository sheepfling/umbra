#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
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
}  // namespace
