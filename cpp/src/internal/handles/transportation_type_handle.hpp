#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>
#include <string_view>

namespace rti1516_2025::umbra_binding_detail {

// Private construction and mandatory-name boundary for the official
// TransportationTypeHandle value type. IEEE 1516.1-2025 requires every RTI to
// support HLAreliable and HLAbestEffort. Additional transportation types are
// execution-scoped FOM declarations resolved by the composed catalog.
TransportationTypeHandle makeTransportationTypeHandle(std::uint64_t value);
TransportationTypeHandle decodeTransportationTypeHandle(VariableLengthData const& encodedValue);
[[nodiscard]] std::optional<std::uint64_t> transportationTypeHandleValue(
    TransportationTypeHandle const& handle) noexcept;
[[nodiscard]] std::optional<std::uint64_t> standardTransportationTypeValue(
    std::wstring_view name) noexcept;
[[nodiscard]] std::optional<std::wstring_view> standardTransportationTypeName(
    std::uint64_t value) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
