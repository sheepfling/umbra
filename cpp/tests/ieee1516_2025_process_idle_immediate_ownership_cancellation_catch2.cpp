#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "ieee1516_2025_attribute_ownership_test_support.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"

#include <chrono>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace fixture_hla = umbra::test::hla::wide;
using namespace std::chrono_literals;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceOperation;
using umbra::test::attribute_ownership_2025::composedProcessOwnershipDefinition;
using umbra::test::attribute_ownership_2025::makeRti;
using umbra::test::attribute_ownership_2025::nextFederationName;
using umbra::test::attribute_ownership_2025::resourcePath;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::VariableLengthData;

class IdleCancellationObserver final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct OwnershipEvent final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    {
      std::lock_guard lock(mutex_);
      discovered_.push_back(objectInstance);
    }
    changed_.notify_all();
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const&) override {
    {
      std::lock_guard lock(mutex_);
      releases_.push_back({objectInstance, attributes});
    }
    changed_.notify_all();
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    {
      std::lock_guard lock(mutex_);
      confirmations_.push_back({objectInstance, attributes});
    }
    changed_.notify_all();
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const&) override {
    {
      std::lock_guard lock(mutex_);
      acquisitions_.push_back({objectInstance, attributes});
    }
    changed_.notify_all();
  }

  bool waitForDiscovery(std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [this] {
      return !discovered_.empty();
    });
  }

  bool waitForRelease(std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [this] {
      return !releases_.empty();
    });
  }

  bool waitForConfirmation(std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [this] {
      return !confirmations_.empty();
    });
  }

  std::vector<ObjectInstanceHandle> discoveries() const {
    std::lock_guard lock(mutex_);
    return discovered_;
  }

  std::vector<OwnershipEvent> releases() const {
    std::lock_guard lock(mutex_);
    return releases_;
  }

  std::vector<OwnershipEvent> confirmations() const {
    std::lock_guard lock(mutex_);
    return confirmations_;
  }

  std::vector<OwnershipEvent> acquisitions() const {
    std::lock_guard lock(mutex_);
    return acquisitions_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  std::vector<ObjectInstanceHandle> discovered_;
  std::vector<OwnershipEvent> releases_;
  std::vector<OwnershipEvent> confirmations_;
  std::vector<OwnershipEvent> acquisitions_;
};

}  // namespace

TEST_CASE(
    "RTIambassadors confirm process acquisition cancellation while requester is idle",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-acquisition-cancellation-idle-push]"
    "[public-endpoint][attribute-ownership-acquisition]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    std::shared_ptr<umbra::detail::ProcessTransportConnection> ownerConnection;
    std::shared_ptr<umbra::detail::ProcessTransportConnection> requesterConnection;
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition(),
          ProcessFederationServiceOptions{true});
      ownerConnection = listener->accept(
          nullptr, {"process-idle-immediate-ownership-cancellation", 0xA70DU},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session, [&](auto const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::create_federation_execution,
                    "Idle cancellation server lost owner Create.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::join_federation_execution,
                    "Idle cancellation server lost owner Join.");

      requesterConnection = listener->accept(
          nullptr, {"process-idle-immediate-ownership-cancellation", 0xA70EU},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(requesterSession, requesterHandler,
                    TransportServiceOperation::join_federation_execution,
                    "Idle cancellation server lost requester Join.");

      auto serveUntilResign = [](ProcessTransportSession& session,
                                 auto const& handler) {
        for (std::size_t operations = 0U; operations < 64U; ++operations) {
          bool resigned = false;
          if (!ProcessTransportServiceDispatcher::serveOne(
                  session, [&](auto const& request) {
                    if (request.operation ==
                        TransportServiceOperation::receive_interaction) {
                      throw std::runtime_error(
                          "An idle HLA_IMMEDIATE cancellation client polled callbacks.");
                    }
                    resigned = request.operation ==
                        TransportServiceOperation::resign_federation_execution;
                    return handler(request);
                  })) {
            throw std::runtime_error(
                "Idle cancellation server lost the process service stream.");
          }
          if (resigned) {
            return;
          }
        }
        throw std::runtime_error(
            "Idle cancellation server exceeded its bounded service allowance.");
      };
      std::exception_ptr ownerWorkerError;
      std::thread ownerWorker([&] {
        try {
          serveUntilResign(ownerSession, ownerHandler);
        } catch (...) {
          ownerWorkerError = std::current_exception();
          requesterConnection->close();
          ownerConnection->close();
        }
      });
      try {
        serveUntilResign(requesterSession, requesterHandler);
      } catch (...) {
        requesterConnection->close();
        ownerConnection->close();
        ownerWorker.join();
        if (ownerWorkerError) {
          std::rethrow_exception(ownerWorkerError);
        }
        throw;
      }
      ownerWorker.join();
      if (ownerWorkerError) {
        std::rethrow_exception(ownerWorkerError);
      }
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
      if (requesterConnection) {
        requesterConnection->close();
      }
      if (ownerConnection) {
        ownerConnection->close();
      }
    }
  });

  auto const federationName =
      nextFederationName() + L"-process-idle-immediate-ownership-cancellation";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  IdleCancellationObserver ownerReports;
  IdleCancellationObserver requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto makeConfiguration = [port](std::wstring name) {
    auto configuration = rti1516_2025::RtiConfiguration::createConfiguration();
    configuration.withConfigurationName(name);
    configuration.withRtiAddress(
        L"tcp://127.0.0.1:" + std::to_wstring(port));
    return configuration;
  };
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, rti1516_2025::HLA_IMMEDIATE,
        makeConfiguration(L"process-idle-immediate-cancellation-owner")));
    REQUIRE_NOTHROW(owner->createFederationExecution(federationName, fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-idle-immediate-cancellation-owner", L"publisher",
        federationName));
    ownerJoined = true;
    REQUIRE_NOTHROW(requester->connect(
        requesterReports, rti1516_2025::HLA_IMMEDIATE,
        makeConfiguration(L"process-idle-immediate-cancellation-requester")));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-idle-immediate-cancellation-requester", L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass, fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass, AttributeHandleSet{requesterAttribute}));
    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass, fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass, AttributeHandleSet{ownerAttribute}));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());

    REQUIRE(requesterReports.waitForDiscovery(3s));
    REQUIRE(requesterReports.discoveries() ==
            std::vector<ObjectInstanceHandle>{objectInstance});
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass, AttributeHandleSet{requesterAttribute}));
    unsigned char const acquisitionBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const acquisitionTag(
        acquisitionBytes, sizeof(acquisitionBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance, AttributeHandleSet{requesterAttribute},
        acquisitionTag));

    // Observe the owner's release request before cancellation. This case does
    // not exercise cancellation against a still-buffered release event.
    REQUIRE(ownerReports.waitForRelease(3s));
    auto const releases = ownerReports.releases();
    REQUIRE(releases.size() == 1U);
    REQUIRE(releases.front().objectInstance == objectInstance);
    REQUIRE(releases.front().attributes == AttributeHandleSet{ownerAttribute});

    REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
        objectInstance, AttributeHandleSet{requesterAttribute}));

    // There is no requester RTI service or Evoke call between cancellation
    // and this condition-variable wait for the pushed callback.
    REQUIRE(requesterReports.waitForConfirmation(3s));
    auto const confirmations = requesterReports.confirmations();
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(confirmations.front().objectInstance == objectInstance);
    REQUIRE(confirmations.front().attributes ==
            AttributeHandleSet{requesterAttribute});
    REQUIRE(requesterReports.acquisitions().empty());
    REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, ownerAttribute));
    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        objectInstance, requesterAttribute));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
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
            rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
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
  // Keep the listener object alive while accept() unwinds on a failed client
  // setup; destroying it before joining the server races its socket use.
  listener->close();
  if (server.joinable()) {
    server.join();
  }
  listener.reset();
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  if (serverError) {
    std::rethrow_exception(serverError);
  }
}
