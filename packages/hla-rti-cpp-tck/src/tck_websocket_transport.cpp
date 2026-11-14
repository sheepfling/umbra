#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioWebSocketTransportSupport(
    Options const& options, rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("ws://", 0U) == 0U,
      "WebSocket transport coverage requires an adapter-supplied ws:// RTI address");

  Session session(options, model, "connection");
  auto const connection = session.connect();
  require(
      connection.addressUsed,
      "Connect did not report use of the adapter-supplied WebSocket RTI address");

  std::wstring const absentFederation =
      L"__cpp_tck_websocket_transport_absent_federation__";
  session.rtiAmbassador().listFederationExecutions();
  waitFor(
      session,
      [&] {
        return session.recorder().lastFederationExecutionReportOmits(
            absentFederation);
      },
      options,
      "WebSocket federation-execution query response");
  session.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
