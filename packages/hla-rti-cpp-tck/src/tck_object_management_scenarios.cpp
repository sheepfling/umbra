#include "tck_main_scenarios.hpp"

#include <type_traits>
#include <utility>

namespace hla_rti_cpp_tck::detail {

void scenarioObjectRegistration(Options const& options, rti::CallbackModel model) {
  Session registrationLifecycle(options, model, "registration-lifecycle");
  rti::ObjectClassHandle const invalidObjectClass;
  std::wstring const lifecycleObjectName = L"tck-lifecycle-reservation";
  std::set<std::wstring> const lifecycleObjectNames{lifecycleObjectName};
  auto requireReservationLifecycle = [&](std::wstring const& expected,
                                          std::string const& phase) {
    requireException(
        [&] {
          registrationLifecycle.rtiAmbassador().reserveObjectInstanceName(
              lifecycleObjectName);
        },
        expected,
        "reserving an object name " + phase);
    requireException(
        [&] {
          registrationLifecycle.rtiAmbassador().releaseObjectInstanceName(
              lifecycleObjectName);
        },
        expected,
        "releasing an object name " + phase);
    requireException(
        [&] {
          registrationLifecycle.rtiAmbassador().reserveMultipleObjectInstanceNames(
              lifecycleObjectNames);
        },
        expected,
        "reserving multiple object names " + phase);
    requireException(
        [&] {
          registrationLifecycle.rtiAmbassador().releaseMultipleObjectInstanceNames(
              lifecycleObjectNames);
        },
        expected,
        "releasing multiple object names " + phase);
  };
  requireReservationLifecycle(L"NotConnected", "before connect");
  auto requireRegistrationLifecycle = [&](std::wstring const& expected,
                                           std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(registrationLifecycle.rtiAmbassador().registerObjectInstance(
              invalidObjectClass));
        },
        expected,
        "registering an object " + phase);
    requireException(
        [&] {
          static_cast<void>(registrationLifecycle.rtiAmbassador().registerObjectInstance(
              invalidObjectClass,
              L"tck-lifecycle-object"));
        },
        expected,
        "registering a named object " + phase);
  };
  requireRegistrationLifecycle(L"NotConnected", "before connect");
  registrationLifecycle.connect();
  requireReservationLifecycle(L"FederateNotExecutionMember", "after connect before join");
  requireRegistrationLifecycle(L"FederateNotExecutionMember", "after connect before join");
  registrationLifecycle.disconnect();

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "object-registration");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().registerObjectInstance(invalidObjectClass));
      },
      L"ObjectClassNotDefined",
      "registering an object instance for an invalid object class");
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  auto const unreservedName = federation + L"-unreserved-name";
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().registerObjectInstance(
            objectClass,
            unreservedName));
      },
      L"ObjectInstanceNameNotReserved",
      "registering an object instance with an unreserved name");
  auto const releasedName = federation + L"-released-before-registration";
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(releasedName);
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(releasedName); },
      options,
      "reserving the name before releasing it");
  owner.rtiAmbassador().releaseObjectInstanceName(releasedName);
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().registerObjectInstance(
            objectClass,
            releasedName));
      },
      L"ObjectInstanceNameNotReserved",
      "registering with a name whose successful reservation was released");
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Object registration returned an invalid handle");
  auto const name = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!name.empty(), "Object registration returned an empty instance name");

  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "object discovery");
  auto const discovered = member.recorder().discovery();
  require(discovered.object == object, "Discovery returned the wrong object handle");
  require(discovered.objectClass == objectClass, "Discovery returned the wrong object class");
  require(discovered.name == name, "Discovery returned the wrong object name");
  require(discovered.producer == owner.federateHandle(),
          "Discovery returned the wrong producing federate");
  require(member.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Object instance name lookup did not round-trip");
  require(member.rtiAmbassador().getKnownObjectClassHandle(object) == objectClass,
          "Known object class lookup did not round-trip");

  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(secondObject.isValid(), "Second object registration returned an invalid handle");
  auto const secondName = owner.rtiAmbassador().getObjectInstanceName(secondObject);
  require(!secondName.empty(), "Second object registration returned an empty instance name");
  require(secondName != name,
          "RTI-generated object instance names were not unique within the federation");
  require(owner.rtiAmbassador().getObjectInstanceHandle(secondName) == secondObject,
          "Second object instance name lookup did not round-trip");

  auto const memberObjectClass = member.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberObjectClass,
      options.attributeName);
  require(
      memberObjectClass.isValid() && memberAttribute.isValid(),
      "Member object registration handles were invalid");
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  member.rtiAmbassador().publishObjectClassAttributes(
      memberObjectClass,
      memberAttributes);
  auto const memberObject =
      member.rtiAmbassador().registerObjectInstance(memberObjectClass);
  require(
      memberObject.isValid(),
      "Member registration returned an invalid handle");
  auto const memberName =
      member.rtiAmbassador().getObjectInstanceName(memberObject);
  require(
      !memberName.empty() && memberName != name && memberName != secondName,
      "RTI-generated object instance names were not unique across joined federates");
  require(
      member.rtiAmbassador().getObjectInstanceHandle(memberName) == memberObject,
      "Member-generated object instance name did not round-trip");

  waitFor(member, [&] { return member.recorder().hasDiscovery(secondObject); }, options,
          "second object discovery");
  auto const secondDiscovered = member.recorder().discovery();
  require(secondDiscovered.object == secondObject,
          "Second discovery returned the wrong object handle");
  require(secondDiscovered.name == secondName,
          "Second discovery returned the wrong object name");

  requireException(
      [&] { member.rtiAmbassador().localDeleteObjectInstance(rti::ObjectInstanceHandle{}); },
      L"ObjectInstanceNotKnown",
      "local deletion of an unknown object instance");
  member.rtiAmbassador().localDeleteObjectInstance(object);
  require(member.recorder().removals().empty(),
          "Local object deletion unexpectedly delivered a removal callback");
  requireException(
      [&] { static_cast<void>(member.rtiAmbassador().getObjectInstanceHandle(name)); },
      L"ObjectInstanceNotKnown",
      "Object lookup after local deletion");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioObjectDeletion(Options const& options, rti::CallbackModel model) {
  // Without a timestamp, the official API cannot provide a retraction handle.
  static_assert(std::is_same_v<
                decltype(std::declval<rti::RTIambassador&>().deleteObjectInstance(
                    std::declval<rti::ObjectInstanceHandle const&>(),
                    std::declval<rti::VariableLengthData const&>())),
                void>);

  Session deletionLifecycle(options, model, "deletion-lifecycle");
  rti::ObjectInstanceHandle const invalidObjectInstance;
  rti::VariableLengthData const emptyDeletionTag;
  auto requireDeletionLifecycle = [&](std::wstring const& expected,
                                       std::string const& phase) {
    requireException(
        [&] {
          deletionLifecycle.rtiAmbassador().deleteObjectInstance(
              invalidObjectInstance,
              emptyDeletionTag);
        },
        expected,
        "deleting an object " + phase);
    requireException(
        [&] {
          deletionLifecycle.rtiAmbassador().localDeleteObjectInstance(
              invalidObjectInstance);
        },
        expected,
        "locally deleting an object " + phase);
  };
  requireDeletionLifecycle(L"NotConnected", "before connect");
  deletionLifecycle.connect();
  requireDeletionLifecycle(L"FederateNotExecutionMember", "after connect before join");
  deletionLifecycle.disconnect();

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "object-deletion");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  requireException(
      [&] {
        owner.rtiAmbassador().deleteObjectInstance(
            rti::ObjectInstanceHandle{},
            rti::VariableLengthData{});
      },
      L"ObjectInstanceNotKnown",
      "deleting an unknown object instance");

  auto registerAndDiscover = [&] {
    member.recorder().clearDiscovery();
    auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
    require(object.isValid(), "Object deletion registration returned an invalid handle");
    auto const name = owner.rtiAmbassador().getObjectInstanceName(object);
    require(!name.empty(), "Object deletion registration returned an empty name");
    waitFor(member, [&] { return member.recorder().discovery().present; }, options,
            "object deletion discovery");
    auto const discovered = member.recorder().discovery();
    require(discovered.object == object,
            "Object deletion discovery returned the wrong object handle");
    require(discovered.objectClass == objectClass,
            "Object deletion discovery returned the wrong object class");
    require(discovered.name == name,
            "Object deletion discovery returned the wrong object name");
    return std::pair{object, name};
  };

  auto const local = registerAndDiscover();
  member.recorder().clearRemovals();
  member.rtiAmbassador().localDeleteObjectInstance(local.first);
  for (int pass = 0; pass != 8; ++pass) {
    member.pump();
  }
  require(member.recorder().removals().empty(),
          "Local deletion unexpectedly delivered Remove Object Instance");
  requireException(
      [&] { static_cast<void>(member.rtiAmbassador().getObjectInstanceHandle(local.second)); },
      L"ObjectInstanceNotKnown",
      "Object lookup after local deletion");
  owner.rtiAmbassador().deleteObjectInstance(local.first, rti::VariableLengthData{});

  auto const deleted = registerAndDiscover();
  member.recorder().clearRemovals();
  auto const ownerPrivilege = owner.rtiAmbassador().getAttributeHandle(
      objectClass, L"HLAprivilegeToDeleteObject");
  auto const memberClass = member.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const memberPrivilege = member.rtiAmbassador().getAttributeHandle(
      memberClass, L"HLAprivilegeToDeleteObject");
  require(ownerPrivilege.isValid() && memberPrivilege.isValid(),
          "Object deletion could not resolve the standard delete privilege");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(deleted.first, ownerPrivilege) &&
              !member.rtiAmbassador().isAttributeOwnedByFederate(
                  deleted.first, memberPrivilege),
          "delete privilege did not initially belong only to the registrant");
  std::vector<std::uint8_t> deletionTagBytes{0xD1U, 0xE2U, 0xF3U};
  rti::VariableLengthData deletionTag(deletionTagBytes.data(), deletionTagBytes.size());
  requireException(
      [&] { member.rtiAmbassador().deleteObjectInstance(deleted.first, deletionTag); },
      L"DeletePrivilegeNotHeld",
      "non-owner deleting an object before the privilege transfers");
  require(member.recorder().removals().empty(),
          "rejected non-owner deletion produced a removal callback");
  owner.rtiAmbassador().deleteObjectInstance(deleted.first, deletionTag);
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getKnownObjectClassHandle(deleted.first));
      },
      L"ObjectInstanceNotKnown",
      "deleting owner querying the class before the recipient removal callback");
  if (model == rti::HLA_EVOKED) {
    require(member.recorder().removals().empty(),
            "evoked recipient received removal without servicing callbacks");
    require(member.rtiAmbassador().getKnownObjectClassHandle(deleted.first) == memberClass,
            "evoked recipient forgot the object before its queued removal callback");
  }
  waitFor(member, [&] {
    return member.recorder().hasRemoval(
        deleted.first, deletionTagBytes, owner.federateHandle());
  }, options, "ordinary object removal");
  requireException(
      [&] { static_cast<void>(member.rtiAmbassador().getObjectInstanceHandle(deleted.second)); },
      L"ObjectInstanceNotKnown",
      "Object lookup after remote deletion");
  requireException(
      [&] {
        static_cast<void>(member.rtiAmbassador().getKnownObjectClassHandle(deleted.first));
      },
      L"ObjectInstanceNotKnown",
      "Known-object class lookup after remote deletion");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectInstanceName(deleted.first)); },
      L"ObjectInstanceNotKnown",
      "Owner object lookup after remote deletion");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().isAttributeOwnedByFederate(
            deleted.first,
            attribute));
      },
      L"ObjectInstanceNotKnown",
      "Owner ownership-status query after remote deletion");

  auto const resigned = registerAndDiscover();
  require(resigned.first != deleted.first,
          "object instance handle was reused after deletion in the same federation execution");
  member.recorder().clearRemovals();
  owner.resign(rti::DELETE_OBJECTS);
  waitFor(member, [&] {
    return member.recorder().hasRemoval(
        resigned.first, {}, owner.federateHandle());
  }, options, "resign-time object removal");
  requireException(
      [&] { static_cast<void>(member.rtiAmbassador().getObjectInstanceHandle(resigned.second)); },
      L"ObjectInstanceNotKnown",
      "Object lookup after resign-time deletion");
  member.resign(rti::NO_ACTION);
}

void scenarioFqrTimestampedObjectDeletionKnownState(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation =
      federationName(options, "fqr-timestamped-object-deletion-known-state");
  connectAndJoin(owner, receiver, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  auto const receiverObjectClass =
      receiver.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverObjectClass, options.attributeName);
  require(receiverObjectClass == objectClass && receiverAttribute == attribute,
          "FQR timestamped deletion FOM handles differ across federates");

  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(attribute);
  rti::AttributeHandleSet receiverAttributes;
  receiverAttributes.insert(receiverAttribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, ownerAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverObjectClass, receiverAttributes, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "FQR timestamped deletion returned an invalid object handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!objectName.empty(), "FQR timestamped deletion returned an empty object name");
  waitFor(receiver, [&] { return receiver.recorder().discovery().present; }, options,
          "FQR timestamped deletion object discovery");
  require(receiver.recorder().discovery().object == object,
          "FQR timestamped deletion discovered the wrong object");
  require(receiver.rtiAmbassador().getKnownObjectClassHandle(object) == receiverObjectClass,
          "FQR timestamped deletion receiver did not initially know the object");

  auto ownerTime = makeTimeContext(owner);
  auto receiverTime = makeTimeContext(receiver);
  requireTimeEquals(*ownerTime.initial, *receiverTime.initial,
                    "FQR timestamped object-deletion initial time");
  enableTimestampedRoles(
      owner, receiver, ownerTime, receiverTime, options,
      "FQR timestamped object deletion");
  auto const deletionTime = timeAfter(
      *ownerTime.factory, *ownerTime.initial, *ownerTime.epsilon, 4U);
  auto const afterDeletionTime = timeAfter(
      *ownerTime.factory, *ownerTime.initial, *ownerTime.epsilon, 5U);
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x51U, 0x52U, 0x2DU, 0x44U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = owner.rtiAmbassador().deleteObjectInstance(
      object, tag, *deletionTime);
  require(retraction.isValid(),
          "FQR timestamped deletion returned an invalid retraction handle");
  require(receiver.recorder().timedRemovals().empty(),
          "FQR timestamped deletion was delivered before Flush Queue Request");

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*deletionTime);
  waitFor(receiver, [&] {
    return receiver.recorder().timedRemovals().size() == 1U &&
        receiver.recorder().flushQueueGrants().size() == 1U;
  }, options, "FQR timestamped object deletion and flush grant");
  auto const removal = receiver.recorder().timedRemovals().front();
  auto const flushGrant = receiver.recorder().flushQueueGrants().front();
  require(receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"remove", "flush-grant"},
          "FQR delivered its grant before the timestamped object removal");
  require(removal.object == object && removal.tag == tagBytes &&
              removal.producer == owner.federateHandle() &&
              removal.time == encodeTime(*deletionTime) &&
              removal.sentOrder == rti::TIMESTAMP &&
              removal.receivedOrder == rti::TIMESTAMP &&
              removal.retractionPresent &&
              removal.retraction == copyBytes(retraction.encode()),
          "FQR returned the wrong timestamped object removal metadata");
  require(flushGrant.optimisticTime == encodeTime(*deletionTime),
          "FQR returned the wrong optimistic time for timestamped object deletion");

  owner.rtiAmbassador().timeAdvanceRequest(*afterDeletionTime);
  waitFor(owner, [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "FQR timestamped deletion producer advance");
  receiver.rtiAmbassador().timeAdvanceRequest(*afterDeletionTime);
  waitFor(receiver,
          [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
          options, "FQR timestamped deletion receiver advance");
  auto receiverLogicalTime = receiverTime.factory->makeInitial();
  require(receiverLogicalTime != nullptr,
          "FQR timestamped deletion could not allocate a logical-time query");
  receiver.rtiAmbassador().queryLogicalTime(*receiverLogicalTime);
  require(*receiverLogicalTime >= *deletionTime,
          "FQR receiver did not advance to or past the object-deletion timestamp");
  requireException(
      [&] {
        static_cast<void>(receiver.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "FQR timestamped object lookup after advancing past deletion time");
  requireException(
      [&] {
        static_cast<void>(receiver.rtiAmbassador().getKnownObjectClassHandle(object));
      },
      L"ObjectInstanceNotKnown",
      "FQR timestamped known-object lookup after advancing past deletion time");
  receiver.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
}

void scenarioAttributeValueRequest(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "attribute-value-request");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle ownerClass;
  rti::AttributeHandle ownerAttribute;
  rti::InteractionClassHandle ownerInteraction;
  rti::ParameterHandle ownerParameter;
  handles(owner, options, ownerClass, ownerAttribute, ownerInteraction, ownerParameter);
  auto const memberClass = member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute =
      member.rtiAmbassador().getAttributeHandle(memberClass, options.attributeName);
  require(memberClass.isValid(), "Value-request member class lookup returned an invalid handle");
  require(memberAttribute.isValid(),
          "Value-request member attribute lookup returned an invalid handle");
  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(ownerAttribute);
  rti::AttributeHandleSet memberAttributes;
  memberAttributes.insert(memberAttribute);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass, memberAttributes, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Value-request registration returned an invalid handle");
  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "value-request object discovery");
  auto const knownObject = member.recorder().discovery().object;
  require(knownObject == object, "Value-request discovery returned the wrong object handle");

  rti::ObjectClassHandle const invalidObjectClass;
  rti::AttributeHandleSet const invalidAttributes{rti::AttributeHandle{}};
  requireException(
      [&] {
        member.rtiAmbassador().requestAttributeValueUpdate(
            invalidObjectClass,
            memberAttributes,
            rti::VariableLengthData{});
      },
      L"ObjectClassNotDefined",
      "class attribute-value request with an invalid object class");
  requireException(
      [&] {
        member.rtiAmbassador().requestAttributeValueUpdate(
            memberClass,
            invalidAttributes,
            rti::VariableLengthData{});
      },
      L"AttributeNotDefined",
      "class attribute-value request with an invalid attribute");
  requireException(
      [&] {
        member.rtiAmbassador().requestAttributeValueUpdate(
            knownObject,
            invalidAttributes,
            rti::VariableLengthData{});
      },
      L"AttributeNotDefined",
      "instance attribute-value request with an invalid attribute");

  auto respondToRequest = [&](std::vector<std::uint8_t> const& value,
                              std::vector<std::uint8_t> const& updateTag,
                              std::string const& description) {
    member.recorder().clearReflection();
    rti::AttributeHandleValueMap values;
    values.emplace(
        ownerAttribute,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(updateTag.data(), updateTag.size());
    owner.rtiAmbassador().updateAttributeValues(object, values, tag);
    waitFor(member, [&] { return member.recorder().reflection().present; }, options, description);
    auto const reflected = member.recorder().reflection();
    require(reflected.object == knownObject,
            description + " returned the wrong object handle");
    require(reflected.values.size() == 1U && reflected.values.count(memberAttribute) == 1U,
            description + " returned the wrong attribute set");
    require(copyBytes(reflected.values.at(memberAttribute)) == value,
            description + " returned the wrong attribute value");
    require(reflected.tag == updateTag, description + " returned the wrong update tag");
  };

  std::vector<std::uint8_t> instanceRequestTagBytes{0x31U, 0x32U};
  rti::VariableLengthData instanceRequestTag(
      instanceRequestTagBytes.data(), instanceRequestTagBytes.size());
  owner.recorder().clearProvidedUpdates();
  member.rtiAmbassador().requestAttributeValueUpdate(
      knownObject, memberAttributes, instanceRequestTag);
  waitFor(owner, [&] {
    auto const requests = owner.recorder().providedUpdates();
    return !requests.empty() && requests.back().object == object &&
        requests.back().attributes == ownerAttributes &&
        requests.back().tag == instanceRequestTagBytes;
  }, options, "instance attribute-value request callback");
  auto const instanceRequests = owner.recorder().providedUpdates();
  require(instanceRequests.back().object == object,
          "Instance value-request callback returned the wrong object");
  require(instanceRequests.back().attributes == ownerAttributes,
          "Instance value-request callback returned the wrong attributes");
  require(instanceRequests.back().tag == instanceRequestTagBytes,
          "Instance value-request callback returned the wrong tag");
  respondToRequest(
      {0x41U, 0x42U, 0x43U},
      {0x51U, 0x52U},
      "instance attribute-value response");

  std::vector<std::uint8_t> classRequestTagBytes{0x61U, 0x62U};
  rti::VariableLengthData classRequestTag(
      classRequestTagBytes.data(), classRequestTagBytes.size());
  owner.recorder().clearProvidedUpdates();
  member.rtiAmbassador().requestAttributeValueUpdate(
      memberClass, memberAttributes, classRequestTag);
  waitFor(owner, [&] {
    auto const requests = owner.recorder().providedUpdates();
    return !requests.empty() && requests.back().object == object &&
        requests.back().attributes == ownerAttributes &&
        requests.back().tag == classRequestTagBytes;
  }, options, "class attribute-value request callback");
  auto const classRequests = owner.recorder().providedUpdates();
  require(classRequests.back().object == object,
          "Class value-request callback returned the wrong object");
  require(classRequests.back().attributes == ownerAttributes,
          "Class value-request callback returned the wrong attributes");
  require(classRequests.back().tag == classRequestTagBytes,
          "Class value-request callback returned the wrong tag");
  respondToRequest(
      {0x71U, 0x72U, 0x73U},
      {0x81U, 0x82U},
      "class attribute-value response");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioNamedRegistration(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "named-registration");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  requireException(
      [&] { owner.rtiAmbassador().reserveObjectInstanceName(L""); },
      L"IllegalName",
      "reserving an empty object instance name");

  requireException(
      [&] {
        owner.rtiAmbassador().reserveMultipleObjectInstanceNames(
            std::set<std::wstring>{});
      },
      L"NameSetWasEmpty",
      "reserving an empty object instance name set");
  requireException(
      [&] {
        owner.rtiAmbassador().reserveMultipleObjectInstanceNames(
            std::set<std::wstring>{L"HLA.ReservedByTheRTI"});
      },
      L"IllegalName",
      "reserving a name with the reserved HLA prefix");
  auto const validNameInRejectedSet = federation + L"-valid-in-rejected-set";
  requireException(
      [&] {
        owner.rtiAmbassador().reserveMultipleObjectInstanceNames(
            std::set<std::wstring>{validNameInRejectedSet, L"HLA.Invalid"});
      },
      L"IllegalName",
      "rejecting a multiple-name reservation containing one illegal name");
  member.recorder().clearReservations();
  member.rtiAmbassador().reserveObjectInstanceName(validNameInRejectedSet);
  waitFor(
      member,
      [&] { return member.recorder().reservationSucceeded(validNameInRejectedSet); },
      options,
      "valid name remains available after an illegal multiple-name reservation");
  require(
      !member.recorder().hasMultipleReservationOutcome(),
      "single-name reservation used a multiple-name reservation callback");
  member.rtiAmbassador().releaseObjectInstanceName(validNameInRejectedSet);

  auto const releasedName = federation + L"-released-name";
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(releasedName);
  waitFor(owner, [&] { return owner.recorder().reservationSucceeded(releasedName); }, options,
          "single object-name reservation");
  require(
      !owner.recorder().hasMultipleReservationOutcome(),
      "single-name reservation used a multiple-name reservation callback");
  owner.rtiAmbassador().releaseObjectInstanceName(releasedName);
  member.recorder().clearReservations();
  member.rtiAmbassador().reserveObjectInstanceName(releasedName);
  waitFor(
      member,
      [&] { return member.recorder().reservationSucceeded(releasedName); },
      options,
      "a released object name is available to another joined federate");
  member.rtiAmbassador().releaseObjectInstanceName(releasedName);
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(releasedName);
  waitFor(owner, [&] { return owner.recorder().reservationSucceeded(releasedName); }, options,
          "reuse of a released object name");
  owner.rtiAmbassador().releaseObjectInstanceName(releasedName);
  requireException(
      [&] { owner.rtiAmbassador().releaseObjectInstanceName(releasedName); },
      L"ObjectInstanceNameNotReserved",
      "second release of an object name");

  std::set<std::wstring> batchNames{
      federation + L"-batch-a",
      federation + L"-batch-b"};
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveMultipleObjectInstanceNames(batchNames);
  waitFor(owner, [&] { return owner.recorder().multipleReservationSucceeded(batchNames); }, options,
          "multiple object-name reservation");
  require(
      !owner.recorder().hasSingleReservationOutcome(),
      "multiple-name reservation used a single-name reservation callback");
  auto const unreservedReleaseName = federation + L"-not-reserved-for-release";
  requireException(
      [&] {
        owner.rtiAmbassador().releaseMultipleObjectInstanceNames(
            std::set<std::wstring>{*batchNames.begin(), unreservedReleaseName});
      },
      L"ObjectInstanceNameNotReserved",
      "releasing a multiple-name set that contains an unreserved name");
  for (auto const& batchName : batchNames) {
    member.recorder().clearReservations();
    member.rtiAmbassador().reserveObjectInstanceName(batchName);
    waitFor(
        member,
        [&] { return member.recorder().reservationFailed(batchName); },
        options,
        "failed multiple-name release retains every reserved name");
  }
  owner.rtiAmbassador().releaseMultipleObjectInstanceNames(batchNames);
  member.recorder().clearReservations();
  member.rtiAmbassador().reserveObjectInstanceName(*batchNames.begin());
  waitFor(
      member,
      [&] { return member.recorder().reservationSucceeded(*batchNames.begin()); },
      options,
      "a name released from a multiple-name set is available to another joined federate");
  member.rtiAmbassador().releaseObjectInstanceName(*batchNames.begin());
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(*batchNames.begin());
  waitFor(owner, [&] {
    return owner.recorder().reservationSucceeded(*batchNames.begin());
  }, options, "reuse of a released multiple reservation");
  owner.rtiAmbassador().releaseObjectInstanceName(*batchNames.begin());

  auto const name = federation + L"-named-object";
  owner.rtiAmbassador().reserveObjectInstanceName(name);
  waitFor(owner, [&] { return owner.recorder().reservationSucceeded(name); }, options,
          "named object reservation");
  member.recorder().clearReservations();
  member.rtiAmbassador().reserveObjectInstanceName(name);
  waitFor(
      member,
      [&] { return member.recorder().reservationFailed(name); },
      options,
      "another federate cannot successfully reserve a name held by the owner");
  require(
      !member.recorder().hasMultipleReservationOutcome(),
      "single-name contention used a multiple-name reservation callback");
  auto const memberClass = member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass, options.attributeName);
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  member.rtiAmbassador().publishObjectClassAttributes(memberClass, memberAttributes);
  requireException(
      [&] {
        static_cast<void>(member.rtiAmbassador().registerObjectInstance(memberClass, name));
      },
      L"ObjectInstanceNameNotReserved",
      "another publisher consuming the owner's object-name reservation");
  owner.rtiAmbassador().unpublishObjectClassAttributes(objectClass, attributes);
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().registerObjectInstance(objectClass, name));
      },
      L"ObjectClassNotPublished",
      "named registration before object-class publication");
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass, name);
  require(object.isValid(), "Named registration returned an invalid handle");
  require(owner.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Named registration lookup did not round-trip at the owner");
  requireException(
      [&] { owner.rtiAmbassador().releaseObjectInstanceName(name); },
      L"ObjectInstanceNameNotReserved",
      "releasing an object-name reservation consumed by registration");
  requireException(
      [&] {
        static_cast<void>(member.rtiAmbassador().registerObjectInstance(memberClass, name));
      },
      L"ObjectInstanceNameInUse",
      "another publisher registering an already-used object name");
  member.rtiAmbassador().unpublishObjectClassAttributes(memberClass, memberAttributes);

  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "named object discovery");
  auto const discovered = member.recorder().discovery();
  require(discovered.object == object, "Named discovery returned the wrong object handle");
  require(discovered.name == name, "Named discovery returned the wrong instance name");
  require(member.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Named registration lookup did not round-trip at the member");

  auto const reservedAgainstGeneration = federation + L"-reserved-against-generation";
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(reservedAgainstGeneration);
  waitFor(owner, [&] {
    return owner.recorder().reservationSucceeded(reservedAgainstGeneration);
  }, options, "reservation before unnamed registration");
  member.recorder().clearDiscovery();
  auto const generatedObject = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(generatedObject.isValid(), "Unnamed registration returned an invalid handle");
  auto const generatedName = owner.rtiAmbassador().getObjectInstanceName(generatedObject);
  require(generatedName != reservedAgainstGeneration,
          "Unnamed registration consumed another reserved object name");
  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "unnamed object discovery after name reservation");
  auto const generatedDiscovery = member.recorder().discovery();
  require(generatedDiscovery.object == generatedObject &&
              generatedDiscovery.name == generatedName,
          "Unnamed discovery did not preserve its generated name and handle");
  owner.rtiAmbassador().releaseObjectInstanceName(reservedAgainstGeneration);

  auto const unreservedName = federation + L"-unreserved-name";
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().registerObjectInstance(
            objectClass,
            unreservedName));
      },
      L"ObjectInstanceNameNotReserved",
      "named registration with an unreserved object name");

  member.recorder().clearReservations();
  member.rtiAmbassador().reserveObjectInstanceName(name);
  waitFor(member, [&] { return member.recorder().reservationFailed(name); }, options,
          "reservation failure for an in-use object name");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioLocalDeleteObjectInstanceNameLookupBoundaryPortable(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Local-delete/orphan cleanup requires an adapter-supplied FOM");

  Session owner(options, model, "local-delete-orphan-owner");
  Session member(options, model, "local-delete-orphan-member");
  auto const federation =
      federationName(options, "local-delete-object-name-lookup-boundary");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass, options.attributeName);
  auto const deletePrivilege = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, L"HLAprivilegeToDeleteObject");
  require(ownerClass.isValid() && memberClass.isValid() &&
              ownerAttribute.isValid() && memberAttribute.isValid() &&
              deletePrivilege.isValid(),
          "Local-delete/orphan cleanup lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute, deletePrivilege};
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(memberClass, memberAttributes);

  auto const objectName = L"tck-orphan-object-" + federation;
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(owner,
          [&] { return owner.recorder().reservationSucceeded(objectName); },
          options, "reserving the orphan-cleanup object name");
  member.recorder().clearDiscovery();
  auto const object =
      owner.rtiAmbassador().registerObjectInstance(ownerClass, objectName);
  require(object.isValid(),
          "Local-delete/orphan registration returned an invalid object handle");
  waitFor(member,
          [&] { return member.recorder().hasDiscovery(object); },
          options, "local-delete/orphan object discovery");
  auto const discovery = member.recorder().discovery();
  require(discovery.object == object && discovery.name == objectName &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, deletePrivilege),
          "Named object discovery or initial ownership was incorrect");

  member.rtiAmbassador().localDeleteObjectInstance(object);
  requireException(
      [&] { static_cast<void>(member.rtiAmbassador().getObjectInstanceName(object)); },
      L"ObjectInstanceNotKnown",
      "looking up a locally forgotten object instance");
  require(owner.rtiAmbassador().getObjectInstanceName(object) == objectName,
          "Subscriber-local deletion changed the publisher's object identity");

  // Make every federate-owned attribute unowned, then let the last federate
  // that knows the object locally forget it.  With no remaining owner or
  // known instance, §6.1.6 requires the RTI to delete the orphan.
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, ownerAttributes, rti::VariableLengthData{});
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              !owner.rtiAmbassador().isAttributeOwnedByFederate(object, deletePrivilege),
          "The orphan-cleanup setup left an instance attribute owned");
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  owner.rtiAmbassador().localDeleteObjectInstance(object);
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectInstanceName(object)); },
      L"ObjectInstanceNotKnown",
      "looking up the last federate's locally forgotten object instance");

  member.recorder().clearReservations();
  member.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(member,
          [&] {
            return member.recorder().reservationSucceeded(objectName) ||
                member.recorder().reservationFailed(objectName);
          },
          options, "reserving the name released by orphan-object deletion");
  require(member.recorder().reservationSucceeded(objectName),
          "RTI retained the orphan object instead of deleting it");

  member.rtiAmbassador().releaseObjectInstanceName(objectName);

  // An owner that unconditionally divests on resignation is the other public
  // path to the same orphan state: after the last known federate leaves, the
  // still-joined member must be able to reserve the former instance name.
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const resignedObjectName = L"tck-orphan-resigned-object-" + federation;
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(resignedObjectName);
  waitFor(owner,
          [&] { return owner.recorder().reservationSucceeded(resignedObjectName); },
          options,
          "reserving the name for resignation-driven orphan cleanup");
  member.recorder().clearDiscovery();
  auto const resignedObject = owner.rtiAmbassador().registerObjectInstance(
      ownerClass,
      resignedObjectName);
  waitFor(member,
          [&] { return member.recorder().hasDiscovery(resignedObject); },
          options,
          "discovering the resignation-driven orphan object");
  member.rtiAmbassador().localDeleteObjectInstance(resignedObject);
  owner.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);

  member.recorder().clearReservations();
  member.rtiAmbassador().reserveObjectInstanceName(resignedObjectName);
  waitFor(member,
          [&] {
            return member.recorder().reservationSucceeded(resignedObjectName) ||
                member.recorder().reservationFailed(resignedObjectName);
          },
          options,
          "reserving the name released by resignation-driven orphan cleanup");
  require(member.recorder().reservationSucceeded(resignedObjectName),
          "RTI retained an orphan after the final-known federate resigned");
  member.rtiAmbassador().releaseObjectInstanceName(resignedObjectName);

  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      memberClass, memberAttributes);
  member.resign(rti::NO_ACTION);
  owner.disconnect();
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

void scenarioLocalDeleteObjectInstanceNameLookupBoundaryContractPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioLocalDeleteObjectInstanceNameLookupBoundaryPortable(options, model);
}

} // namespace hla_rti_cpp_tck::detail
