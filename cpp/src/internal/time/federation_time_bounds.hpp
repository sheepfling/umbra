#pragma once

#include "internal/federation/federation_registry.hpp"

#include <RTI/time/LogicalTime.h>

#include <cstdint>
#include <memory>

namespace umbra::detail {

enum class FederationTimeBoundStatus {
  available,
  undefined,
  requesting_federate_not_registered,
  factory_unavailable,
  inconsistent_temporal_state,
};

// GALT and LITS are both represented by an absent value when undefined.  The
// NRG setting is retained for the future grant scheduler: it affects what a
// time-constrained federate may be granted when GALT is undefined, not the
// definition of the read-only bound itself.
struct FederationTimeBounds {
  FederationTimeBoundStatus status = FederationTimeBoundStatus::inconsistent_temporal_state;
  std::shared_ptr<rti1516_2025::LogicalTime const> galt;
  std::shared_ptr<rti1516_2025::LogicalTime const> lits;
  bool nonRegulatedGrant = false;
  // The private TSO foundation may make GALT equal to the next queued
  // timestamp. A constrained TAR at that exact message boundary is eligible
  // for the corresponding callback; ordinary regulator-only GALT remains
  // strict as before.
  bool galtIsTsoBoundary = false;
};

// Computes the federation-owned 2025 GALT/LITS input boundary. An other
// regulator's earliest possible future timestamp is its current logical time
// (or pending advance target) plus actual lookahead; a forward TAR by a
// zero-lookahead regulator makes that lower bound exclusive, so the selected
// factory's epsilon is added. The recipient's delivered-since-last-advance,
// queued, and in-transit TSO timestamps are also considered. LITS is the
// smallest future (queued or in-transit) TSO timestamp constrained by GALT;
// when GALT is undefined it can still be defined by such an incoming message.
class FederationTimeBoundsCalculator final {
 public:
  [[nodiscard]] FederationTimeBounds calculate(
      FederationTimeExecutionSnapshot const& execution,
      std::uint64_t requestingFederateId) const;
};

}  // namespace umbra::detail
