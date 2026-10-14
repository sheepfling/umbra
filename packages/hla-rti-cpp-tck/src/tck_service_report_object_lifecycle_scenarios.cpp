#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportRegisterObjectInstance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Register-object service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-register-owner");
  Session observer(options, model, "service-report-register-observer");
  auto const federation = federationName(options, "service-report-register-object-instance");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-register-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-register-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Register-object service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Register-object service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const subjectClass = subject.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subjectAttribute = subject.rtiAmbassador().getAttributeHandle(
      subjectClass,
      options.attributeName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      subjectClass.isValid() && subjectAttribute.isValid() && observerClass.isValid() &&
          observerAttribute.isValid(),
      "Register-object service-report source lookup returned an invalid handle");
  rti::AttributeHandleSet const subjectAttributes{subjectAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};

  // Keep setup services out of the asserted report. The registration below is
  // the only service invocation whose report is enabled.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().publishObjectClassAttributes(subjectClass, subjectAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");
  observer.recorder().clearInteraction();

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const object = subject.rtiAmbassador().registerObjectInstance(subjectClass);
  require(
      object.isValid(),
      "Register-object service-report registration returned an invalid object");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "register-object service-report object discovery");
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the register-object HLAreportServiceInvocation callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Register-object service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Register-object service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Register-object service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Register-object service-report callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"RegisterObjectInstance",
      "Register-object service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Register-object service-report callback returned the wrong object-management service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Register-object service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful register-object service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() == 0,
      "First register-object service report did not use serial number zero");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDeleteObjectInstance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Delete-object service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-delete-owner");
  Session observer(options, model, "service-report-delete-observer");
  auto const federation = federationName(options, "service-report-delete-object-instance");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-delete-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-delete-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Delete-object service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Delete-object service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subjectAttribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      objectClass.isValid() && subjectAttribute.isValid() && observerClass.isValid() &&
          observerAttribute.isValid(),
      "Delete-object service-report source lookup returned an invalid handle");
  rti::AttributeHandleSet const subjectAttributes{subjectAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};

  // Suppress setup reports. The ordinary delete below is the only asserted
  // service invocation in this scenario.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().publishObjectClassAttributes(objectClass, subjectAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");

  auto const object = subject.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Delete-object service-report registration returned an invalid object");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "delete-object service-report object discovery");
  observer.recorder().clearInteraction();
  observer.recorder().clearRemovals();

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const deletionTagBytes{0x44U, 0x45U, 0x4CU};
  rti::VariableLengthData const deletionTag(
      deletionTagBytes.data(),
      deletionTagBytes.size());
  subject.rtiAmbassador().deleteObjectInstance(object, deletionTag);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the delete-object HLAreportServiceInvocation callback");
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasRemoval(
            object,
            deletionTagBytes,
            subject.federateHandle());
      },
      options,
      "the ordinary delete-object callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Delete-object service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Delete-object service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Delete-object service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Delete-object service-report callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"DeleteObjectInstance",
      "Delete-object service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Delete-object service-report callback returned the wrong object-management service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Delete-object service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful delete-object service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() == 0,
      "First delete-object service report did not use serial number zero");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDeleteObjectInstanceFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Delete-object failure service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-delete-failure-owner");
  Session observer(options, model, "service-report-delete-failure-observer");
  auto const federation = federationName(options, "service-report-delete-object-instance-failure");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-delete-failure-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-delete-failure-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Delete-object failure service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Delete-object failure service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const subjectClass = subject.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subjectAttribute = subject.rtiAmbassador().getAttributeHandle(
      subjectClass,
      options.attributeName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      subjectClass.isValid() && subjectAttribute.isValid() && observerClass.isValid() &&
          observerAttribute.isValid(),
      "Delete-object failure service-report source lookup returned an invalid handle");
  rti::AttributeHandleSet const subjectAttributes{subjectAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};

  // Keep setup services out of the asserted reports. The invalid and stale
  // deletion calls below are the only failed service invocations asserted.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().publishObjectClassAttributes(subjectClass, subjectAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");

  auto const object = subject.rtiAmbassador().registerObjectInstance(subjectClass);
  require(object.isValid(), "Delete-object failure service-report registration returned an invalid object");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "delete-object failure service-report object discovery");
  observer.recorder().clearInteraction();
  observer.recorder().clearRemovals();

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const deletionTagBytes{0x72U, 0x65U, 0x67U};
  rti::VariableLengthData const deletionTag(
      deletionTagBytes.data(),
      deletionTagBytes.size());

  auto const verifyFailureReport = [&](std::wstring const& expectedObjectValue,
                                       std::int32_t expectedSerial,
                                       std::string const& phase) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        "the " + phase + " failed-delete HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Failed delete-object service-report callback returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Failed delete-object service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Failed delete-object service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Failed delete-object service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == L"DeleteObjectInstance",
        "Failed delete-object service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 2,
        "Failed delete-object service-report callback returned the wrong object-management service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        !success.get(),
        "Failed delete-object service-report callback incorrectly reported success");
    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == 3U,
        "Failed delete-object service-report callback omitted supplied arguments");
    auto const& objectArgument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(0U));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(objectArgument.get(0U)).get() == 37 &&
            dynamic_cast<rti::HLAunicodeString const&>(objectArgument.get(1U)).get() ==
                L"Object instance designator" &&
            dynamic_cast<rti::HLAunicodeString const&>(objectArgument.get(2U)).get() ==
                L"\"" + expectedObjectValue + L"\"",
        "Failed delete-object service-report callback returned the wrong object argument");
    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() == L"null",
        "Failed delete-object service-report callback returned the wrong returned-argument value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    require(
        exception.get().find(L"ObjectInstanceNotKnown") != std::wstring::npos,
        "Failed delete-object service-report callback returned the wrong exception");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == expectedSerial,
        "Failed delete-object service-report callback returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  rti::ObjectInstanceHandle const invalidObject;
  requireException(
      [&] { subject.rtiAmbassador().deleteObjectInstance(invalidObject, deletionTag); },
      L"ObjectInstanceNotKnown",
      "deleting an unknown object with service reporting enabled");
  verifyFailureReport(invalidObject.toString(), 0, "unknown-object");

  subject.rtiAmbassador().deleteObjectInstance(object, deletionTag);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the successful delete-object service-report callback before stale failure");
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasRemoval(
            object,
            deletionTagBytes,
            subject.federateHandle());
      },
      options,
      "the successful delete-object callback before stale failure");
  observer.recorder().clearInteraction();
  observer.recorder().clearRemovals();

  requireException(
      [&] { subject.rtiAmbassador().deleteObjectInstance(object, deletionTag); },
      L"ObjectInstanceNotKnown",
      "deleting a stale object with service reporting enabled");
  verifyFailureReport(object.toString(), 2, "stale-object");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
