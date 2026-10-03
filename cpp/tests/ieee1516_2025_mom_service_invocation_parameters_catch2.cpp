#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM service invocation reports include inherited HLAfederate",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][mom-service-invocation-parameters]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.set-service-reporting-switch]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.resign-federation-execution][rti.service.decode-federate-handle]"
    "[federate.callback.announce-synchronization-point][federate.callback.receive-interaction]") {
  for (auto const callbackModel : {HLA_EVOKED, rti1516_2025::HLA_IMMEDIATE}) {
    DYNAMIC_SECTION((callbackModel == HLA_EVOKED ? "HLA_EVOKED" : "HLA_IMMEDIATE")) {
      ReportingFederateAmbassador subjectReports;
      ReportingFederateAmbassador observerReports;
      auto subject = makeRti();
      auto observer = makeRti();
      auto const federationName = nextFederationName();
      auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

      REQUIRE_NOTHROW(subject->connect(subjectReports, callbackModel));
      REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel));
      REQUIRE_NOTHROW(subject->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle subjectFederate;
      REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
          L"service-invocation-subject", L"subject", federationName));
      REQUIRE(subjectFederate.isValid());
      REQUIRE_NOTHROW(observer->joinFederationExecution(
          L"service-invocation-observer", L"observer", federationName));

      auto const reportClass = observer->getInteractionClassHandle(
          standard_hla::mom::report_service_invocation);
      REQUIRE(reportClass.isValid());
      std::vector<ParameterHandle> reportParameters;
      for (auto const* name : {
               standard_hla::mom::service,
               standard_hla::mom::service_type,
               standard_hla::mom::success_indicator,
               standard_hla::mom::supplied_arguments,
               standard_hla::mom::returned_argument,
               standard_hla::mom::exception,
               standard_hla::mom::serial_number,
               standard_hla::mom::federate}) {
        auto const parameter = observer->getParameterHandle(reportClass, name);
        REQUIRE(parameter.isValid());
        reportParameters.push_back(parameter);
      }
      auto const reliable = observer->getTransportationTypeHandle(standard_hla::mom::reliable);
      REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());
      REQUIRE_FALSE(observer->getServiceReportingSwitch());
      REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
      REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));

      auto drainCallbacks = [&] {
        while (subject->evokeCallback(0.0)) {
        }
        while (observer->evokeCallback(0.0)) {
        }
      };
      auto verifyReport = [&](std::wstring const& service) {
        ReportingFederateAmbassador::InteractionReport const* matched = nullptr;
        for (auto const& candidate : observerReports.interactionReports) {
          if (candidate.interactionClass != reportClass) {
            continue;
          }
          rti1516_2025::HLAunicodeString candidateService;
          candidateService.decode(candidate.parameterValues.at(reportParameters[0]));
          if (candidateService.get() == service) {
            REQUIRE(matched == nullptr);
            matched = &candidate;
          }
        }
        REQUIRE(matched != nullptr);
        auto const& report = *matched;
        REQUIRE(report.parameterValues.size() == 8U);
        for (auto const& parameter : reportParameters) {
          REQUIRE(report.parameterValues.count(parameter) == 1U);
        }
        // HLAfederateReference is the entire official encoded handle. It is
        // already an HLAvariableArray of octets, not an opaque handle body to
        // wrap in a second variable array.
        auto const& reportedFederate = report.parameterValues.at(reportParameters[7]);
        REQUIRE(variableLengthDataBytes(reportedFederate) ==
                variableLengthDataBytes(subjectFederate.encode()));
        REQUIRE(observer->decodeFederateHandle(reportedFederate) == subjectFederate);
        REQUIRE(report.transportationType == reliable);
        REQUIRE(report.userSuppliedTag.size() == 0U);
        REQUIRE_FALSE(report.sentRegionsSupplied);
        // This recorder receives the untimed Receive Interaction overload;
        // no timestamped callback may stand in for the MIM's Receive order.
        REQUIRE(observerReports.timestampedInteractionReports.empty());
        rti1516_2025::HLAinteger16BE decodedServiceType;
        REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
        REQUIRE(decodedServiceType.get() == 0);
        rti1516_2025::HLAboolean decodedSuccess;
        REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
        REQUIRE(decodedSuccess.get());
        rti1516_2025::HLAunicodeString decodedException;
        REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
        REQUIRE(decodedException.get().empty());
        rti1516_2025::HLAinteger32BE decodedSerial;
        REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
        REQUIRE(decodedSerial.get() >= 0);
        return decodedSerial.get();
      };

      // Registration exercises the direct service emitter; its announcement
      // exercises the RTI-initiated recipient emitter for the same member.
      auto const label = std::wstring{L"inherited-service-report-federate"};
      unsigned char const tagBytes[] = {'s', 'y', 'n', 'c'};
      VariableLengthData const tag(tagBytes, sizeof(tagBytes));
      REQUIRE_NOTHROW(subject->registerFederationSynchronizationPoint(
          label, tag, FederateHandleSet{subjectFederate}));
      if (callbackModel == HLA_EVOKED) {
        REQUIRE(observerReports.interactionReports.empty());
        REQUIRE(subjectReports.synchronizationPointAnnouncementReports.empty());
      }
      drainCallbacks();
      REQUIRE(subjectReports.synchronizationPointAnnouncementReports.size() == 1U);
      auto const registrationSerial = verifyReport(L"RegisterFederationSynchronizationPoint");
      auto const announcementSerial = verifyReport(L"AnnounceSynchronizationPoint");
      REQUIRE(announcementSerial > registrationSerial);

      // The terminal report is reserved before membership removal but may be
      // delivered afterwards. It must retain the original joined identity.
      REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
      while (observer->evokeCallback(0.0)) {
      }
      auto const resignationSerial = verifyReport(L"ResignFederationExecution");
      REQUIRE(resignationSerial > announcementSerial);

      REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
      REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(observer->disconnect());
      REQUIRE_NOTHROW(subject->disconnect());
    }
  }
}
}  // namespace
