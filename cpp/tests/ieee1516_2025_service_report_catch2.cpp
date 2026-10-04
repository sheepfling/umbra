#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "internal/federation/embedded_transport.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_save_commit_store.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "hla_test_names.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

class ServiceReportFederationEventAmbassador final : public NullFederateAmbassador {
 public:
  void connectionLost(std::wstring const& faultDescription) override {
    faultDescriptions.push_back(faultDescription);
  }

  void federateResigned(std::wstring const& reasonForResignDescription) override {
    resignationDescriptions.push_back(reasonForResignDescription);
  }

  std::vector<std::wstring> faultDescriptions;
  std::vector<std::wstring> resignationDescriptions;
};

class ServiceReportFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct TimeAdvanceGrantReport {
    std::wstring implementationName;
    std::wstring value;
  };

  struct InteractionReport {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct TimestampedInteractionReport {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct RequestRetractionReport {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  struct AttributeTransportationTypeChangeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    TransportationTypeHandle transportationType;
  };

  struct AttributeTransportationTypeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandle attribute;
    TransportationTypeHandle transportationType;
  };

  struct InteractionTransportationTypeChangeReport {
    InteractionClassHandle interactionClass;
    TransportationTypeHandle transportationType;
  };

  struct InteractionTransportationTypeReport {
    FederateHandle federate;
    InteractionClassHandle interactionClass;
    TransportationTypeHandle transportationType;
  };

  struct FlushQueueGrantReport {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  void federateResigned(std::wstring const& reasonForResignDescription) override {
    resignationDescriptions.push_back(reasonForResignDescription);
  }

  void connectionLost(std::wstring const& faultDescription) override {
    faultDescriptions.push_back(faultDescription);
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
  }

  void flushQueueGrant(
      LogicalTime const& time,
      LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
        optionalSentRegions != nullptr,
        optionalSentRegions != nullptr ? *optionalSentRegions : RegionHandleSet{},
    });
  }

  void requestRetraction(MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({
        retraction.isValid(),
        retraction.encode(),
    });
  }

  void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      TransportationTypeHandle const& transportationType) override {
    attributeTransportationTypeChangeReports.push_back({
        objectInstance,
        attributes,
        transportationType,
    });
  }

  void reportAttributeTransportationType(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute,
      TransportationTypeHandle const& transportationType) override {
    attributeTransportationTypeReports.push_back({
        objectInstance,
        attribute,
        transportationType,
    });
  }

  void confirmInteractionTransportationTypeChange(
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    interactionTransportationTypeChangeReports.push_back({
        interactionClass,
        transportationType,
    });
  }

  void reportInteractionTransportationType(
      FederateHandle const& federate,
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    interactionTransportationTypeReports.push_back({
        federate,
        interactionClass,
        transportationType,
    });
  }

  std::vector<std::wstring> resignationDescriptions;
  std::vector<std::wstring> faultDescriptions;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
  std::vector<TimeAdvanceGrantReport> timeRegulationEnabledReports;
  std::vector<InteractionReport> interactionReports;
  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<AttributeTransportationTypeChangeReport>
      attributeTransportationTypeChangeReports;
  std::vector<AttributeTransportationTypeReport> attributeTransportationTypeReports;
  std::vector<InteractionTransportationTypeChangeReport>
      interactionTransportationTypeChangeReports;
  std::vector<InteractionTransportationTypeReport> interactionTransportationTypeReports;
};

TEST_CASE(
    "Embedded service reporting records the final Resign Federation Execution invocation",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][resign-federation-execution]"
    "[rti.service.resign-federation-execution]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"resign-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // First advance the joined-federate audit sequence through an ordinary
  // selected-file service. A rejected action then has no accepted invocation
  // to report. The successful voluntary resignation must consume the final
  // next serial before membership and its writer are released, so no later
  // invocation can append to this completed lifetime file.
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  auto const expectedSwitch =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch);

  REQUIRE_THROWS_AS(
      rti->resignFederationExecution(static_cast<ResignAction>(99)),
      rti1516_2025::InvalidResignAction);
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  auto const expectedResign =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ResignFederationExecution","HLAsuppliedArguments":[{"HLAargumentType":44,"HLAargumentName":"HLAresignAction","HLAargumentValue":"NO_ACTION"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch + expectedResign);
  REQUIRE_THROWS_AS(
      rti->resignFederationExecution(NO_ACTION),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch + expectedResign);

  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting records Federate Resigned before its callback",
    "[integration][development-profile][federation-management][transport][mom]"
    "[service-report-file][service-reporting][federate-resigned-service-report]"
    "[federate.callback.federate-resigned]") {
  ServiceReportFederationEventAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"forced-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Advance the per-joined-federate sequence once, then use the RTI-originated
  // §4.13 control seam. Its report must become durable before the separately
  // queued Federate Resigned callback and before the writer is released.
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  auto const expectedSwitch =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch);

  auto const reason = std::wstring{L"embedded session control"};
  REQUIRE(umbra::detail::forceEmbeddedFederateResignationForTesting(*rti, reason));
  auto const expectedResigned =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"FederateResigned","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Reason for resigning","HLAargumentValue":"embedded session control"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch + expectedResigned);
  REQUIRE(reports.resignationDescriptions.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.resignationDescriptions == std::vector<std::wstring>{reason});

  REQUIRE_FALSE(umbra::detail::forceEmbeddedFederateResignationForTesting(
      *rti,
      L"duplicate RTI membership control"));
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch + expectedResigned);
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers Federate Resigned through MOM interaction",
    "[integration][development-profile][federation-management][transport][mom]"
    "[service-reporting][service-report-interaction][federate-resigned-service-report]"
    "[rti.service.federate-resigned][federate.callback.federate-resigned]") {
  ServiceReportFederationEventAmbassador subjectReports;
  ServiceReportFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"forced-mom-subject",
      L"subject",
      federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"forced-mom-observer",
      L"observer",
      federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }

  // Enable the subject's public service-report route before the observer
  // subscribes so the setter itself cannot consume the first report serial.
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const reason = std::wstring{L"embedded MOM session control"};
  REQUIRE(umbra::detail::forceEmbeddedFederateResignationForTesting(*subject, reason));

  // The observer is immediate while the forced subject is evoked. The public
  // report must therefore be visible before the subject's Federate Resigned
  // callback is allowed to enter user code.
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(subjectReports.resignationDescriptions.empty());
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(decodedService.get() == L"FederateResigned");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 0);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord suppliedPrototype;
  suppliedPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{suppliedPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 1U);
  auto const& reasonArgument = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(0U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(reasonArgument.get(0U)).get() ==
          53);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(reasonArgument.get(1U)).get() ==
          L"Reason for resigning");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(reasonArgument.get(2U)).get() ==
          L"\"" + reason + L"\"");

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(
      returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
          34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
          L"null");

  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE(subjectReports.resignationDescriptions == std::vector<std::wstring>{reason});
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

TEST_CASE(
    "Embedded service reporting records Connection Lost before its callback",
    "[integration][development-profile][federation-management][transport][mom]"
    "[service-report-file][service-reporting][connection-lost-service-report]"
    "[rti.service.connection-lost]"
    "[federate.callback.connection-lost]") {
  ServiceReportFederationEventAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"fault-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Advance the per-joined-federate sequence once, then force the distinct
  // RTI-initiated §4.4 service. The fault record must become durable before
  // the best-effort callback is queued and before its writer is released.
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  auto const expectedSwitch =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch);

  auto const fault = std::wstring{L"embedded transport fault"};
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(*rti, fault));
  auto const expectedConnectionLost =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ConnectionLost","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Fault description","HLAargumentValue":"embedded transport fault"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch + expectedConnectionLost);
  REQUIRE(reports.faultDescriptions.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.faultDescriptions == std::vector<std::wstring>{fault});

  REQUIRE_FALSE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *rti,
      L"duplicate transport fault"));
  REQUIRE(readTextFile(reportFile) == initialText + expectedSwitch + expectedConnectionLost);
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting records an accepted deferred Modify Lookahead request",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][time-management][time-role][modify-lookahead]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.modify-lookahead]"
    "[federate.callback.time-regulation-enabled]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"modify-lookahead-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const initialText = readTextFile(files.front());

  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)));
  auto const expectedEnable =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"EnableTimeRegulation","HLAsuppliedArguments":[{"HLAargumentType":32,"HLAargumentName":"Lookahead","HLAargumentValue":"3"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedEnable);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);

  // A decrease is accepted and reported now, while §8.20 keeps the actual
  // lookahead unchanged until later time advancement.
  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  auto const expectedModify =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ModifyLookahead","HLAsuppliedArguments":[{"HLAargumentType":32,"HLAargumentName":"Requested lookahead","HLAargumentValue":"1"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedEnable + expectedModify);
  rti1516_2025::HLAinteger64Interval actualLookahead;
  REQUIRE_NOTHROW(rti->queryLookahead(actualLookahead));
  REQUIRE(actualLookahead.getInterval() == 3);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting records an accepted Time Advance Request before its grant",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][time-management][time-advance-request]"
    "[rti.service.time-advance-request]"
    "[rti.service.query-logical-time]"
    "[federate.callback.time-advance-grant]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"time-advance-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const initialText = readTextFile(files.front());

  // §8.8 accepts the requested time before the later Time Advance Grant
  // completes the advance. The selected report file must record that accepted
  // invocation without treating the grant as part of the request result.
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  auto const expectedRequest =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"TimeAdvanceRequest","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"7"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedRequest);
  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.isInitial());
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 7);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting records an accepted Time Advance Request Available before its grant",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][time-management][time-advance-request-available]"
    "[rti.service.time-advance-request-available]"
    "[rti.service.query-logical-time]"
    "[federate.callback.time-advance-grant]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"time-advance-available-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const initialText = readTextFile(files.front());

  // §8.9 accepts the Available-form request before the later Time Advance
  // Grant completes it. The request report remains distinct from the grant.
  REQUIRE_NOTHROW(
      rti->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(7)));
  auto const expectedRequest =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"TimeAdvanceRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"7"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedRequest);
  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.isInitial());
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 7);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves the supplied Next Message Request boundary",
    "[integration][development-profile][federation-management][interaction-management][mom]"
    "[service-report-file][service-reporting][time-management][next-message-request]"
    "[rti.service.next-message-request][rti.service.send-interaction]"
    "[rti.service.time-advance-request][rti.service.query-logical-time]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ServiceReportFederateAmbassador publisherReports;
  ServiceReportFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto receiverConfiguration = configurationForServiceReportDirectory(directory.path());
  receiverConfiguration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {0x4E, 0x4D, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED, receiverConfiguration));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmr-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmr-report-receiver", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const receiverReportFile = files.front();

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xC1, 0xD2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const handle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(handle.isValid());
  auto const textBeforeRequest = readTextFile(receiverReportFile);

  // §8.10 supplies 10 even though the queued TSO message at 7 determines the
  // later grant. The service report must preserve the supplied argument, not
  // manufacture a report from the implementation's effective target.
  REQUIRE_NOTHROW(receiver->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  auto const expectedRequest =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"NextMessageRequest","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"10"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(receiverReportFile) == textBeforeRequest + expectedRequest);
  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);
  REQUIRE(readTextFile(receiverReportFile) == textBeforeRequest + expectedRequest);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Change Interaction Order Type arguments",
    "[integration][development-profile][federation-management][interaction-management][mom]"
    "[service-report-file][service-reporting][time-management][change-interaction-order-type]"
    "[rti.service.publish-interaction-class][rti.service.change-interaction-order-type]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"interaction-order-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  // An unsuccessful §8.26 invocation has no successful-void report record.
  REQUIRE_THROWS_AS(
      rti->changeInteractionOrderType(interactionClass, TIMESTAMP),
      rti1516_2025::InteractionClassNotPublished);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(interactionClass, TIMESTAMP));
  auto const handleText = interactionClass.toString();
  std::string handleValue;
  handleValue.reserve(handleText.size());
  for (wchar_t const character : handleText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    handleValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ChangeInteractionOrderType","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      handleValue +
      R"("},{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"TIMESTAMP"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  auto const expectedPublishRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"PublishInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      handleValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded order and transportation MOM reports retain their service classifications",
    "[integration][development-profile][federation-management][declaration-management]"
    "[object-management][interaction-management][transportation-management][mom]"
    "[service-reporting][service-report-interaction]"
    "[rti.service.change-attribute-order-type]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.change-default-attribute-transportation-type]") {
  ServiceReportFederateAmbassador senderReports;
  ServiceReportFederateAmbassador observerReports;
  auto sender = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"order-classification-sender", L"sender", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"order-classification-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::exception,
       }) {
    reportParameters.push_back(observer->getParameterHandle(reportClass, name));
    REQUIRE(reportParameters.back().isValid());
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(sender->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(sender->setSendServiceReportsToFileSwitch(false));

  auto const objectClass = sender->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute = sender->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(objectClass, attributes));
  auto const objectInstance = sender->registerObjectInstance(objectClass);
  auto const interactionClass =
      sender->getInteractionClassHandle(fixture_hla::fom::server_take_order);
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  auto const reliable = sender->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(objectInstance.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(sender->setServiceReportingSwitch(true));

  REQUIRE_NOTHROW(sender->changeAttributeOrderType(objectInstance, attributes, TIMESTAMP));
  REQUIRE_NOTHROW(sender->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(sender->changeDefaultAttributeTransportationType(
      objectClass, attributes, reliable));

  std::vector<std::pair<std::wstring, std::int32_t>> const expected{
      {L"ChangeAttributeOrderType", 1},
      {L"ChangeDefaultAttributeOrderType", 1},
      {L"ChangeInteractionOrderType", 1},
      {L"ChangeDefaultAttributeTransportationType", 2},
  };
  REQUIRE(observerReports.interactionReports.size() == expected.size());
  for (std::size_t index = 0; index < expected.size(); ++index) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    rti1516_2025::HLAunicodeString service;
    REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(service.get() == expected[index].first);
    rti1516_2025::HLAinteger16BE serviceType;
    REQUIRE_NOTHROW(serviceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(serviceType.get() == expected[index].second);
    rti1516_2025::HLAboolean success;
    REQUIRE_NOTHROW(success.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(success.get());
    rti1516_2025::HLAunicodeString exception;
    REQUIRE_NOTHROW(exception.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(exception.get().empty());
  }

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Request Interaction Transportation Type Change arguments",
    "[integration][development-profile][federation-management][interaction-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[service-report-request-interaction-transportation-type-change]"
    "[request-interaction-transportation-type-change]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[federate.callback.confirm-interaction-transportation-type-change]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"interaction-transport-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const bestEffort = rti->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(interactionClass.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  // An unsuccessful §6.30 invocation has no successful-void report record.
  REQUIRE_THROWS_AS(
      rti->requestInteractionTransportationTypeChange(interactionClass, bestEffort),
      rti1516_2025::InteractionClassNotPublished);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(
      rti->requestInteractionTransportationTypeChange(interactionClass, bestEffort));
  auto const interactionClassText = interactionClass.toString();
  auto const transportationText = bestEffort.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  std::string transportationValue;
  transportationValue.reserve(transportationText.size());
  for (wchar_t const character : transportationText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    transportationValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"RequestInteractionTransportationTypeChange","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":59,"HLAargumentName":"Transportation type","HLAargumentValue":")" +
      transportationValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  auto const expectedPublishRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"PublishInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The report describes accepted request invocation. The preferred
  // transportation does not change until the separately queued confirmation.
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord + expectedRecord);
  REQUIRE(reports.interactionTransportationTypeChangeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.interactionTransportationTypeChangeReports.size() == 1U);
  REQUIRE(reports.interactionTransportationTypeChangeReports.front().interactionClass ==
          interactionClass);
  REQUIRE(reports.interactionTransportationTypeChangeReports.front().transportationType ==
          bestEffort);
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Request Attribute Transportation Type Change arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[request-attribute-transportation-type-change]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[federate.callback.confirm-attribute-transportation-type-change]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"attribute-transport-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{efficiency};
  auto const bestEffort = rti->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(objectClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §6.25 invocation has no successful-void report record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      rti->requestAttributeTransportationTypeChange(unknownObject, attributes, bestEffort),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  auto const textBeforeRequest = readTextFile(reportFile);
  ObjectInstanceHandle objectInstance;
  // Keep setup registration outside this focused request-service report lane.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(
      rti->requestAttributeTransportationTypeChange(objectInstance, attributes, bestEffort));

  auto const objectInstanceText = objectInstance.toString();
  auto const attributeText = efficiency.toString();
  auto const transportationText = bestEffort.toString();
  std::string objectInstanceValue;
  objectInstanceValue.reserve(objectInstanceText.size());
  for (wchar_t const character : objectInstanceText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectInstanceValue.push_back(static_cast<char>(character));
  }
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }
  std::string transportationValue;
  transportationValue.reserve(transportationText.size());
  for (wchar_t const character : transportationText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    transportationValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"RequestAttributeTransportationTypeChange","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":59,"HLAargumentName":"Transportation type","HLAargumentValue":")" +
      transportationValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The report describes an accepted request. The preferred transportation
  // changes only with the separately queued confirmation callback.
  REQUIRE(readTextFile(reportFile) == textBeforeRequest + expectedRecord);
  REQUIRE(reports.attributeTransportationTypeChangeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.attributeTransportationTypeChangeReports.size() == 1U);
  REQUIRE(reports.attributeTransportationTypeChangeReports.front().objectInstance ==
          objectInstance);
  REQUIRE(reports.attributeTransportationTypeChangeReports.front().attributes == attributes);
  REQUIRE(reports.attributeTransportationTypeChangeReports.front().transportationType ==
          bestEffort);
  REQUIRE(readTextFile(reportFile) == textBeforeRequest + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Change Default Attribute Transportation Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[service-report-change-default-attribute-transportation-type]"
    "[change-default-attribute-transportation-type]"
    "[rti.service.change-default-attribute-transportation-type]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"default-attribute-transport-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{efficiency};
  auto const bestEffort = rti->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(objectClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §6.27 invocation has no successful-void report record.
  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      rti->changeDefaultAttributeTransportationType(
          unknownObjectClass, attributes, bestEffort),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(
      rti->changeDefaultAttributeTransportationType(objectClass, attributes, bestEffort));

  auto const objectClassText = objectClass.toString();
  auto const attributeText = efficiency.toString();
  auto const transportationText = bestEffort.toString();
  std::string objectClassValue;
  objectClassValue.reserve(objectClassText.size());
  for (wchar_t const character : objectClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectClassValue.push_back(static_cast<char>(character));
  }
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }
  std::string transportationValue;
  transportationValue.reserve(transportationText.size());
  for (wchar_t const character : transportationText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    transportationValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ChangeDefaultAttributeTransportationType","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":59,"HLAargumentName":"Transportation type","HLAargumentValue":")" +
      transportationValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Query Attribute Transportation Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[query-attribute-transportation-type]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.query-attribute-transportation-type]"
    "[federate.callback.report-attribute-transportation-type]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"query-attribute-transport-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{efficiency};
  auto const reliable = rti->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(objectClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §6.28 invocation has no successful-void report record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      rti->queryAttributeTransportationType(unknownObject, efficiency),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  auto const textBeforeQuery = readTextFile(reportFile);
  ObjectInstanceHandle objectInstance;
  // Keep setup registration outside this focused query-service report lane.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->queryAttributeTransportationType(objectInstance, efficiency));

  auto const objectInstanceText = objectInstance.toString();
  auto const attributeText = efficiency.toString();
  std::string objectInstanceValue;
  objectInstanceValue.reserve(objectInstanceText.size());
  for (wchar_t const character : objectInstanceText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectInstanceValue.push_back(static_cast<char>(character));
  }
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"QueryAttributeTransportationType","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":0,"HLAargumentName":"Attribute designator","HLAargumentValue":")" +
      attributeValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The service report records accepted query invocation; the separate
  // Report Attribute Transportation Type callback remains later work.
  REQUIRE(readTextFile(reportFile) == textBeforeQuery + expectedRecord);
  REQUIRE(reports.attributeTransportationTypeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.attributeTransportationTypeReports.size() == 1U);
  REQUIRE(reports.attributeTransportationTypeReports.front().objectInstance == objectInstance);
  REQUIRE(reports.attributeTransportationTypeReports.front().attribute == efficiency);
  REQUIRE(reports.attributeTransportationTypeReports.front().transportationType == reliable);
  REQUIRE(readTextFile(reportFile) == textBeforeQuery + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Query Interaction Transportation Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[query-interaction-transportation-type]"
    "[rti.service.query-interaction-transportation-type]"
    "[federate.callback.report-interaction-transportation-type]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle joinedFederate;
  REQUIRE_NOTHROW(joinedFederate = rti->joinFederationExecution(
      L"query-interaction-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass =
      rti->getInteractionClassHandle(fixture_hla::fom::server_take_order);
  auto const reliable = rti->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(joinedFederate.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §6.32 invocation has no successful-void report record.
  FederateHandle const invalidFederate;
  REQUIRE_THROWS_AS(
      rti->queryInteractionTransportationType(invalidFederate, interactionClass),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(
      rti->queryInteractionTransportationType(joinedFederate, interactionClass));

  auto const federateText = joinedFederate.toString();
  auto const interactionClassText = interactionClass.toString();
  std::string federateValue;
  federateValue.reserve(federateText.size());
  for (wchar_t const character : federateText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    federateValue.push_back(static_cast<char>(character));
  }
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"QueryInteractionTransportationType","HLAsuppliedArguments":[{"HLAargumentType":15,"HLAargumentName":"Federate designator","HLAargumentValue":")" +
      federateValue +
      R"("},{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The service report records accepted query invocation; the separate Report
  // Interaction Transportation Type callback remains later work.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(reports.interactionTransportationTypeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.interactionTransportationTypeReports.size() == 1U);
  REQUIRE(reports.interactionTransportationTypeReports.front().federate == joinedFederate);
  REQUIRE(
      reports.interactionTransportationTypeReports.front().interactionClass == interactionClass);
  REQUIRE(
      reports.interactionTransportationTypeReports.front().transportationType == reliable);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Change Attribute Order Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[time-management][mom][service-report-file][service-reporting]"
    "[change-attribute-order-type]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.change-attribute-order-type]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"attribute-order-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{efficiency};
  REQUIRE(objectClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §8.24 invocation has no successful-void report record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      rti->changeAttributeOrderType(unknownObject, attributes, TIMESTAMP),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  auto const textBeforeChange = readTextFile(reportFile);
  ObjectInstanceHandle objectInstance;
  // Keep setup registration outside this focused order-change report lane.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->changeAttributeOrderType(objectInstance, attributes, TIMESTAMP));

  auto const objectInstanceText = objectInstance.toString();
  auto const attributeText = efficiency.toString();
  std::string objectInstanceValue;
  objectInstanceValue.reserve(objectInstanceText.size());
  for (wchar_t const character : objectInstanceText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectInstanceValue.push_back(static_cast<char>(character));
  }
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ChangeAttributeOrderType","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"TIMESTAMP"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == textBeforeChange + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Change Default Attribute Order Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[time-management][mom][service-report-file][service-reporting]"
    "[change-default-attribute-order-type]"
    "[rti.service.change-default-attribute-order-type]") {
  ServiceReportFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"default-attribute-order-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{efficiency};
  REQUIRE(objectClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §8.25 invocation has no successful-void report record.
  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      rti->changeDefaultAttributeOrderType(unknownObjectClass, attributes, TIMESTAMP),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));

  auto const objectClassText = objectClass.toString();
  auto const attributeText = efficiency.toString();
  std::string objectClassValue;
  objectClassValue.reserve(objectClassText.size());
  for (wchar_t const character : objectClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectClassValue.push_back(static_cast<char>(character));
  }
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ChangeDefaultAttributeOrderType","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"TIMESTAMP"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves the supplied Retract designator",
    "[integration][development-profile][federation-management][interaction-management][mom]"
    "[service-report-file][service-reporting][time-management][retract]"
    "[rti.service.enable-time-regulation][rti.service.send-interaction]"
    "[rti.service.retract][federate.callback.receive-interaction]"
    "[federate.callback.request-retraction]") {
  ServiceReportFederateAmbassador publisherReports;
  ServiceReportFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto publisherConfiguration = configurationForServiceReportDirectory(directory.path());
  publisherConfiguration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {0x52, 0x45, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, publisherConfiguration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"retract-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"retract-report-receiver", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const publisherReportFile = files.front();

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const textBeforeRetract = readTextFile(publisherReportFile);
  REQUIRE(textBeforeRetract.find("\"HLAservice\":\"EnableTimeRegulation\"") !=
          std::string::npos);

  auto const handleText = retraction.toString();
  auto const opening = handleText.find(L'(');
  auto const closing = handleText.find(L')', opening);
  REQUIRE(opening != std::wstring::npos);
  REQUIRE(closing == handleText.size() - 1U);
  auto const messageIdText = handleText.substr(opening + 1U, closing - opening - 1U);
  REQUIRE_FALSE(messageIdText.empty());
  std::string messageId;
  messageId.reserve(messageIdText.size());
  for (wchar_t const character : messageIdText) {
    REQUIRE(character >= L'0');
    REQUIRE(character <= L'9');
    messageId.push_back(static_cast<char>(character));
  }

  REQUIRE_NOTHROW(publisher->retract(retraction));
  auto const expectedRequest =
      std::string{
          "{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[null],\"HLAservice\":\"Retract\","
          "\"HLAsuppliedArguments\":[{\"HLAargumentType\":33,\"HLAargumentName\":"
          "\"MessageRetractionDesignator\",\"HLAargumentValue\":\"MessageRetractionHandle<"} +
      messageId +
      ">\"}],\"HLAsuccessIndicator\":true,\"HLAexception\":null}";
  REQUIRE(readTextFile(publisherReportFile) == textBeforeRetract + expectedRequest);

  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(readTextFile(publisherReportFile) == textBeforeRetract + expectedRequest);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers Retract through MOM before Request Retraction",
    "[integration][development-profile][federation-management][interaction-management][mom]"
    "[service-report-interaction][service-reporting][time-management][retract]"
    "[rti.service.enable-time-regulation][rti.service.send-interaction]"
    "[rti.service.retract][federate.callback.receive-interaction]"
    "[federate.callback.request-retraction]") {
  ServiceReportFederateAmbassador publisherReports;
  ServiceReportFederateAmbassador receiverReports;
  ServiceReportFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto publisherConfiguration = configurationForServiceReportDirectory(directory.path());
  publisherConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, publisherConfiguration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"retract-mom-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"retract-mom-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"retract-mom-observer", L"observer", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Keep setup and declaration calls out of the one interaction-selected
  // assertion. The observer's own switch remains disabled so it can receive
  // the RTI-originated report interaction.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      VariableLengthData{},
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const reportCountBeforeRetract = observerReports.interactionReports.size();

  auto const handleText = retraction.toString();
  auto const opening = handleText.find(L'(');
  auto const closing = handleText.find(L')', opening);
  REQUIRE(opening != std::wstring::npos);
  REQUIRE(closing == handleText.size() - 1U);
  auto const messageIdText = handleText.substr(opening + 1U, closing - opening - 1U);
  REQUIRE_FALSE(messageIdText.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  // HLA_IMMEDIATE makes the report callback observable before retract() can
  // queue the separately required Request Retraction callback.
  REQUIRE(observerReports.interactionReports.size() == reportCountBeforeRetract + 1U);
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.back();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(decodedService.get() == L"Retract");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 4);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord suppliedPrototype;
  suppliedPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{suppliedPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 1U);
  auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(0U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == 33);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() ==
          L"MessageRetractionDesignator");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
          L"\"MessageRetractionHandle<" + messageIdText + L">\"");

  rti1516_2025::HLAfixedRecord nullReturned;
  nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() ==
          L"null");

  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() >= 0);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves the supplied Flush Queue Request boundary",
    "[integration][development-profile][federation-management][interaction-management][mom]"
    "[service-report-file][service-reporting][time-management][flush-queue-request]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[rti.service.query-logical-time][federate.callback.receive-interaction]"
    "[federate.callback.flush-queue-grant]") {
  ServiceReportFederateAmbassador publisherReports;
  ServiceReportFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto receiverConfiguration = configurationForServiceReportDirectory(directory.path());
  receiverConfiguration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {0x46, 0x51, 0x52, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED, receiverConfiguration));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"fqr-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"fqr-report-receiver", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const receiverReportFile = files.front();

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xF3, 0xF4};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  auto const first = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  auto const second = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(12));
  REQUIRE(first.isValid());
  REQUIRE(second.isValid());
  auto const textBeforeRequest = readTextFile(receiverReportFile);

  // §8.12 supplies 10, while the queued messages and the regulator's GALT
  // later determine FQG's actual time 5 and optimistic time 7. The service
  // report is the accepted invocation, so it retains the supplied boundary.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  auto const expectedRequest =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"FlushQueueRequest","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"10"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(receiverReportFile) == textBeforeRequest + expectedRequest);
  REQUIRE(receiverReports.flushQueueGrantReports.empty());

  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(receiverReports.flushQueueGrantReports.front().optimisticValue == L"7");
  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 5);
  REQUIRE(readTextFile(receiverReportFile) == textBeforeRequest + expectedRequest);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting appends Table 5 void records for advisory switch setters",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][support-switches][declaration-management][object-management]"
    "[rti.service.set-object-class-relevance-advisory-switch]"
    "[rti.service.set-attribute-relevance-advisory-switch]"
    "[rti.service.set-attribute-scope-advisory-switch]"
    "[rti.service.set-interaction-relevance-advisory-switch]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"advisory-switch-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto expectedText = readTextFile(files.front());
  auto appendExpectedRecord = [&](std::uint32_t serialNumber, std::string_view service) {
    expectedText += std::string{"{\"HLAserialNumber\":"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":")" + std::string{service} +
        R"(","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
    REQUIRE(readTextFile(files.front()) == expectedText);
  };

  REQUIRE_NOTHROW(rti->setObjectClassRelevanceAdvisorySwitch(false));
  appendExpectedRecord(0U, "SetObjectClassRelevanceAdvisorySwitch");
  REQUIRE_NOTHROW(rti->setAttributeRelevanceAdvisorySwitch(false));
  appendExpectedRecord(1U, "SetAttributeRelevanceAdvisorySwitch");
  REQUIRE_NOTHROW(rti->setAttributeScopeAdvisorySwitch(false));
  appendExpectedRecord(2U, "SetAttributeScopeAdvisorySwitch");
  REQUIRE_NOTHROW(rti->setInteractionRelevanceAdvisorySwitch(false));
  appendExpectedRecord(3U, "SetInteractionRelevanceAdvisorySwitch");

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service-report configuration rejects unusable directories without fallback",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-report-store][service-reporting][service-report-directory-config]"
    "[rti.service.connect]") {
  TestFederateAmbassador emptySettingReports;
  auto emptySettingRti = makeRti();
  auto const emptyDirectorySetting = configurationForServiceReportDirectory({});

  // The typed profile configuration exposes a real directory, not a
  // selectable memory sink. An empty explicit directory therefore fails
  // before Connect changes lifecycle state, and a later ordinary Connect
  // remains possible.
  REQUIRE_THROWS_AS(
      emptySettingRti->connect(emptySettingReports, HLA_EVOKED, emptyDirectorySetting),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(emptySettingRti->connect(emptySettingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(emptySettingRti->disconnect());

  static std::atomic_uint64_t counter{0};
  auto const filePath = std::filesystem::temp_directory_path() /
      ("umbra-service-report-not-a-directory-" + std::to_string(++counter));
  {
    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    REQUIRE(output.good());
    output << "not a directory";
    REQUIRE(output.good());
  }
  ScopedTemporaryFile fileGuard(filePath);

  TestFederateAmbassador fileSettingReports;
  auto fileSettingRti = makeRti();
  auto const fileDirectorySetting = RtiConfiguration::createConfiguration()
      .withAdditionalSettings(L"serviceReportDirectory=" + filePath.wstring());
  REQUIRE_THROWS_AS(
      fileSettingRti->connect(fileSettingReports, HLA_EVOKED, fileDirectorySetting),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(fileSettingRti->connect(fileSettingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fileSettingRti->disconnect());
}

TEST_CASE(
    "Embedded service-report files retain one joined-federate path across switch cycles",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][service-report-file-lifecycle][support-switches]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-cycle-subject", L"subject", federationName));

  auto files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  REQUIRE(reportFile.is_absolute());
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->getObjectClassHandle(standard_hla::fom::object_root));
  auto const enabledText = readTextFile(reportFile);
  REQUIRE(enabledText.size() > initialText.size());

  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->getObjectClassHandle(standard_hla::fom::object_root));
  REQUIRE(serviceReportFiles(directory.path()) == std::vector<std::filesystem::path>{reportFile});
  REQUIRE(readTextFile(reportFile) == enabledText);

  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->getObjectClassHandle(standard_hla::fom::object_root));
  auto const reenabledText = readTextFile(reportFile);
  REQUIRE(reenabledText.size() > enabledText.size());
  REQUIRE(serviceReportFiles(directory.path()) == std::vector<std::filesystem::path>{reportFile});

  // The service-reporting switch is an independent gate.  Disabling it must
  // suppress later file appends without changing the joined-federate file
  // identity; re-enabling it must resume on that same file.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->getObjectClassHandle(standard_hla::fom::object_root));
  REQUIRE(serviceReportFiles(directory.path()) == std::vector<std::filesystem::path>{reportFile});
  REQUIRE(readTextFile(reportFile) == reenabledText);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->getObjectClassHandle(standard_hla::fom::object_root));
  auto const reenabledReportingText = readTextFile(reportFile);
  REQUIRE(reenabledReportingText.size() > reenabledText.size());
  REQUIRE(serviceReportFiles(directory.path()) == std::vector<std::filesystem::path>{reportFile});

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}


}  // namespace
