#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "ieee1516_2025_attribute_ownership_test_support.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::VariableLengthData;

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;

using umbra::test::attribute_ownership_2025::ReportingFederateAmbassador;
using umbra::test::attribute_ownership_2025::composedProcessOwnershipDefinition;
using umbra::test::attribute_ownership_2025::makeRti;
using umbra::test::attribute_ownership_2025::nextFederationName;
using umbra::test::attribute_ownership_2025::resourcePath;
using umbra::test::attribute_ownership_2025::variableLengthDataBytes;

}  // namespace

TEST_CASE(
    "RTIambassadors deliver Confirm Divestiture through a configured process endpoint in push receive mode",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-confirm-divestiture-push][public-endpoint]"
    "[negotiated-attribute-ownership-divestiture][confirm-divestiture]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification][2025]") {
  class ConfirmingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    struct RequestReport final {
      ObjectInstanceHandle objectInstance;
      AttributeHandleSet attributes;
      VariableLengthData userSuppliedTag;
    };

    struct State final {
      bool confirmed = false;
      std::vector<RequestReport> requests;
    };

    void requestDivestitureConfirmation(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& attributes,
        VariableLengthData const& userSuppliedTag) override {
      {
        std::lock_guard lock(mutex_);
        state_.requests.push_back({objectInstance, attributes, userSuppliedTag});
      }
      changed_.notify_all();
      if (rti != nullptr) {
        unsigned char const confirmationTagBytes[] = {0xE1, 0x6A, 0xB4, 0x2D};
        VariableLengthData const confirmationTag(
            confirmationTagBytes, sizeof(confirmationTagBytes));
        rti->confirmDivestiture(
            objectInstance, attributes, confirmationTag);
        {
          std::lock_guard lock(mutex_);
          state_.confirmed = true;
        }
        changed_.notify_all();
      }
    }

    [[nodiscard]] State snapshot() const {
      std::lock_guard lock(mutex_);
      return state_;
    }

    [[nodiscard]] bool waitForConfirmation() {
      std::unique_lock lock(mutex_);
      return changed_.wait_for(lock, std::chrono::seconds(2),
                               [&] { return state_.confirmed; });
    }

    RTIambassador* rti = nullptr;
   private:
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    State state_;
  } ownerReports;

  class PushReceivingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    using ObjectDiscoveryReport =
        ReportingFederateAmbassador::ObjectDiscoveryReport;
    using AcquisitionReport = ReportingFederateAmbassador::AcquisitionReport;

    struct State final {
      std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
      std::vector<AcquisitionReport> acquisitionReports;
    };

    void discoverObjectInstance(
        ObjectInstanceHandle const& objectInstance,
        ObjectClassHandle const& objectClass,
        std::wstring const&,
        FederateHandle const&) override {
      {
        std::lock_guard lock(mutex_);
        state_.objectDiscoveryReports.push_back({objectInstance, objectClass});
      }
      changed_.notify_all();
    }

    void attributeOwnershipAcquisitionNotification(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& securedAttributes,
        VariableLengthData const& userSuppliedTag) override {
      {
        std::lock_guard lock(mutex_);
        state_.acquisitionReports.push_back({
            AcquisitionReport::Kind::notification,
            objectInstance,
            securedAttributes,
            userSuppliedTag,
        });
      }
      changed_.notify_all();
    }

    [[nodiscard]] State snapshot() const {
      std::lock_guard lock(mutex_);
      return state_;
    }

    [[nodiscard]] bool waitForDiscovery() {
      std::unique_lock lock(mutex_);
      return changed_.wait_for(lock, std::chrono::seconds(2), [&] {
        return !state_.objectDiscoveryReports.empty();
      });
    }

    [[nodiscard]] bool waitForAcquisition() {
      std::unique_lock lock(mutex_);
      return changed_.wait_for(lock, std::chrono::seconds(2), [&] {
        return !state_.acquisitionReports.empty();
      });
    }

   private:
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    State state_;
  } requesterReports;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions;
      serviceOptions.pushReceiveOrderEvents = true;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition(), serviceOptions);

      auto ownerConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-push-server", 0xA71DU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& processSession,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description) {
        bool expectedOperationServed = false;
        std::size_t callbackPollCount = 0U;
        while (!expectedOperationServed) {
          umbra::detail::TransportServiceMessage response;
          if (!ProcessTransportServiceDispatcher::serveOne(
                  processSession,
                  [&](umbra::detail::TransportServiceMessage const& request) {
                    if ((request.operation ==
                             umbra::detail::TransportServiceOperation::receive_interaction &&
                         operation !=
                             umbra::detail::TransportServiceOperation::receive_interaction) ||
                        (request.operation ==
                             umbra::detail::TransportServiceOperation::claim_attribute_ownership_release &&
                         &processSession == &ownerSession)) {
                      if (++callbackPollCount > 256U) {
                        throw std::runtime_error(
                            "The push Confirm Divestiture client exceeded the bounded callback poll allowance.");
                      }
                      response = handler(request);
                      return response;
                    }
                    if (request.operation != operation) {
                      throw std::runtime_error(description);
                    }
                    expectedOperationServed = true;
                    response = handler(request);
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
          if (response.status != umbra::detail::TransportServiceStatus::ok) {
            throw std::runtime_error(description);
          }
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The push Confirm Divestiture server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The push Confirm Divestiture server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-push-server", 0xA71EU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The push Confirm Divestiture server lost requester Join.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The push Confirm Divestiture server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The push Confirm Divestiture server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The push Confirm Divestiture server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The push Confirm Divestiture server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The push Confirm Divestiture server lost owner Register.");
      // In push mode the discovery event is already on the requester socket;
      // this ordinary lookup is the HLA_IMMEDIATE polling fence that receives
      // and dispatches it before returning its handle.
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost requester discovery fence.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The push Confirm Divestiture server lost requester Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The push Confirm Divestiture server lost Acquisition.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::negotiated_attribute_ownership_divestiture,
          "The push Confirm Divestiture server lost Negotiated Divestiture.");
      // The confirmation request is pushed before the negotiated response is
      // returned.  The client dispatches it after that response, then the
      // re-entrant public callback issues this request on the same session.
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::confirm_divestiture,
          "The push Confirm Divestiture server lost Confirm Divestiture.");
      serveExpected(ownerSession, ownerHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The push Confirm Divestiture server lost its service-report append.");
      // The requester lookup consumes the pushed acquisition notification.
      serveExpected(requesterSession, requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost requester acquisition fence.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The push Confirm Divestiture server lost requester Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The push Confirm Divestiture server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto owner = makeRti();
  auto requester = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-confirm-divestiture-push-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-confirm-divestiture-push-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto const federationName =
      nextFederationName() + L"-process-confirm-divestiture-push";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_IMMEDIATE, ownerConfiguration));
    ownerReports.rti = owner.get();
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-confirm-divestiture-push-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_IMMEDIATE, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-confirm-divestiture-push-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    // The push discovery may arrive before this lookup begins. Keep the
    // lookup as an observation fence without requiring delayed delivery.
    REQUIRE_NOTHROW(static_cast<void>(requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(requesterReports.waitForDiscovery());
    auto requesterState = requesterReports.snapshot();
    REQUIRE(requesterState.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterState.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE(requesterState.objectDiscoveryReports.front().objectClass ==
            requesterClass);

    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));
    unsigned char const negotiatedTagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const negotiatedTag(
        negotiatedTagBytes, sizeof(negotiatedTagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        negotiatedTag));
    REQUIRE(requesterReports.snapshot().acquisitionReports.empty());
    REQUIRE(ownerReports.snapshot().requests.empty());
    REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute},
        negotiatedTag));
    REQUIRE(ownerReports.waitForConfirmation());
    auto const ownerState = ownerReports.snapshot();
    REQUIRE(ownerState.requests.size() == 1U);
    REQUIRE(ownerState.requests.front().objectInstance == objectInstance);
    REQUIRE(ownerState.requests.front().attributes ==
            AttributeHandleSet{ownerAttribute});
    REQUIRE(variableLengthDataBytes(
                ownerState.requests.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                negotiatedTagBytes,
                negotiatedTagBytes + sizeof(negotiatedTagBytes)));
    REQUIRE(ownerState.confirmed);

    // Confirm pushes the acquisition notification to this HLA_IMMEDIATE
    // requester. It may have arrived already; the lookup is an observation
    // fence, not a required trigger for callback delivery.
    REQUIRE_NOTHROW(static_cast<void>(requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(requesterReports.waitForAcquisition());
    requesterState = requesterReports.snapshot();
    REQUIRE(requesterState.acquisitionReports.size() == 1U);
    REQUIRE(requesterState.acquisitionReports.front().kind ==
            ReportingFederateAmbassador::AcquisitionReport::Kind::notification);
    REQUIRE(requesterState.acquisitionReports.front().objectInstance ==
            objectInstance);
    REQUIRE(requesterState.acquisitionReports.front().attributes ==
            AttributeHandleSet{requesterAttribute});
    unsigned char const confirmationTagBytes[] = {0xE1, 0x6A, 0xB4, 0x2D};
    REQUIRE(variableLengthDataBytes(
                requesterState.acquisitionReports.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                confirmationTagBytes,
                confirmationTagBytes + sizeof(confirmationTagBytes)));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
      } catch (...) {
      }
    }
    if (ownerJoined) {
      try {
        owner->resignFederationExecution(
            rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (...) {
      }
    }
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
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
}

#endif
