#pragma once

#include <RTI/Enums.h>

#include "tck_options.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedInteractionMixedAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionMixedAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraEqualBoundary(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraEqualBoundaryContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarEqualBoundary(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarEqualBoundaryContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarPostGrantCutoff(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarPostGrantCutoffContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPublicationSendTransition(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPublicationSendTransitionContract(Options const&, rti::CallbackModel);

void scenarioTimestampedInteractionMixedAdvancesPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionMixedAdvancesContractPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraEqualBoundaryPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraEqualBoundaryContractPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarEqualBoundaryPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarEqualBoundaryContractPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarPostGrantCutoffPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarPostGrantCutoffContractPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPublicationSendTransitionPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPublicationSendTransitionContractPortable(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
