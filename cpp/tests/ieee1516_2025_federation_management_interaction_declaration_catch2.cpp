#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded interaction declaration services retain valid 2025 lifecycle and FOM boundaries",
    "[integration][development-profile][federation-management]"
    "[declaration-management][publish-interaction-class][subscribe-interaction-class]"
    "[unpublish-interaction-class]"
    "[unsubscribe-interaction-class]"
    "[rti.service.publish-interaction-class][rti.service.unpublish-interaction-class]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador publisherFederate;
  TestFederateAmbassador subscriberFederate;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  InteractionClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->publishInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->subscribeInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unpublishInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->unsubscribeInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      publisher->joinFederationExecution(L"declaration-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      subscriber->joinFederationExecution(L"declaration-subscriber", L"subscriber", federationName));

  REQUIRE_THROWS_AS(
      publisher->publishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->unpublishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->unsubscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);

  auto const takeOrder = publisher->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(takeOrder.isValid());

  // Declaration state remains independent and idempotent per federate.  The
  // dedicated Send Interaction regression exercises the same state for
  // hierarchy-aware callback routing.
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Publish Interaction Class arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[publish-interaction-class]"
    "[rti.service.publish-interaction-class]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"publish-interaction-report-subject", L"publisher", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated declaration-failure matrix owns failed-service records;
  // keep this success-argument regression focused on the accepted call.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  InteractionClassHandle const unknownInteraction;
  REQUIRE_THROWS_AS(
      rti->publishInteractionClass(unknownInteraction),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  auto const interactionClassText = interactionClass.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"PublishInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Subscribe Interaction Class arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class]") {

  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"subscribe-interaction-report-subject", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated declaration-subscription failure matrix owns failed-service
  // records; keep this argument regression focused on the accepted call.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  InteractionClassHandle const unknownInteraction;
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(unknownInteraction, false),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  // The C++ `active == false` selector maps to the service's Optional passive
  // subscription indicator == true, exercising the standards-facing inverse.
  REQUIRE_NOTHROW(rti->subscribeInteractionClass(interactionClass, false));
  auto const interactionClassText = interactionClass.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SubscribeInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
