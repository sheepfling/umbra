#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// Private construction boundary for the official FederateHandle value type.
// Federation services keep identities as integers internally and cross this
// boundary only when a standard API needs to return or consume a handle.
FederateHandle makeFederateHandle(std::uint64_t value);
FederateHandle decodeFederateHandle(VariableLengthData const& encodedValue);

// Extracts the private integer identity from an Umbra-created official handle.
// A default-constructed (or zero-valued) handle has no usable identity.
[[nodiscard]] std::optional<std::uint64_t> federateHandleValue(
    FederateHandle const& handle) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
