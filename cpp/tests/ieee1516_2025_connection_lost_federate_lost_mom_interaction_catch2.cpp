#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss reports HLAreportFederateLost to subscribed survivors",
    "[integration][development-profile][federation-management][transport][mom]"
    "[connection-lost-federate-lost-mom-interaction]"
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

} // namespace
