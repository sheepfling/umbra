#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedAttributeUpdateOwnershipTransfer(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped ownership-transfer testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "timestamped-ownership-owner");
  Session acquirer(options, model, "timestamped-ownership-acquirer");
  auto const federation = federationName(options, "timestamped-attribute-ownership-transfer");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const acquirerClass = acquirer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const acquirerAttribute = acquirer.rtiAmbassador().getAttributeHandle(
      acquirerClass,
      options.attributeName);
  require(
      ownerClass.isValid() && acquirerClass.isValid() &&
          ownerAttribute.isValid() && acquirerAttribute.isValid(),
      "Timestamped ownership-transfer lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(
      acquirerClass,
      acquirerAttributes);
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass,
      acquirerAttributes,
      true,
      L"");
  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      ownerClass,
      ownerAttributes,
      rti::TIMESTAMP);

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Timestamped ownership-transfer registration returned an invalid handle");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().hasDiscovery(object); },
      options,
      "Timestamped ownership-transfer discovery");

  auto ownerTime = makeTimeContext(owner);
  auto acquirerTime = makeTimeContext(acquirer);
  require(
      ownerTime.factory->getName() == acquirerTime.factory->getName(),
      "Timestamped ownership-transfer members selected different logical-time factories");

  auto const lookaheadTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  auto lookahead = ownerTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped ownership-transfer could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *ownerTime.initial);

  acquirer.rtiAmbassador().enableTimeConstrained();
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped ownership-transfer time-constrained callback");
  owner.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped ownership-transfer time-regulation callback");

  auto const updateTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  std::vector<std::uint8_t> const valueBytes{0xBEU, 0xEFU};
  std::vector<std::uint8_t> const updateTagBytes{
      0x54U, 0x53U, 0x4FU, 0x2DU, 0x4FU, 0x57U, 0x4EU};
  std::vector<std::uint8_t> const divestitureTagBytes{0x44U, 0x49U, 0x56U};
  std::vector<std::uint8_t> const acquisitionTagBytes{0x41U, 0x43U, 0x51U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      ownerAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData updateTag(updateTagBytes.data(), updateTagBytes.size());
  rti::VariableLengthData divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  rti::VariableLengthData acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  auto const retraction = owner.rtiAmbassador().updateAttributeValues(
      object,
      values,
      updateTag,
      *updateTime);
  require(
      retraction.isValid(),
      "Timestamped ownership-transfer update returned an invalid retraction handle");
  require(
      acquirer.recorder().timedReflections().empty(),
      "Timestamped ownership-transfer update reflected before the constrained grant");

  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      divestitureTag);
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAssumption().has_value(); },
      options,
      "Timestamped ownership-transfer assumption callback");
  auto const assumption = acquirer.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes == acquirerAttributes &&
          assumption->tag == divestitureTagBytes,
      "Timestamped ownership-transfer assumption returned the wrong metadata");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
      "Timestamped ownership-transfer divestiture changed ownership before acquisition");

  acquirer.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      acquirerAttributes,
      acquisitionTag);
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
      options,
      "Timestamped ownership-transfer acquisition callback");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == acquirerAttributes &&
          acquisition->tag == acquisitionTagBytes,
      "Timestamped ownership-transfer acquisition returned the wrong metadata");
  require(
      acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
      "Timestamped ownership-transfer acquisition did not transfer ownership");

  auto const ownerTarget = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      2U);
  auto const acquirerTarget = timeAfter(
      *acquirerTime.factory,
      *acquirerTime.initial,
      *acquirerTime.epsilon,
      5U);
  acquirer.rtiAmbassador().timeAdvanceRequest(*acquirerTarget);
  owner.rtiAmbassador().timeAdvanceRequest(*ownerTarget);
  waitForSessions(
      {&owner, &acquirer},
      [&] {
        return owner.recorder().timeAdvanceGrants().size() >= 1U &&
            acquirer.recorder().timeAdvanceGrants().size() >= 1U &&
            acquirer.recorder().timedReflections().size() >= 1U;
      },
      options,
      "Timestamped ownership-transfer grant and reflection");

  auto const reflections = acquirer.recorder().timedReflections();
  require(
      reflections.size() == 1U,
      "Timestamped ownership-transfer delivered duplicate reflections");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(acquirerAttribute) == 1U &&
          copyBytes(reflection.values.at(acquirerAttribute)) == valueBytes,
      "Timestamped ownership-transfer reflection returned the wrong object or value");
  require(
      reflection.tag == updateTagBytes &&
          reflection.producer == owner.federateHandle() &&
          reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty(),
      "Timestamped ownership-transfer reflection changed value metadata");
  require(
      reflection.transportation.isValid() &&
          !acquirer.rtiAmbassador()
               .getTransportationTypeName(reflection.transportation)
               .empty(),
      "Timestamped ownership-transfer reflection returned an unknown transportation type");
  require(
      reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()),
      "Timestamped ownership-transfer reflection returned the wrong order or retraction");
  require(
      owner.recorder().timeAdvanceGrants().front().encoded == encodeTime(*ownerTarget) &&
          acquirer.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*acquirerTarget),
      "Timestamped ownership-transfer returned the wrong grant times");

  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  acquirer.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  acquirer.disconnect();
}

void scenarioTimestampedAttributeUpdateQueuedPasselRetraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Timestamped queued-passel testing requires an adapter-supplied multi-attribute FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped queued-passel testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "timestamped-queued-passel-publisher");
  Session receiver(options, model, "timestamped-queued-passel-receiver");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-queued-passel-retraction");
  connectAndJoin(
      publisher,
      receiver,
      options,
      federation,
      options.multiAttributeFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publisherFirst = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeFirstName);
  auto const publisherSecond = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeSecondName);
  auto const receiverFirst = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.multiAttributeFirstName);
  auto const receiverSecond = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.multiAttributeSecondName);
  require(
      publisherClass.isValid() && receiverClass.isValid(),
      "Timestamped queued-passel object-class lookup returned an invalid handle");
  require(
      publisherFirst.isValid() && publisherSecond.isValid() &&
          receiverFirst.isValid() && receiverSecond.isValid() &&
          publisherFirst != publisherSecond,
      "Timestamped queued-passel attribute lookup returned invalid or duplicate handles");

  rti::AttributeHandleSet const publisherAttributes{
      publisherFirst,
      publisherSecond};
  rti::AttributeHandleSet const receiverAttributes{
      receiverFirst,
      receiverSecond};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Timestamped queued-passel registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Timestamped queued-passel discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped queued-passel members selected different logical-time factories");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped queued-passel time-constrained callback");
  auto const lookaheadTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped queued-passel could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTarget, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped queued-passel time-regulation callback");

  auto const invalidUpdateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const firstUpdateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const secondUpdateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);

  std::vector<std::uint8_t> const firstValueBytes{0x11U, 0x22U};
  std::vector<std::uint8_t> const secondValueBytes{0x33U, 0x44U, 0x55U};
  std::vector<std::uint8_t> const tagBytes{0xA4U, 0x15U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherFirst,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  values.emplace(
      publisherSecond,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            object,
            values,
            tag,
            *invalidUpdateTime);
      },
      L"InvalidLogicalTime",
      "Timestamped queued-passel update below the producer lookahead");

  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *firstUpdateTime);
  require(
      firstRetraction.isValid(),
      "Timestamped queued-passel first update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "Timestamped queued-passel first update reflected before its grant");

  receiver.rtiAmbassador().timeAdvanceRequest(*firstUpdateTime);
  publisher.rtiAmbassador().retract(firstRetraction);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Timestamped queued-passel first grant");
  require(
      receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timedReflections().empty() &&
          receiver.recorder().retractions().empty(),
      "Timestamped queued-passel retraction leaked a first callback");

  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *secondUpdateTime);
  require(
      secondRetraction.isValid(),
      "Timestamped queued-passel second update returned an invalid retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*secondUpdateTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*secondUpdateTime);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return receiver.recorder().timedReflections().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "Timestamped queued-passel second delivery and grant");

  auto const reflections = receiver.recorder().timedReflections();
  require(
      !reflections.empty(),
      "Timestamped queued-passel returned no reflections");
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      callbackOrder.size() >= 3U && callbackOrder.front() == "grant" &&
          callbackOrder.back() == "grant" &&
          std::count(callbackOrder.begin(), callbackOrder.end(), "reflect") >= 1,
      "Timestamped queued-passel returned the wrong callback ordering");

  bool firstReported = false;
  bool secondReported = false;
  for (auto const& reflection : reflections) {
    require(
        reflection.object == object &&
            !reflection.values.empty() &&
            reflection.tag == tagBytes &&
            reflection.producer == publisher.federateHandle() &&
            reflection.time == encodeTime(*secondUpdateTime) &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP &&
            reflection.retractionPresent &&
            reflection.retraction == copyBytes(secondRetraction.encode()) &&
            reflection.transportation.isValid(),
        "Timestamped queued-passel reflection returned the wrong metadata");
    if (reflection.values.count(receiverFirst) == 1U) {
      require(
              copyBytes(reflection.values.at(receiverFirst)) == firstValueBytes,
          "Timestamped queued-passel first reflection returned the wrong value");
      require(
          !firstReported,
          "Timestamped queued-passel delivered the first attribute more than once");
      firstReported = true;
    }
    if (reflection.values.count(receiverSecond) == 1U) {
      require(
              copyBytes(reflection.values.at(receiverSecond)) == secondValueBytes,
          "Timestamped queued-passel second reflection returned the wrong value");
      require(
          !secondReported,
          "Timestamped queued-passel delivered the second attribute more than once");
      secondReported = true;
    }
    require(
        reflection.values.count(receiverFirst) == 1U ||
            reflection.values.count(receiverSecond) == 1U,
        "Timestamped queued-passel returned an unknown attribute passel");
  }
  require(
      firstReported && secondReported,
      "Timestamped queued-passel did not preserve both attribute values");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "Timestamped queued-passel delivered retraction reuse");

  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeSourceResignation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute source-resignation testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session survivor(options, model, "member");
  Session clock(options, model, "source-resignation-clock");
  auto const federation = federationName(
      options,
      "timestamped-attribute-source-resignation");
  connectAndJoin(owner, survivor, options, federation, options.fom);
  clock.connect();
  clock.join(
      options.memberFederateName + L"-timestamped-attribute-resignation-clock",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const survivorClass = survivor.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  require(
      ownerClass.isValid() && survivorClass.isValid() &&
          ownerAttribute.isValid() && survivorAttribute.isValid(),
      "Timestamped attribute source-resignation lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  survivor.rtiAmbassador().publishObjectClassAttributes(
      survivorClass,
      survivorAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");
  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      ownerClass,
      ownerAttributes,
      rti::TIMESTAMP);

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Timestamped attribute source-resignation registration returned an invalid object handle");
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "Timestamped attribute source-resignation discovery");

  auto ownerTime = makeTimeContext(owner);
  auto survivorTime = makeTimeContext(survivor);
  auto clockTime = makeTimeContext(clock);
  require(
      ownerTime.factory->getName() == survivorTime.factory->getName() &&
          ownerTime.factory->getName() == clockTime.factory->getName(),
      "Timestamped attribute source-resignation members selected different logical-time factories");

  auto makeLookahead = [](TimeContext const& time) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "Timestamped attribute source-resignation could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto ownerLookahead = makeLookahead(ownerTime);
  auto clockLookahead = makeLookahead(clockTime);

  survivor.rtiAmbassador().enableTimeConstrained();
  waitFor(
      survivor,
      [&] { return survivor.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped attribute source-resignation time-constrained callback");
  owner.rtiAmbassador().enableTimeRegulation(*ownerLookahead);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped attribute source-resignation owner time-regulation callback");
  clock.rtiAmbassador().enableTimeRegulation(*clockLookahead);
  waitFor(
      clock,
      [&] { return clock.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped attribute source-resignation clock time-regulation callback");

  auto const messageTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  auto const survivorTarget = timeAfter(
      *survivorTime.factory,
      *survivorTime.initial,
      *survivorTime.epsilon,
      5U);
  auto const clockTarget = timeAfter(
      *clockTime.factory,
      *clockTime.initial,
      *clockTime.epsilon,
      2U);
  std::vector<std::uint8_t> const valueBytes{0xCAU, 0xFEU};
  std::vector<std::uint8_t> const updateTagBytes{
      0x52U, 0x45U, 0x53U, 0x49U, 0x47U, 0x4EU};
  std::vector<std::uint8_t> const acquisitionTagBytes{
      0x41U, 0x43U, 0x51U, 0x2DU, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      ownerAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const updateTag(
      updateTagBytes.data(),
      updateTagBytes.size());
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  auto const retraction = owner.rtiAmbassador().updateAttributeValues(
      object,
      values,
      updateTag,
      *messageTime);
  require(
      retraction.isValid(),
      "Timestamped attribute source-resignation update returned an invalid retraction handle");
  require(
      survivor.recorder().timedReflections().empty(),
      "Timestamped attribute source-resignation update reflected before the survivor grant");

  auto const ownerFederate = owner.federateHandle();
  owner.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  waitFor(
      survivor,
      [&] { return survivor.recorder().ownershipAssumption().has_value(); },
      options,
      "Timestamped attribute source-resignation assumption callback");
  auto const assumption = survivor.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes.count(survivorAttribute) == 1U,
      "Timestamped attribute source-resignation assumption returned the wrong metadata");

  survivor.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      survivorAttributes,
      acquisitionTag);
  waitFor(
      survivor,
      [&] { return survivor.recorder().ownershipAcquisition().has_value(); },
      options,
      "Timestamped attribute source-resignation acquisition callback");
  auto const acquisition = survivor.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == survivorAttributes &&
          acquisition->tag == acquisitionTagBytes,
      "Timestamped attribute source-resignation acquisition returned the wrong metadata");
  require(
      survivor.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          survivorAttribute),
      "Timestamped attribute source-resignation acquisition did not transfer ownership");
  requireException(
      [&] { owner.rtiAmbassador().retract(retraction); },
      L"FederateNotExecutionMember",
      "Timestamped attribute source-resignation post-resignation retraction");

  survivor.recorder().clearCallbackOrder();
  survivor.rtiAmbassador().timeAdvanceRequest(*survivorTarget);
  clock.rtiAmbassador().timeAdvanceRequest(*clockTarget);
  waitForSessions(
      {&clock, &survivor},
      [&] {
        return clock.recorder().timeAdvanceGrants().size() >= 1U &&
            survivor.recorder().timeAdvanceGrants().size() >= 1U &&
            survivor.recorder().timedReflections().size() >= 1U;
      },
      options,
      "Timestamped attribute source-resignation delivery and grant");

  auto const reflections = survivor.recorder().timedReflections();
  require(
      reflections.size() == 1U,
      "Timestamped attribute source-resignation delivered duplicate reflections");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(survivorAttribute) == 1U &&
          copyBytes(reflection.values.at(survivorAttribute)) == valueBytes,
      "Timestamped attribute source-resignation reflection returned the wrong object or value");
  require(
      reflection.tag == updateTagBytes &&
          reflection.producer == ownerFederate &&
          reflection.time == encodeTime(*messageTime) &&
          !reflection.timeText.empty(),
      "Timestamped attribute source-resignation reflection changed value metadata");
  require(
      reflection.transportation.isValid() &&
          !survivor.rtiAmbassador()
               .getTransportationTypeName(reflection.transportation)
               .empty(),
      "Timestamped attribute source-resignation reflection returned an unknown transportation type");
  require(
      reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()),
      "Timestamped attribute source-resignation reflection returned the wrong order or retraction");
  require(
      survivor.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Timestamped attribute source-resignation reflected after its grant");
  require(
      clock.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*clockTarget) &&
          survivor.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*survivorTarget),
      "Timestamped attribute source-resignation returned the wrong grant times");

  survivor.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  clock.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  survivor.disconnect();
  clock.disconnect();
}

void scenarioTimestampedAttributeOrderCohort(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-order cohort testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session tara(options, model, "member");
  Session nmra(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-attribute-order-cohort");
  connectAndJoin(publisher, tara, options, federation, options.fom);
  nmra.connect();
  nmra.join(
      options.memberFederateName + L"-timestamped-attribute-order-cohort-peer",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const taraClass = tara.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const nmraClass = nmra.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const taraAttribute = tara.rtiAmbassador().getAttributeHandle(
      taraClass,
      options.attributeName);
  auto const nmraAttribute = nmra.rtiAmbassador().getAttributeHandle(
      nmraClass,
      options.attributeName);
  require(
      publisherClass.isValid() && taraClass.isValid() && nmraClass.isValid(),
      "Timestamped attribute-order cohort lookup returned an invalid object-class handle");
  require(
      publisherAttribute.isValid() && taraAttribute.isValid() &&
          nmraAttribute.isValid(),
      "Timestamped attribute-order cohort lookup returned an invalid attribute handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const taraAttributes{taraAttribute};
  rti::AttributeHandleSet const nmraAttributes{nmraAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  tara.rtiAmbassador().subscribeObjectClassAttributes(
      taraClass,
      taraAttributes,
      true,
      L"");
  nmra.rtiAmbassador().subscribeObjectClassAttributes(
      nmraClass,
      nmraAttributes,
      true,
      L"");
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Timestamped attribute-order cohort registration returned an invalid object handle");
  waitFor(
      tara,
      [&] { return tara.recorder().hasDiscovery(object); },
      options,
      "Timestamped attribute-order cohort first discovery");
  waitFor(
      nmra,
      [&] { return nmra.recorder().hasDiscovery(object); },
      options,
      "Timestamped attribute-order cohort second discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto taraTime = makeTimeContext(tara);
  auto nmraTime = makeTimeContext(nmra);
  require(
      publisherTime.factory->getName() == taraTime.factory->getName() &&
          publisherTime.factory->getName() == nmraTime.factory->getName(),
      "Timestamped attribute-order cohort members selected different logical-time factories");

  tara.rtiAmbassador().enableTimeConstrained();
  waitFor(
      tara,
      [&] { return tara.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-order cohort first time-constrained callback");
  nmra.rtiAmbassador().enableTimeConstrained();
  waitFor(
      nmra,
      [&] { return nmra.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-order cohort second time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-order cohort time-regulation callback");

  auto const timeFive = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const timeSeven = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const taraFive = timeAfter(
      *taraTime.factory,
      *taraTime.initial,
      *taraTime.epsilon,
      5U);
  auto const taraSeven = timeAfter(
      *taraTime.factory,
      *taraTime.initial,
      *taraTime.epsilon,
      7U);
  auto const nmraFive = timeAfter(
      *nmraTime.factory,
      *nmraTime.initial,
      *nmraTime.epsilon,
      5U);
  auto const nmraSeven = timeAfter(
      *nmraTime.factory,
      *nmraTime.initial,
      *nmraTime.epsilon,
      7U);
  auto const publisherFour = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const publisherSix = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);

  std::vector<std::uint8_t> const earlyValue{0x41U, 0x56U, 0x2DU, 0x35U};
  std::vector<std::uint8_t> const equalValue{
      0x41U, 0x56U, 0x2DU, 0x35U, 0x45U};
  std::vector<std::uint8_t> const lateValue{0x41U, 0x56U, 0x2DU, 0x37U};
  std::vector<std::uint8_t> const earlyTag{0x54U, 0x41U};
  std::vector<std::uint8_t> const equalTag{0x54U, 0x45U};
  std::vector<std::uint8_t> const lateTag{0x54U, 0x37U};
  rti::AttributeHandleValueMap earlyValues;
  earlyValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(earlyValue.data(), earlyValue.size()));
  rti::AttributeHandleValueMap equalValues;
  equalValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(equalValue.data(), equalValue.size()));
  rti::AttributeHandleValueMap lateValues;
  lateValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(lateValue.data(), lateValue.size()));
  rti::VariableLengthData earlyTagData(earlyTag.data(), earlyTag.size());
  rti::VariableLengthData equalTagData(equalTag.data(), equalTag.size());
  rti::VariableLengthData lateTagData(lateTag.data(), lateTag.size());

  auto const lateRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      lateValues,
      lateTagData,
      *timeSeven);
  auto const earlyRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      earlyValues,
      earlyTagData,
      *timeFive);
  auto const equalRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      equalValues,
      equalTagData,
      *timeFive);
  require(
      lateRetraction.isValid() && earlyRetraction.isValid() &&
          equalRetraction.isValid(),
      "Timestamped attribute-order cohort update returned an invalid retraction handle");
  require(
      tara.recorder().timedReflections().empty() &&
          nmra.recorder().timedReflections().empty(),
      "Timestamped attribute-order cohort reflected before a recipient request");

  tara.recorder().clearCallbackOrder();
  nmra.recorder().clearCallbackOrder();
  tara.rtiAmbassador().timeAdvanceRequest(*taraFive);
  nmra.rtiAmbassador().timeAdvanceRequest(*nmraFive);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherFour);
  waitForSessions(
      {&publisher, &tara, &nmra},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            tara.recorder().timedReflections().size() >= 2U &&
            tara.recorder().timeAdvanceGrants().size() >= 1U &&
            nmra.recorder().timedReflections().size() >= 2U &&
            nmra.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Timestamped attribute-order cohort first boundary");
  require(
      tara.recorder().timedReflections().size() == 2U &&
          tara.recorder().timeAdvanceGrants().size() == 1U &&
          tara.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "reflect", "grant"},
      "Timestamped attribute-order cohort first recipient boundary was unordered");
  require(
      nmra.recorder().timedReflections().size() == 2U &&
          nmra.recorder().timeAdvanceGrants().size() == 1U &&
          nmra.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "reflect", "grant"},
      "Timestamped attribute-order cohort second recipient boundary was unordered");

  auto assertCommon = [&](TimedReflectionRecord const& reflection,
                          rti::AttributeHandle const& attribute,
                          std::vector<std::uint8_t> const& expectedTime,
                          std::vector<std::uint8_t> const& expectedRetraction,
                          Session& inspector) {
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.values.count(attribute) == 1U &&
            reflection.time == expectedTime && !reflection.timeText.empty(),
        "Timestamped attribute-order cohort reflection returned the wrong object or time");
    require(
        reflection.producer == publisher.federateHandle() &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP &&
            reflection.retractionPresent &&
            reflection.retraction == expectedRetraction,
        "Timestamped attribute-order cohort reflection returned the wrong metadata");
    require(
        reflection.transportation.isValid() &&
            !inspector.rtiAmbassador()
                 .getTransportationTypeName(reflection.transportation)
                 .empty() &&
            !reflection.regions.has_value(),
        "Timestamped attribute-order cohort reflection returned invalid transport or regions");
  };
  auto assertCohort = [&](std::vector<TimedReflectionRecord> const& reflections,
                          rti::AttributeHandle const& attribute,
                          Session& inspector) {
    require(
        reflections.size() == 2U,
        "Timestamped attribute-order cohort changed the first callback count");
    auto const& first = reflections.at(0);
    auto const& second = reflections.at(1);
    auto const firstValue = copyBytes(first.values.at(attribute));
    auto const secondValue = copyBytes(second.values.at(attribute));
    auto const firstIsEarly = firstValue == earlyValue && first.tag == earlyTag;
    auto const firstIsEqual = firstValue == equalValue && first.tag == equalTag;
    auto const secondIsEarly = secondValue == earlyValue && second.tag == earlyTag;
    auto const secondIsEqual = secondValue == equalValue && second.tag == equalTag;
    require(
        (firstIsEarly && secondIsEqual) ||
            (firstIsEqual && secondIsEarly),
        "Timestamped attribute-order cohort did not preserve the complete equal-time cohort");
    auto const earlyRetractionBytes = copyBytes(earlyRetraction.encode());
    auto const equalRetractionBytes = copyBytes(equalRetraction.encode());
    assertCommon(first, attribute, encodeTime(*timeFive),
                 firstIsEarly ? earlyRetractionBytes : equalRetractionBytes,
                 inspector);
    assertCommon(second, attribute, encodeTime(*timeFive),
                 secondIsEarly ? earlyRetractionBytes : equalRetractionBytes,
                 inspector);
  };
  assertCohort(tara.recorder().timedReflections(), taraAttribute, tara);
  assertCohort(nmra.recorder().timedReflections(), nmraAttribute, nmra);
  require(
      tara.recorder().timeAdvanceGrants().front().encoded == encodeTime(*taraFive) &&
          nmra.recorder().timeAdvanceGrants().front().encoded == encodeTime(*nmraFive),
      "Timestamped attribute-order cohort returned the wrong first grants");

  tara.rtiAmbassador().timeAdvanceRequest(*taraSeven);
  nmra.rtiAmbassador().timeAdvanceRequest(*nmraSeven);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherSix);
  waitForSessions(
      {&publisher, &tara, &nmra},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            tara.recorder().timedReflections().size() >= 3U &&
            tara.recorder().timeAdvanceGrants().size() >= 2U &&
            nmra.recorder().timedReflections().size() >= 3U &&
            nmra.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "Timestamped attribute-order cohort second boundary");
  require(
      tara.recorder().timedReflections().size() == 3U &&
          nmra.recorder().timedReflections().size() == 3U,
      "Timestamped attribute-order cohort delivered an unexpected reflection count");
  auto assertLate = [&](TimedReflectionRecord const& reflection,
                        rti::AttributeHandle const& attribute,
                        Session& inspector) {
    assertCommon(
        reflection,
        attribute,
        encodeTime(*timeSeven),
        copyBytes(lateRetraction.encode()),
        inspector);
    require(
        copyBytes(reflection.values.at(attribute)) == lateValue &&
            reflection.tag == lateTag,
        "Timestamped attribute-order cohort delivered the wrong later record");
  };
  assertLate(tara.recorder().timedReflections().back(), taraAttribute, tara);
  assertLate(nmra.recorder().timedReflections().back(), nmraAttribute, nmra);
  require(
      tara.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "reflect", "grant", "reflect", "grant"} &&
          nmra.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "reflect", "grant", "reflect", "grant"},
      "Timestamped attribute-order cohort second boundary was unordered");
  require(
      tara.recorder().timeAdvanceGrants().back().encoded == encodeTime(*taraSeven) &&
          nmra.recorder().timeAdvanceGrants().back().encoded == encodeTime(*nmraSeven),
      "Timestamped attribute-order cohort returned the wrong final grants");

  tara.rtiAmbassador().disableTimeConstrained();
  nmra.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  tara.rtiAmbassador().unsubscribeObjectClassAttributes(
      taraClass,
      taraAttributes);
  nmra.rtiAmbassador().unsubscribeObjectClassAttributes(
      nmraClass,
      nmraAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  nmra.resign(rti::NO_ACTION);
  tara.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nmra.disconnect();
  tara.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeUpdate(Options const& options, rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session immediate(options, model, "member");
  Session constrained(options, model, "member");
  auto const federation = federationName(options, "timestamped-attribute-update");
  connectAndJoin(publisher, immediate, options, federation, options.fom);
  constrained.connect();
  constrained.join(
      options.memberFederateName + L"-timestamped-attribute-constrained",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const immediateClass = immediate.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const immediateAttribute = immediate.rtiAmbassador().getAttributeHandle(
      immediateClass, options.attributeName);
  auto const constrainedAttribute = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass, options.attributeName);
  require(
      publisherClass.isValid() && immediateClass.isValid() && constrainedClass.isValid(),
      "timestamped attribute-update object-class lookup returned an invalid handle");
  require(
      publisherAttribute.isValid() && immediateAttribute.isValid() &&
          constrainedAttribute.isValid(),
      "timestamped attribute-update attribute lookup returned an invalid handle");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet immediateAttributes;
  immediateAttributes.insert(immediateAttribute);
  rti::AttributeHandleSet constrainedAttributes;
  constrainedAttributes.insert(constrainedAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass, publisherAttributes, rti::TIMESTAMP);
  immediate.rtiAmbassador().subscribeObjectClassAttributes(
      immediateClass, immediateAttributes, true, L"");
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass, constrainedAttributes, true, L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "timestamped attribute-update registration returned an invalid handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().discovery().present; },
      options,
      "timestamped attribute-update immediate discovery");
  waitFor(
      constrained,
      [&] { return constrained.recorder().discovery().present; },
      options,
      "timestamped attribute-update constrained discovery");
  require(
      immediate.recorder().discovery().object == object &&
          constrained.recorder().discovery().object == object,
      "timestamped attribute-update discovery returned the wrong object handle");

  auto publisherTime = makeTimeContext(publisher);
  auto immediateTime = makeTimeContext(immediate);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      publisherTime.factory->getName() == immediateTime.factory->getName() &&
          publisherTime.factory->getName() == constrainedTime.factory->getName(),
      "timestamped attribute-update members selected different logical-time factories");
  enableTimestampedRoles(
      publisher,
      constrained,
      publisherTime,
      constrainedTime,
      options,
      "timestamped attribute-update");
  auto const invalidUpdateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  rti::AttributeHandleValueMap invalidObjectValues;
  invalidObjectValues.emplace(publisherAttribute, rti::VariableLengthData{});
  rti::AttributeHandleValueMap invalidAttributeValues;
  invalidAttributeValues.emplace(rti::AttributeHandle{}, rti::VariableLengthData{});
  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            rti::ObjectInstanceHandle{},
            invalidObjectValues,
            rti::VariableLengthData{},
            *invalidUpdateTime);
      },
      L"ObjectInstanceNotKnown",
      "timestamped attribute update for an unknown object");
  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            object,
            invalidAttributeValues,
            rti::VariableLengthData{},
            *invalidUpdateTime);
      },
      L"AttributeNotDefined",
      "timestamped attribute update with an invalid attribute");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::vector<std::uint8_t> const& expectedTime,
                              rti::FederateHandle const& expectedProducer,
                              rti::OrderType expectedReceivedOrder,
                              Session& inspector) {
    require(record.object == object,
            "timestamped attribute reflection returned the wrong object handle");
    require(record.values.size() == 1U && record.values.count(expectedAttribute) == 1U,
            "timestamped attribute reflection returned the wrong attribute set");
    require(copyBytes(record.values.at(expectedAttribute)) == expectedValue,
            "timestamped attribute reflection returned the wrong attribute value");
    require(record.tag == expectedTag,
            "timestamped attribute reflection did not preserve the user tag");
    require(record.producer == expectedProducer,
            "timestamped attribute reflection returned the wrong producing federate");
    require(record.transportation.isValid() &&
                !inspector.rtiAmbassador()
                     .getTransportationTypeName(record.transportation)
                     .empty(),
            "timestamped attribute reflection returned an unresolvable transportation type");
    require(record.time == expectedTime,
            "timestamped attribute reflection returned the wrong logical time");
    require(!record.timeText.empty(),
            "timestamped attribute reflection returned no logical-time representation");
    require(record.sentOrder == rti::TIMESTAMP,
            "timestamped attribute reflection returned a non-timestamp sent order");
    require(record.receivedOrder == expectedReceivedOrder,
            "timestamped attribute reflection returned the wrong received order");
  };

  auto firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto firstConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> firstValueBytes{0x01U, 0x02U, 0x03U};
  std::vector<std::uint8_t> firstTagBytes{0x11U, 0x12U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      firstTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "timestamped attribute update returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 1U; },
      options,
      "non-constrained timestamped attribute reflection");
  auto const firstImmediate = immediate.recorder().timedReflections();
  require(
      firstImmediate.size() == 1U,
      "non-constrained recipient received duplicate timestamped attribute reflection");
  assertReflection(
      firstImmediate.front(),
      immediateAttribute,
      firstValueBytes,
      firstTagBytes,
      encodeTime(*firstTime),
      publisher.federateHandle(),
      rti::RECEIVE,
      immediate);
  require(
      firstImmediate.front().retractionPresent &&
          firstImmediate.front().retraction == copyBytes(firstRetraction.encode()),
      "timestamped attribute reflection returned the wrong retraction handle");
  require(
      constrained.recorder().timedReflections().empty(),
      "time-constrained recipient received a timestamped attribute reflection before its grant");

  publisher.rtiAmbassador().retract(firstRetraction);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      options,
      "retraction callback for delivered timestamped attribute reflection");
  auto const firstRetractionCallbacks = immediate.recorder().retractions();
  require(
      firstRetractionCallbacks.size() == 1U && firstRetractionCallbacks.front().valid &&
          firstRetractionCallbacks.front().encoded == copyBytes(firstRetraction.encode()),
      "timestamped attribute retraction callback returned the wrong message handle");
  require(
      constrained.recorder().retractions().empty(),
      "pending timestamped attribute reflection produced a retraction callback before delivery");

  immediate.recorder().clearCallbackOrder();
  constrained.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  constrained.rtiAmbassador().timeAdvanceRequest(*firstConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
  waitFor(
      publisher,
      constrained,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "first timestamped attribute time advance grants");
  for (int pass = 0; pass != 8; ++pass) {
    constrained.pump();
  }
  require(
      constrained.recorder().timedReflections().empty(),
      "retracted timestamped attribute reflection was delivered at its grant");
  require(
      constrained.recorder().retractions().empty(),
      "retracted pending timestamped attribute reflection produced a stale callback");

  auto secondTime = timeAfter(
      *publisherTime.factory,
      *firstTime,
      *publisherTime.epsilon,
      3U);
  auto secondConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *firstConstrainedTime,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> secondValueBytes{0x21U, 0x22U, 0x23U};
  std::vector<std::uint8_t> secondTagBytes{0x31U, 0x32U};
  rti::AttributeHandleValueMap secondValues;
  secondValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  auto secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      secondValues,
      secondTag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "second timestamped attribute update returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 2U; },
      options,
      "second non-constrained timestamped attribute reflection");
  auto const secondImmediate = immediate.recorder().timedReflections();
  require(
      secondImmediate.size() == 2U,
      "non-constrained recipient received duplicate second timestamped attribute reflection");
  assertReflection(
      secondImmediate.at(1),
      immediateAttribute,
      secondValueBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      publisher.federateHandle(),
      rti::RECEIVE,
      immediate);
  require(
      secondImmediate.at(1).retractionPresent &&
          secondImmediate.at(1).retraction == copyBytes(secondRetraction.encode()),
      "second timestamped attribute reflection returned the wrong retraction handle");

  constrained.rtiAmbassador().timeAdvanceRequest(*secondConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
  waitFor(
      publisher,
      constrained,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timedReflections().size() >= 1U;
      },
      options,
      "second timestamped attribute reflection and time advance grants");
  auto const constrainedReflections = constrained.recorder().timedReflections();
  require(
      constrainedReflections.size() == 1U,
      "time-constrained recipient received the wrong number of timestamped attribute reflections");
  assertReflection(
      constrainedReflections.front(),
      constrainedAttribute,
      secondValueBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      publisher.federateHandle(),
      rti::TIMESTAMP,
      constrained);
  require(
      constrainedReflections.front().retractionPresent &&
          constrainedReflections.front().retraction == copyBytes(secondRetraction.encode()),
      "time-constrained timestamped attribute reflection returned the wrong retraction handle");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered timestamped attribute update at the producer boundary");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  immediate.rtiAmbassador().unsubscribeObjectClassAttributes(
      immediateClass, immediateAttributes);
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass, constrainedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publisherAttributes);
  constrained.resign(rti::NO_ACTION);
  immediate.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  immediate.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeUpdateNoFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-update no-fanout requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-no-fanout");
  publisher.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);

  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Timestamped attribute-update no-fanout lookup returned an invalid handle");

  rti::AttributeHandleSet const attributes{attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      rti::TIMESTAMP);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(
      object.isValid(),
      "Timestamped attribute-update no-fanout registration returned an invalid handle");

  auto publisherTime = makeTimeContext(publisher);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-update no-fanout time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  std::vector<std::uint8_t> valueBytes{0x41U, 0x42U, 0x43U};
  std::vector<std::uint8_t> tagBytes{0x4EU, 0x46U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Timestamped attribute-update no-fanout returned an invalid retraction handle");

  publisher.rtiAmbassador().retract(retraction);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "Timestamped attribute-update no-fanout accepted a second retraction");

  publisher.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "Timestamped attribute-update no-fanout time-advance grant");
  require(
      publisher.recorder().timedReflections().empty() &&
          publisher.recorder().retractions().empty(),
      "Timestamped attribute-update no-fanout produced a local callback without an eligible recipient");

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unpublishObjectClassAttributes(objectClass, attributes);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
}

void scenarioTimestampedAttributeUpdateAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-update alternate advances require an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "member");
  Session nextMessageAvailable(options, model, "member");
  Session nextMessageRequest(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  nextMessageRequest.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  flushQueue.join(
      options.memberFederateName + L"-attribute-flush-queue",
      options.federateType,
      federation);
  timeAdvanceAvailable.join(
      options.memberFederateName + L"-attribute-time-advance-available",
      options.federateType,
      federation);
  nextMessageAvailable.join(
      options.memberFederateName + L"-attribute-next-message-available",
      options.federateType,
      federation);
  nextMessageRequest.join(
      options.memberFederateName + L"-attribute-next-message-request",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid(),
      "Timestamped attribute-update alternate-advance publisher lookup returned an invalid handle");

  auto const members = std::vector<Session*>{
      &flushQueue,
      &timeAdvanceAvailable,
      &nextMessageAvailable,
      &nextMessageRequest,
  };
  std::vector<rti::ObjectClassHandle> memberClasses;
  std::vector<rti::AttributeHandle> memberAttributes;
  memberClasses.reserve(members.size());
  memberAttributes.reserve(members.size());
  for (auto* member : members) {
    auto const objectClass = member->rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const attribute = member->rtiAmbassador().getAttributeHandle(
        objectClass,
        options.attributeName);
    require(
        objectClass.isValid() && attribute.isValid(),
        "Timestamped attribute-update alternate-advance member lookup returned an invalid handle");
    memberClasses.push_back(objectClass);
    memberAttributes.push_back(attribute);
  }

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().subscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)},
        true,
        L"");
  }

  auto const nmrIndex = members.size() - 1U;
  rti::AttributeHandleSet nmrPublishedAttributes;
  nmrPublishedAttributes.insert(memberAttributes.at(nmrIndex));
  nextMessageRequest.rtiAmbassador().publishObjectClassAttributes(
      memberClasses.at(nmrIndex),
      nmrPublishedAttributes);
  nextMessageRequest.rtiAmbassador().changeDefaultAttributeOrderType(
      memberClasses.at(nmrIndex),
      nmrPublishedAttributes,
      rti::TIMESTAMP);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped attribute-update alternate-advance registration returned an invalid handle");
  for (auto* member : members) {
    waitFor(
        *member,
        [&] { return member->recorder().hasDiscovery(object); },
        options,
        "Timestamped attribute-update alternate-advance discovery");
  }
  auto const nmrObject = nextMessageRequest.rtiAmbassador().registerObjectInstance(
      memberClasses.at(nmrIndex));
  require(
      nmrObject.isValid(),
      "Timestamped attribute-update NMR sender registration returned an invalid handle");
  for (std::size_t index = 0U; index != nmrIndex; ++index) {
    auto* member = members.at(index);
    waitFor(
        *member,
        [&] { return member->recorder().hasDiscovery(nmrObject); },
        options,
        "Timestamped attribute-update NMR sender-object discovery");
  }

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  auto nextMessageRequestTime = makeTimeContext(nextMessageRequest);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageRequestTime.factory->getName(),
      "Timestamped attribute-update alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "Timestamped attribute-update alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-update alternate-advance time-regulation callback");
  nextMessageRequest.rtiAmbassador().enableTimeRegulation(*nextMessageRequestTime.epsilon);
  waitFor(
      nextMessageRequest,
      [&] { return nextMessageRequest.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-update NMR requester time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  std::vector<std::uint8_t> const valueBytes{
      0x41U, 0x54U, 0x54U, 0x52U, 0x2DU, 0x41U};
  std::vector<std::uint8_t> const tagBytes{
      0x41U, 0x4CU, 0x54U, 0x2DU, 0x41U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Timestamped attribute-update alternate-advance update returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedReflections().empty(),
        "Timestamped attribute-update alternate-advance delivered before its request");
    member->recorder().clearCallbackOrder();
  }
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  nextMessageRequest.rtiAmbassador().nextMessageRequest(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable,
       &nextMessageRequest},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedReflections().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedReflections().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedReflections().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageRequest.recorder().timedReflections().size() >= 1U &&
            nextMessageRequest.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Timestamped attribute-update alternate-advance deliveries and grants");

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "Timestamped attribute-update alternate-advance publisher received the wrong grant count");
  require(
      flushQueue.recorder().timedReflections().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Timestamped attribute-update Flush Queue Request returned the wrong callback pair");
  require(
      timeAdvanceAvailable.recorder().timedReflections().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "Timestamped attribute-update Time Advance Request Available returned the wrong callback pair");
  require(
      nextMessageAvailable.recorder().timedReflections().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "Timestamped attribute-update Next Message Request Available returned the wrong callback pair");
  require(
      nextMessageRequest.recorder().timedReflections().size() == 1U &&
          nextMessageRequest.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageRequest.recorder().flushQueueGrants().empty(),
      "Timestamped attribute-update Next Message Request returned the wrong callback pair");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "flush-grant"},
      "Timestamped attribute-update Flush Queue Request granted before reflection");
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Timestamped attribute-update Time Advance Request Available granted before reflection");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Timestamped attribute-update Next Message Request Available granted before reflection");
  require(
      nextMessageRequest.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Timestamped attribute-update Next Message Request granted before reflection");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              Session& inspector) {
    require(
        record.object == object &&
            record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U,
        "Timestamped attribute-update alternate-advance callback returned the wrong object or attribute set");
    require(
        copyBytes(record.values.at(expectedAttribute)) == valueBytes &&
            record.tag == tagBytes,
        "Timestamped attribute-update alternate-advance callback did not preserve value or tag");
    require(
        record.producer == publisher.federateHandle() &&
            record.time == encodeTime(*messageTime) &&
            !record.timeText.empty(),
        "Timestamped attribute-update alternate-advance callback returned the wrong producer or time");
    require(
        record.transportation.isValid() &&
            !inspector.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "Timestamped attribute-update alternate-advance callback returned an unresolvable transport");
    require(
        record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
        "Timestamped attribute-update alternate-advance callback returned the wrong order metadata");
    require(
        !record.regions.has_value(),
        "Timestamped attribute-update alternate-advance callback unexpectedly supplied regions");
    require(
        record.retractionPresent && record.retraction == copyBytes(retraction.encode()),
        "Timestamped attribute-update alternate-advance callback returned the wrong retraction handle");
  };
  assertReflection(
      flushQueue.recorder().timedReflections().front(),
      memberAttributes.at(0),
      flushQueue);
  assertReflection(
      timeAdvanceAvailable.recorder().timedReflections().front(),
      memberAttributes.at(1),
      timeAdvanceAvailable);
  assertReflection(
      nextMessageAvailable.recorder().timedReflections().front(),
      memberAttributes.at(2),
      nextMessageAvailable);
  assertReflection(
      nextMessageRequest.recorder().timedReflections().front(),
      memberAttributes.at(3),
      nextMessageRequest);

  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "Timestamped attribute-update alternate-advance publisher received the wrong grant time");
  require(
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime) &&
          nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime) &&
          nextMessageRequest.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "Timestamped attribute-update alternate-advance returned the wrong grant time");

  auto const flushGrants = flushQueue.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*messageTime) &&
          !flushGrants.front().time.empty() &&
          !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "Timestamped attribute-update alternate-advance Flush Queue Grant returned incomplete time metadata");
  auto actualFlushTime = decodeTime(*flushQueueTime.factory, flushGrants.front().time);
  auto optimisticFlushTime =
      decodeTime(*flushQueueTime.factory, flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *flushQueueTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "Timestamped attribute-update alternate-advance Flush Queue Grant returned time outside bounds");
  auto queriedFlushQueue = flushQueueTime.factory->makeInitial();
  flushQueue.rtiAmbassador().queryLogicalTime(*queriedFlushQueue);
  require(
      encodeTime(*queriedFlushQueue) == flushGrants.front().time,
      "Timestamped attribute-update alternate-advance Flush Queue query did not match its grant");
  auto queriedTimeAdvanceAvailable = timeAdvanceAvailableTime.factory->makeInitial();
  timeAdvanceAvailable.rtiAmbassador().queryLogicalTime(*queriedTimeAdvanceAvailable);
  requireTimeEquals(
      *queriedTimeAdvanceAvailable,
      *messageTime,
      "Timestamped attribute-update alternate-advance TARA query");
  auto queriedNextMessageAvailable = nextMessageAvailableTime.factory->makeInitial();
  nextMessageAvailable.rtiAmbassador().queryLogicalTime(*queriedNextMessageAvailable);
  requireTimeEquals(
      *queriedNextMessageAvailable,
      *messageTime,
      "Timestamped attribute-update alternate-advance NMRA query");
  auto queriedNextMessageRequest = nextMessageRequestTime.factory->makeInitial();
  nextMessageRequest.rtiAmbassador().queryLogicalTime(*queriedNextMessageRequest);
  requireTimeEquals(
      *queriedNextMessageRequest,
      *messageTime,
      "Timestamped attribute-update alternate-advance NMR query");

  auto queriedNmrLookahead = nextMessageRequestTime.factory->makeZero();
  nextMessageRequest.rtiAmbassador().queryLookahead(*queriedNmrLookahead);
  require(
      *queriedNmrLookahead == *nextMessageRequestTime.epsilon,
      "Timestamped attribute-update NMR lookahead changed unexpectedly");
  auto const outgoingTime = timeAfter(
      *nextMessageRequestTime.factory,
      *queriedNextMessageRequest,
      *queriedNmrLookahead,
      2U);
  std::vector<std::uint8_t> const outgoingValueBytes{
      0x41U, 0x54U, 0x54U, 0x52U, 0x2DU, 0x42U};
  rti::AttributeHandleValueMap outgoingValues;
  outgoingValues.emplace(
      memberAttributes.at(nmrIndex),
      rti::VariableLengthData(
          outgoingValueBytes.data(),
          outgoingValueBytes.size()));
  auto const outgoingRetraction = nextMessageRequest.rtiAmbassador().updateAttributeValues(
      nmrObject,
      outgoingValues,
      tag,
      *outgoingTime);
  require(
      outgoingRetraction.isValid(),
      "Timestamped attribute-update NMR post-grant update at time 7 returned an invalid retraction handle");
  auto logicalTimeAfterNmrSend = nextMessageRequestTime.factory->makeInitial();
  nextMessageRequest.rtiAmbassador().queryLogicalTime(*logicalTimeAfterNmrSend);
  requireTimeEquals(
      *logicalTimeAfterNmrSend,
      *messageTime,
      "Timestamped attribute-update NMR post-grant send logical-time query");

  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered timestamped attribute update after alternate advancement");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  nextMessageRequest.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().unsubscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)});
  }
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  nextMessageRequest.rtiAmbassador().unpublishObjectClassAttributes(
      memberClasses.at(nmrIndex),
      nmrPublishedAttributes);
  nextMessageRequest.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  nextMessageRequest.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeUpdateReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-update re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-reenable");
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
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "timestamped attribute-update re-enable lookup returned an invalid handle");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet receiverAttributes;
  receiverAttributes.insert(receiverAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "timestamped attribute-update re-enable registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped attribute-update re-enable discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped attribute-update re-enable members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "timestamped attribute-update re-enable could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped attribute-update re-enable time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped attribute-update re-enable time-regulation callback");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  std::vector<std::uint8_t> valueBytes{0x41U, 0x54U, 0x54U, 0x52U};
  std::vector<std::uint8_t> tagBytes{0x52U, 0x45U, 0x45U, 0x4EU, 0x2DU, 0x41U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "timestamped attribute-update re-enable returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped attribute-update re-enable delivered before its grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped attribute-update re-enable delivered while time constrained was disabled");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 2U; },
      options,
      "timestamped attribute-update re-enable second time-constrained callback");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped attribute-update re-enable delivered during re-enable callback");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*updateTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 1U;
      },
      options,
      "timestamped attribute-update re-enable delivery and grants");
  require(
      receiver.recorder().timedReflections().size() == 1U &&
          receiver.recorder().timeConstrainedEnabled().size() == 2U,
      "timestamped attribute-update re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "timestamped attribute-update re-enable delivered after its grant");
  auto const reflections = receiver.recorder().timedReflections();
  auto const& reflection = reflections.front();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(receiverAttribute) == 1U &&
          copyBytes(reflection.values.at(receiverAttribute)) == valueBytes,
      "timestamped attribute-update re-enable returned the wrong object or value");
  require(
      reflection.tag == tagBytes && reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty(),
      "timestamped attribute-update re-enable did not preserve value metadata");
  require(
      reflection.producer == publisher.federateHandle() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP,
      "timestamped attribute-update re-enable returned the wrong producer or order");
  require(
      reflection.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              reflection.transportation).empty(),
      "timestamped attribute-update re-enable returned an unresolvable transport");
  require(
      !reflection.regions.has_value(),
      "timestamped attribute-update re-enable unexpectedly supplied source regions");
  require(
      reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()),
      "timestamped attribute-update re-enable omitted its retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget) &&
          receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*updateTime),
      "timestamped attribute-update re-enable returned the wrong grants");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped attribute-update re-enable retraction after delivery");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
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

void scenarioTimestampedAttributeUpdateRegulationReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-update regulation re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-regulation-reenable");
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
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "timestamped attribute-update regulation re-enable lookup returned an invalid handle");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet receiverAttributes;
  receiverAttributes.insert(receiverAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "timestamped attribute-update regulation re-enable registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped attribute-update regulation re-enable discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped attribute-update regulation re-enable members selected different logical-time factories");
  auto const initialLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto initialLookahead = publisherTime.factory->makeZero();
  require(
      initialLookahead != nullptr,
      "timestamped attribute-update regulation re-enable could not allocate the initial lookahead");
  initialLookahead->setToDifference(
      *initialLookaheadTime,
      *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped attribute-update regulation re-enable time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*initialLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped attribute-update regulation re-enable time-regulation callback");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const changedLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto changedLookahead = publisherTime.factory->makeZero();
  require(
      changedLookahead != nullptr,
      "timestamped attribute-update regulation re-enable could not allocate the changed lookahead");
  changedLookahead->setToDifference(
      *changedLookaheadTime,
      *publisherTime.initial);

  std::vector<std::uint8_t> valueBytes{0xB7U, 0x2CU, 0x91U};
  std::vector<std::uint8_t> tagBytes{0x43U, 0x48U, 0x47U, 0x41U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "timestamped attribute-update regulation re-enable returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped attribute-update regulation re-enable delivered before its grant");

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().enableTimeRegulation(*changedLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 2U; },
      options,
      "timestamped attribute-update regulation re-enable callback");
  auto queriedLookahead = publisherTime.factory->makeZero();
  require(
      queriedLookahead != nullptr,
      "timestamped attribute-update regulation re-enable could not allocate a lookahead query");
  publisher.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(
      *queriedLookahead == *changedLookahead,
      "timestamped attribute-update regulation re-enable did not expose the changed lookahead");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*updateTime);
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped attribute-update regulation re-enable delivered while the producer boundary was pending");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 1U;
      },
      options,
      "timestamped attribute-update regulation re-enable delivery");

  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U &&
          publisher.recorder().timeRegulationEnabled().size() == 2U,
      "timestamped attribute-update regulation re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "timestamped attribute-update regulation re-enable delivered after its grant");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(receiverAttribute) == 1U &&
          copyBytes(reflection.values.at(receiverAttribute)) == valueBytes,
      "timestamped attribute-update regulation re-enable returned the wrong object or value");
  require(
      reflection.tag == tagBytes && reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty(),
      "timestamped attribute-update regulation re-enable did not preserve value metadata");
  require(
      reflection.producer == publisher.federateHandle() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP,
      "timestamped attribute-update regulation re-enable returned the wrong producer or order");
  require(
      reflection.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              reflection.transportation).empty(),
      "timestamped attribute-update regulation re-enable returned an unresolvable transport");
  require(
      !reflection.regions.has_value(),
      "timestamped attribute-update regulation re-enable unexpectedly supplied source regions");
  require(
      reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()),
      "timestamped attribute-update regulation re-enable omitted the retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget),
      "timestamped attribute-update regulation re-enable returned the wrong producer grant");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*updateTime),
      "timestamped attribute-update regulation re-enable returned the wrong receiver grant");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped attribute-update regulation re-enable retraction after delivery");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
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

void scenarioTimestampedAttributeSourceResignationFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute source resignation fanout requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session first(options, model, "first");
  Session second(options, model, "second");
  Session clock(options, model, "source-resignation-clock");
  auto const federation = federationName(
      options,
      "timestamped-attribute-source-resignation-fanout");
  connectAndJoin(publisher, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName + L"-timestamped-attribute-resignation-second",
      options.federateType,
      federation);
  clock.connect();
  clock.join(
      options.memberFederateName + L"-timestamped-attribute-resignation-clock",
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
      "timestamped attribute source resignation fanout lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
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

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "timestamped attribute source resignation fanout registration returned an invalid handle");
  waitFor(
      first,
      [&] { return first.recorder().hasDiscovery(object); },
      options,
      "timestamped attribute source resignation fanout first discovery");
  waitFor(
      second,
      [&] { return second.recorder().hasDiscovery(object); },
      options,
      "timestamped attribute source resignation fanout second discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto firstTime = makeTimeContext(first);
  auto secondTime = makeTimeContext(second);
  auto clockTime = makeTimeContext(clock);
  require(
      publisherTime.factory->getName() == firstTime.factory->getName() &&
          publisherTime.factory->getName() == secondTime.factory->getName() &&
          publisherTime.factory->getName() == clockTime.factory->getName(),
      "timestamped attribute source resignation fanout members selected different logical-time factories");

  auto makeLookahead = [](TimeContext const& time) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped attribute source resignation fanout could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime);
  auto clockLookahead = makeLookahead(clockTime);

  first.rtiAmbassador().enableTimeConstrained();
  waitFor(
      first,
      [&] { return first.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped attribute source resignation fanout first time-constrained callback");
  second.rtiAmbassador().enableTimeConstrained();
  waitFor(
      second,
      [&] { return second.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped attribute source resignation fanout second time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped attribute source resignation fanout publisher time-regulation callback");
  clock.rtiAmbassador().enableTimeRegulation(*clockLookahead);
  waitFor(
      clock,
      [&] { return clock.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped attribute source resignation fanout clock time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const firstTarget = timeAfter(
      *firstTime.factory,
      *firstTime.initial,
      *firstTime.epsilon,
      7U);
  auto const secondTarget = timeAfter(
      *secondTime.factory,
      *secondTime.initial,
      *secondTime.epsilon,
      10U);
  auto const clockTarget = timeAfter(
      *clockTime.factory,
      *clockTime.initial,
      *clockTime.epsilon,
      2U);
  std::vector<std::uint8_t> const valueBytes{0xA7U, 0x71U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{
      0x52U, 0x45U, 0x53U, 0x2DU, 0x46U, 0x41U, 0x4EU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped attribute source resignation fanout update returned an invalid retraction handle");
  require(
      first.recorder().timedReflections().empty() &&
          second.recorder().timedReflections().empty(),
      "timestamped attribute source resignation fanout delivered before either recipient frontier");

  first.rtiAmbassador().timeAdvanceRequest(*firstTarget);
  second.rtiAmbassador().nextMessageRequest(*secondTarget);
  require(
      first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped attribute source resignation fanout granted before the producer frontier");
  publisher.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"FederateNotExecutionMember",
      "timestamped attribute source resignation fanout post-resignation retraction");
  require(
      first.recorder().timedReflections().empty() &&
          second.recorder().timedReflections().empty() &&
          first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped attribute source resignation fanout released queued callbacks too early");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  clock.rtiAmbassador().timeAdvanceRequest(*clockTarget);
  waitFor(
      clock,
      [&] { return clock.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped attribute source resignation fanout clock frontier");
  waitFor(
      first,
      [&] {
        return first.recorder().timedReflections().size() >= 1U &&
            first.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped attribute source resignation fanout first delivery and grant");
  waitFor(
      second,
      [&] {
        return second.recorder().timedReflections().size() >= 1U &&
            second.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped attribute source resignation fanout second delivery and grant");

  auto assertReflection = [&](Session& receiver,
                              rti::AttributeHandle const& expectedAttribute,
                              TimedReflectionRecord const& reflection,
                              std::vector<std::uint8_t> const& expectedGrant) {
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.values.count(expectedAttribute) == 1U &&
            copyBytes(reflection.values.at(expectedAttribute)) == valueBytes,
        "timestamped attribute source resignation fanout returned the wrong object or value");
    require(
        reflection.tag == tagBytes && reflection.time == encodeTime(*messageTime) &&
            !reflection.timeText.empty(),
        "timestamped attribute source resignation fanout did not preserve value or time");
    require(
        reflection.producer == publisher.federateHandle() &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP,
        "timestamped attribute source resignation fanout returned the wrong producer or order metadata");
    require(
        reflection.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                reflection.transportation).empty(),
        "timestamped attribute source resignation fanout returned an unresolvable transport");
    require(
        !reflection.regions.has_value(),
        "timestamped attribute source resignation fanout unexpectedly supplied source regions");
    require(
        reflection.retractionPresent &&
            reflection.retraction == copyBytes(retraction.encode()),
        "timestamped attribute source resignation fanout omitted its retraction handle");
    require(
        receiver.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().front().encoded == expectedGrant,
        "timestamped attribute source resignation fanout returned the wrong recipient grant");
    require(
        receiver.recorder().callbackOrder() ==
            std::vector<std::string>{"reflect", "grant"},
        "timestamped attribute source resignation fanout reflected after its grant");
  };

  auto const firstReflections = first.recorder().timedReflections();
  auto const secondReflections = second.recorder().timedReflections();
  require(
      firstReflections.size() == 1U && secondReflections.size() == 1U,
      "timestamped attribute source resignation fanout returned duplicate deliveries");
  assertReflection(
      first,
      firstAttribute,
      firstReflections.front(),
      encodeTime(*firstTarget));
  assertReflection(
      second,
      secondAttribute,
      secondReflections.front(),
      encodeTime(*messageTime));

  first.rtiAmbassador().disableTimeConstrained();
  second.rtiAmbassador().disableTimeConstrained();
  clock.rtiAmbassador().disableTimeRegulation();
  first.rtiAmbassador().unsubscribeObjectClassAttributes(firstClass, firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(secondClass, secondAttributes);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  clock.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  first.disconnect();
  second.disconnect();
  clock.disconnect();
}

void scenarioTimestampedRegionalAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped regional attribute testing requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped regional attribute testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session immediate(options, model, "member");
  Session constrained(options, model, "constrained");
  Session disjoint(options, model, "passive");
  auto const federation = federationName(options, "timestamped-regional-attribute-update");
  connectAndJoin(publisher, immediate, options, federation, options.ddmFom);
  constrained.connect();
  constrained.join(
      options.memberFederateName + L"-timestamped-regional-constrained",
      options.federateType,
      federation);
  disjoint.connect();
  disjoint.join(
      options.memberFederateName + L"-timestamped-regional-disjoint",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const immediateHandles = ddmHandles(immediate, options);
  auto const constrainedHandles = ddmHandles(constrained, options);
  auto const disjointHandles = ddmHandles(disjoint, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped regional attribute update");

  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const immediateAttributes{immediateHandles.attribute};
  rti::AttributeHandleSet const constrainedAttributes{constrainedHandles.attribute};
  rti::AttributeHandleSet const disjointAttributes{disjointHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const immediateRegion = createDdmRegion(
      immediate,
      immediateHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const constrainedRegion = createDdmRegion(
      constrained,
      constrainedHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const disjointRegion = createDdmRegion(
      disjoint,
      disjointHandles,
      7UL,
      9UL,
      7UL,
      9UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const immediateRegionSet = rti::RegionHandleSet{immediateRegion};
  auto const constrainedRegionSet = rti::RegionHandleSet{constrainedRegion};
  auto const disjointRegionSet = rti::RegionHandleSet{disjointRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      publisherAttributes,
      sourceRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const immediatePair{{
      immediateAttributes,
      immediateRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const constrainedPair{{
      constrainedAttributes,
      constrainedRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const disjointPair{{
      disjointAttributes,
      disjointRegionSet,
  }};
  immediate.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  constrained.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  disjoint.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      immediate.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          constrained.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          disjoint.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "timestamped regional attribute receivers did not enable region designator callbacks");
  immediate.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      immediateHandles.objectClass,
      immediatePair,
      true,
      L"");
  constrained.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      constrainedHandles.objectClass,
      constrainedPair,
      true,
      L"");
  disjoint.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      disjointHandles.objectClass,
      disjointPair,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      publisherPair);
  require(
      object.isValid(),
      "timestamped regional attribute registration returned an invalid object handle");
  waitForSessions(
      {&publisher, &immediate, &constrained, &disjoint},
      [&] {
        return immediate.recorder().hasDiscovery(object) &&
            constrained.recorder().hasDiscovery(object);
      },
      options,
      "timestamped regional attribute object discovery");
  for (int pass = 0; pass != 8; ++pass) {
    disjoint.pump();
  }
  require(
      !disjoint.recorder().hasDiscovery(object),
      "disjoint timestamped regional attribute subscription discovered the object");

  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  enableTimestampedRoles(
      publisher,
      constrained,
      publisherTime,
      constrainedTime,
      options,
      "timestamped regional attribute update");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::vector<std::uint8_t> const& expectedTime,
                              rti::OrderType expectedReceivedOrder,
                              Session& inspector) {
    require(
        record.object == object && record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U,
        "timestamped regional attribute reflection returned the wrong object or attribute set");
    require(
        copyBytes(record.values.at(expectedAttribute)) == expectedValue &&
            record.tag == expectedTag && record.time == expectedTime,
        "timestamped regional attribute reflection did not preserve value, tag, or time");
    require(
        record.producer == publisher.federateHandle() && !record.timeText.empty(),
        "timestamped regional attribute reflection returned the wrong producer or time text");
    std::wstring transportationName;
    if (record.transportation.isValid()) {
      transportationName = inspector.rtiAmbassador().getTransportationTypeName(
          record.transportation);
    }
    require(
        record.transportation.isValid() && !transportationName.empty(),
        "timestamped regional attribute reflection returned an unresolvable transport");
    if (options.requireCustomTransportation) {
      require(
          transportationName == options.fomCustomTransportationName,
          "timestamped regional attribute reflection changed the adapter-declared custom transport");
    }
    require(
        record.sentOrder == rti::TIMESTAMP && record.receivedOrder == expectedReceivedOrder,
        "timestamped regional attribute reflection returned the wrong order metadata");
    require(
        record.regions.has_value() && record.regions->size() == 1U &&
            record.regions->count(sourceRegion) == 1U,
        "timestamped regional attribute reflection omitted the source region designator set");
  };

  auto firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto firstConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> firstValue{0x01U, 0x02U, 0x03U};
  std::vector<std::uint8_t> firstTag{0x52U, 0x31U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  rti::VariableLengthData firstUserTag(firstTag.data(), firstTag.size());
  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      firstUserTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "timestamped regional attribute update returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 1U; },
      options,
      "immediate timestamped regional attribute reflection");
  auto const firstImmediate = immediate.recorder().timedReflections();
  require(
      firstImmediate.size() == 1U,
      "immediate regional attribute receiver received duplicate timestamped reflections");
  assertReflection(
      firstImmediate.front(),
      immediateHandles.attribute,
      firstValue,
      firstTag,
      encodeTime(*firstTime),
      rti::RECEIVE,
      immediate);
  require(
      firstImmediate.front().retractionPresent &&
          firstImmediate.front().retraction == copyBytes(firstRetraction.encode()),
      "immediate regional attribute reflection omitted its retraction handle");
  require(
      constrained.recorder().timedReflections().empty(),
      "constrained regional attribute receiver delivered before its time grant");
  for (int pass = 0; pass != 8; ++pass) {
    disjoint.pump();
  }
  require(
      disjoint.recorder().timedReflections().empty(),
      "disjoint regional attribute receiver received a timestamped reflection");

  publisher.rtiAmbassador().retract(firstRetraction);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      options,
      "timestamped regional attribute retraction callback");
  auto const firstRetractionCallbacks = immediate.recorder().retractions();
  require(
      firstRetractionCallbacks.size() == 1U && firstRetractionCallbacks.front().valid &&
          firstRetractionCallbacks.front().encoded == copyBytes(firstRetraction.encode()),
      "timestamped regional attribute retraction returned the wrong handle");
  require(
      constrained.recorder().retractions().empty(),
      "pending constrained regional attribute update produced a retraction callback");

  constrained.rtiAmbassador().disableTimeConstrained();
  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() >= 2U; },
      options,
      "timestamped regional attribute time-constrained re-enable");
  constrained.rtiAmbassador().timeAdvanceRequest(*firstConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
  waitFor(
      publisher,
      constrained,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "first timestamped regional attribute time-advance grants");
  for (int pass = 0; pass != 8; ++pass) {
    constrained.pump();
  }
  require(
      constrained.recorder().timedReflections().empty() &&
          constrained.recorder().retractions().empty(),
      "retracted constrained regional attribute update produced a stale callback");

  auto secondTime = timeAfter(
      *publisherTime.factory,
      *firstTime,
      *publisherTime.epsilon,
      3U);
  auto secondConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *firstConstrainedTime,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> secondValue{0x21U, 0x22U, 0x23U};
  std::vector<std::uint8_t> secondTag{0x52U, 0x32U};
  rti::AttributeHandleValueMap secondValues;
  secondValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(secondValue.data(), secondValue.size()));
  rti::VariableLengthData secondUserTag(secondTag.data(), secondTag.size());
  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      secondValues,
      secondUserTag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "second timestamped regional attribute update returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 2U; },
      options,
      "second immediate timestamped regional attribute reflection");
  auto const secondImmediate = immediate.recorder().timedReflections();
  require(
      secondImmediate.size() == 2U,
      "immediate regional attribute receiver received duplicate second reflections");
  assertReflection(
      secondImmediate.at(1),
      immediateHandles.attribute,
      secondValue,
      secondTag,
      encodeTime(*secondTime),
      rti::RECEIVE,
      immediate);
  require(
      secondImmediate.at(1).retractionPresent &&
          secondImmediate.at(1).retraction == copyBytes(secondRetraction.encode()),
      "second immediate regional attribute reflection omitted its retraction handle");
  require(
      constrained.recorder().timedReflections().empty(),
      "constrained regional attribute receiver delivered the second update early");

  constrained.recorder().clearCallbackOrder();
  constrained.rtiAmbassador().timeAdvanceRequest(*secondConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
  waitFor(
      publisher,
      constrained,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timedReflections().size() >= 1U;
      },
      options,
      "second timestamped regional attribute delivery and grants");
  auto const constrainedReflections = constrained.recorder().timedReflections();
  require(
      constrainedReflections.size() == 1U,
      "constrained regional attribute receiver received the wrong reflection count");
  assertReflection(
      constrainedReflections.front(),
      constrainedHandles.attribute,
      secondValue,
      secondTag,
      encodeTime(*secondTime),
      rti::TIMESTAMP,
      constrained);
  require(
      constrained.recorder().callbackOrder() == std::vector<std::string>{"reflect", "grant"},
      "constrained regional attribute delivery occurred after its time grant");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retraction after constrained regional attribute delivery");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  immediate.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      immediateHandles.objectClass,
      immediatePair);
  constrained.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      constrainedHandles.objectClass,
      constrainedPair);
  disjoint.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      disjointHandles.objectClass,
      disjointPair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, publisherPair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  immediate.rtiAmbassador().deleteRegion(immediateRegion);
  constrained.rtiAmbassador().deleteRegion(constrainedRegion);
  disjoint.rtiAmbassador().deleteRegion(disjointRegion);

  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  immediate.resign(rti::NO_ACTION);
  constrained.resign(rti::NO_ACTION);
  disjoint.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  immediate.disconnect();
  constrained.disconnect();
  disjoint.disconnect();
}

void scenarioTimestampedRegionalAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedRegionalAttributeUpdate(options, model);
}

void scenarioTimestampedRegionalAttributeAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped regional alternate advances require an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped regional alternate advances require an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "member");
  Session nextMessageAvailable(options, model, "member");
  auto const federation =
      federationName(options, "timestamped-regional-attribute-alternate-advances");
  connectAndJoin(publisher, flushQueue, options, federation, options.ddmFom);
  timeAdvanceAvailable.connect();
  timeAdvanceAvailable.join(
      options.memberFederateName + L"-timestamped-regional-tara",
      options.federateType,
      federation);
  nextMessageAvailable.connect();
  nextMessageAvailable.join(
      options.memberFederateName + L"-timestamped-regional-nmra",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const flushQueueHandles = ddmHandles(flushQueue, options);
  auto const timeAdvanceAvailableHandles = ddmHandles(timeAdvanceAvailable, options);
  auto const nextMessageAvailableHandles = ddmHandles(nextMessageAvailable, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped regional alternate advances");

  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const flushQueueAttributes{flushQueueHandles.attribute};
  rti::AttributeHandleSet const timeAdvanceAvailableAttributes{
      timeAdvanceAvailableHandles.attribute};
  rti::AttributeHandleSet const nextMessageAvailableAttributes{
      nextMessageAvailableHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const flushQueueRegion = createDdmRegion(
      flushQueue,
      flushQueueHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const timeAdvanceAvailableRegion = createDdmRegion(
      timeAdvanceAvailable,
      timeAdvanceAvailableHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const nextMessageAvailableRegion = createDdmRegion(
      nextMessageAvailable,
      nextMessageAvailableHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const flushQueueRegionSet = rti::RegionHandleSet{flushQueueRegion};
  auto const timeAdvanceAvailableRegionSet =
      rti::RegionHandleSet{timeAdvanceAvailableRegion};
  auto const nextMessageAvailableRegionSet =
      rti::RegionHandleSet{nextMessageAvailableRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      publisherAttributes,
      sourceRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const flushQueuePair{{
      flushQueueAttributes,
      flushQueueRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const timeAdvanceAvailablePair{{
      timeAdvanceAvailableAttributes,
      timeAdvanceAvailableRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const nextMessageAvailablePair{{
      nextMessageAvailableAttributes,
      nextMessageAvailableRegionSet,
  }};
  flushQueue.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  timeAdvanceAvailable.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  nextMessageAvailable.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      flushQueue.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          timeAdvanceAvailable.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          nextMessageAvailable.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "timestamped regional alternate recipients did not enable region designator callbacks");
  flushQueue.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      flushQueueHandles.objectClass,
      flushQueuePair,
      true,
      L"");
  timeAdvanceAvailable.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      timeAdvanceAvailableHandles.objectClass,
      timeAdvanceAvailablePair,
      true,
      L"");
  nextMessageAvailable.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      nextMessageAvailableHandles.objectClass,
      nextMessageAvailablePair,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      publisherPair);
  require(
      object.isValid(),
      "timestamped regional alternate registration returned an invalid object handle");
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return flushQueue.recorder().hasDiscovery(object) &&
            timeAdvanceAvailable.recorder().hasDiscovery(object) &&
            nextMessageAvailable.recorder().hasDiscovery(object);
      },
      options,
      "timestamped regional alternate object discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "timestamped regional alternate members selected different logical-time factories");
  for (auto* member :
       std::vector<Session*>{&flushQueue, &timeAdvanceAvailable, &nextMessageAvailable}) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "timestamped regional alternate time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped regional alternate time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> valueBytes{0x61U, 0x62U, 0x63U};
  std::vector<std::uint8_t> tagBytes{0x52U, 0x41U, 0x41U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped regional alternate update returned an invalid retraction handle");
  require(
      flushQueue.recorder().timedReflections().empty() &&
          timeAdvanceAvailable.recorder().timedReflections().empty() &&
          nextMessageAvailable.recorder().timedReflections().empty(),
      "timestamped regional alternate recipient received a reflection before its request");
  flushQueue.recorder().clearCallbackOrder();
  timeAdvanceAvailable.recorder().clearCallbackOrder();
  nextMessageAvailable.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);

  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    flushQueue.pump();
    timeAdvanceAvailable.pump();
    nextMessageAvailable.pump();
    if (publisher.recorder().timeAdvanceGrants().size() >= 1U &&
        flushQueue.recorder().timedReflections().size() >= 1U &&
        flushQueue.recorder().flushQueueGrants().size() >= 1U &&
        timeAdvanceAvailable.recorder().timedReflections().size() >= 1U &&
        timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
        nextMessageAvailable.recorder().timedReflections().size() >= 1U &&
        nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped regional alternate publisher received the wrong grant count");
  require(
      flushQueue.recorder().timedReflections().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "timestamped regional Flush Queue Request returned the wrong callback pair");
  require(
      timeAdvanceAvailable.recorder().timedReflections().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "timestamped regional Time Advance Request Available returned the wrong callback pair");
  require(
      nextMessageAvailable.recorder().timedReflections().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "timestamped regional Next Message Request Available returned the wrong callback pair");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "flush-grant"},
      "timestamped regional Flush Queue Request granted before reflection");
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "timestamped regional Time Advance Request Available granted before reflection");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "timestamped regional Next Message Request Available granted before reflection");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              Session& inspector) {
    require(
        record.object == object && record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U,
        "timestamped regional alternate reflection returned the wrong object or attribute set");
    require(
        copyBytes(record.values.at(expectedAttribute)) == valueBytes &&
            record.tag == tagBytes,
        "timestamped regional alternate reflection did not preserve value or tag");
    require(
        record.producer == publisher.federateHandle() && record.time == encodeTime(*messageTime) &&
            !record.timeText.empty(),
        "timestamped regional alternate reflection returned the wrong producer or time");
    require(
        record.transportation.isValid() &&
            !inspector.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "timestamped regional alternate reflection returned an unresolvable transport");
    require(
        record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
        "timestamped regional alternate reflection returned the wrong order metadata");
    require(
        record.regions.has_value() && record.regions->size() == 1U &&
            record.regions->count(sourceRegion) == 1U,
        "timestamped regional alternate reflection omitted the source region designator");
    require(
        record.retractionPresent && record.retraction == copyBytes(retraction.encode()),
        "timestamped regional alternate reflection omitted the retraction handle");
  };
  assertReflection(
      flushQueue.recorder().timedReflections().front(),
      flushQueueHandles.attribute,
      flushQueue);
  assertReflection(
      timeAdvanceAvailable.recorder().timedReflections().front(),
      timeAdvanceAvailableHandles.attribute,
      timeAdvanceAvailable);
  assertReflection(
      nextMessageAvailable.recorder().timedReflections().front(),
      nextMessageAvailableHandles.attribute,
      nextMessageAvailable);

  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*publisherTarget),
      "timestamped regional alternate publisher received the wrong grant time");
  auto const flushGrants = flushQueue.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*messageTime),
      "timestamped regional Flush Queue Grant returned the wrong optimistic time");
  require(
      !flushGrants.front().time.empty() && !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "timestamped regional Flush Queue Grant returned incomplete time metadata");
  auto actualFlushTime = decodeTime(*flushQueueTime.factory, flushGrants.front().time);
  auto optimisticFlushTime =
      decodeTime(*flushQueueTime.factory, flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *flushQueueTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "timestamped regional Flush Queue Grant returned time outside standard bounds");
  require(
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime) &&
          nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "timestamped regional alternate grants returned the wrong logical time");

  auto queriedFlushQueue = flushQueueTime.factory->makeInitial();
  flushQueue.rtiAmbassador().queryLogicalTime(*queriedFlushQueue);
  require(
      encodeTime(*queriedFlushQueue) == flushGrants.front().time,
      "timestamped regional Flush Queue Request query did not match its grant");
  auto queriedTimeAdvanceAvailable = timeAdvanceAvailableTime.factory->makeInitial();
  timeAdvanceAvailable.rtiAmbassador().queryLogicalTime(*queriedTimeAdvanceAvailable);
  requireTimeEquals(
      *queriedTimeAdvanceAvailable,
      *messageTime,
      "timestamped regional Time Advance Request Available logical-time query");
  auto queriedNextMessageAvailable = nextMessageAvailableTime.factory->makeInitial();
  nextMessageAvailable.rtiAmbassador().queryLogicalTime(*queriedNextMessageAvailable);
  requireTimeEquals(
      *queriedNextMessageAvailable,
      *messageTime,
      "timestamped regional Next Message Request Available logical-time query");

  auto const pastFlushQueueTime = timeAfter(
      *flushQueueTime.factory,
      *flushQueueTime.initial,
      *flushQueueTime.epsilon,
      1U);
  auto const pastTimeAdvanceAvailableTime = timeAfter(
      *timeAdvanceAvailableTime.factory,
      *timeAdvanceAvailableTime.initial,
      *timeAdvanceAvailableTime.epsilon,
      1U);
  auto const pastNextMessageAvailableTime = timeAfter(
      *nextMessageAvailableTime.factory,
      *nextMessageAvailableTime.initial,
      *nextMessageAvailableTime.epsilon,
      1U);
  requireException(
      [&] { flushQueue.rtiAmbassador().flushQueueRequest(*pastFlushQueueTime); },
      L"LogicalTimeAlreadyPassed",
      "timestamped regional Flush Queue Request before current time");
  requireException(
      [&] {
        timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
            *pastTimeAdvanceAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "timestamped regional Time Advance Request Available before current time");
  requireException(
      [&] {
        nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(
            *pastNextMessageAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "timestamped regional Next Message Request Available before current time");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped regional alternate retraction after delivery");

  flushQueue.rtiAmbassador().disableTimeConstrained();
  timeAdvanceAvailable.rtiAmbassador().disableTimeConstrained();
  nextMessageAvailable.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  flushQueue.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      flushQueueHandles.objectClass,
      flushQueuePair);
  timeAdvanceAvailable.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      timeAdvanceAvailableHandles.objectClass,
      timeAdvanceAvailablePair);
  nextMessageAvailable.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      nextMessageAvailableHandles.objectClass,
      nextMessageAvailablePair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, publisherPair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  flushQueue.rtiAmbassador().deleteRegion(flushQueueRegion);
  timeAdvanceAvailable.rtiAmbassador().deleteRegion(timeAdvanceAvailableRegion);
  nextMessageAvailable.rtiAmbassador().deleteRegion(nextMessageAvailableRegion);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionAttributeAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region alternate advances require an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region alternate advances require an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "member");
  Session nextMessageAvailable(options, model, "member");
  auto const federation =
      federationName(options, "timestamped-default-region-attribute-alternate-advances");
  connectAndJoin(publisher, flushQueue, options, federation, options.ddmFom);
  timeAdvanceAvailable.connect();
  timeAdvanceAvailable.join(
      options.memberFederateName + L"-default-region-tara",
      options.federateType,
      federation);
  nextMessageAvailable.connect();
  nextMessageAvailable.join(
      options.memberFederateName + L"-default-region-nmra",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const flushQueueHandles = ddmHandles(flushQueue, options);
  auto const timeAdvanceAvailableHandles = ddmHandles(timeAdvanceAvailable, options);
  auto const nextMessageAvailableHandles = ddmHandles(nextMessageAvailable, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region alternate advances");
  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const flushQueueAttributes{flushQueueHandles.attribute};
  rti::AttributeHandleSet const timeAdvanceAvailableAttributes{
      timeAdvanceAvailableHandles.attribute};
  rti::AttributeHandleSet const nextMessageAvailableAttributes{
      nextMessageAvailableHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const flushQueueRegion = createDdmRegion(
      flushQueue,
      flushQueueHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const timeAdvanceAvailableRegion = createDdmRegion(
      timeAdvanceAvailable,
      timeAdvanceAvailableHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const nextMessageAvailableRegion = createDdmRegion(
      nextMessageAvailable,
      nextMessageAvailableHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const flushQueueRegionSet = rti::RegionHandleSet{flushQueueRegion};
  auto const timeAdvanceAvailableRegionSet =
      rti::RegionHandleSet{timeAdvanceAvailableRegion};
  auto const nextMessageAvailableRegionSet =
      rti::RegionHandleSet{nextMessageAvailableRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const flushQueuePair{{
      flushQueueAttributes,
      flushQueueRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const timeAdvanceAvailablePair{{
      timeAdvanceAvailableAttributes,
      timeAdvanceAvailableRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const nextMessageAvailablePair{{
      nextMessageAvailableAttributes,
      nextMessageAvailableRegionSet,
  }};
  flushQueue.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  timeAdvanceAvailable.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  nextMessageAvailable.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      flushQueue.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          timeAdvanceAvailable.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          nextMessageAvailable.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "timestamped default-region alternate recipients did not enable region designator callbacks");
  flushQueue.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      flushQueueHandles.objectClass,
      flushQueuePair,
      true,
      L"");
  timeAdvanceAvailable.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      timeAdvanceAvailableHandles.objectClass,
      timeAdvanceAvailablePair,
      true,
      L"");
  nextMessageAvailable.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      nextMessageAvailableHandles.objectClass,
      nextMessageAvailablePair,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherHandles.objectClass);
  require(
      object.isValid(),
      "timestamped default-region alternate registration returned an invalid object handle");
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return flushQueue.recorder().hasDiscovery(object) &&
            timeAdvanceAvailable.recorder().hasDiscovery(object) &&
            nextMessageAvailable.recorder().hasDiscovery(object);
      },
      options,
      "timestamped default-region alternate object discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "timestamped default-region alternate members selected different logical-time factories");
  for (auto* member :
       std::vector<Session*>{&flushQueue, &timeAdvanceAvailable, &nextMessageAvailable}) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "timestamped default-region alternate time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region alternate time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> valueBytes{0x44U, 0x45U, 0x46U, 0x2DU, 0x41U};
  std::vector<std::uint8_t> tagBytes{0x44U, 0x45U, 0x46U, 0x2DU, 0x46U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped default-region alternate update returned an invalid retraction handle");
  require(
      flushQueue.recorder().timedReflections().empty() &&
          timeAdvanceAvailable.recorder().timedReflections().empty() &&
          nextMessageAvailable.recorder().timedReflections().empty(),
      "timestamped default-region alternate recipient received a reflection before its request");
  flushQueue.recorder().clearCallbackOrder();
  timeAdvanceAvailable.recorder().clearCallbackOrder();
  nextMessageAvailable.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    flushQueue.pump();
    timeAdvanceAvailable.pump();
    nextMessageAvailable.pump();
    if (publisher.recorder().timeAdvanceGrants().size() >= 1U &&
        flushQueue.recorder().timedReflections().size() >= 1U &&
        flushQueue.recorder().flushQueueGrants().size() >= 1U &&
        timeAdvanceAvailable.recorder().timedReflections().size() >= 1U &&
        timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
        nextMessageAvailable.recorder().timedReflections().size() >= 1U &&
        nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          flushQueue.recorder().timedReflections().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().timedReflections().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().timedReflections().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped default-region alternate delivery returned the wrong callback counts");
  require(
      flushQueue.recorder().timeAdvanceGrants().empty() &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty() &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "timestamped default-region alternate delivery mixed grant types");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "flush-grant"} &&
          timeAdvanceAvailable.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "grant"} &&
          nextMessageAvailable.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "grant"},
      "timestamped default-region alternate delivery granted before reflection");

  auto assertDefaultRegionReflection = [&](TimedReflectionRecord const& record,
                                            rti::AttributeHandle const& expectedAttribute,
                                            Session& inspector) {
    require(
        record.object == object && record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U &&
            copyBytes(record.values.at(expectedAttribute)) == valueBytes,
        "timestamped default-region alternate reflection returned the wrong object or value");
    require(
        record.tag == tagBytes && record.time == encodeTime(*messageTime) &&
            !record.timeText.empty(),
        "timestamped default-region alternate reflection did not preserve value metadata");
    require(
        record.producer == publisher.federateHandle() && record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "timestamped default-region alternate reflection returned the wrong producer or order");
    require(
        record.transportation.isValid() &&
            !inspector.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "timestamped default-region alternate reflection returned an unresolvable transport");
    require(
        record.regions.has_value() && record.regions->empty(),
        "timestamped default-region alternate reflection returned a non-empty source region");
    require(
        record.retractionPresent && record.retraction == copyBytes(retraction.encode()),
        "timestamped default-region alternate reflection omitted the retraction handle");
  };
  assertDefaultRegionReflection(
      flushQueue.recorder().timedReflections().front(),
      flushQueueHandles.attribute,
      flushQueue);
  assertDefaultRegionReflection(
      timeAdvanceAvailable.recorder().timedReflections().front(),
      timeAdvanceAvailableHandles.attribute,
      timeAdvanceAvailable);
  assertDefaultRegionReflection(
      nextMessageAvailable.recorder().timedReflections().front(),
      nextMessageAvailableHandles.attribute,
      nextMessageAvailable);

  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*publisherTarget),
      "timestamped default-region alternate publisher received the wrong grant time");
  auto const flushGrants = flushQueue.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*messageTime) &&
          !flushGrants.front().time.empty() && !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "timestamped default-region alternate Flush Queue Grant returned wrong time metadata");
  auto actualFlushTime = decodeTime(*flushQueueTime.factory, flushGrants.front().time);
  auto optimisticFlushTime =
      decodeTime(*flushQueueTime.factory, flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *flushQueueTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "timestamped default-region alternate Flush Queue Grant returned time outside bounds");
  require(
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime) &&
          nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "timestamped default-region alternate grants returned the wrong logical time");
  auto queriedFlushQueue = flushQueueTime.factory->makeInitial();
  flushQueue.rtiAmbassador().queryLogicalTime(*queriedFlushQueue);
  require(
      encodeTime(*queriedFlushQueue) == flushGrants.front().time,
      "timestamped default-region Flush Queue query did not match its grant");
  auto queriedTimeAdvanceAvailable = timeAdvanceAvailableTime.factory->makeInitial();
  timeAdvanceAvailable.rtiAmbassador().queryLogicalTime(*queriedTimeAdvanceAvailable);
  requireTimeEquals(
      *queriedTimeAdvanceAvailable,
      *messageTime,
      "timestamped default-region Time Advance Request Available query");
  auto queriedNextMessageAvailable = nextMessageAvailableTime.factory->makeInitial();
  nextMessageAvailable.rtiAmbassador().queryLogicalTime(*queriedNextMessageAvailable);
  requireTimeEquals(
      *queriedNextMessageAvailable,
      *messageTime,
      "timestamped default-region Next Message Request Available query");

  auto const pastFlushQueueTime = timeAfter(
      *flushQueueTime.factory,
      *flushQueueTime.initial,
      *flushQueueTime.epsilon,
      1U);
  auto const pastTimeAdvanceAvailableTime = timeAfter(
      *timeAdvanceAvailableTime.factory,
      *timeAdvanceAvailableTime.initial,
      *timeAdvanceAvailableTime.epsilon,
      1U);
  auto const pastNextMessageAvailableTime = timeAfter(
      *nextMessageAvailableTime.factory,
      *nextMessageAvailableTime.initial,
      *nextMessageAvailableTime.epsilon,
      1U);
  requireException(
      [&] { flushQueue.rtiAmbassador().flushQueueRequest(*pastFlushQueueTime); },
      L"LogicalTimeAlreadyPassed",
      "timestamped default-region Flush Queue Request before current time");
  requireException(
      [&] {
        timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
            *pastTimeAdvanceAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "timestamped default-region Time Advance Request Available before current time");
  requireException(
      [&] {
        nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(
            *pastNextMessageAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "timestamped default-region Next Message Request Available before current time");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region alternate retraction after delivery");

  flushQueue.rtiAmbassador().disableTimeConstrained();
  timeAdvanceAvailable.rtiAmbassador().disableTimeConstrained();
  nextMessageAvailable.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  flushQueue.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      flushQueueHandles.objectClass,
      flushQueuePair);
  timeAdvanceAvailable.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      timeAdvanceAvailableHandles.objectClass,
      timeAdvanceAvailablePair);
  nextMessageAvailable.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      nextMessageAvailableHandles.objectClass,
      nextMessageAvailablePair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  flushQueue.rtiAmbassador().deleteRegion(flushQueueRegion);
  timeAdvanceAvailable.rtiAmbassador().deleteRegion(timeAdvanceAvailableRegion);
  nextMessageAvailable.rtiAmbassador().deleteRegion(nextMessageAvailableRegion);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionAttributeAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionAttributeAlternateAdvances(options, model);
}

void scenarioTimestampedDefaultRegionAttributeReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region re-enable requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation =
      federationName(options, "timestamped-default-region-attribute-reenable");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region re-enable");
  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      receiverRegionSet,
  }};
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair,
      true,
      L"");
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "timestamped default-region re-enable did not enable region designator callbacks");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherHandles.objectClass);
  require(
      object.isValid(),
      "timestamped default-region re-enable registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped default-region re-enable discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped default-region re-enable members selected different logical-time factories");
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "timestamped default-region re-enable");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> valueBytes{0x52U, 0x45U, 0x47U, 0x41U};
  std::vector<std::uint8_t> tagBytes{0x52U, 0x45U, 0x45U, 0x4EU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "timestamped default-region re-enable update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped default-region re-enable delivered before its grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped default-region re-enable delivered while time constrained was disabled");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 2U; },
      options,
      "timestamped default-region re-enable callback");
  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*updateTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 1U;
      },
      options,
      "timestamped default-region re-enable delivery");
  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U &&
          receiver.recorder().timeConstrainedEnabled().size() == 2U,
      "timestamped default-region re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() == std::vector<std::string>{"reflect", "grant"},
      "timestamped default-region re-enable delivered after its grant");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(reflection.values.at(receiverHandles.attribute)) == valueBytes,
      "timestamped default-region re-enable returned the wrong object or value");
  require(
      reflection.tag == tagBytes && reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty(),
      "timestamped default-region re-enable did not preserve value metadata");
  require(
      reflection.producer == publisher.federateHandle() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP,
      "timestamped default-region re-enable returned the wrong producer or order");
  require(
      reflection.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(reflection.transportation).empty(),
      "timestamped default-region re-enable returned an unresolvable transport");
  require(
      reflection.regions.has_value() && reflection.regions->empty(),
      "timestamped default-region re-enable returned a non-empty source region");
  require(
      reflection.retractionPresent && reflection.retraction == copyBytes(retraction.encode()),
      "timestamped default-region re-enable omitted the retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*publisherTarget),
      "timestamped default-region re-enable returned the wrong producer grant");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded == encodeTime(*updateTime),
      "timestamped default-region re-enable returned the wrong receiver grant");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region re-enable retraction after delivery");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionAttributeReenableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionAttributeReenable(options, model);
}

void scenarioTimestampedDefaultRegionAttributeRegulationReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region regulation re-enable requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region regulation re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation = federationName(
      options,
      "timestamped-default-region-attribute-regulation-reenable");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region regulation re-enable");
  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      receiverRegionSet,
  }};
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair,
      true,
      L"");
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "timestamped default-region regulation re-enable did not enable region designator callbacks");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherHandles.objectClass);
  require(
      object.isValid(),
      "timestamped default-region regulation re-enable registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped default-region regulation re-enable discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped default-region regulation re-enable members selected different logical-time factories");
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "timestamped default-region regulation re-enable");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const changedLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto changedLookahead = publisherTime.factory->makeZero();
  require(
      changedLookahead != nullptr,
      "timestamped default-region regulation re-enable could not allocate the changed lookahead");
  changedLookahead->setToDifference(
      *changedLookaheadTime,
      *publisherTime.initial);

  std::vector<std::uint8_t> valueBytes{0x52U, 0x45U, 0x47U, 0x52U};
  std::vector<std::uint8_t> tagBytes{0x52U, 0x45U, 0x47U, 0x41U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "timestamped default-region regulation re-enable update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped default-region regulation re-enable delivered before its grant");

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().enableTimeRegulation(*changedLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 2U; },
      options,
      "timestamped default-region regulation re-enable callback");
  auto queriedLookahead = publisherTime.factory->makeZero();
  require(
      queriedLookahead != nullptr,
      "timestamped default-region regulation re-enable could not allocate a lookahead query");
  publisher.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(
      *queriedLookahead == *changedLookahead,
      "timestamped default-region regulation re-enable did not expose the changed lookahead");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*updateTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 1U;
      },
      options,
      "timestamped default-region regulation re-enable delivery");
  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U &&
          publisher.recorder().timeRegulationEnabled().size() == 2U,
      "timestamped default-region regulation re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() == std::vector<std::string>{"reflect", "grant"},
      "timestamped default-region regulation re-enable delivered after its grant");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(reflection.values.at(receiverHandles.attribute)) == valueBytes,
      "timestamped default-region regulation re-enable returned the wrong object or value");
  require(
      reflection.tag == tagBytes && reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty(),
      "timestamped default-region regulation re-enable did not preserve value metadata");
  require(
      reflection.producer == publisher.federateHandle() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP,
      "timestamped default-region regulation re-enable returned the wrong producer or order");
  require(
      reflection.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(reflection.transportation).empty(),
      "timestamped default-region regulation re-enable returned an unresolvable transport");
  require(
      reflection.regions.has_value() && reflection.regions->empty(),
      "timestamped default-region regulation re-enable returned a non-empty source region");
  require(
      reflection.retractionPresent && reflection.retraction == copyBytes(retraction.encode()),
      "timestamped default-region regulation re-enable omitted the retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*publisherTarget),
      "timestamped default-region regulation re-enable returned the wrong producer grant");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded == encodeTime(*updateTime),
      "timestamped default-region regulation re-enable returned the wrong receiver grant");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region regulation re-enable retraction after delivery");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionAttributeRegulationReenableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionAttributeRegulationReenable(options, model);
}

void scenarioTimestampedDefaultRegionAttributeMixedFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region attribute mixed fanout requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region attribute mixed fanout requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session constrained(options, model, "constrained");
  Session immediate(options, model, "immediate");
  auto const federation = federationName(
      options,
      "timestamped-default-region-attribute-mixed-fanout");
  connectAndJoin(publisher, constrained, options, federation, options.ddmFom);
  immediate.connect();
  immediate.join(
      options.memberFederateName + L"-default-region-attribute-mixed-immediate",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const constrainedHandles = ddmHandles(constrained, options);
  auto const immediateHandles = ddmHandles(immediate, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region attribute mixed fanout");
  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const constrainedAttributes{constrainedHandles.attribute};
  rti::AttributeHandleSet const immediateAttributes{immediateHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const constrainedRegion = createDdmRegion(
      constrained,
      constrainedHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const immediateRegion = createDdmRegion(
      immediate,
      immediateHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const constrainedRegionSet = rti::RegionHandleSet{constrainedRegion};
  auto const immediateRegionSet = rti::RegionHandleSet{immediateRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const constrainedPair{{
      constrainedAttributes,
      constrainedRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const immediatePair{{
      immediateAttributes,
      immediateRegionSet,
  }};
  constrained.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  immediate.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      constrained.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          immediate.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "timestamped default-region attribute mixed fanout did not enable region designator callbacks");
  constrained.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      constrainedHandles.objectClass,
      constrainedPair,
      true,
      L"");
  immediate.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      immediateHandles.objectClass,
      immediatePair,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherHandles.objectClass);
  require(
      object.isValid(),
      "timestamped default-region attribute mixed fanout registration returned an invalid object handle");
  waitForSessions(
      {&publisher, &constrained, &immediate},
      [&] {
        return constrained.recorder().hasDiscovery(object) &&
            immediate.recorder().hasDiscovery(object);
      },
      options,
      "timestamped default-region attribute mixed fanout object discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      publisherTime.factory->getName() == constrainedTime.factory->getName(),
      "timestamped default-region attribute mixed fanout members selected different logical-time factories");
  auto makeLookahead = [](TimeContext const& time, std::size_t steps) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, steps);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped default-region attribute mixed fanout could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime, 5U);
  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped default-region attribute mixed fanout time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region attribute mixed fanout time-regulation callback");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> valueBytes{0x44U, 0x45U, 0x46U, 0x41U};
  std::vector<std::uint8_t> tagBytes{0x44U, 0x45U, 0x46U, 0x2DU, 0x54U, 0x53U, 0x4FU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  constrained.recorder().clearCallbackOrder();
  immediate.recorder().clearCallbackOrder();
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "timestamped default-region attribute mixed fanout update returned an invalid retraction handle");
  require(
      constrained.recorder().timedReflections().empty(),
      "timestamped default-region attribute mixed fanout delivered the constrained copy before its frontier");

  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 1U; },
      options,
      "timestamped default-region attribute mixed fanout immediate delivery");
  auto const immediateReflections = immediate.recorder().timedReflections();
  require(
      immediateReflections.size() == 1U,
      "timestamped default-region attribute mixed fanout delivered duplicate immediate reflections");
  auto const& immediateReflection = immediateReflections.front();
  require(
      immediateReflection.object == object &&
          immediateReflection.values.size() == 1U &&
          immediateReflection.values.count(immediateHandles.attribute) == 1U &&
          copyBytes(immediateReflection.values.at(immediateHandles.attribute)) == valueBytes,
      "timestamped default-region attribute mixed fanout returned the wrong immediate object or value");
  require(
      immediateReflection.tag == tagBytes &&
          immediateReflection.time == encodeTime(*updateTime) &&
          !immediateReflection.timeText.empty(),
      "timestamped default-region attribute mixed fanout did not preserve immediate value metadata");
  require(
      immediateReflection.producer == publisher.federateHandle() &&
          immediateReflection.sentOrder == rti::TIMESTAMP &&
          immediateReflection.receivedOrder == rti::RECEIVE,
      "timestamped default-region attribute mixed fanout returned the wrong immediate producer or order");
  require(
      immediateReflection.transportation.isValid() &&
          !immediate.rtiAmbassador().getTransportationTypeName(
              immediateReflection.transportation).empty(),
      "timestamped default-region attribute mixed fanout returned an unresolvable immediate transport");
  require(
      immediateReflection.regions.has_value() && immediateReflection.regions->empty(),
      "timestamped default-region attribute mixed fanout returned a non-empty immediate source region");
  require(
      immediateReflection.retractionPresent &&
          immediateReflection.retraction == copyBytes(retraction.encode()),
      "timestamped default-region attribute mixed fanout omitted the immediate retraction handle");
  require(
      immediate.recorder().callbackOrder() == std::vector<std::string>{"reflect"},
      "timestamped default-region attribute mixed fanout returned the wrong immediate callback order");

  constrained.rtiAmbassador().timeAdvanceRequest(*updateTime);
  require(
      constrained.recorder().timeAdvanceGrants().empty(),
      "timestamped default-region attribute mixed fanout granted before the publisher frontier");
  publisher.rtiAmbassador().retract(retraction);
  require(
      constrained.recorder().timedReflections().empty() &&
          constrained.recorder().callbackOrder().empty(),
      "timestamped default-region attribute mixed fanout leaked the retracted constrained callback");
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      options,
      "timestamped default-region attribute mixed fanout immediate retraction callback");
  auto const retractions = immediate.recorder().retractions();
  require(
      retractions.size() == 1U && retractions.front().valid &&
          retractions.front().encoded == copyBytes(retraction.encode()),
      "timestamped default-region attribute mixed fanout returned the wrong retraction callback");
  require(
      immediate.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "request-retraction"},
      "timestamped default-region attribute mixed fanout returned the wrong immediate retraction order");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped default-region attribute mixed fanout grants");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          constrained.recorder().timeAdvanceGrants().size() == 1U &&
          constrained.recorder().timedReflections().empty() &&
          constrained.recorder().retractions().empty(),
      "timestamped default-region attribute mixed fanout returned the wrong constrained callback counts");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget) &&
          constrained.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*updateTime),
      "timestamped default-region attribute mixed fanout returned the wrong grants");
  require(
      constrained.recorder().callbackOrder() == std::vector<std::string>{"grant"},
      "timestamped default-region attribute mixed fanout granted after a suppressed callback");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region attribute mixed fanout terminal retraction");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  constrained.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      constrainedHandles.objectClass,
      constrainedPair);
  immediate.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      immediateHandles.objectClass,
      immediatePair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  constrained.rtiAmbassador().deleteRegion(constrainedRegion);
  immediate.rtiAmbassador().deleteRegion(immediateRegion);
  constrained.resign(rti::NO_ACTION);
  immediate.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  immediate.disconnect();
  constrained.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionAttributeMixedFanoutContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionAttributeMixedFanout(options, model);
}

void scenarioTimestampedAttributeUpdateTarPostGrantCutoff(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "timestamped attribute post-grant cutoff requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  Session frontier(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-tar-post-grant-cutoff");
  connectAndJoin(publisher, receiver, options, federation, options.fom);
  frontier.connect();
  frontier.join(
      options.memberFederateName + L"-frontier",
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
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "timestamped attribute post-grant cutoff could not resolve the adapter class or attribute");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet receiverAttributes;
  receiverAttributes.insert(receiverAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "timestamped attribute post-grant cutoff registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped attribute post-grant cutoff discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  auto frontierTime = makeTimeContext(frontier);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName() &&
          publisherTime.factory->getName() == frontierTime.factory->getName() &&
          encodeTime(*publisherTime.initial) == encodeTime(*receiverTime.initial) &&
          encodeTime(*publisherTime.initial) == encodeTime(*frontierTime.initial),
      "timestamped attribute post-grant cutoff members selected incompatible logical times");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped attribute post-grant cutoff receiver time-constrained callback");
  frontier.rtiAmbassador().enableTimeRegulation(*frontierTime.epsilon);
  waitFor(
      frontier,
      [&] { return frontier.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped attribute post-grant cutoff frontier time-regulation callback");

  auto const receiverAtFive = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      5U);
  auto const frontierAtFive = timeAfter(
      *frontierTime.factory,
      *frontierTime.initial,
      *frontierTime.epsilon,
      5U);
  frontier.rtiAmbassador().timeAdvanceRequest(*frontierAtFive);
  waitFor(
      frontier,
      [&] { return frontier.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped attribute post-grant cutoff frontier TAR(5)");
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverAtFive);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped attribute post-grant cutoff completed receiver TAR(5)");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*receiverAtFive) &&
          receiver.recorder().timedReflections().empty(),
      "receiver did not complete TAR(5) without a timestamped attribute reflection");

  // The publisher joins the time-regulating set only after the receiver has
  // completed TAR(5), making these attribute updates late TSO arrivals.
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped attribute post-grant cutoff late-publisher regulation callback");

  auto const belowTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const equalTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherAtFour = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const publisherAtSix = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const frontierAtSix = timeAfter(
      *frontierTime.factory,
      *frontierTime.initial,
      *frontierTime.epsilon,
      6U);
  auto const receiverAtSix = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);

  std::vector<std::uint8_t> belowValue{0x41U, 0x54U, 0x34U};
  std::vector<std::uint8_t> belowTagBytes{'b', 'e', 'l', 'o', 'w'};
  rti::AttributeHandleValueMap belowValues;
  belowValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(belowValue.data(), belowValue.size()));
  rti::VariableLengthData belowTag(belowTagBytes.data(), belowTagBytes.size());
  try {
    auto const belowRetraction = publisher.rtiAmbassador().updateAttributeValues(
        object,
        belowValues,
        belowTag,
        *belowTime);
    require(
        belowRetraction.isValid(),
        "accepted timestamp-4 attribute update returned an invalid retraction handle");
  } catch (rti::InvalidLogicalTime const&) {
    // The conformance assertion is the delivery cutoff, not sender acceptance.
  }

  std::vector<std::uint8_t> equalValue{0x41U, 0x54U, 0x35U};
  std::vector<std::uint8_t> equalTagBytes{'e', 'q', 'u', 'a', 'l'};
  rti::AttributeHandleValueMap equalValues;
  equalValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(equalValue.data(), equalValue.size()));
  rti::VariableLengthData equalTag(equalTagBytes.data(), equalTagBytes.size());
  auto const equalRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      equalValues,
      equalTag,
      *equalTime);
  require(
      equalRetraction.isValid(),
      "timestamp-5 attribute update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "late timestamped attribute update crossed the completed TAR(5) boundary immediately");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAtFour);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped attribute post-grant cutoff publisher TAR(4)");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped attribute update at or below 5 arrived while publisher advanced to 4");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAtSix);
  frontier.rtiAmbassador().timeAdvanceRequest(*frontierAtSix);
  waitForSessions(
      {&publisher, &frontier},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            frontier.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "timestamped attribute post-grant cutoff producer frontiers at 6");
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverAtSix);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() >= 2U; },
      options,
      "timestamped attribute post-grant cutoff receiver TAR(6)");
  require(
      receiver.recorder().timedReflections().empty(),
      "an attribute update at or below the completed TAR(5) value was delivered later");

  auto const publisherAtSeven = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const publisherAtEight = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  auto const frontierAtEight = timeAfter(
      *frontierTime.factory,
      *frontierTime.initial,
      *frontierTime.epsilon,
      8U);
  auto const receiverAtEight = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      8U);
  std::vector<std::uint8_t> positiveValue{0x41U, 0x54U, 0x37U};
  std::vector<std::uint8_t> positiveTagBytes{'a', 'f', 't', 'e', 'r'};
  rti::AttributeHandleValueMap positiveValues;
  positiveValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(positiveValue.data(), positiveValue.size()));
  rti::VariableLengthData positiveTag(
      positiveTagBytes.data(),
      positiveTagBytes.size());
  auto const positiveRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      positiveValues,
      positiveTag,
      *publisherAtSeven);
  require(
      positiveRetraction.isValid(),
      "timestamp-7 positive-control update returned an invalid retraction handle");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAtEight);
  frontier.rtiAmbassador().timeAdvanceRequest(*frontierAtEight);
  waitForSessions(
      {&publisher, &frontier},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 3U &&
            frontier.recorder().timeAdvanceGrants().size() >= 3U;
      },
      options,
      "timestamped attribute post-grant cutoff positive-control frontiers at 8");
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverAtEight);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() >= 3U; },
      options,
      "timestamped attribute post-grant cutoff positive-control receiver TAR(8)");
  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U && reflections.front().object == object &&
          reflections.front().values.size() == 1U &&
          reflections.front().values.count(receiverAttribute) == 1U &&
          copyBytes(reflections.front().values.at(receiverAttribute)) == positiveValue &&
          reflections.front().tag == positiveTagBytes &&
          reflections.front().time == encodeTime(*publisherAtSeven) &&
          !reflections.front().timeText.empty() &&
          reflections.front().sentOrder == rti::TIMESTAMP &&
          reflections.front().receivedOrder == rti::TIMESTAMP,
      "post-cutoff timestamp-7 positive control did not reflect exactly once with its value and time");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  frontier.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  frontier.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  frontier.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeUpdateTarPostGrantCutoffContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateTarPostGrantCutoff(options, model);
}


}  // namespace hla_rti_cpp_tck::detail
