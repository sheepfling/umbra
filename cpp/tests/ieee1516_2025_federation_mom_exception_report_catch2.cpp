#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM rejects malformed interactions through HLAreportMOMexception",
    "[integration][development-profile][federation-management][mom][mom-request-report][mom-exception]"
    "[interaction-management][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]") {
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
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"mom-exception-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-exception-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mom_exception);
  auto const federateParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::federate);
  auto const serviceParameter = observer->getParameterHandle(reportClass, standard_hla::mom::service);
  auto const exceptionParameter = observer->getParameterHandle(reportClass, standard_hla::mom::exception);
  auto const parameterErrorParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::parameter_error);
  auto const setSwitchesClass = subject->getInteractionClassHandle(
      standard_hla::mom::set_switches_federate);
  auto const serviceReportingParameter = subject->getParameterHandle(
      setSwitchesClass, standard_hla::mom::service_reporting);
  auto const objectInstancesUpdatedRequestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_object_instances_updated);
  auto const serviceInvocationClass = subject->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(serviceParameter.isValid());
  REQUIRE(exceptionParameter.isValid());
  REQUIRE(parameterErrorParameter.isValid());
  REQUIRE(setSwitchesClass.isValid());
  REQUIRE(serviceReportingParameter.isValid());
  REQUIRE(objectInstancesUpdatedRequestClass.isValid());
  REQUIRE(serviceInvocationClass.isValid());

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  while (observer->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();

  // HLAsetSwitches is a MOM interaction with an explicit at-least-one-
  // parameter rule. The malformed request still reaches the caller as the
  // standard C++ exception, while the RTI emits HLAreportMOMexception with
  // HLAfederate, HLAservice, HLAexception, and HLAparameterError.
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.parameterValues.contains(federateParameter));
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType == observer->getTransportationTypeHandle(
      standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(serviceParameter)));
  REQUIRE(decodedService.get() ==
          standard_hla::mom::set_switches_federate);
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(exceptionParameter)));
  REQUIRE(decodedException.get().find(L"InteractionParameterNotDefined") !=
          std::wstring::npos);
  rti1516_2025::HLAboolean decodedParameterError;
  REQUIRE_NOTHROW(
      decodedParameterError.decode(report.parameterValues.at(parameterErrorParameter)));
  REQUIRE(decodedParameterError.get());

  // HLArequestObjectInstancesUpdated is also a Subscribe-only MOM
  // interaction. Supplying a parameter that belongs to HLAsetSwitches is
  // globally well-formed but not defined for this request class; the caller
  // receives the typed exception and eligible subscribers receive the same
  // distinct HLAreportMOMexception projection.
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          objectInstancesUpdatedRequestClass,
          ParameterHandleValueMap{{
              serviceReportingParameter,
              rti1516_2025::HLAboolean{true}.encode(),
          }},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& malformedObjectRequestReport = observerReports.interactionReports.front();
  rti1516_2025::HLAunicodeString malformedObjectRequestService;
  REQUIRE_NOTHROW(malformedObjectRequestService.decode(
      malformedObjectRequestReport.parameterValues.at(serviceParameter)));
  REQUIRE(malformedObjectRequestService.get() ==
          standard_hla::mom::request_object_instances_updated);
  rti1516_2025::HLAunicodeString malformedObjectRequestException;
  REQUIRE_NOTHROW(malformedObjectRequestException.decode(
      malformedObjectRequestReport.parameterValues.at(exceptionParameter)));
  REQUIRE(malformedObjectRequestException.get().find(
              L"InteractionParameterNotDefined") != std::wstring::npos);
  rti1516_2025::HLAboolean malformedObjectRequestParameterError;
  REQUIRE_NOTHROW(malformedObjectRequestParameterError.decode(
      malformedObjectRequestReport.parameterValues.at(parameterErrorParameter)));
  REQUIRE(malformedObjectRequestParameterError.get());

  // A well-formed MOM adjustment can still fail a service precondition. In
  // that case HLAparameterError is false while the caller retains the RTI
  // exception and the observer receives the same distinct MOM report class.
  REQUIRE_NOTHROW(subject->subscribeInteractionClass(serviceInvocationClass));
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{{
              serviceReportingParameter,
              rti1516_2025::HLAboolean{true}.encode(),
          }},
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& preconditionReport = observerReports.interactionReports.front();
  rti1516_2025::HLAunicodeString preconditionException;
  REQUIRE_NOTHROW(preconditionException.decode(
      preconditionReport.parameterValues.at(exceptionParameter)));
  REQUIRE(preconditionException.get().find(L"RTIinternalError") !=
          std::wstring::npos);
  rti1516_2025::HLAboolean preconditionParameterError;
  REQUIRE_NOTHROW(preconditionParameterError.decode(
      preconditionReport.parameterValues.at(parameterErrorParameter)));
  REQUIRE_FALSE(preconditionParameterError.get());
  REQUIRE_NOTHROW(subject->unsubscribeInteractionClass(serviceInvocationClass));

  // Timestamped MOM invocations must use the same HLAreportMOMexception
  // projection as receive-order invocations.  The timestamp is valid for the
  // subject's integer64 time state, while HLAsetSwitches remains a
  // Subscribe-only MOM interaction and therefore fails the normal publication
  // precondition during timestamped planning.
  REQUIRE_NOTHROW(subject->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (subject->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{},
          VariableLengthData{},
          rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::InteractionClassNotPublished);
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& timestampedMomExceptionReport = observerReports.interactionReports.front();
  REQUIRE(timestampedMomExceptionReport.interactionClass == reportClass);
  rti1516_2025::HLAunicodeString timestampedMomService;
  REQUIRE_NOTHROW(timestampedMomService.decode(
      timestampedMomExceptionReport.parameterValues.at(serviceParameter)));
  REQUIRE(timestampedMomService.get() == standard_hla::mom::set_switches_federate);
  rti1516_2025::HLAunicodeString timestampedMomException;
  REQUIRE_NOTHROW(timestampedMomException.decode(
      timestampedMomExceptionReport.parameterValues.at(exceptionParameter)));
  REQUIRE(timestampedMomException.get().find(L"InteractionClassNotPublished") !=
          std::wstring::npos);
  rti1516_2025::HLAboolean timestampedMomParameterError;
  REQUIRE_NOTHROW(timestampedMomParameterError.decode(
      timestampedMomExceptionReport.parameterValues.at(parameterErrorParameter)));
  REQUIRE_FALSE(timestampedMomParameterError.get());

  // The queued report is subject to callback-time subscription revalidation;
  // withdrawing the report subscription before Evoke suppresses delivery.
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
}
