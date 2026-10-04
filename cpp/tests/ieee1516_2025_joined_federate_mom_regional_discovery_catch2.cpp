#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded joined-federate MOM regional discovery uses the immutable HLAfederate point",
    "[integration][development-profile][federation-management][mom][object-management][ddm]"
    "[service-report-file][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values][federate.callback.remove-object-instance]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador subjectReports;
    ReportingFederateAmbassador matchingReports;
    ReportingFederateAmbassador disjointReports;
    auto subject = makeRti();
    auto matching = makeRti();
    auto disjoint = makeRti();
    auto subjectDirectory = temporaryServiceReportDirectory();
    auto matchingDirectory = temporaryServiceReportDirectory();
    auto disjointDirectory = temporaryServiceReportDirectory();
    auto subjectConfiguration = configurationForServiceReportDirectory(subjectDirectory.path());
    auto matchingConfiguration = configurationForServiceReportDirectory(matchingDirectory.path());
    auto disjointConfiguration = configurationForServiceReportDirectory(disjointDirectory.path());
    subjectConfiguration.withRtiAddress(L"in-process");
    matchingConfiguration.withRtiAddress(L"in-process");
    disjointConfiguration.withRtiAddress(L"in-process");
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
         "switch-nrg-disabled-fom.xml")
            .wstring();

    REQUIRE_NOTHROW(subject->connect(subjectReports, callbackModel, subjectConfiguration));
    REQUIRE_NOTHROW(matching->connect(matchingReports, callbackModel, matchingConfiguration));
    REQUIRE_NOTHROW(disjoint->connect(disjointReports, callbackModel, disjointConfiguration));
    REQUIRE_NOTHROW(
        subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
    auto const subjectFederate = subject->joinFederationExecution(
        L"regional-mom-subject", L"subject", federationName);
    REQUIRE_NOTHROW(matching->joinFederationExecution(
        L"regional-mom-matching", L"observer", federationName));
    REQUIRE_NOTHROW(disjoint->joinFederationExecution(
        L"regional-mom-disjoint", L"observer", federationName));

    auto const momClass = matching->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const federateNameAttribute = matching->getAttributeHandle(
        momClass, standard_hla::mom::federate_name);
    auto const federateDimension = matching->getDimensionHandle(standard_hla::mom::federate);
    auto const reliable = matching->getTransportationTypeHandle(standard_hla::mom::reliable);
    REQUIRE(momClass.isValid());
    REQUIRE(federateNameAttribute.isValid());
    REQUIRE(federateDimension.isValid());
    REQUIRE(reliable.isValid());

    auto const normalizedSubject = matching->normalizeFederateHandle(subjectFederate);
    REQUIRE(normalizedSubject < std::numeric_limits<unsigned long>::max());
    auto const disjointPoint = normalizedSubject == 0UL ? 1UL : normalizedSubject - 1UL;
    auto const matchingRegion = matching->createRegion(DimensionHandleSet{federateDimension});
    auto const disjointRegion = disjoint->createRegion(DimensionHandleSet{federateDimension});
    REQUIRE_NOTHROW(matching->setRangeBounds(
        matchingRegion,
        federateDimension,
        RangeBounds(disjointPoint, disjointPoint + 1UL)));
    REQUIRE_NOTHROW(disjoint->setRangeBounds(
        disjointRegion,
        federateDimension,
        RangeBounds(disjointPoint, disjointPoint + 1UL)));
    REQUIRE_NOTHROW(matching->commitRegionModifications(RegionHandleSet{matchingRegion}));
    REQUIRE_NOTHROW(disjoint->commitRegionModifications(RegionHandleSet{disjointRegion}));

    AttributeHandleSetRegionHandleSetPairVector const matchingPair{{
        AttributeHandleSet{federateNameAttribute},
        RegionHandleSet{matchingRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const disjointPair{{
        AttributeHandleSet{federateNameAttribute},
        RegionHandleSet{disjointRegion},
    }};
    REQUIRE_NOTHROW(matching->subscribeObjectClassAttributesWithRegions(
        momClass,
        matchingPair));
    REQUIRE_NOTHROW(disjoint->subscribeObjectClassAttributesWithRegions(
        momClass,
        disjointPair));
    while (matching->evokeCallback(0.0)) {
    }
    while (disjoint->evokeCallback(0.0)) {
    }
    REQUIRE(matchingReports.objectDiscoveryReports.empty());
    REQUIRE(disjointReports.objectDiscoveryReports.empty());

    // A committed range mutation must re-evaluate existing regional MOM
    // subscriptions and discover the point once it becomes eligible.
    REQUIRE_NOTHROW(matching->setRangeBounds(
        matchingRegion,
        federateDimension,
        RangeBounds(normalizedSubject, normalizedSubject + 1UL)));
    REQUIRE_NOTHROW(matching->commitRegionModifications(RegionHandleSet{matchingRegion}));
    while (matching->evokeCallback(0.0)) {
    }
    while (disjoint->evokeCallback(0.0)) {
    }

    auto const discovered = std::find_if(
        matchingReports.objectDiscoveryReports.begin(),
        matchingReports.objectDiscoveryReports.end(),
        [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
          return report.objectClass == momClass;
        });
    REQUIRE(discovered != matchingReports.objectDiscoveryReports.end());
    REQUIRE_FALSE(discovered->producingFederate.isValid());
    REQUIRE(disjointReports.objectDiscoveryReports.empty());

    auto const reflected = std::find_if(
        matchingReports.attributeReflectionReports.begin(),
        matchingReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == discovered->objectInstance &&
              report.attributeValues.contains(federateNameAttribute);
        });
    REQUIRE(reflected != matchingReports.attributeReflectionReports.end());
    REQUIRE(reflected->attributeValues.size() == 1U);
    rti1516_2025::HLAunicodeString reflectedName;
    REQUIRE_NOTHROW(reflectedName.decode(
        reflected->attributeValues.at(federateNameAttribute)));
    REQUIRE(reflectedName.get() == L"regional-mom-subject");
    REQUIRE(reflected->transportationType == reliable);
    REQUIRE_FALSE(reflected->producingFederate.isValid());
    REQUIRE_FALSE(reflected->sentRegionsSupplied);

    // The class-level regional request must use the same immutable
    // HLAfederate point as regional discovery.  A matching request is served
    // directly by the RTI-owned MOM object; it does not induce a provider
    // callback at the represented federate.
    auto const reflectionCount = matchingReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(matching->requestAttributeValueUpdateWithRegions(
        momClass,
        matchingPair,
        VariableLengthData{}));
    REQUIRE(subjectReports.attributeValueUpdateRequestReports.empty());
    while (matching->evokeCallback(0.0)) {
    }
    REQUIRE(matchingReports.attributeReflectionReports.size() == reflectionCount + 1U);
    auto const& regionalRequested = matchingReports.attributeReflectionReports.back();
    REQUIRE(regionalRequested.objectInstance == discovered->objectInstance);
    REQUIRE(regionalRequested.attributeValues.size() == 1U);
    REQUIRE(regionalRequested.attributeValues.contains(federateNameAttribute));
    REQUIRE_FALSE(regionalRequested.sentRegionsSupplied);

    // Re-evaluate the explicit request region at callback time.  In the
    // evoked model, moving it away from the represented point before delivery
    // suppresses the stale queued reflection.  Immediate delivery has already
    // crossed the callback boundary, so that second request is expected to
    // arrive before the mutation.
    auto const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    REQUIRE_NOTHROW(matching->requestAttributeValueUpdateWithRegions(
        momClass,
        matchingPair,
        VariableLengthData{}));
    if (!immediate) {
      REQUIRE_NOTHROW(matching->setRangeBounds(
          matchingRegion,
          federateDimension,
          RangeBounds(disjointPoint, disjointPoint + 1UL)));
      REQUIRE_NOTHROW(matching->commitRegionModifications(RegionHandleSet{matchingRegion}));
    }
    while (matching->evokeCallback(0.0)) {
    }
    REQUIRE(matchingReports.attributeReflectionReports.size() ==
            reflectionCount + (immediate ? 2U : 1U));
    if (immediate) {
      REQUIRE_NOTHROW(matching->setRangeBounds(
          matchingRegion,
          federateDimension,
          RangeBounds(disjointPoint, disjointPoint + 1UL)));
      REQUIRE_NOTHROW(matching->commitRegionModifications(RegionHandleSet{matchingRegion}));
    }

    // The non-regional request remains an independent direct-value query and
    // is not accidentally gated by the moved regional declaration.
    REQUIRE_NOTHROW(matching->requestAttributeValueUpdate(
        discovered->objectInstance,
        AttributeHandleSet{federateNameAttribute},
        VariableLengthData{}));
    while (matching->evokeCallback(0.0)) {
    }
    REQUIRE(matchingReports.attributeReflectionReports.size() == reflectionCount +
            (immediate ? 3U : 2U));
    auto const& requested = matchingReports.attributeReflectionReports.back();
    REQUIRE(requested.objectInstance == discovered->objectInstance);
    REQUIRE(requested.attributeValues.size() == 1U);
    REQUIRE(requested.transportationType == reliable);
    REQUIRE_FALSE(requested.producingFederate.isValid());

    REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
    while (matching->evokeCallback(0.0)) {
    }
    while (disjoint->evokeCallback(0.0)) {
    }
    auto const removed = std::find_if(
        matchingReports.objectRemovalReports.begin(),
        matchingReports.objectRemovalReports.end(),
        [&](ReportingFederateAmbassador::ObjectRemovalReport const& report) {
          return report.objectInstance == discovered->objectInstance;
        });
    REQUIRE(removed != matchingReports.objectRemovalReports.end());
    REQUIRE_FALSE(removed->producingFederate.isValid());
    auto const disjointRemoved = std::find_if(
        disjointReports.objectRemovalReports.begin(),
        disjointReports.objectRemovalReports.end(),
        [&](ReportingFederateAmbassador::ObjectRemovalReport const& report) {
          return report.objectInstance == discovered->objectInstance;
        });
    REQUIRE(disjointRemoved == disjointReports.objectRemovalReports.end());

    REQUIRE_NOTHROW(matching->unsubscribeObjectClassAttributesWithRegions(
        momClass,
        matchingPair));
    REQUIRE_NOTHROW(disjoint->unsubscribeObjectClassAttributesWithRegions(
        momClass,
        disjointPair));
    REQUIRE_NOTHROW(matching->deleteRegion(matchingRegion));
    REQUIRE_NOTHROW(disjoint->deleteRegion(disjointRegion));
    REQUIRE_NOTHROW(matching->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(disjoint->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(matching->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subject->disconnect());
    REQUIRE_NOTHROW(matching->disconnect());
    REQUIRE_NOTHROW(disjoint->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
}
