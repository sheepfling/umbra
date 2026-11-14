#include "tck_nmra_galt_boundary_router.hpp"

#include <string>

#include "tck_harness.hpp"
#include "tck_portable_scenario_runner.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNmraNoPregrantTsoFloor(Options const&, rti::CallbackModel);
void scenarioNmraNoPregrantTsoFloorContract(Options const&, rti::CallbackModel);
void scenarioNmraNoTsoAtGaltGrantTime(Options const&, rti::CallbackModel);
void scenarioNmraNoTsoAtGaltGrantTimeContract(
    Options const&, rti::CallbackModel);
void scenarioNmraNoTsoWaitsForGalt(Options const&, rti::CallbackModel);
void scenarioNmraNoTsoWaitsForGaltContract(Options const&, rti::CallbackModel);

void scenarioNmraNoPregrantTsoFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoPregrantTsoFloor(options, model);
}

void scenarioNmraNoPregrantTsoFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoPregrantTsoFloorContract(options, model);
}

void scenarioNmraNoTsoAtGaltGrantTimePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoAtGaltGrantTime(options, model);
}

void scenarioNmraNoTsoAtGaltGrantTimeContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoAtGaltGrantTimeContract(options, model);
}

void scenarioNmraNoTsoWaitsForGaltPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoWaitsForGalt(options, model);
}

void scenarioNmraNoTsoWaitsForGaltContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoWaitsForGaltContract(options, model);
}

namespace {

constexpr char nmraNoPregrantTsoFloorScenario[] =
    "cpp-tck.nmra-no-pregrant-tso-floor";
constexpr char nmraNoPregrantTsoFloorContractId[] =
    "cpp-tck.nmra-no-pregrant-tso-floor-contract";
constexpr char nmraNoTsoAtGaltGrantTimeScenario[] =
    "cpp-tck.nmra-no-tso-at-galt-grant-time";
constexpr char nmraNoTsoAtGaltGrantTimeContractId[] =
    "cpp-tck.nmra-no-tso-at-galt-grant-time-contract";
constexpr char nmraNoTsoWaitsForGaltScenario[] =
    "cpp-tck.nmra-no-tso-waits-for-galt";
constexpr char nmraNoTsoWaitsForGaltContractId[] =
    "cpp-tck.nmra-no-tso-waits-for-galt-contract";

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

std::optional<int> tryRunNmraGaltBoundaryScenario(int argc, char** argv) {
  if (selectedScenario(
          argc,
          argv,
          nmraNoPregrantTsoFloorScenario,
          nmraNoPregrantTsoFloorContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmraNoPregrantTsoFloorScenario,
        nmraNoPregrantTsoFloorContractId,
        scenarioNmraNoPregrantTsoFloor,
        scenarioNmraNoPregrantTsoFloorContract);
  }
  if (selectedScenario(
          argc,
          argv,
          nmraNoTsoAtGaltGrantTimeScenario,
          nmraNoTsoAtGaltGrantTimeContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmraNoTsoAtGaltGrantTimeScenario,
        nmraNoTsoAtGaltGrantTimeContractId,
        scenarioNmraNoTsoAtGaltGrantTime,
        scenarioNmraNoTsoAtGaltGrantTimeContract);
  }
  if (selectedScenario(
          argc,
          argv,
          nmraNoTsoWaitsForGaltScenario,
          nmraNoTsoWaitsForGaltContractId)) {
    return runPortableScenarioPair(
        argc,
        argv,
        nmraNoTsoWaitsForGaltScenario,
        nmraNoTsoWaitsForGaltContractId,
        scenarioNmraNoTsoWaitsForGalt,
        scenarioNmraNoTsoWaitsForGaltContract);
  }
  return std::nullopt;
}

}  // namespace hla_rti_cpp_tck::detail
