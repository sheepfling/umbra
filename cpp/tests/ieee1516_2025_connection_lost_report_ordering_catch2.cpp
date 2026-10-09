#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

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

} // namespace
