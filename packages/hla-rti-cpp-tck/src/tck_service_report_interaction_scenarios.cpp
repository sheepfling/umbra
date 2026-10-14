#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportDirectedInteractionDeclaration(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Directed-interaction declaration service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "directed-interaction-declaration-service-report-subject");
  Session observer(options, model, "directed-interaction-declaration-service-report-observer");
  auto const federation = federationName(
      options,
      "service-report-directed-interaction-declaration");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-directed-interaction-declaration-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-directed-interaction-declaration-service-report-observer",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      objectClass.isValid() && interactionClass.isValid(),
      "Directed-interaction declaration service-report lookup returned an invalid object or interaction handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Directed-interaction declaration service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
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
        "Directed-interaction declaration service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const subscribeArguments = [&](std::wstring const& objectValue,
                                      std::wstring const& interactionSetValue,
                                      bool universal) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {28, L"Set of directed interaction designators", interactionSetValue},
        {6, L"Optional universal subscription indicator", universal ? L"true" : L"false"}};
  };
  auto const unsubscribeArguments = [&](std::wstring const& objectValue,
                                        std::wstring const& interactionSetValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {28, L"Optional set of directed interaction designators", interactionSetValue}};
  };
  auto const wholeUnsubscribeArguments = [&](std::wstring const& objectValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {34, L"Optional set of directed interaction designators", L"null"}};
  };

  auto verifyReport = [&](std::size_t index,
                          std::wstring const& expectedService,
                          std::vector<ExpectedArgument> const& expectedArguments,
                          bool expectedSuccess,
                          std::wstring const& expectedException) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        "directed-interaction declaration HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Directed-interaction declaration service-report callback returned the wrong identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Directed-interaction declaration service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Directed-interaction declaration service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Directed-interaction declaration service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == expectedService,
        "Directed-interaction declaration service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 1,
        "Directed-interaction declaration service-report callback returned the wrong service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        success.get() == expectedSuccess,
        "Directed-interaction declaration service-report callback returned the wrong success indicator");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == expectedArguments.size(),
        "Directed-interaction declaration service-report callback returned the wrong supplied-argument count");
    for (std::size_t argumentIndex = 0U;
         argumentIndex != expectedArguments.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() ==
                  expectedArguments[argumentIndex].type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() ==
                  expectedArguments[argumentIndex].name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedArguments[argumentIndex].value,
          "Directed-interaction declaration service-report callback returned the wrong supplied-argument value");
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        "Directed-interaction declaration service-report callback returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    if (expectedException.empty()) {
      require(
          exception.get().empty(),
          "Successful directed-interaction declaration service report carried an exception");
    } else {
      require(
          exception.get().find(expectedException) != std::wstring::npos,
          "Directed-interaction declaration service report returned the wrong exception name");
    }
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == static_cast<std::int32_t>(index),
        "Directed-interaction declaration service-report callback returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const invalidObject = rti::ObjectClassHandle{}.toString();
  auto const validObject = objectClass.toString();
  auto const invalidInteraction = rti::InteractionClassHandle{}.toString();
  auto const validInteraction = interactionClass.toString();
  auto const invalidSet = L"[\"" + invalidInteraction + L"\"]";
  auto const validSet = L"[\"" + validInteraction + L"\"]";
  rti::InteractionClassHandleSet const interactions{interactionClass};

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  requireException(
      [&] {
        subject.rtiAmbassador().subscribeObjectClassDirectedInteractions(
            rti::ObjectClassHandle{},
            rti::InteractionClassHandleSet{},
            false);
      },
      L"ObjectClassNotDefined",
      "subscribing directed interactions for an undefined object class with service reporting enabled");
  verifyReport(
      0U,
      L"SubscribeObjectClassDirectedInteractions",
      subscribeArguments(invalidObject, L"[]", false),
      false,
      L"ObjectClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().subscribeObjectClassDirectedInteractions(
            objectClass,
            rti::InteractionClassHandleSet{rti::InteractionClassHandle{}},
            true);
      },
      L"InteractionClassNotDefined",
      "subscribing an undefined directed interaction with service reporting enabled");
  verifyReport(
      1U,
      L"SubscribeObjectClassDirectedInteractions",
      subscribeArguments(validObject, invalidSet, true),
      false,
      L"InteractionClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
            rti::ObjectClassHandle{},
            rti::InteractionClassHandleSet{});
      },
      L"ObjectClassNotDefined",
      "unsubscribing directed interactions for an undefined object class with service reporting enabled");
  verifyReport(
      2U,
      L"UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArguments(invalidObject, L"[]"),
      false,
      L"ObjectClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
            objectClass,
            rti::InteractionClassHandleSet{rti::InteractionClassHandle{}});
      },
      L"InteractionClassNotDefined",
      "unsubscribing an undefined directed interaction with service reporting enabled");
  verifyReport(
      3U,
      L"UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArguments(validObject, invalidSet),
      false,
      L"InteractionClassNotDefined");

  subject.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      objectClass,
      interactions,
      false);
  verifyReport(
      4U,
      L"SubscribeObjectClassDirectedInteractions",
      subscribeArguments(validObject, validSet, false),
      true,
      L"");
  subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      objectClass,
      interactions);
  verifyReport(
      5U,
      L"UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArguments(validObject, validSet),
      true,
      L"");
  subject.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(objectClass);
  verifyReport(
      6U,
      L"UnsubscribeObjectClassDirectedInteractions",
      wholeUnsubscribeArguments(validObject),
      true,
      L"");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDirectedInteractionPublication(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Directed-interaction publication service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "directed-interaction-publication-service-report-subject");
  Session observer(options, model, "directed-interaction-publication-service-report-observer");
  auto const federation = federationName(
      options,
      "service-report-directed-interaction-publication");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-directed-interaction-publication-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-directed-interaction-publication-service-report-observer",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      objectClass.isValid() && interactionClass.isValid(),
      "Directed-interaction publication service-report lookup returned an invalid object or interaction handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Directed-interaction publication service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
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
        "Directed-interaction publication service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const publishArguments = [&](std::wstring const& objectValue,
                                    std::wstring const& interactionSetValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {28, L"Set of interaction class designators", interactionSetValue}};
  };
  auto const unpublishArguments = [&](std::wstring const& objectValue,
                                      std::wstring const& interactionSetValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {28, L"Optional set of interaction class designators", interactionSetValue}};
  };
  auto const wholeUnpublishArguments = [&](std::wstring const& objectValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {34, L"Optional set of interaction class designators", L"null"}};
  };

  auto verifyReport = [&](std::size_t index,
                          std::wstring const& expectedService,
                          std::vector<ExpectedArgument> const& expectedArguments,
                          bool expectedSuccess,
                          std::wstring const& expectedException) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        "directed-interaction publication HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Directed-interaction publication service-report callback returned the wrong identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Directed-interaction publication service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Directed-interaction publication service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Directed-interaction publication service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == expectedService,
        "Directed-interaction publication service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 1,
        "Directed-interaction publication service-report callback returned the wrong service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        success.get() == expectedSuccess,
        "Directed-interaction publication service-report callback returned the wrong success indicator");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == expectedArguments.size(),
        "Directed-interaction publication service-report callback returned the wrong supplied-argument count");
    for (std::size_t argumentIndex = 0U;
         argumentIndex != expectedArguments.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() ==
                  expectedArguments[argumentIndex].type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() ==
                  expectedArguments[argumentIndex].name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedArguments[argumentIndex].value,
          "Directed-interaction publication service-report callback returned the wrong supplied-argument value");
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        "Directed-interaction publication service-report callback returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    if (expectedException.empty()) {
      require(
          exception.get().empty(),
          "Successful directed-interaction publication service report carried an exception");
    } else {
      require(
          exception.get().find(expectedException) != std::wstring::npos,
          "Directed-interaction publication service report returned the wrong exception name");
    }
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == static_cast<std::int32_t>(index),
        "Directed-interaction publication service-report callback returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const invalidObject = rti::ObjectClassHandle{}.toString();
  auto const validObject = objectClass.toString();
  auto const invalidInteraction = rti::InteractionClassHandle{}.toString();
  auto const validInteraction = interactionClass.toString();
  auto const invalidSet = L"[\"" + invalidInteraction + L"\"]";
  auto const validSet = L"[\"" + validInteraction + L"\"]";
  rti::InteractionClassHandleSet const interactions{interactionClass};

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  requireException(
      [&] {
        subject.rtiAmbassador().publishObjectClassDirectedInteractions(
            rti::ObjectClassHandle{},
            rti::InteractionClassHandleSet{});
      },
      L"ObjectClassNotDefined",
      "publishing directed interactions for an undefined object class with service reporting enabled");
  verifyReport(
      0U,
      L"PublishObjectClassDirectedInteractions",
      publishArguments(invalidObject, L"[]"),
      false,
      L"ObjectClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().publishObjectClassDirectedInteractions(
            objectClass,
            rti::InteractionClassHandleSet{rti::InteractionClassHandle{}});
      },
      L"InteractionClassNotDefined",
      "publishing an undefined directed interaction with service reporting enabled");
  verifyReport(
      1U,
      L"PublishObjectClassDirectedInteractions",
      publishArguments(validObject, invalidSet),
      false,
      L"InteractionClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(
            rti::ObjectClassHandle{},
            rti::InteractionClassHandleSet{});
      },
      L"ObjectClassNotDefined",
      "unpublishing directed interactions for an undefined object class with service reporting enabled");
  verifyReport(
      2U,
      L"UnpublishObjectClassDirectedInteractions",
      unpublishArguments(invalidObject, L"[]"),
      false,
      L"ObjectClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(
            objectClass,
            rti::InteractionClassHandleSet{rti::InteractionClassHandle{}});
      },
      L"InteractionClassNotDefined",
      "unpublishing an undefined directed interaction with service reporting enabled");
  verifyReport(
      3U,
      L"UnpublishObjectClassDirectedInteractions",
      unpublishArguments(validObject, invalidSet),
      false,
      L"InteractionClassNotDefined");

  subject.rtiAmbassador().publishObjectClassDirectedInteractions(
      objectClass,
      interactions);
  verifyReport(
      4U,
      L"PublishObjectClassDirectedInteractions",
      publishArguments(validObject, validSet),
      true,
      L"");
  subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      objectClass,
      interactions);
  verifyReport(
      5U,
      L"UnpublishObjectClassDirectedInteractions",
      unpublishArguments(validObject, validSet),
      true,
      L"");
  subject.rtiAmbassador().unpublishObjectClassDirectedInteractions(objectClass);
  verifyReport(
      6U,
      L"UnpublishObjectClassDirectedInteractions",
      wholeUnpublishArguments(validObject),
      true,
      L"");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportInteractionPublication(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Interaction publication service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "interaction-publication-service-report-subject");
  Session observer(options, model, "interaction-publication-service-report-observer");
  auto const federation = federationName(
      options,
      "service-report-interaction-publication");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-interaction-publication-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-interaction-publication-service-report-observer",
      options.federateType,
      federation);

  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      interactionClass.isValid(),
      "Interaction publication service-report lookup returned an invalid interaction handle");
  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Interaction publication service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
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
        "Interaction publication service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const argument = [&](std::wstring const& value) {
    return std::vector<ExpectedArgument>{
        {27, L"Interaction class designator", quoted(value)}};
  };

  auto verifyReport = [&](std::size_t index,
                          std::wstring const& expectedService,
                          std::wstring const& handleValue,
                          bool expectedSuccess,
                          std::wstring const& expectedException) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        "interaction publication HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Interaction publication service-report callback returned the wrong identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Interaction publication service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Interaction publication service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Interaction publication service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == expectedService,
        "Interaction publication service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 1,
        "Interaction publication service-report callback returned the wrong service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        success.get() == expectedSuccess,
        "Interaction publication service-report callback returned the wrong success indicator");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    auto const expectedArguments = argument(handleValue);
    require(
        suppliedArguments.size() == expectedArguments.size(),
        "Interaction publication service-report callback returned the wrong supplied-argument count");
    for (std::size_t argumentIndex = 0U;
         argumentIndex != expectedArguments.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() ==
                  expectedArguments[argumentIndex].type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() ==
                  expectedArguments[argumentIndex].name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedArguments[argumentIndex].value,
          "Interaction publication service-report callback returned the wrong supplied-argument value");
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        "Interaction publication service-report callback returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    if (expectedException.empty()) {
      require(
          exception.get().empty(),
          "Successful interaction publication service report carried an exception");
    } else {
      require(
          exception.get().find(expectedException) != std::wstring::npos,
          "Interaction publication service report returned the wrong exception name");
    }
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == static_cast<std::int32_t>(index),
        "Interaction publication service-report callback returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const invalid = rti::InteractionClassHandle{}.toString();
  auto const valid = interactionClass.toString();
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  requireException(
      [&] { subject.rtiAmbassador().publishInteractionClass(rti::InteractionClassHandle{}); },
      L"InteractionClassNotDefined",
      "publishing an undefined interaction class with service reporting enabled");
  verifyReport(
      0U,
      L"PublishInteractionClass",
      invalid,
      false,
      L"InteractionClassNotDefined");
  requireException(
      [&] { subject.rtiAmbassador().unpublishInteractionClass(rti::InteractionClassHandle{}); },
      L"InteractionClassNotDefined",
      "unpublishing an undefined interaction class with service reporting enabled");
  verifyReport(
      1U,
      L"UnpublishInteractionClass",
      invalid,
      false,
      L"InteractionClassNotDefined");
  subject.rtiAmbassador().publishInteractionClass(interactionClass);
  verifyReport(2U, L"PublishInteractionClass", valid, true, L"");
  subject.rtiAmbassador().unpublishInteractionClass(interactionClass);
  verifyReport(3U, L"UnpublishInteractionClass", valid, true, L"");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportInteractionSubscription(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Interaction subscription service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "interaction-subscription-service-report-subject");
  Session observer(options, model, "interaction-subscription-service-report-observer");
  auto const federation = federationName(
      options,
      "service-report-interaction-subscription");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-interaction-subscription-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-interaction-subscription-service-report-observer",
      options.federateType,
      federation);

  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      interactionClass.isValid(),
      "Interaction subscription service-report lookup returned an invalid interaction handle");
  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Interaction subscription service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
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
        "Interaction subscription service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const subscribeArguments = [&](std::wstring const& value, bool passive) {
    return std::vector<ExpectedArgument>{
        {27, L"Interaction class designator", quoted(value)},
        {6, L"Optional passive subscription indicator", passive ? L"true" : L"false"}};
  };
  auto const unsubscribeArguments = [&](std::wstring const& value) {
    return std::vector<ExpectedArgument>{
        {27, L"Interaction class designator", quoted(value)}};
  };

  auto verifyReport = [&](std::size_t index,
                          std::wstring const& expectedService,
                          std::vector<ExpectedArgument> const& expectedArguments,
                          bool expectedSuccess,
                          std::wstring const& expectedException) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        "interaction subscription HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Interaction subscription service-report callback returned the wrong identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Interaction subscription service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Interaction subscription service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Interaction subscription service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == expectedService,
        "Interaction subscription service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 1,
        "Interaction subscription service-report callback returned the wrong service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        success.get() == expectedSuccess,
        "Interaction subscription service-report callback returned the wrong success indicator");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == expectedArguments.size(),
        "Interaction subscription service-report callback returned the wrong supplied-argument count");
    for (std::size_t argumentIndex = 0U;
         argumentIndex != expectedArguments.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() ==
                  expectedArguments[argumentIndex].type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() ==
                  expectedArguments[argumentIndex].name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedArguments[argumentIndex].value,
          "Interaction subscription service-report callback returned the wrong supplied-argument value");
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        "Interaction subscription service-report callback returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    if (expectedException.empty()) {
      require(
          exception.get().empty(),
          "Successful interaction subscription service report carried an exception");
    } else {
      require(
          exception.get().find(expectedException) != std::wstring::npos,
          "Interaction subscription service report returned the wrong exception name");
    }
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == static_cast<std::int32_t>(index),
        "Interaction subscription service-report callback returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const invalid = rti::InteractionClassHandle{}.toString();
  auto const valid = interactionClass.toString();
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  requireException(
      [&] {
        subject.rtiAmbassador().subscribeInteractionClass(
            rti::InteractionClassHandle{},
            false);
      },
      L"InteractionClassNotDefined",
      "subscribing an undefined interaction class with service reporting enabled");
  verifyReport(
      0U,
      L"SubscribeInteractionClass",
      subscribeArguments(invalid, true),
      false,
      L"InteractionClassNotDefined");
  requireException(
      [&] { subject.rtiAmbassador().unsubscribeInteractionClass(rti::InteractionClassHandle{}); },
      L"InteractionClassNotDefined",
      "unsubscribing an undefined interaction class with service reporting enabled");
  verifyReport(
      1U,
      L"UnsubscribeInteractionClass",
      unsubscribeArguments(invalid),
      false,
      L"InteractionClassNotDefined");
  subject.rtiAmbassador().subscribeInteractionClass(interactionClass, true);
  verifyReport(
      2U,
      L"SubscribeInteractionClass",
      subscribeArguments(valid, false),
      true,
      L"");
  subject.rtiAmbassador().unsubscribeInteractionClass(interactionClass);
  verifyReport(
      3U,
      L"UnsubscribeInteractionClass",
      unsubscribeArguments(valid),
      true,
      L"");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportNextMessageRequestAvailableBoundary(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Next Message Request Available service-report testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "service-report-nmra-publisher");
  Session subject(options, model, "service-report-nmra-subject");
  Session observer(options, model, "service-report-nmra-observer");
  auto const federation = federationName(options, "service-report-nmra-boundary");
  publisher.connect();
  subject.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-nmra-publisher",
      options.federateType,
      federation);
  subject.join(
      options.memberFederateName + L"-service-report-nmra-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-nmra-observer",
      options.federateType,
      federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const subjectInteraction = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const subjectParameter = subject.rtiAmbassador().getParameterHandle(
      subjectInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          subjectInteraction.isValid() && subjectParameter.isValid(),
      "Next Message Request Available service-report interaction lookup returned an invalid handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Next Message Request Available service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
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
        "Next Message Request Available service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  subject.rtiAmbassador().subscribeInteractionClass(subjectInteraction, true);

  auto const publisherTime = makeTimeContext(publisher);
  auto const subjectTime = makeTimeContext(subject);
  require(
      publisherTime.factory->getName() == subjectTime.factory->getName(),
      "Next Message Request Available service-report members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(lookahead != nullptr, "logical-time factory returned no service-report lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  subject.rtiAmbassador().enableTimeConstrained();
  waitFor(
      subject,
      [&] { return subject.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Next Message Request Available service-report constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Next Message Request Available service-report regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const requestBoundary = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      10U);
  std::vector<std::uint8_t> const payload{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> const tag{0x54U, 0x43U, 0x4BU};
  rti::ParameterHandleValueMap values;
  values.emplace(
      publisherParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      values,
      rti::VariableLengthData(tag.data(), tag.size()),
      *messageTime);
  require(
      retraction.isValid() && subject.recorder().timedInteractions().empty(),
      "Next Message Request Available service-report setup failed to queue a timestamped interaction");

  observer.recorder().clearInteraction();
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "Next Message Request Available HLAreportServiceInvocation callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass && report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.regions.has_value() && !report.producer.isValid(),
      "Next Message Request Available service report returned incorrect interaction metadata");
  for (auto const parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "Next Message Request Available service report omitted a standard parameter");
  }
  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      service.get() == L"NextMessageRequestAvailable",
      "Next Message Request Available service report returned the wrong service name");
  require(
      serviceType.get() == 4,
      "Next Message Request Available service report did not use the standard time-management service type");
  require(
      success.get(),
      "Next Message Request Available service report did not report success");
  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray arguments{argumentPrototype};
  arguments.decode(report.parameters.at(reportParameters[3]));
  require(
      arguments.size() == 1U,
      "Next Message Request Available service report returned the wrong argument count");
  auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(arguments.get(0U));
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() == 31,
      "Next Message Request Available service report returned the wrong argument type");
  require(
      dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() == L"Logical time",
      "Next Message Request Available service report returned the wrong argument name");
  auto const reportedBoundary =
      dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get();
  auto const expectedBoundary = L"\"" + requestBoundary->toString() + L"\"";
  require(
      reportedBoundary == expectedBoundary,
      "Next Message Request Available service report boundary mismatch: got " +
          toNarrow(reportedBoundary) + " expected " + toNarrow(expectedBoundary));
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      exception.get().empty() && serial.get() == 0,
      "Successful Next Message Request Available service report returned an exception or wrong serial");

  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      subject,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 1U &&
            subject.recorder().timeAdvanceGrants().size() == 1U &&
            subject.recorder().timedInteractions().size() == 1U;
      },
      options,
      "Next Message Request Available service-report grant at the queued message time");
  require(
      subject.recorder().timedInteractions().front().time == encodeTime(*messageTime) &&
          subject.recorder().timeAdvanceGrants().front().encoded == encodeTime(*messageTime),
      "Next Message Request Available service-report scenario did not deliver and grant at the queued time");

  subject.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  subject.rtiAmbassador().unsubscribeInteractionClass(subjectInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
  publisher.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
