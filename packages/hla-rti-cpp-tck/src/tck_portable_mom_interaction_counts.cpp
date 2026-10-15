#include "tck_portable_mom_interaction_counts.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomInteractionsReceivedAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Joined-federate MOM interactions-received attribute testing requires an adapter-supplied FOM and standard MIM");
  require(
      !options.interactionClassName.empty() && !options.parameterName.empty(),
      "Joined-federate MOM interactions-received attribute testing requires adapter-supplied interaction and parameter names");

  Session receiver(options, model, "member");
  Session publisher(options, model, "owner");
  auto const federation = federationName(
      options,
      "joined-federate-mom-interactions-received-mim-attribute");
  receiver.connect();
  publisher.connect();
  if (options.logicalTimeImplementationName.empty()) {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  receiver.join(
      options.memberFederateName + L"-mom-interactions-received-receiver",
      options.federateType,
      federation);
  auto const momClass = receiver.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const interactionsReceivedAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAinteractionsReceived");
  auto const reliable = receiver.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          interactionsReceivedAttribute.isValid() && reliable.isValid() &&
          receiver.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          receiver.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          receiver.rtiAmbassador().getAttributeName(
              momClass,
              interactionsReceivedAttribute) == L"HLAinteractionsReceived" &&
          receiver.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM interactions-received lookup did not round-trip the standard MIM handles");
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          interactionsReceivedAttribute},
      true,
      L"");

  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      receiverInteraction.isValid() && receiverParameter.isValid() &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Joined-federate MOM interactions-received application lookup did not round-trip");
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);

  publisher.join(
      options.ownerFederateName + L"-mom-interactions-received-publisher",
      options.federateType,
      federation);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "Joined-federate MOM interactions-received application handles differed between federates");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);

  auto findMomObject = [&]() -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = receiver.recorder().reflections();
    auto const expectedHandle = copyBytes(receiver.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      receiver,
      [&] { return findMomObject().has_value(); },
      options,
      "joined-federate MOM interactions-received object discovery and reflection");
  auto const receiverMomObject = findMomObject();
  require(
      receiverMomObject.has_value() &&
          receiver.rtiAmbassador().getKnownObjectClassHandle(
              *receiverMomObject) == momClass,
      "Joined-federate MOM interactions-received lookup found the wrong receiver object");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *receiverMomObject &&
              reflection.values.count(interactionsReceivedAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.count(interactionsReceivedAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(interactionsReceivedAttribute));
    require(value.get() >= 0, description + " returned a negative interaction count");
    return value.get();
  };
  auto requestCount = [&](std::string const& description) {
    auto const before = receiver.recorder().reflections().size();
    receiver.rtiAmbassador().requestAttributeValueUpdate(
        *receiverMomObject,
        rti::AttributeHandleSet{interactionsReceivedAttribute},
        rti::VariableLengthData{});
    waitFor(
        receiver,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no interactions-received value");
    return decodeCount(*reflection, description);
  };

  auto countApplicationInteractions = [&](std::size_t firstInteraction,
                                          std::vector<std::uint8_t> const& payload) {
    auto const interactions = receiver.recorder().interactions();
    auto const begin = std::min(firstInteraction, interactions.size());
    return static_cast<std::size_t>(std::count_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(begin),
        interactions.end(),
        [&](InteractionRecord const& interaction) {
          auto const value = interaction.parameters.find(receiverParameter);
          return interaction.present && interaction.interaction == receiverInteraction &&
              interaction.producer == publisher.federateHandle() &&
              value != interaction.parameters.end() &&
              copyBytes(value->second) == payload && interaction.tag.empty() &&
              !interaction.regions.has_value();
        }));
  };

  auto const setTiming = receiver.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          receiver.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          receiver.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          receiver.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM interactions-received HLAsetTiming lookup did not round-trip");
  auto sendSetTiming = [&](std::int32_t period) {
    receiver.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, receiver.federateHandle().encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  std::vector<std::uint8_t> const firstPayload{0x49U, 0x52U, 0x01U};
  std::vector<std::uint8_t> const periodicPayload{0x49U, 0x52U, 0x02U};
  auto sendApplicationInteraction = [&](std::vector<std::uint8_t> const& payload) {
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        rti::ParameterHandleValueMap{{
            publisherParameter,
            rti::VariableLengthData(payload.data(), payload.size())}},
        rti::VariableLengthData{});
  };

  require(
      requestCount("Joined-federate MOM initial interactions-received value") == 0,
      "Joined-federate MOM initial interactions-received value was not zero");

  std::size_t const directInteractionStart =
      receiver.recorder().interactions().size();
  for (std::size_t message = 0; message != 3U; ++message) {
    sendApplicationInteraction(firstPayload);
  }
  waitFor(
      receiver,
      [&] {
        return countApplicationInteractions(directInteractionStart, firstPayload) == 3U;
      },
      options,
      "joined-federate MOM direct interactions-received application callbacks");
  require(
      requestCount("Joined-federate MOM direct interactions-received value") == 3,
      "Joined-federate MOM direct interactions-received value did not match delivered callbacks");

  std::size_t const periodicInteractionStart =
      receiver.recorder().interactions().size();
  std::size_t const periodicReflectionStart =
      receiver.recorder().reflections().size();
  sendSetTiming(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  for (std::size_t message = 0; message != 2U; ++message) {
    sendApplicationInteraction(periodicPayload);
  }
  waitFor(
      receiver,
      [&] {
        return countApplicationInteractions(periodicInteractionStart, periodicPayload) == 2U;
      },
      options,
      "joined-federate MOM periodic interactions-received application callbacks");

  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() + static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *receiverMomObject ||
          iterator->values.count(interactionsReceivedAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(interactionsReceivedAttribute));
      if (value.get() >= 5) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      receiver,
      [&] { return findPeriodicCount().has_value(); },
      options,
      "joined-federate MOM periodic interactions-received value");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(*periodicReflection,
                      "Joined-federate MOM periodic interactions-received reflection") == 5,
      "Joined-federate MOM periodic interactions-received value did not match delivered callbacks");

  sendSetTiming(0);
  require(
      requestCount("Joined-federate MOM final interactions-received value") == 5,
      "Joined-federate MOM final interactions-received value changed after reporting was disabled");
  require(
      countApplicationInteractions(directInteractionStart, firstPayload) == 3U &&
          countApplicationInteractions(periodicInteractionStart, periodicPayload) == 2U,
      "Joined-federate MOM interactions-received phases delivered unexpected application callback counts");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          interactionsReceivedAttribute});
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
  publisher.rtiAmbassador().resignFederationExecution(
      rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioJoinedFederateMomInteractionsReceivedAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomInteractionsReceivedAttribute(options, model);
}

void scenarioJoinedFederateMomInteractionsSentAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Joined-federate MOM interactions-sent attribute testing requires an adapter-supplied FOM and standard MIM");
  require(
      !options.interactionClassName.empty() && !options.parameterName.empty(),
      "Joined-federate MOM interactions-sent attribute testing requires adapter-supplied interaction and parameter names");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "joined-federate-mom-interactions-sent-mim-attribute");
  publisher.connect();
  receiver.connect();
  if (options.logicalTimeImplementationName.empty()) {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  publisher.join(
      options.ownerFederateName + L"-mom-interactions-sent-publisher",
      options.federateType,
      federation);
  auto const momClass = publisher.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = publisher.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const interactionsSentAttribute = publisher.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAinteractionsSent");
  auto const reliable = publisher.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          interactionsSentAttribute.isValid() && reliable.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              interactionsSentAttribute) == L"HLAinteractionsSent" &&
          publisher.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM interactions-sent lookup did not round-trip the standard MIM handles");
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          interactionsSentAttribute},
      true,
      L"");

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          publisher.rtiAmbassador().getInteractionClassName(publisherInteraction) ==
              options.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == options.parameterName,
      "Joined-federate MOM interactions-sent application lookup did not round-trip");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);

  receiver.join(
      options.memberFederateName + L"-mom-interactions-sent-receiver",
      options.federateType,
      federation);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      receiverInteraction == publisherInteraction &&
          receiverParameter == publisherParameter,
      "Joined-federate MOM interactions-sent application handles differed between federates");
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);

  auto findMomObject = [&]() -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = publisher.recorder().reflections();
    auto const expectedHandle = copyBytes(publisher.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      publisher,
      [&] { return findMomObject().has_value(); },
      options,
      "joined-federate MOM interactions-sent object discovery and reflection");
  auto const publisherMomObject = findMomObject();
  require(
      publisherMomObject.has_value() &&
          publisher.rtiAmbassador().getKnownObjectClassHandle(
              *publisherMomObject) == momClass,
      "Joined-federate MOM interactions-sent lookup found the wrong publisher object");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *publisherMomObject &&
              reflection.values.count(interactionsSentAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.count(interactionsSentAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(interactionsSentAttribute));
    require(value.get() >= 0, description + " returned a negative interaction count");
    return value.get();
  };
  auto requestCount = [&](std::string const& description) {
    auto const before = publisher.recorder().reflections().size();
    publisher.rtiAmbassador().requestAttributeValueUpdate(
        *publisherMomObject,
        rti::AttributeHandleSet{interactionsSentAttribute},
        rti::VariableLengthData{});
    waitFor(
        publisher,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no interactions-sent value");
    return decodeCount(*reflection, description);
  };

  auto countApplicationInteractions = [&](std::size_t firstInteraction,
                                          std::vector<std::uint8_t> const& payload) {
    auto const interactions = receiver.recorder().interactions();
    auto const begin = std::min(firstInteraction, interactions.size());
    return static_cast<std::size_t>(std::count_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(begin),
        interactions.end(),
        [&](InteractionRecord const& interaction) {
          auto const value = interaction.parameters.find(receiverParameter);
          return interaction.present && interaction.interaction == receiverInteraction &&
              interaction.producer == publisher.federateHandle() &&
              value != interaction.parameters.end() &&
              copyBytes(value->second) == payload && interaction.tag.empty() &&
              !interaction.regions.has_value();
        }));
  };

  auto const setTiming = publisher.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = publisher.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = publisher.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          publisher.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          publisher.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          publisher.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM interactions-sent HLAsetTiming lookup did not round-trip");
  auto sendSetTiming = [&](std::int32_t period) {
    publisher.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, publisher.federateHandle().encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  std::vector<std::uint8_t> const payload{0x49U, 0x53U, 0x01U};
  auto sendApplicationInteraction = [&] {
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        rti::ParameterHandleValueMap{{
            publisherParameter,
            rti::VariableLengthData(payload.data(), payload.size())}},
        rti::VariableLengthData{});
  };

  require(
      requestCount("Joined-federate MOM initial interactions-sent value") == 0,
      "Joined-federate MOM initial interactions-sent value was not zero");
  sendSetTiming(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto const directBaseline = requestCount(
      "Joined-federate MOM interactions-sent baseline with periodic reporting enabled");
  auto const periodicReflectionStart =
      publisher.recorder().reflections().size();
  std::size_t const interactionStart = receiver.recorder().interactions().size();
  for (std::size_t message = 0; message != 3U; ++message) {
    sendApplicationInteraction();
  }
  waitFor(
      receiver,
      [&] {
        return countApplicationInteractions(interactionStart, payload) == 3U;
      },
      options,
      "joined-federate MOM interactions-sent application deliveries");
  auto const expectedCount = directBaseline + 3;

  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() + static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *publisherMomObject ||
          iterator->values.count(interactionsSentAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(interactionsSentAttribute));
      if (value.get() >= expectedCount) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      publisher,
      [&] { return findPeriodicCount().has_value(); },
      options,
      "joined-federate MOM periodic interactions-sent value");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(*periodicReflection,
                      "Joined-federate MOM periodic interactions-sent reflection") == expectedCount,
      "Joined-federate MOM periodic interactions-sent value did not match the direct service count");
  require(
      requestCount("Joined-federate MOM direct interactions-sent value") == expectedCount,
      "Joined-federate MOM direct interactions-sent value did not increase once per accepted application send");

  sendSetTiming(0);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          interactionsSentAttribute});
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().resignFederationExecution(
      rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  receiver.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  receiver.disconnect();
}

void scenarioJoinedFederateMomInteractionsSentAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomInteractionsSentAttribute(options, model);
}

void scenarioJoinedFederateMomDirectedInteractionsReceivedAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Joined-federate MOM directed-interactions-received testing requires an adapter-supplied FOM and standard MIM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty() &&
          !options.interactionClassName.empty() && !options.parameterName.empty(),
      "Joined-federate MOM directed-interactions-received testing requires adapter-supplied object, attribute, interaction, and parameter names");

  Session receiver(options, model, "member");
  Session publisher(options, model, "owner");
  auto const federation = federationName(
      options,
      "joined-federate-mom-directed-interactions-received-mim-attribute");
  receiver.connect();
  publisher.connect();
  if (options.logicalTimeImplementationName.empty()) {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  receiver.join(
      options.memberFederateName +
          L"-mom-directed-interactions-received-receiver",
      options.federateType,
      federation);
  auto const momClass = receiver.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const interactionsReceivedAttribute =
      receiver.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAinteractionsReceived");
  auto const directedInteractionsReceivedAttribute =
      receiver.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAdirectedInteractionsReceived");
  auto const reliable = receiver.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          interactionsReceivedAttribute.isValid() &&
          directedInteractionsReceivedAttribute.isValid() && reliable.isValid() &&
          receiver.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          receiver.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          receiver.rtiAmbassador().getAttributeName(
              momClass,
              interactionsReceivedAttribute) == L"HLAinteractionsReceived" &&
          receiver.rtiAmbassador().getAttributeName(
              momClass,
              directedInteractionsReceivedAttribute) ==
              L"HLAdirectedInteractionsReceived" &&
          receiver.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM directed-interactions-received lookup did not round-trip standard MIM handles");
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          interactionsReceivedAttribute,
          directedInteractionsReceivedAttribute},
      true,
      L"");

  auto const receiverObjectClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverObjectAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverObjectClass,
      options.attributeName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      receiverObjectClass.isValid() && receiverObjectAttribute.isValid() &&
          receiverInteraction.isValid() && receiverParameter.isValid() &&
          receiver.rtiAmbassador().getObjectClassName(receiverObjectClass) ==
              options.objectClassName &&
          receiver.rtiAmbassador().getAttributeName(
              receiverObjectClass,
              receiverObjectAttribute) == options.attributeName &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Joined-federate MOM directed-interactions-received application lookups did not round-trip");
  rti::AttributeHandleSet const receiverObjectAttributes{
      receiverObjectAttribute};
  rti::InteractionClassHandleSet const receiverDirectedInteractions{
      receiverInteraction};
  receiver.rtiAmbassador().publishObjectClassAttributes(
      receiverObjectClass,
      receiverObjectAttributes);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedInteractions);

  publisher.join(
      options.ownerFederateName +
          L"-mom-directed-interactions-received-publisher",
      options.federateType,
      federation);
  auto const publisherObjectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherObjectAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherObjectClass,
      options.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      publisherObjectClass == receiverObjectClass &&
          publisherObjectAttribute == receiverObjectAttribute &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter &&
          publisher.rtiAmbassador().getObjectClassName(publisherObjectClass) ==
              options.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherObjectClass,
              publisherObjectAttribute) == options.attributeName &&
          publisher.rtiAmbassador().getInteractionClassName(publisherInteraction) ==
              options.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == options.parameterName,
      "Joined-federate MOM directed-interactions-received application handles differed between federates");
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherObjectClass,
      rti::AttributeHandleSet{publisherObjectAttribute},
      true,
      L"");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherObjectClass,
      rti::InteractionClassHandleSet{publisherInteraction});

  auto const target = receiver.rtiAmbassador().registerObjectInstance(
      receiverObjectClass);
  require(
      target.isValid(),
      "Joined-federate MOM directed-interactions-received target registration returned an invalid object handle");
  waitFor(
      publisher,
      [&] { return publisher.recorder().hasDiscovery(target); },
      options,
      "joined-federate MOM directed-interactions-received target discovery");

  auto findMomObject = [&]() -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = receiver.recorder().reflections();
    auto const expectedHandle = copyBytes(receiver.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      receiver,
      [&] { return findMomObject().has_value(); },
      options,
      "joined-federate MOM directed-interactions-received object discovery");
  auto const receiverMomObject = findMomObject();
  require(
      receiverMomObject.has_value() &&
          receiver.rtiAmbassador().getKnownObjectClassHandle(
              *receiverMomObject) == momClass,
      "Joined-federate MOM directed-interactions-received lookup found the wrong receiver object");

  auto findCountReflection = [&](std::size_t firstReflection,
                                 rti::AttributeHandle const& attribute)
      -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *receiverMomObject &&
              reflection.values.count(attribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         rti::AttributeHandle const& attribute,
                         std::string const& description) {
    require(
        reflection.values.count(attribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(attribute));
    require(value.get() >= 0, description + " returned a negative interaction count");
    return value.get();
  };
  auto requestCount = [&](rti::AttributeHandle const& attribute,
                          std::string const& description) {
    auto const before = receiver.recorder().reflections().size();
    receiver.rtiAmbassador().requestAttributeValueUpdate(
        *receiverMomObject,
        rti::AttributeHandleSet{attribute},
        rti::VariableLengthData{});
    waitFor(
        receiver,
        [&] { return findCountReflection(before, attribute).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before, attribute);
    require(reflection.has_value(), description + " produced no scalar value");
    return decodeCount(*reflection, attribute, description);
  };

  auto countOrdinaryCallbacks = [&](std::size_t firstInteraction,
                                    std::vector<std::uint8_t> const& payload) {
    auto const interactions = receiver.recorder().interactions();
    auto const begin = std::min(firstInteraction, interactions.size());
    return static_cast<std::size_t>(std::count_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(begin),
        interactions.end(),
        [&](InteractionRecord const& interaction) {
          auto const value = interaction.parameters.find(receiverParameter);
          return interaction.present && interaction.interaction == receiverInteraction &&
              interaction.producer == publisher.federateHandle() &&
              value != interaction.parameters.end() &&
              copyBytes(value->second) == payload && interaction.tag.empty() &&
              interaction.transportation == reliable;
        }));
  };
  auto countDirectedCallbacks = [&](std::size_t firstInteraction,
                                    std::vector<std::uint8_t> const& payload) {
    auto const interactions = receiver.recorder().directedInteractions();
    auto const begin = std::min(firstInteraction, interactions.size());
    return static_cast<std::size_t>(std::count_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(begin),
        interactions.end(),
        [&](DirectedInteractionRecord const& interaction) {
          auto const value = interaction.parameters.find(receiverParameter);
          return interaction.interaction == receiverInteraction &&
              interaction.object == target &&
              interaction.producer == publisher.federateHandle() &&
              value != interaction.parameters.end() &&
              copyBytes(value->second) == payload && interaction.tag.empty() &&
              interaction.transportation == reliable;
        }));
  };

  auto const setTiming = receiver.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          receiver.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          receiver.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          receiver.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM directed-interactions-received HLAsetTiming lookup did not round-trip");
  auto sendSetTiming = [&](std::int32_t period) {
    receiver.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, receiver.federateHandle().encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  std::vector<std::uint8_t> const ordinaryPayload{0xA1U};
  std::vector<std::uint8_t> const directPayload{0xD1U};
  std::vector<std::uint8_t> const periodicPayload{0xD2U};
  auto makeParameters = [&](std::vector<std::uint8_t> const& payload) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(payload.data(), payload.size()));
    return parameters;
  };
  auto sendOrdinaryInteraction = [&](std::vector<std::uint8_t> const& payload) {
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        makeParameters(payload),
        rti::VariableLengthData{});
  };
  auto sendDirectedInteraction = [&](std::vector<std::uint8_t> const& payload) {
    publisher.rtiAmbassador().sendDirectedInteraction(
        publisherInteraction,
        target,
        makeParameters(payload),
        rti::VariableLengthData{});
  };

  require(
      requestCount(
          interactionsReceivedAttribute,
          "Joined-federate MOM initial interactions-received value") == 0 &&
          requestCount(
              directedInteractionsReceivedAttribute,
              "Joined-federate MOM initial directed-interactions-received value") == 0,
      "Joined-federate MOM interaction receive counters were not initially zero");

  std::size_t const ordinaryStart = receiver.recorder().interactions().size();
  sendOrdinaryInteraction(ordinaryPayload);
  waitFor(
      receiver,
      [&] { return countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) >= 1U; },
      options,
      "joined-federate MOM ordinary interaction callback before directed delivery");
  require(
      countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) == 1U &&
          receiver.recorder().interactions().size() == ordinaryStart + 1U &&
          receiver.recorder().directedInteractions().empty(),
      "Joined-federate MOM ordinary interaction did not stay on the ordinary callback route");
  require(
      requestCount(
          interactionsReceivedAttribute,
          "Joined-federate MOM ordinary interactions-received value") == 1 &&
          requestCount(
              directedInteractionsReceivedAttribute,
              "Joined-federate MOM directed count after ordinary delivery") == 0,
      "Joined-federate MOM directed-interactions-received included an ordinary callback");

  std::size_t const directStart = receiver.recorder().directedInteractions().size();
  for (std::size_t message = 0; message != 3U; ++message) {
    sendDirectedInteraction(directPayload);
  }
  waitFor(
      receiver,
      [&] { return countDirectedCallbacks(directStart, directPayload) >= 3U; },
      options,
      "joined-federate MOM direct directed-interaction callbacks");
  require(
      countDirectedCallbacks(directStart, directPayload) == 3U &&
          receiver.recorder().directedInteractions().size() == directStart + 3U &&
          countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) == 1U &&
          receiver.recorder().interactions().size() == ordinaryStart + 1U,
      "Joined-federate MOM direct directed sends returned duplicate or ordinary callbacks");
  require(
      requestCount(
          interactionsReceivedAttribute,
          "Joined-federate MOM total value after direct directed delivery") == 4 &&
          requestCount(
              directedInteractionsReceivedAttribute,
              "Joined-federate MOM direct directed-interactions-received value") == 3,
      "Joined-federate MOM direct counters did not separate ordinary and directed callbacks");

  std::size_t const periodicStart = receiver.recorder().directedInteractions().size();
  std::size_t const periodicReflectionStart =
      receiver.recorder().reflections().size();
  sendSetTiming(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  for (std::size_t message = 0; message != 2U; ++message) {
    sendDirectedInteraction(periodicPayload);
  }
  waitFor(
      receiver,
      [&] { return countDirectedCallbacks(periodicStart, periodicPayload) >= 2U; },
      options,
      "joined-federate MOM periodic directed-interaction callbacks");
  require(
      countDirectedCallbacks(periodicStart, periodicPayload) == 2U &&
          receiver.recorder().directedInteractions().size() == periodicStart + 2U &&
          countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) == 1U &&
          receiver.recorder().interactions().size() == ordinaryStart + 1U,
      "Joined-federate MOM periodic directed sends returned duplicate or ordinary callbacks");

  auto findPeriodicCount = [&](rti::AttributeHandle const& attribute,
                               std::int32_t minimum)
      -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() + static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *receiverMomObject ||
          iterator->values.count(attribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(attribute));
      if (value.get() >= minimum) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  auto awaitPeriodicCount = [&](rti::AttributeHandle const& attribute,
                                std::int32_t expected,
                                std::string const& description) {
    waitFor(
        receiver,
        [&] { return findPeriodicCount(attribute, expected).has_value(); },
        options,
        description);
    auto const reflection = findPeriodicCount(attribute, expected);
    require(reflection.has_value(), description + " produced no periodic value");
    require(
        decodeCount(*reflection, attribute, description) == expected,
        description + " did not match delivered callbacks");
  };
  awaitPeriodicCount(
      directedInteractionsReceivedAttribute,
      5,
      "joined-federate MOM periodic directed-interactions-received value");
  awaitPeriodicCount(
      interactionsReceivedAttribute,
      6,
      "joined-federate MOM periodic interactions-received value");

  sendSetTiming(0);
  require(
      requestCount(
          interactionsReceivedAttribute,
          "Joined-federate MOM final interactions-received value") == 6 &&
          requestCount(
              directedInteractionsReceivedAttribute,
              "Joined-federate MOM final directed-interactions-received value") == 5,
      "Joined-federate MOM receive counters changed after periodic reporting was disabled");

  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedInteractions);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          interactionsReceivedAttribute,
          directedInteractionsReceivedAttribute});
  publisher.rtiAmbassador().unsubscribeObjectClassAttributes(
      publisherObjectClass,
      rti::AttributeHandleSet{publisherObjectAttribute});
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherObjectClass,
      rti::InteractionClassHandleSet{publisherInteraction});
  receiver.rtiAmbassador().unpublishObjectClassAttributes(
      receiverObjectClass,
      receiverObjectAttributes);
  receiver.rtiAmbassador().resignFederationExecution(
      rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().resignFederationExecution(
      rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioJoinedFederateMomDirectedInteractionsReceivedAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomDirectedInteractionsReceivedAttribute(options, model);
}

void scenarioJoinedFederateMomDirectedInteractionsSentAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Joined-federate MOM directed-interactions-sent testing requires an adapter-supplied FOM and standard MIM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty() &&
          !options.interactionClassName.empty() && !options.parameterName.empty(),
      "Joined-federate MOM directed-interactions-sent testing requires adapter-supplied object, attribute, interaction, and parameter names");

  Session sender(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "joined-federate-mom-directed-interactions-sent-mim-attribute");
  sender.connect();
  receiver.connect();
  if (options.logicalTimeImplementationName.empty()) {
    sender.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    sender.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  sender.join(
      options.ownerFederateName +
          L"-mom-directed-interactions-sent-sender",
      options.federateType,
      federation);
  auto const momClass = sender.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = sender.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const interactionsSentAttribute = sender.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAinteractionsSent");
  auto const directedInteractionsSentAttribute =
      sender.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAdirectedInteractionsSent");
  auto const reliable = sender.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          interactionsSentAttribute.isValid() &&
          directedInteractionsSentAttribute.isValid() && reliable.isValid() &&
          sender.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          sender.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          sender.rtiAmbassador().getAttributeName(
              momClass,
              interactionsSentAttribute) == L"HLAinteractionsSent" &&
          sender.rtiAmbassador().getAttributeName(
              momClass,
              directedInteractionsSentAttribute) ==
              L"HLAdirectedInteractionsSent" &&
          sender.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM directed-interactions-sent lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      interactionsSentAttribute,
      directedInteractionsSentAttribute};
  sender.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  receiver.join(
      options.memberFederateName +
          L"-mom-directed-interactions-sent-receiver",
      options.federateType,
      federation);
  auto const senderObjectClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverObjectClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderObjectAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderObjectClass,
      options.attributeName);
  auto const receiverObjectAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverObjectClass,
      options.attributeName);
  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      senderObjectClass.isValid() && receiverObjectClass == senderObjectClass &&
          senderObjectAttribute.isValid() &&
          receiverObjectAttribute == senderObjectAttribute &&
          senderInteraction.isValid() && receiverInteraction == senderInteraction &&
          senderParameter.isValid() && receiverParameter == senderParameter &&
          sender.rtiAmbassador().getObjectClassName(senderObjectClass) ==
              options.objectClassName &&
          receiver.rtiAmbassador().getObjectClassName(receiverObjectClass) ==
              options.objectClassName &&
          sender.rtiAmbassador().getAttributeName(
              senderObjectClass,
              senderObjectAttribute) == options.attributeName &&
          receiver.rtiAmbassador().getAttributeName(
              receiverObjectClass,
              receiverObjectAttribute) == options.attributeName &&
          sender.rtiAmbassador().getInteractionClassName(senderInteraction) ==
              options.interactionClassName &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName &&
          sender.rtiAmbassador().getParameterName(
              senderInteraction,
              senderParameter) == options.parameterName &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Joined-federate MOM directed-interactions-sent application lookups did not round-trip");

  rti::AttributeHandleSet const senderObjectAttributes{senderObjectAttribute};
  rti::AttributeHandleSet const receiverObjectAttributes{receiverObjectAttribute};
  rti::InteractionClassHandleSet const senderDirectedInteractions{
      senderInteraction};
  rti::InteractionClassHandleSet const receiverDirectedInteractions{
      receiverInteraction};
  receiver.rtiAmbassador().publishObjectClassAttributes(
      receiverObjectClass,
      receiverObjectAttributes);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedInteractions);
  sender.rtiAmbassador().subscribeObjectClassAttributes(
      senderObjectClass,
      senderObjectAttributes,
      true,
      L"");
  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  sender.rtiAmbassador().publishObjectClassDirectedInteractions(
      senderObjectClass,
      senderDirectedInteractions);

  auto const target = receiver.rtiAmbassador().registerObjectInstance(
      receiverObjectClass);
  require(
      target.isValid(),
      "Joined-federate MOM directed-interactions-sent target registration returned an invalid object handle");
  waitFor(
      sender,
      [&] { return sender.recorder().hasDiscovery(target); },
      options,
      "joined-federate MOM directed-interactions-sent target discovery");

  auto findMomObject = [&]() -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = sender.recorder().reflections();
    auto const expectedHandle = copyBytes(sender.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      sender,
      [&] { return findMomObject().has_value(); },
      options,
      "joined-federate MOM directed-interactions-sent object discovery");
  auto const senderMomObject = findMomObject();
  require(
      senderMomObject.has_value() &&
          sender.rtiAmbassador().getKnownObjectClassHandle(
              *senderMomObject) == momClass,
      "Joined-federate MOM directed-interactions-sent lookup found the wrong sender object");

  auto findCountReflection = [&](std::size_t firstReflection,
                                 rti::AttributeHandle const& attribute)
      -> std::optional<ReflectionRecord> {
    auto const reflections = sender.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *senderMomObject &&
              reflection.values.count(attribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         rti::AttributeHandle const& attribute,
                         std::string const& description) {
    require(
        reflection.values.count(attribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(attribute));
    require(value.get() >= 0, description + " returned a negative interaction count");
    return value.get();
  };
  auto requestCount = [&](rti::AttributeHandle const& attribute,
                          std::string const& description) {
    auto const before = sender.recorder().reflections().size();
    sender.rtiAmbassador().requestAttributeValueUpdate(
        *senderMomObject,
        rti::AttributeHandleSet{attribute},
        rti::VariableLengthData{});
    waitFor(
        sender,
        [&] { return findCountReflection(before, attribute).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before, attribute);
    require(reflection.has_value(), description + " produced no scalar value");
    return decodeCount(*reflection, attribute, description);
  };

  auto countOrdinaryCallbacks = [&](std::size_t firstInteraction,
                                    std::vector<std::uint8_t> const& payload) {
    auto const interactions = receiver.recorder().interactions();
    auto const begin = std::min(firstInteraction, interactions.size());
    return static_cast<std::size_t>(std::count_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(begin),
        interactions.end(),
        [&](InteractionRecord const& interaction) {
          auto const value = interaction.parameters.find(receiverParameter);
          return interaction.present && interaction.interaction == receiverInteraction &&
              interaction.producer == sender.federateHandle() &&
              value != interaction.parameters.end() &&
              copyBytes(value->second) == payload && interaction.tag.empty() &&
              interaction.transportation == reliable;
        }));
  };
  auto countDirectedCallbacks = [&](std::size_t firstInteraction,
                                    std::vector<std::uint8_t> const& payload) {
    auto const interactions = receiver.recorder().directedInteractions();
    auto const begin = std::min(firstInteraction, interactions.size());
    return static_cast<std::size_t>(std::count_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(begin),
        interactions.end(),
        [&](DirectedInteractionRecord const& interaction) {
          auto const value = interaction.parameters.find(receiverParameter);
          return interaction.interaction == receiverInteraction &&
              interaction.object == target &&
              interaction.producer == sender.federateHandle() &&
              value != interaction.parameters.end() &&
              copyBytes(value->second) == payload && interaction.tag.empty() &&
              interaction.transportation == reliable;
        }));
  };

  auto const setTiming = sender.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = sender.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = sender.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          sender.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          sender.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          sender.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM directed-interactions-sent HLAsetTiming lookup did not round-trip");
  auto sendSetTiming = [&](std::int32_t period) {
    sender.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, sender.federateHandle().encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  std::vector<std::uint8_t> const ordinaryPayload{0xA2U};
  std::vector<std::uint8_t> const directPayload{0xD3U};
  std::vector<std::uint8_t> const periodicPayload{0xD4U};
  auto makeParameters = [&](std::vector<std::uint8_t> const& payload) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        senderParameter,
        rti::VariableLengthData(payload.data(), payload.size()));
    return parameters;
  };
  auto sendOrdinaryInteraction = [&](std::vector<std::uint8_t> const& payload) {
    sender.rtiAmbassador().sendInteraction(
        senderInteraction,
        makeParameters(payload),
        rti::VariableLengthData{});
  };
  auto sendDirectedInteraction = [&](std::vector<std::uint8_t> const& payload) {
    sender.rtiAmbassador().sendDirectedInteraction(
        senderInteraction,
        target,
        makeParameters(payload),
        rti::VariableLengthData{});
  };

  require(
      requestCount(
          interactionsSentAttribute,
          "Joined-federate MOM initial interactions-sent value") == 0 &&
          requestCount(
              directedInteractionsSentAttribute,
              "Joined-federate MOM initial directed-interactions-sent value") == 0,
      "Joined-federate MOM interaction-send counters were not initially zero");
  sendSetTiming(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto const totalBaseline = requestCount(
      interactionsSentAttribute,
      "Joined-federate MOM interactions-sent baseline with periodic reporting enabled");
  auto const directedBaseline = requestCount(
      directedInteractionsSentAttribute,
      "Joined-federate MOM directed-interactions-sent baseline with periodic reporting enabled");
  require(
      directedBaseline == 0,
      "Joined-federate MOM periodic-report setup changed the directed-send baseline");

  std::size_t const ordinaryStart = receiver.recorder().interactions().size();
  sendOrdinaryInteraction(ordinaryPayload);
  waitFor(
      receiver,
      [&] { return countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) >= 1U; },
      options,
      "joined-federate MOM ordinary send before directed delivery");
  require(
      countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) == 1U &&
          receiver.recorder().interactions().size() == ordinaryStart + 1U &&
          receiver.recorder().directedInteractions().empty(),
      "Joined-federate MOM ordinary send did not stay on the ordinary callback route");
  require(
      requestCount(
          interactionsSentAttribute,
          "Joined-federate MOM total after ordinary Send Interaction") ==
              totalBaseline + 1 &&
          requestCount(
              directedInteractionsSentAttribute,
              "Joined-federate MOM directed count after ordinary Send Interaction") ==
              directedBaseline,
      "Joined-federate MOM directed-interactions-sent included an ordinary send");

  std::size_t const directStart = receiver.recorder().directedInteractions().size();
  for (std::size_t message = 0; message != 3U; ++message) {
    sendDirectedInteraction(directPayload);
  }
  waitFor(
      receiver,
      [&] { return countDirectedCallbacks(directStart, directPayload) >= 3U; },
      options,
      "joined-federate MOM direct directed-interaction deliveries");
  require(
      countDirectedCallbacks(directStart, directPayload) == 3U &&
          receiver.recorder().directedInteractions().size() == directStart + 3U &&
          countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) == 1U &&
          receiver.recorder().interactions().size() == ordinaryStart + 1U,
      "Joined-federate MOM direct directed sends returned duplicate or ordinary callbacks");
  require(
      requestCount(
          interactionsSentAttribute,
          "Joined-federate MOM total after direct directed sends") ==
              totalBaseline + 4 &&
          requestCount(
              directedInteractionsSentAttribute,
              "Joined-federate MOM direct directed-interactions-sent value") ==
              directedBaseline + 3,
      "Joined-federate MOM direct send counters did not separate ordinary and directed sends");

  auto const periodicTotalBaseline = requestCount(
      interactionsSentAttribute,
      "Joined-federate MOM total baseline before periodic directed sends");
  auto const periodicDirectedBaseline = requestCount(
      directedInteractionsSentAttribute,
      "Joined-federate MOM directed baseline before periodic directed sends");
  require(
      periodicDirectedBaseline == directedBaseline + 3,
      "Joined-federate MOM direct directed-send baseline changed before periodic sends");
  std::size_t const periodicReflectionStart =
      sender.recorder().reflections().size();
  std::size_t const periodicStart =
      receiver.recorder().directedInteractions().size();
  for (std::size_t message = 0; message != 2U; ++message) {
    sendDirectedInteraction(periodicPayload);
  }
  waitFor(
      receiver,
      [&] { return countDirectedCallbacks(periodicStart, periodicPayload) >= 2U; },
      options,
      "joined-federate MOM periodic directed-interaction deliveries");
  require(
      countDirectedCallbacks(periodicStart, periodicPayload) == 2U &&
          receiver.recorder().directedInteractions().size() == periodicStart + 2U &&
          countOrdinaryCallbacks(ordinaryStart, ordinaryPayload) == 1U &&
          receiver.recorder().interactions().size() == ordinaryStart + 1U,
      "Joined-federate MOM periodic directed sends returned duplicate or ordinary callbacks");
  auto const expectedPeriodicTotal = periodicTotalBaseline + 2;
  auto const expectedPeriodicDirected = periodicDirectedBaseline + 2;

  auto findPeriodicCount = [&](rti::AttributeHandle const& attribute,
                               std::int32_t minimum)
      -> std::optional<ReflectionRecord> {
    auto const reflections = sender.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() + static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *senderMomObject ||
          iterator->values.count(attribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(attribute));
      if (value.get() >= minimum) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  auto awaitPeriodicCount = [&](rti::AttributeHandle const& attribute,
                                std::int32_t expected,
                                std::string const& description) {
    waitFor(
        sender,
        [&] { return findPeriodicCount(attribute, expected).has_value(); },
        options,
        description);
    auto const reflection = findPeriodicCount(attribute, expected);
    require(reflection.has_value(), description + " produced no periodic value");
    require(
        decodeCount(*reflection, attribute, description) == expected,
        description + " did not match accepted send invocations");
  };
  awaitPeriodicCount(
      interactionsSentAttribute,
      expectedPeriodicTotal,
      "joined-federate MOM periodic interactions-sent value");
  awaitPeriodicCount(
      directedInteractionsSentAttribute,
      expectedPeriodicDirected,
      "joined-federate MOM periodic directed-interactions-sent value");
  require(
      requestCount(
          interactionsSentAttribute,
          "Joined-federate MOM direct total after periodic directed sends") ==
              expectedPeriodicTotal &&
          requestCount(
              directedInteractionsSentAttribute,
              "Joined-federate MOM direct directed count after periodic sends") ==
              expectedPeriodicDirected,
      "Joined-federate MOM direct counters did not increase once per accepted directed send");

  sendSetTiming(0);
  require(
      requestCount(
          directedInteractionsSentAttribute,
          "Joined-federate MOM final directed-interactions-sent value") ==
          expectedPeriodicDirected,
      "Joined-federate MOM directed-interactions-sent changed after periodic reporting was disabled");

  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedInteractions);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  sender.rtiAmbassador().unsubscribeObjectClassAttributes(
      senderObjectClass,
      senderObjectAttributes);
  sender.rtiAmbassador().unsubscribeObjectClassAttributes(momClass, momAttributes);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  sender.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      senderObjectClass,
      senderDirectedInteractions);
  receiver.rtiAmbassador().unpublishObjectClassAttributes(
      receiverObjectClass,
      receiverObjectAttributes);
  receiver.rtiAmbassador().resignFederationExecution(
      rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  sender.rtiAmbassador().resignFederationExecution(
      rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
  receiver.disconnect();
}

void scenarioJoinedFederateMomDirectedInteractionsSentAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomDirectedInteractionsSentAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
