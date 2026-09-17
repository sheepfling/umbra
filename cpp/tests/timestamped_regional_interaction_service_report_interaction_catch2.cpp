#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped regional interaction MOM service-report test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-regional-interaction-mom-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::string asAscii(std::wstring const& text) {
  std::string result;
  result.reserve(text.size());
  for (wchar_t const character : text) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    result.push_back(static_cast<char>(character));
  }
  return result;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ReceiverAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimestampedInteraction final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct TimeAdvanceGrant final {
    std::wstring timeImplementationName;
    std::wstring timeValue;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractions.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("interaction");
    if (onTimestampedInteraction) {
      onTimestampedInteraction();
    }
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrants.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<TimestampedInteraction> timestampedInteractions;
  std::vector<TimeAdvanceGrant> timeAdvanceGrants;
  std::vector<std::string> callbackOrder;
  std::function<void()> onTimestampedInteraction;
};

class ObserverAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ServiceReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*) override {
    serviceReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
    if (onServiceReport) {
      onServiceReport();
    }
  }

  std::vector<ServiceReport> serviceReports;
  std::function<void()> onServiceReport;
};

}  // namespace

TEST_CASE(
    "Embedded service reporting delivers accepted timestamped regional Send Interaction With Regions through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][time-management][mom][service-reporting][service-report-interaction][tso]"
    "[timestamped-regional-interaction][timestamped-regional-interaction-service-report]"
    "[rti.service.timestamped-regional-interaction-service-report-interaction]"
    "[rti.service.send-interaction-with-regions][rti.service.change-interaction-order-type]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.set-service-reporting-switch][rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.subscribe-interaction-class][federate.callback.receive-interaction]") {
  rti1516_2025::NullFederateAmbassador publisherCallbacks;
  ReceiverAmbassador receiverCallbacks;
  ObserverAmbassador observerCallbacks;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerCallbacks, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-interaction-mom-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-interaction-mom-receiver",
      L"receiver",
      federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-regional-interaction-mom-observer",
      L"observer",
      federationName));

  // Setup calls must not consume the accepted interaction's service-report serial.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  auto const serviceParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::service);
  auto const serviceTypeParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::service_type);
  auto const successParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::success_indicator);
  auto const suppliedArgumentsParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::supplied_arguments);
  auto const returnedArgumentParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::returned_argument);
  auto const exceptionParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::exception);
  auto const serialParameter = observer->getParameterHandle(
      reportClass,
      standard_hla::mom::serial_number);
  for (auto const& parameter : {
           serviceParameter,
           serviceTypeParameter,
           successParameter,
           suppliedArgumentsParameter,
           returnedArgumentParameter,
           exceptionParameter,
           serialParameter}) {
    REQUIRE(parameter.isValid());
  }
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const dimension = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  auto const receiverRegion = receiver->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      dimension,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      dimension,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*publisher);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());
  bool reportPresentAtReceiverCallback = false;
  receiverCallbacks.onTimestampedInteraction = [&] {
    reportPresentAtReceiverCallback = observerCallbacks.serviceReports.size() == 1U;
  };

  ParameterHandleValueMap const parameterValues{{parameter, parameterValue}};
  auto const retraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      timestamp);
  REQUIRE(retraction.isValid());
  REQUIRE(receiverCallbacks.timestampedInteractions.empty());
  REQUIRE(observerCallbacks.serviceReports.size() == 1U);

  auto const& report = observerCallbacks.serviceReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 7U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(serviceParameter)));
  REQUIRE(decodedService.get() == L"SendInteractionWithRegions");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(serviceTypeParameter)));
  REQUIRE(decodedServiceType.get() == 2);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(successParameter)));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(suppliedArguments.decode(
      report.parameterValues.at(suppliedArgumentsParameter)));
  REQUIRE(suppliedArguments.size() == 5U);
  auto const interactionValue = asAscii(interactionClass.toString());
  auto const parameterDesignator = asAscii(parameter.toString());
  auto const regionValue = asAscii(publisherRegion.toString());
  auto const timestampValue = asAscii(timestamp.toString());
  auto const verifyArgument = [&](std::size_t index,
                                  std::int32_t type,
                                  std::wstring const& name,
                                  std::wstring const& value) {
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == value);
  };
  verifyArgument(0U, 27, L"Interaction class designator", L"\"" +
      std::wstring{interactionClass.toString()} + L"\"");
  verifyArgument(1U, 40, L"Constrained set of interaction parameter designator and value pairs",
                 L"{\"" + std::wstring{parameter.toString()} + L"\":\"AQI=\"}");
  verifyArgument(2U, 43, L"Set of region designators", L"[\"" +
      std::wstring{publisherRegion.toString()} + L"\"]");
  verifyArgument(3U, 63, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(4U, 31, L"Optional timestamp", L"\"" + timestamp.toString() + L"\"");

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(returnedArgument.decode(
      report.parameterValues.at(returnedArgumentParameter)));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
          33);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
          L"Message retraction designator");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  REQUIRE(retractionOpen != std::wstring::npos);
  REQUIRE(retractionClose > retractionOpen + 1U);
  auto const momRetraction = dynamic_cast<rti1516_2025::HLAunicodeString const&>(
      returnedArgument.get(2U)).get();
  constexpr std::wstring_view prefix = L"\"MessageRetractionHandle<";
  REQUIRE(momRetraction.rfind(std::wstring{prefix}, 0U) == 0U);
  REQUIRE(momRetraction.size() > prefix.size() + 1U);
  REQUIRE(momRetraction[momRetraction.size() - 2U] == L'>');
  REQUIRE(momRetraction.back() == L'\"');
  REQUIRE(momRetraction.substr(prefix.size(), momRetraction.size() - prefix.size() - 2U) ==
          retractionText.substr(retractionOpen + 1U,
                                retractionClose - retractionOpen - 1U));
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(exceptionParameter)));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(serialParameter)));
  REQUIRE(decodedSerial.get() == 0);

  // The MOM report is delivered while the publisher invokes the service, and
  // the timestamped regional callback remains ordered before its grant.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(timestamp));
  REQUIRE(receiverCallbacks.timestampedInteractions.empty());
  REQUIRE(receiverCallbacks.timeAdvanceGrants.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  drain(*publisher);
  drain(*receiver);
  REQUIRE(reportPresentAtReceiverCallback);
  REQUIRE(receiverCallbacks.timestampedInteractions.size() == 1U);
  REQUIRE(receiverCallbacks.timeAdvanceGrants.size() == 1U);
  REQUIRE(receiverCallbacks.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto const& interaction = receiverCallbacks.timestampedInteractions.front();
  REQUIRE(interaction.interactionClass == interactionClass);
  REQUIRE(interaction.parameterValues.size() == 1U);
  REQUIRE(interaction.parameterValues.contains(parameter));
  auto const* receivedBytes = static_cast<unsigned char const*>(
      interaction.parameterValues.at(parameter).data());
  REQUIRE(receivedBytes != nullptr);
  REQUIRE(interaction.parameterValues.at(parameter).size() == sizeof(parameterBytes));
  REQUIRE(receivedBytes[0] == parameterBytes[0]);
  REQUIRE(receivedBytes[1] == parameterBytes[1]);
  auto const* receivedTag = static_cast<unsigned char const*>(interaction.userSuppliedTag.data());
  REQUIRE(receivedTag != nullptr);
  REQUIRE(interaction.userSuppliedTag.size() == sizeof(tagBytes));
  REQUIRE(receivedTag[0] == tagBytes[0]);
  REQUIRE(receivedTag[1] == tagBytes[1]);
  REQUIRE(receivedTag[2] == tagBytes[2]);
  REQUIRE(interaction.transportationType ==
          receiver->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(interaction.producingFederate == publisherHandle);
  REQUIRE(interaction.sentRegionsSupplied);
  REQUIRE(interaction.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE(interaction.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(interaction.timeValue == timestamp.toString());
  REQUIRE(interaction.sentOrderType == TIMESTAMP);
  REQUIRE(interaction.receivedOrderType == TIMESTAMP);
  REQUIRE(interaction.retractionSupplied);
  REQUIRE(interaction.retractionValid);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
