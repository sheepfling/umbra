#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// Private construction boundary for the official InteractionClassHandle value
// type. The federation registry owns the per-federation numeric directory;
// callers cross this boundary only through the standard support-service
// methods.
InteractionClassHandle makeInteractionClassHandle(std::uint64_t value);
InteractionClassHandle decodeInteractionClassHandle(VariableLengthData const& encodedValue);
[[nodiscard]] std::optional<std::uint64_t> interactionClassHandleValue(
    InteractionClassHandle const& handle) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
