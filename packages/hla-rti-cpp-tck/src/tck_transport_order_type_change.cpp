#include "tck_transport_order_type_change.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioTransportationTypeChange(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "transportation-type-change-owner");
  Session observer(options, model, "transportation-type-change-observer");
  auto const federation = federationName(options, "transportation-type-change");
  connectAndJoin(owner, observer, options, federation, options.fom);

  rti::ObjectClassHandle ownerClass;
  rti::AttributeHandle ownerAttribute;
  rti::InteractionClassHandle ownerInteraction;
  rti::ParameterHandle ownerParameter;
  handles(owner, options, ownerClass, ownerAttribute, ownerInteraction, ownerParameter);

  rti::ObjectClassHandle observerClass;
  rti::AttributeHandle observerAttribute;
  rti::InteractionClassHandle observerInteraction;
  rti::ParameterHandle observerParameter;
  handles(
      observer,
      options,
      observerClass,
      observerAttribute,
      observerInteraction,
      observerParameter);
  require(
      ownerClass == observerClass && ownerAttribute == observerAttribute &&
          ownerInteraction == observerInteraction && ownerParameter == observerParameter,
      "transportation-type change lookups did not retain cross-federate handle identity");

  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = owner.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() && reliable != bestEffort,
      "transportation-type change lookup did not return distinct standard transports");

  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(ownerAttribute);
  rti::AttributeHandleSet observerAttributes;
  observerAttributes.insert(observerAttribute);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");
  owner.rtiAmbassador().publishInteractionClass(ownerInteraction);
  observer.rtiAmbassador().subscribeInteractionClass(observerInteraction, true);

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "transportation-type change registration returned an invalid handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "transportation-type change object discovery");

  auto sameTransport = [](rti::RTIambassador& inspector,
                          rti::TransportationTypeHandle const& actual,
                          std::wstring const& expectedName) {
    return actual.isValid() && inspector.getTransportationTypeName(actual) == expectedName;
  };

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryAttributeTransportationType(object, ownerAttribute);
  waitFor(
      owner,
      [&] { return owner.recorder().attributeTransportationReports().size() >= 1U; },
      options,
      "initial attribute transportation query");
  auto const initialAttributeReports = owner.recorder().attributeTransportationReports();
  auto const initialAttributeReport = std::find_if(
      initialAttributeReports.begin(),
      initialAttributeReports.end(),
      [&](auto const& report) {
        return report.object == object && report.attribute == ownerAttribute;
      });
  require(
      initialAttributeReport != initialAttributeReports.end() &&
          sameTransport(owner.rtiAmbassador(), initialAttributeReport->transportation, L"HLAreliable"),
      "initial attribute transportation query did not report reliable transport");

  auto sendAttribute = [&](std::uint8_t valueByte, std::uint8_t tagByte) {
    std::vector<std::uint8_t> valueBytes{valueByte};
    std::vector<std::uint8_t> tagBytes{tagByte};
    rti::AttributeHandleValueMap values;
    values.emplace(
        ownerAttribute,
        rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    owner.rtiAmbassador().updateAttributeValues(object, values, tag);
  };

  owner.recorder().clearTransportationRecords();
  observer.recorder().clearReflection();
  owner.rtiAmbassador().requestAttributeTransportationTypeChange(
      object,
      ownerAttributes,
      bestEffort);
  if (model == rti::HLA_EVOKED) {
    requireException(
        [&] {
          owner.rtiAmbassador().requestAttributeTransportationTypeChange(
              object,
              ownerAttributes,
              reliable);
        },
        L"AttributeAlreadyBeingChanged",
        "duplicate attribute transportation-type change request");
  } else {
    waitFor(
        owner,
        [&] { return owner.recorder().attributeTransportationConfirmations().size() >= 1U; },
        options,
        "immediate attribute transportation-type change confirmation");
  }

  sendAttribute(0x01U, 0x11U);
  waitFor(
      observer,
      [&] { return observer.recorder().reflection().present; },
      options,
      "pre-confirmation attribute update");
  auto const beforeReflection = observer.recorder().reflection();
  require(
      beforeReflection.object == object && beforeReflection.values.size() == 1U &&
          beforeReflection.values.count(observerAttribute) == 1U &&
          copyBytes(beforeReflection.values.at(observerAttribute)) ==
              std::vector<std::uint8_t>{0x01U} &&
          beforeReflection.tag == std::vector<std::uint8_t>{0x11U} &&
          beforeReflection.producer == owner.federateHandle() &&
          sameTransport(
              observer.rtiAmbassador(),
              beforeReflection.transportation,
              model == rti::HLA_EVOKED ? L"HLAreliable" : L"HLAbestEffort"),
      model == rti::HLA_EVOKED
          ? "pre-confirmation attribute update did not use reliable transport"
          : "immediate attribute transportation change did not use best-effort transport");

  waitFor(
      owner,
      [&] { return owner.recorder().attributeTransportationConfirmations().size() >= 1U; },
      options,
      "attribute transportation-type change confirmation");
  auto const attributeConfirmations = owner.recorder().attributeTransportationConfirmations();
  auto const& attributeConfirmation = attributeConfirmations.back();
  require(
      attributeConfirmation.object == object &&
          attributeConfirmation.attributes.count(ownerAttribute) == 1U &&
          sameTransport(owner.rtiAmbassador(), attributeConfirmation.transportation,
                        L"HLAbestEffort"),
      "attribute transportation-type change confirmation did not commit best effort");

  observer.recorder().clearReflection();
  sendAttribute(0x02U, 0x12U);
  waitFor(
      observer,
      [&] { return observer.recorder().reflection().present; },
      options,
      "post-confirmation attribute update");
  auto const afterReflection = observer.recorder().reflection();
  require(
      afterReflection.object == object && afterReflection.values.size() == 1U &&
          afterReflection.values.count(observerAttribute) == 1U &&
          copyBytes(afterReflection.values.at(observerAttribute)) ==
              std::vector<std::uint8_t>{0x02U} &&
          afterReflection.tag == std::vector<std::uint8_t>{0x12U} &&
          sameTransport(observer.rtiAmbassador(), afterReflection.transportation,
                        L"HLAbestEffort"),
      "post-confirmation attribute update did not use best-effort transport");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryAttributeTransportationType(object, ownerAttribute);
  waitFor(
      owner,
      [&] { return owner.recorder().attributeTransportationReports().size() >= 1U; },
      options,
      "committed attribute transportation query");
  auto const committedAttributeReports = owner.recorder().attributeTransportationReports();
  auto const committedAttributeReport = std::find_if(
      committedAttributeReports.begin(),
      committedAttributeReports.end(),
      [&](auto const& report) {
        return report.object == object && report.attribute == ownerAttribute;
      });
  require(
      committedAttributeReport != committedAttributeReports.end() &&
          sameTransport(owner.rtiAmbassador(), committedAttributeReport->transportation,
                        L"HLAbestEffort"),
      "committed attribute transportation query did not report best effort");

  auto sendInteraction = [&](std::uint8_t tagByte) {
    std::vector<std::uint8_t> tagBytes{tagByte};
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    owner.rtiAmbassador().sendInteraction(
        ownerInteraction,
        rti::ParameterHandleValueMap{},
        tag);
  };

  owner.recorder().clearTransportationRecords();
  observer.recorder().clearInteraction();
  owner.rtiAmbassador().requestInteractionTransportationTypeChange(
      ownerInteraction,
      bestEffort);
  if (model == rti::HLA_EVOKED) {
    requireException(
        [&] {
          owner.rtiAmbassador().requestInteractionTransportationTypeChange(
              ownerInteraction,
              reliable);
        },
        L"InteractionClassAlreadyBeingChanged",
        "duplicate interaction transportation-type change request");
  } else {
    waitFor(
        owner,
        [&] { return owner.recorder().interactionTransportationConfirmations().size() >= 1U; },
        options,
        "immediate interaction transportation-type change confirmation");
  }

  sendInteraction(0x21U);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "pre-confirmation interaction delivery");
  auto const beforeInteraction = observer.recorder().interaction();
  require(
      beforeInteraction.interaction == observerInteraction &&
          beforeInteraction.parameters.empty() &&
          beforeInteraction.tag == std::vector<std::uint8_t>{0x21U} &&
          beforeInteraction.producer == owner.federateHandle() &&
          sameTransport(
              observer.rtiAmbassador(),
              beforeInteraction.transportation,
              model == rti::HLA_EVOKED ? L"HLAreliable" : L"HLAbestEffort"),
      model == rti::HLA_EVOKED
          ? "pre-confirmation interaction did not use reliable transport"
          : "immediate interaction transportation change did not use best-effort transport");

  waitFor(
      owner,
      [&] { return owner.recorder().interactionTransportationConfirmations().size() >= 1U; },
      options,
      "interaction transportation-type change confirmation");
  auto const interactionConfirmations = owner.recorder().interactionTransportationConfirmations();
  auto const& interactionConfirmation = interactionConfirmations.back();
  require(
      interactionConfirmation.interaction == ownerInteraction &&
          sameTransport(owner.rtiAmbassador(), interactionConfirmation.transportation,
                        L"HLAbestEffort"),
      "interaction transportation-type change confirmation did not commit best effort");

  observer.recorder().clearInteraction();
  sendInteraction(0x22U);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "post-confirmation interaction delivery");
  auto const afterInteraction = observer.recorder().interaction();
  require(
      afterInteraction.interaction == observerInteraction &&
          afterInteraction.parameters.empty() &&
          afterInteraction.tag == std::vector<std::uint8_t>{0x22U} &&
          sameTransport(observer.rtiAmbassador(), afterInteraction.transportation,
                        L"HLAbestEffort"),
      "post-confirmation interaction did not use best-effort transport");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().requestInteractionTransportationTypeChange(
      ownerInteraction,
      bestEffort);
  waitFor(
      owner,
      [&] {
        return owner.recorder().interactionTransportationConfirmations().size() >=
            1U;
      },
      options,
      "unchanged interaction transportation-type confirmation");
  auto const unchangedInteractionConfirmations =
      owner.recorder().interactionTransportationConfirmations();
  require(
      unchangedInteractionConfirmations.size() == 1U &&
          unchangedInteractionConfirmations.front().interaction == ownerInteraction &&
          sameTransport(
              owner.rtiAmbassador(),
              unchangedInteractionConfirmations.front().transportation,
              L"HLAbestEffort"),
      "unchanged interaction transportation-type request did not confirm the achieved best-effort type");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryInteractionTransportationType(
      owner.federateHandle(),
      ownerInteraction);
  waitFor(
      owner,
      [&] { return owner.recorder().interactionTransportationReports().size() >= 1U; },
      options,
      "committed interaction transportation query");
  auto const interactionReports = owner.recorder().interactionTransportationReports();
  auto const interactionReport = std::find_if(
      interactionReports.begin(),
      interactionReports.end(),
      [&](auto const& report) {
        return report.federate == owner.federateHandle() &&
            report.interaction == ownerInteraction;
      });
  require(
      interactionReport != interactionReports.end() &&
          sameTransport(owner.rtiAmbassador(), interactionReport->transportation,
                        L"HLAbestEffort"),
      "committed interaction transportation query did not report best effort");

  observer.rtiAmbassador().unsubscribeInteractionClass(observerInteraction);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(observerClass, observerAttributes);
  owner.rtiAmbassador().unpublishInteractionClass(ownerInteraction);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  owner.rtiAmbassador().unpublishObjectClass(ownerClass);
  owner.resign(rti::DELETE_OBJECTS);
  observer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioTransportationTypeChangeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTransportationTypeChange(options, model);
}

void scenarioOrderTypeChange(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Order-type change testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "order-type-change-publisher");
  Session receiver(options, model, "order-type-change-receiver");
  auto const federation = federationName(options, "order-type-change");
  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(
      options.memberFederateName + L"-order-type-change",
      options.federateType,
      federation);

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
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherClass == receiverClass && publisherAttribute == receiverAttribute &&
          publisherInteraction == receiverInteraction && publisherParameter == receiverParameter,
      "Order-type change lookup returned invalid or inconsistent adapter-supplied handles");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::RECEIVE);
  auto const receiveObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(receiveObject.isValid(), "Receive-order registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(receiveObject); },
      options,
      "Receive-order object discovery");

  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  auto const defaultTimestampObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      defaultTimestampObject.isValid(),
      "Timestamp-order default registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(defaultTimestampObject); },
      options,
      "Timestamp-order default object discovery");

  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::RECEIVE);
  auto const explicitTimestampObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      explicitTimestampObject.isValid(),
      "Explicit timestamp-order registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(explicitTimestampObject); },
      options,
      "Explicit timestamp-order object discovery");
  publisher.rtiAmbassador().changeAttributeOrderType(
      explicitTimestampObject,
      publisherAttributes,
      rti::TIMESTAMP);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Order-type change members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(lookahead != nullptr, "Order-type change could not allocate lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Order-type change time-constrained callback");
  receiver.rtiAmbassador().enableAsynchronousDelivery();
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Order-type change time-regulation callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const receiveBytes{0x11U};
  std::vector<std::uint8_t> const defaultTimestampBytes{0x22U};
  std::vector<std::uint8_t> const explicitTimestampBytes{0x33U};
  std::vector<std::uint8_t> const attributeTagBytes{0x4FU, 0x52U, 0x44U};
  rti::AttributeHandleValueMap receiveValues;
  receiveValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(receiveBytes.data(), receiveBytes.size()));
  rti::AttributeHandleValueMap defaultTimestampValues;
  defaultTimestampValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(
          defaultTimestampBytes.data(),
          defaultTimestampBytes.size()));
  rti::AttributeHandleValueMap explicitTimestampValues;
  explicitTimestampValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(
          explicitTimestampBytes.data(),
          explicitTimestampBytes.size()));
  rti::VariableLengthData attributeTag(
      attributeTagBytes.data(),
      attributeTagBytes.size());

  publisher.rtiAmbassador().updateAttributeValues(
      receiveObject,
      receiveValues,
      attributeTag);
  auto const defaultTimestampRetraction =
      publisher.rtiAmbassador().updateAttributeValues(
          defaultTimestampObject,
          defaultTimestampValues,
          attributeTag,
          *timestamp);
  auto const explicitTimestampRetraction =
      publisher.rtiAmbassador().updateAttributeValues(
          explicitTimestampObject,
          explicitTimestampValues,
          attributeTag,
          *timestamp);
  require(
      defaultTimestampRetraction.isValid() &&
          explicitTimestampRetraction.isValid(),
      "Order-type change returned the wrong attribute retraction validity");

  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] { return receiver.recorder().timedReflections().size() >= 2U; },
      options,
      "Order-type change timestamped attribute deliveries");
  waitForSessions(
      {&publisher, &receiver},
      [&] { return receiver.recorder().reflections().size() >= 1U; },
      options,
      "Order-type change ordinary attribute delivery");
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "Order-type change publisher time advance grant");
  require(
      receiver.recorder().reflections().size() == 1U &&
          receiver.recorder().timedReflections().size() == 2U,
      "Order-type change delivered the wrong ordinary/timestamped split");

  auto const ordinaryReflection = receiver.recorder().reflections().front();
  require(
      ordinaryReflection.object == receiveObject &&
          ordinaryReflection.values.size() == 1U &&
          ordinaryReflection.values.count(receiverAttribute) == 1U &&
          copyBytes(ordinaryReflection.values.at(receiverAttribute)) == receiveBytes &&
          ordinaryReflection.tag == attributeTagBytes,
      "Receive-order attribute reflection did not preserve the object or payload");
  auto const timedReflections = receiver.recorder().timedReflections();
  for (auto const& reflection : timedReflections) {
    require(
        (reflection.object == defaultTimestampObject ||
         reflection.object == explicitTimestampObject) &&
            reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U &&
            copyBytes(reflection.values.at(receiverAttribute)) != receiveBytes &&
            reflection.tag == attributeTagBytes &&
            reflection.time == encodeTime(*timestamp) &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP &&
            reflection.retractionPresent && reflection.regions == std::nullopt,
        "Timestamp-order attribute reflection returned the wrong metadata");
  }
  require(
      timedReflections.at(0).object != timedReflections.at(1).object,
      "Order-type change collapsed the two timestamped object instances");
  auto const findTimedRetraction = [&](rti::ObjectInstanceHandle const& object) {
    for (auto const& reflection : timedReflections) {
      if (reflection.object == object) {
        return reflection.retraction;
      }
    }
    return std::vector<std::uint8_t>{};
  };
  require(
      findTimedRetraction(defaultTimestampObject) ==
          copyBytes(defaultTimestampRetraction.encode()) &&
          findTimedRetraction(explicitTimestampObject) ==
              copyBytes(explicitTimestampRetraction.encode()),
      "Timestamp-order attribute reflection returned the wrong retraction handle");

  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::RECEIVE);
  std::vector<std::uint8_t> const parameterBytes{0x49U, 0x4EU, 0x54U};
  std::vector<std::uint8_t> const interactionTagBytes{0x49U, 0x4EU, 0x54U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData interactionTag(
      interactionTagBytes.data(),
      interactionTagBytes.size());
  auto const interactionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const interactionRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      interactionTag,
      *interactionTime);
  require(
      !interactionRetraction.isValid(),
      "Receive-order interaction unexpectedly returned a retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() >= 1U; },
      options,
      "Order-type change receive-order interaction");
  require(
      receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().interactions().empty(),
      "Receive-order interaction used the wrong callback overload");
  auto const interaction = receiver.recorder().timedInteractions().front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverParameter)) == parameterBytes &&
          interaction.tag == interactionTagBytes &&
          interaction.time == encodeTime(*interactionTime) &&
          interaction.sentOrder == rti::RECEIVE &&
          interaction.receivedOrder == rti::RECEIVE &&
          !interaction.retractionPresent &&
          interaction.regions == std::nullopt,
      "Receive-order interaction did not preserve the adapter FOM payload");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioOrderTypeChangeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOrderTypeChange(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
