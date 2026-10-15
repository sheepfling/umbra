#include "tck_inherited_derived_delivery_scenarios.hpp"
#include "tck_main_scenarios.hpp"

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioInheritedObjectAttributeProjection(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Inherited object attribute projection requires an adapter-supplied model FOM");

  Session publisher(options, model, "owner");
  Session baseSubscriber(options, model, "member-base");
  Session derivedSubscriber(options, model, "member-derived");
  auto const federation = federationName(
      options,
      "inherited-object-attribute-projection");
  connectAndJoin(
      publisher,
      baseSubscriber,
      options,
      federation,
      options.modelFom);
  derivedSubscriber.connect();
  derivedSubscriber.join(
      options.memberFederateName + L"-derived",
      options.federateType,
      federation);

  auto const publisherDerivedClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const baseClass = baseSubscriber.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const derivedClass = derivedSubscriber.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  require(
      publisherDerivedClass.isValid() && baseClass.isValid() && derivedClass.isValid(),
      "inherited object attribute projection returned an invalid object class handle");

  auto const publisherIdentity = publisher.rtiAmbassador().getAttributeHandle(
      publisherDerivedClass,
      options.typedIdentityAttributeName);
  auto const publisherDerived = publisher.rtiAmbassador().getAttributeHandle(
      publisherDerivedClass,
      options.typedDerivedAttributeName);
  auto const baseIdentity = baseSubscriber.rtiAmbassador().getAttributeHandle(
      baseClass,
      options.typedIdentityAttributeName);
  auto const derivedIdentity = derivedSubscriber.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.typedIdentityAttributeName);
  auto const derivedAttribute = derivedSubscriber.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.typedDerivedAttributeName);
  require(
      publisherIdentity.isValid() && publisherDerived.isValid() &&
          baseIdentity.isValid() && derivedIdentity.isValid() &&
          derivedAttribute.isValid(),
      "inherited object attribute projection returned an invalid attribute handle");
  require(
      publisher.rtiAmbassador().getAttributeHandle(
          publisher.rtiAmbassador().getObjectClassHandle(
              options.typedObjectClassName),
          options.typedIdentityAttributeName) == publisherIdentity &&
          baseIdentity == publisherIdentity && derivedIdentity == publisherIdentity,
      "inherited object attribute projection changed the base attribute handle");
  require(
      publisherDerived != publisherIdentity && derivedAttribute != derivedIdentity,
      "inherited object attribute projection returned duplicate base and derived handles");

  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherDerivedClass,
      rti::AttributeHandleSet{publisherIdentity, publisherDerived});
  baseSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      baseClass,
      rti::AttributeHandleSet{baseIdentity},
      true,
      L"");
  derivedSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      derivedClass,
      rti::AttributeHandleSet{derivedIdentity, derivedAttribute},
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherDerivedClass);
  require(
      object.isValid(),
      "inherited object attribute projection registration returned an invalid object handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty() &&
          publisher.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "inherited object attribute projection registration did not round-trip its identity");

  waitFor(
      baseSubscriber,
      [&] { return baseSubscriber.recorder().hasDiscovery(object); },
      options,
      "inherited object attribute base-class discovery");
  waitFor(
      derivedSubscriber,
      [&] { return derivedSubscriber.recorder().hasDiscovery(object); },
      options,
      "inherited object attribute derived-class discovery");
  require(
      baseSubscriber.recorder().discoveryCount() == 1U &&
          derivedSubscriber.recorder().discoveryCount() == 1U,
      "inherited object attribute projection delivered duplicate discovery callbacks");
  for (auto* subscriber : {&baseSubscriber, &derivedSubscriber}) {
    auto const discovery = subscriber->recorder().discovery();
    require(
        discovery.object == object && !discovery.name.empty() &&
            discovery.producer == publisher.federateHandle(),
        "inherited object attribute projection changed discovery identity metadata");
    require(
        subscriber->rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
            subscriber->rtiAmbassador().getObjectInstanceName(object) == objectName,
        "inherited object attribute projection did not establish subscriber identity lookups");
  }

  std::vector<std::uint8_t> const identityBytes{0x49U, 0x44U};
  std::vector<std::uint8_t> const derivedBytes{0x44U, 0x56U};
  std::vector<std::uint8_t> const tagBytes{0x50U, 0x52U, 0x4AU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherIdentity,
      rti::VariableLengthData(identityBytes.data(), identityBytes.size()));
  values.emplace(
      publisherDerived,
      rti::VariableLengthData(derivedBytes.data(), derivedBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().updateAttributeValues(object, values, tag);

  waitFor(
      baseSubscriber,
      [&] { return baseSubscriber.recorder().reflectionCount() >= 1U; },
      options,
      "inherited object attribute base-class projection");
  waitFor(
      derivedSubscriber,
      [&] { return derivedSubscriber.recorder().reflectionCount() >= 1U; },
      options,
      "inherited object attribute derived-class projection");
  auto const baseReflection = baseSubscriber.recorder().reflection();
  auto const derivedReflection = derivedSubscriber.recorder().reflection();
  require(
      baseReflection.object == object && baseReflection.tag == tagBytes &&
          baseReflection.producer == publisher.federateHandle() &&
          baseReflection.transportation.isValid() &&
          baseReflection.values.size() == 1U &&
          baseReflection.values.count(baseIdentity) == 1U &&
          copyBytes(baseReflection.values.at(baseIdentity)) == identityBytes &&
          baseReflection.values.count(derivedAttribute) == 0U,
      "base-class subscription did not project only inherited attributes");
  require(
      derivedReflection.object == object && derivedReflection.tag == tagBytes &&
          derivedReflection.producer == publisher.federateHandle() &&
          derivedReflection.transportation.isValid() &&
          derivedReflection.values.size() == 2U &&
          derivedReflection.values.count(derivedIdentity) == 1U &&
          derivedReflection.values.count(derivedAttribute) == 1U &&
          copyBytes(derivedReflection.values.at(derivedIdentity)) == identityBytes &&
          copyBytes(derivedReflection.values.at(derivedAttribute)) == derivedBytes,
      "derived-class subscription did not retain inherited and declared attributes");
  require(
      publisher.recorder().reflectionCount() == 0U,
      "inherited object attribute projection looped the update back to its publisher");

  baseSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      baseClass,
      rti::AttributeHandleSet{baseIdentity});
  derivedSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      derivedClass,
      rti::AttributeHandleSet{derivedIdentity, derivedAttribute});
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherDerivedClass,
      rti::AttributeHandleSet{publisherIdentity, publisherDerived});
  baseSubscriber.resign(rti::NO_ACTION);
  derivedSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  derivedSubscriber.disconnect();
  baseSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioInheritedObjectAttributeProjectionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInheritedObjectAttributeProjection(options, model);
}

void scenarioInheritedInteractionClassDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Inherited interaction delivery requires an adapter-supplied model FOM");

  Session publisher(options, model, "interaction-hierarchy-publisher");
  Session baseSubscriber(options, model, "interaction-hierarchy-base-subscriber");
  Session derivedSubscriber(options, model, "interaction-hierarchy-derived-subscriber");
  auto const federation = federationName(options, "inherited-interaction-class-delivery");
  connectAndJoin(
      publisher,
      baseSubscriber,
      options,
      federation,
      options.modelFom);
  derivedSubscriber.connect();
  derivedSubscriber.join(
      options.memberFederateName + L"-derived-interaction-subscriber",
      options.federateType,
      federation);

  auto const baseInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const derivedInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  auto const baseSubscriberBaseInteraction =
      baseSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.typedInteractionClassName);
  auto const derivedSubscriberDerivedInteraction =
      derivedSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.typedDerivedInteractionClassName);
  require(
      baseInteraction.isValid() && derivedInteraction.isValid() &&
          baseSubscriberBaseInteraction == baseInteraction &&
          derivedSubscriberDerivedInteraction == derivedInteraction,
      "interaction-class hierarchy handles were invalid or unstable across members");

  auto const baseParameter = publisher.rtiAmbassador().getParameterHandle(
      baseInteraction,
      options.typedIntegerParameterName);
  auto const derivedBaseParameter = publisher.rtiAmbassador().getParameterHandle(
      derivedInteraction,
      options.typedIntegerParameterName);
  auto const derivedParameter = publisher.rtiAmbassador().getParameterHandle(
      derivedInteraction,
      options.typedDerivedParameterName);
  auto const baseSubscriberParameter = baseSubscriber.rtiAmbassador().getParameterHandle(
      baseSubscriberBaseInteraction,
      options.typedIntegerParameterName);
  auto const derivedSubscriberBaseParameter =
      derivedSubscriber.rtiAmbassador().getParameterHandle(
          derivedSubscriberDerivedInteraction,
          options.typedIntegerParameterName);
  auto const derivedSubscriberParameter =
      derivedSubscriber.rtiAmbassador().getParameterHandle(
          derivedSubscriberDerivedInteraction,
          options.typedDerivedParameterName);
  require(
      baseParameter.isValid() && derivedBaseParameter == baseParameter &&
          derivedParameter.isValid() && baseSubscriberParameter == baseParameter &&
          derivedSubscriberBaseParameter == baseParameter &&
          derivedSubscriberParameter == derivedParameter,
      "inherited interaction parameter handles were invalid or unstable across members");

  publisher.rtiAmbassador().publishInteractionClass(baseInteraction);
  publisher.rtiAmbassador().publishInteractionClass(derivedInteraction);
  baseSubscriber.rtiAmbassador().subscribeInteractionClass(
      baseSubscriberBaseInteraction,
      true);
  derivedSubscriber.rtiAmbassador().subscribeInteractionClass(
      derivedSubscriberDerivedInteraction,
      true);

  auto const baseValue = encodeDataElement(rti::HLAinteger32BE{37});
  auto const derivedValue = encodeDataElement(rti::HLAunicodeString{L"derived-value"});
  rti::ParameterHandleValueMap derivedParameters;
  derivedParameters.emplace(
      derivedBaseParameter,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  derivedParameters.emplace(
      derivedParameter,
      rti::VariableLengthData(derivedValue.data(), derivedValue.size()));
  std::vector<std::uint8_t> derivedTagBytes{0x49U, 0x48U, 0x44U, 0x31U};
  rti::VariableLengthData derivedTag(derivedTagBytes.data(), derivedTagBytes.size());
  publisher.rtiAmbassador().sendInteraction(
      derivedInteraction,
      derivedParameters,
      derivedTag);

  waitFor(
      baseSubscriber,
      [&] { return baseSubscriber.recorder().interactions().size() >= 1U; },
      options,
      "base interaction subscription receiving a derived interaction");
  waitFor(
      derivedSubscriber,
      [&] { return derivedSubscriber.recorder().interactions().size() >= 1U; },
      options,
      "derived interaction subscription receiving a derived interaction");
  auto const baseReceivedDerived = baseSubscriber.recorder().interactions();
  auto const derivedReceivedDerived = derivedSubscriber.recorder().interactions();
  require(
      baseReceivedDerived.size() == 1U && derivedReceivedDerived.size() == 1U,
      "derived interaction was not delivered exactly once to each matching subscription");
  auto const& baseReceived = baseReceivedDerived.front();
  require(
      baseReceived.interaction == baseInteraction,
      "base subscription did not report the closest subscribed interaction class");
  require(
      baseReceived.tag == derivedTagBytes,
      "base subscription did not receive the sent interaction tag");
  require(
      baseReceived.parameters.size() == 1U &&
          baseReceived.parameters.count(baseParameter) == 1U &&
          baseReceived.parameters.count(derivedParameter) == 0U,
      "base subscription did not project the derived interaction to base parameters");
  require(
      copyBytes(baseReceived.parameters.at(baseParameter)) == baseValue,
      "base subscription received a changed inherited parameter value");

  auto const& derivedReceived = derivedReceivedDerived.front();
  require(
      derivedReceived.interaction == derivedInteraction,
      "derived subscription did not report the sent derived interaction class");
  require(
      derivedReceived.tag == derivedTagBytes,
      "derived subscription did not receive the sent interaction tag");
  require(
      derivedReceived.parameters.size() == 2U &&
          derivedReceived.parameters.count(derivedBaseParameter) == 1U &&
          derivedReceived.parameters.count(derivedParameter) == 1U,
      "derived subscription did not receive inherited and derived parameters");
  require(
      copyBytes(derivedReceived.parameters.at(derivedBaseParameter)) == baseValue &&
          copyBytes(derivedReceived.parameters.at(derivedParameter)) == derivedValue,
      "derived subscription received changed inherited or derived parameter values");

  baseSubscriber.recorder().clearInteraction();
  derivedSubscriber.recorder().clearInteraction();
  rti::ParameterHandleValueMap baseParameters;
  baseParameters.emplace(
      baseParameter,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  std::vector<std::uint8_t> baseTagBytes{0x49U, 0x48U, 0x42U, 0x31U};
  rti::VariableLengthData baseTag(baseTagBytes.data(), baseTagBytes.size());
  publisher.rtiAmbassador().sendInteraction(baseInteraction, baseParameters, baseTag);

  waitFor(
      baseSubscriber,
      [&] { return baseSubscriber.recorder().interactions().size() >= 1U; },
      options,
      "base interaction subscription receiving a base interaction");
  for (int pass = 0; pass != 8; ++pass) {
    derivedSubscriber.pump();
  }
  auto const baseReceivedBase = baseSubscriber.recorder().interactions();
  require(
      baseReceivedBase.size() == 1U &&
          baseReceivedBase.front().interaction == baseInteraction &&
          baseReceivedBase.front().tag == baseTagBytes &&
          baseReceivedBase.front().parameters.size() == 1U &&
          baseReceivedBase.front().parameters.count(baseParameter) == 1U &&
          copyBytes(baseReceivedBase.front().parameters.at(baseParameter)) == baseValue,
      "base interaction subscription did not receive the base interaction parameters");
  require(
      derivedSubscriber.recorder().interactions().empty(),
      "derived-only interaction subscription received a base-class interaction");

  baseSubscriber.rtiAmbassador().unsubscribeInteractionClass(baseInteraction);
  derivedSubscriber.rtiAmbassador().unsubscribeInteractionClass(derivedInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(derivedInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(baseInteraction);
  baseSubscriber.resign(rti::NO_ACTION);
  derivedSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  derivedSubscriber.disconnect();
  baseSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioInheritedInteractionClassDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInheritedInteractionClassDelivery(options, model);
}

void scenarioTimestampedInheritedInteractionClassDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Timestamped inherited interaction delivery requires an adapter-supplied model FOM");

  Session publisher(options, model, "timestamped-interaction-hierarchy-publisher");
  Session baseSubscriber(options, model, "timestamped-interaction-hierarchy-base-subscriber");
  Session derivedSubscriber(
      options,
      model,
      "timestamped-interaction-hierarchy-derived-subscriber");
  auto const federation = federationName(
      options,
      "timestamped-inherited-interaction-class-delivery");
  connectAndJoin(
      publisher,
      baseSubscriber,
      options,
      federation,
      options.modelFom);
  derivedSubscriber.connect();
  derivedSubscriber.join(
      options.memberFederateName + L"-timestamped-derived-subscriber",
      options.federateType,
      federation);

  auto const baseInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const derivedInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  auto const baseSubscriberBaseInteraction =
      baseSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.typedInteractionClassName);
  auto const derivedSubscriberDerivedInteraction =
      derivedSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.typedDerivedInteractionClassName);
  require(
      baseInteraction.isValid() && derivedInteraction.isValid() &&
          baseSubscriberBaseInteraction == baseInteraction &&
          derivedSubscriberDerivedInteraction == derivedInteraction,
      "timestamped interaction hierarchy handles were invalid or unstable across members");

  auto const baseParameter = publisher.rtiAmbassador().getParameterHandle(
      baseInteraction,
      options.typedIntegerParameterName);
  auto const derivedBaseParameter = publisher.rtiAmbassador().getParameterHandle(
      derivedInteraction,
      options.typedIntegerParameterName);
  auto const derivedParameter = publisher.rtiAmbassador().getParameterHandle(
      derivedInteraction,
      options.typedDerivedParameterName);
  auto const baseSubscriberParameter = baseSubscriber.rtiAmbassador().getParameterHandle(
      baseSubscriberBaseInteraction,
      options.typedIntegerParameterName);
  auto const derivedSubscriberBaseParameter =
      derivedSubscriber.rtiAmbassador().getParameterHandle(
          derivedSubscriberDerivedInteraction,
          options.typedIntegerParameterName);
  auto const derivedSubscriberParameter =
      derivedSubscriber.rtiAmbassador().getParameterHandle(
          derivedSubscriberDerivedInteraction,
          options.typedDerivedParameterName);
  require(
      baseParameter.isValid() && derivedBaseParameter == baseParameter &&
          derivedParameter.isValid() && baseSubscriberParameter == baseParameter &&
          derivedSubscriberBaseParameter == baseParameter &&
          derivedSubscriberParameter == derivedParameter,
      "timestamped inherited interaction parameter handles were invalid or unstable");

  publisher.rtiAmbassador().publishInteractionClass(derivedInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      derivedInteraction,
      rti::TIMESTAMP);
  baseSubscriber.rtiAmbassador().subscribeInteractionClass(
      baseSubscriberBaseInteraction,
      true);
  derivedSubscriber.rtiAmbassador().subscribeInteractionClass(
      derivedSubscriberDerivedInteraction,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto baseSubscriberTime = makeTimeContext(baseSubscriber);
  auto derivedSubscriberTime = makeTimeContext(derivedSubscriber);
  require(
      publisherTime.factory->getName() == baseSubscriberTime.factory->getName() &&
          publisherTime.factory->getName() == derivedSubscriberTime.factory->getName(),
      "timestamped interaction hierarchy selected different logical-time factories");
  baseSubscriber.rtiAmbassador().enableTimeConstrained();
  waitFor(
      baseSubscriber,
      [&] { return baseSubscriber.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "base subscriber time-constrained callback for inherited timestamped interaction");
  derivedSubscriber.rtiAmbassador().enableTimeConstrained();
  waitFor(
      derivedSubscriber,
      [&] { return derivedSubscriber.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "derived subscriber time-constrained callback for inherited timestamped interaction");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "time-regulation callback for inherited timestamped interaction");

  auto const sentTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const baseSubscriberTimeAdvance = timeAfter(
      *baseSubscriberTime.factory,
      *baseSubscriberTime.initial,
      *baseSubscriberTime.epsilon,
      4U);
  auto const derivedSubscriberTimeAdvance = timeAfter(
      *derivedSubscriberTime.factory,
      *derivedSubscriberTime.initial,
      *derivedSubscriberTime.epsilon,
      4U);
  auto const sentTimeBytes = encodeTime(*sentTime);
  require(
      sentTimeBytes == encodeTime(*baseSubscriberTimeAdvance) &&
          sentTimeBytes == encodeTime(*derivedSubscriberTimeAdvance),
      "timestamped interaction and recipient advances selected different logical times");

  auto const baseValue = encodeDataElement(rti::HLAinteger32BE{0x13579bdf});
  auto const derivedValue = encodeDataElement(rti::HLAunicodeString{L"timestamped-derived"});
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      derivedBaseParameter,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  parameters.emplace(
      derivedParameter,
      rti::VariableLengthData(derivedValue.data(), derivedValue.size()));
  std::vector<std::uint8_t> tagBytes{0x54U, 0x53U, 0x4fU, 0x31U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      derivedInteraction,
      parameters,
      tag,
      *sentTime);
  require(
      retraction.isValid(),
      "timestamped inherited interaction send returned an invalid retraction handle");

  for (int pass = 0; pass != 8; ++pass) {
    baseSubscriber.pump();
    derivedSubscriber.pump();
  }
  require(
      baseSubscriber.recorder().timedInteractions().empty() &&
          derivedSubscriber.recorder().timedInteractions().empty(),
      "time-constrained interaction hierarchy received a timestamped send before advancing");

  baseSubscriber.rtiAmbassador().timeAdvanceRequest(*baseSubscriberTimeAdvance);
  derivedSubscriber.rtiAmbassador().timeAdvanceRequest(*derivedSubscriberTimeAdvance);
  publisher.rtiAmbassador().timeAdvanceRequest(*sentTime);
  waitFor(
      baseSubscriber,
      derivedSubscriber,
      [&] {
        return baseSubscriber.recorder().timedInteractions().size() >= 1U &&
            baseSubscriber.recorder().timeAdvanceGrants().size() >= 1U &&
            derivedSubscriber.recorder().timedInteractions().size() >= 1U &&
            derivedSubscriber.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped inherited interaction delivery and recipient grants");
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "publisher time-advance grant for inherited timestamped interaction");
  for (int pass = 0; pass != 8; ++pass) {
    baseSubscriber.pump();
    derivedSubscriber.pump();
  }

  auto const baseReceived = baseSubscriber.recorder().timedInteractions();
  auto const derivedReceived = derivedSubscriber.recorder().timedInteractions();
  require(
      baseReceived.size() == 1U && derivedReceived.size() == 1U &&
          baseSubscriber.recorder().timeAdvanceGrants().size() == 1U &&
          derivedSubscriber.recorder().timeAdvanceGrants().size() == 1U &&
          publisher.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped derived interaction was not delivered once per subscription and grant");

  auto const assertTimestampedMetadata = [&](TimedInteractionRecord const& received,
                                            Session& subscriber,
                                            std::string const& description) {
    require(
        received.tag == tagBytes && received.time == sentTimeBytes &&
            !received.timeText.empty(),
        description + " did not preserve timestamp or user tag");
    require(
        received.sentOrder == rti::TIMESTAMP &&
            received.receivedOrder == rti::TIMESTAMP,
        description + " returned the wrong sent or received order");
    require(
        received.producer == publisher.federateHandle(),
        description + " returned the wrong producing federate");
    require(
        received.transportation.isValid() &&
            !subscriber.rtiAmbassador().getTransportationTypeName(received.transportation).empty(),
        description + " returned an unresolvable transportation type");
    require(
        received.retractionPresent &&
            received.retraction == copyBytes(retraction.encode()),
        description + " returned the wrong timestamped retraction designator");
  };

  require(
      baseReceived.front().interaction == baseInteraction,
      "base subscription did not report its received class for the derived timestamped send");
  require(
      baseReceived.front().parameters.size() == 1U &&
          baseReceived.front().parameters.count(baseParameter) == 1U &&
          baseReceived.front().parameters.count(derivedParameter) == 0U &&
          copyBytes(baseReceived.front().parameters.at(baseParameter)) == baseValue,
      "base subscription did not project timestamped delivery to base parameters");
  assertTimestampedMetadata(
      baseReceived.front(),
      baseSubscriber,
      "base-subscription timestamped interaction callback");

  require(
      derivedReceived.front().interaction == derivedInteraction,
      "derived subscription did not report the sent class for timestamped delivery");
  require(
      derivedReceived.front().parameters.size() == 2U &&
          derivedReceived.front().parameters.count(derivedBaseParameter) == 1U &&
          derivedReceived.front().parameters.count(derivedParameter) == 1U &&
          copyBytes(derivedReceived.front().parameters.at(derivedBaseParameter)) == baseValue &&
          copyBytes(derivedReceived.front().parameters.at(derivedParameter)) == derivedValue,
      "derived subscription did not receive inherited and derived timestamped parameters");
  assertTimestampedMetadata(
      derivedReceived.front(),
      derivedSubscriber,
      "derived-subscription timestamped interaction callback");

  baseSubscriber.rtiAmbassador().disableTimeConstrained();
  derivedSubscriber.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  baseSubscriber.rtiAmbassador().unsubscribeInteractionClass(baseInteraction);
  derivedSubscriber.rtiAmbassador().unsubscribeInteractionClass(derivedInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(derivedInteraction);
  baseSubscriber.resign(rti::NO_ACTION);
  derivedSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  derivedSubscriber.disconnect();
  baseSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedInheritedInteractionClassDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInheritedInteractionClassDelivery(options, model);
}

void scenarioDirectedDerivedObjectTargetDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Directed derived-object target delivery requires an adapter-supplied model FOM");

  Session publisher(options, model, "directed-derived-target-publisher");
  Session targetedSubscriber(options, model, "directed-derived-target-subscriber");
  Session unsubscribedObserver(options, model, "directed-derived-target-observer");
  auto const federation = federationName(options, "directed-derived-object-target");
  connectAndJoin(
      publisher,
      targetedSubscriber,
      options,
      federation,
      options.modelFom);
  unsubscribedObserver.connect();
  unsubscribedObserver.join(
      options.memberFederateName + L"-directed-derived-target-observer",
      options.federateType,
      federation);

  auto const publisherObjectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const targetObjectClass = targetedSubscriber.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const observerObjectClass = unsubscribedObserver.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  require(
      publisherObjectClass.isValid() &&
          targetObjectClass == publisherObjectClass &&
          observerObjectClass == publisherObjectClass,
      "directed derived-object class handles were invalid or unstable across members");

  auto const publisherIdentity = publisher.rtiAmbassador().getAttributeHandle(
      publisherObjectClass,
      options.typedIdentityAttributeName);
  auto const targetIdentity = targetedSubscriber.rtiAmbassador().getAttributeHandle(
      targetObjectClass,
      options.typedIdentityAttributeName);
  auto const observerIdentity = unsubscribedObserver.rtiAmbassador().getAttributeHandle(
      observerObjectClass,
      options.typedIdentityAttributeName);
  require(
      publisherIdentity.isValid() &&
          targetIdentity == publisherIdentity &&
          observerIdentity == publisherIdentity,
      "inherited target identity-attribute handles were invalid or unstable");

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const targetInteraction = targetedSubscriber.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  require(
      publisherInteraction.isValid() && targetInteraction == publisherInteraction,
      "directed interaction-class handles were invalid or unstable across members");
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.typedIntegerParameterName);
  auto const targetParameter = targetedSubscriber.rtiAmbassador().getParameterHandle(
      targetInteraction,
      options.typedIntegerParameterName);
  require(
      publisherParameter.isValid() && targetParameter == publisherParameter,
      "directed interaction parameter handles were invalid or unstable across members");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherIdentity);
  rti::AttributeHandleSet targetAttributes;
  targetAttributes.insert(targetIdentity);
  rti::AttributeHandleSet observerAttributes;
  observerAttributes.insert(observerIdentity);
  rti::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(publisherInteraction);
  rti::InteractionClassHandleSet targetDirectedInteractions;
  targetDirectedInteractions.insert(targetInteraction);

  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  targetedSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      targetObjectClass,
      targetAttributes,
      true,
      L"");
  unsubscribedObserver.rtiAmbassador().subscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherObjectClass,
      directedInteractions);
  targetedSubscriber.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      targetObjectClass,
      targetDirectedInteractions,
      true);

  auto const target = publisher.rtiAmbassador().registerObjectInstance(
      publisherObjectClass);
  waitFor(
      targetedSubscriber,
      [&] { return targetedSubscriber.recorder().hasDiscovery(target); },
      options,
      "directed subscriber discovery of derived target object");
  waitFor(
      unsubscribedObserver,
      [&] { return unsubscribedObserver.recorder().hasDiscovery(target); },
      options,
      "unsubscribed observer discovery of derived target object");
  for (Session* subscriber : {&targetedSubscriber, &unsubscribedObserver}) {
    auto const discovery = subscriber->recorder().discovery();
    require(
        subscriber->recorder().discoveryCount() == 1U &&
            discovery.object == target &&
            discovery.objectClass == publisherObjectClass &&
            discovery.producer == publisher.federateHandle(),
        "directed derived-object subscriber received incorrect target discovery");
  }

  auto const value = encodeDataElement(rti::HLAinteger32BE{0x2468ace0});
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(value.data(), value.size()));
  std::vector<std::uint8_t> tagBytes{0x44U, 0x44U, 0x4fU, 0x31U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag);
  waitFor(
      targetedSubscriber,
      [&] { return targetedSubscriber.recorder().directedInteractions().size() >= 1U; },
      options,
      "directed interaction delivery to derived target object");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    targetedSubscriber.pump();
    unsubscribedObserver.pump();
  }

  auto const received = targetedSubscriber.recorder().directedInteractions();
  require(
      received.size() == 1U &&
          received.front().interaction == targetInteraction &&
          received.front().object == target &&
          received.front().parameters.size() == 1U &&
          received.front().parameters.count(targetParameter) == 1U &&
          copyBytes(received.front().parameters.at(targetParameter)) == value &&
          received.front().tag == tagBytes &&
          received.front().producer == publisher.federateHandle(),
      "directed interaction to derived target returned incorrect class, object, payload, or metadata");
  require(
      received.front().transportation.isValid() &&
          !targetedSubscriber.rtiAmbassador()
               .getTransportationTypeName(received.front().transportation)
               .empty(),
      "directed interaction to derived target returned an unresolvable transportation type");
  require(
      unsubscribedObserver.recorder().directedInteractions().empty(),
      "observer without a directed subscription received the derived-target interaction");

  targetedSubscriber.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      targetObjectClass,
      targetDirectedInteractions);
  targetedSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      targetObjectClass,
      targetAttributes);
  unsubscribedObserver.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes);
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherObjectClass,
      directedInteractions);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  targetedSubscriber.resign(rti::NO_ACTION);
  unsubscribedObserver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  unsubscribedObserver.disconnect();
  targetedSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioDirectedDerivedObjectTargetDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDirectedDerivedObjectTargetDelivery(options, model);
}

void scenarioDirectedDerivedObjectTargetEligibility(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Directed derived-object target eligibility requires an adapter-supplied model FOM");

  Session publisher(options, model, "directed-derived-eligibility-publisher");
  Session targetOwner(options, model, "directed-derived-eligibility-owner");
  Session universalSubscriber(options, model, "directed-derived-eligibility-universal");
  Session attributeOnlyObserver(options, model, "directed-derived-eligibility-observer");
  auto const federation = federationName(
      options,
      "directed-derived-object-target-eligibility");
  connectAndJoin(
      publisher,
      targetOwner,
      options,
      federation,
      options.modelFom);
  universalSubscriber.connect();
  universalSubscriber.join(
      options.memberFederateName + L"-directed-derived-eligibility-universal",
      options.federateType,
      federation);
  attributeOnlyObserver.connect();
  attributeOnlyObserver.join(
      options.memberFederateName + L"-directed-derived-eligibility-observer",
      options.federateType,
      federation);

  auto const publisherObjectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const ownerObjectClass = targetOwner.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const universalObjectClass = universalSubscriber.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const observerObjectClass = attributeOnlyObserver.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  require(
      publisherObjectClass.isValid() &&
          ownerObjectClass == publisherObjectClass &&
          universalObjectClass == publisherObjectClass &&
          observerObjectClass == publisherObjectClass,
      "directed derived-object eligibility class handles were invalid or unstable");

  auto const publisherIdentity = publisher.rtiAmbassador().getAttributeHandle(
      publisherObjectClass,
      options.typedIdentityAttributeName);
  auto const ownerIdentity = targetOwner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass,
      options.typedIdentityAttributeName);
  auto const universalIdentity = universalSubscriber.rtiAmbassador().getAttributeHandle(
      universalObjectClass,
      options.typedIdentityAttributeName);
  auto const observerIdentity = attributeOnlyObserver.rtiAmbassador().getAttributeHandle(
      observerObjectClass,
      options.typedIdentityAttributeName);
  require(
      publisherIdentity.isValid() &&
          ownerIdentity == publisherIdentity &&
          universalIdentity == publisherIdentity &&
          observerIdentity == publisherIdentity,
      "directed derived-object inherited attribute handles were invalid or unstable");

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const ownerInteraction = targetOwner.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const universalInteraction = universalSubscriber.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  require(
      publisherInteraction.isValid() &&
          ownerInteraction == publisherInteraction &&
          universalInteraction == publisherInteraction,
      "directed derived-object interaction handles were invalid or unstable");
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.typedIntegerParameterName);
  auto const ownerParameter = targetOwner.rtiAmbassador().getParameterHandle(
      ownerInteraction,
      options.typedIntegerParameterName);
  auto const universalParameter = universalSubscriber.rtiAmbassador().getParameterHandle(
      universalInteraction,
      options.typedIntegerParameterName);
  require(
      publisherParameter.isValid() &&
          ownerParameter == publisherParameter &&
          universalParameter == publisherParameter,
      "directed derived-object interaction parameter handles were invalid or unstable");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherIdentity);
  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(ownerIdentity);
  rti::AttributeHandleSet universalAttributes;
  universalAttributes.insert(universalIdentity);
  rti::AttributeHandleSet observerAttributes;
  observerAttributes.insert(observerIdentity);
  rti::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(publisherInteraction);
  rti::InteractionClassHandleSet ownerDirectedInteractions;
  ownerDirectedInteractions.insert(ownerInteraction);
  rti::InteractionClassHandleSet universalDirectedInteractions;
  universalDirectedInteractions.insert(universalInteraction);

  targetOwner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes,
      true,
      L"");
  universalSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      universalObjectClass,
      universalAttributes,
      true,
      L"");
  attributeOnlyObserver.rtiAmbassador().subscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherObjectClass,
      directedInteractions);
  targetOwner.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      ownerObjectClass,
      ownerDirectedInteractions,
      false);
  universalSubscriber.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      universalObjectClass,
      universalDirectedInteractions,
      true);

  auto const target = targetOwner.rtiAmbassador().registerObjectInstance(
      ownerObjectClass);
  require(
      target.isValid(),
      "directed derived-object eligibility registration returned an invalid handle");
  for (Session* subscriber : {&publisher, &universalSubscriber, &attributeOnlyObserver}) {
    waitFor(
        *subscriber,
        [&] { return subscriber->recorder().hasDiscovery(target); },
        options,
        "directed derived-object eligibility target discovery");
    auto const discovery = subscriber->recorder().discovery();
    require(
        subscriber->recorder().discoveryCount() == 1U &&
            discovery.object == target &&
            discovery.objectClass == ownerObjectClass &&
            discovery.producer == targetOwner.federateHandle(),
        "directed derived-object eligibility discovery returned incorrect metadata");
  }

  auto const value = encodeDataElement(rti::HLAinteger32BE{0x10203040});
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(value.data(), value.size()));
  std::vector<std::uint8_t> tagBytes{0x44U, 0x44U, 0x45U, 0x31U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag);
  waitFor(
      targetOwner,
      [&] { return targetOwner.recorder().directedInteractions().size() >= 1U; },
      options,
      "directed interaction delivery to the derived target owner");
  waitFor(
      universalSubscriber,
      [&] { return universalSubscriber.recorder().directedInteractions().size() >= 1U; },
      options,
      "universal directed delivery to the derived target");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    targetOwner.pump();
    universalSubscriber.pump();
    attributeOnlyObserver.pump();
  }

  auto const assertDelivery = [&](auto const& record,
                                 rti::InteractionClassHandle expectedInteraction,
                                 rti::ParameterHandle expectedParameter,
                                 auto& receiver) {
    require(
        record.interaction == expectedInteraction &&
            record.object == target &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == value &&
            record.tag == tagBytes &&
            record.producer == publisher.federateHandle(),
        "directed derived-object eligibility callback returned incorrect target or payload");
    require(
        record.transportation.isValid() &&
            !receiver.rtiAmbassador()
                 .getTransportationTypeName(record.transportation)
                 .empty(),
        "directed derived-object eligibility callback returned an unresolvable transportation type");
  };
  auto const ownerDeliveries = targetOwner.recorder().directedInteractions();
  require(
      ownerDeliveries.size() == 1U,
      "derived target owner did not receive exactly one directed interaction");
  assertDelivery(
      ownerDeliveries.front(),
      ownerInteraction,
      ownerParameter,
      targetOwner);
  auto const universalDeliveries = universalSubscriber.recorder().directedInteractions();
  require(
      universalDeliveries.size() == 1U,
      "universal subscriber did not receive exactly one derived-target interaction");
  assertDelivery(
      universalDeliveries.front(),
      universalInteraction,
      universalParameter,
      universalSubscriber);
  require(
      publisher.recorder().directedInteractions().empty(),
      "directed interaction publisher received a copy without a target-role subscription");
  require(
      attributeOnlyObserver.recorder().directedInteractions().empty(),
      "attribute-only observer received a directed interaction for the derived target");

  targetOwner.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      ownerObjectClass,
      ownerDirectedInteractions);
  universalSubscriber.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      universalObjectClass,
      universalDirectedInteractions);
  publisher.rtiAmbassador().unsubscribeObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  universalSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      universalObjectClass,
      universalAttributes);
  attributeOnlyObserver.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes);
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherObjectClass,
      directedInteractions);
  targetOwner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  attributeOnlyObserver.resign(rti::NO_ACTION);
  universalSubscriber.resign(rti::NO_ACTION);
  targetOwner.resign(rti::DELETE_OBJECTS);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  attributeOnlyObserver.disconnect();
  universalSubscriber.disconnect();
  targetOwner.disconnect();
  publisher.disconnect();
}

void scenarioDirectedDerivedObjectTargetEligibilityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDirectedDerivedObjectTargetEligibility(options, model);
}

void scenarioTimestampedDirectedDerivedObjectTargetDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Timestamped directed derived-object delivery requires an adapter-supplied model FOM");

  Session publisher(options, model, "timestamped-directed-derived-target-publisher");
  Session targetedSubscriber(options, model, "timestamped-directed-derived-target-subscriber");
  Session unsubscribedObserver(options, model, "timestamped-directed-derived-target-observer");
  auto const federation = federationName(
      options,
      "timestamped-directed-derived-object-target");
  connectAndJoin(
      publisher,
      targetedSubscriber,
      options,
      federation,
      options.modelFom);
  unsubscribedObserver.connect();
  unsubscribedObserver.join(
      options.memberFederateName + L"-timestamped-directed-derived-target-observer",
      options.federateType,
      federation);

  auto const publisherObjectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const targetObjectClass = targetedSubscriber.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const observerObjectClass = unsubscribedObserver.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  require(
      publisherObjectClass.isValid() &&
          targetObjectClass == publisherObjectClass &&
          observerObjectClass == publisherObjectClass,
      "timestamped directed derived-object class handles were invalid or unstable");

  auto const publisherIdentity = publisher.rtiAmbassador().getAttributeHandle(
      publisherObjectClass,
      options.typedIdentityAttributeName);
  auto const targetIdentity = targetedSubscriber.rtiAmbassador().getAttributeHandle(
      targetObjectClass,
      options.typedIdentityAttributeName);
  auto const observerIdentity = unsubscribedObserver.rtiAmbassador().getAttributeHandle(
      observerObjectClass,
      options.typedIdentityAttributeName);
  require(
      publisherIdentity.isValid() &&
          targetIdentity == publisherIdentity &&
          observerIdentity == publisherIdentity,
      "timestamped directed inherited identity-attribute handles were invalid or unstable");

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const targetInteraction = targetedSubscriber.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  require(
      publisherInteraction.isValid() && targetInteraction == publisherInteraction,
      "timestamped directed interaction-class handles were invalid or unstable");
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.typedIntegerParameterName);
  auto const targetParameter = targetedSubscriber.rtiAmbassador().getParameterHandle(
      targetInteraction,
      options.typedIntegerParameterName);
  require(
      publisherParameter.isValid() && targetParameter == publisherParameter,
      "timestamped directed interaction parameter handles were invalid or unstable");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherIdentity);
  rti::AttributeHandleSet targetAttributes;
  targetAttributes.insert(targetIdentity);
  rti::AttributeHandleSet observerAttributes;
  observerAttributes.insert(observerIdentity);
  rti::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(publisherInteraction);
  rti::InteractionClassHandleSet targetDirectedInteractions;
  targetDirectedInteractions.insert(targetInteraction);

  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  targetedSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      targetObjectClass,
      targetAttributes,
      true,
      L"");
  unsubscribedObserver.rtiAmbassador().subscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherObjectClass,
      directedInteractions);
  targetedSubscriber.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      targetObjectClass,
      targetDirectedInteractions,
      true);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);

  auto const target = publisher.rtiAmbassador().registerObjectInstance(
      publisherObjectClass);
  require(
      target.isValid(),
      "timestamped directed derived-object registration returned an invalid handle");
  waitFor(
      targetedSubscriber,
      [&] { return targetedSubscriber.recorder().hasDiscovery(target); },
      options,
      "timestamped directed subscriber discovery of the derived target");
  waitFor(
      unsubscribedObserver,
      [&] { return unsubscribedObserver.recorder().hasDiscovery(target); },
      options,
      "timestamped directed observer discovery of the derived target");
  for (Session* subscriber : {&targetedSubscriber, &unsubscribedObserver}) {
    auto const discovery = subscriber->recorder().discovery();
    require(
        subscriber->recorder().discoveryCount() == 1U &&
            discovery.object == target &&
            discovery.objectClass == publisherObjectClass &&
            discovery.producer == publisher.federateHandle(),
        "timestamped directed derived-object discovery returned incorrect metadata");
  }

  auto publisherTime = makeTimeContext(publisher);
  auto targetTime = makeTimeContext(targetedSubscriber);
  require(
      publisherTime.factory->getName() == targetTime.factory->getName(),
      "timestamped directed derived-object members selected different logical-time factories");
  auto publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "timestamped directed derived-target could not allocate a lookahead interval");
  publisherLookahead->setToDifference(*publisherLookaheadTime, *publisherTime.initial);
  targetedSubscriber.rtiAmbassador().enableTimeConstrained();
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);

  auto waitForTimeState = [&](auto predicate, std::string const& description) {
    auto const deadline = Clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline) {
      publisher.pump();
      targetedSubscriber.pump();
      unsubscribedObserver.pump();
      if (predicate()) {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("Timed out waiting for " + description);
  };
  waitForTimeState(
      [&] {
        return publisher.recorder().timeRegulationEnabled().size() >= 1U &&
            targetedSubscriber.recorder().timeConstrainedEnabled().size() >= 1U;
      },
      "timestamped directed derived-target time-role callbacks");
  require(
      publisher.recorder().timeRegulationEnabled().size() == 1U &&
          targetedSubscriber.recorder().timeConstrainedEnabled().size() == 1U,
      "timestamped directed derived-target time-role callback was delivered more than once");

  auto sentTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto targetAdvanceTime = timeAfter(
      *targetTime.factory,
      *targetTime.initial,
      *targetTime.epsilon,
      7U);
  auto publisherAdvanceTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const expectedTime = encodeTime(*sentTime);
  auto const expectedGrantTime = encodeTime(*targetAdvanceTime);
  require(
      expectedTime == encodeTime(*timeAfter(
                          *targetTime.factory,
                          *targetTime.initial,
                          *targetTime.epsilon,
                          7U)),
      "timestamped directed derived-target logical times did not align across members");

  auto const value = encodeDataElement(rti::HLAinteger32BE{0x13579bdf});
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(value.data(), value.size()));
  std::vector<std::uint8_t> tagBytes{0x54U, 0x44U, 0x44U, 0x4fU, 0x31U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag,
      *sentTime);
  require(
      retraction.isValid(),
      "timestamped directed derived-target send returned an invalid retraction handle");

  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    targetedSubscriber.pump();
    unsubscribedObserver.pump();
  }
  require(
      targetedSubscriber.recorder().timedDirectedInteractions().empty(),
      "timestamped directed derived-target callback arrived before time advance");
  require(
      unsubscribedObserver.recorder().timedDirectedInteractions().empty(),
      "observer without a directed subscription received the timestamped derived-target interaction");

  targetedSubscriber.rtiAmbassador().timeAdvanceRequest(*targetAdvanceTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvanceTime);
  waitForTimeState(
      [&] {
        return targetedSubscriber.recorder().timedDirectedInteractions().size() >= 1U &&
            targetedSubscriber.recorder().timeAdvanceGrants().size() >= 1U &&
            publisher.recorder().timeAdvanceGrants().size() >= 1U;
      },
      "timestamped directed derived-target delivery and time advance grants");

  auto const received = targetedSubscriber.recorder().timedDirectedInteractions();
  require(
      received.size() == 1U,
      "timestamped directed derived-target subscriber received an unexpected delivery count");
  auto const& record = received.front();
  require(
      record.interaction == targetInteraction &&
          record.object == target &&
          record.parameters.size() == 1U &&
          record.parameters.count(targetParameter) == 1U &&
          copyBytes(record.parameters.at(targetParameter)) == value &&
          record.tag == tagBytes &&
          record.producer == publisher.federateHandle(),
      "timestamped directed derived-target callback returned incorrect class, object, payload, or metadata");
  require(
      record.transportation.isValid() &&
          !targetedSubscriber.rtiAmbassador()
               .getTransportationTypeName(record.transportation)
               .empty(),
      "timestamped directed derived-target callback returned an unresolvable transportation type");
  require(
      record.time == expectedTime && !record.timeText.empty() &&
          record.sentOrder == rti::TIMESTAMP &&
          record.receivedOrder == rti::TIMESTAMP,
      "timestamped directed derived-target callback returned incorrect time or order metadata");
  require(
      targetedSubscriber.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "timestamped directed derived-target callback arrived after its time advance grant");
  require(
      record.retractionPresent &&
          record.retraction == copyBytes(retraction.encode()),
      "timestamped directed derived-target callback returned the wrong retraction handle");
  auto const grants = targetedSubscriber.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == expectedGrantTime &&
          publisher.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped directed derived-target TAR returned incorrect or duplicate grant metadata");
  require(
      unsubscribedObserver.recorder().timedDirectedInteractions().empty(),
      "observer without a directed subscription received the timestamped derived-target interaction");

  targetedSubscriber.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  targetedSubscriber.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      targetObjectClass,
      targetDirectedInteractions);
  targetedSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      targetObjectClass,
      targetAttributes);
  unsubscribedObserver.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes);
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherObjectClass,
      directedInteractions);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  targetedSubscriber.resign(rti::NO_ACTION);
  unsubscribedObserver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  unsubscribedObserver.disconnect();
  targetedSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDirectedDerivedObjectTargetDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedDerivedObjectTargetDelivery(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
