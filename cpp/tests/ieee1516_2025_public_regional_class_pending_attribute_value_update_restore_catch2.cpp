#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds pending regional object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][pending-application-request-state]"
    "[application-value-state][object-lifecycle-state][attribute-value-update][ddm][region-state]"
    "[process-restart-regional-pending-attribute-value-update]"
    "[public-process-restart-regional-pending-attribute-value-update-provider-departure]"
    "[public-process-restart-regional-pending-attribute-value-update-negative]"
    "[public-process-restart-regional-pending-attribute-value-update]"
    "[callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  auto runScenario = [](CallbackModel callbackModel, bool providerDeparture) {
    auto const saveDirectory = temporaryFederationSaveDirectory();
    auto const saveStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const federationName = nextFederationName();
    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-pending-regional-avu-evoked"}
        : std::wstring{L"public-pending-regional-avu-immediate"};
    unsigned char const valueBytes[] = {0x52, 0x47};
    unsigned char const requestTagBytes[] = {0x52, 0x45, 0x47};
    VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));

    auto const drain = [](RTIambassador& ambassador) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(ambassador.evokeCallback(0.0));
      }
    };
    auto const drainBoth = [&](RTIambassador& first, RTIambassador& second) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(first.evokeCallback(0.0));
        static_cast<void>(second.evokeCallback(0.0));
      }
    };

    FederateHandle sourceOwnerHandle;
    ObjectInstanceHandle sourceObjectInstance;
    ObjectClassHandle sourceSoda;
    AttributeHandle sourceFlavor;
    DimensionHandle sourceSodaFlavor;
    RegionHandle sourceOwnerRegion;
    RegionHandle sourceRequesterRegion;

    {
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador requesterReports;
      auto owner = makeRti();
      auto requester = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
          L"public-pending-regional-avu-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(requester->joinFederationExecution(
          L"public-pending-regional-avu-requester", L"subscriber", federationName));

      sourceSoda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
      sourceFlavor = owner->getAttributeHandle(
          sourceSoda, fixture_hla::fixture::flavor);
      sourceSodaFlavor = owner->getDimensionHandle(
          fixture_hla::fixture::soda_flavor);
      auto const requesterSoda = requester->getObjectClassHandle(
          fixture_hla::fom::food_drink_soda);
      auto const requesterFlavor = requester->getAttributeHandle(
          requesterSoda, fixture_hla::fixture::flavor);
      auto const requesterSodaFlavor = requester->getDimensionHandle(
          fixture_hla::fixture::soda_flavor);
      REQUIRE(sourceSoda.isValid());
      REQUIRE(sourceFlavor.isValid());
      REQUIRE(sourceSodaFlavor.isValid());
      REQUIRE(requesterSoda.isValid());
      REQUIRE(requesterFlavor.isValid());
      REQUIRE(requesterSodaFlavor.isValid());
      AttributeHandleSet const ownerFlavorOnly{sourceFlavor};
      AttributeHandleSet const requesterFlavorOnly{requesterFlavor};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceSoda, ownerFlavorOnly));

      REQUIRE_NOTHROW(sourceOwnerRegion = owner->createRegion(
          DimensionHandleSet{sourceSodaFlavor}));
      REQUIRE_NOTHROW(sourceRequesterRegion = requester->createRegion(
          DimensionHandleSet{requesterSodaFlavor}));
      REQUIRE_NOTHROW(owner->setRangeBounds(
          sourceOwnerRegion, sourceSodaFlavor, RangeBounds(0UL, 2UL)));
      REQUIRE_NOTHROW(owner->commitRegionModifications(
          RegionHandleSet{sourceOwnerRegion}));
      REQUIRE_NOTHROW(requester->setRangeBounds(
          sourceRequesterRegion, requesterSodaFlavor, RangeBounds(1UL, 3UL)));
      REQUIRE_NOTHROW(requester->commitRegionModifications(
          RegionHandleSet{sourceRequesterRegion}));
      AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
          ownerFlavorOnly,
          RegionHandleSet{sourceOwnerRegion},
      }};
      AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
          requesterFlavorOnly,
          RegionHandleSet{sourceRequesterRegion},
      }};
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstanceWithRegions(
                              sourceSoda, ownerPair));

      AttributeHandleValueMap values;
      values.emplace(
          sourceFlavor,
          VariableLengthData(valueBytes, sizeof(valueBytes)));
      VariableLengthData valueTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, valueTag));
      drain(*requester);

      // Keep the provider callback at the pre-save boundary for both public
      // callback models so the saved image contains the accepted request.
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requester->disableCallbacks());
      }
      REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
          requesterSoda, requesterPair, requestTag));
      REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(requester->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(requester->federateSaveComplete());

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(durableImage.objects.size() == 1U);
      REQUIRE(durableImage.regions.size() == 2U);
      auto const& savedObject = durableImage.objects.front();
      REQUIRE(savedObject.attributeValuesPresent);
      REQUIRE(savedObject.attributeValues.size() == 1U);
      REQUIRE(savedObject.pendingAttributeValueUpdateRegionalRequestsPresent);
      REQUIRE(savedObject.pendingAttributeValueUpdateRegionalRequests.size() == 1U);
      auto const& savedRequest =
          savedObject.pendingAttributeValueUpdateRegionalRequests.front();
      REQUIRE(savedRequest.requestingFederateId != 0U);
      auto const sourceOwnerValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceOwnerHandle);
      REQUIRE(sourceOwnerValue.has_value());
      REQUIRE(savedRequest.providingFederateId == *sourceOwnerValue);
      REQUIRE(savedRequest.requestedObjectClassHandle != 0U);
      REQUIRE(savedRequest.requestedAttributeHandles.size() == 1U);
      auto const sourceFlavorValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(sourceFlavor);
      REQUIRE(sourceFlavorValue.has_value());
      REQUIRE(savedRequest.requestedAttributeHandles.front() == *sourceFlavorValue);
      REQUIRE(savedRequest.requestRegionsByAttribute.size() == 1U);
      REQUIRE(savedRequest.requestRegionsByAttribute.front().first ==
              rti1516_2025::umbra_binding_detail::attributeHandleValue(
                  requesterFlavor)
                  .value());
      REQUIRE(savedRequest.requestRegionsByAttribute.front().second ==
              std::vector<std::uint64_t>{
                  rti1516_2025::umbra_binding_detail::regionHandleValue(
                      sourceRequesterRegion)
                      .value()});
      REQUIRE(savedRequest.userSuppliedTag ==
              std::string(reinterpret_cast<char const*>(requestTagBytes),
                          sizeof(requestTagBytes)));
      REQUIRE(savedObject.attributeValues.front().value ==
              std::string(reinterpret_cast<char const*>(valueBytes),
                          sizeof(valueBytes)));

      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(requester->disconnect());
    }

    {
      auto const freshRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador requesterReports;
      auto owner = makeRti();
      auto requester = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-pending-regional-avu-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(requester->joinFederationExecution(
          L"public-pending-regional-avu-requester", L"subscriber", federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);

      auto const freshSoda = owner->getObjectClassHandle(
          fixture_hla::fom::food_drink_soda);
      auto const freshFlavor = owner->getAttributeHandle(
          freshSoda, fixture_hla::fixture::flavor);
      auto const freshSodaFlavor = owner->getDimensionHandle(
          fixture_hla::fixture::soda_flavor);
      REQUIRE(freshSoda == sourceSoda);
      REQUIRE(freshFlavor == sourceFlavor);
      REQUIRE(freshSodaFlavor == sourceSodaFlavor);
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshSoda, AttributeHandleSet{freshFlavor}));
      AttributeHandleSetRegionHandleSetPairVector const freshRequesterPair{{
          AttributeHandleSet{freshFlavor},
          RegionHandleSet{sourceRequesterRegion},
      }};

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requester->disableCallbacks());
      }
      ownerReports.onProvideAttributeValueUpdate = [&] {
        ownerReports.callbackOrder.push_back("provide");
      };
      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainBoth(*owner, *requester);
        REQUIRE(ownerReports.federationRestoredReportCount == 0U);
        REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
      }

      REQUIRE_NOTHROW(owner->federateRestoreComplete());
      REQUIRE_NOTHROW(requester->federateRestoreComplete());
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
        REQUIRE_NOTHROW(requester->enableCallbacks());
      } else {
        drainBoth(*owner, *requester);
      }

      REQUIRE(ownerReports.federationRestoredReportCount == 1U);
      REQUIRE(requesterReports.federationRestoredReportCount == 1U);
      REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
      auto const& provide = ownerReports.attributeValueUpdateRequestReports.front();
      REQUIRE(provide.objectInstance == sourceObjectInstance);
      REQUIRE(provide.attributes == AttributeHandleSet{freshFlavor});
      REQUIRE(variableLengthDataBytes(provide.userSuppliedTag) ==
              std::vector<unsigned char>(
                  requestTagBytes, requestTagBytes + sizeof(requestTagBytes)));
      REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());
      auto const restoredComplete = std::find(
          ownerReports.callbackOrder.begin(), ownerReports.callbackOrder.end(),
          "restore-complete");
      auto const provideCallback = std::find(
          ownerReports.callbackOrder.begin(), ownerReports.callbackOrder.end(),
          "provide");
      REQUIRE(restoredComplete != ownerReports.callbackOrder.end());
      REQUIRE(provideCallback != ownerReports.callbackOrder.end());
      REQUIRE(restoredComplete < provideCallback);
      REQUIRE_FALSE(owner->evokeCallback(0.0));
      REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);

      auto const provideCountAfterRestore =
          ownerReports.attributeValueUpdateRequestReports.size();

      if (providerDeparture) {
        // Admit a second request, then remove the provider before callback
        // entry.  The pending regional request must be retired with the
        // provider's joined-federate lifetime rather than producing a stale
        // Provide callback after the provider has departed.
        if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
          REQUIRE_NOTHROW(owner->disableCallbacks());
          REQUIRE_NOTHROW(requester->disableCallbacks());
        }
        REQUIRE_NOTHROW(requester->setRangeBounds(
            sourceRequesterRegion,
            freshSodaFlavor,
            RangeBounds(1UL, 3UL)));
        REQUIRE_NOTHROW(requester->commitRegionModifications(
            RegionHandleSet{sourceRequesterRegion}));
        REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
            freshSoda, freshRequesterPair, requestTag));
        REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
                provideCountAfterRestore);
        REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());

        REQUIRE_NOTHROW(owner->resignFederationExecution(
            CANCEL_THEN_DELETE_THEN_DIVEST));

        if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
          REQUIRE_NOTHROW(requester->enableCallbacks());
        }
        drain(*requester);
        REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
                provideCountAfterRestore);
        REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());

        // A second drain is the provider-departure one-shot fence: the
        // retired request cannot reappear as a late callback.
        drain(*requester);
        REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
                provideCountAfterRestore);
        REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());

        REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
        REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
        REQUIRE_NOTHROW(owner->disconnect());
        REQUIRE_NOTHROW(requester->disconnect());
        return;
      }

      bool requesterResigned = false;
      auto runSuppressedRegionalRequest = [&](bool resignRequester) {
        if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
          REQUIRE_NOTHROW(owner->disableCallbacks());
          REQUIRE_NOTHROW(requester->disableCallbacks());
        }

        // Re-establish the original overlap before admitting the request.
        REQUIRE_NOTHROW(requester->setRangeBounds(
            sourceRequesterRegion,
            freshSodaFlavor,
            RangeBounds(1UL, 3UL)));
        REQUIRE_NOTHROW(requester->commitRegionModifications(
            RegionHandleSet{sourceRequesterRegion}));
        REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
            freshSoda,
            freshRequesterPair,
            requestTag));

        if (resignRequester) {
          // The queued provider work must be suppressed when the requesting
          // joined federate leaves before callback entry.
          REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
          requesterResigned = true;
        } else {
          // The request was accepted while the regions overlapped.  Mutating
          // the committed requester range before callback entry makes the
          // saved request stale and must suppress delivery.
          REQUIRE_NOTHROW(requester->setRangeBounds(
              sourceRequesterRegion,
              freshSodaFlavor,
              RangeBounds(3UL, 4UL)));
          REQUIRE_NOTHROW(requester->commitRegionModifications(
              RegionHandleSet{sourceRequesterRegion}));
        }

        if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
          REQUIRE_NOTHROW(owner->enableCallbacks());
          if (!resignRequester) {
            REQUIRE_NOTHROW(requester->enableCallbacks());
          }
        }
        if (resignRequester) {
          drain(*owner);
        } else {
          drainBoth(*owner, *requester);
        }
        REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
                provideCountAfterRestore);

        // A second drain is the public one-shot fence: stale or departed
        // work cannot be reintroduced as a duplicate Provide callback.
        if (resignRequester) {
          drain(*owner);
        } else {
          drainBoth(*owner, *requester);
        }
        REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
                provideCountAfterRestore);
      };

      runSuppressedRegionalRequest(false);
      runSuppressedRegionalRequest(true);

      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      if (!requesterResigned) {
        REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
      }
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(requester->disconnect());
    }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED, false);
    runScenario(HLA_EVOKED, true);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE, false);
    runScenario(rti1516_2025::HLA_IMMEDIATE, true);
  }
}

}
