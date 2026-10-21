#include "tck_timestamped_attribute_tara_router.hpp"

#include <string>

#include "tck_main_scenarios.hpp"
#include "tck_portable_scenario_runner.hpp"
#include "tck_timestamped_attribute_pending_tara.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNmrFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model);
void scenarioNmrFutureTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model);

namespace {

constexpr char tarPostGrantCutoffScenario[] =
    "cpp-tck.timestamped-attribute-update-tar-post-grant-cutoff";
constexpr char tarPostGrantCutoffContractId[] =
    "cpp-tck.timestamped-attribute-update-tar-post-grant-cutoff-contract";
constexpr char taraFutureArrivalScenario[] =
    "cpp-tck.timestamped-attribute-update-future-tara-arrival";
constexpr char taraFutureArrivalContractId[] =
    "cpp-tck.timestamped-attribute-update-future-tara-arrival-contract";
constexpr char nmrFutureArrivalScenario[] =
    "cpp-tck.timestamped-attribute-update-future-nmr-arrival";
constexpr char nmrFutureArrivalContractId[] =
    "cpp-tck.timestamped-attribute-update-future-nmr-arrival-contract";

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

std::optional<int> tryRunTimestampedAttributeTaraScenario(
    int argc,
    char** argv) {
  if (selectedScenario(
          argc,
          argv,
          tarPostGrantCutoffScenario,
          tarPostGrantCutoffContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        tarPostGrantCutoffScenario,
        tarPostGrantCutoffContractId,
        scenarioTimestampedAttributeUpdateTarPostGrantCutoff,
        scenarioTimestampedAttributeUpdateTarPostGrantCutoffContract);
  }
  if (selectedScenario(
          argc,
          argv,
          taraFutureArrivalScenario,
          taraFutureArrivalContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        taraFutureArrivalScenario,
        taraFutureArrivalContractId,
        scenarioTaraFutureTimestampedAttributeUpdate,
        scenarioTaraFutureTimestampedAttributeUpdateContract);
  }
  if (selectedScenario(
          argc,
          argv,
          nmrFutureArrivalScenario,
          nmrFutureArrivalContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmrFutureArrivalScenario,
        nmrFutureArrivalContractId,
        scenarioNmrFutureTimestampedAttributeUpdate,
        scenarioNmrFutureTimestampedAttributeUpdateContract);
  }
  return std::nullopt;
}

}  // namespace hla_rti_cpp_tck::detail
