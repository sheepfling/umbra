#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioObjectRegistrationDiscoveryLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "object registration/discovery lifecycle requires an adapter-supplied rich FOM");

  Session unjoined(options, model, "unjoined");
  Session publisher(options, model, "owner");
  Session exact(options, model, "member");
  Session base(options, model, "member-base");
  Session cancelled(options, model, "member-cancelled");
  Session late(options, model, "member-late");
  auto const federation = federationName(options, "object-registration-discovery-lifecycle");

  rti::ObjectClassHandle const invalidObjectClass;
  requireException(
      [&] {
        static_cast<void>(unjoined.rtiAmbassador().registerObjectInstance(invalidObjectClass));
      },
      L"NotConnected",
      "registering an object before connect");
  unjoined.connect();
  requireException(
      [&] {
        static_cast<void>(unjoined.rtiAmbassador().registerObjectInstance(invalidObjectClass));
      },
      L"FederateNotExecutionMember",
      "registering an object after connect before join");

  publisher.connect();
  exact.connect();
  base.connect();
  cancelled.connect();
  late.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  exact.join(
      options.memberFederateName + L"-exact",
      options.federateType,
      federation);
  base.join(
      options.memberFederateName + L"-base",
      options.federateType,
      federation);
  cancelled.join(
      options.memberFederateName + L"-cancelled",
      options.federateType,
      federation);
  late.join(
      options.memberFederateName + L"-late",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const exactClass = exact.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const baseClass = base.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const cancelledClass = cancelled.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const lateClass = late.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  require(
      publisherClass.isValid() && exactClass.isValid() && baseClass.isValid() &&
          cancelledClass.isValid() && lateClass.isValid(),
      "object registration/discovery FOM class lookup returned an invalid handle");

  auto const publisherIdentity = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.typedIdentityAttributeName);
  auto const publisherDerived = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.typedDerivedAttributeName);
  auto const exactDerived = exact.rtiAmbassador().getAttributeHandle(
      exactClass,
      options.typedDerivedAttributeName);
  auto const baseIdentity = base.rtiAmbassador().getAttributeHandle(
      baseClass,
      options.typedIdentityAttributeName);
  auto const cancelledDerived = cancelled.rtiAmbassador().getAttributeHandle(
      cancelledClass,
      options.typedDerivedAttributeName);
  auto const lateDerived = late.rtiAmbassador().getAttributeHandle(
      lateClass,
      options.typedDerivedAttributeName);
  require(
      publisherIdentity.isValid() && publisherDerived.isValid() &&
          exactDerived.isValid() && baseIdentity.isValid() &&
          cancelledDerived.isValid() && lateDerived.isValid(),
      "object registration/discovery FOM attribute lookup returned an invalid handle");

  auto const publisherBaseIdentity = publisher.rtiAmbassador().getAttributeHandle(
      publisher.rtiAmbassador().getObjectClassHandle(options.typedObjectClassName),
      options.typedIdentityAttributeName);
  require(
      publisherBaseIdentity == publisherIdentity && baseIdentity == publisherIdentity,
      "inherited object registration/discovery attribute handle changed across classes");

  rti::AttributeHandleSet const publishedAttributes{publisherIdentity, publisherDerived};
  rti::AttributeHandleSet const exactAttributes{exactDerived};
  rti::AttributeHandleSet const baseAttributes{baseIdentity};
  rti::AttributeHandleSet const cancelledAttributes{cancelledDerived};
  rti::AttributeHandleSet const lateAttributes{lateDerived};

  requireException(
      [&] {
        static_cast<void>(publisher.rtiAmbassador().registerObjectInstance(invalidObjectClass));
      },
      L"ObjectClassNotDefined",
      "registering an object for an invalid object class");
  requireException(
      [&] { static_cast<void>(publisher.rtiAmbassador().registerObjectInstance(publisherClass)); },
      L"ObjectClassNotPublished",
      "registering an object before publication");

  exact.rtiAmbassador().subscribeObjectClassAttributes(
      exactClass,
      exactAttributes,
      true,
      L"");
  base.rtiAmbassador().subscribeObjectClassAttributes(
      baseClass,
      baseAttributes,
      true,
      L"");
  cancelled.rtiAmbassador().subscribeObjectClassAttributes(
      cancelledClass,
      cancelledAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publishedAttributes);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "object registration/discovery returned an invalid object handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(!objectName.empty(), "object registration/discovery returned an empty object name");
  require(
      publisher.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          publisher.rtiAmbassador().getKnownObjectClassHandle(object) == publisherClass,
      "publisher object identity lookup did not round-trip");

  requireException(
      [&] { static_cast<void>(late.rtiAmbassador().getObjectInstanceHandle(objectName)); },
      L"ObjectInstanceNotKnown",
      "looking up an undiscovered object by name");
  if (model == rti::HLA_EVOKED) {
    require(
        exact.recorder().discoveryCount() == 0U &&
            base.recorder().discoveryCount() == 0U &&
            cancelled.recorder().discoveryCount() == 0U,
        "evoked discovery callback arrived before callback servicing");
    requireException(
        [&] { static_cast<void>(exact.rtiAmbassador().getKnownObjectClassHandle(object)); },
        L"ObjectInstanceNotKnown",
        "looking up an evoked object before discovery");
  } else {
    require(
        exact.recorder().discoveryCount() == 1U &&
            base.recorder().discoveryCount() == 1U &&
            cancelled.recorder().discoveryCount() == 1U,
        "immediate registration did not deliver one discovery callback per subscriber");
  }

  if (model == rti::HLA_EVOKED) {
    cancelled.rtiAmbassador().unsubscribeObjectClassAttributes(
        cancelledClass,
        cancelledAttributes);
    require(
        !cancelled.evokeMultipleCallbacks(0.0, 0.0),
        "cancelled discovery callback remained queued after unsubscribe");
  }

  late.rtiAmbassador().subscribeObjectClassAttributes(
      lateClass,
      lateAttributes,
      true,
      L"");

  auto requireDiscovery = [&](Session& session,
                              rti::ObjectClassHandle const& expectedClass,
                              std::string const& description) {
    waitFor(
        session,
        [&] { return session.recorder().discovery().present; },
        options,
        description);
    auto const discovery = session.recorder().discovery();
    require(
        discovery.object == object && discovery.objectClass == expectedClass &&
            discovery.name == objectName,
        description + " returned the wrong object identity");
    require(
        session.rtiAmbassador().getKnownObjectClassHandle(object) == expectedClass &&
            session.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
            session.rtiAmbassador().getObjectInstanceName(object) == objectName,
        description + " did not establish stable identity lookups");
  };

  requireDiscovery(exact, exactClass, "exact object discovery");
  requireDiscovery(base, baseClass, "base-class object discovery");
  requireDiscovery(late, lateClass, "late object discovery");
  if (model == rti::HLA_EVOKED) {
    require(
        cancelled.recorder().discoveryCount() == 0U,
        "unsubscribed evoked subscriber received object discovery");
    requireException(
        [&] { static_cast<void>(cancelled.rtiAmbassador().getObjectInstanceName(object)); },
        L"ObjectInstanceNotKnown",
        "looking up an unsubscribed object by handle");
  } else {
    cancelled.rtiAmbassador().unsubscribeObjectClassAttributes(
        cancelledClass,
        cancelledAttributes);
    require(
        cancelled.recorder().discoveryCount() == 1U,
        "immediate subscriber changed a delivered discovery count after unsubscribe");
  }

  exact.rtiAmbassador().subscribeObjectClassAttributes(
      exactClass,
      exactAttributes,
      true,
      L"");
  require(
      !exact.evokeMultipleCallbacks(0.0, 0.0) && exact.recorder().discoveryCount() == 1U,
      "repeating an object subscription generated a duplicate discovery callback");

  late.resign(rti::NO_ACTION);
  cancelled.resign(rti::NO_ACTION);
  base.resign(rti::NO_ACTION);
  exact.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  unjoined.disconnect();
  late.disconnect();
  cancelled.disconnect();
  base.disconnect();
  exact.disconnect();
  publisher.disconnect();
}

void scenarioObjectRegistrationDiscoveryMultiRecipient(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "registration-fanout-publisher");
  Session first(options, model, "registration-fanout-first");
  Session second(options, model, "registration-fanout-second");
  auto const federation = federationName(
      options,
      "object-registration-discovery-multi-recipient");
  connectAndJoin(publisher, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName + L"-second",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const firstClass = first.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const secondClass = second.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const firstAttribute = first.rtiAmbassador().getAttributeHandle(
      firstClass,
      options.attributeName);
  auto const secondAttribute = second.rtiAmbassador().getAttributeHandle(
      secondClass,
      options.attributeName);
  require(
      publisherClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
          publisherAttribute.isValid() && firstAttribute.isValid() &&
          secondAttribute.isValid(),
      "multi-recipient registration lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  first.rtiAmbassador().subscribeObjectClassAttributes(
      firstClass,
      firstAttributes,
      true,
      L"");
  second.rtiAmbassador().subscribeObjectClassAttributes(
      secondClass,
      secondAttributes,
      true,
      L"");

  auto const firstObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  auto const secondObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      firstObject.isValid() && secondObject.isValid() && firstObject != secondObject,
      "multi-recipient registration returned invalid or duplicate object handles");
  auto const firstName = publisher.rtiAmbassador().getObjectInstanceName(firstObject);
  auto const secondName = publisher.rtiAmbassador().getObjectInstanceName(secondObject);
  require(
      !firstName.empty() && !secondName.empty() && firstName != secondName,
      "multi-recipient registration returned invalid or duplicate object names");

  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasDiscovery(firstObject) &&
            first.recorder().hasDiscovery(secondObject) &&
            second.recorder().hasDiscovery(firstObject) &&
            second.recorder().hasDiscovery(secondObject);
      },
      options,
      "multi-recipient object registration discovery");
  require(
      publisher.recorder().discoveryCount() == 0U &&
          first.recorder().discoveryCount() == 2U &&
          second.recorder().discoveryCount() == 2U,
      "multi-recipient registration delivered the wrong discovery callback counts");

  auto assertIdentity = [&](Session& receiver,
                            rti::ObjectClassHandle const& receiverClass,
                            std::string const& description) {
    require(
        receiver.rtiAmbassador().getObjectInstanceHandle(firstName) == firstObject &&
            receiver.rtiAmbassador().getObjectInstanceHandle(secondName) == secondObject &&
            receiver.rtiAmbassador().getObjectInstanceName(firstObject) == firstName &&
            receiver.rtiAmbassador().getObjectInstanceName(secondObject) == secondName &&
            receiver.rtiAmbassador().getKnownObjectClassHandle(firstObject) == receiverClass &&
            receiver.rtiAmbassador().getKnownObjectClassHandle(secondObject) == receiverClass,
        description + " did not preserve both standard object identities");
    auto const latest = receiver.recorder().discovery();
    require(
        latest.present && latest.object == secondObject &&
            latest.objectClass == receiverClass && latest.name == secondName &&
            latest.producer == publisher.federateHandle(),
        description + " returned the wrong final discovery metadata");
  };
  assertIdentity(first, firstClass, "first registration recipient");
  assertIdentity(second, secondClass, "second registration recipient");

  first.rtiAmbassador().unsubscribeObjectClassAttributes(firstClass, firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(secondClass, secondAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  publisher.disconnect();
}



void scenarioNamedRegistrationMultiRecipient(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "named-registration-fanout-publisher");
  Session first(options, model, "named-registration-fanout-first");
  Session second(options, model, "named-registration-fanout-second");
  auto const federation = federationName(options, "named-registration-multi-recipient");
  connectAndJoin(publisher, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName + L"-second",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const firstClass = first.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const secondClass = second.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const firstAttribute = first.rtiAmbassador().getAttributeHandle(
      firstClass,
      options.attributeName);
  auto const secondAttribute = second.rtiAmbassador().getAttributeHandle(
      secondClass,
      options.attributeName);
  require(
      publisherClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
          publisherAttribute.isValid() && firstAttribute.isValid() &&
          secondAttribute.isValid(),
      "multi-recipient named registration lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  first.rtiAmbassador().subscribeObjectClassAttributes(
      firstClass,
      firstAttributes,
      true,
      L"");
  second.rtiAmbassador().subscribeObjectClassAttributes(
      secondClass,
      secondAttributes,
      true,
      L"");

  auto const firstName = federation + L"-named-first";
  auto const secondName = federation + L"-named-second";
  publisher.recorder().clearReservations();
  publisher.rtiAmbassador().reserveObjectInstanceName(firstName);
  waitFor(
      publisher,
      [&] { return publisher.recorder().reservationSucceeded(firstName); },
      options,
      "first named-registration fan-out reservation");
  publisher.recorder().clearReservations();
  publisher.rtiAmbassador().reserveObjectInstanceName(secondName);
  waitFor(
      publisher,
      [&] { return publisher.recorder().reservationSucceeded(secondName); },
      options,
      "second named-registration fan-out reservation");

  auto const firstObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass,
      firstName);
  auto const secondObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass,
      secondName);
  require(
      firstObject.isValid() && secondObject.isValid() && firstObject != secondObject,
      "multi-recipient named registration returned invalid or duplicate object handles");
  require(
      publisher.rtiAmbassador().getObjectInstanceHandle(firstName) == firstObject &&
          publisher.rtiAmbassador().getObjectInstanceHandle(secondName) == secondObject &&
          publisher.rtiAmbassador().getObjectInstanceName(firstObject) == firstName &&
          publisher.rtiAmbassador().getObjectInstanceName(secondObject) == secondName,
      "publisher named-registration identity lookup did not round-trip");

  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasDiscovery(firstObject) &&
            first.recorder().hasDiscovery(secondObject) &&
            second.recorder().hasDiscovery(firstObject) &&
            second.recorder().hasDiscovery(secondObject);
      },
      options,
      "multi-recipient named registration discovery");
  require(
      publisher.recorder().discoveryCount() == 0U &&
          first.recorder().discoveryCount() == 2U &&
          second.recorder().discoveryCount() == 2U,
      "multi-recipient named registration delivered the wrong discovery callback counts");

  auto assertIdentity = [&](Session& receiver,
                            rti::ObjectClassHandle const& receiverClass,
                            std::string const& description) {
    require(
        receiver.rtiAmbassador().getObjectInstanceHandle(firstName) == firstObject &&
            receiver.rtiAmbassador().getObjectInstanceHandle(secondName) == secondObject &&
            receiver.rtiAmbassador().getObjectInstanceName(firstObject) == firstName &&
            receiver.rtiAmbassador().getObjectInstanceName(secondObject) == secondName &&
            receiver.rtiAmbassador().getKnownObjectClassHandle(firstObject) == receiverClass &&
            receiver.rtiAmbassador().getKnownObjectClassHandle(secondObject) == receiverClass,
        description + " did not preserve both named object identities");
  };
  assertIdentity(first, firstClass, "first named-registration recipient");
  assertIdentity(second, secondClass, "second named-registration recipient");

  first.rtiAmbassador().unsubscribeObjectClassAttributes(firstClass, firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(secondClass, secondAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  publisher.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
