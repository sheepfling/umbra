#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioObjectRemovalMultiRecipientFifo(Options const&, rti::CallbackModel);
void scenarioObjectRemovalMultiRecipientFifoContract(Options const&, rti::CallbackModel);
void scenarioReceiveOrderObjectRemovalSubscriptionWithdrawal(Options const&, rti::CallbackModel);
void scenarioReceiveOrderObjectRemovalSubscriptionWithdrawalContract(Options const&, rti::CallbackModel);
void scenarioResignDeleteObjectsMultiRecipientFifo(Options const&, rti::CallbackModel);
void scenarioResignDeleteObjectsMultiRecipientFifoContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
