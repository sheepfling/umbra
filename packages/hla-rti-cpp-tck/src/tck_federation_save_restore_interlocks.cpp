#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederationSaveRestoreInterlocks(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Federation save/restore interlocks require an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Federation save/restore interlocks require an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "federation-save-restore-interlocks");
  connectAndJoin(owner, peer, options, federation, options.ddmFom);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const peerHandles = ddmHandles(peer, options);
  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const peerAttributes{peerHandles.attribute};
  rti::InteractionClassHandleSet const ownerInteractions{ownerHandles.interactionClass};
  rti::InteractionClassHandleSet const peerInteractions{peerHandles.interactionClass};

  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  peer.rtiAmbassador().subscribeObjectClassAttributes(
      peerHandles.objectClass,
      peerAttributes,
      true,
      L"");
  owner.rtiAmbassador().publishInteractionClass(ownerHandles.interactionClass);
  peer.rtiAmbassador().subscribeInteractionClass(
      peerHandles.interactionClass,
      true);
  owner.rtiAmbassador().publishObjectClassDirectedInteractions(
      ownerHandles.objectClass,
      ownerInteractions);
  peer.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      peerHandles.objectClass,
      peerInteractions);

  auto const object = owner.rtiAmbassador().registerObjectInstance(
      ownerHandles.objectClass);
  require(
      object.isValid(),
      "save/restore interlock setup returned an invalid object handle");

  auto const region = createDdmRegion(
      owner,
      ownerHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const attributeRegions{{
      ownerAttributes,
      rti::RegionHandleSet{region},
  }};
  rti::RegionHandleSet const regions{region};
  rti::AttributeHandleValueMap const emptyAttributes;
  rti::ParameterHandleValueMap const emptyParameters;
  rti::VariableLengthData const emptyTag;
  rti::AttributeHandleSet divestedAttributes;
  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      reliable.isValid(),
      "save/restore interlock setup returned an invalid transportation handle");
  auto const ownerFederate = owner.federateHandle();
  auto const ownerTime = makeTimeContext(owner);
  auto const interlockTime = ownerTime.factory->makeInitial();
  require(
      interlockTime != nullptr,
      "save/restore interlock setup returned no working logical time");

  auto checkInterlocks = [&](std::wstring const& expected,
                             std::string const& phase) {
    auto check = [&](auto operation, std::string const& description) {
      requireException(
          operation,
          expected,
          description + " during " + phase);
    };

    check(
        [&] {
          owner.rtiAmbassador().publishObjectClassAttributes(
              ownerHandles.objectClass,
              ownerAttributes);
        },
        "publishing object-class attributes");
    check(
        [&] {
          owner.rtiAmbassador().unpublishObjectClass(ownerHandles.objectClass);
        },
        "unpublishing an object class");
    check(
        [&] {
          owner.rtiAmbassador().unpublishObjectClassAttributes(
              ownerHandles.objectClass,
              ownerAttributes);
        },
        "unpublishing object-class attributes");
    check(
        [&] {
          peer.rtiAmbassador().subscribeObjectClassAttributes(
              peerHandles.objectClass,
              peerAttributes,
              true,
              L"");
        },
        "subscribing to object-class attributes");
    check(
        [&] {
          peer.rtiAmbassador().unsubscribeObjectClass(peerHandles.objectClass);
        },
        "unsubscribing an object class");
    check(
        [&] {
          peer.rtiAmbassador().unsubscribeObjectClassAttributes(
              peerHandles.objectClass,
              peerAttributes);
        },
        "unsubscribing object-class attributes");

    check(
        [&] {
          owner.rtiAmbassador().publishInteractionClass(
              ownerHandles.interactionClass);
        },
        "publishing an interaction class");
    check(
        [&] {
          owner.rtiAmbassador().unpublishInteractionClass(
              ownerHandles.interactionClass);
        },
        "unpublishing an interaction class");
    check(
        [&] {
          peer.rtiAmbassador().subscribeInteractionClass(
              peerHandles.interactionClass,
              true);
        },
        "subscribing to an interaction class");
    check(
        [&] {
          peer.rtiAmbassador().unsubscribeInteractionClass(
              peerHandles.interactionClass);
        },
        "unsubscribing an interaction class");

    check(
        [&] {
          owner.rtiAmbassador().publishObjectClassDirectedInteractions(
              ownerHandles.objectClass,
              ownerInteractions);
        },
        "publishing directed interactions");
    check(
        [&] {
          owner.rtiAmbassador().unpublishObjectClassDirectedInteractions(
              ownerHandles.objectClass);
        },
        "unpublishing all directed interactions");
    check(
        [&] {
          owner.rtiAmbassador().unpublishObjectClassDirectedInteractions(
              ownerHandles.objectClass,
              ownerInteractions);
        },
        "unpublishing selected directed interactions");
    check(
        [&] {
          peer.rtiAmbassador().subscribeObjectClassDirectedInteractions(
              peerHandles.objectClass,
              peerInteractions);
        },
        "subscribing to directed interactions");
    check(
        [&] {
          peer.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
              peerHandles.objectClass);
        },
        "unsubscribing all directed interactions");
    check(
        [&] {
          peer.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
              peerHandles.objectClass,
              peerInteractions);
        },
        "unsubscribing selected directed interactions");

    check(
        [&] {
          owner.rtiAmbassador().changeAttributeOrderType(
              object,
              ownerAttributes,
              rti::RECEIVE);
        },
        "changing object-attribute order");
    check(
        [&] {
          owner.rtiAmbassador().changeDefaultAttributeOrderType(
              ownerHandles.objectClass,
              ownerAttributes,
              rti::RECEIVE);
        },
        "changing default object-attribute order");
    check(
        [&] {
          owner.rtiAmbassador().changeInteractionOrderType(
              ownerHandles.interactionClass,
              rti::RECEIVE);
        },
        "changing interaction order");
    check(
        [&] {
          owner.rtiAmbassador().requestAttributeTransportationTypeChange(
              object,
              ownerAttributes,
              reliable);
        },
        "requesting an attribute transportation change");
    check(
        [&] {
          owner.rtiAmbassador().changeDefaultAttributeTransportationType(
              ownerHandles.objectClass,
              ownerAttributes,
              reliable);
        },
        "changing default attribute transportation");
    check(
        [&] {
          owner.rtiAmbassador().queryAttributeTransportationType(
              object,
              ownerHandles.attribute);
        },
        "querying attribute transportation");
    check(
        [&] {
          owner.rtiAmbassador().requestInteractionTransportationTypeChange(
              ownerHandles.interactionClass,
              reliable);
        },
        "requesting an interaction transportation change");
    check(
        [&] {
          owner.rtiAmbassador().queryInteractionTransportationType(
              ownerFederate,
              ownerHandles.interactionClass);
        },
        "querying interaction transportation");

    check(
        [&] {
          owner.rtiAmbassador().queryAttributeOwnership(
              object,
              ownerAttributes);
        },
        "querying attribute ownership");
    check(
        [&] {
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              ownerHandles.attribute);
        },
        "querying attribute ownership by federate");
    check(
        [&] {
          owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
              object,
              ownerAttributes,
              emptyTag);
        },
        "requesting negotiated divestiture");
    check(
        [&] {
          owner.rtiAmbassador().confirmDivestiture(
              object,
              ownerAttributes,
              emptyTag);
        },
        "confirming divestiture");
    check(
        [&] {
          owner.rtiAmbassador().cancelNegotiatedAttributeOwnershipDivestiture(
              object,
              ownerAttributes);
        },
        "cancelling negotiated divestiture");
    check(
        [&] {
          owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
              object,
              ownerAttributes,
              emptyTag);
        },
        "requesting unconditional divestiture");
    check(
        [&] {
          owner.rtiAmbassador().attributeOwnershipReleaseDenied(
              object,
              ownerAttributes,
              emptyTag);
        },
        "reporting denied ownership release");
    check(
        [&] {
          owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
              object,
              ownerAttributes,
              emptyTag,
              divestedAttributes);
        },
        "requesting conditional divestiture");
    check(
        [&] {
          owner.rtiAmbassador().cancelAttributeOwnershipAcquisition(
              object,
              ownerAttributes);
        },
        "cancelling ownership acquisition");

    check(
        [&] {
          owner.rtiAmbassador().registerObjectInstance(
              ownerHandles.objectClass);
        },
        "registering an object instance");
    check(
        [&] {
          owner.rtiAmbassador().createRegion(
              rti::DimensionHandleSet{
                  ownerHandles.firstDimension,
                  ownerHandles.secondDimension});
        },
        "creating a region");
    check(
        [&] {
          owner.rtiAmbassador().updateAttributeValues(
              object,
              emptyAttributes,
              emptyTag);
        },
        "updating attribute values");
    check(
        [&] {
          owner.rtiAmbassador().sendInteraction(
              ownerHandles.interactionClass,
              emptyParameters,
              emptyTag);
        },
        "sending an interaction");
    check(
        [&] {
          owner.rtiAmbassador().attributeOwnershipAcquisition(
              object,
              ownerAttributes,
              emptyTag);
        },
        "acquiring attribute ownership");

    check(
        [&] {
          owner.rtiAmbassador().timeAdvanceRequest(*interlockTime);
        },
        "requesting a time advance");
    check(
        [&] {
          owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
        },
        "enabling time regulation");
    check(
        [&] {
          owner.rtiAmbassador().disableTimeRegulation();
        },
        "disabling time regulation");
    check(
        [&] {
          owner.rtiAmbassador().enableTimeConstrained();
        },
        "enabling time constrained");
    check(
        [&] {
          owner.rtiAmbassador().disableTimeConstrained();
        },
        "disabling time constrained");
    check(
        [&] {
          owner.rtiAmbassador().queryLogicalTime(*interlockTime);
        },
        "querying logical time");
    check(
        [&] {
          owner.rtiAmbassador().queryGALT(*interlockTime);
        },
        "querying GALT");
    check(
        [&] {
          owner.rtiAmbassador().queryLITS(*interlockTime);
        },
        "querying LITS");
    check(
        [&] {
          owner.rtiAmbassador().queryLookahead(*ownerTime.epsilon);
        },
        "querying lookahead");
    check(
        [&] {
          owner.rtiAmbassador().modifyLookahead(*ownerTime.epsilon);
        },
        "modifying lookahead");

    check(
        [&] {
          static_cast<void>(
              owner.rtiAmbassador().getAttributeScopeAdvisorySwitch());
        },
        "getting the attribute-scope advisory switch");
    check(
        [&] {
          owner.rtiAmbassador().setAttributeScopeAdvisorySwitch(false);
        },
        "setting the attribute-scope advisory switch");

    check(
        [&] {
          owner.rtiAmbassador().registerObjectInstanceWithRegions(
              ownerHandles.objectClass,
              attributeRegions);
        },
        "registering an object instance with regions");
    check(
        [&] {
          owner.rtiAmbassador().associateRegionsForUpdates(
              object,
              attributeRegions);
        },
        "associating regions for updates");
    check(
        [&] {
          owner.rtiAmbassador().unassociateRegionsForUpdates(
              object,
              attributeRegions);
        },
        "unassociating regions for updates");
    check(
        [&] {
          owner.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
              ownerHandles.objectClass,
              attributeRegions);
        },
        "subscribing to regional object attributes");
    check(
        [&] {
          owner.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
              ownerHandles.objectClass,
              attributeRegions);
        },
        "unsubscribing regional object attributes");
    check(
        [&] {
          owner.rtiAmbassador().requestAttributeValueUpdateWithRegions(
              ownerHandles.objectClass,
              attributeRegions,
              emptyTag);
        },
        "requesting regional attribute values");
    check(
        [&] {
          owner.rtiAmbassador().subscribeInteractionClassWithRegions(
              ownerHandles.interactionClass,
              regions);
        },
        "subscribing to regional interactions");
    check(
        [&] {
          owner.rtiAmbassador().unsubscribeInteractionClassWithRegions(
              ownerHandles.interactionClass,
              regions);
        },
        "unsubscribing regional interactions");
    check(
        [&] {
          owner.rtiAmbassador().sendInteractionWithRegions(
              ownerHandles.interactionClass,
              emptyParameters,
              regions,
              emptyTag);
        },
        "sending a regional interaction");

    rti::FederateHandleSet const synchronizationSet{ownerFederate};
    check(
        [&] {
          owner.rtiAmbassador().registerFederationSynchronizationPoint(
              L"tck-interlock-sync",
              emptyTag);
        },
        "registering a synchronization point");
    check(
        [&] {
          owner.rtiAmbassador().registerFederationSynchronizationPoint(
              L"tck-interlock-sync-set",
              emptyTag,
              synchronizationSet);
        },
        "registering an explicit-set synchronization point");
    check(
        [&] {
          owner.rtiAmbassador().synchronizationPointAchieved(
              L"tck-interlock-sync");
        },
        "achieving a synchronization point");
  };

  std::wstring const saveLabel = L"tck-save-restore-interlocks";
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federateSaveInitiations().size() >= 1U &&
            peer.federateSaveInitiations().size() >= 1U;
      },
      options,
      "save/restore interlock save initiation callbacks");
  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  checkInterlocks(L"SaveInProgress", "federation save");
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationSavedCount() >= 1U &&
            peer.federationSavedCount() >= 1U;
      },
      options,
      "save/restore interlock save completion callbacks");
  require(
      owner.federationNotSavedReasons().empty() &&
          peer.federationNotSavedReasons().empty(),
      "save/restore interlock save reported a failure");

  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() >= 1U &&
            owner.federationRestoreBegunCount() >= 1U &&
            peer.federationRestoreBegunCount() >= 1U &&
            owner.federateRestoreInitiations().size() >= 1U &&
            peer.federateRestoreInitiations().size() >= 1U;
      },
      options,
      "save/restore interlock restore initiation callbacks");
  checkInterlocks(L"RestoreInProgress", "federation restore");
  owner.rtiAmbassador().federateRestoreComplete();
  peer.rtiAmbassador().federateRestoreComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoredCount() >= 1U &&
            peer.federationRestoredCount() >= 1U;
      },
      options,
      "save/restore interlock restore completion callbacks");
  require(
      owner.federationNotRestoredReasons().empty() &&
          peer.federationNotRestoredReasons().empty(),
      "save/restore interlock restore reported a failure");

  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  peer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
}

} // namespace hla_rti_cpp_tck::detail
