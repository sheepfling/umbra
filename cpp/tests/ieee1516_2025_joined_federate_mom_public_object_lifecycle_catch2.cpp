#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <exception>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::array<std::wstring_view, 42> kTable8FederatePublishedAttributeNames{
    L"HLAfederateHandle",
    L"HLAfederateName",
    L"HLAfederateType",
    L"HLAfederateHost",
    L"HLARTIversion",
    L"HLAFOMmoduleDesignatorList",
    L"HLAtimeConstrained",
    L"HLAtimeRegulating",
    L"HLAasynchronousDelivery",
    L"HLAfederateState",
    L"HLAtimeManagerState",
    L"HLAlogicalTime",
    L"HLAlookahead",
    L"HLAGALT",
    L"HLALITS",
    L"HLAROlength",
    L"HLATSOlength",
    L"HLAreflectionsReceived",
    L"HLAupdatesSent",
    L"HLAinteractionsReceived",
    L"HLAinteractionsSent",
    L"HLAdirectedInteractionsReceived",
    L"HLAdirectedInteractionsSent",
    L"HLAobjectInstancesThatCanBeDeleted",
    L"HLAobjectInstancesUpdated",
    L"HLAobjectInstancesReflected",
    L"HLAobjectInstancesDeleted",
    L"HLAobjectInstancesRemoved",
    L"HLAobjectInstancesRegistered",
    L"HLAobjectInstancesDiscovered",
    L"HLAtimeGrantedTime",
    L"HLAtimeAdvancingTime",
    L"HLAobjectClassRelevanceAdvisory",
    L"HLAattributeRelevanceAdvisory",
    L"HLAattributeScopeAdvisory",
    L"HLAinteractionRelevanceAdvisory",
    L"HLAconveyRegionDesignatorSets",
    L"HLAautomaticResignAction",
    L"HLAserviceReporting",
    L"HLAexceptionReporting",
    L"HLAsendServiceReportsToFile",
    L"HLAreportServiceFile",
};

TEST_CASE(
    "Embedded joined-federate MOM objects use the public discovery reflection and removal route",
    "[integration][development-profile][federation-management][mom][object-management]"
    "[service-report-file][service-report-file-lifecycle][service-reporting]"
    "[transportation-management][query-attribute-transportation-type]"
    "[rti.service.query-attribute-transportation-type]"
    "[federate.callback.report-attribute-transportation-type]"
    "[fom][fom-module-management][joined-federate-mom-public-object-lifecycle]"
    "[mom-table8-p-attribute-publication][mom-no-metadata-callbacks]"
    "[mom-ownership-transfer-prohibition]"
    "[mom-delivery-type-immutability]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-290-l165-54-no-metadata]"
    "[requirement-candidate-content-clauses-10-support-services-page-287-l60-19]"
    "[requirement-candidate-content-clauses-10-support-services-page-287-l60-19-register]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l77-25]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l77-25-transport]") {
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
    // Table 8 designates all 42 HLAfederate attributes with P in the P/S
    // column. Subscribe to that complete set plus the inherited root
    // HLAprivilegeToDeleteObject attribute, which Table 8 does not designate
    // for the MOM leaf class. Initial discovery must expose the static P
    // values only and must not leak the inherited non-Table-8 attribute.
    AttributeHandleSet table8PublishedAttributes;
    for (std::wstring_view const attributeName :
         kTable8FederatePublishedAttributeNames) {
      table8PublishedAttributes.insert(observer->getAttributeHandle(
          momClass,
          std::wstring{attributeName}));
    }
    REQUIRE(table8PublishedAttributes.size() ==
            kTable8FederatePublishedAttributeNames.size());
    auto const inheritedPrivilegeToDeleteAttribute = observer->getAttributeHandle(
        momClass,
        L"HLAprivilegeToDeleteObject");
    REQUIRE(inheritedPrivilegeToDeleteAttribute.isValid());
    auto subscribedAttributes = table8PublishedAttributes;
    subscribedAttributes.insert(inheritedPrivilegeToDeleteAttribute);
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        subscribedAttributes,
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
    REQUIRE(reflectedSubject->attributeValues.find(
                inheritedPrivilegeToDeleteAttribute) ==
            reflectedSubject->attributeValues.end());
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
    REQUIRE(reflectedSubject->receivedOrderType == RECEIVE);
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
    REQUIRE(requestedReflection.receivedOrderType == RECEIVE);
    REQUIRE_FALSE(requestedReflection.producingFederate.isValid());
    REQUIRE(variableLengthDataBytes(requestedReflection.attributeValues.at(reportFileAttribute)) ==
            variableLengthDataBytes(expectedReportFile));

    auto const reflectedCountBeforeTable8Request =
        observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        table8PublishedAttributes,
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() ==
            reflectedCountBeforeTable8Request + 1U);
    auto const& table8RequestedReflection =
        observerReports.attributeReflectionReports.back();
    REQUIRE(table8RequestedReflection.objectInstance == subjectObjectInstance);
    REQUIRE(table8RequestedReflection.attributeValues.size() ==
            table8PublishedAttributes.size());
    for (auto const attribute : table8PublishedAttributes) {
      REQUIRE(table8RequestedReflection.attributeValues.contains(attribute));
    }
    REQUIRE(table8RequestedReflection.attributeValues.find(
                inheritedPrivilegeToDeleteAttribute) ==
            table8RequestedReflection.attributeValues.end());

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

    // Section 11.4.1 forbids the RTI from engaging in ownership transfer of
    // MOM instance attributes, but does not prescribe the response to a
    // federate's ownership request.  Exercise both acquisition forms against
    // a publicly discovered MOM attribute and assert only the prohibited
    // transfer callbacks are absent.
    REQUIRE_NOTHROW(observer->publishObjectClassAttributes(
        momClass,
        AttributeHandleSet{reportFileAttribute}));
    auto const acquisitionReportStart =
        observerReports.attributeOwnershipAcquisitionReports.size();
    auto const releaseRequestStart =
        subjectReports.attributeOwnershipReleaseRequestReports.size();
    try {
      observer->attributeOwnershipAcquisitionIfAvailable(
          subjectObjectInstance,
          AttributeHandleSet{reportFileAttribute},
          VariableLengthData{});
    } catch (rti1516_2025::Exception const&) {
      // The clause does not define which service response applies here.
    }
    while (observer->evokeCallback(0.0)) {
    }
    while (subject->evokeCallback(0.0)) {
    }
    REQUIRE(std::none_of(
        observerReports.attributeOwnershipAcquisitionReports.begin() +
            static_cast<std::ptrdiff_t>(acquisitionReportStart),
        observerReports.attributeOwnershipAcquisitionReports.end(),
        [&](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
          return report.kind ==
                  ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::
                      notification &&
              report.objectInstance == subjectObjectInstance &&
              report.attributes.contains(reportFileAttribute);
        }));
    REQUIRE(
        subjectReports.attributeOwnershipReleaseRequestReports.size() == releaseRequestStart);

    auto const negotiatedAcquisitionReportStart =
        observerReports.attributeOwnershipAcquisitionReports.size();
    auto const negotiatedReleaseRequestStart =
        subjectReports.attributeOwnershipReleaseRequestReports.size();
    try {
      observer->attributeOwnershipAcquisition(
          subjectObjectInstance,
          AttributeHandleSet{reportFileAttribute},
          VariableLengthData{});
    } catch (rti1516_2025::Exception const&) {
      // Keep the oracle on ownership-transfer effects, not an unspecified
      // rejection exception or response.
    }
    while (observer->evokeCallback(0.0)) {
    }
    while (subject->evokeCallback(0.0)) {
    }
    REQUIRE(std::none_of(
        observerReports.attributeOwnershipAcquisitionReports.begin() +
            static_cast<std::ptrdiff_t>(negotiatedAcquisitionReportStart),
        observerReports.attributeOwnershipAcquisitionReports.end(),
        [&](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
          return report.kind ==
                  ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::
                      notification &&
              report.objectInstance == subjectObjectInstance &&
              report.attributes.contains(reportFileAttribute);
        }));
    REQUIRE(subjectReports.attributeOwnershipReleaseRequestReports.size() ==
            negotiatedReleaseRequestStart);

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
    auto const switchedPathReflection = std::find_if(
        observerReports.attributeReflectionReports.begin() +
            static_cast<std::ptrdiff_t>(switchedPathReflectionCount),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == secondSubjectObjectInstance &&
              report.attributeValues.contains(reportFileAttribute);
        });
    REQUIRE(switchedPathReflection != observerReports.attributeReflectionReports.end());
    REQUIRE(switchedPathReflection->transportationType == reliable);
    REQUIRE(switchedPathReflection->receivedOrderType == RECEIVE);
    REQUIRE(variableLengthDataBytes(
                switchedPathReflection->attributeValues.at(reportFileAttribute)) ==
            variableLengthDataBytes(expectedSecondReportFile));
    auto const postSwitchTransportationReportCount =
        observerReports.attributeTransportationTypeReports.size();
    REQUIRE_NOTHROW(observer->queryAttributeTransportationType(
        secondSubjectObjectInstance,
        reportFileAttribute));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeTransportationTypeReports.size() ==
            postSwitchTransportationReportCount + 1U);
    REQUIRE(observerReports.attributeTransportationTypeReports.back().transportationType == reliable);

    for (auto const& reflection : observerReports.attributeReflectionReports) {
      if (reflection.objectInstance != subjectObjectInstance &&
          reflection.objectInstance != secondSubjectObjectInstance) {
        continue;
      }
      REQUIRE(reflection.userSuppliedTag.size() == 0U);
      REQUIRE(reflection.timeImplementationName.empty());
      REQUIRE(reflection.timeValue.empty());
    }

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
    REQUIRE(removedSubject->userSuppliedTag.size() == 0U);
    REQUIRE(removedSubject->timeImplementationName.empty());
    REQUIRE(removedSubject->timeValue.empty());

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
    REQUIRE(removedSecondSubject->userSuppliedTag.size() == 0U);
    REQUIRE(removedSecondSubject->timeImplementationName.empty());
    REQUIRE(removedSecondSubject->timeValue.empty());

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

TEST_CASE(
    "Embedded HLAfederate attribute reflection regions specify only HLAfederate",
    "[integration][development-profile][federation-management][mom]"
    "[object-management][joined-federate-mom-federate-dimension-region]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-handle-set]"
    "[rti.service.normalize-federate-handle][rti.service.get-range-bounds]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador subjectReports;
    ReportingFederateAmbassador observerReports;
    auto subject = makeRti();
    auto observer = makeRti();
    auto subjectDirectory = temporaryServiceReportDirectory();
    auto observerDirectory = temporaryServiceReportDirectory();
    auto subjectConfiguration = configurationForServiceReportDirectory(subjectDirectory.path());
    auto observerConfiguration = configurationForServiceReportDirectory(observerDirectory.path());
    subjectConfiguration.withRtiAddress(L"in-process");
    observerConfiguration.withRtiAddress(L"in-process");
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

    REQUIRE_NOTHROW(subject->connect(subjectReports, callbackModel, subjectConfiguration));
    REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel, observerConfiguration));
    REQUIRE_NOTHROW(subject->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle subjectFederate;
    REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
        L"mom-region-subject", L"subject", federationName));
    REQUIRE_NOTHROW(observer->joinFederationExecution(
        L"mom-region-observer", L"observer", federationName));

    auto const momClass = observer->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const federateHandleAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_handle);
    auto const federateDimension = observer->getDimensionHandle(L"HLAfederate");
    REQUIRE(momClass.isValid());
    REQUIRE(federateHandleAttribute.isValid());
    REQUIRE(federateDimension.isValid());
    REQUIRE(observer->getDimensionName(federateDimension) == L"HLAfederate");
    auto const normalizedSubjectHandle =
        observer->normalizeFederateHandle(subjectFederate);
    REQUIRE(normalizedSubjectHandle != std::numeric_limits<unsigned long>::max());

    AttributeHandleSet table8PublishedAttributes;
    for (std::wstring_view const attributeName :
         kTable8FederatePublishedAttributeNames) {
      table8PublishedAttributes.insert(observer->getAttributeHandle(
          momClass,
          std::wstring{attributeName}));
    }
    REQUIRE(table8PublishedAttributes.size() ==
            kTable8FederatePublishedAttributeNames.size());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass, table8PublishedAttributes, true));
    while (observer->evokeCallback(0.0)) {
    }

    auto const subjectHandleBytes = variableLengthDataBytes(subjectFederate.encode());
    auto const subjectReflection = std::find_if(
        observerReports.attributeReflectionReports.begin(),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(federateHandleAttribute);
          return value != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) == subjectHandleBytes;
        });
    REQUIRE(subjectReflection != observerReports.attributeReflectionReports.end());
    auto const subjectObject = subjectReflection->objectInstance;

    REQUIRE_NOTHROW(observer->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE(observer->getConveyRegionDesignatorSetsSwitch());
    while (observer->evokeCallback(0.0)) {
    }

    struct RegionCallbackEvidence final {
      struct Region final {
        rti1516_2025::DimensionHandleSet dimensions;
        unsigned long lowerBound = 0UL;
        unsigned long upperBound = 0UL;
      };
      bool callbackSeen = false;
      bool sentRegionsSupplied = false;
      std::vector<Region> regions;
      std::string lookupError;
    };
    RegionCallbackEvidence callbackEvidence;
    std::mutex callbackEvidenceMutex;
    std::condition_variable callbackEvidenceReady;
    observerReports.onAttributeReflection = [&] {
      auto const& report = observerReports.attributeReflectionReports.back();
      if (report.objectInstance != subjectObject ||
          report.attributeValues.size() != table8PublishedAttributes.size()) {
        return;
      }
      RegionCallbackEvidence collected;
      collected.callbackSeen = true;
      collected.sentRegionsSupplied = report.sentRegionsSupplied;
      try {
        for (auto const& region : report.sentRegions) {
          auto const dimensions = observer->getDimensionHandleSet(region);
          auto const bounds = observer->getRangeBounds(region, federateDimension);
          collected.regions.push_back({
              dimensions,
              bounds.getLowerBound(),
              bounds.getUpperBound(),
          });
        }
      } catch (std::exception const& error) {
        collected.lookupError = error.what();
      } catch (...) {
        collected.lookupError = "unknown exception while querying callback region";
      }
      {
        std::scoped_lock lock(callbackEvidenceMutex);
        callbackEvidence = std::move(collected);
      }
      callbackEvidenceReady.notify_one();
    };

    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObject,
        table8PublishedAttributes,
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    RegionCallbackEvidence completedEvidence;
    bool callbackCompleted = false;
    {
      std::unique_lock lock(callbackEvidenceMutex);
      callbackCompleted = callbackEvidenceReady.wait_for(
          lock,
          std::chrono::seconds{2},
          [&] { return callbackEvidence.callbackSeen; });
      completedEvidence = callbackEvidence;
    }
    observerReports.onAttributeReflection = {};
    REQUIRE(callbackCompleted);
    REQUIRE(completedEvidence.lookupError.empty());
    REQUIRE(completedEvidence.sentRegionsSupplied);
    REQUIRE_FALSE(completedEvidence.regions.empty());
    for (auto const& region : completedEvidence.regions) {
      REQUIRE(region.dimensions ==
              rti1516_2025::DimensionHandleSet{federateDimension});
      REQUIRE(region.lowerBound == normalizedSubjectHandle);
      REQUIRE(region.upperBound == normalizedSubjectHandle + 1UL);
    }

    REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(observer->disconnect());
    REQUIRE_NOTHROW(subject->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

}
