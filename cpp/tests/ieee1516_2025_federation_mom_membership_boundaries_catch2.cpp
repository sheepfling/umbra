#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
void verifyCreateDesignatorIsRetainedInFederationMom(
    bool withExplicitMim,
    bool useFileUrl = false) {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const suppliedPath = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  auto const genericPath = suppliedPath.generic_wstring();
  auto encodedUrlPath = genericPath;
  auto const restaurantName = encodedUrlPath.find(L"RestaurantFOMmodule");
  if (restaurantName != std::wstring::npos) {
    encodedUrlPath.replace(restaurantName, 1U, L"%52");
  }
  auto const suppliedDesignator = useFileUrl
      ? (encodedUrlPath.starts_with(L"/") ? L"file://" + encodedUrlPath
                                           : L"file:///" + encodedUrlPath)
      : suppliedPath.wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  if (withExplicitMim) {
    auto const mimDesignator = resourcePath("mim/HLAstandardMIM-2025.xml").wstring();
    REQUIRE_NOTHROW(rti->createFederationExecutionWithMIM(
        federationName,
        std::vector<std::wstring>{suppliedDesignator},
        mimDesignator,
        standard_hla::mom::integer64_time));
  } else {
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName,
        std::vector<std::wstring>{suppliedDesignator},
        standard_hla::mom::integer64_time));
  }
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"create-designator-observer", L"observer", federationName));

  auto const momClass = rti->getObjectClassHandle(
      standard_hla::mom::federation_object_class);
  auto const designatorAttribute = rti->getAttributeHandle(
      momClass, standard_hla::mom::fom_module_designator_list);
  REQUIRE(momClass.isValid());
  REQUIRE(designatorAttribute.isValid());

  REQUIRE_NOTHROW(rti->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{designatorAttribute},
      true));
  while (rti->evokeCallback(0.0)) {
  }
  auto const discoveredFederation = std::find_if(
      reports.objectDiscoveryReports.begin(),
      reports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectClass == momClass &&
            report.objectInstanceName == standard_hla::mom::federation;
      });
  REQUIRE(discoveredFederation != reports.objectDiscoveryReports.end());

  auto const reflectionCount = reports.attributeReflectionReports.size();
  REQUIRE_NOTHROW(rti->requestAttributeValueUpdate(
      discoveredFederation->objectInstance,
      AttributeHandleSet{designatorAttribute},
      VariableLengthData{}));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.attributeReflectionReports.size() == reflectionCount + 1U);
  auto const& reflection = reports.attributeReflectionReports.back();
  REQUIRE(reflection.objectInstance == discoveredFederation->objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  auto const designators = decodeHlaUnicodeStringList(
      reflection.attributeValues.at(designatorAttribute));
  REQUIRE(designators);
  REQUIRE(*designators == std::vector<std::wstring>{suppliedDesignator});

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation MOM retains the exact Create FOM module designator",
    "[integration][development-profile][federation-management][mom][fom]"
    "[fom-module-designator-preservation-create]"
    "[rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  verifyCreateDesignatorIsRetainedInFederationMom(false);
}

TEST_CASE(
    "Embedded federation MOM retains the exact Create With MIM FOM module designator",
    "[integration][development-profile][federation-management][mom][fom]"
    "[fom-module-designator-preservation-create-with-mim]"
    "[rti.service.create-federation-execution-with-mim]"
    "[rti.service.join-federation-execution]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  verifyCreateDesignatorIsRetainedInFederationMom(true);
}

TEST_CASE(
    "Embedded Create Federation Execution loads a file URL FOM designator",
    "[integration][development-profile][federation-management][mom][fom]"
    "[fom-module-designator-file-url]"
    "[rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  verifyCreateDesignatorIsRetainedInFederationMom(false, true);
}

TEST_CASE(
    "Embedded federation MOM reflects HLAfederatesInFederation at membership boundaries",
    "[integration][development-profile][federation-management][mom][mom-membership]"
    "[hla-federates-in-federation-membership]"
    "[rti.service.join-federation-execution][rti.service.resign-federation-execution]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));

  auto const observerFederate = observer->joinFederationExecution(
      L"mom-membership-observer", L"observer", federationName);
  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federation_object_class);
  auto const membershipAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federates_in_federation);
  auto const reliable = observer->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(momClass.isValid());
  REQUIRE(membershipAttribute.isValid());
  REQUIRE(reliable.isValid());

  // HLAfederatesInFederation is Conditional rather than an initial value.
  // Subscribe first, then use the normal MOM discovery route to establish the
  // execution-scoped object before requesting its current membership list.
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{membershipAttribute},
      true));
  while (observer->evokeCallback(0.0)) {
  }
  auto const discoveredFederation = std::find_if(
      observerReports.objectDiscoveryReports.begin(),
      observerReports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectClass == momClass &&
            report.objectInstanceName == standard_hla::mom::federation;
      });
  REQUIRE(discoveredFederation != observerReports.objectDiscoveryReports.end());
  auto const federationObjectInstance = discoveredFederation->objectInstance;

  // The execution-scoped RTI-owned MOM object follows the same public
  // known-instance support and transportation-query contract as a joined
  // federate's HLAfederate object.
  REQUIRE(observer->getKnownObjectClassHandle(federationObjectInstance) == momClass);
  REQUIRE(observer->getObjectInstanceName(federationObjectInstance) ==
          standard_hla::mom::federation);
  REQUIRE(observer->getObjectInstanceHandle(standard_hla::mom::federation) ==
          federationObjectInstance);
  auto const transportationReportCount =
      observerReports.attributeTransportationTypeReports.size();
  REQUIRE_NOTHROW(observer->queryAttributeTransportationType(
      federationObjectInstance,
      membershipAttribute));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.attributeTransportationTypeReports.size() ==
          transportationReportCount + 1U);
  auto const& transportationReport =
      observerReports.attributeTransportationTypeReports.back();
  REQUIRE(transportationReport.objectInstance == federationObjectInstance);
  REQUIRE(transportationReport.attribute == membershipAttribute);
  REQUIRE(transportationReport.transportationType == reliable);

  auto decodeMembership = [&](VariableLengthData const& encoded) {
    auto const raw = variableLengthDataBytes(encoded);
    std::vector<rti1516_2025::Octet> bytes(raw.begin(), raw.end());
    rti1516_2025::HLAinteger32BE count;
    auto index = count.decodeFrom(bytes, 0U);
    REQUIRE(count.get() >= 0);
    std::vector<FederateHandle> result;
    result.reserve(static_cast<std::size_t>(count.get()));
    for (rti1516_2025::Integer32 element = 0; element < count.get(); ++element) {
      // HLAfederateReference is HLAvariableArray<HLAbyte>; the nested
      // variable-array count and eight-byte identity are both four-octet
      // aligned in the official encoding.
      REQUIRE(index % 4U == 0U);
      REQUIRE(index + 12U <= bytes.size());
      result.push_back(observer->decodeFederateHandle(
          VariableLengthData(bytes.data() + index, 12U)));
      index += 12U;
    }
    REQUIRE(index == bytes.size());
    return result;
  };

  auto requestMembership = [&]() {
    auto const before = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        federationObjectInstance,
        AttributeHandleSet{membershipAttribute},
        VariableLengthData{}));
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.attributeReflectionReports.size() == before + 1U);
    auto const& reflection = observerReports.attributeReflectionReports.back();
    REQUIRE(reflection.objectInstance == federationObjectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    return decodeMembership(reflection.attributeValues.at(membershipAttribute));
  };

  auto initialMembership = requestMembership();
  REQUIRE(initialMembership.size() == 1U);
  REQUIRE(initialMembership.front() == observerFederate);

  auto const subjectFederate = subject->joinFederationExecution(
      L"mom-membership-subject", L"subject", federationName);
  while (observer->evokeCallback(0.0)) {
  }
  auto joinedReflections = std::find_if(
      observerReports.attributeReflectionReports.rbegin(),
      observerReports.attributeReflectionReports.rend(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& reflection) {
        return reflection.objectInstance == federationObjectInstance &&
            reflection.attributeValues.contains(membershipAttribute);
      });
  REQUIRE(joinedReflections != observerReports.attributeReflectionReports.rend());
  auto joinedMembership = decodeMembership(
      joinedReflections->attributeValues.at(membershipAttribute));
  REQUIRE(joinedMembership.size() == 2U);
  REQUIRE(joinedMembership[0] == observerFederate);
  REQUIRE(joinedMembership[1] == subjectFederate);

  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  while (observer->evokeCallback(0.0)) {
  }
  auto resignedReflections = std::find_if(
      observerReports.attributeReflectionReports.rbegin(),
      observerReports.attributeReflectionReports.rend(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& reflection) {
        return reflection.objectInstance == federationObjectInstance &&
            reflection.attributeValues.contains(membershipAttribute);
      });
  REQUIRE(resignedReflections != observerReports.attributeReflectionReports.rend());
  auto resignedMembership = decodeMembership(
      resignedReflections->attributeValues.at(membershipAttribute));
  REQUIRE(resignedMembership.size() == 1U);
  REQUIRE(resignedMembership.front() == observerFederate);

  // Direct Request Attribute Value Update remains a current-value path after
  // both lifecycle-triggered conditional reflections, so it must agree with
  // the final membership ledger rather than a stale callback snapshot.
  auto finalMembership = requestMembership();
  REQUIRE(finalMembership == resignedMembership);

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

}
