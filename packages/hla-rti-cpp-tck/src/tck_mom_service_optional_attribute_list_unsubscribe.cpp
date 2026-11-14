#include "tck_main_scenarios.hpp"

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void verifyMomServiceOptionalAttributeListUnsubscribe(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Optional HLAservice argument coverage requires an application FOM, the standard MIM, and logical-time configuration");

  Session publisher(options, model, "mom-service-unsubscribe-publisher");
  Session subscriber(options, model, "mom-service-unsubscribe-subscriber");
  auto const federation =
      federationName(options, "mom-service-optional-attribute-list-unsubscribe");
  publisher.connect();
  subscriber.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const publisherName =
      options.ownerFederateName + L"-mom-service-unsubscribe-publisher";
  auto const subscriberName =
      options.memberFederateName + L"-mom-service-unsubscribe-subscriber";
  publisher.join(publisherName, options.federateType, federation);
  subscriber.join(subscriberName, options.federateType, federation);

  auto& publisherRti = publisher.rtiAmbassador();
  auto& subscriberRti = subscriber.rtiAmbassador();
  auto const publisherClass =
      publisherRti.getObjectClassHandle(options.objectClassName);
  auto const subscriberClass =
      subscriberRti.getObjectClassHandle(options.objectClassName);
  auto const publisherAttribute =
      publisherRti.getAttributeHandle(publisherClass, options.attributeName);
  auto const subscriberAttribute =
      subscriberRti.getAttributeHandle(subscriberClass, options.attributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid(),
      "Adapter FOM lacks the object class or attribute for optional-argument unsubscribe");

  publisherRti.publishObjectClassAttributes(
      publisherClass, rti::AttributeHandleSet{publisherAttribute});
  subscriberRti.subscribeObjectClassAttributes(
      subscriberClass, rti::AttributeHandleSet{subscriberAttribute});
  auto const object = publisherRti.registerObjectInstance(publisherClass);
  require(object.isValid(), "Optional-argument unsubscribe registered no object");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "object discovery before HLAservice unsubscribe");

  auto sendValue = [&](std::vector<std::uint8_t> const& value) {
    rti::AttributeHandleValueMap const values{{
        publisherAttribute,
        rti::VariableLengthData(value.data(), value.size()),
    }};
    publisherRti.updateAttributeValues(object, values, rti::VariableLengthData{});
  };
  std::vector<std::uint8_t> const firstValue{0x41U, 0x52U};
  sendValue(firstValue);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflections().size() == 1U; },
      options,
      "initial active subscription reflection");
  auto const initialReflections = subscriber.recorder().reflections();
  require(
      initialReflections.size() == 1U &&
          initialReflections.front().object == object &&
          initialReflections.front().values.size() == 1U &&
          initialReflections.front().values.count(subscriberAttribute) == 1U &&
          copyBytes(initialReflections.front().values.at(subscriberAttribute)) ==
              firstValue,
      "Initial active subscription did not reflect the expected attribute");

  subscriber.recorder().clearReflection();
  auto const serviceInteraction = subscriberRti.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassAttributes");
  auto const objectClassParameter = subscriberRti.getParameterHandle(
      serviceInteraction, L"HLAobjectClass");
  require(
      serviceInteraction.isValid() && objectClassParameter.isValid(),
      "Standard MIM lacks HLAunsubscribeObjectClassAttributes or HLAobjectClass");
  subscriberRti.publishInteractionClass(serviceInteraction);

  // Omitting HLAattributeList is distinct from supplying an empty list: the
  // standard whole-class form must remove the active attribute subscription.
  rti::ParameterHandleValueMap const parameters{{
      objectClassParameter, subscriberClass.encode()}};
  subscriberRti.sendInteraction(
      serviceInteraction, parameters, rti::VariableLengthData{});

  std::vector<std::uint8_t> const laterValue{0x63U, 0x74U};
  sendValue(laterValue);
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    subscriber.pump();
    if (!subscriber.recorder().reflections().empty()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      subscriber.recorder().reflections().empty(),
      "Omitting HLAattributeList did not perform whole-class unsubscribe");

  publisherRti.unpublishObjectClassAttributes(
      publisherClass, rti::AttributeHandleSet{publisherAttribute});
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherRti.destroyFederationExecution(federation);
}

}  // namespace

void scenarioMomServiceOptionalAttributeListUnsubscribe(
    Options const& options,
    rti::CallbackModel model) {
  verifyMomServiceOptionalAttributeListUnsubscribe(options, model);
}

void scenarioMomServiceOptionalAttributeListUnsubscribeContract(
    Options const& options,
    rti::CallbackModel model) {
  verifyMomServiceOptionalAttributeListUnsubscribe(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
