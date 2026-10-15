#include "tck_post_resignation_valid_input_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioStandardOrderTransportLookupsAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Post-resignation standard order/transport lookup requires an adapter-supplied FOM");

  Session subject(options, model, "standard-order-transport-resigned-subject");
  auto const federation =
      federationName(options, "standard-order-transport-lookups-after-resignation");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-standard-order-transport-subject",
      options.federateType,
      federation);

  auto const receiveOrder = subject.rtiAmbassador().getOrderType(L"Receive");
  auto const timestampOrder = subject.rtiAmbassador().getOrderType(L"TimeStamp");
  auto const reliable = subject.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const bestEffort = subject.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  require(
      receiveOrder == rti::RECEIVE && timestampOrder == rti::TIMESTAMP &&
          reliable.isValid() && bestEffort.isValid() && reliable != bestEffort,
      "Mandatory standard order/transport lookup returned unexpected values");
  require(
      subject.rtiAmbassador().getOrderName(receiveOrder) == L"Receive" &&
          subject.rtiAmbassador().getOrderName(timestampOrder) == L"TimeStamp" &&
          subject.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable" &&
          subject.rtiAmbassador().getTransportationTypeName(bestEffort) ==
              L"HLAbestEffort",
      "Mandatory standard order/transport lookup did not round-trip while joined");

  subject.resign(rti::NO_ACTION);
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getOrderType(L"Receive"));
      },
      L"FederateNotExecutionMember",
      "looking up the valid Receive order type after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getOrderName(receiveOrder));
      },
      L"FederateNotExecutionMember",
      "looking up the valid Receive order name after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getOrderType(L"TimeStamp"));
      },
      L"FederateNotExecutionMember",
      "looking up the valid TimeStamp order type after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getOrderName(timestampOrder));
      },
      L"FederateNotExecutionMember",
      "looking up the valid TimeStamp order name after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getTransportationTypeHandle(
            L"HLAreliable"));
      },
      L"FederateNotExecutionMember",
      "looking up the valid reliable transportation handle after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getTransportationTypeName(
            reliable));
      },
      L"FederateNotExecutionMember",
      "looking up the valid reliable transportation name after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getTransportationTypeHandle(
            L"HLAbestEffort"));
      },
      L"FederateNotExecutionMember",
      "looking up the valid best-effort transportation handle after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getTransportationTypeName(
            bestEffort));
      },
      L"FederateNotExecutionMember",
      "looking up the valid best-effort transportation name after resignation");

  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioStandardOrderTransportLookupsAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioStandardOrderTransportLookupsAfterResignationPortable(options, model);
}

void scenarioObjectAttributeDeclarationsValidInputsAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Post-resignation object declaration testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty(),
      "Post-resignation object declaration testing requires adapter-supplied class and attribute names");

  Session subject(options, model, "object-attribute-declarations-resigned-subject");
  auto const federation = federationName(
      options,
      "object-attribute-declarations-valid-inputs-after-resignation");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-object-attribute-declarations-subject",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid() &&
          subject.rtiAmbassador().getObjectClassName(objectClass) ==
              options.objectClassName &&
          subject.rtiAmbassador().getAttributeName(objectClass, attribute) ==
              options.attributeName,
      "Adapter-supplied object/attribute fixture did not provide valid lookup handles");
  rti::AttributeHandleSet const attributes{attribute};

  subject.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  subject.rtiAmbassador().unpublishObjectClassAttributes(objectClass, attributes);
  subject.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  subject.rtiAmbassador().unpublishObjectClass(objectClass);
  subject.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes);
  subject.rtiAmbassador().unsubscribeObjectClassAttributes(objectClass, attributes);
  subject.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes);
  subject.rtiAmbassador().unsubscribeObjectClass(objectClass);

  subject.resign(rti::NO_ACTION);
  requireException(
      [&] {
        subject.rtiAmbassador().publishObjectClassAttributes(
            objectClass,
            attributes);
      },
      L"FederateNotExecutionMember",
      "publishing valid object attributes after resignation");
  requireException(
      [&] { subject.rtiAmbassador().unpublishObjectClass(objectClass); },
      L"FederateNotExecutionMember",
      "unpublishing a valid object class after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().unpublishObjectClassAttributes(
            objectClass,
            attributes);
      },
      L"FederateNotExecutionMember",
      "unpublishing valid object attributes after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().subscribeObjectClassAttributes(
            objectClass,
            attributes);
      },
      L"FederateNotExecutionMember",
      "subscribing to valid object attributes after resignation");
  requireException(
      [&] { subject.rtiAmbassador().unsubscribeObjectClass(objectClass); },
      L"FederateNotExecutionMember",
      "unsubscribing a valid object class after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().unsubscribeObjectClassAttributes(
            objectClass,
            attributes);
      },
      L"FederateNotExecutionMember",
      "unsubscribing valid object attributes after resignation");

  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioObjectAttributeDeclarationsValidInputsAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectAttributeDeclarationsValidInputsAfterResignationPortable(
      options,
      model);
}

void scenarioInteractionDeclarationsValidInputsAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Post-resignation interaction declaration testing requires an adapter-supplied FOM");
  require(
      !options.interactionClassName.empty(),
      "Post-resignation interaction declaration testing requires an adapter-supplied class name");

  Session subject(options, model, "interaction-declarations-resigned-subject");
  auto const federation = federationName(
      options,
      "interaction-declarations-valid-inputs-after-resignation");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-interaction-declarations-subject",
      options.federateType,
      federation);

  auto const interactionClass =
      subject.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  require(
      interactionClass.isValid() &&
          subject.rtiAmbassador().getInteractionClassName(interactionClass) ==
              options.interactionClassName,
      "Adapter-supplied interaction fixture did not provide a valid lookup handle");

  subject.rtiAmbassador().publishInteractionClass(interactionClass);
  subject.rtiAmbassador().unpublishInteractionClass(interactionClass);
  subject.rtiAmbassador().publishInteractionClass(interactionClass);
  subject.rtiAmbassador().unpublishInteractionClass(interactionClass);
  subject.rtiAmbassador().subscribeInteractionClass(interactionClass);
  subject.rtiAmbassador().unsubscribeInteractionClass(interactionClass);
  subject.rtiAmbassador().subscribeInteractionClass(interactionClass);
  subject.rtiAmbassador().unsubscribeInteractionClass(interactionClass);

  subject.resign(rti::NO_ACTION);
  requireException(
      [&] { subject.rtiAmbassador().publishInteractionClass(interactionClass); },
      L"FederateNotExecutionMember",
      "publishing a valid interaction class after resignation");
  requireException(
      [&] { subject.rtiAmbassador().unpublishInteractionClass(interactionClass); },
      L"FederateNotExecutionMember",
      "unpublishing a valid interaction class after resignation");
  requireException(
      [&] { subject.rtiAmbassador().subscribeInteractionClass(interactionClass); },
      L"FederateNotExecutionMember",
      "subscribing to a valid interaction class after resignation");
  requireException(
      [&] { subject.rtiAmbassador().unsubscribeInteractionClass(interactionClass); },
      L"FederateNotExecutionMember",
      "unsubscribing a valid interaction class after resignation");

  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioInteractionDeclarationsValidInputsAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionDeclarationsValidInputsAfterResignationPortable(
      options,
      model);
}

void scenarioDirectedInteractionDeclarationsValidInputsAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Post-resignation directed-interaction declaration testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.interactionClassName.empty(),
      "Post-resignation directed-interaction declaration testing requires adapter-supplied object and interaction class names");

  Session subject(options, model, "directed-interaction-declarations-resigned-subject");
  auto const federation = federationName(
      options,
      "directed-interaction-declarations-valid-inputs-after-resignation");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-directed-interaction-declarations-subject",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const interactionClass =
      subject.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  require(
      objectClass.isValid() && interactionClass.isValid() &&
          subject.rtiAmbassador().getObjectClassName(objectClass) ==
              options.objectClassName &&
          subject.rtiAmbassador().getInteractionClassName(interactionClass) ==
              options.interactionClassName,
      "Adapter-supplied directed-interaction FOM fixture did not provide valid class lookup handles");

  rti::InteractionClassHandleSet const interactions{interactionClass};
  subject.rtiAmbassador().publishObjectClassDirectedInteractions(
      objectClass,
      interactions);
  subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      objectClass,
      interactions);
  subject.rtiAmbassador().publishObjectClassDirectedInteractions(
      objectClass,
      interactions);
  subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(objectClass);
  subject.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      objectClass,
      interactions,
      false);
  subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      objectClass,
      interactions);
  subject.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      objectClass,
      interactions,
      false);
  subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(objectClass);

  subject.resign(rti::NO_ACTION);
  requireException(
      [&] {
        subject.rtiAmbassador().publishObjectClassDirectedInteractions(
            objectClass,
            interactions);
      },
      L"FederateNotExecutionMember",
      "publishing valid class-scoped directed interactions after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(
            objectClass,
            interactions);
      },
      L"FederateNotExecutionMember",
      "unpublishing a valid directed-interaction set after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(
            objectClass);
      },
      L"FederateNotExecutionMember",
      "unpublishing a valid object class for directed interactions after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().subscribeObjectClassDirectedInteractions(
            objectClass,
            interactions,
            false);
      },
      L"FederateNotExecutionMember",
      "subscribing to valid class-scoped directed interactions after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
            objectClass,
            interactions);
      },
      L"FederateNotExecutionMember",
      "unsubscribing from a valid directed-interaction set after resignation");
  requireException(
      [&] {
        subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
            objectClass);
      },
      L"FederateNotExecutionMember",
      "unsubscribing a valid object class from directed interactions after resignation");

  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioDirectedInteractionDeclarationsValidInputsAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDirectedInteractionDeclarationsValidInputsAfterResignationPortable(
      options,
      model);
}

void scenarioInteractionTransportationQueryValidInputsAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.interactionClassName.empty(),
      "Post-resignation interaction transportation query testing requires an adapter-supplied FOM and interaction class");

  Session requester(options, model, "interaction-transportation-query-requester");
  Session publisher(options, model, "interaction-transportation-query-publisher");
  auto const federation = federationName(
      options,
      "interaction-transportation-query-valid-inputs-after-resignation");
  requester.connect();
  publisher.connect();
  requester.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  requester.join(
      options.ownerFederateName + L"-transportation-query-requester",
      options.federateType,
      federation);
  publisher.join(
      options.ownerFederateName + L"-transportation-query-publisher",
      options.federateType,
      federation);

  auto const interactionClass =
      requester.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherInteractionClass =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherFederate = publisher.federateHandle();
  require(
      interactionClass.isValid() && publisherInteractionClass.isValid() &&
          requester.rtiAmbassador().getInteractionClassName(interactionClass) ==
              options.interactionClassName &&
          publisher.rtiAmbassador().getInteractionClassName(
              publisherInteractionClass) == options.interactionClassName &&
          publisherFederate.isValid(),
      "Adapter-supplied interaction transportation query fixture did not provide valid class and peer handles");

  publisher.rtiAmbassador().publishInteractionClass(
      publisherInteractionClass);
  requester.rtiAmbassador().queryInteractionTransportationType(
      publisherFederate,
      interactionClass);
  waitFor(
      requester,
      [&] {
        return !requester.recorder().interactionTransportationReports().empty();
      },
      options,
      "valid interaction transportation query report");
  auto const reports = requester.recorder().interactionTransportationReports();
  auto const report = std::find_if(
      reports.begin(),
      reports.end(),
      [&](auto const& value) {
        return value.federate == publisherFederate &&
            value.interaction == interactionClass;
      });
  require(
      report != reports.end() && report->transportation.isValid() &&
          !requester.rtiAmbassador().getTransportationTypeName(
              report->transportation).empty(),
      "Valid interaction transportation query did not report the active peer and adapter-FOM class with a resolvable standard type");

  requester.resign(rti::NO_ACTION);
  requireException(
      [&] {
        requester.rtiAmbassador().queryInteractionTransportationType(
            publisherFederate,
            interactionClass);
      },
      L"FederateNotExecutionMember",
      "querying transportation for a valid active peer and interaction class after resignation");

  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherInteractionClass);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  publisher.disconnect();
}

void scenarioInteractionTransportationQueryValidInputsAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionTransportationQueryValidInputsAfterResignationPortable(
      options,
      model);
}

}  // namespace hla_rti_cpp_tck::detail
