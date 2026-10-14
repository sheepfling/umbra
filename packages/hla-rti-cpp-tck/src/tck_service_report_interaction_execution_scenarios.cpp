#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportInteraction(Options const& options, rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Service-report interaction testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-subject");
  Session observer(options, model, "service-report-observer");
  auto const federation = federationName(options, "service-report-interaction");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Service-report interaction lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Service-report interaction parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = subject.rtiAmbassador().getParameterHandle(
      interactionClass,
      options.parameterName);
  require(
      interactionClass.isValid() && parameter.isValid(),
      "Service-report source interaction lookup returned an invalid handle");

  // Suppress reports for setup calls. The source service invocation below is
  // the only operation whose report is asserted by this scenario.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().publishInteractionClass(interactionClass);
  observer.recorder().clearInteraction();

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const valueBytes{0x53U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x4DU, 0x4FU, 0x4DU};
  rti::ParameterHandleValueMap values;
  values.emplace(
      parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  subject.rtiAmbassador().sendInteraction(interactionClass, values, tag);

  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the HLAreportServiceInvocation callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Service-report callback omitted a standard report parameter");
  }
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "Service-report callback identified the wrong joined federate");

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"SendInteraction",
      "Service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Service-report callback returned the wrong interaction service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(success.get(), "Service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(exception.get().empty(), "Successful service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(serial.get() == 0, "First service report did not use serial number zero");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportReceiveOrderInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order interaction service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Receive-order interaction service-report testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "service-report-receive-order-publisher");
  Session receiver(options, model, "service-report-receive-order-receiver");
  Session observer(options, model, "service-report-receive-order-observer");
  auto const federation =
      federationName(options, "service-report-receive-order-interaction");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-receive-order-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-receive-order-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-receive-order-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Receive-order interaction service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Receive-order interaction service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const interactionClass = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interactionClass,
      options.parameterName);
  require(
      interactionClass.isValid() && parameter.isValid(),
      "Receive-order interaction source lookup returned an invalid handle");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      reliable.isValid(),
      "Receive-order interaction service-report lookup returned an invalid reliable transportation handle");

  // Keep setup calls out of the one accepted SendInteraction report. The
  // publisher reports to the public MOM subscriber; the receiver only gets
  // the ordinary application interaction.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  publisher.rtiAmbassador().publishInteractionClass(interactionClass);
  receiver.rtiAmbassador().subscribeInteractionClass(interactionClass);
  observer.recorder().clearInteraction();
  receiver.recorder().clearInteraction();

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{'r', 'o', 'i'};
  rti::ParameterHandleValueMap values;
  values.emplace(
      parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendInteraction(interactionClass, values, tag);

  if (model == rti::HLA_IMMEDIATE) {
    require(
        observer.recorder().interactions().size() == 1U &&
            receiver.recorder().interactions().size() == 1U,
        "Immediate receive-order interaction did not deliver both callbacks synchronously");
  } else {
    require(
        observer.recorder().interactions().empty() &&
            receiver.recorder().interactions().empty(),
        "Evoked receive-order interaction delivered a callback before callback servicing");
    waitFor(
        observer,
        receiver,
        [&] {
          return observer.recorder().interactions().size() == 1U &&
              receiver.recorder().interactions().size() == 1U;
        },
        options,
        "receive-order interaction and service-report callbacks");
  }

  auto const report = observer.recorder().interactions().at(0U);
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"SendInteraction",
      {{27, L"Interaction class designator", quoted(interactionClass.toString())},
       {40,
        L"Constrained set of interaction parameter designator and value pairs",
        L"{\"" + parameter.toString() + L"\":\"AQI=\"}"},
       {60, L"User-supplied tag", L"\"cm9p\""},
       {34, L"Optional timestamp", L"null"}},
      {34, L"", L"null"},
      0,
      "Receive-order SendInteraction service report",
      2);

  auto const interaction = receiver.recorder().interactions().at(0U);
  require(
      interaction.interaction == interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(parameter) == 1U &&
          copyBytes(interaction.parameters.at(parameter)) == valueBytes &&
          interaction.tag == tagBytes && interaction.transportation == reliable &&
          interaction.producer == publisher.federateHandle() &&
          !interaction.regions.has_value(),
      "Receive-order application interaction returned non-standard callback data");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeInteractionClass(interactionClass);
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
