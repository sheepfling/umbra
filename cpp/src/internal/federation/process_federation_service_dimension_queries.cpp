#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"

#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleGetDimensionUpperBound(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const dimensionRequest =
      decodeProcessFederationDimensionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != dimensionRequest.federationName ||
        state->second.federateId != dimensionRequest.federateId ||
        !registry_.memberById(
            dimensionRequest.federationName, dimensionRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const upperBound = registry_.dimensionUpperBoundFor(
      dimensionRequest.federationName, dimensionRequest.dimensionHandle);
  if (upperBound &&
      !appendSelectedServiceReportRecord(
          session,
          dimensionRequest.federationName,
          dimensionRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::support_services),
          [dimensionHandle = dimensionRequest.dimensionHandle,
           upperBound = *upperBound](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetDimensionUpperBound",
                {{MomArgumentType::dimension_handle,
                  L"Dimension handle",
                  formatMomDimensionHandle(
                      rti1516_2025::umbra_binding_detail::makeDimensionHandle(
                          dimensionHandle))}},
                {MomArgumentType::number,
                 L"Dimension upper bound",
                 formatMomNumber(std::to_wstring(upperBound))});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationDimensionUpperBoundResult(
          ProcessFederationDimensionUpperBoundResult{
              upperBound.has_value(), upperBound.value_or(0UL)}));
}

TransportServiceMessage
ProcessFederationService::handleGetAvailableDimensionsForObjectClass(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const classRequest =
      decodeProcessFederationClassHandleRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != classRequest.federationName ||
        state->second.federateId != classRequest.federateId ||
        !registry_.memberById(
            classRequest.federationName, classRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const handles = registry_.availableDimensionsForObjectClass(
      classRequest.federationName, classRequest.classHandle);
  std::vector<std::uint64_t> encodedHandles;
  rti1516_2025::DimensionHandleSet reportHandles;
  if (handles) {
    encodedHandles.assign(handles->begin(), handles->end());
    for (auto const handle : *handles) {
      reportHandles.insert(
          rti1516_2025::umbra_binding_detail::makeDimensionHandle(handle));
    }
    if (!appendSelectedServiceReportRecord(
            session,
            classRequest.federationName,
            classRequest.federateId,
            static_cast<std::uint16_t>(MomServiceType::support_services),
            [classHandle = classRequest.classHandle,
             reportHandles = std::move(reportHandles)](
                std::uint32_t serialNumber) {
              return formatMomSuccessfulServiceReportRecord(
                  serialNumber,
                  L"GetAvailableDimensionsForObjectClass",
                  {{MomArgumentType::object_class_handle,
                    L"Object class handle",
                    formatMomObjectClassHandle(
                        rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                            classHandle))}},
                  {MomArgumentType::dimension_handle_set,
                   L"A set of dimension handles",
                   formatMomDimensionHandleSet(reportHandles)});
            })) {
      return internalError(request);
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAvailableDimensionsResult(
          ProcessFederationAvailableDimensionsResult{
              handles.has_value(), std::move(encodedHandles)}));
}

TransportServiceMessage
ProcessFederationService::handleGetAvailableDimensionsForInteractionClass(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const classRequest =
      decodeProcessFederationClassHandleRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != classRequest.federationName ||
        state->second.federateId != classRequest.federateId ||
        !registry_.memberById(
            classRequest.federationName, classRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const handles = registry_.availableDimensionsForInteractionClass(
      classRequest.federationName, classRequest.classHandle);
  std::vector<std::uint64_t> encodedHandles;
  rti1516_2025::DimensionHandleSet reportHandles;
  if (handles) {
    encodedHandles.assign(handles->begin(), handles->end());
    for (auto const handle : *handles) {
      reportHandles.insert(
          rti1516_2025::umbra_binding_detail::makeDimensionHandle(handle));
    }
    if (!appendSelectedServiceReportRecord(
            session,
            classRequest.federationName,
            classRequest.federateId,
            static_cast<std::uint16_t>(MomServiceType::support_services),
            [classHandle = classRequest.classHandle,
             reportHandles = std::move(reportHandles)](
                std::uint32_t serialNumber) {
              return formatMomSuccessfulServiceReportRecord(
                  serialNumber,
                  L"GetAvailableDimensionsForInteractionClass",
                  {{MomArgumentType::interaction_class_handle,
                    L"Interaction class handle",
                    formatMomInteractionClassHandle(
                        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                            classHandle))}},
                  {MomArgumentType::dimension_handle_set,
                   L"A set of dimension handles",
                   formatMomDimensionHandleSet(reportHandles)});
            })) {
      return internalError(request);
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAvailableDimensionsResult(
          ProcessFederationAvailableDimensionsResult{
              handles.has_value(), std::move(encodedHandles)}));
}

}  // namespace umbra::detail
