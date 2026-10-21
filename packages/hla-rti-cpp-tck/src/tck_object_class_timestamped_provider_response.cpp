#include "tck_main_scenarios.hpp"

#include <mutex>
#include <optional>

namespace hla_rti_cpp_tck::detail {

void scenarioObjectClassTimestampedProviderResponse(
    Options const& options, rti::CallbackModel model) {
  require(!options.autoProvideFom.empty(),
          "Timestamped class-request response requires an adapter-supplied Auto Provide FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Timestamped class-request response requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "object-class-timestamped-provider-response");
  connectAndJoin(owner, requester, options, federation, options.autoProvideFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.autoProvideFirstAttributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.autoProvideFirstAttributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerAttribute.isValid() && requesterAttribute.isValid(),
          "Timestamped class-request response resolved an invalid FOM handle");
  require(owner.rtiAmbassador().getAutoProvideSwitch(),
          "Adapter Auto Provide FOM did not enable its standard switch");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      ownerClass, ownerAttributes, rti::TIMESTAMP);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Timestamped class-request registration returned an invalid handle");
  waitFor(owner, requester,
          [&] { return requester.recorder().hasDiscovery(object) &&
                         !owner.recorder().providedUpdates().empty(); },
          options, "class-request setup discovery and Auto Provide callback");
  require(requester.rtiAmbassador().getKnownObjectClassHandle(object) == requesterClass,
          "Class-request setup discovered the wrong object class");
  owner.recorder().clearProvidedUpdates();

  auto const ownerTime = makeTimeContext(owner);
  auto const requesterTime = makeTimeContext(requester);
  require(ownerTime.factory->getName() == requesterTime.factory->getName(),
          "Timestamped class-request members selected different time factories");
  auto const responseTime = timeAfter(
      *ownerTime.factory, *ownerTime.initial, *ownerTime.epsilon, 2U);
  auto const requesterTarget = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 2U);
  std::vector<std::uint8_t> const requestTagBytes{0x43U, 0x4CU, 0x53U};
  std::vector<std::uint8_t> const responseTagBytes{0x43U, 0x4CU, 0x53U, 0x2DU, 0x52U};
  // The adapter's Auto Provide attribute is HLAbyte, so its response is one octet.
  std::vector<std::uint8_t> const responseValueBytes{0x6AU};
  rti::VariableLengthData const requestTag(requestTagBytes.data(), requestTagBytes.size());
  rti::VariableLengthData const responseTag(responseTagBytes.data(), responseTagBytes.size());
  rti::VariableLengthData const responseValue(responseValueBytes.data(), responseValueBytes.size());

  std::mutex responseMutex;
  std::optional<rti::MessageRetractionHandle> responseRetraction;
  owner.recorder().setProvideAttributeValueUpdateHook(
      [&](rti::ObjectInstanceHandle const& callbackObject,
          rti::AttributeHandleSet const& callbackAttributes,
          rti::VariableLengthData const&) {
        rti::AttributeHandleValueMap values;
        for (auto const& attribute : callbackAttributes) {
          values.emplace(attribute, responseValue);
        }
        auto const retraction = owner.rtiAmbassador().updateAttributeValues(
            callbackObject, values, responseTag, *responseTime);
        std::lock_guard<std::mutex> lock(responseMutex);
        responseRetraction = retraction;
      });

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(requester,
          [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "class-request requester time-constrained callback");
  owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "class-request owner time-regulation callback");

  requester.recorder().clearCallbackOrder();
  requester.rtiAmbassador().requestAttributeValueUpdate(
      requesterClass, requesterAttributes, requestTag);
  if (model == rti::HLA_EVOKED) {
    require(owner.recorder().providedUpdates().empty(),
            "Evoked class-request provider callback ran synchronously");
  }
  waitFor(owner,
          [&] {
            std::lock_guard<std::mutex> lock(responseMutex);
            return owner.recorder().providedUpdates().size() == 1U &&
                responseRetraction.has_value();
          },
          options, "timestamped class-request provider response");
  auto const requests = owner.recorder().providedUpdates();
  require(requests.size() == 1U && requests.front().object == object &&
              requests.front().attributes == ownerAttributes &&
              requests.front().tag == requestTagBytes,
          "Class request returned the wrong provider callback metadata");
  rti::MessageRetractionHandle retraction;
  {
    std::lock_guard<std::mutex> lock(responseMutex);
    retraction = *responseRetraction;
  }
  require(retraction.isValid(), "Timestamped provider response returned an invalid retraction handle");
  require(requester.recorder().timedReflections().empty(),
          "Constrained requester reflected before its time advance");

  requester.rtiAmbassador().timeAdvanceRequest(*requesterTarget);
  owner.rtiAmbassador().timeAdvanceRequest(*responseTime);
  waitFor(owner, requester,
          [&] { return requester.recorder().timedReflections().size() == 1U &&
                         requester.recorder().timeAdvanceGrants().size() == 1U; },
          options, "timestamped class-request reflection and grant");
  auto const reflections = requester.recorder().timedReflections();
  auto const& reflection = reflections.front();
  require(reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(requesterAttribute) == 1U &&
              copyBytes(reflection.values.at(requesterAttribute)) == responseValueBytes &&
              reflection.tag == responseTagBytes &&
              reflection.transportation == owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable") &&
              reflection.producer == owner.federateHandle() &&
              reflection.time == encodeTime(*requesterTarget) &&
              reflection.sentOrder == rti::TIMESTAMP &&
              reflection.receivedOrder == rti::TIMESTAMP &&
              reflection.retractionPresent &&
              reflection.regions == std::nullopt,
          "Timestamped class-request reflection lost value, tag, producer, time, or order metadata");
  require(requester.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "grant"},
          "Timestamped class-request reflection did not precede its grant");
  require(requester.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*requesterTarget),
          "Timestamped class-request requester advanced to the wrong time");
  bool cannotRetract = false;
  try {
    owner.rtiAmbassador().retract(retraction);
  } catch (rti::MessageCanNoLongerBeRetracted const&) {
    cannotRetract = true;
  }
  require(cannotRetract,
          "Delivered timestamped class-request response remained retractable");

  owner.recorder().setProvideAttributeValueUpdateHook({});
  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass, requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioObjectClassTimestampedProviderResponseContract(
    Options const& options, rti::CallbackModel model) {
  scenarioObjectClassTimestampedProviderResponse(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
