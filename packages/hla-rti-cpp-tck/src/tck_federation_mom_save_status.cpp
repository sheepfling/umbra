#include "tck_federation_mom_save_status.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioFederationMomSaveStatus(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "federation-mom-save-status-owner");
  Session peer(options, model, "federation-mom-save-status-peer");
  auto const federation = federationName(options, "federation-mom-save-status");
  require(
      !options.modelFom.empty() && !options.mimFom.empty(),
      "Federation MOM save-status reporting requires the adapter model FOM and standard MIM");
  owner.connect();
  peer.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-federation-mom-save-status-owner",
      options.federateType,
      federation);
  peer.join(
      options.memberFederateName + L"-federation-mom-save-status-peer",
      options.federateType,
      federation);

  auto const reportClass = peer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Federation MOM save-status reporting returned an invalid report class");
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
    auto const parameter = peer.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Federation MOM save-status reporting returned an invalid parameter handle");
    reportParameters.push_back(parameter);
  }
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  peer.rtiAmbassador().setServiceReportingSwitch(false);
  peer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  peer.rtiAmbassador().subscribeInteractionClass(reportClass);
  owner.rtiAmbassador().setServiceReportingSwitch(true);

  auto const ownerHandle = owner.federateHandle();
  auto const peerHandle = peer.federateHandle();
  auto const statusBefore = owner.federationSaveStatusResponses().size();
  owner.rtiAmbassador().queryFederationSaveStatus();
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federationSaveStatusResponses().size() > statusBefore &&
            peer.recorder().interactions().size() >= 2U;
      },
      options,
      "federation MOM save-status callback and service report");

  auto const statuses = owner.federationSaveStatusResponses().back();
  require(
      statuses.size() == 2U,
      "federation MOM save-status response omitted a joined federate");
  auto requireStatus = [&](rti::FederateHandle const& handle,
                           rti::SaveStatus expected,
                           std::string const& description) {
    auto const found = std::find_if(
        statuses.begin(),
        statuses.end(),
        [&](auto const& entry) { return entry.first == handle; });
    require(found != statuses.end(), description + " omitted a federate");
    require(found->second == expected, description + " returned the wrong status");
  };
  requireStatus(
      ownerHandle,
      rti::NO_SAVE_IN_PROGRESS,
      "federation MOM save-status owner entry");
  requireStatus(
      peerHandle,
      rti::NO_SAVE_IN_PROGRESS,
      "federation MOM save-status peer entry");

  std::wstring expectedStatusPairs = L"[";
  for (std::size_t index = 0; index < statuses.size(); ++index) {
    if (index != 0U) expectedStatusPairs.push_back(L',');
    auto const& entry = statuses[index];
    expectedStatusPairs += L"{\"handle\":\"" + entry.first.toString() +
        L"\",\"status\":\"NO_SAVE_IN_PROGRESS\"}";
  }
  expectedStatusPairs.push_back(L']');
  auto const reports = peer.recorder().interactions();
  require(
      reports.size() == 2U,
      "Federation MOM save-status report count was unstable");
  auto const statusReport = std::find_if(
      reports.begin(),
      reports.end(),
      [&](InteractionRecord const& report) {
        rti::HLAunicodeString decodedService;
        decodedService.decode(report.parameters.at(reportParameters.at(0U)));
        return decodedService.get() == L"FederationSaveStatusResponse";
      });
  require(
      statusReport != reports.end(),
      "Federation MOM save-status response report was not observed");
  requirePortableServiceReport(
      peer,
      *statusReport,
      reportClass,
      reportParameters,
      L"FederationSaveStatusResponse",
      {{17,
        L"List of joined federates and save status for each",
        expectedStatusPairs}},
      {34, L"", L"null"},
      1,
      "Federation save-status Table 5 argument encoding",
      0);

  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
