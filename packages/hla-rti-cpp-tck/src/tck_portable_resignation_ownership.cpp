#include "tck_portable_resignation_ownership.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioResignDeleteObjectLookupBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Resign-delete object lookup testing requires an adapter-supplied FOM");

  Session owner(options, model, "resign-delete-lookup-owner");
  Session member(options, model, "resign-delete-lookup-member");
  auto const federation =
      federationName(options, "resign-delete-object-lookup-boundary");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);

  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      memberAttributes);

  member.recorder().clearDiscovery();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Resign-delete lookup setup returned an invalid object handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!objectName.empty(), "Resign-delete lookup setup returned an empty object name");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "resign-delete lookup object discovery");
  auto const discovery = member.recorder().discovery();
  require(
      discovery.object == object && discovery.objectClass == memberClass &&
          discovery.name == objectName &&
          member.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          member.rtiAmbassador().getObjectInstanceName(discovery.object) == objectName &&
          member.rtiAmbassador().getKnownObjectClassHandle(discovery.object) == memberClass,
      "Discovered object name, handle, and class did not resolve before resign-time deletion");

  member.recorder().clearRemovals();
  owner.resign(rti::DELETE_OBJECTS);
  waitFor(
      member,
      [&] {
        return member.recorder().hasRemoval(
            object,
            {},
            owner.federateHandle());
      },
      options,
      "resign-delete lookup removal callback");
  requireException(
      [&] {
        static_cast<void>(
            member.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up the name after resign-time object deletion");
  requireException(
      [&] {
        static_cast<void>(
            member.rtiAmbassador().getObjectInstanceName(discovery.object));
      },
      L"ObjectInstanceNotKnown",
      "looking up the handle's name after resign-time object deletion");
  requireException(
      [&] {
        static_cast<void>(
            member.rtiAmbassador().getKnownObjectClassHandle(discovery.object));
      },
      L"ObjectInstanceNotKnown",
      "querying the known class after resign-time object deletion");

  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  member.disconnect();
}

void scenarioResignDeleteObjectLookupBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioResignDeleteObjectLookupBoundaryPortable(options, model);
}

void scenarioResignNoActionOwnedAttributesBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "NO_ACTION resignation testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty(),
      "NO_ACTION resignation testing requires adapter-supplied object and attribute names");

  Session owner(options, model, "resign-no-action-owned-attributes-owner");
  Session member(options, model, "resign-no-action-owned-attributes-member");
  auto const federation =
      federationName(options, "resign-no-action-owned-attributes-boundary");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const objectClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "NO_ACTION resignation fixture lookup returned an invalid standard handle");
  rti::AttributeHandleSet const attributes{attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);

  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(
      object.isValid(),
      "NO_ACTION resignation fixture returned an invalid object handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!objectName.empty(), "NO_ACTION resignation fixture returned an empty object name");

  requireException(
      [&] { owner.resign(rti::NO_ACTION); },
      L"FederateOwnsAttributes",
      "resigning with NO_ACTION while owning an object's attributes");
  require(
      owner.rtiAmbassador().getObjectInstanceName(object) == objectName,
      "rejected NO_ACTION resignation changed membership or object identity");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  member.disconnect();
}

void scenarioResignNoActionOwnedAttributesBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioResignNoActionOwnedAttributesBoundaryPortable(options, model);
}

void scenarioResignDeleteObjectsThenDivestCrossObjectEffectsPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "DELETE_OBJECTS_THEN_DIVEST testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty(),
      "DELETE_OBJECTS_THEN_DIVEST testing requires adapter-supplied object and attribute names");

  Session registrar(options, model, "resign-delete-then-divest-registrar");
  Session departing(options, model, "resign-delete-then-divest-departing");
  Session recipient(options, model, "resign-delete-then-divest-recipient");
  auto const federation =
      federationName(options, "resign-delete-objects-then-divest-cross-object-effects");
  connectAndJoin(registrar, departing, options, federation, options.fom);
  recipient.connect();
  recipient.join(
      options.memberFederateName + L"-divest-recipient",
      options.federateType,
      federation);

  auto const registrarClass =
      registrar.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const departingClass =
      departing.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const recipientClass =
      recipient.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const registrarAttribute = registrar.rtiAmbassador().getAttributeHandle(
      registrarClass,
      options.attributeName);
  auto const departingAttribute = departing.rtiAmbassador().getAttributeHandle(
      departingClass,
      options.attributeName);
  auto const recipientAttribute = recipient.rtiAmbassador().getAttributeHandle(
      recipientClass,
      options.attributeName);
  require(
      registrarClass.isValid() && departingClass.isValid() &&
          recipientClass.isValid() && registrarAttribute.isValid() &&
          departingAttribute.isValid() && recipientAttribute.isValid(),
      "DELETE_OBJECTS_THEN_DIVEST fixture lookup returned an invalid standard handle");

  rti::AttributeHandleSet const registrarAttributes{registrarAttribute};
  rti::AttributeHandleSet const departingAttributes{departingAttribute};
  rti::AttributeHandleSet const recipientAttributes{recipientAttribute};
  registrar.rtiAmbassador().publishObjectClassAttributes(
      registrarClass,
      registrarAttributes);
  departing.rtiAmbassador().publishObjectClassAttributes(
      departingClass,
      departingAttributes);
  recipient.rtiAmbassador().publishObjectClassAttributes(
      recipientClass,
      recipientAttributes);
  registrar.rtiAmbassador().subscribeObjectClassAttributes(
      registrarClass,
      registrarAttributes,
      true,
      L"");
  departing.rtiAmbassador().subscribeObjectClassAttributes(
      departingClass,
      departingAttributes,
      true,
      L"");
  recipient.rtiAmbassador().subscribeObjectClassAttributes(
      recipientClass,
      recipientAttributes,
      true,
      L"");

  auto const retainedObject =
      registrar.rtiAmbassador().registerObjectInstance(registrarClass);
  require(
      retainedObject.isValid(),
      "DELETE_OBJECTS_THEN_DIVEST fixture returned an invalid retained-object handle");
  auto const retainedObjectName =
      registrar.rtiAmbassador().getObjectInstanceName(retainedObject);
  require(
      !retainedObjectName.empty(),
      "DELETE_OBJECTS_THEN_DIVEST fixture returned an empty retained-object name");
  waitForSessions(
      {&departing, &recipient},
      [&] {
        return departing.recorder().hasDiscovery(retainedObject) &&
            recipient.recorder().hasDiscovery(retainedObject);
      },
      options,
      "DELETE_OBJECTS_THEN_DIVEST retained-object discovery");

  registrar.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      registrarAttributes,
      rti::VariableLengthData{});
  std::vector<std::uint8_t> const departingAcquisitionTagBytes{
      0xD7U,
      0x25U,
      0x01U};
  rti::VariableLengthData const departingAcquisitionTag(
      departingAcquisitionTagBytes.data(),
      departingAcquisitionTagBytes.size());
  departing.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      departingAttributes,
      departingAcquisitionTag);
  waitFor(
      departing,
      [&] { return departing.recorder().ownershipAcquisition().has_value(); },
      options,
      "DELETE_OBJECTS_THEN_DIVEST setup ownership acquisition");
  auto const departingAcquisition =
      departing.recorder().ownershipAcquisition();
  require(
      departingAcquisition->object == retainedObject &&
          departingAcquisition->attributes == departingAttributes &&
          departingAcquisition->tag == departingAcquisitionTagBytes &&
          departing.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject,
              departingAttribute),
      "DELETE_OBJECTS_THEN_DIVEST setup did not transfer the retained-object attribute");

  auto const deletedObject =
      departing.rtiAmbassador().registerObjectInstance(departingClass);
  require(
      deletedObject.isValid() && deletedObject != retainedObject,
      "DELETE_OBJECTS_THEN_DIVEST fixture returned an invalid or duplicate departing-object handle");
  auto const deletedObjectName =
      departing.rtiAmbassador().getObjectInstanceName(deletedObject);
  require(
      !deletedObjectName.empty() && deletedObjectName != retainedObjectName,
      "DELETE_OBJECTS_THEN_DIVEST fixture returned an invalid or duplicate departing-object name");
  waitForSessions(
      {&registrar, &recipient},
      [&] {
        return registrar.recorder().hasDiscovery(deletedObject) &&
            recipient.recorder().hasDiscovery(deletedObject);
      },
      options,
      "DELETE_OBJECTS_THEN_DIVEST departing-object discovery");
  require(
      departing.rtiAmbassador().isAttributeOwnedByFederate(
          deletedObject,
          departingAttribute),
      "DELETE_OBJECTS_THEN_DIVEST fixture did not establish departing-object attribute ownership");

  auto const departingFederateHandle = departing.federateHandle();
  registrar.recorder().clearRemovals();
  recipient.recorder().clearRemovals();
  departing.resign(rti::DELETE_OBJECTS_THEN_DIVEST);
  waitForSessions(
      {&registrar, &recipient},
      [&] {
        return registrar.recorder().hasRemoval(
                   deletedObject,
                   {},
                   departingFederateHandle) &&
            recipient.recorder().hasRemoval(
                deletedObject,
                {},
                departingFederateHandle);
      },
      options,
      "DELETE_OBJECTS_THEN_DIVEST object removal callbacks");

  auto const registrarRemovals = registrar.recorder().removals();
  auto const recipientRemovals = recipient.recorder().removals();
  require(
      registrarRemovals.size() == 1U &&
          registrarRemovals.front().object == deletedObject &&
          registrarRemovals.front().tag.empty() &&
          registrarRemovals.front().producer == departingFederateHandle &&
          recipientRemovals.size() == 1U &&
          recipientRemovals.front().object == deletedObject &&
          recipientRemovals.front().tag.empty() &&
          recipientRemovals.front().producer == departingFederateHandle,
      "DELETE_OBJECTS_THEN_DIVEST returned incorrect or duplicate object-removal metadata");
  require(
      registrar.rtiAmbassador().getObjectInstanceHandle(retainedObjectName) ==
              retainedObject &&
          recipient.rtiAmbassador().getObjectInstanceHandle(retainedObjectName) ==
              retainedObject,
      "DELETE_OBJECTS_THEN_DIVEST removed an object registered by another federate");
  requireException(
      [&] {
        static_cast<void>(
            recipient.rtiAmbassador().getObjectInstanceHandle(deletedObjectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up the departing federate's object after DELETE_OBJECTS_THEN_DIVEST");

  recipient.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const recipientAcquisitionTagBytes{
      0xD7U,
      0x25U,
      0x02U};
  rti::VariableLengthData const recipientAcquisitionTag(
      recipientAcquisitionTagBytes.data(),
      recipientAcquisitionTagBytes.size());
  recipient.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      recipientAttributes,
      recipientAcquisitionTag);
  waitFor(
      recipient,
      [&] { return recipient.recorder().ownershipAcquisition().has_value(); },
      options,
      "DELETE_OBJECTS_THEN_DIVEST released-attribute acquisition");
  auto const recipientAcquisition = recipient.recorder().ownershipAcquisition();
  require(
      recipientAcquisition->object == retainedObject &&
          recipientAcquisition->attributes == recipientAttributes &&
          recipientAcquisition->tag == recipientAcquisitionTagBytes &&
          !registrar.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject,
              registrarAttribute) &&
          recipient.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject,
              recipientAttribute),
      "DELETE_OBJECTS_THEN_DIVEST did not release the retained-object attribute for acquisition");

  auto const registrarFederateHandle = registrar.federateHandle();
  registrar.resign(rti::DELETE_OBJECTS);
  waitFor(
      recipient,
      [&] {
        return recipient.recorder().hasRemoval(
            retainedObject,
            {},
            registrarFederateHandle);
      },
      options,
      "DELETE_OBJECTS_THEN_DIVEST fixture cleanup removal");
  recipient.resign(rti::NO_ACTION);
  registrar.rtiAmbassador().destroyFederationExecution(federation);
  registrar.disconnect();
  departing.disconnect();
  recipient.disconnect();
}

void scenarioResignDeleteObjectsThenDivestCrossObjectEffectsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioResignDeleteObjectsThenDivestCrossObjectEffectsPortable(options, model);
}

void scenarioResignDeleteThenDivestPrivilegeEpoch(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Delete-then-divest privilege epochs require an adapter-supplied FOM");
  Session owner(options, model, "delete-then-divest-epoch-owner");
  Session peer(options, model, "delete-then-divest-epoch-peer");
  auto const federation = federationName(options, "delete-then-divest-privilege-epoch");
  connectAndJoin(owner, peer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const peerClass = peer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const peerAttribute = peer.rtiAmbassador().getAttributeHandle(
      peerClass, options.attributeName);
  auto const ownerPrivilege = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, L"HLAprivilegeToDeleteObject");
  require(ownerClass.isValid() && peerClass.isValid() &&
              ownerAttribute.isValid() && peerAttribute.isValid() &&
              ownerPrivilege.isValid(),
          "Adapter FOM did not resolve the ordinary attribute and standard delete privilege");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const peerAttributes{peerAttribute};
  rti::AttributeHandleSet const privilegeOnly{ownerPrivilege};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  peer.rtiAmbassador().subscribeObjectClassAttributes(
      peerClass, peerAttributes, true, L"");
  peer.rtiAmbassador().publishObjectClassAttributes(peerClass, peerAttributes);

  auto const retained = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const retainedName = owner.rtiAmbassador().getObjectInstanceName(retained);
  require(retained.isValid() && !retainedName.empty() &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(
                  retained, ownerAttribute) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(
                  retained, ownerPrivilege),
          "First registration did not establish ordinary and delete-privilege ownership");
  waitFor(peer, [&] { return peer.recorder().hasDiscovery(retained); },
          options, "retained object discovery");

  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass, privilegeOnly);
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(
                  retained, ownerAttribute) &&
              !owner.rtiAmbassador().isAttributeOwnedByFederate(
                  retained, ownerPrivilege),
          "Unpublishing the delete privilege did not preserve only ordinary ownership");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, privilegeOnly);
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(
              retained, ownerPrivilege),
          "Republishing delete privilege restored ownership of the earlier instance");

  auto const deleted = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const deletedName = owner.rtiAmbassador().getObjectInstanceName(deleted);
  require(deleted.isValid() && deleted != retained &&
              !deletedName.empty() && deletedName != retainedName &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(
                  deleted, ownerAttribute) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(
                  deleted, ownerPrivilege),
          "Later registration did not establish a new delete-privilege epoch");
  waitFor(peer, [&] { return peer.recorder().hasDiscovery(deleted); },
          options, "deleted object discovery");

  peer.recorder().clearRemovals();
  peer.recorder().clearOwnershipRecords();
  owner.resign(rti::DELETE_OBJECTS_THEN_DIVEST);
  waitFor(peer,
          [&] {
            return peer.recorder().removals().size() == 1U &&
                peer.recorder().ownershipAssumption().has_value();
          }, options, "delete-then-divest removal and assumption callbacks");
  auto const removals = peer.recorder().removals();
  auto const assumption = peer.recorder().ownershipAssumption();
  require(removals.size() == 1U && removals.front().object == deleted &&
              assumption->object == retained &&
              assumption->attributes == peerAttributes &&
              assumption->tag.empty(),
          "Delete-then-divest did not separate the retained and deleted instances");
  require(peer.rtiAmbassador().getObjectInstanceHandle(retainedName) == retained,
          "The instance without delete privilege was removed");
  requireException(
      [&] { static_cast<void>(peer.rtiAmbassador().getObjectInstanceHandle(deletedName)); },
      L"ObjectInstanceNotKnown",
      "looking up the delete-privileged instance after resignation");

  peer.resign(rti::NO_ACTION);
  peer.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

void scenarioResignDeleteThenDivestPrivilegeEpochContract(
    Options const& options, rti::CallbackModel model) {
  scenarioResignDeleteThenDivestPrivilegeEpoch(options, model);
}

void scenarioResignUnconditionallyDivestAttributesPreservesObjectsPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty(),
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES testing requires adapter-supplied object and attribute names");

  Session registrar(options, model, "resign-unconditional-divest-registrar");
  Session departing(options, model, "resign-unconditional-divest-departing");
  Session recipient(options, model, "resign-unconditional-divest-recipient");
  auto const federation = federationName(
      options,
      "resign-unconditionally-divest-attributes-preserves-objects");
  connectAndJoin(registrar, departing, options, federation, options.fom);
  recipient.connect();
  recipient.join(
      options.memberFederateName + L"-uncond-recipient",
      options.federateType,
      federation);

  auto const registrarClass =
      registrar.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const departingClass =
      departing.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const recipientClass =
      recipient.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const registrarAttribute = registrar.rtiAmbassador().getAttributeHandle(
      registrarClass,
      options.attributeName);
  auto const departingAttribute = departing.rtiAmbassador().getAttributeHandle(
      departingClass,
      options.attributeName);
  auto const recipientAttribute = recipient.rtiAmbassador().getAttributeHandle(
      recipientClass,
      options.attributeName);
  require(
      registrarClass.isValid() && departingClass.isValid() &&
          recipientClass.isValid() && registrarAttribute.isValid() &&
          departingAttribute.isValid() && recipientAttribute.isValid(),
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES fixture lookup returned an invalid standard handle");

  rti::AttributeHandleSet const registrarAttributes{registrarAttribute};
  rti::AttributeHandleSet const departingAttributes{departingAttribute};
  rti::AttributeHandleSet const recipientAttributes{recipientAttribute};
  registrar.rtiAmbassador().publishObjectClassAttributes(
      registrarClass,
      registrarAttributes);
  departing.rtiAmbassador().publishObjectClassAttributes(
      departingClass,
      departingAttributes);
  recipient.rtiAmbassador().publishObjectClassAttributes(
      recipientClass,
      recipientAttributes);
  registrar.rtiAmbassador().subscribeObjectClassAttributes(
      registrarClass,
      registrarAttributes,
      true,
      L"");
  departing.rtiAmbassador().subscribeObjectClassAttributes(
      departingClass,
      departingAttributes,
      true,
      L"");
  recipient.rtiAmbassador().subscribeObjectClassAttributes(
      recipientClass,
      recipientAttributes,
      true,
      L"");

  auto const retainedObject =
      registrar.rtiAmbassador().registerObjectInstance(registrarClass);
  require(
      retainedObject.isValid(),
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES fixture returned an invalid retained-object handle");
  auto const retainedObjectName =
      registrar.rtiAmbassador().getObjectInstanceName(retainedObject);
  require(
      !retainedObjectName.empty(),
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES fixture returned an empty retained-object name");
  waitForSessions(
      {&departing, &recipient},
      [&] {
        return departing.recorder().hasDiscovery(retainedObject) &&
            recipient.recorder().hasDiscovery(retainedObject);
      },
      options,
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES retained-object discovery");

  registrar.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      registrarAttributes,
      rti::VariableLengthData{});
  std::vector<std::uint8_t> const setupTagBytes{0xD7U, 0x25U, 0x11U};
  rti::VariableLengthData const setupTag(setupTagBytes.data(), setupTagBytes.size());
  departing.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      departingAttributes,
      setupTag);
  waitFor(
      departing,
      [&] { return departing.recorder().ownershipAcquisition().has_value(); },
      options,
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES setup acquisition");
  auto const setupAcquisition = departing.recorder().ownershipAcquisition();
  require(
      setupAcquisition->object == retainedObject &&
          setupAcquisition->attributes == departingAttributes &&
          setupAcquisition->tag == setupTagBytes &&
          departing.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject,
              departingAttribute),
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES setup did not establish foreign-object ownership");

  registrar.recorder().clearRemovals();
  recipient.recorder().clearRemovals();
  departing.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  require(
      registrar.rtiAmbassador().getObjectInstanceHandle(retainedObjectName) ==
              retainedObject &&
          recipient.rtiAmbassador().getObjectInstanceHandle(retainedObjectName) ==
              retainedObject,
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES removed the surviving federate's registered object");

  auto acquireReleasedAttribute = [&](rti::ObjectInstanceHandle const& object,
                                      std::vector<std::uint8_t> const& tagBytes,
                                      char const* context) {
    recipient.recorder().clearOwnershipRecords();
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    recipient.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
        object,
        recipientAttributes,
        tag);
    waitForSessions(
        {&registrar, &recipient},
        [&] { return recipient.recorder().ownershipAcquisition().has_value(); },
        options,
        context);
    auto const acquisition = recipient.recorder().ownershipAcquisition();
    require(
        acquisition->object == object &&
            acquisition->attributes == recipientAttributes &&
            acquisition->tag == tagBytes &&
            recipient.rtiAmbassador().isAttributeOwnedByFederate(
                object,
                recipientAttribute),
        std::string(context) + " returned incorrect acquisition metadata or ownership");
  };
  acquireReleasedAttribute(
      retainedObject,
      {0xD7U, 0x25U, 0x12U},
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES retained-object acquisition");
  require(
      registrar.recorder().removals().empty() &&
          recipient.recorder().removals().empty() &&
          !registrar.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject,
              registrarAttribute),
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES removed the surviving object or retained its original attribute owner");

  auto const registrarFederateHandle = registrar.federateHandle();
  registrar.resign(rti::DELETE_OBJECTS);
  waitFor(
      recipient,
      [&] {
        return recipient.recorder().hasRemoval(
            retainedObject,
            {},
            registrarFederateHandle);
      },
      options,
      "UNCONDITIONALLY_DIVEST_ATTRIBUTES fixture cleanup removal");
  recipient.resign(rti::NO_ACTION);
  registrar.rtiAmbassador().destroyFederationExecution(federation);
  registrar.disconnect();
  departing.disconnect();
  recipient.disconnect();
}

void scenarioResignUnconditionallyDivestAttributesPreservesObjectsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioResignUnconditionallyDivestAttributesPreservesObjectsPortable(options, model);
}

void scenarioResignCancelPendingOwnershipAcquisitionsPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty(),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS testing requires adapter-supplied object and attribute names");

  Session owner(options, model, "resign-cancel-pending-acquisitions-owner");
  Session requester(options, model, "resign-cancel-pending-acquisitions-requester");
  Session successor(options, model, "resign-cancel-pending-acquisitions-successor");
  auto const federation =
      federationName(options, "resign-cancel-pending-ownership-acquisitions-action");
  connectAndJoin(owner, requester, options, federation, options.fom);
  successor.connect();
  successor.join(
      options.memberFederateName + L"-cancel-successor",
      options.federateType,
      federation);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const requesterClass =
      requester.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const successorClass =
      successor.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  auto const successorAttribute = successor.rtiAmbassador().getAttributeHandle(
      successorClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          successorClass.isValid() && ownerAttribute.isValid() &&
          requesterAttribute.isValid() && successorAttribute.isValid(),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS fixture lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  rti::AttributeHandleSet const successorAttributes{successorAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  successor.rtiAmbassador().publishObjectClassAttributes(
      successorClass,
      successorAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  successor.rtiAmbassador().subscribeObjectClassAttributes(
      successorClass,
      successorAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS fixture returned an invalid object handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS fixture returned an empty object name");
  waitForSessions(
      {&requester, &successor},
      [&] {
        return requester.recorder().hasDiscovery(object) &&
            successor.recorder().hasDiscovery(object);
      },
      options,
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS object discovery");

  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const requestTagBytes{0xD7U, 0x25U, 0x21U};
  rti::VariableLengthData const requestTag(requestTagBytes.data(), requestTagBytes.size());
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      requestTag);
  require(
      !requester.recorder().ownershipAcquisition().has_value() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS setup did not retain the owner's pending transfer");

  requester.resign(rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  require(
      owner.rtiAmbassador().getObjectInstanceName(object) == objectName &&
          successor.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS resignation changed object identity or transferred ownership");

  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      rti::VariableLengthData{});
  successor.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const successorTagBytes{0xD7U, 0x25U, 0x22U};
  rti::VariableLengthData const successorTag(
      successorTagBytes.data(),
      successorTagBytes.size());
  successor.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      successorAttributes,
      successorTag);
  waitFor(
      successor,
      [&] { return successor.recorder().ownershipAcquisition().has_value(); },
      options,
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS subsequent acquisition");
  auto const acquisition = successor.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == successorAttributes &&
          acquisition->tag == successorTagBytes &&
          !successor.recorder().ownershipUnavailable().has_value() &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          successor.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              successorAttribute),
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS left stale acquisition state or returned incorrect ownership");

  auto const ownerFederateHandle = owner.federateHandle();
  owner.resign(rti::DELETE_OBJECTS);
  waitFor(
      successor,
      [&] {
        return successor.recorder().hasRemoval(object, {}, ownerFederateHandle);
      },
      options,
      "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS fixture cleanup removal");
  successor.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
  successor.disconnect();
}

void scenarioResignCancelPendingOwnershipAcquisitionsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioResignCancelPendingOwnershipAcquisitionsPortable(options, model);
}

void scenarioResignCancelThenDeleteThenDivestCrossObjectEffectsPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "CANCEL_THEN_DELETE_THEN_DIVEST testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty(),
      "CANCEL_THEN_DELETE_THEN_DIVEST testing requires adapter-supplied object and attribute names");

  Session owner(options, model, "resign-cancel-delete-divest-owner");
  Session departing(options, model, "resign-cancel-delete-divest-departing");
  Session successor(options, model, "resign-cancel-delete-divest-successor");
  auto const federation = federationName(
      options,
      "resign-cancel-then-delete-then-divest-cross-object-effects");
  connectAndJoin(owner, departing, options, federation, options.fom);
  successor.connect();
  successor.join(
      options.memberFederateName + L"-cancel-next",
      options.federateType,
      federation);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const departingClass =
      departing.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const successorClass =
      successor.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const departingAttribute = departing.rtiAmbassador().getAttributeHandle(
      departingClass,
      options.attributeName);
  auto const successorAttribute = successor.rtiAmbassador().getAttributeHandle(
      successorClass,
      options.attributeName);
  require(
      ownerClass.isValid() && departingClass.isValid() &&
          successorClass.isValid() && ownerAttribute.isValid() &&
          departingAttribute.isValid() && successorAttribute.isValid(),
      "CANCEL_THEN_DELETE_THEN_DIVEST fixture lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const departingAttributes{departingAttribute};
  rti::AttributeHandleSet const successorAttributes{successorAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  departing.rtiAmbassador().publishObjectClassAttributes(
      departingClass,
      departingAttributes);
  successor.rtiAmbassador().publishObjectClassAttributes(
      successorClass,
      successorAttributes);
  owner.rtiAmbassador().subscribeObjectClassAttributes(
      ownerClass,
      ownerAttributes,
      true,
      L"");
  departing.rtiAmbassador().subscribeObjectClassAttributes(
      departingClass,
      departingAttributes,
      true,
      L"");
  successor.rtiAmbassador().subscribeObjectClassAttributes(
      successorClass,
      successorAttributes,
      true,
      L"");

  auto const retainedObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const pendingObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      retainedObject.isValid() && pendingObject.isValid() &&
          retainedObject != pendingObject,
      "CANCEL_THEN_DELETE_THEN_DIVEST fixture returned invalid or duplicate owner objects");
  auto const retainedObjectName =
      owner.rtiAmbassador().getObjectInstanceName(retainedObject);
  auto const pendingObjectName =
      owner.rtiAmbassador().getObjectInstanceName(pendingObject);
  require(
      !retainedObjectName.empty() && !pendingObjectName.empty() &&
          retainedObjectName != pendingObjectName,
      "CANCEL_THEN_DELETE_THEN_DIVEST fixture returned invalid owner object names");
  waitForSessions(
      {&departing, &successor},
      [&] {
        return departing.recorder().hasDiscovery(retainedObject) &&
            departing.recorder().hasDiscovery(pendingObject) &&
            successor.recorder().hasDiscovery(retainedObject) &&
            successor.recorder().hasDiscovery(pendingObject);
      },
      options,
      "CANCEL_THEN_DELETE_THEN_DIVEST owner-object discovery");

  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      ownerAttributes,
      rti::VariableLengthData{});
  std::vector<std::uint8_t> const retainedTagBytes{0xD7U, 0x25U, 0x31U};
  rti::VariableLengthData const retainedTag(
      retainedTagBytes.data(),
      retainedTagBytes.size());
  departing.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      departingAttributes,
      retainedTag);
  waitFor(
      departing,
      [&] { return departing.recorder().ownershipAcquisition().has_value(); },
      options,
      "CANCEL_THEN_DELETE_THEN_DIVEST retained-object ownership setup");
  auto const retainedAcquisition = departing.recorder().ownershipAcquisition();
  require(
      retainedAcquisition->object == retainedObject &&
          retainedAcquisition->attributes == departingAttributes &&
          retainedAcquisition->tag == retainedTagBytes &&
          departing.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject,
              departingAttribute),
      "CANCEL_THEN_DELETE_THEN_DIVEST setup did not transfer the retained-object attribute");

  auto const deletedObject =
      departing.rtiAmbassador().registerObjectInstance(departingClass);
  require(
      deletedObject.isValid() && deletedObject != retainedObject &&
          deletedObject != pendingObject,
      "CANCEL_THEN_DELETE_THEN_DIVEST fixture returned an invalid departing object");
  auto const deletedObjectName =
      departing.rtiAmbassador().getObjectInstanceName(deletedObject);
  require(
      !deletedObjectName.empty() && deletedObjectName != retainedObjectName &&
          deletedObjectName != pendingObjectName,
      "CANCEL_THEN_DELETE_THEN_DIVEST fixture returned an invalid departing-object name");
  waitForSessions(
      {&owner, &successor},
      [&] {
        return owner.recorder().hasDiscovery(deletedObject) &&
            successor.recorder().hasDiscovery(deletedObject);
      },
      options,
      "CANCEL_THEN_DELETE_THEN_DIVEST departing-object discovery");
  require(
      departing.rtiAmbassador().isAttributeOwnedByFederate(
          deletedObject,
          departingAttribute),
      "CANCEL_THEN_DELETE_THEN_DIVEST setup did not establish departing-object ownership");

  departing.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const pendingTagBytes{0xD7U, 0x25U, 0x32U};
  rti::VariableLengthData const pendingTag(
      pendingTagBytes.data(),
      pendingTagBytes.size());
  departing.rtiAmbassador().attributeOwnershipAcquisition(
      pendingObject,
      departingAttributes,
      pendingTag);
  require(
      !departing.recorder().ownershipAcquisition().has_value() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              pendingObject,
              ownerAttribute),
      "CANCEL_THEN_DELETE_THEN_DIVEST setup did not retain the pending request at its owner");

  auto const departingFederateHandle = departing.federateHandle();
  owner.recorder().clearRemovals();
  successor.recorder().clearRemovals();
  departing.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  waitForSessions(
      {&owner, &successor},
      [&] {
        return owner.recorder().hasRemoval(
                   deletedObject,
                   {},
                   departingFederateHandle) &&
            successor.recorder().hasRemoval(
                deletedObject,
                {},
                departingFederateHandle);
      },
      options,
      "CANCEL_THEN_DELETE_THEN_DIVEST object-removal callbacks");
  auto const ownerRemovals = owner.recorder().removals();
  auto const successorRemovals = successor.recorder().removals();
  require(
      ownerRemovals.size() == 1U &&
          ownerRemovals.front().object == deletedObject &&
          ownerRemovals.front().tag.empty() &&
          ownerRemovals.front().producer == departingFederateHandle &&
          successorRemovals.size() == 1U &&
          successorRemovals.front().object == deletedObject &&
          successorRemovals.front().tag.empty() &&
          successorRemovals.front().producer == departingFederateHandle,
      "CANCEL_THEN_DELETE_THEN_DIVEST returned incorrect or duplicate removal metadata");
  require(
      owner.rtiAmbassador().getObjectInstanceHandle(retainedObjectName) ==
              retainedObject &&
          owner.rtiAmbassador().getObjectInstanceHandle(pendingObjectName) ==
              pendingObject &&
          successor.rtiAmbassador().getObjectInstanceHandle(retainedObjectName) ==
              retainedObject &&
          successor.rtiAmbassador().getObjectInstanceHandle(pendingObjectName) ==
              pendingObject &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              pendingObject,
              ownerAttribute) &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject,
              ownerAttribute),
      "CANCEL_THEN_DELETE_THEN_DIVEST changed retained objects or ownership unexpectedly");
  requireException(
      [&] {
        static_cast<void>(
            successor.rtiAmbassador().getObjectInstanceHandle(deletedObjectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up the departing object after CANCEL_THEN_DELETE_THEN_DIVEST");

  auto acquireReleasedAttribute = [&](rti::ObjectInstanceHandle const& object,
                                      std::vector<std::uint8_t> const& tagBytes,
                                      char const* context) {
    successor.recorder().clearOwnershipRecords();
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    successor.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
        object,
        successorAttributes,
        tag);
    waitFor(
        successor,
        [&] { return successor.recorder().ownershipAcquisition().has_value(); },
        options,
        context);
    auto const acquisition = successor.recorder().ownershipAcquisition();
    require(
        acquisition->object == object &&
            acquisition->attributes == successorAttributes &&
            acquisition->tag == tagBytes &&
            !successor.recorder().ownershipUnavailable().has_value() &&
            successor.rtiAmbassador().isAttributeOwnedByFederate(
                object,
                successorAttribute),
        std::string(context) + " returned incorrect acquisition metadata or ownership");
  };
  acquireReleasedAttribute(
      retainedObject,
      {0xD7U, 0x25U, 0x33U},
      "CANCEL_THEN_DELETE_THEN_DIVEST retained-object acquisition");

  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      pendingObject,
      ownerAttributes,
      rti::VariableLengthData{});
  acquireReleasedAttribute(
      pendingObject,
      {0xD7U, 0x25U, 0x34U},
      "CANCEL_THEN_DELETE_THEN_DIVEST pending-object successor acquisition");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(
          pendingObject,
          ownerAttribute),
      "CANCEL_THEN_DELETE_THEN_DIVEST left the pending-object attribute with its owner");

  successor.recorder().clearRemovals();
  auto const ownerFederateHandle = owner.federateHandle();
  owner.resign(rti::DELETE_OBJECTS);
  waitForSessions(
      {&successor},
      [&] {
        return successor.recorder().hasRemoval(
                   retainedObject,
                   {},
                   ownerFederateHandle) &&
            successor.recorder().hasRemoval(
                pendingObject,
                {},
                ownerFederateHandle);
      },
      options,
      "CANCEL_THEN_DELETE_THEN_DIVEST fixture cleanup removals");
  auto const cleanupRemovals = successor.recorder().removals();
  require(
      cleanupRemovals.size() == 2U &&
          std::all_of(
              cleanupRemovals.begin(),
              cleanupRemovals.end(),
              [&](auto const& removal) {
                return removal.tag.empty() &&
                    removal.producer == ownerFederateHandle;
              }) &&
          std::count_if(
              cleanupRemovals.begin(),
              cleanupRemovals.end(),
              [&](auto const& removal) { return removal.object == retainedObject; }) ==
              1 &&
          std::count_if(
              cleanupRemovals.begin(),
              cleanupRemovals.end(),
              [&](auto const& removal) { return removal.object == pendingObject; }) ==
              1,
      "CANCEL_THEN_DELETE_THEN_DIVEST cleanup returned incorrect owner-object removals");

  successor.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  departing.disconnect();
  successor.disconnect();
}

void scenarioResignCancelThenDeleteThenDivestCrossObjectEffectsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioResignCancelThenDeleteThenDivestCrossObjectEffectsPortable(options, model);
}


}  // namespace hla_rti_cpp_tck::detail
