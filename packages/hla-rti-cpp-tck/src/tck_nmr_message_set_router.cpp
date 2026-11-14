#include "tck_nmr_message_set_router.hpp"

#include <string>

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

constexpr char nmrNoPregrantTsoFloorScenario[] =
    "cpp-tck.nmr-no-pregrant-tso-floor";
constexpr char nmrNoPregrantTsoFloorContractId[] =
    "cpp-tck.nmr-no-pregrant-tso-floor-contract";
constexpr char nmrReceivedTsoFloorScenario[] =
    "cpp-tck.nmr-received-tso-floor";
constexpr char nmrReceivedTsoFloorContractId[] =
    "cpp-tck.nmr-received-tso-floor-contract";
constexpr char nmrRoEarliestTsoCohortScenario[] =
    "cpp-tck.nmr-ro-earliest-tso-cohort";
constexpr char nmrRoEarliestTsoCohortContractId[] =
    "cpp-tck.nmr-ro-earliest-tso-cohort-contract";
constexpr char nmrFutureTsoArrivalWhilePendingScenario[] =
    "cpp-tck.nmr-future-tso-arrival-while-pending";
constexpr char nmrFutureTsoArrivalWhilePendingContractId[] =
    "cpp-tck.nmr-future-tso-arrival-while-pending-contract";
constexpr char nmrDirectedFutureTsoArrivalWhilePendingScenario[] =
    "cpp-tck.nmr-directed-future-tso-arrival-while-pending";
constexpr char nmrDirectedFutureTsoArrivalWhilePendingContractId[] =
    "cpp-tck.nmr-directed-future-tso-arrival-while-pending-contract";
constexpr char nmraDirectedFutureTsoArrivalWhilePendingScenario[] =
    "cpp-tck.nmra-directed-future-tso-arrival-while-pending";
constexpr char nmraDirectedFutureTsoArrivalWhilePendingContractId[] =
    "cpp-tck.nmra-directed-future-tso-arrival-while-pending-contract";
constexpr char nmrReceivedTsoAttributeUpdateFloorScenario[] =
    "cpp-tck.nmr-received-tso-attribute-update-floor";
constexpr char nmrReceivedTsoAttributeUpdateFloorContractId[] =
    "cpp-tck.nmr-received-tso-attribute-update-floor-contract";
constexpr char nmrReceivedTsoDirectedInteractionFloorScenario[] =
    "cpp-tck.nmr-received-tso-directed-interaction-floor";
constexpr char nmrReceivedTsoDirectedInteractionFloorContractId[] =
    "cpp-tck.nmr-received-tso-directed-interaction-floor-contract";
constexpr char nmrEmptyMessageSetGrantTimeScenario[] =
    "cpp-tck.nmr-empty-message-set-grant-time";
constexpr char nmrEmptyMessageSetGrantTimeContractId[] =
    "cpp-tck.nmr-empty-message-set-grant-time-contract";
constexpr char nmrRequestBelowGaltImmediateGrantScenario[] =
    "cpp-tck.nmr-request-below-galt-immediate-grant";
constexpr char nmrRequestBelowGaltImmediateGrantContractId[] =
    "cpp-tck.nmr-request-below-galt-immediate-grant-contract";
constexpr char nmrLitsBelowGaltImmediateGrantScenario[] =
    "cpp-tck.nmr-lits-below-galt-immediate-grant";
constexpr char nmrLitsBelowGaltImmediateGrantContractId[] =
    "cpp-tck.nmr-lits-below-galt-immediate-grant-contract";
constexpr char nmrNoTsoUndefinedGaltNrgScenario[] =
    "cpp-tck.nmr-no-tso-undefined-galt-nrg";
constexpr char nmrNoTsoUndefinedGaltNrgContractId[] =
    "cpp-tck.nmr-no-tso-undefined-galt-nrg-contract";
constexpr char nmraNoTsoUndefinedGaltNrgScenario[] =
    "cpp-tck.nmra-no-tso-undefined-galt-nrg";
constexpr char nmraNoTsoUndefinedGaltNrgContractId[] =
    "cpp-tck.nmra-no-tso-undefined-galt-nrg-contract";
constexpr char nmraReceivedTsoActualLookaheadFloorScenario[] =
    "cpp-tck.nmra-received-tso-actual-lookahead-floor";
constexpr char nmraReceivedTsoActualLookaheadFloorContractId[] =
    "cpp-tck.nmra-received-tso-actual-lookahead-floor-contract";
constexpr char nmraRoEarliestTsoCohortScenario[] =
    "cpp-tck.nmra-ro-earliest-tso-cohort";
constexpr char nmraRoEarliestTsoCohortContractId[] =
    "cpp-tck.nmra-ro-earliest-tso-cohort-contract";

bool selectedScenario(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const selected = std::string(argv[index + 1]);
    if (selected == scenarioId || selected == contractId) {
      return true;
    }
  }
  return false;
}

}  // namespace

void scenarioNmrNoPregrantTsoFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoPregrantTsoFloor(options, model);
}

void scenarioNmrNoPregrantTsoFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoPregrantTsoFloorContract(options, model);
}

void scenarioNmrReceivedTsoFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoFloor(options, model);
}

void scenarioNmrReceivedTsoFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoFloorContract(options, model);
}

void scenarioNmrQueuedRoAndEarliestTsoCohortPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrQueuedRoAndEarliestTsoCohort(options, model);
}

void scenarioNmrQueuedRoAndEarliestTsoCohortContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrQueuedRoAndEarliestTsoCohortContract(options, model);
}

void scenarioNmrFutureTsoArrivesWhilePendingPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrFutureTsoArrivesWhilePending(options, model);
}

void scenarioNmrFutureTsoArrivesWhilePendingContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrFutureTsoArrivesWhilePendingContract(options, model);
}

void scenarioNmrDirectedFutureTsoArrivesWhilePendingPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrDirectedFutureTsoArrivesWhilePending(options, model);
}

void scenarioNmrDirectedFutureTsoArrivesWhilePendingContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrDirectedFutureTsoArrivesWhilePendingContract(options, model);
}

void scenarioNmraDirectedFutureTsoArrivesWhilePendingPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraDirectedFutureTsoArrivesWhilePending(options, model);
}

void scenarioNmraDirectedFutureTsoArrivesWhilePendingContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraDirectedFutureTsoArrivesWhilePendingContract(options, model);
}

void scenarioNmrReceivedTsoAttributeUpdateFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoAttributeUpdateFloor(options, model);
}

void scenarioNmrReceivedTsoAttributeUpdateFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoAttributeUpdateFloorContract(options, model);
}

void scenarioNmrReceivedTsoDirectedInteractionFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoDirectedInteractionFloor(options, model);
}

void scenarioNmrReceivedTsoDirectedInteractionFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoDirectedInteractionFloorContract(options, model);
}

void scenarioNmrEmptyMessageSetGrantTimePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrEmptyMessageSetGrantTime(options, model);
}

void scenarioNmrEmptyMessageSetGrantTimeContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrEmptyMessageSetGrantTimeContract(options, model);
}

void scenarioNmrRequestBelowGaltImmediateGrantPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrRequestBelowGaltImmediateGrant(options, model);
}

void scenarioNmrRequestBelowGaltImmediateGrantContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrRequestBelowGaltImmediateGrantContract(options, model);
}

void scenarioNmrLitsBelowGaltImmediateGrantPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrLitsBelowGaltImmediateGrant(options, model);
}

void scenarioNmrLitsBelowGaltImmediateGrantContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrLitsBelowGaltImmediateGrantContract(options, model);
}

void scenarioNmrNoTsoUndefinedGaltNrgPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoTsoUndefinedGaltNrg(options, model);
}

void scenarioNmrNoTsoUndefinedGaltNrgContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoTsoUndefinedGaltNrgContract(options, model);
}

void scenarioNmraNoTsoUndefinedGaltNrgPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoUndefinedGaltNrg(options, model);
}

void scenarioNmraNoTsoUndefinedGaltNrgContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoUndefinedGaltNrgContract(options, model);
}

void scenarioNmraReceivedTsoActualLookaheadFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraReceivedTsoActualLookaheadFloor(options, model);
}

void scenarioNmraReceivedTsoActualLookaheadFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraReceivedTsoActualLookaheadFloorContract(options, model);
}

void scenarioNmraQueuedRoAndEarliestTsoCohortPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraQueuedRoAndEarliestTsoCohort(options, model);
}

void scenarioNmraQueuedRoAndEarliestTsoCohortContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraQueuedRoAndEarliestTsoCohortContract(options, model);
}

std::optional<int> tryRunNmrMessageSetScenario(int argc, char** argv) {
  struct Route final {
    char const* scenarioId;
    char const* contractId;
    PortableScenario scenario;
    PortableScenario contract;
  };
  constexpr Route routes[] = {
      {nmrNoPregrantTsoFloorScenario,
       nmrNoPregrantTsoFloorContractId,
       scenarioNmrNoPregrantTsoFloorPortable,
       scenarioNmrNoPregrantTsoFloorContractPortable},
      {nmrReceivedTsoFloorScenario,
       nmrReceivedTsoFloorContractId,
       scenarioNmrReceivedTsoFloorPortable,
       scenarioNmrReceivedTsoFloorContractPortable},
      {nmrRoEarliestTsoCohortScenario,
       nmrRoEarliestTsoCohortContractId,
       scenarioNmrQueuedRoAndEarliestTsoCohortPortable,
       scenarioNmrQueuedRoAndEarliestTsoCohortContractPortable},
      {nmrFutureTsoArrivalWhilePendingScenario,
       nmrFutureTsoArrivalWhilePendingContractId,
       scenarioNmrFutureTsoArrivesWhilePendingPortable,
       scenarioNmrFutureTsoArrivesWhilePendingContractPortable},
      {nmrDirectedFutureTsoArrivalWhilePendingScenario,
       nmrDirectedFutureTsoArrivalWhilePendingContractId,
       scenarioNmrDirectedFutureTsoArrivesWhilePendingPortable,
       scenarioNmrDirectedFutureTsoArrivesWhilePendingContractPortable},
      {nmraDirectedFutureTsoArrivalWhilePendingScenario,
       nmraDirectedFutureTsoArrivalWhilePendingContractId,
       scenarioNmraDirectedFutureTsoArrivesWhilePendingPortable,
       scenarioNmraDirectedFutureTsoArrivesWhilePendingContractPortable},
      {nmrReceivedTsoAttributeUpdateFloorScenario,
       nmrReceivedTsoAttributeUpdateFloorContractId,
       scenarioNmrReceivedTsoAttributeUpdateFloorPortable,
       scenarioNmrReceivedTsoAttributeUpdateFloorContractPortable},
      {nmrReceivedTsoDirectedInteractionFloorScenario,
       nmrReceivedTsoDirectedInteractionFloorContractId,
       scenarioNmrReceivedTsoDirectedInteractionFloorPortable,
       scenarioNmrReceivedTsoDirectedInteractionFloorContractPortable},
      {nmrEmptyMessageSetGrantTimeScenario,
       nmrEmptyMessageSetGrantTimeContractId,
       scenarioNmrEmptyMessageSetGrantTimePortable,
       scenarioNmrEmptyMessageSetGrantTimeContractPortable},
      {nmrRequestBelowGaltImmediateGrantScenario,
       nmrRequestBelowGaltImmediateGrantContractId,
       scenarioNmrRequestBelowGaltImmediateGrantPortable,
       scenarioNmrRequestBelowGaltImmediateGrantContractPortable},
      {nmrLitsBelowGaltImmediateGrantScenario,
       nmrLitsBelowGaltImmediateGrantContractId,
       scenarioNmrLitsBelowGaltImmediateGrantPortable,
       scenarioNmrLitsBelowGaltImmediateGrantContractPortable},
      {nmrNoTsoUndefinedGaltNrgScenario,
       nmrNoTsoUndefinedGaltNrgContractId,
       scenarioNmrNoTsoUndefinedGaltNrgPortable,
       scenarioNmrNoTsoUndefinedGaltNrgContractPortable},
      {nmraNoTsoUndefinedGaltNrgScenario,
       nmraNoTsoUndefinedGaltNrgContractId,
       scenarioNmraNoTsoUndefinedGaltNrgPortable,
       scenarioNmraNoTsoUndefinedGaltNrgContractPortable},
      {nmraReceivedTsoActualLookaheadFloorScenario,
       nmraReceivedTsoActualLookaheadFloorContractId,
       scenarioNmraReceivedTsoActualLookaheadFloorPortable,
       scenarioNmraReceivedTsoActualLookaheadFloorContractPortable},
      {nmraRoEarliestTsoCohortScenario,
       nmraRoEarliestTsoCohortContractId,
       scenarioNmraQueuedRoAndEarliestTsoCohortPortable,
       scenarioNmraQueuedRoAndEarliestTsoCohortContractPortable},
  };

  for (auto const& route : routes) {
    if (selectedScenario(
            argc, argv, route.scenarioId, route.contractId)) {
      return runPortableScenarioPair(
          argc,
          argv,
          route.scenarioId,
          route.contractId,
          route.scenario,
          route.contract);
    }
  }
  return std::nullopt;
}

}  // namespace hla_rti_cpp_tck::detail
