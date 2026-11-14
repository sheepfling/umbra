#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void verifyStandardConfigurationAndAuthorizationContract();
void verifyStandardAuthorizerFactoryFactoryContract();
void scenarioAuthorizerConnectServiceDecision(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerUnrecognizedCredentialsUnauthorized(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerDisabledServiceRejectsCredentials(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerFederationServiceDecision(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerDestroyServiceDecision(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerJoinServiceDecision(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerNoCredentialsConnect(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerRidConfigurationDecision(
    Options const&, rti::CallbackModel callbackModel);
void scenarioAuthorizerSingleServicePerRidProfile(
    Options const&, rti::CallbackModel callbackModel);

}  // namespace hla_rti_cpp_tck::detail
