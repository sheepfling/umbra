#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedInteractionSourceResignation(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionSourceResignationFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionFlushQueueFutureInput(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTsoDesignatorTerminalization(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionCrossProducerOrder(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionNoFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionRetractionFanout(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
