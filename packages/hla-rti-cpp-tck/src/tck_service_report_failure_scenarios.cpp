#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportRegionalInteractionFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Regional service-report failure testing requires an adapter-supplied dimensional FOM");
  require(
      !options.mimFom.empty(),
      "Regional service-report failure testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "regional-service-report-failure-publisher");
  Session receiver(options, model, "regional-service-report-failure-receiver");
  Session observer(options, model, "regional-service-report-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-regional-interaction-failure");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.ddmFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-regional-service-report-failure-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-regional-service-report-failure-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-regional-service-report-failure-observer",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "regional service-report failure");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Regional service-report failure lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Regional service-report failure parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

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
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x41U, 0x49U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::ParameterHandleValueMap validValues;
  validValues.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));

  auto const verifyFailureReport = [&](std::int32_t expectedSerial,
                                       std::wstring const& expectedException,
                                       std::string const& description) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        description + " HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        description + " report returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        description + " report returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        description + " report returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          description + " report omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == L"SendInteractionWithRegions",
        description + " report returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 2,
        description + " report returned the wrong interaction service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(!success.get(), description + " report incorrectly reported success");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == 5U,
        description + " report omitted standard supplied arguments");
    auto const verifyArgument = [&](std::size_t index,
                                    std::int32_t type,
                                    std::wstring const& name) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() == type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() == name,
          description + " report returned the wrong supplied-argument shape");
    };
    verifyArgument(0U, 27, L"Interaction class designator");
    verifyArgument(
        1U,
        40,
        L"Constrained set of interaction parameter designator and value pairs");
    verifyArgument(2U, 43, L"Set of region designators");
    verifyArgument(3U, 60, L"User-supplied tag");
    verifyArgument(4U, 34, L"Optional timestamp");

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
        description + " report returned the wrong Null return value");

    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    require(
        exception.get().find(expectedException) != std::wstring::npos,
        description + " report returned the wrong exception name");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == expectedSerial,
        description + " report returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteractionWithRegions(
            rti::InteractionClassHandle{},
            validValues,
            sourceRegionSet,
            tag);
      },
      L"InteractionClassNotDefined",
      "regional send with an invalid interaction class");
  verifyFailureReport(
      0,
      L"InteractionClassNotDefined",
      "invalid interaction-class regional send");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      !receiver.recorder().interaction().present,
      "invalid interaction-class regional send delivered an application callback");

  rti::ParameterHandleValueMap invalidParameterValues;
  invalidParameterValues.emplace(
      rti::ParameterHandle{},
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteractionWithRegions(
            publisherHandles.interactionClass,
            invalidParameterValues,
            sourceRegionSet,
            tag);
      },
      L"InteractionParameterNotDefined",
      "regional send with an invalid parameter");
  verifyFailureReport(
      1,
      L"InteractionParameterNotDefined",
      "invalid parameter regional send");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      !receiver.recorder().interaction().present,
      "invalid parameter regional send delivered an application callback");

  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteractionWithRegions(
            publisherHandles.interactionClass,
            validValues,
            rti::RegionHandleSet{rti::RegionHandle{}},
            tag);
      },
      L"InvalidRegion",
      "regional send with an invalid region");
  verifyFailureReport(
      2,
      L"InvalidRegion",
      "invalid region regional send");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      !receiver.recorder().interaction().present,
      "invalid region regional send delivered an application callback");

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
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioServiceReportAttributeUpdateFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Service-report attribute-update failure testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "service-report-attribute-failure-publisher");
  Session receiver(options, model, "service-report-attribute-failure-receiver");
  Session observer(options, model, "service-report-attribute-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-attribute-update-failure");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-attribute-failure-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-attribute-failure-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-attribute-failure-observer",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() && receiverClass.isValid() &&
          receiverAttribute.isValid(),
      "Service-report attribute-update failure lookup returned an invalid object or attribute handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Service-report attribute-update failure lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Service-report attribute-update failure parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep setup services out of the asserted reports. The two failed updates
  // below are the only service invocations whose reports are enabled.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Service-report attribute-update failure registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "service-report attribute-update failure object discovery");
  observer.recorder().clearInteraction();
  auto const reflectionsBeforeFailures = receiver.recorder().reflections().size();

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{0x72U, 0x65U, 0x67U};
  rti::VariableLengthData const value(valueBytes.data(), valueBytes.size());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::AttributeHandleValueMap validValues;
  validValues.emplace(publisherAttribute, value);
  rti::AttributeHandleValueMap invalidValues;
  invalidValues.emplace(rti::AttributeHandle{}, value);

  auto const verifyFailureReport = [&](std::int32_t expectedSerial,
                                       std::wstring const& objectValue,
                                       std::wstring const& mapValue,
                                       std::wstring const& expectedException,
                                       std::string const& description) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        description + " HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        description + " report returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        description + " report returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        description + " report returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          description + " report omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == L"UpdateAttributeValues",
        description + " report returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 2,
        description + " report returned the wrong object-management service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(!success.get(), description + " report incorrectly reported success");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == 4U,
        description + " report omitted standard supplied arguments");
    auto const verifyArgument = [&](std::size_t index,
                                    std::int32_t type,
                                    std::wstring const& name,
                                    std::wstring const& expectedValue) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() == type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() == name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedValue,
          description + " report returned the wrong supplied-argument shape");
    };
    verifyArgument(
        0U,
        37,
        L"Object instance designator",
        L"\"" + objectValue + L"\"");
    verifyArgument(
        1U,
        2,
        L"Constrained set of attribute designator and value pairs",
        mapValue);
    verifyArgument(2U, 60, L"User-supplied tag", L"\"cmVn\"");
    verifyArgument(3U, 34, L"Optional timestamp", L"null");

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
        description + " report returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    require(
        exception.get().find(expectedException) != std::wstring::npos,
        description + " report returned the wrong exception name");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == expectedSerial,
        description + " report returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const validMapValue = L"{\"" + publisherAttribute.toString() + L"\":\"AQI=\"}";
  auto const invalidMapValue = L"{\"" + rti::AttributeHandle{}.toString() + L"\":\"AQI=\"}";
  rti::ObjectInstanceHandle const invalidObject;
  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            invalidObject,
            validValues,
            tag);
      },
      L"ObjectInstanceNotKnown",
      "updating an unknown object with service reporting enabled");
  verifyFailureReport(
      0,
      invalidObject.toString(),
      validMapValue,
      L"ObjectInstanceNotKnown",
      "unknown-object attribute update");

  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            object,
            invalidValues,
            tag);
      },
      L"AttributeNotDefined",
      "updating an undefined attribute with service reporting enabled");
  verifyFailureReport(
      1,
      object.toString(),
      invalidMapValue,
      L"AttributeNotDefined",
      "undefined-attribute update");

  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().reflections().size() == reflectionsBeforeFailures,
      "failed attribute updates delivered an ordinary reflection callback");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  observer.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}




void scenarioServiceReportInteractionFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Service-report interaction failure testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "service-report-interaction-failure-publisher");
  Session receiver(options, model, "service-report-interaction-failure-receiver");
  Session observer(options, model, "service-report-interaction-failure-observer");
  auto const federation = federationName(options, "service-report-interaction-failure");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-interaction-failure-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-interaction-failure-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-interaction-failure-observer",
      options.federateType,
      federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          receiverInteraction.isValid() && receiverParameter.isValid(),
      "Service-report interaction failure source lookup returned an invalid handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Service-report interaction failure lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Service-report interaction failure parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep setup calls out of the asserted reports. The failed sends below are
  // the only service invocations whose reports are enabled.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  std::vector<std::uint8_t> const valueBytes{0x53U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x41U, 0x49U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::ParameterHandleValueMap validValues;
  validValues.emplace(
      publisherParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));

  auto const verifyFailureReport = [&](std::int32_t expectedSerial,
                                       std::wstring const& expectedException,
                                       std::string const& description) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        description + " HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        description + " report returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        description + " report returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        description + " report returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          description + " report omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == L"SendInteraction",
        description + " report returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 2,
        description + " report returned the wrong interaction service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(!success.get(), description + " report incorrectly reported success");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == 4U,
        description + " report omitted standard supplied arguments");
    auto const verifyArgument = [&](std::size_t index,
                                    std::int32_t type,
                                    std::wstring const& name) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() == type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() == name,
          description + " report returned the wrong supplied-argument shape");
    };
    verifyArgument(0U, 27, L"Interaction class designator");
    verifyArgument(
        1U,
        40,
        L"Constrained set of interaction parameter designator and value pairs");
    verifyArgument(2U, 60, L"User-supplied tag");
    verifyArgument(3U, 34, L"Optional timestamp");

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
        description + " report returned the wrong Null return value");

    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    require(
        exception.get().find(expectedException) != std::wstring::npos,
        description + " report returned the wrong exception name");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == expectedSerial,
        description + " report returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const pumpReceiverWithoutApplicationCallback = [&](std::string const& description) {
    receiver.recorder().clearInteraction();
    for (int pass = 0; pass != 8; ++pass) {
      receiver.pump();
    }
    require(
        !receiver.recorder().interaction().present,
        description + " delivered an application callback");
  };

  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteraction(
            rti::InteractionClassHandle{},
            validValues,
            tag);
      },
      L"InteractionClassNotDefined",
      "sending an interaction with an invalid interaction class while service reporting is enabled");
  verifyFailureReport(
      0,
      L"InteractionClassNotDefined",
      "invalid interaction-class send");
  pumpReceiverWithoutApplicationCallback("invalid interaction-class send");

  rti::ParameterHandleValueMap invalidParameterValues;
  invalidParameterValues.emplace(
      rti::ParameterHandle{},
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteraction(
            publisherInteraction,
            invalidParameterValues,
            tag);
      },
      L"InteractionParameterNotDefined",
      "sending an interaction with an invalid parameter while service reporting is enabled");
  verifyFailureReport(1, L"InteractionParameterNotDefined", "invalid-parameter send");
  pumpReceiverWithoutApplicationCallback("invalid-parameter send");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteraction(
            publisherInteraction,
            validValues,
            tag);
      },
      L"InteractionClassNotPublished",
      "sending an unpublished interaction while service reporting is enabled");
  verifyFailureReport(2, L"InteractionClassNotPublished", "unpublished-class send");
  pumpReceiverWithoutApplicationCallback("unpublished-class send");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  observer.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
