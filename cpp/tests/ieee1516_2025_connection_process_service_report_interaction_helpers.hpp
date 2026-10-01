#pragma once
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

namespace {
enum class ProcessServiceReportInteractionOperation {
  GetDimensionHandleNameNotFound,
  GetObjectClassHandleNameNotFound,
  GetInteractionClassHandleNameNotFound,
  GetTransportationTypeHandleInvalidName,
  GetTransportationTypeNameInvalidHandle,
  GetTransportationTypeNameSuccess,
  GetOrderNameSuccess,
  GetOrderTypeSuccess,
  GetOrderNameInvalidType,
  GetObjectClassNameInvalidHandle,
  GetObjectClassNameSuccess,
  GetInteractionClassNameInvalidHandle,
  GetInteractionClassNameSuccess,
  GetDimensionName,
  GetDimensionUpperBound,
  GetAvailableDimensionsForObjectClass,
  GetAvailableDimensionsForInteractionClass,
  SendInteractionSuccess,
  SendInteractionWithRegionsSuccess,
  SendInteractionWithRegionsInvalidRegion,
  SendInteractionWithRegionsInvalidParameter
};

enum class ProcessFailedServiceReportHandleState {
  NotApplicable,
  BindingInvalid,
  FederationUnknown
};

void runProcessServiceReportInteraction(
    ProcessServiceReportInteractionOperation operation,
    ProcessFailedServiceReportHandleState handleState,
    bool selectFileDestination = false) {
  auto const bindingInvalidHandle =
      handleState == ProcessFailedServiceReportHandleState::BindingInvalid;
  auto const dimensionHandleNameNotFound =
      operation ==
      ProcessServiceReportInteractionOperation::GetDimensionHandleNameNotFound;
  auto const objectClassHandleNameNotFound =
      operation ==
      ProcessServiceReportInteractionOperation::GetObjectClassHandleNameNotFound;
  auto const interactionClassHandleNameNotFound =
      operation == ProcessServiceReportInteractionOperation::
                       GetInteractionClassHandleNameNotFound;
  auto const transportationTypeHandleInvalidName =
      operation == ProcessServiceReportInteractionOperation::
                       GetTransportationTypeHandleInvalidName;
  auto const transportationTypeNameInvalidHandle =
      operation == ProcessServiceReportInteractionOperation::
                       GetTransportationTypeNameInvalidHandle;
  auto const transportationTypeNameSuccess =
      operation == ProcessServiceReportInteractionOperation::
                       GetTransportationTypeNameSuccess &&
      handleState == ProcessFailedServiceReportHandleState::NotApplicable;
  auto const orderNameInvalidType =
      operation == ProcessServiceReportInteractionOperation::GetOrderNameInvalidType;
  auto const orderNameSuccess =
      operation == ProcessServiceReportInteractionOperation::GetOrderNameSuccess;
  auto const orderTypeSuccess =
      operation == ProcessServiceReportInteractionOperation::GetOrderTypeSuccess;
  auto const dimensionNameSuccess =
      operation == ProcessServiceReportInteractionOperation::GetDimensionName &&
      handleState == ProcessFailedServiceReportHandleState::NotApplicable;
  auto const objectClassNameSuccess =
      operation ==
          ProcessServiceReportInteractionOperation::GetObjectClassNameSuccess &&
      handleState == ProcessFailedServiceReportHandleState::NotApplicable;
  auto const interactionClassNameSuccess =
      operation == ProcessServiceReportInteractionOperation::
                       GetInteractionClassNameSuccess &&
      handleState == ProcessFailedServiceReportHandleState::NotApplicable;
  auto const objectClassNameInvalidHandle =
      operation ==
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle;
  auto const interactionClassNameInvalidHandle =
      operation == ProcessServiceReportInteractionOperation::
                       GetInteractionClassNameInvalidHandle;
  auto const dimensionUpperBoundLookup =
      operation == ProcessServiceReportInteractionOperation::GetDimensionUpperBound;
  auto const dimensionUpperBoundSuccess =
      dimensionUpperBoundLookup &&
      handleState == ProcessFailedServiceReportHandleState::NotApplicable;
  auto const availableDimensionsForObjectClassLookup =
      operation ==
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass;
  auto const availableDimensionsForObjectClassSuccess =
      availableDimensionsForObjectClassLookup &&
      handleState == ProcessFailedServiceReportHandleState::NotApplicable;
  auto const availableDimensionsForInteractionClassLookup =
      operation == ProcessServiceReportInteractionOperation::
                       GetAvailableDimensionsForInteractionClass;
  auto const availableDimensionsForInteractionClassSuccess =
      availableDimensionsForInteractionClassLookup &&
      handleState == ProcessFailedServiceReportHandleState::NotApplicable;
  auto const sendInteractionSuccess =
      operation == ProcessServiceReportInteractionOperation::SendInteractionSuccess;
  auto const sendInteractionWithRegionsSuccess =
      operation == ProcessServiceReportInteractionOperation::
                       SendInteractionWithRegionsSuccess;
  auto const sendInteractionWithRegionsInvalidRegion =
      operation == ProcessServiceReportInteractionOperation::
                       SendInteractionWithRegionsInvalidRegion;
  auto const sendInteractionWithRegionsInvalidParameter =
      operation == ProcessServiceReportInteractionOperation::
                       SendInteractionWithRegionsInvalidParameter;
  auto const regionalSend = sendInteractionWithRegionsSuccess ||
      sendInteractionWithRegionsInvalidRegion ||
      sendInteractionWithRegionsInvalidParameter;
  auto const sendInteractionServiceSuccess =
      sendInteractionSuccess || sendInteractionWithRegionsSuccess;
  auto const sendInteractionService =
      sendInteractionServiceSuccess || sendInteractionWithRegionsInvalidRegion ||
      sendInteractionWithRegionsInvalidParameter;
  class ServiceReportObserver final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      ++receivedInteractionCount;
      receivedInteractionClass = interactionClass;
      receivedParameterValues = parameterValues;
      receivedTag = userSuppliedTag;
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedSentRegions = optionalSentRegions != nullptr;
    }

    std::size_t receivedInteractionCount = 0U;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    ParameterHandleValueMap receivedParameterValues;
    VariableLengthData receivedTag;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    bool receivedSentRegions = false;
  };

  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;
  using umbra::detail::TransportServiceMessage;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-dimension-lookup-interaction-report-execution";

  std::exception_ptr serverError;
  std::mutex serverErrorMutex;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      std::vector<std::unique_ptr<ProcessTransportSession>> sessions;
      std::vector<std::shared_ptr<umbra::detail::ProcessTransportConnection>>
          connections;
      std::vector<std::thread> workers;
      auto acceptAndServe = [&](char const* endpointName, std::uint64_t endpointId) {
        auto connection = listener->accept(
            nullptr,
            {endpointName, endpointId},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        auto session = std::make_unique<ProcessTransportSession>(connection);
        auto* sessionPointer = session.get();
        auto handler = service.handlerFor(*sessionPointer);
        sessions.push_back(std::move(session));
        connections.push_back(connection);
        workers.emplace_back(
            [&, sessionPointer, connection, handler = std::move(handler)]() mutable {
              try {
                while (ProcessTransportServiceDispatcher::serveOne(
                    *sessionPointer, handler)) {
                }
                service.detach(*sessionPointer);
                connection->close();
              } catch (...) {
                service.detach(*sessionPointer);
                connection->close();
                std::scoped_lock lock(serverErrorMutex);
                if (!serverError) {
                  serverError = std::current_exception();
                }
              }
            });
      };

      // Start servicing the reporting federate before accepting the observer;
      // its Create and Join requests are synchronous public API calls.
      acceptAndServe("process-service-report-interaction-reporter", 0x9E31U);
      acceptAndServe("process-service-report-interaction-observer", 0x9E32U);
      for (auto& worker : workers) {
        worker.join();
      }
    } catch (...) {
      std::scoped_lock lock(serverErrorMutex);
      if (!serverError) {
        serverError = std::current_exception();
      }
    }
  });

  TestFederateAmbassador reporterCallbacks;
  ServiceReportObserver observerCallbacks;
  auto reporter = makeRti();
  auto observer = makeRti();
  auto reporterConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-service-report-interaction-reporter")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto observerConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-service-report-interaction-observer")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool reporterJoined = false;
  bool observerJoined = false;
  auto reportFiles = [&] {
    std::vector<std::pair<std::filesystem::path, std::string>> files;
    std::error_code error;
    for (auto const& entry :
         std::filesystem::directory_iterator(reportDirectory, error)) {
      if (error) {
        break;
      }
      if (!entry.is_regular_file(error) || error) {
        continue;
      }
      std::ifstream stream(entry.path(), std::ios::binary);
      files.emplace_back(
          entry.path(),
          std::string(
              std::istreambuf_iterator<char>(stream),
              std::istreambuf_iterator<char>()));
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  std::string successfulObjectClassText;
  std::string successfulInteractionClassText;
  std::string successfulTransportationTypeText;
  std::string successfulDimensionText;
  std::string successfulSendInteractionClassText;
  std::string successfulSendInteractionParameterText;
  std::string successfulSendInteractionRegionText;
  std::string failedSendInteractionParameterText;
  auto toAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto expectedSuccessFileRecord = [&](std::size_t serial) {
    auto const serialText = std::to_string(serial);
    if (sendInteractionSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[null],"HLAservice":"SendInteraction","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
          successfulSendInteractionClassText +
          R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":{")" +
          successfulSendInteractionParameterText +
          R"(":"ECA="}},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"UFVC"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (sendInteractionWithRegionsSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[null],"HLAservice":"SendInteractionWithRegions","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
          successfulSendInteractionClassText +
          R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":{")" +
          successfulSendInteractionParameterText +
          R"(":"ECA="}},{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
          successfulSendInteractionRegionText +
          R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"UFVC"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (availableDimensionsForObjectClassSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
          successfulDimensionText +
          R"("]}],"HLAservice":"GetAvailableDimensionsForObjectClass","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
          successfulObjectClassText +
          R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (availableDimensionsForInteractionClassSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
          successfulDimensionText +
          R"("]}],"HLAservice":"GetAvailableDimensionsForInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
          successfulInteractionClassText +
          R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (objectClassNameSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Object class name","HLAargumentValue":"HLAobjectRoot.Food.Drink"}],"HLAservice":"GetObjectClassName","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
          successfulObjectClassText +
          R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (interactionClassNameSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Interaction class name","HLAargumentValue":"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"}],"HLAservice":"GetInteractionClassName","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
          successfulInteractionClassText +
          R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (transportationTypeNameSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Transportation type name","HLAargumentValue":"HLAreliable"}],"HLAservice":"GetTransportationTypeName","HLAsuppliedArguments":[{"HLAargumentType":59,"HLAargumentName":"Transportation type handle","HLAargumentValue":")" +
          successfulTransportationTypeText +
          R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (dimensionNameSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"ServerId"}],"HLAservice":"GetDimensionName","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
          successfulDimensionText +
          R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (dimensionUpperBoundSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Dimension upper bound","HLAargumentValue":25}],"HLAservice":"GetDimensionUpperBound","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
          successfulDimensionText +
          R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    if (orderNameSuccess) {
      return std::string{R"({"HLAserialNumber":)"} + serialText +
          R"(,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"Receive"}],"HLAservice":"GetOrderName","HLAsuppliedArguments":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"RECEIVE"}],"HLAsuccessIndicator":true,"HLAexception":null})";
    }
    return std::string{R"({"HLAserialNumber":)"} + serialText +
        R"(,"HLAreturnedArgument":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"TIMESTAMP"}],"HLAservice":"GetOrderType","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"TimeStamp"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };
  auto verifySingleSuccessfulFileAppend = [&](auto const& before,
                                              auto const& after,
                                              std::filesystem::path const& reporterPath) {
    REQUIRE(before.size() == after.size());
    bool foundReporterFile = false;
    for (std::size_t index = 0U; index < before.size(); ++index) {
      REQUIRE(before[index].first == after[index].first);
      if (before[index].first != reporterPath) {
        REQUIRE(before[index].second == after[index].second);
        continue;
      }
      foundReporterFile = true;
      REQUIRE(after[index].second.size() > before[index].second.size());
      REQUIRE(after[index].second.compare(
                  0U,
                  before[index].second.size(),
                  before[index].second) == 0);
      auto const appendedRecord = after[index].second.substr(
          before[index].second.size());
      constexpr std::string_view serialPrefix = R"({"HLAserialNumber":)";
      REQUIRE(appendedRecord.rfind(serialPrefix, 0U) == 0U);
      auto const serialEnd = appendedRecord.find(',', serialPrefix.size());
      REQUIRE(serialEnd != std::string::npos);
      auto const serial = static_cast<std::size_t>(std::stoull(
          appendedRecord.substr(serialPrefix.size(), serialEnd - serialPrefix.size())));
      REQUIRE(appendedRecord == expectedSuccessFileRecord(serial));
    }
    REQUIRE(foundReporterFile);
  };
  auto verifySingleFailedFileAppend = [&](auto const& before,
                                          auto const& after,
                                          std::filesystem::path const& reporterPath,
                                          std::string_view expectedServiceName,
                                          std::string_view expectedException,
                                          std::string_view expectedAppendedRecord = {}) {
    REQUIRE(before.size() == after.size());
    bool foundReporterFile = false;
    for (std::size_t index = 0U; index < before.size(); ++index) {
      REQUIRE(before[index].first == after[index].first);
      if (before[index].first != reporterPath) {
        REQUIRE(before[index].second == after[index].second);
        continue;
      }
      foundReporterFile = true;
      REQUIRE(after[index].second.size() > before[index].second.size());
      REQUIRE(after[index].second.compare(
                  0U,
                  before[index].second.size(),
                  before[index].second) == 0);
      auto const appendedRecord = after[index].second.substr(
          before[index].second.size());
      constexpr std::string_view serialPrefix = R"({"HLAserialNumber":)";
      REQUIRE(appendedRecord.rfind(serialPrefix, 0U) == 0U);
      auto const serialEnd = appendedRecord.find(',', serialPrefix.size());
      REQUIRE(serialEnd != std::string::npos);
      REQUIRE_NOTHROW(static_cast<void>(std::stoull(
          appendedRecord.substr(serialPrefix.size(), serialEnd - serialPrefix.size()))));
      auto const serviceFragment = std::string{R"("HLAservice":")"} +
          std::string{expectedServiceName} + '"';
      auto const exceptionFragment = std::string{R"("HLAexception":")"} +
          std::string{expectedException} + '"';
      REQUIRE(appendedRecord.find(serviceFragment) != std::string::npos);
      REQUIRE(appendedRecord.find(R"("HLAsuccessIndicator":false)") !=
              std::string::npos);
      REQUIRE(appendedRecord.find(R"("HLAreturnedArgument":[null])") !=
              std::string::npos);
      REQUIRE(appendedRecord.find(exceptionFragment) != std::string::npos);
      if (!expectedAppendedRecord.empty()) {
        REQUIRE(appendedRecord.compare(expectedAppendedRecord) == 0);
      }
    }
    REQUIRE(foundReporterFile);
  };
  try {
    REQUIRE(reporter->connect(
                reporterCallbacks, HLA_EVOKED, reporterConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(reporter->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const reporterHandle = reporter->joinFederationExecution(
        L"process-service-report-interaction-reporter",
        L"process-service-report-interaction-type",
        federationName);
    REQUIRE(reporterHandle.isValid());
    reporterJoined = true;
    REQUIRE_FALSE(reporter->getServiceReportingSwitch());
    REQUIRE_FALSE(reporter->getSendServiceReportsToFileSwitch());

    auto const reporterInitialFile = reportFiles();
    REQUIRE(reporterInitialFile.size() == 1U);
    REQUIRE_FALSE(reporterInitialFile.front().second.empty());
    rti1516_2025::InteractionClassHandle successfulSendInteractionClass;
    rti1516_2025::ParameterHandle successfulSendInteractionParameter;
    rti1516_2025::ParameterHandle invalidSendInteractionParameter;
    rti1516_2025::RegionHandle successfulSendInteractionRegion;
    if (sendInteractionService) {
      successfulSendInteractionClass = reporter->getInteractionClassHandle(
          L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
      REQUIRE(successfulSendInteractionClass.isValid());
      successfulSendInteractionParameter = reporter->getParameterHandle(
          successfulSendInteractionClass, L"TimelinessOk");
      REQUIRE(successfulSendInteractionParameter.isValid());
      successfulSendInteractionClassText =
          toAscii(successfulSendInteractionClass.toString());
      successfulSendInteractionParameterText =
          toAscii(successfulSendInteractionParameter.toString());
      if (sendInteractionWithRegionsInvalidParameter) {
        failedSendInteractionParameterText =
            toAscii(invalidSendInteractionParameter.toString());
      }
      if (sendInteractionWithRegionsSuccess ||
          sendInteractionWithRegionsInvalidParameter) {
        auto const dimension = reporter->getDimensionHandle(L"ServerId");
        REQUIRE(dimension.isValid());
        successfulSendInteractionRegion = reporter->createRegion(
            rti1516_2025::DimensionHandleSet{dimension});
        REQUIRE(successfulSendInteractionRegion.isValid());
        REQUIRE_NOTHROW(reporter->setRangeBounds(
            successfulSendInteractionRegion,
            dimension,
            rti1516_2025::RangeBounds(0UL, 1UL)));
        REQUIRE_NOTHROW(reporter->commitRegionModifications(
            rti1516_2025::RegionHandleSet{successfulSendInteractionRegion}));
        successfulSendInteractionRegionText =
            toAscii(successfulSendInteractionRegion.toString());
      }
      REQUIRE_NOTHROW(
          reporter->publishInteractionClass(successfulSendInteractionClass));
    }
    REQUIRE_NOTHROW(reporter->setServiceReportingSwitch(true));
    auto const failedDimension = bindingInvalidHandle
                                     ? rti1516_2025::DimensionHandle{}
                                     : rti1516_2025::umbra_binding_detail::makeDimensionHandle(
                                           0xFEDCBA9876543210ULL);
    auto const failedObjectClass = bindingInvalidHandle
                                       ? rti1516_2025::ObjectClassHandle{}
                                       : rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                                             0xFEDCBA9876543210ULL);
    auto const failedInteractionClass = bindingInvalidHandle
                                            ? rti1516_2025::InteractionClassHandle{}
                                            : rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                                                  0xFEDCBA9876543210ULL);
    auto const failedTransportationType = bindingInvalidHandle
                                              ? rti1516_2025::TransportationTypeHandle{}
                                              : rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle(
                                                    0xFEDCBA9876543210ULL);
    auto const successfulTransportationType = transportationTypeNameSuccess
                                                  ? reporter->getTransportationTypeHandle(
                                                        L"HLAreliable")
                                                  : rti1516_2025::TransportationTypeHandle{};
    constexpr wchar_t const* failedDimensionName = L"MissingDimension";
    constexpr wchar_t const* failedObjectClassName = L"MissingObjectClass";
    constexpr wchar_t const* failedInteractionClassName =
        L"MissingInteractionClass";
    constexpr wchar_t const* failedTransportationTypeName =
        L"MissingTransportation";
    auto const successfulObjectClass =
        (availableDimensionsForObjectClassSuccess || objectClassNameSuccess)
                                           ? reporter->getObjectClassHandle(
                                                 L"HLAobjectRoot.Food.Drink")
                                           : rti1516_2025::ObjectClassHandle{};
    auto const successfulInteractionClass =
        (availableDimensionsForInteractionClassSuccess ||
         interactionClassNameSuccess)
            ? reporter->getInteractionClassHandle(
                  L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed")
            : rti1516_2025::InteractionClassHandle{};
    auto const successfulDimension =
        (availableDimensionsForObjectClassSuccess ||
         dimensionUpperBoundSuccess)
                                         ? reporter->getDimensionHandle(L"BarQuantity")
                                         : (availableDimensionsForInteractionClassSuccess ||
                                            dimensionNameSuccess)
                                               ? reporter->getDimensionHandle(L"ServerId")
                                               : rti1516_2025::DimensionHandle{};
    REQUIRE(failedDimension.isValid() != bindingInvalidHandle);
    if (availableDimensionsForObjectClassSuccess || objectClassNameSuccess ||
        availableDimensionsForInteractionClassSuccess ||
        interactionClassNameSuccess || transportationTypeNameSuccess ||
        dimensionNameSuccess ||
        dimensionUpperBoundSuccess) {
      if (availableDimensionsForObjectClassSuccess || objectClassNameSuccess) {
        REQUIRE(successfulObjectClass.isValid());
        successfulObjectClassText = toAscii(successfulObjectClass.toString());
      }
      if (availableDimensionsForInteractionClassSuccess ||
          interactionClassNameSuccess) {
        REQUIRE(successfulInteractionClass.isValid());
        successfulInteractionClassText =
            toAscii(successfulInteractionClass.toString());
      }
      if (transportationTypeNameSuccess) {
        REQUIRE(successfulTransportationType.isValid());
        successfulTransportationTypeText =
            toAscii(successfulTransportationType.toString());
      }
      if (availableDimensionsForObjectClassSuccess ||
          availableDimensionsForInteractionClassSuccess || dimensionNameSuccess ||
          dimensionUpperBoundSuccess) {
        REQUIRE(successfulDimension.isValid());
        successfulDimensionText = toAscii(successfulDimension.toString());
      }
    }
    if ((!availableDimensionsForObjectClassSuccess &&
         availableDimensionsForObjectClassLookup) ||
        objectClassNameInvalidHandle) {
      REQUIRE(failedObjectClass.isValid() != bindingInvalidHandle);
    }
    if ((!availableDimensionsForInteractionClassSuccess &&
         availableDimensionsForInteractionClassLookup) ||
        interactionClassNameInvalidHandle) {
      REQUIRE(failedInteractionClass.isValid() != bindingInvalidHandle);
    }
    if (transportationTypeNameInvalidHandle) {
      REQUIRE(failedTransportationType.isValid() != bindingInvalidHandle);
    }
    auto invokeService = [&] {
      if (dimensionHandleNameNotFound) {
        static_cast<void>(reporter->getDimensionHandle(failedDimensionName));
      } else if (objectClassHandleNameNotFound) {
        static_cast<void>(
            reporter->getObjectClassHandle(failedObjectClassName));
      } else if (interactionClassHandleNameNotFound) {
        static_cast<void>(
            reporter->getInteractionClassHandle(failedInteractionClassName));
      } else if (transportationTypeHandleInvalidName) {
        static_cast<void>(reporter->getTransportationTypeHandle(
            failedTransportationTypeName));
      } else if (transportationTypeNameInvalidHandle) {
        static_cast<void>(
            reporter->getTransportationTypeName(failedTransportationType));
      } else if (transportationTypeNameSuccess) {
        REQUIRE(reporter->getTransportationTypeName(
                    successfulTransportationType) == L"HLAreliable");
      } else if (orderNameSuccess) {
        REQUIRE(reporter->getOrderName(rti1516_2025::OrderType::RECEIVE) == L"Receive");
      } else if (orderTypeSuccess) {
        REQUIRE(reporter->getOrderType(L"TimeStamp") ==
                rti1516_2025::OrderType::TIMESTAMP);
      } else if (orderNameInvalidType) {
        static_cast<void>(reporter->getOrderName(
            static_cast<rti1516_2025::OrderType>(0x7f)));
      } else if (objectClassNameSuccess) {
        REQUIRE(reporter->getObjectClassName(successfulObjectClass) ==
                L"HLAobjectRoot.Food.Drink");
      } else if (interactionClassNameSuccess) {
        REQUIRE(reporter->getInteractionClassName(successfulInteractionClass) ==
                L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
      } else if (objectClassNameInvalidHandle) {
        static_cast<void>(reporter->getObjectClassName(failedObjectClass));
      } else if (interactionClassNameInvalidHandle) {
        static_cast<void>(
            reporter->getInteractionClassName(failedInteractionClass));
      } else if (availableDimensionsForObjectClassSuccess) {
        REQUIRE(reporter->getAvailableDimensionsForObjectClass(
                    successfulObjectClass) ==
                rti1516_2025::DimensionHandleSet{successfulDimension});
      } else if (availableDimensionsForObjectClassLookup &&
                 !availableDimensionsForObjectClassSuccess) {
        static_cast<void>(
            reporter->getAvailableDimensionsForObjectClass(failedObjectClass));
      } else if (availableDimensionsForInteractionClassSuccess) {
        REQUIRE(reporter->getAvailableDimensionsForInteractionClass(
                    successfulInteractionClass) ==
                rti1516_2025::DimensionHandleSet{successfulDimension});
      } else if (dimensionNameSuccess) {
        REQUIRE(reporter->getDimensionName(successfulDimension) == L"ServerId");
      } else if (dimensionUpperBoundSuccess) {
        REQUIRE(reporter->getDimensionUpperBound(successfulDimension) == 25UL);
      } else if (sendInteractionSuccess) {
        std::array<std::uint8_t, 2U> encodedParameter{0x10U, 0x20U};
        ParameterHandleValueMap parameterValues;
        parameterValues.emplace(
            successfulSendInteractionParameter,
            VariableLengthData(encodedParameter.data(), encodedParameter.size()));
        std::array<std::uint8_t, 3U> encodedTag{0x50U, 0x55U, 0x42U};
        reporter->sendInteraction(
            successfulSendInteractionClass,
            parameterValues,
            VariableLengthData(encodedTag.data(), encodedTag.size()));
      } else if (sendInteractionWithRegionsSuccess) {
        std::array<std::uint8_t, 2U> encodedParameter{0x10U, 0x20U};
        ParameterHandleValueMap parameterValues;
        parameterValues.emplace(
            successfulSendInteractionParameter,
            VariableLengthData(encodedParameter.data(), encodedParameter.size()));
        std::array<std::uint8_t, 3U> encodedTag{0x50U, 0x55U, 0x42U};
        reporter->sendInteractionWithRegions(
            successfulSendInteractionClass,
            parameterValues,
            rti1516_2025::RegionHandleSet{successfulSendInteractionRegion},
            VariableLengthData(encodedTag.data(), encodedTag.size()));
      } else if (sendInteractionWithRegionsInvalidRegion) {
        std::array<std::uint8_t, 2U> encodedParameter{0x10U, 0x20U};
        ParameterHandleValueMap parameterValues;
        parameterValues.emplace(
            successfulSendInteractionParameter,
            VariableLengthData(encodedParameter.data(), encodedParameter.size()));
        std::array<std::uint8_t, 3U> encodedTag{0x50U, 0x55U, 0x42U};
        reporter->sendInteractionWithRegions(
            successfulSendInteractionClass,
            parameterValues,
            rti1516_2025::RegionHandleSet{rti1516_2025::RegionHandle{}},
            VariableLengthData(encodedTag.data(), encodedTag.size()));
      } else if (sendInteractionWithRegionsInvalidParameter) {
        std::array<std::uint8_t, 2U> encodedParameter{0x10U, 0x20U};
        ParameterHandleValueMap parameterValues;
        parameterValues.emplace(
            invalidSendInteractionParameter,
            VariableLengthData(encodedParameter.data(), encodedParameter.size()));
        std::array<std::uint8_t, 3U> encodedTag{0x50U, 0x55U, 0x42U};
        reporter->sendInteractionWithRegions(
            successfulSendInteractionClass,
            parameterValues,
            rti1516_2025::RegionHandleSet{successfulSendInteractionRegion},
            VariableLengthData(encodedTag.data(), encodedTag.size()));
      } else if (availableDimensionsForInteractionClassLookup) {
        static_cast<void>(reporter->getAvailableDimensionsForInteractionClass(
            failedInteractionClass));
      } else if (dimensionUpperBoundLookup) {
        static_cast<void>(reporter->getDimensionUpperBound(failedDimension));
      } else {
        static_cast<void>(reporter->getDimensionName(failedDimension));
      }
    };
    auto invokeAndVerifyService = [&] {
      if (orderNameSuccess || orderTypeSuccess || dimensionNameSuccess ||
          objectClassNameSuccess || interactionClassNameSuccess ||
          transportationTypeNameSuccess ||
          dimensionUpperBoundSuccess ||
          availableDimensionsForObjectClassSuccess ||
          availableDimensionsForInteractionClassSuccess ||
          sendInteractionServiceSuccess) {
        invokeService();
      } else if (sendInteractionWithRegionsInvalidRegion) {
        REQUIRE_THROWS_AS(invokeService(), rti1516_2025::InvalidRegion);
      } else if (sendInteractionWithRegionsInvalidParameter) {
        REQUIRE_THROWS_AS(
            invokeService(), rti1516_2025::InteractionParameterNotDefined);
      } else if (dimensionHandleNameNotFound || objectClassHandleNameNotFound ||
                 interactionClassHandleNameNotFound) {
        REQUIRE_THROWS_AS(invokeService(), rti1516_2025::NameNotFound);
      } else if (transportationTypeHandleInvalidName) {
        REQUIRE_THROWS_AS(
            invokeService(), rti1516_2025::InvalidTransportationName);
      } else if (transportationTypeNameInvalidHandle) {
        REQUIRE_THROWS_AS(
            invokeService(), rti1516_2025::InvalidTransportationTypeHandle);
      } else if (orderNameInvalidType) {
        REQUIRE_THROWS_AS(invokeService(), rti1516_2025::InvalidOrderType);
      } else if (objectClassNameInvalidHandle ||
                 (availableDimensionsForObjectClassLookup &&
                  !availableDimensionsForObjectClassSuccess)) {
        REQUIRE_THROWS_AS(
            invokeService(), rti1516_2025::InvalidObjectClassHandle);
      } else if (availableDimensionsForInteractionClassLookup ||
                 interactionClassNameInvalidHandle) {
        REQUIRE_THROWS_AS(
            invokeService(), rti1516_2025::InvalidInteractionClassHandle);
      } else {
        REQUIRE_THROWS_AS(
            invokeService(), rti1516_2025::InvalidDimensionHandle);
      }
    };
    invokeAndVerifyService();
    REQUIRE(reportFiles() == reporterInitialFile);

    REQUIRE(observer->connect(
                observerCallbacks, HLA_EVOKED, observerConfiguration)
                .addressUsed);
    auto const observerHandle = observer->joinFederationExecution(
        L"process-service-report-interaction-observer",
        L"process-service-report-interaction-type",
        federationName);
    REQUIRE(observerHandle.isValid());
    observerJoined = true;

    auto const reportClass = observer->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
    REQUIRE(reportClass.isValid());
    std::array<rti1516_2025::ParameterHandle, 8> reportParameters;
    std::array<wchar_t const*, 8> const parameterNames{
        L"HLAservice",
        L"HLAserviceType",
        L"HLAsuccessIndicator",
        L"HLAsuppliedArguments",
        L"HLAreturnedArgument",
        L"HLAexception",
        L"HLAserialNumber",
        L"HLAfederate",
    };
    for (std::size_t index = 0U; index < parameterNames.size(); ++index) {
      reportParameters[index] = observer->getParameterHandle(
          reportClass, parameterNames[index]);
      REQUIRE(reportParameters[index].isValid());
    }
    auto const reliableTransportation =
        observer->getTransportationTypeHandle(L"HLAreliable");
    REQUIRE(reliableTransportation.isValid());
    REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass, true));
    if (selectFileDestination) {
      REQUIRE_NOTHROW(reporter->setSendServiceReportsToFileSwitch(true));
    }
    REQUIRE(
        reporter->getSendServiceReportsToFileSwitch() ==
        selectFileDestination);
    if (selectFileDestination) {
      std::size_t drainedCallbacks = 0U;
      while (observer->evokeCallback(0.0)) {
        REQUIRE(drainedCallbacks < 16U);
        ++drainedCallbacks;
      }
    }

    auto const initialReportFiles = reportFiles();
    REQUIRE(initialReportFiles.size() == 2U);
    REQUIRE(std::all_of(
        initialReportFiles.begin(), initialReportFiles.end(),
        [](auto const& file) { return !file.second.empty(); }));
    auto const interactionCountBeforeService =
        observerCallbacks.receivedInteractionCount;

    invokeAndVerifyService();
    auto const reportFilesAfterService = reportFiles();
    if (selectFileDestination) {
      if (sendInteractionWithRegionsInvalidRegion) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "SendInteractionWithRegions",
            "InvalidRegion: Send Interaction With Regions requires valid RegionHandle values.");
        auto const reporterBefore = std::find_if(
            initialReportFiles.begin(), initialReportFiles.end(),
            [&](auto const& file) {
              return file.first == reporterInitialFile.front().first;
            });
        auto const reporterAfter = std::find_if(
            reportFilesAfterService.begin(), reportFilesAfterService.end(),
            [&](auto const& file) {
              return file.first == reporterInitialFile.front().first;
            });
        REQUIRE(reporterBefore != initialReportFiles.end());
        REQUIRE(reporterAfter != reportFilesAfterService.end());
        auto const appendedRecord = reporterAfter->second.substr(
            reporterBefore->second.size());
        constexpr std::string_view serialPrefix = R"({"HLAserialNumber":)";
        REQUIRE(appendedRecord.rfind(serialPrefix, 0U) == 0U);
        auto const serialEnd = appendedRecord.find(',', serialPrefix.size());
        REQUIRE(serialEnd != std::string::npos);
        auto const serial = std::to_string(static_cast<std::size_t>(
            std::stoull(appendedRecord.substr(
                serialPrefix.size(), serialEnd - serialPrefix.size()))));
        auto const invalidRegionText =
            toAscii(rti1516_2025::RegionHandle{}.toString());
        auto const expectedFailedRegionalFileRecord =
            std::string{R"({"HLAserialNumber":)"} + serial +
            R"(,"HLAreturnedArgument":[null],"HLAservice":"SendInteractionWithRegions","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
            successfulSendInteractionClassText +
            R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":{")" +
            successfulSendInteractionParameterText +
            R"(":"ECA="}},{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
            invalidRegionText +
            R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"UFVC"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRegion: Send Interaction With Regions requires valid RegionHandle values."})";
        REQUIRE(appendedRecord == expectedFailedRegionalFileRecord);
      } else if (sendInteractionWithRegionsInvalidParameter) {
        constexpr char const* invalidParameterException =
            "InteractionParameterNotDefined: Send Interaction With Regions requires defined ParameterHandle values.";
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "SendInteractionWithRegions",
            invalidParameterException);
        auto const reporterBefore = std::find_if(
            initialReportFiles.begin(), initialReportFiles.end(),
            [&](auto const& file) {
              return file.first == reporterInitialFile.front().first;
            });
        auto const reporterAfter = std::find_if(
            reportFilesAfterService.begin(), reportFilesAfterService.end(),
            [&](auto const& file) {
              return file.first == reporterInitialFile.front().first;
            });
        REQUIRE(reporterBefore != initialReportFiles.end());
        REQUIRE(reporterAfter != reportFilesAfterService.end());
        auto const appendedRecord = reporterAfter->second.substr(
            reporterBefore->second.size());
        constexpr std::string_view serialPrefix = R"({"HLAserialNumber":)";
        REQUIRE(appendedRecord.rfind(serialPrefix, 0U) == 0U);
        auto const serialEnd = appendedRecord.find(',', serialPrefix.size());
        REQUIRE(serialEnd != std::string::npos);
        auto const serial = std::to_string(static_cast<std::size_t>(
            std::stoull(appendedRecord.substr(
                serialPrefix.size(), serialEnd - serialPrefix.size()))));
        auto const expectedFailedRegionalFileRecord =
            std::string{R"({"HLAserialNumber":)"} + serial +
            R"(,"HLAreturnedArgument":[null],"HLAservice":"SendInteractionWithRegions","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
            successfulSendInteractionClassText +
            R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":{")" +
            failedSendInteractionParameterText +
            R"(":"ECA="}},{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
            successfulSendInteractionRegionText +
            R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"UFVC"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":false,"HLAexception":"InteractionParameterNotDefined: Send Interaction With Regions requires defined ParameterHandle values."})";
        REQUIRE(appendedRecord == expectedFailedRegionalFileRecord);
      } else if (interactionClassHandleNameNotFound) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetInteractionClassHandle",
            "NameNotFound: The supplied interaction class name is not defined in this federation execution.",
            R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"GetInteractionClassHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Interaction class name","HLAargumentValue":"MissingInteractionClass"}],"HLAsuccessIndicator":false,"HLAexception":"NameNotFound: The supplied interaction class name is not defined in this federation execution."})");
      } else if (transportationTypeHandleInvalidName) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetTransportationTypeHandle",
            "InvalidTransportationName: The supplied transportation type name is not declared in this federation execution.");
      } else if (transportationTypeNameInvalidHandle) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetTransportationTypeName",
            bindingInvalidHandle
                ? "InvalidTransportationTypeHandle: Get Transportation Type Name requires a valid TransportationTypeHandle."
                : "InvalidTransportationTypeHandle: The supplied TransportationTypeHandle is not declared in this federation execution.");
      } else if (orderNameInvalidType) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetOrderName",
            "InvalidOrderType: The supplied OrderType is not supported by this embedded profile.");
      } else if (objectClassNameInvalidHandle) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetObjectClassName",
            bindingInvalidHandle
                ? "InvalidObjectClassHandle: Get Object Class Name requires a valid ObjectClassHandle."
                : "InvalidObjectClassHandle: The supplied ObjectClassHandle is not known in this federation execution.");
      } else if (interactionClassNameInvalidHandle) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetInteractionClassName",
            bindingInvalidHandle
                ? "InvalidInteractionClassHandle: Get Interaction Class Name requires a valid InteractionClassHandle."
                : "InvalidInteractionClassHandle: The supplied InteractionClassHandle is not known in this federation execution.");
      } else if (dimensionUpperBoundLookup && !dimensionUpperBoundSuccess) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetDimensionUpperBound",
            bindingInvalidHandle
                ? "InvalidDimensionHandle: Get Dimension Upper Bound requires a valid DimensionHandle."
                : "InvalidDimensionHandle: The supplied DimensionHandle is not known in this federation execution.");
      } else if (dimensionHandleNameNotFound) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetDimensionHandle",
            "NameNotFound: The supplied dimension name is not defined in this federation execution.");
      } else if (
          operation == ProcessServiceReportInteractionOperation::GetDimensionName &&
          !dimensionNameSuccess) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetDimensionName",
            bindingInvalidHandle
                ? "InvalidDimensionHandle: Get Dimension Name requires a valid DimensionHandle."
                : "InvalidDimensionHandle: The supplied DimensionHandle is not known in this federation execution.");
      } else if (availableDimensionsForObjectClassLookup &&
                 !availableDimensionsForObjectClassSuccess) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetAvailableDimensionsForObjectClass",
            bindingInvalidHandle
                ? "InvalidObjectClassHandle: Get Available Dimensions for Object Class requires a valid ObjectClassHandle."
                : "InvalidObjectClassHandle: The supplied ObjectClassHandle is not known in this federation execution.");
      } else if (availableDimensionsForInteractionClassLookup &&
                 !availableDimensionsForInteractionClassSuccess) {
        verifySingleFailedFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first,
            "GetAvailableDimensionsForInteractionClass",
            bindingInvalidHandle
                ? "InvalidInteractionClassHandle: Get Available Dimensions for Interaction Class requires a valid InteractionClassHandle."
                : "InvalidInteractionClassHandle: The supplied InteractionClassHandle is not known in this federation execution.");
      } else {
        verifySingleSuccessfulFileAppend(
            initialReportFiles,
            reportFilesAfterService,
            reporterInitialFile.front().first);
      }
    } else {
      REQUIRE(reportFilesAfterService == initialReportFiles);
    }

    if (selectFileDestination) {
      std::size_t drainedCallbacks = 0U;
      while (observer->evokeCallback(0.0)) {
        REQUIRE(drainedCallbacks < 16U);
        ++drainedCallbacks;
      }
      REQUIRE(
          observerCallbacks.receivedInteractionCount ==
          interactionCountBeforeService);
    } else {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    if (selectFileDestination) {
      REQUIRE(observerCallbacks.receivedInteractionCount ==
              interactionCountBeforeService);
    } else {
    REQUIRE(observerCallbacks.receivedInteractionCount == 1U);
    REQUIRE(observerCallbacks.receivedInteractionClass == reportClass);
    REQUIRE(observerCallbacks.receivedParameterValues.size() == 8U);
    REQUIRE(observerCallbacks.receivedTag.size() == 0U);
    REQUIRE(observerCallbacks.receivedTransportationType == reliableTransportation);
    REQUIRE_FALSE(observerCallbacks.receivedProducingFederate.isValid());
    REQUIRE_FALSE(observerCallbacks.receivedSentRegions);

    auto const& values = observerCallbacks.receivedParameterValues;
    rti1516_2025::HLAunicodeString serviceName;
    REQUIRE_NOTHROW(serviceName.decode(values.at(reportParameters[0])));
    wchar_t const* expectedServiceName = nullptr;
    switch (operation) {
      case ProcessServiceReportInteractionOperation::GetDimensionHandleNameNotFound:
        expectedServiceName = L"GetDimensionHandle";
        break;
      case ProcessServiceReportInteractionOperation::GetObjectClassHandleNameNotFound:
        expectedServiceName = L"GetObjectClassHandle";
        break;
      case ProcessServiceReportInteractionOperation::GetInteractionClassHandleNameNotFound:
        expectedServiceName = L"GetInteractionClassHandle";
        break;
      case ProcessServiceReportInteractionOperation::GetTransportationTypeHandleInvalidName:
        expectedServiceName = L"GetTransportationTypeHandle";
        break;
      case ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle:
      case ProcessServiceReportInteractionOperation::GetTransportationTypeNameSuccess:
        expectedServiceName = L"GetTransportationTypeName";
        break;
      case ProcessServiceReportInteractionOperation::GetOrderNameSuccess:
        expectedServiceName = L"GetOrderName";
        break;
      case ProcessServiceReportInteractionOperation::GetOrderTypeSuccess:
        expectedServiceName = L"GetOrderType";
        break;
      case ProcessServiceReportInteractionOperation::GetOrderNameInvalidType:
        expectedServiceName = L"GetOrderName";
        break;
      case ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle:
      case ProcessServiceReportInteractionOperation::GetObjectClassNameSuccess:
        expectedServiceName = L"GetObjectClassName";
        break;
      case ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle:
      case ProcessServiceReportInteractionOperation::GetInteractionClassNameSuccess:
        expectedServiceName = L"GetInteractionClassName";
        break;
      case ProcessServiceReportInteractionOperation::GetDimensionName:
        expectedServiceName = L"GetDimensionName";
        break;
      case ProcessServiceReportInteractionOperation::GetDimensionUpperBound:
        expectedServiceName = L"GetDimensionUpperBound";
        break;
      case ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass:
        expectedServiceName = L"GetAvailableDimensionsForObjectClass";
        break;
      case ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass:
        expectedServiceName = L"GetAvailableDimensionsForInteractionClass";
        break;
      case ProcessServiceReportInteractionOperation::SendInteractionSuccess:
        expectedServiceName = L"SendInteraction";
        break;
      case ProcessServiceReportInteractionOperation::SendInteractionWithRegionsSuccess:
      case ProcessServiceReportInteractionOperation::SendInteractionWithRegionsInvalidRegion:
      case ProcessServiceReportInteractionOperation::SendInteractionWithRegionsInvalidParameter:
        expectedServiceName = L"SendInteractionWithRegions";
        break;
    }
    REQUIRE(serviceName.get() == expectedServiceName);
    rti1516_2025::HLAinteger16BE serviceType;
    REQUIRE_NOTHROW(serviceType.decode(values.at(reportParameters[1])));
    REQUIRE(serviceType.get() == (sendInteractionService ? 2 : 6));
    rti1516_2025::HLAboolean success;
    REQUIRE_NOTHROW(success.decode(values.at(reportParameters[2])));
    REQUIRE(success.get() ==
            (orderNameSuccess || orderTypeSuccess || dimensionNameSuccess ||
             objectClassNameSuccess || interactionClassNameSuccess ||
             transportationTypeNameSuccess ||
             dimensionUpperBoundSuccess ||
             availableDimensionsForObjectClassSuccess ||
             availableDimensionsForInteractionClassSuccess ||
             sendInteractionServiceSuccess));

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(values.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() ==
            (sendInteractionSuccess
                 ? 4U
                 : (sendInteractionWithRegionsSuccess ||
                    sendInteractionWithRegionsInvalidRegion ||
                    sendInteractionWithRegionsInvalidParameter) ? 5U : 1U));
    auto const& suppliedArgument =
        dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
            suppliedArguments.get(0U));
    if (dimensionHandleNameNotFound) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Dimension name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() == L"\"MissingDimension\"");
    } else if (objectClassHandleNameNotFound) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Object class name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"MissingObjectClass\"");
    } else if (interactionClassHandleNameNotFound) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Interaction class name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"MissingInteractionClass\"");
    } else if (transportationTypeHandleInvalidName) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Transportation type name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"MissingTransportation\"");
    } else if (transportationTypeNameInvalidHandle ||
               transportationTypeNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 59);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Transportation type handle");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"" +
                  (transportationTypeNameSuccess
                       ? successfulTransportationType.toString()
                       : failedTransportationType.toString()) +
                  L"\"");
    } else if (orderNameSuccess || orderNameInvalidType) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 38);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Order type");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              (orderNameSuccess ? L"\"RECEIVE\"" : L"\"UNSUPPORTED\""));
    } else if (orderTypeSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Order name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() == L"\"TimeStamp\"");
    } else if (availableDimensionsForObjectClassLookup ||
               objectClassNameInvalidHandle || objectClassNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 36);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Object class handle");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"" +
                  ((availableDimensionsForObjectClassSuccess ||
                    objectClassNameSuccess)
                       ? successfulObjectClass.toString()
                       : failedObjectClass.toString()) +
                  L"\"");
    } else if (availableDimensionsForInteractionClassLookup ||
               interactionClassNameInvalidHandle ||
               interactionClassNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 27);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Interaction class handle");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"" +
                  ((availableDimensionsForInteractionClassSuccess ||
                    interactionClassNameSuccess)
                       ? successfulInteractionClass.toString()
                       : failedInteractionClass.toString()) +
                  L"\"");
    } else if (sendInteractionService) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 27);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Interaction class designator");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"" + successfulSendInteractionClass.toString() + L"\"");
    } else if (dimensionNameSuccess || dimensionUpperBoundSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 10);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Dimension handle");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"" + successfulDimension.toString() + L"\"");
    } else {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  suppliedArgument.get(0U)).get() == 10);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(1U)).get() == L"Dimension handle");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  suppliedArgument.get(2U)).get() ==
              L"\"" + failedDimension.toString() + L"\"");
    }
    if (sendInteractionService) {
      auto const& parameterMapArgument =
          dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
              suppliedArguments.get(1U));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  parameterMapArgument.get(0U)).get() == 40);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  parameterMapArgument.get(1U)).get() ==
              L"Constrained set of interaction parameter designator and value pairs");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  parameterMapArgument.get(2U)).get() ==
              L"{\"" + (sendInteractionWithRegionsInvalidParameter
                            ? invalidSendInteractionParameter.toString()
                            : successfulSendInteractionParameter.toString()) +
                  L"\":\"ECA=\"}");

      if (regionalSend) {
        auto const& regionSetArgument =
            dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
                suppliedArguments.get(2U));
        REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                    regionSetArgument.get(0U)).get() == 43);
        REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                    regionSetArgument.get(1U)).get() ==
                L"Set of region designators");
        REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                    regionSetArgument.get(2U)).get() ==
                (sendInteractionWithRegionsSuccess ||
                         sendInteractionWithRegionsInvalidParameter
                     ? L"[\"" + successfulSendInteractionRegion.toString() + L"\"]"
                     : L"[\"" + rti1516_2025::RegionHandle{}.toString() + L"\"]"));
      }
      auto const tagIndex = regionalSend ? 3U : 2U;
      auto const timestampIndex = regionalSend ? 4U : 3U;
      auto const& tagArgument = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(tagIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  tagArgument.get(0U)).get() == 60);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  tagArgument.get(1U)).get() == L"User-supplied tag");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  tagArgument.get(2U)).get() == L"\"UFVC\"");

      auto const& timestampArgument =
          dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
              suppliedArguments.get(timestampIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  timestampArgument.get(0U)).get() == 34);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  timestampArgument.get(1U)).get() == L"Optional timestamp");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  timestampArgument.get(2U)).get() == L"null");
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(returnedArgument.decode(values.at(reportParameters[4])));
    if (transportationTypeNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() == L"Transportation type name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() == L"\"HLAreliable\"");
    } else if (orderNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() == L"Order name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() == L"\"Receive\"");
    } else if (orderTypeSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 38);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() == L"Order type");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() == L"\"TIMESTAMP\"");
    } else if (objectClassNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() == L"Object class name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() ==
              L"\"HLAobjectRoot.Food.Drink\"");
    } else if (interactionClassNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() == L"Interaction class name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() ==
              L"\"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed\"");
    } else if (dimensionUpperBoundSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 35);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() == L"Dimension upper bound");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() == L"25");
    } else if (dimensionNameSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 53);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() == L"Dimension name");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() == L"\"ServerId\"");
    } else if (availableDimensionsForObjectClassSuccess ||
               availableDimensionsForInteractionClassSuccess) {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 11);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get() ==
              L"A set of dimension handles");
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() ==
              L"[\"" + successfulDimension.toString() + L"\"]");
    } else {
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
                  returnedArgument.get(0U)).get() == 34);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(1U)).get().empty());
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
                  returnedArgument.get(2U)).get() == L"null");
    }

    rti1516_2025::HLAunicodeString exception;
    REQUIRE_NOTHROW(exception.decode(values.at(reportParameters[5])));
    wchar_t const* expectedException = nullptr;
    switch (operation) {
      case ProcessServiceReportInteractionOperation::GetDimensionHandleNameNotFound:
        expectedException =
            L"NameNotFound: The supplied dimension name is not defined in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetObjectClassHandleNameNotFound:
        expectedException =
            L"NameNotFound: The supplied object class name is not defined in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetInteractionClassHandleNameNotFound:
        expectedException =
            L"NameNotFound: The supplied interaction class name is not defined in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetTransportationTypeHandleInvalidName:
        expectedException =
            L"InvalidTransportationName: The supplied transportation type name is not declared in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle:
        expectedException = bindingInvalidHandle
                                ? L"InvalidTransportationTypeHandle: Get Transportation Type Name requires a valid TransportationTypeHandle."
                                : L"InvalidTransportationTypeHandle: The supplied TransportationTypeHandle is not declared in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetTransportationTypeNameSuccess:
        expectedException = L"";
        break;
      case ProcessServiceReportInteractionOperation::GetOrderNameSuccess:
        expectedException = L"";
        break;
      case ProcessServiceReportInteractionOperation::GetOrderTypeSuccess:
        expectedException = L"";
        break;
      case ProcessServiceReportInteractionOperation::GetOrderNameInvalidType:
        expectedException =
            L"InvalidOrderType: The supplied OrderType is not supported by this embedded profile.";
        break;
      case ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle:
        expectedException = bindingInvalidHandle
                                ? L"InvalidObjectClassHandle: Get Object Class Name requires a valid ObjectClassHandle."
                                : L"InvalidObjectClassHandle: The supplied ObjectClassHandle is not known in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetObjectClassNameSuccess:
        expectedException = L"";
        break;
      case ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle:
        expectedException = bindingInvalidHandle
                                ? L"InvalidInteractionClassHandle: Get Interaction Class Name requires a valid InteractionClassHandle."
                                : L"InvalidInteractionClassHandle: The supplied InteractionClassHandle is not known in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetInteractionClassNameSuccess:
        expectedException = L"";
        break;
      case ProcessServiceReportInteractionOperation::GetDimensionName:
        expectedException = dimensionNameSuccess
                                ? L""
                                : bindingInvalidHandle
                                ? L"InvalidDimensionHandle: Get Dimension Name requires a valid DimensionHandle."
                                : L"InvalidDimensionHandle: The supplied DimensionHandle is not known in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetDimensionUpperBound:
        expectedException = dimensionUpperBoundSuccess
                                ? L""
                                : bindingInvalidHandle
                                ? L"InvalidDimensionHandle: Get Dimension Upper Bound requires a valid DimensionHandle."
                                : L"InvalidDimensionHandle: The supplied DimensionHandle is not known in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass:
        expectedException = availableDimensionsForObjectClassSuccess
                                ? L""
                                : bindingInvalidHandle
                                ? L"InvalidObjectClassHandle: Get Available Dimensions for Object Class requires a valid ObjectClassHandle."
                                : L"InvalidObjectClassHandle: The supplied ObjectClassHandle is not known in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass:
        expectedException = availableDimensionsForInteractionClassSuccess
                                ? L""
                                : bindingInvalidHandle
                                ? L"InvalidInteractionClassHandle: Get Available Dimensions for Interaction Class requires a valid InteractionClassHandle."
                                : L"InvalidInteractionClassHandle: The supplied InteractionClassHandle is not known in this federation execution.";
        break;
      case ProcessServiceReportInteractionOperation::SendInteractionSuccess:
      case ProcessServiceReportInteractionOperation::SendInteractionWithRegionsSuccess:
        expectedException = L"";
        break;
      case ProcessServiceReportInteractionOperation::SendInteractionWithRegionsInvalidRegion:
        expectedException =
            L"InvalidRegion: Send Interaction With Regions requires valid RegionHandle values.";
        break;
      case ProcessServiceReportInteractionOperation::SendInteractionWithRegionsInvalidParameter:
        expectedException =
            L"InteractionParameterNotDefined: Send Interaction With Regions requires defined ParameterHandle values.";
        break;
    }
    REQUIRE(exception.get() == expectedException);
    rti1516_2025::HLAinteger32BE serialNumber;
    REQUIRE_NOTHROW(serialNumber.decode(values.at(reportParameters[6])));
    REQUIRE(serialNumber.get() == (selectFileDestination ? 1 : 0));
    auto const expectedReporterHandle = reporterHandle.encode();
    auto const& encodedReporterHandle = values.at(reportParameters[7]);
    REQUIRE(encodedReporterHandle.size() == expectedReporterHandle.size());
    auto const* expectedHandleBytes =
        static_cast<std::uint8_t const*>(expectedReporterHandle.data());
    auto const* encodedHandleBytes =
        static_cast<std::uint8_t const*>(encodedReporterHandle.data());
    REQUIRE(std::equal(
        expectedHandleBytes,
        expectedHandleBytes + expectedReporterHandle.size(),
        encodedHandleBytes));
    }
    REQUIRE(reportFiles() == reportFilesAfterService);

    REQUIRE_NOTHROW(reporter->resignFederationExecution(NO_ACTION));
    reporterJoined = false;
    REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
    REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
    observerJoined = false;
    REQUIRE_NOTHROW(reporter->disconnect());
    REQUIRE_NOTHROW(observer->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (observerJoined) {
      try {
        observer->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (reporterJoined) {
      try {
        reporter->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      observer->disconnect();
    } catch (...) {
    }
    try {
      reporter->disconnect();
    } catch (...) {
    }
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(reporterJoined);
  REQUIRE_FALSE(observerJoined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

}  // namespace
