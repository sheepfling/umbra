#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioMalformedInputs(Options const&, rti::CallbackModel);
void scenarioFomModel(Options const&, rti::CallbackModel);
void scenarioFomQualifiedClassNameIdentity(Options const&, rti::CallbackModel);

void scenarioFomModuleComposition(Options const&, rti::CallbackModel);
void scenarioFomModuleCompositionContract(Options const&, rti::CallbackModel);
void scenarioFomAdditionalModuleJoinAtomicity(Options const&, rti::CallbackModel);
void scenarioFomAdditionalModuleJoinAtomicityContract(Options const&, rti::CallbackModel);
void scenarioFomTransportationHandleStability(Options const&, rti::CallbackModel);
void scenarioFomTransportationHandleStabilityContract(Options const&, rti::CallbackModel);
void scenarioFomDimensionHandleStability(Options const&, rti::CallbackModel);
void scenarioFomDimensionHandleStabilityContract(Options const&, rti::CallbackModel);
void scenarioFomUpdateRateValueStability(Options const&, rti::CallbackModel);
void scenarioFomUpdateRateValueStabilityContract(Options const&, rti::CallbackModel);
void scenarioFomStandardInvalidInputCorpus(Options const&, rti::CallbackModel);
void scenarioFomStandardInvalidInputCorpusContract(Options const&, rti::CallbackModel);
void scenarioFomInvalidCreateAtomicity(Options const&, rti::CallbackModel);
void scenarioFomInvalidCreateAtomicityContract(Options const&, rti::CallbackModel);
void scenarioFomInvalidCompositeJoinAtomicity(Options const&, rti::CallbackModel);
void scenarioFomInvalidCompositeJoinAtomicityContract(Options const&, rti::CallbackModel);
void scenarioFomInvalidMimCreateAtomicity(Options const&, rti::CallbackModel);
void scenarioFomInvalidMimCreateAtomicityContract(Options const&, rti::CallbackModel);
void scenarioFomInvalidCompositeMimCreateAtomicity(Options const&, rti::CallbackModel);
void scenarioFomInvalidCompositeMimCreateAtomicityContract(Options const&, rti::CallbackModel);
void scenarioFomEmptyMimCreateAtomicity(Options const&, rti::CallbackModel);
void scenarioFomEmptyMimCreateAtomicityContract(Options const&, rti::CallbackModel);
void scenarioFomEmptyModuleValidation(Options const&, rti::CallbackModel);
void scenarioFomEmptyModuleValidationContract(Options const&, rti::CallbackModel);
void scenarioFomDataTypeForwardReference(Options const&, rti::CallbackModel);
void scenarioFomDataTypeForwardReferenceContract(Options const&, rti::CallbackModel);
void scenarioFomDataTypeConflictingDefinition(Options const&, rti::CallbackModel);
void scenarioFomDataTypeConflictingDefinitionContract(Options const&, rti::CallbackModel);
void scenarioFomVariantAlternativeOverlap(Options const&, rti::CallbackModel);
void scenarioFomVariantAlternativeOverlapContract(Options const&, rti::CallbackModel);
void scenarioFomInteractionParameterMergeConflict(Options const&, rti::CallbackModel);
void scenarioFomInteractionParameterMergeConflictContract(Options const&, rti::CallbackModel);
void scenarioFomObjectAttributeMergeConflict(Options const&, rti::CallbackModel);
void scenarioFomObjectAttributeMergeConflictContract(Options const&, rti::CallbackModel);
void scenarioFomObjectClassSemanticsConflict(Options const&, rti::CallbackModel);
void scenarioFomObjectClassSemanticsConflictContract(Options const&, rti::CallbackModel);
void scenarioFomMatchingDataTypes(Options const&, rti::CallbackModel);
void scenarioFomMatchingDataTypesContract(Options const&, rti::CallbackModel);
void scenarioFomObjectClassDimensionMerge(Options const&, rti::CallbackModel);
void scenarioFomObjectClassDimensionMergeContract(Options const&, rti::CallbackModel);
void scenarioFomInteractionClassDimensionMerge(Options const&, rti::CallbackModel);
void scenarioFomInteractionClassDimensionMergeContract(Options const&, rti::CallbackModel);
void scenarioFomNamedVariantAlternativeMerge(Options const&, rti::CallbackModel);
void scenarioFomNamedVariantAlternativeMergeContract(Options const&, rti::CallbackModel);
void scenarioFomExtendableVariantHlaOther(Options const&, rti::CallbackModel);
void scenarioFomExtendableVariantHlaOtherContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
