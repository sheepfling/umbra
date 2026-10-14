#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportRequestAttributeValueUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Request attribute-value service-report testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "service-report-request-owner");
  Session requester(options, model, "service-report-request-requester");
  Session observer(options, model, "service-report-request-observer");
  auto const federation = federationName(
      options,
      "service-report-request-attribute-value-update");
  owner.connect();
  requester.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-service-report-request-owner",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-service-report-request-requester",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-request-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Request attribute-value service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Request attribute-value service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
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
      "Request attribute-value service-report source lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};

  // Keep setup services out of the asserted report. The request below is the
  // only service invocation whose reporting switch is enabled.
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  requester.rtiAmbassador().setServiceReportingSwitch(false);
  requester.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  owner.rtiAmbassador().subscribeInteractionClass(reportClass);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Request attribute-value service-report registration returned an invalid object");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "request attribute-value service-report object discovery");
  auto const knownObject = requester.recorder().discovery().object;
  require(
      knownObject == object,
      "Request attribute-value service-report discovery returned the wrong object");
  owner.recorder().clearProvidedUpdates();
  observer.recorder().clearInteraction();

  requester.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const requestTagBytes{0x52U, 0x51U, 0x54U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdate(
      knownObject,
      requesterAttributes,
      requestTag);

  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the request attribute-value HLAreportServiceInvocation callback");
  waitFor(
      owner,
      [&] { return !owner.recorder().providedUpdates().empty(); },
      options,
      "the request attribute-value provider callback");

  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Request attribute-value service-report callback returned the wrong identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Request attribute-value service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Request attribute-value service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Request attribute-value service-report callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"RequestAttributeValueUpdate",
      "Request attribute-value service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Request attribute-value service-report callback returned the wrong service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Request attribute-value service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful request attribute-value service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() == 0,
      "First request attribute-value service report did not use serial number zero");

  auto const providedUpdates = owner.recorder().providedUpdates();
  require(
      providedUpdates.size() == 1U,
      "Request attribute-value service report produced an unexpected provider callback count");
  require(
      providedUpdates.front().object == object &&
          providedUpdates.front().attributes == ownerAttributes &&
          providedUpdates.front().tag == requestTagBytes,
      "Request attribute-value provider callback returned the wrong object, attributes, or tag");

  // The class overload is a distinct standard service route. Keep its report
  // and provider callback isolated so the serial progression and callback
  // arguments are checked independently from the known-object overload.
  owner.recorder().clearProvidedUpdates();
  observer.recorder().clearInteraction();
  std::vector<std::uint8_t> const classRequestTagBytes{0x43U, 0x4CU, 0x53U};
  rti::VariableLengthData const classRequestTag(
      classRequestTagBytes.data(),
      classRequestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdate(
      requesterClass,
      requesterAttributes,
      classRequestTag);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the class request attribute-value HLAreportServiceInvocation callback");
  waitFor(
      owner,
      [&] { return !owner.recorder().providedUpdates().empty(); },
      options,
      "the class request attribute-value provider callback");

  auto const classReport = observer.recorder().interaction();
  require(
      classReport.interaction == reportClass &&
          classReport.parameters.size() == reportParameters.size() &&
          !classReport.producer.isValid(),
      "Class request attribute-value service-report callback returned the wrong identity or producer metadata");
  require(
      classReport.tag.empty() && !classReport.regions.has_value(),
      "Class request attribute-value service-report callback returned user tag or region metadata");
  require(
      classReport.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Class request attribute-value service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        classReport.parameters.count(parameterHandle) == 1U,
        "Class request attribute-value service-report callback omitted a standard report parameter");
  }
  rti::HLAunicodeString classService;
  classService.decode(classReport.parameters.at(reportParameters[0]));
  require(
      classService.get() == L"RequestAttributeValueUpdate",
      "Class request attribute-value service-report callback returned the wrong service name");
  rti::HLAinteger16BE classServiceType;
  classServiceType.decode(classReport.parameters.at(reportParameters[1]));
  require(
      classServiceType.get() == 2,
      "Class request attribute-value service-report callback returned the wrong service type");
  rti::HLAboolean classSuccess;
  classSuccess.decode(classReport.parameters.at(reportParameters[2]));
  require(
      classSuccess.get(),
      "Class request attribute-value service-report callback did not report a successful invocation");
  rti::HLAunicodeString classException;
  classException.decode(classReport.parameters.at(reportParameters[5]));
  require(
      classException.get().empty(),
      "Successful class request attribute-value service report carried an exception");
  rti::HLAinteger32BE classSerial;
  classSerial.decode(classReport.parameters.at(reportParameters[6]));
  require(
      classSerial.get() == 1,
      "Second request attribute-value service report did not use serial number one");

  auto const classProvidedUpdates = owner.recorder().providedUpdates();
  require(
      classProvidedUpdates.size() == 1U,
      "Class request attribute-value service report produced an unexpected provider callback count");
  require(
      classProvidedUpdates.front().object == object &&
          classProvidedUpdates.front().attributes == ownerAttributes &&
          classProvidedUpdates.front().tag == classRequestTagBytes,
      "Class request attribute-value provider callback returned the wrong object, attributes, or tag");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  requester.rtiAmbassador().setServiceReportingSwitch(false);
  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  requester.disconnect();
  owner.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
