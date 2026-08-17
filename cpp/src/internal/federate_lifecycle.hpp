#pragma once

namespace umbra::detail {

// Private projection of the top-level federate-lifetime machine in the pinned
// Requirements Lab corpus. It intentionally covers only the six transitions
// below; save/restore and normal-activity substates are separate future slices.
enum class FederateLifecycleState {
  not_connected,
  not_joined,
  joined,
};

enum class FederateLifecycleEvent {
  connect,
  join,
  resign,
  connection_lost,
  rti_resign,
  disconnect,
};

enum class FederateLifecycleResult {
  applied,
  invalid_transition,
};

class FederateLifecycle final {
 public:
  [[nodiscard]] FederateLifecycleState state() const noexcept;
  [[nodiscard]] FederateLifecycleResult apply(FederateLifecycleEvent event) noexcept;

 private:
  FederateLifecycleState state_ = FederateLifecycleState::not_connected;
};

}  // namespace umbra::detail
