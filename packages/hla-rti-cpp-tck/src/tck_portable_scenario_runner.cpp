#include "tck_portable_scenario_runner.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract) {
  return runPortableScenarioPair(
      argc, argv, scenarioId, contractId, scenario, contract, false);
}

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract,
    bool evokedOnly) {
  return runPortableScenarioPair(
      argc, argv, scenarioId, contractId, scenario, contract,
      evokedOnly ? CallbackScope::EvokedOnly : CallbackScope::All);
}

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract,
    CallbackScope scope) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      if (selected != scenarioId && selected != contractId) {
        continue;
      }
      auto const started = std::chrono::steady_clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if ((scope == CallbackScope::EvokedOnly && callback.second != rti::HLA_EVOKED) ||
          (scope == CallbackScope::ImmediateOnly && callback.second != rti::HLA_IMMEDIATE)) {
        result.status = "skipped";
        result.message = scope == CallbackScope::EvokedOnly
            ? "this scenario requires evoked callback ordering"
            : "this scenario requires immediate callbacks";
      } else try {
        if (selected == scenarioId) {
          scenario(options, callback.second);
        } else {
          contract(options, callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

}  // namespace hla_rti_cpp_tck::detail
