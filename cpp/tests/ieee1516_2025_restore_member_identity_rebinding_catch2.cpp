#include "ieee1516_2025_public_federation_restore_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "internal/runtime/ambassador_connection_support.hpp"
#include "internal/federation/federation_state_image.hpp"

TEST_CASE(
    "Embedded public restore rebinds member-only saved state by federate type",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][fresh-registry][multi-federate]"
    "[restore-member-identity-rebinding]"
    "[rti.service.enable-time-regulation][rti.service.time-advance-request]"
    "[rti.service.modify-lookahead]"
    "[rti.service.query-logical-time][rti.service.query-lookahead]"
    "[rti.service.join-federation-execution][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const saveLabel = std::wstring{L"member-type-identity-rebinding"};
  auto const drainBoth = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  FederateHandle savedFirstHandle;
  FederateHandle savedSecondHandle;
  rti1516_2025::HLAinteger64Time savedLogicalTime;
  rti1516_2025::HLAinteger64Interval savedLookahead;
  {
    auto const sourceStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, sourceStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador firstReports;
    ReportingFederateAmbassador secondReports;
    auto first = makeRti();
    auto second = makeRti();

    REQUIRE_NOTHROW(first->connect(
        firstReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(second->connect(
        secondReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(first->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedFirstHandle = first->joinFederationExecution(
        L"saved-first", L"participant", federationName));
    REQUIRE_NOTHROW(savedSecondHandle = second->joinFederationExecution(
        L"saved-second", L"participant", federationName));

    REQUIRE_NOTHROW(first->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(2)));
    REQUIRE_NOTHROW(first->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(4)));
    drainBoth(*first, *second);
    REQUIRE_NOTHROW(first->queryLogicalTime(savedLogicalTime));
    REQUIRE(savedLogicalTime.getTime() == 4);
    REQUIRE_NOTHROW(first->queryLookahead(savedLookahead));
    REQUIRE(savedLookahead.getInterval() == 2);

    REQUIRE_NOTHROW(first->requestFederationSave(saveLabel));
    drainBoth(*first, *second);
    REQUIRE(firstReports.initiateFederateSaveReports.size() == 1U);
    REQUIRE(secondReports.initiateFederateSaveReports.size() == 1U);
    REQUIRE_NOTHROW(first->federateSaveBegun());
    REQUIRE_NOTHROW(second->federateSaveBegun());
    REQUIRE_NOTHROW(first->federateSaveComplete());
    REQUIRE_NOTHROW(second->federateSaveComplete());
    drainBoth(*first, *second);
    REQUIRE(firstReports.federationSavedReportCount == 1U);
    REQUIRE(secondReports.federationSavedReportCount == 1U);

    REQUIRE_NOTHROW(first->modifyLookahead(
        rti1516_2025::HLAinteger64Interval(5)));
    REQUIRE_NOTHROW(first->queryLookahead(savedLookahead));
    REQUIRE(savedLookahead.getInterval() == 5);

    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(second->disconnect());
    REQUIRE_NOTHROW(first->disconnect());
  }

  {
    auto const freshStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, freshStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador firstReports;
    ReportingFederateAmbassador secondReports;
    auto first = makeRti();
    auto second = makeRti();

    REQUIRE_NOTHROW(first->connect(
        firstReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(second->connect(
        secondReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(first->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(first->joinFederationExecution(
        L"identity-burn-first", L"participant", federationName));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(second->joinFederationExecution(
        L"identity-burn-second", L"participant", federationName));
    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
    FederateHandle restoredFirstHandle;
    FederateHandle restoredSecondHandle;
    REQUIRE_NOTHROW(restoredFirstHandle = first->joinFederationExecution(
        L"restored-first", L"participant", federationName));
    REQUIRE_NOTHROW(restoredSecondHandle = second->joinFederationExecution(
        L"restored-second", L"participant", federationName));
    REQUIRE(restoredFirstHandle != savedFirstHandle);
    REQUIRE(restoredSecondHandle != savedSecondHandle);

    REQUIRE_NOTHROW(first->requestFederationRestore(saveLabel));
    drainBoth(*first, *second);
    REQUIRE(firstReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(firstReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(secondReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(firstReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-first");
    REQUIRE(secondReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-second");
    REQUIRE(firstReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
            restoredFirstHandle);
    REQUIRE(secondReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
            restoredSecondHandle);
    REQUIRE_NOTHROW(first->federateRestoreComplete());
    REQUIRE_NOTHROW(second->federateRestoreComplete());
    drainBoth(*first, *second);
    REQUIRE(firstReports.federationRestoredReportCount == 1U);
    REQUIRE(secondReports.federationRestoredReportCount == 1U);

    rti1516_2025::HLAinteger64Time restoredLogicalTime;
    rti1516_2025::HLAinteger64Interval restoredLookahead;
    REQUIRE_NOTHROW(first->queryLogicalTime(restoredLogicalTime));
    REQUIRE(restoredLogicalTime.getTime() == 4);
    REQUIRE_NOTHROW(first->queryLookahead(restoredLookahead));
    REQUIRE(restoredLookahead.getInterval() == 2);

    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(second->disconnect());
    REQUIRE_NOTHROW(first->disconnect());
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds one member-owned object-class publication declaration",
    "[integration][development-profile][federation-management][object-management]"
    "[declaration-management][save-restore][durable-save][filesystem][fresh-registry][restore]"
    "[restore-member-identity-rebinding]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[requirement:hla-1516.1-2025:clause-4.1.2][2025]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration =
      configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  std::wstring const saveLabel = L"single-object-publication-declaration-rebind";
  std::vector<unsigned char> const updateBytes{0x6FU, 0x62U, 0x6AU, 0x01U};
  auto const drain = [](
                          rti1516_2025::RTIambassador& first,
                          rti1516_2025::RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  FederateHandle savedPublisherHandle;
  FederateHandle savedReceiverHandle;
  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedPublisherHandle = publisher->joinFederationExecution(
        L"saved-object-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(savedReceiverHandle = receiver->joinFederationExecution(
        L"saved-object-receiver", L"participant", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));

    REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.objects.empty());
    REQUIRE(image.objectClassDeclarationCount == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.size() == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.front().classes.size() == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.front()
                .classes.front().explicitlyPublishedAttributeHandles.size() == 1U);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"identity-burn-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"identity-burn-receiver", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    FederateHandle livePublisherHandle;
    FederateHandle liveReceiverHandle;
    REQUIRE_NOTHROW(livePublisherHandle = publisher->joinFederationExecution(
        L"live-object-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(liveReceiverHandle = receiver->joinFederationExecution(
        L"live-object-receiver", L"participant", federationName));
    REQUIRE(livePublisherHandle != savedPublisherHandle);
    REQUIRE(liveReceiverHandle != savedReceiverHandle);

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(publisherReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-object-publisher");
    REQUIRE(publisherReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == livePublisherHandle);
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);

    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
    REQUIRE(objectInstance.isValid());
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);

    AttributeHandleValueMap values;
    values.emplace(
        attribute,
        VariableLengthData(updateBytes.data(), updateBytes.size()));
    VariableLengthData emptyTag;
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance, values, emptyTag));
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
    REQUIRE(variableLengthDataBytes(
                receiverReports.attributeReflectionReports.front()
                    .attributeValues.at(attribute)) == updateBytes);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }
}

TEST_CASE(
    "Embedded public restore rejects equal-total membership with different federate-type counts",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][fresh-registry][multi-federate]"
    "[restore-member-identity-rebinding]"
    "[rti.service.join-federation-execution][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-failed]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[requirement:hla-1516.1-2025:clause-4.1.2][2025]") {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const saveLabel = std::wstring{L"member-type-count-mismatch"};
  auto const drainBoth = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  {
    auto const sourceStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, sourceStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador firstReports;
    ReportingFederateAmbassador secondReports;
    auto first = makeRti();
    auto second = makeRti();

    REQUIRE_NOTHROW(first->connect(
        firstReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(second->connect(
        secondReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(first->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(first->joinFederationExecution(
        L"saved-first", L"participant", federationName));
    REQUIRE_NOTHROW(second->joinFederationExecution(
        L"saved-second", L"participant", federationName));

    REQUIRE_NOTHROW(first->requestFederationSave(saveLabel));
    drainBoth(*first, *second);
    REQUIRE(firstReports.initiateFederateSaveReports.size() == 1U);
    REQUIRE(secondReports.initiateFederateSaveReports.size() == 1U);
    REQUIRE_NOTHROW(first->federateSaveBegun());
    REQUIRE_NOTHROW(second->federateSaveBegun());
    REQUIRE_NOTHROW(first->federateSaveComplete());
    REQUIRE_NOTHROW(second->federateSaveComplete());
    drainBoth(*first, *second);
    REQUIRE(firstReports.federationSavedReportCount == 1U);
    REQUIRE(secondReports.federationSavedReportCount == 1U);

    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(second->disconnect());
    REQUIRE_NOTHROW(first->disconnect());
  }

  {
    auto const freshStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, freshStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador firstReports;
    ReportingFederateAmbassador secondReports;
    auto first = makeRti();
    auto second = makeRti();

    REQUIRE_NOTHROW(first->connect(
        firstReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(second->connect(
        secondReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(first->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    // The total remains two, but the saved {participant: 2} distribution is
    // deliberately changed to {participant: 1, observer: 1}.
    REQUIRE_NOTHROW(first->joinFederationExecution(
        L"restored-first", L"participant", federationName));
    REQUIRE_NOTHROW(second->joinFederationExecution(
        L"restored-second", L"observer", federationName));

    REQUIRE_NOTHROW(first->requestFederationRestore(saveLabel));
    drainBoth(*first, *second);
    REQUIRE(firstReports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(firstReports.requestFederationRestoreFailedReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(secondReports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(secondReports.requestFederationRestoreFailedReports.empty());
    REQUIRE(firstReports.federationRestoreBegunReportCount == 0U);
    REQUIRE(secondReports.federationRestoreBegunReportCount == 0U);
    REQUIRE(firstReports.initiateFederateRestoreReports.empty());
    REQUIRE(secondReports.initiateFederateRestoreReports.empty());
    REQUIRE(firstReports.federationRestoredReportCount == 0U);
    REQUIRE(secondReports.federationRestoredReportCount == 0U);
    REQUIRE(firstReports.federationNotRestoredReasons.empty());
    REQUIRE(secondReports.federationNotRestoredReasons.empty());

    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(second->disconnect());
    REQUIRE_NOTHROW(first->disconnect());
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds a route-free interaction declaration for one member",
    "[integration][development-profile][federation-management][interaction-management]"
    "[declaration-management][save-restore][durable-save][filesystem][fresh-registry][restore]"
    "[restore-member-identity-rebinding]"
    "[callback-immediate]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration =
      configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"single-member-interaction-declaration-rebind";
  auto const drain = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  FederateHandle savedSubscriberHandle;
  FederateHandle savedProducerHandle;
  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador subscriberReports;
    ReportingFederateAmbassador producerReports;
    auto subscriber = makeRti();
    auto producer = makeRti();

    REQUIRE_NOTHROW(subscriber->connect(
        subscriberReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(producer->connect(
        producerReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(subscriber->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedSubscriberHandle = subscriber->joinFederationExecution(
        L"saved-interaction-subscriber", L"participant", federationName));
    REQUIRE_NOTHROW(savedProducerHandle = producer->joinFederationExecution(
        L"saved-interaction-producer", L"participant", federationName));

    auto const savedInteraction = subscriber->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    REQUIRE(savedInteraction.isValid());
    REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(savedInteraction));

    REQUIRE_NOTHROW(subscriber->requestFederationSave(saveLabel));
    drain(*subscriber, *producer);
    REQUIRE(subscriberReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(producerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(subscriber->federateSaveBegun());
    REQUIRE_NOTHROW(producer->federateSaveBegun());
    REQUIRE_NOTHROW(subscriber->federateSaveComplete());
    REQUIRE_NOTHROW(producer->federateSaveComplete());
    drain(*subscriber, *producer);
    REQUIRE(subscriberReports.federationSavedReportCount == 1U);
    REQUIRE(producerReports.federationSavedReportCount == 1U);

    REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(subscriber->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(producer->disconnect());
    REQUIRE_NOTHROW(subscriber->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador subscriberReports;
    ReportingFederateAmbassador producerReports;
    auto subscriber = makeRti();
    auto producer = makeRti();

    REQUIRE_NOTHROW(subscriber->connect(
        subscriberReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(producer->connect(
        producerReports,
        rti1516_2025::HLA_IMMEDIATE,
        configuration));
    REQUIRE_NOTHROW(subscriber->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"identity-burn-subscriber", L"participant", federationName));
    REQUIRE_NOTHROW(producer->joinFederationExecution(
        L"identity-burn-producer", L"participant", federationName));
    REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    FederateHandle liveSubscriberHandle;
    FederateHandle liveProducerHandle;
    REQUIRE_NOTHROW(liveSubscriberHandle = subscriber->joinFederationExecution(
        L"live-interaction-subscriber", L"participant", federationName));
    REQUIRE_NOTHROW(liveProducerHandle = producer->joinFederationExecution(
        L"live-interaction-producer", L"participant", federationName));
    REQUIRE(liveSubscriberHandle != savedSubscriberHandle);
    REQUIRE(liveProducerHandle != savedProducerHandle);

    auto const liveInteraction = subscriber->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    REQUIRE(liveInteraction.isValid());
    REQUIRE_NOTHROW(subscriber->requestFederationRestore(saveLabel));
    drain(*subscriber, *producer);
    REQUIRE(subscriberReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(producerReports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(subscriberReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(producerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(subscriberReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(producerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(subscriberReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-interaction-subscriber");
    REQUIRE(producerReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-interaction-producer");
    REQUIRE(subscriberReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
            liveSubscriberHandle);
    REQUIRE(producerReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
            liveProducerHandle);
    REQUIRE_NOTHROW(subscriber->federateRestoreComplete());
    REQUIRE_NOTHROW(producer->federateRestoreComplete());
    drain(*subscriber, *producer);
    REQUIRE(subscriberReports.federationRestoredReportCount == 1U);
    REQUIRE(producerReports.federationRestoredReportCount == 1U);

    REQUIRE_NOTHROW(producer->publishInteractionClass(liveInteraction));
    subscriberReports.interactionReports.clear();
    REQUIRE_NOTHROW(producer->sendInteraction(
        liveInteraction,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    drain(*subscriber, *producer);
    REQUIRE(subscriberReports.interactionReports.size() == 1U);
    REQUIRE(subscriberReports.interactionReports.front().interactionClass == liveInteraction);
    REQUIRE(subscriberReports.interactionReports.front().producingFederate ==
            liveProducerHandle);

    REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(subscriber->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(producer->disconnect());
    REQUIRE_NOTHROW(subscriber->disconnect());
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds one saved named object instance identity",
    "[integration][development-profile][federation-management][object-management]"
    "[save-restore][durable-save][filesystem][fresh-registry][restore]"
    "[restore-member-identity-rebinding][callback-evoked]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.reserve-object-instance-name][rti.service.register-object-instance]"
    "[rti.service.get-object-instance-handle][rti.service.get-object-instance-name]"
    "[rti.service.get-known-object-class-handle][rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[requirement:hla-1516.1-2025:clause-4.1.2][2025]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration =
      configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  std::wstring const saveLabel = L"single-object-instance-identity-rebind";
  std::wstring const afterRestoreLabel =
      L"single-object-instance-identity-rebind-after-restore";
  std::wstring const objectName = L"identity-rebound-object";
  std::vector<unsigned char> const savedValueBytes{0x72U, 0x73U, 0x01U};
  std::vector<unsigned char> const updatedValueBytes{0x72U, 0x73U, 0x02U};
  std::string const savedValue(
      reinterpret_cast<char const*>(savedValueBytes.data()),
      savedValueBytes.size());
  auto const drain = [](
                          RTIambassador& first,
                          RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  FederateHandle savedPublisherHandle;
  FederateHandle savedReceiverHandle;
  ObjectInstanceHandle savedObjectInstance;
  std::uint64_t savedPublisherId = 0U;
  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedPublisherHandle = publisher->joinFederationExecution(
        L"saved-object-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(savedReceiverHandle = receiver->joinFederationExecution(
        L"saved-object-receiver", L"participant", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));
    REQUIRE_NOTHROW(publisher->reserveObjectInstanceName(objectName));
    REQUIRE_NOTHROW(savedObjectInstance =
                        publisher->registerObjectInstance(objectClass, objectName));
    REQUIRE(savedObjectInstance.isValid());
    REQUIRE(publisher->getObjectInstanceName(savedObjectInstance) == objectName);
    REQUIRE(publisher->getObjectInstanceHandle(objectName) == savedObjectInstance);

    AttributeHandleValueMap values;
    values.emplace(
        attribute,
        VariableLengthData(savedValueBytes.data(), savedValueBytes.size()));
    VariableLengthData emptyTag;
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        savedObjectInstance, values, emptyTag));
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.empty());
    REQUIRE(receiverReports.attributeReflectionReports.empty());

    REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.objectInstanceCount == 1U);
    REQUIRE(image.objects.size() == 1U);
    REQUIRE(image.objectClassDeclarationCount == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.size() == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.front().classes.size() == 1U);
    auto const savedAttributeValue =
        rti1516_2025::umbra_binding_detail::attributeHandleValue(attribute);
    REQUIRE(savedAttributeValue.has_value());
    REQUIRE(image.objectClassAttributeDeclarations.front()
                .classes.front().explicitlyPublishedAttributeHandles ==
            std::vector<std::uint64_t>{*savedAttributeValue});
    auto const& savedObject = image.objects.front();
    auto const savedObjectValue =
        rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
            savedObjectInstance);
    REQUIRE(savedObjectValue.has_value());
    REQUIRE(savedObject.handle == *savedObjectValue);
    REQUIRE(savedObject.name == objectName);
    REQUIRE(savedObject.producingFederateId ==
            image.objectClassAttributeDeclarations.front().federateId);
    savedPublisherId = savedObject.producingFederateId;
    REQUIRE(savedObject.attributeValuesPresent);
    REQUIRE(savedObject.attributeValues.size() == 1U);
    REQUIRE(savedObject.attributeValues.front().value == savedValue);
    REQUIRE(savedObject.attributes.size() >= 1U);
    bool foundSavedValueOwner = false;
    for (auto const& savedAttribute : savedObject.attributes) {
      REQUIRE((savedAttribute.ownerFederateId == 0U ||
               savedAttribute.ownerFederateId == savedPublisherId));
      REQUIRE(savedAttribute.updateRegionHandles.empty());
      if (savedAttribute.handle == *savedAttributeValue) {
        foundSavedValueOwner = true;
        REQUIRE(savedAttribute.ownerFederateId == savedPublisherId);
      }
    }
    REQUIRE(foundSavedValueOwner);
    REQUIRE(savedObject.knownObjectClassHandlesByFederate.size() == 1U);
    REQUIRE(savedObject.knownObjectClassHandlesByFederate.front().federateId ==
            savedPublisherId);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"identity-burn-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"identity-burn-receiver", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    FederateHandle livePublisherHandle;
    FederateHandle liveReceiverHandle;
    REQUIRE_NOTHROW(livePublisherHandle = publisher->joinFederationExecution(
        L"live-object-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(liveReceiverHandle = receiver->joinFederationExecution(
        L"live-object-receiver", L"participant", federationName));
    REQUIRE(livePublisherHandle != savedPublisherHandle);
    REQUIRE(liveReceiverHandle != savedReceiverHandle);

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(publisherReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(publisherReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(publisherReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-object-publisher");
    REQUIRE(receiverReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-object-receiver");
    REQUIRE(publisherReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == livePublisherHandle);
    REQUIRE(receiverReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == liveReceiverHandle);
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);

    REQUIRE(publisher->getObjectInstanceHandle(objectName) == savedObjectInstance);
    REQUIRE(publisher->getObjectInstanceName(savedObjectInstance) == objectName);
    REQUIRE(publisher->getKnownObjectClassHandle(savedObjectInstance) == objectClass);

    REQUIRE_NOTHROW(publisher->requestFederationSave(afterRestoreLabel));
    drain(*publisher, *receiver);
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisher, *receiver);
    auto const afterRestore = saveStore->load(
        federationName, afterRestoreLabel);
    REQUIRE(afterRestore.has_value());
    auto const afterRestoreImage =
        umbra::detail::FederationStateImageCodec::decode(
            afterRestore->stateImage);
    REQUIRE(afterRestoreImage.objects.size() == 1U);
    REQUIRE(afterRestoreImage.objectClassAttributeDeclarations.size() == 1U);
    auto const& restoredObject = afterRestoreImage.objects.front();
    auto const& restoredPublication =
        afterRestoreImage.objectClassAttributeDeclarations.front();
    REQUIRE(restoredPublication.federateId != savedPublisherId);
    REQUIRE(restoredObject.handle ==
            rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
                savedObjectInstance));
    REQUIRE(restoredObject.name == objectName);
    REQUIRE(restoredObject.producingFederateId == restoredPublication.federateId);
    REQUIRE(restoredObject.attributeValuesPresent);
    REQUIRE(restoredObject.attributeValues.size() == 1U);
    REQUIRE(restoredObject.attributeValues.front().value == savedValue);
    REQUIRE(restoredObject.attributes.size() >= 1U);
    bool foundRestoredValueOwner = false;
    for (auto const& restoredAttribute : restoredObject.attributes) {
      REQUIRE((restoredAttribute.ownerFederateId == 0U ||
               restoredAttribute.ownerFederateId == restoredPublication.federateId));
      REQUIRE(restoredAttribute.updateRegionHandles.empty());
      if (restoredAttribute.handle ==
          restoredObject.attributeValues.front().attributeHandle) {
        foundRestoredValueOwner = true;
        REQUIRE(restoredAttribute.ownerFederateId ==
                restoredPublication.federateId);
      }
    }
    REQUIRE(foundRestoredValueOwner);
    REQUIRE(restoredObject.knownObjectClassHandlesByFederate.size() == 1U);
    REQUIRE(restoredObject.knownObjectClassHandlesByFederate.front().federateId ==
            restoredPublication.federateId);

    AttributeHandleSet const attributes{attribute};
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
        objectClass, attributes));
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance ==
            savedObjectInstance);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectClass == objectClass);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstanceName == objectName);
    REQUIRE(receiverReports.objectDiscoveryReports.front().producingFederate ==
            livePublisherHandle);
    REQUIRE(receiver->getObjectInstanceHandle(objectName) == savedObjectInstance);
    REQUIRE(receiver->getObjectInstanceName(savedObjectInstance) == objectName);
    REQUIRE(receiver->getKnownObjectClassHandle(savedObjectInstance) == objectClass);

    receiverReports.attributeReflectionReports.clear();
    AttributeHandleValueMap nextValues;
    nextValues.emplace(
        attribute,
        VariableLengthData(updatedValueBytes.data(), updatedValueBytes.size()));
    VariableLengthData nextTag;
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        savedObjectInstance, nextValues, nextTag));
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverReports.attributeReflectionReports.front().objectInstance ==
            savedObjectInstance);
    REQUIRE(receiverReports.attributeReflectionReports.front().producingFederate ==
            livePublisherHandle);
    REQUIRE(variableLengthDataBytes(
                receiverReports.attributeReflectionReports.front()
                    .attributeValues.at(attribute)) == updatedValueBytes);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore rebinds two saved named object instances",
    "[integration][development-profile][federation-management][object-management]"
    "[save-restore][durable-save][filesystem][fresh-registry][restore]"
    "[restore-member-identity-rebinding][callback-evoked]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.reserve-object-instance-name][rti.service.register-object-instance]"
    "[rti.service.get-object-instance-handle][rti.service.get-object-instance-name]"
    "[rti.service.get-known-object-class-handle][rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[requirement:hla-1516.1-2025:clause-4.1.2][2025]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration =
      configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  std::wstring const saveLabel = L"two-object-instance-identity-rebind";
  std::wstring const afterRestoreLabel =
      L"two-object-instance-identity-rebind-after-restore";
  std::vector<std::wstring> const objectNames{
      L"identity-rebound-object-a",
      L"identity-rebound-object-b",
  };
  std::vector<std::vector<unsigned char>> const savedValueBytes{
      {0x72U, 0x73U, 0x11U},
      {0x72U, 0x73U, 0x12U},
  };
  std::vector<std::vector<unsigned char>> const updatedValueBytes{
      {0x72U, 0x73U, 0x21U},
      {0x72U, 0x73U, 0x22U},
  };
  std::vector<std::string> savedValues;
  for (auto const& value : savedValueBytes) {
    savedValues.emplace_back(
        reinterpret_cast<char const*>(value.data()), value.size());
  }
  auto const drain = [](
                          RTIambassador& first,
                          RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };
  auto const findObjectByName = [](
                                    std::vector<umbra::detail::FederationStateImageObject> const& objects,
                                    std::wstring const& name)
      -> umbra::detail::FederationStateImageObject const* {
    for (auto const& object : objects) {
      if (object.name == name) {
        return &object;
      }
    }
    return nullptr;
  };

  FederateHandle savedPublisherHandle;
  FederateHandle savedReceiverHandle;
  std::vector<ObjectInstanceHandle> savedObjectInstances;
  std::uint64_t savedPublisherId = 0U;
  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedPublisherHandle = publisher->joinFederationExecution(
        L"saved-two-object-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(savedReceiverHandle = receiver->joinFederationExecution(
        L"saved-two-object-receiver", L"participant", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    auto const savedAttributeValue =
        rti1516_2025::umbra_binding_detail::attributeHandleValue(attribute);
    REQUIRE(savedAttributeValue.has_value());
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));

    for (auto const& objectName : objectNames) {
      REQUIRE_NOTHROW(publisher->reserveObjectInstanceName(objectName));
      ObjectInstanceHandle objectInstance;
      REQUIRE_NOTHROW(objectInstance =
                          publisher->registerObjectInstance(objectClass, objectName));
      REQUIRE(objectInstance.isValid());
      REQUIRE(publisher->getObjectInstanceName(objectInstance) == objectName);
      REQUIRE(publisher->getObjectInstanceHandle(objectName) == objectInstance);
      savedObjectInstances.push_back(objectInstance);
    }
    REQUIRE(savedObjectInstances.size() == 2U);
    REQUIRE(savedObjectInstances[0] != savedObjectInstances[1]);

    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      AttributeHandleValueMap values;
      values.emplace(
          attribute,
          VariableLengthData(
              savedValueBytes[index].data(), savedValueBytes[index].size()));
      VariableLengthData emptyTag;
      REQUIRE_NOTHROW(publisher->updateAttributeValues(
          savedObjectInstances[index], values, emptyTag));
    }
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.empty());
    REQUIRE(receiverReports.attributeReflectionReports.empty());

    REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.objectInstanceCount == 2U);
    REQUIRE(image.objects.size() == 2U);
    REQUIRE(image.objectClassDeclarationCount == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.size() == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.front().classes.size() == 1U);
    REQUIRE(image.objectClassAttributeDeclarations.front()
                .classes.front().explicitlyPublishedAttributeHandles ==
            std::vector<std::uint64_t>{*savedAttributeValue});
    savedPublisherId =
        image.objectClassAttributeDeclarations.front().federateId;
    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      auto const* savedObject = findObjectByName(image.objects, objectNames[index]);
      REQUIRE(savedObject != nullptr);
      auto const savedObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              savedObjectInstances[index]);
      REQUIRE(savedObjectValue.has_value());
      REQUIRE(savedObject->handle == *savedObjectValue);
      REQUIRE(savedObject->registeredObjectClassHandle ==
              image.objectClassAttributeDeclarations.front()
                  .classes.front().objectClassHandle);
      REQUIRE(savedObject->producingFederateId == savedPublisherId);
      REQUIRE(savedObject->name == objectNames[index]);
      REQUIRE(savedObject->attributeValuesPresent);
      REQUIRE(savedObject->attributeValues.size() == 1U);
      REQUIRE(savedObject->attributeValues.front().attributeHandle ==
              *savedAttributeValue);
      REQUIRE(savedObject->attributeValues.front().value == savedValues[index]);
      REQUIRE(savedObject->knownObjectClassHandlesByFederate.size() == 1U);
      REQUIRE(savedObject->knownObjectClassHandlesByFederate.front().federateId ==
              savedPublisherId);
      REQUIRE(savedObject->knownObjectClassHandlesByFederate.front()
                  .objectClassHandle ==
              savedObject->registeredObjectClassHandle);
      bool foundSavedValueOwner = false;
      for (auto const& savedAttribute : savedObject->attributes) {
        REQUIRE((savedAttribute.ownerFederateId == 0U ||
                 savedAttribute.ownerFederateId == savedPublisherId));
        REQUIRE(savedAttribute.updateRegionHandles.empty());
        if (savedAttribute.handle == *savedAttributeValue) {
          foundSavedValueOwner = true;
          REQUIRE(savedAttribute.ownerFederateId == savedPublisherId);
        }
      }
      REQUIRE(foundSavedValueOwner);
    }

    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"identity-burn-two-object-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"identity-burn-two-object-receiver", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    FederateHandle livePublisherHandle;
    FederateHandle liveReceiverHandle;
    REQUIRE_NOTHROW(livePublisherHandle = publisher->joinFederationExecution(
        L"live-two-object-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(liveReceiverHandle = receiver->joinFederationExecution(
        L"live-two-object-receiver", L"participant", federationName));
    REQUIRE(livePublisherHandle != savedPublisherHandle);
    REQUIRE(liveReceiverHandle != savedReceiverHandle);

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(publisherReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(publisherReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(publisherReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-two-object-publisher");
    REQUIRE(receiverReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-two-object-receiver");
    REQUIRE(publisherReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == livePublisherHandle);
    REQUIRE(receiverReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == liveReceiverHandle);
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);

    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      REQUIRE(publisher->getObjectInstanceHandle(objectNames[index]) ==
              savedObjectInstances[index]);
      REQUIRE(publisher->getObjectInstanceName(savedObjectInstances[index]) ==
              objectNames[index]);
      REQUIRE(publisher->getKnownObjectClassHandle(savedObjectInstances[index]) ==
              objectClass);
    }

    REQUIRE_NOTHROW(publisher->requestFederationSave(afterRestoreLabel));
    drain(*publisher, *receiver);
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisher, *receiver);
    auto const afterRestore = saveStore->load(
        federationName, afterRestoreLabel);
    REQUIRE(afterRestore.has_value());
    auto const afterRestoreImage =
        umbra::detail::FederationStateImageCodec::decode(
            afterRestore->stateImage);
    REQUIRE(afterRestoreImage.objectInstanceCount == 2U);
    REQUIRE(afterRestoreImage.objects.size() == 2U);
    REQUIRE(afterRestoreImage.objectClassAttributeDeclarations.size() == 1U);
    auto const& restoredPublication =
        afterRestoreImage.objectClassAttributeDeclarations.front();
    REQUIRE(restoredPublication.federateId != savedPublisherId);
    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      auto const* restoredObject =
          findObjectByName(afterRestoreImage.objects, objectNames[index]);
      REQUIRE(restoredObject != nullptr);
      auto const restoredObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              savedObjectInstances[index]);
      REQUIRE(restoredObjectValue.has_value());
      REQUIRE(restoredObject->handle == *restoredObjectValue);
      REQUIRE(restoredObject->name == objectNames[index]);
      REQUIRE(restoredObject->producingFederateId == restoredPublication.federateId);
      REQUIRE(restoredObject->attributeValuesPresent);
      REQUIRE(restoredObject->attributeValues.size() == 1U);
      REQUIRE(restoredObject->attributeValues.front().value == savedValues[index]);
      REQUIRE(restoredObject->attributes.size() >= 1U);
      bool foundRestoredValueOwner = false;
      for (auto const& restoredAttribute : restoredObject->attributes) {
        REQUIRE((restoredAttribute.ownerFederateId == 0U ||
                 restoredAttribute.ownerFederateId == restoredPublication.federateId));
        REQUIRE(restoredAttribute.updateRegionHandles.empty());
        if (restoredAttribute.handle ==
            restoredObject->attributeValues.front().attributeHandle) {
          foundRestoredValueOwner = true;
          REQUIRE(restoredAttribute.ownerFederateId ==
                  restoredPublication.federateId);
        }
      }
      REQUIRE(foundRestoredValueOwner);
      REQUIRE(restoredObject->knownObjectClassHandlesByFederate.size() == 1U);
      REQUIRE(restoredObject->knownObjectClassHandlesByFederate.front().federateId ==
              restoredPublication.federateId);
      REQUIRE(restoredObject->knownObjectClassHandlesByFederate.front()
                  .objectClassHandle ==
              restoredObject->registeredObjectClassHandle);
    }

    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 2U);
    // Subscription may report current values for the restored instances;
    // isolate the callbacks generated by the explicit updates below.
    receiverReports.attributeReflectionReports.clear();
    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      bool foundDiscovery = false;
      for (auto const& discovery : receiverReports.objectDiscoveryReports) {
        if (discovery.objectInstanceName == objectNames[index]) {
          foundDiscovery = true;
          REQUIRE(discovery.objectInstance == savedObjectInstances[index]);
          REQUIRE(discovery.objectClass == objectClass);
          REQUIRE(discovery.producingFederate == livePublisherHandle);
        }
      }
      REQUIRE(foundDiscovery);
      REQUIRE(receiver->getObjectInstanceHandle(objectNames[index]) ==
              savedObjectInstances[index]);
      REQUIRE(receiver->getObjectInstanceName(savedObjectInstances[index]) ==
              objectNames[index]);
      REQUIRE(receiver->getKnownObjectClassHandle(savedObjectInstances[index]) ==
              objectClass);
    }

    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      AttributeHandleValueMap nextValues;
      nextValues.emplace(
          attribute,
          VariableLengthData(
              updatedValueBytes[index].data(), updatedValueBytes[index].size()));
      VariableLengthData nextTag;
      REQUIRE_NOTHROW(publisher->updateAttributeValues(
          savedObjectInstances[index], nextValues, nextTag));
      drain(*publisher, *receiver);
      REQUIRE(receiverReports.attributeReflectionReports.size() == index + 1U);
      auto const& reflection =
          receiverReports.attributeReflectionReports.back();
      REQUIRE(reflection.objectInstance == savedObjectInstances[index]);
      REQUIRE(reflection.producingFederate == livePublisherHandle);
      REQUIRE(variableLengthDataBytes(
                  reflection.attributeValues.at(attribute)) == updatedValueBytes[index]);
    }

    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore independently rebinds two named object producers",
    "[integration][development-profile][federation-management][object-management]"
    "[save-restore][durable-save][filesystem][fresh-registry][restore]"
    "[restore-member-identity-rebinding][callback-evoked]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.reserve-object-instance-name][rti.service.register-object-instance]"
    "[rti.service.get-object-instance-handle][rti.service.get-object-instance-name]"
    "[rti.service.get-known-object-class-handle][rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[requirement:hla-1516.1-2025:clause-4.1.2][2025]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration =
      configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  std::wstring const saveLabel = L"two-producer-object-identity-rebind";
  std::wstring const afterRestoreLabel =
      L"two-producer-object-identity-rebind-after-restore";
  std::vector<std::wstring> const savedMemberNames{
      L"saved-object-publisher-a",
      L"saved-object-publisher-b",
      L"saved-object-receiver",
  };
  std::vector<std::wstring> const liveMemberNames{
      L"live-object-publisher-a",
      L"live-object-publisher-b",
      L"live-object-receiver",
  };
  std::vector<std::wstring> const burnMemberNames{
      L"burn-object-publisher-a",
      L"burn-object-publisher-b",
      L"burn-object-receiver",
  };
  std::vector<std::wstring> const objectNames{
      L"independently-rebound-object-a",
      L"independently-rebound-object-b",
  };
  std::vector<std::vector<unsigned char>> const savedValueBytes{
      {0x74U, 0x75U, 0x31U},
      {0x74U, 0x75U, 0x32U},
  };
  std::vector<std::vector<unsigned char>> const updatedValueBytes{
      {0x74U, 0x75U, 0x41U},
      {0x74U, 0x75U, 0x42U},
  };
  std::vector<std::string> savedValues;
  for (auto const& value : savedValueBytes) {
    savedValues.emplace_back(
        reinterpret_cast<char const*>(value.data()), value.size());
  }
  auto const drain = [](
                          RTIambassador& publisherA,
                          RTIambassador& publisherB,
                          RTIambassador& receiver) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(publisherA.evokeCallback(0.0));
      static_cast<void>(publisherB.evokeCallback(0.0));
      static_cast<void>(receiver.evokeCallback(0.0));
    }
  };
  auto const findMemberId = [](
                                umbra::detail::FederationStateImage const& image,
                                std::wstring const& name) {
    for (auto const& member : image.members) {
      if (member.name == name) {
        return member.id;
      }
    }
    return std::uint64_t{0};
  };
  auto const findObjectByName = [](
                                    std::vector<umbra::detail::FederationStateImageObject> const& objects,
                                    std::wstring const& name)
      -> umbra::detail::FederationStateImageObject const* {
    for (auto const& object : objects) {
      if (object.name == name) {
        return &object;
      }
    }
    return nullptr;
  };
  auto const findPublication = [](
                                   umbra::detail::FederationStateImage const& image,
                                   std::uint64_t federateId)
      -> umbra::detail::FederationStateImageObjectClassAttributeDeclarations const* {
    for (auto const& declaration : image.objectClassAttributeDeclarations) {
      if (declaration.federateId == federateId) {
        return &declaration;
      }
    }
    return nullptr;
  };

  std::vector<FederateHandle> savedFederateHandles(3U);
  std::vector<FederateHandle> liveFederateHandles(3U);
  std::vector<ObjectInstanceHandle> savedObjectInstances(2U);
  std::vector<std::uint64_t> savedPublisherIds(2U);
  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador publisherAReports;
    ReportingFederateAmbassador publisherBReports;
    ReportingFederateAmbassador receiverReports;
    auto publisherA = makeRti();
    auto publisherB = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisherA->connect(
        publisherAReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisherB->connect(
        publisherBReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisherA->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedFederateHandles[0] = publisherA->joinFederationExecution(
        savedMemberNames[0], L"participant", federationName));
    REQUIRE_NOTHROW(savedFederateHandles[1] = publisherB->joinFederationExecution(
        savedMemberNames[1], L"participant", federationName));
    REQUIRE_NOTHROW(savedFederateHandles[2] = receiver->joinFederationExecution(
        savedMemberNames[2], L"participant", federationName));

    auto const objectClass = publisherA->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisherA->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    auto const savedAttributeValue =
        rti1516_2025::umbra_binding_detail::attributeHandleValue(attribute);
    REQUIRE(savedAttributeValue.has_value());
    REQUIRE_NOTHROW(publisherA->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));
    REQUIRE_NOTHROW(publisherB->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));

    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      RTIambassador& owner = index == 0U ? *publisherA : *publisherB;
      REQUIRE_NOTHROW(owner.reserveObjectInstanceName(objectNames[index]));
      REQUIRE_NOTHROW(savedObjectInstances[index] =
                          owner.registerObjectInstance(objectClass, objectNames[index]));
      REQUIRE(savedObjectInstances[index].isValid());
      REQUIRE(owner.getObjectInstanceName(savedObjectInstances[index]) ==
              objectNames[index]);
      REQUIRE(owner.getObjectInstanceHandle(objectNames[index]) ==
              savedObjectInstances[index]);

      AttributeHandleValueMap values;
      values.emplace(
          attribute,
          VariableLengthData(
              savedValueBytes[index].data(), savedValueBytes[index].size()));
      VariableLengthData emptyTag;
      REQUIRE_NOTHROW(owner.updateAttributeValues(
          savedObjectInstances[index], values, emptyTag));
    }
    REQUIRE(savedObjectInstances[0] != savedObjectInstances[1]);
    drain(*publisherA, *publisherB, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.empty());
    REQUIRE(receiverReports.attributeReflectionReports.empty());

    REQUIRE_NOTHROW(publisherA->requestFederationSave(saveLabel));
    drain(*publisherA, *publisherB, *receiver);
    REQUIRE(publisherAReports.initiateFederateSaveReports.size() == 1U);
    REQUIRE(publisherBReports.initiateFederateSaveReports.size() == 1U);
    REQUIRE(receiverReports.initiateFederateSaveReports.size() == 1U);
    REQUIRE_NOTHROW(publisherA->federateSaveBegun());
    REQUIRE_NOTHROW(publisherB->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisherA->federateSaveComplete());
    REQUIRE_NOTHROW(publisherB->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisherA, *publisherB, *receiver);
    REQUIRE(publisherAReports.federationSavedReportCount == 1U);
    REQUIRE(publisherBReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.members.size() == 3U);
    REQUIRE(image.objectInstanceCount == 2U);
    REQUIRE(image.objects.size() == 2U);
    REQUIRE(image.objectClassDeclarationCount == 2U);
    REQUIRE(image.objectClassAttributeDeclarations.size() == 2U);
    for (std::size_t index = 0; index != savedPublisherIds.size(); ++index) {
      savedPublisherIds[index] = findMemberId(image, savedMemberNames[index]);
      REQUIRE(savedPublisherIds[index] != 0U);
      auto const* publication = findPublication(image, savedPublisherIds[index]);
      REQUIRE(publication != nullptr);
      REQUIRE(publication->classes.size() == 1U);
      REQUIRE(publication->classes.front().explicitlyPublishedAttributeHandles ==
              std::vector<std::uint64_t>{*savedAttributeValue});
    }
    REQUIRE(savedPublisherIds[0] != savedPublisherIds[1]);
    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      auto const* object = findObjectByName(image.objects, objectNames[index]);
      REQUIRE(object != nullptr);
      auto const handleValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              savedObjectInstances[index]);
      REQUIRE(handleValue.has_value());
      REQUIRE(object->handle == *handleValue);
      REQUIRE(object->name == objectNames[index]);
      REQUIRE(object->producingFederateId == savedPublisherIds[index]);
      REQUIRE(object->attributeValues.size() == 1U);
      REQUIRE(object->attributeValues.front().attributeHandle ==
              *savedAttributeValue);
      REQUIRE(object->attributeValues.front().value ==
              std::string(
                  reinterpret_cast<char const*>(savedValueBytes[index].data()),
                  savedValueBytes[index].size()));
      REQUIRE(object->knownObjectClassHandlesByFederate.size() == 1U);
      REQUIRE(object->knownObjectClassHandlesByFederate.front().federateId ==
              savedPublisherIds[index]);
      REQUIRE(object->knownObjectClassHandlesByFederate.front().objectClassHandle ==
              object->registeredObjectClassHandle);
      auto const* publication = findPublication(image, savedPublisherIds[index]);
      REQUIRE(publication != nullptr);
      REQUIRE(publication->classes.front().objectClassHandle ==
              object->registeredObjectClassHandle);
      bool foundValueOwner = false;
      for (auto const& objectAttribute : object->attributes) {
        REQUIRE((objectAttribute.ownerFederateId == 0U ||
                 objectAttribute.ownerFederateId == savedPublisherIds[index]));
        REQUIRE(objectAttribute.updateRegionHandles.empty());
        if (objectAttribute.handle == *savedAttributeValue) {
          foundValueOwner = true;
          REQUIRE(objectAttribute.ownerFederateId == savedPublisherIds[index]);
        }
      }
      REQUIRE(foundValueOwner);
    }

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisherB->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisherA->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisherA->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisherB->disconnect());
    REQUIRE_NOTHROW(publisherA->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador publisherAReports;
    ReportingFederateAmbassador publisherBReports;
    ReportingFederateAmbassador receiverReports;
    auto publisherA = makeRti();
    auto publisherB = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisherA->connect(
        publisherAReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisherB->connect(
        publisherBReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisherA->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    for (std::size_t index = 0; index != burnMemberNames.size(); ++index) {
      RTIambassador& federate =
          index == 0U ? *publisherA : index == 1U ? *publisherB : *receiver;
      REQUIRE_NOTHROW(federate.joinFederationExecution(
          burnMemberNames[index], L"participant", federationName));
    }
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisherB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisherA->resignFederationExecution(NO_ACTION));
    for (std::size_t index = 0; index != liveMemberNames.size(); ++index) {
      RTIambassador& federate =
          index == 0U ? *publisherA : index == 1U ? *publisherB : *receiver;
      REQUIRE_NOTHROW(liveFederateHandles[index] = federate.joinFederationExecution(
          liveMemberNames[index], L"participant", federationName));
      REQUIRE(liveFederateHandles[index] != savedFederateHandles[index]);
    }

    auto const objectClass = publisherA->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = publisherA->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    auto const liveAttributeValue =
        rti1516_2025::umbra_binding_detail::attributeHandleValue(attribute);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(liveAttributeValue.has_value());
    REQUIRE_NOTHROW(publisherA->requestFederationRestore(saveLabel));
    drain(*publisherA, *publisherB, *receiver);
    REQUIRE(publisherAReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherBReports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(receiverReports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(publisherAReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(publisherBReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(publisherAReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(publisherBReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(publisherAReports.initiateFederateRestoreReports.front().federateName ==
            savedMemberNames[0]);
    REQUIRE(publisherBReports.initiateFederateRestoreReports.front().federateName ==
            savedMemberNames[1]);
    REQUIRE(receiverReports.initiateFederateRestoreReports.front().federateName ==
            savedMemberNames[2]);
    REQUIRE(publisherAReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == liveFederateHandles[0]);
    REQUIRE(publisherBReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == liveFederateHandles[1]);
    REQUIRE(receiverReports.initiateFederateRestoreReports.front()
                .postRestoreFederateHandle == liveFederateHandles[2]);
    REQUIRE_NOTHROW(publisherA->federateRestoreComplete());
    REQUIRE_NOTHROW(publisherB->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    drain(*publisherA, *publisherB, *receiver);
    REQUIRE(publisherAReports.federationRestoredReportCount == 1U);
    REQUIRE(publisherBReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);

    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      RTIambassador& owner = index == 0U ? *publisherA : *publisherB;
      REQUIRE(owner.getObjectInstanceHandle(objectNames[index]) ==
              savedObjectInstances[index]);
      REQUIRE(owner.getObjectInstanceName(savedObjectInstances[index]) ==
              objectNames[index]);
      REQUIRE(owner.getKnownObjectClassHandle(savedObjectInstances[index]) ==
              objectClass);
    }

    REQUIRE_NOTHROW(publisherA->requestFederationSave(afterRestoreLabel));
    drain(*publisherA, *publisherB, *receiver);
    REQUIRE_NOTHROW(publisherA->federateSaveBegun());
    REQUIRE_NOTHROW(publisherB->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisherA->federateSaveComplete());
    REQUIRE_NOTHROW(publisherB->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisherA, *publisherB, *receiver);
    auto const afterRestore = saveStore->load(
        federationName, afterRestoreLabel);
    REQUIRE(afterRestore.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        afterRestore->stateImage);
    REQUIRE(image.objectInstanceCount == 2U);
    REQUIRE(image.objects.size() == 2U);
    REQUIRE(image.objectClassDeclarationCount == 2U);
    REQUIRE(image.objectClassAttributeDeclarations.size() == 2U);
    std::vector<std::uint64_t> livePublisherIds(2U);
    for (std::size_t index = 0; index != livePublisherIds.size(); ++index) {
      livePublisherIds[index] = findMemberId(image, liveMemberNames[index]);
      REQUIRE(livePublisherIds[index] != 0U);
      REQUIRE(livePublisherIds[index] != savedPublisherIds[index]);
      auto const* publication = findPublication(image, livePublisherIds[index]);
      REQUIRE(publication != nullptr);
      REQUIRE(publication->classes.size() == 1U);
      REQUIRE(publication->classes.front().explicitlyPublishedAttributeHandles ==
              std::vector<std::uint64_t>{*liveAttributeValue});
    }
    REQUIRE(livePublisherIds[0] != livePublisherIds[1]);
    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      auto const* object = findObjectByName(image.objects, objectNames[index]);
      REQUIRE(object != nullptr);
      REQUIRE(object->producingFederateId == livePublisherIds[index]);
      REQUIRE(object->attributeValues.size() == 1U);
      REQUIRE(object->attributeValues.front().value == savedValues[index]);
      REQUIRE(object->knownObjectClassHandlesByFederate.size() == 1U);
      REQUIRE(object->knownObjectClassHandlesByFederate.front().federateId ==
              livePublisherIds[index]);
      bool foundValueOwner = false;
      for (auto const& objectAttribute : object->attributes) {
        REQUIRE((objectAttribute.ownerFederateId == 0U ||
                 objectAttribute.ownerFederateId == livePublisherIds[index]));
        REQUIRE(objectAttribute.updateRegionHandles.empty());
        if (objectAttribute.handle == object->attributeValues.front().attributeHandle) {
          foundValueOwner = true;
          REQUIRE(objectAttribute.ownerFederateId == livePublisherIds[index]);
        }
      }
      REQUIRE(foundValueOwner);
    }

    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));
    drain(*publisherA, *publisherB, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 2U);
    receiverReports.attributeReflectionReports.clear();
    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      bool foundDiscovery = false;
      for (auto const& discovery : receiverReports.objectDiscoveryReports) {
        if (discovery.objectInstanceName == objectNames[index]) {
          foundDiscovery = true;
          REQUIRE(discovery.objectInstance == savedObjectInstances[index]);
          REQUIRE(discovery.objectClass == objectClass);
          REQUIRE(discovery.producingFederate == liveFederateHandles[index]);
        }
      }
      REQUIRE(foundDiscovery);
      REQUIRE(receiver->getObjectInstanceHandle(objectNames[index]) ==
              savedObjectInstances[index]);
      REQUIRE(receiver->getKnownObjectClassHandle(savedObjectInstances[index]) ==
              objectClass);
    }

    for (std::size_t index = 0; index != objectNames.size(); ++index) {
      RTIambassador& owner = index == 0U ? *publisherA : *publisherB;
      AttributeHandleValueMap nextValues;
      nextValues.emplace(
          attribute,
          VariableLengthData(
              updatedValueBytes[index].data(), updatedValueBytes[index].size()));
      VariableLengthData nextTag;
      REQUIRE_NOTHROW(owner.updateAttributeValues(
          savedObjectInstances[index], nextValues, nextTag));
      drain(*publisherA, *publisherB, *receiver);
      REQUIRE(receiverReports.attributeReflectionReports.size() == index + 1U);
      auto const& reflection = receiverReports.attributeReflectionReports.back();
      REQUIRE(reflection.objectInstance == savedObjectInstances[index]);
      REQUIRE(reflection.producingFederate == liveFederateHandles[index]);
      REQUIRE(variableLengthDataBytes(
                  reflection.attributeValues.at(attribute)) ==
              updatedValueBytes[index]);
    }

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisherB->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisherA->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisherA->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisherB->disconnect());
    REQUIRE_NOTHROW(publisherA->disconnect());
  }
}
