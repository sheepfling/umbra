#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioCustomTransportationAttributeDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationAttributeDeliveryContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedAttributeDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedAttributeDeliveryContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedAttributeAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedAttributeAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedInteractionAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedInteractionAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDirectedDeliveryPortable(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDirectedDeliveryPortableContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationInteractionDeliveryPortable(Options const&, rti::CallbackModel);
void scenarioCustomTransportationInteractionDeliveryPortableContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDeliveryPortable(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDeliveryPortableContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationDirectedInteractionDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationDirectedInteractionDeliveryContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvancesContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
