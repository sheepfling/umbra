#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_codec_support.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <iterator>
#include <set>
#include <utility>

#include "internal/federation/process_federation_service_payload_helpers.hpp"
namespace umbra::detail {
 using namespace process_federation_service_codec_support;
using process_federation_payload::parameterVector;

std::vector<std::uint8_t>
encodeProcessFederationGetFederateHandleRequest(
    ProcessFederationGetFederateHandleRequest const& request) {
  if (request.federationName.empty() || request.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-handle lookup requires execution and federate names.");
  }
  requireNonzero(
      request.federateId,
      "A process federate-handle lookup requires a requesting federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.federateName);
  return std::move(writer).finish();
}

ProcessFederationGetFederateHandleRequest
decodeProcessFederationGetFederateHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetFederateHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.federateName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-handle lookup requires execution and federate names.");
  }
  requireNonzero(
      result.federateId,
      "A process federate-handle lookup requires a requesting federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetFederateNameRequest(
    ProcessFederationGetFederateNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federate-name lookup requires a requesting federate identity.");
  requireNonzero(
      request.targetFederateId,
      "A process federate-name lookup requires a target federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.targetFederateId);
  return std::move(writer).finish();
}

ProcessFederationGetFederateNameRequest
decodeProcessFederationGetFederateNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetFederateNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.targetFederateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federate-name lookup requires a requesting federate identity.");
  requireNonzero(
      result.targetFederateId,
      "A process federate-name lookup requires a target federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationNormalizeHandleRequest(
    ProcessFederationNormalizeHandleRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process handle-normalization request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process handle-normalization request requires a requesting federate identity.");
  requireNonzero(
      request.handle,
      "A process handle-normalization request requires a non-zero handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.handle);
  return std::move(writer).finish();
}

ProcessFederationNormalizeHandleRequest
decodeProcessFederationNormalizeHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationNormalizeHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.handle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process handle-normalization request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process handle-normalization request requires a requesting federate identity.");
  requireNonzero(
      result.handle,
      "A process handle-normalization request requires a non-zero handle.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetInteractionClassHandleRequest(
    ProcessFederationGetInteractionClassHandleRequest const& request) {
  if (request.federationName.empty() || request.interactionClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class lookup requires execution and class names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation interaction-class lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.interactionClassName);
  return std::move(writer).finish();
}

ProcessFederationGetInteractionClassHandleRequest
decodeProcessFederationGetInteractionClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetInteractionClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.interactionClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class lookup requires execution and class names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation interaction-class lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectClassHandleRequest(
    ProcessFederationGetObjectClassHandleRequest const& request) {
  if (request.federationName.empty() || request.objectClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class lookup requires execution and class names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.objectClassName);
  writer.unsigned8(request.callbacksEnabled ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationGetObjectClassHandleRequest
decodeProcessFederationGetObjectClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassName = reader.wideString();
  if (reader.remaining() != 0U) {
    result.callbacksEnabled = reader.unsigned8() != 0U;
  }
  reader.finish();
  if (result.federationName.empty() || result.objectClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class lookup requires execution and class names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectClassNameRequest(
    ProcessFederationGetObjectClassNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class name lookup requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object-class name lookup requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  return std::move(writer).finish();
}

ProcessFederationGetObjectClassNameRequest
decodeProcessFederationGetObjectClassNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectClassNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class name lookup requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object-class name lookup requires an object class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetInteractionClassNameRequest(
    ProcessFederationGetInteractionClassNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation interaction-class name lookup requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation interaction-class name lookup requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  return std::move(writer).finish();
}

ProcessFederationGetInteractionClassNameRequest
decodeProcessFederationGetInteractionClassNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetInteractionClassNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation interaction-class name lookup requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation interaction-class name lookup requires an interaction class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetAttributeNameRequest(
    ProcessFederationGetAttributeNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute-name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation attribute-name lookup requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation attribute-name lookup requires an object class.");
  requireNonzero(
      request.attributeHandle,
      "A process federation attribute-name lookup requires an attribute.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64(request.attributeHandle);
  return std::move(writer).finish();
}

ProcessFederationGetAttributeNameRequest
decodeProcessFederationGetAttributeNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetAttributeNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute-name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation attribute-name lookup requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation attribute-name lookup requires an object class.");
  requireNonzero(
      result.attributeHandle,
      "A process federation attribute-name lookup requires an attribute.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetParameterNameRequest(
    ProcessFederationGetParameterNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter-name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation parameter-name lookup requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation parameter-name lookup requires an interaction class.");
  requireNonzero(
      request.parameterHandle,
      "A process federation parameter-name lookup requires a parameter.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64(request.parameterHandle);
  return std::move(writer).finish();
}

ProcessFederationGetParameterNameRequest
decodeProcessFederationGetParameterNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetParameterNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.parameterHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter-name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation parameter-name lookup requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation parameter-name lookup requires an interaction class.");
  requireNonzero(
      result.parameterHandle,
      "A process federation parameter-name lookup requires a parameter.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationGetParameterHandleRequest(
    ProcessFederationGetParameterHandleRequest const& request) {
  if (request.federationName.empty() || request.parameterName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter lookup requires execution and parameter names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation parameter lookup requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation parameter lookup requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.wideString(request.parameterName);
  return std::move(writer).finish();
}

ProcessFederationGetParameterHandleRequest
decodeProcessFederationGetParameterHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetParameterHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.parameterName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.parameterName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter lookup requires execution and parameter names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation parameter lookup requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation parameter lookup requires an interaction class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetAttributeHandleRequest(
    ProcessFederationGetAttributeHandleRequest const& request) {
  if (request.federationName.empty() || request.attributeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute lookup requires execution and attribute names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation attribute lookup requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation attribute lookup requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.wideString(request.attributeName);
  return std::move(writer).finish();
}

ProcessFederationGetAttributeHandleRequest
decodeProcessFederationGetAttributeHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetAttributeHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.attributeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute lookup requires execution and attribute names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation attribute lookup requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation attribute lookup requires an object class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectInstanceHandleRequest(
    ProcessFederationGetObjectInstanceHandleRequest const& request) {
  if (request.federationName.empty() || request.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance handle lookup requires execution and instance names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-instance handle lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.objectInstanceName);
  return std::move(writer).finish();
}

ProcessFederationGetObjectInstanceHandleRequest
decodeProcessFederationGetObjectInstanceHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectInstanceHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance handle lookup requires execution and instance names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-instance handle lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectInstanceNameRequest(
    ProcessFederationGetObjectInstanceNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-instance name lookup requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process federation object-instance name lookup requires an object instance.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  return std::move(writer).finish();
}

ProcessFederationGetObjectInstanceNameRequest
decodeProcessFederationGetObjectInstanceNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectInstanceNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-instance name lookup requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process federation object-instance name lookup requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetKnownObjectClassHandleRequest(
    ProcessFederationGetKnownObjectClassHandleRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process known-object class lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process known-object class lookup requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process known-object class lookup requires an object instance.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  return std::move(writer).finish();
}

ProcessFederationGetKnownObjectClassHandleRequest
decodeProcessFederationGetKnownObjectClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetKnownObjectClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process known-object class lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process known-object class lookup requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process known-object class lookup requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetUpdateRateValueRequest(
    ProcessFederationGetUpdateRateValueRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process update-rate lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process update-rate lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.string(request.updateRateDesignator);
  return std::move(writer).finish();
}

ProcessFederationGetUpdateRateValueRequest
decodeProcessFederationGetUpdateRateValueRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetUpdateRateValueRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.updateRateDesignator = reader.string();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process update-rate lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process update-rate lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetUpdateRateValueForAttributeRequest(
    ProcessFederationGetUpdateRateValueForAttributeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute update-rate lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process attribute update-rate lookup requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute update-rate lookup requires an object instance.");
  requireNonzero(
      request.attributeHandle,
      "A process attribute update-rate lookup requires an attribute.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64(request.attributeHandle);
  return std::move(writer).finish();
}

ProcessFederationGetUpdateRateValueForAttributeRequest
decodeProcessFederationGetUpdateRateValueForAttributeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetUpdateRateValueForAttributeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute update-rate lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process attribute update-rate lookup requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute update-rate lookup requires an object instance.");
  requireNonzero(
      result.attributeHandle,
      "A process attribute update-rate lookup requires an attribute.");
  return result;
}
}  // namespace umbra::detail
