#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

struct FederateSaveIdentity final {
  std::wstring federationSaveLabel;
  std::wstring federateType;
  rti::FederateHandle federateDesignator;
};

struct FederateSaveIdentityLess final {
  bool operator()(
      FederateSaveIdentity const& left,
      FederateSaveIdentity const& right) const {
    if (left.federationSaveLabel != right.federationSaveLabel) {
      return left.federationSaveLabel < right.federationSaveLabel;
    }
    if (left.federateType != right.federateType) {
      return left.federateType < right.federateType;
    }
    return left.federateDesignator < right.federateDesignator;
  }
};

using ApplicationSaveStore =
    std::map<FederateSaveIdentity, std::wstring, FederateSaveIdentityLess>;

void persistInitiatedFederateState(
    Session const& federate,
    std::wstring const& federateType,
    std::wstring const& expectedLabel,
    std::wstring const& applicationState,
    ApplicationSaveStore& store) {
  auto const initiatedSaves = federate.federateSaveInitiations();
  require(
      !initiatedSaves.empty() && initiatedSaves.back() == expectedLabel,
      "Federate save callback did not supply the requested federation save label");

  FederateSaveIdentity const identity{
      initiatedSaves.back(),
      federateType,
      federate.federateHandle(),
  };
  require(
      identity.federateDesignator.isValid(),
      "Join did not return a valid federate designator for application save identity");
  auto const inserted = store.emplace(identity, applicationState).second;
  require(
      inserted,
      "Federate save identity collided with an existing application-state record");
}

std::wstring restoreApplicationState(
    std::wstring const& federationSaveLabel,
    std::wstring const& federateType,
    rti::FederateHandle const& federateDesignator,
    ApplicationSaveStore const& store) {
  auto const savedState = store.find(FederateSaveIdentity{
      federationSaveLabel, federateType, federateDesignator});
  require(
      savedState != store.end(),
      "Application restore could not find state for the save label, federate type, and Join result");
  return savedState->second;
}

void runApplicationSaveIdentityCycle(
    Session& owner,
    Session& peer,
    std::wstring const& ownerType,
    std::wstring const& peerType,
    std::wstring const& saveLabel,
    std::wstring const& ownerState,
    std::wstring const& peerState,
    ApplicationSaveStore& store,
    Options const& options) {
  auto const ownerInitiationsBefore = owner.federateSaveInitiations().size();
  auto const peerInitiationsBefore = peer.federateSaveInitiations().size();
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federateSaveInitiations().size() > ownerInitiationsBefore &&
            peer.federateSaveInitiations().size() > peerInitiationsBefore;
      },
      options,
      "application save-identity initiation callbacks");

  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  persistInitiatedFederateState(
      owner, ownerType, saveLabel, ownerState, store);
  persistInitiatedFederateState(
      peer, peerType, saveLabel, peerState, store);

  auto const ownerSavedBefore = owner.federationSavedCount();
  auto const peerSavedBefore = peer.federationSavedCount();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationSavedCount() > ownerSavedBefore &&
            peer.federationSavedCount() > peerSavedBefore;
      },
      options,
      "application save-identity completion callbacks");
  require(
      owner.federationNotSavedReasons().empty() &&
          peer.federationNotSavedReasons().empty(),
      "Application save-identity scenario received a failed-save callback");
}

}  // namespace

void scenarioFederateSaveIdentity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federate save-identity witness requires an adapter-supplied FOM");

  Session owner(options, model, "application-save-identity-owner");
  Session peer(options, model, "application-save-identity-peer");
  auto const federation =
      federationName(options, "federate-application-save-identity");
  owner.connect();
  peer.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);

  std::wstring const ownerType =
      options.federateType + L"-application-save-category-owner";
  std::wstring const peerType =
      options.federateType + L"-application-save-category-peer";
  require(
      ownerType != peerType,
      "The sample federates did not use distinct save/restore categories");
  owner.join(options.ownerFederateName, ownerType, federation);
  peer.join(options.memberFederateName, peerType, federation);
  require(
      owner.federateHandle() != peer.federateHandle(),
      "Distinct joined federates received the same federate designator");
  auto const ownerJoinHandle = owner.federateHandle();
  auto const peerJoinHandle = peer.federateHandle();

  ApplicationSaveStore applicationState;
  std::wstring const firstLabel = L"tck-application-save-identity-first";
  std::wstring const secondLabel = L"tck-application-save-identity-second";
  runApplicationSaveIdentityCycle(
      owner,
      peer,
      ownerType,
      peerType,
      firstLabel,
      L"owner-state-first",
      L"peer-state-first",
      applicationState,
      options);
  runApplicationSaveIdentityCycle(
      owner,
      peer,
      ownerType,
      peerType,
      secondLabel,
      L"owner-state-second",
      L"peer-state-second",
      applicationState,
      options);

  require(
      applicationState.size() == 4U,
      "Save label, federate type, and designator did not distinguish the four application-state records");
  require(
      applicationState.at(FederateSaveIdentity{
          firstLabel, ownerType, ownerJoinHandle}) == L"owner-state-first" &&
          applicationState.at(FederateSaveIdentity{
              firstLabel, peerType, peerJoinHandle}) == L"peer-state-first" &&
          applicationState.at(FederateSaveIdentity{
              secondLabel, ownerType, ownerJoinHandle}) == L"owner-state-second" &&
          applicationState.at(FederateSaveIdentity{
              secondLabel, peerType, peerJoinHandle}) == L"peer-state-second",
      "Application save identity selected the wrong saved federate state");

  auto const ownerRestoreSuccessBefore =
      owner.federationRestoreRequestsSucceeded().size();
  auto const ownerRestoreBegunBefore = owner.federationRestoreBegunCount();
  auto const peerRestoreBegunBefore = peer.federationRestoreBegunCount();
  auto const ownerRestoreInitiationBefore =
      owner.federateRestoreInitiations().size();
  auto const peerRestoreInitiationBefore =
      peer.federateRestoreInitiations().size();
  owner.rtiAmbassador().requestFederationRestore(secondLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() >
                ownerRestoreSuccessBefore &&
            owner.federationRestoreBegunCount() > ownerRestoreBegunBefore &&
            peer.federationRestoreBegunCount() > peerRestoreBegunBefore &&
            owner.federateRestoreInitiations().size() >
                ownerRestoreInitiationBefore &&
            peer.federateRestoreInitiations().size() >
                peerRestoreInitiationBefore;
      },
      options,
      "application save-identity restore initiation callbacks");

  require(
      owner.federationRestoreRequestsSucceeded().back() == secondLabel,
      "Application restore request acknowledged the wrong save label");
  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const peerRestore = peer.federateRestoreInitiations().back();
  require(
      ownerRestore.label == secondLabel && peerRestore.label == secondLabel &&
          ownerRestore.federateName == options.ownerFederateName &&
          peerRestore.federateName == options.memberFederateName &&
          ownerRestore.postRestoreFederateHandle.isValid() &&
          peerRestore.postRestoreFederateHandle.isValid(),
      "Application restore initiation returned the wrong save/category participants");
  require(
      owner.federationRestoreRequestsFailed().empty() &&
          peer.federationRestoreRequestsFailed().empty(),
      "Application save-identity restore request failed");

  auto const restoredOwnerState = restoreApplicationState(
      ownerRestore.label, ownerType, ownerJoinHandle, applicationState);
  auto const restoredPeerState = restoreApplicationState(
      peerRestore.label, peerType, peerJoinHandle, applicationState);
  require(
      restoredOwnerState == L"owner-state-second" &&
          restoredPeerState == L"peer-state-second" &&
          ownerType != peerType,
      "Federate type did not select the correct saved category during restore");

  auto const ownerRestoredBefore = owner.federationRestoredCount();
  auto const peerRestoredBefore = peer.federationRestoredCount();
  owner.rtiAmbassador().federateRestoreComplete();
  peer.rtiAmbassador().federateRestoreComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoredCount() > ownerRestoredBefore &&
            peer.federationRestoredCount() > peerRestoredBefore;
      },
      options,
      "application save-identity restore completion callbacks");
  require(
      owner.federationNotRestoredReasons().empty() &&
          peer.federationNotRestoredReasons().empty(),
      "Application save-identity restore did not complete successfully");

  owner.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  peer.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
