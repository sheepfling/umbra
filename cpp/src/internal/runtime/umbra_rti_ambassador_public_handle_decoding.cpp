#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

namespace rti1516_2025::umbra_binding_detail {

// Public handle-decoding service implementations.

FederateHandle UmbraRtiAmbassador::decodeFederateHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeFederateHandle");
  try {
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Federate Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Federate Handle", exception);
    throw;
  }
}

ObjectClassHandle UmbraRtiAmbassador::decodeObjectClassHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeObjectClassHandle");
  try {
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Object Class Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Object Class Handle", exception);
    throw;
  }
}

InteractionClassHandle UmbraRtiAmbassador::decodeInteractionClassHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeInteractionClassHandle");
  try {
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Interaction Class Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeInteractionClassHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Interaction Class Handle", exception);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::decodeObjectInstanceHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeObjectInstanceHandle");
  try {
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Object Instance Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Object Instance Handle", exception);
    throw;
  }
}

AttributeHandle UmbraRtiAmbassador::decodeAttributeHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeAttributeHandle");
  try {
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Attribute Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeAttributeHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Attribute Handle", exception);
    throw;
  }
}

ParameterHandle UmbraRtiAmbassador::decodeParameterHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeParameterHandle");
  try {
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Parameter Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeParameterHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Parameter Handle", exception);
    throw;
  }
}

DimensionHandle UmbraRtiAmbassador::decodeDimensionHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeDimensionHandle");
  try {
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Dimension Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeDimensionHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Dimension Handle", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
