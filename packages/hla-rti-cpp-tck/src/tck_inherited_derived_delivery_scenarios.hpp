#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioInheritedObjectAttributeProjection(Options const&, rti::CallbackModel);
void scenarioInheritedObjectAttributeProjectionContract(Options const&, rti::CallbackModel);
void scenarioInheritedInteractionClassDelivery(Options const&, rti::CallbackModel);
void scenarioInheritedInteractionClassDeliveryContract(Options const&, rti::CallbackModel);
void scenarioInteractionClosestAncestorTransition(Options const&, rti::CallbackModel);
void scenarioInteractionClosestAncestorTransitionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInheritedInteractionClassDelivery(Options const&, rti::CallbackModel);
void scenarioTimestampedInheritedInteractionClassDeliveryContract(Options const&, rti::CallbackModel);
void scenarioDirectedDerivedObjectTargetDelivery(Options const&, rti::CallbackModel);
void scenarioDirectedDerivedObjectTargetDeliveryContract(Options const&, rti::CallbackModel);
void scenarioDirectedDerivedObjectTargetEligibility(Options const&, rti::CallbackModel);
void scenarioDirectedDerivedObjectTargetEligibilityContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedDerivedObjectTargetDelivery(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedDerivedObjectTargetDeliveryContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionReceivedClassProjection(Options const&, rti::CallbackModel);
void scenarioDirectedKnownObjectClassFallback(Options const&, rti::CallbackModel);
void scenarioDirectedKnownObjectClassFallbackContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionNoApplicableSubscription(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionSubscriptionAccumulation(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
