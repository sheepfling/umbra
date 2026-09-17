#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The ordinary regional interaction MOM service-report test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-interaction-service-report-mom-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ReceiverAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Interaction final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    interactions.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
    if (onInteraction) {
      onInteraction();
    }
  }

  std::vector<Interaction> interactions;
  std::function<void()> onInteraction;
};

class ObserverAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
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
  }

  std::vector<ServiceReport> serviceReports;
};

}  // namespace

TEST_CASE(
    "Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][mom][service-reporting][service-report-interaction]"
    "[ordinary-regional-interaction][ordinary-regional-interaction-service-report]"
    "[ordinary-regional-interaction-service-report-mom-interaction]"
    "[rti.service.ordinary-regional-interaction-service-report-matrix-interaction]"
    "[rti.service.send-interaction-with-regions][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.subscribe-interaction-class][federate.callback.receive-interaction]"
    "[callback-evoked][callback-immediate]") {
  rti1516_2025::NullFederateAmbassador publisherCallbacks;
  ReceiverAmbassador receiverCallbacks;
  ObserverAmbassador observerCallbacks;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'r', 'e', 'g'};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerCallbacks, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"regional-mom-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-mom-receiver",
      L"receiver",
      federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"regional-mom-observer",
      L"observer",
      federationName));

  // Keep setup calls out of the accepted service-report record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

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

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const dimension = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

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
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());
  bool reportPresentAtCallbackEntry = false;
  receiverCallbacks.onInteraction = [&] {
    reportPresentAtCallbackEntry = observerCallbacks.serviceReports.size() == 1U;
  };

  ParameterHandleValueMap const parameterValues{{parameter, parameterValue}};
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  REQUIRE(receiverCallbacks.interactions.empty());
  REQUIRE(observerCallbacks.serviceReports.size() == 1U);

  auto const& report = observerCallbacks.serviceReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 7U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());

  rti1516_2025::HLAunicodeString service;
  REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(service.get() == L"SendInteractionWithRegions");
  rti1516_2025::HLAinteger16BE serviceType;
  REQUIRE_NOTHROW(serviceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(serviceType.get() == 2);
  rti1516_2025::HLAboolean success;
  REQUIRE_NOTHROW(success.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(success.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(suppliedArguments.decode(
      report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 5U);
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
  verifyArgument(0U, 27, L"Interaction class designator",
                 L"\"" + interactionClass.toString() + L"\"");
  verifyArgument(1U, 40,
                 L"Constrained set of interaction parameter designator and value pairs",
                 L"{\"" + parameter.toString() + L"\":\"AQI=\"}");
  verifyArgument(2U, 43, L"Set of region designators",
                 L"[\"" + publisherRegion.toString() + L"\"]");
  verifyArgument(3U, 63, L"User-supplied tag", L"\"cmVn\"");
  verifyArgument(4U, 34, L"Optional timestamp", L"null");

  rti1516_2025::HLAfixedRecord returned;
  returned.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(returned.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returned.get(0U)).get() == 34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(2U)).get() ==
          L"null");
  rti1516_2025::HLAunicodeString exception;
  REQUIRE_NOTHROW(exception.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(exception.get().empty());
  rti1516_2025::HLAinteger32BE serial;
  REQUIRE_NOTHROW(serial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(serial.get() == 0);

  // The immediate MOM report is delivered while the publisher invokes the
  // service; the regional application callback remains queued for HLA_EVOKED.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  auto const drain = [](rti1516_2025::RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };
  drain(*receiver);
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(receiverCallbacks.interactions.size() == 1U);
  auto const& interaction = receiverCallbacks.interactions.front();
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
