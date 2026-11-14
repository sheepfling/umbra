#include "tck_main_scenarios.hpp"

#include <chrono>
#include <thread>

namespace hla_rti_cpp_tck::detail {

void scenarioPublicTransportNewSessionInteroperability(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("tcp://127.0.0.1:", 0U) == 0U,
      "The Table 33 public-API case requires its independent loopback peer adapter");
  auto heartbeatOptions = options;
  heartbeatOptions.additionalSettings =
      "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25";
  Session session(heartbeatOptions, model, "table33-peer");
  auto const connection = session.connect();
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "The public Connect call did not apply the bounded heartbeat settings");
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  session.rtiAmbassador().disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
