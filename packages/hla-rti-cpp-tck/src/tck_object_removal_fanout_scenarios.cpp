#include "tck_object_removal_fanout_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioObjectRemovalMultiRecipientFifo(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "ordinary-removal-fanout-publisher");
  Session first(options, model, "ordinary-removal-fanout-first");
  Session second(options, model, "ordinary-removal-fanout-second");
  auto const federation = federationName(
      options,
      "ordinary-object-removal-multi-recipient-fifo");
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
      "ordinary object-removal multi-recipient FIFO lookup returned an invalid handle");

  rti::AttributeHandleSet publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet firstAttributes{firstAttribute};
  rti::AttributeHandleSet secondAttributes{secondAttribute};
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

  auto const firstObject = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  auto const secondObject = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "ordinary object-removal multi-recipient FIFO registration returned an invalid object");
  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasDiscovery(firstObject) &&
            first.recorder().hasDiscovery(secondObject) &&
            second.recorder().hasDiscovery(firstObject) &&
            second.recorder().hasDiscovery(secondObject);
      },
      options,
      "ordinary object-removal multi-recipient FIFO discovery");
  first.recorder().clearRemovals();
  second.recorder().clearRemovals();
  publisher.recorder().clearRemovals();

  std::vector<std::uint8_t> const firstRemovalTagBytes{
      0xD1U,
      0xE2U,
      0xF3U};
  std::vector<std::uint8_t> const secondRemovalTagBytes{
      0xA4U,
      0xB5U,
      0xC6U};
  rti::VariableLengthData firstRemovalTag(
      firstRemovalTagBytes.data(),
      firstRemovalTagBytes.size());
  rti::VariableLengthData secondRemovalTag(
      secondRemovalTagBytes.data(),
      secondRemovalTagBytes.size());
  publisher.rtiAmbassador().deleteObjectInstance(firstObject, firstRemovalTag);
  publisher.rtiAmbassador().deleteObjectInstance(secondObject, secondRemovalTag);

  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasRemoval(
                   firstObject,
                   firstRemovalTagBytes,
                   publisher.federateHandle()) &&
            first.recorder().hasRemoval(
                secondObject,
                secondRemovalTagBytes,
                publisher.federateHandle()) &&
            second.recorder().hasRemoval(
                firstObject,
                firstRemovalTagBytes,
                publisher.federateHandle()) &&
            second.recorder().hasRemoval(
                secondObject,
                secondRemovalTagBytes,
                publisher.federateHandle());
      },
      options,
      "ordinary object-removal multi-recipient FIFO delivery");
  require(
      publisher.recorder().removals().empty(),
      "ordinary object-removal multi-recipient FIFO delivered a removal back to its owner");

  auto assertRemoval = [&](Session& receiver, std::string const& description) {
    auto const received = receiver.recorder().removals();
    require(
        received.size() == 2U,
        description + " returned an unexpected callback count");
    require(
        received.front().object == firstObject &&
            received.front().tag == firstRemovalTagBytes &&
            received.front().producer == publisher.federateHandle(),
        description + " returned the wrong first removal metadata");
    require(
        received.back().object == secondObject &&
            received.back().tag == secondRemovalTagBytes &&
            received.back().producer == publisher.federateHandle(),
        description + " returned the wrong second removal metadata");
  };
  assertRemoval(first, "first ordinary object-removal recipient");
  assertRemoval(second, "second ordinary object-removal recipient");

  first.rtiAmbassador().unsubscribeObjectClassAttributes(
      firstClass,
      firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(
      secondClass,
      secondAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  second.resign(rti::NO_ACTION);
  first.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderObjectRemovalSubscriptionWithdrawal(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "receive-order-removal-publisher");
  Session active(options, model, "receive-order-removal-active");
  Session withdrawn(options, model, "receive-order-removal-withdrawn");
  auto const federation = federationName(
      options,
      "receive-order-object-removal-subscription-withdrawal");
  connectAndJoin(publisher, active, options, federation, options.fom);
  withdrawn.connect();
  withdrawn.join(
      options.memberFederateName + L"-withdrawn",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const activeClass = active.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const withdrawnClass = withdrawn.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const activeAttribute = active.rtiAmbassador().getAttributeHandle(
      activeClass,
      options.attributeName);
  auto const withdrawnAttribute = withdrawn.rtiAmbassador().getAttributeHandle(
      withdrawnClass,
      options.attributeName);
  require(
      publisherClass.isValid() && activeClass.isValid() && withdrawnClass.isValid() &&
          publisherAttribute.isValid() && activeAttribute.isValid() &&
          withdrawnAttribute.isValid(),
      "receive-order object-removal subscription-withdrawal lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const activeAttributes{activeAttribute};
  rti::AttributeHandleSet const withdrawnAttributes{withdrawnAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  active.rtiAmbassador().subscribeObjectClassAttributes(
      activeClass,
      activeAttributes,
      true,
      L"");
  withdrawn.rtiAmbassador().subscribeObjectClassAttributes(
      withdrawnClass,
      withdrawnAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "receive-order object-removal subscription-withdrawal registration returned an invalid object");
  waitForSessions(
      {&active, &withdrawn},
      [&] {
        return active.recorder().hasDiscovery(object) &&
            withdrawn.recorder().hasDiscovery(object);
      },
      options,
      "receive-order object-removal subscription-withdrawal discovery");
  publisher.recorder().clearRemovals();
  active.recorder().clearRemovals();
  withdrawn.recorder().clearRemovals();

  std::vector<std::uint8_t> const tagBytes{0x4FU, 0x42, 0x4AU};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().deleteObjectInstance(object, tag);

  if (model == rti::HLA_EVOKED) {
    require(
        active.recorder().removals().empty() &&
            withdrawn.recorder().removals().empty(),
        "evoked object removal arrived before callback servicing");
    withdrawn.rtiAmbassador().unsubscribeObjectClassAttributes(
        withdrawnClass,
        withdrawnAttributes);
    waitFor(
        active,
        [&] {
          return active.recorder().hasRemoval(
              object,
              tagBytes,
              publisher.federateHandle());
        },
        options,
        "receive-order object-removal subscription-withdrawal active removal");
    waitFor(
        withdrawn,
        [&] {
          return withdrawn.recorder().hasRemoval(
              object,
              tagBytes,
              publisher.federateHandle());
        },
        options,
        "receive-order object-removal subscription-withdrawal withdrawn removal");
  } else {
    waitFor(
        active,
        [&] {
          return active.recorder().hasRemoval(
              object,
              tagBytes,
              publisher.federateHandle());
        },
        options,
        "immediate object-removal subscription-withdrawal active removal");
    waitFor(
        withdrawn,
        [&] {
          return withdrawn.recorder().hasRemoval(
              object,
              tagBytes,
              publisher.federateHandle());
        },
        options,
        "immediate object-removal subscription-withdrawal withdrawn removal");
    withdrawn.rtiAmbassador().unsubscribeObjectClassAttributes(
        withdrawnClass,
        withdrawnAttributes);
  }

  auto requireRemoval = [&](Session& receiver, std::string const& description) {
    auto const removals = receiver.recorder().removals();
    require(
        removals.size() == 1U &&
            removals.front().object == object &&
            removals.front().tag == tagBytes &&
            removals.front().producer == publisher.federateHandle(),
        description + " returned the wrong removal metadata");
  };
  requireRemoval(active, "active object-removal recipient");
  requireRemoval(withdrawn, "subscription-withdrawal object-removal recipient");
  require(
      publisher.recorder().removals().empty(),
      "object-removal subscription-withdrawal delivered a removal back to its owner");

  active.rtiAmbassador().unsubscribeObjectClassAttributes(
      activeClass,
      activeAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  withdrawn.resign(rti::NO_ACTION);
  active.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  withdrawn.disconnect();
  active.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderObjectRemovalSubscriptionWithdrawalContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderObjectRemovalSubscriptionWithdrawal(options, model);
}

void scenarioResignDeleteObjectsMultiRecipientFifo(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "resign-removal-fanout-publisher");
  Session first(options, model, "resign-removal-fanout-first");
  Session second(options, model, "resign-removal-fanout-second");
  auto const federation = federationName(
      options,
      "ordinary-resign-delete-objects-multi-recipient-fifo");
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
      "ordinary resign-delete multi-recipient FIFO lookup returned an invalid handle");

  rti::AttributeHandleSet publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet firstAttributes{firstAttribute};
  rti::AttributeHandleSet secondAttributes{secondAttribute};
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

  auto const firstObject = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  auto const secondObject = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "ordinary resign-delete multi-recipient FIFO registration returned an invalid object");
  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasDiscovery(firstObject) &&
            first.recorder().hasDiscovery(secondObject) &&
            second.recorder().hasDiscovery(firstObject) &&
            second.recorder().hasDiscovery(secondObject);
      },
      options,
      "ordinary resign-delete multi-recipient FIFO discovery");
  first.recorder().clearRemovals();
  second.recorder().clearRemovals();
  publisher.recorder().clearRemovals();

  publisher.resign(rti::DELETE_OBJECTS);
  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasRemoval(
                   firstObject,
                   {},
                   publisher.federateHandle()) &&
            first.recorder().hasRemoval(
                secondObject,
                {},
                publisher.federateHandle()) &&
            second.recorder().hasRemoval(
                firstObject,
                {},
                publisher.federateHandle()) &&
            second.recorder().hasRemoval(
                secondObject,
                {},
                publisher.federateHandle());
      },
      options,
      "ordinary resign-delete multi-recipient FIFO delivery");
  require(
      publisher.recorder().removals().empty(),
      "ordinary resign-delete multi-recipient FIFO delivered a removal back to its owner");

  auto assertRemoval = [&](Session& receiver, std::string const& description) {
    auto const received = receiver.recorder().removals();
    require(
        received.size() == 2U,
        description + " returned an unexpected callback count");
    require(
        received.front().object == firstObject &&
            received.front().tag.empty() &&
            received.front().producer == publisher.federateHandle(),
        description + " returned the wrong first removal metadata");
    require(
        received.back().object == secondObject &&
            received.back().tag.empty() &&
            received.back().producer == publisher.federateHandle(),
        description + " returned the wrong second removal metadata");
  };
  assertRemoval(first, "first ordinary resign-delete recipient");
  assertRemoval(second, "second ordinary resign-delete recipient");

  first.rtiAmbassador().unsubscribeObjectClassAttributes(
      firstClass,
      firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(
      secondClass,
      secondAttributes);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  publisher.disconnect();
}

void scenarioObjectRemovalMultiRecipientFifoContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectRemovalMultiRecipientFifo(options, model);
}

void scenarioResignDeleteObjectsMultiRecipientFifoContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioResignDeleteObjectsMultiRecipientFifo(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
