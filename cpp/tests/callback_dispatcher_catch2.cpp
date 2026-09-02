#include <catch2/catch_test_macros.hpp>

#include "internal/callbacks/callback_dispatcher.hpp"
#include "internal/callbacks/callback_session.hpp"
#include "internal/federation/embedded_transport.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

#include <RTI/NullFederateAmbassador.h>

namespace {

using umbra::detail::CallbackDispatcher;
using umbra::detail::CallbackDispatchModel;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::umbra_binding_detail::CallbackSession;
using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;
using umbra::detail::InstrumentationLayer;
using umbra::detail::InstrumentationOperationSnapshot;
using umbra::detail::RuntimeInstrumentation;
using namespace std::chrono_literals;

class ReentrantEvokeFederateAmbassador final : public NullFederateAmbassador {
 public:
  UmbraRtiAmbassador* rti = nullptr;
  bool evokeRejected = false;
  bool multipleRejected = false;

  void reportFederationExecutions(
      rti1516_2025::FederationExecutionInformationVector const&) override {
    REQUIRE(rti != nullptr);
    try {
      static_cast<void>(rti->evokeCallback(0.0));
    } catch (rti1516_2025::CallNotAllowedFromWithinCallback const&) {
      evokeRejected = true;
    }
    try {
      static_cast<void>(rti->evokeMultipleCallbacks(0.0, 0.0));
    } catch (rti1516_2025::CallNotAllowedFromWithinCallback const&) {
      multipleRejected = true;
    }
  }
};

InstrumentationOperationSnapshot const& operation(
    umbra::detail::RuntimeInstrumentationSnapshot const& snapshot,
    InstrumentationLayer layer,
    std::string_view name) {
  auto const found = std::find_if(
      snapshot.operations.begin(),
      snapshot.operations.end(),
      [layer, name](InstrumentationOperationSnapshot const& candidate) {
        return candidate.layer == layer && candidate.name == name;
      });
  REQUIRE(found != snapshot.operations.end());
  return *found;
}

std::uint64_t callsFor(
    umbra::detail::RuntimeInstrumentationSnapshot const& snapshot,
    InstrumentationLayer layer,
    std::string_view name) {
  auto const found = std::find_if(
      snapshot.operations.begin(),
      snapshot.operations.end(),
      [layer, name](InstrumentationOperationSnapshot const& candidate) {
        return candidate.layer == layer && candidate.name == name;
      });
  return found == snapshot.operations.end() ? 0 : found->calls;
}

}  // namespace

TEST_CASE("The immediate callback dispatcher invokes enabled callbacks synchronously", "[unit][kernel][callbacks][foundation]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::immediate);
  std::vector<int> order;

  dispatcher.submit([&] {
    REQUIRE(dispatcher.isExecutingCallback());
    order.push_back(1);
  });
  dispatcher.submit([&] { order.push_back(2); });

  REQUIRE(order == std::vector<int>{1, 2});
  REQUIRE(dispatcher.pendingCount() == 0);
  REQUIRE_FALSE(dispatcher.isExecutingCallback());
  REQUIRE_FALSE(dispatcher.evokeOne(0ms));
}

TEST_CASE(
    "Immediate callback delivery remains FIFO and non-concurrent across producer threads",
    "[unit][kernel][callbacks][concurrency]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::immediate);
  std::mutex synchronizationMutex;
  std::condition_variable callbackStarted;
  std::condition_variable releaseCallback;
  std::vector<int> order;
  bool firstStarted = false;
  bool secondEntered = false;
  bool release = false;

  std::thread firstProducer([&] {
    dispatcher.submit([&] {
      std::unique_lock lock(synchronizationMutex);
      order.push_back(1);
      firstStarted = true;
      callbackStarted.notify_all();
      releaseCallback.wait(lock, [&] { return release; });
    });
  });

  {
    std::unique_lock lock(synchronizationMutex);
    REQUIRE(callbackStarted.wait_for(lock, 1s, [&] { return firstStarted; }));
  }

  std::thread secondProducer([&] {
    dispatcher.submit([&] {
      std::scoped_lock lock(synchronizationMutex);
      secondEntered = true;
      order.push_back(2);
    });
  });
  secondProducer.join();

  {
    std::scoped_lock lock(synchronizationMutex);
    REQUIRE_FALSE(secondEntered);
  }

  {
    std::scoped_lock lock(synchronizationMutex);
    release = true;
  }
  releaseCallback.notify_all();
  firstProducer.join();

  REQUIRE(order == std::vector<int>{1, 2});
  REQUIRE(dispatcher.pendingCount() == 0);
}

TEST_CASE(
    "A lightweight callback route still submits receive-order work",
    "[unit][kernel][callbacks][foundation]") {
  int submissions = 0;
  umbra::detail::FederateCallbackRoute route;
  route.submit = [&](umbra::detail::FederateCallbackInvocation invocation) {
    ++submissions;
    REQUIRE(static_cast<bool>(invocation));
  };

  route.enqueueReceiveOrder([](rti1516_2025::FederateAmbassador&) {});

  REQUIRE(submissions == 1);
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "Evoke services reject re-entry from an immediate federate callback",
    "[integration][connection][callbacks][reentrancy]") {
  UmbraRtiAmbassador rti;
  ReentrantEvokeFederateAmbassador federate;

  REQUIRE_NOTHROW(rti.connect(federate, rti1516_2025::HLA_IMMEDIATE));
  federate.rti = &rti;

  // Listing federations is a real RTI-initiated callback boundary.  The
  // immediate model enters the callback synchronously, so this exercises the
  // public CallNotAllowedFromWithinCallback guards without a test-only
  // dispatcher shortcut.
  REQUIRE_NOTHROW(rti.listFederationExecutions());
  REQUIRE(federate.evokeRejected);
  REQUIRE(federate.multipleRejected);

  REQUIRE_NOTHROW(rti.disconnect());
}
#endif

TEST_CASE(
    "Immediate callback disable stops an enabled backlog at the next boundary",
    "[unit][kernel][callbacks][enable-disable]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::immediate);
  std::vector<int> order;

  dispatcher.setEnabled(false);
  dispatcher.submit([&] {
    order.push_back(1);
    dispatcher.setEnabled(false);
  });
  dispatcher.submit([&] { order.push_back(2); });

  dispatcher.setEnabled(true);
  REQUIRE(order == std::vector<int>{1});
  REQUIRE(dispatcher.pendingCount() == 1);

  dispatcher.setEnabled(true);
  REQUIRE(order == std::vector<int>{1, 2});
  REQUIRE(dispatcher.pendingCount() == 0);
}

TEST_CASE("The evoked callback dispatcher invokes one queued callback at a time", "[unit][kernel][callbacks][foundation]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::evoked);
  std::vector<int> order;

  dispatcher.submit([&] { order.push_back(1); });
  dispatcher.submit([&] { order.push_back(2); });

  REQUIRE(dispatcher.pendingCount() == 2);
  REQUIRE(dispatcher.evokeOne(0ms));
  REQUIRE(order == std::vector<int>{1});
  REQUIRE(dispatcher.pendingCount() == 1);
  REQUIRE_FALSE(dispatcher.evokeOne(0ms));
  REQUIRE(order == std::vector<int>{1, 2});
}

TEST_CASE(
    "Concurrent evokers cannot enter one FederateAmbassador at the same time",
    "[unit][kernel][callbacks][concurrency]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::evoked);
  std::mutex synchronizationMutex;
  std::condition_variable callbackStarted;
  std::condition_variable secondEvokeStarted;
  std::condition_variable releaseCallback;
  std::vector<int> order;
  bool firstStarted = false;
  bool secondAttempted = false;
  bool secondEntered = false;
  bool release = false;

  dispatcher.submit([&] {
    std::unique_lock lock(synchronizationMutex);
    order.push_back(1);
    firstStarted = true;
    callbackStarted.notify_all();
    releaseCallback.wait(lock, [&] { return release; });
  });
  dispatcher.submit([&] {
    std::scoped_lock lock(synchronizationMutex);
    secondEntered = true;
    order.push_back(2);
  });

  std::thread firstEvoker([&] { static_cast<void>(dispatcher.evokeOne(0ms)); });
  {
    std::unique_lock lock(synchronizationMutex);
    REQUIRE(callbackStarted.wait_for(lock, 1s, [&] { return firstStarted; }));
  }

  std::thread secondEvoker([&] {
    {
      std::scoped_lock lock(synchronizationMutex);
      secondAttempted = true;
    }
    secondEvokeStarted.notify_all();
    static_cast<void>(dispatcher.evokeOne(0ms));
  });
  {
    std::unique_lock lock(synchronizationMutex);
    REQUIRE(secondEvokeStarted.wait_for(lock, 1s, [&] { return secondAttempted; }));
  }
  std::this_thread::sleep_for(25ms);
  {
    std::scoped_lock lock(synchronizationMutex);
    REQUIRE_FALSE(secondEntered);
  }

  {
    std::scoped_lock lock(synchronizationMutex);
    release = true;
  }
  releaseCallback.notify_all();
  firstEvoker.join();
  secondEvoker.join();

  REQUIRE(order == std::vector<int>{1, 2});
  REQUIRE(dispatcher.pendingCount() == 0);
}

TEST_CASE("Disabling callbacks preserves pending work until callbacks are enabled", "[unit][kernel][callbacks][foundation]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::immediate);
  std::vector<int> order;

  dispatcher.setEnabled(false);
  dispatcher.submit([&] { order.push_back(1); });
  REQUIRE(order.empty());
  REQUIRE(dispatcher.pendingCount() == 1);

  dispatcher.setEnabled(true);
  REQUIRE(order == std::vector<int>{1});
  REQUIRE(dispatcher.pendingCount() == 0);
}

TEST_CASE("Evoke Multiple Callbacks retains FIFO order through its minimum wait", "[unit][kernel][callbacks][foundation]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::evoked);
  std::vector<int> order;

  dispatcher.submit([&] { order.push_back(1); });
  dispatcher.submit([&] { order.push_back(2); });
  dispatcher.submit([&] { order.push_back(3); });

  REQUIRE_FALSE(dispatcher.evokeMultiple(1ms, 25ms));
  REQUIRE(order == std::vector<int>{1, 2, 3});
  REQUIRE(dispatcher.pendingCount() == 0);
}

TEST_CASE("The evoked callback dispatcher reports pending work while disabled", "[unit][kernel][callbacks][foundation]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::evoked);
  dispatcher.setEnabled(false);
  dispatcher.submit([] {});

  REQUIRE(dispatcher.evokeOne(0ms));
  REQUIRE(dispatcher.pendingCount() == 1);
  dispatcher.setEnabled(true);
  REQUIRE_FALSE(dispatcher.evokeOne(0ms));
}

TEST_CASE(
    "A callback session closes stale delivery and drains an external in-flight callback",
    "[unit][kernel][callbacks][lifecycle][foundation]") {
  NullFederateAmbassador federate;
  CallbackSession session(federate);
  std::mutex synchronizationMutex;
  std::condition_variable callbackStarted;
  std::condition_variable releaseCallback;
  bool started = false;
  bool release = false;

  std::thread callbackThread([&] {
    session.invoke([&](auto&) {
      std::unique_lock lock(synchronizationMutex);
      started = true;
      callbackStarted.notify_all();
      releaseCallback.wait(lock, [&] { return release; });
    });
  });

  {
    std::unique_lock lock(synchronizationMutex);
    started = callbackStarted.wait_for(lock, 1s, [&] { return started; });
  }

  auto closeFuture = std::async(std::launch::async, [&] { session.close(); });
  auto const closeStatusWhileBlocked = started
      ? closeFuture.wait_for(25ms)
      : std::future_status::deferred;

  {
    std::scoped_lock lock(synchronizationMutex);
    release = true;
  }
  releaseCallback.notify_all();
  callbackThread.join();

  CHECK(started);
  if (started) {
    CHECK(closeStatusWhileBlocked == std::future_status::timeout);
  }
  REQUIRE(closeFuture.wait_for(1s) == std::future_status::ready);
  REQUIRE_NOTHROW(closeFuture.get());

  bool staleInvocationRan = false;
  session.invoke([&](auto&) { staleInvocationRan = true; });
  REQUIRE_FALSE(staleInvocationRan);
}

TEST_CASE(
    "A callback session serializes concurrent direct invocations",
    "[unit][kernel][callbacks][concurrency]") {
  NullFederateAmbassador federate;
  CallbackSession session(federate);
  std::mutex synchronizationMutex;
  std::condition_variable callbackStarted;
  std::condition_variable releaseCallback;
  std::vector<int> order;
  bool firstStarted = false;
  bool secondEntered = false;
  bool release = false;

  std::thread firstInvocation([&] {
    session.invoke([&](auto&) {
      std::unique_lock lock(synchronizationMutex);
      order.push_back(1);
      firstStarted = true;
      callbackStarted.notify_all();
      releaseCallback.wait(lock, [&] { return release; });
    });
  });

  {
    std::unique_lock lock(synchronizationMutex);
    REQUIRE(callbackStarted.wait_for(lock, 1s, [&] { return firstStarted; }));
  }

  std::thread secondInvocation([&] {
    session.invoke([&](auto&) {
      std::scoped_lock lock(synchronizationMutex);
      secondEntered = true;
      order.push_back(2);
    });
  });

  std::this_thread::sleep_for(25ms);
  {
    std::scoped_lock lock(synchronizationMutex);
    REQUIRE_FALSE(secondEntered);
  }

  {
    std::scoped_lock lock(synchronizationMutex);
    release = true;
  }
  releaseCallback.notify_all();
  firstInvocation.join();
  secondInvocation.join();

  REQUIRE(order == std::vector<int>{1, 2});
}

TEST_CASE(
    "A callback session can close from its active callback without deadlocking",
    "[unit][kernel][callbacks][lifecycle][foundation]") {
  NullFederateAmbassador federate;
  CallbackSession session(federate);
  bool firstInvocationRan = false;

  REQUIRE_NOTHROW(session.invoke([&](auto&) {
    firstInvocationRan = true;
    session.close();
  }));
  REQUIRE(firstInvocationRan);

  bool staleInvocationRan = false;
  session.invoke([&](auto&) { staleInvocationRan = true; });
  REQUIRE_FALSE(staleInvocationRan);
}

TEST_CASE(
    "Instrumentation times immediate callback dispatch and FederateAmbassador entry",
    "[unit][kernel][instrumentation][callbacks][foundation]") {
  auto instrumentation = std::make_shared<RuntimeInstrumentation>();
  CallbackDispatcher dispatcher(CallbackDispatchModel::immediate, instrumentation);
  NullFederateAmbassador federate;
  CallbackSession session(federate, instrumentation);

  dispatcher.submit([&] {
    session.invoke([&](auto&) { std::this_thread::sleep_for(2ms); });
  });

  auto const snapshot = instrumentation->snapshot();
  auto const& queueDelay = operation(
      snapshot,
      InstrumentationLayer::callback_dispatch,
      "queue_delay.immediate");
  auto const& dispatch = operation(
      snapshot,
      InstrumentationLayer::callback_dispatch,
      "execute.immediate");
  auto const& federateCallback = operation(
      snapshot,
      InstrumentationLayer::federate_ambassador,
      "invoke.immediate");

  REQUIRE(queueDelay.calls == 1);
  REQUIRE(dispatch.calls == 1);
  REQUIRE(federateCallback.calls == 1);
  REQUIRE(dispatch.totalDurationNanoseconds > 0);
  REQUIRE(federateCallback.totalDurationNanoseconds > 0);
}

TEST_CASE(
    "Instrumentation times evoked callback queue delay separately from callback execution",
    "[unit][kernel][instrumentation][callbacks][foundation]") {
  auto instrumentation = std::make_shared<RuntimeInstrumentation>();
  CallbackDispatcher dispatcher(CallbackDispatchModel::evoked, instrumentation);
  NullFederateAmbassador federate;
  CallbackSession session(federate, instrumentation);

  dispatcher.submit([&] {
    session.invoke([&](auto&) { std::this_thread::sleep_for(2ms); });
  });
  std::this_thread::sleep_for(2ms);

  auto const before = instrumentation->snapshot();
  REQUIRE(callsFor(
              before,
              InstrumentationLayer::callback_dispatch,
              "execute.evoked") == 0);
  REQUIRE(callsFor(
              before,
              InstrumentationLayer::federate_ambassador,
              "invoke.evoked") == 0);

  REQUIRE_FALSE(dispatcher.evokeOne(0ms));

  auto const after = instrumentation->snapshot();
  auto const& queueDelay = operation(
      after,
      InstrumentationLayer::callback_dispatch,
      "queue_delay.evoked");
  auto const& dispatch = operation(
      after,
      InstrumentationLayer::callback_dispatch,
      "execute.evoked");
  auto const& federateCallback = operation(
      after,
      InstrumentationLayer::federate_ambassador,
      "invoke.evoked");

  REQUIRE(queueDelay.calls == 1);
  REQUIRE(dispatch.calls == 1);
  REQUIRE(federateCallback.calls == 1);
  REQUIRE(queueDelay.totalDurationNanoseconds > 0);
  REQUIRE(dispatch.totalDurationNanoseconds > 0);
  REQUIRE(federateCallback.totalDurationNanoseconds > 0);
}

TEST_CASE(
    "The RTI ambassador exposes internal call timing without changing its public API",
    "[unit][kernel][instrumentation][rti][foundation]") {
  UmbraRtiAmbassador rti;
  NullFederateAmbassador federate;

  REQUIRE_NOTHROW(rti.connect(federate, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_FALSE(rti.evokeCallback(0.0));
  REQUIRE_NOTHROW(rti.disconnect());

  auto const snapshot = rti.runtimeInstrumentationSnapshotForTesting();
  REQUIRE(operation(
              snapshot,
              InstrumentationLayer::rti_ambassador,
              "connect")
              .calls == 1);
  REQUIRE(operation(
              snapshot,
              InstrumentationLayer::rti_ambassador,
              "evokeCallback")
              .calls == 1);
  REQUIRE(operation(
              snapshot,
              InstrumentationLayer::rti_ambassador,
              "disconnect")
              .calls == 1);
}

TEST_CASE(
    "Transport fault and forced-resignation paths publish internal timing",
    "[unit][kernel][instrumentation][transport][foundation]") {
  auto instrumentation = std::make_shared<RuntimeInstrumentation>();
  int owner = 0;
  bool faultDelivered = false;
  umbra::detail::EmbeddedTransportConnection faultingConnection(
      &owner,
      [&](std::wstring) { faultDelivered = true; },
      [](std::wstring) { return true; },
      instrumentation);
  faultingConnection.fail(L"test fault");
  REQUIRE(faultDelivered);

  umbra::detail::EmbeddedTransportConnection resigningConnection(
      &owner,
      [](std::wstring) {},
      [](std::wstring) { return true; },
      instrumentation);
  REQUIRE(resigningConnection.forceFederateResignation(L"test resignation"));

  auto const snapshot = instrumentation->snapshot();
  REQUIRE(operation(
              snapshot,
              InstrumentationLayer::transport,
              "fault")
              .calls == 1);
  REQUIRE(operation(
              snapshot,
              InstrumentationLayer::transport,
              "forced_resignation")
              .calls == 1);
}
