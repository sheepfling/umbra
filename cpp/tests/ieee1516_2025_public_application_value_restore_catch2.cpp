#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rehydrates latest object application value and report-file lifetime through HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][application-value-state]"
    "[public-process-restart-application-value][attribute-value-update][callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto const saveDirectory = temporaryFederationSaveDirectory();
    auto const saveStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const reportDirectory = temporaryServiceReportDirectory();
    auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
    configuration.withRtiAddress(L"in-process");
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
         "attribute-update-passel-fom.xml")
            .wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-application-value-evoked"}
        : std::wstring{L"public-application-value-immediate"};
    auto const afterRestoreLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-application-value-after-restore-evoked"}
        : std::wstring{L"public-application-value-after-restore-immediate"};
    std::vector<unsigned char> const valueBytes{0x61U, 0x70U, 0x70U, 0x01U};
    std::vector<unsigned char> const nextValueBytes{0x61U, 0x70U, 0x70U, 0x02U};
    auto const expectedValue = std::string(
        reinterpret_cast<char const*>(valueBytes.data()), valueBytes.size());

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
    FederateHandle sourceReceiverHandle;
    ObjectInstanceHandle sourceObjectInstance;
    ObjectClassHandle sourceObjectClass;
    AttributeHandle sourceAttribute;
    std::vector<std::filesystem::path> sourceReportFiles;

    {
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador receiverReports;
      auto owner = makeRti();
      auto receiver = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, configuration));
      REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel, configuration));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
          L"public-application-value-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(sourceReceiverHandle = receiver->joinFederationExecution(
          L"public-application-value-receiver", L"subscriber", federationName));

      sourceReportFiles = serviceReportFiles(reportDirectory.path());
      REQUIRE(sourceReportFiles.size() == 2U);
      for (auto const& file : sourceReportFiles) {
        REQUIRE(std::filesystem::absolute(file).lexically_normal() ==
                file.lexically_normal());
        REQUIRE_FALSE(readTextFile(file).empty());
      }

      sourceObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      sourceAttribute = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_child);
      REQUIRE(sourceObjectClass.isValid());
      REQUIRE(sourceAttribute.isValid());
      AttributeHandleSet const attributes{sourceAttribute};
      REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
          sourceObjectClass, attributes));
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceObjectClass, attributes));
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstance(sourceObjectClass));
      REQUIRE(sourceObjectInstance.isValid());
      drain(*receiver);
      REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance ==
              sourceObjectInstance);

      AttributeHandleValueMap values;
      values.emplace(
          sourceAttribute,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      VariableLengthData emptyTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, emptyTag));
      drain(*receiver);
      REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
      REQUIRE(variableLengthDataBytes(
                  receiverReports.attributeReflectionReports.front()
                      .attributeValues.at(sourceAttribute)) == valueBytes);

      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      drainBoth(*owner, *receiver);
      REQUIRE(ownerReports.initiateFederateSaveReports.size() == 1U);
      REQUIRE(receiverReports.initiateFederateSaveReports.size() == 1U);
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(receiver->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(receiver->federateSaveComplete());
      drainBoth(*owner, *receiver);
      REQUIRE(ownerReports.federationSavedReportCount == 1U);
      REQUIRE(receiverReports.federationSavedReportCount == 1U);

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const image = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(image.objects.size() == 1U);
      auto const& savedObject = image.objects.front();
      auto const sourceObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              sourceObjectInstance);
      REQUIRE(sourceObjectValue.has_value());
      REQUIRE(savedObject.handle == *sourceObjectValue);
      REQUIRE(savedObject.attributeValuesPresent);
      REQUIRE(savedObject.attributeValues.size() == 1U);
      auto const sourceAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(sourceAttribute);
      REQUIRE(sourceAttributeValue.has_value());
      REQUIRE(savedObject.attributeValues.front().attributeHandle ==
              *sourceAttributeValue);
      REQUIRE(savedObject.attributeValues.front().value == expectedValue);

      REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(receiver->disconnect());
      REQUIRE_NOTHROW(owner->disconnect());
    }

    {
      auto const freshRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador receiverReports;
      auto owner = makeRti();
      auto receiver = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, configuration));
      REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel, configuration));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      FederateHandle freshReceiverHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-application-value-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(freshReceiverHandle = receiver->joinFederationExecution(
          L"public-application-value-receiver", L"subscriber", federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);
      REQUIRE(freshReceiverHandle == sourceReceiverHandle);

      auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
      REQUIRE(filesAfterFreshJoin.size() == 4U);
      std::vector<std::filesystem::path> freshReportFiles;
      for (auto const& file : filesAfterFreshJoin) {
        if (std::find(sourceReportFiles.begin(), sourceReportFiles.end(), file) ==
            sourceReportFiles.end()) {
          freshReportFiles.push_back(file);
        }
      }
      REQUIRE(freshReportFiles.size() == 2U);
      std::map<std::filesystem::path, std::size_t> freshInitialSizes;
      for (auto const& file : freshReportFiles) {
        freshInitialSizes.emplace(file, readTextFile(file).size());
      }

      auto const freshObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshAttribute = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_child);
      REQUIRE(freshObjectClass == sourceObjectClass);
      REQUIRE(freshAttribute == sourceAttribute);
      AttributeHandleSet const attributes{freshAttribute};
      REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshObjectClass, attributes));

      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      drainBoth(*owner, *receiver);
      REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
              std::vector<std::wstring>{saveLabel});
      REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
      REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
      REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
      REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);
      REQUIRE_NOTHROW(owner->federateRestoreComplete());
      REQUIRE_NOTHROW(receiver->federateRestoreComplete());
      drainBoth(*owner, *receiver);
      REQUIRE(ownerReports.federationRestoredReportCount == 1U);
      REQUIRE(receiverReports.federationRestoredReportCount == 1U);

      REQUIRE_NOTHROW(owner->requestFederationSave(afterRestoreLabel));
      drainBoth(*owner, *receiver);
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(receiver->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(receiver->federateSaveComplete());
      drainBoth(*owner, *receiver);
      auto const afterRestore = saveStore->load(
          federationName, afterRestoreLabel);
      REQUIRE(afterRestore.has_value());
      auto const afterRestoreImage =
          umbra::detail::FederationStateImageCodec::decode(
              afterRestore->stateImage);
      REQUIRE(afterRestoreImage.objects.size() == 1U);
      auto const& restoredObject = afterRestoreImage.objects.front();
      auto const restoredObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              sourceObjectInstance);
      REQUIRE(restoredObjectValue.has_value());
      REQUIRE(restoredObject.handle == *restoredObjectValue);
      REQUIRE(restoredObject.attributeValuesPresent);
      REQUIRE(restoredObject.attributeValues.size() == 1U);
      REQUIRE(restoredObject.attributeValues.front().value == expectedValue);

      receiverReports.attributeReflectionReports.clear();
      AttributeHandleValueMap nextValues;
      nextValues.emplace(
          freshAttribute,
          VariableLengthData(nextValueBytes.data(), nextValueBytes.size()));
      VariableLengthData nextTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, nextValues, nextTag));
      drain(*receiver);
      REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
      REQUIRE(variableLengthDataBytes(
                  receiverReports.attributeReflectionReports.front()
                      .attributeValues.at(freshAttribute)) == nextValueBytes);

      REQUIRE(serviceReportFiles(reportDirectory.path()) == filesAfterFreshJoin);
      for (auto const& file : freshReportFiles) {
        auto const report = readTextFile(file);
        REQUIRE(report.size() >= freshInitialSizes.at(file));
      }

      REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(receiver->disconnect());
      REQUIRE_NOTHROW(owner->disconnect());
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
