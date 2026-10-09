#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded federation-management services require an RTI connection",
    "[integration][federation-management][connection]") {
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->destroyFederationExecution(federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->joinFederationExecution(L"observer", federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->resignFederationExecution(NO_ACTION),
      rti1516_2025::NotConnected);
}

TEST_CASE(
    "Embedded getTimeFactory returns the joined federation's selected time factory",
    "[integration][development-profile][federation-management][rti.service.get-time-factory]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"time-client", L"observer", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == standard_hla::mom::integer64_time);
  auto initial = factory->makeInitial();
  auto* integerInitial = dynamic_cast<rti1516_2025::HLAinteger64Time*>(initial.get());
  REQUIRE(integerInitial != nullptr);
  REQUIRE(integerInitial->isInitial());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federate lookup services preserve departed designator identities within the joined federation",
    "[integration][development-profile][federation-management][rti.service.get-federate-handle]"
    "[rti.service.get-federate-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador peerFederate;
  TestFederateAmbassador foreignCreatorFederate;
  TestFederateAmbassador foreignMemberFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto foreignCreator = makeRti();
  auto foreignMember = makeRti();
  auto const federationName = nextFederationName();
  auto const foreignFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  FederateHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getFederateName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getFederateName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignCreator->connect(foreignCreatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignMember->connect(foreignMemberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(foreignCreator->createFederationExecution(
      foreignFederationName,
      fomModule,
      standard_hla::mom::integer64_time));

  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  FederateHandle foreignHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"lookup-peer", L"observer", federationName));
  REQUIRE_NOTHROW(foreignHandle = foreignMember->joinFederationExecution(
      L"lookup-foreign",
      L"observer",
      foreignFederationName));

  REQUIRE(owner->getFederateHandle(L"lookup-owner") == ownerHandle);
  REQUIRE(owner->getFederateHandle(L"lookup-peer") == peerHandle);
  REQUIRE(peer->getFederateName(ownerHandle) == L"lookup-owner");
  REQUIRE(peer->getFederateName(peerHandle) == L"lookup-peer");
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"missing"), rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(owner->getFederateName(invalid), rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->getFederateName(foreignHandle),
      rti1516_2025::FederateHandleNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"lookup-peer"), rti1516_2025::NameNotFound);
  // Get Federate Handle applies only to joined names.  The handle returned by
  // Join remains a valid federate designator after resignation, however, so
  // Get Federate Name must retain its immutable identity for the execution.
  REQUIRE(owner->getFederateName(peerHandle) == L"lookup-peer");

  REQUIRE_NOTHROW(foreignMember->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(foreignCreator->destroyFederationExecution(foreignFederationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(foreignCreator->disconnect());
  REQUIRE_NOTHROW(foreignMember->disconnect());
}

TEST_CASE(
    "Embedded public handle decoders enforce lifecycle and preserve encoded identities",
    "[integration][development-profile][federation-management][support-services][handles]"
    "[rti.service.decode-federate-handle][rti.service.decode-object-class-handle]"
    "[rti.service.decode-interaction-class-handle][rti.service.decode-object-instance-handle]"
    "[rti.service.decode-attribute-handle][rti.service.decode-parameter-handle]"
    "[rti.service.decode-dimension-handle][rti.service.decode-message-retraction-handle]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const encodedRetractionBytes[] = {
      0x00, 0x00, 0x00, 0x08, 0x01, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  };
  VariableLengthData const encodedRetraction(
      encodedRetractionBytes, sizeof(encodedRetractionBytes));

  REQUIRE_THROWS_AS(
      unjoined->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle federate;
  REQUIRE_NOTHROW(federate = owner->joinFederationExecution(
      L"handle-decoder-owner", L"owner", federationName));

  auto const objectClass = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const attribute = owner->getAttributeHandle(objectClass, fixture_hla::fixture::flavor);
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = owner->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const dimension = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(federate.isValid());
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass, AttributeHandleSet{attribute}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());

  REQUIRE(owner->decodeFederateHandle(federate.encode()) == federate);
  REQUIRE(owner->decodeObjectClassHandle(objectClass.encode()) == objectClass);
  REQUIRE(owner->decodeInteractionClassHandle(interactionClass.encode()) == interactionClass);
  REQUIRE(owner->decodeObjectInstanceHandle(objectInstance.encode()) == objectInstance);
  REQUIRE(owner->decodeAttributeHandle(attribute.encode()) == attribute);
  REQUIRE(owner->decodeParameterHandle(parameter.encode()) == parameter);
  REQUIRE(owner->decodeDimensionHandle(dimension.encode()) == dimension);
  auto const decodedRetraction = owner->decodeMessageRetractionHandle(encodedRetraction);
  REQUIRE(decodedRetraction.isValid());
  REQUIRE(decodedRetraction.toString() == L"MessageRetractionHandle(72623859790382856)");

  REQUIRE_THROWS_AS(
      owner->decodeFederateHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeObjectClassHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeInteractionClassHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeObjectInstanceHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeAttributeHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeParameterHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeDimensionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      owner->decodeMessageRetractionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records object-instance name reservation arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting]"
    "[object-instance-name-reservation-service-report]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.release-object-instance-name]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"object-name-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // A selected report file exists for the whole joined-federate lifetime, but
  // both switches gate later appends.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const gatedName = std::wstring{L"Umbra.GatedReservation"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(gatedName));
  REQUIRE(readTextFile(reportFile) == initialText);
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  auto const activeName = std::wstring{L"Umbra.ReportedReservation"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(activeName));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const activeNameValue = asAscii(activeName);
  auto const reserveRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ReserveObjectInstanceName","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Name","HLAargumentValue":")" +
      activeNameValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord);

  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(activeName));
  auto const releaseRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ReleaseObjectInstanceName","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Name","HLAargumentValue":")" +
      activeNameValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord + releaseRecord);

  // The dedicated object-name failure matrix owns failed-service records;
  // keep this argument regression focused on accepted forms.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(activeName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord + releaseRecord);

  // Releasing the earlier gated reservation still produces no record.
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(gatedName));
  REQUIRE(readTextFile(reportFile) == initialText + reserveRecord + releaseRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records failed object-instance name reservations",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[object-name-reservation-failure][service-report-object-name-reservation-file-failures]"
    "[rti.service.object-name-reservation-failure-matrix]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"object-name-failure-subject", L"owner", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const quote = [](std::string const& value) {
    return std::string{"\""} + value + "\"";
  };
  auto const nameArgument = [&](std::wstring const& name) {
    return std::string{"{\"HLAargumentType\":53,\"HLAargumentName\":\"Name\",\"HLAargumentValue\":"} +
        quote(asAscii(name)) + "}";
  };
  auto const reportRecord = [&quote](std::uint32_t serial,
                                     std::string const& service,
                                     std::string const& argument,
                                     bool success,
                                     std::string const& exception) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        ",\"HLAreturnedArgument\":[null],\"HLAservice\":\"" + service +
        "\",\"HLAsuppliedArguments\":[" + argument +
        "],\"HLAsuccessIndicator\":" + (success ? "true" : "false") +
        ",\"HLAexception\":" +
        (exception.empty() ? std::string{"null"} : quote(exception)) + "}";
  };
  auto const emptyName = std::wstring{};
  auto const validName = std::wstring{L"Umbra.ObjectNameFailure"};

  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(emptyName),
      rti1516_2025::IllegalName);
  auto const failedReserve = reportRecord(
      0U,
      "ReserveObjectInstanceName",
      nameArgument(emptyName),
      false,
      "IllegalName: Reserve Object Instance Name received an illegal object instance name.");
  REQUIRE(readTextFile(reportFile) == initialText + failedReserve);

  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(validName));
  auto const successfulReserve = reportRecord(
      1U,
      "ReserveObjectInstanceName",
      nameArgument(validName),
      true,
      {});
  REQUIRE(readTextFile(reportFile) == initialText + failedReserve + successfulReserve);

  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(L"Umbra.NotReserved"),
      rti1516_2025::ObjectInstanceNameNotReserved);
  auto const failedRelease = reportRecord(
      2U,
      "ReleaseObjectInstanceName",
      nameArgument(L"Umbra.NotReserved"),
      false,
      "ObjectInstanceNameNotReserved: Release Object Instance Name requires a name reserved by this federate.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedReserve + successfulReserve + failedRelease);

  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(validName));
  auto const successfulRelease = reportRecord(
      3U,
      "ReleaseObjectInstanceName",
      nameArgument(validName),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedReserve + successfulReserve + failedRelease + successfulRelease);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers failed object-instance name reservations through MOM interaction",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-reporting][service-report-interaction][service-failure]"
    "[object-name-reservation-failure][service-report-object-name-reservation-mom-failures]"
    "[rti.service.object-name-reservation-failure-matrix-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"object-name-failure-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"object-name-failure-observer", L"observer", federationName));

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

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::wstring const& value,
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
    REQUIRE(decodedServiceType.get() == 2);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get() == success);
    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 1U);
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(0U));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == 53);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == fixture_hla::fixture::name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
            L"\"" + value + L"\"");
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
    REQUIRE(decodedException.get() == exception);
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  REQUIRE_THROWS_AS(subject->reserveObjectInstanceName(L""), rti1516_2025::IllegalName);
  verifyReport(
      0U,
      L"ReserveObjectInstanceName",
      L"",
      false,
      L"IllegalName: Reserve Object Instance Name received an illegal object instance name.");
  auto const validName = std::wstring{L"Umbra.ObjectNameFailure"};
  REQUIRE_NOTHROW(subject->reserveObjectInstanceName(validName));
  verifyReport(1U, L"ReserveObjectInstanceName", validName, true, L"");
  REQUIRE_THROWS_AS(
      subject->releaseObjectInstanceName(L"Umbra.NotReserved"),
      rti1516_2025::ObjectInstanceNameNotReserved);
  verifyReport(
      2U,
      L"ReleaseObjectInstanceName",
      L"Umbra.NotReserved",
      false,
      L"ObjectInstanceNameNotReserved: Release Object Instance Name requires a name reserved by this federate.");
  REQUIRE_NOTHROW(subject->releaseObjectInstanceName(validName));
  verifyReport(3U, L"ReleaseObjectInstanceName", validName, true, L"");
  REQUIRE(observerReports.interactionReports.size() == 4U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting records multiple object-instance name reservation arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting]"
    "[object-instance-name-reservation-service-report]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-multiple-object-instance-names]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"multiple-object-name-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };

  std::set<std::wstring> const gatedNames{
      L"Umbra.GatedMultipleA", L"Umbra.GatedMultipleB"};
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames(gatedNames));
  REQUIRE(readTextFile(reportFile) == initialText);
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames(gatedNames));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  std::set<std::wstring> const emptyNames;
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames(emptyNames),
      rti1516_2025::NameSetWasEmpty);
  auto const emptyReserveFailure = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ReserveMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name Set","HLAargumentValue":[]}],"HLAsuccessIndicator":false,"HLAexception":"NameSetWasEmpty: Reserve Multiple Object Instance Names requires a non-empty object instance name set."})"};
  REQUIRE(readTextFile(reportFile) == initialText + emptyReserveFailure);

  std::set<std::wstring> const activeNames{
      L"Umbra.MultipleReservationA", L"Umbra.MultipleReservationB"};
  REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames(activeNames));
  auto const activeNameA = asAscii(L"Umbra.MultipleReservationA");
  auto const activeNameB = asAscii(L"Umbra.MultipleReservationB");
  auto const reserveRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ReserveMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name Set","HLAargumentValue":[")" +
      activeNameA +
      R"(",")" + activeNameB +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + emptyReserveFailure + reserveRecord);
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));

  auto const invalidRelease = std::set<std::wstring>{
      L"Umbra.MultipleReservationA", L"Umbra.NeverReserved"};
  REQUIRE_THROWS_AS(
      owner->releaseMultipleObjectInstanceNames(invalidRelease),
      rti1516_2025::ObjectInstanceNameNotReserved);
  auto const invalidReleaseRecord = std::string{
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"ReleaseMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name set","HLAargumentValue":["Umbra.MultipleReservationA","Umbra.NeverReserved"]}],"HLAsuccessIndicator":false,"HLAexception":"ObjectInstanceNameNotReserved: Release Multiple Object Instance Names requires a name reserved by this federate."})"};
  REQUIRE(readTextFile(reportFile) ==
          initialText + emptyReserveFailure + reserveRecord + invalidReleaseRecord);

  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames(activeNames));
  auto const releaseRecord = std::string{
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"ReleaseMultipleObjectInstanceNames","HLAsuppliedArguments":[{"HLAargumentType":54,"HLAargumentName":"Name set","HLAargumentValue":[")" +
      activeNameA +
      R"(",")" + activeNameB +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) ==
          initialText + emptyReserveFailure + reserveRecord + invalidReleaseRecord +
              releaseRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers multiple object-instance name release through MOM",
    "[integration][development-profile][federation-management][object-management][mom]"
    "[service-reporting][service-report-interaction][object-instance-name-reservation-service-report]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-multiple-object-instance-names]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"multiple-object-name-mom-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"multiple-object-name-mom-observer", L"observer", federationName));

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

  // Keep the setup reservation out of the interaction assertion. The
  // accepted release is the only selected public MOM service in this case.
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  std::set<std::wstring> const activeNames{
      L"Umbra.MultipleMomA", L"Umbra.MultipleMomB"};
  REQUIRE_NOTHROW(subject->reserveMultipleObjectInstanceNames(activeNames));
  static_cast<void>(subject->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subject->releaseMultipleObjectInstanceNames(activeNames));

  REQUIRE(observerReports.interactionReports.size() == 1U);
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
  REQUIRE(decodedService.get() == L"ReleaseMultipleObjectInstanceNames");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 2);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord suppliedPrototype;
  suppliedPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{suppliedPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 1U);
  auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(0U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == 54);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() ==
          L"Name set");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
          L"[\"Umbra.MultipleMomA\",\"Umbra.MultipleMomB\"]");

  rti1516_2025::HLAfixedRecord nullReturned;
  nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() ==
          L"null");
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers federate and object-class lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[rti.service.get-federate-handle][rti.service.get-federate-name]"
    "[rti.service.get-object-class-handle][rti.service.get-object-class-name]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  auto const subjectFederate = subject->joinFederationExecution(
      L"lookup-success-subject", L"subject", federationName);
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-observer", L"observer", federationName));

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

  // Keep setup lookups out of the observed serial stream. The subject's
  // switch is enabled only after the observer has selected the public route.
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const subjectName = std::wstring{L"lookup-success-subject"};
  auto const serverName = std::wstring{fixture_hla::fom::employee_server};
  auto const subjectHandle = subject->getFederateHandle(subjectName);
  auto const serverHandle = subject->getObjectClassHandle(serverName);
  REQUIRE(subjectHandle == subjectFederate);
  REQUIRE(serverHandle.isValid());
  REQUIRE(subject->getFederateName(subjectHandle) == subjectName);
  REQUIRE(subject->getObjectClassName(serverHandle) == serverName);

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->getFederateHandle(subjectName));
  REQUIRE_NOTHROW(subject->getFederateName(subjectHandle));
  REQUIRE_NOTHROW(subject->getObjectClassHandle(serverName));
  REQUIRE_NOTHROW(subject->getObjectClassName(serverHandle));
  REQUIRE(observerReports.interactionReports.size() == 4U);

  auto quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto decodeReport = [&](std::size_t index,
                          std::wstring const& service,
                          rti1516_2025::Integer32 suppliedType,
                          std::wstring const& suppliedName,
                          std::wstring const& suppliedValue,
                          rti1516_2025::Integer32 returnedType,
                          std::wstring const& returnedName,
                          std::wstring const& returnedValue) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 1U);
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(0U));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() ==
            suppliedType);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() ==
            suppliedName);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
            suppliedValue);

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
            returnedType);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
            returnedName);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
            returnedValue);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"GetFederateHandle",
      53,
      L"Federate name",
      quoted(subjectName),
      15,
      L"Federate handle",
      quoted(subjectHandle.toString()));
  decodeReport(
      1U,
      L"GetFederateName",
      15,
      L"Federate handle",
      quoted(subjectHandle.toString()),
      53,
      L"Federate name",
      quoted(subjectName));
  decodeReport(
      2U,
      L"GetObjectClassHandle",
      53,
      L"Object class name",
      quoted(serverName),
      36,
      L"Object class handle",
      quoted(serverHandle.toString()));
  decodeReport(
      3U,
      L"GetObjectClassName",
      36,
      L"Object class handle",
      quoted(serverHandle.toString()),
      53,
      L"Object class name",
      quoted(serverName));

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}


















TEST_CASE(
    "Embedded federation-list services dispatch standards reports in both callback models",
    "[integration][development-profile][federation-management][callbacks]"
    "[rti.service.list-federation-executions][rti.service.list-federation-execution-members]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador disconnectedReports;
  TestFederateAmbassador creatorFederate;
  TestFederateAmbassador memberFederate;
  umbra::test::ieee1516_2025::runFederationListingScenario(
      evokedReports,
      immediateReports,
      disconnectedReports,
      creatorFederate,
      memberFederate,
      makeRti,
      resourcePath,
      nextFederationName);
}
