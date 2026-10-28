#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

namespace umbra::detail {

TransportServiceMessage
ProcessFederationService::handleListFederationExecutionMembers(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const listRequest =
      decodeProcessFederationExecutionMembersRequest(request.payload);
  ProcessFederationExecutionMembersResult result;
  {
    std::scoped_lock lock(mutex_);
    if (!sessions_.contains(&session)) {
      return rejected(request);
    }
    auto const members = registry_.membersFor(listRequest.federationName);
    if (!members) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationExecutionMembersResult(result));
    }
    result.federationExists = true;
    result.members.reserve(members->size());
    for (auto const& member : *members) {
      result.members.push_back(
          ProcessFederationExecutionMember{member.name, member.type});
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationExecutionMembersResult(result));
}

TransportServiceMessage ProcessFederationService::handleListFederationExecutions(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  if (!request.payload.empty()) {
    return invalid(request);
  }
  ProcessFederationExecutionsResult result;
  {
    std::scoped_lock lock(mutex_);
    if (!sessions_.contains(&session)) {
      return rejected(request);
    }
    auto const federations = registry_.federationExecutions();
    result.federations.reserve(federations.size());
    for (auto const& federation : federations) {
      result.federations.push_back(ProcessFederationExecutionSummary{
          federation.name, federation.logicalTimeImplementationName});
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationExecutionsResult(result));
}

}  // namespace umbra::detail
