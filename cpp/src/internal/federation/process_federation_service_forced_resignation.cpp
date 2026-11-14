#include "internal/federation/process_federation_service.hpp"

#include "internal/federation/process_federation_service_helpers.hpp"

#include <RTI/Exception.h>

#include <optional>
#include <utility>

namespace umbra::detail {

bool ProcessFederationService::forceFederateResignationForTesting(
    ProcessTransportSession& session,
    std::wstring reasonForResign) {
  std::optional<std::wstring> federationName;
  std::uint64_t federateId = 0U;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() || !state->second.federationName ||
        state->second.federateId == 0U ||
        !registry_.memberById(
            *state->second.federationName, state->second.federateId)) {
      return false;
    }
    federationName = state->second.federationName;
    federateId = state->second.federateId;
  }

  auto result = registry_.resign(
      *federationName,
      federateId,
      rti1516_2025::NO_ACTION);
  if (result.status != FederationRegistryStatus::applied ||
      !dispatchConnectionLossResult(
          *federationName,
          std::move(result),
          federateId)) {
    return false;
  }

  auto scheduled = registry_.reevaluateTimeAdvanceGrants(*federationName);
  if (scheduled.status != FederationTimeGrantStatus::applied) {
    return false;
  }
  dispatchTimeAdvanceGrants(
      *federationName,
      std::move(scheduled.dispatches));

  // Remove the remote joined-federate state before the target can observe the
  // event and make another public service call. The transport itself remains
  // available for a later Join on the same RTI ambassador.
  clearJoinedFederateSessionState(session, federateId);
  return session.send({
      TransportServiceMessageKind::event,
      TransportServiceOperation::federate_resigned,
      TransportServiceStatus::ok,
      0U,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{std::move(reasonForResign)})});
}

}  // namespace umbra::detail
