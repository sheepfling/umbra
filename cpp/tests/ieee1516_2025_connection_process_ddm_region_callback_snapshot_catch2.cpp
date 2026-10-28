#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "hla_test_names.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/time/reference_time_selection.hpp"
#include "process_public_service_fixture.hpp"

namespace {

namespace fixture_hla = umbra::test::hla::wide;
namespace process_fom = umbra::test::process_fom_support_2025;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationManagementCoordinator;
using umbra::detail::FederationManagementResources;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FederationPreparationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceHost;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ReferenceLogicalTimeSelector;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::test::connection_support_2025::makeRti;

std::filesystem::path regionalFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "two-dimensional-regional-interaction-fom.xml";
}

FederationDefinition regionalProcessDefinition() {
  std::vector<process_fom::PrevalidatedFomModule> modules{
      process_fom::validatedProcessModule(
          process_fom::processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-ddm-snapshot-mim"),
      process_fom::validatedProcessModule(
          regionalFom(),
          FomModuleKind::fom,
          L"urn:umbra:test:process-ddm-snapshot-regional-interaction"),
  };
  LibXml2FomModuleComposer composer(
      process_fom::processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error(
        "The process DDM snapshot FOM modules did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

class ProcessSnapshotReceiver final : public NullFederateAmbassador {
 public:
  RTIambassador* receiver = nullptr;
  RTIambassador* publisher = nullptr;
  RegionHandle sourceRegion;
  rti1516_2025::DimensionHandle publisherDimension;
  rti1516_2025::DimensionHandle receiverDimension;
  rti1516_2025::DimensionHandleSet expectedDimensions;
  std::atomic_uint callbacks{0U};
  std::optional<RegionHandle> conveyedRegion;
  rti1516_2025::DimensionHandleSet conveyedDimensions;
  std::optional<RangeBounds> beforeMutation;
  std::optional<RangeBounds> afterMutation;
  std::exception_ptr callbackFailure;

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      rti1516_2025::ParameterHandleValueMap const&,
      rti1516_2025::VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    callbacks.fetch_add(1U, std::memory_order_release);
    if (optionalSentRegions == nullptr || optionalSentRegions->size() != 1U ||
        receiver == nullptr || publisher == nullptr) {
      return;
    }

    try {
      conveyedRegion = *optionalSentRegions->begin();
      conveyedDimensions = receiver->getDimensionHandleSet(*conveyedRegion);
      beforeMutation = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);

      // The publisher changes and commits the source while this callback is
      // still active; the receiving federate's conveyed designator must retain
      // the send-time value for the callback's remaining duration.
      publisher->setRangeBounds(
          sourceRegion,
          publisherDimension,
          RangeBounds(4UL, 6UL));
      publisher->commitRegionModifications(RegionHandleSet{sourceRegion});
      afterMutation = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);
    } catch (...) {
      callbackFailure = std::current_exception();
    }
  }
};

class ProcessTimestampedReflectionSnapshotReceiver final
    : public NullFederateAmbassador {
 public:
  RTIambassador* receiver = nullptr;
  rti1516_2025::DimensionHandle receiverDimension;
  rti1516_2025::DimensionHandleSet expectedDimensions;
  std::atomic_uint callbacks{0U};
  std::atomic_bool timeConstrainedEnabledReceived{false};
  bool timestampedCallback = false;
  std::optional<RegionHandle> conveyedRegion;
  rti1516_2025::DimensionHandleSet conveyedDimensions;
  std::optional<RangeBounds> firstCallbackBounds;
  std::optional<RangeBounds> secondCallbackBounds;
  std::exception_ptr callbackFailure;

  void timeConstrainedEnabled(
      rti1516_2025::LogicalTime const&) override {
    timeConstrainedEnabledReceived.store(true, std::memory_order_release);
  }

  void reflectAttributeValues(
      rti1516_2025::ObjectInstanceHandle const&,
      rti1516_2025::AttributeHandleValueMap const&,
      rti1516_2025::VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const&,
      rti1516_2025::OrderType sentOrder,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const*) override {
    callbacks.fetch_add(1U, std::memory_order_release);
    timestampedCallback = sentOrder == rti1516_2025::TIMESTAMP &&
        receivedOrder == rti1516_2025::TIMESTAMP;
    if (optionalSentRegions == nullptr || optionalSentRegions->size() != 1U ||
        receiver == nullptr) {
      return;
    }

    try {
      conveyedRegion = *optionalSentRegions->begin();
      conveyedDimensions = receiver->getDimensionHandleSet(*conveyedRegion);
      firstCallbackBounds = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);
      secondCallbackBounds = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);
    } catch (...) {
      callbackFailure = std::current_exception();
    }
  }
};

}  // namespace

TEST_CASE(
    "Configured-process HLA_EVOKED regional interaction retains a send-time callback designator snapshot",
    "[integration][development-profile][process-boundary][ddm]"
    "[ddm-process-conveyed-region-designator-callback-snapshot]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.get-range-bounds][rti.service.get-dimension-handle-set]"
    "[rti.service.commit-region-modifications]"
    "[federate.callback.receive-interaction][callback-evoked]"
    "[requirement-candidate-content-clauses-09-data-distribution-management-page-225-l96-30]"
    "[2025]") {
  constexpr wchar_t const* federationName =
      L"process-ddm-region-callback-snapshot";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::mutex serverErrorMutex;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          regionalProcessDefinition(),
          ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-ddm-snapshot-server", 0xA930U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      if (!umbra::test::servePrimaryProcessRequest(
              sender,
              senderHandler,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::create_federation_execution) {
                  throw std::runtime_error(
                      "The process DDM snapshot server expected Create.");
                }
                return senderHandler(request);
              }) ||
          !umbra::test::servePrimaryProcessRequest(
              sender,
              senderHandler,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::join_federation_execution) {
                  throw std::runtime_error(
                      "The process DDM snapshot server expected sender Join.");
                }
                return senderHandler(request);
              })) {
        throw std::runtime_error(
            "The process DDM snapshot server lost sender setup.");
      }

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-ddm-snapshot-server", 0xA931U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      if (!umbra::test::servePrimaryProcessRequest(
              receiver,
              receiverHandler,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::join_federation_execution) {
                  throw std::runtime_error(
                      "The process DDM snapshot server expected receiver Join.");
                }
                return receiverHandler(request);
              })) {
        throw std::runtime_error(
            "The process DDM snapshot server lost receiver Join.");
      }

      auto serveUntilDisconnected = [&](ProcessTransportSession& session,
                                        auto const& handler) {
        try {
          while (ProcessTransportServiceDispatcher::serveOne(session, handler)) {
          }
        } catch (...) {
          {
            std::scoped_lock lock(serverErrorMutex);
            if (!serverError) {
              serverError = std::current_exception();
            }
          }
          senderConnection->close();
          receiverConnection->close();
        }
      };
      std::thread senderWorker([&] {
        serveUntilDisconnected(sender, senderHandler);
      });
      std::thread receiverWorker([&] {
        serveUntilDisconnected(receiver, receiverHandler);
      });
      senderWorker.join();
      receiverWorker.join();
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      std::scoped_lock lock(serverErrorMutex);
      serverError = std::current_exception();
    }
  });

  ProcessSnapshotReceiver receiverCallbacks;
  NullFederateAmbassador senderCallbacks;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto const configuration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-ddm-region-callback-snapshot")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    if (!senderRti->connect(senderCallbacks, HLA_IMMEDIATE, configuration)
             .addressUsed) {
      throw std::runtime_error("The process DDM snapshot sender did not connect.");
    }
    senderRti->createFederationExecution(
        federationName, regionalFom().wstring(), L"");
    senderRti->joinFederationExecution(
        L"process-ddm-snapshot-sender", L"snapshot-sender", federationName);
    senderJoined = true;

    if (!receiverRti->connect(receiverCallbacks, HLA_EVOKED, configuration)
             .addressUsed) {
      throw std::runtime_error("The process DDM snapshot receiver did not connect.");
    }
    receiverRti->joinFederationExecution(
        L"process-ddm-snapshot-receiver", L"snapshot-receiver", federationName);
    receiverJoined = true;

    auto const interaction = receiverRti->getInteractionClassHandle(
        fixture_hla::fom::two_dimensional_regional_interaction);
    auto const senderInteraction = senderRti->getInteractionClassHandle(
        fixture_hla::fom::two_dimensional_regional_interaction);
    auto const senderX = senderRti->getDimensionHandle(
        fixture_hla::fixture::umbra_region_x);
    auto const senderY = senderRti->getDimensionHandle(
        fixture_hla::fixture::umbra_region_y);
    auto const receiverX = receiverRti->getDimensionHandle(
        fixture_hla::fixture::umbra_region_x);
    auto const receiverY = receiverRti->getDimensionHandle(
        fixture_hla::fixture::umbra_region_y);
    senderRti->publishInteractionClass(senderInteraction);

    auto const sourceRegion = senderRti->createRegion(
        rti1516_2025::DimensionHandleSet{senderX, senderY});
    auto const subscriptionRegion = receiverRti->createRegion(
        rti1516_2025::DimensionHandleSet{receiverX, receiverY});
    senderRti->setRangeBounds(sourceRegion, senderX, RangeBounds(1UL, 3UL));
    senderRti->setRangeBounds(sourceRegion, senderY, RangeBounds(1UL, 3UL));
    senderRti->commitRegionModifications(RegionHandleSet{sourceRegion});
    receiverRti->setRangeBounds(
        subscriptionRegion, receiverX, RangeBounds(1UL, 3UL));
    receiverRti->setRangeBounds(
        subscriptionRegion, receiverY, RangeBounds(1UL, 3UL));
    receiverRti->commitRegionModifications(
        RegionHandleSet{subscriptionRegion});
    receiverRti->subscribeInteractionClassWithRegions(
        interaction, RegionHandleSet{subscriptionRegion});
    receiverRti->setConveyRegionDesignatorSetsSwitch(true);

    receiverCallbacks.receiver = receiverRti.get();
    receiverCallbacks.publisher = senderRti.get();
    receiverCallbacks.sourceRegion = sourceRegion;
    receiverCallbacks.publisherDimension = senderX;
    receiverCallbacks.receiverDimension = receiverX;
    receiverCallbacks.expectedDimensions =
        rti1516_2025::DimensionHandleSet{receiverX, receiverY};
    senderRti->sendInteractionWithRegions(
        senderInteraction,
        rti1516_2025::ParameterHandleValueMap{},
        RegionHandleSet{sourceRegion},
        rti1516_2025::VariableLengthData{});

    for (unsigned attempt = 0U;
         attempt != 64U &&
         receiverCallbacks.callbacks.load(std::memory_order_acquire) == 0U;
         ++attempt) {
      static_cast<void>(receiverRti->evokeCallback(0.05));
    }

    senderRti->resignFederationExecution(NO_ACTION);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (receiverJoined) {
      try {
        receiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      senderRti->disconnect();
    } catch (...) {
    }
    try {
      receiverRti->disconnect();
    } catch (...) {
    }
  }

  listener->close();
  if (server.joinable()) {
    server.join();
  }
  listener.reset();
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(receiverCallbacks.callbacks.load(std::memory_order_acquire) == 1U);
  REQUIRE(receiverCallbacks.callbackFailure == nullptr);
  REQUIRE(receiverCallbacks.conveyedRegion.has_value());
  REQUIRE(receiverCallbacks.conveyedDimensions ==
          receiverCallbacks.expectedDimensions);
  REQUIRE(receiverCallbacks.beforeMutation.has_value());
  REQUIRE(receiverCallbacks.afterMutation.has_value());
  CHECK(receiverCallbacks.beforeMutation->getLowerBound() == 1UL);
  CHECK(receiverCallbacks.beforeMutation->getUpperBound() == 3UL);
  CHECK(receiverCallbacks.afterMutation->getLowerBound() == 1UL);
  CHECK(receiverCallbacks.afterMutation->getUpperBound() == 3UL);
}

TEST_CASE(
    "Configured-process HLA_EVOKED timestamped Reflect Attribute Values retains a send-time callback region snapshot",
    "[integration][development-profile][process-boundary][time-management]"
    "[ddm][ddm-process-tso-region-callback-snapshot]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.get-range-bounds]"
    "[rti.service.get-dimension-handle-set]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values][callback-evoked]"
    "[requirement-candidate-content-clauses-09-data-distribution-management-page-225-l96-30]"
    "[2025]") {
  constexpr wchar_t const* federationName =
      L"process-ddm-tso-reflect-region-snapshot";
  LibXml2FomValidator validator;
  LibXml2FomModuleComposer composer(
      process_fom::processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  ReferenceLogicalTimeSelector timeSelector;
  FederationManagementCoordinator coordinator(
      validator,
      composer,
      timeSelector,
      FederationManagementResources{
          process_fom::processResourcePath("mim/HLAstandardMIM-2025.xml"),
          process_fom::processResourcePath("schemas/IEEE1516-DIF-2025.xsd"),
          std::nullopt});
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.createFomPreparation =
      [&coordinator](std::vector<std::wstring> const& fomModules,
                     std::optional<std::wstring> const& mimModule,
                     std::wstring const& logicalTimeImplementationName)
      -> std::optional<FederationDefinition> {
    auto prepared = coordinator.prepareCreate(
        fomModules, mimModule, logicalTimeImplementationName);
    if (prepared.status != FederationPreparationStatus::applied ||
        !prepared.definition) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };
  EmbeddedFederationRegistry registry;
  auto service = std::make_shared<ProcessFederationService>(
      registry,
      regionalProcessDefinition(),
      std::move(serviceOptions));
  ProcessFederationServiceHost host(
      service,
      {"127.0.0.1", 0U},
      {"process-ddm-tso-snapshot-host", 0xA940U});
  host.start();
  REQUIRE(host.address().port != 0U);

  ProcessTimestampedReflectionSnapshotReceiver receiverCallbacks;
  NullFederateAmbassador publisherCallbacks;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const configuration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-ddm-region-callback-snapshot")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" +
                                     std::to_wstring(host.address().port));
  std::exception_ptr clientError;
  bool publisherJoined = false;
  bool receiverJoined = false;
  rti1516_2025::ObjectInstanceHandle objectInstance;
  rti1516_2025::RegionHandle sourceRegion;
  rti1516_2025::RegionHandle subscriptionRegion;
  rti1516_2025::AttributeHandleSetRegionHandleSetPairVector publisherPair;
  rti1516_2025::AttributeHandleSetRegionHandleSetPairVector receiverPair;
  rti1516_2025::AttributeHandle publisherAttribute;
  rti1516_2025::DimensionHandle publisherDimension;
  std::optional<RangeBounds> publisherBoundsAfterMutation;
  try {
    if (!publisher->connect(publisherCallbacks, HLA_EVOKED, configuration)
             .addressUsed) {
      throw std::runtime_error(
          "The process DDM TSO publisher did not connect.");
    }
    publisher->createFederationExecution(
        federationName, regionalFom().wstring(), L"HLAinteger64Time");
    publisher->joinFederationExecution(
        L"process-ddm-tso-publisher", L"snapshot-publisher", federationName);
    publisherJoined = true;

    if (!receiver->connect(receiverCallbacks, HLA_EVOKED, configuration)
             .addressUsed) {
      throw std::runtime_error(
          "The process DDM TSO receiver did not connect.");
    }
    receiver->joinFederationExecution(
        L"process-ddm-tso-receiver", L"snapshot-receiver", federationName);
    receiverJoined = true;

    auto const publisherObjectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::two_dimensional_regional_object);
    auto const receiverObjectClass = receiver->getObjectClassHandle(
        fixture_hla::fom::two_dimensional_regional_object);
    publisherAttribute = publisher->getAttributeHandle(
        publisherObjectClass, umbra::test::hla::wide::fixture::value);
    auto const receiverAttribute = receiver->getAttributeHandle(
        receiverObjectClass, umbra::test::hla::wide::fixture::value);
    publisherDimension = publisher->getDimensionHandle(
        fixture_hla::fixture::umbra_region_x);
    auto const publisherY = publisher->getDimensionHandle(
        fixture_hla::fixture::umbra_region_y);
    auto const receiverX = receiver->getDimensionHandle(
        fixture_hla::fixture::umbra_region_x);
    auto const receiverY = receiver->getDimensionHandle(
        fixture_hla::fixture::umbra_region_y);
    publisher->publishObjectClassAttributes(
        publisherObjectClass,
        rti1516_2025::AttributeHandleSet{publisherAttribute});
    publisher->changeDefaultAttributeOrderType(
        publisherObjectClass,
        rti1516_2025::AttributeHandleSet{publisherAttribute},
        rti1516_2025::TIMESTAMP);

    sourceRegion = publisher->createRegion(
        rti1516_2025::DimensionHandleSet{publisherDimension, publisherY});
    subscriptionRegion = receiver->createRegion(
        rti1516_2025::DimensionHandleSet{receiverX, receiverY});
    publisher->setRangeBounds(
        sourceRegion, publisherDimension, RangeBounds(1UL, 3UL));
    publisher->setRangeBounds(sourceRegion, publisherY, RangeBounds(1UL, 3UL));
    publisher->commitRegionModifications(
        RegionHandleSet{sourceRegion});
    receiver->setRangeBounds(
        subscriptionRegion, receiverX, RangeBounds(1UL, 3UL));
    receiver->setRangeBounds(
        subscriptionRegion, receiverY, RangeBounds(1UL, 3UL));
    receiver->commitRegionModifications(
        RegionHandleSet{subscriptionRegion});

    rti1516_2025::AttributeHandleSet const publisherAttributes{
        publisherAttribute};
    publisherPair = {{
        publisherAttributes,
        RegionHandleSet{sourceRegion},
    }};
    receiverPair = {{
        rti1516_2025::AttributeHandleSet{receiverAttribute},
        RegionHandleSet{subscriptionRegion},
    }};
    objectInstance = publisher->registerObjectInstanceWithRegions(
        publisherObjectClass, publisherPair);
    receiver->subscribeObjectClassAttributesWithRegions(
        receiverObjectClass, receiverPair);
    receiver->setConveyRegionDesignatorSetsSwitch(true);

    receiverCallbacks.receiver = receiver.get();
    receiverCallbacks.receiverDimension = receiverX;
    receiverCallbacks.expectedDimensions =
        rti1516_2025::DimensionHandleSet{receiverX, receiverY};

    // A zero-lookahead publisher keeps GALT at the receiver's initial time,
    // so Enable Time Constrained correctly waits for a later publisher grant.
    // Use one tick of lookahead to make the enable callback immediate here.
    publisher->enableTimeRegulation(HLAinteger64Interval(1));
    receiver->enableTimeConstrained();
    for (unsigned attempt = 0U;
         attempt != 64U &&
         !receiverCallbacks.timeConstrainedEnabledReceived.load(
             std::memory_order_acquire);
         ++attempt) {
      static_cast<void>(publisher->evokeCallback(0.01));
      static_cast<void>(receiver->evokeCallback(0.01));
    }
    if (!receiverCallbacks.timeConstrainedEnabledReceived.load(
            std::memory_order_acquire)) {
      throw std::runtime_error(
          "The process receiver did not dispatch timeConstrainedEnabled.");
    }
    receiver->timeAdvanceRequest(HLAinteger64Time(5));

    unsigned char const valueBytes[]{0x31U, 0x42U};
    rti1516_2025::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti1516_2025::VariableLengthData(valueBytes, sizeof(valueBytes)));
    auto const retraction = publisher->updateAttributeValues(
        objectInstance,
        values,
        rti1516_2025::VariableLengthData{},
        HLAinteger64Time(5));
    if (!retraction.isValid()) {
      throw std::runtime_error(
          "The process DDM TSO update returned no retraction handle.");
    }
    publisher->setRangeBounds(
        sourceRegion, publisherDimension, RangeBounds(4UL, 6UL));
    publisher->commitRegionModifications(RegionHandleSet{sourceRegion});
    publisherBoundsAfterMutation = publisher->getRangeBounds(
        sourceRegion, publisherDimension);
    publisher->timeAdvanceRequest(HLAinteger64Time(5));
    for (unsigned attempt = 0U;
         attempt != 128U &&
         receiverCallbacks.callbacks.load(std::memory_order_acquire) == 0U;
         ++attempt) {
      static_cast<void>(publisher->evokeCallback(0.01));
      static_cast<void>(receiver->evokeCallback(0.05));
    }
    for (unsigned attempt = 0U; attempt != 64U; ++attempt) {
      static_cast<void>(publisher->evokeCallback(0.0));
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    receiver->unsubscribeObjectClassAttributesWithRegions(
        receiverObjectClass, receiverPair);
    publisher->unassociateRegionsForUpdates(objectInstance, publisherPair);
    publisher->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
    publisherJoined = false;
    receiver->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    publisher->disconnect();
    receiver->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (publisherJoined) {
      try {
        publisher->resignFederationExecution(
            rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (...) {
      }
    }
    if (receiverJoined) {
      try {
        receiver->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      publisher->disconnect();
    } catch (...) {
    }
    try {
      receiver->disconnect();
    } catch (...) {
    }
  }

  host.stop();
  host.rethrowIfFailed();
  if (clientError) {
    try {
      std::rethrow_exception(clientError);
    } catch (rti1516_2025::Exception const& error) {
      auto const name = error.name();
      auto const message = error.what();
      FAIL("Configured-process setup or callback loop failed with "
           << std::string(name.begin(), name.end()) << ": "
           << std::string(message.begin(), message.end()));
    } catch (std::exception const& error) {
      FAIL("Configured-process setup or callback loop failed: " << error.what());
    } catch (...) {
      FAIL("Configured-process setup or callback loop failed with a non-standard exception.");
    }
  }
  REQUIRE(receiverCallbacks.callbacks.load(std::memory_order_acquire) == 1U);
  REQUIRE(receiverCallbacks.timestampedCallback);
  REQUIRE(receiverCallbacks.callbackFailure == nullptr);
  REQUIRE(receiverCallbacks.conveyedRegion.has_value());
  REQUIRE(receiverCallbacks.conveyedDimensions ==
          receiverCallbacks.expectedDimensions);
  REQUIRE(receiverCallbacks.firstCallbackBounds.has_value());
  REQUIRE(receiverCallbacks.secondCallbackBounds.has_value());
  CHECK(receiverCallbacks.firstCallbackBounds->getLowerBound() == 1UL);
  CHECK(receiverCallbacks.firstCallbackBounds->getUpperBound() == 3UL);
  CHECK(receiverCallbacks.secondCallbackBounds->getLowerBound() == 1UL);
  CHECK(receiverCallbacks.secondCallbackBounds->getUpperBound() == 3UL);
  REQUIRE(publisherBoundsAfterMutation.has_value());
  CHECK(publisherBoundsAfterMutation->getLowerBound() == 4UL);
  CHECK(publisherBoundsAfterMutation->getUpperBound() == 6UL);
}

#endif
