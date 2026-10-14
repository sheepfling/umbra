#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportRegionalInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Regional service-report interaction testing requires an adapter-supplied dimensional FOM");
  require(
      !options.mimFom.empty(),
      "Regional service-report interaction testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "regional-service-report-publisher");
  Session receiver(options, model, "regional-service-report-receiver");
  Session observer(options, model, "regional-service-report-observer");
  auto const federation = federationName(
      options,
      "service-report-regional-interaction");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.ddmFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-regional-service-report-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-regional-service-report-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-regional-service-report-observer",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "regional service-report interaction");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Regional service-report interaction lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Regional service-report interaction parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep setup calls out of the asserted report. The regional send below is
  // the only service invocation whose report is accepted by this scenario.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet,
      true);
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  publisher.recorder().clearInteraction();
  receiver.recorder().clearInteraction();
  observer.recorder().clearInteraction();

  std::vector<std::uint8_t> const valueBytes{0x52U, 0x45U, 0x47U};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x53U, 0x52U};
  rti::ParameterHandleValueMap values;
  values.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendInteractionWithRegions(
      publisherHandles.interactionClass,
      values,
      sourceRegionSet,
      tag);

  waitFor(
      receiver,
      observer,
      [&] {
        return receiver.recorder().interaction().present &&
            observer.recorder().interaction().present;
      },
      options,
      "regional application and HLAreportServiceInvocation callbacks");

  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Regional service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Regional service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Regional service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Regional service-report callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"SendInteractionWithRegions",
      "Regional service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Regional service-report callback returned the wrong interaction service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Regional service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful regional service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() == 0,
      "First regional service report did not use serial number zero");

  auto const interaction = receiver.recorder().interaction();
  require(
      interaction.interaction == receiverHandles.interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverHandles.parameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverHandles.parameter)) == valueBytes,
      "Regional application callback returned the wrong class, parameter, or value");
  require(
      interaction.tag == tagBytes &&
          interaction.producer == publisher.federateHandle() &&
          interaction.transportation.isValid(),
      "Regional application callback did not preserve ordinary interaction metadata");
  require(
      interaction.regions.has_value() &&
          interaction.regions->size() == 1U &&
          interaction.regions->count(sourceRegion) == 1U,
      "Regional application callback did not convey the source region designator set");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  observer.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioServiceReportRegionalInteractionSubscription(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Regional interaction subscription service-report testing requires an adapter-supplied dimensional FOM");
  require(
      !options.mimFom.empty(),
      "Regional interaction subscription service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "regional-subscription-service-report-subject");
  Session observer(options, model, "regional-subscription-service-report-observer");
  auto const federation = federationName(
      options,
      "service-report-regional-interaction-subscription");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.ddmFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-regional-subscription-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-regional-subscription-service-report-observer",
      options.federateType,
      federation);

  auto const handles = ddmHandles(subject, options);
  verifyDdmClassDimensions(
      subject,
      options,
      handles,
      "regional interaction subscription service-report");
  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      interactionClass.isValid() && dimension.isValid(),
      "Regional interaction subscription service-report lookup returned an invalid handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Regional interaction subscription service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Regional interaction subscription service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const region = createDdmRegion(subject, handles, 0UL, 10UL, 0UL, 10UL);
  auto const classValue = interactionClass.toString();
  auto const regionValue = region.toString();

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const associationArguments = [&](bool passive) {
    return std::vector<ExpectedArgument>{
        {27, L"Interaction class designator", quoted(classValue)},
        {43, L"Set of region designators", L"[\"" + regionValue + L"\"]"},
        {6, L"Optional passive subscription indicator", passive ? L"true" : L"false"},
    };
  };
  auto const unsubscribeArguments = std::vector<ExpectedArgument>{
      {27, L"Interaction class designator", quoted(classValue)},
      {43, L"Set of region designators", L"[\"" + regionValue + L"\"]"},
  };

  auto verifyReport = [&](std::size_t index,
                          std::wstring const& expectedService,
                          std::vector<ExpectedArgument> const& expectedArguments) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        "regional interaction subscription HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Regional interaction subscription service-report callback returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Regional interaction subscription service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Regional interaction subscription service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Regional interaction subscription service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == expectedService,
        "Regional interaction subscription service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 5,
        "Regional interaction subscription service-report callback returned the wrong service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        success.get(),
        "Regional interaction subscription service-report callback did not report a successful invocation");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == expectedArguments.size(),
        "Regional interaction subscription service-report callback returned the wrong supplied-argument count");
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
          "Regional interaction subscription service-report callback returned the wrong supplied-argument value");
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() == L"null",
        "Regional interaction subscription service-report callback returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    require(
        exception.get().empty(),
        "Successful regional interaction subscription service report carried an exception");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == static_cast<std::int32_t>(index),
        "Regional interaction subscription service-report callback returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.rtiAmbassador().subscribeInteractionClassWithRegions(
      interactionClass,
      rti::RegionHandleSet{region},
      false);
  verifyReport(
      0U,
      L"SubscribeInteractionClassWithRegions",
      associationArguments(true));
  subject.rtiAmbassador().subscribeInteractionClassWithRegions(
      interactionClass,
      rti::RegionHandleSet{region},
      true);
  verifyReport(
      1U,
      L"SubscribeInteractionClassWithRegions",
      associationArguments(false));
  subject.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      interactionClass,
      rti::RegionHandleSet{region});
  verifyReport(
      2U,
      L"UnsubscribeInteractionClassWithRegions",
      unsubscribeArguments);

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().deleteRegion(region);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
