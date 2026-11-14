#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void verifyStandardExceptionHierarchyContract();
void verifyStandardEnumContract();
void verifyStandardHandleAndCollectionContract();
void verifyStandardNullFederateAmbassadorContract();
void verifyRegionalSendNoRetractionDesignatorContract();
void scenarioNullFederateAmbassadorContract(Options const&, rti::CallbackModel);
void scenarioExceptionNameString(Options const&, rti::CallbackModel);
void scenarioThreeLibrarySetContract(Options const&, rti::CallbackModel);
void scenarioApiSurfaceInventory(Options const&, rti::CallbackModel);
void scenarioAnnexCApiHeaderInventory(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
