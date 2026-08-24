#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// Private construction boundary for the official ParameterHandle value type.
// The federation registry owns the per-federation numeric directory; callers
// cross this boundary only through the standard support-service methods.
ParameterHandle makeParameterHandle(std::uint64_t value);
ParameterHandle decodeParameterHandle(VariableLengthData const& encodedValue);
[[nodiscard]] std::optional<std::uint64_t> parameterHandleValue(
    ParameterHandle const& handle) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
