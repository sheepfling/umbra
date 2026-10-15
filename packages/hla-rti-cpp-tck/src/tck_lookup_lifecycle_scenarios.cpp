#include "tck_lookup_lifecycle_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioInteractionClassLookupLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Interaction-class lookup testing requires an adapter-supplied base FOM");
  require(
      !options.additionalFomModules.empty(),
      "Interaction-class lookup testing requires an adapter-supplied extension FOM");

  Session observer(options, model, "interaction-class-lookup-observer");
  Session owner(options, model, "interaction-class-lookup-owner");
  Session extension(options, model, "interaction-class-lookup-extension");
  auto const federation = federationName(options, "interaction-class-lookup-lifecycle");
  auto const extensionInteractionName = L"HLAinteractionRoot.TckExtensionInteraction";
  auto const unknownInteractionName = L"HLAinteractionRoot.TckMissingInteraction";
  rti::InteractionClassHandle invalid;

  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getInteractionClassHandle(
            options.interactionClassName));
      },
      L"NotConnected",
      "looking up an interaction class before connecting");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().getInteractionClassName(invalid)); },
      L"NotConnected",
      "looking up an interaction-class name before connecting");

  observer.connect();
  owner.connect();
  extension.connect();
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getInteractionClassHandle(
            options.interactionClassName));
      },
      L"FederateNotExecutionMember",
      "looking up an interaction class after connecting but before joining");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().getInteractionClassName(invalid)); },
      L"FederateNotExecutionMember",
      "looking up an interaction-class name after connecting but before joining");

  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const baseInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      baseInteraction.isValid() &&
          owner.rtiAmbassador().getInteractionClassHandle(options.interactionClassName) ==
              baseInteraction &&
          owner.rtiAmbassador().getInteractionClassName(baseInteraction) ==
              options.interactionClassName,
      "ordinary interaction-class lookup did not round-trip a stable base handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getInteractionClassHandle(
            unknownInteractionName));
      },
      L"NameNotFound",
      "looking up an unknown interaction class");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getInteractionClassName(invalid)); },
      L"InvalidInteractionClassHandle",
      "looking up an interaction-class name from an invalid handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getInteractionClassHandle(
            extensionInteractionName));
      },
      L"NameNotFound",
      "looking up the extension interaction before an additional-module join");

  extension.join(
      options.memberFederateName,
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));
  auto const ownerExtensionInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      extensionInteractionName);
  auto const memberExtensionInteraction = extension.rtiAmbassador().getInteractionClassHandle(
      extensionInteractionName);
  require(
      ownerExtensionInteraction.isValid() &&
          memberExtensionInteraction == ownerExtensionInteraction &&
          owner.rtiAmbassador().getInteractionClassName(ownerExtensionInteraction) ==
              extensionInteractionName &&
          owner.rtiAmbassador().getInteractionClassHandle(options.interactionClassName) ==
              baseInteraction,
      "additional-module join did not compose a shared extension interaction or preserve the base handle");

  extension.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  owner.disconnect();
  observer.disconnect();
}

void scenarioObjectClassLookupLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Object-class lookup testing requires an adapter-supplied base FOM");
  require(
      !options.additionalFomModules.empty(),
      "Object-class lookup testing requires an adapter-supplied extension FOM");

  Session observer(options, model, "object-class-lookup-observer");
  Session owner(options, model, "object-class-lookup-owner");
  Session extension(options, model, "object-class-lookup-extension");
  auto const federation = federationName(options, "object-class-lookup-lifecycle");
  auto const extensionObjectName = L"HLAobjectRoot.TckExtensionObject";
  auto const unknownObjectName = L"HLAobjectRoot.TckMissingObject";
  rti::ObjectClassHandle invalid;

  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getObjectClassHandle(
            options.objectClassName));
      },
      L"NotConnected",
      "looking up an object class before connecting");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().getObjectClassName(invalid)); },
      L"NotConnected",
      "looking up an object-class name before connecting");

  observer.connect();
  owner.connect();
  extension.connect();
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getObjectClassHandle(
            options.objectClassName));
      },
      L"FederateNotExecutionMember",
      "looking up an object class after connecting but before joining");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().getObjectClassName(invalid)); },
      L"FederateNotExecutionMember",
      "looking up an object-class name after connecting but before joining");

  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const baseObject = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  require(
      baseObject.isValid() &&
          owner.rtiAmbassador().getObjectClassHandle(options.objectClassName) ==
              baseObject &&
          owner.rtiAmbassador().getObjectClassName(baseObject) ==
              options.objectClassName,
      "ordinary object-class lookup did not round-trip a stable base handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
            unknownObjectName));
      },
      L"NameNotFound",
      "looking up an unknown object class");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectClassName(invalid)); },
      L"InvalidObjectClassHandle",
      "looking up an object-class name from an invalid handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
            extensionObjectName));
      },
      L"NameNotFound",
      "looking up the extension object class before an additional-module join");

  extension.join(
      options.memberFederateName,
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));
  auto const ownerExtensionObject = owner.rtiAmbassador().getObjectClassHandle(
      extensionObjectName);
  auto const memberExtensionObject = extension.rtiAmbassador().getObjectClassHandle(
      extensionObjectName);
  require(
      ownerExtensionObject.isValid() &&
          memberExtensionObject == ownerExtensionObject &&
          owner.rtiAmbassador().getObjectClassName(ownerExtensionObject) ==
              extensionObjectName &&
          owner.rtiAmbassador().getObjectClassHandle(options.objectClassName) ==
              baseObject,
      "additional-module join did not compose a shared extension object class or preserve the base handle");

  extension.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  owner.disconnect();
  observer.disconnect();
}

void scenarioAttributeLookupLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Attribute lookup testing requires an adapter-supplied rich FOM model");

  Session observer(options, model, "attribute-lookup-observer");
  Session owner(options, model, "attribute-lookup-owner");
  Session member(options, model, "attribute-lookup-member");
  auto const federation = federationName(options, "attribute-lookup-lifecycle");
  auto const missingAttributeName = L"TckMissingAttribute";
  rti::ObjectClassHandle invalidObjectClass;
  rti::AttributeHandle invalidAttribute;

  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getAttributeHandle(
            invalidObjectClass,
            options.typedIdentityAttributeName));
      },
      L"NotConnected",
      "looking up an attribute before connecting");
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getAttributeName(
            invalidObjectClass,
            invalidAttribute));
      },
      L"NotConnected",
      "looking up an attribute name before connecting");

  observer.connect();
  owner.connect();
  member.connect();
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getAttributeHandle(
            invalidObjectClass,
            options.typedIdentityAttributeName));
      },
      L"FederateNotExecutionMember",
      "looking up an attribute after connecting but before joining");
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getAttributeName(
            invalidObjectClass,
            invalidAttribute));
      },
      L"FederateNotExecutionMember",
      "looking up an attribute name after connecting but before joining");

  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const baseObject = owner.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const derivedObject = owner.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const baseIdentity = owner.rtiAmbassador().getAttributeHandle(
      baseObject,
      options.typedIdentityAttributeName);
  auto const derivedIdentity = owner.rtiAmbassador().getAttributeHandle(
      derivedObject,
      options.typedIdentityAttributeName);
  auto const derivedAttribute = owner.rtiAmbassador().getAttributeHandle(
      derivedObject,
      options.typedDerivedAttributeName);
  require(
      baseObject.isValid() && derivedObject.isValid() && baseIdentity.isValid() &&
          derivedIdentity.isValid() && derivedAttribute.isValid(),
      "inherited attribute lookup returned an invalid class or attribute handle");
  require(
      baseIdentity == derivedIdentity &&
          owner.rtiAmbassador().getAttributeHandle(
              derivedObject,
              options.typedIdentityAttributeName) == baseIdentity &&
          owner.rtiAmbassador().getAttributeName(baseObject, baseIdentity) ==
              options.typedIdentityAttributeName &&
          owner.rtiAmbassador().getAttributeName(derivedObject, derivedIdentity) ==
              options.typedIdentityAttributeName &&
          owner.rtiAmbassador().getAttributeName(
              derivedObject,
              derivedAttribute) == options.typedDerivedAttributeName,
      "inherited attribute lookup did not preserve stable handles and names");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeHandle(
            baseObject,
            options.typedDerivedAttributeName));
      },
      L"NameNotFound",
      "looking up a derived attribute on its base object class");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeHandle(
            derivedObject,
            missingAttributeName));
      },
      L"NameNotFound",
      "looking up an unknown attribute name");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeHandle(
            invalidObjectClass,
            options.typedIdentityAttributeName));
      },
      L"InvalidObjectClassHandle",
      "looking up an attribute from an invalid object-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeName(
            invalidObjectClass,
            baseIdentity));
      },
      L"InvalidObjectClassHandle",
      "looking up an attribute name from an invalid object-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeName(
            derivedObject,
            invalidAttribute));
      },
      L"InvalidAttributeHandle",
      "looking up an attribute name from an invalid attribute handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeName(
            baseObject,
            derivedAttribute));
      },
      L"AttributeNotDefined",
      "looking up a derived attribute handle on its base object class");

  member.join(options.memberFederateName, options.federateType, federation);
  auto const memberDerivedObject = member.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const memberIdentity = member.rtiAmbassador().getAttributeHandle(
      memberDerivedObject,
      options.typedIdentityAttributeName);
  auto const memberDerivedAttribute = member.rtiAmbassador().getAttributeHandle(
      memberDerivedObject,
      options.typedDerivedAttributeName);
  require(
      memberDerivedObject == derivedObject && memberIdentity == baseIdentity &&
          memberDerivedAttribute == derivedAttribute,
      "inherited attribute handles did not retain cross-federate identity");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
  observer.disconnect();
}

void scenarioParameterLookupLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Parameter lookup testing requires an adapter-supplied rich FOM model");

  Session observer(options, model, "parameter-lookup-observer");
  Session owner(options, model, "parameter-lookup-owner");
  Session member(options, model, "parameter-lookup-member");
  auto const federation = federationName(options, "parameter-lookup-lifecycle");
  auto const missingParameterName = L"TckMissingParameter";
  rti::InteractionClassHandle invalidInteractionClass;
  rti::ParameterHandle invalidParameter;

  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getParameterHandle(
            invalidInteractionClass,
            options.typedIntegerParameterName));
      },
      L"NotConnected",
      "looking up a parameter before connecting");
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getParameterName(
            invalidInteractionClass,
            invalidParameter));
      },
      L"NotConnected",
      "looking up a parameter name before connecting");

  observer.connect();
  owner.connect();
  member.connect();
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getParameterHandle(
            invalidInteractionClass,
            options.typedIntegerParameterName));
      },
      L"FederateNotExecutionMember",
      "looking up a parameter after connecting but before joining");
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getParameterName(
            invalidInteractionClass,
            invalidParameter));
      },
      L"FederateNotExecutionMember",
      "looking up a parameter name after connecting but before joining");

  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const baseInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const derivedInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  auto const baseParameter = owner.rtiAmbassador().getParameterHandle(
      baseInteraction,
      options.typedIntegerParameterName);
  auto const derivedInheritedParameter = owner.rtiAmbassador().getParameterHandle(
      derivedInteraction,
      options.typedIntegerParameterName);
  auto const derivedParameter = owner.rtiAmbassador().getParameterHandle(
      derivedInteraction,
      options.typedDerivedParameterName);
  require(
      baseInteraction.isValid() && derivedInteraction.isValid() &&
          baseParameter.isValid() && derivedInheritedParameter.isValid() &&
          derivedParameter.isValid(),
      "inherited parameter lookup returned an invalid class or parameter handle");
  require(
      baseParameter == derivedInheritedParameter &&
          owner.rtiAmbassador().getParameterHandle(
              derivedInteraction,
              options.typedIntegerParameterName) == baseParameter &&
          owner.rtiAmbassador().getParameterName(
              baseInteraction,
              baseParameter) == options.typedIntegerParameterName &&
          owner.rtiAmbassador().getParameterName(
              derivedInteraction,
              derivedInheritedParameter) == options.typedIntegerParameterName &&
          owner.rtiAmbassador().getParameterName(
              derivedInteraction,
              derivedParameter) == options.typedDerivedParameterName,
      "inherited parameter lookup did not preserve stable handles and names");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            baseInteraction,
            options.typedDerivedParameterName));
      },
      L"NameNotFound",
      "looking up a derived parameter name on its base interaction class");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            derivedInteraction,
            missingParameterName));
      },
      L"NameNotFound",
      "looking up an unknown parameter name");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            invalidInteractionClass,
            options.typedIntegerParameterName));
      },
      L"InvalidInteractionClassHandle",
      "looking up a parameter from an invalid interaction-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterName(
            invalidInteractionClass,
            baseParameter));
      },
      L"InvalidInteractionClassHandle",
      "looking up a parameter name from an invalid interaction-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterName(
            derivedInteraction,
            invalidParameter));
      },
      L"InvalidParameterHandle",
      "looking up a parameter name from an invalid parameter handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterName(
            baseInteraction,
            derivedParameter));
      },
      L"InteractionParameterNotDefined",
      "looking up a derived parameter handle on its base interaction class");

  member.join(options.memberFederateName, options.federateType, federation);
  auto const memberDerivedInteraction = member.rtiAmbassador().getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  auto const memberInheritedParameter = member.rtiAmbassador().getParameterHandle(
      memberDerivedInteraction,
      options.typedIntegerParameterName);
  auto const memberDerivedParameter = member.rtiAmbassador().getParameterHandle(
      memberDerivedInteraction,
      options.typedDerivedParameterName);
  require(
      memberDerivedInteraction == derivedInteraction &&
          memberInheritedParameter == baseParameter &&
          memberDerivedParameter == derivedParameter,
      "inherited parameter handles did not retain cross-federate identity");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
  observer.disconnect();
}

void scenarioDimensionLookupLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Dimension lookup testing requires an adapter-supplied dimensional FOM");
  require(
      options.ddmDimensionNames.size() >= 2U,
      "Dimension lookup testing requires two adapter-supplied dimension names");

  Session lifecycle(options, model, "dimension-lookup-lifecycle");
  rti::DimensionHandle const invalidDimension;
  rti::ObjectClassHandle const invalidObjectClass;
  rti::InteractionClassHandle const invalidInteractionClass;
  auto requireLookupLifecycle = [&](Session& session,
                                    std::wstring const& expected,
                                    std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(session.rtiAmbassador().getDimensionHandle(
              options.ddmDimensionNames.at(0)));
        },
        expected,
        "looking up a dimension handle " + phase);
    requireException(
        [&] {
          static_cast<void>(session.rtiAmbassador().getDimensionName(
              invalidDimension));
        },
        expected,
        "looking up a dimension name " + phase);
    requireException(
        [&] {
          static_cast<void>(session.rtiAmbassador().getDimensionUpperBound(
              invalidDimension));
        },
        expected,
        "looking up a dimension upper bound " + phase);
    requireException(
        [&] {
          static_cast<void>(session.rtiAmbassador().getAvailableDimensionsForObjectClass(
              invalidObjectClass));
        },
        expected,
        "looking up object-class dimensions " + phase);
    requireException(
        [&] {
          static_cast<void>(session.rtiAmbassador()
                                .getAvailableDimensionsForInteractionClass(
                                    invalidInteractionClass));
        },
        expected,
        "looking up interaction-class dimensions " + phase);
  };

  requireLookupLifecycle(lifecycle, L"NotConnected", "before connect");
  lifecycle.connect();
  requireLookupLifecycle(
      lifecycle,
      L"FederateNotExecutionMember",
      "after connect before join");
  lifecycle.disconnect();

  Session owner(options, model, "dimension-lookup-owner");
  Session member(options, model, "dimension-lookup-member");
  auto const federation = federationName(options, "dimension-lookup-lifecycle");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto const ownerObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const memberObjectClass = member.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerInteractionClass = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const memberInteractionClass = member.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      ownerObjectClass.isValid() && memberObjectClass.isValid() &&
          ownerInteractionClass.isValid() && memberInteractionClass.isValid(),
      "dimension lookup returned an invalid adapter-supplied class handle");

  auto const ownerFirst = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.at(0));
  auto const ownerSecond = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.at(1));
  auto const memberFirst = member.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.at(0));
  auto const memberSecond = member.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.at(1));
  require(
      ownerFirst.isValid() && ownerSecond.isValid() &&
          memberFirst.isValid() && memberSecond.isValid() &&
          ownerFirst != ownerSecond && memberFirst != memberSecond,
      "dimension lookup returned invalid or duplicate handles");
  require(
      ownerFirst == memberFirst && ownerSecond == memberSecond,
      "dimension handles did not retain identity across federates");
  require(
      owner.rtiAmbassador().getDimensionName(ownerFirst) ==
              options.ddmDimensionNames.at(0) &&
          owner.rtiAmbassador().getDimensionName(ownerSecond) ==
              options.ddmDimensionNames.at(1) &&
          member.rtiAmbassador().getDimensionName(memberFirst) ==
              options.ddmDimensionNames.at(0) &&
          member.rtiAmbassador().getDimensionName(memberSecond) ==
              options.ddmDimensionNames.at(1),
      "dimension handle-to-name lookup did not round-trip");
  require(
      owner.rtiAmbassador().getDimensionUpperBound(ownerFirst) > 0U &&
          owner.rtiAmbassador().getDimensionUpperBound(ownerSecond) > 0U &&
          owner.rtiAmbassador().getDimensionUpperBound(ownerFirst) ==
              member.rtiAmbassador().getDimensionUpperBound(memberFirst) &&
          owner.rtiAmbassador().getDimensionUpperBound(ownerSecond) ==
              member.rtiAmbassador().getDimensionUpperBound(memberSecond),
      "dimension upper bounds were not positive and stable across federates");

  auto const ownerObjectDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(ownerObjectClass);
  auto const memberObjectDimensions =
      member.rtiAmbassador().getAvailableDimensionsForObjectClass(memberObjectClass);
  auto const ownerInteractionDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForInteractionClass(
          ownerInteractionClass);
  auto const memberInteractionDimensions =
      member.rtiAmbassador().getAvailableDimensionsForInteractionClass(
          memberInteractionClass);
  require(
      ownerObjectDimensions.count(ownerFirst) == 1U &&
          ownerObjectDimensions.count(ownerSecond) == 1U &&
          memberObjectDimensions.count(memberFirst) == 1U &&
          memberObjectDimensions.count(memberSecond) == 1U &&
          ownerInteractionDimensions.count(ownerFirst) == 1U &&
          ownerInteractionDimensions.count(ownerSecond) == 1U &&
          memberInteractionDimensions.count(memberFirst) == 1U &&
          memberInteractionDimensions.count(memberSecond) == 1U,
      "available class dimensions omitted an adapter-supplied dimension");

  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionHandle(
            L"TckMissingDimension"));
      },
      L"NameNotFound",
      "looking up an unknown dimension name");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionName(invalidDimension));
      },
      L"InvalidDimensionHandle",
      "looking up an invalid dimension name");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionUpperBound(
            invalidDimension));
      },
      L"InvalidDimensionHandle",
      "looking up an invalid dimension upper bound");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAvailableDimensionsForObjectClass(
            invalidObjectClass));
      },
      L"InvalidObjectClassHandle",
      "looking up dimensions for an invalid object class");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador()
                              .getAvailableDimensionsForInteractionClass(
                                  invalidInteractionClass));
      },
      L"InvalidInteractionClassHandle",
      "looking up dimensions for an invalid interaction class");

  owner.resign(rti::NO_ACTION);
  requireLookupLifecycle(
      owner,
      L"FederateNotExecutionMember",
      "after resign before federation destruction");
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioDimensionLookupLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDimensionLookupLifecycle(options, model);
}

void scenarioInteractionClassLookupLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionClassLookupLifecycle(options, model);
}

void scenarioObjectClassLookupLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectClassLookupLifecycle(options, model);
}

void scenarioAttributeLookupLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeLookupLifecycle(options, model);
}

void scenarioParameterLookupLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioParameterLookupLifecycle(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
