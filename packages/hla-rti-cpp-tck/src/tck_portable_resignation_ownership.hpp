#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioResignDeleteObjectLookupBoundaryPortable(Options const&, rti::CallbackModel);
void scenarioResignDeleteObjectLookupBoundaryContractPortable(Options const&, rti::CallbackModel);
void scenarioResignNoActionOwnedAttributesBoundaryPortable(Options const&, rti::CallbackModel);
void scenarioResignNoActionOwnedAttributesBoundaryContractPortable(Options const&, rti::CallbackModel);
void scenarioResignDeleteObjectsThenDivestCrossObjectEffectsPortable(Options const&, rti::CallbackModel);
void scenarioResignDeleteObjectsThenDivestCrossObjectEffectsContractPortable(Options const&, rti::CallbackModel);
void scenarioResignDeleteThenDivestPrivilegeEpoch(Options const&, rti::CallbackModel);
void scenarioResignDeleteThenDivestPrivilegeEpochContract(Options const&, rti::CallbackModel);
void scenarioResignUnconditionallyDivestAttributesPreservesObjectsPortable(Options const&, rti::CallbackModel);
void scenarioResignUnconditionallyDivestAttributesPreservesObjectsContractPortable(Options const&, rti::CallbackModel);
void scenarioResignCancelPendingOwnershipAcquisitionsPortable(Options const&, rti::CallbackModel);
void scenarioResignCancelPendingOwnershipAcquisitionsContractPortable(Options const&, rti::CallbackModel);
void scenarioResignCancelThenDeleteThenDivestCrossObjectEffectsPortable(Options const&, rti::CallbackModel);
void scenarioResignCancelThenDeleteThenDivestCrossObjectEffectsContractPortable(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
