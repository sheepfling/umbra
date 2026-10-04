#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded joined-federate MOM objects use the public discovery reflection and removal route",
    "[integration][development-profile][federation-management][mom][object-management]"
    "[service-report-file][service-report-file-lifecycle][service-reporting]"
    "[transportation-management][query-attribute-transportation-type]"
    "[rti.service.query-attribute-transportation-type]"
    "[federate.callback.report-attribute-transportation-type]"
    "[fom][fom-module-management][joined-federate-mom-public-object-lifecycle]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador subjectReports;
    ReportingFederateAmbassador secondSubjectReports;
    ReportingFederateAmbassador observerReports;
    auto subject = makeRti();
    auto secondSubject = makeRti();
    auto observer = makeRti();
    auto subjectDirectory = temporaryServiceReportDirectory();
    auto secondSubjectDirectory = temporaryServiceReportDirectory();
    auto observerDirectory = temporaryServiceReportDirectory();
    auto subjectConfiguration = configurationForServiceReportDirectory(subjectDirectory.path());
    auto secondSubjectConfiguration =
        configurationForServiceReportDirectory(secondSubjectDirectory.path());
    auto observerConfiguration = configurationForServiceReportDirectory(observerDirectory.path());
    subjectConfiguration.withRtiAddress(L"in-process");
    secondSubjectConfiguration.withRtiAddress(L"in-process");
    observerConfiguration.withRtiAddress(L"in-process");
    auto const federationName = nextFederationName();
    auto const baseFom =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
         "switch-known-class-enabled-fom.xml")
            .wstring();
    auto const joinedFom =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
         "switch-nrg-disabled-fom.xml")
            .wstring();

    REQUIRE_NOTHROW(subject->connect(subjectReports, callbackModel, subjectConfiguration));
    REQUIRE_NOTHROW(
        secondSubject->connect(secondSubjectReports, callbackModel, secondSubjectConfiguration));
    REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel, observerConfiguration));
    REQUIRE_NOTHROW(
        subject->createFederationExecution(
            federationName, baseFom, standard_hla::mom::integer64_time));
    auto const subjectFederate = subject->joinFederationExecution(
        L"public-mom-subject",
        L"subject",
        federationName,
        std::vector<std::wstring>{joinedFom});
    REQUIRE_NOTHROW(observer->joinFederationExecution(
        L"public-mom-observer", L"observer", federationName));

    auto const momClass = observer->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const reportFileAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::report_service_file);
    auto const federateHandleAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_handle);
    auto const federateNameAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_name);
    auto const federateTypeAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_type);
    auto const federateHostAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_host);
    auto const rtiVersionAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::rti_version);
    auto const fomModuleListAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::fom_module_designator_list);
    auto const reliable = observer->getTransportationTypeHandle(standard_hla::mom::reliable);
    AttributeHandleSet const initialAttributes{
        federateHandleAttribute,
        federateNameAttribute,
        federateTypeAttribute,
        federateHostAttribute,
        rtiVersionAttribute,
        fomModuleListAttribute,
        reportFileAttribute,
    };
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        initialAttributes,
        true));
    while (observer->evokeCallback(0.0)) {
    }

    auto const subjectFiles = serviceReportFiles(subjectDirectory.path());
    REQUIRE(subjectFiles.size() == 1U);
    auto const encodedJoinedFom =
        umbra::detail::utf8FromWide(umbra::detail::formatMomString(joinedFom));
    REQUIRE(encodedJoinedFom);
    auto const initialRecordModuleList =
        std::string{"\"HLAFOMmoduleDesignatorList\":["} + *encodedJoinedFom + "]";
    REQUIRE(
        readTextFile(subjectFiles.front()).find(initialRecordModuleList) != std::string::npos);
    auto const expectedReportFile =
        rti1516_2025::HLAunicodeString{subjectFiles.front().wstring()}.encode();
    auto const reflectedSubject = std::find_if(
        observerReports.attributeReflectionReports.begin(),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(reportFileAttribute);
          return value != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) == variableLengthDataBytes(expectedReportFile);
        });
    REQUIRE(reflectedSubject != observerReports.attributeReflectionReports.end());
    auto const subjectObjectInstance = reflectedSubject->objectInstance;
    REQUIRE(reflectedSubject->attributeValues.size() == initialAttributes.size());
    REQUIRE(variableLengthDataBytes(reflectedSubject->attributeValues.at(
                federateHandleAttribute)) ==
            variableLengthDataBytes(subjectFederate.encode()));
    REQUIRE(variableLengthDataBytes(reflectedSubject->attributeValues.at(
                federateNameAttribute)) ==
            variableLengthDataBytes(
                rti1516_2025::HLAunicodeString{L"public-mom-subject"}.encode()));
    REQUIRE(variableLengthDataBytes(reflectedSubject->attributeValues.at(
                federateTypeAttribute)) ==
            variableLengthDataBytes(rti1516_2025::HLAunicodeString{L"subject"}.encode()));
    REQUIRE(variableLengthDataBytes(reflectedSubject->attributeValues.at(
                federateHostAttribute)) ==
            variableLengthDataBytes(rti1516_2025::HLAunicodeString{L"umbra-embedded"}.encode()));
    REQUIRE(variableLengthDataBytes(reflectedSubject->attributeValues.at(
                rtiVersionAttribute)) ==
            variableLengthDataBytes(rti1516_2025::HLAunicodeString{L"Umbra 0.1.0"}.encode()));
    auto const reflectedModuleList = decodeHlaUnicodeStringList(
        reflectedSubject->attributeValues.at(fomModuleListAttribute));
    REQUIRE(reflectedModuleList);
    REQUIRE(*reflectedModuleList == std::vector<std::wstring>{joinedFom});
    REQUIRE(reflectedSubject->transportationType == reliable);
    REQUIRE_FALSE(reflectedSubject->producingFederate.isValid());
    REQUIRE_FALSE(reflectedSubject->sentRegionsSupplied);
    REQUIRE(reflectedSubject->userSuppliedTag.size() == 0U);
    auto const discoveredSubject = std::find_if(
        observerReports.objectDiscoveryReports.begin(),
        observerReports.objectDiscoveryReports.end(),
        [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
          return report.objectInstance == subjectObjectInstance;
        });
    REQUIRE(discoveredSubject != observerReports.objectDiscoveryReports.end());
    REQUIRE(discoveredSubject->objectClass == momClass);
    REQUIRE_FALSE(discoveredSubject->objectInstanceName.empty());
    REQUIRE_FALSE(discoveredSubject->producingFederate.isValid());
    // Support lookups must use the same recipient-local known-instance
    // boundary as discovery.  RTI-owned MOM names are not in the ordinary
    // federate-created name directory, but they remain valid public object
    // designators after discovery.
    REQUIRE(observer->getKnownObjectClassHandle(subjectObjectInstance) == momClass);
    REQUIRE(observer->getObjectInstanceName(subjectObjectInstance) ==
            discoveredSubject->objectInstanceName);
    REQUIRE(observer->getObjectInstanceHandle(discoveredSubject->objectInstanceName) ==
            subjectObjectInstance);

    auto const reflectedCountBeforeRequest = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        initialAttributes,
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == reflectedCountBeforeRequest + 1U);
    auto const& requestedReflection = observerReports.attributeReflectionReports.back();
    REQUIRE(requestedReflection.objectInstance == subjectObjectInstance);
    REQUIRE(requestedReflection.attributeValues.size() == initialAttributes.size());
    REQUIRE(variableLengthDataBytes(requestedReflection.attributeValues.at(
                federateHandleAttribute)) ==
            variableLengthDataBytes(subjectFederate.encode()));
    REQUIRE(variableLengthDataBytes(requestedReflection.attributeValues.at(
                federateNameAttribute)) ==
            variableLengthDataBytes(
                rti1516_2025::HLAunicodeString{L"public-mom-subject"}.encode()));
    REQUIRE(variableLengthDataBytes(requestedReflection.attributeValues.at(
                federateTypeAttribute)) ==
            variableLengthDataBytes(rti1516_2025::HLAunicodeString{L"subject"}.encode()));
    REQUIRE(variableLengthDataBytes(requestedReflection.attributeValues.at(
                federateHostAttribute)) ==
            variableLengthDataBytes(rti1516_2025::HLAunicodeString{L"umbra-embedded"}.encode()));
    REQUIRE(variableLengthDataBytes(requestedReflection.attributeValues.at(
                rtiVersionAttribute)) ==
            variableLengthDataBytes(rti1516_2025::HLAunicodeString{L"Umbra 0.1.0"}.encode()));
    auto const requestedModuleList = decodeHlaUnicodeStringList(
        requestedReflection.attributeValues.at(fomModuleListAttribute));
    REQUIRE(requestedModuleList);
    REQUIRE(*requestedModuleList == std::vector<std::wstring>{joinedFom});
    REQUIRE(requestedReflection.transportationType == reliable);
    REQUIRE_FALSE(requestedReflection.producingFederate.isValid());
    REQUIRE(variableLengthDataBytes(requestedReflection.attributeValues.at(reportFileAttribute)) ==
            variableLengthDataBytes(expectedReportFile));

    // RTI-owned MOM objects live in the same public ObjectInstanceHandle
    // namespace as federate-created objects.  Query Attribute Transportation
    // Type must therefore resolve their MIM-declared transport through the
    // MOM known-instance ledger rather than reporting ObjectInstanceNotKnown.
    auto const transportationReportCount =
        observerReports.attributeTransportationTypeReports.size();
    REQUIRE_NOTHROW(observer->queryAttributeTransportationType(
        subjectObjectInstance,
        reportFileAttribute));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeTransportationTypeReports.size() ==
            transportationReportCount + 1U);
    auto const& transportationReport =
        observerReports.attributeTransportationTypeReports.back();
    REQUIRE(transportationReport.objectInstance == subjectObjectInstance);
    REQUIRE(transportationReport.attribute == reportFileAttribute);
    REQUIRE(transportationReport.transportationType == reliable);

    // A second joined-federate lifetime must receive a distinct RTI-owned
    // object and a distinct immutable filesystem path.  It joins after the
    // observer's subscription, so the normal public discovery/reflection
    // route—not a testing snapshot—proves the second allocation and its
    // represented-federate identity.
    auto const secondSubjectFederate = secondSubject->joinFederationExecution(
        L"public-mom-second-subject", L"subject", federationName);
    auto const secondSubjectFiles = serviceReportFiles(secondSubjectDirectory.path());
    REQUIRE(secondSubjectFiles.size() == 1U);
    REQUIRE(secondSubjectFiles.front().is_absolute());
    REQUIRE(secondSubjectFiles.front() != subjectFiles.front());
    auto const expectedSecondReportFile =
        rti1516_2025::HLAunicodeString{secondSubjectFiles.front().wstring()}.encode();
    while (observer->evokeCallback(0.0)) {
    }
    auto const reflectedSecond = std::find_if(
        observerReports.attributeReflectionReports.begin(),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(reportFileAttribute);
          auto const name = report.attributeValues.find(federateNameAttribute);
          return value != report.attributeValues.end() &&
              name != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) ==
                  variableLengthDataBytes(expectedSecondReportFile) &&
              variableLengthDataBytes(name->second) ==
                  variableLengthDataBytes(
                      rti1516_2025::HLAunicodeString{L"public-mom-second-subject"}.encode());
        });
    REQUIRE(reflectedSecond != observerReports.attributeReflectionReports.end());
    REQUIRE(reflectedSecond->objectInstance != subjectObjectInstance);
    auto const secondSubjectObjectInstance = reflectedSecond->objectInstance;
    auto const reflectedSecondModuleList = decodeHlaUnicodeStringList(
        reflectedSecond->attributeValues.at(fomModuleListAttribute));
    REQUIRE(reflectedSecondModuleList);
    REQUIRE(reflectedSecondModuleList->empty());

    auto const secondReflectionCountBeforeRequest =
        observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        secondSubjectObjectInstance,
        AttributeHandleSet{reportFileAttribute, fomModuleListAttribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(
        observerReports.attributeReflectionReports.size() ==
        secondReflectionCountBeforeRequest + 1U);
    auto const& requestedSecondReflection = observerReports.attributeReflectionReports.back();
    REQUIRE(requestedSecondReflection.objectInstance == secondSubjectObjectInstance);
    REQUIRE(variableLengthDataBytes(
                requestedSecondReflection.attributeValues.at(reportFileAttribute)) ==
            variableLengthDataBytes(expectedSecondReportFile));
    auto const requestedSecondModuleList = decodeHlaUnicodeStringList(
        requestedSecondReflection.attributeValues.at(fomModuleListAttribute));
    REQUIRE(requestedSecondModuleList);
    REQUIRE(requestedSecondModuleList->empty());

    // Switch changes on the second joined federate gate later appends only;
    // they cannot replace the path advertised through its static MOM value.
    REQUIRE_NOTHROW(secondSubject->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(secondSubject->setSendServiceReportsToFileSwitch(true));
    REQUIRE_NOTHROW(secondSubject->getObjectClassHandle(standard_hla::fom::object_root));
    REQUIRE(serviceReportFiles(secondSubjectDirectory.path()) == secondSubjectFiles);
    auto const switchedPathReflectionCount = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        secondSubjectObjectInstance,
        AttributeHandleSet{reportFileAttribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(
        observerReports.attributeReflectionReports.size() == switchedPathReflectionCount + 1U);
    auto const& switchedPathReflection = observerReports.attributeReflectionReports.back();
    REQUIRE(switchedPathReflection.objectInstance == secondSubjectObjectInstance);
    REQUIRE(variableLengthDataBytes(
                switchedPathReflection.attributeValues.at(reportFileAttribute)) ==
            variableLengthDataBytes(expectedSecondReportFile));

    REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
    while (observer->evokeCallback(0.0)) {
    }
    auto const removedSubject = std::find_if(
        observerReports.objectRemovalReports.begin(),
        observerReports.objectRemovalReports.end(),
        [&](ReportingFederateAmbassador::ObjectRemovalReport const& report) {
          return report.objectInstance == subjectObjectInstance;
        });
    REQUIRE(removedSubject != observerReports.objectRemovalReports.end());
    REQUIRE_FALSE(removedSubject->producingFederate.isValid());

    REQUIRE_NOTHROW(secondSubject->resignFederationExecution(NO_ACTION));
    while (observer->evokeCallback(0.0)) {
    }
    auto const removedSecondSubject = std::find_if(
        observerReports.objectRemovalReports.begin(),
        observerReports.objectRemovalReports.end(),
        [&](ReportingFederateAmbassador::ObjectRemovalReport const& report) {
          return report.objectInstance == secondSubjectObjectInstance;
        });
    REQUIRE(removedSecondSubject != observerReports.objectRemovalReports.end());
    REQUIRE_FALSE(removedSecondSubject->producingFederate.isValid());

    auto* const subjectAmbassador =
        dynamic_cast<rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador*>(subject.get());
    REQUIRE(subjectAmbassador != nullptr);
    REQUIRE_FALSE(subjectAmbassador->joinedFederateMomObjectSnapshotForTesting());

    REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subject->disconnect());
    REQUIRE_NOTHROW(secondSubject->disconnect());
    REQUIRE_NOTHROW(observer->disconnect());
    static_cast<void>(subjectFederate);
    static_cast<void>(secondSubjectFederate);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

}
