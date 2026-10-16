#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioDelaySubscriptionEvaluationInteraction(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationDirectedInteraction(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationTimestampedInteraction(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationTimestampedDirectedInteraction(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationTimestampedRegionalAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioDelaySubscriptionEvaluationTimestampedRegionalInteraction(Options const&, rti::CallbackModel);

} // namespace hla_rti_cpp_tck::detail
