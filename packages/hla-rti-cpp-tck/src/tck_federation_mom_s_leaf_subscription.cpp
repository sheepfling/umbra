#include "tck_main_scenarios.hpp"

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

constexpr auto parameterRequiredTable7SLeaves = std::to_array<std::wstring_view>({
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeInteractionOrderType",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdeleteObjectInstance",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableAsynchronousDelivery",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableAsynchronousDelivery",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeConstrained",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeRegulation",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeConstrained",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeRegulation",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateRestoreComplete",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateSaveComplete",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAflushQueueRequest",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAlocalDeleteObjectInstance",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAmodifyLookahead",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequest",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequestAvailable",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestAttributeTransportationTypeChange",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestInteractionTransportationTypeChange",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAresignFederationExecution",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsynchronizationPointAchieved",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequest",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequestAvailable",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunconditionalAttributeOwnershipDivestiture",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches",
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData",
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus",
});

constexpr std::wstring_view requestMimDataLeaf =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata";
constexpr std::wstring_view requestSynchronizationPointsLeaf =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPoints";
static_assert(parameterRequiredTable7SLeaves.size() == 54U);

bool hasInteraction(
    Session const& observer,
    rti::InteractionClassHandle const& expected) {
  auto const reports = observer.recorder().interactions();
  return std::any_of(reports.begin(), reports.end(), [&](auto const& report) {
    return report.present && report.interaction == expected;
  });
}

void requireReportMetadata(
    InteractionRecord const& report,
    rti::InteractionClassHandle const& expectedClass,
    rti::TransportationTypeHandle const& reliable,
    std::string const& description) {
  require(
      report.present && report.interaction == expectedClass &&
          report.tag.empty() && report.transportation == reliable &&
          !report.producer.isValid() && !report.regions.has_value(),
      description + " returned non-standard MOM interaction metadata");
}

void requireNoTimestampedReports(
    Session const& observer, std::string const& description) {
  require(
      observer.recorder().timedInteractions().empty(),
      description + " was delivered through a timestamped interaction callback");
}

}  // namespace

void scenarioFederationMomSLeafSubscription(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM Table 7 S-leaf coverage requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session subject(options, model, "federation-mom-s-leaf-subject");
  Session observer(options, model, "federation-mom-s-leaf-observer");
  auto const federation = federationName(options, "federation-mom-s-leaf-subscription");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName = options.ownerFederateName + L"-mom-s-leaf-subject";
  auto const observerName = options.memberFederateName + L"-mom-s-leaf-observer";
  subject.join(subjectName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);

  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& observerAmbassador = observer.rtiAmbassador();
  auto const exceptionReportClass = observerAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
  auto const exceptionFederateParameter = observerAmbassador.getParameterHandle(
      exceptionReportClass, L"HLAfederate");
  auto const exceptionServiceParameter = observerAmbassador.getParameterHandle(
      exceptionReportClass, L"HLAservice");
  auto const exceptionTextParameter = observerAmbassador.getParameterHandle(
      exceptionReportClass, L"HLAexception");
  auto const parameterErrorParameter = observerAmbassador.getParameterHandle(
      exceptionReportClass, L"HLAparameterError");
  auto const reliable = observerAmbassador.getTransportationTypeHandle(L"HLAreliable");
  auto const subjectFederateHandle = observerAmbassador.getFederateHandle(subjectName);
  require(
      exceptionReportClass.isValid() && exceptionFederateParameter.isValid() &&
          exceptionServiceParameter.isValid() && exceptionTextParameter.isValid() &&
          parameterErrorParameter.isValid() && reliable.isValid() &&
          subjectFederateHandle.isValid(),
      "Table 7 S-leaf scenario could not resolve standard MOM exception-report handles");
  observerAmbassador.subscribeInteractionClass(exceptionReportClass, true);

  std::set<std::wstring> expectedServices;
  std::vector<rti::InteractionClassHandle> publishedLeaves;
  std::vector<Session*> const sessions{&subject, &observer};
  auto const targetFederate = subject.federateHandle();
  auto const subjectTime = makeTimeContext(subject);
  auto const disableAsyncClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableAsynchronousDelivery");
  auto const disableAsyncFederateParameter = subjectAmbassador.getParameterHandle(
      disableAsyncClass, L"HLAfederate");
  auto const enableAsyncClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableAsynchronousDelivery");
  auto const enableAsyncFederateParameter = subjectAmbassador.getParameterHandle(
      enableAsyncClass, L"HLAfederate");
  auto const disableTimeConstrainedClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeConstrained");
  auto const disableTimeConstrainedFederateParameter =
      subjectAmbassador.getParameterHandle(
          disableTimeConstrainedClass, L"HLAfederate");
  auto const disableTimeRegulationClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeRegulation");
  auto const disableTimeRegulationFederateParameter =
      subjectAmbassador.getParameterHandle(
          disableTimeRegulationClass, L"HLAfederate");
  auto const enableTimeConstrainedClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeConstrained");
  auto const enableTimeConstrainedFederateParameter =
      subjectAmbassador.getParameterHandle(
          enableTimeConstrainedClass, L"HLAfederate");
  auto const enableTimeRegulationClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeRegulation");
  auto const enableTimeRegulationFederateParameter =
      subjectAmbassador.getParameterHandle(
          enableTimeRegulationClass, L"HLAfederate");
  auto const enableTimeRegulationLookaheadParameter =
      subjectAmbassador.getParameterHandle(
          enableTimeRegulationClass, L"HLAlookahead");
  auto const flushQueueRequestClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAflushQueueRequest");
  auto const flushQueueRequestTimeStampParameter = subjectAmbassador.getParameterHandle(
      flushQueueRequestClass, L"HLAtimeStamp");
  auto const localDeleteObjectInstanceClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAlocalDeleteObjectInstance");
  auto const localDeleteObjectInstanceParameter = subjectAmbassador.getParameterHandle(
      localDeleteObjectInstanceClass, L"HLAobjectInstance");
  auto const modifyLookaheadClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAmodifyLookahead");
  auto const modifyLookaheadParameter = subjectAmbassador.getParameterHandle(
      modifyLookaheadClass, L"HLAlookahead");
  auto const nextMessageRequestClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequest");
  auto const nextMessageRequestTimeStampParameter = subjectAmbassador.getParameterHandle(
      nextMessageRequestClass, L"HLAtimeStamp");
  auto const nextMessageRequestAvailableClass =
      subjectAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequestAvailable");
  auto const nextMessageRequestAvailableTimeStampParameter =
      subjectAmbassador.getParameterHandle(
          nextMessageRequestAvailableClass, L"HLAtimeStamp");
  auto const publishInteractionClassRequestClass =
      subjectAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishInteractionClass");
  auto const publishInteractionClassParameter =
      subjectAmbassador.getParameterHandle(
          publishInteractionClassRequestClass, L"HLAinteractionClass");
  auto const interactionClassToPublish = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.TckInteraction");
  auto const publishObjectClassAttributesRequestClass =
      subjectAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassAttributes");
  auto const publishObjectClassAttributesObjectClassParameter =
      subjectAmbassador.getParameterHandle(
          publishObjectClassAttributesRequestClass, L"HLAobjectClass");
  auto const publishObjectClassAttributesAttributeListParameter =
      subjectAmbassador.getParameterHandle(
          publishObjectClassAttributesRequestClass, L"HLAattributeList");
  auto const objectClassToPublish =
      subjectAmbassador.getObjectClassHandle(options.objectClassName);
  auto const directedInteractionObjectClass =
      subjectAmbassador.getObjectClassHandle(
          L"HLAobjectRoot.TckObject");
  auto const attributeToPublish = subjectAmbassador.getAttributeHandle(
      objectClassToPublish, options.attributeName);
  auto const publishDirectedInteractionsRequestClass =
      subjectAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassDirectedInteractions");
  auto const publishDirectedInteractionsObjectClassParameter =
      subjectAmbassador.getParameterHandle(
          publishDirectedInteractionsRequestClass, L"HLAobjectClass");
  auto const publishDirectedInteractionsClassListParameter =
      subjectAmbassador.getParameterHandle(
          publishDirectedInteractionsRequestClass, L"HLAinteractionClassList");
  require(
      targetFederate.isValid() && disableAsyncClass.isValid() &&
          disableAsyncFederateParameter.isValid() && enableAsyncClass.isValid() &&
          enableAsyncFederateParameter.isValid() &&
          disableTimeConstrainedClass.isValid() &&
          disableTimeConstrainedFederateParameter.isValid() &&
          disableTimeRegulationClass.isValid() &&
          disableTimeRegulationFederateParameter.isValid() &&
          enableTimeConstrainedClass.isValid() &&
          enableTimeConstrainedFederateParameter.isValid() &&
          enableTimeRegulationClass.isValid() &&
          enableTimeRegulationFederateParameter.isValid() &&
          enableTimeRegulationLookaheadParameter.isValid() &&
          flushQueueRequestClass.isValid() &&
          flushQueueRequestTimeStampParameter.isValid() &&
          localDeleteObjectInstanceClass.isValid() &&
          localDeleteObjectInstanceParameter.isValid() &&
          modifyLookaheadClass.isValid() &&
          modifyLookaheadParameter.isValid() &&
          nextMessageRequestClass.isValid() &&
          nextMessageRequestTimeStampParameter.isValid() &&
          nextMessageRequestAvailableClass.isValid() &&
          nextMessageRequestAvailableTimeStampParameter.isValid() &&
          publishInteractionClassRequestClass.isValid() &&
          publishInteractionClassParameter.isValid() &&
          interactionClassToPublish.isValid() &&
          publishObjectClassAttributesRequestClass.isValid() &&
          publishObjectClassAttributesObjectClassParameter.isValid() &&
          publishObjectClassAttributesAttributeListParameter.isValid() &&
          objectClassToPublish.isValid() && attributeToPublish.isValid() &&
          directedInteractionObjectClass.isValid() &&
          publishDirectedInteractionsRequestClass.isValid() &&
          publishDirectedInteractionsObjectClassParameter.isValid() &&
          publishDirectedInteractionsClassListParameter.isValid() &&
          subjectTime.epsilon != nullptr,
      "MOM service interaction handles were invalid");
  for (auto const leafName : parameterRequiredTable7SLeaves) {
    auto const interactionName = std::wstring{leafName};
    auto const leaf = subjectAmbassador.getInteractionClassHandle(interactionName);
    require(leaf.isValid(), "Standard MIM omitted Table 7 S interaction " +
                                std::string(interactionName.begin(), interactionName.end()));
    // Table 7's S marks the RTI's subscription. The sender still has to meet
    // Send Interaction's publication precondition; otherwise its exception
    // report would test only that precondition, not RTI-side MOM consumption.
    subjectAmbassador.publishInteractionClass(leaf);
    publishedLeaves.push_back(leaf);
    expectedServices.emplace(interactionName);
    bool rejected = false;
    try {
      subjectAmbassador.sendInteraction(
          leaf,
          rti::ParameterHandleValueMap{},
          rti::VariableLengthData{});
    } catch (...) {
      rejected = true;
    }
    require(
        rejected,
        "A parameter-bearing Table 7 S request with no arguments was accepted: " +
            std::string(interactionName.begin(), interactionName.end()));
    require(
        rejected,
        "A parameter-bearing Table 7 S request unexpectedly returned normally: " +
            std::string(interactionName.begin(), interactionName.end()));

    if (interactionName ==
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdeleteObjectInstance") {
      auto const objectClass =
          subjectAmbassador.getObjectClassHandle(options.objectClassName);
      auto const attribute = subjectAmbassador.getAttributeHandle(
          objectClass, options.attributeName);
      require(
          objectClass.isValid() && attribute.isValid(),
          "Delete Object Instance MOM test could not resolve its adapter FOM object class and attribute");
      rti::AttributeHandleSet const publishedAttributes{attribute};
      subjectAmbassador.publishObjectClassAttributes(
          objectClass, publishedAttributes);
      auto const object = subjectAmbassador.registerObjectInstance(objectClass);
      auto const objectParameter = subjectAmbassador.getParameterHandle(
          leaf, L"HLAobjectInstance");
      auto const tagParameter = subjectAmbassador.getParameterHandle(
          leaf, L"HLAtag");
      require(
          object.isValid() && objectParameter.isValid() && tagParameter.isValid(),
          "Delete Object Instance MOM test could not resolve its object or standard parameters");
      subjectAmbassador.sendInteraction(
          leaf,
          rti::ParameterHandleValueMap{
              {objectParameter, object.encode()},
              {tagParameter, rti::VariableLengthData{}}},
          rti::VariableLengthData{});
      requireException(
          [&] { static_cast<void>(subjectAmbassador.getObjectInstanceName(object)); },
          L"ObjectInstanceNotKnown",
          "verifying the valid HLAdeleteObjectInstance request removed its target");
      subjectAmbassador.unpublishObjectClassAttributes(
          objectClass, publishedAttributes);
    }

    if (interactionName ==
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableAsynchronousDelivery") {
      subjectAmbassador.enableAsynchronousDelivery();
      subjectAmbassador.sendInteraction(
          disableAsyncClass,
          rti::ParameterHandleValueMap{
              {disableAsyncFederateParameter, targetFederate.encode()}},
          rti::VariableLengthData{});
      // This succeeds only if the MOM interaction disabled the target first.
      subjectAmbassador.enableAsynchronousDelivery();
      subjectAmbassador.disableAsynchronousDelivery();
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableAsynchronousDelivery") {
      subjectAmbassador.sendInteraction(
          enableAsyncClass,
          rti::ParameterHandleValueMap{
              {enableAsyncFederateParameter, targetFederate.encode()}},
          rti::VariableLengthData{});
      // This succeeds only if the MOM request enabled asynchronous delivery;
      // it also restores the scenario's initial disabled state without
      // generating an expected-exception MOM report.
      subjectAmbassador.disableAsynchronousDelivery();
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeConstrained") {
      auto const callbacksBeforeEnable =
          subject.recorder().timeConstrainedEnabled().size();
      subjectAmbassador.enableTimeConstrained();
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeConstrainedEnabled().size() ==
                callbacksBeforeEnable + 1U;
          },
          options,
          "initial timeConstrainedEnabled callback for the MOM disable test");
      subjectAmbassador.sendInteraction(
          disableTimeConstrainedClass,
          rti::ParameterHandleValueMap{{
              disableTimeConstrainedFederateParameter,
              targetFederate.encode()}},
          rti::VariableLengthData{});

      // A second enable request is accepted only if the MOM request disabled
      // the specified federate. Observe its callback before restoring the
      // original unconstrained state for the rest of this scenario.
      auto const callbacksBeforeReenable =
          subject.recorder().timeConstrainedEnabled().size();
      subjectAmbassador.enableTimeConstrained();
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeConstrainedEnabled().size() ==
                callbacksBeforeReenable + 1U;
          },
          options,
          "timeConstrainedEnabled callback after HLAdisableTimeConstrained");
      subjectAmbassador.disableTimeConstrained();
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeRegulation") {
      auto const callbacksBeforeEnable =
          subject.recorder().timeRegulationEnabled().size();
      subjectAmbassador.enableTimeRegulation(*subjectTime.epsilon);
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeRegulationEnabled().size() ==
                callbacksBeforeEnable + 1U;
          },
          options,
          "initial timeRegulationEnabled callback for the MOM disable test");
      subjectAmbassador.sendInteraction(
          disableTimeRegulationClass,
          rti::ParameterHandleValueMap{{
              disableTimeRegulationFederateParameter,
              targetFederate.encode()}},
          rti::VariableLengthData{});

      // Enabling again proves the MOM request cleared the target's regulation
      // role. Restore the unconstrained/unregulated state after observing it.
      auto const callbacksBeforeReenable =
          subject.recorder().timeRegulationEnabled().size();
      subjectAmbassador.enableTimeRegulation(*subjectTime.epsilon);
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeRegulationEnabled().size() ==
                callbacksBeforeReenable + 1U;
          },
          options,
          "timeRegulationEnabled callback after HLAdisableTimeRegulation");
      subjectAmbassador.disableTimeRegulation();
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeConstrained") {
      auto const callbacksBeforeEnable =
          subject.recorder().timeConstrainedEnabled().size();
      subjectAmbassador.sendInteraction(
          enableTimeConstrainedClass,
          rti::ParameterHandleValueMap{{
              enableTimeConstrainedFederateParameter,
              targetFederate.encode()}},
          rti::VariableLengthData{});
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeConstrainedEnabled().size() ==
                callbacksBeforeEnable + 1U;
          },
          options,
          "timeConstrainedEnabled callback after HLAenableTimeConstrained");
      // The callback is the enable transition witness; this returns the
      // federate to its initial unconstrained state without an extra MOM error.
      subjectAmbassador.disableTimeConstrained();
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeRegulation") {
      auto const callbacksBeforeEnable =
          subject.recorder().timeRegulationEnabled().size();
      subjectAmbassador.sendInteraction(
          enableTimeRegulationClass,
          rti::ParameterHandleValueMap{
              {enableTimeRegulationFederateParameter, targetFederate.encode()},
              {enableTimeRegulationLookaheadParameter,
               subjectTime.epsilon->encode()}},
          rti::VariableLengthData{});
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeRegulationEnabled().size() ==
                callbacksBeforeEnable + 1U;
          },
          options,
          "timeRegulationEnabled callback after HLAenableTimeRegulation");
      subjectAmbassador.disableTimeRegulation();
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAflushQueueRequest") {
      auto const requestedTime = timeAfter(
          *subjectTime.factory,
          *subjectTime.initial,
          *subjectTime.epsilon,
          1U);
      auto const flushGrantsBefore = subject.recorder().flushQueueGrants().size();
      auto const timeAdvanceGrantsBefore =
          subject.recorder().timeAdvanceGrants().size();
      subjectAmbassador.sendInteraction(
          flushQueueRequestClass,
          rti::ParameterHandleValueMap{{
              flushQueueRequestTimeStampParameter,
              requestedTime->encode()}},
          rti::VariableLengthData{});
      waitFor(
          subject,
          [&] {
            return subject.recorder().flushQueueGrants().size() >=
                flushGrantsBefore + 1U;
          },
          options,
          "Flush Queue Grant callback after HLAflushQueueRequest");
      auto const flushGrants = subject.recorder().flushQueueGrants();
      require(
          flushGrants.size() == flushGrantsBefore + 1U &&
              subject.recorder().timeAdvanceGrants().size() ==
                  timeAdvanceGrantsBefore &&
              !flushGrants.back().time.empty() &&
              !flushGrants.back().optimisticTime.empty(),
          "HLAflushQueueRequest did not produce exactly one standard Flush Queue Grant");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAlocalDeleteObjectInstance") {
      auto const objectClass =
          observerAmbassador.getObjectClassHandle(options.objectClassName);
      auto const attribute = observerAmbassador.getAttributeHandle(
          objectClass, options.attributeName);
      require(
          objectClass.isValid() && attribute.isValid(),
          "Local Delete Object Instance MOM test could not resolve its adapter FOM object class and attribute");
      rti::AttributeHandleSet const publishedAttributes{attribute};
      rti::AttributeHandleSet const subscribedAttributes{attribute};
      observerAmbassador.publishObjectClassAttributes(
          objectClass, publishedAttributes);
      subjectAmbassador.subscribeObjectClassAttributes(
          objectClass, subscribedAttributes, true, L"");
      auto const object = observerAmbassador.registerObjectInstance(objectClass);
      require(
          object.isValid(),
          "Local Delete Object Instance MOM test received an invalid registered object handle");
      waitFor(
          subject,
          [&] { return subject.recorder().hasDiscovery(object); },
          options,
          "object discovery before HLAlocalDeleteObjectInstance");
      auto const objectName = observerAmbassador.getObjectInstanceName(object);
      subjectAmbassador.sendInteraction(
          localDeleteObjectInstanceClass,
          rti::ParameterHandleValueMap{{
              localDeleteObjectInstanceParameter,
              object.encode()}},
          rti::VariableLengthData{});
      requireException(
          [&] {
            static_cast<void>(subjectAmbassador.getObjectInstanceHandle(objectName));
          },
          L"ObjectInstanceNotKnown",
          "verifying HLAlocalDeleteObjectInstance forgot the object only locally");
      require(
          observerAmbassador.getObjectInstanceName(object) == objectName,
          "HLAlocalDeleteObjectInstance removed the object from another federate");
      subjectAmbassador.unsubscribeObjectClassAttributes(
          objectClass, subscribedAttributes);
      observerAmbassador.unpublishObjectClassAttributes(
          objectClass, publishedAttributes);
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAmodifyLookahead") {
      auto const callbacksBeforeEnable =
          subject.recorder().timeRegulationEnabled().size();
      subjectAmbassador.enableTimeRegulation(*subjectTime.epsilon);
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeRegulationEnabled().size() ==
                callbacksBeforeEnable + 1U;
          },
          options,
          "timeRegulationEnabled callback before HLAmodifyLookahead");
      auto requestedLookahead = subjectTime.factory->makeEpsilon();
      *requestedLookahead += *subjectTime.epsilon;
      subjectAmbassador.sendInteraction(
          modifyLookaheadClass,
          rti::ParameterHandleValueMap{{
              modifyLookaheadParameter,
              requestedLookahead->encode()}},
          rti::VariableLengthData{});
      auto actualLookahead = subjectTime.factory->makeZero();
      subjectAmbassador.queryLookahead(*actualLookahead);
      require(
          *actualLookahead == *requestedLookahead,
          "HLAmodifyLookahead did not apply the requested HLAlookahead interval");
      subjectAmbassador.disableTimeRegulation();
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequest") {
      auto const requestedTime = timeAfter(
          *subjectTime.factory,
          *subjectTime.initial,
          *subjectTime.epsilon,
          5U);
      auto const grantsBefore = subject.recorder().timeAdvanceGrants().size();
      subjectAmbassador.sendInteraction(
          nextMessageRequestClass,
          rti::ParameterHandleValueMap{{
              nextMessageRequestTimeStampParameter,
              requestedTime->encode()}},
          rti::VariableLengthData{});
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeAdvanceGrants().size() >=
                grantsBefore + 1U;
          },
          options,
          "Time Advance Grant callback after HLAnextMessageRequest");
      auto const grants = subject.recorder().timeAdvanceGrants();
      require(
          grants.size() == grantsBefore + 1U &&
              grants.back().encoded == encodeTime(*requestedTime),
          "HLAnextMessageRequest did not grant its requested time");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequestAvailable") {
      auto const requestedTime = timeAfter(
          *subjectTime.factory,
          *subjectTime.initial,
          *subjectTime.epsilon,
          6U);
      auto const grantsBefore = subject.recorder().timeAdvanceGrants().size();
      subjectAmbassador.sendInteraction(
          nextMessageRequestAvailableClass,
          rti::ParameterHandleValueMap{{
              nextMessageRequestAvailableTimeStampParameter,
              requestedTime->encode()}},
          rti::VariableLengthData{});
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeAdvanceGrants().size() >=
                grantsBefore + 1U;
          },
          options,
          "Time Advance Grant callback after HLAnextMessageRequestAvailable");
      auto const grants = subject.recorder().timeAdvanceGrants();
      require(
          grants.size() == grantsBefore + 1U &&
              grants.back().encoded == encodeTime(*requestedTime),
          "HLAnextMessageRequestAvailable did not grant its requested time");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequest") {
      auto const requestClass = subjectAmbassador.getInteractionClassHandle(
          interactionName);
      auto const timeStampParameter = subjectAmbassador.getParameterHandle(
          requestClass, L"HLAtimeStamp");
      require(
          requestClass.isValid() && timeStampParameter.isValid(),
          "HLAtimeAdvanceRequest could not resolve its standard MIM handles");
      auto const requestedTime = timeAfter(
          *subjectTime.factory,
          *subjectTime.initial,
          *subjectTime.epsilon,
          10U);
      auto const grantsBefore = subject.recorder().timeAdvanceGrants().size();
      subjectAmbassador.sendInteraction(
          requestClass,
          rti::ParameterHandleValueMap{{
              timeStampParameter,
              requestedTime->encode()}},
          rti::VariableLengthData{});
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeAdvanceGrants().size() ==
                grantsBefore + 1U;
          },
          options,
          "Time Advance Grant callback after HLAtimeAdvanceRequest");
      auto const grants = subject.recorder().timeAdvanceGrants();
      require(
          grants.size() == grantsBefore + 1U &&
              grants.back().encoded == encodeTime(*requestedTime),
          "HLAtimeAdvanceRequest did not grant its requested time");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequestAvailable") {
      auto const requestClass = subjectAmbassador.getInteractionClassHandle(
          interactionName);
      auto const timeStampParameter = subjectAmbassador.getParameterHandle(
          requestClass, L"HLAtimeStamp");
      require(
          requestClass.isValid() && timeStampParameter.isValid(),
          "HLAtimeAdvanceRequestAvailable could not resolve its standard MIM handles");
      auto const requestedTime = timeAfter(
          *subjectTime.factory,
          *subjectTime.initial,
          *subjectTime.epsilon,
          12U);
      auto const grantsBefore = subject.recorder().timeAdvanceGrants().size();
      subjectAmbassador.sendInteraction(
          requestClass,
          rti::ParameterHandleValueMap{{
              timeStampParameter,
              requestedTime->encode()}},
          rti::VariableLengthData{});
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeAdvanceGrants().size() ==
                grantsBefore + 1U;
          },
          options,
          "Time Advance Grant callback after HLAtimeAdvanceRequestAvailable");
      auto const grants = subject.recorder().timeAdvanceGrants();
      require(
          grants.size() == grantsBefore + 1U &&
              grants.back().encoded == encodeTime(*requestedTime),
          "HLAtimeAdvanceRequestAvailable did not grant its requested time");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunconditionalAttributeOwnershipDivestiture") {
      auto const requestClass = subjectAmbassador.getInteractionClassHandle(
          interactionName);
      auto const objectParameter = subjectAmbassador.getParameterHandle(
          requestClass, L"HLAobjectInstance");
      auto const attributeListParameter = subjectAmbassador.getParameterHandle(
          requestClass, L"HLAattributeList");
      require(
          requestClass.isValid() && objectParameter.isValid() &&
              attributeListParameter.isValid(),
          "HLAunconditionalAttributeOwnershipDivestiture could not resolve its standard MIM handles");
      rti::AttributeHandleSet const ownedAttributes{attributeToPublish};
      subjectAmbassador.publishObjectClassAttributes(
          objectClassToPublish, ownedAttributes);
      auto const object =
          subjectAmbassador.registerObjectInstance(objectClassToPublish);
      auto const encodedAttribute = copyBytes(attributeToPublish.encode());
      std::vector<std::uint8_t> encodedAttributeList{
          0x00U, 0x00U, 0x00U, 0x01U};
      encodedAttributeList.insert(
          encodedAttributeList.end(),
          encodedAttribute.begin(),
          encodedAttribute.end());
      require(
          object.isValid(),
          "Unconditional divestiture could not register its test object");
      require(
          subjectAmbassador.isAttributeOwnedByFederate(
              object, attributeToPublish),
          "The test federate did not own its newly registered attribute");
      subjectAmbassador.sendInteraction(
          requestClass,
          rti::ParameterHandleValueMap{
              {objectParameter, object.encode()},
              {attributeListParameter,
               rti::VariableLengthData(
                   encodedAttributeList.data(), encodedAttributeList.size())}},
          rti::VariableLengthData{});
      require(
          !subjectAmbassador.isAttributeOwnedByFederate(
              object, attributeToPublish),
          "HLAunconditionalAttributeOwnershipDivestiture did not divest the requested attribute");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishInteractionClass") {
      subjectAmbassador.sendInteraction(
          publishInteractionClassRequestClass,
          rti::ParameterHandleValueMap{{
              publishInteractionClassParameter,
              interactionClassToPublish.encode()}},
          rti::VariableLengthData{});
      // Unpublishing succeeds only if the MIM interaction applied the public
      // Publish Interaction Class transition.
      subjectAmbassador.unpublishInteractionClass(interactionClassToPublish);
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishInteractionClass") {
      auto const requestClass = subjectAmbassador.getInteractionClassHandle(
          interactionName);
      auto const interactionClassParameter =
          subjectAmbassador.getParameterHandle(
              requestClass, L"HLAinteractionClass");
      require(
          requestClass.isValid() && interactionClassParameter.isValid(),
          "HLAunpublishInteractionClass could not resolve its standard MIM handles");
      subjectAmbassador.publishInteractionClass(interactionClassToPublish);
      subjectAmbassador.sendInteraction(
          requestClass,
          rti::ParameterHandleValueMap{{
              interactionClassParameter,
              interactionClassToPublish.encode()}},
          rti::VariableLengthData{});
      requireException(
          [&] {
            subjectAmbassador.sendInteraction(
                interactionClassToPublish,
                rti::ParameterHandleValueMap{},
                rti::VariableLengthData{});
          },
          L"InteractionClassNotPublished",
          "verifying HLAunpublishInteractionClass removed publication");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassAttributes") {
      auto const encodedAttribute = copyBytes(attributeToPublish.encode());
      std::vector<std::uint8_t> encodedAttributeList{
          0x00U, 0x00U, 0x00U, 0x01U};
      encodedAttributeList.insert(
          encodedAttributeList.end(),
          encodedAttribute.begin(),
          encodedAttribute.end());
      rti::AttributeHandleSet const attributesToPublish{attributeToPublish};
      subjectAmbassador.sendInteraction(
          publishObjectClassAttributesRequestClass,
          rti::ParameterHandleValueMap{
              {publishObjectClassAttributesObjectClassParameter,
               objectClassToPublish.encode()},
              {publishObjectClassAttributesAttributeListParameter,
               rti::VariableLengthData(
                   encodedAttributeList.data(), encodedAttributeList.size())}},
          rti::VariableLengthData{});
      // This succeeds only if the MIM interaction applied the declaration.
      subjectAmbassador.unpublishObjectClassAttributes(
          objectClassToPublish, attributesToPublish);
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassAttributes") {
      auto const requestClass = subjectAmbassador.getInteractionClassHandle(
          interactionName);
      auto const objectClassParameter = subjectAmbassador.getParameterHandle(
          requestClass, L"HLAobjectClass");
      auto const attributeListParameter = subjectAmbassador.getParameterHandle(
          requestClass, L"HLAattributeList");
      require(
          requestClass.isValid() && objectClassParameter.isValid() &&
              attributeListParameter.isValid(),
          "HLAunpublishObjectClassAttributes could not resolve its standard MIM handles");
      auto const encodedAttribute = copyBytes(attributeToPublish.encode());
      std::vector<std::uint8_t> encodedAttributeList{
          0x00U, 0x00U, 0x00U, 0x01U};
      encodedAttributeList.insert(
          encodedAttributeList.end(),
          encodedAttribute.begin(),
          encodedAttribute.end());
      rti::AttributeHandleSet const attributesToUnpublish{attributeToPublish};
      subjectAmbassador.publishObjectClassAttributes(
          objectClassToPublish, attributesToUnpublish);
      subjectAmbassador.sendInteraction(
          requestClass,
          rti::ParameterHandleValueMap{
              {objectClassParameter, objectClassToPublish.encode()},
              {attributeListParameter,
               rti::VariableLengthData(
                   encodedAttributeList.data(), encodedAttributeList.size())}},
          rti::VariableLengthData{});
      requireException(
          [&] {
            static_cast<void>(subjectAmbassador.registerObjectInstance(
                objectClassToPublish));
          },
          L"ObjectClassNotPublished",
          "verifying HLAunpublishObjectClassAttributes removed the class publication");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassDirectedInteractions") {
      auto const encodedInteractionClass =
          copyBytes(interactionClassToPublish.encode());
      std::vector<std::uint8_t> encodedInteractionClassList{
          0x00U, 0x00U, 0x00U, 0x01U};
      encodedInteractionClassList.insert(
          encodedInteractionClassList.end(),
          encodedInteractionClass.begin(),
          encodedInteractionClass.end());
      rti::InteractionClassHandleSet const interactionClassesToPublish{
          interactionClassToPublish};
      subjectAmbassador.sendInteraction(
          publishDirectedInteractionsRequestClass,
          rti::ParameterHandleValueMap{
              {publishDirectedInteractionsObjectClassParameter,
               directedInteractionObjectClass.encode()},
              {publishDirectedInteractionsClassListParameter,
               rti::VariableLengthData(
                   encodedInteractionClassList.data(),
                   encodedInteractionClassList.size())}},
          rti::VariableLengthData{});
      // The P0 FOM declares TckInteraction for TckObject.
      // Unpublishing the exact pair proves that the MOM request applied it.
      subjectAmbassador.unpublishObjectClassDirectedInteractions(
          directedInteractionObjectClass, interactionClassesToPublish);
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassDirectedInteractions") {
      auto const requestClass = subjectAmbassador.getInteractionClassHandle(
          interactionName);
      auto const objectClassParameter = subjectAmbassador.getParameterHandle(
          requestClass, L"HLAobjectClass");
      auto const interactionClassListParameter =
          subjectAmbassador.getParameterHandle(
              requestClass, L"HLAinteractionClassList");
      auto const directedInteractionAttribute =
          subjectAmbassador.getAttributeHandle(
              directedInteractionObjectClass, options.attributeName);
      require(
          requestClass.isValid() && objectClassParameter.isValid() &&
              interactionClassListParameter.isValid() &&
              directedInteractionAttribute.isValid(),
          "HLAunpublishObjectClassDirectedInteractions could not resolve its standard MIM handles or target attribute");

      rti::AttributeHandleSet const directedAttributes{
          directedInteractionAttribute};
      subjectAmbassador.publishObjectClassAttributes(
          directedInteractionObjectClass, directedAttributes);
      auto const directedTarget =
          subjectAmbassador.registerObjectInstance(
              directedInteractionObjectClass);
      require(
          directedTarget.isValid(),
          "HLAunpublishObjectClassDirectedInteractions could not register its target object");

      rti::InteractionClassHandleSet const interactionClasses{
          interactionClassToPublish};
      subjectAmbassador.publishObjectClassDirectedInteractions(
          directedInteractionObjectClass, interactionClasses);
      auto const encodedInteractionClass =
          copyBytes(interactionClassToPublish.encode());
      std::vector<std::uint8_t> encodedInteractionClassList{
          0x00U, 0x00U, 0x00U, 0x01U};
      encodedInteractionClassList.insert(
          encodedInteractionClassList.end(),
          encodedInteractionClass.begin(),
          encodedInteractionClass.end());
      subjectAmbassador.sendInteraction(
          requestClass,
          rti::ParameterHandleValueMap{
              {objectClassParameter,
               directedInteractionObjectClass.encode()},
              {interactionClassListParameter,
               rti::VariableLengthData(
                   encodedInteractionClassList.data(),
                   encodedInteractionClassList.size())}},
          rti::VariableLengthData{});
      requireException(
          [&] {
            subjectAmbassador.sendDirectedInteraction(
                interactionClassToPublish,
                directedTarget,
                rti::ParameterHandleValueMap{},
                rti::VariableLengthData{});
          },
          L"InteractionClassNotPublished",
          "verifying HLAunpublishObjectClassDirectedInteractions removed the directed publication");
    } else if (interactionName ==
               L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsynchronizationPointAchieved") {
      auto const synchronizationPointAchievedClass =
          subjectAmbassador.getInteractionClassHandle(
              interactionName);
      auto const labelParameter = subjectAmbassador.getParameterHandle(
          synchronizationPointAchievedClass, L"HLAlabel");
      require(
          synchronizationPointAchievedClass.isValid() &&
              labelParameter.isValid(),
          "HLAsynchronizationPointAchieved could not resolve its standard MIM handles");
      auto const label = L"table7-s-leaf-mom-achieved";
      auto const registrationsBefore =
          subject.synchronizationPointRegistrations().size();
      auto const announcementsBefore =
          observer.synchronizationPointAnnouncements().size();
      auto const subjectSynchronizedBefore =
          subject.recorder().federationSynchronized().size();
      auto const observerSynchronizedBefore =
          observer.recorder().federationSynchronized().size();
      subjectAmbassador.registerFederationSynchronizationPoint(
          label, rti::VariableLengthData{});
      waitForSessions(
          sessions,
          [&] {
            return subject.synchronizationPointRegistrations().size() ==
                    registrationsBefore + 1U &&
                observer.synchronizationPointAnnouncements().size() ==
                    announcementsBefore + 1U;
          },
          options,
          "synchronization-point announcement before the MOM achievement request");
      require(
          subject.synchronizationPointRegistrations().back().label == label &&
              subject.synchronizationPointRegistrations().back().succeeded &&
              observer.synchronizationPointAnnouncements().back().label == label,
          "The MOM synchronization-point achievement probe was not announced successfully");
      observerAmbassador.synchronizationPointAchieved(label);
      subjectAmbassador.sendInteraction(
          synchronizationPointAchievedClass,
          rti::ParameterHandleValueMap{{
              labelParameter,
              rti::HLAunicodeString{label}.encode()}},
          rti::VariableLengthData{});
      waitForSessions(
          sessions,
          [&] {
            return subject.recorder().federationSynchronized().size() ==
                    subjectSynchronizedBefore + 1U &&
                observer.recorder().federationSynchronized().size() ==
                    observerSynchronizedBefore + 1U;
          },
          options,
          "federation synchronized after HLAsynchronizationPointAchieved");
      require(
          subject.recorder().federationSynchronized().back().label == label &&
              observer.recorder().federationSynchronized().back().label == label &&
              subject.recorder().federationSynchronized().back().failedToSyncSet.empty() &&
              observer.recorder().federationSynchronized().back().failedToSyncSet.empty(),
          "HLAsynchronizationPointAchieved did not complete the announced point");
    }
  }

  waitForSessions(
      sessions,
      [&] {
        return observer.recorder().interactions().size() >= expectedServices.size() ||
            !observer.recorder().timedInteractions().empty();
      },
      options,
      "the 54 parameter-required Table 7 S-leaf MOM exception reports");
  requireNoTimestampedReports(observer, "Table 7 S-leaf MOM exception reports");
  auto const reports = observer.recorder().interactions();
  require(
      expectedServices.size() == 54U && reports.size() == expectedServices.size(),
      "The parameter-required Table 7 S-leaf requests did not produce exactly 54 reports");
  std::set<std::wstring> reportedServices;
  auto const subjectEncoding = copyBytes(subjectFederateHandle.encode());
  for (auto const& report : reports) {
    requireReportMetadata(
        report,
        exceptionReportClass,
        reliable,
        "HLAreportMOMexception");
    require(
        report.parameters.size() == 4U &&
            report.parameters.count(exceptionFederateParameter) == 1U &&
            report.parameters.count(exceptionServiceParameter) == 1U &&
            report.parameters.count(exceptionTextParameter) == 1U &&
            report.parameters.count(parameterErrorParameter) == 1U,
        "HLAreportMOMexception omitted one of the required standard parameters");
    rti::HLAunicodeString serviceName;
    serviceName.decode(report.parameters.at(exceptionServiceParameter));
    require(
        expectedServices.count(serviceName.get()) == 1U &&
            reportedServices.insert(serviceName.get()).second,
        "HLAreportMOMexception did not uniquely identify an expected full S-leaf service name");
    rti::HLAunicodeString exceptionText;
    exceptionText.decode(report.parameters.at(exceptionTextParameter));
    auto const exceptionTextValue = exceptionText.get();
    require(!exceptionTextValue.empty(),
            "HLAreportMOMexception supplied no exception description");
    rti::HLAboolean parameterError;
    parameterError.decode(report.parameters.at(parameterErrorParameter));
    auto const serviceNameValue = serviceName.get();
    auto const serviceLabel =
        std::string{serviceNameValue.begin(), serviceNameValue.end()};
    auto const exceptionLabel =
        std::string{exceptionTextValue.begin(), exceptionTextValue.end()};
    require(
        parameterError.get(),
        "HLAreportMOMexception did not flag the missing required parameters for " +
            serviceLabel + ": " + exceptionLabel);
    require(
        copyBytes(report.parameters.at(exceptionFederateParameter)) == subjectEncoding,
        "HLAreportMOMexception identified a different joined federate");
  }
  require(
      reportedServices == expectedServices,
      "The HLAreportMOMexception callbacks did not cover every parameter-bearing Table 7 S leaf");

  // The remaining valid-effect probes include another deliberate malformed
  // request. The observer has finished its exception-report inventory, so
  // unsubscribe it now instead of allowing an evoked callback from that later
  // probe to leak into the parameterless-report count below.
  observerAmbassador.unsubscribeInteractionClass(exceptionReportClass);

  auto const subjectExceptionReportClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
  auto const subscribeInteractionClassRequest =
      subjectAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeInteractionClass");
  auto const subscribeInteractionClassParameter =
      subjectAmbassador.getParameterHandle(
          subscribeInteractionClassRequest, L"HLAinteractionClass");
  auto const subscribeInteractionActiveParameter =
      subjectAmbassador.getParameterHandle(
          subscribeInteractionClassRequest, L"HLAactive");
  require(
      subjectExceptionReportClass.isValid() &&
          subscribeInteractionClassRequest.isValid() &&
          subscribeInteractionClassParameter.isValid() &&
          subscribeInteractionActiveParameter.isValid(),
      "HLAsubscribeInteractionClass could not resolve its standard MIM handles");
  subject.recorder().clearInteraction();
  subjectAmbassador.sendInteraction(
      subscribeInteractionClassRequest,
      rti::ParameterHandleValueMap{
          {subscribeInteractionClassParameter,
           subjectExceptionReportClass.encode()},
          {subscribeInteractionActiveParameter,
           rti::HLAboolean{true}.encode()}},
      rti::VariableLengthData{});

  auto const malformedPublicationRequest =
      observerAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishInteractionClass");
  require(
      malformedPublicationRequest.isValid(),
      "HLApublishInteractionClass could not be resolved for the subscription-effect probe");
  observerAmbassador.publishInteractionClass(malformedPublicationRequest);
  bool malformedPublicationRejected = false;
  try {
    observerAmbassador.sendInteraction(
        malformedPublicationRequest,
        rti::ParameterHandleValueMap{},
        rti::VariableLengthData{});
  } catch (...) {
    malformedPublicationRejected = true;
  }
  require(
      malformedPublicationRejected,
      "The subscription-effect probe did not exercise a rejected MOM request");
  waitFor(
      subject,
      [&] { return hasInteraction(subject, subjectExceptionReportClass); },
      options,
      "HLAreportMOMexception delivery after HLAsubscribeInteractionClass");
  auto const subscribedExceptionReports = subject.recorder().interactions();
  require(
      subscribedExceptionReports.size() == 1U,
      "HLAsubscribeInteractionClass did not deliver exactly one subscribed MOM report");
  requireReportMetadata(
      subscribedExceptionReports.front(),
      subjectExceptionReportClass,
      reliable,
      "HLAsubscribeInteractionClass effect");
  auto const subjectExceptionFederateParameter = subjectAmbassador.getParameterHandle(
      subjectExceptionReportClass, L"HLAfederate");
  auto const subjectExceptionServiceParameter = subjectAmbassador.getParameterHandle(
      subjectExceptionReportClass, L"HLAservice");
  require(
      subjectExceptionFederateParameter.isValid() &&
          subjectExceptionServiceParameter.isValid() &&
          subscribedExceptionReports.front().parameters.size() == 4U &&
          copyBytes(subscribedExceptionReports.front().parameters.at(
              subjectExceptionFederateParameter)) ==
              copyBytes(observerAmbassador.getFederateHandle(observerName).encode()),
      "HLAsubscribeInteractionClass delivered a malformed report or wrong federate identity");
  rti::HLAunicodeString subscribedService;
  subscribedService.decode(subscribedExceptionReports.front().parameters.at(
      subjectExceptionServiceParameter));
  require(
      subscribedService.get() ==
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishInteractionClass",
      "HLAsubscribeInteractionClass delivered a report for the wrong service");
  observerAmbassador.unpublishInteractionClass(malformedPublicationRequest);

  auto const mimReportClass = observerAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata");
  auto const mimRequestClass = subjectAmbassador.getInteractionClassHandle(
      std::wstring{requestMimDataLeaf});
  require(
      mimReportClass.isValid() && mimRequestClass.isValid(),
      "The parameterless Table 7 S MIM-data request/report handles were invalid");
  observerAmbassador.subscribeInteractionClass(mimReportClass, true);
  subjectAmbassador.publishInteractionClass(mimRequestClass);
  publishedLeaves.push_back(mimRequestClass);
  observer.recorder().clearInteraction();
  subjectAmbassador.sendInteraction(
      mimRequestClass,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});
  waitForSessions(
      sessions,
      [&] {
        return hasInteraction(observer, mimReportClass) ||
            !observer.recorder().timedInteractions().empty();
      },
      options,
      "the HLAreportMIMdata response to the parameterless S-leaf request");
  requireNoTimestampedReports(observer, "HLAreportMIMdata");
  auto const mimReports = observer.recorder().interactions();
  require(
      mimReports.size() == 1U,
      "The parameterless HLArequestMIMdata produced an unexpected report count");
  requireReportMetadata(
      mimReports.front(), mimReportClass, reliable, "HLAreportMIMdata");

  auto const synchronizationReportClass = observerAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints");
  auto const synchronizationRequestClass = subjectAmbassador.getInteractionClassHandle(
      std::wstring{requestSynchronizationPointsLeaf});
  require(
      synchronizationReportClass.isValid() && synchronizationRequestClass.isValid(),
      "The parameterless Table 7 S synchronization request/report handles were invalid");
  observerAmbassador.subscribeInteractionClass(synchronizationReportClass, true);
  subjectAmbassador.publishInteractionClass(synchronizationRequestClass);
  publishedLeaves.push_back(synchronizationRequestClass);
  subjectAmbassador.registerFederationSynchronizationPoint(
      L"table7-s-leaf-subscription", rti::VariableLengthData{});
  waitForSessions(
      sessions,
      [&] { return !subject.synchronizationPointRegistrations().empty(); },
      options,
      "the synchronization-point registration used by the parameterless S-leaf query");
  require(
      subject.synchronizationPointRegistrations().front().succeeded,
      "Table 7 S-leaf synchronization-point setup failed");
  observer.recorder().clearInteraction();
  subjectAmbassador.sendInteraction(
      synchronizationRequestClass,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});
  waitForSessions(
      sessions,
      [&] {
        return hasInteraction(observer, synchronizationReportClass) ||
            !observer.recorder().timedInteractions().empty();
      },
      options,
      "the HLAreportSynchronizationPoints response to the parameterless S-leaf request");
  requireNoTimestampedReports(observer, "HLAreportSynchronizationPoints");
  auto const synchronizationReports = observer.recorder().interactions();
  require(
      synchronizationReports.size() == 1U,
      "The parameterless HLArequestSynchronizationPoints produced an unexpected report count");
  requireReportMetadata(
      synchronizationReports.front(),
      synchronizationReportClass,
      reliable,
      "HLAreportSynchronizationPoints");

  for (auto const& publishedLeaf : publishedLeaves) {
    subjectAmbassador.unpublishInteractionClass(publishedLeaf);
  }
  observerAmbassador.unsubscribeInteractionClass(synchronizationReportClass);
  observerAmbassador.unsubscribeInteractionClass(mimReportClass);

  auto const resignFederationExecutionClass =
      observerAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAresignFederationExecution");
  auto const resignActionParameter = observerAmbassador.getParameterHandle(
      resignFederationExecutionClass, L"HLAresignAction");
  require(
      resignFederationExecutionClass.isValid() &&
          resignActionParameter.isValid(),
      "HLAresignFederationExecution could not resolve its standard MIM handles");
  observerAmbassador.publishInteractionClass(resignFederationExecutionClass);
  observerAmbassador.sendInteraction(
      resignFederationExecutionClass,
      rti::ParameterHandleValueMap{{
          resignActionParameter,
          // The observer still owns the object used by the local-delete
          // probe; cancel/delete/divest is required for successful resign.
          rti::HLAinteger32BE{4}.encode()}},
      rti::VariableLengthData{});
  requireException(
      [&] {
        static_cast<void>(subjectAmbassador.getFederateHandle(observerName));
      },
      L"NameNotFound",
      "HLAresignFederationExecution removed its sending federate");

  subject.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  subjectAmbassador.destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioFederationMomSLeafSubscriptionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomSLeafSubscription(options, model);
}

void scenarioFederationMomChangeAttributeOrderType(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM Change Attribute Order Type coverage requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session subject(options, model, "mom-change-attribute-order-subject");
  Session observer(options, model, "mom-change-attribute-order-observer");
  auto const federation = federationName(
      options, "federation-mom-change-attribute-order-type");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-mom-change-attribute-order-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-change-attribute-order-observer",
      options.federateType,
      federation);

  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& observerAmbassador = observer.rtiAmbassador();
  subjectAmbassador.setServiceReportingSwitch(false);
  subjectAmbassador.setSendServiceReportsToFileSwitch(false);
  observerAmbassador.setServiceReportingSwitch(false);
  observerAmbassador.setSendServiceReportsToFileSwitch(false);

  auto const subjectClass = subjectAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const observerClass = observerAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const subjectAttribute = subjectAmbassador.getAttributeHandle(
      subjectClass, options.attributeName);
  auto const observerAttribute = observerAmbassador.getAttributeHandle(
      observerClass, options.attributeName);
  require(
      subjectClass.isValid() && observerClass.isValid() &&
          subjectClass == observerClass && subjectAttribute.isValid() &&
          observerAttribute.isValid() && subjectAttribute == observerAttribute,
      "The adapter FOM did not provide the configured object class and attribute");
  rti::AttributeHandleSet const subjectAttributes{subjectAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  subjectAmbassador.publishObjectClassAttributes(subjectClass, subjectAttributes);
  observerAmbassador.subscribeObjectClassAttributes(
      observerClass, observerAttributes, true, L"");
  auto const object = subjectAmbassador.registerObjectInstance(subjectClass);
  require(object.isValid(),
          "Change Attribute Order Type target registration returned an invalid handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "Change Attribute Order Type target discovery");

  auto subjectTime = makeTimeContext(subject);
  auto lookahead = subjectTime.factory->makeZero();
  require(lookahead != nullptr,
          "Change Attribute Order Type time factory returned no zero lookahead");
  *lookahead += *subjectTime.epsilon;
  subjectAmbassador.enableTimeRegulation(*lookahead);
  waitFor(
      subject,
      [&] { return subject.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Change Attribute Order Type sender regulation callback");

  auto const requestClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType");
  auto const targetFederateParameter = subjectAmbassador.getParameterHandle(
      requestClass, L"HLAfederate");
  auto const objectParameter = subjectAmbassador.getParameterHandle(
      requestClass, L"HLAobjectInstance");
  auto const attributesParameter = subjectAmbassador.getParameterHandle(
      requestClass, L"HLAattributeList");
  auto const orderParameter = subjectAmbassador.getParameterHandle(
      requestClass, L"HLAsendOrder");
  require(
      requestClass.isValid() && targetFederateParameter.isValid() &&
          objectParameter.isValid() &&
          attributesParameter.isValid() && orderParameter.isValid(),
      "The standard MIM omitted the inherited target or Change Attribute Order Type request parameters");
  subjectAmbassador.publishInteractionClass(requestClass);

  auto const encodeAttributeHandleList = [](rti::AttributeHandle const& attribute) {
    auto const encodedAttribute = copyBytes(attribute.encode());
    std::vector<std::uint8_t> bytes{0x00U, 0x00U, 0x00U, 0x01U};
    bytes.insert(bytes.end(), encodedAttribute.begin(), encodedAttribute.end());
    return rti::VariableLengthData(bytes.data(), bytes.size());
  };
  rti::HLAinteger32BE const receiveOrder{1};
  subjectAmbassador.sendInteraction(
      requestClass,
      rti::ParameterHandleValueMap{
          {targetFederateParameter, subject.federateHandle().encode()},
          {objectParameter, object.encode()},
          {attributesParameter, encodeAttributeHandleList(subjectAttribute)},
          {orderParameter, receiveOrder.encode()}},
      rti::VariableLengthData{});

  auto const timestamp = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      5U);
  std::vector<std::uint8_t> const valueBytes{0x4DU, 0x4FU, 0x4DU, 0x41U};
  std::vector<std::uint8_t> const tagBytes{0x4FU, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      subjectAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = subjectAmbassador.updateAttributeValues(
      object, values, tag, *timestamp);
  require(
      !retraction.isValid(),
      "The MOM-selected RECEIVE order unexpectedly returned a retraction designator");
  waitFor(
      observer,
      [&] { return observer.recorder().timedReflections().size() >= 1U; },
      options,
      "Change Attribute Order Type timestamp-bearing update reflection");
  auto const deliveries = observer.recorder().timedReflections();
  require(
      deliveries.size() == 1U && deliveries.front().object == object &&
          deliveries.front().values.size() == 1U &&
          deliveries.front().values.count(observerAttribute) == 1U &&
          copyBytes(deliveries.front().values.at(observerAttribute)) == valueBytes &&
          deliveries.front().tag == tagBytes &&
          deliveries.front().time == encodeTime(*timestamp) &&
          deliveries.front().producer == subject.federateHandle() &&
          deliveries.front().sentOrder == rti::RECEIVE &&
          deliveries.front().receivedOrder == rti::RECEIVE &&
          !deliveries.front().retractionPresent,
      "HLAchangeAttributeOrderType did not change the later attribute delivery to RECEIVE order");

  subjectAmbassador.disableTimeRegulation();
  observerAmbassador.unsubscribeObjectClassAttributes(
      observerClass, observerAttributes);
  subjectAmbassador.unpublishObjectClassAttributes(
      subjectClass, subjectAttributes);
  subjectAmbassador.unpublishInteractionClass(requestClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  subjectAmbassador.destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioFederationMomChangeAttributeOrderTypeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomChangeAttributeOrderType(options, model);
}

void scenarioFederationMomServiceActorIdentity(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM service-actor coverage requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session owner(options, model, "mom-service-actor-object-owner");
  Session invoker(options, model, "mom-service-actor-interaction-sender");
  Session observer(options, model, "mom-service-actor-report-observer");
  auto const federation = federationName(
      options, "federation-mom-service-actor-identity");
  owner.connect();
  invoker.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-service-actor-object-owner",
      options.federateType,
      federation);
  invoker.join(
      options.memberFederateName + L"-service-actor-invoker",
      options.federateType,
      federation);
  observer.join(
      options.ownerFederateName + L"-service-actor-report-observer",
      options.federateType,
      federation);

  auto& ownerAmbassador = owner.rtiAmbassador();
  auto& invokerAmbassador = invoker.rtiAmbassador();
  auto& observerAmbassador = observer.rtiAmbassador();
  for (auto* session : {&owner, &invoker, &observer}) {
    session->rtiAmbassador().setServiceReportingSwitch(false);
    session->rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  }

  auto const ownerClass = ownerAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = ownerAmbassador.getAttributeHandle(
      ownerClass, options.attributeName);
  auto const invokerClass = invokerAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const invokerAttribute = invokerAmbassador.getAttributeHandle(
      invokerClass, options.attributeName);
  auto const observerClass = observerAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const observerAttribute = observerAmbassador.getAttributeHandle(
      observerClass, options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() &&
          invokerClass.isValid() && invokerAttribute.isValid() &&
          observerClass.isValid() && observerAttribute.isValid() &&
          ownerClass == invokerClass && ownerClass == observerClass &&
          ownerAttribute == invokerAttribute && ownerAttribute == observerAttribute,
      "The service-actor FOM did not provide one shared object class and attribute");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const invokerAttributes{invokerAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  ownerAmbassador.publishObjectClassAttributes(ownerClass, ownerAttributes);
  invokerAmbassador.subscribeObjectClassAttributes(
      invokerClass, invokerAttributes, true, L"");
  observerAmbassador.subscribeObjectClassAttributes(
      observerClass, observerAttributes, true, L"");
  auto const object = ownerAmbassador.registerObjectInstance(ownerClass);
  require(object.isValid(),
          "Service-actor target registration returned an invalid object handle");
  waitFor(
      invoker,
      [&] { return invoker.recorder().hasDiscovery(object); },
      options,
      "service-actor sender object discovery");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "service-actor target object discovery");
  require(
      ownerAmbassador.isAttributeOwnedByFederate(object, ownerAttribute) &&
          !invokerAmbassador.isAttributeOwnedByFederate(object, invokerAttribute),
      "Service-actor setup did not make the target attribute exclusive to another joined federate");

  auto const serviceClass = invokerAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType");
  auto const targetFederateParameter = invokerAmbassador.getParameterHandle(
      serviceClass, L"HLAfederate");
  auto const objectParameter = invokerAmbassador.getParameterHandle(
      serviceClass, L"HLAobjectInstance");
  auto const attributesParameter = invokerAmbassador.getParameterHandle(
      serviceClass, L"HLAattributeList");
  auto const orderParameter = invokerAmbassador.getParameterHandle(
      serviceClass, L"HLAsendOrder");
  require(
      serviceClass.isValid() && targetFederateParameter.isValid() &&
          objectParameter.isValid() &&
          attributesParameter.isValid() && orderParameter.isValid(),
      "The standard MIM omitted an inherited target or Change Attribute Order Type parameter");
  invokerAmbassador.publishInteractionClass(serviceClass);

  auto ownerTime = makeTimeContext(owner);
  auto ownerLookahead = ownerTime.factory->makeZero();
  require(ownerLookahead != nullptr,
          "Service-actor time factory returned no zero lookahead");
  *ownerLookahead += *ownerTime.epsilon;
  ownerAmbassador.enableTimeRegulation(*ownerLookahead);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "service-actor target time-regulation callback");

  auto const encodeAttributeHandleList = [](
      rti::AttributeHandle const& attribute) {
    auto const encodedAttribute = copyBytes(attribute.encode());
    std::vector<std::uint8_t> bytes{0x00U, 0x00U, 0x00U, 0x01U};
    bytes.insert(bytes.end(), encodedAttribute.begin(), encodedAttribute.end());
    return rti::VariableLengthData(bytes.data(), bytes.size());
  };
  rti::HLAinteger32BE const receiveOrder{1};
  require(
      invoker.federateHandle() != owner.federateHandle(),
      "Service-actor sender and designated joined federate unexpectedly match");
  bool serviceRequestAccepted = false;
  try {
    invokerAmbassador.sendInteraction(
        serviceClass,
        rti::ParameterHandleValueMap{
            {targetFederateParameter, owner.federateHandle().encode()},
            {objectParameter, object.encode()},
            {attributesParameter, encodeAttributeHandleList(ownerAttribute)},
            {orderParameter, receiveOrder.encode()}},
        rti::VariableLengthData{});
    serviceRequestAccepted = true;
  } catch (rti::Exception const&) {
    // The assertion below identifies the service-request phase separately
    // from the later public attribute-update witness.
  }
  require(
      serviceRequestAccepted,
      "HLAchangeAttributeOrderType did not execute for its designated owner when sent by another federate");

  auto const timestamp = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  std::vector<std::uint8_t> const valueBytes{0x4DU, 0x4FU, 0x4DU, 0x41U};
  std::vector<std::uint8_t> const tagBytes{0x4FU, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      ownerAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = ownerAmbassador.updateAttributeValues(
      object, values, tag, *timestamp);
  require(
      !retraction.isValid(),
      "The HLAservice-designated owner did not apply RECEIVE order to its attribute");
  waitFor(
      observer,
      [&] { return observer.recorder().timedReflections().size() >= 1U; },
      options,
      "service-actor target's timestamp-bearing attribute reflection");
  auto const deliveries = observer.recorder().timedReflections();
  require(
      deliveries.size() == 1U && deliveries.front().object == object &&
          deliveries.front().values.size() == 1U &&
          deliveries.front().values.count(observerAttribute) == 1U &&
          copyBytes(deliveries.front().values.at(observerAttribute)) == valueBytes &&
          deliveries.front().tag == tagBytes &&
          deliveries.front().time == encodeTime(*timestamp) &&
          deliveries.front().producer == owner.federateHandle() &&
          deliveries.front().sentOrder == rti::RECEIVE &&
          deliveries.front().receivedOrder == rti::RECEIVE &&
          !deliveries.front().retractionPresent,
      "HLAchangeAttributeOrderType did not execute as the HLAfederate-designated member");

  ownerAmbassador.disableTimeRegulation();
  invokerAmbassador.unsubscribeObjectClassAttributes(
      invokerClass, invokerAttributes);
  observerAmbassador.unsubscribeObjectClassAttributes(
      observerClass, observerAttributes);
  invokerAmbassador.unpublishInteractionClass(serviceClass);
  ownerAmbassador.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  observer.resign(rti::NO_ACTION);
  invoker.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  ownerAmbassador.destroyFederationExecution(federation);
  observer.disconnect();
  invoker.disconnect();
  owner.disconnect();
}

void scenarioFederationMomServiceActorIdentityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomServiceActorIdentity(options, model);
}

void scenarioFederationMomChangeInteractionOrderType(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM Change Interaction Order Type coverage requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session subject(options, model, "mom-change-interaction-order-subject");
  Session observer(options, model, "mom-change-interaction-order-observer");
  auto const federation = federationName(
      options, "federation-mom-change-interaction-order-type");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-mom-change-interaction-order-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-change-interaction-order-observer",
      options.federateType,
      federation);

  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& observerAmbassador = observer.rtiAmbassador();
  subjectAmbassador.setServiceReportingSwitch(false);
  subjectAmbassador.setSendServiceReportsToFileSwitch(false);
  observerAmbassador.setServiceReportingSwitch(false);
  observerAmbassador.setSendServiceReportsToFileSwitch(false);

  auto const subjectInteraction = subjectAmbassador.getInteractionClassHandle(
      options.interactionClassName);
  auto const observerInteraction = observerAmbassador.getInteractionClassHandle(
      options.interactionClassName);
  require(
      subjectInteraction.isValid() && observerInteraction.isValid() &&
          subjectInteraction == observerInteraction,
      "The adapter FOM did not provide its configured interaction class");
  subjectAmbassador.publishInteractionClass(subjectInteraction);
  observerAmbassador.subscribeInteractionClass(observerInteraction);

  auto subjectTime = makeTimeContext(subject);
  auto lookahead = subjectTime.factory->makeZero();
  require(lookahead != nullptr,
          "Change Interaction Order Type time factory returned no zero lookahead");
  *lookahead += *subjectTime.epsilon;
  subjectAmbassador.enableTimeRegulation(*lookahead);
  waitFor(
      subject,
      [&] { return subject.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Change Interaction Order Type sender regulation callback");

  auto const requestClass = subjectAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeInteractionOrderType");
  auto const targetFederateParameter = subjectAmbassador.getParameterHandle(
      requestClass, L"HLAfederate");
  auto const interactionParameter = subjectAmbassador.getParameterHandle(
      requestClass, L"HLAinteractionClass");
  auto const orderParameter = subjectAmbassador.getParameterHandle(
      requestClass, L"HLAsendOrder");
  require(
      requestClass.isValid() && targetFederateParameter.isValid() &&
          interactionParameter.isValid() &&
          orderParameter.isValid(),
      "The standard MIM omitted the inherited target or Change Interaction Order Type request parameters");
  subjectAmbassador.publishInteractionClass(requestClass);

  rti::HLAinteger32BE const receiveOrder{1};
  subjectAmbassador.sendInteraction(
      requestClass,
      rti::ParameterHandleValueMap{
          {targetFederateParameter, subject.federateHandle().encode()},
          {interactionParameter, subjectInteraction.encode()},
          {orderParameter, receiveOrder.encode()}},
      rti::VariableLengthData{});

  auto const timestamp = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      5U);
  std::vector<std::uint8_t> const tagBytes{0x49U, 0x4EU, 0x54U, 0x4FU};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = subjectAmbassador.sendInteraction(
      subjectInteraction,
      rti::ParameterHandleValueMap{},
      tag,
      *timestamp);
  require(
      !retraction.isValid(),
      "The MOM-selected RECEIVE order unexpectedly returned a retraction designator");
  waitFor(
      observer,
      [&] { return observer.recorder().timedInteractions().size() >= 1U; },
      options,
      "Change Interaction Order Type timestamp-bearing interaction");
  auto const deliveries = observer.recorder().timedInteractions();
  require(
      deliveries.size() == 1U &&
          deliveries.front().interaction == observerInteraction &&
          deliveries.front().parameters.empty() &&
          deliveries.front().tag == tagBytes &&
          deliveries.front().time == encodeTime(*timestamp) &&
          deliveries.front().producer == subject.federateHandle() &&
          deliveries.front().sentOrder == rti::RECEIVE &&
          deliveries.front().receivedOrder == rti::RECEIVE &&
          !deliveries.front().retractionPresent,
      "HLAchangeInteractionOrderType did not change the later interaction delivery to RECEIVE order");

  subjectAmbassador.disableTimeRegulation();
  observerAmbassador.unsubscribeInteractionClass(observerInteraction);
  subjectAmbassador.unpublishInteractionClass(subjectInteraction);
  subjectAmbassador.unpublishInteractionClass(requestClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  subjectAmbassador.destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioFederationMomChangeInteractionOrderTypeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomChangeInteractionOrderType(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
