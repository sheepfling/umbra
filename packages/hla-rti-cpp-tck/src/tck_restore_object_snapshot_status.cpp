#include "tck_restore_object_snapshot_status.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

std::wstring additionalSettingsWithServiceReportDirectory(
    Options const& options) {
  std::wstring const settings = toWide(options.additionalSettings);
  constexpr std::wstring_view directorySetting = L"serviceReportDirectory=";
  std::wstring result;
  auto append = [&](std::wstring_view token) {
    if (token.empty()) return;
    if (!result.empty()) result.push_back(L';');
    result.append(token);
  };

  std::size_t begin = 0U;
  while (begin <= settings.size()) {
    auto const end = settings.find(L';', begin);
    auto const token = std::wstring_view(settings).substr(
        begin,
        end == std::wstring::npos ? std::wstring::npos : end - begin);
    if (!token.starts_with(directorySetting)) append(token);
    if (end == std::wstring::npos) break;
    begin = end + 1U;
  }

  append(directorySetting);
  result.append(options.serviceReportDirectory.wstring());
  return result;
}

void connectOwnerWithServiceReportDirectory(
    Session& owner,
    Options const& options) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  auto const configurationName = options.ownerConfigurationName.empty()
      ? options.configurationName
      : options.ownerConfigurationName;
  if (!configurationName.empty()) {
    configuration.withConfigurationName(toWide(configurationName));
  }
  if (!options.rtiAddress.empty()) {
    configuration.withRtiAddress(toWide(options.rtiAddress));
  }
  configuration.withAdditionalSettings(
      additionalSettingsWithServiceReportDirectory(options));
  static_cast<void>(owner.connectWithConfiguration(configuration));
}

std::optional<std::filesystem::path> advertisedServiceReportFile(
    Recorder const& recorder,
    rti::AttributeHandle const& federateNameAttribute,
    rti::AttributeHandle const& reportFileAttribute,
    std::wstring const& federateName) {
  for (auto const& reflection : recorder.reflections()) {
    auto const name = reflection.values.find(federateNameAttribute);
    auto const reportFile = reflection.values.find(reportFileAttribute);
    if (name == reflection.values.end() ||
        reportFile == reflection.values.end()) {
      continue;
    }
    rti::HLAunicodeString decodedName;
    decodedName.decode(name->second);
    if (decodedName.get() != federateName) continue;
    rti::HLAunicodeString decodedReportFile;
    decodedReportFile.decode(reportFile->second);
    require(
        !decodedReportFile.get().empty(),
        "Owner Federate MOM advertised an empty HLAreportServiceFile value");
    return std::filesystem::path(decodedReportFile.get());
  }
  return std::nullopt;
}

std::string readServiceReportFile(std::filesystem::path const& file) {
  std::ifstream input(file, std::ios::binary);
  require(static_cast<bool>(input),
          "Unable to read the file advertised by HLAreportServiceFile");
  return std::string(
      std::istreambuf_iterator<char>{input},
      std::istreambuf_iterator<char>{});
}

std::size_t occurrenceCount(std::string_view value, std::string_view marker) {
  std::size_t count = 0U;
  for (auto position = value.find(marker); position != std::string_view::npos;
       position = value.find(marker, position + marker.size())) {
    ++count;
  }
  return count;
}

void requireStatusFor(rti::FederateRestoreStatusVector const& statuses,
                      rti::FederateHandle const& pre,
                      rti::FederateHandle const& post,
                      rti::RestoreStatus expected) {
  std::size_t matches = 0U;
  for (auto const& descriptor : statuses) {
    if (descriptor.preRestoreHandle != pre) continue;
    ++matches;
    require(descriptor.postRestoreHandle == post && descriptor.status == expected,
            "Restore status descriptor had the wrong post handle or state");
  }
  require(matches == 1U, "Restore status omitted or duplicated a joined federate");
}

void runRestoreObjectSnapshotStatus(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_IMMEDIATE || model == rti::HLA_EVOKED,
          "Object snapshot restore requires a supported callback model");
  require(!options.fom.empty() && !options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Object snapshot restore requires adapter FOM, standard MIM, and time implementation");
  require(!options.serviceReportDirectory.empty(),
          "Object snapshot restore status file testing requires a service-report directory");

  Session owner(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "restore-object-snapshot-status");
  std::wstring const saveLabel = L"tck-object-baseline";
  connectOwnerWithServiceReportDirectory(owner, options);
  peer.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType + L"-owner", federation);
  peer.join(options.memberFederateName, options.federateType + L"-peer", federation);

  auto const applicationInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const applicationParameter = owner.rtiAmbassador().getParameterHandle(
      applicationInteraction, options.parameterName);
  require(applicationInteraction.isValid() && applicationParameter.isValid(),
          "Adapter FOM lacks the application interaction needed for restore gating");
  owner.rtiAmbassador().publishInteractionClass(applicationInteraction);
  peer.rtiAmbassador().subscribeInteractionClass(applicationInteraction);
  std::vector<std::uint8_t> const restoreGuardPayload{0x52U};
  rti::VariableLengthData const restoreGuardValue(
      restoreGuardPayload.data(), restoreGuardPayload.size());
  rti::ParameterHandleValueMap const restoreGuardParameters{
      {applicationParameter, restoreGuardValue}};
  rti::VariableLengthData const restoreGuardTag;

  auto const federateMomClass = peer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = peer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = peer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  peer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");

  auto const reportClass = peer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(reportClass.isValid(),
          "Restore-status reporting returned an invalid service-report class");
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
    require(parameter.isValid(),
            "Restore-status reporting returned an invalid service-report parameter");
    reportParameters.push_back(parameter);
  }
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  peer.rtiAmbassador().setServiceReportingSwitch(false);
  peer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  peer.rtiAmbassador().subscribeInteractionClass(reportClass);

  std::optional<std::filesystem::path> ownerReportFile;
  waitFor(
      owner,
      peer,
      [&] {
        ownerReportFile = advertisedServiceReportFile(
            peer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            options.ownerFederateName);
        return ownerReportFile.has_value();
      },
      options,
      "owner HLAreportServiceFile MOM reflection");
  require(std::filesystem::is_regular_file(*ownerReportFile),
          "Owner HLAreportServiceFile did not name an existing report file");
  require(!readServiceReportFile(*ownerReportFile).empty(),
          "Owner HLAreportServiceFile did not contain its initial record");

  auto const ownerHandle = owner.federateHandle();
  auto const peerHandle = peer.federateHandle();
  rti::FederateHandle const noPostRestoreHandle;
  auto const preRestoreStatusBefore =
      owner.federationRestoreStatusResponses().size();
  owner.rtiAmbassador().queryFederationRestoreStatus();
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federationRestoreStatusResponses().size() ==
            preRestoreStatusBefore + 1U;
      },
      options,
      "pre-restore federation status response");
  auto const preRestoreStatus =
      owner.federationRestoreStatusResponses().back();
  require(preRestoreStatus.size() == 2U,
          "Pre-restore status omitted a joined member");
  requireStatusFor(preRestoreStatus, ownerHandle, noPostRestoreHandle,
                   rti::NO_RESTORE_IN_PROGRESS);
  requireStatusFor(preRestoreStatus, peerHandle, noPostRestoreHandle,
                   rti::NO_RESTORE_IN_PROGRESS);
  require(owner.federateRestoreInitiations().empty() &&
              peer.federateRestoreInitiations().empty() &&
              owner.federationRestoreBegunCount() == 0U &&
              peer.federationRestoreBegunCount() == 0U,
          "Pre-restore status was queried after restore initiation began");

  // Capture the enabled switch in the save image so it remains enabled after
  // restore; Set Service Reporting Switch is unavailable during restoration.
  owner.rtiAmbassador().setServiceReportingSwitch(true);
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
            peer.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
      },
      options,
      "object baseline save initiation callbacks");
  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federationSavedCount() == 1U && peer.federationSavedCount() == 1U;
      },
      options,
      "object baseline save completion callbacks");
  require(owner.federationNotSavedReasons().empty() &&
              peer.federationNotSavedReasons().empty(),
          "Object baseline did not complete successfully");

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter FOM lacks object class or attribute for restore snapshot");
  rti::AttributeHandleSet const attributes{attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  peer.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes);
  auto const mutated = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(mutated.isValid(), "Post-save object registration returned an invalid handle");
  waitFor(
      owner,
      peer,
      [&] { return peer.recorder().hasDiscovery(mutated); },
      options,
      "post-save object discovery callback");

  auto hasRestoreRequestConfirmationReport = [&] {
    auto const reports = peer.recorder().interactions();
    return std::any_of(
        reports.begin(),
        reports.end(),
        [&](InteractionRecord const& report) {
          if (report.interaction != reportClass) return false;
          rti::HLAunicodeString decodedService;
          decodedService.decode(report.parameters.at(reportParameters.at(0U)));
          return decodedService.get() == L"ConfirmFederationRestorationRequest";
        });
  };
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federationRestoreRequestsSucceeded() ==
                std::vector<std::wstring>{saveLabel} &&
            peer.federationRestoreRequestsSucceeded().empty() &&
            owner.federationRestoreRequestsFailed().empty() &&
            peer.federationRestoreRequestsFailed().empty() &&
            owner.federationRestoreBegunCount() == 1U &&
            peer.federationRestoreBegunCount() == 1U &&
            owner.federateRestoreInitiations().size() == 1U &&
            peer.federateRestoreInitiations().size() == 1U &&
            hasRestoreRequestConfirmationReport();
      },
      options,
      "single restore control callbacks at both members");
  auto const ownerRequestSuccesses = owner.federationRestoreRequestsSucceeded();
  auto const peerRequestSuccesses = peer.federationRestoreRequestsSucceeded();
  auto const ownerRequestFailures = owner.federationRestoreRequestsFailed();
  auto const peerRequestFailures = peer.federationRestoreRequestsFailed();
  require(
      ownerRequestSuccesses == std::vector<std::wstring>{saveLabel} &&
          peerRequestSuccesses.empty() && ownerRequestFailures.empty() &&
          peerRequestFailures.empty(),
      "The single restore requester did not receive exactly one positive confirmation");
  auto const restoreRequestReports = peer.recorder().interactions();
  auto const restoreRequestConfirmation = std::find_if(
      restoreRequestReports.begin(),
      restoreRequestReports.end(),
      [&](InteractionRecord const& report) {
        if (report.interaction != reportClass) return false;
        rti::HLAunicodeString decodedService;
        decodedService.decode(report.parameters.at(reportParameters.at(0U)));
        return decodedService.get() == L"ConfirmFederationRestorationRequest";
      });
  require(restoreRequestConfirmation != restoreRequestReports.end(),
          "Valid Request Federation Restore omitted its confirmation service report");
  auto const confirmationFederate = peer.rtiAmbassador().decodeFederateHandle(
      restoreRequestConfirmation->parameters.at(reportParameters.at(7U)));
  require(confirmationFederate == ownerHandle,
          "Restore confirmation service report identified the wrong federate");
  auto const confirmationReportIndex = static_cast<std::size_t>(
      std::distance(restoreRequestReports.begin(), restoreRequestConfirmation));
  rti::Integer32 expectedConfirmationSerial = 0;
  for (std::size_t index = 0U; index < confirmationReportIndex; ++index) {
    if (peer.rtiAmbassador().decodeFederateHandle(
            restoreRequestReports.at(index).parameters.at(reportParameters.at(7U))) ==
        confirmationFederate) {
      ++expectedConfirmationSerial;
    }
  }
  rti::HLAinteger32BE decodedConfirmationSerial;
  decodedConfirmationSerial.decode(
      restoreRequestConfirmation->parameters.at(reportParameters.at(6U)));
  require(decodedConfirmationSerial.get() == expectedConfirmationSerial,
          "Restore confirmation service report broke its federate serial sequence");
  requirePortableServiceReport(
      peer,
      *restoreRequestConfirmation,
      reportClass,
      reportParameters,
      L"ConfirmFederationRestorationRequest",
      {{53, L"Federation save label", std::wstring{L"\""} + saveLabel + L"\""},
       {6, L"Request-success indicator", L"true"}},
      {34, L"", L"null"},
      decodedConfirmationSerial.get(),
      "Confirm Federation Restoration Request Table 5 arguments",
      0);
  require(
      owner.federationRestoreBegunCount() == 1U &&
          peer.federationRestoreBegunCount() == 1U,
      "FederationRestoreBegun did not notify both joined members");
  auto const receivedApplicationInteractionCount = [&] {
    auto const received = peer.recorder().interactions();
    return std::count_if(
        received.begin(),
        received.end(),
        [&](InteractionRecord const& interaction) {
          return interaction.interaction == applicationInteraction;
        });
  };
  auto const receivedInteractionsBeforeBlockedSend =
      receivedApplicationInteractionCount();
  requireException(
      [&] {
        owner.rtiAmbassador().sendInteraction(
            applicationInteraction,
            restoreGuardParameters,
            restoreGuardTag);
      },
      L"RestoreInProgress",
      "application Send Interaction after FederationRestoreBegun");
  require(
      receivedApplicationInteractionCount() == receivedInteractionsBeforeBlockedSend,
      "Send Interaction delivered new information after FederationRestoreBegun");
  auto const ownerRestore = owner.federateRestoreInitiations().front();
  auto const peerRestore = peer.federateRestoreInitiations().front();
  require(ownerRestore.label == saveLabel && peerRestore.label == saveLabel &&
              ownerRestore.federateName == options.ownerFederateName &&
              peerRestore.federateName == options.memberFederateName &&
              ownerRestore.postRestoreFederateHandle == ownerHandle &&
              peerRestore.postRestoreFederateHandle == peerHandle,
          "Restore initiation lost participant name or post-restore handle");

  auto const restoreStatusBefore = owner.federationRestoreStatusResponses().size();
  auto const reportsBeforeStatusQuery = peer.recorder().interactions().size();
  owner.rtiAmbassador().queryFederationRestoreStatus();
  waitFor(
      owner,
      peer,
      [&] {
        auto const reports = peer.recorder().interactions();
        return owner.federationRestoreStatusResponses().size() > restoreStatusBefore &&
            std::any_of(
                reports.begin(),
                reports.end(),
                [&](InteractionRecord const& report) {
                  rti::HLAunicodeString decodedService;
                  decodedService.decode(report.parameters.at(reportParameters.at(0U)));
                  return decodedService.get() == L"FederationRestoreStatusResponse";
                });
      },
      options,
      "restore-status callback and public service-report interaction");
  require(owner.federationRestoreStatusResponses().size() == restoreStatusBefore + 1U,
          "Immediate restoring-status response was not delivered directly");
  auto const restoring = owner.federationRestoreStatusResponses().back();
  require(restoring.size() == 2U,
          "Restoring-status response omitted a joined member");
  requireStatusFor(restoring, ownerHandle,
                   ownerRestore.postRestoreFederateHandle,
                   rti::FEDERATE_RESTORING);
  requireStatusFor(restoring, peerHandle,
                   peerRestore.postRestoreFederateHandle,
                   rti::FEDERATE_RESTORING);

  std::wstring expectedRestoreStatusRecords = L"[";
  for (std::size_t index = 0; index < restoring.size(); ++index) {
    if (index != 0U) expectedRestoreStatusRecords.push_back(L',');
    auto const& descriptor = restoring[index];
    expectedRestoreStatusRecords +=
        L"{\"preRestoreHandle\":\"" + descriptor.preRestoreHandle.toString() +
        L"\",\"postRestoreHandle\":\"" + descriptor.postRestoreHandle.toString() +
        L"\",\"status\":\"FEDERATE_RESTORING\"}";
  }
  expectedRestoreStatusRecords.push_back(L']');
  auto const reports = peer.recorder().interactions();
  require(reports.size() >= reportsBeforeStatusQuery + 2U,
          "Restore-status query omitted the service call or RTI response report");
  auto const responseReport = std::find_if(
      reports.begin(),
      reports.end(),
      [&](InteractionRecord const& report) {
        rti::HLAunicodeString decodedService;
        decodedService.decode(report.parameters.at(reportParameters.at(0U)));
        return decodedService.get() == L"FederationRestoreStatusResponse";
      });
  require(responseReport != reports.end(),
          "Federation restore-status response report was not observed");
  auto const responseReportIndex = static_cast<std::size_t>(
      std::distance(reports.begin(), responseReport));
  auto const responseReportFederate = peer.rtiAmbassador().decodeFederateHandle(
      responseReport->parameters.at(reportParameters.at(7U)));
  require(responseReportFederate == ownerHandle,
          "Federation restore-status response report identified the wrong federate");
  rti::Integer32 expectedResponseSerial = 0;
  rti::Integer32 responseReportSerial = -1;
  for (std::size_t index = 0U; index <= responseReportIndex; ++index) {
    auto const reportFederate = peer.rtiAmbassador().decodeFederateHandle(
        reports.at(index).parameters.at(reportParameters.at(7U)));
    if (reportFederate != responseReportFederate) {
      continue;
    }
    rti::HLAinteger32BE decodedSerial;
    decodedSerial.decode(reports.at(index).parameters.at(reportParameters.at(6U)));
    require(
        decodedSerial.get() == expectedResponseSerial,
        "Restore-status MOM report serial for the response federate was " +
            std::to_string(decodedSerial.get()) + " at source position " +
            std::to_string(index) + "; expected " +
            std::to_string(expectedResponseSerial));
    responseReportSerial = decodedSerial.get();
    ++expectedResponseSerial;
  }
  require(responseReportSerial >= 0,
          "Federation restore-status response report was absent from its source sequence");
  requirePortableServiceReport(
      peer,
      *responseReport,
      reportClass,
      reportParameters,
      L"FederationRestoreStatusResponse",
      {{20,
        L"List of joined federates and restore status for each",
        expectedRestoreStatusRecords}},
      {34, L"", L"null"},
      responseReportSerial,
      "Federation restore-status Table 5 argument encoding",
      0);
  owner.rtiAmbassador().federateRestoreComplete();
  require(owner.federationRestoredCount() == 0U &&
              peer.federationRestoredCount() == 0U,
          "Restore finished before the final member completed");
  auto const waitingStatusBefore =
      peer.federationRestoreStatusResponses().size();
  peer.rtiAmbassador().queryFederationRestoreStatus();
  waitFor(
      owner,
      peer,
      [&] {
        return peer.federationRestoreStatusResponses().size() ==
            waitingStatusBefore + 1U;
      },
      options,
      "restore status while one member awaits federation completion");
  auto const waiting = peer.federationRestoreStatusResponses().back();
  require(waiting.size() == 2U,
          "Waiting-for-federation status omitted a joined member");
  requireStatusFor(
      waiting, ownerHandle, ownerRestore.postRestoreFederateHandle,
      rti::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE);
  requireStatusFor(
      waiting, peerHandle, peerRestore.postRestoreFederateHandle,
      rti::FEDERATE_RESTORING);
  peer.rtiAmbassador().federateRestoreComplete();
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federationRestoredCount() == 1U &&
            peer.federationRestoredCount() == 1U;
      },
      options,
      "object snapshot restore completion callbacks");
  require(owner.federationNotRestoredReasons().empty() &&
              peer.federationNotRestoredReasons().empty(),
          "Object snapshot restore did not complete successfully");
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  requireException([&] { owner.rtiAmbassador().getKnownObjectClassHandle(mutated); },
                   L"ObjectInstanceNotKnown",
                   "post-save object after baseline restore");

  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  owner.rtiAmbassador().setServiceReportingSwitch(true);
  auto const idleStatusBefore = owner.federationRestoreStatusResponses().size();
  auto const beforeIdleStatusQuery = readServiceReportFile(*ownerReportFile);
  owner.rtiAmbassador().queryFederationRestoreStatus();
  auto const afterIdleStatusQuery = readServiceReportFile(*ownerReportFile);
  auto const idleStatusService = afterIdleStatusQuery.find(
      "\"HLAservice\":\"FederationRestoreStatusResponse\"",
      beforeIdleStatusQuery.size());
  auto const idleStatusRecordStart = afterIdleStatusQuery.rfind('{', idleStatusService);
  auto const idleStatusRecordEnd = afterIdleStatusQuery.find('\n', idleStatusService);
  require(
      idleStatusService != std::string::npos &&
          idleStatusRecordStart != std::string::npos,
      "Advertised HLAreportServiceFile omitted FederationRestoreStatusResponse");
  auto const idleStatusRecord = std::string_view(afterIdleStatusQuery).substr(
      idleStatusRecordStart,
      (idleStatusRecordEnd == std::string::npos
           ? afterIdleStatusQuery.size()
           : idleStatusRecordEnd) -
          idleStatusRecordStart);
  constexpr std::string_view idleStatusArgumentMarker =
      "\"HLAargumentType\":20,\"HLAargumentName\":\"List of joined federates and restore status for each\","
      "\"HLAargumentValue\":[";
  auto const expectedIdleStatus = [&](rti::FederateHandle const& handle) {
    return std::string{"{\"preRestoreHandle\":\""} +
        toNarrow(handle.toString()) + "\",\"postRestoreHandle\":\"" +
        toNarrow(noPostRestoreHandle.toString()) +
        "\",\"status\":\"NO_RESTORE_IN_PROGRESS\"}";
  };
  auto const ownerIdleStatus = expectedIdleStatus(ownerHandle);
  auto const peerIdleStatus = expectedIdleStatus(peerHandle);
  require(
      idleStatusRecord.find(idleStatusArgumentMarker) !=
              std::string_view::npos &&
          occurrenceCount(idleStatusRecord, "\"preRestoreHandle\":") == 2U &&
          idleStatusRecord.find(ownerIdleStatus) != std::string_view::npos &&
          idleStatusRecord.find(peerIdleStatus) != std::string_view::npos,
      "FederationRestoreStatusResponse file record did not encode exactly the two joined handles with NO_RESTORE_IN_PROGRESS");
  if (model == rti::HLA_EVOKED) {
    require(
        owner.federationRestoreStatusResponses().size() == idleStatusBefore,
        "Evoked idle restore-status callback ran before HLAreportServiceFile inspection");
  }
  waitFor(
      owner,
      [&] {
        return owner.federationRestoreStatusResponses().size() ==
            idleStatusBefore + 1U;
      },
      options,
      "idle federation restore-status callback");
  require(owner.federationRestoreStatusResponses().size() == idleStatusBefore + 1U,
          "Idle restore-status response was not delivered");
  auto const idle = owner.federationRestoreStatusResponses().back();
  require(idle.size() == 2U, "Idle restore status omitted a joined member");
  requireStatusFor(idle, ownerHandle, noPostRestoreHandle,
                   rti::NO_RESTORE_IN_PROGRESS);
  requireStatusFor(idle, peerHandle, noPostRestoreHandle,
                   rti::NO_RESTORE_IN_PROGRESS);

  // Keep the competing-request arbitration check after the status/report
  // assertions above: it adds a second reporting federate, while the Table 5
  // serial-number checks intentionally examine the earlier single-reporter
  // phase.
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  peer.rtiAmbassador().setServiceReportingSwitch(false);
  peer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  auto const competingSaveLabel = saveLabel + L"-competing-restore";
  owner.rtiAmbassador().requestFederationSave(competingSaveLabel);
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federateSaveInitiations() ==
                std::vector<std::wstring>{saveLabel, competingSaveLabel} &&
            peer.federateSaveInitiations() ==
                std::vector<std::wstring>{saveLabel, competingSaveLabel};
      },
      options,
      "competing restore arbitration save initiation callbacks");
  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federationSavedCount() == 2U &&
            peer.federationSavedCount() == 2U;
      },
      options,
      "competing restore arbitration save completion callbacks");
  require(owner.federationNotSavedReasons().empty() &&
              peer.federationNotSavedReasons().empty(),
          "Competing restore arbitration baseline save failed");

  auto const ownerSuccessesBeforeCompetition =
      owner.federationRestoreRequestsSucceeded().size();
  auto const peerSuccessesBeforeCompetition =
      peer.federationRestoreRequestsSucceeded().size();
  auto const ownerFailuresBeforeCompetition =
      owner.federationRestoreRequestsFailed().size();
  auto const peerFailuresBeforeCompetition =
      peer.federationRestoreRequestsFailed().size();
  auto const ownerBegunBeforeCompetition = owner.federationRestoreBegunCount();
  auto const peerBegunBeforeCompetition = peer.federationRestoreBegunCount();
  auto const ownerInitiationsBeforeCompetition =
      owner.federateRestoreInitiations().size();
  auto const peerInitiationsBeforeCompetition =
      peer.federateRestoreInitiations().size();

  std::mutex requestGateMutex;
  std::condition_variable requestGate;
  std::size_t requestersReady = 0U;
  bool releaseRequesters = false;
  std::exception_ptr ownerRequestException;
  std::exception_ptr peerRequestException;
  auto requestAtCommonPoint = [&](Session& requester,
                                  std::exception_ptr& serviceException) {
    {
      std::unique_lock lock(requestGateMutex);
      ++requestersReady;
      requestGate.notify_all();
      if (!requestGate.wait_for(
              lock,
              std::chrono::seconds(10),
              [&] { return releaseRequesters; })) {
        serviceException = std::make_exception_ptr(
            std::runtime_error("Competing restore request did not reach the shared start gate"));
        return;
      }
    }
    try {
      requester.rtiAmbassador().requestFederationRestore(competingSaveLabel);
    } catch (...) {
      serviceException = std::current_exception();
    }
  };
  std::thread ownerRequester([&] {
    requestAtCommonPoint(owner, ownerRequestException);
  });
  std::thread peerRequester([&] {
    requestAtCommonPoint(peer, peerRequestException);
  });
  bool const bothRequestersReady = [&] {
    std::unique_lock lock(requestGateMutex);
    bool const ready = requestGate.wait_for(
        lock,
        std::chrono::seconds(10),
        [&] { return requestersReady == 2U; });
    releaseRequesters = true;
    return ready;
  }();
  requestGate.notify_all();
  ownerRequester.join();
  peerRequester.join();
  require(bothRequestersReady,
          "Both competing restore requesters did not reach the shared start gate");
  require(ownerRequestException == nullptr && peerRequestException == nullptr,
          "A competing Request Federation Restore threw instead of confirming its outcome");
  waitFor(
      owner,
      peer,
      [&] {
        auto const responseCount =
            owner.federationRestoreRequestsSucceeded().size() +
            owner.federationRestoreRequestsFailed().size() +
            peer.federationRestoreRequestsSucceeded().size() +
            peer.federationRestoreRequestsFailed().size();
        auto const responseCountBefore = ownerSuccessesBeforeCompetition +
            peerSuccessesBeforeCompetition + ownerFailuresBeforeCompetition +
            peerFailuresBeforeCompetition;
        return responseCount == responseCountBefore + 2U &&
            owner.federationRestoreBegunCount() == ownerBegunBeforeCompetition + 1U &&
            peer.federationRestoreBegunCount() == peerBegunBeforeCompetition + 1U &&
            owner.federateRestoreInitiations().size() ==
                ownerInitiationsBeforeCompetition + 1U &&
            peer.federateRestoreInitiations().size() ==
                peerInitiationsBeforeCompetition + 1U;
      },
      options,
      "competing restore request outcomes and single restore initiation");
  auto const ownerSuccesses = owner.federationRestoreRequestsSucceeded();
  auto const peerSuccesses = peer.federationRestoreRequestsSucceeded();
  auto const ownerFailures = owner.federationRestoreRequestsFailed();
  auto const peerFailures = peer.federationRestoreRequestsFailed();
  auto const ownerSuccessDelta =
      ownerSuccesses.size() - ownerSuccessesBeforeCompetition;
  auto const peerSuccessDelta =
      peerSuccesses.size() - peerSuccessesBeforeCompetition;
  auto const ownerFailureDelta =
      ownerFailures.size() - ownerFailuresBeforeCompetition;
  auto const peerFailureDelta =
      peerFailures.size() - peerFailuresBeforeCompetition;
  require(
      ownerSuccessDelta + peerSuccessDelta == 1U &&
          ownerFailureDelta + peerFailureDelta == 1U &&
          ownerSuccessDelta + ownerFailureDelta == 1U &&
          peerSuccessDelta + peerFailureDelta == 1U &&
          (ownerSuccessDelta == 0U || ownerSuccesses.back() == competingSaveLabel) &&
          (peerSuccessDelta == 0U || peerSuccesses.back() == competingSaveLabel) &&
          (ownerFailureDelta == 0U || ownerFailures.back() == competingSaveLabel) &&
          (peerFailureDelta == 0U || peerFailures.back() == competingSaveLabel),
      "Competing restore requests did not receive one positive and one negative confirmation");
  auto const confirmationsBeforeLateRequest =
      ownerSuccesses.size() + peerSuccesses.size() + ownerFailures.size() +
      peerFailures.size();
  requireException(
      [&] { peer.rtiAmbassador().requestFederationRestore(competingSaveLabel); },
      L"RestoreInProgress",
      "Request Federation Restore after the competing requests were resolved");
  require(
      owner.federationRestoreRequestsSucceeded().size() +
              owner.federationRestoreRequestsFailed().size() +
              peer.federationRestoreRequestsSucceeded().size() +
              peer.federationRestoreRequestsFailed().size() ==
          confirmationsBeforeLateRequest,
      "A later restore request produced a confirmation instead of RestoreInProgress");

  owner.rtiAmbassador().federateRestoreComplete();
  peer.rtiAmbassador().federateRestoreComplete();
  waitFor(
      owner,
      peer,
      [&] {
        return owner.federationRestoredCount() == 2U &&
            peer.federationRestoredCount() == 2U;
      },
      options,
      "competing restore arbitration completion callbacks");
  require(owner.federationNotRestoredReasons().empty() &&
              peer.federationNotRestoredReasons().empty(),
          "Competing restore arbitration did not complete successfully");

  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  peer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioRestoreObjectSnapshotStatus(Options const& options, rti::CallbackModel model) {
  runRestoreObjectSnapshotStatus(options, model);
}

void scenarioRestoreObjectSnapshotStatusContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreObjectSnapshotStatus(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
