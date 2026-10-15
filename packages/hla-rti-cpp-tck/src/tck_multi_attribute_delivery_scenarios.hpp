#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOrdinaryMultiAttributeSubscriptionProjection(
    Options const&, rti::CallbackModel);
void scenarioOrdinaryMultiAttributeSubscriptionProjectionContract(
    Options const&, rti::CallbackModel);
void scenarioOrdinaryMultiAttributeValueUpdateRequestResponse(
    Options const&, rti::CallbackModel);
void scenarioOrdinaryMultiAttributeValueUpdateRequestResponseContract(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
