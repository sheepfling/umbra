#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportAttributeUpdate(Options const& options, rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Service-report attribute-update testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-attribute-subject");
  Session observer(options, model, "service-report-attribute-observer");
  auto const federation = federationName(options, "service-report-attribute-update");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-attribute-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-attribute-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Service-report attribute-update lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Service-report attribute-update parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Service-report attribute-update source lookup returned an invalid handle");

  // Keep setup operations out of the asserted report. The update below is the
  // only service invocation whose report is accepted by this scenario.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  rti::AttributeHandleSet attributes{attribute};
  subject.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  auto const object = subject.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Service-report attribute-update registration returned an invalid object");
  observer.recorder().clearInteraction();

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{0x03U, 0x04U, 0x05U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  subject.rtiAmbassador().updateAttributeValues(object, values, tag);

  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the HLAreportServiceInvocation attribute-update callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Service-report attribute-update callback returned the wrong identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Service-report attribute-update callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Service-report attribute-update callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Service-report attribute-update callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"UpdateAttributeValues",
      "Service-report attribute-update callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Service-report attribute-update callback returned the wrong service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Service-report attribute-update callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful service-report attribute-update carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() == 0,
      "First service-report attribute-update did not use serial number zero");

  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{argumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
  require(
      suppliedArguments.size() == 4U,
      "Service-report attribute-update returned an unexpected supplied-argument count");
  auto const& attributeValueArgument = dynamic_cast<rti::HLAfixedRecord const&>(
      suppliedArguments.get(1U));
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(attributeValueArgument.get(0U)).get() == 2,
      "Service-report attribute-update returned the wrong AttributeHandleValueMap type");
  require(
      dynamic_cast<rti::HLAunicodeString const&>(attributeValueArgument.get(1U)).get() ==
          L"Constrained set of attribute designator and value pairs",
      "Service-report attribute-update returned the wrong AttributeHandleValueMap name");
  std::wstring const expectedAttributeValueMap =
      std::wstring{L"{\""} + attribute.toString() + L"\":\"AQI=\"}";
  require(
      dynamic_cast<rti::HLAunicodeString const&>(attributeValueArgument.get(2U)).get() ==
          expectedAttributeValueMap,
      "Service-report attribute-update did not encode the binary attribute value as Base64");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
