#include <catch2/catch_test_macros.hpp>

#include "process_timestamped_delete_object_instance_scenario.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassadors retarget pending process NMR and NMRA to a future timestamped object deletion",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][timestamped-delete-object-instance]"
    "[process-tso-object-deletion-nmr-nmra-future-arrival]"
    "[process-time-advance-next-message-queued-focused]"
    "[rti.service.delete-object-instance][rti.service.next-message-request]"
    "[rti.service.next-message-request-available][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant][2025]") {
  SECTION("HLA_EVOKED NMR") {
    verifyTimestampedDeleteObjectInstanceProcessScenario(
        rti1516_2025::HLA_EVOKED, true, true, false, false, true);
  }
  SECTION("HLA_IMMEDIATE NMR") {
    verifyTimestampedDeleteObjectInstanceProcessScenario(
        rti1516_2025::HLA_IMMEDIATE, true, true, false, false, true);
  }
  SECTION("HLA_EVOKED NMRA") {
    verifyTimestampedDeleteObjectInstanceProcessScenario(
        rti1516_2025::HLA_EVOKED, true, true, false, true, false);
  }
  SECTION("HLA_IMMEDIATE NMRA") {
    verifyTimestampedDeleteObjectInstanceProcessScenario(
        rti1516_2025::HLA_IMMEDIATE, true, true, false, true, false);
  }
}
#endif
