#include "tck_attribute_value_update_request_response.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioAttributeValueUpdateRequestBaseline(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Attribute-value request baseline requires an adapter-supplied multi-attribute FOM");
  Session owner(options, model, "request-baseline-owner");
  Session requester(options, model, "request-baseline-requester");
  auto const federation = federationName(options, "attribute-value-update-request-baseline");
  connectAndJoin(owner, requester, options, federation, options.multiAttributeFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeSecondName);
  require(
      ownerClass.isValid() && requesterClass.isValid() && ownerFirst.isValid() &&
          ownerSecond.isValid() && requesterFirst.isValid() && requesterSecond.isValid(),
      "Attribute-value request baseline lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const requesterAttributes{requesterSecond};
  rti::AttributeHandleSet const requestAttributes{
      requesterFirst,
      requesterSecond,
  };
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requestAttributes,
      true,
      L"");

  auto const ownerObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      ownerObject.isValid(),
      "Attribute-value request baseline registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(ownerObject); },
      options,
      "attribute-value request baseline discovery");

  std::vector<std::uint8_t> const requestTagBytes{0x52U, 0x51U, 0x2DU, 0x31U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  owner.recorder().clearProvidedUpdates();
  requester.rtiAmbassador().requestAttributeValueUpdate(
      ownerObject,
      requestAttributes,
      requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "attribute-value request baseline provider callback");
  auto const ownerRequests = owner.recorder().providedUpdates();
  require(
      ownerRequests.front().object == ownerObject &&
          ownerRequests.front().attributes == ownerAttributes &&
          ownerRequests.front().tag == requestTagBytes,
      "attribute-value request baseline returned the wrong owner callback metadata");

  // A requester-owned attribute is not solicited back to the same federate;
  // the remaining requested attributes are unowned on this second object.
  auto const requesterObject =
      requester.rtiAmbassador().registerObjectInstance(requesterClass);
  require(
      requesterObject.isValid(),
      "Attribute-value request baseline requester registration returned an invalid handle");
  requester.recorder().clearProvidedUpdates();
  requester.rtiAmbassador().requestAttributeValueUpdate(
      requesterObject,
      requestAttributes,
      requestTag);
  for (int pass = 0; pass != 8; ++pass) {
    requester.pump();
  }
  require(
      requester.recorder().providedUpdates().empty(),
      "Attribute-value request baseline solicited the requester-owned or unowned attributes");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requestAttributes);
  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::DELETE_OBJECTS);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAttributeValueUpdateRequestBaselineContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeValueUpdateRequestBaseline(options, model);
}

void scenarioObjectClassAttributeValueUpdateRequestBaseline(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Object-class attribute-value request baseline requires an adapter-supplied rich FOM");

  Session owner(options, model, "class-request-baseline-owner");
  Session requester(options, model, "class-request-baseline-requester");
  auto const federation = federationName(options, "object-class-attribute-value-request-baseline");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  requester.join(options.memberFederateName, options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const ownerIdentity = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.typedIdentityAttributeName);
  auto const ownerDerived = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.typedDerivedAttributeName);
  auto const requesterIdentity = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.typedIdentityAttributeName);
  auto const requesterInteger = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.typedIntegerAttributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() && ownerIdentity.isValid() &&
          ownerDerived.isValid() && requesterIdentity.isValid() &&
          requesterInteger.isValid(),
      "Object-class attribute-value request baseline lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerIdentity, ownerDerived};
  rti::AttributeHandleSet const requesterAttributes{requesterInteger};
  rti::AttributeHandleSet const requestAttributes{
      requesterIdentity,
      requesterInteger,
  };
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requestAttributes,
      true,
      L"");

  std::vector<rti::ObjectInstanceHandle> ownerObjects;
  ownerObjects.push_back(owner.rtiAmbassador().registerObjectInstance(ownerClass));
  ownerObjects.push_back(owner.rtiAmbassador().registerObjectInstance(ownerClass));
  require(
      ownerObjects.front().isValid() && ownerObjects.back().isValid(),
      "Object-class attribute-value request baseline registration returned an invalid handle");
  for (auto const object : ownerObjects) {
    waitFor(
        requester,
        [&] { return requester.recorder().hasDiscovery(object); },
        options,
        "object-class attribute-value request baseline discovery");
  }
  auto const requesterObject =
      requester.rtiAmbassador().registerObjectInstance(requesterClass);
  require(
      requesterObject.isValid(),
      "Object-class attribute-value request baseline requester registration returned an invalid handle");

  std::vector<std::uint8_t> const requestTagBytes{0x43U, 0x4CU, 0x53U, 0x2DU, 0x31U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  owner.recorder().clearProvidedUpdates();
  requester.recorder().clearProvidedUpdates();
  requester.rtiAmbassador().requestAttributeValueUpdate(
      requesterClass,
      requestAttributes,
      requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() == ownerObjects.size(); },
      options,
      "object-class attribute-value request baseline provider callbacks");
  auto const ownerRequests = owner.recorder().providedUpdates();
  require(
      ownerRequests.size() == ownerObjects.size(),
      "Object-class attribute-value request baseline returned an unexpected callback count");
  for (auto const& request : ownerRequests) {
    require(
        (request.object == ownerObjects.front() || request.object == ownerObjects.back()) &&
            request.attributes == rti::AttributeHandleSet{ownerIdentity} &&
            request.tag == requestTagBytes,
        "Object-class attribute-value request baseline returned the wrong callback metadata");
  }
  require(
      requester.recorder().providedUpdates().empty(),
      "Object-class attribute-value request baseline solicited the requester-owned attribute");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requestAttributes);
  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::DELETE_OBJECTS);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioObjectClassAttributeValueUpdateRequestBaselineContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectClassAttributeValueUpdateRequestBaseline(options, model);
}


void scenarioAttributeValueUpdateResponse(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "attribute-response-owner");
  Session member(options, model, "attribute-response-member");
  auto const federation = federationName(options, "attribute-value-update-response");
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
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && memberClass.isValid() &&
          memberAttribute.isValid(),
      "Attribute-value response lookup returned an invalid class or attribute handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const memberAttributes{memberAttribute};

  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      memberAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Attribute-value response registration returned an invalid object handle");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "attribute-value response object discovery");
  auto const knownObject = member.recorder().discovery().object;
  require(
      knownObject == object,
      "Attribute-value response discovery returned the wrong object handle");

  owner.recorder().clearProvidedUpdates();
  member.recorder().clearReflection();
  std::vector<std::uint8_t> const requestTagBytes{0x52U, 0x45U, 0x51U};
  std::vector<std::uint8_t> const responseValueBytes{0x5AU, 0x25U, 0x07U};
  std::vector<std::uint8_t> const responseTagBytes{0x52U, 0x45U, 0x53U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  member.rtiAmbassador().requestAttributeValueUpdate(
      knownObject,
      memberAttributes,
      requestTag);

  waitFor(
      owner,
      [&] { return !owner.recorder().providedUpdates().empty(); },
      options,
      "attribute-value response provider callback");
  require(
      member.recorder().reflectionCount() == 0U,
      "Attribute-value response was reflected before the provider answered");
  auto const requests = owner.recorder().providedUpdates();
  require(
      requests.size() == 1U && requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          requests.front().tag == requestTagBytes,
      "Attribute-value response provider callback returned the wrong object, attributes, or tag");

  rti::AttributeHandleValueMap responseValues;
  responseValues.emplace(
      ownerAttribute,
      rti::VariableLengthData(responseValueBytes.data(), responseValueBytes.size()));
  rti::VariableLengthData const responseTag(
      responseTagBytes.data(),
      responseTagBytes.size());
  owner.rtiAmbassador().updateAttributeValues(object, responseValues, responseTag);
  waitFor(
      member,
      [&] { return member.recorder().reflection().present; },
      options,
      "attribute-value response reflection");

  auto const reflection = member.recorder().reflection();
  require(
      reflection.object == knownObject && reflection.values.size() == 1U &&
          reflection.values.count(memberAttribute) == 1U &&
          copyBytes(reflection.values.at(memberAttribute)) == responseValueBytes &&
          reflection.tag == responseTagBytes,
      "Attribute-value response reflection returned the wrong object, value, or tag");
  require(
      reflection.transportation ==
              member.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable") &&
          reflection.producer == owner.federateHandle() && !reflection.regions.has_value(),
      "Attribute-value response reflection returned the wrong standard delivery metadata");

  member.rtiAmbassador().unsubscribeObjectClassAttributes(memberClass, memberAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  member.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioAttributeValueUpdateResponseContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeValueUpdateResponse(options, model);
}

void scenarioAttributeValueUpdateRequestMultiRequester(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "request-fanout-owner");
  Session first(options, model, "request-fanout-first");
  Session second(options, model, "request-fanout-second");
  auto const federation = federationName(
      options,
      "attribute-value-update-request-multi-requester");
  connectAndJoin(owner, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName + L"-second",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const firstClass = first.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const secondClass = second.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const firstAttribute = first.rtiAmbassador().getAttributeHandle(
      firstClass,
      options.attributeName);
  auto const secondAttribute = second.rtiAmbassador().getAttributeHandle(
      secondClass,
      options.attributeName);
  require(
      ownerClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
          ownerAttribute.isValid() && firstAttribute.isValid() &&
          secondAttribute.isValid(),
      "multi-requester attribute-value request lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
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

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "multi-requester attribute-value request registration returned an invalid object");
  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasDiscovery(object) &&
            second.recorder().hasDiscovery(object);
      },
      options,
      "multi-requester attribute-value request discovery");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty() &&
          first.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          second.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          first.rtiAmbassador().getKnownObjectClassHandle(object) == firstClass &&
          second.rtiAmbassador().getKnownObjectClassHandle(object) == secondClass,
      "multi-requester attribute-value request discovery did not establish stable identity");

  std::vector<std::uint8_t> const firstRequestTag{0x51U, 0x52U, 0x31U};
  std::vector<std::uint8_t> const secondRequestTag{0x51U, 0x52U, 0x32U};
  rti::VariableLengthData const firstRequestTagData(
      firstRequestTag.data(),
      firstRequestTag.size());
  rti::VariableLengthData const secondRequestTagData(
      secondRequestTag.data(),
      secondRequestTag.size());
  owner.recorder().clearProvidedUpdates();
  first.recorder().clearProvidedUpdates();
  second.recorder().clearProvidedUpdates();
  first.rtiAmbassador().requestAttributeValueUpdate(
      first.recorder().discovery().object,
      firstAttributes,
      firstRequestTagData);
  second.rtiAmbassador().requestAttributeValueUpdate(
      second.recorder().discovery().object,
      secondAttributes,
      secondRequestTagData);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() >= 2U; },
      options,
      "multi-requester attribute-value request provider callbacks");
  auto const requests = owner.recorder().providedUpdates();
  require(
      requests.size() == 2U,
      "multi-requester attribute-value request returned duplicate provider callbacks");
  bool firstRequestObserved = false;
  bool secondRequestObserved = false;
  for (auto const& request : requests) {
    require(
        request.object == object && request.attributes == ownerAttributes,
        "multi-requester attribute-value request returned the wrong provider metadata");
    if (request.tag == firstRequestTag) {
      require(
          !firstRequestObserved,
          "multi-requester attribute-value request duplicated the first request tag");
      firstRequestObserved = true;
    } else if (request.tag == secondRequestTag) {
      require(
          !secondRequestObserved,
          "multi-requester attribute-value request duplicated the second request tag");
      secondRequestObserved = true;
    } else {
      throw std::runtime_error(
          "multi-requester attribute-value request returned an unknown request tag");
    }
  }
  require(
      firstRequestObserved && secondRequestObserved,
      "multi-requester attribute-value request lost one requester callback");
  require(
      first.recorder().providedUpdates().empty() &&
          second.recorder().providedUpdates().empty(),
      "multi-requester attribute-value request looped a provider callback to a requester");

  std::vector<std::uint8_t> const responseValue{0x41U, 0x56U, 0x52U, 0x31U};
  std::vector<std::uint8_t> const responseTag{0x52U, 0x53U, 0x50U};
  rti::AttributeHandleValueMap responseValues;
  responseValues.emplace(
      ownerAttribute,
      rti::VariableLengthData(responseValue.data(), responseValue.size()));
  rti::VariableLengthData const responseTagData(
      responseTag.data(),
      responseTag.size());
  first.recorder().clearReflection();
  second.recorder().clearReflection();
  owner.rtiAmbassador().updateAttributeValues(
      object,
      responseValues,
      responseTagData);
  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().reflection().present &&
            second.recorder().reflection().present;
      },
      options,
      "multi-requester attribute-value response fan-out");

  auto assertResponse = [&](Session& receiver,
                            rti::AttributeHandle const& receiverAttribute,
                            std::string const& description) {
    auto const reflection = receiver.recorder().reflection();
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U &&
            copyBytes(reflection.values.at(receiverAttribute)) == responseValue &&
            reflection.tag == responseTag &&
            reflection.producer == owner.federateHandle() &&
            reflection.transportation.isValid() &&
            !reflection.regions.has_value(),
        description + " returned the wrong response metadata");
  };
  assertResponse(first, firstAttribute, "first multi-requester response");
  assertResponse(second, secondAttribute, "second multi-requester response");

  first.rtiAmbassador().unsubscribeObjectClassAttributes(firstClass, firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(secondClass, secondAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  owner.disconnect();
}

void scenarioAttributeValueUpdateRequestMultiRequesterContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeValueUpdateRequestMultiRequester(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
