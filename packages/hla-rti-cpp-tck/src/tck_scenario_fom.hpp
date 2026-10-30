#pragma once

#include "tck_harness.hpp"

#include <filesystem>
#include <string>

namespace hla_rti_cpp_tck::detail {

[[nodiscard]] inline std::filesystem::path adapterScenarioFom(
    Options const& options,
    std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  return found == options.scenarioFoms.end() ? options.fom : found->second;
}

}  // namespace hla_rti_cpp_tck::detail
