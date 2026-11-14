#include "tck_main_scenarios.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

template <typename Handle>
void requireMimHandleByteArray(Handle const& handle,
                               std::string const& description) {
  require(handle.isValid(), description + " handle is invalid");
  auto const encoded = handle.encode();
  rti::HLAvariableArrayT<rti::HLAoctet> mimValue;
  static_cast<void>(mimValue.decode(encoded));
  require(copyBytes(mimValue.encode()) == copyBytes(encoded),
          description + " encoding did not round-trip as its HLAstandardMIM byte array");
}

template <typename Handle, typename Decoder>
void requireMimHandleByteArray(Handle const& handle,
                               Decoder&& decoder,
                               std::string const& description) {
  requireMimHandleByteArray(handle, description);
  require(decoder(handle.encode()) == handle,
          description + " did not round-trip through its typed public decoder");
}

}  // namespace

void scenarioHandleEncodingMimDataTypes(Options const& options,
                                         rti::CallbackModel model) {
  Session owner(options, model, "handle-encoding-owner");
  Session member(options, model, "handle-encoding-member");
  require(!options.ddmFom.empty(),
          "Handle-encoding TCK requires its configured DDM FOM");
  auto const federation = federationName(options, "handle-encoding-mim-types");
  owner.connect();
  member.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const objectClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const attribute =
      owner.rtiAmbassador().getAttributeHandle(objectClass, options.attributeName);
  auto const interactionClass = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = owner.rtiAmbassador().getParameterHandle(
      interactionClass, options.parameterName);
  require(!options.ddmDimensionNames.empty(),
          "Handle-encoding DDM FOM supplied no dimension names");
  auto const dimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const transportation =
      owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");

  rti::AttributeHandleSet publishedAttributes;
  publishedAttributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, publishedAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Handle-encoding object registration returned an invalid handle");

  auto const region = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(region.isValid(), "Handle-encoding region creation returned an invalid handle");

  owner.rtiAmbassador().publishInteractionClass(interactionClass);
  owner.rtiAmbassador().changeInteractionOrderType(
      interactionClass, rti::TIMESTAMP);
  member.rtiAmbassador().subscribeInteractionClass(interactionClass, true);
  auto time = makeTimeContext(owner);
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(
      owner,
      [&] { return !owner.recorder().timeRegulationEnabled().empty(); },
      options,
      "Handle-encoding time-regulation callback");
  auto const messageTime = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 1U);
  std::vector<std::uint8_t> parameterBytes{0x48U, 0x4CU, 0x41U};
  rti::VariableLengthData parameterValue(
      parameterBytes.data(), parameterBytes.size());
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(parameter, parameterValue);
  std::vector<std::uint8_t> tagBytes{0x54U, 0x43U, 0x4BU};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = owner.rtiAmbassador().sendInteraction(
      interactionClass, parameters, tag, *messageTime);
  require(retraction.isValid(),
          "Timestamped handle-encoding interaction returned an invalid retraction handle");

  requireMimHandleByteArray(
      owner.federateHandle(),
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeFederateHandle(bytes);
      },
      "HLAfederateHandle");
  requireMimHandleByteArray(
      objectClass,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeObjectClassHandle(bytes);
      },
      "HLAobjectClassHandle");
  requireMimHandleByteArray(
      interactionClass,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeInteractionClassHandle(bytes);
      },
      "HLAinteractionClassHandle");
  requireMimHandleByteArray(
      object,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeObjectInstanceHandle(bytes);
      },
      "HLAobjectInstanceHandle");
  requireMimHandleByteArray(
      attribute,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeAttributeHandle(bytes);
      },
      "HLAattributeHandle");
  requireMimHandleByteArray(
      parameter,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeParameterHandle(bytes);
      },
      "HLAparameterHandle");
  requireMimHandleByteArray(
      dimension,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeDimensionHandle(bytes);
      },
      "HLAdimensionHandle");
  requireMimHandleByteArray(
      retraction,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeMessageRetractionHandle(bytes);
      },
      "HLAmessageRetractionHandle");
  requireMimHandleByteArray(
      region,
      [&](rti::VariableLengthData const& bytes) {
        return member.rtiAmbassador().decodeRegionHandle(bytes);
      },
      "HLAregionHandle");
  requireMimHandleByteArray(transportation, "HLAtransportationTypeHandle");

  owner.rtiAmbassador().deleteRegion(region);
  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioHandleParameterValueRoundTrip(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "handle-value-owner");
  Session receiver(options, model, "handle-value-receiver");
  auto const federation = federationName(options, "handle-parameter-value-round-trip");
  connectAndJoin(owner, receiver, options, federation, options.fom);

  auto const ownerObjectClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const receiverObjectClass =
      receiver.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass, options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverObjectClass, options.attributeName);
  auto const ownerInteractionClass = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteractionClass =
      receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const ownerParameter = owner.rtiAmbassador().getParameterHandle(
      ownerInteractionClass, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteractionClass, options.parameterName);
  require(
      ownerObjectClass.isValid() && receiverObjectClass.isValid() &&
          ownerAttribute.isValid() && receiverAttribute.isValid() &&
          ownerInteractionClass.isValid() && receiverInteractionClass.isValid() &&
          ownerParameter.isValid() && receiverParameter.isValid(),
      "handle-value transfer FOM lookup returned an invalid designator");

  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass, rti::AttributeHandleSet{ownerAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverObjectClass, rti::AttributeHandleSet{receiverAttribute}, true, L"");
  owner.rtiAmbassador().publishInteractionClass(ownerInteractionClass);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteractionClass, true);

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(object.isValid(), "handle-value transfer registration returned an invalid handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!objectName.empty(), "handle-value transfer registration returned an empty name");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "handle-value transfer object discovery");

  auto const encodedHandle = object.encode();
  require(!copyBytes(encodedHandle).empty(), "object-instance handle encoding was empty");
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(ownerParameter, encodedHandle);
  receiver.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().sendInteraction(
      ownerInteractionClass, parameters, rti::VariableLengthData{});
  std::vector<std::uint8_t> replacementParameterBytes{0x00U, 0x5AU};
  parameters.at(ownerParameter).setData(
      replacementParameterBytes.data(), replacementParameterBytes.size());
  require(
      copyBytes(parameters.at(ownerParameter)) == replacementParameterBytes,
      "test did not mutate the caller-owned interaction parameter after Send Interaction returned");
  receiver.rtiAmbassador().enableCallbacks();
  waitFor(
      receiver,
      [&] { return receiver.recorder().interaction().present; },
      options,
      "encoded object-instance handle interaction parameter");

  auto const received = receiver.recorder().interaction();
  auto const receivedValue = received.parameters.find(receiverParameter);
  require(
      received.interaction == receiverInteractionClass &&
          received.producer == owner.federateHandle() &&
          receivedValue != received.parameters.end(),
      "receiver did not get the encoded handle under its local interaction designators");
  require(
      copyBytes(receivedValue->second) == copyBytes(encodedHandle),
      "RTI changed the encoded object-instance handle while forwarding its parameter value");
  auto const decoded = receiver.rtiAmbassador().decodeObjectInstanceHandle(
      receivedValue->second);
  require(
      decoded == object &&
          receiver.rtiAmbassador().getObjectInstanceHandle(objectName) == decoded &&
          receiver.rtiAmbassador().getObjectInstanceName(decoded) == objectName,
      "receiver did not reconstitute the sender's object-instance handle");
  auto rejectWrongType = [&](auto&& decoder, std::string const& description) {
    requireException(
        [&] { static_cast<void>(decoder(receivedValue->second)); },
        L"CouldNotDecode",
        description);
  };
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeFederateHandle(bytes);
      },
      "reconstituting an object-instance handle as a federate handle");
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeObjectClassHandle(bytes);
      },
      "reconstituting an object-instance handle as an object-class handle");
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeInteractionClassHandle(bytes);
      },
      "reconstituting an object-instance handle as an interaction-class handle");
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeAttributeHandle(bytes);
      },
      "reconstituting an object-instance handle as an attribute handle");
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeParameterHandle(bytes);
      },
      "reconstituting an object-instance handle as a parameter handle");
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeDimensionHandle(bytes);
      },
      "reconstituting an object-instance handle as a dimension handle");
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeMessageRetractionHandle(bytes);
      },
      "reconstituting an object-instance handle as a message-retraction handle");
  rejectWrongType(
      [&](auto const& bytes) {
        return receiver.rtiAmbassador().decodeRegionHandle(bytes);
      },
      "reconstituting an object-instance handle as a region handle");

  rti::AttributeHandleValueMap attributeValues;
  attributeValues.emplace(ownerAttribute, encodedHandle);
  receiver.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().updateAttributeValues(
      object, attributeValues, rti::VariableLengthData{});
  std::vector<std::uint8_t> replacementAttributeBytes{0x00U, 0x6AU};
  attributeValues.at(ownerAttribute).setData(
      replacementAttributeBytes.data(), replacementAttributeBytes.size());
  require(
      copyBytes(attributeValues.at(ownerAttribute)) == replacementAttributeBytes,
      "test did not mutate the caller-owned attribute value after Update Attribute Values returned");
  receiver.rtiAmbassador().enableCallbacks();
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflection().present; },
      options,
      "encoded object-instance handle attribute value");
  auto const reflected = receiver.recorder().reflection();
  auto const reflectedValue = reflected.values.find(receiverAttribute);
  require(
      reflected.object == object && reflectedValue != reflected.values.end(),
      "receiver did not get the encoded handle under its local attribute designator");
  require(
      copyBytes(reflectedValue->second) == copyBytes(encodedHandle),
      "RTI changed the encoded object-instance handle while forwarding its attribute value");
  auto const decodedAttributeValue = receiver.rtiAmbassador().decodeObjectInstanceHandle(
      reflectedValue->second);
  require(
      decodedAttributeValue == object &&
          receiver.rtiAmbassador().getObjectInstanceHandle(objectName) ==
              decodedAttributeValue,
      "receiver did not reconstitute the object-instance handle from the attribute value");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverObjectClass, rti::AttributeHandleSet{receiverAttribute});
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteractionClass);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass, rti::AttributeHandleSet{ownerAttribute});
  owner.rtiAmbassador().unpublishInteractionClass(ownerInteractionClass);
  receiver.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
