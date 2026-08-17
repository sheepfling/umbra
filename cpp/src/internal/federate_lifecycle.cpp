#include "internal/federate_lifecycle.hpp"

namespace umbra::detail {

FederateLifecycleState FederateLifecycle::state() const noexcept {
  return state_;
}

FederateLifecycleResult FederateLifecycle::apply(FederateLifecycleEvent event) noexcept {
  switch (event) {
    case FederateLifecycleEvent::connect:
      if (state_ == FederateLifecycleState::not_connected) {
        state_ = FederateLifecycleState::not_joined;
        return FederateLifecycleResult::applied;
      }
      break;
    case FederateLifecycleEvent::join:
      if (state_ == FederateLifecycleState::not_joined) {
        state_ = FederateLifecycleState::joined;
        return FederateLifecycleResult::applied;
      }
      break;
    case FederateLifecycleEvent::resign:
    case FederateLifecycleEvent::rti_resign:
      if (state_ == FederateLifecycleState::joined) {
        state_ = FederateLifecycleState::not_joined;
        return FederateLifecycleResult::applied;
      }
      break;
    case FederateLifecycleEvent::connection_lost:
      if (state_ == FederateLifecycleState::joined) {
        state_ = FederateLifecycleState::not_connected;
        return FederateLifecycleResult::applied;
      }
      break;
    case FederateLifecycleEvent::disconnect:
      if (state_ == FederateLifecycleState::not_joined) {
        state_ = FederateLifecycleState::not_connected;
        return FederateLifecycleResult::applied;
      }
      break;
  }
  return FederateLifecycleResult::invalid_transition;
}

}  // namespace umbra::detail
