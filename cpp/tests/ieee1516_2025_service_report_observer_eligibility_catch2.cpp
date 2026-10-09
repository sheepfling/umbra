#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
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
} // namespace
