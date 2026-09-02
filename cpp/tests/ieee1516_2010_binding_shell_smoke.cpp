#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>

#include <cassert>
#include <memory>
#include <string>

namespace {

class RecordingFederateAmbassador final : public rti1516e::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti1516e::ObjectInstanceHandle object,
      rti1516e::ObjectClassHandle objectClass,
      std::wstring const& name,
      rti1516e::FederateHandle producingFederate) throw(rti1516e::FederateInternalError) override {
    discovered = true;
    discoveredObject = object;
    discoveredClass = objectClass;
    discoveredName = name;
    discoveredFederate = producingFederate;
  }

  void reflectAttributeValues(
      rti1516e::ObjectInstanceHandle object,
      rti1516e::AttributeHandleValueMap const& values,
      rti1516e::VariableLengthData const& tag,
      rti1516e::OrderType,
      rti1516e::TransportationType,
      rti1516e::SupplementalReflectInfo info) throw(rti1516e::FederateInternalError) override {
    reflected = true;
    reflectedObject = object;
    reflectedValues = values;
    reflectedTag = tag;
    reflectedInfo = info;
  }

  void receiveInteraction(
      rti1516e::InteractionClassHandle interaction,
      rti1516e::ParameterHandleValueMap const& values,
      rti1516e::VariableLengthData const& tag,
      rti1516e::OrderType,
      rti1516e::TransportationType,
      rti1516e::SupplementalReceiveInfo info) throw(rti1516e::FederateInternalError) override {
    received = true;
    receivedInteraction = interaction;
    receivedValues = values;
    receivedTag = tag;
    receivedInfo = info;
  }

  void informAttributeOwnership(
      rti1516e::ObjectInstanceHandle object,
      rti1516e::AttributeHandle attribute,
      rti1516e::FederateHandle owner) throw(rti1516e::FederateInternalError) override {
    ownershipReported = true;
    ownershipObject = object;
    ownershipAttribute = attribute;
    ownershipFederate = owner;
  }

  bool discovered = false;
  bool reflected = false;
  rti1516e::ObjectInstanceHandle discoveredObject;
  rti1516e::ObjectClassHandle discoveredClass;
  rti1516e::FederateHandle discoveredFederate;
  std::wstring discoveredName;
  rti1516e::ObjectInstanceHandle reflectedObject;
  rti1516e::AttributeHandleValueMap reflectedValues;
  rti1516e::VariableLengthData reflectedTag;
  rti1516e::SupplementalReflectInfo reflectedInfo;
  bool received = false;
  rti1516e::InteractionClassHandle receivedInteraction;
  rti1516e::ParameterHandleValueMap receivedValues;
  rti1516e::VariableLengthData receivedTag;
  rti1516e::SupplementalReceiveInfo receivedInfo;
  bool ownershipReported = false;
  rti1516e::ObjectInstanceHandle ownershipObject;
  rti1516e::AttributeHandle ownershipAttribute;
  rti1516e::FederateHandle ownershipFederate;
};

}  // namespace

int main() {
  std::wstring const referenceObjectClass = L"HLAobjectRoot.Reference";
  std::wstring const referenceInteractionClass =
      L"HLAinteractionRoot.Reference";
  std::wstring const referenceAttribute = L"Payload";
  std::wstring const referenceParameter = L"Payload";
  assert(rti1516e::rtiName() == L"Umbra IEEE 1516.1-2010 reference RTI");
  assert(rti1516e::rtiVersion() == L"0.2.0-reference");

  rti1516e::RTIambassadorFactory factory;
  std::auto_ptr<rti1516e::RTIambassador> ambassador =
      factory.createRTIambassador();
  RecordingFederateAmbassador federate;
  try {
    ambassador->disconnect();
    assert(false);
  } catch (rti1516e::RTIinternalError const& error) {
    assert(error.what() ==
           L"Umbra IEEE 1516.1-2010 binding is not connected.");
  }
  ambassador->connect(federate, rti1516e::HLA_IMMEDIATE);
  try {
    ambassador->connect(federate, rti1516e::HLA_IMMEDIATE);
    assert(false);
  } catch (rti1516e::AlreadyConnected const& error) {
    assert(error.what() ==
           L"Umbra IEEE 1516.1-2010 binding is already connected.");
  }
  ambassador->disconnect();

  std::auto_ptr<rti1516e::RTIambassador> publisher = factory.createRTIambassador();
  std::auto_ptr<rti1516e::RTIambassador> subscriber = factory.createRTIambassador();
  RecordingFederateAmbassador publisherFederate;
  RecordingFederateAmbassador subscriberFederate;
  publisher->connect(publisherFederate, rti1516e::HLA_IMMEDIATE);
  subscriber->connect(subscriberFederate, rti1516e::HLA_IMMEDIATE);
  std::wstring const federation = L"umbra-2010-reference-shell";
  publisher->createFederationExecution(federation, L"ignored-fom.xml");
  rti1516e::FederateHandle publisherHandle =
      publisher->joinFederationExecution(L"publisher", L"reference", federation);
  subscriber->joinFederationExecution(L"subscriber", L"reference", federation);
  rti1516e::ObjectClassHandle objectClass =
      publisher->getObjectClassHandle(referenceObjectClass);
  rti1516e::AttributeHandle attribute =
      publisher->getAttributeHandle(objectClass, referenceAttribute);
  rti1516e::AttributeHandleSet attributes;
  attributes.insert(attribute);
  publisher->publishObjectClassAttributes(objectClass, attributes);
  subscriber->subscribeObjectClassAttributes(objectClass, attributes);
  rti1516e::ObjectInstanceHandle object = publisher->registerObjectInstance(objectClass);
  assert(subscriberFederate.discovered);
  assert(subscriberFederate.discoveredObject == object);
  assert(subscriberFederate.discoveredClass == objectClass);
  assert(subscriberFederate.discoveredFederate == publisherHandle);
  assert(!subscriberFederate.discoveredName.empty());
  unsigned char payload[] = {0x01, 0x02, 0x03};
  unsigned char tagBytes[] = {0x7f};
  rti1516e::AttributeHandleValueMap values;
  values.emplace(attribute, rti1516e::VariableLengthData(payload, sizeof(payload)));
  publisher->updateAttributeValues(
      object, values, rti1516e::VariableLengthData(tagBytes, sizeof(tagBytes)));
  assert(subscriberFederate.reflected);
  assert(subscriberFederate.reflectedObject == object);
  assert(subscriberFederate.reflectedValues.size() == 1);
  assert(subscriberFederate.reflectedValues.begin()->second.size() == sizeof(payload));
  assert(subscriberFederate.reflectedTag.size() == sizeof(tagBytes));
  assert(subscriberFederate.reflectedInfo.hasProducingFederate);
  assert(subscriberFederate.reflectedInfo.producingFederate == publisherHandle);
  subscriber->queryAttributeOwnership(object, attribute);
  assert(subscriberFederate.ownershipReported);
  assert(subscriberFederate.ownershipObject == object);
  assert(subscriberFederate.ownershipAttribute == attribute);
  assert(subscriberFederate.ownershipFederate == publisherHandle);
  assert(publisher->getFederateHandle(L"publisher") == publisherHandle);
  assert(publisher->getFederateName(publisherHandle) == L"publisher");
  assert(publisher->isAttributeOwnedByFederate(object, attribute));
  assert(!subscriber->isAttributeOwnedByFederate(object, attribute));
  subscriber->unsubscribeObjectClassAttributes(objectClass, attributes);
  publisher->unpublishObjectClassAttributes(objectClass, attributes);

  rti1516e::InteractionClassHandle interactionClass =
      publisher->getInteractionClassHandle(referenceInteractionClass);
  rti1516e::ParameterHandle parameter =
      publisher->getParameterHandle(interactionClass, referenceParameter);
  assert(publisher->getInteractionClassName(interactionClass) ==
         referenceInteractionClass);
  assert(publisher->getParameterName(interactionClass, parameter) ==
         referenceParameter);
  publisher->publishInteractionClass(interactionClass);
  subscriber->subscribeInteractionClass(interactionClass);
  unsigned char interactionPayload[] = {0x05, 0x06, 0x07};
  unsigned char interactionTag[] = {0x0a, 0x0b};
  rti1516e::ParameterHandleValueMap interactionValues;
  interactionValues.emplace(
      parameter,
      rti1516e::VariableLengthData(interactionPayload, sizeof(interactionPayload)));
  publisher->sendInteraction(
      interactionClass,
      interactionValues,
      rti1516e::VariableLengthData(interactionTag, sizeof(interactionTag)));
  assert(subscriberFederate.received);
  assert(subscriberFederate.receivedInteraction == interactionClass);
  assert(subscriberFederate.receivedValues.size() == 1);
  assert(subscriberFederate.receivedValues.begin()->second.size() ==
         sizeof(interactionPayload));
  assert(subscriberFederate.receivedTag.size() == sizeof(interactionTag));
  assert(subscriberFederate.receivedInfo.hasProducingFederate);
  assert(subscriberFederate.receivedInfo.producingFederate == publisherHandle);
  subscriber->unsubscribeInteractionClass(interactionClass);
  publisher->unpublishInteractionClass(interactionClass);
  subscriber->resignFederationExecution(rti1516e::NO_ACTION);
  publisher->resignFederationExecution(rti1516e::NO_ACTION);
  subscriber->disconnect();
  publisher->disconnect();
  publisher->connect(publisherFederate, rti1516e::HLA_IMMEDIATE);
  publisher->destroyFederationExecution(federation);
  publisher->disconnect();
  return 0;
}
