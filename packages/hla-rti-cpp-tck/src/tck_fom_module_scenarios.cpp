#include "tck_fom_module_scenarios.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

std::wstring fomModuleUrl(std::filesystem::path const& module) {
  auto const utf8 = std::filesystem::absolute(module).generic_u8string();
  std::string path;
  path.reserve(utf8.size());
  for (auto const byte : utf8) {
    path.push_back(static_cast<char>(byte));
  }
  require(!path.empty(), "FOM module URL requires a nonempty module path");

  auto const alphanumeric = [](unsigned char byte) {
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
        (byte >= '0' && byte <= '9');
  };
  std::string url = path.starts_with("//") ? "file:" : "file://";
  if (path.front() != '/') {
    url.push_back('/');
  }
  constexpr char hex[] = "0123456789ABCDEF";
  for (std::size_t index = 0; index < path.size(); ++index) {
    auto const byte = static_cast<unsigned char>(path[index]);
    auto const allowed = alphanumeric(byte) || byte == '/' || byte == ':' ||
        byte == '-' || byte == '.' || byte == '_' || byte == '~';
    if (!allowed) {
      url.push_back('%');
      url.push_back(hex[byte >> 4U]);
      url.push_back(hex[byte & 0x0FU]);
    } else {
      url.push_back(static_cast<char>(byte));
    }
  }
  return {url.begin(), url.end()};
}

std::vector<std::wstring> fomModuleUrls(
    std::vector<std::filesystem::path> const& modules) {
  std::vector<std::wstring> result;
  result.reserve(modules.size());
  for (auto const& module : modules) {
    result.push_back(fomModuleUrl(module));
  }
  return result;
}

} // namespace

void scenarioFomModuleComposition(Options const& options, rti::CallbackModel model) {
  require(!options.additionalFomModules.empty(),
          "FOM extension input was not supplied by the adapter");
  auto const baseModule = fomModuleUrl(options.fom);
  auto const extensionModules = fomModuleUrls(options.additionalFomModules);
  require(baseModule.starts_with(L"file://") &&
              std::all_of(
                  extensionModules.begin(), extensionModules.end(),
                  [](std::wstring const& module) {
                    return module.starts_with(L"file://");
                  }),
          "FOM composition inputs were not supplied as file URLs");

  Session composedOwner(options, model, "fom-composed-owner");
  Session composedMember(options, model, "fom-composed-member");
  auto const composedFederation = federationName(options, "fom-composed-create");
  composedOwner.connect();
  composedMember.connect();
  auto const verifyObjectClassTable = [](
      rti::RTIambassador& ambassador,
      std::vector<std::wstring> const& expectedClassNames,
      std::string const& context) {
    std::vector<rti::ObjectClassHandle> handles;
    handles.reserve(expectedClassNames.size());
    for (auto const& expectedName : expectedClassNames) {
      auto const handle = ambassador.getObjectClassHandle(expectedName);
      require(handle.isValid(), context + " omitted an expected object class");
      require(ambassador.getObjectClassName(handle) == expectedName,
              context + " did not round-trip an object-class path");
      for (auto const& previous : handles) {
        require(!(previous == handle),
                context + " assigned one object-class handle to distinct paths");
      }
      handles.push_back(handle);
    }
    return handles;
  };
  auto const verifyInteractionClassTable = [](
      rti::RTIambassador& ambassador,
      std::vector<std::wstring> const& expectedClassNames,
      std::string const& context) {
    std::vector<rti::InteractionClassHandle> handles;
    handles.reserve(expectedClassNames.size());
    for (auto const& expectedName : expectedClassNames) {
      auto const handle = ambassador.getInteractionClassHandle(expectedName);
      require(handle.isValid(), context + " omitted an expected interaction class");
      require(ambassador.getInteractionClassName(handle) == expectedName,
              context + " did not round-trip an interaction-class path");
      for (auto const& previous : handles) {
        require(!(previous == handle),
                context + " assigned one interaction-class handle to distinct paths");
      }
      handles.push_back(handle);
    }
    return handles;
  };
  auto const verifyAttributeTable = [](
      rti::RTIambassador& ambassador,
      std::vector<std::pair<std::wstring, std::vector<std::wstring>>> const& expectedAttributes,
      std::string const& context) {
    std::vector<rti::AttributeHandle> handles;
    for (auto const& [objectClassName, attributeNames] : expectedAttributes) {
      auto const objectClass = ambassador.getObjectClassHandle(objectClassName);
      require(objectClass.isValid(), context + " omitted an expected object class");
      for (auto const& attributeName : attributeNames) {
        auto const handle = ambassador.getAttributeHandle(objectClass, attributeName);
        require(handle.isValid(), context + " omitted an expected attribute");
        require(ambassador.getAttributeName(objectClass, handle) == attributeName,
                context + " did not round-trip an object/attribute table entry");
        handles.push_back(handle);
      }
    }
    return handles;
  };
  auto const verifyParameterTable = [](
      rti::RTIambassador& ambassador,
      std::vector<std::pair<std::wstring, std::vector<std::wstring>>> const& expectedParameters,
      std::string const& context) {
    std::vector<rti::ParameterHandle> handles;
    for (auto const& [interactionClassName, parameterNames] : expectedParameters) {
      auto const interactionClass = ambassador.getInteractionClassHandle(interactionClassName);
      require(interactionClass.isValid(), context + " omitted an expected interaction class");
      for (auto const& parameterName : parameterNames) {
        auto const handle = ambassador.getParameterHandle(interactionClass, parameterName);
        require(handle.isValid(), context + " omitted an expected parameter");
        require(ambassador.getParameterName(interactionClass, handle) == parameterName,
                context + " did not round-trip an interaction/parameter table entry");
        handles.push_back(handle);
      }
    }
    return handles;
  };
  auto const sendAndVerifyDirectedExtension = [&](Session& publisher,
                                                  Session& subscriber,
                                                  rti::InteractionClassHandle const& interaction,
                                                  rti::ObjectInstanceHandle const& object,
                                                  rti::ParameterHandle const& parameter,
                                                  std::string const& context) {
    std::vector<std::uint8_t> const valueBytes{0x4dU, 0x46U};
    std::vector<std::uint8_t> const tagBytes{0xd1U, 0x21U};
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        parameter,
        rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendDirectedInteraction(interaction, object, parameters, tag);
    waitFor(
        subscriber,
        [&] { return subscriber.recorder().directedInteractions().size() == 1U; },
        options,
        context + " delivery");
    auto const delivered = subscriber.recorder().directedInteractions();
    require(delivered.size() == 1U, context + " delivered duplicates");
    auto const& record = delivered.front();
    require(
        record.interaction == interaction && record.object == object &&
            record.parameters.size() == 1U && record.parameters.count(parameter) == 1U &&
            copyBytes(record.parameters.at(parameter)) == valueBytes && record.tag == tagBytes &&
            record.producer == publisher.federateHandle(),
        context + " did not preserve the directed FOM row's target and payload");
  };
  auto const baseObjectClassNames = std::vector<std::wstring>{
      L"HLAobjectRoot",
      L"HLAobjectRoot.TckObject",
      L"HLAobjectRoot.TckObject.TckSecondaryObject"};
  auto const completeObjectClassNames = std::vector<std::wstring>{
      L"HLAobjectRoot",
      L"HLAobjectRoot.TckObject",
      L"HLAobjectRoot.TckObject.TckSecondaryObject",
      L"HLAobjectRoot.TckExtensionObject",
      L"HLAobjectRoot.TckExtensionDimensionalObject"};
  auto const baseInteractionClassNames = std::vector<std::wstring>{
      L"HLAinteractionRoot",
      L"HLAinteractionRoot.TckInteraction",
      L"HLAinteractionRoot.TckPreferredTimestampInteraction",
      L"HLAinteractionRoot.TckPreferredReceiveInteraction",
      L"HLAinteractionRoot.TckSecondaryDirectedInteraction"};
  auto const completeInteractionClassNames = std::vector<std::wstring>{
      L"HLAinteractionRoot",
      L"HLAinteractionRoot.TckInteraction",
      L"HLAinteractionRoot.TckPreferredTimestampInteraction",
      L"HLAinteractionRoot.TckPreferredReceiveInteraction",
      L"HLAinteractionRoot.TckSecondaryDirectedInteraction",
      L"HLAinteractionRoot.TckExtensionInteraction"};
  auto const baseAttributeTable =
      std::vector<std::pair<std::wstring, std::vector<std::wstring>>>{
          {L"HLAobjectRoot.TckObject", {L"Name", L"PayRate", L"Value"}}};
  auto const completeAttributeTable =
      std::vector<std::pair<std::wstring, std::vector<std::wstring>>>{
          {L"HLAobjectRoot.TckObject", {L"Name", L"PayRate", L"Value"}},
          {L"HLAobjectRoot.TckExtensionObject", {L"ExtensionValue"}},
          {L"HLAobjectRoot.TckExtensionDimensionalObject", {L"AdditionalValue"}}};
  auto const baseParameterTable =
      std::vector<std::pair<std::wstring, std::vector<std::wstring>>>{
          {L"HLAinteractionRoot.TckInteraction", {L"Payload"}},
          {L"HLAinteractionRoot.TckPreferredTimestampInteraction", {L"Payload"}},
          {L"HLAinteractionRoot.TckPreferredReceiveInteraction", {L"Payload"}},
          {L"HLAinteractionRoot.TckSecondaryDirectedInteraction", {L"SecondaryPayload"}}};
  auto const completeParameterTable =
      std::vector<std::pair<std::wstring, std::vector<std::wstring>>>{
          {L"HLAinteractionRoot.TckInteraction", {L"Payload"}},
          {L"HLAinteractionRoot.TckPreferredTimestampInteraction", {L"Payload"}},
          {L"HLAinteractionRoot.TckPreferredReceiveInteraction", {L"Payload"}},
          {L"HLAinteractionRoot.TckSecondaryDirectedInteraction", {L"SecondaryPayload"}},
          {L"HLAinteractionRoot.TckExtensionInteraction", {L"ExtensionParameter"}}};
  std::vector<std::wstring> createModules{baseModule};
  createModules.insert(createModules.end(), extensionModules.begin(), extensionModules.end());
  composedOwner.rtiAmbassador().createFederationExecution(
      composedFederation,
      createModules,
      options.logicalTimeImplementationName);
  composedOwner.join(options.ownerFederateName, options.federateType, composedFederation);
  composedMember.join(options.memberFederateName, options.federateType, composedFederation);
  auto const composedOwnerObjectClassHandles = verifyObjectClassTable(
      composedOwner.rtiAmbassador(), completeObjectClassNames,
      "Created FOM composition owner object-class table");
  auto const composedMemberObjectClassHandles = verifyObjectClassTable(
      composedMember.rtiAmbassador(), completeObjectClassNames,
      "Created FOM composition member object-class table");
  require(composedOwnerObjectClassHandles == composedMemberObjectClassHandles,
          "Created FOM composition did not share the complete expected object-class table");
  auto const composedOwnerInteractionClassHandles = verifyInteractionClassTable(
      composedOwner.rtiAmbassador(), completeInteractionClassNames,
      "Created FOM composition owner interaction-class table");
  auto const composedMemberInteractionClassHandles = verifyInteractionClassTable(
      composedMember.rtiAmbassador(), completeInteractionClassNames,
      "Created FOM composition member interaction-class table");
  require(composedOwnerInteractionClassHandles == composedMemberInteractionClassHandles,
          "Created FOM composition did not share the expected interaction-class table");
  auto const composedOwnerAttributeHandles = verifyAttributeTable(
      composedOwner.rtiAmbassador(), completeAttributeTable,
      "Created FOM composition owner attribute table");
  auto const composedMemberAttributeHandles = verifyAttributeTable(
      composedMember.rtiAmbassador(), completeAttributeTable,
      "Created FOM composition member attribute table");
  require(composedOwnerAttributeHandles == composedMemberAttributeHandles,
          "Created FOM composition did not share the expected attribute table entries");
  auto const composedOwnerParameterHandles = verifyParameterTable(
      composedOwner.rtiAmbassador(), completeParameterTable,
      "Created FOM composition owner parameter table");
  auto const composedMemberParameterHandles = verifyParameterTable(
      composedMember.rtiAmbassador(), completeParameterTable,
      "Created FOM composition member parameter table");
  require(composedOwnerParameterHandles == composedMemberParameterHandles,
          "Created FOM composition did not share the expected parameter table entries");

  require(!composedOwner.rtiAmbassador().getAttributeScopeAdvisorySwitch() &&
              !composedMember.rtiAmbassador().getAttributeScopeAdvisorySwitch(),
          "Unspecified Attribute Scope Advisory did not default to Disabled after create-time FOM composition");

  auto const composedFederateMomClass = composedOwner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const composedFederateHandleAttribute = composedOwner.rtiAmbassador().getAttributeHandle(
      composedFederateMomClass, L"HLAfederateHandle");
  require(composedFederateMomClass.isValid() && composedFederateHandleAttribute.isValid(),
          "Created FOM composition omitted standard MIM federate declarations");
  require(composedMember.rtiAmbassador().getObjectClassHandle(
              L"HLAobjectRoot.HLAmanager.HLAfederate") == composedFederateMomClass &&
              composedMember.rtiAmbassador().getAttributeHandle(
                  composedFederateMomClass, L"HLAfederateHandle") ==
                  composedFederateHandleAttribute,
          "Created FOM composition did not share standard MIM declaration handles");

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
  rti::InteractionClassHandleSet extensionDirectedInteractions;
  extensionDirectedInteractions.insert(extensionInteraction);
  composedOwner.rtiAmbassador().publishObjectClassDirectedInteractions(
      extensionClass, extensionDirectedInteractions);
  composedMember.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      extensionClass, extensionDirectedInteractions, false);

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
              extensionBytes && composedMember.recorder().reflection().tag == extensionTagBytes &&
              composedMember.recorder().reflection().transportation.isValid() &&
              composedMember.rtiAmbassador().getTransportationTypeName(
                  composedMember.recorder().reflection().transportation) == L"HLAreliable",
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
  require(composedMember.recorder().interaction().parameters.count(extensionParameter) == 1U &&
              copyBytes(composedMember.recorder().interaction().parameters.at(extensionParameter)) ==
                  extensionBytes,
          "Created FOM module composition changed the extension interaction parameter payload");
  composedMember.rtiAmbassador().publishObjectClassAttributes(
      extensionClass, extensionAttributes);
  composedOwner.rtiAmbassador().subscribeObjectClassAttributes(
      extensionClass, extensionAttributes, true, L"");
  auto const directedExtensionObject =
      composedMember.rtiAmbassador().registerObjectInstance(extensionClass);
  waitFor(
      composedOwner,
      [&] { return composedOwner.recorder().hasDiscovery(directedExtensionObject); },
      options,
      "created-composition directed extension target discovery");
  sendAndVerifyDirectedExtension(
      composedOwner,
      composedMember,
      extensionInteraction,
      directedExtensionObject,
      extensionParameter,
      "Created-composition directed extension interaction");
  composedOwner.resign(rti::DELETE_OBJECTS);
  composedMember.resign(rti::NO_ACTION);

  Session baseOwner(options, model, "fom-base-owner");
  Session extensionJoiner(options, model, "fom-extension-joiner");
  auto const joinedFederation = federationName(options, "fom-composed-join");
  baseOwner.connect();
  extensionJoiner.connect();
  baseOwner.rtiAmbassador().createFederationExecution(
      joinedFederation,
      baseModule,
      options.logicalTimeImplementationName);
  baseOwner.join(options.ownerFederateName, options.federateType, joinedFederation);
  requireException(
      [&] {
        static_cast<void>(baseOwner.rtiAmbassador().getObjectClassHandle(
            L"HLAobjectRoot.TckExtensionObject"));
      },
      L"NameNotFound",
      "extension lookup before additional-module join");
  static_cast<void>(verifyObjectClassTable(
      baseOwner.rtiAmbassador(), baseObjectClassNames,
      "Base federation object-class table"));
  static_cast<void>(verifyInteractionClassTable(
      baseOwner.rtiAmbassador(), baseInteractionClassNames,
      "Base federation interaction-class table"));
  static_cast<void>(verifyAttributeTable(
      baseOwner.rtiAmbassador(), baseAttributeTable,
      "Base federation attribute table"));
  static_cast<void>(verifyParameterTable(
      baseOwner.rtiAmbassador(), baseParameterTable,
      "Base federation parameter table"));
  auto const baseFederateMomClass = baseOwner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const baseFederateHandleAttribute = baseOwner.rtiAmbassador().getAttributeHandle(
      baseFederateMomClass, L"HLAfederateHandle");
  require(baseFederateMomClass.isValid() && baseFederateHandleAttribute.isValid(),
          "Base federation omitted standard MIM federate declarations");
  extensionJoiner.join(
      options.memberFederateName,
      options.federateType,
      joinedFederation,
      extensionModules);
  auto const baseOwnerJoinedObjectClassHandles = verifyObjectClassTable(
      baseOwner.rtiAmbassador(), completeObjectClassNames,
      "Additional-module-joined owner object-class table");
  auto const extensionJoinerObjectClassHandles = verifyObjectClassTable(
      extensionJoiner.rtiAmbassador(), completeObjectClassNames,
      "Additional-module joiner object-class table");
  require(baseOwnerJoinedObjectClassHandles == extensionJoinerObjectClassHandles,
          "Additional-module join did not expose one complete expected object-class table to all members");
  auto const baseOwnerJoinedInteractionClassHandles = verifyInteractionClassTable(
      baseOwner.rtiAmbassador(), completeInteractionClassNames,
      "Additional-module-joined owner interaction-class table");
  auto const extensionJoinerInteractionClassHandles = verifyInteractionClassTable(
      extensionJoiner.rtiAmbassador(), completeInteractionClassNames,
      "Additional-module joiner interaction-class table");
  require(baseOwnerJoinedInteractionClassHandles == extensionJoinerInteractionClassHandles,
          "Additional-module join did not expose one expected interaction-class table to all members");
  auto const baseOwnerJoinedAttributeHandles = verifyAttributeTable(
      baseOwner.rtiAmbassador(), completeAttributeTable,
      "Additional-module-joined owner attribute table");
  auto const extensionJoinerAttributeHandles = verifyAttributeTable(
      extensionJoiner.rtiAmbassador(), completeAttributeTable,
      "Additional-module joiner attribute table");
  require(baseOwnerJoinedAttributeHandles == extensionJoinerAttributeHandles,
          "Additional-module join did not expose shared expected attribute table entries");
  auto const baseOwnerJoinedParameterHandles = verifyParameterTable(
      baseOwner.rtiAmbassador(), completeParameterTable,
      "Additional-module-joined owner parameter table");
  auto const extensionJoinerParameterHandles = verifyParameterTable(
      extensionJoiner.rtiAmbassador(), completeParameterTable,
      "Additional-module joiner parameter table");
  require(baseOwnerJoinedParameterHandles == extensionJoinerParameterHandles,
          "Additional-module join did not expose shared expected parameter table entries");
  auto const joinedExtensionClass = baseOwner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckExtensionObject");
  require(joinedExtensionClass.isValid() &&
              extensionJoiner.rtiAmbassador().getObjectClassHandle(
                  L"HLAobjectRoot.TckExtensionObject") == joinedExtensionClass,
          "Additional-module join did not compose the extension for all members");
  require(baseOwner.rtiAmbassador().getObjectClassName(joinedExtensionClass) ==
              L"HLAobjectRoot.TckExtensionObject",
          "Additional-module join extension name did not round-trip");
  require(baseOwner.rtiAmbassador().getObjectClassHandle(
              L"HLAobjectRoot.HLAmanager.HLAfederate") == baseFederateMomClass &&
              baseOwner.rtiAmbassador().getAttributeHandle(
                  baseFederateMomClass, L"HLAfederateHandle") ==
                  baseFederateHandleAttribute &&
              extensionJoiner.rtiAmbassador().getObjectClassHandle(
                  L"HLAobjectRoot.HLAmanager.HLAfederate") == baseFederateMomClass &&
              extensionJoiner.rtiAmbassador().getAttributeHandle(
                  baseFederateMomClass, L"HLAfederateHandle") ==
                  baseFederateHandleAttribute,
          "Additional-module join did not preserve standard MIM declarations for all members");
  auto const joinedExtensionAttribute = baseOwner.rtiAmbassador().getAttributeHandle(
      joinedExtensionClass, L"ExtensionValue");
  auto const joinedExtensionInteraction = baseOwner.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.TckExtensionInteraction");
  auto const joinedExtensionParameter = baseOwner.rtiAmbassador().getParameterHandle(
      joinedExtensionInteraction, L"ExtensionParameter");
  require(
      joinedExtensionAttribute.isValid() && joinedExtensionInteraction.isValid() &&
          joinedExtensionParameter.isValid() &&
          extensionJoiner.rtiAmbassador().getInteractionClassHandle(
              L"HLAinteractionRoot.TckExtensionInteraction") == joinedExtensionInteraction &&
          extensionJoiner.rtiAmbassador().getAttributeHandle(
              joinedExtensionClass, L"ExtensionValue") == joinedExtensionAttribute,
      "Additional-module join did not expose the directed FOM row's declarations to all members");
  rti::AttributeHandleSet joinedExtensionAttributes;
  joinedExtensionAttributes.insert(joinedExtensionAttribute);
  extensionJoiner.rtiAmbassador().publishObjectClassAttributes(
      joinedExtensionClass, joinedExtensionAttributes);
  baseOwner.rtiAmbassador().subscribeObjectClassAttributes(
      joinedExtensionClass, joinedExtensionAttributes, true, L"");
  rti::InteractionClassHandleSet joinedExtensionDirectedInteractions;
  joinedExtensionDirectedInteractions.insert(joinedExtensionInteraction);
  baseOwner.rtiAmbassador().publishObjectClassDirectedInteractions(
      joinedExtensionClass, joinedExtensionDirectedInteractions);
  extensionJoiner.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      joinedExtensionClass, joinedExtensionDirectedInteractions, false);
  auto const joinedExtensionObject =
      extensionJoiner.rtiAmbassador().registerObjectInstance(joinedExtensionClass);
  waitFor(
      baseOwner,
      [&] { return baseOwner.recorder().hasDiscovery(joinedExtensionObject); },
      options,
      "additional-module directed extension target discovery");
  sendAndVerifyDirectedExtension(
      baseOwner,
      extensionJoiner,
      joinedExtensionInteraction,
      joinedExtensionObject,
      joinedExtensionParameter,
      "Additional-module-joined directed extension interaction");
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
  auto invalidFomModules = options.invalidFomModules;
  invalidFomModules.insert(
      invalidFomModules.end(),
      options.standardInvalidFomModules.begin(),
      options.standardInvalidFomModules.end());
  for (auto const& invalid : invalidFomModules) {
    auto attemptedModules = fomModuleNames(options.additionalFomModules);
    attemptedModules.push_back(invalid.wstring());
    requireExceptionOneOf(
        [&] {
          member.join(
              options.memberFederateName,
              options.federateType,
              federation,
              attemptedModules);
        },
        invalidFomErrors,
        "joining with a valid extension and invalid additional FOM " + invalid.string());
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

  auto const duplicateModule = options.scenarioFoms.find(
      "additional-join-duplicate");
  require(
      duplicateModule != options.scenarioFoms.end() &&
          !duplicateModule->second.empty(),
      "additional-module duplicate-content testing requires its adapter-supplied FOM");
  auto const baseObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckObject");
  auto const baseValue = owner.rtiAmbassador().getAttributeHandle(
      baseObjectClass, L"Value");
  member.join(
      options.memberFederateName,
      options.federateType,
      federation,
      std::vector<std::wstring>{duplicateModule->second.wstring()});
  auto const duplicateObjectClass = member.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckObject");
  auto const duplicateValue = member.rtiAmbassador().getAttributeHandle(
      duplicateObjectClass, L"Value");
  require(
      baseObjectClass.isValid() && baseValue.isValid() &&
          duplicateObjectClass == baseObjectClass && duplicateValue.isValid() &&
          duplicateValue == baseValue,
      "an additional module duplicating an identical current-FDD attribute was not accepted");
  member.resign(rti::NO_ACTION);

  auto const conflictModule = options.scenarioFoms.find(
      "additional-join-conflict");
  require(
      conflictModule != options.scenarioFoms.end() &&
          !conflictModule->second.empty(),
      "additional-module conflict testing requires its adapter-supplied FOM");
  requireException(
      [&] {
        member.join(
            options.memberFederateName,
            options.federateType,
            federation,
            std::vector<std::wstring>{conflictModule->second.wstring()});
      },
      L"InconsistentFOM",
      "joining with an additional module that conflicts with the current FDD");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(
            options.memberFederateName));
      },
      L"NameNotFound",
      "looking up a member after rejected conflicting additional-module join");

  auto const interactionConflictModule = options.scenarioFoms.find(
      "additional-join-interaction-conflict");
  require(
      interactionConflictModule != options.scenarioFoms.end() &&
          !interactionConflictModule->second.empty(),
      "additional-module interaction-conflict testing requires its adapter-supplied FOM");
  requireException(
      [&] {
        member.join(
            options.memberFederateName,
            options.federateType,
            federation,
            std::vector<std::wstring>{
                interactionConflictModule->second.wstring()});
      },
      L"InconsistentFOM",
      "joining with an additional module whose interaction ordering conflicts with the current FDD");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(
            options.memberFederateName));
      },
      L"NameNotFound",
      "looking up a member after rejected interaction-conflicting additional-module join");

  auto const transportationConflictModule = options.scenarioFoms.find(
      "additional-join-transport-conflict");
  require(
      transportationConflictModule != options.scenarioFoms.end() &&
          !transportationConflictModule->second.empty(),
      "additional-module transportation-conflict testing requires its adapter-supplied FOM");
  requireException(
      [&] {
        member.join(
            options.memberFederateName,
            options.federateType,
            federation,
            std::vector<std::wstring>{
                transportationConflictModule->second.wstring()});
      },
      L"InconsistentFOM",
      "joining with an additional module whose attribute transportation conflicts with the current FDD");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(
            options.memberFederateName));
      },
      L"NameNotFound",
      "looking up a member after rejected transportation-conflicting additional-module join");

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

  auto const dimensionBaseFom = options.scenarioFoms.find(
      "dimension-duplicate-base");
  auto const conflictingDimensionFom = options.scenarioFoms.find(
      "dimension-duplicate-conflicting-upper-bound");
  require(
      dimensionBaseFom != options.scenarioFoms.end() &&
          !dimensionBaseFom->second.empty() &&
          conflictingDimensionFom != options.scenarioFoms.end() &&
          !conflictingDimensionFom->second.empty(),
      "additional-module dimension-conflict testing requires its adapter-supplied FOM pair");

  Session dimensionOwner(options, model, "dimension-owner");
  Session dimensionJoiner(options, model, "dimension-joiner");
  auto const dimensionFederation =
      federationName(options, "fom-additional-dimension-join-conflict");
  dimensionOwner.connect();
  dimensionJoiner.connect();
  dimensionOwner.rtiAmbassador().createFederationExecution(
      dimensionFederation,
      dimensionBaseFom->second.wstring(),
      options.logicalTimeImplementationName);
  dimensionOwner.join(
      options.ownerFederateName,
      options.federateType,
      dimensionFederation);
  auto const currentDimension = dimensionOwner.rtiAmbassador().getDimensionHandle(
      L"TckDuplicateDimension");
  require(
      currentDimension.isValid() &&
          dimensionOwner.rtiAmbassador().getDimensionUpperBound(currentDimension) ==
              100UL,
      "dimension-conflict base FOM did not expose its current dimension bound");
  requireException(
      [&] {
        dimensionJoiner.join(
            options.memberFederateName,
            options.federateType,
            dimensionFederation,
            std::vector<std::wstring>{conflictingDimensionFom->second.wstring()});
      },
      L"InconsistentFOM",
      "joining with an additional module whose dimension upper bound conflicts with the current FDD");
  requireException(
      [&] {
        static_cast<void>(dimensionOwner.rtiAmbassador().getFederateHandle(
            options.memberFederateName));
      },
      L"NameNotFound",
      "looking up a member after rejected dimension-conflicting additional-module join");
  require(
      dimensionOwner.rtiAmbassador().getDimensionUpperBound(currentDimension) == 100UL,
      "rejected dimension-conflicting join changed the current FDD dimension bound");
  dimensionOwner.resign(rti::NO_ACTION);
  dimensionOwner.rtiAmbassador().destroyFederationExecution(dimensionFederation);
  dimensionJoiner.disconnect();
  dimensionOwner.disconnect();
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
  auto const bestEffortBefore = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  auto const customBefore = owner.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  require(
      reliableBefore.isValid() && bestEffortBefore.isValid() && customBefore.isValid() &&
          reliableBefore != bestEffortBefore && reliableBefore != customBefore &&
          bestEffortBefore != customBefore,
      "rich adapter FOM did not expose distinct standard and custom transportation handles");
  require(
      owner.rtiAmbassador().getTransportationTypeName(reliableBefore) == L"HLAreliable" &&
          owner.rtiAmbassador().getTransportationTypeName(bestEffortBefore) ==
              L"HLAbestEffort" &&
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
  auto const ownerBestEffortAfter = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  auto const ownerCustomAfter = owner.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  auto const ownerAdditional = owner.rtiAmbassador().getTransportationTypeHandle(
      options.fomAdditionalTransportationName);
  auto const extensionReliable = extension.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const extensionBestEffort = extension.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  auto const extensionCustom = extension.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  auto const extensionAdditional = extension.rtiAmbassador().getTransportationTypeHandle(
      options.fomAdditionalTransportationName);
  require(
      ownerReliableAfter == reliableBefore && ownerBestEffortAfter == bestEffortBefore &&
          ownerCustomAfter == customBefore && extensionReliable == reliableBefore &&
          extensionBestEffort == bestEffortBefore && extensionCustom == customBefore,
      "additional-module join renumbered a pre-existing transportation handle");
  require(
      owner.rtiAmbassador().getTransportationTypeName(ownerReliableAfter) == L"HLAreliable" &&
          owner.rtiAmbassador().getTransportationTypeName(ownerBestEffortAfter) ==
              L"HLAbestEffort" &&
          owner.rtiAmbassador().getTransportationTypeName(ownerCustomAfter) ==
              options.fomCustomTransportationName &&
          extension.rtiAmbassador().getTransportationTypeName(extensionReliable) ==
              L"HLAreliable" &&
          extension.rtiAmbassador().getTransportationTypeName(extensionBestEffort) ==
              L"HLAbestEffort" &&
          extension.rtiAmbassador().getTransportationTypeName(extensionCustom) ==
              options.fomCustomTransportationName,
      "additional-module join changed a base transportation name entry");
  require(
      ownerAdditional.isValid() && extensionAdditional.isValid() &&
          ownerAdditional == extensionAdditional && ownerAdditional != customBefore &&
          ownerAdditional != reliableBefore && ownerAdditional != bestEffortBefore &&
          extensionAdditional != extensionReliable &&
          extensionAdditional != extensionBestEffort &&
          extensionAdditional != extensionCustom,
      "additional-module transportation did not receive one shared handle unique among all standard and custom transportation types");
  require(
      owner.rtiAmbassador().getTransportationTypeName(ownerAdditional) ==
              options.fomAdditionalTransportationName &&
          extension.rtiAmbassador().getTransportationTypeName(extensionAdditional) ==
              options.fomAdditionalTransportationName,
      "additional-module transportation handle did not round-trip to its name entry");

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

  auto const verifyDimensionTable = [](
      rti::RTIambassador& ambassador,
      std::vector<std::pair<std::wstring, unsigned long>> const& expectedDimensions,
      std::string const& context) {
    std::vector<rti::DimensionHandle> handles;
    handles.reserve(expectedDimensions.size());
    for (auto const& [expectedName, expectedUpperBound] : expectedDimensions) {
      auto const handle = ambassador.getDimensionHandle(expectedName);
      require(handle.isValid(), context + " omitted an expected dimension");
      require(ambassador.getDimensionName(handle) == expectedName,
              context + " did not round-trip a dimension name");
      require(ambassador.getDimensionUpperBound(handle) == expectedUpperBound,
              context + " did not retain the fixture dimension upper bound");
      for (auto const& previous : handles) {
        require(!(previous == handle), context + " assigned one handle to distinct dimensions");
      }
      handles.push_back(handle);
    }
    return handles;
  };
  auto const baseDimensionTable =
      std::vector<std::pair<std::wstring, unsigned long>>{
          {options.fomDimensionName, 64UL},
          {L"TckDimensionX", 10UL},
          {L"TckDimensionY", 10UL}};
  auto const baseDimensionHandles = verifyDimensionTable(
      owner.rtiAmbassador(), baseDimensionTable, "Rich adapter FOM base dimension table");
  auto const baseDimension = baseDimensionHandles.front();

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

  auto completeDimensionTable = baseDimensionTable;
  completeDimensionTable.emplace_back(options.fomAdditionalDimensionName, 128UL);
  auto const ownerCompleteDimensionHandles = verifyDimensionTable(
      owner.rtiAmbassador(), completeDimensionTable,
      "Additional-module-joined owner dimension table");
  auto const extensionCompleteDimensionHandles = verifyDimensionTable(
      extension.rtiAmbassador(), completeDimensionTable,
      "Additional-module joiner dimension table");
  require(
      ownerCompleteDimensionHandles == extensionCompleteDimensionHandles,
      "additional-module join did not expose one shared expected dimension table");
  for (std::size_t index = 0U; index < baseDimensionHandles.size(); ++index) {
    require(ownerCompleteDimensionHandles[index] == baseDimensionHandles[index],
            "additional-module join renumbered a pre-existing dimension handle");
  }
  auto const ownerAdditional = ownerCompleteDimensionHandles.back();
  auto const extensionAdditional = extensionCompleteDimensionHandles.back();
  require(ownerAdditional != baseDimension,
          "additional-module dimension reused an existing base dimension handle");

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
  auto const slowBaseRate = owner.rtiAmbassador().getUpdateRateValue(L"TckSlow");
  require(
      baseRate == 25.0 && slowBaseRate == 0.5,
      "rich adapter FOM did not retain the selected base update-rate table values");
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
  auto const ownerSlowAfter = owner.rtiAmbassador().getUpdateRateValue(L"TckSlow");
  auto const extensionSlow = extension.rtiAmbassador().getUpdateRateValue(L"TckSlow");
  auto const ownerAdditional = owner.rtiAmbassador().getUpdateRateValue(
      options.fomAdditionalUpdateRateName);
  auto const extensionAdditional = extension.rtiAmbassador().getUpdateRateValue(
      options.fomAdditionalUpdateRateName);
  require(
      ownerBaseAfter == 25.0 && extensionBase == 25.0 && ownerBaseAfter == baseRate &&
          ownerSlowAfter == 0.5 && extensionSlow == 0.5 && ownerSlowAfter == slowBaseRate,
      "additional-module join changed a selected base update-rate table entry");
  require(
      ownerAdditional == 2.0 && extensionAdditional == 2.0 &&
          extensionAdditional == ownerAdditional,
      "additional-module update-rate table entry did not retain its exact value across members");

  extension.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  owner.disconnect();
}

void scenarioFomBasicDataRepresentationEndianValues(
    Options const& options,
    rti::CallbackModel model) {
  Session creator(options, model, "fom-basic-data-endian-creator");
  creator.connect();

  for (auto const* fixtureName : {
           "basic-data-representation-endian-big",
           "basic-data-representation-endian-little"}) {
    auto const fixture = options.scenarioFoms.find(fixtureName);
    require(
        fixture != options.scenarioFoms.end(),
        std::string("Basic-data Endian acceptance requires adapter fixture ") +
            fixtureName);
    auto const federation = federationName(options, fixtureName);
    creator.rtiAmbassador().createFederationExecution(
        federation,
        fixture->second.wstring(),
        options.logicalTimeImplementationName);
    creator.join(options.ownerFederateName, options.federateType, federation);
    require(
        creator.federateHandle().isValid(),
        std::string("valid Endian fixture did not establish membership: ") +
            fixtureName);
    creator.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }

  creator.disconnect();
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
  require(
      !options.schemaValidFom.empty() && !options.schemaInvalidFom.empty(),
      "Annex F FOM testing requires adapter-supplied XSD-valid and XSD-invalid FDDs");
  require(
      !options.invalidFomModules.empty(),
      "FOM validity testing requires adapter-supplied malformed or invalid modules");

  Session creator(options, model, "fom-standard-invalid-creator");
  creator.connect();
  auto const invalidFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM",
      L"ErrorReadingFOM",
      L"InvalidFOM",
      L"InconsistentFOM"};

  auto const schemaFederation = federationName(options, "fom-schema-validity");
  requireExceptionOneOf(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            schemaFederation,
            options.schemaInvalidFom.wstring(),
            options.logicalTimeImplementationName);
      },
      invalidFomErrors,
      "rejecting correctly namespaced FDD with an XSD-invalid reliable value");
  creator.rtiAmbassador().createFederationExecution(
      schemaFederation,
      options.schemaValidFom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, schemaFederation);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(schemaFederation);

  auto invalidInputModules = options.invalidFomModules;
  invalidInputModules.insert(
      invalidInputModules.end(),
      options.standardInvalidFomModules.begin(),
      options.standardInvalidFomModules.end());
  for (std::size_t index = 0; index < invalidInputModules.size(); ++index) {
    auto const& invalid = invalidInputModules.at(index);
    if (invalid == options.schemaInvalidFom) {
      continue;
    }
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
  // IEEE 1516.1-2025 §4.5.5 also requires a nonempty FOM module set when a
  // valid MIM is supplied to the With-MIM overload.
  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecutionWithMIM(
            federation,
            std::vector<std::wstring>{},
            options.mimFom.wstring(),
            options.logicalTimeImplementationName);
      },
      L"InvalidFOM",
      "creating a federation with a MIM but no FOM modules");

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

void scenarioFomEmptyDomainTablesAccepted(
    Options const& options,
    rti::CallbackModel model) {
  auto const fixture = options.scenarioFoms.find("empty-domain-tables");
  require(
      fixture != options.scenarioFoms.end() && !fixture->second.empty(),
      "Empty-domain-content testing requires an adapter-supplied 2025 FOM");

  Session creator(options, model, "fom-empty-domain-creator");
  Session member(options, model, "fom-empty-domain-member");
  auto const federation = federationName(options, "fom-empty-domain-tables");
  creator.connect();
  member.connect();
  creator.rtiAmbassador().createFederationExecution(
      federation,
      fixture->second.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const objectRoot =
      creator.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot");
  auto const federationMom = creator.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const interactionRoot =
      creator.rtiAmbassador().getInteractionClassHandle(L"HLAinteractionRoot");
  require(
      creator.federateHandle().isValid() && member.federateHandle().isValid() &&
          objectRoot.isValid() && federationMom.isValid() &&
          interactionRoot.isValid() &&
          member.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot") == objectRoot &&
          member.rtiAmbassador().getObjectClassHandle(
              L"HLAobjectRoot.HLAmanager.HLAfederation") == federationMom &&
          member.rtiAmbassador().getInteractionClassHandle(L"HLAinteractionRoot") ==
              interactionRoot,
      "The root-only FOM did not create a shared model with the required federation MOM");

  member.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioMomTableClassStructure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "MOM table class lookup requires an adapter-supplied 2025 FOM");

  Session creator(options, model, "mom-table-class-creator");
  Session member(options, model, "mom-table-class-member");
  auto const federation = federationName(options, "mom-table-class-structure");
  creator.connect();
  member.connect();
  creator.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);

  // IEEE 1516.1-2025 §11.6 Tables 6 and 7 define these MOM class paths.
  // Probe the public handle/name API from both joined federates so an
  // application FOM with empty domain tables still exposes the standard MOM.
  std::vector<std::wstring> const objectClassNames{
      L"HLAobjectRoot",
      L"HLAobjectRoot.HLAmanager",
      L"HLAobjectRoot.HLAmanager.HLAfederate",
      L"HLAobjectRoot.HLAmanager.HLAfederation",
  };
  std::vector<std::wstring> const interactionClassNames{
      L"HLAinteractionRoot",
      L"HLAinteractionRoot.HLAmanager",
      L"HLAinteractionRoot.HLAmanager.HLAfederate",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation",
  };

  auto const verifyObjectClasses = [&](rti::RTIambassador& ambassador) {
    for (auto const& name : objectClassNames) {
      auto const handle = ambassador.getObjectClassHandle(name);
      require(handle.isValid(),
              "A §11.6 Table 6 MOM object class is not available");
      require(ambassador.getObjectClassName(handle) == name,
              "A §11.6 Table 6 MOM object class name did not round-trip");
      require(creator.rtiAmbassador().getObjectClassHandle(name) == handle,
              "MOM object class handles differ across joined federates");
    }
  };
  auto const verifyInteractionClasses = [&](rti::RTIambassador& ambassador) {
    for (auto const& name : interactionClassNames) {
      auto const handle = ambassador.getInteractionClassHandle(name);
      require(handle.isValid(),
              "A §11.6 Table 7 MOM interaction class is not available");
      require(ambassador.getInteractionClassName(handle) == name,
              "A §11.6 Table 7 MOM interaction class name did not round-trip");
      require(creator.rtiAmbassador().getInteractionClassHandle(name) == handle,
              "MOM interaction class handles differ across joined federates");
    }
  };
  verifyObjectClasses(creator.rtiAmbassador());
  verifyObjectClasses(member.rtiAmbassador());
  verifyInteractionClasses(creator.rtiAmbassador());
  verifyInteractionClasses(member.rtiAmbassador());

  member.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioMomTableInformationNotAltered(
    Options const& options,
    rti::CallbackModel model) {
  auto const conflict =
      options.scenarioFoms.find("mom-table-attribute-transportation-conflict");
  require(
      conflict != options.scenarioFoms.end() && !conflict->second.empty(),
      "MOM table alteration testing requires its adapter-supplied FOM module");
  require(!options.mimFom.empty() && !options.fom.empty(),
          "MOM table alteration testing requires the official MIM and a valid 2025 FOM");

  Session creator(options, model, "mom-table-integrity-creator");
  auto const federation = federationName(options, "mom-table-information-not-altered");
  creator.connect();
  auto const createWithMim = [&](std::filesystem::path const& fom) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  };

  requireException(
      [&] { createWithMim(conflict->second); },
      L"InconsistentFOM",
      "altering HLAfederate.HLAfederateName transportation from HLAreliable");
  createWithMim(options.fom);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomEmptyDomainTablesAcceptedContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomEmptyDomainTablesAccepted(options, model);
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


void scenarioFomBasicDataRepresentationEndianValuesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomBasicDataRepresentationEndianValues(options, model);
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

void scenarioFomDuplicateObjectClassAttributeName(
    Options const& options,
    rti::CallbackModel model) {
  auto const duplicateAttributeFom = std::find_if(
      options.invalidFomModules.begin(),
      options.invalidFomModules.end(),
      [](std::filesystem::path const& path) {
        return path.filename().wstring() == L"invalid-duplicate-tck.xml";
      });
  require(
      duplicateAttributeFom != options.invalidFomModules.end(),
      "adapter did not supply the duplicate-object-attribute FOM fixture");
  require(
      !options.logicalTimeImplementationName.empty(),
      "duplicate-object-attribute FOM check requires an adapter logical-time name");

  Session session(options, model, "fom-duplicate-object-class-attribute-name");
  session.connect();
  auto const expectedFomErrors = std::vector<std::wstring>{
      L"CouldNotOpenFOM", L"ErrorReadingFOM", L"InvalidFOM", L"InconsistentFOM"};
  requireExceptionOneOf(
      [&] {
        session.rtiAmbassador().createFederationExecution(
            federationName(options, "fom-duplicate-object-class-attribute-name"),
            duplicateAttributeFom->wstring(),
            options.logicalTimeImplementationName);
      },
      expectedFomErrors,
      "FOM that repeats an attribute name within one object class");
  session.disconnect();
}

void scenarioFomModel(Options const& options, rti::CallbackModel model) {
  require(!options.modelFom.empty(), "FOM model input was not supplied by the adapter");
  auto const caseVariant = [](std::wstring const& name) {
    auto result = name;
    auto const separator = result.find_last_of(L'.');
    auto const begin = separator == std::wstring::npos ? 0U : separator + 1U;
    for (auto index = begin; index < result.size(); ++index) {
      auto& character = result[index];
      if (character >= L'A' && character <= L'Z') {
        character = static_cast<wchar_t>(character - L'A' + L'a');
        return result;
      }
      if (character >= L'a' && character <= L'z') {
        character = static_cast<wchar_t>(character - L'a' + L'A');
        return result;
      }
    }
    throw std::runtime_error(
        "case-sensitivity probe requires an ASCII letter in the selected FOM name");
  };
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

  auto const ownerMimFederateClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const memberMimFederateClass = member.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  require(ownerMimFederateClass.isValid() &&
              memberMimFederateClass == ownerMimFederateClass &&
              owner.rtiAmbassador().getObjectClassName(ownerMimFederateClass) ==
                  L"HLAobjectRoot.HLAmanager.HLAfederate",
          "MIM-defined federate MOM class was not visible in the composed model");

  auto const typedObject =
      owner.rtiAmbassador().getObjectClassHandle(options.typedObjectClassName);
  auto const typedDerivedObject =
      owner.rtiAmbassador().getObjectClassHandle(options.typedDerivedObjectClassName);
  auto const memberTypedObject =
      member.rtiAmbassador().getObjectClassHandle(options.typedObjectClassName);
  auto const typedInteraction =
      owner.rtiAmbassador().getInteractionClassHandle(options.typedInteractionClassName);
  auto const typedDerivedInteraction =
      owner.rtiAmbassador().getInteractionClassHandle(
          options.typedDerivedInteractionClassName);
  require(typedObject.isValid() && typedDerivedObject.isValid() &&
              memberTypedObject == typedObject &&
              member.rtiAmbassador().getObjectClassName(memberTypedObject) ==
                  options.typedObjectClassName,
          "FOM-defined object class was not visible with a stable name through both joined federates");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
            caseVariant(options.typedObjectClassName)));
      },
      L"NameNotFound",
      "wrong-case FOM object-class lookup");
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
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getInteractionClassHandle(
            caseVariant(options.typedInteractionClassName)));
      },
      L"NameNotFound",
      "wrong-case FOM interaction-class lookup");
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
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionHandle(
            caseVariant(options.fomDimensionName)));
      },
      L"NameNotFound",
      "wrong-case FOM dimension lookup");
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
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getUpdateRateValue(
            caseVariant(options.fomUpdateRateName)));
      },
      L"InvalidUpdateRateDesignator",
      "wrong-case FOM update-rate designator lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeHandle(
            caseVariant(options.fomCustomTransportationName)));
      },
      L"InvalidTransportationName",
      "wrong-case FOM transportation-type lookup");
  require(owner.rtiAmbassador().getTransportationTypeName(customTransportation) ==
              options.fomCustomTransportationName,
          "FOM model custom transportation name did not round-trip");
  require(
      !owner.rtiAmbassador().getAutoProvideSwitch() &&
          !owner.rtiAmbassador().getAttributeRelevanceAdvisorySwitch() &&
          !owner.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch() &&
          !owner.rtiAmbassador().getInteractionRelevanceAdvisorySwitch() &&
          !owner.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() &&
          !owner.rtiAmbassador().getNonRegulatedGrantSwitch() &&
          !owner.rtiAmbassador().getAllowRelaxedDDMSwitch() &&
          !owner.rtiAmbassador().getAdvisoriesUseKnownClassSwitch() &&
          !member.rtiAmbassador().getAutoProvideSwitch() &&
          !member.rtiAmbassador().getAttributeRelevanceAdvisorySwitch() &&
          !member.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch() &&
          !member.rtiAmbassador().getInteractionRelevanceAdvisorySwitch() &&
          !member.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() &&
          !member.rtiAmbassador().getNonRegulatedGrantSwitch() &&
          !member.rtiAmbassador().getAllowRelaxedDDMSwitch() &&
          !member.rtiAmbassador().getAdvisoriesUseKnownClassSwitch(),
      "FOM model switches table did not retain all eight disabled entries for both members");

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
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeHandle(
            typedDerivedObject,
            caseVariant(options.typedIdentityAttributeName)));
      },
      L"NameNotFound",
      "wrong-case FOM attribute lookup");
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
  rti::AttributeHandleSet attributeDefinitionRowHandles;
  for (auto const& [name, handle] : ownerAttributesByName) {
    require(owner.rtiAmbassador().getAttributeName(typedDerivedObject, handle) == name,
            "FOM attribute-definition row did not retain its attribute name");
    attributeDefinitionRowHandles.insert(handle);
    auto const memberHandle = member.rtiAmbassador().getAttributeHandle(
        memberDerivedObject, name);
    require(memberHandle.isValid(), "FOM model member attribute lookup failed");
    require(member.rtiAmbassador().getAttributeName(memberDerivedObject, memberHandle) == name,
            "FOM model attribute name lookup did not round-trip");
    memberAttributesByName.emplace(name, memberHandle);
  }
  require(attributeDefinitionRowHandles.size() == ownerAttributesByName.size(),
          "FOM attribute-definition rows did not resolve to distinct public handles");
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
  rti::HLAfixedArrayT<rti::HLAbyte> receivedFixedArray{3U};
  receivedFixedArray.decode(reflection.values.at(
      memberAttributesByName.at(options.typedFixedArrayAttributeName)));
  require(
      receivedFixedArray.size() == 3U &&
          receivedFixedArray[0U].get() == static_cast<rti::Octet>(0x11U) &&
          receivedFixedArray[1U].get() == static_cast<rti::Octet>(0x22U) &&
          receivedFixedArray[2U].get() == static_cast<rti::Octet>(0x33U),
      "FOM model fixed-array reflection changed the encoded element sequence");
  rti::HLAvariableArrayT<rti::HLAbyte> receivedVariableArray;
  receivedVariableArray.decode(reflection.values.at(
      memberAttributesByName.at(options.typedVariableArrayAttributeName)));
  require(
      receivedVariableArray.size() == 2U &&
          receivedVariableArray[0U].get() == static_cast<rti::Octet>(0x44U) &&
          receivedVariableArray[1U].get() == static_cast<rti::Octet>(0x55U),
      "FOM model variable-array reflection changed the encoded element sequence");

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
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            typedDerivedInteraction,
            caseVariant(options.typedIntegerParameterName)));
      },
      L"NameNotFound",
      "wrong-case FOM parameter lookup");
  require(owner.rtiAmbassador().getParameterHandle(
              typedInteraction, options.typedIntegerParameterName) == ownerIntegerParameter,
          "Inherited FOM parameter lookup did not retain the base handle");
  std::map<std::wstring, rti::ParameterHandle> ownerParametersByName{
      {options.typedIntegerParameterName, ownerIntegerParameter},
      {options.typedAsciiParameterName, ownerAsciiParameter},
      {options.typedFixedArrayParameterName, ownerFixedArrayParameter},
      {options.typedRecordParameterName, ownerRecordParameter},
      {options.typedChoiceParameterName, ownerChoiceParameter},
      {options.typedDerivedParameterName, ownerDerivedParameter},
  };
  rti::ParameterHandleSet parameterDefinitionRowHandles;
  for (auto const& [name, handle] : ownerParametersByName) {
    require(owner.rtiAmbassador().getParameterName(typedDerivedInteraction, handle) == name,
            "FOM parameter-definition row did not retain its parameter name");
    parameterDefinitionRowHandles.insert(handle);
  }
  require(parameterDefinitionRowHandles.size() == ownerParametersByName.size(),
          "FOM parameter-definition rows did not resolve to distinct public handles");
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
  require(parameterValues.size() == ownerParametersByName.size(),
          "FOM parameter-definition rows did not produce separate interaction values");
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
  auto const firstName =
      std::wstring(L"HLAobjectRoot.TckTypedObject.TckClassNameProbe");
  auto const secondName = std::wstring(
      L"HLAobjectRoot.TckTransportRegionalObject.TckClassNameProbe");
  auto const firstLocalName = firstName.substr(firstName.rfind(L'.') + 1U);
  auto const secondLocalName = secondName.substr(secondName.rfind(L'.') + 1U);
  require(firstLocalName == secondLocalName,
          "qualified-name fixture must repeat a local class name under distinct parents");
  auto const first = session.rtiAmbassador().getObjectClassHandle(firstName);
  auto const second = session.rtiAmbassador().getObjectClassHandle(secondName);
  require(first.isValid() && second.isValid() && first != second,
          "duplicate local object-class names did not resolve to distinct handles");
  require(session.rtiAmbassador().getObjectClassName(first) ==
              firstName &&
              session.rtiAmbassador().getObjectClassName(second) ==
                  secondName,
          "qualified object-class names did not round-trip through handles");
  session.resign(rti::NO_ACTION);
  session.disconnect();
}

void scenarioFomInteractionClassHierarchy(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.modelFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "FOM interaction-class hierarchy requires an adapter model FOM and logical time");
  Session session(options, model, "fom-interaction-class-hierarchy");
  auto const federation = federationName(options, "fom-interaction-class-hierarchy");
  session.connect();
  session.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.ownerFederateName, options.federateType, federation);

  auto& rti = session.rtiAmbassador();
  auto const rootName = std::wstring{L"HLAinteractionRoot"};
  auto const baseName = std::wstring{L"HLAinteractionRoot.TckTypedInteraction"};
  auto const derivedName = std::wstring{
      L"HLAinteractionRoot.TckTypedInteraction.TckTypedDerivedInteraction"};
  auto const grandchildName = std::wstring{
      L"HLAinteractionRoot.TckTypedInteraction.TckTypedDerivedInteraction.TckTypedGrandchildInteraction"};
  auto const root = rti.getInteractionClassHandle(rootName);
  auto const base = rti.getInteractionClassHandle(baseName);
  auto const derived = rti.getInteractionClassHandle(derivedName);
  auto const grandchild = rti.getInteractionClassHandle(grandchildName);
  require(root.isValid() && base.isValid() && derived.isValid() &&
              grandchild.isValid() && root != base && base != derived &&
              derived != grandchild,
          "FOM interaction-class hierarchy did not resolve to distinct public handles");
  require(rti.getInteractionClassName(root) == rootName &&
              rti.getInteractionClassName(base) == baseName &&
              rti.getInteractionClassName(derived) == derivedName &&
              rti.getInteractionClassName(grandchild) == grandchildName,
          "FOM interaction-class hierarchy paths did not round-trip through handles");
  require(derivedName.rfind(baseName + L".", 0U) == 0U &&
              grandchildName.rfind(derivedName + L".", 0U) == 0U,
          "FOM interaction names did not preserve their superclass-qualified chain");

  auto const baseParameterName = options.typedIntegerParameterName;
  auto const derivedParameterName = options.typedDerivedParameterName;
  auto const baseParameter = rti.getParameterHandle(base, baseParameterName);
  auto const derivedParameter =
      rti.getParameterHandle(derived, derivedParameterName);
  require(baseParameter.isValid() && derivedParameter.isValid() &&
              baseParameter != derivedParameter,
          "FOM interaction parameter rows did not resolve to distinct handles");
  require(rti.getParameterHandle(derived, baseParameterName) == baseParameter &&
              rti.getParameterHandle(grandchild, baseParameterName) ==
                  baseParameter &&
              rti.getParameterHandle(grandchild, derivedParameterName) ==
                  derivedParameter,
          "FOM interaction parameter hierarchy did not preserve inherited parameter identity");
  require(rti.getParameterName(base, baseParameter) == baseParameterName &&
              rti.getParameterName(derived, baseParameter) == baseParameterName &&
              rti.getParameterName(derived, derivedParameter) ==
                  derivedParameterName &&
              rti.getParameterName(grandchild, baseParameter) ==
                  baseParameterName &&
              rti.getParameterName(grandchild, derivedParameter) ==
                  derivedParameterName,
          "FOM interaction parameter names did not round-trip across their qualified class paths");

  session.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);
  session.disconnect();
}

void scenarioFomInteractionQualifiedClassNameIdentity(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.modelFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "qualified interaction-class names require an adapter model FOM and logical time");
  Session session(options, model, "fom-interaction-qualified-class-name");
  auto const federation =
      federationName(options, "fom-interaction-qualified-class-name");
  session.connect();
  session.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.ownerFederateName, options.federateType, federation);

  auto& rti = session.rtiAmbassador();
  auto const firstName = std::wstring{
      L"HLAinteractionRoot.TckTypedInteraction.TckClassNameProbe"};
  auto const secondName = std::wstring{
      L"HLAinteractionRoot.TckTransportRegionalInteraction.TckClassNameProbe"};
  auto const firstLocalName = firstName.substr(firstName.rfind(L'.') + 1U);
  auto const secondLocalName = secondName.substr(secondName.rfind(L'.') + 1U);
  require(firstLocalName == secondLocalName,
          "interaction-name fixture must repeat a local class name under distinct parents");

  auto const first = rti.getInteractionClassHandle(firstName);
  auto const second = rti.getInteractionClassHandle(secondName);
  require(first.isValid() && second.isValid() && first != second,
          "duplicate local interaction-class names did not resolve to distinct handles");
  require(rti.getInteractionClassName(first) == firstName &&
              rti.getInteractionClassName(second) == secondName,
          "qualified interaction-class names did not round-trip through handles");

  session.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);
  session.disconnect();
}


void scenarioFomObjectClassNameConventions(Options const& options,
                                           rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Object-class name conventions require adapter-supplied MIM and logical time");
  auto const requiredFom = [&](std::string const& key)
      -> std::filesystem::path const& {
    auto const found = options.scenarioFoms.find(key);
    require(found != options.scenarioFoms.end(),
            "Object-class name conventions require adapter-supplied FOM modules");
    return found->second;
  };
  auto const& period = requiredFom("object-class-name-period");
  auto const& reservedHla = requiredFom("object-class-name-reserved-hla");
  auto const& reservedNa = requiredFom("object-class-name-reserved-na");
  auto const& allowed = requiredFom("object-class-name-allowed-punctuation");
  Session creator(options, model, "object-class-name-creator");
  Session member(options, model, "object-class-name-member");
  auto const federation = federationName(options, "fom-object-class-name-conventions");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(period); }, L"InconsistentFOM",
                   "a period in an object-class local name");
  requireException([&] { createWithMim(reservedHla); }, L"InconsistentFOM",
                   "an unknown HLA-prefixed object-class name");
  requireException([&] { createWithMim(reservedNa); }, L"InconsistentFOM",
                   "the reserved NA marker as an object-class name");

  // The accepted form also confirms failed Creates left no federation behind.
  createWithMim(allowed);
  member.join(options.memberFederateName, options.federateType, federation);
  auto& rti = member.rtiAmbassador();
  auto const name = std::wstring(L"HLAobjectRoot.Customer-2_0");
  auto const objectClass = rti.getObjectClassHandle(name);
  require(objectClass.isValid() && rti.getObjectClassName(objectClass) == name,
          "an accepted object-class name did not round-trip publicly");
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomObjectClassNameConventionsContract(Options const& options,
                                                   rti::CallbackModel model) {
  scenarioFomObjectClassNameConventions(options, model);
}

void scenarioFomModelModificationDate(Options const& options,
                                      rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Model modification date requires adapter-supplied MIM and logical time");
  auto const requiredFom = [&](std::string const& key)
      -> std::filesystem::path const& {
    auto const found = options.scenarioFoms.find(key);
    require(found != options.scenarioFoms.end(),
            "Model modification date requires adapter-supplied FOM modules");
    return found->second;
  };
  auto const& timezone = requiredFom("model-modification-date-timezone");
  auto const& exact = requiredFom("model-modification-date-exact");
  Session creator(options, model, "model-modification-date-creator");
  Session member(options, model, "model-modification-date-member");
  auto const federation = federationName(options, "fom-model-modification-date");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(timezone); }, L"InconsistentFOM",
                   "a timezone suffix on a model modification date");

  // The accepted form also confirms failed Create left the name reusable.
  createWithMim(exact);
  member.join(options.memberFederateName, options.federateType, federation);
  auto& rti = member.rtiAmbassador();
  auto const name = std::wstring(L"HLAobjectRoot.TckDateModelObject");
  auto const objectClass = rti.getObjectClassHandle(name);
  require(objectClass.isValid() && rti.getObjectClassName(objectClass) == name,
          "a model with an exact modification date did not compose publicly");
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomModelModificationDateContract(Options const& options,
                                              rti::CallbackModel model) {
  scenarioFomModelModificationDate(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
