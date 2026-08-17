#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <RTI/RTI1516.h>

TEST_CASE("IEEE 1516.1-2025 headers expose the expected API version", "[baseline][headers]") {
  REQUIRE(HLA_API_MAJOR_VERSION == 2);
  REQUIRE(HLA_API_MINOR_VERSION == 0);
}

TEST_CASE(
    "Selected 2025 exception values retain their official identities",
    "[baseline][support-types]") {
  rti1516_2025::NameNotFound missing(L"missing federate");
  rti1516_2025::InvalidFederateHandle invalid(L"invalid federate handle");
  rti1516_2025::FederateHandleNotKnown unknown(L"unknown federate handle");
  rti1516_2025::InvalidObjectClassHandle invalidObjectClass(L"invalid object class handle");
  rti1516_2025::ObjectClassNotDefined objectClassNotDefined(L"missing object class");
  rti1516_2025::ObjectClassNotPublished objectClassNotPublished(L"unpublished object class");
  rti1516_2025::ObjectInstanceNotKnown objectInstanceNotKnown(L"unknown object instance");
  rti1516_2025::DeletePrivilegeNotHeld deletePrivilegeNotHeld(L"missing delete privilege");
  rti1516_2025::InvalidInteractionClassHandle invalidInteractionClass(
      L"invalid interaction class handle");
  rti1516_2025::InteractionClassNotDefined interactionClassNotDefined(
      L"missing interaction class");
  rti1516_2025::InteractionClassNotPublished interactionClassNotPublished(
      L"unpublished interaction class");
  rti1516_2025::AttributeNotDefined attributeNotDefined(L"missing attribute");
  rti1516_2025::AttributeNotOwned attributeNotOwned(L"unowned attribute");
  rti1516_2025::InvalidAttributeHandle invalidAttribute(L"invalid attribute handle");
  rti1516_2025::InteractionParameterNotDefined parameterNotDefined(L"missing parameter");
  rti1516_2025::InvalidParameterHandle invalidParameter(L"invalid parameter handle");
  rti1516_2025::InvalidTransportationName invalidTransportationName(
      L"invalid transportation name");
  rti1516_2025::InvalidTransportationTypeHandle invalidTransportationType(
      L"invalid transportation handle");

  REQUIRE(missing.name() == L"NameNotFound");
  REQUIRE(missing.what() == L"missing federate");
  REQUIRE(invalid.name() == L"InvalidFederateHandle");
  REQUIRE(invalid.what() == L"invalid federate handle");
  REQUIRE(unknown.name() == L"FederateHandleNotKnown");
  REQUIRE(unknown.what() == L"unknown federate handle");
  REQUIRE(invalidObjectClass.name() == L"InvalidObjectClassHandle");
  REQUIRE(invalidObjectClass.what() == L"invalid object class handle");
  REQUIRE(objectClassNotDefined.name() == L"ObjectClassNotDefined");
  REQUIRE(objectClassNotDefined.what() == L"missing object class");
  REQUIRE(objectClassNotPublished.name() == L"ObjectClassNotPublished");
  REQUIRE(objectClassNotPublished.what() == L"unpublished object class");
  REQUIRE(objectInstanceNotKnown.name() == L"ObjectInstanceNotKnown");
  REQUIRE(objectInstanceNotKnown.what() == L"unknown object instance");
  REQUIRE(deletePrivilegeNotHeld.name() == L"DeletePrivilegeNotHeld");
  REQUIRE(deletePrivilegeNotHeld.what() == L"missing delete privilege");
  REQUIRE(invalidInteractionClass.name() == L"InvalidInteractionClassHandle");
  REQUIRE(invalidInteractionClass.what() == L"invalid interaction class handle");
  REQUIRE(interactionClassNotDefined.name() == L"InteractionClassNotDefined");
  REQUIRE(interactionClassNotDefined.what() == L"missing interaction class");
  REQUIRE(interactionClassNotPublished.name() == L"InteractionClassNotPublished");
  REQUIRE(interactionClassNotPublished.what() == L"unpublished interaction class");
  REQUIRE(attributeNotDefined.name() == L"AttributeNotDefined");
  REQUIRE(attributeNotDefined.what() == L"missing attribute");
  REQUIRE(attributeNotOwned.name() == L"AttributeNotOwned");
  REQUIRE(attributeNotOwned.what() == L"unowned attribute");
  REQUIRE(invalidAttribute.name() == L"InvalidAttributeHandle");
  REQUIRE(invalidAttribute.what() == L"invalid attribute handle");
  REQUIRE(parameterNotDefined.name() == L"InteractionParameterNotDefined");
  REQUIRE(parameterNotDefined.what() == L"missing parameter");
  REQUIRE(invalidParameter.name() == L"InvalidParameterHandle");
  REQUIRE(invalidParameter.what() == L"invalid parameter handle");
  REQUIRE(invalidTransportationName.name() == L"InvalidTransportationName");
  REQUIRE(invalidTransportationName.what() == L"invalid transportation name");
  REQUIRE(invalidTransportationType.name() == L"InvalidTransportationTypeHandle");
  REQUIRE(invalidTransportationType.what() == L"invalid transportation handle");
}

TEST_CASE("The binding shell routes profile-gated federation listing explicitly", "[baseline][binding-shell]") {
  rti1516_2025::RTIambassadorFactory factory;
  std::unique_ptr<rti1516_2025::RTIambassador> rti = factory.createRTIambassador();

  REQUIRE(rti);
  REQUIRE_FALSE(rti1516_2025::rtiName().empty());
  REQUIRE_FALSE(rti1516_2025::rtiVersion().empty());
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  REQUIRE_THROWS_AS(rti->listFederationExecutions(), rti1516_2025::NotConnected);
#else
  REQUIRE_THROWS_AS(rti->listFederationExecutions(), rti1516_2025::RTIinternalError);
#endif
}

TEST_CASE("The binding shell profile-gates FOM and transportation lookup services explicitly", "[baseline][binding-shell]") {
  rti1516_2025::RTIambassadorFactory factory;
  std::unique_ptr<rti1516_2025::RTIambassador> rti = factory.createRTIambassador();
  rti1516_2025::ObjectClassHandle objectClass;
  rti1516_2025::InteractionClassHandle interactionClass;
  rti1516_2025::AttributeHandle attribute;
  rti1516_2025::ParameterHandle parameter;
  rti1516_2025::TransportationTypeHandle transportationType;
  rti1516_2025::AttributeHandleSet attributes;
  rti1516_2025::ParameterHandleValueMap parameterValues;
  rti1516_2025::VariableLengthData userSuppliedTag;

  REQUIRE(rti);
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  REQUIRE_THROWS_AS(rti->getObjectClassHandle(L"HLAobjectRoot"), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getInteractionClassHandle(L"HLAinteractionRoot"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(rti->getAttributeHandle(objectClass, L"Name"), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getParameterHandle(interactionClass, L"Identifier"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getAttributeName(objectClass, attribute),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getParameterName(interactionClass, parameter),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->publishInteractionClass(interactionClass),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->unpublishInteractionClass(interactionClass),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(interactionClass),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->unsubscribeInteractionClass(interactionClass),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->sendInteraction(interactionClass, parameterValues, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getTransportationTypeHandle(L"HLAreliable"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getTransportationTypeName(transportationType),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->publishObjectClassAttributes(objectClass, attributes),
      rti1516_2025::NotConnected);
#else
  REQUIRE_THROWS_AS(
      rti->getObjectClassHandle(L"HLAobjectRoot"),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getInteractionClassHandle(L"HLAinteractionRoot"),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getAttributeHandle(objectClass, L"Name"),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getParameterHandle(interactionClass, L"Identifier"),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getAttributeName(objectClass, attribute),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getParameterName(interactionClass, parameter),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->publishInteractionClass(interactionClass),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->unpublishInteractionClass(interactionClass),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(interactionClass),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->unsubscribeInteractionClass(interactionClass),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->sendInteraction(interactionClass, parameterValues, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getTransportationTypeHandle(L"HLAreliable"),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getTransportationTypeName(transportationType),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->publishObjectClassAttributes(objectClass, attributes),
      rti1516_2025::RTIinternalError);
#endif
}

TEST_CASE(
    "The binding shell profile-gates the initial 2025 object-instance service set explicitly",
    "[baseline][binding-shell][object-management]") {
  rti1516_2025::RTIambassadorFactory factory;
  std::unique_ptr<rti1516_2025::RTIambassador> rti = factory.createRTIambassador();
  rti1516_2025::ObjectClassHandle objectClass;
  rti1516_2025::ObjectInstanceHandle objectInstance;
  rti1516_2025::AttributeHandle attribute;
  rti1516_2025::AttributeHandleSet attributes;
  rti1516_2025::AttributeHandleValueMap attributeValues;
  rti1516_2025::VariableLengthData userSuppliedTag;

  REQUIRE(rti);
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  REQUIRE_THROWS_AS(
      rti->registerObjectInstance(objectClass),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->deleteObjectInstance(objectInstance, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->updateAttributeValues(objectInstance, attributeValues, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->requestAttributeValueUpdate(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->requestAttributeValueUpdate(objectClass, attributes, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->queryAttributeOwnership(objectInstance, attributes),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->isAttributeOwnedByFederate(objectInstance, attribute),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          attributes,
          userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->confirmDivestiture(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, attributes),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          attributes,
          userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipAcquisition(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipAcquisitionIfAvailable(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipReleaseDenied(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          attributes,
          userSuppliedTag,
          attributes),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->cancelAttributeOwnershipAcquisition(objectInstance, attributes),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getObjectInstanceHandle(L"unknown-instance"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->getObjectInstanceName(objectInstance),
      rti1516_2025::NotConnected);
#else
  REQUIRE_THROWS_AS(
      rti->registerObjectInstance(objectClass),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->deleteObjectInstance(objectInstance, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->updateAttributeValues(objectInstance, attributeValues, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->requestAttributeValueUpdate(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->requestAttributeValueUpdate(objectClass, attributes, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->queryAttributeOwnership(objectInstance, attributes),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->isAttributeOwnedByFederate(objectInstance, attribute),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          attributes,
          userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->confirmDivestiture(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, attributes),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          attributes,
          userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipAcquisition(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipAcquisitionIfAvailable(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipReleaseDenied(objectInstance, attributes, userSuppliedTag),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          attributes,
          userSuppliedTag,
          attributes),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->cancelAttributeOwnershipAcquisition(objectInstance, attributes),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getObjectInstanceHandle(L"unknown-instance"),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      rti->getObjectInstanceName(objectInstance),
      rti1516_2025::RTIinternalError);
#endif
}
