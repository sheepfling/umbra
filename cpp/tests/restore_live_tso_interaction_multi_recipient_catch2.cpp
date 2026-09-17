#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The live timestamped interaction restore tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"restore-live-tso-interaction-multi-recipient-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimestampedInteractionReport final {
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

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  void federationSaved() override {
    ++federationSavedReportCount;
    callbackOrder.push_back("save-complete");
  }

  void federationRestored() override {
    ++federationRestoredReportCount;
    callbackOrder.push_back("restore-complete");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantValues.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

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
  }

  void requestRetraction(MessageRetractionHandle const& retraction) override {
    requestRetractions.push_back({retraction.isValid(), retraction.encode()});
    callbackOrder.push_back("request-retraction");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrants.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractions;
  std::vector<RequestRetractionReport> requestRetractions;
  std::vector<FlushQueueGrantReport> flushQueueGrants;
  std::vector<std::string> callbackOrder;
  std::vector<std::wstring> timeAdvanceGrantValues;
  std::size_t federationSavedReportCount = 0U;
  std::size_t federationRestoredReportCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded federation restore restores one queued timestamped interaction to multiple recipients",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][time-management][tso]"
    "[timestamped-interaction][mixed-fanout][multi-federate-callback-ordering]"
    "[timestamped-interaction-restore-multi-recipient]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.federation-saved]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-restored]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x4D, 0x55, 0x4C, 0x54, 0x49};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x53, 0x54, 0x4F, 0x52, 0x45};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"restore-live-tso-interaction-multi";
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-live-tso-multi-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"restore-live-tso-multi-first", L"subscriber", federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"restore-live-tso-multi-second", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::temperature_ok);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  parameterValues.emplace(
      parameter,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(firstReceiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(secondReceiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(firstReceiver->enableTimeConstrained());
  drain(*firstReceiver);
  REQUIRE_NOTHROW(secondReceiver->enableTimeConstrained());
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*publisher);

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReceiverReports.timestampedInteractions.empty());
  REQUIRE(secondReceiverReports.timestampedInteractions.empty());

  // Capture both pending recipient copies while each constrained federate is
  // below the timestamp. Restore must preserve each copy and its retraction
  // ledger independently.
  REQUIRE_NOTHROW(firstReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(secondReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.timeAdvanceGrantValues ==
          std::vector<std::wstring>{L"1"});
  REQUIRE(secondReceiverReports.timeAdvanceGrantValues ==
          std::vector<std::wstring>{L"1"});
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.timestampedInteractions.empty());
  REQUIRE(secondReceiverReports.timestampedInteractions.empty());

  // Remove the live post-save handle. Restore must reconstruct the queued
  // passel and both recipient entries from the saved image.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.timestampedInteractions.empty());
  REQUIRE(secondReceiverReports.timestampedInteractions.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);

  auto const verifyInteraction = [&](ReportingFederateAmbassador const& reports) {
    REQUIRE(reports.timestampedInteractions.size() == 1U);
    REQUIRE(reports.flushQueueGrants.size() == 1U);
    auto const& report = reports.timestampedInteractions.front();
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(parameter));
    REQUIRE(variableLengthDataBytes(report.parameterValues.at(parameter)) ==
            std::vector<unsigned char>(
                parameterBytes, parameterBytes + sizeof(parameterBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(report.sentRegions.empty());
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
    REQUIRE(reports.flushQueueGrants.front().timeImplementationName ==
            standard_hla::mom::integer64_time);
    REQUIRE(reports.flushQueueGrants.front().value == L"5");
    REQUIRE(reports.flushQueueGrants.front().optimisticValue == L"7");
  };

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(firstReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.timestampedInteractions.size() == 1U);
  REQUIRE(firstReceiverReports.flushQueueGrants.size() == 1U);
  REQUIRE(secondReceiverReports.timestampedInteractions.empty());
  REQUIRE(secondReceiverReports.flushQueueGrants.empty());
  REQUIRE(secondReceiverReports.callbackOrder.empty());
  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  verifyInteraction(firstReceiverReports);
  verifyInteraction(secondReceiverReports);
  REQUIRE(firstReceiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});
  REQUIRE(secondReceiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.requestRetractions.size() == 1U);
  REQUIRE(secondReceiverReports.requestRetractions.size() == 1U);
  REQUIRE(firstReceiverReports.requestRetractions.front().retractionValid);
  REQUIRE(secondReceiverReports.requestRetractions.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              firstReceiverReports.requestRetractions.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(variableLengthDataBytes(
              secondReceiverReports.requestRetractions.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(firstReceiverReports.callbackOrder ==
          std::vector<std::string>{"request-retraction"});
  REQUIRE(secondReceiverReports.callbackOrder ==
          std::vector<std::string>{"request-retraction"});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(firstReceiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(secondReceiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
