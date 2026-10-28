#include "tck_federation_lifecycle_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederationListServices(Options const& options, rti::CallbackModel model) {
  Session observer(options, model, "federation-list-observer");
  Session owner(options, model, "owner");
  Session member(options, model, "federation-list-member");
  Session stale(options, model, "federation-list-stale");

  auto const firstFederation = federationName(options, "federation-list-primary");
  auto const secondFederation = options.federationOverride.empty()
      ? federationName(options, "federation-list-secondary")
      : firstFederation + L"-secondary";
  auto const missingFederation = options.federationOverride.empty()
      ? federationName(options, "federation-list-missing")
      : firstFederation + L"-missing";

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
  owner.join(options.ownerFederateName, options.federateType, firstFederation);
  member.join(options.memberFederateName, options.federateType, firstFederation);

  auto requireCallbackBoundary = [&](auto const& predicate, std::string const& description) {
    if (model == rti::HLA_EVOKED) {
      require(!predicate(), description + " was delivered before the Evoke boundary");
    } else {
      require(predicate(), description + " was not delivered by the immediate callback boundary");
    }
  };

  observer.rtiAmbassador().listFederationExecutions();
  requireCallbackBoundary(
      [&] {
        return observer.recorder().listsFederation(
                   firstFederation,
                   options.logicalTimeImplementationName) &&
            observer.recorder().listsFederation(
                secondFederation,
                options.logicalTimeImplementationName);
      },
      "federation execution report");
  waitFor(
      observer,
      [&] {
        return observer.recorder().listsFederation(
                   firstFederation,
                   options.logicalTimeImplementationName) &&
            observer.recorder().listsFederation(
                secondFederation,
                options.logicalTimeImplementationName);
      },
      options,
      "the complete federation execution report");

  observer.rtiAmbassador().listFederationExecutionMembers(firstFederation);
  requireCallbackBoundary(
      [&] {
        return observer.recorder().listsMembers(
                   firstFederation,
                   {options.ownerFederateName, options.memberFederateName},
                   2U) &&
            observer.recorder().listsMember(
                firstFederation,
                options.memberFederateName,
                options.federateType);
      },
      "federation member report");
  waitFor(
      observer,
      [&] {
        return observer.recorder().listsMembers(
                   firstFederation,
                   {options.ownerFederateName, options.memberFederateName},
                   2U) &&
            observer.recorder().listsMember(
                firstFederation,
                options.memberFederateName,
                options.federateType);
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
        return observer.recorder().listsMembers(
                   firstFederation,
                   {options.ownerFederateName},
                   1U) &&
            observer.recorder().listsMember(
                firstFederation,
                options.ownerFederateName,
                options.federateType);
      },
      "post-resignation federation member report");
  waitFor(
      observer,
      [&] {
        return observer.recorder().listsMembers(
                   firstFederation,
                   {options.ownerFederateName},
                   1U) &&
            observer.recorder().listsMember(
                firstFederation,
                options.ownerFederateName,
                options.federateType);
      },
      options,
      "the post-resignation federation member report");

  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(firstFederation);
  owner.rtiAmbassador().destroyFederationExecution(secondFederation);
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
  auto const firstFederation =
      federationName(options, "application-multiple-federates-first-execution");
  auto const secondFederation =
      federationName(options, "application-multiple-federates-second-execution");

  static_cast<void>(creator.connect());
  static_cast<void>(firstExecutionOne.connect());
  static_cast<void>(firstExecutionTwo.connect());
  static_cast<void>(secondExecutionOne.connect());
  static_cast<void>(secondExecutionTwo.connect());
  creator.rtiAmbassador().createFederationExecution(
      firstFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  creator.rtiAmbassador().createFederationExecution(
      secondFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);

  auto const firstOneName = std::wstring(L"multiple-executions-first-one");
  auto const firstTwoName = std::wstring(L"multiple-executions-first-two");
  auto const secondOneName = std::wstring(L"multiple-executions-second-one");
  auto const secondTwoName = std::wstring(L"multiple-executions-second-two");
  firstExecutionOne.join(
      firstOneName, options.federateType, firstFederation);
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
      "one application could not join multiple federates across two executions");

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
  member.disconnect();
  owner.disconnect();
}

void scenarioExplicitMimCreationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioExplicitMimCreation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
