#include "tck_federation_lifecycle_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederationListServices(Options const& options, rti::CallbackModel model) {
  Session observer(options, model, "federation-list-observer");
  Session owner(options, model, "owner");
  Session member(options, model, "federation-list-member");
  Session stale(options, model, "federation-list-stale");

  auto const firstFederation =
      federationName(options, "federation-list-primary") + L"-\u03A9-\u00E9";
  auto const secondFederation = options.federationOverride.empty()
      ? federationName(options, "federation-list-secondary")
      : firstFederation + L"-secondary";
  auto const missingFederation = options.federationOverride.empty()
      ? federationName(options, "federation-list-missing")
      : firstFederation + L"-missing";
  auto const ownerFederateName = options.ownerFederateName + L"-\u00E9";
  auto const memberFederateName = options.memberFederateName + L"-\u03A9";
  auto const expectedFederations = std::map<std::wstring, std::wstring>{
      {firstFederation, options.logicalTimeImplementationName},
      {secondFederation, options.logicalTimeImplementationName}};
  auto const expectedFederationMembers = std::map<std::wstring, std::wstring>{
      {ownerFederateName, options.federateType},
      {memberFederateName, options.federateType}};
  auto const expectedRemainingFederationMembers = std::map<std::wstring, std::wstring>{
      {ownerFederateName, options.federateType}};

  requireException(
      [&] { observer.rtiAmbassador().listFederationExecutions(); },
      L"NotConnected",
      "listing federation executions before connect");
  requireException(
      [&] {
        observer.rtiAmbassador().listFederationExecutionMembers(firstFederation);
      },
      L"NotConnected",
      "listing federation members before connect");

  observer.connect();
  owner.connect();
  member.connect();
  stale.connect();
  owner.rtiAmbassador().createFederationExecution(
      firstFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.rtiAmbassador().createFederationExecution(
      secondFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(ownerFederateName, options.federateType, firstFederation);
  member.join(memberFederateName, options.federateType, firstFederation);

  auto requireCallbackBoundary = [&](auto const& predicate, std::string const& description) {
    if (model == rti::HLA_EVOKED) {
      require(!predicate(), description + " was delivered before the Evoke boundary");
    } else {
      require(predicate(), description + " was not delivered by the immediate callback boundary");
    }
  };

  observer.rtiAmbassador().listFederationExecutions();
  requireCallbackBoundary(
      [&] { return observer.recorder().listsFederationsInSingleReport(expectedFederations); },
      "federation execution report");
  waitFor(
      observer,
      [&] { return observer.recorder().listsFederationsInSingleReport(expectedFederations); },
      options,
      "the complete federation execution report");

  observer.rtiAmbassador().listFederationExecutionMembers(firstFederation);
  requireCallbackBoundary(
      [&] {
        return observer.recorder().listsMembersInSingleReport(
            firstFederation, expectedFederationMembers);
      },
      "federation member report");
  waitFor(
      observer,
      [&] {
        return observer.recorder().listsMembersInSingleReport(
            firstFederation, expectedFederationMembers);
      },
      options,
      "the federation member report");

  observer.rtiAmbassador().listFederationExecutionMembers(missingFederation);
  requireCallbackBoundary(
      [&] { return observer.recorder().reportsMissing(missingFederation); },
      "federation-does-not-exist report");
  waitFor(
      observer,
      [&] { return observer.recorder().reportsMissing(missingFederation); },
      options,
      "the federation-does-not-exist report");

  stale.rtiAmbassador().listFederationExecutions();
  if (model == rti::HLA_EVOKED) {
    require(
        !stale.recorder().listsFederation(
            firstFederation,
            options.logicalTimeImplementationName),
        "stale evoked federation report was delivered before Evoke");
    stale.disconnect();
    require(
        !stale.recorder().listsFederation(
            firstFederation,
            options.logicalTimeImplementationName),
        "disconnect retained a queued federation report");
  } else {
    require(
        stale.recorder().listsFederation(
            firstFederation,
            options.logicalTimeImplementationName),
        "immediate federation report was not delivered");
    stale.disconnect();
  }

  member.resign(rti::NO_ACTION);
  require(
      member.recorder().federateResignedReasons().empty(),
      "ordinary federate-initiated resignation synthesized federateResigned");
  member.disconnect();
  observer.rtiAmbassador().listFederationExecutionMembers(firstFederation);
  requireCallbackBoundary(
      [&] {
        return observer.recorder().listsMembersInSingleReport(
            firstFederation, expectedRemainingFederationMembers);
      },
      "post-resignation federation member report");
  waitFor(
      observer,
      [&] {
        return observer.recorder().listsMembersInSingleReport(
            firstFederation, expectedRemainingFederationMembers);
      },
      options,
      "the post-resignation federation member report");

  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(firstFederation);
  owner.rtiAmbassador().destroyFederationExecution(secondFederation);
  observer.rtiAmbassador().listFederationExecutions();
  waitFor(
      observer,
      [&] {
        return observer.recorder().lastFederationExecutionReportOmits(
                   firstFederation) &&
            observer.recorder().lastFederationExecutionReportOmits(
                secondFederation);
      },
      options,
      "the post-destroy federation execution report");
  observer.rtiAmbassador().listFederationExecutionMembers(firstFederation);
  requireCallbackBoundary(
      [&] { return observer.recorder().reportsMissing(firstFederation); },
      "destroyed-federation report");
  waitFor(
      observer,
      [&] { return observer.recorder().reportsMissing(firstFederation); },
      options,
      "the destroyed-federation report");
  observer.disconnect();
}

void scenarioFederationListServicesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationListServices(options, model);
}

void scenarioFederateLookupLifecycle(Options const& options, rti::CallbackModel model) {
  Session unjoined(options, model, "federate-lookup-unjoined");
  Session owner(options, model, "owner");
  Session peer(options, model, "federate-lookup-peer");
  Session foreignCreator(options, model, "owner");
  Session foreignMember(options, model, "federate-lookup-foreign");

  auto const federation = federationName(options, "federate-lookup");
  auto const foreignFederation = options.federationOverride.empty()
      ? federationName(options, "federate-lookup-foreign")
      : federation + L"-foreign";

  requireException(
      [&] {
        static_cast<void>(unjoined.rtiAmbassador().getFederateHandle(
            options.ownerFederateName));
      },
      L"NotConnected",
      "looking up a federate handle before connect");
  requireException(
      [&] {
        static_cast<void>(unjoined.rtiAmbassador().getFederateName(
            rti::FederateHandle{}));
      },
      L"NotConnected",
      "looking up a federate name before connect");

  unjoined.connect();
  requireException(
      [&] {
        static_cast<void>(unjoined.rtiAmbassador().getFederateHandle(
            options.ownerFederateName));
      },
      L"FederateNotExecutionMember",
      "looking up a federate handle before joining");
  requireException(
      [&] {
        static_cast<void>(unjoined.rtiAmbassador().getFederateName(
            rti::FederateHandle{}));
      },
      L"FederateNotExecutionMember",
      "looking up a federate name before joining");

  owner.connect();
  peer.connect();
  foreignCreator.connect();
  foreignMember.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  foreignCreator.rtiAmbassador().createFederationExecution(
      foreignFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  peer.join(options.memberFederateName, options.federateType, federation);
  auto const foreignName = options.memberFederateName + L"-foreign";
  foreignMember.join(foreignName, options.federateType, foreignFederation);

  auto const ownerHandle = owner.federateHandle();
  auto const peerHandle = peer.federateHandle();
  auto const foreignHandle = foreignMember.federateHandle();
  require(
      owner.rtiAmbassador().getFederateHandle(options.ownerFederateName) == ownerHandle,
      "owner federate name-to-handle lookup did not round-trip");
  require(
      owner.rtiAmbassador().getFederateHandle(options.memberFederateName) == peerHandle,
      "peer federate name-to-handle lookup did not round-trip");
  require(
      peer.rtiAmbassador().getFederateName(ownerHandle) == options.ownerFederateName,
      "owner federate handle-to-name lookup did not round-trip");
  require(
      peer.rtiAmbassador().getFederateName(peerHandle) == options.memberFederateName,
      "peer federate handle-to-name lookup did not round-trip");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(
            L"tck-missing-federate"));
      },
      L"NameNotFound",
      "missing federate name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(foreignName));
      },
      L"NameNotFound",
      "looking up a federate name that is joined only in another execution");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateName(
            rti::FederateHandle{}));
      },
      L"InvalidFederateHandle",
      "invalid federate handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateName(foreignHandle));
      },
      L"FederateHandleNotKnown",
      "foreign federate handle lookup");

  peer.resign(rti::NO_ACTION);
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(
            options.memberFederateName));
      },
      L"NameNotFound",
      "departed federate name lookup");
  require(
      owner.rtiAmbassador().getFederateName(peerHandle) == options.memberFederateName,
      "departed federate handle did not retain its designator identity");

  foreignMember.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  foreignCreator.rtiAmbassador().destroyFederationExecution(foreignFederation);
  unjoined.disconnect();
  owner.disconnect();
  peer.disconnect();
  foreignCreator.disconnect();
  foreignMember.disconnect();
}

void scenarioFederateLookupLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateLookupLifecycle(options, model);
}

void scenarioFederationLifecycle(Options const& options, rti::CallbackModel model) {
  Session directiveLifecycle(options, model, "automatic-resign-lifecycle");
  auto requireDirectiveLifecycle = [&](std::wstring const& expected,
                                       std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(directiveLifecycle.rtiAmbassador()
                                .getAutomaticResignDirective());
        },
        expected,
        "getting the automatic resign directive " + phase);
    requireException(
        [&] {
          directiveLifecycle.rtiAmbassador().setAutomaticResignDirective(
              rti::NO_ACTION);
        },
        expected,
        "setting the automatic resign directive " + phase);
  };
  requireDirectiveLifecycle(L"NotConnected", "before connect");
  directiveLifecycle.connect();
  requireDirectiveLifecycle(
      L"FederateNotExecutionMember",
      "after connect before joining an execution");
  directiveLifecycle.disconnect();

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  Session observer(options, model, "observer");
  auto const federation = federationName(options, "lifecycle");
  owner.connect();
  member.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  requireException(
      [&] {
        observer.rtiAmbassador().createFederationExecution(
            federation,
            options.fom.wstring(),
            options.logicalTimeImplementationName);
      },
      L"FederationExecutionAlreadyExists",
      "duplicate federation execution creation");
  owner.join(options.ownerFederateName, options.federateType, federation);
  requireException(
      [&] {
        owner.rtiAmbassador().joinFederationExecution(
            options.ownerFederateName,
            options.federateType,
            federation);
      },
      L"FederateAlreadyExecutionMember",
      "duplicate federation membership");
  member.join(options.memberFederateName, options.federateType, federation);
  requireException(
      [&] {
        observer.rtiAmbassador().joinFederationExecution(
            options.ownerFederateName,
            options.federateType,
            federation);
      },
      L"FederateNameAlreadyInUse",
      "duplicate federate name membership");

  auto const originalAutomaticResign = owner.rtiAmbassador().getAutomaticResignDirective();
  for (auto const directive : std::vector<rti::ResignAction>{
           rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES,
           rti::DELETE_OBJECTS,
           rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
           rti::DELETE_OBJECTS_THEN_DIVEST,
           rti::CANCEL_THEN_DELETE_THEN_DIVEST,
           rti::NO_ACTION}) {
    owner.rtiAmbassador().setAutomaticResignDirective(directive);
    require(
        owner.rtiAmbassador().getAutomaticResignDirective() == directive,
        "Automatic resign directive did not round-trip");
  }
  owner.rtiAmbassador().setAutomaticResignDirective(originalAutomaticResign);

  require(owner.rtiAmbassador().getFederateHandle(options.ownerFederateName) ==
              owner.federateHandle(),
          "Federate name-to-handle lookup did not round-trip");
  require(owner.rtiAmbassador().getFederateName(owner.federateHandle()) ==
              options.ownerFederateName,
          "Federate handle-to-name lookup did not round-trip");

  observer.rtiAmbassador().listFederationExecutions();
  waitFor(observer, [&] { return observer.recorder().listsFederation(federation); },
          options, "the federation execution report");

  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(observer, [&] {
    return observer.recorder().listsMembers(
        federation, {options.ownerFederateName, options.memberFederateName}, 2U);
  }, options, "the complete federation member report");
  requireException(
      [&] { observer.rtiAmbassador().destroyFederationExecution(federation); },
      L"FederatesCurrentlyJoined",
      "destroying a federation with joined federates");

  member.resign(rti::NO_ACTION);
  member.disconnect();
  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(observer, [&] {
    return observer.recorder().listsMembers(federation, {options.ownerFederateName}, 1U);
  }, options, "the post-resign member report");

  owner.resign(rti::NO_ACTION);
  require(
      owner.recorder().federateResignedReasons().empty(),
      "owner federate-initiated resignation synthesized federateResigned");
  owner.disconnect();
  observer.rtiAmbassador().destroyFederationExecution(federation);
  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(observer, [&] { return observer.recorder().reportsMissing(federation); },
          options, "the federation-does-not-exist report");
  requireException(
      [&] {
        observer.rtiAmbassador().joinFederationExecution(
            options.ownerFederateName,
            options.federateType,
            federation);
      },
      L"FederationExecutionDoesNotExist",
      "joining a destroyed federation");
  observer.disconnect();
}

void scenarioFederationLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationLifecycle(options, model);
}

void scenarioApplicationMultipleFederatesAcrossExecutions(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "multiple-execution participation requires an adapter-supplied FOM");

  Session creator(options, model, "multiple-executions-creator");
  Session firstExecutionOne(options, model, "multiple-executions-first-one");
  Session firstExecutionTwo(options, model, "multiple-executions-first-two");
  Session secondExecutionOne(options, model, "multiple-executions-second-one");
  Session secondExecutionTwo(options, model, "multiple-executions-second-two");
  auto const caseVariant = [](std::wstring name) {
    for (wchar_t& character : name) {
      if (character >= L'a' && character <= L'z') {
        character = static_cast<wchar_t>(L'A' + (character - L'a'));
        return name;
      }
    }
    return std::wstring{};
  };
  auto const firstFederation =
      federationName(options, "application-multiple-federates-first-execution");
  auto const secondFederation = caseVariant(firstFederation);
  auto const firstOneName = std::wstring(L"multiple-executions-first-one");
  auto const firstTwoName = caseVariant(firstOneName);
  auto const secondOneName = firstOneName;
  auto const secondTwoName = firstTwoName;
  require(
      secondFederation != firstFederation && firstTwoName != firstOneName &&
          secondOneName == firstOneName && secondTwoName == firstTwoName,
      "independent-execution fixture did not produce the intended case variants and reused names");

  static_cast<void>(creator.connect());
  static_cast<void>(firstExecutionOne.connect());
  static_cast<void>(firstExecutionTwo.connect());
  static_cast<void>(secondExecutionOne.connect());
  static_cast<void>(secondExecutionTwo.connect());
  creator.rtiAmbassador().createFederationExecution(
      firstFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            firstFederation,
            options.fom.wstring(),
            options.logicalTimeImplementationName);
      },
      L"FederationExecutionAlreadyExists",
      "creating a federation execution with an exact duplicate name");
  creator.rtiAmbassador().createFederationExecution(
      secondFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  auto const expectedFederations = std::map<std::wstring, std::wstring>{
      {firstFederation, options.logicalTimeImplementationName},
      {secondFederation, options.logicalTimeImplementationName}};
  creator.rtiAmbassador().listFederationExecutions();
  waitFor(
      creator,
      [&] { return creator.recorder().listsFederationsInSingleReport(expectedFederations); },
      options,
      "two federation executions whose names differ only by case");

  firstExecutionOne.join(
      firstOneName, options.federateType, firstFederation);
  requireException(
      [&] {
        firstExecutionTwo.rtiAmbassador().joinFederationExecution(
            firstOneName, options.federateType, firstFederation);
      },
      L"FederateNameAlreadyInUse",
      "joining with an exact duplicate federate name");
  firstExecutionTwo.join(
      firstTwoName, options.federateType, firstFederation);
  secondExecutionOne.join(
      secondOneName, options.federateType, secondFederation);
  secondExecutionTwo.join(
      secondTwoName, options.federateType, secondFederation);

  auto const* firstOneAmbassador = &firstExecutionOne.rtiAmbassador();
  auto const* firstTwoAmbassador = &firstExecutionTwo.rtiAmbassador();
  auto const* secondOneAmbassador = &secondExecutionOne.rtiAmbassador();
  auto const* secondTwoAmbassador = &secondExecutionTwo.rtiAmbassador();
  require(
      firstOneAmbassador != firstTwoAmbassador &&
          firstOneAmbassador != secondOneAmbassador &&
          firstOneAmbassador != secondTwoAmbassador &&
          firstTwoAmbassador != secondOneAmbassador &&
          firstTwoAmbassador != secondTwoAmbassador &&
          secondOneAmbassador != secondTwoAmbassador,
      "one application did not receive distinct RTIambassador objects");
  require(
      firstExecutionOne.federateHandle().isValid() &&
          firstExecutionTwo.federateHandle().isValid() &&
          secondExecutionOne.federateHandle().isValid() &&
          secondExecutionTwo.federateHandle().isValid(),
      "one application could not join multiple federates, including reused federate names, across two executions");
  auto const expectedFirstExecutionMembers =
      std::map<std::wstring, std::wstring>{
          {firstOneName, options.federateType},
          {firstTwoName, options.federateType}};
  creator.rtiAmbassador().listFederationExecutionMembers(firstFederation);
  waitFor(
      creator,
      [&] {
        return creator.recorder().listsMembersInSingleReport(
            firstFederation, expectedFirstExecutionMembers);
      },
      options,
      "the first execution member set with case-variant names");
  auto const expectedSecondExecutionMembers = expectedFirstExecutionMembers;
  creator.rtiAmbassador().listFederationExecutionMembers(secondFederation);
  waitFor(
      creator,
      [&] {
        return creator.recorder().listsMembersInSingleReport(
            secondFederation, expectedSecondExecutionMembers);
      },
      options,
      "the second execution's independent member set with the same federate names");

  auto const publisherClass =
      firstExecutionOne.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const publisherAttribute = firstExecutionOne.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const localSubscriberClass =
      firstExecutionTwo.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const localSubscriberAttribute = firstExecutionTwo.rtiAmbassador().getAttributeHandle(
      localSubscriberClass,
      options.attributeName);
  auto const otherSubscriberClass =
      secondExecutionOne.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const otherSubscriberAttribute = secondExecutionOne.rtiAmbassador().getAttributeHandle(
      otherSubscriberClass,
      options.attributeName);
  auto const publisherInteraction = firstExecutionOne.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = firstExecutionOne.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const localSubscriberInteraction =
      firstExecutionTwo.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const localSubscriberParameter =
      firstExecutionTwo.rtiAmbassador().getParameterHandle(
          localSubscriberInteraction,
          options.parameterName);
  auto const otherSubscriberInteraction =
      secondExecutionOne.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const otherSubscriberParameter =
      secondExecutionOne.rtiAmbassador().getParameterHandle(
          otherSubscriberInteraction,
          options.parameterName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          localSubscriberClass.isValid() && localSubscriberAttribute.isValid() &&
          otherSubscriberClass.isValid() && otherSubscriberAttribute.isValid() &&
          publisherInteraction.isValid() && publisherParameter.isValid() &&
          localSubscriberInteraction.isValid() && localSubscriberParameter.isValid() &&
          otherSubscriberInteraction.isValid() && otherSubscriberParameter.isValid(),
      "independent executions did not expose the selected FOM object, attribute, and interaction");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const localSubscriberAttributes{localSubscriberAttribute};
  rti::AttributeHandleSet const otherSubscriberAttributes{otherSubscriberAttribute};
  firstExecutionOne.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  firstExecutionTwo.rtiAmbassador().subscribeObjectClassAttributes(
      localSubscriberClass,
      localSubscriberAttributes,
      true,
      L"");
  secondExecutionOne.rtiAmbassador().subscribeObjectClassAttributes(
      otherSubscriberClass,
      otherSubscriberAttributes,
      true,
      L"");
  firstExecutionOne.rtiAmbassador().publishInteractionClass(publisherInteraction);
  firstExecutionTwo.rtiAmbassador().subscribeInteractionClass(
      localSubscriberInteraction,
      true);
  secondExecutionOne.rtiAmbassador().subscribeInteractionClass(
      otherSubscriberInteraction,
      true);

  auto const otherExecutionDiscoveryCount =
      secondExecutionOne.recorder().discoveryCount();
  auto const otherExecutionReflectionCount =
      secondExecutionOne.recorder().reflectionCount();
  auto const otherExecutionInteractionCount =
      secondExecutionOne.recorder().interactions().size();
  auto const object =
      firstExecutionOne.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "isolated-execution control registration returned an invalid handle");
  waitFor(
      firstExecutionTwo,
      [&] { return firstExecutionTwo.recorder().hasDiscovery(object); },
      options,
      "same-execution object discovery control");

  std::vector<rti::Octet> valueBytes{static_cast<rti::Octet>(0x2aU)};
  rti::VariableLengthData const value(valueBytes.data(), valueBytes.size());
  rti::AttributeHandleValueMap const values{{publisherAttribute, value}};
  rti::VariableLengthData const tag;
  auto const localReflectionCount = firstExecutionTwo.recorder().reflectionCount();
  firstExecutionOne.rtiAmbassador().updateAttributeValues(object, values, tag);
  waitFor(
      firstExecutionTwo,
      [&] {
        return firstExecutionTwo.recorder().reflectionCount() > localReflectionCount;
      },
      options,
      "same-execution attribute reflection control");
  auto const localReflection = firstExecutionTwo.recorder().reflection();
  require(
      localReflection.object == object &&
          localReflection.values.count(localSubscriberAttribute) == 1U,
      "same-execution control did not reflect the published attribute");

  std::vector<rti::Octet> interactionValueBytes{static_cast<rti::Octet>(0x2bU)};
  rti::VariableLengthData const interactionValue(
      interactionValueBytes.data(), interactionValueBytes.size());
  rti::ParameterHandleValueMap const interactionValues{
      {publisherParameter, interactionValue}};
  rti::VariableLengthData const interactionTag;
  auto const localInteractionCount =
      firstExecutionTwo.recorder().interactions().size();
  firstExecutionOne.rtiAmbassador().sendInteraction(
      publisherInteraction,
      interactionValues,
      interactionTag);
  waitFor(
      firstExecutionTwo,
      [&] {
        return firstExecutionTwo.recorder().interactions().size() >
            localInteractionCount;
      },
      options,
      "same-execution interaction delivery control");
  auto const localInteraction = firstExecutionTwo.recorder().interaction();
  require(
      localInteraction.interaction == localSubscriberInteraction &&
          localInteraction.parameters.size() == 1U &&
          localInteraction.parameters.count(localSubscriberParameter) == 1U &&
          copyBytes(localInteraction.parameters.at(localSubscriberParameter)) ==
              std::vector<std::uint8_t>{0x2bU},
      "same-execution control did not deliver the published interaction and parameter");

  auto const isolationDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(500);
  while (std::chrono::steady_clock::now() < isolationDeadline &&
         secondExecutionOne.recorder().discoveryCount() == otherExecutionDiscoveryCount &&
         secondExecutionOne.recorder().reflectionCount() == otherExecutionReflectionCount &&
         secondExecutionOne.recorder().interactions().size() ==
             otherExecutionInteractionCount) {
    secondExecutionOne.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      secondExecutionOne.recorder().discoveryCount() == otherExecutionDiscoveryCount &&
          secondExecutionOne.recorder().reflectionCount() == otherExecutionReflectionCount &&
          secondExecutionOne.recorder().interactions().size() ==
              otherExecutionInteractionCount,
      "receive-order attribute or interaction data crossed into an independent federation execution");

  auto publisherTime = makeTimeContext(firstExecutionOne);
  auto localSubscriberTime = makeTimeContext(firstExecutionTwo);
  auto otherRegulatorTime = makeTimeContext(secondExecutionTwo);
  auto otherSubscriberTime = makeTimeContext(secondExecutionOne);
  require(
      publisherTime.factory->getName() == localSubscriberTime.factory->getName() &&
          publisherTime.factory->getName() == otherRegulatorTime.factory->getName() &&
          publisherTime.factory->getName() == otherSubscriberTime.factory->getName(),
      "cross-execution TSO isolation members selected different logical-time factories");
  enableTimestampedRoles(
      firstExecutionOne,
      firstExecutionTwo,
      publisherTime,
      localSubscriberTime,
      options,
      "same-execution TSO isolation");
  enableTimestampedRoles(
      secondExecutionTwo,
      secondExecutionOne,
      otherRegulatorTime,
      otherSubscriberTime,
      options,
      "other-execution TSO isolation");
  firstExecutionOne.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);

  firstExecutionOne.recorder().clearTimeCallbacks();
  firstExecutionTwo.recorder().clearTimeCallbacks();
  secondExecutionOne.recorder().clearTimeCallbacks();
  secondExecutionTwo.recorder().clearTimeCallbacks();
  firstExecutionOne.recorder().clearTimedInteractions();
  firstExecutionTwo.recorder().clearTimedInteractions();
  secondExecutionOne.recorder().clearTimedInteractions();
  secondExecutionTwo.recorder().clearTimedInteractions();
  firstExecutionOne.recorder().clearCallbackOrder();
  firstExecutionTwo.recorder().clearCallbackOrder();
  secondExecutionOne.recorder().clearCallbackOrder();
  secondExecutionTwo.recorder().clearCallbackOrder();

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const grantTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  std::vector<std::uint8_t> const tsoValueBytes{0x2cU, 0x2dU};
  std::vector<std::uint8_t> const tsoTagBytes{0x54U, 0x53U, 0x4fU};
  rti::ParameterHandleValueMap tsoParameters;
  tsoParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(tsoValueBytes.data(), tsoValueBytes.size()));
  rti::VariableLengthData const tsoTag(tsoTagBytes.data(), tsoTagBytes.size());
  auto const retraction = firstExecutionOne.rtiAmbassador().sendInteraction(
      publisherInteraction,
      tsoParameters,
      tsoTag,
      *messageTime);
  require(retraction.isValid(), "cross-execution TSO isolation send returned no retraction handle");

  // Give any incorrectly routed TSO callback a chance to queue in the other
  // execution before its grant request; the grant then checks the full
  // timestamp interval for an illicit delivery.
  for (int pass = 0; pass != 32; ++pass) {
    secondExecutionOne.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  firstExecutionOne.rtiAmbassador().timeAdvanceRequest(*grantTime);
  firstExecutionTwo.rtiAmbassador().timeAdvanceRequest(*grantTime);
  secondExecutionTwo.rtiAmbassador().timeAdvanceRequest(*grantTime);
  secondExecutionOne.rtiAmbassador().timeAdvanceRequest(*grantTime);
  waitFor(
      firstExecutionOne,
      firstExecutionTwo,
      [&] {
        return firstExecutionOne.recorder().timeAdvanceGrants().size() == 1U &&
            firstExecutionTwo.recorder().timeAdvanceGrants().size() == 1U &&
            firstExecutionTwo.recorder().timedInteractions().size() == 1U;
      },
      options,
      "same-execution TSO interaction and grant");
  waitFor(
      secondExecutionTwo,
      secondExecutionOne,
      [&] {
        return secondExecutionTwo.recorder().timeAdvanceGrants().size() == 1U &&
            secondExecutionOne.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "other-execution TSO isolation grant");

  auto const localTsoInteraction =
      firstExecutionTwo.recorder().timedInteractions().front();
  require(
      localTsoInteraction.interaction == localSubscriberInteraction &&
          localTsoInteraction.parameters.size() == 1U &&
          localTsoInteraction.parameters.count(localSubscriberParameter) == 1U &&
          copyBytes(localTsoInteraction.parameters.at(localSubscriberParameter)) ==
              tsoValueBytes &&
          localTsoInteraction.tag == tsoTagBytes &&
          localTsoInteraction.time == encodeTime(*messageTime) &&
          localTsoInteraction.sentOrder == rti::TIMESTAMP &&
          localTsoInteraction.receivedOrder == rti::TIMESTAMP &&
          localTsoInteraction.retractionPresent &&
          localTsoInteraction.retraction == copyBytes(retraction.encode()),
      "same-execution TSO control did not preserve interaction data and timestamp metadata");
  require(
      firstExecutionOne.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*grantTime) &&
          firstExecutionTwo.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*grantTime) &&
          secondExecutionTwo.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*grantTime) &&
          secondExecutionOne.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*grantTime),
      "cross-execution TSO isolation participants received the wrong grant time");
  require(
      firstExecutionTwo.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "same-execution TSO interaction was not delivered before its grant");
  auto const tsoIsolationDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(500);
  while (std::chrono::steady_clock::now() < tsoIsolationDeadline &&
         secondExecutionOne.recorder().timedInteractions().empty()) {
    secondExecutionOne.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      secondExecutionOne.recorder().timedInteractions().empty() &&
          secondExecutionOne.recorder().callbackOrder() ==
              std::vector<std::string>{"grant"},
      "timestamped interaction crossed into a different federation execution");

  firstExecutionTwo.resign(rti::NO_ACTION);
  firstExecutionTwo.join(
      L"multiple-executions-first-two-rejoined",
      options.federateType,
      secondFederation);
  require(
      firstExecutionTwo.federateHandle().isValid(),
      "an application ambassador could not join another execution after resigning");

  firstExecutionOne.resign(rti::NO_ACTION);
  secondExecutionOne.resign(rti::NO_ACTION);
  secondExecutionTwo.resign(rti::NO_ACTION);
  firstExecutionTwo.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(firstFederation);
  creator.rtiAmbassador().destroyFederationExecution(secondFederation);

  firstExecutionOne.disconnect();
  firstExecutionTwo.disconnect();
  secondExecutionOne.disconnect();
  secondExecutionTwo.disconnect();
  creator.disconnect();
}

void scenarioApplicationMultipleFederatesAcrossExecutionsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioApplicationMultipleFederatesAcrossExecutions(options, model);
}

void scenarioConnectBeforeFederationAndDisconnectAfterResign(
    Options const& options,
    rti::CallbackModel model) {
  Session federationOwner(options, model, "connect-disconnect-owner");
  Session federate(options, model, "connect-disconnect-federate");
  auto const federation = federationName(options, "connect-disconnect-after-resign");

  requireException(
      [&] {
        federate.rtiAmbassador().createFederationExecution(
            federation, options.fom.wstring());
      },
      L"NotConnected",
      "creating a federation execution before Connect");

  federationOwner.connect();
  federate.connect();
  federationOwner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(options.memberFederateName, options.federateType, federation);
  federate.resign(rti::NO_ACTION);
  federate.disconnect();

  requireException(
      [&] {
        federate.rtiAmbassador().createFederationExecution(
            federation, options.fom.wstring());
      },
      L"NotConnected",
      "creating a federation execution after resigning and Disconnect");

  federationOwner.rtiAmbassador().destroyFederationExecution(federation);
  federationOwner.disconnect();
}

void scenarioFederationManagementRequiresConnection(
    Options const& options,
    rti::CallbackModel model) {
  Session disconnected(options, model, "federation-management-requires-connection");
  auto const federation = federationName(options, "requires-connection");
  requireException(
      [&] {
        disconnected.rtiAmbassador().createFederationExecution(
            federation, options.fom);
      },
      L"NotConnected",
      "creating a federation execution before Connect");
  requireException(
      [&] { disconnected.rtiAmbassador().destroyFederationExecution(federation); },
      L"NotConnected",
      "destroying a federation execution before Connect");
  requireException(
      [&] {
        disconnected.rtiAmbassador().joinFederationExecution(
            L"disconnected-member", federation);
      },
      L"NotConnected",
      "joining a federation execution before Connect");
  requireException(
      [&] { disconnected.rtiAmbassador().resignFederationExecution(rti::NO_ACTION); },
      L"NotConnected",
      "resigning from a federation execution before Connect");
}

void scenarioStandardExceptionBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "standard-exception-boundaries");
  verifyResignExceptionPrecedence(options, model);
  verifyDisconnectedFederationManagementServices(options, model);
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const invalidResignAction = static_cast<rti::ResignAction>(99);
  requireException(
      [&] { owner.rtiAmbassador().resignFederationExecution(invalidResignAction); },
      L"InvalidResignAction",
      "resigning with an invalid standard action");
  requireException(
      [&] { owner.rtiAmbassador().setAutomaticResignDirective(invalidResignAction); },
      L"InvalidResignAction",
      "setting an invalid automatic resign directive");

  auto const time = makeTimeContext(owner);
  std::unique_ptr<rti::LogicalTimeInterval> invalidLookahead;
  if (time.factory->getName() == L"HLAinteger64Time") {
    invalidLookahead = std::make_unique<rti::HLAfloat64Interval>(1.0);
  } else {
    invalidLookahead = std::make_unique<rti::HLAinteger64Interval>(1);
  }
  requireException(
      [&] {
        // A positive interval using the other standard representation reaches
        // the RTI service's implementation-compatibility boundary.
        owner.rtiAmbassador().enableTimeRegulation(*invalidLookahead);
      },
      L"InvalidLookahead",
      "enabling time regulation with an interval from another implementation");

  requireException(
      [&] { member.rtiAmbassador().disconnect(); },
      L"FederateIsExecutionMember",
      "disconnecting while still a federation execution member");

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const memberClass = member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && memberClass.isValid() &&
          memberAttribute.isValid(),
      "standard exception boundary handle lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      memberAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "standard exception boundary registration returned an invalid handle");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "standard exception boundary object discovery");

  rti::VariableLengthData const emptyTag;
  requireException(
      [&] {
        owner.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object,
            ownerAttributes);
      },
      L"AttributeAlreadyOwned",
      "canceling an acquisition for an attribute already owned by the federate");
  requireException(
      [&] {
        member.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object,
            memberAttributes);
      },
      L"AttributeAcquisitionWasNotRequested",
      "canceling an ownership acquisition that was never requested");
  requireException(
      [&] { owner.rtiAmbassador().confirmDivestiture(object, ownerAttributes, emptyTag); },
      L"AttributeDivestitureWasNotRequested",
      "confirming divestiture without a divestiture request");
  requireException(
      [&] { member.rtiAmbassador().deleteObjectInstance(object, emptyTag); },
      L"DeletePrivilegeNotHeld",
      "deleting an object without its delete privilege");

  auto const name = federation + L"-named-object";
  owner.rtiAmbassador().reserveObjectInstanceName(name);
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(name); },
      options,
      "standard exception boundary object-name reservation");
  auto const namedObject = owner.rtiAmbassador().registerObjectInstance(ownerClass, name);
  require(
      namedObject.isValid(),
      "standard exception boundary named registration returned an invalid handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().registerObjectInstance(ownerClass, name));
      },
      L"ObjectInstanceNameInUse",
      "registering a duplicate object instance name");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioStandardExceptionBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioStandardExceptionBoundaries(options, model);
}

void scenarioPostResignationServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "post-resignation-owner");
  Session member(options, model, "post-resignation-member");
  auto const federation = federationName(options, "post-resignation-service-boundaries");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const memberClass = member.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  auto const memberInteraction = member.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const memberParameter = member.rtiAmbassador().getParameterHandle(
      memberInteraction,
      options.parameterName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && memberClass.isValid() &&
          memberAttribute.isValid() && memberInteraction.isValid() &&
          memberParameter.isValid(),
      "post-resignation boundary setup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      memberAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "post-resignation boundary setup returned an invalid object");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "post-resignation boundary object discovery");
  auto const objectName = member.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty() &&
          member.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          member.rtiAmbassador().getKnownObjectClassHandle(object) == memberClass,
      "post-resignation boundary object identity did not round-trip");

  auto const reliable = member.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(reliable.isValid(), "post-resignation boundary reliable handle is invalid");
  auto time = makeTimeContext(member);
  auto logicalTime = time.factory->makeInitial();
  auto galt = time.factory->makeInitial();
  auto lits = time.factory->makeInitial();
  auto lookahead = time.factory->makeZero();

  std::vector<std::uint8_t> const valueBytes{0x50U, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      memberAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  std::vector<std::uint8_t> const parameterBytes{0x50U, 0x41U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      memberParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const emptyTag;
  rti::InteractionClassHandleSet const emptyInteractionClasses;
  rti::FederateHandleSet const synchronizationSet{owner.federateHandle()};
  std::set<std::wstring> const reservedNames{
      federation + L"-post-resignation-a",
      federation + L"-post-resignation-b"};

  member.resign(rti::NO_ACTION);
  auto requireNotMember = [&](std::function<void()> operation,
                              std::string const& description) {
    requireException(
        std::move(operation),
        L"FederateNotExecutionMember",
        description);
  };

  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getFederateHandle(
            options.ownerFederateName));
      },
      "federate name lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getFederateName(
            owner.federateHandle()));
      },
      "federate handle lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getObjectClassHandle(
            options.objectClassName));
      },
      "object-class handle lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getObjectClassName(memberClass));
      },
      "object-class name lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getKnownObjectClassHandle(object));
      },
      "known object-class lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      "object-instance handle lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getObjectInstanceName(object));
      },
      "object-instance name lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getAttributeHandle(
            memberClass,
            options.attributeName));
      },
      "attribute handle lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getAttributeName(
            memberClass,
            memberAttribute));
      },
      "attribute name lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getInteractionClassHandle(
            options.interactionClassName));
      },
      "interaction-class handle lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getInteractionClassName(
            memberInteraction));
      },
      "interaction-class name lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getParameterHandle(
            memberInteraction,
            options.parameterName));
      },
      "parameter handle lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getParameterName(
            memberInteraction,
            memberParameter));
      },
      "parameter name lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getAvailableDimensionsForObjectClass(
            memberClass));
      },
      "object-class dimension lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getAvailableDimensionsForInteractionClass(
            memberInteraction));
      },
      "interaction-class dimension lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getDimensionHandle(
            options.fomDimensionName));
      },
      "dimension handle lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getOrderType(L"Receive"));
      },
      "order-type lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getOrderName(rti::RECEIVE));
      },
      "order-name lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getTransportationTypeHandle(
            L"HLAreliable"));
      },
      "transportation-type lookup after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().getTransportationTypeName(reliable));
      },
      "transportation-name lookup after resignation");

  requireNotMember(
      [&] {
        member.rtiAmbassador().publishObjectClassAttributes(
            memberClass,
            memberAttributes);
      },
      "object-attribute publication after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().unpublishObjectClassAttributes(
            memberClass,
            memberAttributes);
      },
      "object-attribute unpublication after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().unpublishObjectClass(memberClass); },
      "object-class unpublication after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            memberClass,
            memberAttributes,
            true,
            L"");
      },
      "object-attribute subscription after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().unsubscribeObjectClassAttributes(
            memberClass,
            memberAttributes);
      },
      "object-attribute unsubscription after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().unsubscribeObjectClass(memberClass); },
      "object-class unsubscription after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().publishInteractionClass(memberInteraction); },
      "interaction publication after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().unpublishInteractionClass(memberInteraction); },
      "interaction unpublication after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().subscribeInteractionClass(memberInteraction); },
      "interaction subscription after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().unsubscribeInteractionClass(memberInteraction); },
      "interaction unsubscription after resignation");

  auto const singleName = federation + L"-post-resignation-single";
  requireNotMember(
      [&] { member.rtiAmbassador().reserveObjectInstanceName(singleName); },
      "object-name reservation after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().releaseObjectInstanceName(singleName); },
      "object-name release after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().reserveMultipleObjectInstanceNames(reservedNames);
      },
      "multiple object-name reservation after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().releaseMultipleObjectInstanceNames(reservedNames);
      },
      "multiple object-name release after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().registerObjectInstance(memberClass));
      },
      "object registration after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().registerObjectInstance(
            memberClass,
            singleName));
      },
      "named object registration after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().updateAttributeValues(object, values, emptyTag);
      },
      "attribute update after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().requestAttributeValueUpdate(
            object,
            memberAttributes,
            emptyTag);
      },
      "object attribute-value request after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().requestAttributeValueUpdate(
            memberClass,
            memberAttributes,
            emptyTag);
      },
      "object-class attribute-value request after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().deleteObjectInstance(object, emptyTag); },
      "object deletion after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().localDeleteObjectInstance(object); },
      "local object deletion after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().sendInteraction(
            memberInteraction,
            parameters,
            emptyTag);
      },
      "interaction send after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().sendDirectedInteraction(
            memberInteraction,
            object,
            parameters,
            emptyTag);
      },
      "directed interaction send after resignation");

  requireNotMember(
      [&] {
        member.rtiAmbassador().queryAttributeOwnership(object, memberAttributes);
      },
      "attribute ownership query after resignation");
  requireNotMember(
      [&] {
        static_cast<void>(member.rtiAmbassador().isAttributeOwnedByFederate(
            object,
            memberAttribute));
      },
      "attribute ownership status after resignation");

  requireNotMember(
      [&] { member.rtiAmbassador().enableAsynchronousDelivery(); },
      "asynchronous delivery enable after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().disableAsynchronousDelivery(); },
      "asynchronous delivery disable after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().enableTimeConstrained(); },
      "time-constrained enable after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().disableTimeConstrained(); },
      "time-constrained disable after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().enableTimeRegulation(*time.zero); },
      "time-regulation enable after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().disableTimeRegulation(); },
      "time-regulation disable after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().modifyLookahead(*time.epsilon); },
      "lookahead modification after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().queryLookahead(*lookahead); },
      "lookahead query after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().queryLogicalTime(*logicalTime); },
      "logical-time query after resignation");
  requireNotMember(
      [&] { static_cast<void>(member.rtiAmbassador().queryGALT(*galt)); },
      "GALT query after resignation");
  requireNotMember(
      [&] { static_cast<void>(member.rtiAmbassador().queryLITS(*lits)); },
      "LITS query after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().timeAdvanceRequest(*time.initial); },
      "time advance request after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().timeAdvanceRequestAvailable(*time.initial);
      },
      "available time advance request after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().nextMessageRequest(*time.initial); },
      "next-message request after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().nextMessageRequestAvailable(*time.initial);
      },
      "available next-message request after resignation");
  requireNotMember(
      [&] { member.rtiAmbassador().flushQueueRequest(*time.initial); },
      "flush-queue request after resignation");

  requireNotMember(
      [&] {
        member.rtiAmbassador().registerFederationSynchronizationPoint(
            federation + L"-post-resignation-global",
            emptyTag);
      },
      "global synchronization-point registration after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().registerFederationSynchronizationPoint(
            federation + L"-post-resignation-explicit",
            emptyTag,
            synchronizationSet);
      },
      "explicit synchronization-point registration after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().synchronizationPointAchieved(
            federation + L"-post-resignation-achieved");
      },
      "synchronization-point achievement after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().requestFederationSave(
            federation + L"-post-resignation-save");
      },
      "federation save request after resignation");
  requireNotMember(
      [&] {
        member.rtiAmbassador().requestFederationSave(
            federation + L"-post-resignation-timestamped-save",
            *time.initial);
      },
      "timestamped federation save request after resignation");

  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioPostResignationServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPostResignationServiceBoundaries(options, model);
}

void scenarioUnnamedJoinOverload(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "unnamed-join-owner");
  Session member(options, model, "unnamed-join-member");
  Session secondUnnamed(options, model, "unnamed-join-second-unnamed");
  Session observer(options, model, "unnamed-join-observer");
  auto const federation = federationName(options, "unnamed-join-overload");
  owner.connect();
  member.connect();
  secondUnnamed.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);

  auto const unnamedHandle = owner.rtiAmbassador().joinFederationExecution(
      options.federateType,
      federation);
  require(
      unnamedHandle.isValid(),
      "unnamed join overload returned an invalid federate handle");
  auto const generatedName = owner.rtiAmbassador().getFederateName(unnamedHandle);
  require(
      !generatedName.empty() && generatedName != options.memberFederateName &&
          owner.rtiAmbassador().getFederateHandle(generatedName) == unnamedHandle,
      "unnamed join overload did not establish a reversible generated federate name");

  auto const secondUnnamedHandle = secondUnnamed.rtiAmbassador().joinFederationExecution(
      options.federateType,
      federation);
  require(
      secondUnnamedHandle.isValid(),
      "second unnamed join overload returned an invalid federate handle");
  auto const secondGeneratedName =
      secondUnnamed.rtiAmbassador().getFederateName(secondUnnamedHandle);
  require(
      !secondGeneratedName.empty() && secondGeneratedName != generatedName &&
          secondUnnamed.rtiAmbassador().getFederateHandle(secondGeneratedName) ==
              secondUnnamedHandle,
      "unnamed joins did not receive distinct, reversible generated federate names");

  auto const memberHandle = member.rtiAmbassador().joinFederationExecution(
      options.memberFederateName,
      options.federateType,
      federation);
  require(memberHandle.isValid(), "named join control returned an invalid federate handle");
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().joinFederationExecution(
            options.memberFederateName,
            options.federateType,
            federation));
      },
      L"FederateNameAlreadyInUse",
      "joining with a federate name already active in the execution");

  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(
      observer,
      [&] { return observer.recorder().listsMembers(federation, {}, 3U); },
      options,
      "unnamed join federation member report");
  require(
      observer.recorder().listsMember(
          federation,
          options.memberFederateName,
          options.federateType) &&
          observer.recorder().listsMember(
              federation, generatedName, options.federateType) &&
          observer.recorder().listsMember(
              federation, secondGeneratedName, options.federateType),
      "unnamed join member report omitted a generated or named federate");

  member.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
  auto const rejoinedHandle = member.rtiAmbassador().joinFederationExecution(
      options.memberFederateName,
      options.federateType,
      federation);
  require(
      rejoinedHandle.isValid() && rejoinedHandle != memberHandle,
      "federate designator was reused after the same federate name rejoined the execution");

  // With no owned objects or pending acquisitions, verify argument acceptance
  // for each official 2025 value without claiming its stateful side effects.
  rti::ResignAction const actions[]{
      rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES,
      rti::DELETE_OBJECTS,
      rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
      rti::DELETE_OBJECTS_THEN_DIVEST,
      rti::CANCEL_THEN_DELETE_THEN_DIVEST,
      rti::NO_ACTION};
  auto previousActionHandle = rejoinedHandle;
  for (auto const action : actions) {
    member.rtiAmbassador().resignFederationExecution(action);
    auto const actionRejoinHandle = member.rtiAmbassador().joinFederationExecution(
        options.memberFederateName,
        options.federateType,
        federation);
    require(
        actionRejoinHandle.isValid() &&
            actionRejoinHandle != previousActionHandle,
        "a standard ResignAction value was rejected or reused a prior federate designator");
    previousActionHandle = actionRejoinHandle;
  }

  member.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
  secondUnnamed.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
  owner.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  secondUnnamed.disconnect();
}

void scenarioUnnamedJoinOverloadContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnnamedJoinOverload(options, model);
}

void scenarioExplicitMimCreation(Options const& options, rti::CallbackModel model) {
  require(!options.mimFom.empty(),
          "Explicit-MIM creation requires an adapter-supplied MIM module");

  Session owner(options, model, "mim-owner");
  Session member(options, model, "mim-member");
  auto const federation = federationName(options, "explicit-mim");
  owner.connect();
  member.connect();

  requireException(
      [&] {
        owner.rtiAmbassador().createFederationExecutionWithMIM(
            federation,
            std::vector<std::wstring>{options.fom.wstring()},
            L"HLAstandardMIM",
            options.logicalTimeImplementationName);
      },
      L"DesignatorIsHLAstandardMIM",
      "reserved standard-MIM designator");

  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const ownerFederationClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const memberFederationClass = member.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const ownerFederateClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const memberFederateClass = member.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  require(
      ownerFederationClass.isValid() && memberFederationClass.isValid() &&
          ownerFederateClass.isValid() && memberFederateClass.isValid(),
      "Explicit-MIM creation did not compose the standard MOM object classes");
  require(
      ownerFederationClass == memberFederationClass &&
          ownerFederateClass == memberFederateClass,
      "Explicit-MIM creation returned inconsistent MOM class handles");
  require(
      owner.rtiAmbassador().getObjectClassName(ownerFederationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          owner.rtiAmbassador().getObjectClassName(ownerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate",
      "Explicit-MIM MOM class names did not round-trip");

  auto const ownerFederationNameAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederationClass, L"HLAfederationName");
  auto const memberFederationNameAttribute = member.rtiAmbassador().getAttributeHandle(
      memberFederationClass, L"HLAfederationName");
  auto const ownerFederateNameAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass, L"HLAfederateName");
  auto const memberFederateNameAttribute = member.rtiAmbassador().getAttributeHandle(
      memberFederateClass, L"HLAfederateName");
  require(
      ownerFederationNameAttribute.isValid() && memberFederationNameAttribute.isValid() &&
          ownerFederateNameAttribute.isValid() && memberFederateNameAttribute.isValid(),
      "Explicit-MIM creation did not compose standard MOM attributes");
  require(
      ownerFederationNameAttribute == memberFederationNameAttribute &&
          ownerFederateNameAttribute == memberFederateNameAttribute,
      "Explicit-MIM creation returned inconsistent MOM attribute handles");
  require(
      owner.rtiAmbassador().getAttributeName(
          ownerFederationClass, ownerFederationNameAttribute) == L"HLAfederationName" &&
          owner.rtiAmbassador().getAttributeName(
              ownerFederateClass, ownerFederateNameAttribute) == L"HLAfederateName",
      "Explicit-MIM MOM attribute names did not round-trip");

  rti::ObjectClassHandle ordinaryClass;
  rti::AttributeHandle ordinaryAttribute;
  rti::InteractionClassHandle ordinaryInteraction;
  rti::ParameterHandle ordinaryParameter;
  handles(
      owner,
      options,
      ordinaryClass,
      ordinaryAttribute,
      ordinaryInteraction,
      ordinaryParameter);
  require(
      ordinaryClass.isValid() && ordinaryAttribute.isValid() &&
          ordinaryInteraction.isValid() && ordinaryParameter.isValid(),
      "Explicit-MIM composition displaced the caller FOM declarations");

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);

  auto const defaultMimFederation = federationName(options, "default-mim");
  owner.rtiAmbassador().createFederationExecution(
      defaultMimFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, defaultMimFederation);
  auto const defaultMimFederationClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const defaultMimFederationNameAttribute = owner.rtiAmbassador().getAttributeHandle(
      defaultMimFederationClass,
      L"HLAfederationName");
  require(
      defaultMimFederationClass.isValid() &&
          defaultMimFederationNameAttribute.isValid() &&
          owner.rtiAmbassador().getObjectClassName(defaultMimFederationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          owner.rtiAmbassador().getAttributeName(
              defaultMimFederationClass,
              defaultMimFederationNameAttribute) == L"HLAfederationName",
      "Create without an explicit MIM did not automatically load the standard MIM");
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(defaultMimFederation);
  member.disconnect();
  owner.disconnect();
}

void scenarioExplicitMimCreationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioExplicitMimCreation(options, model);
}

void scenarioFederateTypeRestoreCountMismatch(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "restore-type-owner");
  Session peer(options, model, "restore-type-peer");
  auto const federation = federationName(
      options,
      "restore-federate-type-count-mismatch");
  connectAndJoin(owner, peer, options, federation, options.fom);

  std::wstring const saveLabel = L"restore-federate-type-count-mismatch";
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return !owner.federateSaveInitiations().empty() &&
            !peer.federateSaveInitiations().empty();
      },
      options,
      "same-type federate save initiation");
  require(
      owner.federateSaveInitiations().front() == saveLabel &&
          peer.federateSaveInitiations().front() == saveLabel,
      "same-type save did not initiate both joined federates with the requested label");
  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationSavedCount() >= 1U &&
            peer.federationSavedCount() >= 1U;
      },
      options,
      "same-type federation save completion");

  // The saved distribution is {options.federateType: 2}. Replace one member
  // with a different type while retaining the same total of two joined members.
  owner.resign(rti::NO_ACTION);
  std::wstring const differentType = options.federateType + L"-restore-mismatch";
  owner.join(options.ownerFederateName, differentType, federation);
  require(
      owner.federateHandle().isValid() && peer.federateHandle().isValid() &&
          owner.federateHandle() != peer.federateHandle(),
      "restore type-count mismatch fixture did not retain two distinct joined federates");

  peer.rtiAmbassador().requestFederationRestore(saveLabel);
  waitFor(
      peer,
      [&] { return !peer.federationRestoreRequestsFailed().empty(); },
      options,
      "restore rejection for a changed per-type membership count");
  auto const failures = peer.federationRestoreRequestsFailed();
  require(
      failures.size() == 1U && failures.front() == saveLabel,
      "restore failure did not identify the save whose federate-type counts changed");
  require(
      owner.federationRestoreBegunCount() == 0U &&
          peer.federationRestoreBegunCount() == 0U &&
          owner.federateRestoreInitiations().empty() &&
          peer.federateRestoreInitiations().empty(),
      "restore began despite a changed federate-type membership count");

  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

void scenarioFederateTypeRestoreIdentityRebinding(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(
      options,
      "restore-federate-type-identity-rebinding");
  connectAndJoin(owner, peer, options, federation, options.fom);

  std::wstring const saveLabel = L"restore-federate-type-identity-rebinding";
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return !owner.federateSaveInitiations().empty() &&
            !peer.federateSaveInitiations().empty();
      },
      options,
      "same-type identity-rebinding save initiation");
  require(
      owner.federateSaveInitiations().front() == saveLabel &&
          peer.federateSaveInitiations().front() == saveLabel,
      "same-type save did not initiate both original federates");
  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationSavedCount() >= 1U &&
            peer.federationSavedCount() >= 1U;
      },
      options,
      "same-type identity-rebinding save completion");

  owner.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  std::wstring const replacementOwnerName =
      options.ownerFederateName + L"-same-type-replacement";
  std::wstring const replacementPeerName =
      options.memberFederateName + L"-same-type-replacement";
  owner.join(replacementOwnerName, options.federateType, federation);
  peer.join(replacementPeerName, options.federateType, federation);
  auto const replacementOwnerHandle = owner.federateHandle();
  auto const replacementPeerHandle = peer.federateHandle();
  require(
      replacementOwnerHandle.isValid() && replacementPeerHandle.isValid() &&
          replacementOwnerHandle != replacementPeerHandle,
      "same-type replacement joins did not produce two distinct members");

  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return !owner.federationRestoreRequestsSucceeded().empty() &&
            owner.federationRestoreBegunCount() >= 1U &&
            peer.federationRestoreBegunCount() >= 1U &&
            !owner.federateRestoreInitiations().empty() &&
            !peer.federateRestoreInitiations().empty();
      },
      options,
      "same-type identity-rebinding restore initiation");

  auto const ownerRestore = owner.federateRestoreInitiations().front();
  auto const peerRestore = peer.federateRestoreInitiations().front();
  require(
      owner.federationRestoreRequestsSucceeded().front() == saveLabel &&
          ownerRestore.label == saveLabel && peerRestore.label == saveLabel,
      "same-type restore did not confirm the requested saved federation");
  require(
      ownerRestore.federateName == options.ownerFederateName &&
          peerRestore.federateName == options.memberFederateName,
      "same-type replacements were not rebound to the saved federate identities");
  require(
      ownerRestore.postRestoreFederateHandle == replacementOwnerHandle &&
          peerRestore.postRestoreFederateHandle == replacementPeerHandle,
      "same-type restore did not associate saved identities with current handles");
  require(
      owner.federationRestoreRequestsFailed().empty() &&
          peer.federationRestoreRequestsFailed().empty(),
      "same-type identity-rebinding restore unexpectedly failed");

  owner.rtiAmbassador().federateRestoreComplete();
  peer.rtiAmbassador().federateRestoreComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoredCount() >= 1U &&
            peer.federationRestoredCount() >= 1U;
      },
      options,
      "same-type identity-rebinding restore completion");
  require(
      owner.federationNotRestoredReasons().empty() &&
          peer.federationNotRestoredReasons().empty(),
      "same-type identity-rebinding restore reported a failure");

  owner.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
