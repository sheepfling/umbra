#include <catch2/catch_test_macros.hpp>

#include "ieee1516_2025_connection_process_tso_regional_interaction_fixture.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassadors send timestamped regional interactions as receive order when the sender is not regulating",
    "[integration][development-profile][2025][data-distribution-management]"
    "[interaction-management][time-management][process-boundary][public-endpoint]"
    "[regional-interaction][unregulated-timestamped-regional-interaction]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  verifyProcessTsoRegionalInteraction(
      RegionalInteractionAdvance::unregulated_timestamp);
}

TEST_CASE(
    "RTIambassadors retain a timestamped regional interaction while callbacks are disabled",
    "[integration][foundation][data-distribution-management][interaction-management]"
    "[time-management][time-advance][callbacks][callback-gating][transport]"
    "[process-boundary][public-endpoint][multi-federate][regional-interaction]"
    "[timestamped-process-regional-interaction][regional-ddm-routing]"
    "[process-tso-regional-interaction-callback-gating]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-interaction-class]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  verifyProcessTsoRegionalInteraction(RegionalInteractionAdvance::tar);
}

TEST_CASE(
    "RTIambassadors retarget a pending process NMR to a future timestamped regional interaction",
    "[integration][foundation][data-distribution-management][interaction-management]"
    "[time-management][time-advance][transport][process-boundary][public-endpoint]"
    "[multi-federate][regional-interaction][timestamped-process-regional-interaction]"
    "[process-time-advance-next-message-queued-focused]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-interaction-class]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  verifyProcessTsoRegionalInteraction(RegionalInteractionAdvance::nmr);
}

TEST_CASE(
    "RTIambassadors retarget a pending process NMRA to a future timestamped regional interaction while callbacks are disabled",
    "[integration][foundation][data-distribution-management][interaction-management]"
    "[time-management][time-advance][callbacks][callback-gating][transport]"
    "[process-boundary][public-endpoint][multi-federate][regional-interaction]"
    "[timestamped-process-regional-interaction]"
    "[process-time-advance-next-message-queued-focused]"
    "[process-tso-regional-interaction-callback-gating]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-interaction-class]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request-available][rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  verifyProcessTsoRegionalInteraction(RegionalInteractionAdvance::nmra);
}
#endif
