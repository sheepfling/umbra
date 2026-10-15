#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederationMomCurrentFdd(Options const&, rti::CallbackModel);
void scenarioFederationMomCurrentFddContract(Options const&, rti::CallbackModel);
void scenarioFederationMomFomModuleDesignatorList(Options const&, rti::CallbackModel);
void scenarioFederationMomFomModuleDesignatorListContract(
    Options const&, rti::CallbackModel);
void scenarioFederationMomContentReports(Options const&, rti::CallbackModel);
void scenarioFederationMomContentReportsContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
