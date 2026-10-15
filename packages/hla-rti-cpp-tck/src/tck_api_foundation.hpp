#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void verifyStandardExceptionHierarchyContract();
void verifyStandardEnumContract();
void verifyStandardHandleAndCollectionContract();
void verifyStandardNullFederateAmbassadorContract();
void scenarioNullFederateAmbassadorContract(Options const&, rti::CallbackModel);
void scenarioApiSurfaceInventory(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
