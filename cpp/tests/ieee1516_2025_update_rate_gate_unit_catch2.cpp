#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstddef>

#include "internal/time/update_rate_gate.hpp"

namespace {

TEST_CASE(
    "Private update-rate gate spaces best-effort streams and never drops reliable delivery",
    "[update-rate-reduction][update-rate-gate][object-management][unit]") {
  using Gate = umbra::detail::UpdateRateGate;
  Gate::Clock::time_point now{};
  Gate gate([&now] { return now; });

  REQUIRE(gate.admit("receiver/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("receiver/attribute", 2.0, false));
  now += std::chrono::milliseconds(500);
  REQUIRE(gate.admit("receiver/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("receiver/attribute", 2.0, false));

  // A faster producer (three submissions in one half-second) is reduced to
  // the slower subscriber's two-per-second allowance.
  std::size_t delivered = 1;
  for (int submission = 0; submission < 2; ++submission) {
    now += std::chrono::milliseconds(100);
    delivered += gate.admit("producer/subscriber", 2.0, false) ? 1U : 0U;
  }
  REQUIRE(delivered == 2);
  now += std::chrono::milliseconds(400);
  REQUIRE(gate.admit("producer/subscriber", 2.0, false));

  // Distinct projected attributes keep independent admission histories.
  REQUIRE(gate.admit("receiver/fast-attribute", 10.0, false));
  REQUIRE(gate.admit("receiver/slow-attribute", 1.0, false));
  now += std::chrono::milliseconds(50);
  REQUIRE_FALSE(gate.admit("receiver/fast-attribute", 10.0, false));
  REQUIRE_FALSE(gate.admit("receiver/slow-attribute", 1.0, false));

  // A subscription mutation is encoded into the delivery key by the
  // registry.  A later generation therefore starts with a fresh admission
  // history even when the federate/object/attribute identity is unchanged.
  REQUIRE(gate.admit("receiver/object/generation-1/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("receiver/object/generation-1/attribute", 2.0, false));
  REQUIRE(gate.admit("receiver/object/generation-2/attribute", 2.0, false));

  REQUIRE(gate.admit("reliable", 0.001, true));
  REQUIRE(gate.admit("reliable", 0.001, true));
  REQUIRE(gate.admit("default", 0.0, false));
  REQUIRE(gate.admit("default", 0.0, false));
  gate.erase("receiver/attribute");
  REQUIRE(gate.admit("receiver/attribute", 2.0, false));

  // Federation teardown must only discard that execution's wall-clock
  // history.  A second live execution can have the same recipient/object
  // handles and still needs its reduction interval preserved.
  REQUIRE(gate.admit("federation-a/receiver/object/1/generation-1/attribute", 2.0, false));
  REQUIRE(gate.admit("federation-b/receiver/object/1/generation-1/attribute", 2.0, false));
  gate.erasePrefix("federation-a/");
  REQUIRE(gate.admit("federation-a/receiver/object/1/generation-1/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("federation-b/receiver/object/1/generation-1/attribute", 2.0, false));

  // Federate resignation is narrower than federation teardown.  Reclaiming
  // one recipient's history must leave a second recipient's live window
  // intact even when both share the same object and attribute handles.
  REQUIRE(gate.admit("federation-a/101/object/1/generation-1/attribute", 2.0, false));
  REQUIRE(gate.admit("federation-a/202/object/1/generation-1/attribute", 2.0, false));
  gate.erasePrefix("federation-a/101/");
  REQUIRE(gate.admit("federation-a/101/object/1/generation-1/attribute", 2.0, false));
  REQUIRE_FALSE(gate.admit("federation-a/202/object/1/generation-1/attribute", 2.0, false));

  gate.clear();
  REQUIRE(gate.admit("receiver/attribute", 2.0, false));
}
}  // namespace
