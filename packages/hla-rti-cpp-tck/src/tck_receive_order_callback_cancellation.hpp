#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioAsynchronousAttributeDelivery(Options const&, rti::CallbackModel);
void scenarioReceiveOrderAttributeUpdateCallbackCancellation(
    Options const&, rti::CallbackModel);
void scenarioReceiveOrderAttributeUpdateCallbackCancellationContract(
    Options const&, rti::CallbackModel);
void scenarioReceiveOrderAttributeUpdateReceiverResignationFenceContract(
    Options const&, rti::CallbackModel);
void scenarioReceiveOrderMultiAttributeUpdateCallbackCancellation(
    Options const&, rti::CallbackModel);
void scenarioReceiveOrderMultiAttributeUpdateCallbackCancellationContract(
    Options const&, rti::CallbackModel);
void scenarioReceiveOrderInteractionCallbackCancellation(
    Options const&, rti::CallbackModel);
void scenarioReceiveOrderInteractionCallbackCancellationContract(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
