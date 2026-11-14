#include "tck_restore_save_after_federation_destruction.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void verifyRestoreAfterFederationExecutionDestruction(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Restore after federation-execution destruction requires an adapter FOM and time implementation");
  Session originalOwner(options, model, "restore-persistence-owner");
  Session originalPeer(options, model, "restore-persistence-peer");
  auto const federation =
      federationName(options, "restore-save-after-execution-destruction");
  auto const ownerName = options.ownerFederateName + L"-restore-save-after-destruction";
  auto const peerName = options.memberFederateName + L"-restore-save-after-destruction";
  std::wstring const saveLabel = L"tck-save-after-execution-destruction";
  std::wstring const synchronizationLabel =
      L"tck-sync-point-after-execution-destruction";
  std::array<std::uint8_t, 2> const tagBytes{'P', 'D'};
  auto const expectedTag = std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  originalOwner.connect();
  originalPeer.connect();
  originalOwner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  originalOwner.join(ownerName, options.federateType, federation);
  originalPeer.join(peerName, options.federateType, federation);
  rti::FederateHandleSet const synchronizationSet{
      originalOwner.federateHandle(), originalPeer.federateHandle()};
  originalOwner.rtiAmbassador().registerFederationSynchronizationPoint(
      synchronizationLabel, tag, synchronizationSet);
  waitForSessions({&originalOwner, &originalPeer}, [&] {
    return originalOwner.synchronizationPointRegistrations().size() == 1U &&
           originalOwner.synchronizationPointAnnouncements().size() == 1U &&
           originalPeer.synchronizationPointAnnouncements().size() == 1U;
  }, options, "persistent synchronization-point registration");
  auto const registrations = originalOwner.synchronizationPointRegistrations();
  auto const ownerAnnouncements = originalOwner.synchronizationPointAnnouncements();
  auto const peerAnnouncements = originalPeer.synchronizationPointAnnouncements();
  require(registrations.front().succeeded &&
              registrations.front().label == synchronizationLabel &&
              ownerAnnouncements.front().label == synchronizationLabel &&
              ownerAnnouncements.front().tag == expectedTag &&
              peerAnnouncements.front().label == synchronizationLabel &&
              peerAnnouncements.front().tag == expectedTag,
          "The synchronization point was not announced before the federation save");

  originalOwner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&originalOwner, &originalPeer}, [&] {
    return originalOwner.federateSaveInitiations().size() == 1U &&
           originalPeer.federateSaveInitiations().size() == 1U;
  }, options, "persistent synchronization-point save initiation");
  require(originalOwner.federateSaveInitiations().front() == saveLabel &&
              originalPeer.federateSaveInitiations().front() == saveLabel,
          "The persistent synchronization-point save used a different label");
  originalOwner.rtiAmbassador().federateSaveBegun();
  originalOwner.rtiAmbassador().federateSaveComplete();
  originalPeer.rtiAmbassador().federateSaveBegun();
  originalPeer.rtiAmbassador().federateSaveComplete();
  waitForSessions({&originalOwner, &originalPeer}, [&] {
    return originalOwner.federationSavedCount() == 1U &&
           originalPeer.federationSavedCount() == 1U;
  }, options, "persistent synchronization-point federation saved");

  originalPeer.resign(rti::NO_ACTION);
  originalOwner.resign(rti::NO_ACTION);
  originalOwner.rtiAmbassador().destroyFederationExecution(federation);
  originalPeer.disconnect();
  originalOwner.disconnect();

  Session restoredOwner(options, model, "restore-persistence-restored-owner");
  Session restoredPeer(options, model, "restore-persistence-restored-peer");
  restoredOwner.connect();
  restoredPeer.connect();
  restoredOwner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  restoredOwner.join(ownerName, options.federateType, federation);
  restoredPeer.join(peerName, options.federateType, federation);
  restoredOwner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&restoredOwner, &restoredPeer}, [&] {
    return !restoredOwner.recorder().federationRestoreRequestsFailed().empty() ||
           (!restoredOwner.recorder().federationRestoreRequestsSucceeded().empty() &&
            restoredOwner.federateRestoreInitiations().size() == 1U &&
            restoredPeer.federateRestoreInitiations().size() == 1U);
  }, options, "restore request after federation execution destruction");
  require(restoredOwner.recorder().federationRestoreRequestsFailed().empty(),
          "The post-destruction restore request was rejected");
  auto const restoreRequestsSucceeded =
      restoredOwner.recorder().federationRestoreRequestsSucceeded();
  require(restoreRequestsSucceeded.size() == 1U &&
              restoreRequestsSucceeded.front() == saveLabel &&
              restoredOwner.federateRestoreInitiations().front().label == saveLabel &&
              restoredPeer.federateRestoreInitiations().front().label == saveLabel,
          "The post-destruction restore used an unexpected save label");
  restoredOwner.rtiAmbassador().federateRestoreComplete();
  restoredPeer.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&restoredOwner, &restoredPeer}, [&] {
    return restoredOwner.federationRestoredCount() == 1U &&
           restoredPeer.federationRestoredCount() == 1U;
  }, options, "federation restored after execution destruction");
  require(restoredOwner.federationNotRestoredReasons().empty() &&
              restoredPeer.federationNotRestoredReasons().empty(),
          "The saved synchronization-point state failed to restore");

  restoredOwner.rtiAmbassador().synchronizationPointAchieved(synchronizationLabel);
  restoredPeer.rtiAmbassador().synchronizationPointAchieved(synchronizationLabel);
  waitForSessions({&restoredOwner, &restoredPeer}, [&] {
    return restoredOwner.federationSynchronized().size() == 1U &&
           restoredPeer.federationSynchronized().size() == 1U;
  }, options, "saved synchronization point completed after execution recreation");
  auto const ownerCompletions = restoredOwner.federationSynchronized();
  auto const peerCompletions = restoredPeer.federationSynchronized();
  require(ownerCompletions.front().label == synchronizationLabel &&
              ownerCompletions.front().failedToSyncSet.empty() &&
              peerCompletions.front().label == synchronizationLabel &&
              peerCompletions.front().failedToSyncSet.empty(),
          "The pre-destruction synchronization point did not survive restore");

  restoredPeer.resign(rti::NO_ACTION);
  restoredOwner.resign(rti::NO_ACTION);
  restoredOwner.rtiAmbassador().destroyFederationExecution(federation);
  restoredPeer.disconnect();
  restoredOwner.disconnect();
}

void verifyQueuedReceiveOrderInteractionAcrossSaveRestore(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Queued receive-order save/restore evidence is scoped to HLA_EVOKED");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Queued receive-order save/restore requires an adapter FOM and time implementation");

  auto const federation =
      federationName(options, "save-restore-queued-ro-interaction");
  auto const publisherName =
      options.ownerFederateName + L"-queued-ro-save";
  auto const subscriberName =
      options.memberFederateName + L"-queued-ro-save";
  std::wstring const saveLabel = L"tck-save-queued-ro-interaction";
  std::vector<std::uint8_t> const parameterBytes{0x52U, 0x4FU, 0x49U};
  std::vector<std::uint8_t> const tagBytes{0x53U, 0x41U, 0x56U, 0x45U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  {
    Session publisher(options, model, "owner");
    Session subscriber(options, model, "member");
    publisher.connect();
    subscriber.connect();
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        options.fom.wstring(),
        options.logicalTimeImplementationName);
    publisher.join(publisherName, options.federateType, federation);
    subscriber.join(subscriberName, options.federateType, federation);

    auto const publisherInteraction =
        publisher.rtiAmbassador().getInteractionClassHandle(
            options.preferredReceiveInteractionClassName);
    auto const subscriberInteraction =
        subscriber.rtiAmbassador().getInteractionClassHandle(
            options.preferredReceiveInteractionClassName);
    auto const publisherParameter =
        publisher.rtiAmbassador().getParameterHandle(
            publisherInteraction, options.parameterName);
    auto const subscriberParameter =
        subscriber.rtiAmbassador().getParameterHandle(
            subscriberInteraction, options.parameterName);
    require(
        publisherInteraction.isValid() && subscriberInteraction.isValid() &&
            publisherParameter.isValid() && subscriberParameter.isValid(),
        "The adapter FOM omitted the selected receive-order interaction or parameter");
    publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
    subscriber.rtiAmbassador().subscribeInteractionClass(
        subscriberInteraction, true);
    for (int pass = 0; pass != 32; ++pass) {
      publisher.pump();
      subscriber.pump();
    }

    rti::VariableLengthData const parameter(
        parameterBytes.data(), parameterBytes.size());
    rti::ParameterHandleValueMap const parameters{
        {publisherParameter, parameter}};
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction, parameters, tag);
    require(
        subscriber.recorder().interactions().empty(),
        "The receive-order message was delivered before the federation save request");

    publisher.rtiAmbassador().requestFederationSave(saveLabel);
    waitForSessions(
        {&publisher, &subscriber},
        [&] {
          return !publisher.federateSaveInitiations().empty() &&
              !subscriber.federateSaveInitiations().empty();
        },
        options,
        "queued receive-order federation save initiation");
    require(
        publisher.federateSaveInitiations().size() == 1U &&
            publisher.federateSaveInitiations().front() == saveLabel &&
            subscriber.federateSaveInitiations().size() == 1U &&
            subscriber.federateSaveInitiations().front() == saveLabel,
        "The queued receive-order save used an unexpected label");
    publisher.rtiAmbassador().federateSaveBegun();
    subscriber.rtiAmbassador().federateSaveBegun();
    publisher.rtiAmbassador().federateSaveComplete();
    subscriber.rtiAmbassador().federateSaveComplete();
    waitForSessions(
        {&publisher, &subscriber},
        [&] {
          return publisher.federationSavedCount() == 1U &&
              subscriber.federationSavedCount() == 1U;
        },
        options,
        "queued receive-order federation save completion");
    require(
        publisher.federationNotSavedReasons().empty() &&
            subscriber.federationNotSavedReasons().empty(),
        "The queued receive-order federation save reported failure");

    subscriber.resign(rti::NO_ACTION);
    publisher.resign(rti::NO_ACTION);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    subscriber.disconnect();
    publisher.disconnect();
  }

  {
    Session publisher(options, model, "owner");
    Session subscriber(options, model, "member");
    publisher.connect();
    subscriber.connect();
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        options.fom.wstring(),
        options.logicalTimeImplementationName);
    publisher.join(publisherName, options.federateType, federation);
    subscriber.join(subscriberName, options.federateType, federation);

    auto const subscriberInteraction =
        subscriber.rtiAmbassador().getInteractionClassHandle(
            options.preferredReceiveInteractionClassName);
    auto const subscriberParameter =
        subscriber.rtiAmbassador().getParameterHandle(
            subscriberInteraction, options.parameterName);
    require(
        subscriberInteraction.isValid() && subscriberParameter.isValid(),
        "The restored FOM omitted the selected receive-order interaction or parameter");
    require(
        subscriber.recorder().interactions().empty(),
        "A queued interaction was delivered before the saved federation was restored");

    publisher.rtiAmbassador().requestFederationRestore(saveLabel);
    waitForSessions(
        {&publisher},
        [&] {
          return !publisher.federationRestoreRequestsSucceeded().empty() ||
              !publisher.federationRestoreRequestsFailed().empty();
        },
        options,
        "queued receive-order federation restore request result");
    auto const restoreFailures = publisher.federationRestoreRequestsFailed();
    require(
        restoreFailures.empty(),
        "Queued receive-order restore was rejected with label " +
            (restoreFailures.empty() ? std::string{} : toNarrow(restoreFailures.front())));
    require(
        publisher.federationRestoreRequestsSucceeded().size() == 1U &&
            publisher.federationRestoreRequestsSucceeded().front() == saveLabel,
        "Queued receive-order restore succeeded with an unexpected label");
    waitForSessions(
        {&publisher, &subscriber},
        [&] {
          return !publisher.federateRestoreInitiations().empty() &&
              !subscriber.federateRestoreInitiations().empty();
        },
        options,
        "queued receive-order federation restore initiation callbacks");
    auto const publisherRestore = publisher.federateRestoreInitiations().front();
    auto const subscriberRestore = subscriber.federateRestoreInitiations().front();
    require(
        publisherRestore.label == saveLabel &&
            subscriberRestore.label == saveLabel &&
            publisherRestore.federateName == publisherName &&
            subscriberRestore.federateName == subscriberName &&
            publisherRestore.postRestoreFederateHandle.isValid() &&
            subscriberRestore.postRestoreFederateHandle.isValid(),
        "The queued receive-order restore did not identify both saved federates");
    publisher.rtiAmbassador().federateRestoreComplete();
    subscriber.rtiAmbassador().federateRestoreComplete();
    waitForSessions(
        {&publisher, &subscriber},
        [&] {
          return publisher.federationRestoredCount() == 1U &&
              subscriber.federationRestoredCount() == 1U &&
              subscriber.recorder().interactions().size() >= 1U;
        },
        options,
        "queued receive-order interaction delivery after restore");

    auto const restoredInteractions = subscriber.recorder().interactions();
    require(
        restoredInteractions.size() == 1U,
        "The saved receive-order interaction was not delivered exactly once after restore");
    auto const& restored = restoredInteractions.front();
    require(
        restored.present && restored.interaction == subscriberInteraction &&
            restored.producer == publisherRestore.postRestoreFederateHandle &&
            restored.parameters.size() == 1U &&
            restored.parameters.count(subscriberParameter) == 1U &&
            copyBytes(restored.parameters.at(subscriberParameter)) == parameterBytes &&
            restored.tag == tagBytes && restored.transportation.isValid() &&
            !restored.regions.has_value(),
        "The restored receive-order interaction changed its identity or payload");
    for (int pass = 0; pass != 32; ++pass) {
      publisher.pump();
      subscriber.pump();
    }
    require(
        subscriber.recorder().interactions().size() == 1U,
        "The restored receive-order interaction was delivered more than once");

    subscriber.resign(rti::NO_ACTION);
    publisher.resign(rti::NO_ACTION);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    subscriber.disconnect();
    publisher.disconnect();
  }
}

}  // namespace

void scenarioRestoreSaveAfterFederationDestruction(
    Options const& options,
    rti::CallbackModel model) {
  verifyRestoreAfterFederationExecutionDestruction(options, model);
}

void scenarioRestoreSaveAfterFederationDestructionContract(
    Options const& options,
    rti::CallbackModel model) {
  verifyRestoreAfterFederationExecutionDestruction(options, model);
}

void scenarioSaveRestoreQueuedReceiveOrderInteraction(
    Options const& options,
    rti::CallbackModel model) {
  verifyQueuedReceiveOrderInteractionAcrossSaveRestore(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
