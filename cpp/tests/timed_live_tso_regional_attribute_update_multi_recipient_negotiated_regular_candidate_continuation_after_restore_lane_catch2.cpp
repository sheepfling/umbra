#include <catch2/catch_test_macros.hpp>

// Reuse the established four-federate regional save/restore harness while
// exposing the regular-to-regular candidate continuation as a direct
// TEST_CASE declaration for source indexing and a standalone CTest target.
#define UMBRA_REGULAR_CANDIDATE_TEST_CASE(...) \
  void umbra_run_regular_candidate_continuation()
#include "timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_catch2.cpp"
#undef UMBRA_REGULAR_CANDIDATE_TEST_CASE

TEST_CASE(
    "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[cancel-pending-ownership-acquisitions][negotiated-attribute-ownership-divestiture][negotiated-willing-to-acquire-continuation][willing-to-acquire][timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-state][tso-regional-attribute-update-timed-negotiated-continuation-state]"
    "[tso-regional-attribute-update-timed-negotiated-regular-candidate-state][tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-state]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions][rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values][rti.service.publish-object-class-attributes]"
    "[rti.service.attribute-ownership-acquisition][rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture][rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution][rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.request-divestiture-confirmation][federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant][federate.callback.time-advance-grant]"
    "[federate.callback.request-attribute-ownership-release]"
    "[tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore]") {
  REQUIRE_NOTHROW(umbra_run_regular_candidate_continuation());
}
