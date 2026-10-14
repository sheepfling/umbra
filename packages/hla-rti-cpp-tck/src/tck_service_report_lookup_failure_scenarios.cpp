#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportFederateObjectClassLookupFailures(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Federate lookup failure service-report testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Federate lookup failure service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-federate-lookup-failure-subject");
  Session observer(options, model, "service-report-federate-lookup-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-federate-object-class-lookup-failures");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName =
      options.ownerFederateName + L"-service-report-federate-lookup-failure-subject";
  subject.join(subjectName, options.federateType, federation);
  observer.join(
      options.memberFederateName + L"-service-report-federate-lookup-failure-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Federate lookup failure service-report returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Federate lookup failure service-report returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const subjectFederate = subject.federateHandle();
  require(
      subjectFederate.isValid(),
      "Federate lookup failure service-report setup returned an invalid federate handle");
  auto const missingFederateName = std::wstring{L"TckMissingFederate"};
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  requireException(
      [&] { subject.rtiAmbassador().getFederateHandle(missingFederateName); },
      L"NameNotFound",
      "missing federate name lookup with service reporting enabled");
  requireException(
      [&] { subject.rtiAmbassador().getFederateName(rti::FederateHandle{}); },
      L"InvalidFederateHandle",
      "invalid federate handle lookup with service reporting enabled");
  auto const federateByName =
      subject.rtiAmbassador().getFederateHandle(subjectName);
  auto const federateName =
      subject.rtiAmbassador().getFederateName(subjectFederate);
  require(
      federateByName == subjectFederate && federateName == subjectName,
      "Federate lookup did not recover after reported failures");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() == 4U; },
      options,
      "federate lookup failure service-report callbacks");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 4U,
      "Federate lookup failure service-report count was unstable");
  requirePortableServiceReportFailure(
      observer,
      reports.at(0U),
      reportClass,
      reportParameters,
      L"GetFederateHandle",
      {{53, L"Federate name", quoted(missingFederateName)}},
      L"NameNotFound",
      0,
      "GetFederateHandle failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetFederateName",
      {{15, L"Federate handle", quoted(rti::FederateHandle{}.toString())}},
      L"InvalidFederateHandle",
      1,
      "GetFederateName failure service report");
  requirePortableServiceReport(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"GetFederateHandle",
      {{53, L"Federate name", quoted(subjectName)}},
      {15, L"Federate handle", quoted(federateByName.toString())},
      2,
      "GetFederateHandle recovery service report");
  requirePortableServiceReport(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"GetFederateName",
      {{15, L"Federate handle", quoted(subjectFederate.toString())}},
      {53, L"Federate name", quoted(federateName)},
      3,
      "GetFederateName recovery service report");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportFederateObjectClassLookupFailuresContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederateObjectClassLookupFailures(options, model);
}

void scenarioServiceReportInteractionParameterLookupFailures(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Interaction lookup failure service-report testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Interaction lookup failure service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-interaction-lookup-failure-subject");
  Session observer(options, model, "service-report-interaction-lookup-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-interaction-parameter-lookup-failures");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-interaction-lookup-failure-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-interaction-lookup-failure-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Interaction lookup failure service-report returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Interaction lookup failure service-report returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const interactionName = options.typedInteractionClassName;
  auto const parameterName = options.typedIntegerParameterName;
  auto const interaction =
      subject.rtiAmbassador().getInteractionClassHandle(interactionName);
  auto const parameter =
      subject.rtiAmbassador().getParameterHandle(interaction, parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "Interaction lookup failure service-report setup returned an invalid handle");

  auto const missingInteractionName =
      std::wstring{L"HLAinteractionRoot.TckMissingInteraction"};
  auto const missingParameterName = std::wstring{L"TckMissingParameter"};
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  requireException(
      [&] {
        subject.rtiAmbassador().getInteractionClassHandle(missingInteractionName);
      },
      L"NameNotFound",
      "missing interaction class lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getInteractionClassName(rti::InteractionClassHandle{});
      },
      L"InvalidInteractionClassHandle",
      "invalid interaction class handle lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getParameterHandle(interaction, missingParameterName);
      },
      L"NameNotFound",
      "missing parameter lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getParameterName(
            interaction,
            rti::ParameterHandle{});
      },
      L"InvalidParameterHandle",
      "invalid parameter handle lookup with service reporting enabled");
  auto const interactionByName =
      subject.rtiAmbassador().getInteractionClassHandle(interactionName);
  require(
      interactionByName == interaction,
      "Interaction class lookup did not recover after reported failures");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() == 5U; },
      options,
      "interaction/parameter lookup failure service-report callbacks");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 5U,
      "Interaction/parameter lookup failure service-report count was unstable");
  requirePortableServiceReportFailure(
      observer,
      reports.at(0U),
      reportClass,
      reportParameters,
      L"GetInteractionClassHandle",
      {{53, L"Interaction class name", quoted(missingInteractionName)}},
      L"NameNotFound",
      0,
      "GetInteractionClassHandle failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetInteractionClassName",
      {{27, L"Interaction class handle",
        quoted(rti::InteractionClassHandle{}.toString())}},
      L"InvalidInteractionClassHandle",
      1,
      "GetInteractionClassName failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"GetParameterHandle",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {53, L"Parameter name", quoted(missingParameterName)}},
      L"NameNotFound",
      2,
      "GetParameterHandle failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"GetParameterName",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {39, L"Parameter handle", quoted(rti::ParameterHandle{}.toString())}},
      L"InvalidParameterHandle",
      3,
      "GetParameterName failure service report");
  requirePortableServiceReport(
      observer,
      reports.at(4U),
      reportClass,
      reportParameters,
      L"GetInteractionClassHandle",
      {{53, L"Interaction class name", quoted(interactionName)}},
      {27, L"Interaction class handle", quoted(interactionByName.toString())},
      4,
      "GetInteractionClassHandle recovery service report");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportInteractionParameterLookupFailuresContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionParameterLookupFailures(options, model);
}

void scenarioServiceReportObjectAttributeUpdateRateLookupFailures(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Object lookup failure service-report testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Object lookup failure service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-object-lookup-failure-subject");
  Session observer(options, model, "service-report-object-lookup-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-object-attribute-update-rate-lookup-failures");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-object-lookup-failure-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-object-lookup-failure-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Object lookup failure service-report returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Object lookup failure service-report returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.rateBestEffortAttributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Object lookup failure service-report setup returned an invalid handle");

  auto const missingObjectName = std::wstring{L"TckMissingObjectInstance"};
  auto const missingRateName = std::wstring{L"TckMissingUpdateRate"};
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  requireException(
      [&] {
        subject.rtiAmbassador().getKnownObjectClassHandle(
            rti::ObjectInstanceHandle{});
      },
      L"ObjectInstanceNotKnown",
      "invalid object handle class lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getObjectInstanceHandle(missingObjectName);
      },
      L"ObjectInstanceNotKnown",
      "missing object name lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getObjectInstanceName(rti::ObjectInstanceHandle{});
      },
      L"ObjectInstanceNotKnown",
      "invalid object handle name lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getAttributeHandle(
            rti::ObjectClassHandle{},
            options.rateBestEffortAttributeName);
      },
      L"InvalidObjectClassHandle",
      "invalid object class attribute lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getAttributeName(
            objectClass,
            rti::AttributeHandle{});
      },
      L"InvalidAttributeHandle",
      "invalid attribute name lookup with service reporting enabled");
  requireException(
      [&] { subject.rtiAmbassador().getUpdateRateValue(missingRateName); },
      L"InvalidUpdateRateDesignator",
      "missing update-rate lookup with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getUpdateRateValueForAttribute(
            rti::ObjectInstanceHandle{},
            attribute);
      },
      L"ObjectInstanceNotKnown",
      "invalid object update-rate lookup with service reporting enabled");
  auto const objectClassByName =
      subject.rtiAmbassador().getObjectClassHandle(options.rateObjectClassName);
  require(
      objectClassByName == objectClass,
      "Object-class lookup did not recover after reported failures");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() == 8U; },
      options,
      "object/attribute/update-rate lookup failure service-report callbacks");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 8U,
      "Object/attribute/update-rate lookup failure service-report count was unstable");
  requirePortableServiceReportFailure(
      observer,
      reports.at(0U),
      reportClass,
      reportParameters,
      L"GetKnownObjectClassHandle",
      {{37, L"Object instance handle",
        quoted(rti::ObjectInstanceHandle{}.toString())}},
      L"ObjectInstanceNotKnown",
      0,
      "GetKnownObjectClassHandle failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetObjectInstanceHandle",
      {{53, L"Object instance name", quoted(missingObjectName)}},
      L"ObjectInstanceNotKnown",
      1,
      "GetObjectInstanceHandle failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"GetObjectInstanceName",
      {{37, L"Object instance handle",
        quoted(rti::ObjectInstanceHandle{}.toString())}},
      L"ObjectInstanceNotKnown",
      2,
      "GetObjectInstanceName failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"GetAttributeHandle",
      {{36, L"Object class handle",
        quoted(rti::ObjectClassHandle{}.toString())},
       {53, L"Class attribute name",
        quoted(options.rateBestEffortAttributeName)}},
      L"InvalidObjectClassHandle",
      3,
      "GetAttributeHandle failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(4U),
      reportClass,
      reportParameters,
      L"GetAttributeName",
      {{36, L"Object class handle", quoted(objectClass.toString())},
       {0, L"Class attribute handle",
        quoted(rti::AttributeHandle{}.toString())}},
      L"InvalidAttributeHandle",
      4,
      "GetAttributeName failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(5U),
      reportClass,
      reportParameters,
      L"GetUpdateRateValue",
      {{53, L"Update rate name", quoted(missingRateName)}},
      L"InvalidUpdateRateDesignator",
      5,
      "GetUpdateRateValue failure service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(6U),
      reportClass,
      reportParameters,
      L"GetUpdateRateValueForAttribute",
      {{37, L"Object instance handle",
        quoted(rti::ObjectInstanceHandle{}.toString())},
       {0, L"Attribute handle", quoted(attribute.toString())}},
      L"ObjectInstanceNotKnown",
      6,
      "GetUpdateRateValueForAttribute failure service report");
  requirePortableServiceReport(
      observer,
      reports.at(7U),
      reportClass,
      reportParameters,
      L"GetObjectClassHandle",
      {{53, L"Object class name", quoted(options.rateObjectClassName)}},
      {36, L"Object class handle", quoted(objectClassByName.toString())},
      7,
      "GetObjectClassHandle recovery service report");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportObjectAttributeUpdateRateLookupFailuresContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportObjectAttributeUpdateRateLookupFailures(options, model);
}


} // namespace hla_rti_cpp_tck::detail
