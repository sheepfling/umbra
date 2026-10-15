#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioConnection(Options const& options, rti::CallbackModel model) {
  auto requireConfigurationResult = [](rti::ConfigurationResult const& result,
                                       std::string const& description) {
    require(
        result.additionalSettingsResult == rti::SETTINGS_IGNORED ||
            result.additionalSettingsResult == rti::SETTINGS_FAILED_TO_PARSE ||
            result.additionalSettingsResult == rti::SETTINGS_APPLIED,
        description + " returned an invalid additional-settings result code");
  };
  auto exerciseConnectOverload =
      [&](std::string const& description, auto const& connect) {
        rti::RTIambassadorFactory factory;
        auto ambassador = factory.createRTIambassador();
        require(
            static_cast<bool>(ambassador),
            description + " did not create a standard RTIambassador");
        Recorder recorder;
        auto const result = connect(*ambassador, recorder);
        requireConfigurationResult(result, description);
        ambassador->disconnect();
      };
  auto emptyConfiguration = rti::RtiConfiguration::createConfiguration();
  auto adapterConfiguration = rti::RtiConfiguration::createConfiguration();
  if (!options.configurationName.empty()) {
    adapterConfiguration.withConfigurationName(
        toWide(options.configurationName));
  }
  if (!options.rtiAddress.empty()) {
    adapterConfiguration.withRtiAddress(toWide(options.rtiAddress));
  }
  if (!options.additionalSettings.empty()) {
    adapterConfiguration.withAdditionalSettings(
        toWide(options.additionalSettings));
  }
  rti::HLAnoCredentials noCredentials;

  exerciseConnectOverload(
      "base connect overload",
      [model](rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(federate, model);
      });
  exerciseConnectOverload(
      "configuration connect overload",
      [model, &emptyConfiguration](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(federate, model, emptyConfiguration);
      });
  exerciseConnectOverload(
      "credentials connect overload",
      [model, &noCredentials](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(federate, model, noCredentials);
      });
  exerciseConnectOverload(
      "configuration and credentials connect overload",
      [model, &emptyConfiguration, &noCredentials](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(
            federate, model, emptyConfiguration, noCredentials);
      });
  exerciseConnectOverload(
      "adapter configuration and HLAnoCredentials connect overload",
      [model, &adapterConfiguration, &noCredentials](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(
            federate, model, adapterConfiguration, noCredentials);
      });

  {
    rti::RTIambassadorFactory factory;
    auto ambassador = factory.createRTIambassador();
    require(
        static_cast<bool>(ambassador),
        "unsupported callback-model check did not create a standard RTIambassador");
    Recorder recorder;
    requireException(
        [&] {
          static_cast<void>(ambassador->connect(
              recorder, static_cast<rti::CallbackModel>(-1)));
        },
        L"UnsupportedCallbackModel",
        "unsupported callback model");
    requireConfigurationResult(
        ambassador->connect(recorder, model),
        "connect after unsupported callback model");
    ambassador->disconnect();
  }

  {
    rti::RTIambassadorFactory factory;
    auto ambassador = factory.createRTIambassador();
    require(
        static_cast<bool>(ambassador),
        "pre-connect disconnect check did not create a standard RTIambassador");
    requireException(
        [&] { ambassador->disconnect(); },
        L"NotConnected",
        "disconnect before connect");
  }

  Session session(options, model, "connection");
  requireException(
      [&] { session.rtiAmbassador().listFederationExecutions(); },
      L"NotConnected",
      "listing federation executions before connect");
  requireException(
      [&] {
        session.rtiAmbassador().listFederationExecutionMembers(
            L"tck-not-connected-federation");
      },
      L"NotConnected",
      "listing federation members before connect");
  requireException(
      [&] {
        static_cast<void>(session.rtiAmbassador().getObjectClassHandle(
            options.objectClassName));
      },
      L"NotConnected",
      "looking up an object class before connect");
  requireException(
      [&] {
        session.rtiAmbassador().reserveObjectInstanceName(
            L"tck-not-connected-object");
      },
      L"NotConnected",
      "reserving an object name before connect");
  auto const baseResult = session.connect();
  requireConfigurationResult(baseResult, "base connect");
  requireException(
      [&] {
        static_cast<void>(session.rtiAmbassador().connect(session.recorder(), model));
      },
      L"AlreadyConnected",
      "duplicate base connect");
  session.rtiAmbassador().disableCallbacks();
  session.rtiAmbassador().enableCallbacks();
  require(
      !session.rtiAmbassador().evokeCallback(0.0),
      "an empty callback queue unexpectedly evoked a callback");
  require(
      !session.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0),
      "an empty callback queue unexpectedly evoked multiple callbacks");
  session.disconnect();

  Session configured(options, model, "connection");
  auto const configuredResult = configured.connectWithConfiguration(emptyConfiguration);
  requireConfigurationResult(configuredResult, "empty-configuration connect");
  configured.disconnect();

  Session reconnect(options, model, "connection-reconnect");
  requireConfigurationResult(reconnect.connect(), "first reconnect-cycle connect");
  reconnect.disconnect();
  requireConfigurationResult(reconnect.connect(), "second reconnect-cycle connect");
  reconnect.disconnect();
}

void scenarioConnectionCallbackContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnection(options, model);
}

void waitForConnectionLossAndSignal(
    Session& lost,
    Options const& options,
    std::string const& description) {
  if (!options.connectionLossReadyMarker.empty()) {
    std::ofstream readyMarker(
        options.connectionLossReadyMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(readyMarker),
        "Could not write the connection-loss readiness marker");
    readyMarker << "ready\n";
    require(
        static_cast<bool>(readyMarker),
        "Could not flush the connection-loss readiness marker");
  }
  auto const lossDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < lossDeadline && !lost.recorder().hasConnectionLost()) {
    // Connection loss is an external transport event.  Even with
    // HLA_IMMEDIATE, a provider may need a standard callback-servicing call to
    // observe an EOF before it can deliver the normative immediate callback.
    // This remains a bounded standard-API progress probe; ordinary
    // application callbacks are still recorded under the model selected at
    // connect time.
    try {
      static_cast<void>(lost.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
    } catch (...) {
      // A provider may surface the socket fault on the Evoke operation before
      // placing the normative callback on the next callback turn.
    }
    if (!lost.recorder().hasConnectionLost()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  require(
      lost.recorder().hasConnectionLost(),
      "Timed out waiting for " + description + " connectionLost callback");
  require(
      !lost.recorder().connectionLostDescription().empty(),
      description + " connectionLost did not provide a fault description");
  if (!options.connectionLossMarker.empty()) {
    std::ofstream marker(
        options.connectionLossMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker),
        "Could not write the connection-loss completion marker");
    marker << "callback-ok\n";
    require(
        static_cast<bool>(marker),
        "Could not flush the connection-loss completion marker");
  }
}

void scenarioConnectionLoss(Options const& options, rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Connection-loss cleanup requires an adapter-managed fault fixture");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "connection-loss");
  connectAndJoin(owner, member, options, federation, options.fom);

  waitForConnectionLossAndSignal(member, options, "ordinary connection-loss cleanup");

  rti::InteractionClassHandle interactionClass =
      owner.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  require(interactionClass.isValid(), "The surviving member could not resolve an interaction class");
  owner.rtiAmbassador().sendInteraction(
      interactionClass, rti::ParameterHandleValueMap{}, rti::VariableLengthData{});
  try {
    member.disconnect();
  } catch (...) {
    // Some providers transition the lost member to a disconnected state
    // before delivering connectionLost. Cleanup is best effort after loss.
  }
  owner.resign(rti::NO_ACTION);
  owner.disconnect();
}

void scenarioConnectionLossContract(Options const& options, rti::CallbackModel model) {
  scenarioConnectionLoss(options, model);
}

void scenarioConnectionLossAutomaticUnconditionalDivestiture(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic connection-loss divestiture requires an adapter-managed fault fixture");
  Session survivor(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(
      options,
      "connection-loss-automatic-unconditional-divestiture");
  connectAndJoin(survivor, lost, options, federation, options.fom);

  auto const lostClass =
      lost.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const survivorClass =
      survivor.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const lostAttribute =
      lost.rtiAmbassador().getAttributeHandle(lostClass, options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  auto const lostPrivilege = lost.rtiAmbassador().getAttributeHandle(
      lostClass,
      L"HLAprivilegeToDeleteObject");
  auto const survivorPrivilege = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      L"HLAprivilegeToDeleteObject");
  require(
      lostClass.isValid() && survivorClass.isValid(),
      "automatic connection-loss divestiture object-class lookup returned an invalid handle");
  require(
      lostAttribute.isValid() && survivorAttribute.isValid() &&
          lostPrivilege.isValid() && survivorPrivilege.isValid(),
      "automatic connection-loss divestiture attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  rti::AttributeHandleSet const expectedAssumption{
      survivorAttribute,
      survivorPrivilege};
  lost.rtiAmbassador().publishObjectClassAttributes(lostClass, lostAttributes);
  survivor.rtiAmbassador().publishObjectClassAttributes(
      survivorClass,
      survivorAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");

  auto const object = lost.rtiAmbassador().registerObjectInstance(lostClass);
  require(
      object.isValid(),
      "automatic connection-loss divestiture registration returned an invalid handle");
  auto const objectName = lost.rtiAmbassador().getObjectInstanceName(object);
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "automatic connection-loss divestiture object discovery");

  lost.rtiAmbassador().setAutomaticResignDirective(
      rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() ==
          rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES,
      "automatic connection-loss divestiture directive did not round-trip");
  survivor.recorder().clearOwnershipRecords();

  waitForConnectionLossAndSignal(
      lost,
      options,
      "automatic unconditional-divestiture");
  waitFor(
      survivor,
      [&] { return survivor.recorder().ownershipAssumption().has_value(); },
      options,
      "automatic unconditional-divestiture ownership assumption");
  auto const assumption = survivor.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes == expectedAssumption && assumption->tag.empty(),
      "automatic unconditional-divestiture offered the wrong ownership set or tag");
  require(
      survivor.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "automatic unconditional-divestiture removed the retained object");
  require(
      !survivor.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          survivorAttribute) &&
          !survivor.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              survivorPrivilege),
      "automatic unconditional-divestiture left the survivor owning released attributes");

  survivor.resign(rti::NO_ACTION);
  survivor.disconnect();
}

void scenarioConnectionLossAutomaticUnconditionalDivestitureContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionLossAutomaticUnconditionalDivestiture(options, model);
}

void scenarioConnectionLossAutomaticCancelPendingAcquisition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic connection-loss acquisition cancellation requires an adapter-managed fault fixture");
  Session owner(options, model, "owner");
  Session lost(options, model, "member");
  Session survivor(options, model, "survivor");
  auto const federation = federationName(
      options,
      "connection-loss-automatic-cancel-pending-acquisition");
  connectAndJoin(owner, lost, options, federation, options.fom);
  survivor.connect();
  survivor.join(
      options.memberFederateName + L"-survivor",
      options.federateType,
      federation);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const lostClass =
      lost.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const survivorClass =
      survivor.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute =
      owner.rtiAmbassador().getAttributeHandle(ownerClass, options.attributeName);
  auto const lostAttribute =
      lost.rtiAmbassador().getAttributeHandle(lostClass, options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  require(
      ownerClass.isValid() && lostClass.isValid() && survivorClass.isValid(),
      "automatic acquisition-cancellation object-class lookup returned an invalid handle");
  require(
      ownerAttribute.isValid() && lostAttribute.isValid() && survivorAttribute.isValid(),
      "automatic acquisition-cancellation attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  lost.rtiAmbassador().publishObjectClassAttributes(lostClass, lostAttributes);
  lost.rtiAmbassador().subscribeObjectClassAttributes(
      lostClass,
      lostAttributes,
      true,
      L"");
  survivor.rtiAmbassador().publishObjectClassAttributes(
      survivorClass,
      survivorAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "automatic acquisition-cancellation registration returned an invalid handle");
  waitFor(
      lost,
      [&] { return lost.recorder().hasDiscovery(object); },
      options,
      "automatic acquisition-cancellation lost-member discovery");
  require(
      !lost.rtiAmbassador().isAttributeOwnedByFederate(object, lostAttribute),
      "automatic acquisition-cancellation lost member unexpectedly owns the attribute");
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "automatic acquisition-cancellation survivor discovery");
  require(
      !survivor.rtiAmbassador().isAttributeOwnedByFederate(object, survivorAttribute),
      "automatic acquisition-cancellation survivor unexpectedly owns the attribute");

  std::vector<std::uint8_t> const acquisitionTagBytes{0xD6U, 0x25U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xD7U, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  lost.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      lostAttributes,
      acquisitionTag);
  require(
      !owner.recorder().ownershipReleaseRequest().has_value(),
      "automatic acquisition-cancellation queued an owner release callback too early");
  lost.rtiAmbassador().setAutomaticResignDirective(
      rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() ==
          rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
      "automatic acquisition-cancellation directive did not round-trip");
  survivor.recorder().clearOwnershipRecords();

  waitForConnectionLossAndSignal(
      lost,
      options,
      "automatic pending-acquisition cancellation");
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  require(
      !owner.recorder().ownershipReleaseRequest().has_value(),
      "automatic pending-acquisition cancellation delivered stale owner release work");

  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      divestitureTag);
  waitFor(
      survivor,
      [&] { return survivor.recorder().ownershipAssumption().has_value(); },
      options,
      "automatic acquisition-cancellation survivor assumption");
  auto const assumption = survivor.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes == survivorAttributes &&
          assumption->tag == divestitureTagBytes,
      "automatic acquisition-cancellation offered the wrong surviving ownership metadata");
  require(
      !survivor.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          survivorAttribute),
      "automatic acquisition-cancellation transferred ownership before acquisition");

  survivor.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.disconnect();
  survivor.disconnect();
}

void scenarioConnectionLossAutomaticCancelPendingAcquisitionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionLossAutomaticCancelPendingAcquisition(options, model);
}

std::vector<std::pair<std::string, rti::CallbackModel>> callbackModels(Options const& options) {
  if (options.callbackModel == "evoked") {
    return {{"evoked", rti::HLA_EVOKED}};
  }
  if (options.callbackModel == "immediate") {
    return {{"immediate", rti::HLA_IMMEDIATE}};
  }
  return {{"evoked", rti::HLA_EVOKED}, {"immediate", rti::HLA_IMMEDIATE}};
}
}  // namespace hla_rti_cpp_tck::detail
