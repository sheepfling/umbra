#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioUpdateRateQueries(Options const&, rti::CallbackModel);
void scenarioUpdateRateQueriesContract(Options const&, rti::CallbackModel);
void scenarioMixedUpdateRateSubscriptions(Options const&, rti::CallbackModel);
void scenarioMixedUpdateRateSubscriptionsContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateRateReduction(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateRateReductionContract(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
