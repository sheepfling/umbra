#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers failed timestamped Send Directed Interaction invocations through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management][directed][time-management]"
    "[mom][service-reporting][service-report-interaction][service-failure][tso]"
    "[timestamped-directed-interaction-failure][timestamped-directed-interaction-failure-mom-route]"
    "[rti.service.timestamped-directed-interaction-failure-matrix-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                               "data" / "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                                    "data" / "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto publisherConfiguration = configurationForServiceReportDirectory(directory.path());
  auto observerConfiguration = configurationForServiceReportDirectory(directory.path());
  publisherConfiguration.withRtiAddress(L"in-process");
  observerConfiguration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const parameterBytes[] = {0x01};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, publisherConfiguration));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE, observerConfiguration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider, switchFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-directed-mom-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-directed-mom-observer", L"observer", federationName));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

  auto const reportClass = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           L"HLAservice",
           L"HLAserviceType",
           L"HLAsuccessIndicator",
           L"HLAsuppliedArguments",
           L"HLAreturnedArgument",
           L"HLAexception",
           L"HLAserialNumber"}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(observer->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (observer->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(observerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  ParameterHandleValueMap const values;
  ParameterHandleValueMap const invalidValues{{ParameterHandle{}, parameterValue}};
  auto const interactionValue = interactionClass.toString();
  auto const invalidInteractionValue = InteractionClassHandle{}.toString();
  auto const targetValue = target.toString();
  auto const invalidTargetValue = ObjectInstanceHandle{}.toString();
  auto const mapValue = L"{}";
  auto const invalidMapValue = L"{\"" + ParameterHandle{}.toString() + L"\":\"AQ==\"}";
  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& expectedInteraction,
                                std::wstring const& expectedTarget,
                                std::wstring const& expectedMap,
                                std::wstring const& expectedTimestamp,
                                std::wstring const& expectedException) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == L"SendDirectedInteraction");
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 2);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE_FALSE(decodedSuccess.get());
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
                                    std::wstring const& valueText) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == valueText);
    };
    verifyArgument(0U, 27, L"Interaction class designator", L"\"" + expectedInteraction + L"\"");
    verifyArgument(1U, 37, L"Object instance designator", L"\"" + expectedTarget + L"\"");
    verifyArgument(2U, 40, L"Constrained set of interaction parameter designator and value pairs", expectedMap);
    verifyArgument(3U, 60, L"User-supplied tag", L"\"dHNv\"");
    verifyArgument(4U, 31, L"Optional timestamp", L"\"" + expectedTimestamp + L"\"");
    rti1516_2025::HLAfixedRecord nullReturned;
    nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() == L"null");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get() == expectedException);
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };
  auto const invalidInteractionException =
      L"InteractionClassNotDefined: Timestamped Send Directed Interaction requires a defined InteractionClassHandle.";
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          InteractionClassHandle{}, target, values, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionClassNotDefined);
  verifyReport(0U, invalidInteractionValue, targetValue, mapValue, L"6", invalidInteractionException);
  auto const invalidTargetException =
      L"ObjectInstanceNotKnown: Timestamped Send Directed Interaction requires a valid target ObjectInstanceHandle.";
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass, ObjectInstanceHandle{}, values, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::ObjectInstanceNotKnown);
  verifyReport(1U, interactionValue, invalidTargetValue, mapValue, L"6", invalidTargetException);
  auto const invalidParameterException =
      L"InteractionParameterNotDefined: Timestamped Send Directed Interaction requires defined ParameterHandle values.";
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass, target, invalidValues, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionParameterNotDefined);
  verifyReport(2U, interactionValue, targetValue, invalidMapValue, L"6", invalidParameterException);
  auto const invalidTimeException =
      L"InvalidLogicalTime: A timestamped service is earlier than the sender's current logical time plus lookahead.";
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass, target, values, tag, rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  verifyReport(3U, interactionValue, targetValue, mapValue, L"4", invalidTimeException);
  REQUIRE(observerReports.interactionReports.size() == 4U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          InteractionClassHandle{}, target, values, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(observerReports.interactionReports.size() == 4U);

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass,
          ObjectInstanceHandle{},
          values,
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::ObjectInstanceNotKnown);
  verifyReport(4U, interactionValue, invalidTargetValue, mapValue, L"6", invalidTargetException);
  REQUIRE(observerReports.interactionReports.size() == 5U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}  // namespace
