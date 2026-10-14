#include "tck_federation_mom_save_status.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <string>

namespace hla_rti_cpp_tck::detail {

void scenarioFederationMomSaveStatus(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "federation-mom-save-status-owner");
  Session peer(options, model, "federation-mom-save-status-peer");
  auto const federation = federationName(options, "federation-mom-save-status");
  connectAndJoin(owner, peer, options, federation, options.fom);

  auto const ownerHandle = owner.federateHandle();
  auto const peerHandle = peer.federateHandle();
  auto const statusBefore = owner.federationSaveStatusResponses().size();
  owner.rtiAmbassador().queryFederationSaveStatus();
  waitFor(
      owner,
      [&] {
        return owner.federationSaveStatusResponses().size() > statusBefore;
      },
      options,
      "federation MOM save-status response");

  auto const statuses = owner.federationSaveStatusResponses().back();
  require(
      statuses.size() == 2U,
      "federation MOM save-status response omitted a joined federate");
  auto requireStatus = [&](rti::FederateHandle const& handle,
                           rti::SaveStatus expected,
                           std::string const& description) {
    auto const found = std::find_if(
        statuses.begin(),
        statuses.end(),
        [&](auto const& entry) { return entry.first == handle; });
    require(found != statuses.end(), description + " omitted a federate");
    require(found->second == expected, description + " returned the wrong status");
  };
  requireStatus(
      ownerHandle,
      rti::NO_SAVE_IN_PROGRESS,
      "federation MOM save-status owner entry");
  requireStatus(
      peerHandle,
      rti::NO_SAVE_IN_PROGRESS,
      "federation MOM save-status peer entry");

  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
