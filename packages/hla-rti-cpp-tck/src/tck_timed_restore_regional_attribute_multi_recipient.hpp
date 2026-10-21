#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimedRestoreRegionalAttributeMultiRecipient(
    Options const& options, rti::CallbackModel model);
void scenarioTimedRestoreRegionalAttributeMultiRecipientContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
