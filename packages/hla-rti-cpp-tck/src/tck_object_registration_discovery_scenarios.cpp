#include "tck_main_scenarios.hpp"

#include <chrono>
#include <thread>

namespace hla_rti_cpp_tck::detail {

void scenarioReservedObjectNameDoesNotDiscover(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "reserved-name-publisher");
  Session subscriber(options, model, "reserved-name-subscriber");
  auto const federation = federationName(options, "reserved-name-no-discovery");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      options.attributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid(),
      "reserved-name no-discovery lookup returned an invalid handle");

  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      rti::AttributeHandleSet{publisherAttribute});
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      rti::AttributeHandleSet{subscriberAttribute},
      true,
      L"");

  auto const objectName = federation + L"-reserved-without-discovery";
  subscriber.recorder().clearDiscovery();
  publisher.recorder().clearReservations();
  publisher.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitForSessions(
      {&publisher, &subscriber},
      [&] { return publisher.recorder().reservationSucceeded(objectName); },
      options,
      "reserved object-name success before registration");
  require(
      subscriber.recorder().discoveryCount() == 0U,
      "Reserve Object Instance Name caused an object-discovery callback");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass,
      objectName);
  require(object.isValid(), "named registration after reservation returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "registered object discovery after name reservation");
  auto const discovery = subscriber.recorder().discovery();
  require(
      subscriber.recorder().discoveryCount() == 1U &&
          discovery.present && discovery.object == object &&
          discovery.objectClass == subscriberClass && discovery.name == objectName,
      "registration did not produce exactly one matching discovery after reservation");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      rti::AttributeHandleSet{subscriberAttribute});
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      rti::AttributeHandleSet{publisherAttribute});
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

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

void scenarioOverlappingObjectClassSubscriptionsSingleDiscovery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "overlapping object-class subscriptions require an adapter-supplied hierarchy FOM");
  Session publisher(options, model, "overlap-publisher");
  Session subscriber(options, model, "overlap-subscriber");
  auto const federation = federationName(
      options, "overlapping-object-class-subscriptions-single-discovery");
  connectAndJoin(publisher, subscriber, options, federation, options.modelFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const subscriberBaseClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const subscriberDerivedClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const publisherIdentity = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.typedIdentityAttributeName);
  auto const publisherDerivedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.typedDerivedAttributeName);
  auto const subscriberBaseIdentity = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberBaseClass, options.typedIdentityAttributeName);
  auto const subscriberDerivedAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberDerivedClass, options.typedDerivedAttributeName);
  require(
      publisherClass.isValid() && subscriberBaseClass.isValid() &&
          subscriberDerivedClass.isValid() &&
          subscriberBaseClass != subscriberDerivedClass &&
          publisherIdentity.isValid() && publisherDerivedAttribute.isValid() &&
          subscriberBaseIdentity.isValid() && subscriberDerivedAttribute.isValid(),
      "overlapping subscription hierarchy FOM lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherIdentity, publisherDerivedAttribute};
  rti::AttributeHandleSet const baseAttributes{subscriberBaseIdentity};
  rti::AttributeHandleSet const derivedAttributes{subscriberDerivedAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberBaseClass, baseAttributes, true, L"");
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberDerivedClass, derivedAttributes, true, L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "overlapping-subscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().discoveryCount() != 0U; },
      options,
      "discovery through overlapping base and derived subscriptions");

  auto const quietDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(150);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    subscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto const discovery = subscriber.recorder().discovery();
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      subscriber.recorder().discoveryCount() == 1U && discovery.present &&
          discovery.object == object && discovery.objectClass == subscriberDerivedClass &&
          !objectName.empty() &&
          subscriber.rtiAmbassador().getKnownObjectClassHandle(object) ==
              subscriberDerivedClass &&
          subscriber.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "one derived registration did not produce exactly one discovery of its registered class");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberDerivedClass, derivedAttributes);
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberBaseClass, baseAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
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

void scenarioActiveSubscriptionObjectDiscovery(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "active-subscription-discovery-publisher");
  Session subscriber(options, model, "active-subscription-discovery-subscriber");
  auto const federation = federationName(options, "active-subscription-object-discovery");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      options.attributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid(),
      "active-subscription discovery lookup returned an invalid class or attribute handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);

  auto const existingObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      existingObject.isValid(),
      "pre-subscription object registration returned an invalid handle");
  auto const existingName = publisher.rtiAmbassador().getObjectInstanceName(existingObject);
  require(!existingName.empty(), "pre-subscription object registration returned an empty name");

  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      subscriber.recorder().discoveryCount() == 0U,
      "unsubscribed member received discovery for an existing object");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(existingObject); },
      options,
      "discovery of an object already registered when the active subscription is made");
  auto const existingDiscovery = subscriber.recorder().discovery();
  require(
      subscriber.recorder().discoveryCount() == 1U && existingDiscovery.present &&
          existingDiscovery.object == existingObject &&
          existingDiscovery.objectClass == subscriberClass &&
          existingDiscovery.name == existingName &&
          existingDiscovery.producer == publisher.federateHandle(),
      "active subscription did not produce exactly one matching discovery for the existing object");

  auto const laterObject = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      laterObject.isValid() && laterObject != existingObject,
      "post-subscription object registration returned an invalid or duplicate handle");
  auto const laterName = publisher.rtiAmbassador().getObjectInstanceName(laterObject);
  require(
      !laterName.empty() && laterName != existingName,
      "post-subscription object registration returned an invalid or duplicate name");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(laterObject); },
      options,
      "discovery of an object registered after the active subscription");
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }

  auto const laterDiscovery = subscriber.recorder().discovery();
  require(
      subscriber.recorder().discoveryCount() == 2U &&
          subscriber.recorder().hasDiscovery(existingObject) &&
          subscriber.recorder().hasDiscovery(laterObject) &&
          laterDiscovery.present && laterDiscovery.object == laterObject &&
          laterDiscovery.objectClass == subscriberClass && laterDiscovery.name == laterName &&
          laterDiscovery.producer == publisher.federateHandle() &&
          subscriber.rtiAmbassador().getObjectInstanceHandle(existingName) == existingObject &&
          subscriber.rtiAmbassador().getObjectInstanceHandle(laterName) == laterObject &&
          subscriber.rtiAmbassador().getKnownObjectClassHandle(existingObject) == subscriberClass &&
          subscriber.rtiAmbassador().getKnownObjectClassHandle(laterObject) == subscriberClass,
      "active subscription did not discover each existing and later object exactly once with matching identity");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
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

void scenarioPerFederateDiscoveredClass(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty(),
      "per-federate discovered-class testing requires an adapter-supplied hierarchy FOM");
  Session publisher(options, model, "per-federate-class-publisher");
  Session baseSubscriber(options, model, "per-federate-class-base-subscriber");
  Session derivedSubscriber(options, model, "per-federate-class-derived-subscriber");
  auto const federation = federationName(options, "per-federate-discovered-class");
  connectAndJoin(
      publisher,
      baseSubscriber,
      options,
      federation,
      options.knownClassFom);
  derivedSubscriber.connect();
  derivedSubscriber.join(
      options.memberFederateName + L"-derived-view",
      options.federateType,
      federation);

  auto const publisherBaseClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const publisherDerivedClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const baseSubscriberClass = baseSubscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const derivedSubscriberClass = derivedSubscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const publisherBaseAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherDerivedClass,
      options.knownClassKnownAttributeName);
  auto const publisherDerivedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherDerivedClass,
      options.knownClassDerivedAttributeName);
  auto const baseSubscriberAttribute = baseSubscriber.rtiAmbassador().getAttributeHandle(
      baseSubscriberClass,
      options.knownClassKnownAttributeName);
  auto const derivedSubscriberAttribute = derivedSubscriber.rtiAmbassador().getAttributeHandle(
      derivedSubscriberClass,
      options.knownClassDerivedAttributeName);
  require(
      publisherBaseClass.isValid() && publisherDerivedClass.isValid() &&
          publisherBaseClass != publisherDerivedClass &&
          baseSubscriberClass.isValid() && derivedSubscriberClass.isValid() &&
          baseSubscriberClass != derivedSubscriberClass &&
          publisherBaseAttribute.isValid() && publisherDerivedAttribute.isValid() &&
          baseSubscriberAttribute.isValid() && derivedSubscriberAttribute.isValid(),
      "adapter hierarchy FOM did not resolve the configured classes and attributes");

  rti::AttributeHandleSet const publishedAttributes{
      publisherBaseAttribute,
      publisherDerivedAttribute};
  rti::AttributeHandleSet const baseAttributes{baseSubscriberAttribute};
  rti::AttributeHandleSet const derivedAttributes{derivedSubscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherDerivedClass,
      publishedAttributes);
  baseSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      baseSubscriberClass,
      baseAttributes);
  derivedSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      derivedSubscriberClass,
      derivedAttributes);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherDerivedClass);
  require(object.isValid(), "per-federate class registration returned an invalid handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  waitForSessions(
      {&baseSubscriber, &derivedSubscriber},
      [&] {
        return baseSubscriber.recorder().discoveryCount() == 1U &&
            derivedSubscriber.recorder().discoveryCount() == 1U;
      },
      options,
      "per-federate base and derived object discoveries");

  auto const quietDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(150);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    baseSubscriber.pump();
    derivedSubscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto const baseDiscovery = baseSubscriber.recorder().discovery();
  auto const derivedDiscovery = derivedSubscriber.recorder().discovery();
  require(
      baseSubscriber.recorder().discoveryCount() == 1U &&
          derivedSubscriber.recorder().discoveryCount() == 1U &&
          baseDiscovery.present && derivedDiscovery.present &&
          baseDiscovery.objectClass == baseSubscriberClass &&
          derivedDiscovery.objectClass == derivedSubscriberClass &&
          baseSubscriber.rtiAmbassador().getObjectInstanceName(baseDiscovery.object) ==
              objectName &&
          derivedSubscriber.rtiAmbassador().getObjectInstanceName(derivedDiscovery.object) ==
              objectName &&
          baseSubscriber.rtiAmbassador().getKnownObjectClassHandle(baseDiscovery.object) ==
              baseSubscriberClass &&
          derivedSubscriber.rtiAmbassador().getKnownObjectClassHandle(derivedDiscovery.object) ==
              derivedSubscriberClass &&
          publisher.rtiAmbassador().getKnownObjectClassHandle(object) ==
              publisherDerivedClass &&
          publisher.recorder().discoveryCount() == 0U,
      "the same object did not have one subscriber-specific discovered class per federate");

  baseSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      baseSubscriberClass,
      baseAttributes);
  derivedSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      derivedSubscriberClass,
      derivedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherDerivedClass,
      publishedAttributes);
  derivedSubscriber.resign(rti::NO_ACTION);
  baseSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  derivedSubscriber.disconnect();
  baseSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioDiscoveredClassStabilityAfterSpecificSubscription(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty(),
      "discovered-class stability requires an adapter-supplied hierarchy FOM");
  Session publisher(options, model, "specific-subscription-publisher");
  Session subscriber(options, model, "specific-subscription-subscriber");
  auto const federation = federationName(options, "discovered-class-stability");
  connectAndJoin(publisher, subscriber, options, federation, options.knownClassFom);

  auto const publisherBaseClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const publisherDerivedClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const subscriberBaseClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const subscriberDerivedClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const publisherBaseAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherDerivedClass,
      options.knownClassKnownAttributeName);
  auto const publisherDerivedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherDerivedClass,
      options.knownClassDerivedAttributeName);
  auto const subscriberBaseAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberBaseClass,
      options.knownClassKnownAttributeName);
  auto const subscriberDerivedAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberDerivedClass,
      options.knownClassDerivedAttributeName);
  require(
      publisherBaseClass.isValid() && publisherDerivedClass.isValid() &&
          publisherBaseClass != publisherDerivedClass &&
          subscriberBaseClass.isValid() && subscriberDerivedClass.isValid() &&
          subscriberBaseClass != subscriberDerivedClass &&
          publisherBaseAttribute.isValid() && publisherDerivedAttribute.isValid() &&
          subscriberBaseAttribute.isValid() && subscriberDerivedAttribute.isValid(),
      "adapter hierarchy FOM did not resolve the configured base/derived classes and attributes");

  rti::AttributeHandleSet const publishedAttributes{
      publisherBaseAttribute,
      publisherDerivedAttribute};
  rti::AttributeHandleSet const baseAttributes{subscriberBaseAttribute};
  rti::AttributeHandleSet const derivedAttributes{subscriberDerivedAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherDerivedClass,
      publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberBaseClass,
      baseAttributes);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherDerivedClass);
  require(object.isValid(), "specific-subscription registration returned an invalid handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  waitForSessions(
      {&subscriber},
      [&] { return subscriber.recorder().discoveryCount() == 1U; },
      options,
      "base-class object discovery before adding a specific subscription");

  auto pumpQuietWindow = [&] {
    auto const quietDeadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(150);
    while (std::chrono::steady_clock::now() < quietDeadline) {
      subscriber.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };
  auto assertBaseViewStable = [&] {
    auto const discovery = subscriber.recorder().discovery();
    require(
        subscriber.recorder().discoveryCount() == 1U && discovery.present &&
            discovery.objectClass == subscriberBaseClass &&
            subscriber.rtiAmbassador().getObjectInstanceName(discovery.object) == objectName &&
            subscriber.rtiAmbassador().getKnownObjectClassHandle(discovery.object) ==
                subscriberBaseClass &&
            publisher.rtiAmbassador().getKnownObjectClassHandle(object) ==
                publisherDerivedClass &&
            publisher.recorder().discoveryCount() == 0U,
        "adding a more-specific subscription changed the existing discovered class");
  };
  pumpQuietWindow();
  assertBaseViewStable();

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberDerivedClass,
      derivedAttributes);
  pumpQuietWindow();
  assertBaseViewStable();

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberDerivedClass,
      derivedAttributes);
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberBaseClass,
      baseAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherDerivedClass,
      publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioRegisteredObjectClassImmutability(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty(),
      "registered-class immutability testing requires an adapter-supplied hierarchy FOM");
  Session publisher(options, model, "registered-class-publisher");
  Session baseSubscriber(options, model, "registered-class-base-subscriber");
  Session derivedSubscriber(options, model, "registered-class-derived-subscriber");
  auto const federation = federationName(options, "registered-object-class-immutability");
  connectAndJoin(
      publisher,
      baseSubscriber,
      options,
      federation,
      options.knownClassFom);
  derivedSubscriber.connect();
  derivedSubscriber.join(
      options.memberFederateName + L"-registered-class-view",
      options.federateType,
      federation);

  auto const baseClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const derivedClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const publisherBaseAttribute = publisher.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.knownClassKnownAttributeName);
  auto const publisherDerivedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.knownClassDerivedAttributeName);
  auto const subscriberBaseClass = baseSubscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const baseSubscriberDerivedClass = baseSubscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const derivedSubscriberBaseClass = derivedSubscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const subscriberDerivedClass = derivedSubscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const baseAttribute = baseSubscriber.rtiAmbassador().getAttributeHandle(
      subscriberBaseClass,
      options.knownClassKnownAttributeName);
  auto const derivedAttribute = derivedSubscriber.rtiAmbassador().getAttributeHandle(
      subscriberDerivedClass,
      options.knownClassDerivedAttributeName);
  auto const baseViewDerivedAttribute = baseSubscriber.rtiAmbassador().getAttributeHandle(
      baseSubscriberDerivedClass,
      options.knownClassDerivedAttributeName);
  auto const derivedViewBaseAttribute = derivedSubscriber.rtiAmbassador().getAttributeHandle(
      derivedSubscriberBaseClass,
      options.knownClassKnownAttributeName);
  require(
      baseClass.isValid() && derivedClass.isValid() && baseClass != derivedClass &&
          subscriberBaseClass.isValid() && baseSubscriberDerivedClass.isValid() &&
          subscriberBaseClass != baseSubscriberDerivedClass &&
          derivedSubscriberBaseClass.isValid() && subscriberDerivedClass.isValid() &&
          derivedSubscriberBaseClass != subscriberDerivedClass &&
          publisherBaseAttribute.isValid() && publisherDerivedAttribute.isValid() &&
          baseAttribute.isValid() && derivedAttribute.isValid() &&
          baseViewDerivedAttribute.isValid() && derivedViewBaseAttribute.isValid(),
      "adapter hierarchy FOM did not resolve the configured base/derived classes and attributes");

  rti::AttributeHandleSet const publishedAttributes{
      publisherBaseAttribute,
      publisherDerivedAttribute};
  rti::AttributeHandleSet const baseAttributes{baseAttribute};
  rti::AttributeHandleSet const derivedAttributes{derivedAttribute};
  rti::AttributeHandleSet const baseViewDerivedAttributes{baseViewDerivedAttribute};
  rti::AttributeHandleSet const derivedViewBaseAttributes{derivedViewBaseAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(derivedClass, publishedAttributes);
  baseSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberBaseClass,
      baseAttributes);
  derivedSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberDerivedClass,
      derivedAttributes);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(derivedClass);
  require(object.isValid(), "registered-class identity test returned an invalid object handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  waitForSessions(
      {&baseSubscriber, &derivedSubscriber},
      [&] {
        return baseSubscriber.recorder().discoveryCount() == 1U &&
            derivedSubscriber.recorder().discoveryCount() == 1U;
      },
      options,
      "initial base and derived class-view discoveries");

  auto pumpQuietWindow = [&] {
    auto const quietDeadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(150);
    while (std::chrono::steady_clock::now() < quietDeadline) {
      baseSubscriber.pump();
      derivedSubscriber.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };
  auto assertStableViews = [&] {
    auto const baseDiscovery = baseSubscriber.recorder().discovery();
    auto const derivedDiscovery = derivedSubscriber.recorder().discovery();
    require(
        baseSubscriber.recorder().discoveryCount() == 1U &&
            derivedSubscriber.recorder().discoveryCount() == 1U &&
            baseDiscovery.present && derivedDiscovery.present &&
            baseSubscriber.rtiAmbassador().getObjectInstanceName(baseDiscovery.object) ==
                objectName &&
            derivedSubscriber.rtiAmbassador().getObjectInstanceName(derivedDiscovery.object) ==
                objectName &&
            baseDiscovery.objectClass == subscriberBaseClass &&
            derivedDiscovery.objectClass == subscriberDerivedClass &&
            baseSubscriber.rtiAmbassador().getKnownObjectClassHandle(baseDiscovery.object) ==
                subscriberBaseClass &&
            derivedSubscriber.rtiAmbassador().getKnownObjectClassHandle(derivedDiscovery.object) ==
                subscriberDerivedClass &&
            publisher.rtiAmbassador().getKnownObjectClassHandle(object) == derivedClass &&
            publisher.recorder().discoveryCount() == 0U,
        "registered or subscriber-known object class changed with a federate's class view");
  };
  pumpQuietWindow();
  assertStableViews();

  baseSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      baseSubscriberDerivedClass,
      baseViewDerivedAttributes);
  pumpQuietWindow();
  assertStableViews();

  derivedSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberDerivedClass,
      derivedAttributes);
  derivedSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      derivedSubscriberBaseClass,
      derivedViewBaseAttributes);
  pumpQuietWindow();
  assertStableViews();

  baseSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      baseSubscriberDerivedClass,
      baseViewDerivedAttributes);
  baseSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberBaseClass,
      baseAttributes);
  derivedSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      derivedSubscriberBaseClass,
      derivedViewBaseAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(derivedClass, publishedAttributes);
  derivedSubscriber.resign(rti::NO_ACTION);
  baseSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  derivedSubscriber.disconnect();
  baseSubscriber.disconnect();
  publisher.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
