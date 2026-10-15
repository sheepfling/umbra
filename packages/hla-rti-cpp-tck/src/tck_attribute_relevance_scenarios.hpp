#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioKnownClassAttributeRelevance(Options const&, rti::CallbackModel);
void scenarioKnownClassAttributeRelevanceDisabled(Options const&, rti::CallbackModel);
void scenarioAttributeRelevanceRateReissue(Options const&, rti::CallbackModel);
void scenarioAttributeRelevanceRateReissueContract(Options const&, rti::CallbackModel);
void scenarioRelevanceAdvisories(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
