#include "tck_restore_object_snapshot_status.hpp"

#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void requireStatusFor(rti::FederateRestoreStatusVector const& statuses,
                      rti::FederateHandle const& pre,
                      rti::FederateHandle const& post,
                      rti::RestoreStatus expected) {
  std::size_t matches = 0U;
  for (auto const& descriptor : statuses) {
    if (descriptor.preRestoreHandle != pre) continue;
    ++matches;
    require(descriptor.postRestoreHandle == post && descriptor.status == expected,
            "Restore status descriptor had the wrong post handle or state");
  }
  require(matches == 1U, "Restore status omitted or duplicated a joined federate");
}

void runRestoreObjectSnapshotStatus(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_IMMEDIATE,
          "Object snapshot restore has native evidence only for immediate callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Object snapshot restore requires adapter FOM and time implementation");

  Session owner(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "restore-object-snapshot-status");
  std::wstring const saveLabel = L"tck-object-baseline";
  owner.connect();
  peer.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType + L"-owner", federation);
  peer.join(options.memberFederateName, options.federateType + L"-peer", federation);
  auto const ownerHandle = owner.federateHandle();
  auto const peerHandle = peer.federateHandle();

  owner.rtiAmbassador().requestFederationSave(saveLabel);
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              peer.federateSaveInitiations() == std::vector<std::wstring>{saveLabel},
          "Immediate object baseline save did not initiate at both members");
  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  require(owner.federationSavedCount() == 1U && peer.federationSavedCount() == 1U &&
              owner.federationNotSavedReasons().empty() &&
              peer.federationNotSavedReasons().empty(),
          "Object baseline did not complete successfully");

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter FOM lacks object class or attribute for restore snapshot");
  rti::AttributeHandleSet const attributes{attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  peer.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes);
  auto const mutated = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(mutated.isValid() && peer.recorder().hasDiscovery(mutated),
          "Post-save object was not registered and discovered");

  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  require(owner.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              owner.federationRestoreBegunCount() == 1U &&
              peer.federationRestoreBegunCount() == 1U &&
              owner.federateRestoreInitiations().size() == 1U &&
              peer.federateRestoreInitiations().size() == 1U,
          "Immediate restore control callbacks did not reach both members");
  auto const ownerRestore = owner.federateRestoreInitiations().front();
  auto const peerRestore = peer.federateRestoreInitiations().front();
  require(ownerRestore.label == saveLabel && peerRestore.label == saveLabel &&
              ownerRestore.federateName == options.ownerFederateName &&
              peerRestore.federateName == options.memberFederateName &&
              ownerRestore.postRestoreFederateHandle == ownerHandle &&
              peerRestore.postRestoreFederateHandle == peerHandle,
          "Restore initiation lost participant name or post-restore handle");

  owner.rtiAmbassador().queryFederationRestoreStatus();
  require(owner.federationRestoreStatusResponses().size() == 1U,
          "Immediate restoring-status response was not delivered directly");
  auto const restoring = owner.federationRestoreStatusResponses().back();
  require(restoring.size() == 2U,
          "Restoring-status response omitted a joined member");
  requireStatusFor(restoring, ownerHandle, ownerHandle, rti::FEDERATE_RESTORING);
  requireStatusFor(restoring, peerHandle, peerHandle, rti::FEDERATE_RESTORING);

  owner.rtiAmbassador().federateRestoreComplete();
  require(owner.federationRestoredCount() == 0U &&
              peer.federationRestoredCount() == 0U,
          "Restore finished before the final member completed");
  peer.rtiAmbassador().federateRestoreComplete();
  require(owner.federationRestoredCount() == 1U &&
              peer.federationRestoredCount() == 1U &&
              owner.federationNotRestoredReasons().empty() &&
              peer.federationNotRestoredReasons().empty(),
          "Object snapshot restore did not complete successfully");
  requireException([&] { owner.rtiAmbassador().getKnownObjectClassHandle(mutated); },
                   L"ObjectInstanceNotKnown",
                   "post-save object after baseline restore");

  owner.rtiAmbassador().queryFederationRestoreStatus();
  require(owner.federationRestoreStatusResponses().size() == 2U,
          "Immediate idle restore-status response was not delivered directly");
  auto const idle = owner.federationRestoreStatusResponses().back();
  require(idle.size() == 2U, "Idle restore status omitted a joined member");
  rti::FederateHandle const noPostRestoreHandle;
  requireStatusFor(idle, ownerHandle, noPostRestoreHandle,
                   rti::NO_RESTORE_IN_PROGRESS);
  requireStatusFor(idle, peerHandle, noPostRestoreHandle,
                   rti::NO_RESTORE_IN_PROGRESS);

  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioRestoreObjectSnapshotStatus(Options const& options, rti::CallbackModel model) {
  runRestoreObjectSnapshotStatus(options, model);
}

void scenarioRestoreObjectSnapshotStatusContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreObjectSnapshotStatus(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
