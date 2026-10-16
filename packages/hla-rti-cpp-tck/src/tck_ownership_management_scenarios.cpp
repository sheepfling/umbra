#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioPrivilegeToDeleteTransfer(
    Options const& options,
    rti::CallbackModel model) {
  Session oldOwner(options, model, "owner");
  Session newOwner(options, model, "member");
  Session observer(options, model, "observer");
  auto const federation = federationName(options, "privilege-to-delete-transfer");
  connectAndJoin(oldOwner, newOwner, options, federation, options.fom);
  observer.connect();
  observer.join(
      options.memberFederateName + L"-observer",
      options.federateType,
      federation);

  auto const oldClass = oldOwner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const newClass = newOwner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const oldValue = oldOwner.rtiAmbassador().getAttributeHandle(
      oldClass,
      options.attributeName);
  auto const newValue = newOwner.rtiAmbassador().getAttributeHandle(
      newClass,
      options.attributeName);
  auto const observerValue = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  auto const oldPrivilege = oldOwner.rtiAmbassador().getAttributeHandle(
      oldClass,
      L"HLAprivilegeToDeleteObject");
  auto const newPrivilege = newOwner.rtiAmbassador().getAttributeHandle(
      newClass,
      L"HLAprivilegeToDeleteObject");
  require(
      oldClass.isValid() && newClass.isValid() && observerClass.isValid() &&
          oldValue.isValid() && newValue.isValid() && observerValue.isValid() &&
          oldPrivilege.isValid() && newPrivilege.isValid(),
      "Delete-privilege scenario could not resolve the configured class or standard attributes");

  rti::AttributeHandleSet const oldValues{oldValue};
  rti::AttributeHandleSet const newValues{newValue};
  rti::AttributeHandleSet const observerValues{observerValue};
  rti::AttributeHandleSet const oldPrivileges{oldPrivilege};
  rti::AttributeHandleSet const newPrivileges{newPrivilege};
  oldOwner.rtiAmbassador().publishObjectClassAttributes(oldClass, oldValues);
  newOwner.rtiAmbassador().subscribeObjectClassAttributes(newClass, newValues);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerValues);

  auto const object = oldOwner.rtiAmbassador().registerObjectInstance(oldClass);
  require(object.isValid(), "Delete-privilege registration returned an invalid handle");
  waitFor(
      newOwner,
      [&] { return newOwner.recorder().hasDiscovery(object); },
      options,
      "new-owner object discovery");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "observer object discovery");
  require(
      oldOwner.rtiAmbassador().isAttributeOwnedByFederate(object, oldPrivilege) &&
          !newOwner.rtiAmbassador().isAttributeOwnedByFederate(object, newPrivilege),
      "delete privilege did not initially belong only to the registrant");

  newOwner.rtiAmbassador().publishObjectClassAttributes(newClass, newPrivileges);
  std::vector<std::uint8_t> const acquisitionTagBytes{0xA5U, 0x25U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xD1U, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  newOwner.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      newPrivileges,
      acquisitionTag);
  rti::AttributeHandleSet divested;
  oldOwner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      oldPrivileges,
      divestitureTag,
      divested);
  require(
      divested == oldPrivileges,
      "Divestiture If Wanted did not return the transferred delete privilege");
  waitFor(
      newOwner,
      [&] { return newOwner.recorder().ownershipAcquisition().has_value(); },
      options,
      "delete-privilege acquisition notification");
  auto const acquisition = newOwner.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == newPrivileges &&
          acquisition->tag == divestitureTagBytes,
      "delete-privilege acquisition notification returned the wrong object, attributes, or tag");
  require(
      !oldOwner.rtiAmbassador().isAttributeOwnedByFederate(object, oldPrivilege) &&
          newOwner.rtiAmbassador().isAttributeOwnedByFederate(object, newPrivilege),
      "delete-privilege ownership did not transfer to the acquirer");

  std::vector<std::uint8_t> const deletionTagBytes{0xD3U, 0x1EU, 0x25U};
  rti::VariableLengthData const deletionTag(
      deletionTagBytes.data(),
      deletionTagBytes.size());
  requireException(
      [&] { oldOwner.rtiAmbassador().deleteObjectInstance(object, deletionTag); },
      L"DeletePrivilegeNotHeld",
      "former registrant deleting after transferring delete privilege");
  require(
      observer.recorder().removals().empty(),
      "failed former-owner deletion produced an object-removal callback");

  newOwner.rtiAmbassador().deleteObjectInstance(object, deletionTag);
  waitFor(
      observer,
      [&] { return observer.recorder().removals().size() == 1U; },
      options,
      "object removal after delete-privilege transfer");
  auto const removals = observer.recorder().removals();
  require(
      removals.front().object == object &&
          removals.front().tag == deletionTagBytes &&
          removals.front().producer == oldOwner.federateHandle(),
      "object removal returned the wrong identity, tag, or registrant metadata");

  observer.resign(rti::NO_ACTION);
  newOwner.resign(rti::NO_ACTION);
  oldOwner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  oldOwner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  newOwner.disconnect();
  oldOwner.disconnect();
}

void scenarioPrivilegeToDeleteTransferContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPrivilegeToDeleteTransfer(options, model);
}

void scenarioOwnership(Options const& options, rti::CallbackModel model) {
  Session lifecycle(options, model, "ownership-lifecycle");
  rti::ObjectInstanceHandle const lifecycleInvalidObject;
  rti::AttributeHandle const lifecycleInvalidAttribute;
  rti::AttributeHandleSet const emptyAttributes;
  rti::VariableLengthData const emptyTag;
  auto checkOwnershipLifecycle = [&](std::wstring const& expected,
                                     std::string const& phase) {
    requireException(
        [&] {
          lifecycle.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
              lifecycleInvalidObject,
              emptyAttributes,
              emptyTag);
        },
        expected,
        phase + " unconditional divestiture");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
              lifecycleInvalidObject,
              emptyAttributes,
              emptyTag);
        },
        expected,
        phase + " negotiated divestiture");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().confirmDivestiture(
              lifecycleInvalidObject,
              emptyAttributes,
              emptyTag);
        },
        expected,
        phase + " divestiture confirmation");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().attributeOwnershipAcquisition(
              lifecycleInvalidObject,
              emptyAttributes,
              emptyTag);
        },
        expected,
        phase + " ownership acquisition");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
              lifecycleInvalidObject,
              emptyAttributes,
              emptyTag);
        },
        expected,
        phase + " If Available ownership acquisition");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().attributeOwnershipReleaseDenied(
              lifecycleInvalidObject,
              emptyAttributes,
              emptyTag);
        },
        expected,
        phase + " ownership release denial");
    rti::AttributeHandleSet divestedAttributes;
    requireException(
        [&] {
          lifecycle.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
              lifecycleInvalidObject,
              emptyAttributes,
              emptyTag,
              divestedAttributes);
        },
        expected,
        phase + " If Wanted divestiture");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().cancelNegotiatedAttributeOwnershipDivestiture(
              lifecycleInvalidObject,
              emptyAttributes);
        },
        expected,
        phase + " negotiated divestiture cancellation");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().cancelAttributeOwnershipAcquisition(
              lifecycleInvalidObject,
              emptyAttributes);
        },
        expected,
        phase + " acquisition cancellation");
    requireException(
        [&] {
          lifecycle.rtiAmbassador().queryAttributeOwnership(
              lifecycleInvalidObject,
              emptyAttributes);
        },
        expected,
        phase + " ownership query");
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().isAttributeOwnedByFederate(
              lifecycleInvalidObject,
              lifecycleInvalidAttribute));
        },
        expected,
        phase + " ownership status query");
  };
  checkOwnershipLifecycle(L"NotConnected", "before connect");
  lifecycle.connect();
  checkOwnershipLifecycle(L"FederateNotExecutionMember", "before joining an execution");
  lifecycle.disconnect();

  Session owner(options, model, "owner");
  Session acquirer(options, model, "acquirer");
  auto const federation = federationName(options, "ownership");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const acquirerClass =
      acquirer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute =
      owner.rtiAmbassador().getAttributeHandle(ownerClass, options.attributeName);
  auto const acquirerAttribute =
      acquirer.rtiAmbassador().getAttributeHandle(acquirerClass, options.attributeName);
  require(ownerClass.isValid() && acquirerClass.isValid(),
          "Ownership object class lookup returned an invalid handle");
  require(ownerAttribute.isValid() && acquirerAttribute.isValid(),
          "Ownership attribute lookup returned an invalid handle");

  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(ownerAttribute);
  rti::AttributeHandleSet acquirerAttributes;
  acquirerAttributes.insert(acquirerAttribute);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(acquirerClass, acquirerAttributes);
  owner.rtiAmbassador().subscribeObjectClassAttributes(
      ownerClass, ownerAttributes, true, L"");
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass, acquirerAttributes, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Ownership registration returned an invalid object handle");
  waitFor(acquirer, [&] { return acquirer.recorder().discovery().present; }, options,
          "ownership object discovery");
  auto const discovered = acquirer.recorder().discovery();
  require(discovered.object == object,
          "Ownership discovery returned the wrong object handle");

  rti::AttributeHandleSet const invalidAttributes{rti::AttributeHandle{}};
  requireException(
      [&] {
        acquirer.rtiAmbassador().queryAttributeOwnership(
            rti::ObjectInstanceHandle{},
            acquirerAttributes);
      },
      L"ObjectInstanceNotKnown",
      "querying ownership for an unknown object instance");
  requireException(
      [&] {
        acquirer.rtiAmbassador().queryAttributeOwnership(
            object,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "querying ownership for an undefined attribute");
  requireException(
      [&] {
        static_cast<void>(acquirer.rtiAmbassador().isAttributeOwnedByFederate(
            rti::ObjectInstanceHandle{},
            acquirerAttribute));
      },
      L"ObjectInstanceNotKnown",
      "checking ownership for an unknown object instance");
  requireException(
      [&] {
        static_cast<void>(acquirer.rtiAmbassador().isAttributeOwnedByFederate(
            object,
            rti::AttributeHandle{}));
      },
      L"AttributeNotDefined",
      "checking ownership for an undefined attribute");
  rti::ObjectInstanceHandle const invalidObject;
  rti::VariableLengthData const emptyOwnershipTag;
  rti::AttributeHandleSet ignoredIfWanted;
  requireException(
      [&] {
        owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
            invalidObject,
            ownerAttributes,
            emptyOwnershipTag);
      },
      L"ObjectInstanceNotKnown",
      "unconditional divestiture for an unknown object");
  requireException(
      [&] {
        owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
            invalidObject,
            ownerAttributes,
            emptyOwnershipTag);
      },
      L"ObjectInstanceNotKnown",
      "negotiated divestiture for an unknown object");
  requireException(
      [&] {
        owner.rtiAmbassador().confirmDivestiture(
            invalidObject,
            ownerAttributes,
            emptyOwnershipTag);
      },
      L"ObjectInstanceNotKnown",
      "divestiture confirmation for an unknown object");
  requireException(
      [&] {
        owner.rtiAmbassador().cancelNegotiatedAttributeOwnershipDivestiture(
            invalidObject,
            ownerAttributes);
      },
      L"ObjectInstanceNotKnown",
      "negotiated divestiture cancellation for an unknown object");
  requireException(
      [&] {
        owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
            invalidObject,
            ownerAttributes,
            emptyOwnershipTag,
            ignoredIfWanted);
      },
      L"ObjectInstanceNotKnown",
      "If Wanted divestiture for an unknown object");
  requireException(
      [&] {
        owner.rtiAmbassador().attributeOwnershipReleaseDenied(
            invalidObject,
            ownerAttributes,
            emptyOwnershipTag);
      },
      L"ObjectInstanceNotKnown",
      "ownership release denial for an unknown object");
  requireException(
      [&] {
        acquirer.rtiAmbassador().attributeOwnershipAcquisition(
            invalidObject,
            acquirerAttributes,
            emptyOwnershipTag);
      },
      L"ObjectInstanceNotKnown",
      "ownership acquisition for an unknown object");
  requireException(
      [&] {
        acquirer.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
            invalidObject,
            acquirerAttributes,
            emptyOwnershipTag);
      },
      L"ObjectInstanceNotKnown",
      "If Available acquisition for an unknown object");
  requireException(
      [&] {
        acquirer.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            invalidObject,
            acquirerAttributes);
      },
      L"ObjectInstanceNotKnown",
      "acquisition cancellation for an unknown object");

  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner did not initially own the published attribute");
  require(!acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer initially reported ownership of the attribute");

  // Querying through the subscriber exercises the owner report and the
  // standard cross-federate handle identity.
  acquirer.recorder().clearOwnershipRecords();
  acquirer.rtiAmbassador().queryAttributeOwnership(object, acquirerAttributes);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipInformation().has_value();
  }, options, "the ownership query report");
  auto const information = acquirer.recorder().ownershipInformation();
  require(information->object == object && information->attributes == acquirerAttributes,
          "Ownership query returned the wrong object or attribute set");
  require(information->owner == owner.federateHandle(),
          "Ownership query did not identify the owning federate");

  // Negotiated acquisition: the acquirer's tag reaches the owner's request,
  // and the owner's confirmation tag reaches the acquisition notification.
  acquirer.recorder().clearOwnershipRecords();
  owner.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> divestitureTagBytes{0x10U};
  std::vector<std::uint8_t> acquisitionRequestTagBytes{0x20U};
  std::vector<std::uint8_t> confirmationTagBytes{0x30U};
  rti::VariableLengthData divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  rti::VariableLengthData acquisitionRequestTag(
      acquisitionRequestTagBytes.data(), acquisitionRequestTagBytes.size());
  rti::VariableLengthData confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, acquisitionRequestTag);
  waitFor(owner, [&] {
    return owner.recorder().divestitureConfirmation().has_value();
  }, options, "the negotiated divestiture confirmation request");
  auto const divestitureConfirmation = owner.recorder().divestitureConfirmation();
  require(divestitureConfirmation->object == object &&
              divestitureConfirmation->attributes == ownerAttributes,
          "Negotiated divestiture returned the wrong object or attributes");
  require(divestitureConfirmation->tag == acquisitionRequestTagBytes,
          "Negotiated divestiture did not preserve the acquisition tag");

  owner.rtiAmbassador().confirmDivestiture(object, ownerAttributes, confirmationTag);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipAcquisition().has_value();
  }, options, "the negotiated acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(acquisition->object == object && acquisition->attributes == acquirerAttributes,
          "Acquisition notification returned the wrong object or attributes");
  require(acquisition->tag == confirmationTagBytes,
          "Acquisition notification did not preserve the confirmation tag");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner retained ownership after confirmed divestiture");
  require(acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer did not gain ownership after confirmation");

  // The If Wanted service is valid for an owner even when there is no pending
  // acquisition. The returned set is provider-arbitrated by the standard API.
  rti::AttributeHandleSet wanted;
  std::vector<std::uint8_t> noPendingDivestitureTagBytes{0x31U};
  rti::VariableLengthData noPendingDivestitureTag(
      noPendingDivestitureTagBytes.data(), noPendingDivestitureTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      acquirerAttributes,
      noPendingDivestitureTag,
      wanted);
  require(wanted.empty(),
          "If Wanted unexpectedly divested an attribute without a pending acquirer");

  // An unconditional divestiture makes the attribute immediately available;
  // the owner can then reacquire it through the If Available service.
  acquirer.recorder().clearOwnershipRecords();
  owner.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> unconditionalTagBytes{0x40U};
  std::vector<std::uint8_t> ifAvailableTagBytes{0x50U};
  rti::VariableLengthData unconditionalTag(
      unconditionalTagBytes.data(), unconditionalTagBytes.size());
  rti::VariableLengthData ifAvailableTag(
      ifAvailableTagBytes.data(), ifAvailableTagBytes.size());
  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, acquirerAttributes, unconditionalTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAssumption().has_value();
  }, options, "the ownership assumption offer");
  auto const assumption = owner.recorder().ownershipAssumption();
  require(assumption->object == object && assumption->attributes == ownerAttributes,
          "Ownership assumption returned the wrong object or attributes");
  require(assumption->tag == unconditionalTagBytes,
          "Ownership assumption did not preserve the divestiture tag");

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, ownerAttributes);
  waitFor(owner, [&] {
    return owner.recorder().ownershipNotOwned().has_value();
  }, options, "the unowned ownership query report");
  auto const unowned = owner.recorder().ownershipNotOwned();
  require(unowned->object == object && unowned->attributes == ownerAttributes,
          "Unowned ownership query returned the wrong object or attributes");

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, ownerAttributes, ifAvailableTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAcquisition().has_value();
  }, options, "the If Available acquisition notification");
  auto const ifAvailableAcquisition = owner.recorder().ownershipAcquisition();
  require(ifAvailableAcquisition->object == object &&
              ifAvailableAcquisition->attributes == ownerAttributes,
          "If Available acquisition returned the wrong object or attributes");
  require(ifAvailableAcquisition->tag == ifAvailableTagBytes,
          "If Available acquisition did not preserve its user tag");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner did not regain ownership through If Available acquisition");
  require(!acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer retained ownership after unconditional divestiture");

  // A pending regular acquisition can also be satisfied directly by the
  // current owner through Divestiture If Wanted. The divestiture tag, rather
  // than the older acquisition tag, is delivered to the new owner.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> ifWantedAcquisitionTagBytes{0xA0U};
  std::vector<std::uint8_t> ifWantedDivestitureTagBytes{0xA1U};
  rti::VariableLengthData ifWantedAcquisitionTag(
      ifWantedAcquisitionTagBytes.data(), ifWantedAcquisitionTagBytes.size());
  rti::VariableLengthData ifWantedDivestitureTag(
      ifWantedDivestitureTagBytes.data(), ifWantedDivestitureTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, ifWantedAcquisitionTag);
  rti::AttributeHandleSet ifWantedDivestedAttributes;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      ifWantedDivestitureTag,
      ifWantedDivestedAttributes);
  require(ifWantedDivestedAttributes == ownerAttributes,
          "If Wanted did not return the transferred attribute set");
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipAcquisition().has_value();
  }, options, "the If Wanted acquisition notification");
  auto const ifWantedAcquisition = acquirer.recorder().ownershipAcquisition();
  require(ifWantedAcquisition->object == object &&
              ifWantedAcquisition->attributes == acquirerAttributes,
          "If Wanted acquisition returned the wrong object or attributes");
  require(ifWantedAcquisition->tag == ifWantedDivestitureTagBytes,
          "If Wanted acquisition did not preserve the divestiture tag");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner retained ownership after If Wanted transfer");
  require(acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer did not gain ownership through If Wanted transfer");

  // Return the attribute to the owner so the denial and cancellation checks
  // start from the same owner-held state as the Java parity scenario.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> returnAcquisitionTagBytes{0xB0U};
  std::vector<std::uint8_t> returnDivestitureTagBytes{0xB1U};
  rti::VariableLengthData returnAcquisitionTag(
      returnAcquisitionTagBytes.data(), returnAcquisitionTagBytes.size());
  rti::VariableLengthData returnDivestitureTag(
      returnDivestitureTagBytes.data(), returnDivestitureTagBytes.size());
  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      acquirerAttributes,
      returnDivestitureTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAssumption().has_value();
  }, options, "the return ownership assumption offer");
  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, ownerAttributes, returnAcquisitionTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipAcquisition().has_value();
  }, options, "the return If Available acquisition notification");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner did not regain ownership before denial checks");

  // A regular acquisition asks the current owner to release. Denying it must
  // preserve ownership and deliver the denial tag as Unavailable.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> deniedAcquisitionTagBytes{0x60U};
  std::vector<std::uint8_t> denialTagBytes{0x70U};
  rti::VariableLengthData deniedAcquisitionTag(
      deniedAcquisitionTagBytes.data(), deniedAcquisitionTagBytes.size());
  rti::VariableLengthData denialTag(denialTagBytes.data(), denialTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, deniedAcquisitionTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipReleaseRequest().has_value();
  }, options, "the ownership release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(releaseRequest->object == object && releaseRequest->attributes == ownerAttributes,
          "Ownership release request returned the wrong object or attributes");
  require(releaseRequest->tag == deniedAcquisitionTagBytes,
          "Ownership release request did not preserve the acquisition tag");
  owner.rtiAmbassador().attributeOwnershipReleaseDenied(object, ownerAttributes, denialTag);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipUnavailable().has_value();
  }, options, "the ownership-unavailable denial callback");
  auto const unavailable = acquirer.recorder().ownershipUnavailable();
  require(unavailable->object == object && unavailable->attributes == acquirerAttributes,
          "Ownership-unavailable callback returned the wrong object or attributes");
  require(unavailable->tag == denialTagBytes,
          "Ownership-unavailable callback did not preserve the denial tag");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner lost ownership after denying the acquisition");

  // A second pending acquisition can be canceled, and the requester receives
  // the standard confirmation callback.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> cancellationAcquisitionTagBytes{0x80U};
  rti::VariableLengthData cancellationAcquisitionTag(
      cancellationAcquisitionTagBytes.data(), cancellationAcquisitionTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, cancellationAcquisitionTag);
  waitFor(owner, [&] {
    return owner.recorder().ownershipReleaseRequest().has_value();
  }, options, "the cancelable ownership release request");
  acquirer.rtiAmbassador().cancelAttributeOwnershipAcquisition(object, acquirerAttributes);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipAcquisitionCancellation().has_value();
  }, options, "the ownership acquisition cancellation confirmation");
  auto const cancellation = acquirer.recorder().ownershipAcquisitionCancellation();
  require(cancellation->object == object && cancellation->attributes == acquirerAttributes,
          "Ownership cancellation returned the wrong object or attributes");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner lost ownership after acquisition cancellation");

  std::vector<std::uint8_t> canceledDivestitureTagBytes{0x90U};
  rti::VariableLengthData canceledDivestitureTag(
      canceledDivestitureTagBytes.data(), canceledDivestitureTagBytes.size());
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, canceledDivestitureTag);
  owner.rtiAmbassador().cancelNegotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes);
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner lost ownership after negotiated divestiture cancellation");

  // If Available must fail immediately while another federate still owns the
  // attribute.  It does not ask the owner to release anything; the requester
  // receives exactly one Unavailable callback carrying its original tag.
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> unavailableIfAvailableTagBytes{0xC0U};
  rti::VariableLengthData unavailableIfAvailableTag(
      unavailableIfAvailableTagBytes.data(), unavailableIfAvailableTagBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      acquirerAttributes,
      unavailableIfAvailableTag);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipUnavailableEvents().size() >= 1U;
  }, options, "the If Available unavailable callback");
  auto const unavailableIfAvailableEvents =
      acquirer.recorder().ownershipUnavailableEvents();
  require(
      unavailableIfAvailableEvents.size() == 1U &&
          unavailableIfAvailableEvents.front().object == object &&
          unavailableIfAvailableEvents.front().attributes == acquirerAttributes &&
          unavailableIfAvailableEvents.front().tag == unavailableIfAvailableTagBytes,
      "If Available unavailable callback returned the wrong object, attributes, or tag");
  for (int pass = 0; pass != 8; ++pass) {
    acquirer.pump();
    owner.pump();
  }
  require(
      acquirer.recorder().ownershipUnavailableEvents().size() == 1U,
      "If Available unavailable callback was delivered more than once");
  require(
      !owner.recorder().ownershipReleaseRequest().has_value() &&
          !acquirer.recorder().ownershipAcquisition().has_value() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
      "If Available unavailable changed ownership or requested owner release");

  owner.resign(rti::DELETE_OBJECTS);
  acquirer.resign(rti::NO_ACTION);
}


void scenarioOwnershipSaveRestore(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session acquirer(options, model, "acquirer");
  auto const federation = federationName(options, "ownership-state-save-restore");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const acquirerClass =
      acquirer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute =
      owner.rtiAmbassador().getAttributeHandle(ownerClass, options.attributeName);
  auto const acquirerAttribute =
      acquirer.rtiAmbassador().getAttributeHandle(acquirerClass, options.attributeName);
  require(ownerClass.isValid() && acquirerClass.isValid(),
          "Ownership save/restore class lookup returned an invalid handle");
  require(ownerAttribute.isValid() && acquirerAttribute.isValid(),
          "Ownership save/restore attribute lookup returned an invalid handle");

  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(ownerAttribute);
  rti::AttributeHandleSet acquirerAttributes;
  acquirerAttributes.insert(acquirerAttribute);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(acquirerClass, acquirerAttributes);
  owner.rtiAmbassador().subscribeObjectClassAttributes(
      ownerClass, ownerAttributes, true, L"");
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass, acquirerAttributes, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Ownership save/restore registration returned an invalid handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  waitFor(acquirer, [&] { return acquirer.recorder().discovery().present; }, options,
          "ownership save/restore object discovery");
  require(acquirer.recorder().discovery().object == object,
          "Ownership save/restore discovery returned the wrong object handle");

  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Owner did not initially own the published attribute");
  require(!acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer initially reported ownership of the attribute");

  std::vector<std::uint8_t> const divestitureTagBytes{0x44U, 0x49U, 0x56U};
  std::vector<std::uint8_t> const acquisitionTagBytes{0x41U, 0x43U, 0x51U};
  std::vector<std::uint8_t> const confirmationTagBytes{0x43U, 0x4FU, 0x4EU};
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());

  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, acquisitionTag);
  waitFor(owner, [&] {
    return owner.recorder().divestitureConfirmation().has_value();
  }, options, "ownership save/restore divestiture confirmation");
  auto const divestitureConfirmation = owner.recorder().divestitureConfirmation();
  require(divestitureConfirmation->object == object &&
              divestitureConfirmation->attributes == ownerAttributes,
          "Ownership save/restore confirmation returned the wrong object or attributes");
  require(divestitureConfirmation->tag == acquisitionTagBytes,
          "Ownership save/restore confirmation did not preserve the acquisition tag");

  owner.rtiAmbassador().confirmDivestiture(object, ownerAttributes, confirmationTag);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipAcquisition().has_value();
  }, options, "ownership save/restore acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(acquisition->object == object && acquisition->attributes == acquirerAttributes,
          "Ownership save/restore notification returned the wrong object or attributes");
  require(acquisition->tag == confirmationTagBytes,
          "Ownership save/restore notification did not preserve the confirmation tag");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Previous owner retained ownership after the negotiated transfer");
  require(acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "Acquirer did not own the attribute before federation save");

  std::wstring const saveLabel = L"tck-ownership-state-save-restore";
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &acquirer},
      [&] {
        return owner.federateSaveInitiations().size() >= 1U &&
            acquirer.federateSaveInitiations().size() >= 1U;
      },
      options,
      "ownership save/restore save initiation callbacks");
  require(owner.federateSaveInitiations().back() == saveLabel &&
              acquirer.federateSaveInitiations().back() == saveLabel,
          "Ownership save/restore initiation returned the wrong label");
  owner.rtiAmbassador().federateSaveBegun();
  acquirer.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  require(owner.federationSavedCount() == 0U && acquirer.federationSavedCount() == 0U,
          "Federation save completed before every federate reported completion");
  acquirer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &acquirer},
      [&] {
        return owner.federationSavedCount() >= 1U &&
            acquirer.federationSavedCount() >= 1U;
      },
      options,
      "ownership save/restore federation-saved callbacks");
  require(owner.federationNotSavedReasons().empty() &&
              acquirer.federationNotSavedReasons().empty(),
          "Ownership save/restore reported a failed federation save");

  auto const restoreRequestBefore = owner.federationRestoreRequestsSucceeded().size();
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions(
      {&owner, &acquirer},
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() > restoreRequestBefore &&
            owner.federationRestoreBegunCount() >= 1U &&
            acquirer.federationRestoreBegunCount() >= 1U &&
            owner.federateRestoreInitiations().size() >= 1U &&
            acquirer.federateRestoreInitiations().size() >= 1U;
      },
      options,
      "ownership save/restore initiation callbacks");
  require(owner.federationRestoreRequestsSucceeded().back() == saveLabel,
          "Ownership save/restore request confirmation returned the wrong label");
  require(owner.federationRestoreRequestsFailed().empty() &&
              acquirer.federationRestoreRequestsFailed().empty(),
          "Ownership save/restore request reported a failure");

  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const acquirerRestore = acquirer.federateRestoreInitiations().back();
  require(ownerRestore.label == saveLabel && acquirerRestore.label == saveLabel,
          "Ownership save/restore initiation returned the wrong save label");
  require(ownerRestore.federateName == options.ownerFederateName &&
              acquirerRestore.federateName == options.memberFederateName,
          "Ownership save/restore initiation returned the wrong federate name");
  require(ownerRestore.postRestoreFederateHandle.isValid() &&
              acquirerRestore.postRestoreFederateHandle.isValid(),
          "Ownership save/restore returned an invalid post-restore federate handle");

  owner.rtiAmbassador().federateRestoreComplete();
  acquirer.rtiAmbassador().federateRestoreComplete();
  waitForSessions(
      {&owner, &acquirer},
      [&] {
        return owner.federationRestoredCount() >= 1U &&
            acquirer.federationRestoredCount() >= 1U;
      },
      options,
      "ownership save/restore federation-restored callbacks");
  require(owner.federationNotRestoredReasons().empty() &&
              acquirer.federationNotRestoredReasons().empty(),
          "Ownership save/restore reported a failed federation restore");
  require(owner.rtiAmbassador().getFederateHandle(options.ownerFederateName) ==
              ownerRestore.postRestoreFederateHandle &&
              acquirer.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
                  acquirerRestore.postRestoreFederateHandle,
          "Ownership save/restore did not install the post-restore federate handles");

  auto const restoredOwnerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const restoredOwnerAttribute = owner.rtiAmbassador().getAttributeHandle(
      restoredOwnerClass, options.attributeName);
  auto const restoredAcquirerClass =
      acquirer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const restoredAcquirerAttribute = acquirer.rtiAmbassador().getAttributeHandle(
      restoredAcquirerClass, options.attributeName);
  rti::AttributeHandleSet restoredAcquirerAttributes;
  restoredAcquirerAttributes.insert(restoredAcquirerAttribute);
  auto const restoredObject = owner.rtiAmbassador().getObjectInstanceHandle(objectName);
  require(restoredObject.isValid(),
          "Restored ownership object could not be resolved by its standard instance name");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(
              restoredObject, restoredOwnerAttribute),
          "Previous owner regained the attribute after federation restore");
  require(acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              restoredObject, restoredAcquirerAttribute),
          "Acquirer did not retain ownership after federation restore");

  acquirer.recorder().clearOwnershipRecords();
  acquirer.rtiAmbassador().queryAttributeOwnership(
      restoredObject, restoredAcquirerAttributes);
  waitFor(acquirer, [&] {
    return acquirer.recorder().ownershipInformation().has_value();
  }, options, "restored attribute ownership query");
  auto const restoredOwnership = acquirer.recorder().ownershipInformation();
  require(restoredOwnership->object == restoredObject &&
              restoredOwnership->attributes == restoredAcquirerAttributes,
          "Restored ownership query returned the wrong object or attributes");
  require(restoredOwnership->owner == acquirerRestore.postRestoreFederateHandle,
          "Restored ownership query returned the wrong post-restore owner");

  acquirer.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  acquirer.disconnect();
  owner.disconnect();
}


void scenarioOwnershipAcquisitionCancellationSaveRestore(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "requester");
  auto const federation = federationName(
      options,
      "ownership-acquisition-cancellation-save-restore");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Ownership-cancellation save/restore lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Ownership-cancellation save/restore registration returned an invalid handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "ownership-cancellation save/restore object discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "Ownership-cancellation save/restore started with the wrong owner");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x43U, 0x41U, 0x4EU};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();

  // Keep the cancellation notification pending across the snapshot boundary.
  // Evoked delivery naturally holds the requester's callback queue; the
  // immediate model uses the standard callback-control services for the same
  // delivery boundary.
  if (model == rti::HLA_IMMEDIATE) {
    owner.rtiAmbassador().disableCallbacks();
    requester.rtiAmbassador().disableCallbacks();
  }
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
      object,
      requesterAttributes);

  std::wstring const saveLabel = L"tck-ownership-acquisition-cancellation";
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  owner.rtiAmbassador().federateSaveBegun();
  requester.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  requester.rtiAmbassador().federateSaveComplete();
  if (model == rti::HLA_EVOKED) {
    waitFor(
        owner,
        [&] {
          return owner.federateSaveInitiations().size() >= 1U &&
              owner.federationSavedCount() >= 1U;
        },
        options,
        "ownership-cancellation federation-saved callback");
  }

  auto const restoreRequestBefore =
      owner.federationRestoreRequestsSucceeded().size();
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  if (model == rti::HLA_EVOKED) {
    waitFor(
        owner,
        [&] {
          return owner.federationRestoreRequestsSucceeded().size() >
                  restoreRequestBefore &&
              owner.federationRestoreBegunCount() >= 1U &&
              owner.federateRestoreInitiations().size() >= 1U;
        },
        options,
        "ownership-cancellation restore initiation callbacks");
  }
  owner.rtiAmbassador().federateRestoreComplete();
  requester.rtiAmbassador().federateRestoreComplete();
  if (model == rti::HLA_IMMEDIATE) {
    owner.rtiAmbassador().enableCallbacks();
    requester.rtiAmbassador().enableCallbacks();
  } else {
    waitFor(
        owner,
        [&] { return owner.federationRestoredCount() >= 1U; },
        options,
        "ownership-cancellation federation-restored callback");
  }

  waitForSessions(
      {&owner, &requester},
      [&] {
        return owner.federationSavedCount() >= 1U &&
            requester.federationSavedCount() >= 1U &&
            owner.federationRestoreRequestsSucceeded().size() >
                restoreRequestBefore &&
            requester.federationRestoreBegunCount() >= 1U &&
            owner.federationRestoredCount() >= 1U &&
            requester.federationRestoredCount() >= 1U &&
            requester.recorder()
                    .ownershipAcquisitionCancellationEvents()
                    .size() >= 1U;
      },
      options,
      "ownership-cancellation save/restore callbacks");
  require(
      owner.federationNotSavedReasons().empty() &&
          requester.federationNotSavedReasons().empty() &&
          owner.federationRestoreRequestsFailed().empty() &&
          requester.federationRestoreRequestsFailed().empty() &&
          owner.federationNotRestoredReasons().empty() &&
          requester.federationNotRestoredReasons().empty(),
      "Ownership-cancellation save/restore reported a lifecycle failure");
  require(
      owner.federateSaveInitiations().size() == 1U &&
          requester.federateSaveInitiations().size() == 1U &&
          owner.federateSaveInitiations().front() == saveLabel &&
          requester.federateSaveInitiations().front() == saveLabel &&
          owner.federationSavedCount() == 1U &&
          requester.federationSavedCount() == 1U,
      "Ownership-cancellation save callbacks returned the wrong label or count");
  auto const ownerRestore = owner.federateRestoreInitiations();
  auto const requesterRestore = requester.federateRestoreInitiations();
  require(
      ownerRestore.size() == 1U && requesterRestore.size() == 1U &&
          ownerRestore.front().label == saveLabel &&
          requesterRestore.front().label == saveLabel &&
          ownerRestore.front().federateName == options.ownerFederateName &&
          requesterRestore.front().federateName == options.memberFederateName &&
          ownerRestore.front().postRestoreFederateHandle.isValid() &&
          requesterRestore.front().postRestoreFederateHandle.isValid(),
      "Ownership-cancellation restore callbacks returned the wrong federate data");
  require(
      owner.federationRestoreRequestsSucceeded().size() == 1U &&
          owner.federationRestoreRequestsSucceeded().front() == saveLabel &&
          owner.federationRestoreBegunCount() == 1U &&
          requester.federationRestoreBegunCount() == 1U &&
          owner.federationRestoredCount() == 1U &&
          requester.federationRestoredCount() == 1U,
      "Ownership-cancellation restore callbacks returned the wrong label or count");

  auto const cancellationEvents =
      requester.recorder().ownershipAcquisitionCancellationEvents();
  require(
      cancellationEvents.size() == 1U &&
          cancellationEvents.front().object == object &&
          cancellationEvents.front().attributes == requesterAttributes &&
          cancellationEvents.front().tag.empty(),
      "Restored ownership-cancellation callback returned the wrong standard data");
  require(
      !requester.recorder().ownershipAcquisition().has_value(),
      "Canceled ownership acquisition completed after federation restore");

  auto const restoredObject = owner.rtiAmbassador().getObjectInstanceHandle(objectName);
  auto const restoredOwnerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const restoredRequesterClass =
      requester.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const restoredOwnerAttribute = owner.rtiAmbassador().getAttributeHandle(
      restoredOwnerClass,
      options.attributeName);
  auto const restoredRequesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      restoredRequesterClass,
      options.attributeName);
  require(
      restoredObject.isValid() && restoredOwnerClass.isValid() &&
          restoredRequesterClass.isValid() && restoredOwnerAttribute.isValid() &&
          restoredRequesterAttribute.isValid(),
      "Ownership-cancellation restore could not resolve standard handles by name");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(
          restoredObject,
          restoredOwnerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              restoredObject,
              restoredRequesterAttribute),
      "Canceled ownership acquisition changed owner state after restore");

  requester.rtiAmbassador().queryAttributeOwnership(
      restoredObject,
      rti::AttributeHandleSet{restoredRequesterAttribute});
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipInformation().has_value(); },
      options,
      "ownership-cancellation restored ownership query");
  auto const ownership = requester.recorder().ownershipInformation();
  require(
      ownership->object == restoredObject &&
          ownership->attributes == rti::AttributeHandleSet{restoredRequesterAttribute} &&
          ownership->owner == ownerRestore.front().postRestoreFederateHandle,
      "Ownership-cancellation restored query returned the wrong owner");

  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
    requester.pump();
  }
  require(
      requester.recorder().ownershipAcquisitionCancellationEvents().size() == 1U,
      "Ownership-cancellation confirmation was delivered more than once");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}


void scenarioOwnershipAcquisitionPublicationFence(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Ownership-acquisition publication testing requires an adapter-supplied multi-attribute FOM");

  Session owner(options, model, "ownership-publication-owner");
  Session requester(options, model, "ownership-publication-requester");
  auto const federation = federationName(
      options,
      "ownership-acquisition-publication-fence");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-ownership-publication-owner",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-ownership-publication-requester",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeSecondName);
  require(
      ownerClass.isValid() && requesterClass.isValid(),
      "Ownership-acquisition publication object-class lookup returned an invalid handle");
  require(
      ownerFirst.isValid() && ownerSecond.isValid() && requesterFirst.isValid() &&
          requesterSecond.isValid(),
      "Ownership-acquisition publication attribute lookup returned an invalid handle");
  require(
      ownerFirst != ownerSecond,
      "Ownership-acquisition publication adapter supplied duplicate attribute names");

  rti::AttributeHandleSet const ownerFirstOnly{ownerFirst};
  rti::AttributeHandleSet const ownerSecondOnly{ownerSecond};
  rti::AttributeHandleSet const requesterFirstOnly{requesterFirst};
  rti::AttributeHandleSet const requesterSecondOnly{requesterSecond};
  rti::AttributeHandleSet const requesterAttributes{requesterFirst, requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerFirstOnly);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Ownership-acquisition publication registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "Ownership-acquisition publication object discovery");

  std::vector<std::uint8_t> const initialTagBytes{0x11U, 0x22U};
  std::vector<std::uint8_t> const acquisitionTagBytes{0x33U, 0x44U, 0x55U};
  std::vector<std::uint8_t> const denialTagBytes{0x66U, 0x77U};
  rti::VariableLengthData const initialTag(
      initialTagBytes.data(),
      initialTagBytes.size());
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const denialTag(
      denialTagBytes.data(),
      denialTagBytes.size());

  // A requester cannot acquire an attribute until it publishes that
  // attribute. Subscription and object discovery do not replace publication.
  requireException(
      [&] {
        requester.rtiAmbassador().attributeOwnershipAcquisition(
            object,
            requesterSecondOnly,
            initialTag);
      },
      L"ObjectClassNotPublished",
      "acquiring an unpublished ownership attribute");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);

  rti::AttributeHandle const invalidAttribute;
  requireException(
      [&] {
        requester.rtiAmbassador().attributeOwnershipAcquisition(
            object,
            rti::AttributeHandleSet{invalidAttribute},
            acquisitionTag);
      },
      L"AttributeNotDefined",
      "acquiring an undefined ownership attribute");
  requireException(
      [&] {
        owner.rtiAmbassador().attributeOwnershipReleaseDenied(
            object,
            ownerSecondOnly,
            denialTag);
      },
      L"AttributeNotOwned",
      "denying release for an attribute not owned by the owner");

  // If Available reserves the owned attribute, then the regular request for
  // both attributes supersedes that reservation. The still-unowned second
  // attribute is acquired directly, while the owner is asked to release only
  // the first attribute that it actually owns.
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      requesterFirstOnly,
      initialTag);
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  requireException(
      [&] {
        requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
            object,
            requesterFirstOnly,
            initialTag);
      },
      L"AttributeAlreadyBeingAcquired",
      "repeating an If Available request after regular acquisition began");

  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "Ownership-acquisition publication release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object &&
          releaseRequest->attributes == ownerFirstOnly &&
          releaseRequest->tag == acquisitionTagBytes,
      "Ownership-acquisition publication release request returned the wrong subset or tag");

  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "Ownership-acquisition publication direct acquisition");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == requesterSecondOnly &&
          acquisition->tag == acquisitionTagBytes,
      "Ownership-acquisition publication notification returned the wrong unowned subset or tag");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterFirst) &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterSecond),
      "Ownership-acquisition publication established the wrong split ownership state");

  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterSecondOnly);
  requireException(
      [&] {
        requester.rtiAmbassador().unpublishObjectClassAttributes(
            requesterClass,
            requesterFirstOnly);
      },
      L"OwnershipAcquisitionPending",
      "unpublishing an attribute with a pending regular acquisition");

  requester.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().attributeOwnershipReleaseDenied(
      object,
      ownerFirstOnly,
      denialTag);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipUnavailable().has_value(); },
      options,
      "Ownership-acquisition publication denial");
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(
      unavailable->object == object &&
          unavailable->attributes == requesterFirstOnly &&
          unavailable->tag == denialTagBytes,
      "Ownership-acquisition publication denial returned the wrong subset or tag");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterFirst),
      "Ownership-acquisition publication denial changed ownership");
  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterFirstOnly);

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}


void scenarioOwnershipQuerySaveRestoreResults(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Ownership-query save/restore testing requires an adapter-supplied multi-attribute FOM");

  Session owner(options, model, "ownership-query-restore-owner");
  Session requester(options, model, "ownership-query-restore-requester");
  auto const federation = federationName(options, "ownership-query-save-restore-results");
  connectAndJoin(owner, requester, options, federation, options.multiAttributeFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeSecondName);
  require(
      ownerClass.isValid() && requesterClass.isValid() && ownerFirst.isValid() &&
          ownerSecond.isValid() && requesterFirst.isValid() && requesterSecond.isValid() &&
          ownerFirst != ownerSecond,
      "Ownership-query save/restore lookup returned invalid or duplicate handles");

  rti::AttributeHandleSet const ownerFirstOnly{ownerFirst};
  rti::AttributeHandleSet const requesterFirstOnly{requesterFirst};
  rti::AttributeHandleSet const requesterQuerySet{requesterFirst, requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerFirstOnly);
  requester.rtiAmbassador().publishObjectClassAttributes(requesterClass, requesterFirstOnly);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterQuerySet,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Ownership-query save/restore registration returned an invalid handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "ownership-query save/restore object discovery");

  auto const requesterFederateClass = requester.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const requesterFederateHandleAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterFederateClass,
      L"HLAfederateHandle");
  require(
      requesterFederateClass.isValid() && requesterFederateHandleAttribute.isValid(),
      "Ownership-query save/restore MOM lookup returned invalid handles");
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterFederateClass,
      rti::AttributeHandleSet{requesterFederateHandleAttribute},
      true,
      L"");
  auto findRequesterMomReflection = [&](rti::FederateHandle const& federateHandle)
      -> std::optional<ReflectionRecord> {
    auto const expectedFederateHandle = copyBytes(federateHandle.encode());
    auto const reflections = requester.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(requesterFederateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto const requesterFederateBeforeSave =
      requester.rtiAmbassador().getFederateHandle(options.memberFederateName);
  waitFor(
      requester,
      [&] { return findRequesterMomReflection(requesterFederateBeforeSave).has_value(); },
      options,
      "ownership-query save/restore joined-federate MOM reflection");
  auto const requesterMomReflection =
      findRequesterMomReflection(requesterFederateBeforeSave);
  require(
      requesterMomReflection.has_value() &&
          requester.rtiAmbassador().getKnownObjectClassHandle(
              requesterMomReflection->object) == requesterFederateClass,
      "Ownership-query save/restore MOM reflection identified the wrong object");
  auto const requesterMomObject = requesterMomReflection->object;

  std::wstring const saveLabel = L"tck-ownership-query-save-restore-results";
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &requester},
      [&] {
        return owner.federateSaveInitiations().size() >= 1U &&
            requester.federateSaveInitiations().size() >= 1U;
      },
      options,
      "ownership-query save initiation callbacks");
  require(
      owner.federateSaveInitiations().back() == saveLabel &&
          requester.federateSaveInitiations().back() == saveLabel,
      "Ownership-query save initiation returned the wrong label");
  owner.rtiAmbassador().federateSaveBegun();
  requester.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  require(
      owner.federationSavedCount() == 0U && requester.federationSavedCount() == 0U,
      "Ownership-query federation save completed before every federate finished");
  requester.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &requester},
      [&] {
        return owner.federationSavedCount() >= 1U &&
            requester.federationSavedCount() >= 1U;
      },
      options,
      "ownership-query federation-saved callbacks");
  require(
      owner.federationNotSavedReasons().empty() &&
          requester.federationNotSavedReasons().empty(),
      "Ownership-query save/restore reported a failed federation save");

  // Change the saved federate-owned result in the live execution. Restore
  // must put that attribute back with its saved owner; the never-published
  // companion remains unowned.
  std::vector<std::uint8_t> const acquisitionTagBytes{0x51U, 0x52U};
  std::vector<std::uint8_t> const confirmationTagBytes{0x53U, 0x54U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object,
      ownerFirstOnly,
      rti::VariableLengthData{});
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterFirstOnly,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().divestitureConfirmation().has_value(); },
      options,
      "ownership-query post-save divestiture confirmation request");
  auto const confirmationRequest = owner.recorder().divestitureConfirmation();
  require(
      confirmationRequest->object == object &&
          confirmationRequest->attributes == ownerFirstOnly &&
          confirmationRequest->tag == acquisitionTagBytes,
      "Ownership-query post-save confirmation request returned the wrong attributes or tag");
  owner.rtiAmbassador().confirmDivestiture(object, ownerFirstOnly, confirmationTag);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "ownership-query post-save acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == requesterFirstOnly &&
          acquisition->tag == confirmationTagBytes,
      "Ownership-query post-save acquisition returned the wrong attributes or tag");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterFirst),
      "Ownership-query post-save transfer did not change the live owner");

  auto const restoreRequestCount = owner.federationRestoreRequestsSucceeded().size();
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions(
      {&owner, &requester},
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() > restoreRequestCount &&
            owner.federationRestoreBegunCount() >= 1U &&
            requester.federationRestoreBegunCount() >= 1U &&
            owner.federateRestoreInitiations().size() >= 1U &&
            requester.federateRestoreInitiations().size() >= 1U;
      },
      options,
      "ownership-query restore initiation callbacks");
  require(
      owner.federationRestoreRequestsSucceeded().back() == saveLabel &&
          owner.federationRestoreRequestsFailed().empty() &&
          requester.federationRestoreRequestsFailed().empty(),
      "Ownership-query restore request returned an unexpected result");
  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const requesterRestore = requester.federateRestoreInitiations().back();
  require(
      ownerRestore.label == saveLabel && requesterRestore.label == saveLabel &&
          ownerRestore.postRestoreFederateHandle.isValid() &&
          requesterRestore.postRestoreFederateHandle.isValid(),
      "Ownership-query restore initiation returned invalid identity data");
  owner.rtiAmbassador().federateRestoreComplete();
  requester.rtiAmbassador().federateRestoreComplete();
  waitForSessions(
      {&owner, &requester},
      [&] {
        return owner.federationRestoredCount() >= 1U &&
            requester.federationRestoredCount() >= 1U;
      },
      options,
      "ownership-query federation-restored callbacks");
  require(
      owner.federationNotRestoredReasons().empty() &&
          requester.federationNotRestoredReasons().empty(),
      "Ownership-query restore reported a failure");

  auto const restoredObject = requester.rtiAmbassador().getObjectInstanceHandle(objectName);
  auto const restoredOwnerFirst = owner.rtiAmbassador().getAttributeHandle(
      owner.rtiAmbassador().getObjectClassHandle(options.multiAttributeObjectClassName),
      options.multiAttributeFirstName);
  auto const restoredRequesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const restoredRequesterFirst = requester.rtiAmbassador().getAttributeHandle(
      restoredRequesterClass,
      options.multiAttributeFirstName);
  auto const restoredRequesterSecond = requester.rtiAmbassador().getAttributeHandle(
      restoredRequesterClass,
      options.multiAttributeSecondName);
  require(
      restoredObject.isValid() && restoredOwnerFirst.isValid() &&
          restoredRequesterFirst.isValid() && restoredRequesterSecond.isValid(),
      "Ownership-query restore could not rebind standard object or attribute handles");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(restoredObject, restoredOwnerFirst) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              restoredObject,
              restoredRequesterFirst),
      "Ownership-query restore did not reinstate the saved federate owner");

  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().queryAttributeOwnership(
      restoredObject,
      rti::AttributeHandleSet{restoredRequesterFirst, restoredRequesterSecond});
  waitFor(
      requester,
      [&] {
        return requester.recorder().ownershipInformationEvents().size() >= 1U &&
            requester.recorder().ownershipNotOwnedEvents().size() >= 1U;
      },
      options,
      "restored mixed ownership-query results");
  auto const ownershipInformation = requester.recorder().ownershipInformationEvents();
  auto const ownershipNotOwned = requester.recorder().ownershipNotOwnedEvents();
  require(
      ownershipInformation.size() == 1U && ownershipNotOwned.size() == 1U &&
          ownershipInformation.front().object == restoredObject &&
          ownershipInformation.front().attributes == rti::AttributeHandleSet{restoredRequesterFirst} &&
          ownershipInformation.front().owner == ownerRestore.postRestoreFederateHandle,
      "Restored ownership query returned the wrong federate-owned subset or owner handle");
  require(
      ownershipNotOwned.front().object == restoredObject &&
          ownershipNotOwned.front().attributes ==
              rti::AttributeHandleSet{restoredRequesterSecond} &&
          requester.recorder().ownershipOwnedByRtiEvents().empty(),
      "Restored ownership query did not distinguish the unowned attribute");

  auto const requesterFederateAfterRestore = requester.rtiAmbassador().getFederateHandle(
      options.memberFederateName);
  require(
      requesterFederateAfterRestore == requesterRestore.postRestoreFederateHandle &&
          requester.rtiAmbassador().getKnownObjectClassHandle(requesterMomObject) ==
              requesterFederateClass,
      "Ownership-query restore did not preserve the joined-federate MOM object identity");
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().queryAttributeOwnership(
      requesterMomObject,
      rti::AttributeHandleSet{requesterFederateHandleAttribute});
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipOwnedByRtiEvents().size() >= 1U; },
      options,
      "restored RTI-owned MOM attribute query");
  auto const ownershipOwnedByRti = requester.recorder().ownershipOwnedByRtiEvents();
  require(
      ownershipOwnedByRti.size() == 1U &&
          ownershipOwnedByRti.front().object == requesterMomObject &&
          ownershipOwnedByRti.front().attributes ==
              rti::AttributeHandleSet{requesterFederateHandleAttribute} &&
          requester.recorder().ownershipInformationEvents().empty() &&
          requester.recorder().ownershipNotOwnedEvents().empty(),
      "Restored MOM ownership query did not identify exactly the RTI-owned attribute");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}


void scenarioOwnershipQueryPartitionCleanup(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Ownership-query testing requires an adapter-supplied multi-attribute FOM");

  Session owner(options, model, "ownership-query-owner");
  Session requester(options, model, "ownership-query-requester");
  auto const federation = federationName(
      options,
      "ownership-query-partition-cleanup");
  connectAndJoin(owner, requester, options, federation, options.multiAttributeFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeSecondName);
  require(
      ownerClass.isValid() && requesterClass.isValid(),
      "Ownership-query object-class lookup returned an invalid handle");
  require(
      ownerFirst.isValid() && ownerSecond.isValid() && requesterFirst.isValid() &&
          requesterSecond.isValid(),
      "Ownership-query attribute lookup returned an invalid handle");
  require(
      ownerFirst != ownerSecond,
      "Ownership-query adapter supplied duplicate attribute names");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst};
  rti::AttributeHandleSet const requesterAttributes{requesterFirst, requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Ownership-query registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "Ownership-query object discovery");

  rti::ObjectInstanceHandle const invalidObject;
  rti::AttributeHandle const invalidAttribute;
  rti::AttributeHandleSet const invalidAttributes{invalidAttribute};
  requireException(
      [&] {
        requester.rtiAmbassador().queryAttributeOwnership(
            invalidObject,
            requesterAttributes);
      },
      L"ObjectInstanceNotKnown",
      "querying ownership for an unknown object instance");
  requireException(
      [&] {
        requester.rtiAmbassador().queryAttributeOwnership(
            object,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "querying ownership for an undefined attribute");

  // A query over mixed ownership is reported as exact owned and unowned
  // attribute subsets. Callback order is intentionally not part of the check.
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().queryAttributeOwnership(object, requesterAttributes);
  waitFor(
      requester,
      [&] {
        return requester.recorder().ownershipInformationEvents().size() == 1U &&
            requester.recorder().ownershipNotOwnedEvents().size() == 1U;
      },
      options,
      "the partitioned ownership query result");
  auto const ownershipInformation = requester.recorder().ownershipInformationEvents();
  auto const ownershipNotOwned = requester.recorder().ownershipNotOwnedEvents();
  require(
      ownershipInformation.front().object == object &&
          ownershipInformation.front().attributes ==
              rti::AttributeHandleSet{requesterFirst} &&
          ownershipInformation.front().owner == owner.federateHandle(),
      "ownership query did not report the exact owned attribute subset");
  require(
      ownershipNotOwned.front().object == object &&
          ownershipNotOwned.front().attributes ==
              rti::AttributeHandleSet{requesterSecond},
      "ownership query did not report the exact unowned attribute subset");
  require(
      requester.recorder().ownershipOwnedByRtiEvents().empty(),
      "ownership query reported an ordinary unowned attribute as RTI-owned");

  // Removing the object invalidates any query result that has not yet crossed
  // the callback boundary. Immediate delivery may already have produced the
  // result; in either model removal must not append a stale second result.
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().queryAttributeOwnership(object, requesterAttributes);
  auto const informationBeforeRemoval =
      requester.recorder().ownershipInformationEvents().size();
  auto const notOwnedBeforeRemoval =
      requester.recorder().ownershipNotOwnedEvents().size();
  owner.rtiAmbassador().deleteObjectInstance(object, rti::VariableLengthData{});
  waitFor(
      requester,
      [&] { return requester.recorder().removals().size() == 1U; },
      options,
      "the removed object after an ownership query");
  for (int pass = 0; pass != 4; ++pass) {
    requester.pump();
  }
  require(
      requester.recorder().ownershipInformationEvents().size() ==
          informationBeforeRemoval &&
          requester.recorder().ownershipNotOwnedEvents().size() ==
              notOwnedBeforeRemoval,
      "object removal delivered stale ownership-query results");
  require(
      requester.recorder().removals().front().object == object,
      "object removal after an ownership query returned the wrong object");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}


void scenarioOwnershipAcquisitionCancellationTransferRace(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "ownership-race-owner");
  Session requester(options, model, "ownership-race-requester");
  auto const federation = federationName(
      options,
      "ownership-acquisition-cancellation-transfer-race");
  connectAndJoin(owner, requester, options, federation, options.fom);

  rti::ObjectClassHandle ownerClass;
  rti::AttributeHandle ownerAttribute;
  rti::InteractionClassHandle ownerInteraction;
  rti::ParameterHandle ownerParameter;
  handles(owner, options, ownerClass, ownerAttribute, ownerInteraction, ownerParameter);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && requesterClass.isValid() &&
          requesterAttribute.isValid(),
      "Ownership cancellation race lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Ownership cancellation race registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "ownership cancellation race discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
    requester.pump();
  }

  std::vector<std::uint8_t> const acquisitionTagBytes{0xc1U, 0x25U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xd1U, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();

  if (model == rti::HLA_IMMEDIATE) {
    requester.rtiAmbassador().attributeOwnershipAcquisition(
        object,
        requesterAttributes,
        acquisitionTag);
    rti::AttributeHandleSet divestedAttributes;
    owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
        object,
        ownerAttributes,
        divestitureTag,
        divestedAttributes);
    require(
        divestedAttributes == ownerAttributes,
        "Immediate ownership cancellation race returned the wrong divested set");
    waitFor(
        requester,
        [&] { return requester.recorder().ownershipAcquisition().has_value(); },
        options,
        "immediate ownership transfer notification");
    require(
        !requester.recorder().ownershipAcquisitionCancellation().has_value(),
        "Immediate ownership transfer delivered a stale cancellation confirmation");
    auto const notification = requester.recorder().ownershipAcquisition();
    require(
        notification->object == object &&
            notification->attributes == requesterAttributes &&
            notification->tag == divestitureTagBytes,
        "Immediate ownership transfer notification returned the wrong data");
    require(
        !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
            requester.rtiAmbassador().isAttributeOwnedByFederate(
                object,
                requesterAttribute),
        "Immediate ownership transfer did not transfer ownership");

    requester.rtiAmbassador().unpublishObjectClassAttributes(
        requesterClass,
        requesterAttributes);
    owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
    requester.resign(rti::NO_ACTION);
    owner.rtiAmbassador().destroyFederationExecution(federation);
    requester.disconnect();
    owner.disconnect();
    return;
  }

  std::mutex coordinationMutex;
  std::condition_variable coordination;
  bool releaseCallbackEntered = false;
  bool cancellationCompleted = false;
  std::exception_ptr cancellationFailure;
  std::exception_ptr divestitureFailure;
  owner.recorder().setOwnershipReleaseHook(
      [&](rti::ObjectInstanceHandle const& callbackObject,
          rti::AttributeHandleSet const& callbackAttributes,
          rti::VariableLengthData const&) {
        {
          std::lock_guard<std::mutex> lock(coordinationMutex);
          releaseCallbackEntered = true;
        }
        coordination.notify_all();

        std::unique_lock<std::mutex> lock(coordinationMutex);
        bool const observedCancellation = coordination.wait_for(
            lock,
            std::chrono::milliseconds(options.timeoutMilliseconds),
            [&] { return cancellationCompleted; });
        lock.unlock();
        if (!observedCancellation) {
          std::lock_guard<std::mutex> failureLock(coordinationMutex);
          divestitureFailure = std::make_exception_ptr(
              std::runtime_error(
                  "Ownership cancellation race did not observe cancellation"));
          return;
        }

        try {
          rti::AttributeHandleSet divestedAttributes;
          owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
              callbackObject,
              callbackAttributes,
              divestitureTag,
              divestedAttributes);
          require(
              divestedAttributes == callbackAttributes,
              "Ownership cancellation race Divestiture If Wanted returned the wrong set");
        } catch (...) {
          std::lock_guard<std::mutex> failureLock(coordinationMutex);
          divestitureFailure = std::current_exception();
        }
      });

  std::thread canceller([&] {
    std::unique_lock<std::mutex> lock(coordinationMutex);
    bool const observedRelease = coordination.wait_for(
        lock,
        std::chrono::milliseconds(options.timeoutMilliseconds),
        [&] { return releaseCallbackEntered; });
    lock.unlock();
    if (!observedRelease) {
      cancellationFailure = std::make_exception_ptr(
          std::runtime_error(
              "Ownership cancellation race did not observe the release callback"));
    } else {
      try {
        requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object,
            requesterAttributes);
      } catch (...) {
        cancellationFailure = std::current_exception();
      }
    }
    {
      std::lock_guard<std::mutex> completionLock(coordinationMutex);
      cancellationCompleted = true;
    }
    coordination.notify_all();
  });

  std::exception_ptr driverFailure;
  try {
    requester.rtiAmbassador().attributeOwnershipAcquisition(
        object,
        requesterAttributes,
        acquisitionTag);
    owner.pump();
  } catch (...) {
    driverFailure = std::current_exception();
  }
  canceller.join();
  owner.recorder().setOwnershipReleaseHook({});
  if (driverFailure) {
    std::rethrow_exception(driverFailure);
  }
  auto describeFailure = [](std::exception_ptr const& failure) {
    if (!failure) {
      return std::string{};
    }
    try {
      std::rethrow_exception(failure);
    } catch (rti::Exception const& error) {
      return toNarrow(error.name());
    } catch (std::exception const& error) {
      return std::string(error.what());
    } catch (...) {
      return std::string("unknown exception");
    }
  };
  require(
      !cancellationFailure,
      "Ownership cancellation race acquisition cancellation failed: " +
          describeFailure(cancellationFailure));
  require(
      !divestitureFailure,
      "Ownership cancellation race divestiture failed: " +
          describeFailure(divestitureFailure));

  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "ownership cancellation race acquisition notification");
  require(
      !requester.recorder().ownershipAcquisitionCancellation().has_value(),
      "Ownership cancellation race delivered a stale cancellation confirmation");
  auto const notification = requester.recorder().ownershipAcquisition();
  require(
      notification->object == object &&
          notification->attributes == requesterAttributes &&
          notification->tag == divestitureTagBytes,
      "Ownership cancellation race acquisition notification returned the wrong data");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "Ownership cancellation race did not transfer ownership");

  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  requester.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}


void scenarioOwnershipTransferRegionalUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Ownership transfer/update-region testing requires an adapter-supplied dimensional FOM");

  Session owner(options, model, "owner");
  Session newOwner(options, model, "member");
  Session receiver(options, model, "receiver");
  auto const federation = federationName(options, "ownership-transfer-regional-update");
  owner.connect();
  newOwner.connect();
  receiver.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  newOwner.join(
      options.memberFederateName + L"-ownership-transfer",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-ownership-receiver",
      options.federateType,
      federation);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const newOwnerHandles = ddmHandles(newOwner, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      owner,
      options,
      ownerHandles,
      "ownership transfer/update-region");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const newOwnerAttributes{newOwnerHandles.attribute};
  rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  newOwner.rtiAmbassador().publishObjectClassAttributes(
      newOwnerHandles.objectClass,
      newOwnerAttributes);

  auto const ownerRegion = createDdmRegion(
      owner,
      ownerHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const newOwnerRegion = createDdmRegion(
      newOwner,
      newOwnerHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const receiverOwnerRegion = createDdmRegion(
      receiver,
      receiverHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const receiverReplacementRegion = createDdmRegion(
      receiver,
      receiverHandles,
      6UL,
      8UL,
      6UL,
      8UL);

  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const newOwnerPair{{
      newOwnerAttributes,
      rti::RegionHandleSet{newOwnerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      rti::RegionHandleSet{receiverOwnerRegion, receiverReplacementRegion},
  }};

  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "ownership transfer receiver did not enable region designator callbacks");
  newOwner.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      newOwnerHandles.objectClass,
      newOwnerPair,
      true,
      L"");
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass,
      ownerPair);
  require(
      object.isValid(),
      "ownership transfer/update-region registration returned an invalid object handle");
  waitForSessions(
      {&owner, &newOwner, &receiver},
      [&] {
        return newOwner.recorder().hasDiscovery(object) &&
            receiver.recorder().hasDiscovery(object);
      },
      options,
      "ownership transfer/update-region object discovery");

  std::vector<std::uint8_t> ownerValue{0x4FU, 0x4CUL, 0x44U, 0x25U};
  std::vector<std::uint8_t> defaultValue{0x44U, 0x45U, 0x46U, 0x25U};
  std::vector<std::uint8_t> replacementValue{0x4EU, 0x45U, 0x57U, 0x25U};
  std::vector<std::uint8_t> acquisitionTagBytes{0x41U, 0x43U, 0x51U, 0x25U};
  std::vector<std::uint8_t> divestitureTagBytes{0x44U, 0x49U, 0x56U, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());

  rti::AttributeHandleValueMap ownerValues;
  ownerValues.emplace(
      ownerHandles.attribute,
      rti::VariableLengthData(ownerValue.data(), ownerValue.size()));
  owner.rtiAmbassador().updateAttributeValues(object, ownerValues, divestitureTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "ownership transfer/update-region initial reflection");
  auto const initialReflection = receiver.recorder().reflection();
  require(
      initialReflection.object == object &&
          initialReflection.values.size() == 1U &&
          initialReflection.values.count(receiverHandles.attribute) == 1U,
      "ownership transfer/update-region initial reflection returned the wrong attribute set");
  require(
      copyBytes(initialReflection.values.at(receiverHandles.attribute)) == ownerValue &&
          initialReflection.tag == divestitureTagBytes &&
          initialReflection.producer == owner.federateHandle() &&
          initialReflection.transportation.isValid(),
      "ownership transfer/update-region initial reflection lost message metadata");
  require(
      initialReflection.regions.has_value() &&
          initialReflection.regions->size() == 1U &&
          initialReflection.regions->count(ownerRegion) == 1U,
      "ownership transfer/update-region initial reflection lost the source region");

  newOwner.recorder().clearOwnershipRecords();
  rti::AttributeHandleSet divestedAttributes;
  // HLA_IMMEDIATE reports an unavailable If Available request inline. Use a
  // regular pending acquisition in that model so this test reaches the same
  // Divestiture If Wanted transfer boundary as evoked delivery.
  if (model == rti::HLA_IMMEDIATE) {
    newOwner.rtiAmbassador().attributeOwnershipAcquisition(
        object,
        newOwnerAttributes,
        acquisitionTag);
  } else {
    newOwner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
        object,
        newOwnerAttributes,
        acquisitionTag);
  }
  require(
      !newOwner.recorder().ownershipAcquisition().has_value(),
      "If Available acquisition completed before the current owner divested");
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      divestitureTag,
      divestedAttributes);
  require(
      divestedAttributes == ownerAttributes,
      "If Wanted divestiture returned the wrong attribute set");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerHandles.attribute) &&
          newOwner.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              newOwnerHandles.attribute),
      "ownership transfer did not move the attribute to the pending acquirer");
  waitFor(
      newOwner,
      [&] { return newOwner.recorder().ownershipAcquisition().has_value(); },
      options,
      "ownership transfer/update-region acquisition notification");
  auto const acquisition = newOwner.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == newOwnerAttributes &&
          acquisition->tag == divestitureTagBytes,
      "ownership transfer/update-region acquisition callback returned the wrong data");

  receiver.recorder().clearReflection();
  requireException(
      [&] {
        owner.rtiAmbassador().updateAttributeValues(object, ownerValues, divestitureTag);
      },
      L"AttributeNotOwned",
      "former owner update after ownership transfer");

  rti::AttributeHandleValueMap defaultValues;
  defaultValues.emplace(
      newOwnerHandles.attribute,
      rti::VariableLengthData(defaultValue.data(), defaultValue.size()));
  newOwner.rtiAmbassador().updateAttributeValues(object, defaultValues, divestitureTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "ownership transfer/update-region default-source reflection");
  auto const defaultReflection = receiver.recorder().reflection();
  require(
      defaultReflection.object == object &&
          defaultReflection.values.size() == 1U &&
          defaultReflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(defaultReflection.values.at(receiverHandles.attribute)) == defaultValue,
      "ownership transfer/update-region default-source reflection returned the wrong value");
  require(
      defaultReflection.tag == divestitureTagBytes &&
          defaultReflection.producer == newOwner.federateHandle() &&
          defaultReflection.transportation.isValid(),
      "ownership transfer/update-region default-source reflection lost message metadata");
  require(
      !defaultReflection.regions.has_value() || defaultReflection.regions->empty(),
      "former owner's source region was incorrectly inherited by the new owner");

  receiver.recorder().clearReflection();
  newOwner.rtiAmbassador().setRangeBounds(
      newOwnerRegion,
      newOwnerHandles.firstDimension,
      rti::RangeBounds(6UL, 8UL));
  newOwner.rtiAmbassador().setRangeBounds(
      newOwnerRegion,
      newOwnerHandles.secondDimension,
      rti::RangeBounds(6UL, 8UL));
  newOwner.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{newOwnerRegion});
  newOwner.rtiAmbassador().associateRegionsForUpdates(object, newOwnerPair);
  rti::AttributeHandleValueMap replacementValues;
  replacementValues.emplace(
      newOwnerHandles.attribute,
      rti::VariableLengthData(replacementValue.data(), replacementValue.size()));
  newOwner.rtiAmbassador().updateAttributeValues(
      object,
      replacementValues,
      acquisitionTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "ownership transfer/update-region replacement reflection");
  auto const replacementReflection = receiver.recorder().reflection();
  require(
      replacementReflection.object == object &&
          replacementReflection.values.size() == 1U &&
          replacementReflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(replacementReflection.values.at(receiverHandles.attribute)) ==
              replacementValue,
      "ownership transfer/update-region replacement reflection returned the wrong value");
  require(
      replacementReflection.tag == acquisitionTagBytes &&
          replacementReflection.producer == newOwner.federateHandle() &&
          replacementReflection.transportation.isValid(),
      "ownership transfer/update-region replacement reflection lost message metadata");
  require(
      replacementReflection.regions.has_value() &&
          replacementReflection.regions->size() == 1U &&
          replacementReflection.regions->count(newOwnerRegion) == 1U,
      "new owner replacement association did not supply its source region");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair);
  newOwner.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      newOwnerHandles.objectClass,
      newOwnerPair);
  newOwner.rtiAmbassador().unassociateRegionsForUpdates(object, newOwnerPair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  newOwner.rtiAmbassador().unpublishObjectClassAttributes(
      newOwnerHandles.objectClass,
      newOwnerAttributes);
  receiver.rtiAmbassador().deleteRegion(receiverOwnerRegion);
  receiver.rtiAmbassador().deleteRegion(receiverReplacementRegion);
  newOwner.rtiAmbassador().deleteRegion(newOwnerRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);

  receiver.resign(rti::NO_ACTION);
  newOwner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  newOwner.disconnect();
  owner.disconnect();
}


void scenarioOwnershipTransferDeferredRegionalUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Deferred ownership transfer/update-region testing requires an adapter-supplied dimensional FOM");

  Session owner(options, model, "owner");
  Session newOwner(options, model, "member");
  Session receiver(options, model, "receiver");
  auto const federation = federationName(
      options,
      "ownership-transfer-deferred-regional-update");
  owner.connect();
  newOwner.connect();
  receiver.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  newOwner.join(
      options.memberFederateName + L"-ownership-transfer-deferred",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-ownership-receiver-deferred",
      options.federateType,
      federation);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const newOwnerHandles = ddmHandles(newOwner, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      owner,
      options,
      ownerHandles,
      "deferred ownership transfer/update-region");
  require(
      ownerHandles.objectClass == newOwnerHandles.objectClass &&
          ownerHandles.objectClass == receiverHandles.objectClass &&
          ownerHandles.attribute == newOwnerHandles.attribute &&
          ownerHandles.attribute == receiverHandles.attribute,
      "deferred ownership transfer members resolved different declaration handles");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const newOwnerAttributes{newOwnerHandles.attribute};
  rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  newOwner.rtiAmbassador().publishObjectClassAttributes(
      newOwnerHandles.objectClass,
      newOwnerAttributes);

  auto const ownerRegion = createDdmRegion(
      owner,
      ownerHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const deferredRegion = createDdmRegion(
      newOwner,
      newOwnerHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const receiverOwnerRegion = createDdmRegion(
      receiver,
      receiverHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const receiverDeferredRegion = createDdmRegion(
      receiver,
      receiverHandles,
      1UL,
      3UL,
      1UL,
      3UL);

  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const deferredPair{{
      newOwnerAttributes,
      rti::RegionHandleSet{deferredRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      rti::RegionHandleSet{receiverOwnerRegion, receiverDeferredRegion},
  }};

  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  newOwner.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      newOwnerHandles.objectClass,
      deferredPair,
      true,
      L"");
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass,
      ownerPair);
  require(
      object.isValid(),
      "deferred ownership transfer/update-region registration returned an invalid object handle");
  waitForSessions(
      {&owner, &newOwner, &receiver},
      [&] {
        return newOwner.recorder().hasDiscovery(object) &&
            receiver.recorder().hasDiscovery(object);
      },
      options,
      "deferred ownership transfer/update-region object discovery");

  std::vector<std::uint8_t> const ownerValue{
      0x4FU,
      0x4CUL,
      0x44U,
      0x26U};
  std::vector<std::uint8_t> const newOwnerValue{
      0x4EU,
      0x45U,
      0x57U,
      0x26U};
  std::vector<std::uint8_t> const acquisitionTagBytes{
      0x41U,
      0x43U,
      0x51U,
      0x26U};
  std::vector<std::uint8_t> const divestitureTagBytes{
      0x44U,
      0x49U,
      0x56U,
      0x26U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());

  // A non-owner can establish its future update-region association, but that
  // association must not retarget updates while the current owner still owns
  // the attribute.
  newOwner.rtiAmbassador().associateRegionsForUpdates(object, deferredPair);
  rti::AttributeHandleValueMap ownerValues;
  ownerValues.emplace(
      ownerHandles.attribute,
      rti::VariableLengthData(ownerValue.data(), ownerValue.size()));
  owner.rtiAmbassador().updateAttributeValues(object, ownerValues, divestitureTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "deferred ownership transfer/update-region owner reflection");
  auto const ownerReflection = receiver.recorder().reflection();
  require(
      ownerReflection.object == object &&
          ownerReflection.values.size() == 1U &&
          ownerReflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(ownerReflection.values.at(receiverHandles.attribute)) == ownerValue &&
          ownerReflection.tag == divestitureTagBytes &&
          ownerReflection.producer == owner.federateHandle() &&
          ownerReflection.transportation.isValid(),
      "deferred ownership transfer/update-region owner reflection returned the wrong metadata");
  require(
      ownerReflection.regions.has_value() &&
          ownerReflection.regions->size() == 1U &&
          ownerReflection.regions->count(ownerRegion) == 1U,
      "deferred ownership transfer/update-region association retargeted the owner update");

  newOwner.recorder().clearOwnershipRecords();
  receiver.recorder().clearReflection();
  rti::AttributeHandleSet divestedAttributes;
  owner.recorder().clearOwnershipRecords();
  newOwner.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      newOwnerAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "deferred ownership transfer release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object &&
          releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "deferred ownership transfer release request returned the wrong metadata");
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      divestitureTag,
      divestedAttributes);
  waitFor(
      newOwner,
      [&] { return newOwner.recorder().ownershipAcquisition().has_value(); },
      options,
      "deferred ownership transfer acquisition notification");
  require(
      divestedAttributes == ownerAttributes &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              ownerHandles.attribute) &&
          newOwner.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              newOwnerHandles.attribute),
      "deferred ownership transfer did not move the attribute to the pending acquirer");
  auto const acquisition = newOwner.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == newOwnerAttributes &&
          acquisition->tag == divestitureTagBytes,
      "deferred ownership transfer acquisition callback returned the wrong metadata");

  requireException(
      [&] { owner.rtiAmbassador().updateAttributeValues(object, ownerValues, divestitureTag); },
      L"AttributeNotOwned",
      "former owner update after deferred ownership transfer");

  // Acquisition promotes the deferred association without another associate
  // call.  The source region must now be the new owner's region.
  rti::AttributeHandleValueMap newOwnerValues;
  newOwnerValues.emplace(
      newOwnerHandles.attribute,
      rti::VariableLengthData(newOwnerValue.data(), newOwnerValue.size()));
  newOwner.rtiAmbassador().updateAttributeValues(
      object,
      newOwnerValues,
      acquisitionTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "deferred ownership transfer promoted-region reflection");
  auto const promotedReflection = receiver.recorder().reflection();
  require(
      promotedReflection.object == object &&
          promotedReflection.values.size() == 1U &&
          promotedReflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(promotedReflection.values.at(receiverHandles.attribute)) == newOwnerValue &&
          promotedReflection.tag == acquisitionTagBytes &&
          promotedReflection.producer == newOwner.federateHandle() &&
          promotedReflection.transportation.isValid(),
      "deferred ownership transfer promoted-region reflection returned the wrong metadata");
  require(
      promotedReflection.regions.has_value() &&
          promotedReflection.regions->size() == 1U &&
          promotedReflection.regions->count(deferredRegion) == 1U,
      "deferred ownership transfer did not promote the pending update-region association");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair);
  newOwner.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      newOwnerHandles.objectClass,
      deferredPair);
  newOwner.rtiAmbassador().unassociateRegionsForUpdates(object, deferredPair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  newOwner.rtiAmbassador().unpublishObjectClassAttributes(
      newOwnerHandles.objectClass,
      newOwnerAttributes);
  receiver.rtiAmbassador().deleteRegion(receiverOwnerRegion);
  receiver.rtiAmbassador().deleteRegion(receiverDeferredRegion);
  newOwner.rtiAmbassador().deleteRegion(deferredRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  receiver.resign(rti::NO_ACTION);
  newOwner.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  newOwner.disconnect();
  owner.disconnect();
}

void scenarioOwnershipTransferDeferredRegionalUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipTransferDeferredRegionalUpdate(options, model);
}

} // namespace hla_rti_cpp_tck::detail
