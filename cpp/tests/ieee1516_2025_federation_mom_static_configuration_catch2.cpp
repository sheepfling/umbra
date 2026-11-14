#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <array>
#include <string_view>

namespace {
constexpr std::array<std::wstring_view, 16> kTable8FederationPublishedAttributeNames{
    L"HLAfederationName",
    L"HLAfederatesInFederation",
    L"HLARTIversion",
    L"HLAMIMdesignator",
    L"HLAFOMmoduleDesignatorList",
    L"HLAcurrentFDD",
    L"HLAtimeImplementationName",
    L"HLAlastSaveName",
    L"HLAlastSaveTime",
    L"HLAnextSaveName",
    L"HLAnextSaveTime",
    L"HLAautoProvide",
    L"HLAadvisoriesUseKnownClass",
    L"HLAdelaySubscriptionEvaluation",
    L"HLAnonRegulatedGrant",
    L"HLAallowRelaxedDDM",
};

TEST_CASE(
    "Embedded federation MOM exposes static configuration values",
    "[integration][development-profile][federation-management][mom][mom-static]"
    "[federation-mom-static-configuration]"
    "[rti.service.join-federation-execution][rti.service.subscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[mom-table8-p-attribute-publication]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador observerReports;
  auto owner = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const knownClassFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-known-class-enabled-fom.xml")
          .wstring();
  auto const nonRegulatedGrantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-enabled-fom.xml")
          .wstring();
  auto const relaxedDdmFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "allow-relaxed-ddm-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{
      restaurantFom,
      knownClassFom,
      nonRegulatedGrantFom,
      relaxedDdmFom,
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mom-static-owner", L"owner", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-static-observer", L"observer", federationName));

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federation_object_class);
  auto attribute = [&](wchar_t const* name) {
    return observer->getAttributeHandle(momClass, name);
  };
  auto const federationNameAttribute = attribute(L"HLAfederationName");
  auto const rtiVersionAttribute = attribute(L"HLARTIversion");
  auto const mimDesignatorAttribute = attribute(L"HLAMIMdesignator");
  auto const timeImplementationAttribute = attribute(L"HLAtimeImplementationName");
  auto const knownClassAttribute = attribute(L"HLAadvisoriesUseKnownClass");
  auto const delayedSubscriptionAttribute = attribute(L"HLAdelaySubscriptionEvaluation");
  auto const nonRegulatedGrantAttribute = attribute(L"HLAnonRegulatedGrant");
  auto const relaxedDdmAttribute = attribute(L"HLAallowRelaxedDDM");
  AttributeHandleSet const staticAttributes{
      federationNameAttribute,
      rtiVersionAttribute,
      mimDesignatorAttribute,
      timeImplementationAttribute,
      knownClassAttribute,
      delayedSubscriptionAttribute,
      nonRegulatedGrantAttribute,
      relaxedDdmAttribute,
  };
  // Table 8 designates all 16 HLAfederation attributes with P in the P/S
  // column. Subscribe to that complete set plus the inherited root
  // HLAprivilegeToDeleteObject attribute, which Table 8 does not designate
  // for this MOM leaf class. The initial reflection must contain exactly the
  // static P attributes, with no inherited non-Table-8 value.
  AttributeHandleSet table8PublishedAttributes;
  for (std::wstring_view const attributeName :
       kTable8FederationPublishedAttributeNames) {
    table8PublishedAttributes.insert(observer->getAttributeHandle(
        momClass,
        std::wstring{attributeName}));
  }
  REQUIRE(table8PublishedAttributes.size() ==
          kTable8FederationPublishedAttributeNames.size());
  auto const inheritedPrivilegeToDeleteAttribute = observer->getAttributeHandle(
      momClass,
      L"HLAprivilegeToDeleteObject");
  REQUIRE(inheritedPrivilegeToDeleteAttribute.isValid());
  auto subscribedAttributes = table8PublishedAttributes;
  subscribedAttributes.insert(inheritedPrivilegeToDeleteAttribute);
  REQUIRE(momClass.isValid());
  REQUIRE(staticAttributes.size() == 8U);
  for (auto const handle : staticAttributes) {
    REQUIRE(handle.isValid());
  }

  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass, subscribedAttributes, true));
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
  auto const reflectedFederation = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == federationObjectInstance &&
            report.attributeValues.size() == staticAttributes.size();
      });
  REQUIRE(reflectedFederation != observerReports.attributeReflectionReports.end());
  REQUIRE(reflectedFederation->attributeValues.find(
              inheritedPrivilegeToDeleteAttribute) ==
          reflectedFederation->attributeValues.end());
  REQUIRE(reflectedFederation->transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(reflectedFederation->producingFederate.isValid());

  auto decodeUnicode = [&](AttributeHandle const handle) {
    rti1516_2025::HLAunicodeString decoded;
    REQUIRE_NOTHROW(decoded.decode(reflectedFederation->attributeValues.at(handle)));
    return decoded.get();
  };
  auto decodeSwitch = [&](AttributeHandle const handle) {
    rti1516_2025::HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(reflectedFederation->attributeValues.at(handle)));
    return decoded.get();
  };
  REQUIRE(decodeUnicode(federationNameAttribute) == federationName);
  REQUIRE(decodeUnicode(rtiVersionAttribute) == L"Umbra 0.1.0");
  REQUIRE(decodeUnicode(mimDesignatorAttribute) == L"HLAstandardMIM");
  REQUIRE(decodeUnicode(timeImplementationAttribute) ==
          standard_hla::mom::integer64_time);
  REQUIRE(decodeSwitch(knownClassAttribute) == 1);
  REQUIRE(decodeSwitch(delayedSubscriptionAttribute) == 0);
  REQUIRE(decodeSwitch(nonRegulatedGrantAttribute) == 1);
  REQUIRE(decodeSwitch(relaxedDdmAttribute) == 1);

  auto const reflectedCountBeforeTable8Request =
      observerReports.attributeReflectionReports.size();
  REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
      federationObjectInstance,
      table8PublishedAttributes,
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.attributeReflectionReports.size() ==
          reflectedCountBeforeTable8Request + 1U);
  auto const& table8RequestedReflection =
      observerReports.attributeReflectionReports.back();
  REQUIRE(table8RequestedReflection.objectInstance == federationObjectInstance);
  REQUIRE(table8RequestedReflection.attributeValues.size() ==
          table8PublishedAttributes.size());
  for (auto const attribute : table8PublishedAttributes) {
    REQUIRE(table8RequestedReflection.attributeValues.contains(attribute));
  }
  REQUIRE(table8RequestedReflection.attributeValues.find(
              inheritedPrivilegeToDeleteAttribute) ==
          table8RequestedReflection.attributeValues.end());

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
} // namespace
