#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

template <typename Expected, typename Operation>
void requirePendingException(Operation&& operation, std::string const& description) {
  try {
    operation();
  } catch (Expected const&) {
    return;
  }
  throw std::runtime_error(description + " did not report the pending role request");
}

void runTimeRolePendingEnable(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Pending role enablement must be observed before evoking its callback");
  require(!options.logicalTimeImplementationName.empty(),
          "Pending role enablement requires an adapter-supplied time implementation");

  Session member(options, model, "owner");
  auto const federation = federationName(options, "time-role-pending-enable");
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);

  auto const time = makeTimeContext(member);
  auto const advance = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 1U);

  member.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  require(member.recorder().timeRegulationEnabled().empty(),
          "Time Regulation Enabled escaped before evoked callback service");
  requirePendingException<rti::RequestForTimeRegulationPending>(
      [&] { member.rtiAmbassador().timeAdvanceRequest(*advance); },
      "Time Advance Request during pending Time Regulation");
  requirePendingException<rti::RequestForTimeRegulationPending>(
      [&] { member.rtiAmbassador().enableTimeRegulation(*time.epsilon); },
      "Repeated Enable Time Regulation");
  waitFor(member,
          [&] { return member.recorder().timeRegulationEnabled().size() == 1U; },
          options, "Time Regulation Enabled callback");
  auto actualLookahead = time.factory->makeZero();
  require(actualLookahead != nullptr, "Could not allocate Query Lookahead output");
  member.rtiAmbassador().queryLookahead(*actualLookahead);
  require(*actualLookahead == *time.epsilon,
          "Pending Time Regulation changed the requested lookahead");
  member.rtiAmbassador().disableTimeRegulation();

  member.rtiAmbassador().enableTimeConstrained();
  require(member.recorder().timeConstrainedEnabled().empty(),
          "Time Constrained Enabled escaped before evoked callback service");
  requirePendingException<rti::RequestForTimeConstrainedPending>(
      [&] { member.rtiAmbassador().timeAdvanceRequest(*advance); },
      "Time Advance Request during pending Time Constrained");
  requirePendingException<rti::RequestForTimeConstrainedPending>(
      [&] { member.rtiAmbassador().enableTimeConstrained(); },
      "Repeated Enable Time Constrained");
  waitFor(member,
          [&] { return member.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "Time Constrained Enabled callback");
  member.rtiAmbassador().disableTimeConstrained();

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

}  // namespace

void scenarioTimeRolePendingEnable(Options const& options, rti::CallbackModel model) {
  runTimeRolePendingEnable(options, model);
}

void scenarioTimeRolePendingEnableContract(Options const& options, rti::CallbackModel model) {
  runTimeRolePendingEnable(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
