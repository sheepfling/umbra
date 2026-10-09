#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

namespace {
using rti1516_2025::CallbackModel;
using rti1516_2025::DELETE_OBJECTS;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::NO_ACTION;
using rti1516_2025::RtiConfiguration;
using TestFederateAmbassador = NullFederateAmbassador;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::process_fom_support_2025::composedProcessDefinition;
}  // namespace
TEST_CASE(
    "RTIambassador delivers regional Attribute Relevance Advisory transitions through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][regional-attribute-relevance][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.create-region][rti.service.commit-region-modifications][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.associate-regions-for-updates][rti.service.unassociate-regions-for-updates][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator = updateRateDesignator;
      }

      void turnUpdatesOffForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOffCount;
        turnedOffObjectInstance = objectInstance;
        turnedOffAttributes = attributes;
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::optional<std::wstring> turnedOnUpdateRateDesignator;
      std::size_t turnedOffCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOffAttributes;
    } ownerFederate;
    TestFederateAmbassador receiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-regional-attribute-relevance-execution";
    constexpr wchar_t const* objectClassName =
        L"HLAobjectRoot.Food.Drink.Soda";
    constexpr wchar_t const* attributeName = L"Flavor";
    constexpr wchar_t const* dimensionName = L"SodaFlavor";
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{true});
        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-server", 0xA601U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderHandler = service.handlerFor(sender);
        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
                                 char const* description) {
          if (!umbra::test::servePrimaryProcessRequest(
                  session, handler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation != operation) {
                      throw std::runtime_error(description);
                    }
                    return handler(request);
                  })) {
            throw std::runtime_error(description);
          }
        };

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_federation_execution,
            "The public regional advisory server lost Create.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost owner Join.");
        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-server", 0xA602U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost receiver Join.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Set.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Get.");

        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
             }) {
          serveExpected(
              sender,
              senderHandler,
              operation,
              "The public regional advisory server lost owner DDM setup.");
        }
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::subscribe_object_class_attributes_with_regions,
             }) {
          serveExpected(
              receiver,
              receiverHandler,
              operation,
              "The public regional advisory server lost receiver DDM setup.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The public regional advisory server lost Publish.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::register_object_instance_with_regions,
            "The public regional advisory server lost regional Register.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::get_object_class_handle,
              "The public regional advisory server lost immediate discovery polling.");
        } else {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The public regional advisory server lost discovery Receive.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The public regional advisory server lost disjoint-region Associate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::unassociate_regions_for_updates,
            "The public regional advisory server lost source-region Unassociate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The public regional advisory server lost source-region Associate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost owner Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost receiver Resign.");
        service.detach(sender);
        service.detach(receiver);
        senderConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto receiverRti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-regional-attribute-relevance-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-owner",
          L"public-process-regional-attribute-relevance-type",
          federationName);
      ownerJoined = true;

      REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
      receiverRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-receiver",
          L"public-process-regional-attribute-relevance-type",
          federationName);
      receiverJoined = true;
      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass =
          ownerRti->getObjectClassHandle(objectClassName);
      auto const ownerAttribute =
          ownerRti->getAttributeHandle(ownerObjectClass, attributeName);
      auto const ownerDimension = ownerRti->getDimensionHandle(dimensionName);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerAttribute.isValid());
      REQUIRE(ownerDimension.isValid());

      auto const ownerRegion =
          ownerRti->createRegion(rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(ownerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          ownerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{ownerRegion}));
      auto const disjointOwnerRegion =
          ownerRti->createRegion(rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(disjointOwnerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          disjointOwnerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(2UL, 3UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{disjointOwnerRegion}));

      auto const receiverObjectClass =
          receiverRti->getObjectClassHandle(objectClassName);
      auto const receiverAttribute =
          receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
      auto const receiverDimension = receiverRti->getDimensionHandle(dimensionName);
      REQUIRE(receiverObjectClass == ownerObjectClass);
      REQUIRE(receiverAttribute == ownerAttribute);
      REQUIRE(receiverDimension == ownerDimension);
      auto const receiverRegion =
          receiverRti->createRegion(rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion,
          receiverDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));

      rti1516_2025::AttributeHandleSet const ownerAttributes{ownerAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
          rti1516_2025::AttributeHandleSet{receiverAttribute},
          rti1516_2025::RegionHandleSet{receiverRegion},
      }};
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass,
          receiverPairs,
          true,
          L"High"));
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass,
          ownerAttributes));
      auto const objectInstance = ownerRti->registerObjectInstanceWithRegions(
          ownerObjectClass,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }});
      REQUIRE(objectInstance.isValid());
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }

      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});
      ownerFederate.turnedOnCount = 0U;
      ownerFederate.turnedOnObjectInstance = {};
      ownerFederate.turnedOnAttributes.clear();
      ownerFederate.turnedOnUpdateRateDesignator.reset();

      // A disjoint association does not change effective relevance while the
      // original overlap remains active.
      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{disjointOwnerRegion},
          }}));
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Removing the last overlapping association crosses the edge to Off.
      REQUIRE_NOTHROW(ownerRti->unassociateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 1U);
      REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOffAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Restoring the overlap crosses the edge to On and retains High.
      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      ownerRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
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
        ownerRti->disconnect();
      } catch (...) {
      }
      try {
        receiverRti->disconnect();
      } catch (...) {
      }
    }
    if (listener) {
      listener.reset();
    }
    if (server.joinable()) {
      server.join();
    }
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}
#endif
