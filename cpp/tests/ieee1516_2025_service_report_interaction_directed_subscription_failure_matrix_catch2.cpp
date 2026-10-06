#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers failed directed subscriptions through MOM interaction",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-reporting][service-report-interaction][service-failure]"
    "[directed-subscription-failure][rti.service.directed-subscription-failure-matrix-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
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

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"directed-subscription-failure-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"directed-subscription-failure-observer", L"observer", federationName));

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
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = subject->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const interactionClass = subject->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedArguments,
                                bool success,
                                std::wstring const& exception) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 1);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get() == success);
    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedArguments.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedArguments.size();
         ++argumentIndex) {
      auto const& actual = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(actual.get(0U)).get() ==
              expectedArguments[argumentIndex].type);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(actual.get(1U)).get() ==
              expectedArguments[argumentIndex].name);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(actual.get(2U)).get() ==
              expectedArguments[argumentIndex].value);
    }
    rti1516_2025::HLAfixedRecord nullReturned;
    nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() ==
            34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() ==
            L"null");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get() == exception);
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  auto const invalidObject = ObjectClassHandle{}.toString();
  auto const validObject = objectClass.toString();
  auto const invalidInteraction = InteractionClassHandle{}.toString();
  auto const validInteraction = interactionClass.toString();
  auto const subscribeArgs = [&](std::wstring const& objectValue,
                                 std::wstring const& setValue,
                                 bool universally) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", L"\"" + objectValue + L"\""},
        {28, L"Set of directed interaction designators", setValue},
        {6, L"Optional universal subscription indicator", universally ? L"true" : L"false"}};
  };
  auto const unsubscribeArgs = [&](std::wstring const& objectValue,
                                   std::wstring const& setValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", L"\"" + objectValue + L"\""},
        {28, L"Optional set of directed interaction designators", setValue}};
  };
  auto const wholeUnsubscribeArgs = [&](std::wstring const& objectValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", L"\"" + objectValue + L"\""},
        {34, L"Optional set of directed interaction designators", L"null"}};
  };
  auto const invalidSet = L"[\"" + invalidInteraction + L"\"]";
  auto const validSet = L"[\"" + validInteraction + L"\"]";

  REQUIRE_THROWS_AS(subject->subscribeObjectClassDirectedInteractions(
                        ObjectClassHandle{}, InteractionClassHandleSet{}, false),
                    rti1516_2025::ObjectClassNotDefined);
  verifyReport(
      0U,
      L"SubscribeObjectClassDirectedInteractions",
      subscribeArgs(invalidObject, L"[]", false),
      false,
      L"ObjectClassNotDefined: Subscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  REQUIRE_THROWS_AS(subject->subscribeObjectClassDirectedInteractions(
                        objectClass, InteractionClassHandleSet{InteractionClassHandle{}}, true),
                    rti1516_2025::InteractionClassNotDefined);
  verifyReport(
      1U,
      L"SubscribeObjectClassDirectedInteractions",
      subscribeArgs(validObject, invalidSet, true),
      false,
      L"InteractionClassNotDefined: Subscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
  REQUIRE_THROWS_AS(subject->unsubscribeObjectClassDirectedInteractions(
                        ObjectClassHandle{}, InteractionClassHandleSet{}),
                    rti1516_2025::ObjectClassNotDefined);
  verifyReport(
      2U,
      L"UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArgs(invalidObject, L"[]"),
      false,
      L"ObjectClassNotDefined: Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  REQUIRE_THROWS_AS(subject->unsubscribeObjectClassDirectedInteractions(
                        objectClass, InteractionClassHandleSet{InteractionClassHandle{}}),
                    rti1516_2025::InteractionClassNotDefined);
  verifyReport(
      3U,
      L"UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArgs(validObject, invalidSet),
      false,
      L"InteractionClassNotDefined: Unsubscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");

  InteractionClassHandleSet const interactionClasses{interactionClass};
  REQUIRE_NOTHROW(subject->subscribeObjectClassDirectedInteractions(
      objectClass, interactionClasses, false));
  verifyReport(
      4U,
      L"SubscribeObjectClassDirectedInteractions",
      subscribeArgs(validObject, validSet, false),
      true,
      L"");
  REQUIRE_NOTHROW(subject->unsubscribeObjectClassDirectedInteractions(
      objectClass, interactionClasses));
  verifyReport(
      5U,
      L"UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArgs(validObject, validSet),
      true,
      L"");
  REQUIRE_NOTHROW(subject->unsubscribeObjectClassDirectedInteractions(objectClass));
  verifyReport(
      6U,
      L"UnsubscribeObjectClassDirectedInteractions",
      wholeUnsubscribeArgs(validObject),
      true,
      L"");
  REQUIRE(observerReports.interactionReports.size() == 7U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
