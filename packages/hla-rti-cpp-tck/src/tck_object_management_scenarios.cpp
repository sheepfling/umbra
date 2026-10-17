#include "tck_main_scenarios.hpp"

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
  std::vector<std::uint8_t> deletionTagBytes{0xD1U, 0xE2U, 0xF3U};
  rti::VariableLengthData deletionTag(deletionTagBytes.data(), deletionTagBytes.size());
  owner.rtiAmbassador().deleteObjectInstance(deleted.first, deletionTag);
  waitFor(member, [&] {
    return member.recorder().hasRemoval(
        deleted.first, deletionTagBytes, owner.federateHandle());
  }, options, "ordinary object removal");
  requireException(
      [&] { static_cast<void>(member.rtiAmbassador().getObjectInstanceHandle(deleted.second)); },
      L"ObjectInstanceNotKnown",
      "Object lookup after remote deletion");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectInstanceName(deleted.first)); },
      L"ObjectInstanceNotKnown",
      "Owner object lookup after remote deletion");

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

  auto const releasedName = federation + L"-released-name";
  owner.rtiAmbassador().reserveObjectInstanceName(releasedName);
  waitFor(owner, [&] { return owner.recorder().reservationSucceeded(releasedName); }, options,
          "single object-name reservation");
  owner.rtiAmbassador().releaseObjectInstanceName(releasedName);
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
  owner.rtiAmbassador().releaseMultipleObjectInstanceNames(batchNames);
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
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass, name);
  require(object.isValid(), "Named registration returned an invalid handle");
  require(owner.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Named registration lookup did not round-trip at the owner");

  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "named object discovery");
  auto const discovered = member.recorder().discovery();
  require(discovered.object == object, "Named discovery returned the wrong object handle");
  require(discovered.name == name, "Named discovery returned the wrong instance name");
  require(member.rtiAmbassador().getObjectInstanceHandle(name) == object,
          "Named registration lookup did not round-trip at the member");

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

} // namespace hla_rti_cpp_tck::detail
