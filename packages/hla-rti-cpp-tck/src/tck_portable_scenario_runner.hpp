#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

using PortableScenario = void (*)(Options const&, rti::CallbackModel);

enum class CallbackScope { All, EvokedOnly, ImmediateOnly };

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract);

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract,
    bool evokedOnly);

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract,
    CallbackScope scope);

}  // namespace hla_rti_cpp_tck::detail
