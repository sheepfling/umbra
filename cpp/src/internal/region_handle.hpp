#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// Private construction boundary for the official RegionHandle value type.
// Region ownership and lifecycle remain federation-registry state; callers
// cross this boundary only through the standard RTIambassador services.
RegionHandle makeRegionHandle(std::uint64_t value);
RegionHandle decodeRegionHandle(VariableLengthData const& encodedValue);
[[nodiscard]] std::optional<std::uint64_t> regionHandleValue(
    RegionHandle const& handle) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
