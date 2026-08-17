#include <catch2/catch_test_macros.hpp>

#include "internal/callback_dispatcher.hpp"
#include "internal/callback_session.hpp"

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
using namespace std::chrono_literals;

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
