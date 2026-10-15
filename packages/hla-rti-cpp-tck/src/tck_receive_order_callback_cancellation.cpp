#include "tck_receive_order_callback_cancellation.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioReceiveOrderAttributeUpdateCallbackCancellation(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session active(options, model, "active");
  Session cancelled(options, model, "cancelled");
  auto const federation = federationName(
      options,
      "receive-order-attribute-update-callback-cancellation");
  connectAndJoin(publisher, active, options, federation, options.fom);
  cancelled.connect();
  cancelled.join(
      options.memberFederateName + L"-cancelled",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const activeClass = active.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const cancelledClass = cancelled.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const activeAttribute = active.rtiAmbassador().getAttributeHandle(
      activeClass,
      options.attributeName);
  auto const cancelledAttribute = cancelled.rtiAmbassador().getAttributeHandle(
      cancelledClass,
      options.attributeName);
  require(
      publisherClass.isValid() && activeClass.isValid() && cancelledClass.isValid() &&
          publisherAttribute.isValid() && activeAttribute.isValid() &&
          cancelledAttribute.isValid(),
      "receive-order attribute-update callback-cancellation lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const activeAttributes{activeAttribute};
  rti::AttributeHandleSet const cancelledAttributes{cancelledAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  active.rtiAmbassador().subscribeObjectClassAttributes(
      activeClass,
      activeAttributes,
      true,
      L"");
  cancelled.rtiAmbassador().subscribeObjectClassAttributes(
      cancelledClass,
      cancelledAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "receive-order attribute-update callback-cancellation registration returned an invalid object handle");
  waitFor(
      active,
      [&] { return active.recorder().hasDiscovery(object); },
      options,
      "receive-order attribute-update callback-cancellation active discovery");
  waitFor(
      cancelled,
      [&] { return cancelled.recorder().hasDiscovery(object); },
      options,
      "receive-order attribute-update callback-cancellation cancelled discovery");
  active.recorder().clearReflection();
  cancelled.recorder().clearReflection();

  std::vector<std::uint8_t> const value{0x52U, 0x4FU, 0x50U};
  std::vector<std::uint8_t> const tagBytes{0x43U, 0x41U, 0x4EU};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(value.data(), value.size()));
  publisher.rtiAmbassador().updateAttributeValues(object, values, tag);

  if (model == rti::HLA_EVOKED) {
    require(
        !active.recorder().reflection().present &&
            !cancelled.recorder().reflection().present,
        "receive-order attribute-update callback-cancellation delivered before callback servicing");
    cancelled.rtiAmbassador().unsubscribeObjectClassAttributes(
        cancelledClass,
        cancelledAttributes);
    waitFor(
        active,
        [&] { return active.recorder().reflection().present; },
        options,
        "receive-order attribute-update callback-cancellation active reflection");
    for (int pass = 0; pass != 8; ++pass) {
      cancelled.pump();
    }
    require(
        !cancelled.recorder().reflection().present,
        "receive-order attribute-update callback-cancellation delivered a stale unsubscribed reflection");
  } else {
    waitFor(
        active,
        [&] { return active.recorder().reflection().present; },
        options,
        "immediate attribute-update callback-cancellation active reflection");
    waitFor(
        cancelled,
        [&] { return cancelled.recorder().reflection().present; },
        options,
        "immediate attribute-update callback-cancellation cancelled reflection");
    cancelled.rtiAmbassador().unsubscribeObjectClassAttributes(
        cancelledClass,
        cancelledAttributes);
  }

  auto requireReflection = [&](Session& receiver,
                               rti::AttributeHandle const& receiverAttribute,
                               std::string const& description) {
    auto const reflected = receiver.recorder().reflection();
    require(reflected.object == object, description + " returned the wrong object handle");
    require(
        reflected.values.size() == 1U && reflected.values.count(receiverAttribute) == 1U,
        description + " returned the wrong attribute set");
    require(
        copyBytes(reflected.values.at(receiverAttribute)) == value,
        description + " returned the wrong attribute value");
    require(reflected.tag == tagBytes, description + " returned the wrong user tag");
    require(
        reflected.producer == publisher.federateHandle(),
        description + " returned the wrong producing federate");
    require(
        reflected.transportation.isValid(),
        description + " returned an invalid transportation handle");
  };
  requireReflection(
      active,
      activeAttribute,
      "receive-order attribute-update callback-cancellation active reflection");
  if (model == rti::HLA_IMMEDIATE) {
    requireReflection(
        cancelled,
        cancelledAttribute,
        "immediate attribute-update callback-cancellation cancelled reflection");
  }

  publisher.resign(rti::DELETE_OBJECTS);
  active.resign(rti::NO_ACTION);
  cancelled.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  cancelled.disconnect();
  active.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderAttributeUpdateCallbackCancellationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderAttributeUpdateCallbackCancellation(options, model);
}

void scenarioReceiveOrderMultiAttributeUpdateCallbackCancellation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "receive-order multi-attribute update callback cancellation requires an adapter-supplied FOM");
  Session publisher(options, model, "owner");
  Session active(options, model, "active");
  Session cancelled(options, model, "cancelled");
  auto const federation = federationName(
      options,
      "receive-order-multi-attribute-update-callback-cancellation");
  connectAndJoin(publisher, active, options, federation, options.multiAttributeFom);
  cancelled.connect();
  cancelled.join(
      options.memberFederateName + L"-cancelled-multi-attribute",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const activeClass = active.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const cancelledClass = cancelled.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publisherFirst = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeFirstName);
  auto const publisherSecond = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeSecondName);
  auto const activeFirst = active.rtiAmbassador().getAttributeHandle(
      activeClass,
      options.multiAttributeFirstName);
  auto const activeSecond = active.rtiAmbassador().getAttributeHandle(
      activeClass,
      options.multiAttributeSecondName);
  auto const cancelledSecond = cancelled.rtiAmbassador().getAttributeHandle(
      cancelledClass,
      options.multiAttributeSecondName);
  require(
      publisherClass.isValid() && activeClass.isValid() && cancelledClass.isValid() &&
          publisherFirst.isValid() && publisherSecond.isValid() &&
          activeFirst.isValid() && activeSecond.isValid() && cancelledSecond.isValid(),
      "receive-order multi-attribute update callback cancellation lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{
      publisherFirst,
      publisherSecond};
  rti::AttributeHandleSet const activeAttributes{activeFirst, activeSecond};
  rti::AttributeHandleSet const cancelledAttributes{cancelledSecond};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  active.rtiAmbassador().subscribeObjectClassAttributes(
      activeClass,
      activeAttributes,
      true,
      L"");
  cancelled.rtiAmbassador().subscribeObjectClassAttributes(
      cancelledClass,
      cancelledAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "receive-order multi-attribute update callback cancellation registration returned an invalid object handle");
  waitForSessions(
      {&active, &cancelled},
      [&] {
        return active.recorder().hasDiscovery(object) &&
            cancelled.recorder().hasDiscovery(object);
      },
      options,
      "receive-order multi-attribute update callback cancellation discovery");
  active.recorder().clearReflection();
  cancelled.recorder().clearReflection();

  auto const reliable = active.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      reliable.isValid(),
      "receive-order multi-attribute update callback cancellation could not resolve HLAreliable");

  std::vector<std::uint8_t> const firstValue{0x11U};
  std::vector<std::uint8_t> const secondValue{0x22U};
  std::vector<std::uint8_t> const firstTag{0xD1U, 0x01U};
  rti::AttributeHandleValueMap bothValues;
  bothValues.emplace(
      publisherFirst,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  bothValues.emplace(
      publisherSecond,
      rti::VariableLengthData(secondValue.data(), secondValue.size()));
  rti::VariableLengthData bothTag(firstTag.data(), firstTag.size());
  publisher.rtiAmbassador().updateAttributeValues(object, bothValues, bothTag);

  if (model == rti::HLA_EVOKED) {
    require(
        active.recorder().reflectionCount() == 0U &&
            cancelled.recorder().reflectionCount() == 0U,
        "receive-order multi-attribute update callback cancellation delivered before callback servicing");
    cancelled.rtiAmbassador().unsubscribeObjectClassAttributes(
        cancelledClass,
        cancelledAttributes);
    waitFor(
        active,
        [&] { return active.recorder().reflectionCount() >= 1U; },
        options,
        "receive-order multi-attribute update callback cancellation active reflection");
    for (int pass = 0; pass != 8; ++pass) {
      cancelled.pump();
    }
    require(
        cancelled.recorder().reflectionCount() == 0U,
        "receive-order multi-attribute update callback cancellation delivered a stale unsubscribed reflection");
  } else {
    waitFor(
        active,
        [&] { return active.recorder().reflectionCount() >= 1U; },
        options,
        "immediate multi-attribute update callback cancellation active reflection");
    waitFor(
        cancelled,
        [&] { return cancelled.recorder().reflectionCount() >= 1U; },
        options,
        "immediate multi-attribute update callback cancellation cancelled reflection");
    cancelled.rtiAmbassador().unsubscribeObjectClassAttributes(
        cancelledClass,
        cancelledAttributes);
  }

  auto requireReflection = [&](Session& receiver,
                               rti::AttributeHandle const& firstAttribute,
                               rti::AttributeHandle const& secondAttribute,
                               bool expectBoth,
                               std::string const& description) {
    auto const reflection = receiver.recorder().reflection();
    require(
        reflection.present && reflection.object == object &&
            reflection.tag == firstTag && reflection.transportation == reliable &&
            reflection.producer == publisher.federateHandle() &&
            !reflection.regions.has_value(),
        description + " returned the wrong object or delivery metadata");
    if (expectBoth) {
      require(
          reflection.values.size() == 2U &&
              reflection.values.count(firstAttribute) == 1U &&
              reflection.values.count(secondAttribute) == 1U,
          description + " returned the wrong complete attribute projection");
      require(
          copyBytes(reflection.values.at(firstAttribute)) == firstValue &&
              copyBytes(reflection.values.at(secondAttribute)) == secondValue,
          description + " returned the wrong complete attribute values");
    } else {
      require(
          reflection.values.size() == 1U &&
              reflection.values.count(secondAttribute) == 1U,
          description + " returned the wrong selective attribute projection");
      require(
          copyBytes(reflection.values.at(secondAttribute)) == secondValue,
          description + " returned the wrong selective attribute value");
    }
  };

  requireReflection(
      active,
      activeFirst,
      activeSecond,
      true,
      "receive-order multi-attribute update callback cancellation active reflection");
  if (model == rti::HLA_IMMEDIATE) {
    requireReflection(
        cancelled,
        cancelledSecond,
        cancelledSecond,
        false,
        "immediate multi-attribute update callback cancellation cancelled reflection");
  }
  require(
      publisher.recorder().reflectionCount() == 0U,
      "receive-order multi-attribute update callback cancellation looped a reflection to the publisher");

  active.recorder().clearReflection();
  std::vector<std::uint8_t> const replacementValue{0x33U};
  std::vector<std::uint8_t> const replacementTag{0xD1U, 0x02U};
  rti::AttributeHandleValueMap firstOnlyValues;
  firstOnlyValues.emplace(
      publisherFirst,
      rti::VariableLengthData(
          replacementValue.data(),
          replacementValue.size()));
  rti::VariableLengthData replacementUserTag(
      replacementTag.data(),
      replacementTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstOnlyValues,
      replacementUserTag);
  waitFor(
      active,
      [&] { return active.recorder().reflectionCount() >= 1U; },
      options,
      "receive-order multi-attribute update callback cancellation post-withdrawal reflection");
  auto const replacementReflection = active.recorder().reflection();
  require(
      replacementReflection.present && replacementReflection.object == object &&
          replacementReflection.values.size() == 1U &&
          replacementReflection.values.count(activeFirst) == 1U &&
          copyBytes(replacementReflection.values.at(activeFirst)) == replacementValue &&
          replacementReflection.tag == replacementTag &&
          replacementReflection.transportation == reliable &&
          replacementReflection.producer == publisher.federateHandle() &&
          !replacementReflection.regions.has_value(),
      "receive-order multi-attribute update callback cancellation post-withdrawal update was not projected correctly");
  for (int pass = 0; pass != 8; ++pass) {
    cancelled.pump();
  }
  require(
      cancelled.recorder().reflectionCount() ==
          (model == rti::HLA_IMMEDIATE ? 1U : 0U),
      "receive-order multi-attribute update callback cancellation delivered after withdrawal");

  active.rtiAmbassador().unsubscribeObjectClassAttributes(
      activeClass,
      activeAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  cancelled.resign(rti::NO_ACTION);
  active.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  cancelled.disconnect();
  active.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderMultiAttributeUpdateCallbackCancellationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderMultiAttributeUpdateCallbackCancellation(options, model);
}

void scenarioReceiveOrderInteractionCallbackCancellation(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session active(options, model, "active");
  Session cancelled(options, model, "cancelled");
  auto const federation = federationName(
      options,
      "receive-order-interaction-callback-cancellation");
  connectAndJoin(publisher, active, options, federation, options.fom);
  cancelled.connect();
  cancelled.join(
      options.memberFederateName + L"-cancelled",
      options.federateType,
      federation);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const activeInteraction =
      active.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const cancelledInteraction =
      cancelled.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const activeParameter = active.rtiAmbassador().getParameterHandle(
      activeInteraction,
      options.parameterName);
  auto const cancelledParameter = cancelled.rtiAmbassador().getParameterHandle(
      cancelledInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && activeInteraction.isValid() &&
          cancelledInteraction.isValid() && publisherParameter.isValid() &&
          activeParameter.isValid() && cancelledParameter.isValid(),
      "receive-order interaction callback-cancellation lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  active.rtiAmbassador().subscribeInteractionClass(activeInteraction, true);
  cancelled.rtiAmbassador().subscribeInteractionClass(cancelledInteraction, true);

  std::vector<std::uint8_t> const parameterBytes{0x49U, 0x4EU, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x43U, 0x41U, 0x4EU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag);

  if (model == rti::HLA_EVOKED) {
    require(
        !active.recorder().interaction().present &&
            !cancelled.recorder().interaction().present,
        "receive-order interaction callback-cancellation delivered before callback servicing");
    cancelled.rtiAmbassador().unsubscribeInteractionClass(cancelledInteraction);
    waitFor(
        active,
        [&] { return active.recorder().interaction().present; },
        options,
        "receive-order interaction callback-cancellation active delivery");
    for (int pass = 0; pass != 8; ++pass) {
      cancelled.pump();
    }
    require(
        !cancelled.recorder().interaction().present,
        "receive-order interaction callback-cancellation delivered a stale unsubscribed interaction");
  } else {
    waitFor(
        active,
        [&] { return active.recorder().interaction().present; },
        options,
        "immediate interaction callback-cancellation active delivery");
    waitFor(
        cancelled,
        [&] { return cancelled.recorder().interaction().present; },
        options,
        "immediate interaction callback-cancellation cancelled delivery");
    cancelled.rtiAmbassador().unsubscribeInteractionClass(cancelledInteraction);
  }

  auto requireInteraction = [&](Session& receiver,
                                rti::ParameterHandle const& receiverParameter,
                                rti::InteractionClassHandle const& receiverInteraction,
                                std::string const& description) {
    auto const received = receiver.recorder().interaction();
    require(
        received.interaction == receiverInteraction,
        description + " returned the wrong interaction class handle");
    require(
        received.parameters.size() == 1U &&
            received.parameters.count(receiverParameter) == 1U,
        description + " returned the wrong parameter set");
    require(
        copyBytes(received.parameters.at(receiverParameter)) == parameterBytes,
        description + " returned the wrong parameter value");
    require(received.tag == tagBytes, description + " returned the wrong user tag");
    require(
        received.producer == publisher.federateHandle(),
        description + " returned the wrong producing federate");
    require(
        received.transportation.isValid(),
        description + " returned an invalid transportation handle");
  };
  requireInteraction(
      active,
      activeParameter,
      activeInteraction,
      "receive-order interaction callback-cancellation active delivery");
  if (model == rti::HLA_IMMEDIATE) {
    requireInteraction(
        cancelled,
        cancelledParameter,
        cancelledInteraction,
        "immediate interaction callback-cancellation cancelled delivery");
  }

  publisher.resign(rti::NO_ACTION);
  active.resign(rti::NO_ACTION);
  cancelled.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  cancelled.disconnect();
  active.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderInteractionCallbackCancellationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderInteractionCallbackCancellation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
