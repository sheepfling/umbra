#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportReserveObjectInstanceName(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Object-name reservation service-report testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "service-report-reserve-owner");
  Session observer(options, model, "service-report-reserve-observer");
  auto const federation = federationName(options, "service-report-reserve-object-instance-name");
  owner.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-service-report-reserve-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-reserve-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Object-name reservation service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Object-name reservation service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep setup services out of the asserted report. The reservation below is
  // the only service invocation whose report is enabled.
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  owner.recorder().clearReservations();
  observer.recorder().clearInteraction();

  auto const objectName = federation + L"-service-report-reserved-name";
  owner.rtiAmbassador().setServiceReportingSwitch(true);
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the object-name reservation HLAreportServiceInvocation callback");
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(objectName); },
      options,
      "the object-name reservation success callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Object-name reservation service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Object-name reservation service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Object-name reservation service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Object-name reservation service-report callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"ReserveObjectInstanceName",
      "Object-name reservation service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Object-name reservation service-report callback returned the wrong object-management service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Object-name reservation service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful object-name reservation service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() == 0,
      "First object-name reservation service report did not use serial number zero");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().releaseObjectInstanceName(objectName);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
