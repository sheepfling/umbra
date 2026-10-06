#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers directed invocation reports before directed callbacks",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][directed][timestamped-directed-interaction]"
    "[directed-send-interaction-service-report-interaction]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-interaction-object-consumer-fom.xml")
          .wstring();
  auto const interactionProvider =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-interaction-interaction-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{
      objectConsumer,
      interactionProvider,
      switchFom,
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"directed-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"directed-report-observer", L"observer", federationName));

  // The support FOM enables both switches by default.  Disable the
  // publisher's file route and the observer's own reporting before selecting
  // the public report interaction; only the publisher's accepted services
  // should reserve report serials for the observer.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));

  auto const publisherObjectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const publisherMarker = publisher->getAttributeHandle(
      publisherObjectClass, fixture_hla::fixture::directed_target_marker);
  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(publisherObjectClass.isValid());
  REQUIRE(publisherMarker.isValid());
  REQUIRE(publisherInteraction.isValid());

  InteractionClassHandleSet const directedClasses{publisherInteraction};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherObjectClass, AttributeHandleSet{publisherMarker}));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      publisherObjectClass, AttributeHandleSet{publisherMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      publisherObjectClass, directedClasses));
  REQUIRE_NOTHROW(observer->subscribeObjectClassDirectedInteractions(
      publisherObjectClass, directedClasses, true));
  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(publisherObjectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(observer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(observerReports.objectDiscoveryReports.size() == 1U);

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
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));

  unsigned char const tagBytes[] = {'r', 'e', 'p'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap const parameters;
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      publisherInteraction, target, parameters, tag));
  rti1516_2025::HLAinteger64Time const timestamp(1);
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      publisherInteraction, target, parameters, tag, timestamp));
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE(observerReports.directedInteractionReports.empty());

  // Each accepted invocation queues its RTI-originated report first and its
  // directed callback second.  Evoking one slot at a time makes that ordering
  // observable without relying on a broad callback-drain helper.
  REQUIRE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(observerReports.directedInteractionReports.empty());
  REQUIRE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(observerReports.directedInteractionReports.size() == 1U);
  REQUIRE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.size() == 2U);
  REQUIRE(observerReports.directedInteractionReports.size() == 1U);
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.size() == 2U);
  REQUIRE(observerReports.directedInteractionReports.size() == 2U);

  auto const decodeReport = [&](std::size_t index,
                                std::int32_t expectedSerial,
                                bool timestamped) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType == observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == L"SendDirectedInteraction");
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
    REQUIRE_NOTHROW(suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 5U);
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
        L"\"" + publisherInteraction.toString() + L"\"");
    verifyArgument(
        1U,
        37,
        L"Object instance designator",
        L"\"" + target.toString() + L"\"");
    verifyArgument(
        2U,
        40,
        L"Constrained set of interaction parameter designator and value pairs",
        L"{}");
    verifyArgument(3U, 60, L"User-supplied tag", L"\"cmVw\"");
    verifyArgument(
        4U,
        timestamped ? 31 : 34,
        L"Optional timestamp",
        timestamped ? L"\"1\"" : L"null");

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
            L"null");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == expectedSerial);
  };
  decodeReport(0U, 0, false);
  decodeReport(1U, 1, true);

  auto const& untimestampedDirected = observerReports.directedInteractionReports.at(0U);
  REQUIRE(untimestampedDirected.interactionClass == publisherInteraction);
  REQUIRE(untimestampedDirected.objectInstance == target);
  REQUIRE(untimestampedDirected.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(untimestampedDirected.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(untimestampedDirected.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(untimestampedDirected.timeValue.empty());

  auto const& timestampedDirected = observerReports.directedInteractionReports.at(1U);
  REQUIRE(timestampedDirected.interactionClass == publisherInteraction);
  REQUIRE(timestampedDirected.objectInstance == target);
  REQUIRE(timestampedDirected.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(timestampedDirected.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(timestampedDirected.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(timestampedDirected.timeValue == L"1");
  REQUIRE(timestampedDirected.sentOrderType == rti1516_2025::RECEIVE);
  REQUIRE(timestampedDirected.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE_FALSE(timestampedDirected.retractionSupplied);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}
