#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {

void runQueuedTsoIdentityRebindingScenario(bool const freshRegistryBoundary) {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();
  std::wstring const saveLabel = L"queued-tso-member-identity-rebinding";
  unsigned char const parameterBytes[] = {0x5A, 0x52};
  unsigned char const tagBytes[] = {0x53, 0x4E, 0x50};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drainBoth = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  FederateHandle savedPublisherHandle;
  FederateHandle savedReceiverHandle;
  rti1516_2025::MessageRetractionHandle retraction;
  auto const restoreAndCheckQueuedInteraction = [&] (
      RTIambassador& publisher,
      RTIambassador& receiver,
      ReportingFederateAmbassador& publisherReports,
      ReportingFederateAmbassador& receiverReports,
      FederateHandle const& livePublisherHandle,
      FederateHandle const& liveReceiverHandle) {
    auto const livePublisherInteraction = publisher.getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    auto const liveReceiverInteraction = receiver.getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    auto const liveReceiverIdentifier = receiver.getParameterHandle(
        liveReceiverInteraction, fixture_hla::fixture::identifier);
    REQUIRE(livePublisherInteraction.isValid());
    REQUIRE(liveReceiverInteraction.isValid());
    REQUIRE(liveReceiverIdentifier.isValid());

    REQUIRE_NOTHROW(publisher.requestFederationRestore(saveLabel));
    drainBoth(publisher, receiver);
    REQUIRE(publisherReports.requestFederationRestoreFailedReports.empty());
    REQUIRE(publisherReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
    auto const restoredPublisher = std::find_if(
        publisherReports.initiateFederateRestoreReports.begin(),
        publisherReports.initiateFederateRestoreReports.end(),
        [](auto const& report) {
          return report.federateName == L"saved-queued-tso-publisher";
        });
    auto const restoredReceiver = std::find_if(
        receiverReports.initiateFederateRestoreReports.begin(),
        receiverReports.initiateFederateRestoreReports.end(),
        [](auto const& report) {
          return report.federateName == L"saved-queued-tso-receiver";
        });
    REQUIRE(restoredPublisher !=
            publisherReports.initiateFederateRestoreReports.end());
    REQUIRE(restoredReceiver !=
            receiverReports.initiateFederateRestoreReports.end());
    REQUIRE(restoredPublisher->postRestoreFederateHandle == livePublisherHandle);
    REQUIRE(restoredReceiver->postRestoreFederateHandle == liveReceiverHandle);
    REQUIRE_NOTHROW(publisher.federateRestoreComplete());
    REQUIRE_NOTHROW(receiver.federateRestoreComplete());
    drainBoth(publisher, receiver);
    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.timestampedInteractionReports.empty());

    receiverReports.callbackOrder.clear();
    REQUIRE_NOTHROW(receiver.flushQueueRequest(
        rti1516_2025::HLAinteger64Time(7)));
    drainBoth(publisher, receiver);
    REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
    REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(receiverReports.callbackOrder ==
            std::vector<std::string>{"interaction", "flush-grant"});

    auto const& delivered = receiverReports.timestampedInteractionReports.front();
    REQUIRE(delivered.interactionClass == liveReceiverInteraction);
    REQUIRE(delivered.producingFederate == livePublisherHandle);
    REQUIRE(delivered.parameterValues.size() == 1U);
    REQUIRE(variableLengthDataBytes(delivered.parameterValues.at(
                liveReceiverIdentifier)) ==
            std::vector<unsigned char>{0x5A, 0x52});
    REQUIRE(variableLengthDataBytes(delivered.userSuppliedTag) ==
            std::vector<unsigned char>{0x53, 0x4E, 0x50});
    REQUIRE(delivered.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(delivered.timeValue == L"7");
    REQUIRE(delivered.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(delivered.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE_FALSE(delivered.sentRegionsSupplied);
    REQUIRE(delivered.retractionSupplied);
    REQUIRE(delivered.retractionValid);
    REQUIRE(retraction.isValid());

    REQUIRE_NOTHROW(publisher.retract(retraction));
    drainBoth(publisher, receiver);
    REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
    REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
    REQUIRE(variableLengthDataBytes(
                receiverReports.requestRetractionReports.front().encodedRetraction) ==
            variableLengthDataBytes(retraction.encode()));
  };

  {
    auto const sourceStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, sourceStore);
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
        L"saved-queued-tso-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(savedReceiverHandle = receiver->joinFederationExecution(
        L"saved-queued-tso-receiver", L"participant", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    auto const identifier = publisher->getParameterHandle(
        interactionClass, fixture_hla::fixture::identifier);
    REQUIRE(interactionClass.isValid());
    REQUIRE(identifier.isValid());
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
        interactionClass, rti1516_2025::TIMESTAMP));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    drainBoth(*publisher, *receiver);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(5)));
    drainBoth(*publisher, *receiver);

    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        identifier, VariableLengthData(parameterBytes, sizeof(parameterBytes)));
    REQUIRE_NOTHROW(retraction = publisher->sendInteraction(
        interactionClass,
        parameterValues,
        tag,
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverReports.timestampedInteractionReports.empty());

    // Keep the receiver below the queued timestamp while making it eligible
    // to participate in the untimed save.
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(1)));
    drainBoth(*publisher, *receiver);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");
    REQUIRE(receiverReports.timestampedInteractionReports.empty());

    REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
    // The untimed save is admitted at the constrained receiver's next time
    // advance boundary. This target is below the initial GALT and the queued
    // interaction at 7, so the save can leave no pending TAR in its image.
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    drainBoth(*publisher, *receiver);
    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drainBoth(*publisher, *receiver);
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.timestampedInteractionReports.empty());

    if (freshRegistryBoundary) {
      REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(receiver->disconnect());
      REQUIRE_NOTHROW(publisher->disconnect());
    } else {
      REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->joinFederationExecution(
          L"burned-publisher-identity", L"participant", federationName));
      REQUIRE_NOTHROW(receiver->joinFederationExecution(
          L"burned-receiver-identity", L"participant", federationName));
      REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
      FederateHandle livePublisherHandle;
      FederateHandle liveReceiverHandle;
      REQUIRE_NOTHROW(livePublisherHandle = publisher->joinFederationExecution(
          L"live-queued-tso-publisher", L"participant", federationName));
      REQUIRE_NOTHROW(liveReceiverHandle = receiver->joinFederationExecution(
          L"live-queued-tso-receiver", L"participant", federationName));
      REQUIRE(livePublisherHandle != savedPublisherHandle);
      REQUIRE(liveReceiverHandle != savedReceiverHandle);

      restoreAndCheckQueuedInteraction(
          *publisher,
          *receiver,
          publisherReports,
          receiverReports,
          livePublisherHandle,
          liveReceiverHandle);

      REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(receiver->disconnect());
      REQUIRE_NOTHROW(publisher->disconnect());
    }
  }

  if (freshRegistryBoundary) {
    auto const freshStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, freshStore);
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
        L"burned-publisher-identity", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"burned-receiver-identity", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    FederateHandle livePublisherHandle;
    FederateHandle liveReceiverHandle;
    REQUIRE_NOTHROW(livePublisherHandle = publisher->joinFederationExecution(
        L"live-queued-tso-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(liveReceiverHandle = receiver->joinFederationExecution(
        L"live-queued-tso-receiver", L"participant", federationName));
    REQUIRE(livePublisherHandle != savedPublisherHandle);
    REQUIRE(liveReceiverHandle != savedReceiverHandle);

    restoreAndCheckQueuedInteraction(
        *publisher,
        *receiver,
        publisherReports,
        receiverReports,
        livePublisherHandle,
        liveReceiverHandle);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }
}

}  // namespace

TEST_CASE(
    "Embedded public fresh-registry restore rebinds one queued timestamped interaction by member identity",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][fresh-registry][two-federate][time-management][tso]"
    "[restore-member-identity-rebinding][callback-evoked]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.change-interaction-order-type][rti.service.send-interaction]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.flush-queue-grant][federate.callback.time-advance-grant]") {
  runQueuedTsoIdentityRebindingScenario(true);
}

TEST_CASE(
    "Embedded public process-local snapshot restore rebinds one queued timestamped interaction by member identity",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][process-local-snapshot][two-federate][time-management][tso]"
    "[restore-member-identity-rebinding][callback-evoked]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.change-interaction-order-type][rti.service.send-interaction]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.flush-queue-grant][federate.callback.time-advance-grant]") {
  runQueuedTsoIdentityRebindingScenario(false);
}
