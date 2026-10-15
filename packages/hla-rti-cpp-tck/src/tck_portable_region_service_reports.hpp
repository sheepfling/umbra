#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportDimensionHandleSet(Options const&, rti::CallbackModel);
void scenarioServiceReportDimensionHandleSetContract(Options const&, rti::CallbackModel);
void scenarioServiceReportRangeBounds(Options const&, rti::CallbackModel);
void scenarioServiceReportRangeBoundsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportSetRangeBounds(Options const&, rti::CallbackModel);
void scenarioServiceReportSetRangeBoundsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportCommitRegionModifications(Options const&, rti::CallbackModel);
void scenarioServiceReportCommitRegionModificationsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDeleteRegion(Options const&, rti::CallbackModel);
void scenarioServiceReportDeleteRegionContract(Options const&, rti::CallbackModel);
void scenarioServiceReportCreateRegion(Options const&, rti::CallbackModel);
void scenarioServiceReportCreateRegionContract(Options const&, rti::CallbackModel);
void scenarioRegionRangeValidation(Options const&, rti::CallbackModel);
void scenarioRegionRangeValidationContract(Options const&, rti::CallbackModel);
void scenarioRegionRangeDimensionValidation(Options const&, rti::CallbackModel);
void scenarioRegionRangeDimensionValidationContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
