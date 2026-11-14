#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_service_host.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace {

using rti1516_2025::NullFederateAmbassador;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomModuleKind;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationClient;
using umbra::detail::ProcessFederationServiceHost;

FederationDefinition recoveryFederationDefinition() {
  auto module = [](wchar_t const* designator, wchar_t const* source) {
    return PrevalidatedFomModule{
        designator,
        source,
        L"C:/fom/IEEE1516-DIF-2025.xsd",
        FomModuleKind::fom,
        L"IEEE1516-DIF-2025.xsd"};
  };
  return {
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/extensions.xml", L"C:/fom/extensions.xml"),
      },
      L"HLAinteger64Time",
  };
}

class RecoverySynchronizationPointAmbassador final
    : public NullFederateAmbassador {
 public:
  void announceSynchronizationPoint(
      std::wstring const& label,
      rti1516_2025::VariableLengthData const& tag) override {
    ++announcementCount;
    callbackOrder.push_back(L"announceSynchronizationPoint");
    announcementLabel = label;
    announcementTag.clear();
    if (tag.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(tag.data());
      announcementTag.assign(data, data + tag.size());
    }
  }

  void federationSynchronized(
      std::wstring const& label,
      rti1516_2025::FederateHandleSet const& failedToSyncSet) override {
    callbackOrder.push_back(L"federationSynchronized");
    synchronizedLabel = label;
    failedToSyncCount = failedToSyncSet.size();
  }

  std::size_t announcementCount = 0U;
  std::wstring announcementLabel;
  std::vector<std::uint8_t> announcementTag;
  std::vector<std::wstring> callbackOrder;
  std::wstring synchronizedLabel;
  std::size_t failedToSyncCount = 0U;
};

}  // namespace

TEST_CASE(
    "Process federation client delivers a host synchronization callback after socket recovery",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-host-generated-callback]") {
  EmbeddedFederationRegistry registry;
  umbra::detail::ProcessFederationServiceOptions serviceOptions;
  serviceOptions.pushReceiveOrderEvents = false;
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-recovery-callback-test", 0xB817U},
      serviceOptions);
  host.start();

  ProcessFederationClient registrar(
      {"127.0.0.1", host.address().port},
      {"umbra-process-recovery-callback-registrar", 0U});
  registrar.createFederationExecution(L"recovery-callback-execution");
  auto const registeringFederate = registrar.joinFederationExecution(
      L"recovery-callback-execution",
      L"recovery-callback-registrar",
      L"recovery-callback-registrar");
  REQUIRE(registeringFederate.federateId != 0U);

  ProcessFederationClient receiver(
      {"127.0.0.1", host.address().port},
      {"umbra-process-recovery-callback-receiver", 0U});
  auto const receivingFederate = receiver.joinFederationExecution(
      L"recovery-callback-execution",
      L"recovery-callback-receiver",
      L"recovery-callback-receiver");
  REQUIRE(receivingFederate.federateId != 0U);

  RecoverySynchronizationPointAmbassador ambassador;
  receiver.attachCallbackBridge(ambassador);
  receiver.disconnectTransportForTesting();

  std::vector<std::uint8_t> const tag{0x2AU, 0x7FU};
  auto const registration = registrar.registerFederationSynchronizationPoint(
      L"recovery-callback-execution",
      registeringFederate.federateId,
      L"announced-during-recovery",
      tag,
      {},
      false);
  REQUIRE(registration.succeeded);

  auto const poll = receiver.receiveInteraction(
      L"recovery-callback-execution", receivingFederate.federateId);
  REQUIRE_FALSE(poll.interaction.has_value());
  REQUIRE(poll.bufferedOtherEvent);
  static_cast<void>(receiver.evokeOne(std::chrono::seconds(1)));
  REQUIRE(ambassador.announcementCount == 1U);
  REQUIRE(ambassador.announcementLabel == L"announced-during-recovery");
  REQUIRE(ambassador.announcementTag == tag);

  receiver.resignFederationExecution(
      L"recovery-callback-execution",
      receivingFederate.federateId,
      rti1516_2025::NO_ACTION);
  registrar.resignFederationExecution(
      L"recovery-callback-execution",
      registeringFederate.federateId,
      rti1516_2025::NO_ACTION);
  receiver.close();
  registrar.close();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation client reports terminal loss after its bounded recovery window",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][bounded-recovery-window]") {
  auto listener = umbra::detail::ProcessTransportListener::listen(
      {"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const address = listener->address();
  auto serverConnectionFuture = std::async(
      std::launch::async,
      [&listener] {
        return listener->accept(
            nullptr,
            {"umbra-process-bounded-recovery-service", 0xB819U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
      });

  std::promise<std::wstring> failurePromise;
  auto failureFuture = failurePromise.get_future();
  std::atomic_bool failureReported{false};
  std::promise<void> receiveLoopStarted;
  auto receiveLoopFuture = receiveLoopStarted.get_future();
  std::atomic_bool receiveLoopStartReported{false};
  umbra::detail::ProcessFederationClientOptions clientOptions;
  clientOptions.transportRecoveryWindow = std::chrono::milliseconds(250);
  clientOptions.beforeTransportReadForTesting = [&] {
    if (!receiveLoopStartReported.exchange(true)) {
      receiveLoopStarted.set_value();
    }
  };
  ProcessFederationClient client(
      {"127.0.0.1", address.port},
      {"umbra-process-bounded-recovery-client", 0U},
      {},
      [&](std::wstring description) {
        if (!failureReported.exchange(true)) {
          failurePromise.set_value(std::move(description));
        }
      },
      {},
      std::move(clientOptions));

  auto serverConnection = serverConnectionFuture.get();
  REQUIRE(receiveLoopFuture.wait_for(std::chrono::seconds(1)) ==
          std::future_status::ready);
  listener->close();
  serverConnection->close();
  REQUIRE(failureFuture.wait_for(std::chrono::seconds(3)) ==
          std::future_status::ready);
  REQUIRE_FALSE(failureFuture.get().empty());
  client.close();
}

TEST_CASE(
    "Process federation client recovers an in-flight Join response from host history",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-host-inflight-service-request]") {
  EmbeddedFederationRegistry registry;
  std::atomic_bool pauseNextJoinResponse{false};
  std::atomic_uint32_t joinHandlerCallCount{0U};
  std::promise<void> joinHandlerEntered;
  std::promise<void> releaseJoinHandler;
  auto const releaseJoinHandlerFuture = releaseJoinHandler.get_future().share();
  umbra::detail::ProcessFederationServiceOptions serviceOptions;
  serviceOptions.afterJoinedFederateSessionInstalledForTesting = [&] {
    joinHandlerCallCount.fetch_add(1U, std::memory_order_relaxed);
    if (pauseNextJoinResponse.exchange(false, std::memory_order_acq_rel)) {
      joinHandlerEntered.set_value();
      releaseJoinHandlerFuture.wait();
    }
  };
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-inflight-join-test", 0xB818U},
      serviceOptions);
  host.start();

  ProcessFederationClient registrar(
      {"127.0.0.1", host.address().port},
      {"umbra-process-inflight-join-registrar", 0U});
  registrar.createFederationExecution(L"inflight-join-execution");
  auto const registeringFederate = registrar.joinFederationExecution(
      L"inflight-join-execution",
      L"inflight-join-registrar",
      L"inflight-join-registrar");
  REQUIRE(registeringFederate.federateId != 0U);

  std::atomic_uint32_t reportedTransportFailures{0U};
  ProcessFederationClient recoveringClient(
      {"127.0.0.1", host.address().port},
      {"umbra-process-inflight-join-client", 0U},
      {},
      [&reportedTransportFailures](std::wstring) {
        reportedTransportFailures.fetch_add(1U, std::memory_order_relaxed);
      });
  auto const joinCallsBeforeRecovery =
      joinHandlerCallCount.load(std::memory_order_relaxed);
  auto joinHandlerEnteredFuture = joinHandlerEntered.get_future();
  pauseNextJoinResponse.store(true, std::memory_order_release);
  auto joinFuture = std::async(std::launch::async, [&recoveringClient] {
    return recoveringClient.joinFederationExecution(
        L"inflight-join-execution",
        L"inflight-join-recovering-federate",
        L"inflight-join-recovering-federate");
  });

  if (joinHandlerEnteredFuture.wait_for(std::chrono::seconds(5)) !=
      std::future_status::ready) {
    releaseJoinHandler.set_value();
    recoveringClient.close();
    FAIL("the service host did not reach the in-flight Join response gate");
  }
  recoveringClient.disconnectTransportForTesting();
  releaseJoinHandler.set_value();

  if (joinFuture.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
    recoveringClient.close();
    static_cast<void>(joinFuture.get());
    FAIL("the client did not recover its in-flight Join response");
  }
  auto const recoveredJoin = joinFuture.get();
  REQUIRE(recoveredJoin.federateId != 0U);
  REQUIRE(joinHandlerCallCount.load(std::memory_order_relaxed) ==
          joinCallsBeforeRecovery + 1U);
  REQUIRE(registry.memberById(
      L"inflight-join-execution", recoveredJoin.federateId).has_value());
  auto const members = recoveringClient.listFederationExecutionMembers(
      L"inflight-join-execution");
  REQUIRE(members.federationExists);
  REQUIRE(members.members.size() == 2U);
  bool recoveringFederateWasListed = false;
  for (auto const& member : members.members) {
    recoveringFederateWasListed = recoveringFederateWasListed ||
        member.name == L"inflight-join-recovering-federate";
  }
  REQUIRE(recoveringFederateWasListed);
  REQUIRE(reportedTransportFailures.load(std::memory_order_relaxed) == 0U);

  recoveringClient.resignFederationExecution(
      L"inflight-join-execution",
      recoveredJoin.federateId,
      rti1516_2025::NO_ACTION);
  registrar.resignFederationExecution(
      L"inflight-join-execution",
      registeringFederate.federateId,
      rti1516_2025::NO_ACTION);
  recoveringClient.close();
  registrar.close();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation client replays pushed callbacks around an interleaved service response across repeated socket loss",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-pushed-event-frame]"
    "[resume-interleaved-pushed-frame]") {
  EmbeddedFederationRegistry registry;
  std::promise<void> synchronizedEventPushed;
  auto synchronizedEventPushedFuture = synchronizedEventPushed.get_future();
  umbra::detail::ProcessFederationServiceOptions serviceOptions;
  serviceOptions.pushReceiveOrderEvents = true;
  serviceOptions.afterFederationSynchronizedEventSentForTesting =
      [&synchronizedEventPushed](
          std::uint64_t, std::wstring const&) {
        synchronizedEventPushed.set_value();
      };
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-pushed-frame-recovery-test", 0xB819U},
      serviceOptions);
  host.start();

  ProcessFederationClient registrar(
      {"127.0.0.1", host.address().port},
      {"umbra-process-pushed-frame-registrar", 0U});
  registrar.createFederationExecution(L"pushed-frame-recovery-execution");
  auto const registeringFederate = registrar.joinFederationExecution(
      L"pushed-frame-recovery-execution",
      L"pushed-frame-registrar",
      L"pushed-frame-registrar");
  REQUIRE(registeringFederate.federateId != 0U);

  std::mutex receiveGateMutex;
  std::condition_variable receiveGateChanged;
  std::size_t receiveGatesRequested = 0U;
  std::size_t receiveGatesEntered = 0U;
  std::size_t receiveGatesReleased = 0U;
  auto armReceiveGate = [&] {
    std::scoped_lock lock(receiveGateMutex);
    return ++receiveGatesRequested;
  };
  auto waitForReceiveGate = [&](std::size_t gateIndex) {
    std::unique_lock lock(receiveGateMutex);
    return receiveGateChanged.wait_for(
        lock,
        std::chrono::seconds(5),
        [&] { return receiveGatesEntered >= gateIndex; });
  };
  auto releaseReceiveGate = [&](std::size_t gateIndex) {
    {
      std::scoped_lock lock(receiveGateMutex);
      if (gateIndex > receiveGatesReleased) {
        receiveGatesReleased = gateIndex;
      }
    }
    receiveGateChanged.notify_all();
  };
  umbra::detail::ProcessFederationClientOptions receiverOptions;
  receiverOptions.beforeTransportReadForTesting = [&] {
    std::unique_lock lock(receiveGateMutex);
    if (receiveGatesEntered >= receiveGatesRequested) {
      return;
    }
    auto const gateIndex = ++receiveGatesEntered;
    receiveGateChanged.notify_all();
    receiveGateChanged.wait(
        lock, [&] { return receiveGatesReleased >= gateIndex; });
  };
  ProcessFederationClient receiver(
      {"127.0.0.1", host.address().port},
      {"umbra-process-pushed-frame-receiver", 0U},
      {},
      {},
      {},
      std::move(receiverOptions));
  auto const receivingFederate = receiver.joinFederationExecution(
      L"pushed-frame-recovery-execution",
      L"pushed-frame-receiver",
      L"pushed-frame-receiver");
  REQUIRE(receivingFederate.federateId != 0U);

  RecoverySynchronizationPointAmbassador ambassador;
  receiver.attachCallbackBridge(ambassador);
  auto activeReceiveGate = armReceiveGate();
  auto memberQuery = std::async(std::launch::async, [&receiver] {
    return receiver.listFederationExecutionMembers(
        L"pushed-frame-recovery-execution");
  });
  bool memberQueryResponseReceived = false;
  for (std::size_t attempt = 0U; attempt < 32U; ++attempt) {
    if (!waitForReceiveGate(activeReceiveGate)) {
      releaseReceiveGate(activeReceiveGate);
      receiver.close();
      try {
        static_cast<void>(memberQuery.get());
      } catch (...) {
      }
      FAIL("the process client did not reach the initial response gate");
    }
    if (memberQuery.wait_for(std::chrono::seconds(5)) ==
        std::future_status::ready) {
      memberQueryResponseReceived = true;
      break;
    }
    auto const nextReceiveGate = armReceiveGate();
    releaseReceiveGate(activeReceiveGate);
    activeReceiveGate = nextReceiveGate;
  }
  if (!memberQueryResponseReceived) {
    releaseReceiveGate(activeReceiveGate);
    receiver.close();
    try {
      static_cast<void>(memberQuery.get());
    } catch (...) {
    }
    FAIL("the member query did not complete at the initial receive gate");
  }
  auto const members = memberQuery.get();

  std::optional<umbra::detail::ProcessFederationRegisterSynchronizationPointResult>
      registration;
  std::exception_ptr registrationFailure;
  try {
    registration = registrar.registerFederationSynchronizationPoint(
        L"pushed-frame-recovery-execution",
        registeringFederate.federateId,
        L"pushed-frame-across-recovery",
        {0x41U, 0x52U},
        {receivingFederate.federateId},
        true);
  } catch (...) {
    registrationFailure = std::current_exception();
  }
  if (registrationFailure) {
    releaseReceiveGate(activeReceiveGate);
    receiver.close();
    std::rethrow_exception(registrationFailure);
  }
  if (!registration.has_value() || !registration->succeeded) {
    releaseReceiveGate(activeReceiveGate);
    receiver.close();
    FAIL("the receiver synchronization point was not registered");
  }
  receiver.disconnectTransportForTesting();
  releaseReceiveGate(activeReceiveGate);

  auto const announcementPoll = receiver.receiveInteraction(
      L"pushed-frame-recovery-execution", receivingFederate.federateId);
  static_cast<void>(receiver.evokeOne(std::chrono::milliseconds::zero()));
  auto const callbackOrderAfterAnnouncement = ambassador.callbackOrder;

  activeReceiveGate = armReceiveGate();
  auto interleavedMemberQuery = std::async(std::launch::async, [&receiver] {
    return receiver.listFederationExecutionMembers(
        L"pushed-frame-recovery-execution");
  });
  bool interleavedResponseReceived = false;
  for (std::size_t attempt = 0U; attempt < 32U; ++attempt) {
    if (!waitForReceiveGate(activeReceiveGate)) {
      releaseReceiveGate(activeReceiveGate);
      receiver.close();
      try {
        static_cast<void>(interleavedMemberQuery.get());
      } catch (...) {
      }
      FAIL("the process client did not reach the interleaved-response gate");
    }
    if (interleavedMemberQuery.wait_for(std::chrono::seconds(5)) ==
        std::future_status::ready) {
      interleavedResponseReceived = true;
      break;
    }
    auto const followingReceiveGate = armReceiveGate();
    releaseReceiveGate(activeReceiveGate);
    activeReceiveGate = followingReceiveGate;
  }
  if (!interleavedResponseReceived) {
    releaseReceiveGate(activeReceiveGate);
    receiver.close();
    try {
      static_cast<void>(interleavedMemberQuery.get());
    } catch (...) {
    }
    FAIL("the interleaved member response was not received before the gate");
  }
  std::optional<umbra::detail::ProcessFederationExecutionMembersResult>
      interleavedMembers;
  try {
    interleavedMembers = interleavedMemberQuery.get();
  } catch (...) {
    releaseReceiveGate(activeReceiveGate);
    receiver.close();
    throw;
  }
  static_cast<void>(receiver.evokeOne(std::chrono::milliseconds::zero()));
  auto const callbackOrderBeforeCompletion = ambassador.callbackOrder;

  auto achievement = std::async(
      std::launch::async, [&receiver, &receivingFederate] {
        return receiver.synchronizationPointAchieved(
            L"pushed-frame-recovery-execution",
            receivingFederate.federateId,
            L"pushed-frame-across-recovery",
            true);
      });
  if (synchronizedEventPushedFuture.wait_for(std::chrono::seconds(5)) !=
      std::future_status::ready) {
    releaseReceiveGate(activeReceiveGate);
    receiver.close();
    try {
      static_cast<void>(achievement.get());
    } catch (...) {
    }
    FAIL("the service did not push the synchronization-completion callback");
  }
  receiver.disconnectTransportForTesting();
  releaseReceiveGate(activeReceiveGate);

  if (achievement.wait_for(std::chrono::seconds(5)) !=
      std::future_status::ready) {
    receiver.close();
    try {
      static_cast<void>(achievement.get());
    } catch (...) {
    }
    FAIL("the process client did not recover the in-flight synchronization achievement");
  }
  auto const achieved = achievement.get();
  REQUIRE(registration.has_value());
  REQUIRE(registration->succeeded);
  REQUIRE(achieved.status ==
          umbra::detail::ProcessFederationSynchronizationPointAchievedStatus::applied);
  REQUIRE(members.federationExists);
  REQUIRE(members.members.size() == 2U);
  REQUIRE(interleavedMembers.has_value());
  REQUIRE(interleavedMembers->federationExists);
  REQUIRE(interleavedMembers->members.size() == 2U);
  std::vector<std::wstring> const expectedAnnouncementOrder{
      L"announceSynchronizationPoint"};
  REQUIRE_FALSE(announcementPoll.interaction.has_value());
  REQUIRE(ambassador.announcementCount == 1U);
  REQUIRE(callbackOrderAfterAnnouncement == expectedAnnouncementOrder);
  REQUIRE(callbackOrderBeforeCompletion == expectedAnnouncementOrder);
  static_cast<void>(receiver.evokeMultiple(
      std::chrono::milliseconds::zero(), std::chrono::seconds(5)));
  REQUIRE(ambassador.announcementCount == 1U);
  REQUIRE(ambassador.announcementLabel == L"pushed-frame-across-recovery");
  std::vector<std::uint8_t> const expectedTag{0x41U, 0x52U};
  REQUIRE(ambassador.announcementTag == expectedTag);
  std::vector<std::wstring> const expectedCallbackOrder{
      L"announceSynchronizationPoint", L"federationSynchronized"};
  REQUIRE(ambassador.callbackOrder == expectedCallbackOrder);
  REQUIRE(ambassador.synchronizedLabel == L"pushed-frame-across-recovery");
  REQUIRE(ambassador.failedToSyncCount == 0U);

  auto const acknowledgmentFence = receiver.listFederationExecutionMembers(
      L"pushed-frame-recovery-execution");
  REQUIRE(acknowledgmentFence.members.size() == 2U);
  receiver.disconnectTransportForTesting();
  auto const membersAfterSecondRecovery =
      receiver.listFederationExecutionMembers(
          L"pushed-frame-recovery-execution");
  REQUIRE(membersAfterSecondRecovery.federationExists);
  REQUIRE(membersAfterSecondRecovery.members.size() == 2U);
  static_cast<void>(receiver.evokeMultiple(
      std::chrono::milliseconds::zero(), std::chrono::milliseconds(100)));
  REQUIRE(ambassador.announcementCount == 1U);
  REQUIRE(ambassador.callbackOrder == expectedCallbackOrder);
  REQUIRE(receiver.pendingPushedSynchronizationPointAnnouncementCount() == 0U);
  REQUIRE(receiver.pendingCallbackCount() == 0U);

  receiver.resignFederationExecution(
      L"pushed-frame-recovery-execution",
      receivingFederate.federateId,
      rti1516_2025::NO_ACTION);
  registrar.resignFederationExecution(
      L"pushed-frame-recovery-execution",
      registeringFederate.federateId,
      rti1516_2025::NO_ACTION);
  receiver.close();
  registrar.close();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}
