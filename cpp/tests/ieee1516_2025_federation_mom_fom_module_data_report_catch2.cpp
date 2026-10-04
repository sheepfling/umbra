#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM requestFOMmoduleData reports retained joined module content",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[federation-management][interaction-management]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
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
  FederateHandle subjectFederate;
  REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
      L"mom-fom-module-subject",
      L"subject",
      federationName,
      std::vector<std::wstring>{fomModule}));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-fom-module-observer", L"observer", federationName));

  auto const requestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_fom_module_data_federate);
  auto const requestFederateParameter = subject->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  auto const requestIndicatorParameter = subject->getParameterHandle(
      requestClass, standard_hla::mom::fom_module_indicator);
  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_fom_module_data_federate);
  auto const reportIndicatorParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::fom_module_indicator);
  auto const reportDataParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::fom_module_data);
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederateParameter.isValid());
  REQUIRE(requestIndicatorParameter.isValid());
  REQUIRE(reportClass.isValid());
  REQUIRE(reportIndicatorParameter.isValid());
  REQUIRE(reportDataParameter.isValid());

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  while (observer->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();

  REQUIRE_NOTHROW(subject->sendInteraction(
      requestClass,
      ParameterHandleValueMap{
          {requestFederateParameter, subjectFederate.encode()},
          {requestIndicatorParameter, rti1516_2025::HLAinteger32BE{0}.encode()},
      },
      VariableLengthData{}));
  // The report is receive-order traffic and must remain behind HLA_EVOKED.
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 2U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType == observer->getTransportationTypeHandle(
      standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAinteger32BE decodedIndicator;
  REQUIRE_NOTHROW(decodedIndicator.decode(
      report.parameterValues.at(reportIndicatorParameter)));
  REQUIRE(decodedIndicator.get() == 0);
  rti1516_2025::HLAunicodeString decodedModule;
  REQUIRE_NOTHROW(decodedModule.decode(
      report.parameterValues.at(reportDataParameter)));
  REQUIRE(decodedModule.get().find(L"<objectModel") != std::wstring::npos);
  REQUIRE(decodedModule.get().find(L"Restaurant FOM Module") != std::wstring::npos);

  // An unavailable index is rejected synchronously and does not enqueue a
  // stale report from the prior successful request.
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          requestClass,
          ParameterHandleValueMap{
              {requestFederateParameter, subjectFederate.encode()},
              {requestIndicatorParameter, rti1516_2025::HLAinteger32BE{99}.encode()},
          },
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
} // namespace
