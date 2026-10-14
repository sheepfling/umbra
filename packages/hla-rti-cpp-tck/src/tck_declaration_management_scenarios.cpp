#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioDeclarations(Options const& options, rti::CallbackModel model) {
  Session declarationLifecycle(options, model, "declaration-lifecycle");
  rti::ObjectClassHandle const invalidObjectClass;
  rti::AttributeHandleSet const emptyAttributes;
  rti::InteractionClassHandle const invalidInteractionClass;
  auto requireDeclarationLifecycle = [&](std::wstring const& expected,
                                          std::string const& phase) {
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().publishObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "publishing object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unpublishObjectClass(
              invalidObjectClass);
        },
        expected,
        "unpublishing an object class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unpublishObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "unpublishing object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().subscribeObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "subscribing to object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unsubscribeObjectClass(
              invalidObjectClass);
        },
        expected,
        "unsubscribing an object class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unsubscribeObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "unsubscribing object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().publishInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "publishing an interaction class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unpublishInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "unpublishing an interaction class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().subscribeInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "subscribing to an interaction class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unsubscribeInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "unsubscribing an interaction class " + phase);
  };
  requireDeclarationLifecycle(L"NotConnected", "before connect");
  declarationLifecycle.connect();
  requireDeclarationLifecycle(L"FederateNotExecutionMember", "after connect before join");
  declarationLifecycle.disconnect();

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  Session passive(options, model, "passive");
  auto const federation = federationName(options, "declarations");
  connectAndJoin(owner, member, options, federation, options.fom);
  passive.connect();
  passive.join(
      options.memberFederateName + L"-passive",
      options.federateType,
      federation);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  rti::AttributeHandleSet const invalidAttributes{rti::AttributeHandle{}};
  requireException(
      [&] {
        owner.rtiAmbassador().publishObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "publishing attributes for an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().publishObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "publishing an invalid object attribute");
  requireException(
      [&] { owner.rtiAmbassador().unpublishObjectClass(invalidObjectClass); },
      L"ObjectClassNotDefined",
      "unpublishing an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().unpublishObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "unpublishing attributes for an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().unpublishObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "unpublishing an invalid object attribute");
  requireException(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "subscribing to an invalid object class");
  requireException(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "subscribing to an invalid object attribute");
  requireException(
      [&] { member.rtiAmbassador().unsubscribeObjectClass(invalidObjectClass); },
      L"ObjectClassNotDefined",
      "unsubscribing from an invalid object class");
  requireException(
      [&] {
        member.rtiAmbassador().unsubscribeObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "unsubscribing attributes from an invalid object class");
  requireException(
      [&] {
        member.rtiAmbassador().unsubscribeObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "unsubscribing an invalid object attribute");

  requireException(
      [&] { owner.rtiAmbassador().publishInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "publishing an invalid interaction class");
  requireException(
      [&] { owner.rtiAmbassador().unpublishInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "unpublishing an invalid interaction class");
  requireException(
      [&] { member.rtiAmbassador().subscribeInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "subscribing to an invalid interaction class");
  requireException(
      [&] { member.rtiAmbassador().unsubscribeInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "unsubscribing from an invalid interaction class");

  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  auto const passiveClass = passive.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const passiveAttribute =
      passive.rtiAmbassador().getAttributeHandle(passiveClass, options.attributeName);
  require(passiveClass.isValid() && passiveAttribute.isValid(),
          "Passive declaration handle lookup returned an invalid handle");
  rti::AttributeHandleSet passiveAttributes;
  passiveAttributes.insert(passiveAttribute);
  passive.rtiAmbassador().subscribeObjectClassAttributes(
      passiveClass, passiveAttributes, false, L"");
  owner.rtiAmbassador().publishInteractionClass(interactionClass);
  member.rtiAmbassador().subscribeInteractionClass(interactionClass, true);
  auto const passiveInteraction =
      passive.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  require(passiveInteraction.isValid(),
          "Passive declaration interaction lookup returned an invalid handle");
  passive.rtiAmbassador().subscribeInteractionClass(passiveInteraction, false);

  member.rtiAmbassador().unsubscribeObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().unsubscribeObjectClass(objectClass);
  member.rtiAmbassador().unsubscribeInteractionClass(interactionClass);
  passive.rtiAmbassador().unsubscribeObjectClassAttributes(passiveClass, passiveAttributes);
  passive.rtiAmbassador().unsubscribeObjectClass(passiveClass);
  passive.rtiAmbassador().unsubscribeInteractionClass(passiveInteraction);
  owner.rtiAmbassador().unpublishObjectClassAttributes(objectClass, attributes);
  owner.rtiAmbassador().unpublishObjectClass(objectClass);
  owner.rtiAmbassador().unpublishInteractionClass(interactionClass);
  passive.resign(rti::NO_ACTION);
}

} // namespace hla_rti_cpp_tck::detail
