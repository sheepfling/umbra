#include "internal/federate_lifecycle.hpp"

using umbra::detail::FederateLifecycle;
using umbra::detail::FederateLifecycleEvent;
using umbra::detail::FederateLifecycleResult;
using umbra::detail::FederateLifecycleState;

int main() {
  FederateLifecycle lifecycle;
  if (lifecycle.state() != FederateLifecycleState::not_connected) {
    return 1;
  }
  if (lifecycle.apply(FederateLifecycleEvent::join) != FederateLifecycleResult::invalid_transition) {
    return 2;
  }
  if (lifecycle.apply(FederateLifecycleEvent::connect) != FederateLifecycleResult::applied ||
      lifecycle.apply(FederateLifecycleEvent::join) != FederateLifecycleResult::applied ||
      lifecycle.apply(FederateLifecycleEvent::connection_lost) != FederateLifecycleResult::applied) {
    return 3;
  }
  if (lifecycle.state() != FederateLifecycleState::not_connected) {
    return 4;
  }
  return 0;
}
