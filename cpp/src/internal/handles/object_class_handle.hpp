#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// Private construction boundary for the official ObjectClassHandle value type.
// The federation registry owns the per-federation numeric directory; callers
// cross this boundary only through the standard support-service methods.
ObjectClassHandle makeObjectClassHandle(std::uint64_t value);
ObjectClassHandle decodeObjectClassHandle(VariableLengthData const& encodedValue);
[[nodiscard]] std::optional<std::uint64_t> objectClassHandleValue(
    ObjectClassHandle const& handle) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
