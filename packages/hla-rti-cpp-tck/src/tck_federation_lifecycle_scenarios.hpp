#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederationListServices(Options const&, rti::CallbackModel);
void scenarioFederationListServicesContract(Options const&, rti::CallbackModel);
void scenarioFederateLookupLifecycle(Options const&, rti::CallbackModel);
void scenarioFederateLookupLifecycleContract(Options const&, rti::CallbackModel);
void scenarioFederationLifecycle(Options const&, rti::CallbackModel);
void scenarioFederationLifecycleContract(Options const&, rti::CallbackModel);
void scenarioApplicationMultipleFederatesAcrossExecutions(
    Options const&, rti::CallbackModel);
void scenarioApplicationMultipleFederatesAcrossExecutionsContract(
    Options const&, rti::CallbackModel);
void scenarioConnectBeforeFederationAndDisconnectAfterResign(
    Options const&, rti::CallbackModel);
void scenarioFederationManagementRequiresConnection(Options const&, rti::CallbackModel);
void scenarioStandardExceptionBoundaries(Options const&, rti::CallbackModel);
void scenarioStandardExceptionBoundariesContract(Options const&, rti::CallbackModel);
void scenarioPostResignationServiceBoundaries(Options const&, rti::CallbackModel);
void scenarioPostResignationServiceBoundariesContract(Options const&, rti::CallbackModel);
void scenarioUnnamedJoinOverload(Options const&, rti::CallbackModel);
void scenarioUnnamedJoinOverloadContract(Options const&, rti::CallbackModel);
void scenarioExplicitMimCreation(Options const&, rti::CallbackModel);
void scenarioExplicitMimCreationContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
