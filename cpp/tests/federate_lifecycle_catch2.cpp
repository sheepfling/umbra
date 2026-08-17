#include <catch2/catch_test_macros.hpp>

#include "internal/federate_lifecycle.hpp"

using umbra::detail::FederateLifecycle;
using umbra::detail::FederateLifecycleEvent;
using umbra::detail::FederateLifecycleResult;
using umbra::detail::FederateLifecycleState;

TEST_CASE("The private federate lifecycle accepts the modeled top-level paths", "[unit][kernel][lifecycle]") {
  FederateLifecycle lifecycle;

  REQUIRE(lifecycle.apply(FederateLifecycleEvent::connect) == FederateLifecycleResult::applied);
  REQUIRE(lifecycle.apply(FederateLifecycleEvent::join) == FederateLifecycleResult::applied);
  REQUIRE(lifecycle.apply(FederateLifecycleEvent::resign) == FederateLifecycleResult::applied);
  REQUIRE(lifecycle.apply(FederateLifecycleEvent::disconnect) == FederateLifecycleResult::applied);
  REQUIRE(lifecycle.state() == FederateLifecycleState::not_connected);
}

TEST_CASE("The private federate lifecycle rejects transitions outside their source state", "[unit][kernel][lifecycle]") {
  FederateLifecycle lifecycle;

  REQUIRE(lifecycle.apply(FederateLifecycleEvent::disconnect) == FederateLifecycleResult::invalid_transition);
  REQUIRE(lifecycle.apply(FederateLifecycleEvent::connect) == FederateLifecycleResult::applied);
  REQUIRE(lifecycle.apply(FederateLifecycleEvent::connection_lost) == FederateLifecycleResult::invalid_transition);
  REQUIRE(lifecycle.apply(FederateLifecycleEvent::join) == FederateLifecycleResult::applied);
  REQUIRE(lifecycle.apply(FederateLifecycleEvent::connection_lost) == FederateLifecycleResult::applied);
  REQUIRE(lifecycle.state() == FederateLifecycleState::not_connected);
}
