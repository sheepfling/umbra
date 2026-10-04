#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers time-regulated timestamped Send Directed Interaction through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management]"
    "[directed][time-management][mom][service-reporting][service-report-interaction]"
    "[tso][timestamped-directed-interaction-time-regulated-service-report-interaction]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance]"
    "[rti.service.send-directed-interaction]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
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
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  // Keep the public MOM observer immediate so its report is visible at the
  // accepted-send boundary while the directed recipient remains constrained.
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-directed-mom-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-directed-mom-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-directed-mom-observer", L"observer", federationName));

  // Isolate the accepted timestamped directed send from setup traffic.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

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

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const targetName = publisher->getObjectInstanceName(target);
  REQUIRE(receiver->getObjectInstanceHandle(targetName) == target);

  // Enable temporal roles before service reporting so their callbacks do not
  // consume the serial reserved for the accepted directed invocation.
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  auto const retraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      timestamp);
  REQUIRE(retraction.isValid());
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
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
      L"\"" + interactionClass.toString() + L"\"");
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
  verifyArgument(3U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(
      4U,
      31,
      L"Optional timestamp",
      L"\"" + timestamp.toString() + L"\"");

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 33);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
          L"Message retraction designator");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  REQUIRE(retractionOpen != std::wstring::npos);
  REQUIRE(retractionClose > retractionOpen + 1U);
  auto const momRetraction =
      dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get();
  constexpr std::wstring_view momRetractionPrefix = L"\"MessageRetractionHandle<";
  REQUIRE(momRetraction.rfind(std::wstring{momRetractionPrefix}, 0U) == 0U);
  REQUIRE(momRetraction.size() > momRetractionPrefix.size() + 1U);
  REQUIRE(momRetraction[momRetraction.size() - 2U] == L'>');
  REQUIRE(momRetraction.back() == L'\"');
  REQUIRE(
      momRetraction.substr(
          momRetractionPrefix.size(),
          momRetraction.size() - momRetractionPrefix.size() - 2U) ==
      retractionText.substr(retractionOpen + 1U, retractionClose - retractionOpen - 1U));
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  // The public MOM report is visible before the constrained recipient's TSO
  // callback.  Advancing the receiver to six then the publisher to one makes
  // GALT reach the timestamp while preserving the callback-before-grant rule.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(timestamp));
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.directedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"directed", "grant"});

  auto const& directed = receiverReports.directedInteractionReports.front();
  REQUIRE(directed.interactionClass == interactionClass);
  REQUIRE(directed.objectInstance == target);
  REQUIRE(directed.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(directed.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(directed.transportationType ==
          receiver->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(directed.producingFederate == publisherHandle);
  REQUIRE(directed.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(directed.timeValue == timestamp.toString());
  REQUIRE(directed.sentOrderType == TIMESTAMP);
  REQUIRE(directed.receivedOrderType == TIMESTAMP);
  REQUIRE(directed.retractionSupplied);
  REQUIRE(directed.retractionValid);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace
