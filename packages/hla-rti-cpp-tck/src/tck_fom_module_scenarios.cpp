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

std::filesystem::path temporaryFomPath(std::string const& label) {
  static std::uint64_t sequence = 0U;
  auto const now = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  return std::filesystem::temp_directory_path() /
      ("hla-rti-cpp-tck-" + label + "-" + std::to_string(now) + "-" +
       std::to_string(++sequence) + ".xml");
}

void scenarioMalformedInputs(Options const& options, rti::CallbackModel model) {
  Session session(options, model, "malformed-inputs");
  session.connect();
  auto const expectedFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM", L"ErrorReadingFOM", L"InvalidFOM", L"InconsistentFOM"};
  auto const missing = temporaryFomPath("missing");
  require(
      !std::filesystem::exists(missing),
      "temporary missing-FOM path unexpectedly exists");
  requireExceptionOneOf(
      [&] {
        session.rtiAmbassador().createFederationExecution(
            federationName(options, "missing-fom"),
            missing.wstring(),
            options.logicalTimeImplementationName);
      },
      expectedFomErrors,
      "missing FOM input");

  auto const malformed = temporaryFomPath("malformed");
  {
    std::ofstream output(malformed, std::ios::binary | std::ios::trunc);
    require(static_cast<bool>(output), "could not create temporary malformed FOM");
    output << "<?xml version=\"1.0\"?><objectModel xmlns=\"http://standards.ieee.org/IEEE1516-2025\"><modelIdentification>";
  }
  requireExceptionOneOf(
      [&] {
        session.rtiAmbassador().createFederationExecution(
            federationName(options, "malformed-fom"),
            malformed.wstring(),
            options.logicalTimeImplementationName);
      },
      expectedFomErrors,
      "malformed FOM input");
  std::filesystem::remove(malformed);

  for (auto const& invalid : options.invalidFomModules) {
    requireExceptionOneOf(
        [&] {
          session.rtiAmbassador().createFederationExecution(
              federationName(options, "invalid-fom"),
              invalid.wstring(),
              options.logicalTimeImplementationName);
        },
        expectedFomErrors,
        "invalid FOM input " + invalid.string());
  }

  rti::HLAinteger32BE decoded;
  std::vector<std::uint8_t> truncated{0x01U, 0x02U};
  rti::VariableLengthData truncatedData(truncated.data(), truncated.size());
  requireException(
      [&] { decoded.decode(truncatedData); },
      L"EncoderException",
      "malformed encoded input");
  session.disconnect();
}

void scenarioFomModel(Options const& options, rti::CallbackModel model) {
  require(!options.modelFom.empty(), "FOM model input was not supplied by the adapter");
  Session owner(options, model, "fom-owner");
  Session member(options, model, "fom-member");
  auto const federation = federationName(options, "fom-model");
  owner.connect();
  member.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const typedObject =
      owner.rtiAmbassador().getObjectClassHandle(options.typedObjectClassName);
  auto const typedDerivedObject =
      owner.rtiAmbassador().getObjectClassHandle(options.typedDerivedObjectClassName);
  auto const typedInteraction =
      owner.rtiAmbassador().getInteractionClassHandle(options.typedInteractionClassName);
  auto const typedDerivedInteraction =
      owner.rtiAmbassador().getInteractionClassHandle(
          options.typedDerivedInteractionClassName);
  require(typedObject.isValid() && typedDerivedObject.isValid(),
          "FOM model object class lookup returned an invalid handle");
  auto const firstQualifiedProbe = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckTypedObject.TckClassNameProbe");
  auto const secondQualifiedProbe = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckTransportRegionalObject.TckClassNameProbe");
  require(firstQualifiedProbe.isValid() && secondQualifiedProbe.isValid() &&
              firstQualifiedProbe != secondQualifiedProbe,
          "duplicate local object-class names did not resolve to distinct valid handles");
  require(owner.rtiAmbassador().getObjectClassName(firstQualifiedProbe) ==
              L"HLAobjectRoot.TckTypedObject.TckClassNameProbe" &&
              owner.rtiAmbassador().getObjectClassName(secondQualifiedProbe) ==
                  L"HLAobjectRoot.TckTransportRegionalObject.TckClassNameProbe",
          "qualified object-class paths did not round-trip through handles");
  require(typedInteraction.isValid() && typedDerivedInteraction.isValid(),
          "FOM model interaction class lookup returned an invalid handle");
  require(
      owner.rtiAmbassador().getObjectClassName(typedObject) == options.typedObjectClassName &&
          owner.rtiAmbassador().getObjectClassName(typedDerivedObject) ==
              options.typedDerivedObjectClassName,
      "FOM model object class name lookup did not round-trip");
  require(
      owner.rtiAmbassador().getInteractionClassName(typedInteraction) ==
              options.typedInteractionClassName &&
          owner.rtiAmbassador().getInteractionClassName(typedDerivedInteraction) ==
              options.typedDerivedInteractionClassName,
      "FOM model interaction class name lookup did not round-trip");

  auto const modelDimension =
      owner.rtiAmbassador().getDimensionHandle(options.fomDimensionName);
  require(modelDimension.isValid(), "FOM model dimension lookup returned an invalid handle");
  require(owner.rtiAmbassador().getDimensionName(modelDimension) == options.fomDimensionName,
          "FOM model dimension name lookup did not round-trip");
  require(owner.rtiAmbassador().getDimensionUpperBound(modelDimension) == 64U,
          "FOM model dimension upper bound did not round-trip");
  auto const objectDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(typedDerivedObject);
  require(objectDimensions.count(modelDimension) == 1U,
          "FOM model object-class dimension association was not composed");
  auto const interactionDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForInteractionClass(typedInteraction);
  require(interactionDimensions.count(modelDimension) == 1U,
          "FOM model interaction-class dimension association was not composed");

  require(owner.rtiAmbassador().getUpdateRateValue(options.fomUpdateRateName) == 25.0,
          "FOM model update-rate value did not round-trip");
  require(owner.rtiAmbassador().getUpdateRateValue(L"TckSlow") == 0.5,
          "FOM model second update-rate value did not round-trip");
  auto const customTransportation = owner.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  require(customTransportation.isValid(),
          "FOM model custom transportation lookup returned an invalid handle");
  require(owner.rtiAmbassador().getTransportationTypeName(customTransportation) ==
              options.fomCustomTransportationName,
          "FOM model custom transportation name did not round-trip");
  static_cast<void>(owner.rtiAmbassador().getAutoProvideSwitch());
  static_cast<void>(owner.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch());
  static_cast<void>(owner.rtiAmbassador().getAttributeRelevanceAdvisorySwitch());
  static_cast<void>(owner.rtiAmbassador().getInteractionRelevanceAdvisorySwitch());

  auto const publishOnlyClass =
      owner.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot.TckPublishOnlyObject");
  auto const publishOnlyAttribute =
      owner.rtiAmbassador().getAttributeHandle(publishOnlyClass, L"PublishedValue");
  require(publishOnlyClass.isValid() && publishOnlyAttribute.isValid(),
          "FOM model publication-only declaration was not loaded");

  auto const ownerIdentity = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedIdentityAttributeName);
  auto const ownerInteger = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedIntegerAttributeName);
  auto const ownerFloat = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedFloatAttributeName);
  auto const ownerAscii = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedAsciiAttributeName);
  auto const ownerUnicode = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedUnicodeAttributeName);
  auto const ownerEnum = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedEnumAttributeName);
  auto const ownerFixedArray = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedFixedArrayAttributeName);
  auto const ownerVariableArray = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedVariableArrayAttributeName);
  auto const ownerRecord = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedRecordAttributeName);
  auto const ownerChoice = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedChoiceAttributeName);
  auto const ownerReference = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedReferenceAttributeName);
  auto const ownerOpaque = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedOpaqueAttributeName);
  auto const ownerDerived = owner.rtiAmbassador().getAttributeHandle(
      typedDerivedObject, options.typedDerivedAttributeName);
  auto const ownerBaseInteger = owner.rtiAmbassador().getAttributeHandle(
      typedObject, options.typedIntegerAttributeName);
  require(ownerIdentity.isValid() && ownerInteger.isValid() && ownerFloat.isValid() &&
              ownerAscii.isValid() && ownerUnicode.isValid() && ownerEnum.isValid() &&
              ownerFixedArray.isValid() && ownerVariableArray.isValid() && ownerRecord.isValid() &&
              ownerChoice.isValid() && ownerReference.isValid() && ownerOpaque.isValid() &&
              ownerDerived.isValid(),
          "FOM model typed attribute lookup returned an invalid handle");
  require(ownerBaseInteger == ownerInteger,
          "Inherited FOM attribute lookup did not retain the base handle");

  auto const memberDerivedObject =
      member.rtiAmbassador().getObjectClassHandle(options.typedDerivedObjectClassName);
  auto const memberInteger = member.rtiAmbassador().getAttributeHandle(
      memberDerivedObject, options.typedIntegerAttributeName);
  auto const memberDerived = member.rtiAmbassador().getAttributeHandle(
      memberDerivedObject, options.typedDerivedAttributeName);
  require(memberDerivedObject.isValid() && memberInteger == ownerInteger &&
              memberDerived == ownerDerived,
          "FOM model member handles did not retain cross-federate identity");

  rti::HLAASCIIstring identity{std::string{"typed-object"}};
  rti::HLAinteger32BE integer{42};
  rti::HLAfloat64BE floating{3.5};
  rti::HLAASCIIstring ascii{std::string{"ascii"}};
  rti::HLAunicodeString unicode{L"unicode"};
  rti::HLAinteger32BE enumeration{1};
  rti::HLAfixedArrayT<rti::HLAbyte> fixedArray{3U};
  fixedArray.set(0U, rti::HLAbyte{static_cast<rti::Octet>(0x11U)});
  fixedArray.set(1U, rti::HLAbyte{static_cast<rti::Octet>(0x22U)});
  fixedArray.set(2U, rti::HLAbyte{static_cast<rti::Octet>(0x33U)});
  rti::HLAvariableArrayT<rti::HLAbyte> variableArray;
  variableArray.addElement(rti::HLAbyte{static_cast<rti::Octet>(0x44U)});
  variableArray.addElement(rti::HLAbyte{static_cast<rti::Octet>(0x55U)});
  rti::HLAfixedRecord record;
  record.appendElement(rti::HLAinteger32BE{7});
  record.appendElement(rti::HLAboolean{true});
  rti::HLAvariantRecordT<rti::HLAinteger32BE> choice;
  rti::HLAinteger32BE ready{0};
  rti::HLAinteger32BE busy{1};
  choice.addVariant(ready, rti::HLAinteger32BE{});
  choice.addVariant(busy, rti::HLAASCIIstring{});
  choice.setVariant(ready, rti::HLAinteger32BE{41});
  choice.setDiscriminant(ready);
  rti::HLAASCIIstring reference{std::string{"typed-object"}};
  std::vector<rti::Octet> opaqueBytes{
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x5aU)};
  rti::HLAopaqueData opaque(opaqueBytes.data(), opaqueBytes.size());
  std::map<std::wstring, std::vector<std::uint8_t>> valuesByName{
      {options.typedIdentityAttributeName, encodeDataElement(identity)},
      {options.typedIntegerAttributeName, encodeDataElement(integer)},
      {options.typedFloatAttributeName, encodeDataElement(floating)},
      {options.typedAsciiAttributeName, encodeDataElement(ascii)},
      {options.typedUnicodeAttributeName, encodeDataElement(unicode)},
      {options.typedEnumAttributeName, encodeDataElement(enumeration)},
      {options.typedFixedArrayAttributeName, encodeDataElement(fixedArray)},
      {options.typedVariableArrayAttributeName, encodeDataElement(variableArray)},
      {options.typedRecordAttributeName, encodeDataElement(record)},
      {options.typedChoiceAttributeName, encodeDataElement(choice)},
      {options.typedReferenceAttributeName, encodeDataElement(reference)},
      {options.typedOpaqueAttributeName, encodeDataElement(opaque)},
      {options.typedDerivedAttributeName, encodeDataElement(rti::HLAinteger32BE{99})},
  };
  std::map<std::wstring, rti::AttributeHandle> ownerAttributesByName{
      {options.typedIdentityAttributeName, ownerIdentity},
      {options.typedIntegerAttributeName, ownerInteger},
      {options.typedFloatAttributeName, ownerFloat},
      {options.typedAsciiAttributeName, ownerAscii},
      {options.typedUnicodeAttributeName, ownerUnicode},
      {options.typedEnumAttributeName, ownerEnum},
      {options.typedFixedArrayAttributeName, ownerFixedArray},
      {options.typedVariableArrayAttributeName, ownerVariableArray},
      {options.typedRecordAttributeName, ownerRecord},
      {options.typedChoiceAttributeName, ownerChoice},
      {options.typedReferenceAttributeName, ownerReference},
      {options.typedOpaqueAttributeName, ownerOpaque},
      {options.typedDerivedAttributeName, ownerDerived},
  };
  std::map<std::wstring, rti::AttributeHandle> memberAttributesByName;
  for (auto const& [name, handle] : ownerAttributesByName) {
    auto const memberHandle = member.rtiAmbassador().getAttributeHandle(
        memberDerivedObject, name);
    require(memberHandle.isValid(), "FOM model member attribute lookup failed");
    require(member.rtiAmbassador().getAttributeName(memberDerivedObject, memberHandle) == name,
            "FOM model attribute name lookup did not round-trip");
    memberAttributesByName.emplace(name, memberHandle);
  }
  rti::AttributeHandleSet ownerAttributes;
  rti::AttributeHandleSet memberAttributes;
  for (auto const& [name, handle] : ownerAttributesByName) {
    ownerAttributes.insert(handle);
    memberAttributes.insert(memberAttributesByName.at(name));
  }
  owner.rtiAmbassador().publishObjectClassAttributes(
      typedDerivedObject, ownerAttributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberDerivedObject, memberAttributes, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(typedDerivedObject);
  require(object.isValid(), "FOM model typed object registration returned an invalid handle");
  waitFor(member, [&] { return member.recorder().discovery().present; }, options,
          "FOM model typed object discovery");
  require(member.recorder().discovery().object == object,
          "FOM model typed object discovery returned the wrong handle");

  rti::AttributeHandleValueMap attributeValues;
  for (auto const& [name, handle] : ownerAttributesByName) {
    auto const& bytes = valuesByName.at(name);
    attributeValues.emplace(
        handle,
        rti::VariableLengthData(bytes.data(), bytes.size()));
  }
  std::vector<std::uint8_t> updateTagBytes{0xf0U, 0x0dU};
  rti::VariableLengthData updateTag(updateTagBytes.data(), updateTagBytes.size());
  owner.rtiAmbassador().updateAttributeValues(object, attributeValues, updateTag);
  waitFor(member, [&] { return member.recorder().reflection().present; }, options,
          "FOM model typed attribute reflection");
  auto const reflection = member.recorder().reflection();
  require(reflection.object == object && reflection.tag == updateTagBytes,
          "FOM model typed reflection returned the wrong object or tag");
  if (reflection.values.size() != valuesByName.size()) {
    std::string actualNames;
    for (auto const& [handle, value] : reflection.values) {
      if (!actualNames.empty()) {
        actualNames += ",";
      }
      actualNames += toNarrow(
          member.rtiAmbassador().getAttributeName(memberDerivedObject, handle));
    }
    throw std::runtime_error(
        "FOM model typed reflection returned " +
        std::to_string(reflection.values.size()) + " attributes instead of " +
        std::to_string(valuesByName.size()) + " (" + actualNames + ")");
  }
  for (auto const& [name, expected] : valuesByName) {
    auto const memberHandle = memberAttributesByName.at(name);
    require(reflection.values.count(memberHandle) == 1U,
            "FOM model typed reflection omitted an attribute");
    require(copyBytes(reflection.values.at(memberHandle)) == expected,
            "FOM model typed reflection changed an encoded data type");
  }

  auto const typedInteractionParameter = [&](std::wstring const& name) {
    return owner.rtiAmbassador().getParameterHandle(typedDerivedInteraction, name);
  };
  auto const ownerIntegerParameter = typedInteractionParameter(
      options.typedIntegerParameterName);
  auto const ownerAsciiParameter = typedInteractionParameter(options.typedAsciiParameterName);
  auto const ownerFixedArrayParameter = typedInteractionParameter(
      options.typedFixedArrayParameterName);
  auto const ownerRecordParameter = typedInteractionParameter(options.typedRecordParameterName);
  auto const ownerChoiceParameter = typedInteractionParameter(options.typedChoiceParameterName);
  auto const ownerDerivedParameter = typedInteractionParameter(
      options.typedDerivedParameterName);
  require(ownerIntegerParameter.isValid() && ownerAsciiParameter.isValid() &&
              ownerFixedArrayParameter.isValid() && ownerRecordParameter.isValid() &&
              ownerChoiceParameter.isValid() && ownerDerivedParameter.isValid(),
          "FOM model typed parameter lookup returned an invalid handle");
  require(owner.rtiAmbassador().getParameterHandle(
              typedInteraction, options.typedIntegerParameterName) == ownerIntegerParameter,
          "Inherited FOM parameter lookup did not retain the base handle");
  auto const memberTypedInteraction = member.rtiAmbassador().getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  auto const memberIntegerParameter = member.rtiAmbassador().getParameterHandle(
      memberTypedInteraction, options.typedIntegerParameterName);
  require(memberIntegerParameter == ownerIntegerParameter,
          "FOM model member parameter handle did not retain identity");
  require(owner.rtiAmbassador().getParameterName(
              typedDerivedInteraction, ownerDerivedParameter) ==
              options.typedDerivedParameterName,
          "FOM model parameter name lookup did not round-trip");

  rti::ParameterHandleValueMap parameterValues;
  parameterValues.emplace(ownerIntegerParameter, rti::VariableLengthData(
      valuesByName.at(options.typedIntegerAttributeName).data(),
      valuesByName.at(options.typedIntegerAttributeName).size()));
  parameterValues.emplace(ownerAsciiParameter, rti::VariableLengthData(
      valuesByName.at(options.typedAsciiAttributeName).data(),
      valuesByName.at(options.typedAsciiAttributeName).size()));
  parameterValues.emplace(ownerFixedArrayParameter, rti::VariableLengthData(
      valuesByName.at(options.typedFixedArrayAttributeName).data(),
      valuesByName.at(options.typedFixedArrayAttributeName).size()));
  parameterValues.emplace(ownerRecordParameter, rti::VariableLengthData(
      valuesByName.at(options.typedRecordAttributeName).data(),
      valuesByName.at(options.typedRecordAttributeName).size()));
  parameterValues.emplace(ownerChoiceParameter, rti::VariableLengthData(
      valuesByName.at(options.typedChoiceAttributeName).data(),
      valuesByName.at(options.typedChoiceAttributeName).size()));
  auto const derivedParameterValue = encodeDataElement(rti::HLAunicodeString{L"derived"});
  parameterValues.emplace(
      ownerDerivedParameter,
      rti::VariableLengthData(derivedParameterValue.data(), derivedParameterValue.size()));
  owner.rtiAmbassador().publishInteractionClass(typedDerivedInteraction);
  member.rtiAmbassador().subscribeInteractionClass(memberTypedInteraction, true);
  std::vector<std::uint8_t> interactionTagBytes{0xc0U, 0xdeU};
  rti::VariableLengthData interactionTag(
      interactionTagBytes.data(), interactionTagBytes.size());
  owner.rtiAmbassador().sendInteraction(
      typedDerivedInteraction, parameterValues, interactionTag);
  waitFor(member, [&] { return member.recorder().interaction().present; }, options,
          "FOM model typed interaction delivery");
  auto const interaction = member.recorder().interaction();
  require(interaction.interaction == memberTypedInteraction &&
              interaction.tag == interactionTagBytes,
          "FOM model typed interaction returned the wrong class or tag");
  require(interaction.parameters.size() == parameterValues.size(),
          "FOM model typed interaction returned the wrong parameter count");
  for (auto const& [handle, expected] : parameterValues) {
    require(interaction.parameters.count(handle) == 1U,
            "FOM model typed interaction omitted an inherited parameter");
    require(copyBytes(interaction.parameters.at(handle)) == copyBytes(expected),
            "FOM model typed interaction changed an encoded parameter");
  }

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioFomQualifiedClassNameIdentity(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.modelFom.empty(), "FOM model input was not supplied by the adapter");
  Session session(options, model, "fom-qualified-class-name");
  auto const federation = federationName(options, "fom-qualified-class-name");
  session.connect();
  session.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.ownerFederateName, options.federateType, federation);
  auto const first = session.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckTypedObject.TckClassNameProbe");
  auto const second = session.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckTransportRegionalObject.TckClassNameProbe");
  require(first.isValid() && second.isValid() && first != second,
          "duplicate local object-class names did not resolve to distinct handles");
  require(session.rtiAmbassador().getObjectClassName(first) ==
              L"HLAobjectRoot.TckTypedObject.TckClassNameProbe" &&
              session.rtiAmbassador().getObjectClassName(second) ==
                  L"HLAobjectRoot.TckTransportRegionalObject.TckClassNameProbe",
          "qualified object-class names did not round-trip through handles");
  session.resign(rti::NO_ACTION);
  session.disconnect();
}


}  // namespace hla_rti_cpp_tck::detail
