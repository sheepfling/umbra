#pragma once

#include <RTI/Enums.h>

#include "tck_options.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedInteractionMixedAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionMixedAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraEqualBoundary(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraEqualBoundaryContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraQueuedEqualBoundary(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraQueuedEqualBoundaryContract(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadSendAtGrant(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadSendAtGrantContract(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadTarSendBoundary(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryContract(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraStrictTimestampCutoff(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarStrictTimestampCutoff(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTarStrictTimestampCutoffContract(
    Options const&, rti::CallbackModel);
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
void scenarioTimestampedInteractionTaraQueuedEqualBoundaryPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTaraQueuedEqualBoundaryContractPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadSendAtGrantPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadSendAtGrantContractPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryPortable(
    Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryContractPortable(
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
