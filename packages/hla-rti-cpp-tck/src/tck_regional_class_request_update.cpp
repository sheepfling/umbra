#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void exerciseRegionalRequestTimestampedResponse(
    Options const& options,
    rti::CallbackModel model);

void exerciseRegionalClassRequestUpdateAcrossHierarchy(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "regional-class-request-owner");
  Session requester(options, model, "regional-class-request-requester");
  auto const federation =
      federationName(options, "regional-class-request-instance-scope");
  connectAndJoin(owner, requester, options, federation, options.ddmFom);

  auto const ownerDimensions = ddmHandles(owner, options);
  auto const requesterDimensions = ddmHandles(requester, options);
  auto const ownerBaseClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckKnownClassObject");
  auto const ownerDerivedClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckKnownClassObject.TckKnownClassDerivedObject");
  auto const requesterBaseClass = requester.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckKnownClassObject");
  auto const ownerBaseAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerBaseClass, L"Name");
  auto const ownerDerivedAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerDerivedClass, L"Name");
  auto const requesterBaseAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterBaseClass, L"Name");
  require(
      ownerBaseClass.isValid() && ownerDerivedClass.isValid() &&
          requesterBaseClass.isValid() && ownerBaseAttribute.isValid() &&
          ownerDerivedAttribute.isValid() && requesterBaseAttribute.isValid(),
      "regional class-request hierarchy lookup returned an invalid handle");

  DdmHandles ownerBaseHandles = ownerDimensions;
  ownerBaseHandles.objectClass = ownerBaseClass;
  ownerBaseHandles.attribute = ownerBaseAttribute;
  DdmHandles ownerDerivedHandles = ownerDimensions;
  ownerDerivedHandles.objectClass = ownerDerivedClass;
  ownerDerivedHandles.attribute = ownerDerivedAttribute;
  DdmHandles requesterBaseHandles = requesterDimensions;
  requesterBaseHandles.objectClass = requesterBaseClass;
  requesterBaseHandles.attribute = requesterBaseAttribute;
  verifyDdmClassDimensions(
      owner, options, ownerBaseHandles, "regional class-request base class");
  verifyDdmClassDimensions(
      owner, options, ownerDerivedHandles, "regional class-request derived class");

  rti::AttributeHandleSet const ownerBaseAttributes{ownerBaseAttribute};
  rti::AttributeHandleSet const ownerDerivedAttributes{ownerDerivedAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterBaseAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerBaseClass, ownerBaseAttributes);
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerDerivedClass, ownerDerivedAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterBaseClass, requesterAttributes);

  auto const sourceRegion = createDdmRegion(
      owner, ownerDimensions, 1UL, 5UL, 1UL, 5UL);
  auto const requestRegion = createDdmRegion(
      requester, requesterDimensions, 2UL, 6UL, 2UL, 6UL);
  auto const excludedSourceRegion = createDdmRegion(
      owner, ownerDimensions, 7UL, 9UL, 7UL, 9UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const baseSourcePair{{
      ownerBaseAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const derivedSourcePair{{
      ownerDerivedAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const excludedSourcePair{{
      ownerBaseAttributes,
      rti::RegionHandleSet{excludedSourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requestPair{{
      requesterAttributes,
      rti::RegionHandleSet{requestRegion},
  }};
  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterBaseClass, requestPair, true, L"");

  auto const firstBaseObject =
      owner.rtiAmbassador().registerObjectInstanceWithRegions(
          ownerBaseClass, baseSourcePair);
  auto const secondBaseObject =
      owner.rtiAmbassador().registerObjectInstanceWithRegions(
          ownerBaseClass, baseSourcePair);
  auto const derivedObject =
      owner.rtiAmbassador().registerObjectInstanceWithRegions(
          ownerDerivedClass, derivedSourcePair);
  auto const excludedObject =
      owner.rtiAmbassador().registerObjectInstanceWithRegions(
          ownerBaseClass, excludedSourcePair);
  auto const requesterOwnedObject =
      requester.rtiAmbassador().registerObjectInstanceWithRegions(
          requesterBaseClass, requestPair);
  require(
      firstBaseObject.isValid() && secondBaseObject.isValid() &&
          derivedObject.isValid() && excludedObject.isValid() &&
          requesterOwnedObject.isValid() &&
          firstBaseObject != secondBaseObject &&
          firstBaseObject != derivedObject && secondBaseObject != derivedObject &&
          firstBaseObject != excludedObject &&
          secondBaseObject != excludedObject && derivedObject != excludedObject &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(
              requesterOwnedObject, requesterBaseAttribute),
      "regional class-request hierarchy registration returned invalid or duplicate instances");

  owner.recorder().clearProvidedUpdates();
  requester.recorder().clearProvidedUpdates();
  std::vector<std::uint8_t> const requestTagBytes{0x71U, 0x52U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterBaseClass, requestPair, requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() >= 3U; },
      options,
      "regional base-class request solicits all base and derived instances");

  auto const requests = owner.recorder().providedUpdates();
  require(
      requests.size() == 3U,
      "regional base-class request did not solicit exactly the registered base and derived instances");
  auto verifySolicitation = [&](rti::ObjectInstanceHandle const& object,
                                rti::AttributeHandle const& attribute) {
    std::size_t matchingCallbacks = 0U;
    for (auto const& request : requests) {
      if (request.object != object) {
        continue;
      }
      ++matchingCallbacks;
      require(
          request.attributes.size() == 1U &&
              request.attributes.count(attribute) == 1U &&
              request.tag == requestTagBytes,
          "regional class-request callback changed its requested attribute or tag");
    }
    require(
        matchingCallbacks == 1U,
        "regional class request did not solicit an expected instance exactly once");
  };
  verifySolicitation(firstBaseObject, ownerBaseAttribute);
  verifySolicitation(secondBaseObject, ownerBaseAttribute);
  verifySolicitation(derivedObject, ownerDerivedAttribute);
  require(
      std::none_of(
          requests.begin(), requests.end(),
          [&](auto const& request) { return request.object == excludedObject; }),
      "regional class request solicited an instance whose update region does not overlap the supplied region");
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(requester.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.01));
  }
  auto const requesterRequests = requester.recorder().providedUpdates();
  require(
      std::none_of(
          requesterRequests.begin(), requesterRequests.end(),
          [&](auto const& request) { return request.object == requesterOwnedObject; }),
      "regional request invoked Provide Attribute Value Update on the requester for an attribute it owns");

  struct ExpectedReflection final {
    rti::ObjectInstanceHandle object;
    rti::AttributeHandle attribute;
    std::vector<std::uint8_t> value;
    std::vector<std::uint8_t> tag;
  };
  std::vector<ExpectedReflection> expectedReflections;
  expectedReflections.reserve(requests.size());
  requester.recorder().clearReflection();
  std::size_t responseIndex = 0U;
  for (auto const& request : requests) {
    std::vector<std::uint8_t> const responseValue{
        0xA1U, static_cast<std::uint8_t>(responseIndex)};
    std::vector<std::uint8_t> const responseTag{
        0x73U, static_cast<std::uint8_t>(responseIndex)};
    rti::AttributeHandleValueMap responseValues;
    for (auto const& attribute : request.attributes) {
      responseValues.emplace(
          attribute,
          rti::VariableLengthData(responseValue.data(), responseValue.size()));
    }
    owner.rtiAmbassador().updateAttributeValues(
        request.object,
        responseValues,
        rti::VariableLengthData(responseTag.data(), responseTag.size()));
    expectedReflections.push_back(ExpectedReflection{
        request.object,
        *request.attributes.begin(),
        responseValue,
        responseTag});
    ++responseIndex;
  }
  waitFor(
      requester,
      [&] { return requester.recorder().reflections().size() >= requests.size(); },
      options,
      "regional class-request update responses for base and derived instances");
  auto const reflections = requester.recorder().reflections();
  require(
      std::none_of(
          reflections.begin(), reflections.end(),
          [&](auto const& reflection) { return reflection.object == excludedObject; }),
      "regional class-request response reflected an instance outside the supplied region set");
  require(
      reflections.size() == expectedReflections.size(),
      "regional class request produced an unexpected number of update reflections");
  for (auto const& expected : expectedReflections) {
    std::size_t matchingReflections = 0U;
    for (auto const& reflection : reflections) {
      if (reflection.object != expected.object) {
        continue;
      }
      ++matchingReflections;
      require(
          reflection.values.size() == 1U &&
              reflection.values.count(expected.attribute) == 1U &&
              copyBytes(reflection.values.at(expected.attribute)) == expected.value &&
              reflection.tag == expected.tag,
          "regional class-request response did not reflect the requested attribute value and tag");
    }
    require(
        matchingReflections == 1U,
        "regional class-request response did not reflect each solicited instance exactly once");
  }

  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterBaseClass, requestPair);
  requester.rtiAmbassador().unassociateRegionsForUpdates(
      requesterOwnedObject, requestPair);
  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterBaseClass, requesterAttributes);
  owner.rtiAmbassador().unassociateRegionsForUpdates(
      firstBaseObject, baseSourcePair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(
      secondBaseObject, baseSourcePair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(
      derivedObject, derivedSourcePair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(
      excludedObject, excludedSourcePair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerDerivedClass, ownerDerivedAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerBaseClass, ownerBaseAttributes);
  owner.rtiAmbassador().deleteRegion(sourceRegion);
  owner.rtiAmbassador().deleteRegion(excludedSourceRegion);
  requester.rtiAmbassador().deleteRegion(requestRegion);

  owner.resign(rti::DELETE_OBJECTS);
  requester.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
  exerciseRegionalRequestTimestampedResponse(options, model);
}

void exerciseRegionalRequestTimestampedResponse(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "regional-request-tso-owner");
  Session requester(options, model, "regional-request-tso-requester");
  auto const federation =
      federationName(options, "regional-request-tso-response-time");
  connectAndJoin(owner, requester, options, federation, options.ddmFom);

  auto ownerTime = makeTimeContext(owner);
  auto requesterTime = makeTimeContext(requester);
  enableTimestampedRoles(
      owner,
      requester,
      ownerTime,
      requesterTime,
      options,
      "regional request timestamped response");

  auto ownerHandles = ddmHandles(owner, options);
  auto requesterHandles = ddmHandles(requester, options);
  ownerHandles.objectClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckKnownClassObject");
  requesterHandles.objectClass = requester.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckKnownClassObject");
  ownerHandles.attribute = owner.rtiAmbassador().getAttributeHandle(
      ownerHandles.objectClass, L"Name");
  requesterHandles.attribute = requester.rtiAmbassador().getAttributeHandle(
      requesterHandles.objectClass, L"Name");
  require(
      ownerHandles.objectClass.isValid() && requesterHandles.objectClass.isValid() &&
          ownerHandles.attribute.isValid() && requesterHandles.attribute.isValid(),
      "regional timestamped request class or attribute lookup returned an invalid handle");
  verifyDdmClassDimensions(
      owner, options, ownerHandles, "regional timestamped request owner class");
  verifyDdmClassDimensions(
      requester, options, requesterHandles, "regional timestamped request class");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const requesterAttributes{requesterHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass, ownerAttributes);
  auto const ownerRegion = createDdmRegion(
      owner, ownerHandles, 1UL, 5UL, 1UL, 5UL);
  auto const requesterRegion = createDdmRegion(
      requester, requesterHandles, 2UL, 6UL, 2UL, 6UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      requesterAttributes,
      rti::RegionHandleSet{requesterRegion},
  }};
  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterHandles.objectClass, requesterPair, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass, ownerPair);
  require(
      object.isValid(),
      "regional timestamped request registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "regional timestamped request object discovery");

  std::vector<std::uint8_t> const requestTagBytes{0x39U, 0x71U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  owner.recorder().clearProvidedUpdates();
  requester.recorder().clearReflection();
  requester.recorder().clearTimedReflections();
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterHandles.objectClass, requesterPair, requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "regional timestamped request provider callback");
  auto const requests = owner.recorder().providedUpdates();
  require(
      requests.size() == 1U && requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          requests.front().tag == requestTagBytes,
      "regional timestamped request provider callback returned the wrong data");

  auto const responseTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      4U);
  std::vector<std::uint8_t> const responseValueBytes{0x46U, 0x92U};
  std::vector<std::uint8_t> const responseTagBytes{0x48U, 0x21U};
  rti::AttributeHandleValueMap responseValues;
  responseValues.emplace(
      ownerHandles.attribute,
      rti::VariableLengthData(
          responseValueBytes.data(), responseValueBytes.size()));
  rti::VariableLengthData const responseTag(
      responseTagBytes.data(), responseTagBytes.size());
  auto const retraction = owner.rtiAmbassador().updateAttributeValues(
      object, responseValues, responseTag, *responseTime);
  require(
      retraction.isValid() && requester.recorder().timedReflections().empty(),
      "regional timestamped request response was not queued as a TSO update");

  requester.rtiAmbassador().timeAdvanceRequest(*responseTime);
  owner.rtiAmbassador().timeAdvanceRequest(*responseTime);
  waitForSessions(
      {&owner, &requester},
      [&] {
        return owner.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timedReflections().size() >= 1U;
      },
      options,
      "regional timestamped request response reflection and grants");

  auto const reflections = requester.recorder().timedReflections();
  require(
      reflections.size() == 1U &&
          reflections.front().object == object &&
          reflections.front().values.size() == 1U &&
          reflections.front().values.count(requesterHandles.attribute) == 1U &&
          copyBytes(reflections.front().values.at(requesterHandles.attribute)) ==
              responseValueBytes &&
          reflections.front().tag == responseTagBytes &&
          reflections.front().producer == owner.federateHandle() &&
          reflections.front().time == encodeTime(*responseTime) &&
          reflections.front().sentOrder == rti::TIMESTAMP &&
          reflections.front().receivedOrder == rti::TIMESTAMP,
      "regional request reflection did not use the updating owner's timestamp and response data");

  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterHandles.objectClass, requesterPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, ownerPair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass, ownerAttributes);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.rtiAmbassador().deleteRegion(requesterRegion);
  owner.resign(rti::DELETE_OBJECTS);
  requester.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
