#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

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

}
