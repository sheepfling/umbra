#include "tck_fom_module_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomModuleComposition(Options const& options, rti::CallbackModel model) {
  require(!options.additionalFomModules.empty(),
          "FOM extension input was not supplied by the adapter");
  auto const extensionModules = fomModuleNames(options.additionalFomModules);

  Session composedOwner(options, model, "fom-composed-owner");
  Session composedMember(options, model, "fom-composed-member");
  auto const composedFederation = federationName(options, "fom-composed-create");
  composedOwner.connect();
  composedMember.connect();
  std::vector<std::wstring> createModules{options.fom.wstring()};
  createModules.insert(createModules.end(), extensionModules.begin(), extensionModules.end());
  composedOwner.rtiAmbassador().createFederationExecution(
      composedFederation,
      createModules,
      options.logicalTimeImplementationName);
  composedOwner.join(options.ownerFederateName, options.federateType, composedFederation);
  composedMember.join(options.memberFederateName, options.federateType, composedFederation);

  auto const extensionClass = composedOwner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckExtensionObject");
  auto const extensionAttribute = composedOwner.rtiAmbassador().getAttributeHandle(
      extensionClass, L"ExtensionValue");
  auto const extensionInteraction = composedOwner.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.TckExtensionInteraction");
  auto const extensionParameter = composedOwner.rtiAmbassador().getParameterHandle(
      extensionInteraction, L"ExtensionParameter");
  require(extensionClass.isValid() && extensionAttribute.isValid() &&
              extensionInteraction.isValid() && extensionParameter.isValid(),
          "Created FOM module composition omitted an extension declaration");
  require(composedMember.rtiAmbassador().getObjectClassHandle(
              L"HLAobjectRoot.TckExtensionObject") == extensionClass,
          "Created FOM module composition did not share class handles");

  rti::AttributeHandleSet extensionAttributes;
  extensionAttributes.insert(extensionAttribute);
  composedOwner.rtiAmbassador().publishObjectClassAttributes(
      extensionClass, extensionAttributes);
  composedMember.rtiAmbassador().subscribeObjectClassAttributes(
      extensionClass, extensionAttributes, true, L"");
  auto const extensionObject =
      composedOwner.rtiAmbassador().registerObjectInstance(extensionClass);
  waitFor(composedMember, [&] { return composedMember.recorder().discovery().present; }, options,
          "created-composition extension discovery");
  require(composedMember.recorder().discovery().object == extensionObject,
          "Created FOM module composition returned the wrong object");
  rti::HLAinteger32BE extensionValue{123};
  auto const extensionBytes = encodeDataElement(extensionValue);
  rti::AttributeHandleValueMap extensionUpdate;
  extensionUpdate.emplace(
      extensionAttribute,
      rti::VariableLengthData(extensionBytes.data(), extensionBytes.size()));
  std::vector<std::uint8_t> extensionTagBytes{0xe1U};
  rti::VariableLengthData extensionTag(extensionTagBytes.data(), extensionTagBytes.size());
  composedOwner.rtiAmbassador().updateAttributeValues(
      extensionObject, extensionUpdate, extensionTag);
  waitFor(composedMember, [&] { return composedMember.recorder().reflection().present; }, options,
          "created-composition extension reflection");
  require(copyBytes(composedMember.recorder().reflection().values.at(extensionAttribute)) ==
              extensionBytes && composedMember.recorder().reflection().tag == extensionTagBytes,
          "Created FOM module composition changed extension data");
  composedOwner.rtiAmbassador().publishInteractionClass(extensionInteraction);
  composedMember.rtiAmbassador().subscribeInteractionClass(extensionInteraction, true);
  rti::ParameterHandleValueMap extensionParameters;
  extensionParameters.emplace(
      extensionParameter,
      rti::VariableLengthData(extensionBytes.data(), extensionBytes.size()));
  composedOwner.rtiAmbassador().sendInteraction(
      extensionInteraction, extensionParameters, extensionTag);
  waitFor(composedMember, [&] { return composedMember.recorder().interaction().present; }, options,
          "created-composition extension interaction");
  require(composedMember.recorder().interaction().parameters.count(extensionParameter) == 1U,
          "Created FOM module composition omitted extension interaction data");
  composedOwner.resign(rti::DELETE_OBJECTS);
  composedMember.resign(rti::NO_ACTION);

  Session baseOwner(options, model, "fom-base-owner");
  Session extensionJoiner(options, model, "fom-extension-joiner");
  auto const joinedFederation = federationName(options, "fom-composed-join");
  baseOwner.connect();
  extensionJoiner.connect();
  baseOwner.rtiAmbassador().createFederationExecution(
      joinedFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  baseOwner.join(options.ownerFederateName, options.federateType, joinedFederation);
  requireException(
      [&] {
        static_cast<void>(baseOwner.rtiAmbassador().getObjectClassHandle(
            L"HLAobjectRoot.TckExtensionObject"));
      },
      L"NameNotFound",
      "extension lookup before additional-module join");
  extensionJoiner.join(
      options.memberFederateName,
      options.federateType,
      joinedFederation,
      extensionModules);
  auto const joinedExtensionClass = baseOwner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckExtensionObject");
  require(joinedExtensionClass.isValid() &&
              extensionJoiner.rtiAmbassador().getObjectClassHandle(
                  L"HLAobjectRoot.TckExtensionObject") == joinedExtensionClass,
          "Additional-module join did not compose the extension for all members");
  require(baseOwner.rtiAmbassador().getObjectClassName(joinedExtensionClass) ==
              L"HLAobjectRoot.TckExtensionObject",
          "Additional-module join extension name did not round-trip");
  baseOwner.resign(rti::NO_ACTION);
  extensionJoiner.resign(rti::NO_ACTION);
}

void scenarioFomAdditionalModuleJoinAtomicity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Additional-module join testing requires an adapter-supplied base FOM");
  require(
      !options.additionalFomModules.empty(),
      "Additional-module join testing requires an adapter-supplied extension FOM");
  require(
      !options.invalidFomModules.empty(),
      "Additional-module join testing requires an adapter-supplied invalid FOM");

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "fom-additional-join-atomicity");
  owner.connect();
  member.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const extensionClassName = L"HLAobjectRoot.TckExtensionObject";
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
            extensionClassName));
      },
      L"NameNotFound",
      "looking up the extension class before an additional-module join");

  auto const invalidFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM",
      L"ErrorReadingFOM",
      L"InvalidFOM",
      L"InconsistentFOM"};
  for (auto const& invalid : options.invalidFomModules) {
    requireExceptionOneOf(
        [&] {
          member.join(
              options.memberFederateName,
              options.federateType,
              federation,
              std::vector<std::wstring>{invalid.wstring()});
        },
        invalidFomErrors,
        "joining with invalid additional FOM " + invalid.string());
    requireException(
        [&] {
          static_cast<void>(owner.rtiAmbassador().getFederateHandle(
              options.memberFederateName));
        },
        L"NameNotFound",
        "looking up a member after rejected additional-module join");
    requireException(
        [&] {
          static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
              extensionClassName));
        },
        L"NameNotFound",
        "looking up the extension class after rejected additional-module join");
  }

  member.join(
      options.memberFederateName,
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));
  auto const ownerExtensionClass = owner.rtiAmbassador().getObjectClassHandle(
      extensionClassName);
  auto const memberExtensionClass = member.rtiAmbassador().getObjectClassHandle(
      extensionClassName);
  require(
      ownerExtensionClass.isValid() &&
          memberExtensionClass == ownerExtensionClass,
      "valid additional-module join did not compose one shared extension class");
  require(
      owner.rtiAmbassador().getObjectClassName(ownerExtensionClass) ==
          extensionClassName &&
          owner.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
              member.federateHandle(),
      "valid additional-module join did not preserve standard class and member lookups");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioFomTransportationHandleStability(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "FOM transportation handle stability requires an adapter-supplied rich FOM model");
  require(
      !options.additionalFomModules.empty(),
      "FOM transportation handle stability requires an adapter-supplied additional FOM");

  Session owner(options, model, "fom-transportation-owner");
  Session extension(options, model, "fom-transportation-extension");
  auto const federation = federationName(options, "fom-transportation-handle-stability");
  owner.connect();
  extension.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const reliableBefore = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const customBefore = owner.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  require(
      reliableBefore.isValid() && customBefore.isValid() &&
          reliableBefore != customBefore,
      "rich adapter FOM did not expose distinct mandatory and custom transportation handles");
  require(
      owner.rtiAmbassador().getTransportationTypeName(reliableBefore) == L"HLAreliable" &&
          owner.rtiAmbassador().getTransportationTypeName(customBefore) ==
              options.fomCustomTransportationName,
      "rich adapter FOM transportation handles did not round-trip to their names");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeHandle(
            options.fomAdditionalTransportationName));
      },
      L"InvalidTransportationName",
      "additional-module transportation was visible before the additional join");

  extension.join(
      options.memberFederateName,
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));

  auto const ownerReliableAfter = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const ownerCustomAfter = owner.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  auto const ownerAdditional = owner.rtiAmbassador().getTransportationTypeHandle(
      options.fomAdditionalTransportationName);
  auto const extensionReliable = extension.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const extensionCustom = extension.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  auto const extensionAdditional = extension.rtiAmbassador().getTransportationTypeHandle(
      options.fomAdditionalTransportationName);
  require(
      ownerReliableAfter == reliableBefore && ownerCustomAfter == customBefore &&
          extensionReliable == reliableBefore && extensionCustom == customBefore,
      "additional-module join renumbered a pre-existing transportation handle");
  require(
      ownerAdditional.isValid() && extensionAdditional.isValid() &&
          ownerAdditional == extensionAdditional && ownerAdditional != customBefore &&
          ownerAdditional != reliableBefore,
      "additional-module transportation did not receive one shared distinct handle");
  require(
      owner.rtiAmbassador().getTransportationTypeName(ownerAdditional) ==
              options.fomAdditionalTransportationName &&
          extension.rtiAmbassador().getTransportationTypeName(extensionAdditional) ==
              options.fomAdditionalTransportationName,
      "additional-module transportation handle did not round-trip to its name");

  extension.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioFomDimensionHandleStability(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "FOM dimension handle stability requires an adapter-supplied rich FOM model");
  require(
      !options.additionalFomModules.empty(),
      "FOM dimension handle stability requires an adapter-supplied additional FOM");

  Session owner(options, model, "fom-dimension-owner");
  Session extension(options, model, "fom-dimension-extension");
  auto const federation = federationName(options, "fom-dimension-handle-stability");
  owner.connect();
  extension.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const baseDimension = owner.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      baseDimension.isValid() &&
          owner.rtiAmbassador().getDimensionName(baseDimension) == options.fomDimensionName &&
          owner.rtiAmbassador().getDimensionUpperBound(baseDimension) > 0U,
      "rich adapter FOM did not expose a valid base dimension lookup");

  auto const baseObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const baseInteractionClass = owner.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  require(
      baseObjectClass.isValid() && baseInteractionClass.isValid(),
      "rich adapter FOM did not expose the typed classes used for dimension association");
  auto const baseObjectDimensions = owner.rtiAmbassador().getAvailableDimensionsForObjectClass(
      baseObjectClass);
  auto const baseInteractionDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForInteractionClass(baseInteractionClass);
  require(
      baseObjectDimensions.count(baseDimension) == 1U &&
          baseInteractionDimensions.count(baseDimension) == 1U,
      "rich adapter FOM did not retain the base dimension associations");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionHandle(
            options.fomAdditionalDimensionName));
      },
      L"NameNotFound",
      "looking up the additional dimension before an additional-module join");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
            options.fomAdditionalDimensionObjectClassName));
      },
      L"NameNotFound",
      "looking up the additional dimensional class before an additional-module join");

  extension.join(
      options.memberFederateName,
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));

  auto const ownerBaseAfter = owner.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  auto const extensionBaseAfter = extension.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      ownerBaseAfter == baseDimension && extensionBaseAfter == baseDimension &&
          owner.rtiAmbassador().getDimensionName(ownerBaseAfter) == options.fomDimensionName &&
          extension.rtiAmbassador().getDimensionName(extensionBaseAfter) ==
              options.fomDimensionName,
      "additional-module join renumbered or split the existing base dimension handle");

  auto const ownerAdditional = owner.rtiAmbassador().getDimensionHandle(
      options.fomAdditionalDimensionName);
  auto const extensionAdditional = extension.rtiAmbassador().getDimensionHandle(
      options.fomAdditionalDimensionName);
  require(
      ownerAdditional.isValid() && extensionAdditional.isValid() &&
          ownerAdditional == extensionAdditional && ownerAdditional != baseDimension,
      "additional-module join did not provide one shared distinct dimension handle");
  require(
      owner.rtiAmbassador().getDimensionName(ownerAdditional) ==
              options.fomAdditionalDimensionName &&
          extension.rtiAmbassador().getDimensionName(extensionAdditional) ==
              options.fomAdditionalDimensionName &&
          owner.rtiAmbassador().getDimensionUpperBound(ownerAdditional) > 0U &&
          owner.rtiAmbassador().getDimensionUpperBound(ownerAdditional) ==
              extension.rtiAmbassador().getDimensionUpperBound(extensionAdditional),
      "additional-module dimension lookup did not round-trip consistently");

  auto const ownerAdditionalClass = owner.rtiAmbassador().getObjectClassHandle(
      options.fomAdditionalDimensionObjectClassName);
  auto const extensionAdditionalClass = extension.rtiAmbassador().getObjectClassHandle(
      options.fomAdditionalDimensionObjectClassName);
  require(
      ownerAdditionalClass.isValid() && extensionAdditionalClass == ownerAdditionalClass,
      "additional-module dimensional class did not receive one shared handle");
  auto const ownerAdditionalAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerAdditionalClass,
      options.fomAdditionalDimensionAttributeName);
  auto const extensionAdditionalAttribute = extension.rtiAmbassador().getAttributeHandle(
      extensionAdditionalClass,
      options.fomAdditionalDimensionAttributeName);
  require(
      ownerAdditionalAttribute.isValid() &&
          extensionAdditionalAttribute == ownerAdditionalAttribute,
      "additional-module dimensional attribute did not resolve consistently");
  auto const ownerAdditionalDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(ownerAdditionalClass);
  auto const extensionAdditionalDimensions =
      extension.rtiAmbassador().getAvailableDimensionsForObjectClass(extensionAdditionalClass);
  require(
      ownerAdditionalDimensions.size() == 1U &&
          ownerAdditionalDimensions.count(ownerAdditional) == 1U &&
          extensionAdditionalDimensions == ownerAdditionalDimensions,
      "additional-module class did not retain its dimension association");

  extension.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  owner.disconnect();
}

void scenarioFomUpdateRateValueStability(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "FOM update-rate value stability requires an adapter-supplied rich FOM model");
  require(
      !options.additionalFomModules.empty(),
      "FOM update-rate value stability requires an adapter-supplied additional FOM");

  Session owner(options, model, "fom-update-rate-owner");
  Session extension(options, model, "fom-update-rate-extension");
  auto const federation = federationName(options, "fom-update-rate-value-stability");
  owner.connect();
  extension.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const baseRate = owner.rtiAmbassador().getUpdateRateValue(
      options.fomUpdateRateName);
  require(
      baseRate > 0.0,
      "rich adapter FOM did not expose a positive base update-rate value");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getUpdateRateValue(
            options.fomAdditionalUpdateRateName));
      },
      L"InvalidUpdateRateDesignator",
      "additional-module update rate was visible before the additional join");

  extension.join(
      options.memberFederateName,
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));

  auto const ownerBaseAfter = owner.rtiAmbassador().getUpdateRateValue(
      options.fomUpdateRateName);
  auto const extensionBase = extension.rtiAmbassador().getUpdateRateValue(
      options.fomUpdateRateName);
  auto const ownerAdditional = owner.rtiAmbassador().getUpdateRateValue(
      options.fomAdditionalUpdateRateName);
  auto const extensionAdditional = extension.rtiAmbassador().getUpdateRateValue(
      options.fomAdditionalUpdateRateName);
  require(
      ownerBaseAfter == baseRate && extensionBase == baseRate,
      "additional-module join changed the existing update-rate value");
  require(
      ownerAdditional > 0.0 && extensionAdditional == ownerAdditional,
      "additional-module update-rate value was not composed consistently across members");

  extension.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  owner.disconnect();
}

void scenarioFomStandardInvalidInputCorpus(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Standard invalid-FOM testing requires an adapter-supplied base FOM");
  require(
      !options.standardInvalidFomModules.empty(),
      "Standard invalid-FOM testing requires adapter-supplied invalid modules");

  Session creator(options, model, "fom-standard-invalid-creator");
  creator.connect();
  auto const invalidFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM",
      L"ErrorReadingFOM",
      L"InvalidFOM",
      L"InconsistentFOM"};

  for (std::size_t index = 0; index < options.standardInvalidFomModules.size(); ++index) {
    auto const& invalid = options.standardInvalidFomModules.at(index);
    auto const federation = federationName(
        options, "fom-standard-invalid-" + std::to_string(index));

    requireExceptionOneOf(
        [&] {
          creator.rtiAmbassador().createFederationExecution(
              federation,
              invalid.wstring(),
              options.logicalTimeImplementationName);
        },
        invalidFomErrors,
        "rejecting standard-invalid FOM input " + invalid.string());

    creator.rtiAmbassador().createFederationExecution(
        federation,
        options.fom.wstring(),
        options.logicalTimeImplementationName);
    creator.join(
        options.ownerFederateName,
        options.federateType,
        federation);
    require(
        creator.federateHandle().isValid(),
        "valid FOM reuse after invalid-FOM rejection did not establish membership");
    creator.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }

  creator.disconnect();
}

void scenarioFomInvalidCreateAtomicity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Invalid-FOM create testing requires an adapter-supplied base FOM");
  require(
      !options.invalidFomModules.empty(),
      "Invalid-FOM create testing requires adapter-supplied invalid FOM modules");

  Session creator(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "fom-invalid-create-atomicity");
  creator.connect();
  member.connect();

  auto const invalidFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM",
      L"ErrorReadingFOM",
      L"InvalidFOM",
      L"InconsistentFOM"};
  for (auto const& invalid : options.invalidFomModules) {
    requireExceptionOneOf(
        [&] {
          creator.rtiAmbassador().createFederationExecution(
              federation,
              invalid.wstring(),
              options.logicalTimeImplementationName);
        },
        invalidFomErrors,
        "creating a federation with invalid FOM " + invalid.string());
  }

  creator.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  require(
      creator.federateHandle().isValid() && member.federateHandle().isValid() &&
          creator.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
              member.federateHandle(),
      "valid create after invalid-FOM rejection did not establish membership");

  member.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomInvalidCompositeJoinAtomicity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Composite additional-FOM join testing requires an adapter-supplied base FOM");
  require(
      !options.additionalFomModules.empty(),
      "Composite additional-FOM join testing requires an adapter-supplied valid FOM");
  require(
      !options.invalidFomModules.empty(),
      "Composite additional-FOM join testing requires adapter-supplied invalid FOM modules");

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "fom-invalid-composite-join-atomicity");
  owner.connect();
  member.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const extensionClassName = L"HLAobjectRoot.TckExtensionObject";
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
            extensionClassName));
      },
      L"NameNotFound",
      "looking up the extension class before a composite additional-module join");

  auto const invalidFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM",
      L"ErrorReadingFOM",
      L"InvalidFOM",
      L"InconsistentFOM"};
  auto const validModules = fomModuleNames(options.additionalFomModules);
  for (auto const& invalid : options.invalidFomModules) {
    auto compositeModules = validModules;
    compositeModules.push_back(invalid.wstring());
    requireExceptionOneOf(
        [&] {
          member.join(
              options.memberFederateName,
              options.federateType,
              federation,
              compositeModules);
        },
        invalidFomErrors,
        "joining with valid and invalid additional FOM modules " + invalid.string());
    requireException(
        [&] {
          static_cast<void>(owner.rtiAmbassador().getFederateHandle(
              options.memberFederateName));
        },
        L"NameNotFound",
        "looking up a member after rejected composite additional-module join");
    requireException(
        [&] {
          static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
              extensionClassName));
        },
        L"NameNotFound",
        "looking up the extension class after rejected composite additional-module join");
  }

  member.join(
      options.memberFederateName,
      options.federateType,
      federation,
      validModules);
  auto const ownerExtensionClass = owner.rtiAmbassador().getObjectClassHandle(
      extensionClassName);
  require(
      ownerExtensionClass.isValid() &&
          member.rtiAmbassador().getObjectClassHandle(extensionClassName) ==
              ownerExtensionClass,
      "valid follow-up additional-module join did not compose one shared extension class");
  require(
      owner.rtiAmbassador().getObjectClassName(ownerExtensionClass) ==
          extensionClassName &&
          owner.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
              member.federateHandle(),
      "valid follow-up additional-module join did not preserve standard lookups");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioFomInvalidMimCreateAtomicity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Invalid-MIM create testing requires an adapter-supplied base FOM");
  require(
      !options.mimFom.empty(),
      "Invalid-MIM create testing requires an adapter-supplied valid MIM");
  require(
      !options.invalidFomModules.empty(),
      "Invalid-MIM create testing requires adapter-supplied invalid modules");

  Session creator(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "fom-invalid-mim-create-atomicity");
  creator.connect();
  member.connect();

  auto const invalidMimErrors = std::vector<std::wstring>{
      L"CouldNotOpenMIM",
      L"ErrorReadingMIM",
      L"InvalidMIM"};
  for (auto const& invalid : options.invalidFomModules) {
    requireExceptionOneOf(
        [&] {
          creator.rtiAmbassador().createFederationExecutionWithMIM(
              federation,
              std::vector<std::wstring>{options.fom.wstring()},
              invalid.wstring(),
              options.logicalTimeImplementationName);
        },
        invalidMimErrors,
        "creating a federation with invalid MIM " + invalid.string());
  }

  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  auto const objectClass = creator.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  require(
      objectClass.isValid() &&
          member.rtiAmbassador().getObjectClassHandle(options.objectClassName) ==
              objectClass &&
          creator.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
              member.federateHandle(),
      "valid MIM create after invalid-MIM rejection did not establish shared declarations and membership");

  member.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomInvalidCompositeMimCreateAtomicity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Composite MIM create testing requires an adapter-supplied base FOM");
  require(
      !options.mimFom.empty(),
      "Composite MIM create testing requires an adapter-supplied valid MIM");
  require(
      !options.invalidFomModules.empty(),
      "Composite MIM create testing requires adapter-supplied invalid FOM modules");

  Session creator(options, model, "owner");
  Session member(options, model, "member");
  auto const federation =
      federationName(options, "fom-invalid-composite-mim-create-atomicity");
  creator.connect();
  member.connect();

  auto const invalidFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM",
      L"ErrorReadingFOM",
      L"InvalidFOM",
      L"InconsistentFOM"};
  for (auto const& invalid : options.invalidFomModules) {
    requireExceptionOneOf(
        [&] {
          creator.rtiAmbassador().createFederationExecutionWithMIM(
              federation,
              std::vector<std::wstring>{options.fom.wstring(), invalid.wstring()},
              options.mimFom.wstring(),
              options.logicalTimeImplementationName);
        },
        invalidFomErrors,
        "creating a federation with valid and invalid FOM modules plus a valid MIM " +
            invalid.string());
  }

  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  auto const objectClass = creator.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  require(
      objectClass.isValid() &&
          member.rtiAmbassador().getObjectClassHandle(options.objectClassName) ==
              objectClass &&
          creator.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
              member.federateHandle(),
      "valid FOM/MIM create after rejected composite FOM input did not establish shared declarations and membership");

  member.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomEmptyMimCreateAtomicity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Empty-MIM create testing requires an adapter-supplied base FOM");
  require(
      !options.mimFom.empty(),
      "Empty-MIM create testing requires an adapter-supplied valid MIM");

  Session creator(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "fom-empty-mim-create-atomicity");
  creator.connect();
  member.connect();

  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecutionWithMIM(
            federation,
            std::vector<std::wstring>{},
            options.mimFom.wstring(),
            options.logicalTimeImplementationName);
      },
      L"InvalidFOM",
      "creating a federation with an empty FOM module vector and a valid MIM");

  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  auto const objectClass = creator.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  require(
      objectClass.isValid() &&
          member.rtiAmbassador().getObjectClassHandle(options.objectClassName) ==
              objectClass &&
          creator.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
              member.federateHandle(),
      "valid FOM/MIM create after empty-FOM rejection did not establish shared declarations and membership");

  member.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomEmptyModuleValidation(
    Options const& options,
    rti::CallbackModel model) {
  Session creator(options, model, "fom-empty-module-creator");
  Session member(options, model, "fom-empty-module-member");
  auto const federation = federationName(options, "fom-empty-module-validation");

  creator.connect();
  member.connect();

  std::vector<std::wstring> emptyModules;
  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            federation,
            emptyModules,
            options.logicalTimeImplementationName);
      },
      L"InvalidFOM",
      "creating a federation execution with no FOM modules");

  // The rejected request must not reserve the federation name or leave a
  // partially initialized execution behind.
  creator.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  require(
      creator.federateHandle().isValid() && member.federateHandle().isValid(),
      "valid FOM creation after empty-module rejection did not establish membership");

  member.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomModuleCompositionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomModuleComposition(options, model);
}

void scenarioFomAdditionalModuleJoinAtomicityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomAdditionalModuleJoinAtomicity(options, model);
}

void scenarioFomTransportationHandleStabilityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomTransportationHandleStability(options, model);
}

void scenarioFomDimensionHandleStabilityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomDimensionHandleStability(options, model);
}

void scenarioFomUpdateRateValueStabilityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomUpdateRateValueStability(options, model);
}


void scenarioFomStandardInvalidInputCorpusContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomStandardInvalidInputCorpus(options, model);
}

void scenarioFomInvalidCreateAtomicityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomInvalidCreateAtomicity(options, model);
}

void scenarioFomInvalidCompositeJoinAtomicityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomInvalidCompositeJoinAtomicity(options, model);
}

void scenarioFomInvalidMimCreateAtomicityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomInvalidMimCreateAtomicity(options, model);
}

void scenarioFomInvalidCompositeMimCreateAtomicityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomInvalidCompositeMimCreateAtomicity(options, model);
}

void scenarioFomEmptyMimCreateAtomicityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomEmptyMimCreateAtomicity(options, model);
}

void scenarioFomEmptyModuleValidationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomEmptyModuleValidation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
