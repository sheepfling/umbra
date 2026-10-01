#include "internal/handles/transportation_type_handle.hpp"
#include "internal/handles/2025_handle_definition.hpp"

#include <string_view>

namespace rti1516_2025 {

UMBRA_2025_DEFINE_HANDLE(TransportationType, transportationType)

namespace {

constexpr std::uint64_t kHlaReliableTransportationType = 1;
constexpr std::uint64_t kHlaBestEffortTransportationType = 2;

}  // namespace

namespace umbra_binding_detail {

std::optional<std::uint64_t> standardTransportationTypeValue(
    std::wstring_view name) noexcept {
  if (name == L"HLAreliable") {
    return kHlaReliableTransportationType;
  }
  if (name == L"HLAbestEffort") {
    return kHlaBestEffortTransportationType;
  }
  return std::nullopt;
}

std::optional<std::wstring_view> standardTransportationTypeName(
    std::uint64_t value) noexcept {
  switch (value) {
    case kHlaReliableTransportationType:
      return L"HLAreliable";
    case kHlaBestEffortTransportationType:
      return L"HLAbestEffort";
    default:
      return std::nullopt;
  }
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
