#include <catch2/catch_test_macros.hpp>

#include "internal/callback_dispatcher.hpp"
#include "internal/callback_session.hpp"
#include "internal/embedded_transport.hpp"
#include "internal/umbra_rti_ambassador.hpp"

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

TEST_CASE("The immediate callback dispatcher invokes enabled callbacks synchronously", "[unit][kernel][callbacks]") {
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

TEST_CASE("The evoked callback dispatcher invokes one queued callback at a time", "[unit][kernel][callbacks]") {
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

TEST_CASE("Disabling callbacks preserves pending work until callbacks are enabled", "[unit][kernel][callbacks]") {
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

TEST_CASE("Evoke Multiple Callbacks retains FIFO order through its minimum wait", "[unit][kernel][callbacks]") {
  CallbackDispatcher dispatcher(CallbackDispatchModel::evoked);
  std::vector<int> order;

  dispatcher.submit([&] { order.push_back(1); });
  dispatcher.submit([&] { order.push_back(2); });
  dispatcher.submit([&] { order.push_back(3); });

  REQUIRE_FALSE(dispatcher.evokeMultiple(1ms, 25ms));
  REQUIRE(order == std::vector<int>{1, 2, 3});
  REQUIRE(dispatcher.pendingCount() == 0);
}

TEST_CASE("The evoked callback dispatcher reports pending work while disabled", "[unit][kernel][callbacks]") {
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
    "[unit][kernel][callbacks][lifecycle]") {
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
    "A callback session can close from its active callback without deadlocking",
    "[unit][kernel][callbacks][lifecycle]") {
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
    "[unit][kernel][instrumentation][callbacks]") {
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
    "[unit][kernel][instrumentation][callbacks]") {
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
    "[unit][kernel][instrumentation][rti]") {
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
    "[unit][kernel][instrumentation][transport]") {
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
