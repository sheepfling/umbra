#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioAttributeValueUpdateRequestBaseline(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateRequestBaselineContract(Options const&, rti::CallbackModel);
void scenarioObjectClassAttributeValueUpdateRequestBaseline(
    Options const&, rti::CallbackModel);
void scenarioObjectClassAttributeValueUpdateRequestBaselineContract(
    Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateResponse(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateResponseContract(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateRequestMultiRequester(
    Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateRequestMultiRequesterContract(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
