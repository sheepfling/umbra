#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Unsubscribe Object Class Attributes arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[unsubscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class]"
    "[rti.service.unsubscribe-object-class-attributes]") {
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
      rti->joinFederationExecution(L"unsubscribe-object-class-report-subject", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated object-attribute subscription failure matrix owns
  // failed-service records; keep this argument regression focused on accepted
  // forms.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClassAttributes(unknownObjectClass, AttributeHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  auto const objectClassText = objectClass.toString();
  std::string objectClassValue;
  objectClassValue.reserve(objectClassText.size());
  for (wchar_t const character : objectClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectClassValue.push_back(static_cast<char>(character));
  }
  auto const attributeText = attribute.toString();
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }

  // §5.9 accepts an attribute-set removal even when no ordinary subscription
  // currently exists.  That idempotent transition lets this regression isolate
  // the supplied non-null optional attribute-set representation.
  REQUIRE_NOTHROW(rti->unsubscribeObjectClassAttributes(objectClass, attributes));
  auto const subsetRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeObjectClassAttributes","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Optional set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subsetRecord);

  // The official C++ whole-class overload supplies no attribute set.  The
  // argument position remains present in the report with Null rather than an
  // empty AttributeHandleSet, which has different §5.9 semantics.
  REQUIRE_NOTHROW(rti->unsubscribeObjectClass(objectClass));
  auto const wholeClassRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeObjectClassAttributes","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":34,"HLAargumentName":"Optional set of attribute designators","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subsetRecord + wholeClassRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + subsetRecord + wholeClassRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "HLAservice unsubscribe accepts an omitted optional attribute list",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-interaction][optional-service-argument]"
    "[rti.service.send-interaction][rti.service.unsubscribe-object-class-attributes]") {
  ReportingFederateAmbassador publisherCallbacks;
  ReportingFederateAmbassador subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const mim = resourcePath("mim/HLAstandardMIM-2025.xml").wstring();
  std::vector<std::wstring> const fomModules{restaurantFom};

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecutionWithMIM(
      federationName, fomModules, mim, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"optional-unsubscribe-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"optional-unsubscribe-subscriber", L"subscriber", federationName));

  auto const publisherClass =
      publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const subscriberClass =
      subscriber->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const publisherAttribute =
      publisher->getAttributeHandle(publisherClass, fixture_hla::fixture::efficiency);
  auto const subscriberAttribute =
      subscriber->getAttributeHandle(subscriberClass, fixture_hla::fixture::efficiency);
  REQUIRE(publisherClass.isValid());
  REQUIRE(subscriberClass.isValid());
  REQUIRE(publisherAttribute.isValid());
  REQUIRE(subscriberAttribute.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass, AttributeHandleSet{publisherAttribute}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberClass, AttributeHandleSet{subscriberAttribute}));

  auto const object = publisher->registerObjectInstance(publisherClass);
  REQUIRE(object.isValid());
  auto drainCallbacks = [](RTIambassador& ambassador) {
    while (ambassador.evokeMultipleCallbacks(0.0, 0.0)) {
    }
  };
  drainCallbacks(*subscriber);

  auto updateAttribute = [&](std::uint8_t value) {
    VariableLengthData const encodedValue(&value, sizeof(value));
    AttributeHandleValueMap values{{publisherAttribute, encodedValue}};
    publisher->updateAttributeValues(object, values, VariableLengthData{});
    drainCallbacks(*subscriber);
  };
  updateAttribute(0x31U);
  REQUIRE(subscriberCallbacks.attributeReflectionReports.size() == 1U);

  auto const serviceInteraction = subscriber->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassAttributes");
  auto const objectClassParameter =
      subscriber->getParameterHandle(serviceInteraction, L"HLAobjectClass");
  REQUIRE(serviceInteraction.isValid());
  REQUIRE(objectClassParameter.isValid());
  ParameterHandleValueMap const parameters{
      {objectClassParameter, subscriberClass.encode()}};
  // §11.4.1 permits omission of HLAattributeList because it corresponds to
  // the optional attribute-set argument of Unsubscribe Object Class Attributes.
  REQUIRE_NOTHROW(subscriber->sendInteraction(
      serviceInteraction, parameters, VariableLengthData{}));

  updateAttribute(0x32U);
  REQUIRE(subscriberCallbacks.attributeReflectionReports.size() == 1U);

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace
