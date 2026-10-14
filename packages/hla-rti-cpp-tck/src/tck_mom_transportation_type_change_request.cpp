#include "tck_mom_transportation_type_change_request.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioMomTransportationTypeChangeRequest(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "MOM transportation-type request testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "MOM transportation-type request testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "mom-transportation-subject");
  Session observer(options, model, "mom-transportation-observer");
  auto const federation = federationName(
      options,
      "mom-transportation-type-change-request");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-mom-transportation-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-transportation-observer",
      options.federateType,
      federation);

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  rti::ObjectClassHandle subjectObjectClass;
  rti::AttributeHandle subjectAttribute;
  rti::InteractionClassHandle subjectInteraction;
  rti::ParameterHandle subjectParameter;
  handles(
      subject,
      options,
      subjectObjectClass,
      subjectAttribute,
      subjectInteraction,
      subjectParameter);
  rti::ObjectClassHandle observerObjectClass;
  rti::AttributeHandle observerAttribute;
  rti::InteractionClassHandle observerInteraction;
  rti::ParameterHandle observerParameter;
  handles(
      observer,
      options,
      observerObjectClass,
      observerAttribute,
      observerInteraction,
      observerParameter);
  require(
      subjectObjectClass == observerObjectClass &&
          subjectAttribute == observerAttribute &&
          subjectInteraction == observerInteraction &&
          subjectParameter == observerParameter &&
          observer.rtiAmbassador().getObjectClassName(observerObjectClass) ==
              options.objectClassName &&
          observer.rtiAmbassador().getAttributeName(
              observerObjectClass,
              observerAttribute) == options.attributeName &&
          observer.rtiAmbassador().getInteractionClassName(observerInteraction) ==
              options.interactionClassName &&
          observer.rtiAmbassador().getParameterName(
              observerInteraction,
              observerParameter) == options.parameterName,
      "MOM transportation-type request adapter declarations did not round-trip");

  auto const reliable = subject.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const bestEffort = subject.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() &&
          subject.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable" &&
          subject.rtiAmbassador().getTransportationTypeName(bestEffort) ==
              L"HLAbestEffort",
      "MOM transportation-type request could not resolve the standard transport pair");

  rti::AttributeHandleSet const subjectAttributes{subjectAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  subject.rtiAmbassador().publishObjectClassAttributes(
      subjectObjectClass,
      subjectAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes,
      true,
      L"");
  subject.rtiAmbassador().publishInteractionClass(subjectInteraction);
  subject.rtiAmbassador().subscribeInteractionClass(subjectInteraction, true);
  observer.rtiAmbassador().publishInteractionClass(observerInteraction);
  observer.rtiAmbassador().subscribeInteractionClass(observerInteraction, true);

  auto const object = subject.rtiAmbassador().registerObjectInstance(
      subjectObjectClass);
  require(
      object.isValid(),
      "MOM transportation-type request object registration returned an invalid handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "MOM transportation-type request object discovery");

  std::vector<std::uint8_t> const baselineValueBytes{0x42U, 0x41U};
  std::vector<std::uint8_t> const baselineTagBytes{0x42U};
  rti::ParameterHandleValueMap baselineParameters;
  baselineParameters.emplace(
      observerParameter,
      rti::VariableLengthData(
          baselineValueBytes.data(),
          baselineValueBytes.size()));
  rti::VariableLengthData const baselineTag(
      baselineTagBytes.data(),
      baselineTagBytes.size());
  subject.recorder().clearInteraction();
  observer.rtiAmbassador().sendInteraction(
      observerInteraction,
      baselineParameters,
      baselineTag);
  waitFor(
      subject,
      [&] { return subject.recorder().interaction().present; },
      options,
      "MOM transportation-type request baseline interaction");
  auto const baselineInteraction = subject.recorder().interaction();
  require(
      baselineInteraction.interaction == subjectInteraction &&
          baselineInteraction.parameters.size() == 1U &&
          copyBytes(baselineInteraction.parameters.at(subjectParameter)) ==
              baselineValueBytes &&
          baselineInteraction.tag == baselineTagBytes &&
          baselineInteraction.transportation == reliable &&
          baselineInteraction.producer == observer.federateHandle() &&
          !baselineInteraction.regions.has_value(),
      "MOM transportation-type request baseline interaction metadata was not reliable");

  auto encodeAttributeHandleList = [](rti::AttributeHandle const& attribute) {
    auto const encodedAttribute = copyBytes(attribute.encode());
    std::vector<std::uint8_t> bytes{0x00U, 0x00U, 0x00U, 0x01U};
    bytes.insert(bytes.end(), encodedAttribute.begin(), encodedAttribute.end());
    return rti::VariableLengthData(bytes.data(), bytes.size());
  };

  auto const requestAttributeClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestAttributeTransportationTypeChange");
  auto const requestAttributeObject = subject.rtiAmbassador().getParameterHandle(
      requestAttributeClass,
      L"HLAobjectInstance");
  auto const requestAttributeList = subject.rtiAmbassador().getParameterHandle(
      requestAttributeClass,
      L"HLAattributeList");
  auto const requestAttributeTransportation = subject.rtiAmbassador().getParameterHandle(
      requestAttributeClass,
      L"HLAtransportation");
  require(
      requestAttributeClass.isValid() && requestAttributeObject.isValid() &&
          requestAttributeList.isValid() && requestAttributeTransportation.isValid(),
      "MOM transportation-type request attribute interaction lookup failed");

  subject.recorder().clearTransportationRecords();
  subject.rtiAmbassador().sendInteraction(
      requestAttributeClass,
      rti::ParameterHandleValueMap{
          {requestAttributeObject, object.encode()},
          {requestAttributeList, encodeAttributeHandleList(subjectAttribute)},
          {requestAttributeTransportation, bestEffort.encode()}},
      rti::VariableLengthData{});
  waitFor(
      subject,
      [&] {
        return subject.recorder().attributeTransportationConfirmations().size() >= 1U;
      },
      options,
      "MOM transportation-type request attribute confirmation");
  auto const attributeConfirmations =
      subject.recorder().attributeTransportationConfirmations();
  require(
      attributeConfirmations.size() == 1U &&
          attributeConfirmations.front().object == object &&
          attributeConfirmations.front().attributes == subjectAttributes &&
          attributeConfirmations.front().transportation == bestEffort,
      "MOM transportation-type request attribute confirmation did not commit best effort");

  std::vector<std::uint8_t> const valueBytes{0xA1U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      subjectAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  observer.recorder().clearReflection();
  subject.rtiAmbassador().updateAttributeValues(
      object,
      values,
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return observer.recorder().reflection().present; },
      options,
      "MOM transportation-type request attribute delivery");
  auto const reflection = observer.recorder().reflection();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          copyBytes(reflection.values.at(observerAttribute)) == valueBytes &&
          reflection.transportation == bestEffort &&
          reflection.producer == subject.federateHandle() &&
          reflection.tag.empty() && !reflection.regions.has_value(),
      "MOM transportation-type request did not route the attribute as best effort");

  auto const requestInteractionClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestInteractionTransportationTypeChange");
  auto const requestInteractionTarget = subject.rtiAmbassador().getParameterHandle(
      requestInteractionClass,
      L"HLAinteractionClass");
  auto const requestInteractionTransportation = subject.rtiAmbassador().getParameterHandle(
      requestInteractionClass,
      L"HLAtransportation");
  require(
      requestInteractionClass.isValid() && requestInteractionTarget.isValid() &&
          requestInteractionTransportation.isValid(),
      "MOM transportation-type request interaction lookup failed");

  subject.recorder().clearInteraction();
  observer.rtiAmbassador().sendInteraction(
      observerInteraction,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});
  waitFor(
      subject,
      [&] { return subject.recorder().interaction().present; },
      options,
      "MOM transportation-type request independent publisher baseline");
  require(
      subject.recorder().interaction().transportation == reliable,
      "MOM transportation-type request changed the independent publisher");

  subject.recorder().clearTransportationRecords();
  subject.rtiAmbassador().sendInteraction(
      requestInteractionClass,
      rti::ParameterHandleValueMap{
          {requestInteractionTarget, subjectInteraction.encode()},
          {requestInteractionTransportation, bestEffort.encode()}},
      rti::VariableLengthData{});
  waitFor(
      subject,
      [&] {
        return subject.recorder().interactionTransportationConfirmations().size() >= 1U;
      },
      options,
      "MOM transportation-type request interaction confirmation");
  auto const interactionConfirmations =
      subject.recorder().interactionTransportationConfirmations();
  require(
      interactionConfirmations.size() == 1U &&
          interactionConfirmations.front().interaction == subjectInteraction &&
          interactionConfirmations.front().transportation == bestEffort,
      "MOM transportation-type request interaction confirmation did not commit best effort");

  std::vector<std::uint8_t> const changedTagBytes{0x54U};
  rti::VariableLengthData const changedTag(
      changedTagBytes.data(),
      changedTagBytes.size());
  observer.recorder().clearInteraction();
  subject.rtiAmbassador().sendInteraction(
      subjectInteraction,
      rti::ParameterHandleValueMap{},
      changedTag);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "MOM transportation-type request changed interaction delivery");
  auto const changedInteraction = observer.recorder().interaction();
  require(
      changedInteraction.interaction == observerInteraction &&
          changedInteraction.transportation == bestEffort &&
          changedInteraction.tag == changedTagBytes &&
          changedInteraction.producer == subject.federateHandle() &&
          !changedInteraction.regions.has_value(),
      "MOM transportation-type request did not route the interaction as best effort");

  subject.recorder().clearInteraction();
  observer.rtiAmbassador().sendInteraction(
      observerInteraction,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});
  waitFor(
      subject,
      [&] { return subject.recorder().interaction().present; },
      options,
      "MOM transportation-type request independent publisher post-change delivery");
  require(
      subject.recorder().interaction().transportation == reliable,
      "MOM transportation-type request changed the independent publisher after interaction change");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  auto const reportService = observer.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAservice");
  require(
      reportClass.isValid() && reportService.isValid(),
      "MOM transportation-type request service-report lookup failed");
  observer.rtiAmbassador().subscribeInteractionClass(reportClass, true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  observer.recorder().clearInteraction();
  subject.rtiAmbassador().sendInteraction(
      requestInteractionClass,
      rti::ParameterHandleValueMap{
          {requestInteractionTarget, subjectInteraction.encode()},
          {requestInteractionTransportation, reliable.encode()}},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "MOM transportation-type request service report");
  auto const serviceReport = observer.recorder().interaction();
  require(
      serviceReport.interaction == reportClass &&
          serviceReport.parameters.count(reportService) == 1U &&
          serviceReport.transportation == reliable &&
          !serviceReport.producer.isValid() && serviceReport.tag.empty() &&
          !serviceReport.regions.has_value(),
      "MOM transportation-type request service report metadata was not standard");
  rti::HLAunicodeString service;
  service.decode(serviceReport.parameters.at(reportService));
  require(
      service.get() == L"SendInteraction",
      "MOM transportation-type request service report named the wrong service");
  subject.rtiAmbassador().setServiceReportingSwitch(false);

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.rtiAmbassador().unsubscribeInteractionClass(observerInteraction);
  subject.rtiAmbassador().unsubscribeInteractionClass(subjectInteraction);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes);
  observer.rtiAmbassador().unpublishInteractionClass(observerInteraction);
  subject.rtiAmbassador().unpublishInteractionClass(subjectInteraction);
  subject.rtiAmbassador().unpublishObjectClassAttributes(
      subjectObjectClass,
      subjectAttributes);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail

