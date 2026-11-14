#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOrderTypeControls(Options const& options, rti::CallbackModel model);
void scenarioInheritedAttributeDefaultOrderScope(
    Options const& options,
    rti::CallbackModel model);
void scenarioInheritedAttributeDefaultOrderScopeContract(
    Options const& options,
    rti::CallbackModel model);
void scenarioFddAttributeOrderDefaultInitialization(
    Options const& options,
    rti::CallbackModel model);
void scenarioFddAttributeOrderDefaultInitializationContract(
    Options const& options,
    rti::CallbackModel model);
void scenarioDefaultOrderAttributeClassHandleValidation(
    Options const& options,
    rti::CallbackModel model);
void scenarioDefaultOrderAttributeClassHandleValidationContract(
    Options const& options,
    rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
