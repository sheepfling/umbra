#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded HLA_IMMEDIATE Query Attribute Ownership dispatches all 2025 report kinds",
    "[integration][development-profile][federation-management][ownership-management]"
    "[query-attribute-ownership][callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.query-attribute-ownership]"
    "[rti.service.is-attribute-owned-by-federate]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]"
    "[federate.callback.attribute-is-owned-by-rti]") {
  auto runApplicationReportKinds = [] {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
         "data" / "attribute-update-passel-fom.xml")
            .wstring();

    REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(
        requester->connect(requesterReports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"immediate-query-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"immediate-query-requester", L"subscriber", federationName));

    auto const child = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownedAttribute = owner->getAttributeHandle(
        child, fixture_hla::fixture::reliable_base_a);
    auto const unownedAttribute = owner->getAttributeHandle(
        child, fixture_hla::fixture::unowned_child);
    REQUIRE(child.isValid());
    REQUIRE(ownedAttribute.isValid());
    REQUIRE(unownedAttribute.isValid());
    AttributeHandleSet const ownedAttributes{ownedAttribute};
    AttributeHandleSet const subscriptions{ownedAttribute, unownedAttribute};
    AttributeHandleSet const queriedAttributes{ownedAttribute, unownedAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, subscriptions));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

    REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 2U);
    auto const federateReport = std::find_if(
        requesterReports.attributeOwnershipReports.begin(),
        requesterReports.attributeOwnershipReports.end(),
        [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
          return report.kind ==
              ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate;
        });
    auto const unownedReport = std::find_if(
        requesterReports.attributeOwnershipReports.begin(),
        requesterReports.attributeOwnershipReports.end(),
        [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
          return report.kind ==
              ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned;
        });
    REQUIRE(federateReport != requesterReports.attributeOwnershipReports.end());
    REQUIRE(unownedReport != requesterReports.attributeOwnershipReports.end());
    REQUIRE(federateReport->attributes == AttributeHandleSet{ownedAttribute});
    REQUIRE(unownedReport->attributes == AttributeHandleSet{unownedAttribute});
    REQUIRE(federateReport->owner == owner->getFederateHandle(
        L"immediate-query-owner"));
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));

    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  };

  auto runRtiOwnedReportKind = [] {
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
         "data" / "switch-nrg-disabled-fom.xml")
            .wstring();

    REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(rti->joinFederationExecution(
        L"immediate-rti-owned-query", L"observer", federationName));

    auto const momClass = rti->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const federateNameAttribute =
        rti->getAttributeHandle(momClass, L"HLAfederateName");
    AttributeHandleSet const queriedAttributes{federateNameAttribute};
    REQUIRE(momClass.isValid());
    REQUIRE(federateNameAttribute.isValid());
    REQUIRE_NOTHROW(rti->subscribeObjectClassAttributes(momClass, queriedAttributes));
    auto const* rtiAmbassador =
        dynamic_cast<rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador*>(
            rti.get());
    REQUIRE(rtiAmbassador != nullptr);
    auto const snapshot = rtiAmbassador->joinedFederateMomObjectSnapshotForTesting();
    REQUIRE(snapshot);
    auto const momObject =
        rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
            snapshot->objectInstanceHandle);
    REQUIRE_FALSE(rti->isAttributeOwnedByFederate(
        momObject, federateNameAttribute));
    REQUIRE_NOTHROW(rti->queryAttributeOwnership(momObject, queriedAttributes));
    REQUIRE(reports.attributeOwnershipReports.size() == 1U);
    auto const& report = reports.attributeOwnershipReports.front();
    REQUIRE(report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti);
    REQUIRE(report.objectInstance == momObject);
    REQUIRE(report.attributes == queriedAttributes);
    REQUIRE_FALSE(report.owner.isValid());
    REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));

    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(rti->disconnect());
  };

  runApplicationReportKinds();
  runRtiOwnedReportKind();
}
} // namespace
