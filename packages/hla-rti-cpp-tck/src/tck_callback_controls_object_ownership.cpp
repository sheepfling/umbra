#include "tck_callback_controls_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioCallbackControls(Options const& options, rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "callback-controls");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  rti::ObjectClassHandle publisherObjectClass;
  rti::AttributeHandle publisherAttribute;
  rti::InteractionClassHandle publisherInteraction;
  rti::ParameterHandle publisherParameter;
  handles(
      publisher,
      options,
      publisherObjectClass,
      publisherAttribute,
      publisherInteraction,
      publisherParameter);
  rti::ObjectClassHandle receiverObjectClass;
  rti::AttributeHandle receiverAttribute;
  rti::InteractionClassHandle receiverInteraction;
  rti::ParameterHandle receiverParameter;
  handles(
      receiver,
      options,
      receiverObjectClass,
      receiverAttribute,
      receiverInteraction,
      receiverParameter);
  require(publisherInteraction == receiverInteraction &&
              publisherParameter == receiverParameter,
          "callback-control handles did not retain identity across members");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);

  std::vector<std::uint8_t> valueBytes{0xC0U, 0xDEU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  std::vector<std::uint8_t> firstTagBytes{0x43U, 0x42U, 0x31U};
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      firstTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().interaction().present; },
      options,
      "baseline callback-control interaction");
  require(receiver.recorder().interaction().tag == firstTagBytes,
          "baseline callback-control interaction lost its tag");

  receiver.recorder().clearInteraction();
  receiver.rtiAmbassador().disableCallbacks();
  std::vector<std::uint8_t> secondTagBytes{0x43U, 0x42U, 0x32U};
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      secondTag);

  if (model == rti::HLA_EVOKED) {
    require(
        receiver.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending process event");
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(!receiver.recorder().interaction().present,
          "disabled callbacks exposed an interaction callback");

  receiver.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().interaction().present; },
      options,
      "re-enabled callback-control interaction");
  require(receiver.recorder().interaction().tag == secondTagBytes,
          "re-enabled callback-control interaction lost its tag");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControls(options, model);
}

void scenarioCallbackControlsAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "callback-controls-attribute-update");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherClass == receiverClass && publisherAttribute == receiverAttribute,
      "callback-control attribute handles did not retain identity across members");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "callback-control attribute registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "callback-control attribute object discovery");

  auto sendUpdate = [&](std::vector<std::uint8_t> const& value,
                        std::vector<std::uint8_t> const& tagBytes) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };

  std::vector<std::uint8_t> const firstValue{0x41U, 0x54U, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x43U, 0x42U, 0x31U};
  sendUpdate(firstValue, firstTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "baseline callback-control attribute reflection");
  auto const firstReflection = receiver.recorder().reflection();
  require(
      firstReflection.object == object &&
          firstReflection.values.size() == 1U &&
          firstReflection.values.count(receiverAttribute) == 1U &&
          copyBytes(firstReflection.values.at(receiverAttribute)) == firstValue &&
          firstReflection.tag == firstTag &&
          firstReflection.producer == publisher.federateHandle() &&
          firstReflection.transportation.isValid(),
      "baseline callback-control attribute reflection lost standard delivery data");

  receiver.recorder().clearReflection();
  receiver.rtiAmbassador().disableCallbacks();
  std::vector<std::uint8_t> const secondValue{0x41U, 0x54U, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x43U, 0x42U, 0x32U};
  sendUpdate(secondValue, secondTag);
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending attribute event");
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !receiver.recorder().reflection().present,
      "disabled callbacks exposed an attribute reflection");

  receiver.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "re-enabled callback-control attribute reflection");
  auto const secondReflection = receiver.recorder().reflection();
  require(
      secondReflection.object == object &&
          secondReflection.values.size() == 1U &&
          secondReflection.values.count(receiverAttribute) == 1U &&
          copyBytes(secondReflection.values.at(receiverAttribute)) == secondValue &&
          secondReflection.tag == secondTag &&
          secondReflection.producer == publisher.federateHandle(),
      "re-enabled callback-control attribute reflection lost standard delivery data");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsAttributeUpdate(options, model);
}

void scenarioCallbackControlsObjectRemoval(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "callback-controls-object-removal");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherClass == receiverClass && publisherAttribute == receiverAttribute,
      "callback-control object-removal handles did not retain identity across members");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const firstObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      firstObject.isValid(),
      "callback-control object-removal baseline registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(firstObject); },
      options,
      "callback-control object-removal baseline discovery");
  std::vector<std::uint8_t> const firstTagBytes{0x52U, 0x4DU, 0x31U};
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  publisher.rtiAmbassador().deleteObjectInstance(firstObject, firstTag);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().hasRemoval(
            firstObject,
            firstTagBytes,
            publisher.federateHandle());
      },
      options,
      "baseline callback-control object removal");

  receiver.recorder().clearDiscovery();
  receiver.recorder().clearRemovals();
  auto const secondObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      secondObject.isValid(),
      "callback-control object-removal gated registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(secondObject); },
      options,
      "callback-control object-removal gated discovery");

  receiver.rtiAmbassador().disableCallbacks();
  std::vector<std::uint8_t> const secondTagBytes{0x52U, 0x4DU, 0x32U};
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  publisher.rtiAmbassador().deleteObjectInstance(secondObject, secondTag);
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending object-removal event");
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !receiver.recorder().hasRemoval(
          secondObject,
          secondTagBytes,
          publisher.federateHandle()),
      "disabled callbacks exposed an object-removal callback");

  receiver.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().hasRemoval(
            secondObject,
            secondTagBytes,
            publisher.federateHandle());
      },
      options,
      "re-enabled callback-control object removal");
  auto const removals = receiver.recorder().removals();
  require(!removals.empty(), "re-enabled object-removal callback was not recorded");
  auto const& removal = removals.back();
  require(
      removal.object == secondObject && removal.tag == secondTagBytes &&
          removal.producer == publisher.federateHandle(),
      "re-enabled object-removal callback lost standard delivery data");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsObjectRemovalContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsObjectRemoval(options, model);
}

void scenarioCallbackControlsObjectDiscovery(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "callback-controls-object-discovery");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherClass == receiverClass && publisherAttribute == receiverAttribute,
      "callback-control object-discovery handles did not retain identity across members");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  receiver.rtiAmbassador().disableCallbacks();
  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "callback-control object-discovery registration returned an invalid handle");
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending object-discovery event");
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !receiver.recorder().hasDiscovery(object),
      "disabled callbacks exposed an object-discovery callback");

  receiver.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "re-enabled callback-control object discovery");
  auto const discovery = receiver.recorder().discovery();
  require(
      discovery.present && discovery.object == object &&
          discovery.objectClass == receiverClass && !discovery.name.empty() &&
          discovery.producer == publisher.federateHandle(),
      "re-enabled object-discovery callback lost standard delivery data");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsObjectDiscoveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsObjectDiscovery(options, model);
}

void scenarioCallbackControlsObjectNameReservation(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  auto const federation = federationName(options, "callback-controls-object-name-reservation");
  owner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const objectName = federation + L"-callback-controlled-name";
  owner.recorder().clearReservations();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  if (model == rti::HLA_EVOKED) {
    require(
        owner.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending object-name reservation event");
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !owner.recorder().reservationSucceeded(objectName),
      "disabled callbacks exposed an object-name reservation callback");

  owner.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(objectName); },
      options,
      "re-enabled callback-control object-name reservation");
  owner.rtiAmbassador().releaseObjectInstanceName(objectName);

  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
}

void scenarioCallbackControlsObjectNameReservationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsObjectNameReservation(options, model);
}

void scenarioCallbackControlsObjectNameReservationFailure(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-object-name-reservation-failure");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const objectName = federation + L"-callback-controlled-contended-name";
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(objectName); },
      options,
      "callback-control object-name reservation baseline");

  requester.recorder().clearReservations();
  requester.rtiAmbassador().disableCallbacks();
  requester.rtiAmbassador().reserveObjectInstanceName(objectName);
  if (model == rti::HLA_EVOKED) {
    require(
        requester.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending object-name reservation failure event");
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !requester.recorder().reservationFailed(objectName),
      "disabled callbacks exposed an object-name reservation failure callback");

  requester.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      requester,
      [&] { return requester.recorder().reservationFailed(objectName); },
      options,
      "re-enabled callback-control object-name reservation failure");

  owner.rtiAmbassador().releaseObjectInstanceName(objectName);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsObjectNameReservationFailureContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsObjectNameReservationFailure(options, model);
}

void scenarioCallbackControlsMultipleObjectNameReservation(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-multiple-object-name-reservation");
  connectAndJoin(owner, peer, options, federation, options.fom);

  auto const firstName = federation + L"-multiple-a";
  auto const secondName = federation + L"-multiple-b";
  auto const thirdName = federation + L"-multiple-c";
  std::set<std::wstring> const ownerNames{firstName, secondName};
  std::set<std::wstring> const peerNames{secondName, thirdName};
  std::set<std::wstring> const peerSucceeded{thirdName};
  std::set<std::wstring> const peerFailed{secondName};

  owner.recorder().clearReservations();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().reserveMultipleObjectInstanceNames(ownerNames);
  if (model == rti::HLA_EVOKED) {
    require(
        owner.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending multiple-reservation success event");
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !owner.recorder().multipleReservationSucceeded(ownerNames),
      "disabled callbacks exposed a multiple object-name reservation success callback");

  owner.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      owner,
      [&] { return owner.recorder().multipleReservationSucceeded(ownerNames); },
      options,
      "re-enabled multiple object-name reservation success callback");

  peer.recorder().clearReservations();
  peer.rtiAmbassador().disableCallbacks();
  peer.rtiAmbassador().reserveMultipleObjectInstanceNames(peerNames);
  if (model == rti::HLA_EVOKED) {
    require(
        peer.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending mixed multiple-reservation events");
  } else {
    static_cast<void>(peer.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !peer.recorder().multipleReservationSucceeded(peerSucceeded) &&
          !peer.recorder().multipleReservationFailed(peerFailed),
      "disabled callbacks exposed mixed multiple object-name reservation callbacks");

  peer.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(peer.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(peer.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      peer,
      [&] {
        return peer.recorder().multipleReservationSucceeded(peerSucceeded) &&
            peer.recorder().multipleReservationFailed(peerFailed);
      },
      options,
      "re-enabled mixed multiple object-name reservation callbacks");

  peer.rtiAmbassador().releaseObjectInstanceName(thirdName);
  owner.rtiAmbassador().releaseMultipleObjectInstanceNames(ownerNames);
  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsMultipleObjectNameReservationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsMultipleObjectNameReservation(options, model);
}

void scenarioCallbackControlsDeclarationAdvisories(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session subscriber(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-declaration-advisories");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      L"Name");
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      L"Name");
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const subscriberInteraction = subscriber.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid() &&
          publisherInteraction.isValid() && subscriberInteraction.isValid() &&
          publisherClass == subscriberClass &&
          publisherAttribute == subscriberAttribute &&
          publisherInteraction == subscriberInteraction,
      "callback-control declaration-advisory handles did not retain identity across members");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  publisher.recorder().clearAdvisories();
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"");
  publisher.rtiAmbassador().disableCallbacks();
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending start-registration advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().registrationStarts().empty(),
      "disabled callbacks exposed a start-registration advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().registrationStarts().size() == 1U; },
      options,
      "re-enabled start-registration advisory");
  require(
      publisher.recorder().registrationStarts().front() == publisherClass,
      "re-enabled start-registration advisory lost the object-class handle");

  publisher.rtiAmbassador().disableCallbacks();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending stop-registration advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().registrationStops().empty(),
      "disabled callbacks exposed a stop-registration advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().registrationStops().size() == 1U; },
      options,
      "re-enabled stop-registration advisory");
  require(
      publisher.recorder().registrationStops().front() == publisherClass,
      "re-enabled stop-registration advisory lost the object-class handle");

  publisher.recorder().clearAdvisories();
  publisher.rtiAmbassador().disableCallbacks();
  subscriber.rtiAmbassador().subscribeInteractionClass(
      subscriberInteraction,
      true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending interaction turn-on advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().interactionsOn().empty(),
      "disabled callbacks exposed an interaction turn-on advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().interactionsOn().size() == 1U; },
      options,
      "re-enabled interaction turn-on advisory");
  require(
      publisher.recorder().interactionsOn().front() == publisherInteraction,
      "re-enabled interaction turn-on advisory lost the interaction handle");

  publisher.rtiAmbassador().disableCallbacks();
  subscriber.rtiAmbassador().unsubscribeInteractionClass(subscriberInteraction);
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending interaction turn-off advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().interactionsOff().empty(),
      "disabled callbacks exposed an interaction turn-off advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().interactionsOff().size() == 1U; },
      options,
      "re-enabled interaction turn-off advisory");
  require(
      publisher.recorder().interactionsOff().front() == publisherInteraction,
      "re-enabled interaction turn-off advisory lost the interaction handle");

  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberClass);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsDeclarationAdvisoriesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsDeclarationAdvisories(options, model);
}

void scenarioCallbackControlsAttributeRelevanceAdvisories(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session subscriber(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-attribute-relevance-advisories");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherName = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      L"Name");
  auto const subscriberName = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      L"Name");
  auto const publisherPayRate = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      L"PayRate");
  auto const subscriberPayRate = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      L"PayRate");
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      options.attributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherName.isValid() && subscriberName.isValid() &&
          publisherPayRate.isValid() && subscriberPayRate.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid() &&
          publisherClass == subscriberClass &&
          publisherName == subscriberName &&
          publisherPayRate == subscriberPayRate &&
          publisherAttribute == subscriberAttribute,
      "callback-control attribute-relevance handles did not retain identity across members");

  rti::AttributeHandleSet const publisherNameSet{publisherName};
  rti::AttributeHandleSet const publisherPayRateSet{publisherPayRate};
  rti::AttributeHandleSet const publisherAttributeSet{publisherAttribute};
  rti::AttributeHandleSet const publisherAllSet{
      publisherName,
      publisherPayRate,
      publisherAttribute};
  rti::AttributeHandleSet const subscriberNameSet{subscriberName};
  rti::AttributeHandleSet const subscriberPayRateSet{subscriberPayRate};
  rti::AttributeHandleSet const subscriberAttributeSet{subscriberAttribute};

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberNameSet,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAllSet);
  waitFor(
      publisher,
      [&] { return publisher.recorder().registrationStarts().size() >= 1U; },
      options,
      "callback-control attribute-relevance registration advisory");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "callback-control attribute-relevance registration returned an invalid object handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "callback-control attribute-relevance discovery");
  // Registration may queue the already-active Name relevance advisory on an
  // evoked publisher. Cross the callback boundary before clearing the
  // recorder so the callback-control checks below start with no stale work.
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  publisher.recorder().clearAdvisories();

  publisher.rtiAmbassador().disableCallbacks();
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberPayRateSet,
      true,
      L"");
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending unnamed turn-updates-on advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().updatesOn().empty(),
      "disabled callbacks exposed an unnamed turn-updates-on advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().updatesOn().size() == 1U; },
      options,
      "re-enabled unnamed turn-updates-on advisory");
  auto const unnamedUpdatesOn = publisher.recorder().updatesOn();
  require(
      unnamedUpdatesOn.front().object == object &&
          unnamedUpdatesOn.front().attributes.size() == 1U &&
          unnamedUpdatesOn.front().attributes.count(publisherPayRate) == 1U &&
          !unnamedUpdatesOn.front().updateRateDesignator.has_value(),
      "re-enabled unnamed turn-updates-on advisory lost standard delivery data");

  publisher.rtiAmbassador().disableCallbacks();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberPayRateSet);
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending unnamed turn-updates-off advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().updatesOff().empty(),
      "disabled callbacks exposed an unnamed turn-updates-off advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().updatesOff().size() == 1U; },
      options,
      "re-enabled unnamed turn-updates-off advisory");
  auto const unnamedUpdatesOff = publisher.recorder().updatesOff();
  require(
      unnamedUpdatesOff.front().object == object &&
          unnamedUpdatesOff.front().attributes.size() == 1U &&
          unnamedUpdatesOff.front().attributes.count(publisherPayRate) == 1U &&
          !unnamedUpdatesOff.front().updateRateDesignator.has_value(),
      "re-enabled unnamed turn-updates-off advisory lost standard delivery data");

  publisher.recorder().clearAdvisories();
  publisher.rtiAmbassador().disableCallbacks();
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributeSet,
      true,
      L"High");
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending named turn-updates-on advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().updatesOn().empty(),
      "disabled callbacks exposed a named turn-updates-on advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().updatesOn().size() == 1U; },
      options,
      "re-enabled named turn-updates-on advisory");
  auto const namedUpdatesOn = publisher.recorder().updatesOn();
  require(
      namedUpdatesOn.front().object == object &&
          namedUpdatesOn.front().attributes.size() == 1U &&
          namedUpdatesOn.front().attributes.count(publisherAttribute) == 1U &&
          namedUpdatesOn.front().updateRateDesignator.has_value() &&
          namedUpdatesOn.front().updateRateDesignator.value() == L"High",
      "re-enabled named turn-updates-on advisory lost the update-rate designator");

  publisher.rtiAmbassador().disableCallbacks();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributeSet);
  if (model == rti::HLA_EVOKED) {
    require(
        publisher.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending named turn-updates-off advisory");
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      publisher.recorder().updatesOff().empty(),
      "disabled callbacks exposed a named turn-updates-off advisory");

  publisher.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(publisher.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(publisher.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      publisher,
      [&] { return publisher.recorder().updatesOff().size() == 1U; },
      options,
      "re-enabled named turn-updates-off advisory");
  auto const namedUpdatesOff = publisher.recorder().updatesOff();
  require(
      namedUpdatesOff.front().object == object &&
          namedUpdatesOff.front().attributes.size() == 1U &&
          namedUpdatesOff.front().attributes.count(publisherAttribute) == 1U &&
          !namedUpdatesOff.front().updateRateDesignator.has_value(),
      "re-enabled named turn-updates-off advisory lost standard delivery data");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberNameSet);
  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberClass);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAllSet);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsAttributeRelevanceAdvisoriesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsAttributeRelevanceAdvisories(options, model);
}

void scenarioCallbackControlsAttributeValueRequest(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "callback-controls-attribute-value-request");
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
          ownerAttribute.isValid() && requesterAttribute.isValid() &&
          ownerClass == requesterClass && ownerAttribute == requesterAttribute,
      "callback-control attribute-value-request handles did not retain identity across members");

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
      "callback-control attribute-value-request registration returned an invalid handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control attribute-value-request discovery");

  owner.recorder().clearProvidedUpdates();
  owner.rtiAmbassador().disableCallbacks();
  std::vector<std::uint8_t> const requestTagBytes{0x56U, 0x52U, 0x31U};
  rti::VariableLengthData requestTag(requestTagBytes.data(), requestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdate(
      object,
      requesterAttributes,
      requestTag);
  if (model == rti::HLA_EVOKED) {
    require(
        owner.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending attribute-value request event");
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      owner.recorder().providedUpdates().empty(),
      "disabled callbacks exposed a provideAttributeValueUpdate callback");

  owner.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "re-enabled callback-control attribute-value request");
  auto const requests = owner.recorder().providedUpdates();
  require(
      requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          requests.front().tag == requestTagBytes,
      "re-enabled provideAttributeValueUpdate callback lost standard delivery data");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsAttributeValueRequestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsAttributeValueRequest(options, model);
}

void scenarioCallbackControlsAutoProvide(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "callback-control Auto Provide testing requires an adapter-supplied Auto Provide FOM");
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "callback-controls-auto-provide");
  connectAndJoin(owner, requester, options, federation, options.autoProvideFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideFirstAttributeName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideSecondAttributeName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideFirstAttributeName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideSecondAttributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() && ownerFirst.isValid() &&
          ownerSecond.isValid() && requesterFirst.isValid() && requesterSecond.isValid() &&
          ownerClass == requesterClass,
      "callback-control Auto Provide lookup returned invalid or inconsistent handles");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          requester.rtiAmbassador().getAutoProvideSwitch(),
      "callback-control Auto Provide FOM did not enable the standard switch");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const requesterAttributes{requesterFirst, requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  owner.recorder().clearProvidedUpdates();
  owner.rtiAmbassador().disableCallbacks();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control Auto Provide registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control Auto Provide discovery");

  if (model == rti::HLA_EVOKED) {
    require(
        owner.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending Auto Provide callback");
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.autoProvideObjectClassName));
  }
  require(
      owner.recorder().providedUpdates().empty(),
      "disabled callbacks exposed a provideAttributeValueUpdate callback");

  owner.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.autoProvideObjectClassName));
  }
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "re-enabled callback-control Auto Provide callback");
  auto const providedUpdates = owner.recorder().providedUpdates();
  require(
      providedUpdates.front().object == object &&
          providedUpdates.front().attributes == ownerAttributes &&
          providedUpdates.front().tag.empty(),
      "re-enabled Auto Provide callback lost standard object, attribute, or tag data");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsAutoProvideContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsAutoProvide(options, model);
}

void scenarioCallbackControlsOwnershipAssumption(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "callback-controls-ownership-assumption");
  connectAndJoin(owner, peer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const peerClass = peer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const peerAttribute = peer.rtiAmbassador().getAttributeHandle(
      peerClass,
      options.attributeName);
  auto const privilegeToDelete = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      L"HLAprivilegeToDeleteObject");
  require(
      ownerClass.isValid() && peerClass.isValid() && ownerAttribute.isValid() &&
          peerAttribute.isValid() && privilegeToDelete.isValid() &&
          ownerClass == peerClass,
      "callback-control ownership-assumption lookup returned invalid or inconsistent handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const peerAttributes{peerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  peer.rtiAmbassador().publishObjectClassAttributes(peerClass, peerAttributes);
  peer.rtiAmbassador().subscribeObjectClassAttributes(
      peerClass,
      peerAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control ownership-assumption registration returned an invalid object handle");
  waitFor(
      peer,
      [&] { return peer.recorder().hasDiscovery(object); },
      options,
      "callback-control ownership-assumption discovery");

  peer.recorder().clearOwnershipRecords();
  peer.rtiAmbassador().disableCallbacks();
  owner.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  if (model == rti::HLA_EVOKED) {
    require(
        peer.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending ownership-assumption event");
  } else {
    static_cast<void>(peer.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !peer.recorder().ownershipAssumption().has_value(),
      "disabled callbacks exposed an ownership-assumption callback");

  peer.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(peer.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(peer.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      peer,
      [&] { return peer.recorder().ownershipAssumption().has_value(); },
      options,
      "re-enabled callback-control ownership-assumption callback");
  auto const assumption = peer.recorder().ownershipAssumption();
  require(
      assumption->object == object && assumption->attributes.count(peerAttribute) == 1U &&
          assumption->tag.empty(),
      "re-enabled ownership-assumption callback lost standard object, attribute, or tag data");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x41U, 0x53U, 0x53U};
  rti::VariableLengthData acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  peer.recorder().clearOwnershipRecords();
  peer.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      peerAttributes,
      acquisitionTag);
  waitFor(
      peer,
      [&] { return peer.recorder().ownershipAcquisition().has_value(); },
      options,
      "callback-control ownership acquisition notification");
  auto const acquisition = peer.recorder().ownershipAcquisition();
  require(
      acquisition->object == object && acquisition->attributes == peerAttributes &&
          acquisition->tag == acquisitionTagBytes &&
          peer.rtiAmbassador().isAttributeOwnedByFederate(object, peerAttribute),
      "ownership acquisition after callback release returned the wrong standard state");

  peer.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  peer.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  peer.disconnect();
}

void scenarioCallbackControlsOwnershipAssumptionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipAssumption(options, model);
}

void scenarioCallbackControlsOwnershipUnavailable(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "callback-controls-ownership-unavailable");
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
      ownerClass.isValid() && requesterClass.isValid() && ownerAttribute.isValid() &&
          requesterAttribute.isValid() && ownerClass == requesterClass,
      "callback-control ownership-unavailable lookup returned invalid or inconsistent handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control ownership-unavailable registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control ownership-unavailable discovery");

  std::vector<std::uint8_t> const requestTagBytes{0x55U, 0x4EU, 0x41U};
  rti::VariableLengthData requestTag(requestTagBytes.data(), requestTagBytes.size());
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().disableCallbacks();
  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      requesterAttributes,
      requestTag);
  if (model == rti::HLA_EVOKED) {
    require(
        requester.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending ownership-unavailable event");
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      requester.recorder().ownershipUnavailableEvents().empty(),
      "disabled callbacks exposed an ownership-unavailable callback");

  requester.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipUnavailableEvents().size() == 1U; },
      options,
      "re-enabled callback-control ownership-unavailable callback");
  auto const unavailable = requester.recorder().ownershipUnavailableEvents();
  require(
      unavailable.front().object == object &&
          unavailable.front().attributes == requesterAttributes &&
          unavailable.front().tag == requestTagBytes &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "re-enabled ownership-unavailable callback returned the wrong standard state");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsOwnershipUnavailableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipUnavailable(options, model);
}

void scenarioCallbackControlsOwnershipReleaseRequest(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "callback-controls-ownership-release-request");
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
      ownerClass.isValid() && requesterClass.isValid() && ownerAttribute.isValid() &&
          requesterAttribute.isValid() && ownerClass == requesterClass,
      "callback-control ownership-release lookup returned invalid or inconsistent handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control ownership-release registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control ownership-release discovery");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x52U, 0x45U, 0x51U};
  rti::VariableLengthData acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().disableCallbacks();
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  if (model == rti::HLA_EVOKED) {
    require(
        owner.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending ownership-release request event");
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !owner.recorder().ownershipReleaseRequest().has_value(),
      "disabled callbacks exposed an ownership-release request callback");

  owner.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "re-enabled callback-control ownership-release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object && releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "re-enabled ownership-release request lost standard object, attribute, or tag data");

  std::vector<std::uint8_t> const divestitureTagBytes{0x44U, 0x49U, 0x56U};
  rti::VariableLengthData divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  requester.recorder().clearOwnershipRecords();
  rti::AttributeHandleSet divestedAttributes;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      divestitureTag,
      divestedAttributes);
  require(
      divestedAttributes == ownerAttributes,
      "ownership-release request did not divest the requested attribute set");
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "callback-control ownership-release acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(
      acquisition->object == object && acquisition->attributes == requesterAttributes &&
          acquisition->tag == divestitureTagBytes &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "ownership-release handoff returned the wrong standard acquisition state");

  requester.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsOwnershipReleaseRequestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipReleaseRequest(options, model);
}

void scenarioCallbackControlsOwnershipAcquisitionCancellation(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-ownership-acquisition-cancellation");
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
      ownerClass.isValid() && requesterClass.isValid() && ownerAttribute.isValid() &&
          requesterAttribute.isValid() && ownerClass == requesterClass,
      "callback-control ownership-cancellation lookup returned invalid or inconsistent handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control ownership-cancellation registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control ownership-cancellation discovery");

  std::vector<std::uint8_t> const requestTagBytes{0x43U, 0x41U, 0x4EU};
  rti::VariableLengthData requestTag(requestTagBytes.data(), requestTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "callback-control ownership-cancellation release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object && releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == requestTagBytes,
      "callback-control ownership-cancellation release request lost standard data");

  requester.rtiAmbassador().disableCallbacks();
  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
      object,
      requesterAttributes);
  if (model == rti::HLA_EVOKED) {
    require(
        requester.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending ownership-cancellation event");
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !requester.recorder().ownershipAcquisitionCancellation().has_value(),
      "disabled callbacks exposed an ownership-acquisition cancellation callback");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "canceled ownership acquisition changed the standard ownership state");

  requester.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisitionCancellation().has_value(); },
      options,
      "re-enabled callback-control ownership-acquisition cancellation");
  auto const cancellation = requester.recorder().ownershipAcquisitionCancellation();
  require(
      cancellation->object == object && cancellation->attributes == requesterAttributes &&
          cancellation->tag.empty() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "re-enabled ownership-acquisition cancellation returned the wrong standard state");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsOwnershipAcquisitionCancellationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipAcquisitionCancellation(options, model);
}

void scenarioCallbackControlsOwnershipReleaseDenied(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-ownership-release-denied");
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
      ownerClass.isValid() && requesterClass.isValid() && ownerAttribute.isValid() &&
          requesterAttribute.isValid() && ownerClass == requesterClass,
      "callback-control ownership-denied lookup returned invalid or inconsistent handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control ownership-denied registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control ownership-denied discovery");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x44U, 0x45U, 0x4EU};
  std::vector<std::uint8_t> const denialTagBytes{0x44U, 0x45U, 0x4EU, 0x59U};
  rti::VariableLengthData acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData denialTag(denialTagBytes.data(), denialTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "callback-control ownership-denied release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object && releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "callback-control ownership-denied release request lost standard data");

  requester.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().attributeOwnershipReleaseDenied(
      object,
      ownerAttributes,
      denialTag);
  if (model == rti::HLA_EVOKED) {
    require(
        requester.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending ownership-denied event");
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      requester.recorder().ownershipUnavailableEvents().empty(),
      "disabled callbacks exposed an ownership-unavailable denial callback");

  requester.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipUnavailableEvents().size() == 1U; },
      options,
      "re-enabled callback-control ownership-denied notification");
  auto const unavailable = requester.recorder().ownershipUnavailableEvents();
  require(
      unavailable.front().object == object &&
          unavailable.front().attributes == requesterAttributes &&
          unavailable.front().tag == denialTagBytes &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "re-enabled ownership-denied callback returned the wrong standard state");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsOwnershipReleaseDeniedContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipReleaseDenied(options, model);
}

void scenarioCallbackControlsOwnershipAcquisitionNotification(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-ownership-acquisition-notification");
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
      ownerClass.isValid() && requesterClass.isValid() && ownerAttribute.isValid() &&
          requesterAttribute.isValid() && ownerClass == requesterClass,
      "callback-control ownership-notification lookup returned invalid or inconsistent handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control ownership-notification registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control ownership-notification discovery");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x4EU, 0x4FU, 0x54U};
  std::vector<std::uint8_t> const divestitureTagBytes{0x47U, 0x41U, 0x49U, 0x4EU};
  rti::VariableLengthData acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "callback-control ownership-notification release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object && releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "callback-control ownership-notification release request lost standard data");

  requester.rtiAmbassador().disableCallbacks();
  rti::AttributeHandleSet divestedAttributes;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      divestitureTag,
      divestedAttributes);
  require(
      divestedAttributes == ownerAttributes,
      "callback-control ownership-notification did not divest the requested attributes");
  if (model == rti::HLA_EVOKED) {
    require(
        requester.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending ownership-notification event");
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !requester.recorder().ownershipAcquisition().has_value(),
      "disabled callbacks exposed an ownership-acquisition notification");

  requester.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "re-enabled callback-control ownership-acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(
      acquisition->object == object && acquisition->attributes == requesterAttributes &&
          acquisition->tag == divestitureTagBytes &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "re-enabled ownership-acquisition notification returned the wrong standard state");

  requester.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsOwnershipAcquisitionNotificationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipAcquisitionNotification(options, model);
}

void scenarioCallbackControlsOwnershipDivestitureConfirmation(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-ownership-divestiture-confirmation");
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
      ownerClass.isValid() && requesterClass.isValid() && ownerAttribute.isValid() &&
          requesterAttribute.isValid() && ownerClass == requesterClass,
      "callback-control divestiture-confirmation lookup returned invalid or inconsistent handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control divestiture-confirmation registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control divestiture-confirmation discovery");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x4EU, 0x45U, 0x47U};
  std::vector<std::uint8_t> const divestitureTagBytes{0x44U, 0x49U, 0x56U};
  rti::VariableLengthData acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);

  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      divestitureTag);
  if (model == rti::HLA_EVOKED) {
    require(
        owner.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending divestiture-confirmation event");
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      !owner.recorder().divestitureConfirmation().has_value(),
      "disabled callbacks exposed a divestiture-confirmation callback");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "negotiated divestiture changed ownership while its callback was disabled");

  owner.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      owner,
      [&] { return owner.recorder().divestitureConfirmation().has_value(); },
      options,
      "re-enabled callback-control divestiture confirmation");
  auto const confirmation = owner.recorder().divestitureConfirmation();
  require(
      confirmation->object == object && confirmation->attributes == ownerAttributes &&
          confirmation->tag == acquisitionTagBytes &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "re-enabled divestiture confirmation returned the wrong standard state");

  owner.rtiAmbassador().cancelNegotiatedAttributeOwnershipDivestiture(
      object,
      ownerAttributes);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "ordinary release request after callback-control divestiture cancellation");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object && releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "callback-control divestiture cancellation restored the wrong release request");
  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
      object,
      requesterAttributes);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisitionCancellation().has_value(); },
      options,
      "callback-control divestiture requester cancellation");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsOwnershipDivestitureConfirmationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipDivestitureConfirmation(options, model);
}

void scenarioCallbackControlsOwnershipQuery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Callback-control ownership-query testing requires an adapter-supplied multi-attribute FOM");
  require(
      !options.mimFom.empty(),
      "Callback-control ownership-query testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control ownership-query testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-ownership-query");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.multiAttributeFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-callback-controls-ownership-query",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-callback-controls-ownership-query",
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
      ownerClass.isValid() && requesterClass.isValid() && ownerFirst.isValid() &&
          ownerSecond.isValid() && requesterFirst.isValid() && requesterSecond.isValid() &&
          ownerFirst != ownerSecond,
      "callback-control ownership-query lookup returned invalid or inconsistent handles");

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
      "callback-control ownership-query registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "callback-control ownership-query discovery");

  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().disableCallbacks();
  requester.rtiAmbassador().queryAttributeOwnership(object, requesterAttributes);
  if (model == rti::HLA_EVOKED) {
    require(
        requester.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending ownership-query callbacks");
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.multiAttributeObjectClassName));
  }
  require(
      requester.recorder().ownershipInformationEvents().empty() &&
          requester.recorder().ownershipNotOwnedEvents().empty() &&
          requester.recorder().ownershipOwnedByRtiEvents().empty(),
      "disabled callbacks exposed ownership-query callbacks");

  requester.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.multiAttributeObjectClassName));
  }
  waitFor(
      requester,
      [&] {
        return requester.recorder().ownershipInformationEvents().size() == 1U &&
            requester.recorder().ownershipNotOwnedEvents().size() == 1U;
      },
      options,
      "re-enabled callback-control ownership-query partition");
  auto const ownershipInformation = requester.recorder().ownershipInformationEvents();
  auto const ownershipNotOwned = requester.recorder().ownershipNotOwnedEvents();
  require(
      ownershipInformation.front().object == object &&
          ownershipInformation.front().attributes == rti::AttributeHandleSet{requesterFirst} &&
          ownershipInformation.front().owner == owner.federateHandle(),
      "re-enabled ownership query did not report the exact owned subset");
  require(
      ownershipNotOwned.front().object == object &&
          ownershipNotOwned.front().attributes == rti::AttributeHandleSet{requesterSecond},
      "re-enabled ownership query did not report the exact unowned subset");

  // Application attributes relinquished by a federate are unowned; they are
  // not RTI-owned. Exercise the distinct attributeIsOwnedByRTI callback on a
  // joined-federate MOM object, whose standard ownership is held by the RTI.
  requester.rtiAmbassador().setServiceReportingSwitch(false);
  auto const requesterFederateClass = requester.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const requesterFederateHandleAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterFederateClass,
      L"HLAfederateHandle");
  require(
      requesterFederateClass.isValid() && requesterFederateHandleAttribute.isValid(),
      "callback-control ownership-query MOM lookup returned invalid handles");
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterFederateClass,
      rti::AttributeHandleSet{requesterFederateHandleAttribute},
      true,
      L"");
  auto findRequesterMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = requester.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(requester.federateHandle().encode());
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
  waitFor(
      requester,
      [&] { return findRequesterMomReflection().has_value(); },
      options,
      "callback-control ownership-query MOM discovery");
  auto const requesterMomReflection = findRequesterMomReflection();
  require(
      requesterMomReflection.has_value() &&
          requester.rtiAmbassador().getKnownObjectClassHandle(
              requesterMomReflection->object) == requesterFederateClass,
      "callback-control ownership-query MOM discovery returned the wrong object");
  auto const requesterMomObject = requesterMomReflection->object;

  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().disableCallbacks();
  requester.rtiAmbassador().queryAttributeOwnership(
      requesterMomObject,
      rti::AttributeHandleSet{requesterFederateHandleAttribute});
  if (model == rti::HLA_EVOKED) {
    require(
        requester.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending MOM ownership-query callback");
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.multiAttributeObjectClassName));
  }
  require(
      requester.recorder().ownershipInformationEvents().empty() &&
          requester.recorder().ownershipNotOwnedEvents().empty() &&
          requester.recorder().ownershipOwnedByRtiEvents().empty(),
      "disabled callbacks exposed the MOM RTI-owned query callback");

  requester.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(requester.rtiAmbassador().getObjectClassHandle(
        options.multiAttributeObjectClassName));
  }
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipOwnedByRtiEvents().size() == 1U; },
      options,
      "re-enabled callback-control MOM RTI-owned query");
  auto const ownershipOwnedByRti = requester.recorder().ownershipOwnedByRtiEvents();
  require(
      ownershipOwnedByRti.front().object == requesterMomObject &&
          ownershipOwnedByRti.front().attributes ==
              rti::AttributeHandleSet{requesterFederateHandleAttribute} &&
          requester.recorder().ownershipInformationEvents().empty() &&
          requester.recorder().ownershipNotOwnedEvents().empty(),
      "re-enabled MOM ownership query did not report the exact RTI-owned subset");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsOwnershipQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsOwnershipQuery(options, model);
}

void scenarioCallbackControlsSynchronization(
    Options const& options,
    rti::CallbackModel model) {
  Session registrar(options, model, "registrar");
  Session member(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-synchronization");
  connectAndJoin(registrar, member, options, federation, options.fom);

  std::vector<std::uint8_t> const tagBytes{0x53U, 0x59U, 0x4EU, 0x43U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  std::wstring const label = L"tck-callback-controls-synchronization";

  // The announcement is generated while the member's callback gate is
  // closed. Servicing the underlying transport must not enter the standard
  // FederateAmbassador callback until enableCallbacks is called.
  member.rtiAmbassador().disableCallbacks();
  registrar.rtiAmbassador().registerFederationSynchronizationPoint(label, tag);
  waitFor(
      registrar,
      [&] {
        return registrar.synchronizationPointRegistrations().size() == 1U &&
            registrar.synchronizationPointAnnouncements().size() == 1U;
      },
      options,
      "callback-control synchronization registration and announcement");
  auto const registrarRegistration = registrar.synchronizationPointRegistrations().front();
  require(
      registrarRegistration.succeeded && registrarRegistration.label == label,
      "callback-control synchronization registration returned the wrong result");
  auto const registrarAnnouncements = registrar.synchronizationPointAnnouncements();
  require(
      registrarAnnouncements.front().label == label &&
          registrarAnnouncements.front().tag == tagBytes,
      "callback-control synchronization announcement returned the wrong metadata");

  if (model == rti::HLA_EVOKED) {
    require(
        member.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending synchronization announcement");
  } else {
    static_cast<void>(member.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      member.synchronizationPointAnnouncements().empty(),
      "disabled callbacks exposed a synchronization announcement");

  member.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(member.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(member.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      member,
      [&] { return member.synchronizationPointAnnouncements().size() == 1U; },
      options,
      "re-enabled callback-control synchronization announcement");
  auto const memberAnnouncements = member.synchronizationPointAnnouncements();
  require(
      memberAnnouncements.front().label == label &&
          memberAnnouncements.front().tag == tagBytes,
      "re-enabled synchronization announcement returned the wrong metadata");

  // Both members achieve the announced point while the member's completion
  // callback gate is closed. The registrar may complete independently, but
  // the member must not enter FederationSynchronized until re-enabled.
  member.rtiAmbassador().disableCallbacks();
  registrar.rtiAmbassador().synchronizationPointAchieved(label, true);
  member.rtiAmbassador().synchronizationPointAchieved(label, true);
  waitFor(
      registrar,
      [&] { return registrar.federationSynchronized().size() == 1U; },
      options,
      "callback-control synchronization completion for registrar");
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(member.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(member.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      member.federationSynchronized().empty(),
      "disabled callbacks exposed a federation-synchronized callback");

  member.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(member.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(member.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      member,
      [&] { return member.federationSynchronized().size() == 1U; },
      options,
      "re-enabled callback-control synchronization completion");
  auto const registrarCompletions = registrar.federationSynchronized();
  auto const memberCompletions = member.federationSynchronized();
  require(
      registrarCompletions.front().label == label &&
          registrarCompletions.front().failedToSyncSet.empty() &&
          memberCompletions.front().label == label &&
          memberCompletions.front().failedToSyncSet.empty(),
      "re-enabled synchronization completion returned the wrong metadata");

  member.resign(rti::NO_ACTION);
  registrar.resign(rti::NO_ACTION);
  registrar.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  registrar.disconnect();
}

void scenarioCallbackControlsSynchronizationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsSynchronization(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
