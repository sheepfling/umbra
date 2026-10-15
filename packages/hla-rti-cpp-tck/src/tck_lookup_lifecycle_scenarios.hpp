#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioInteractionClassLookupLifecycle(Options const&, rti::CallbackModel);
void scenarioInteractionClassLookupLifecycleContract(Options const&, rti::CallbackModel);
void scenarioObjectClassLookupLifecycle(Options const&, rti::CallbackModel);
void scenarioObjectClassLookupLifecycleContract(Options const&, rti::CallbackModel);
void scenarioAttributeLookupLifecycle(Options const&, rti::CallbackModel);
void scenarioAttributeLookupLifecycleContract(Options const&, rti::CallbackModel);
void scenarioParameterLookupLifecycle(Options const&, rti::CallbackModel);
void scenarioParameterLookupLifecycleContract(Options const&, rti::CallbackModel);
void scenarioDimensionLookupLifecycle(Options const&, rti::CallbackModel);
void scenarioDimensionLookupLifecycleContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
