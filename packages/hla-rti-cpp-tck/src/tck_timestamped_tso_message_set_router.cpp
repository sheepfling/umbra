#include "tck_timestamped_tso_message_set_router.hpp"

#include <string>

#include "tck_harness.hpp"
#include "tck_nmr_future_timestamped_object_deletion.hpp"
#include "tck_nmr_regional_timestamped_interaction.hpp"
#include "tck_nmra_future_timestamped_object_deletion.hpp"
#include "tck_tara_future_timestamped_object_deletion.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

constexpr char nmrDeletionScenario[] =
    "cpp-tck.timestamped-object-deletion-future-nmr-arrival";
constexpr char nmrDeletionContractId[] =
    "cpp-tck.timestamped-object-deletion-future-nmr-arrival-contract";
constexpr char nmraDeletionScenario[] =
    "cpp-tck.timestamped-object-deletion-future-nmra-arrival";
constexpr char nmraDeletionContractId[] =
    "cpp-tck.timestamped-object-deletion-future-nmra-arrival-contract";
constexpr char taraDeletionScenario[] =
    "cpp-tck.timestamped-object-deletion-future-tara-arrival";
constexpr char taraDeletionContractId[] =
    "cpp-tck.timestamped-object-deletion-future-tara-arrival-contract";
constexpr char regionalNmrScenario[] =
    "cpp-tck.nmr-regional-timestamped-interaction-message-set";
constexpr char regionalNmrContractId[] =
    "cpp-tck.nmr-regional-timestamped-interaction-message-set-contract";

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

std::optional<int> tryRunTimestampedTsoMessageSetScenario(
    int argc,
    char** argv) {
  if (selectedScenario(argc, argv, nmrDeletionScenario, nmrDeletionContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmrDeletionScenario,
        nmrDeletionContractId,
        scenarioNmrFutureTimestampedObjectDeletion,
        scenarioNmrFutureTimestampedObjectDeletionContract);
  }
  if (selectedScenario(argc, argv, nmraDeletionScenario, nmraDeletionContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmraDeletionScenario,
        nmraDeletionContractId,
        scenarioNmraFutureTimestampedObjectDeletion,
        scenarioNmraFutureTimestampedObjectDeletionContract);
  }
  if (selectedScenario(argc, argv, taraDeletionScenario, taraDeletionContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        taraDeletionScenario,
        taraDeletionContractId,
        scenarioTaraFutureTimestampedObjectDeletion,
        scenarioTaraFutureTimestampedObjectDeletionContract);
  }
  if (selectedScenario(argc, argv, regionalNmrScenario, regionalNmrContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        regionalNmrScenario,
        regionalNmrContractId,
        scenarioNmrRegionalTimestampedInteractionMessageSet,
        scenarioNmrRegionalTimestampedInteractionMessageSetContract);
  }
  return std::nullopt;
}

}  // namespace hla_rti_cpp_tck::detail
