#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipDenialIdle(Options const&, rti::CallbackModel);
void scenarioOwnershipDenialIdleContract(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionCancellationDenialRace(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionCancellationDenialRaceContract(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
