#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioMomPredefinedObjectAndInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Predefined MOM construct testing requires an application FOM, standard MIM, and logical-time configuration");

  Session subject(options, model, "predefined-mom-subject");
  Session observer(options, model, "predefined-mom-observer");
  auto const federation = federationName(options, "mom-predefined-object-interaction");
  auto const subjectName = options.ownerFederateName + L"-predefined-mom-subject";
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(subjectName, options.federateType, federation);
  observer.join(
      options.memberFederateName + L"-predefined-mom-observer",
      options.federateType,
      federation);

  std::vector<std::wstring> const figure20InteractionFamilyClasses{
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeRegulation",
  };
  for (auto const& className : figure20InteractionFamilyClasses) {
    auto const interactionClass =
        observer.rtiAmbassador().getInteractionClassHandle(className);
    require(
        interactionClass.isValid() &&
            observer.rtiAmbassador().getInteractionClassName(interactionClass) ==
                className,
        "A Figure 20 MOM interaction family or representative subclass did not resolve through the public API");
  }

  constexpr auto federateClassName =
      L"HLAobjectRoot.HLAmanager.HLAfederate";
  auto const federateClass =
      observer.rtiAmbassador().getObjectClassHandle(federateClassName);
  auto const federateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateClass, L"HLAfederateHandle");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateClass, L"HLAfederateName");
  require(
      federateClass.isValid() && federateHandleAttribute.isValid() &&
          federateNameAttribute.isValid() &&
          observer.rtiAmbassador().getObjectClassName(federateClass) ==
              federateClassName &&
          observer.rtiAmbassador().getAttributeName(
              federateClass, federateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              federateClass, federateNameAttribute) == L"HLAfederateName",
      "The standard HLAfederate MOM object definition did not resolve through the public API");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateClass,
      rti::AttributeHandleSet{federateHandleAttribute, federateNameAttribute},
      true,
      L"");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  auto const reportService = observer.rtiAmbassador().getParameterHandle(
      reportClass, L"HLAservice");
  auto const reportFederate = observer.rtiAmbassador().getParameterHandle(
      reportClass, L"HLAfederate");
  require(
      reportClass.isValid() && reportService.isValid() &&
          reportFederate.isValid() &&
          observer.rtiAmbassador().getInteractionClassName(reportClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation" &&
          observer.rtiAmbassador().getParameterName(reportClass, reportService) ==
              L"HLAservice" &&
          observer.rtiAmbassador().getParameterName(reportClass, reportFederate) ==
              L"HLAfederate",
      "The standard HLAreportServiceInvocation definition did not resolve through the public API");
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const applicationInteraction =
      subject.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const applicationParameter = subject.rtiAmbassador().getParameterHandle(
      applicationInteraction, options.parameterName);
  require(
      applicationInteraction.isValid() && applicationParameter.isValid(),
      "The adapter application interaction or parameter is unavailable");
  subject.rtiAmbassador().publishInteractionClass(applicationInteraction);

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  auto findSubjectReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          if (!reflection.present || !reflection.object.isValid() ||
              reflection.values.count(federateHandleAttribute) != 1U) {
            return false;
          }
          return observer.rtiAmbassador().decodeFederateHandle(
                     reflection.values.at(federateHandleAttribute)) ==
              subject.federateHandle();
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findSubjectReflection().has_value(); },
      options,
      "the joined federate's predefined HLAfederate MOM object instance");
  auto const subjectReflection = findSubjectReflection();
  require(
      subjectReflection.has_value() &&
          subjectReflection->values.count(federateNameAttribute) == 1U &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              subjectReflection->object) == federateClass,
      "The reflected joined-federate MOM object was not an instance of the predefined HLAfederate class");
  rti::HLAunicodeString reflectedFederateName;
  reflectedFederateName.decode(
      subjectReflection->values.at(federateNameAttribute));
  require(
      reflectedFederateName.get() == subjectName,
      "The predefined HLAfederate MOM object identified the wrong joined federate");

  observer.recorder().clearInteraction();
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const payload{0x4DU, 0x4FU, 0x4DU};
  rti::ParameterHandleValueMap const values{{
      applicationParameter,
      rti::VariableLengthData{payload.data(), payload.size()},
  }};
  subject.rtiAmbassador().sendInteraction(
      applicationInteraction, values, rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const interaction = observer.recorder().interaction();
        return interaction.present && interaction.interaction == reportClass;
      },
      options,
      "the predefined HLAreportServiceInvocation MOM interaction");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass && report.tag.empty() &&
          !report.producer.isValid() &&
          report.parameters.count(reportService) == 1U &&
          report.parameters.count(reportFederate) == 1U,
      "The predefined MOM interaction callback omitted standard identity or parameters");
  rti::HLAunicodeString reportedService;
  reportedService.decode(report.parameters.at(reportService));
  require(
      reportedService.get() == L"SendInteraction" &&
          observer.rtiAmbassador().decodeFederateHandle(
              report.parameters.at(reportFederate)) == subject.federateHandle(),
      "The predefined MOM interaction did not identify the public SendInteraction invocation");

  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioMomModifyAttributeState(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "MOM attribute-state adjustment requires an application FOM, standard MIM, and logical-time configuration");

  Session manager(options, model, "mom-attribute-state-manager");
  Session owner(options, model, "mom-attribute-state-owner");
  Session target(options, model, "mom-attribute-state-target");
  auto const federation = federationName(options, "mom-modify-attribute-state");
  manager.connect();
  owner.connect();
  target.connect();
  manager.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  manager.join(
      options.ownerFederateName + L"-mom-attribute-state-manager",
      options.federateType,
      federation);
  owner.join(
      options.ownerFederateName + L"-mom-attribute-state-owner",
      options.federateType,
      federation);
  target.join(
      options.memberFederateName + L"-mom-attribute-state-target",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const targetClass = target.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const targetAttribute = target.rtiAmbassador().getAttributeHandle(
      targetClass,
      options.attributeName);
  auto const modifyClass = manager.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState");
  auto const targetFederateParameter = manager.rtiAmbassador().getParameterHandle(
      modifyClass,
      L"HLAfederate");
  auto const objectParameter = manager.rtiAmbassador().getParameterHandle(
      modifyClass,
      L"HLAobjectInstance");
  auto const attributeParameter = manager.rtiAmbassador().getParameterHandle(
      modifyClass,
      L"HLAattribute");
  auto const stateParameter = manager.rtiAmbassador().getParameterHandle(
      modifyClass,
      L"HLAattributeState");
  auto const reportClass = manager.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
  auto const reportServiceParameter = manager.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAservice");
  auto const reportExceptionParameter = manager.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAexception");
  auto const reportParameterErrorParameter =
      manager.rtiAmbassador().getParameterHandle(reportClass, L"HLAparameterError");
  auto const reportFederateParameter = manager.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAfederate");
  auto const reliable = manager.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      ownerClass.isValid() && ownerAttribute.isValid() &&
          targetClass.isValid() && targetAttribute.isValid() &&
          modifyClass.isValid() && targetFederateParameter.isValid() &&
          objectParameter.isValid() && attributeParameter.isValid() &&
          stateParameter.isValid() && reportClass.isValid() &&
          reportServiceParameter.isValid() && reportExceptionParameter.isValid() &&
          reportParameterErrorParameter.isValid() &&
          reportFederateParameter.isValid() && reliable.isValid(),
      "Standard HLAmodifyAttributeState, HLAreportMOMexception, or application handle lookup returned an invalid handle");
  manager.rtiAmbassador().subscribeInteractionClass(reportClass, true);

  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  target.rtiAmbassador().subscribeObjectClassAttributes(
      targetClass,
      rti::AttributeHandleSet{targetAttribute},
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "HLAmodifyAttributeState setup returned an invalid object instance handle");
  waitFor(
      target,
      [&] { return target.recorder().hasDiscovery(object); },
      options,
      "HLAmodifyAttributeState target object discovery");
  require(
      target.rtiAmbassador().getKnownObjectClassHandle(object) == targetClass &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !target.rtiAmbassador().isAttributeOwnedByFederate(object, targetAttribute),
      "HLAmodifyAttributeState setup did not establish a known, published target and original owner");

  owner.recorder().clearOwnershipRecords();
  target.recorder().clearOwnershipRecords();
  manager.recorder().clearInteraction();
  rti::ParameterHandleValueMap const parameters{
      {targetFederateParameter, target.federateHandle().encode()},
      {objectParameter, object.encode()},
      {attributeParameter, targetAttribute.encode()},
      {stateParameter, rti::HLAinteger32BE{1}.encode()},
  };

  // The target knows the object but has not published its attribute. The MIM
  // requires no ownership change and an RTI-originated MOM exception report
  // to the managing federate for this failed adjustment precondition.
  try {
    manager.rtiAmbassador().sendInteraction(
        modifyClass,
        parameters,
        rti::VariableLengthData{});
  } catch (rti::Exception const&) {
    // A synchronous service exception does not replace the required MOM report.
  }
  auto hasMomExceptionReport = [&] {
    auto const reports = manager.recorder().interactions();
    return std::any_of(reports.begin(), reports.end(), [&](auto const& report) {
      return report.present && report.interaction == reportClass;
    });
  };
  waitFor(
      manager,
      hasMomExceptionReport,
      options,
      "HLAmodifyAttributeState target-publication precondition report");
  auto const momExceptionReports = manager.recorder().interactions();
  require(
      momExceptionReports.size() == 1U,
      "An invalid HLAmodifyAttributeState did not deliver exactly one HLAreportMOMexception");
  auto const& momExceptionReport = momExceptionReports.front();
  require(
      momExceptionReport.interaction == reportClass &&
          momExceptionReport.tag.empty() && !momExceptionReport.producer.isValid() &&
          momExceptionReport.transportation == reliable &&
          momExceptionReport.parameters.size() == 4U &&
          momExceptionReport.parameters.count(reportServiceParameter) == 1U &&
          momExceptionReport.parameters.count(reportExceptionParameter) == 1U &&
          momExceptionReport.parameters.count(reportParameterErrorParameter) == 1U &&
          momExceptionReport.parameters.count(reportFederateParameter) == 1U,
      "HLAmodifyAttributeState HLAreportMOMexception metadata or parameter set was not standard");
  rti::HLAunicodeString reportedService;
  rti::HLAunicodeString reportedException;
  rti::HLAboolean parameterError;
  reportedService.decode(momExceptionReport.parameters.at(reportServiceParameter));
  reportedException.decode(momExceptionReport.parameters.at(reportExceptionParameter));
  parameterError.decode(
      momExceptionReport.parameters.at(reportParameterErrorParameter));
  require(
      reportedService.get() ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState" &&
          !reportedException.get().empty() && !parameterError.get() &&
          manager.rtiAmbassador().decodeFederateHandle(
              momExceptionReport.parameters.at(reportFederateParameter)) ==
              manager.federateHandle(),
      "HLAmodifyAttributeState HLAreportMOMexception did not identify the service, managing federate, or non-parameter precondition failure");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !target.rtiAmbassador().isAttributeOwnedByFederate(object, targetAttribute),
      "Rejected HLAmodifyAttributeState changed ownership despite the target's missing publication");

  target.rtiAmbassador().publishObjectClassAttributes(
      targetClass,
      rti::AttributeHandleSet{targetAttribute});
  manager.rtiAmbassador().sendInteraction(
      modifyClass,
      parameters,
      rti::VariableLengthData{});

  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          target.rtiAmbassador().isAttributeOwnedByFederate(object, targetAttribute),
      "HLAmodifyAttributeState did not divest the original owner and grant the published attribute to the specified federate");
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
    static_cast<void>(target.evokeMultipleCallbacks(0.0, 1.0));
  }
  require(
      !owner.recorder().divestitureConfirmation().has_value() &&
          !target.recorder().ownershipAcquisition().has_value(),
      "HLAmodifyAttributeState emitted an ownership-change notification to a federate");

  target.resign(rti::DELETE_OBJECTS_THEN_DIVEST);
  manager.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  target.disconnect();
  manager.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
