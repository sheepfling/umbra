#include "tck_main_scenarios.hpp"

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void verifyFederationMomTable8PublishedAttributes(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM Table 8 testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session creator(options, model, "federation-mom-table8-creator");
  Session observer(options, model, "federation-mom-table8-observer");
  auto const federation = federationName(options, "federation-mom-table8-p-attributes");
  creator.connect();
  observer.connect();
  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(
      options.ownerFederateName + L"-table8-creator",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-table8-observer",
      options.federateType,
      federation);
  verifyFederationMomTable7PublishedInteractions(observer);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  std::vector<std::wstring> const publishedNames{
      L"HLAfederationName",
      L"HLAfederatesInFederation",
      L"HLARTIversion",
      L"HLAMIMdesignator",
      L"HLAFOMmoduleDesignatorList",
      L"HLAcurrentFDD",
      L"HLAtimeImplementationName",
      L"HLAlastSaveName",
      L"HLAlastSaveTime",
      L"HLAnextSaveName",
      L"HLAnextSaveTime",
      L"HLAautoProvide",
      L"HLAadvisoriesUseKnownClass",
      L"HLAdelaySubscriptionEvaluation",
      L"HLAnonRegulatedGrant",
      L"HLAallowRelaxedDDM",
  };
  rti::AttributeHandleSet publishedAttributes;
  for (auto const& name : publishedNames) {
    auto const attribute = observer.rtiAmbassador().getAttributeHandle(
        federationClass,
        name);
    require(
        attribute.isValid() &&
            observer.rtiAmbassador().getAttributeName(federationClass, attribute) ==
                name,
        "Federation MOM Table 8 attribute lookup did not round-trip: " +
            std::string(name.begin(), name.end()));
    publishedAttributes.insert(attribute);
  }
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(federationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable" &&
          publishedAttributes.size() == publishedNames.size(),
      "Federation MOM Table 8 did not resolve 16 distinct published attributes");

  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      publishedAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "standard HLAfederation Table 8 MOM discovery");

  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.recorder().discoveryCount() == 1U &&
          federationObject.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
              federationClass &&
          observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
              L"HLAfederation" &&
          observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
              federationObject,
      "Federation MOM Table 8 did not expose exactly one standard instance with the expected identity");

  observer.recorder().clearReflection();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      federationObject,
      publishedAttributes,
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        return reflection.present && reflection.object == federationObject &&
            reflection.values.size() == publishedAttributes.size();
      },
      options,
      "complete HLAfederation Table 8 P-attribute request reflection");

  auto const reflection = observer.recorder().reflection();
  require(
      reflection.values.size() == 16U &&
          reflection.transportation == reliable && reflection.tag.empty() &&
          !reflection.producer.isValid() && !reflection.regions.has_value(),
      "Federation MOM Table 8 request did not return all P values with standard MIM callback metadata");
  for (auto const attribute : publishedAttributes) {
    require(
        reflection.values.count(attribute) == 1U,
        "Federation MOM Table 8 request omitted a P-designated attribute value");
  }

  // Table 6 marks HLAfederationName NoTransfer. Observe its owner, make a
  // standard availability acquisition attempt, then query again to ensure
  // the attempt did not transfer the MOM attribute to a federate.
  auto const ownershipProbeAttribute = publishedAttributes.find(
      observer.rtiAmbassador().getAttributeHandle(
          federationClass,
          L"HLAfederationName"));
  require(
      ownershipProbeAttribute != publishedAttributes.end(),
      "The federation MOM ownership probe attribute was not in the Table 8 P set");
  rti::AttributeHandleSet const ownershipProbeAttributes{
      *ownershipProbeAttribute};
  observer.recorder().clearOwnershipRecords();

  struct OwnershipObservation final {
    bool ownedByRti = false;
    bool notOwned = false;
    std::optional<rti::FederateHandle> federateOwner;
  };
  auto ownershipResponseCount = [&] {
    return observer.recorder().ownershipOwnedByRtiEvents().size() +
        observer.recorder().ownershipNotOwnedEvents().size() +
        observer.recorder().ownershipInformationEvents().size();
  };
  auto queryOwnership = [&] {
    auto const rtiBefore = observer.recorder().ownershipOwnedByRtiEvents().size();
    auto const notOwnedBefore =
        observer.recorder().ownershipNotOwnedEvents().size();
    auto const ownerBefore =
        observer.recorder().ownershipInformationEvents().size();
    auto const responsesBefore = ownershipResponseCount();
    observer.rtiAmbassador().queryAttributeOwnership(
        federationObject,
        ownershipProbeAttributes);
    waitFor(
        observer,
        [&] { return ownershipResponseCount() > responsesBefore; },
        options,
        "HLAfederationName MOM ownership query");

    auto const rtiEvents = observer.recorder().ownershipOwnedByRtiEvents();
    auto const notOwnedEvents = observer.recorder().ownershipNotOwnedEvents();
    auto const ownerEvents = observer.recorder().ownershipInformationEvents();
    auto const rtiDelta = rtiEvents.size() - rtiBefore;
    auto const notOwnedDelta = notOwnedEvents.size() - notOwnedBefore;
    auto const ownerDelta = ownerEvents.size() - ownerBefore;
    require(
        rtiDelta + notOwnedDelta + ownerDelta == 1U,
        "HLAfederationName ownership query did not yield exactly one status");

    OwnershipObservation observation;
    auto requireProbeRecord = [&](
        rti::ObjectInstanceHandle const& object,
        rti::AttributeHandleSet const& attributes) {
      require(
          object == federationObject && attributes == ownershipProbeAttributes,
          "MOM ownership callback identified a different object or attribute");
    };
    if (rtiDelta == 1U) {
      auto const& event = rtiEvents.back();
      requireProbeRecord(event.object, event.attributes);
      observation.ownedByRti = true;
    } else if (notOwnedDelta == 1U) {
      auto const& event = notOwnedEvents.back();
      requireProbeRecord(event.object, event.attributes);
      observation.notOwned = true;
    } else {
      auto const& event = ownerEvents.back();
      requireProbeRecord(event.object, event.attributes);
      observation.federateOwner = event.owner;
    }
    return observation;
  };

  auto const initialOwnership = queryOwnership();
  observer.rtiAmbassador().publishObjectClassAttributes(
      federationClass,
      ownershipProbeAttributes);
  auto const acquisitionEventsBefore =
      observer.recorder().ownershipAcquisitionEvents().size();
  try {
    observer.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
        federationObject,
        ownershipProbeAttributes,
        rti::VariableLengthData{});
  } catch (rti::Exception const&) {
    // The prohibition is about transfer; the standard does not prescribe a
    // single error result for attempting acquisition of a NoTransfer MOM value.
  }
  auto const resultingOwnership = queryOwnership();
  require(
      initialOwnership.ownedByRti == resultingOwnership.ownedByRti &&
          initialOwnership.notOwned == resultingOwnership.notOwned &&
          initialOwnership.federateOwner.has_value() ==
              resultingOwnership.federateOwner.has_value() &&
          (!initialOwnership.federateOwner.has_value() ||
           *initialOwnership.federateOwner == *resultingOwnership.federateOwner),
      "An ownership-acquisition attempt changed HLAfederationName MOM ownership");
  require(
      observer.recorder().ownershipAcquisitionEvents().size() ==
          acquisitionEventsBefore,
      "The RTI transferred the HLAfederationName MOM attribute to the requesting federate");
  observer.rtiAmbassador().unpublishObjectClassAttributes(
      federationClass,
      ownershipProbeAttributes);

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federationClass,
      publishedAttributes);
  observer.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  creator.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
