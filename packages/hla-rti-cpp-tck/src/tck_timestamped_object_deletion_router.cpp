#include "tck_timestamped_object_deletion_router.hpp"

#include <string>

#include "tck_harness.hpp"
#include "tck_nmr_future_timestamped_object_deletion.hpp"
#include "tck_nmra_future_timestamped_object_deletion.hpp"
#include "tck_tara_future_timestamped_object_deletion.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

constexpr char nmrScenario[] =
    "cpp-tck.timestamped-object-deletion-future-nmr-arrival";
constexpr char nmrContractId[] =
    "cpp-tck.timestamped-object-deletion-future-nmr-arrival-contract";
constexpr char nmraScenario[] =
    "cpp-tck.timestamped-object-deletion-future-nmra-arrival";
constexpr char nmraContractId[] =
    "cpp-tck.timestamped-object-deletion-future-nmra-arrival-contract";
constexpr char taraScenario[] =
    "cpp-tck.timestamped-object-deletion-future-tara-arrival";
constexpr char taraContractId[] =
    "cpp-tck.timestamped-object-deletion-future-tara-arrival-contract";

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

std::optional<int> tryRunTimestampedObjectDeletionScenario(
    int argc,
    char** argv) {
  if (selectedScenario(argc, argv, nmrScenario, nmrContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmrScenario,
        nmrContractId,
        scenarioNmrFutureTimestampedObjectDeletion,
        scenarioNmrFutureTimestampedObjectDeletionContract);
  }
  if (selectedScenario(argc, argv, nmraScenario, nmraContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmraScenario,
        nmraContractId,
        scenarioNmraFutureTimestampedObjectDeletion,
        scenarioNmraFutureTimestampedObjectDeletionContract);
  }
  if (selectedScenario(argc, argv, taraScenario, taraContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        taraScenario,
        taraContractId,
        scenarioTaraFutureTimestampedObjectDeletion,
        scenarioTaraFutureTimestampedObjectDeletionContract);
  }
  return std::nullopt;
}

}  // namespace hla_rti_cpp_tck::detail
