#pragma once

#include <functional>
#include <string>

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

using ScenarioFunction = std::function<void(Options const&, rti::CallbackModel)>;

ScenarioFunction scenarioFunction(std::string const& id);

}  // namespace hla_rti_cpp_tck::detail
