#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioStandardOrderTransportLookupsAfterResignationPortable(Options const&, rti::CallbackModel);
void scenarioStandardOrderTransportLookupsAfterResignationContractPortable(Options const&, rti::CallbackModel);
void scenarioObjectAttributeDeclarationsValidInputsAfterResignationPortable(Options const&, rti::CallbackModel);
void scenarioObjectAttributeDeclarationsValidInputsAfterResignationContractPortable(Options const&, rti::CallbackModel);
void scenarioInteractionDeclarationsValidInputsAfterResignationPortable(Options const&, rti::CallbackModel);
void scenarioInteractionDeclarationsValidInputsAfterResignationContractPortable(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionDeclarationsValidInputsAfterResignationPortable(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionDeclarationsValidInputsAfterResignationContractPortable(Options const&, rti::CallbackModel);
void scenarioInteractionTransportationQueryValidInputsAfterResignationPortable(Options const&, rti::CallbackModel);
void scenarioInteractionTransportationQueryValidInputsAfterResignationContractPortable(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
