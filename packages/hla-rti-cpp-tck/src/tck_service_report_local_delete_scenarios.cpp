#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportLocalDeleteObjectInstance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Local-delete service-report testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "service-report-local-delete-owner");
  Session requester(options, model, "service-report-local-delete-requester");
  auto const federation = federationName(options, "service-report-local-delete-object-instance");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-service-report-local-delete-owner",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-service-report-local-delete-requester",
      options.federateType,
      federation);

  auto const reportClass = owner.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Local-delete service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
    auto const parameter = owner.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Local-delete service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && requesterClass.isValid() &&
          requesterAttribute.isValid(),
      "Local-delete service-report source lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};

  // Keep setup services out of the asserted report. The local deletion below
  // is the only service invocation whose report is enabled.
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  requester.rtiAmbassador().setServiceReportingSwitch(false);
  requester.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  owner.rtiAmbassador().subscribeInteractionClass(reportClass);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Local-delete service-report registration returned an invalid object");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "local-delete service-report object discovery");
  owner.recorder().clearInteraction();

  requester.rtiAmbassador().setServiceReportingSwitch(true);
  requester.rtiAmbassador().localDeleteObjectInstance(object);
  waitFor(
      owner,
      [&] { return owner.recorder().interaction().present; },
      options,
      "the local-delete HLAreportServiceInvocation callback");
  auto const report = owner.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Local-delete service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Local-delete service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Local-delete service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Local-delete service-report callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"LocalDeleteObjectInstance",
      "Local-delete service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Local-delete service-report callback returned the wrong object-management service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Local-delete service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful local-delete service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() == 0,
      "First local-delete service report did not use serial number zero");

  owner.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  requester.rtiAmbassador().setServiceReportingSwitch(false);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioServiceReportLocalDeleteObjectInstanceFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Local-delete failure service-report testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "service-report-local-delete-failure-owner");
  Session requester(options, model, "service-report-local-delete-failure-requester");
  auto const federation =
      federationName(options, "service-report-local-delete-object-instance-failure");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-service-report-local-delete-failure-owner",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-service-report-local-delete-failure-requester",
      options.federateType,
      federation);

  auto const reportClass = owner.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Local-delete failure service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
    auto const parameter = owner.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Local-delete failure service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && requesterClass.isValid() &&
          requesterAttribute.isValid(),
      "Local-delete failure service-report source lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};

  // Keep setup services out of the asserted reports. The invalid, successful,
  // and stale local-delete calls below are the only service invocations whose
  // reports are enabled.
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  requester.rtiAmbassador().setServiceReportingSwitch(false);
  requester.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  owner.rtiAmbassador().subscribeInteractionClass(reportClass);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Local-delete failure service-report registration returned an invalid object");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "local-delete failure service-report object discovery");
  owner.recorder().clearInteraction();

  requester.rtiAmbassador().setServiceReportingSwitch(true);
  auto const verifyReport = [&](bool expectedSuccess,
                                std::wstring const& expectedObjectValue,
                                std::int32_t expectedSerial,
                                std::string const& phase) {
    waitFor(
        owner,
        [&] { return owner.recorder().interaction().present; },
        options,
        "the " + phase + " local-delete HLAreportServiceInvocation callback");
    auto const report = owner.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Local-delete failure service-report callback returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Local-delete failure service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Local-delete failure service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Local-delete failure service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == L"LocalDeleteObjectInstance",
        "Local-delete failure service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 2,
        "Local-delete failure service-report callback returned the wrong object-management service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        success.get() == expectedSuccess,
        "Local-delete failure service-report callback returned the wrong success indicator");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == 1U,
        "Local-delete failure service-report callback returned the wrong supplied-argument count");
    auto const& objectArgument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(0U));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(objectArgument.get(0U)).get() == 37 &&
            dynamic_cast<rti::HLAunicodeString const&>(objectArgument.get(1U)).get() ==
                L"Object instance designator" &&
            dynamic_cast<rti::HLAunicodeString const&>(objectArgument.get(2U)).get() ==
                std::wstring(L"\"") + expectedObjectValue + L"\"",
        "Local-delete failure service-report callback returned the wrong object argument");

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
        "Local-delete failure service-report callback returned the wrong returned-argument value");

    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    if (expectedSuccess) {
      require(
          exception.get().empty(),
          "Successful local-delete service report carried an exception");
    } else {
      require(
          exception.get().find(L"ObjectInstanceNotKnown") != std::wstring::npos,
          "Failed local-delete service report carried the wrong exception");
    }
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == expectedSerial,
        "Local-delete failure service-report callback returned the wrong serial number");
    owner.recorder().clearInteraction();
  };

  rti::ObjectInstanceHandle const invalidObject;
  requireException(
      [&] { requester.rtiAmbassador().localDeleteObjectInstance(invalidObject); },
      L"ObjectInstanceNotKnown",
      "locally deleting an unknown object with service reporting enabled");
  verifyReport(false, invalidObject.toString(), 0, "unknown-object");

  requester.rtiAmbassador().localDeleteObjectInstance(object);
  verifyReport(true, object.toString(), 1, "successful-object");

  requireException(
      [&] { requester.rtiAmbassador().localDeleteObjectInstance(object); },
      L"ObjectInstanceNotKnown",
      "locally deleting a stale object with service reporting enabled");
  verifyReport(false, object.toString(), 2, "stale-object");

  requester.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
