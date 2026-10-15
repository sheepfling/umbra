#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioCustomTransportationInteractionDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationInteractionDeliveryContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDeliveryContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDirectedDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDirectedDeliveryContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
