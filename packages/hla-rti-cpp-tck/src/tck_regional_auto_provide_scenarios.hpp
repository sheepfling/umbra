#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRegionalAutoProvideResponse(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideResponseContract(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideTimestampedResponse(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideTimestampedResponseContract(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideOverlap(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideOverlapContract(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideMultiSource(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideMultiSourceContract(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideMultiProvider(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideMultiProviderContract(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideCallbackControl(Options const&, rti::CallbackModel);
void scenarioRegionalAutoProvideCallbackControlContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
