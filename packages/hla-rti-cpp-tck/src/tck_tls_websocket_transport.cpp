#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioSecureWebSocketTransportSupport(
    Options const& options, rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("wss://", 0U) == 0U,
      "Secure WebSocket coverage requires an adapter-supplied wss:// RTI address");

  Session session(options, model, "connection");
  auto const connection = session.connect();
  require(
      connection.addressUsed,
      "Connect did not report use of the adapter-supplied secure WebSocket RTI address");

  std::wstring const absentFederation =
      L"__cpp_tck_secure_websocket_absent_federation__";
  session.rtiAmbassador().listFederationExecutions();
  waitFor(
      session,
      [&] {
        return session.recorder().lastFederationExecutionReportOmits(
            absentFederation);
      },
      options,
      "secure WebSocket federation-execution query response");
  session.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
