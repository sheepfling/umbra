#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioSupportSwitchGetterLifecycleBoundariesPortable(
    Options const&, rti::CallbackModel);
void scenarioSupportSwitchGetterLifecycleBoundariesContractPortable(
    Options const&, rti::CallbackModel);
void scenarioSupportSwitchSetterLifecycleBoundariesPortable(
    Options const&, rti::CallbackModel);
void scenarioSupportSwitchSetterLifecycleBoundariesContractPortable(
    Options const&, rti::CallbackModel);
void scenarioAutomaticResignDirectiveLifecycleBoundariesPortable(
    Options const&, rti::CallbackModel);
void scenarioAutomaticResignDirectiveLifecycleBoundariesContractPortable(
    Options const&, rti::CallbackModel);
void scenarioAutomaticResignDirectiveEnumValuesPortable(
    Options const&, rti::CallbackModel);
void scenarioAutomaticResignDirectiveEnumValuesContractPortable(
    Options const&, rti::CallbackModel);
void scenarioAdvisorySwitchValueRoundTripsPortable(
    Options const&, rti::CallbackModel);
void scenarioAdvisorySwitchValueRoundTripsContractPortable(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
