#include "tck_main_scenarios.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportOmitsFomAddedParameter(
    Options const& options,
    rti::CallbackModel model) {
  auto const extensionFom = options.scenarioFoms.find(
      "mom-report-service-invocation-extension-parameter");
  require(
      extensionFom != options.scenarioFoms.end() &&
          !extensionFom->second.empty() && !options.fom.empty() &&
          !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Service-report extension-parameter TCK requires its FOM, application FOM, standard MIM, and logical-time configuration");

  Session publisher(options, model, "service-report-extension-publisher");
  Session observer(options, model, "service-report-extension-observer");
  auto const federation = federationName(
      options, "service-report-invocation-extension-parameter-not-sent");
  auto const publisherName =
      options.ownerFederateName + L"-service-report-extension-publisher";
  publisher.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{
          options.fom.wstring(), extensionFom->second.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(publisherName, options.federateType, federation);
  observer.join(
      options.memberFederateName + L"-service-report-extension-observer",
      options.federateType,
      federation);

  constexpr auto reportName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
  auto const reportClass =
      observer.rtiAmbassador().getInteractionClassHandle(reportName);
  require(reportClass.isValid(), "The standard HLAreportServiceInvocation class was unavailable");
  constexpr std::array<std::wstring_view, 8U> standardParameterNames{
      L"HLAservice",
      L"HLAserviceType",
      L"HLAsuccessIndicator",
      L"HLAsuppliedArguments",
      L"HLAreturnedArgument",
      L"HLAexception",
      L"HLAserialNumber",
      L"HLAfederate",
  };
  std::array<rti::ParameterHandle, standardParameterNames.size()> standardParameters;
  for (std::size_t index = 0U; index < standardParameterNames.size(); ++index) {
    standardParameters[index] = observer.rtiAmbassador().getParameterHandle(
        reportClass, std::wstring{standardParameterNames[index]});
    require(
        standardParameters[index].isValid(),
        "A predefined HLAreportServiceInvocation parameter was unavailable");
  }
  auto const extensionParameter = observer.rtiAmbassador().getParameterHandle(
      reportClass, L"TckServiceInvocationExtensionPayload");
  require(
      extensionParameter.isValid(),
      "The FOM-added service-invocation report parameter was unavailable");
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const applicationInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const applicationParameter = publisher.rtiAmbassador().getParameterHandle(
      applicationInteraction, options.parameterName);
  require(
      applicationInteraction.isValid() && applicationParameter.isValid(),
      "The adapter application interaction or parameter was unavailable");
  publisher.rtiAmbassador().publishInteractionClass(applicationInteraction);
  publisher.recorder().clearInteraction();
  observer.recorder().clearInteraction();
  publisher.rtiAmbassador().setServiceReportingSwitch(true);

  std::array<unsigned char, 4U> const payloadBytes{0x52U, 0x45U, 0x50U, 0x54U};
  rti::ParameterHandleValueMap const payload{{
      applicationParameter,
      rti::VariableLengthData{payloadBytes.data(), payloadBytes.size()},
  }};
  publisher.rtiAmbassador().sendInteraction(
      applicationInteraction, payload, rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return !observer.recorder().interactions().empty(); },
      options,
      "the RTI-originated HLAreportServiceInvocation callback");

  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U && reports.front().interaction == reportClass,
      "The application send did not produce exactly one standard service-invocation report");
  auto const& report = reports.front();
  auto const publisherHandle =
      publisher.rtiAmbassador().getFederateHandle(publisherName);
  require(
      publisherHandle.isValid() &&
          report.transportation ==
              observer.rtiAmbassador().getTransportationTypeHandle(
                  L"HLAreliable") &&
          report.parameters.size() == standardParameters.size() &&
          report.parameters.find(extensionParameter) == report.parameters.end(),
      "The RTI-originated service report used the wrong producer/transport, added a FOM-only parameter, or omitted standard parameter cardinality");
  for (auto const parameter : standardParameters) {
    require(
        report.parameters.find(parameter) != report.parameters.end(),
        "The RTI-originated service report omitted a predefined standard parameter");
  }
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          report.parameters.at(standardParameters[7])) == publisherHandle,
      "The service report identified the wrong joined federate");
  rti::HLAunicodeString service;
  service.decode(report.parameters.at(standardParameters[0]));
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(standardParameters[1]));
  rti::HLAboolean success;
  success.decode(report.parameters.at(standardParameters[2]));
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(standardParameters[5]));
  rti::HLAinteger32BE serialNumber;
  serialNumber.decode(report.parameters.at(standardParameters[6]));
  require(
      service.get() == L"SendInteraction" && serviceType.get() == 2 &&
          success.get() && exception.get().empty() && serialNumber.get() == 0,
      "The RTI-originated standard service report did not preserve its required base fields");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
