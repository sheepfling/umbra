#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
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

} // namespace
