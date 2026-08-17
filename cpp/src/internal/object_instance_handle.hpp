#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// Private construction boundary for the official ObjectInstanceHandle value
// type. The federation registry allocates execution-wide numeric identities;
// callers cross this boundary only through the standard object-management and
// support-service methods.
ObjectInstanceHandle makeObjectInstanceHandle(std::uint64_t value);
ObjectInstanceHandle decodeObjectInstanceHandle(VariableLengthData const& encodedValue);
[[nodiscard]] std::optional<std::uint64_t> objectInstanceHandleValue(
    ObjectInstanceHandle const& handle) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
