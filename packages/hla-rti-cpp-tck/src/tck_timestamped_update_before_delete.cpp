#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedUpdateBeforeDelete(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "timestamped update-before-delete requires an adapter-supplied logical-time implementation");
  auto const scenarioFom = options.scenarioFoms.find(
      "timestamped-update-before-delete");
  require(
      scenarioFom != options.scenarioFoms.end(),
      "timestamped update-before-delete requires its timestamp-preferred attribute FOM");

  Session owner(options, model, "timestamped-update-before-delete-owner");
  Session receiver(options, model, "timestamped-update-before-delete-receiver");
  auto const federation = federationName(
      options,
      "timestamped-update-before-delete");
  connectAndJoin(owner, receiver, options, federation, scenarioFom->second);

  auto& ownerAmbassador = owner.rtiAmbassador();
  auto& receiverAmbassador = receiver.rtiAmbassador();
  auto const ownerClass = ownerAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiverAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = ownerAmbassador.getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const receiverAttribute = receiverAmbassador.getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      ownerClass.isValid() && receiverClass == ownerClass &&
          ownerAttribute.isValid() && receiverAttribute == ownerAttribute,
      "timestamped update-before-delete FOM handles differ across federates");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  ownerAmbassador.publishObjectClassAttributes(ownerClass, ownerAttributes);
  receiverAmbassador.subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  auto const object = ownerAmbassador.registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "timestamped update-before-delete registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped update-before-delete object discovery");

  auto ownerTime = makeTimeContext(owner);
  auto receiverTime = makeTimeContext(receiver);
  requireTimeEquals(
      *ownerTime.initial,
      *receiverTime.initial,
      "timestamped update-before-delete initial time");
  enableTimestampedRoles(
      owner,
      receiver,
      ownerTime,
      receiverTime,
      options,
      "timestamped update-before-delete");

  auto const updateTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  auto const deletionTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      6U);
  require(
      *updateTime <= *deletionTime,
      "timestamped update-before-delete setup violates update-time <= deletion-time");

  std::vector<std::uint8_t> const valueBytes{0x55U, 0x50U, 0x44U};
  rti::AttributeHandleValueMap const values{{
      ownerAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  std::vector<std::uint8_t> const updateTagBytes{0x55U, 0x50U, 0x44U};
  std::vector<std::uint8_t> const deletionTagBytes{0x44U, 0x45U, 0x4CU};
  rti::VariableLengthData const updateTag(
      updateTagBytes.data(),
      updateTagBytes.size());
  rti::VariableLengthData const deletionTag(
      deletionTagBytes.data(),
      deletionTagBytes.size());
  auto const updateRetraction = ownerAmbassador.updateAttributeValues(
      object,
      values,
      updateTag,
      *updateTime);
  auto const deletionRetraction = ownerAmbassador.deleteObjectInstance(
      object,
      deletionTag,
      *deletionTime);
  require(
      updateRetraction.isValid() && deletionRetraction.isValid(),
      "timestamped update-before-delete did not return both retraction designators");
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timedRemovals().empty(),
      "timestamped update-before-delete delivered a TSO callback before the receiver requested it");

  receiver.recorder().clearCallbackOrder();
  receiverAmbassador.flushQueueRequest(*deletionTime);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() == 1U &&
            receiver.recorder().timedRemovals().size() == 1U &&
            receiver.recorder().flushQueueGrants().size() == 1U;
      },
      options,
      "timestamped update-before-delete reflection, removal, and flush grant");

  auto const reflections = receiver.recorder().timedReflections();
  auto const removals = receiver.recorder().timedRemovals();
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "remove", "flush-grant"},
      "timestamped update-before-delete did not deliver the update before object removal");
  require(
      reflections.size() == 1U && receiver.recorder().reflections().empty(),
      "timestamped update-before-delete did not use exactly one timestamped reflection");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(receiverAttribute) == 1U &&
          copyBytes(reflection.values.at(receiverAttribute)) == valueBytes &&
          reflection.tag == updateTagBytes &&
          reflection.producer == owner.federateHandle() &&
          reflection.time == encodeTime(*updateTime) &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(updateRetraction.encode()),
      "timestamped update-before-delete changed the update's object, payload, order, time, or retraction data");

  require(
      removals.size() == 1U && receiver.recorder().removals().empty(),
      "timestamped update-before-delete did not use exactly one timestamped removal");
  auto const& removal = removals.front();
  require(
      removal.object == object && removal.tag == deletionTagBytes &&
          removal.producer == owner.federateHandle() &&
          removal.time == encodeTime(*deletionTime) &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP &&
          removal.retractionPresent &&
          removal.retraction == copyBytes(deletionRetraction.encode()),
      "timestamped update-before-delete changed the deletion's object, tag, order, time, or retraction data");

  receiver.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
}

}  // namespace hla_rti_cpp_tck::detail
