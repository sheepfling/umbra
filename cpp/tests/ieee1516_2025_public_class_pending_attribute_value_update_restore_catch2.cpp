#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds pending object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][pending-application-request-state]"
    "[application-value-state][object-lifecycle-state][attribute-value-update]"
    "[process-restart-class-pending-attribute-value-update]"
    "[public-process-restart-class-pending-attribute-value-update]"
    "[callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.provide-attribute-value-update]"
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
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
         "data" / "attribute-update-passel-fom.xml")
            .wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-pending-class-avu-evoked"}
        : std::wstring{L"public-pending-class-avu-immediate"};
    std::vector<unsigned char> const valueBytes{0x63U, 0x6CU};
    std::vector<unsigned char> const requestTagBytes{
        0x43U, 0x4CU, 0x41, 0x53U};
    VariableLengthData const requestTag(
        requestTagBytes.data(), requestTagBytes.size());

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
    ObjectClassHandle sourceBaseClass;
    ObjectClassHandle sourceChildClass;
    AttributeHandle sourceAttribute;

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
          L"public-pending-class-avu-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(requester->joinFederationExecution(
          L"public-pending-class-avu-requester", L"subscriber", federationName));

      sourceBaseClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_base);
      sourceChildClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      sourceAttribute = owner->getAttributeHandle(
          sourceChildClass, fixture_hla::fixture::reliable_base_a);
      REQUIRE(sourceBaseClass.isValid());
      REQUIRE(sourceChildClass.isValid());
      REQUIRE(sourceAttribute.isValid());
      AttributeHandleSet const attributes{sourceAttribute};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceChildClass, attributes));
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstance(sourceChildClass));
      REQUIRE(sourceObjectInstance.isValid());

      AttributeHandleValueMap values;
      values.emplace(
          sourceAttribute,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      VariableLengthData valueTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, valueTag));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requester->disableCallbacks());
      }
      // The class-designator request intentionally names the base while the
      // saved object is registered at its child. This proves class expansion
      // and the requested class identity survive the process restart.
      REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(
          sourceBaseClass, attributes, requestTag));
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
      auto const& savedObject = durableImage.objects.front();
      REQUIRE(savedObject.attributeValuesPresent);
      REQUIRE(savedObject.attributeValues.size() == 1U);
      REQUIRE(savedObject.pendingAttributeValueUpdateClassRequestsPresent);
      REQUIRE(savedObject.pendingAttributeValueUpdateClassRequests.size() == 1U);
      auto const& savedRequest =
          savedObject.pendingAttributeValueUpdateClassRequests.front();
      auto const sourceOwnerValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceOwnerHandle);
      REQUIRE(sourceOwnerValue.has_value());
      REQUIRE(savedRequest.providingFederateId == *sourceOwnerValue);
      REQUIRE(savedRequest.requestedObjectClassHandle != 0U);
      REQUIRE(savedRequest.requestedAttributeHandles.size() == 1U);
      auto const sourceAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttribute);
      REQUIRE(sourceAttributeValue.has_value());
      REQUIRE(savedRequest.requestedAttributeHandles.front() ==
              *sourceAttributeValue);
      REQUIRE(savedRequest.userSuppliedTag ==
              std::string(reinterpret_cast<char const*>(requestTagBytes.data()),
                          requestTagBytes.size()));
      REQUIRE(savedObject.attributeValues.front().value ==
              std::string(reinterpret_cast<char const*>(valueBytes.data()),
                          valueBytes.size()));

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
          L"public-pending-class-avu-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(requester->joinFederationExecution(
          L"public-pending-class-avu-requester", L"subscriber", federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);

      auto const freshBaseClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_base);
      auto const freshChildClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshAttribute = owner->getAttributeHandle(
          freshChildClass, fixture_hla::fixture::reliable_base_a);
      REQUIRE(freshBaseClass == sourceBaseClass);
      REQUIRE(freshChildClass == sourceChildClass);
      REQUIRE(freshAttribute == sourceAttribute);
      AttributeHandleSet const attributes{freshAttribute};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshChildClass, attributes));

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
      REQUIRE(provide.attributes == attributes);
      REQUIRE(variableLengthDataBytes(provide.userSuppliedTag) ==
              requestTagBytes);
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
