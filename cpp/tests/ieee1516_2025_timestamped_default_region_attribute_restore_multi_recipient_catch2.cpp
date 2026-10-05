#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation restore restores one queued timestamped default-region attribute update to multiple recipients",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][ddm][time-management][tso]"
    "[timestamped-default-region-attribute-update][default-region]"
    "[mixed-fanout][multi-federate-callback-ordering]"
    "[timestamped-default-region-attribute-restore-multi-recipient]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.federation-saved]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-restored][rti.service.register-object-instance]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x4D, 0x55, 0x4C};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"live-tso-default-region-attribute-multi-recipient-baseline";

  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-live-default-region-attribute-multi-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"restore-live-default-region-attribute-multi-first", L"subscriber", federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"restore-live-default-region-attribute-multi-second", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto prepareReceiver = [&](auto& rti) {
    auto const region = rti->createRegion(DimensionHandleSet{sodaFlavor});
    REQUIRE_NOTHROW(rti->setRangeBounds(region, sodaFlavor, RangeBounds(2UL, 3UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));
    AttributeHandleSetRegionHandleSetPairVector const pair{{
        flavorOnly,
        RegionHandleSet{region},
    }};
    REQUIRE_NOTHROW(rti->subscribeObjectClassAttributesWithRegions(soda, pair));
    REQUIRE_FALSE(rti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(rti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    drain(*rti);
    return std::pair{region, pair};
  };
  auto const [firstRegion, firstPair] = prepareReceiver(firstReceiver);
  auto const [secondRegion, secondPair] = prepareReceiver(secondReceiver);

  // Ordinary registration has no public source-region handle. Each regional
  // subscriber nevertheless sees the publisher's derived default source.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(soda));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReceiverReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*publisher);

  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      flavor,
      VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());

  // Both constrained recipients are below the passel timestamp when the save
  // image is captured. Restore must retain an independent pending copy for
  // each joined federate, including each copy's retraction ledger.
  REQUIRE_NOTHROW(firstReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(secondReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);

  // Terminalize the post-save live handle. The restore below must replace it
  // with the queued passel and both pending-recipient entries.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  auto const verifyReflection = [&](ReportingFederateAmbassador const& reports) {
    REQUIRE(reports.attributeReflectionReports.size() == 1U);
    REQUIRE(reports.flushQueueGrantReports.size() == 1U);
    auto const& report = reports.attributeReflectionReports.front();
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == 1U);
    REQUIRE(report.attributeValues.contains(flavor));
    REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
            std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.sentRegionsSupplied);
    REQUIRE(report.sentRegions.empty());
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
    REQUIRE(reports.flushQueueGrantReports.front().value == L"5");
    REQUIRE(reports.flushQueueGrantReports.front().optimisticValue == L"7");
  };

  // Deliver each restored recipient independently. The first delivery must
  // not consume the second recipient's queued copy or retraction state.
  REQUIRE_NOTHROW(firstReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(firstReceiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.callbackOrder.empty());
  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  verifyReflection(firstReceiverReports);
  verifyReflection(secondReceiverReports);
  REQUIRE(firstReceiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});
  REQUIRE(secondReceiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(secondReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(firstReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(secondReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              firstReceiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(variableLengthDataBytes(
              secondReceiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(firstReceiverReports.callbackOrder == std::vector<std::string>{"request-retraction"});
  REQUIRE(secondReceiverReports.callbackOrder == std::vector<std::string>{"request-retraction"});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(firstReceiver->unsubscribeObjectClassAttributesWithRegions(soda, firstPair));
  REQUIRE_NOTHROW(secondReceiver->unsubscribeObjectClassAttributesWithRegions(soda, secondPair));
  REQUIRE_NOTHROW(firstReceiver->deleteRegion(firstRegion));
  REQUIRE_NOTHROW(secondReceiver->deleteRegion(secondRegion));
  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace
