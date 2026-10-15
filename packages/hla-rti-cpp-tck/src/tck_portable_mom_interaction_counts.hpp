#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomInteractionsReceivedAttribute(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomInteractionsReceivedAttributeContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomInteractionsSentAttribute(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomInteractionsSentAttributeContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsReceivedAttribute(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsReceivedAttributeContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsSentAttribute(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsSentAttributeContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
