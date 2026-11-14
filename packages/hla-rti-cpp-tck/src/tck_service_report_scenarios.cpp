#include "tck_main_scenarios.hpp"
#include "tck_service_report_file_support.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioMomTableAnnexGPrecedenceServiceReportFile(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty() &&
          !options.serviceReportDirectory.empty(),
      "MOM table precedence testing requires the 2025 FOM, MIM, time implementation, and report directory");

  Session publisher(options, model, "mom-precedence-report-file-publisher");
  Session observer(options, model, "mom-precedence-report-file-observer");
  auto const federation = federationName(
      options,
      "mom-table-annex-g-precedence-service-report-file");
  auto const publisherName =
      options.ownerFederateName + L"-mom-precedence-report-file-publisher";
  service_report_file_support::connectWithServiceReportDirectory(publisher, options);
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(
      options.memberFederateName + L"-mom-precedence-report-file-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  auto const serviceReportingAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAserviceReporting");
  auto const sendToFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAsendServiceReportsToFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid() && serviceReportingAttribute.isValid() &&
          sendToFileAttribute.isValid(),
      "MOM table precedence lookup did not find the joined-federate report attributes");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      rti::AttributeHandleSet{
          federateNameAttribute,
          reportFileAttribute,
          serviceReportingAttribute,
          sendToFileAttribute},
      true,
      L"");

  publisher.join(publisherName, options.federateType, federation);
  require(
      !publisher.rtiAmbassador().getServiceReportingSwitch() &&
          !publisher.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "The precedence witness requires both report switches to start false");

  std::optional<rti::ObjectInstanceHandle> publisherMomObject;
  std::optional<std::filesystem::path> initialReportFile;
  auto const observeInitialReportFile = [&] {
    for (auto const& reflection : observer.recorder().reflections()) {
      auto const name = std::find_if(
          reflection.values.begin(),
          reflection.values.end(),
          [&](auto const& value) { return value.first == federateNameAttribute; });
      auto const reportFile = std::find_if(
          reflection.values.begin(),
          reflection.values.end(),
          [&](auto const& value) { return value.first == reportFileAttribute; });
      if (name == reflection.values.end() || reportFile == reflection.values.end()) {
        continue;
      }
      rti::HLAunicodeString decodedName;
      decodedName.decode(name->second);
      if (decodedName.get() != publisherName) {
        continue;
      }
      rti::HLAunicodeString decodedReportFile;
      decodedReportFile.decode(reportFile->second);
      publisherMomObject = reflection.object;
      initialReportFile = std::filesystem::path(decodedReportFile.get());
      return !initialReportFile->empty();
    }
    return false;
  };
  waitFor(
      observer,
      publisher,
      observeInitialReportFile,
      options,
      "joined-federate HLAreportServiceFile initial reflection");
  require(
      publisherMomObject.has_value() && initialReportFile.has_value() &&
          std::filesystem::is_regular_file(*initialReportFile) &&
          !service_report_file_support::readServiceReportFile(*initialReportFile).empty(),
      "The static HLAreportServiceFile value did not identify the join-initialized report file while both switches were false");

  auto const publisherReportFileValues = [&] {
    std::vector<std::filesystem::path> values;
    for (auto const& reflection : observer.recorder().reflections()) {
      if (reflection.object != *publisherMomObject) {
        continue;
      }
      auto const value = reflection.values.find(reportFileAttribute);
      if (value == reflection.values.end()) {
        continue;
      }
      rti::HLAunicodeString decoded;
      decoded.decode(value->second);
      values.emplace_back(decoded.get());
    }
    return values;
  };
  auto const requireStaticValue = [&] {
    auto const values = publisherReportFileValues();
    require(
        values == std::vector<std::filesystem::path>{*initialReportFile},
        "Switch changes altered or republished the static HLAreportServiceFile value");
  };
  auto const waitForAttributeReflectionAfter = [&](
      rti::AttributeHandle const& attribute,
      std::size_t firstReflection,
      char const* const description) {
    waitFor(
        observer,
        publisher,
        [&] {
          auto const reflections = observer.recorder().reflections();
          for (auto index = firstReflection; index < reflections.size(); ++index) {
            auto const& reflection = reflections[index];
            if (reflection.object == *publisherMomObject &&
                reflection.values.contains(attribute)) {
              return true;
            }
          }
          return false;
        },
        options,
        description);
  };

  auto firstReflection = observer.recorder().reflections().size();
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      publisher.rtiAmbassador().getServiceReportingSwitch(),
      "The precedence witness could not enable Service Reporting");
  waitForAttributeReflectionAfter(
      serviceReportingAttribute,
      firstReflection,
      "Service Reporting MOM switch update");
  requireStaticValue();

  firstReflection = observer.recorder().reflections().size();
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  require(
      publisher.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "The precedence witness could not enable Send Service Reports To File");
  waitForAttributeReflectionAfter(
      sendToFileAttribute,
      firstReflection,
      "Send Service Reports To File MOM switch update");
  requireStaticValue();

  firstReflection = observer.recorder().reflections().size();
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  require(
      !publisher.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "The precedence witness could not disable Send Service Reports To File");
  waitForAttributeReflectionAfter(
      sendToFileAttribute,
      firstReflection,
      "Send Service Reports To File disable MOM switch update");
  requireStaticValue();

  firstReflection = observer.recorder().reflections().size();
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  require(
      publisher.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "The precedence witness could not re-enable Send Service Reports To File");
  waitForAttributeReflectionAfter(
      sendToFileAttribute,
      firstReflection,
      "Send Service Reports To File re-enable MOM switch update");
  requireStaticValue();

  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  publisher.disconnect();
}

void scenarioServiceReportFileSwitchLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty() &&
          !options.serviceReportDirectory.empty(),
      "Service-report file lifecycle testing requires the 2025 FOM, MIM, time implementation, and report directory");

  Session first(options, model, "service-report-file-lifecycle-first");
  Session second(options, model, "service-report-file-lifecycle-second");
  Session observer(options, model, "service-report-file-lifecycle-observer");
  auto const federation = federationName(options, "service-report-file-switch-lifecycle");
  std::wstring const firstName = L"service-report-file-lifecycle-first";
  std::wstring const secondName = L"service-report-file-lifecycle-second";
  service_report_file_support::connectWithServiceReportDirectory(first, options);
  service_report_file_support::connectWithServiceReportDirectory(second, options);
  observer.connect();
  first.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(
      options.memberFederateName + L"-service-report-file-lifecycle-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  auto const serviceReportingAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAserviceReporting");
  auto const sendToFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAsendServiceReportsToFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid() && serviceReportingAttribute.isValid() &&
          sendToFileAttribute.isValid(),
      "The standard Federate MOM did not expose service-report file lifecycle attributes");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      rti::AttributeHandleSet{
          federateNameAttribute,
          reportFileAttribute,
          serviceReportingAttribute,
          sendToFileAttribute},
      true,
      L"");

  first.join(firstName, options.federateType, federation);
  second.join(secondName, options.federateType, federation);
  require(
      !first.rtiAmbassador().getServiceReportingSwitch() &&
          !first.rtiAmbassador().getSendServiceReportsToFileSwitch() &&
          !second.rtiAmbassador().getServiceReportingSwitch() &&
          !second.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Service-report switches did not start disabled for each joined federate");

  using AdvertisedReport = std::pair<rti::ObjectInstanceHandle, std::filesystem::path>;
  auto const findAdvertisedReport = [&] (
      std::wstring const& federateName,
      std::size_t firstReflection) -> std::optional<AdvertisedReport> {
    auto const reflections = observer.recorder().reflections();
    for (auto index = firstReflection; index < reflections.size(); ++index) {
      auto const& reflection = reflections[index];
      auto const name = reflection.values.find(federateNameAttribute);
      auto const reportFile = reflection.values.find(reportFileAttribute);
      if (name == reflection.values.end() || reportFile == reflection.values.end()) {
        continue;
      }
      rti::HLAunicodeString decodedName;
      decodedName.decode(name->second);
      if (decodedName.get() != federateName) {
        continue;
      }
      rti::HLAunicodeString decodedReportFile;
      decodedReportFile.decode(reportFile->second);
      if (decodedReportFile.get().empty()) {
        throw std::runtime_error(
            "Joined federate MOM advertised an empty HLAreportServiceFile value");
      }
      return AdvertisedReport{
          reflection.object,
          std::filesystem::path(decodedReportFile.get())};
    }
    return std::nullopt;
  };

  std::optional<AdvertisedReport> firstReport;
  std::optional<AdvertisedReport> secondReport;
  waitFor(
      observer,
      first,
      [&] {
        firstReport = findAdvertisedReport(firstName, 0U);
        secondReport = findAdvertisedReport(secondName, 0U);
        return firstReport.has_value() && secondReport.has_value();
      },
      options,
      "two joined-federate HLAreportServiceFile reflections");
  require(
      firstReport->second != secondReport->second &&
          std::filesystem::is_regular_file(firstReport->second) &&
          std::filesystem::is_regular_file(secondReport->second),
      "Distinct joined federates did not receive distinct existing service-report files");

  auto const narrowAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (auto const character : value) {
      require(
          character >= 0 && character <= 0x7f,
          "The generated service-report lifecycle names must be ASCII for this text-file check");
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const callbackModelName = model == rti::HLA_IMMEDIATE
      ? std::string{"HLA_IMMEDIATE"}
      : std::string{"HLA_EVOKED"};
  auto const verifyInitialRecord = [&](
      std::filesystem::path const& file,
      std::wstring const& federateName) {
    auto const contents = service_report_file_support::readServiceReportFile(file);
    auto const firstLineEnd = contents.find('\n');
    auto const initialRecord = std::string_view(contents).substr(
        0U,
        firstLineEnd == std::string::npos ? contents.size() : firstLineEnd);
    require(
        initialRecord.find("\"CallbackModel\":\"" + callbackModelName + "\"") !=
            std::string_view::npos &&
            initialRecord.find(narrowAscii(federation)) != std::string_view::npos &&
            initialRecord.find(narrowAscii(federateName)) != std::string_view::npos,
        "The first service-report record omitted Connect, federation, or federate information");
  };
  verifyInitialRecord(firstReport->second, firstName);
  verifyInitialRecord(secondReport->second, secondName);

  auto const reportFileValuesForObject = [&](
      rti::ObjectInstanceHandle const& object) {
    std::vector<std::filesystem::path> values;
    for (auto const& reflection : observer.recorder().reflections()) {
      if (reflection.object != object) {
        continue;
      }
      auto const reportFile = reflection.values.find(reportFileAttribute);
      if (reportFile == reflection.values.end()) {
        continue;
      }
      rti::HLAunicodeString decoded;
      decoded.decode(reportFile->second);
      values.emplace_back(decoded.get());
    }
    return values;
  };
  auto const requireStaticReportFile = [&](AdvertisedReport const& report) {
    require(
        reportFileValuesForObject(report.first) ==
            std::vector<std::filesystem::path>{report.second},
        "A service-report switch change altered or republished static HLAreportServiceFile");
  };
  requireStaticReportFile(*firstReport);
  requireStaticReportFile(*secondReport);

  first.rtiAmbassador().setServiceReportingSwitch(true);
  first.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  auto firstBefore = service_report_file_support::readServiceReportFile(firstReport->second);
  static_cast<void>(first.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot"));
  auto firstEnabled = service_report_file_support::readServiceReportFile(firstReport->second);
  require(
      firstEnabled.size() > firstBefore.size() &&
          firstEnabled.find("\"HLAservice\":\"GetObjectClassHandle\"", firstBefore.size()) !=
              std::string::npos,
      "Both enabled reporting switches did not append the GetObjectClassHandle report record");

  second.rtiAmbassador().setServiceReportingSwitch(true);
  second.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  auto secondBefore = service_report_file_support::readServiceReportFile(secondReport->second);
  static_cast<void>(second.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot"));
  auto secondEnabled = service_report_file_support::readServiceReportFile(secondReport->second);
  require(
      secondEnabled.size() > secondBefore.size() &&
          secondEnabled.find("\"HLAservice\":\"GetObjectClassHandle\"", secondBefore.size()) !=
              std::string::npos &&
          service_report_file_support::readServiceReportFile(firstReport->second) == firstEnabled,
      "A joined federate's report interaction did not append to its own unique file");

  first.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  auto const firstFileDisabledBaseline =
      service_report_file_support::readServiceReportFile(firstReport->second);
  static_cast<void>(first.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot"));
  require(
      service_report_file_support::readServiceReportFile(firstReport->second) ==
          firstFileDisabledBaseline,
      "Disabling file reporting appended a service report to the joined federate's file");

  first.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  auto const firstFileReenabledBaseline =
      service_report_file_support::readServiceReportFile(firstReport->second);
  static_cast<void>(first.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot"));
  auto const firstFileReenabled =
      service_report_file_support::readServiceReportFile(firstReport->second);
  require(
      firstFileReenabled.size() > firstFileReenabledBaseline.size() &&
          firstFileReenabled.find(
              "\"HLAservice\":\"GetObjectClassHandle\"",
              firstFileReenabledBaseline.size()) != std::string::npos,
      "Re-enabling file reporting did not append to the same joined-federate file");

  first.rtiAmbassador().setServiceReportingSwitch(false);
  auto const serviceReportingDisabledBaseline =
      service_report_file_support::readServiceReportFile(firstReport->second);
  static_cast<void>(first.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot"));
  require(
      service_report_file_support::readServiceReportFile(firstReport->second) ==
          serviceReportingDisabledBaseline,
      "Disabling Service Reporting appended a service report while file reporting remained enabled");

  first.rtiAmbassador().setServiceReportingSwitch(true);
  auto const serviceReportingReenabledBaseline =
      service_report_file_support::readServiceReportFile(firstReport->second);
  static_cast<void>(first.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot"));
  auto const serviceReportingReenabled =
      service_report_file_support::readServiceReportFile(firstReport->second);
  require(
      serviceReportingReenabled.size() > serviceReportingReenabledBaseline.size() &&
          serviceReportingReenabled.find(
              "\"HLAservice\":\"GetObjectClassHandle\"",
              serviceReportingReenabledBaseline.size()) != std::string::npos,
      "Re-enabling Service Reporting did not resume appending to the same file");
  requireStaticReportFile(*firstReport);
  requireStaticReportFile(*secondReport);

  auto const firstMomReflectionCount = observer.recorder().reflections().size();
  first.resign(rti::NO_ACTION);
  auto const firstFileAfterResign =
      service_report_file_support::readServiceReportFile(firstReport->second);
  first.join(firstName, options.federateType, federation);
  std::optional<AdvertisedReport> rejoinedReport;
  waitFor(
      observer,
      first,
      [&] {
        rejoinedReport = findAdvertisedReport(firstName, firstMomReflectionCount);
        return rejoinedReport.has_value();
      },
      options,
      "rejoined federate's new HLAreportServiceFile reflection");
  require(
      rejoinedReport->second != firstReport->second &&
          rejoinedReport->second != secondReport->second &&
          std::filesystem::is_regular_file(rejoinedReport->second),
      "A later join reused an existing joined-federate service-report file");
  verifyInitialRecord(rejoinedReport->second, firstName);
  require(
      service_report_file_support::readServiceReportFile(firstReport->second) ==
          firstFileAfterResign,
      "The prior joined-federate report file changed after resignation and rejoin");

  observer.resign(rti::NO_ACTION);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  first.disconnect();
  second.disconnect();
}

void scenarioServiceReportTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Timestamped attribute-update service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Timestamped attribute-update service-report testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-update service-report testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "service-report-timestamped-attribute-publisher");
  Session receiver(options, model, "service-report-timestamped-attribute-receiver");
  Session observer(options, model, "service-report-timestamped-attribute-observer");
  auto const federation = federationName(options, "service-report-timestamped-attribute-update");
  service_report_file_support::connectWithServiceReportDirectory(publisher, options);
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const publisherName =
      options.ownerFederateName + L"-service-report-timestamped-attribute-publisher";
  publisher.join(
      publisherName,
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-timestamped-attribute-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-timestamped-attribute-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> publisherReportFile;
  waitFor(
      observer,
      publisher,
      [&] {
        publisherReportFile = service_report_file_support::advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            publisherName);
        return publisherReportFile.has_value();
      },
      options,
      "publisher HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*publisherReportFile) &&
          !service_report_file_support::readServiceReportFile(*publisherReportFile).empty(),
      "Publisher HLAreportServiceFile did not identify a file with its initial record");

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Timestamped attribute-update service-report lookup returned an invalid handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped attribute-update service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Timestamped attribute-update service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }
  auto const observerReliable =
      observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const receiverReliable =
      receiver.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      observerReliable.isValid() && receiverReliable.isValid(),
      "Timestamped attribute-update service-report lookup returned an invalid reliable transportation handle");

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
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped attribute-update service-report registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped attribute-update service-report object discovery");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.recorder().clearInteraction();
  receiver.recorder().clearTimedReflections();

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped attribute-update service-report members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "timestamped attribute-update service-report time-constrained callback");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped attribute-update service-report could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped attribute-update service-report time-regulation callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{'t', 's', 'o'};
  rti::VariableLengthData const value(valueBytes.data(), valueBytes.size());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::AttributeHandleValueMap const values{{publisherAttribute, value}};
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *timestamp);
  require(
      retraction.isValid(),
      "Timestamped attribute-update service-report update returned an invalid retraction handle");

  if (model == rti::HLA_IMMEDIATE) {
    require(
        observer.recorder().interactions().size() == 1U,
        "Immediate timestamped attribute-update service report was not delivered synchronously");
  } else {
    require(
        observer.recorder().interactions().empty(),
        "Evoked timestamped attribute-update service report was delivered before callback servicing");
    waitFor(
        observer,
        [&] { return observer.recorder().interactions().size() == 1U; },
        options,
        "timestamped attribute-update service-report callback");
  }
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped attribute update was delivered before its time advance");

  auto const report = observer.recorder().interactions().at(0U);
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.producer.isValid() &&
          !report.regions.has_value() && report.transportation == observerReliable,
      "Timestamped attribute-update service report returned non-standard metadata");
  for (auto const& parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "Timestamped attribute-update service report omitted a standard parameter");
  }

  rti::HLAunicodeString decodedService;
  decodedService.decode(report.parameters.at(reportParameters.at(0U)));
  require(
      decodedService.get() == L"UpdateAttributeValues",
      "Timestamped attribute-update service report returned the wrong service name");
  rti::HLAinteger16BE decodedServiceType;
  decodedServiceType.decode(report.parameters.at(reportParameters.at(1U)));
  require(
      decodedServiceType.get() == 2,
      "Timestamped attribute-update service report returned the wrong service type");
  rti::HLAboolean decodedSuccess;
  decodedSuccess.decode(report.parameters.at(reportParameters.at(2U)));
  require(
      decodedSuccess.get(),
      "Timestamped attribute-update service report was not successful");

  auto const quoted = [](std::wstring const& valueText) {
    return std::wstring{L"\""} + valueText + L"\"";
  };
  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{argumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
  require(
      suppliedArguments.size() == 4U,
      "Timestamped attribute-update service report returned the wrong supplied-argument count");
  auto const verifyArgument = [&](std::size_t index,
                                  rti::Integer32 type,
                                  std::wstring const& name,
                                  std::wstring const& valueText) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() == type &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() == name &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get() == valueText,
        "Timestamped attribute-update service report returned a wrong supplied argument");
  };
  verifyArgument(
      0U,
      37,
      L"Object instance designator",
      quoted(object.toString()));
  verifyArgument(
      1U,
      2,
      L"Constrained set of attribute designator and value pairs",
      std::wstring{L"{"} + quoted(publisherAttribute.toString()) + L":\"AQI=\"}");
  verifyArgument(2U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(
      3U,
      31,
      L"Optional timestamp",
      quoted(timestamp->toString()));

  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  returnedArgument.decode(report.parameters.at(reportParameters.at(4U)));
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 33 &&
          dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
              L"Message retraction designator",
      "Timestamped attribute-update service report returned the wrong retraction argument");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  require(
      retractionOpen != std::wstring::npos &&
          retractionClose > retractionOpen + 1U,
      "Timestamped attribute-update retraction had no standard text payload");
  auto const momRetraction = dynamic_cast<rti::HLAunicodeString const&>(
      returnedArgument.get(2U)).get();
  constexpr std::wstring_view momRetractionPrefix = L"\"MessageRetractionHandle<";
  require(
      momRetraction.rfind(std::wstring{momRetractionPrefix}, 0U) == 0U &&
          momRetraction.size() > momRetractionPrefix.size() + 1U &&
          momRetraction[momRetraction.size() - 2U] == L'>' &&
          momRetraction.back() == L'\"' &&
          momRetraction.substr(
              momRetractionPrefix.size(),
              momRetraction.size() - momRetractionPrefix.size() - 2U) ==
              retractionText.substr(
                  retractionOpen + 1U,
                  retractionClose - retractionOpen - 1U),
      "Timestamped attribute-update service report returned a mismatched retraction handle");
  rti::HLAunicodeString decodedException;
  decodedException.decode(report.parameters.at(reportParameters.at(5U)));
  require(
      decodedException.get().empty(),
      "Successful timestamped attribute-update service report carried an exception");
  rti::HLAinteger32BE decodedSerial;
  decodedSerial.decode(report.parameters.at(reportParameters.at(6U)));
  require(
      decodedSerial.get() == 0,
      "Timestamped attribute-update service report did not start at serial zero");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          report.parameters.at(reportParameters.at(7U))) ==
          publisher.federateHandle(),
      "Timestamped attribute-update service report identified the wrong publisher federate");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().timeAdvanceRequest(*timestamp);
  require(
      receiver.recorder().timedReflections().empty(),
      "Timestamped attribute update was delivered before the regulating federate advanced");
  auto const publisherAdvance = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timedReflections().size() == 1U;
      },
      options,
      "timestamped attribute-update service-report delivery and grants");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Timestamped attribute-update service report route delivered after its grant");

  auto const reflection = receiver.recorder().timedReflections().at(0U);
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(receiverAttribute) == 1U &&
          copyBytes(reflection.values.at(receiverAttribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.transportation == receiverReliable &&
          reflection.producer == publisher.federateHandle() &&
          reflection.time == encodeTime(*timestamp) &&
          reflection.timeText == timestamp->toString() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()) &&
          !reflection.regions.has_value(),
      "Timestamped attribute-update reflection returned non-standard data");

  auto const fileTimestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const beforeFileReport =
      service_report_file_support::readServiceReportFile(*publisherReportFile);
  auto const fileRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *fileTimestamp);
  require(
      fileRetraction.isValid(),
      "File-backed timestamped attribute update returned an invalid retraction handle");
  auto const afterFileReport =
      service_report_file_support::readServiceReportFile(*publisherReportFile);
  auto const servicePosition = afterFileReport.find(
      "\"HLAservice\":\"UpdateAttributeValues\"",
      beforeFileReport.size());
  require(
      servicePosition != std::string::npos,
      "Public HLAreportServiceFile did not append UpdateAttributeValues");
  auto const recordLineBreak = afterFileReport.rfind('\n', servicePosition);
  auto const recordStart = recordLineBreak == std::string::npos
      ? 0U
      : recordLineBreak + 1U;
  auto const recordEnd = afterFileReport.find('\n', servicePosition);
  auto const record = std::string_view(afterFileReport).substr(
      recordStart,
      (recordEnd == std::string::npos ? afterFileReport.size() : recordEnd) - recordStart);
  auto const suppliedArgumentsPosition =
      record.find("\"HLAsuppliedArguments\":[");
  auto const suppliedArgumentsEnd = record.find(
      "],\"HLAsuccessIndicator\"",
      suppliedArgumentsPosition);
  require(
      suppliedArgumentsPosition != std::string_view::npos &&
          suppliedArgumentsEnd != std::string_view::npos,
      "Timestamped UpdateAttributeValues file report omitted its SuppliedArgumentList array");
  auto const suppliedArgumentsArray = record.substr(
      suppliedArgumentsPosition,
      suppliedArgumentsEnd + 1U - suppliedArgumentsPosition);
  constexpr std::string_view argumentRecordPrefix{"{\"HLAargumentType\":"};
  std::size_t argumentRecordCount = 0U;
  for (auto position = suppliedArgumentsArray.find(argumentRecordPrefix);
       position != std::string_view::npos;) {
    auto const nextPosition = suppliedArgumentsArray.find(
        argumentRecordPrefix,
        position + argumentRecordPrefix.size());
    auto const argumentEnd = nextPosition == std::string_view::npos
        ? suppliedArgumentsArray.find(']', position)
        : nextPosition;
    require(
        argumentEnd != std::string_view::npos,
        "Timestamped UpdateAttributeValues file report truncated an HLAargument record");
    auto const argumentRecord = suppliedArgumentsArray.substr(
        position,
        argumentEnd - position);
    auto const argumentNamePosition =
        argumentRecord.find("\"HLAargumentName\":");
    auto const argumentValuePosition =
        argumentRecord.find("\"HLAargumentValue\":");
    require(
        argumentNamePosition != std::string_view::npos &&
            argumentValuePosition != std::string_view::npos &&
            argumentNamePosition < argumentValuePosition,
        "Timestamped UpdateAttributeValues file array element omitted the HLAargument name/value fields");
    ++argumentRecordCount;
    position = nextPosition;
  }
  require(
      argumentRecordCount == 4U,
      "Timestamped UpdateAttributeValues file SuppliedArgumentList did not contain four HLAargument records");
  std::string logicalTimeText;
  for (auto const character : fileTimestamp->toString()) {
    require(
        character >= 0 && character <= 0x7f,
        "LogicalTime diagnostic string was not ASCII");
    logicalTimeText.push_back(static_cast<char>(character));
  }
  auto const expectedLogicalTimeArgument =
      "\"HLAargumentType\":31,\"HLAargumentName\":\"Optional timestamp\","
      "\"HLAargumentValue\":\"" + logicalTimeText + "\"";
  require(
      record.find(expectedLogicalTimeArgument) != std::string_view::npos,
      "Timestamped UpdateAttributeValues file report did not encode type-31 LogicalTime as time.toString()");
  auto const fileRetractionText = fileRetraction.toString();
  auto const fileRetractionOpen = fileRetractionText.find(L'(');
  auto const fileRetractionClose = fileRetractionText.find(L')');
  require(
      fileRetractionOpen != std::wstring::npos &&
          fileRetractionClose > fileRetractionOpen + 1U,
      "File-backed timestamped attribute update returned no standard retraction handle text");
  std::string retractionPayload;
  for (auto const character : fileRetractionText.substr(
           fileRetractionOpen + 1U,
           fileRetractionClose - fileRetractionOpen - 1U)) {
    require(
        character >= 0 && character <= 0x7f,
        "MessageRetractionHandle payload was not ASCII");
    retractionPayload.push_back(static_cast<char>(character));
  }
  auto const expectedRetractionArgument =
      "\"HLAargumentType\":33,\"HLAargumentValue\":\"MessageRetractionHandle<" +
      retractionPayload + ">\"";
  require(
      record.find(expectedRetractionArgument) != std::string_view::npos,
      "Timestamped UpdateAttributeValues file report did not encode type-33 MessageRetractionHandle from its returned handle");
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  publisher.rtiAmbassador().retract(fileRetraction);

  auto const fileLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto fileLookahead = publisherTime.factory->makeZero();
  require(
      fileLookahead != nullptr,
      "Timestamped attribute-update service-report could not allocate a file lookahead interval");
  fileLookahead->setToDifference(*fileLookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const beforeLookaheadFileReport =
      service_report_file_support::readServiceReportFile(*publisherReportFile);
  publisher.rtiAmbassador().modifyLookahead(*fileLookahead);
  auto const afterLookaheadFileReport =
      service_report_file_support::readServiceReportFile(*publisherReportFile);
  auto const lookaheadServicePosition = afterLookaheadFileReport.find(
      "\"HLAservice\":\"ModifyLookahead\"",
      beforeLookaheadFileReport.size());
  require(
      lookaheadServicePosition != std::string::npos,
      "Public HLAreportServiceFile did not append ModifyLookahead");
  auto const lookaheadLineBreak =
      afterLookaheadFileReport.rfind('\n', lookaheadServicePosition);
  auto const lookaheadRecordStart = lookaheadLineBreak == std::string::npos
      ? 0U
      : lookaheadLineBreak + 1U;
  auto const lookaheadRecordEnd = afterLookaheadFileReport.find('\n', lookaheadServicePosition);
  auto const lookaheadRecord = std::string_view(afterLookaheadFileReport).substr(
      lookaheadRecordStart,
      (lookaheadRecordEnd == std::string::npos
           ? afterLookaheadFileReport.size()
           : lookaheadRecordEnd) - lookaheadRecordStart);
  std::string logicalTimeIntervalText;
  for (auto const character : fileLookahead->toString()) {
    require(
        character >= 0 && character <= 0x7f,
        "LogicalTimeInterval diagnostic string was not ASCII");
    logicalTimeIntervalText.push_back(static_cast<char>(character));
  }
  auto const expectedLogicalTimeIntervalArgument =
      "\"HLAargumentType\":32,\"HLAargumentName\":\"Requested lookahead\","
      "\"HLAargumentValue\":\"" + logicalTimeIntervalText + "\"";
  require(
      lookaheadRecord.find(expectedLogicalTimeIntervalArgument) != std::string_view::npos,
      "ModifyLookahead file report did not encode type-32 LogicalTimeInterval as interval.toString()");
  require(
      lookaheadRecord.find("\"HLAreturnedArgument\":[null]") != std::string_view::npos,
      "Void ModifyLookahead file report did not encode the returned argument as Null");
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioServiceReportTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportTimestampedAttributeUpdate(options, model);
}

void scenarioServiceReportTimestampedDeleteObjectInstance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Timestamped delete-object service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Timestamped delete-object service-report testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped delete-object service-report testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "service-report-timestamped-delete-publisher");
  Session receiver(options, model, "service-report-timestamped-delete-receiver");
  Session observer(options, model, "service-report-timestamped-delete-observer");
  auto const federation = federationName(options, "service-report-timestamped-delete-object-instance");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-timestamped-delete-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-timestamped-delete-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-timestamped-delete-observer",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Timestamped delete-object service-report lookup returned an invalid object or attribute handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped delete-object service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Timestamped delete-object service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }
  auto const observerReliable =
      observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      observerReliable.isValid(),
      "Timestamped delete-object service-report lookup returned an invalid reliable transportation handle");

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
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped delete-object service-report registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped delete-object service-report object discovery");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.recorder().clearInteraction();
  receiver.recorder().clearTimedRemovals();

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped delete-object service-report members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "timestamped delete-object service-report time-constrained callback");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped delete-object service-report could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped delete-object service-report time-regulation callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const tagBytes{'t', 's', 'o'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *timestamp);
  require(
      retraction.isValid(),
      "Timestamped delete-object service-report delete returned an invalid retraction handle");

  if (model == rti::HLA_IMMEDIATE) {
    require(
        observer.recorder().interactions().size() == 1U,
        "Immediate timestamped delete-object service report was not delivered synchronously");
  } else {
    require(
        observer.recorder().interactions().empty(),
        "Evoked timestamped delete-object service report was delivered before callback servicing");
    waitFor(
        observer,
        [&] { return observer.recorder().interactions().size() == 1U; },
        options,
        "timestamped delete-object service-report callback");
  }
  require(
      receiver.recorder().timedRemovals().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped object deletion was delivered before its time advance");

  auto const report = observer.recorder().interactions().at(0U);
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.producer.isValid() &&
          !report.regions.has_value() && report.transportation == observerReliable,
      "Timestamped delete-object service report returned non-standard metadata");
  for (auto const& parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "Timestamped delete-object service report omitted a standard parameter");
  }

  rti::HLAunicodeString decodedService;
  decodedService.decode(report.parameters.at(reportParameters.at(0U)));
  require(
      decodedService.get() == L"DeleteObjectInstance",
      "Timestamped delete-object service report returned the wrong service name");
  rti::HLAinteger16BE decodedServiceType;
  decodedServiceType.decode(report.parameters.at(reportParameters.at(1U)));
  require(
      decodedServiceType.get() == 2,
      "Timestamped delete-object service report returned the wrong service type");
  rti::HLAboolean decodedSuccess;
  decodedSuccess.decode(report.parameters.at(reportParameters.at(2U)));
  require(
      decodedSuccess.get(),
      "Timestamped delete-object service report was not successful");

  auto const quoted = [](std::wstring const& valueText) {
    return std::wstring{L"\""} + valueText + L"\"";
  };
  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{argumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
  require(
      suppliedArguments.size() == 3U,
      "Timestamped delete-object service report returned the wrong supplied-argument count");
  auto const verifyArgument = [&](std::size_t index,
                                  rti::Integer32 type,
                                  std::wstring const& name,
                                  std::wstring const& valueText) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() == type &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() == name &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get() == valueText,
        "Timestamped delete-object service report returned a wrong supplied argument");
  };
  verifyArgument(
      0U,
      37,
      L"Object instance designator",
      quoted(object.toString()));
  verifyArgument(1U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(
      2U,
      31,
      L"Optional timestamp",
      quoted(timestamp->toString()));

  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  returnedArgument.decode(report.parameters.at(reportParameters.at(4U)));
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 33 &&
          dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
              L"Message retraction designator",
      "Timestamped delete-object service report returned the wrong retraction argument");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  require(
      retractionOpen != std::wstring::npos &&
          retractionClose > retractionOpen + 1U,
      "Timestamped delete-object retraction had no standard text payload");
  auto const momRetraction = dynamic_cast<rti::HLAunicodeString const&>(
                                 returnedArgument.get(2U))
                                 .get();
  constexpr std::wstring_view momRetractionPrefix = L"\"MessageRetractionHandle<";
  require(
      momRetraction.rfind(std::wstring{momRetractionPrefix}, 0U) == 0U &&
          momRetraction.size() > momRetractionPrefix.size() + 1U &&
          momRetraction[momRetraction.size() - 2U] == L'>' &&
          momRetraction.back() == L'\"' &&
          momRetraction.substr(
              momRetractionPrefix.size(),
              momRetraction.size() - momRetractionPrefix.size() - 2U) ==
              retractionText.substr(
                  retractionOpen + 1U,
                  retractionClose - retractionOpen - 1U),
      "Timestamped delete-object service report returned a mismatched retraction handle");
  rti::HLAunicodeString decodedException;
  decodedException.decode(report.parameters.at(reportParameters.at(5U)));
  require(
      decodedException.get().empty(),
      "Successful timestamped delete-object service report carried an exception");
  rti::HLAinteger32BE decodedSerial;
  decodedSerial.decode(report.parameters.at(reportParameters.at(6U)));
  require(
      decodedSerial.get() == 0,
      "Timestamped delete-object service report did not start at serial zero");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().timeAdvanceRequest(*timestamp);
  require(
      receiver.recorder().timedRemovals().empty(),
      "Timestamped object deletion was delivered before the regulating federate advanced");
  auto const publisherAdvance = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timedRemovals().size() == 1U;
      },
      options,
      "timestamped delete-object service-report delivery and grants");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "Timestamped delete-object service report route delivered after its grant");

  auto const removal = receiver.recorder().timedRemovals().at(0U);
  require(
      removal.object == object &&
          removal.tag == tagBytes &&
          removal.producer == publisher.federateHandle() &&
          removal.time == encodeTime(*timestamp) &&
          removal.timeText == timestamp->toString() &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP &&
          removal.retractionPresent &&
          removal.retraction == copyBytes(retraction.encode()),
      "Timestamped delete-object removal returned non-standard metadata");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioServiceReportTimestampedDeleteObjectInstanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportTimestampedDeleteObjectInstance(options, model);
}

void scenarioServiceReportInterlock(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Service-report interlock testing requires an adapter-supplied standard MIM");

  Session session(options, model, "service-report-interlock");
  auto const federation = federationName(options, "service-report-interlock");
  session.connect();
  session.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  session.join(
      options.ownerFederateName + L"-service-report-interlock",
      options.federateType,
      federation);

  auto const reportClass = session.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Service-report interlock lookup returned an invalid HLAreportServiceInvocation handle");
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not start with reporting disabled");

  session.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not enable service reporting");
  requireException(
      [&] { session.rtiAmbassador().subscribeInteractionClass(reportClass); },
      L"FederateServiceInvocationsAreBeingReportedViaMOM",
      "active HLAreportServiceInvocation subscription while reporting is enabled");
  requireException(
      [&] { session.rtiAmbassador().subscribeInteractionClass(reportClass, false); },
      L"FederateServiceInvocationsAreBeingReportedViaMOM",
      "passive HLAreportServiceInvocation subscription while reporting is enabled");
  require(
      session.rtiAmbassador().getServiceReportingSwitch(),
      "Rejected HLAreportServiceInvocation subscriptions changed the reporting switch");

  session.rtiAmbassador().setServiceReportingSwitch(false);
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not disable service reporting");
  session.rtiAmbassador().subscribeInteractionClass(reportClass);
  requireException(
      [&] { session.rtiAmbassador().setServiceReportingSwitch(true); },
      L"ReportServiceInvocationsAreSubscribed",
      "enabling reporting with an active HLAreportServiceInvocation subscription");
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Rejected active HLAreportServiceInvocation subscription changed the reporting switch");
  session.rtiAmbassador().unsubscribeInteractionClass(reportClass);

  session.rtiAmbassador().subscribeInteractionClass(reportClass, false);
  requireException(
      [&] { session.rtiAmbassador().setServiceReportingSwitch(true); },
      L"ReportServiceInvocationsAreSubscribed",
      "enabling reporting with a passive HLAreportServiceInvocation subscription");
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Rejected passive HLAreportServiceInvocation subscription changed the reporting switch");
  session.rtiAmbassador().unsubscribeInteractionClass(reportClass);

  session.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not re-enable reporting after unsubscribe");
  session.rtiAmbassador().setServiceReportingSwitch(false);
  session.resign(rti::NO_ACTION);
  session.rtiAmbassador().destroyFederationExecution(federation);
  session.disconnect();
}

void scenarioServiceReportInterlockContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInterlock(options, model);
}

void scenarioServiceReportSynchronization(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Synchronization service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-synchronization-subject");
  Session observer(options, model, "service-report-synchronization-observer");
  Session departed(options, model, "service-report-synchronization-departed");
  Session reportSink(options, model, "service-report-synchronization-report-sink");
  auto const federation = federationName(options, "service-report-synchronization");
  subject.connect();
  observer.connect();
  departed.connect();
  reportSink.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-synchronization-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-synchronization-observer",
      options.federateType,
      federation);
  departed.join(
      options.memberFederateName + L"-service-report-synchronization-departed",
      options.federateType,
      federation);
  auto const departedFederateHandle = departed.federateHandle();
  departed.resign(rti::NO_ACTION);
  departed.disconnect();
  reportSink.join(
      options.memberFederateName + L"-service-report-synchronization-report-sink",
      options.federateType,
      federation);

  auto const reportClass = reportSink.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Synchronization service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = reportSink.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Synchronization service-report lookup returned an invalid report parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();
  auto const reliable = reportSink.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");

  // Service-report routing is enabled only after all setup declarations have
  // completed, so the asserted serial stream starts with the registration
  // service itself.  MOM serial numbers are scoped to the reported federate;
  // leave the observer switch disabled so this no-region subscription exposes
  // one complete, unambiguous subject stream.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  reportSink.rtiAmbassador().setServiceReportingSwitch(false);
  reportSink.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  reportSink.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(true);

  auto const reportService = [&](InteractionRecord const& report) {
    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters.at(0U)));
    return service.get();
  };
  auto const reportCount = [&](std::wstring const& expectedService) {
    std::size_t count = 0U;
    for (auto const& report : reportSink.recorder().interactions()) {
      if (report.interaction == reportClass && reportService(report) == expectedService) {
        ++count;
      }
    }
    return count;
  };
  auto const latestReport = [&](std::wstring const& expectedService) {
    auto const reports = reportSink.recorder().interactions();
    for (auto iterator = reports.rbegin(); iterator != reports.rend(); ++iterator) {
      if (iterator->interaction == reportClass && reportService(*iterator) == expectedService) {
        return *iterator;
      }
    }
    throw std::runtime_error("missing synchronization service report");
  };
  auto const verifyReport = [&](InteractionRecord const& report,
                                std::wstring const& expectedService,
                                std::vector<std::int32_t> const& expectedTypes,
                                std::vector<std::wstring> const& expectedNames,
                                std::int32_t expectedSerial,
                                std::optional<std::wstring> expectedThirdArgumentValue) {
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid() && report.tag.empty() &&
            !report.regions.has_value() && report.transportation == reliable,
        "Synchronization service report returned non-standard identity or metadata");
    require(
        reportService(report) == expectedService,
        "Synchronization service report returned the wrong service name");

    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters.at(1U)));
    require(
        serviceType.get() == 0,
        "Synchronization service report returned the wrong Federation Management type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters.at(2U)));
    require(success.get(), "Successful synchronization service report was not marked successful");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
    require(
        suppliedArguments.size() == expectedTypes.size() &&
            expectedTypes.size() == expectedNames.size(),
        "Synchronization service report returned the wrong supplied-argument count");
    for (std::size_t index = 0U; index != expectedTypes.size(); ++index) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      auto const actualType =
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get();
      auto const actualName =
          dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get();
      auto const actualValue =
          dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get();
      require(
          actualType == expectedTypes.at(index) &&
              actualName == expectedNames.at(index),
          "Synchronization service report returned the wrong supplied-argument shape");
      if (expectedThirdArgumentValue.has_value() && index == 2U) {
        require(
            actualValue == expectedThirdArgumentValue.value(),
            "Synchronization service report returned the wrong third argument value");
      }
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters.at(4U)));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        "Synchronization service report returned a non-Null return argument");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters.at(5U)));
    require(
        exception.get().empty(),
        "Successful synchronization service report carried an exception");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters.at(6U)));
    require(
        serial.get() == expectedSerial,
        "Synchronization service report returned the wrong serial number (expected " +
            std::to_string(expectedSerial) + ", got " + std::to_string(serial.get()) + ")");
  };

  std::vector<std::uint8_t> const tagBytes{0x53U, 0x59U, 0x4EU, 0x43U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  std::wstring const label = L"service-report-synchronization-point";
  rti::FederateHandleSet const synchronizationSet{
      subject.federateHandle(),
      observer.federateHandle()};
  std::wstring expectedFederateHandleSet = L"[";
  for (auto iterator = synchronizationSet.begin();
       iterator != synchronizationSet.end();
       ++iterator) {
    if (iterator != synchronizationSet.begin()) {
      expectedFederateHandleSet.push_back(L',');
    }
    expectedFederateHandleSet += L"\"" + iterator->toString() + L"\"";
  }
  expectedFederateHandleSet.push_back(L']');
  subject.rtiAmbassador().registerFederationSynchronizationPoint(
      label,
      tag,
      synchronizationSet);
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return subject.synchronizationPointRegistrations().size() == 1U; },
      options,
      "synchronization registration callback");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return observer.synchronizationPointAnnouncements().size() == 1U; },
      options,
      "synchronization announcement callback");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"RegisterFederationSynchronizationPoint") == 1U; },
      options,
      "RegisterFederationSynchronizationPoint service report");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"ConfirmSynchronizationPointRegistration") == 1U; },
      options,
      "ConfirmSynchronizationPointRegistration service report");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"AnnounceSynchronizationPoint") == 1U; },
      options,
      "AnnounceSynchronizationPoint service report");
  verifyReport(
      latestReport(L"RegisterFederationSynchronizationPoint"),
      L"RegisterFederationSynchronizationPoint",
      {53, 60, 18},
      {L"Synchronization point label",
       L"User-supplied tag",
       L"Optional set of joined federate designators"},
      0,
      expectedFederateHandleSet);
  verifyReport(
      latestReport(L"ConfirmSynchronizationPointRegistration"),
      L"ConfirmSynchronizationPointRegistration",
      {53, 6, 34},
      {L"Synchronization point label",
       L"Registration-success indicator",
       L"Optional failure reason"},
      1,
      std::nullopt);
  verifyReport(
      latestReport(L"AnnounceSynchronizationPoint"),
      L"AnnounceSynchronizationPoint",
      {53, 60},
      {L"Synchronization point label", L"User-supplied tag"},
      2,
      std::nullopt);
  auto const registration = subject.synchronizationPointRegistrations().front();
  require(
      registration.succeeded && registration.label == label,
      "Synchronization service-report registration callback returned the wrong result");
  auto const announcement = observer.synchronizationPointAnnouncements().front();
  require(
      announcement.label == label && announcement.tag == tagBytes,
      "Synchronization service-report announcement returned the wrong label or tag");

  // A duplicate label is accepted as a service invocation and reported as a
  // failed confirmation callback, matching the standard two-stage API.
  subject.rtiAmbassador().registerFederationSynchronizationPoint(
      label,
      tag,
      synchronizationSet);
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] {
        return subject.synchronizationPointRegistrations().size() == 2U &&
            reportCount(L"RegisterFederationSynchronizationPoint") == 2U &&
            reportCount(L"ConfirmSynchronizationPointRegistration") == 2U;
      },
      options,
      "duplicate synchronization registration service reports");
  verifyReport(
      latestReport(L"RegisterFederationSynchronizationPoint"),
      L"RegisterFederationSynchronizationPoint",
      {53, 60, 18},
      {L"Synchronization point label",
       L"User-supplied tag",
       L"Optional set of joined federate designators"},
      3,
      expectedFederateHandleSet);
  verifyReport(
      latestReport(L"ConfirmSynchronizationPointRegistration"),
      L"ConfirmSynchronizationPointRegistration",
       {53, 6, 56},
      {L"Synchronization point label",
       L"Registration-success indicator",
       L"Optional failure reason"},
      4,
      L"\"SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE\"");
  auto const duplicate = subject.synchronizationPointRegistrations().back();
  require(
      !duplicate.succeeded && duplicate.label == label &&
          duplicate.failureReason.has_value() &&
          duplicate.failureReason.value() == rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE,
      "Duplicate synchronization registration returned the wrong callback result");

  subject.rtiAmbassador().synchronizationPointAchieved(label, true);
  observer.rtiAmbassador().synchronizationPointAchieved(label, false);
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] {
        return subject.federationSynchronized().size() == 1U &&
            observer.federationSynchronized().size() == 1U;
      },
      options,
      "synchronization completion callbacks");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"SynchronizationPointAchieved") == 1U; },
      options,
      "SynchronizationPointAchieved service report");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"FederationSynchronized") == 1U; },
      options,
      "FederationSynchronized service report");
  verifyReport(
      latestReport(L"SynchronizationPointAchieved"),
      L"SynchronizationPointAchieved",
      {53, 6},
      {L"Synchronization point label", L"Optional synchronization-success indicator"},
      5,
      std::nullopt);
  verifyReport(
      latestReport(L"FederationSynchronized"),
      L"FederationSynchronized",
      {53, 18},
      {L"Synchronization point label", L"Set of joined federate designators"},
      6,
      std::nullopt);
  for (auto* session : {&subject, &observer}) {
    auto const completions = session->federationSynchronized();
    require(
        completions.size() == 1U && completions.front().label == label &&
            completions.front().failedToSyncSet.count(observer.federateHandle()) == 1U,
        "Synchronization service-report completion returned the wrong failure set");
  }

  // A previously joined but now-departed federate is still a valid handle
  // value. Supplying it in the explicit set must report the second standard
  // SynchronizationPointFailureReason enumerator rather than silently
  // accepting a set member that is no longer joined.
  std::wstring const departedMemberLabel =
      L"service-report-synchronization-departed-member";
  rti::FederateHandleSet const departedMemberSet{departedFederateHandle};
  subject.rtiAmbassador().registerFederationSynchronizationPoint(
      departedMemberLabel,
      tag,
      departedMemberSet);
  waitForSessions(
      {&subject, &reportSink},
      [&] {
        return subject.synchronizationPointRegistrations().size() == 3U &&
            reportCount(L"RegisterFederationSynchronizationPoint") == 3U &&
            reportCount(L"ConfirmSynchronizationPointRegistration") == 3U;
      },
      options,
      "departed synchronization-set member failure report");
  auto const departedMemberFailure =
      subject.synchronizationPointRegistrations().back();
  require(
      !departedMemberFailure.succeeded &&
          departedMemberFailure.label == departedMemberLabel &&
          departedMemberFailure.failureReason.has_value() &&
          departedMemberFailure.failureReason.value() ==
              rti::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED,
      "Departed synchronization-set member returned the wrong failure reason");
  verifyReport(
      latestReport(L"ConfirmSynchronizationPointRegistration"),
      L"ConfirmSynchronizationPointRegistration",
      {53, 6, 56},
      {L"Synchronization point label",
       L"Registration-success indicator",
       L"Optional failure reason"},
      8,
      L"\"SYNCHRONIZATION_SET_MEMBER_NOT_JOINED\"");

  require(
      reportSink.recorder().interactions().size() == 9U,
      "Synchronization service-report sink received an unexpected report count");

  reportSink.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  reportSink.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
  reportSink.disconnect();
}

void scenarioServiceReportSynchronizationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportSynchronization(options, model);
}

void scenarioServiceReportFederateSaveBegun(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Federate Save Begun service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "federate-save-begun-service-report-subject");
  Session peer(options, model, "federate-save-begun-service-report-peer");
  Session observer(options, model, "federate-save-begun-service-report-observer");
  auto const federation = federationName(options, "service-report-federate-save-begun");
  subject.connect();
  peer.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-federate-save-begun-service-report-subject",
      options.federateType,
      federation);
  peer.join(
      options.memberFederateName + L"-federate-save-begun-service-report-peer",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-federate-save-begun-service-report-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Federate Save Begun service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Federate Save Begun service-report lookup returned an invalid standard parameter handle");
    reportParameters.push_back(parameter);
  }

  for (auto* session : {&subject, &peer, &observer}) {
    session->rtiAmbassador().setServiceReportingSwitch(false);
    session->rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  }
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(true);

  auto const reportService = [&](InteractionRecord const& report) {
    auto const parameter = report.parameters.find(reportParameters.at(0U));
    if (parameter == report.parameters.end()) {
      return std::wstring{};
    }
    rti::HLAunicodeString service;
    service.decode(parameter->second);
    return service.get();
  };
  auto const hasSaveBegunReport = [&] {
    auto const reports = observer.recorder().interactions();
    return std::any_of(reports.begin(), reports.end(), [&](auto const& report) {
      return report.interaction == reportClass &&
          reportService(report) == L"FederateSaveBegun";
    });
  };

  std::wstring const saveLabel = L"service-report-federate-save-begun";
  subject.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&subject, &peer, &observer},
      [&] {
        return subject.federateSaveInitiations().size() == 1U &&
            peer.federateSaveInitiations().size() == 1U &&
            observer.federateSaveInitiations().size() == 1U;
      },
      options,
      "Federate Save Begun initiation callbacks");
  std::vector<std::wstring> const expectedSaveInitiations{saveLabel};
  require(
      subject.federateSaveInitiations() == expectedSaveInitiations &&
          peer.federateSaveInitiations() == expectedSaveInitiations &&
          observer.federateSaveInitiations() == expectedSaveInitiations,
      "Initiate Federate Save did not instruct every joined member with the requested save label");

  subject.rtiAmbassador().federateSaveBegun();
  waitForSessions(
      {&subject, &peer, &observer},
      hasSaveBegunReport,
      options,
      "FederateSaveBegun HLAreportServiceInvocation callback");
  auto const reports = observer.recorder().interactions();
  auto const report = std::find_if(
      reports.begin(), reports.end(), [&](InteractionRecord const& candidate) {
        return candidate.interaction == reportClass &&
            reportService(candidate) == L"FederateSaveBegun";
      });
  require(report != reports.end(), "FederateSaveBegun service report was not recorded");
  rti::HLAinteger32BE serial;
  serial.decode(report->parameters.at(reportParameters.at(6U)));
  require(
      serial.get() >= 0,
      "FederateSaveBegun service report returned a negative serial number");
  requirePortableServiceReport(
      observer,
      *report,
      reportClass,
      reportParameters,
      L"FederateSaveBegun",
      {},
      PortableServiceReportArgument{34, L"", L"null"},
      serial.get(),
      "FederateSaveBegun service report",
      0);

  peer.rtiAmbassador().federateSaveBegun();
  observer.rtiAmbassador().federateSaveBegun();
  subject.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  observer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&subject, &peer, &observer},
      [&] {
        return subject.federationSavedCount() == 1U &&
            peer.federationSavedCount() == 1U &&
            observer.federationSavedCount() == 1U;
      },
      options,
      "Federate Save Begun service-report save completion");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  peer.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  peer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportFederateSaveBegunContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederateSaveBegun(options, model);
}

void scenarioServiceReportQueryGalt(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Query GALT service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "query-galt-service-report-subject");
  Session regulator(options, model, "query-galt-service-report-regulator");
  Session observer(options, model, "query-galt-service-report-observer");
  auto const federation = federationName(options, "service-report-query-galt");
  service_report_file_support::connectWithServiceReportDirectory(subject, options);
  regulator.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName = options.ownerFederateName + L"-query-galt-service-report-subject";
  subject.join(subjectName, options.federateType, federation);
  regulator.join(
      options.memberFederateName + L"-query-galt-service-report-regulator",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-query-galt-service-report-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Query GALT service-report MOM lookup did not find HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      [&] {
        subjectReportFile = service_report_file_support::advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "Query GALT subject HLAreportServiceFile reflection");
  auto const fileBeforeQuery =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  require(
      std::filesystem::is_regular_file(*subjectReportFile) && !fileBeforeQuery.empty(),
      "Query GALT HLAreportServiceFile did not identify a file with its initial record");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Query GALT report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Query GALT report lookup returned an invalid standard parameter handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const time = makeTimeContext(regulator);
  auto const expectedGalt = timeAfter(
      *time.factory,
      *time.initial,
      *time.epsilon,
      3U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Query GALT service-report factory returned no lookahead");
  lookahead->setToDifference(*expectedGalt, *time.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Query GALT service-report time-regulation callback");

  observer.recorder().clearInteraction();
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto actualGalt = time.factory->makeInitial();
  require(actualGalt != nullptr, "Query GALT service-report factory returned no result time");
  require(
      subject.rtiAmbassador().queryGALT(*actualGalt),
      "Query GALT did not report a defined bound for the active regulator");
  require(
      *actualGalt == *expectedGalt,
      "Query GALT returned a value different from the active regulator's current time plus lookahead");

  waitFor(
      observer,
      [&] {
        for (auto const& candidate : observer.recorder().interactions()) {
          auto const service = candidate.parameters.find(reportParameters[0]);
          if (service == candidate.parameters.end()) {
            continue;
          }
          rti::HLAunicodeString name;
          name.decode(service->second);
          if (name.get() == L"QueryGALT") {
            return true;
          }
        }
        return false;
      },
      options,
      "QueryGALT HLAreportServiceInvocation callback");
  auto const reports = observer.recorder().interactions();
  auto const reportIterator = std::find_if(
      reports.begin(),
      reports.end(),
      [&](InteractionRecord const& candidate) {
        auto const service = candidate.parameters.find(reportParameters[0]);
        if (service == candidate.parameters.end()) {
          return false;
        }
        rti::HLAunicodeString name;
        name.decode(service->second);
        return name.get() == L"QueryGALT";
      });
  require(reportIterator != reports.end(), "QueryGALT service report was not recorded");
  auto const& report = *reportIterator;
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.regions.has_value() && !report.producer.isValid() &&
          report.transportation ==
              observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "QueryGALT service report returned invalid interaction metadata");
  for (auto const parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "QueryGALT service report omitted a standard parameter");
  }

  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      serviceType.get() == 4 && success.get(),
      "QueryGALT service report returned the wrong service type or success indicator");
  rti::HLAvariableArray suppliedArguments{rti::HLAfixedRecord{}};
  suppliedArguments.decode(report.parameters.at(reportParameters[3]));
  require(
      suppliedArguments.size() == 0U,
      "QueryGALT service report returned unexpected supplied arguments");

  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  returnedArgument.decode(report.parameters.at(reportParameters[4]));
  auto const reportedArgumentType =
      dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get();
  auto const reportedArgumentName =
      dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get();
  auto const reportedGalt =
      dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get();
  auto const expectedText =
      L"{\"timeIsValid\":true,\"time\":\"" + expectedGalt->toString() + L"\"}";
  require(
      reportedArgumentType == 58 && reportedArgumentName == L"GALT" &&
          reportedGalt == expectedText,
      "QueryGALT service report did not preserve its standard TimeQueryReturn fields: expected type 58, name GALT, value " +
          toNarrow(expectedText) + "; received type " + std::to_string(reportedArgumentType) +
          ", name " + toNarrow(reportedArgumentName) + ", value " + toNarrow(reportedGalt));
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      exception.get().empty() && serial.get() == 0 &&
          observer.rtiAmbassador().decodeFederateHandle(
              report.parameters.at(reportParameters[7])) == subject.federateHandle(),
      "QueryGALT service report returned an exception, wrong serial, or wrong federate");

  auto const interactionCount = observer.recorder().interactions().size();
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      subject.rtiAmbassador().getServiceReportingSwitch() &&
          subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Query GALT file-reporting switches did not retain their enabled state");
  auto fileReportedGalt = time.factory->makeInitial();
  require(
      fileReportedGalt != nullptr && subject.rtiAmbassador().queryGALT(*fileReportedGalt) &&
          *fileReportedGalt == *expectedGalt,
      "Query GALT returned a wrong or undefined value while file reporting was selected");
  auto const fileAfterQuery =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  require(
      fileAfterQuery.size() > fileBeforeQuery.size(),
      "Query GALT did not append a service-report record to the advertised file (before=" +
          std::to_string(fileBeforeQuery.size()) + ", after=" +
          std::to_string(fileAfterQuery.size()) + ", interactions=" +
          std::to_string(observer.recorder().interactions().size()) + ")");
  auto const appendedFileRecords = fileAfterQuery.substr(fileBeforeQuery.size());
  auto const expectedTimeField =
      std::string{"\"time\":\""} + toNarrow(expectedGalt->toString()) + "\"";
  require(
      appendedFileRecords.find("\"HLAservice\":\"QueryGALT\"") != std::string::npos &&
          appendedFileRecords.find("\"timeIsValid\":true") != std::string::npos &&
          appendedFileRecords.find(expectedTimeField) != std::string::npos,
      "Query GALT Table 5 file record omitted its service or valid time-query fields");

  auto subjectTime = makeTimeContext(subject);
  auto subjectLookahead = subjectTime.factory->makeZero();
  require(
      subjectLookahead != nullptr,
      "Query Lookahead service-report factory returned no lookahead interval");
  subjectLookahead->setToDifference(*expectedGalt, *subjectTime.initial);
  subject.rtiAmbassador().enableTimeRegulation(*subjectLookahead);
  waitFor(
      subject,
      [&] { return subject.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Query Lookahead service-report time-regulation callback");
  auto const fileBeforeLookaheadQuery =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  auto queriedLookahead = subjectTime.factory->makeZero();
  require(
      queriedLookahead != nullptr,
      "Query Lookahead service-report factory returned no result interval");
  subject.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(
      *queriedLookahead == *subjectLookahead,
      "Query Lookahead returned an interval different from the active regulator's lookahead");
  auto const fileAfterLookaheadQuery =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  auto const appendedLookaheadRecords =
      fileAfterLookaheadQuery.substr(fileBeforeLookaheadQuery.size());
  auto const expectedLookaheadField =
      std::string{"\"interval\":\""} + toNarrow(subjectLookahead->toString()) + "\"";
  require(
      appendedLookaheadRecords.find("\"HLAservice\":\"QueryLookahead\"") !=
              std::string::npos &&
          appendedLookaheadRecords.find("\"intervalIsValid\":true") !=
              std::string::npos &&
          appendedLookaheadRecords.find(expectedLookaheadField) != std::string::npos,
      "Query Lookahead Table 5 file record omitted its service or valid interval-query fields");
  require(
      observer.recorder().interactions().size() == interactionCount,
      "Time-query file reporting unexpectedly delivered an interaction report");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  regulator.rtiAmbassador().disableTimeRegulation();
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  regulator.disconnect();
  subject.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
