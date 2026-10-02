#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore does not replay a delivered receive-order regional provider response under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ddm][regular-regional-attribute-update]"
    "[receive-order][durable-save-regional-pending-attribute-value-update]"
    "[durable-save-regional-pending-attribute-value-update-regular-response]"
    "[durable-save-regional-pending-attribute-value-update-regular-response-no-replay]"
    "[public-durable-save-regional-pending-attribute-value-update-regular-response-no-replay]"
    "[callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto const saveDirectory = temporaryFederationSaveDirectory();
    auto const saveStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const federationName = nextFederationName();
    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-regional-response-no-replay-evoked"}
        : std::wstring{L"public-regional-response-no-replay-immediate"};
    std::vector<unsigned char> const initialValueBytes{0x11U, 0x22U};
    std::vector<unsigned char> const responseValueBytes{0x5AU, 0x25U, 0x07U};
    std::vector<unsigned char> const requestTagBytes{0x52U, 0x45U, 0x51U};
    std::vector<unsigned char> const freshRequestTagBytes{
        0x4EU, 0x45U, 0x57U};
    std::vector<unsigned char> const responseTagBytes{0x52U, 0x45U, 0x53U};
    VariableLengthData const requestTag(
        requestTagBytes.data(), requestTagBytes.size());
    VariableLengthData const freshRequestTag(
        freshRequestTagBytes.data(), freshRequestTagBytes.size());

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
          L"public-regional-response-no-replay-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(requester->joinFederationExecution(
          L"public-regional-response-no-replay-requester",
          L"subscriber",
          federationName));

      sourceSoda = owner->getObjectClassHandle(
          fixture_hla::fom::food_drink_soda);
      sourceFlavor = owner->getAttributeHandle(
          sourceSoda, fixture_hla::fixture::flavor);
      sourceSodaFlavor = owner->getDimensionHandle(
          fixture_hla::fixture::soda_flavor);
      REQUIRE(sourceSoda.isValid());
      REQUIRE(sourceFlavor.isValid());
      REQUIRE(sourceSodaFlavor.isValid());
      AttributeHandleSet const flavorOnly{sourceFlavor};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceSoda, flavorOnly));

      REQUIRE_NOTHROW(sourceOwnerRegion = owner->createRegion(
          DimensionHandleSet{sourceSodaFlavor}));
      REQUIRE_NOTHROW(sourceRequesterRegion = requester->createRegion(
          DimensionHandleSet{sourceSodaFlavor}));
      REQUIRE_NOTHROW(owner->setRangeBounds(
          sourceOwnerRegion, sourceSodaFlavor, RangeBounds(0UL, 2UL)));
      REQUIRE_NOTHROW(requester->setRangeBounds(
          sourceRequesterRegion, sourceSodaFlavor, RangeBounds(0UL, 2UL)));
      REQUIRE_NOTHROW(owner->commitRegionModifications(
          RegionHandleSet{sourceOwnerRegion}));
      REQUIRE_NOTHROW(requester->commitRegionModifications(
          RegionHandleSet{sourceRequesterRegion}));
      AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
          flavorOnly,
          RegionHandleSet{sourceOwnerRegion},
      }};
      AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
          flavorOnly,
          RegionHandleSet{sourceRequesterRegion},
      }};
      REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
          sourceSoda, requesterPair));
      REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstanceWithRegions(
                              sourceSoda, ownerPair));
      drain(*requester);
      REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);

      AttributeHandleValueMap initialValues;
      initialValues.emplace(
          sourceFlavor,
          VariableLengthData(
              initialValueBytes.data(), initialValueBytes.size()));
      VariableLengthData const initialTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, initialValues, initialTag));
      drain(*requester);
      REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
      REQUIRE(requesterReports.attributeReflectionReports.front().objectInstance ==
              sourceObjectInstance);
      REQUIRE(variableLengthDataBytes(
                  requesterReports.attributeReflectionReports.front()
                      .attributeValues.at(sourceFlavor)) == initialValueBytes);

      // RestaurantFOMmodule-2025 enables automatic provision.  Drain that
      // setup callback before isolating the explicit regional response.
      drain(*owner);
      auto const initialProvideReportCount =
          ownerReports.attributeValueUpdateRequestReports.size();
      std::vector<unsigned char> observedRequestTag;
      ownerReports.provideAttributeValueUpdateHandler =
          [&owner, &observedRequestTag, responseValueBytes, responseTagBytes](
              ObjectInstanceHandle const& callbackObject,
              AttributeHandleSet const& callbackAttributes,
              VariableLengthData const& callbackTag) {
            observedRequestTag = variableLengthDataBytes(callbackTag);
            AttributeHandleValueMap values;
            for (AttributeHandle const& attribute : callbackAttributes) {
              values.emplace(
                  attribute,
                  VariableLengthData(
                      responseValueBytes.data(), responseValueBytes.size()));
            }
            VariableLengthData const responseTag(
                responseTagBytes.data(), responseTagBytes.size());
            owner->updateAttributeValues(
                callbackObject, values, responseTag);
          };

      REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
          sourceSoda, requesterPair, requestTag));
      drain(*owner);
      drain(*requester);
      REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
              initialProvideReportCount + 1U);
      REQUIRE(observedRequestTag == requestTagBytes);
      REQUIRE(requesterReports.attributeReflectionReports.size() == 2U);
      auto const& response = requesterReports.attributeReflectionReports.back();
      REQUIRE(response.objectInstance == sourceObjectInstance);
      REQUIRE(response.attributeValues.size() == 1U);
      REQUIRE(response.attributeValues.contains(sourceFlavor));
      REQUIRE(variableLengthDataBytes(
                  response.attributeValues.at(sourceFlavor)) == responseValueBytes);
      REQUIRE(variableLengthDataBytes(response.userSuppliedTag) == responseTagBytes);
      REQUIRE(response.sentRegionsSupplied);
      REQUIRE(response.sentRegions == RegionHandleSet{sourceOwnerRegion});

      // The delivered receive-order response is now application state, not a
      // pending provider request.  Clear the callback observation before save
      // so a fresh-registry restore can be checked for replay precisely.
      requesterReports.attributeReflectionReports.clear();
      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(requester->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(requester->federateSaveComplete());
      drainBoth(*owner, *requester);
      REQUIRE(ownerReports.federationSavedReportCount == 1U);
      REQUIRE(requesterReports.federationSavedReportCount == 1U);

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(durableImage.objects.size() == 1U);
      REQUIRE(durableImage.regions.size() == 2U);
      auto const& savedObject = durableImage.objects.front();
      REQUIRE(savedObject.attributeValuesPresent);
      REQUIRE(savedObject.attributeValues.size() == 1U);
      REQUIRE(savedObject.attributeValues.front().value ==
              std::string(
                  reinterpret_cast<char const*>(responseValueBytes.data()),
                  responseValueBytes.size()));
      REQUIRE(savedObject.pendingAttributeValueUpdateRegionalRequestsPresent);
      REQUIRE(savedObject.pendingAttributeValueUpdateRegionalRequests.empty());
      auto const sourceFlavorValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(sourceFlavor);
      REQUIRE(sourceFlavorValue.has_value());
      auto const savedAttribute = std::find_if(
          savedObject.attributes.begin(),
          savedObject.attributes.end(),
          [&](auto const& attribute) {
            return attribute.handle == *sourceFlavorValue;
          });
      REQUIRE(savedAttribute != savedObject.attributes.end());
      auto const sourceOwnerRegionValue =
          rti1516_2025::umbra_binding_detail::regionHandleValue(sourceOwnerRegion);
      REQUIRE(sourceOwnerRegionValue.has_value());
      REQUIRE(savedAttribute->updateRegionHandles ==
              std::vector<std::uint64_t>{*sourceOwnerRegionValue});

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
          L"public-regional-response-no-replay-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(requester->joinFederationExecution(
          L"public-regional-response-no-replay-requester",
          L"subscriber",
          federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);

      auto const freshSoda = owner->getObjectClassHandle(
          fixture_hla::fom::food_drink_soda);
      auto const freshFlavor = owner->getAttributeHandle(
          freshSoda, fixture_hla::fixture::flavor);
      REQUIRE(freshSoda == sourceSoda);
      REQUIRE(freshFlavor == sourceFlavor);
      AttributeHandleSet const freshFlavorOnly{freshFlavor};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshSoda, freshFlavorOnly));
      drainBoth(*owner, *requester);
      requesterReports.attributeReflectionReports.clear();
      auto const providerBeforeRestore =
          ownerReports.attributeValueUpdateRequestReports.size();

      std::vector<unsigned char> observedFreshRequestTag;
      ownerReports.provideAttributeValueUpdateHandler =
          [&owner, &observedFreshRequestTag, responseValueBytes, responseTagBytes](
              ObjectInstanceHandle const& callbackObject,
              AttributeHandleSet const& callbackAttributes,
              VariableLengthData const& callbackTag) {
            observedFreshRequestTag = variableLengthDataBytes(callbackTag);
            AttributeHandleValueMap values;
            for (AttributeHandle const& attribute : callbackAttributes) {
              values.emplace(
                  attribute,
                  VariableLengthData(
                      responseValueBytes.data(), responseValueBytes.size()));
            }
            VariableLengthData const responseTag(
                responseTagBytes.data(), responseTagBytes.size());
            owner->updateAttributeValues(
                callbackObject, values, responseTag);
          };

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requester->disableCallbacks());
      }
      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainBoth(*owner, *requester);
        REQUIRE(ownerReports.federationRestoredReportCount == 0U);
        REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
                providerBeforeRestore);
        REQUIRE(requesterReports.attributeReflectionReports.empty());
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
      REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
              providerBeforeRestore);
      REQUIRE(requesterReports.attributeReflectionReports.empty());

      auto const freshRequesterPair =
          AttributeHandleSetRegionHandleSetPairVector{{
              freshFlavorOnly,
              RegionHandleSet{sourceRequesterRegion},
          }};
      auto const beforeFreshProvider =
          ownerReports.attributeValueUpdateRequestReports.size();
      REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
          freshSoda, freshRequesterPair, freshRequestTag));
      drain(*owner);
      drain(*requester);
      REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
              beforeFreshProvider + 1U);
      REQUIRE(observedFreshRequestTag == freshRequestTagBytes);
      REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
      auto const& freshResponse =
          requesterReports.attributeReflectionReports.back();
      REQUIRE(freshResponse.objectInstance == sourceObjectInstance);
      REQUIRE(freshResponse.attributeValues.size() == 1U);
      REQUIRE(freshResponse.attributeValues.contains(freshFlavor));
      REQUIRE(variableLengthDataBytes(
                  freshResponse.attributeValues.at(freshFlavor)) ==
              responseValueBytes);
      REQUIRE(variableLengthDataBytes(freshResponse.userSuppliedTag) ==
              responseTagBytes);
      REQUIRE(freshResponse.sentRegionsSupplied);
      REQUIRE(freshResponse.sentRegions == RegionHandleSet{sourceOwnerRegion});

      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(requester->disconnect());
    }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
}
