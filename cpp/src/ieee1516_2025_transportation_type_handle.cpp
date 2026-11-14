#include "internal/handles/transportation_type_handle.hpp"
#include "internal/handles/2025_handle_definition.hpp"

#include <string_view>

namespace rti1516_2025 {

UMBRA_2025_DEFINE_HANDLE(TransportationType, transportationType, TransportationType)

namespace umbra_binding_detail {

std::optional<std::uint64_t> standardTransportationTypeValue(
    std::wstring_view name) noexcept {
  if (name == L"HLAreliable") {
    return kHlaReliableTransportationTypeValue;
  }
  if (name == L"HLAbestEffort") {
    return kHlaBestEffortTransportationTypeValue;
  }
  return std::nullopt;
}

std::optional<std::wstring_view> standardTransportationTypeName(
    std::uint64_t value) noexcept {
  switch (value) {
    case kHlaReliableTransportationTypeValue:
      return L"HLAreliable";
    case kHlaBestEffortTransportationTypeValue:
      return L"HLAbestEffort";
    default:
      return std::nullopt;
  }
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
