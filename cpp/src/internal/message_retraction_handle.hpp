#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// Private construction boundary for the official MessageRetractionHandle
// value type.  Message identifiers are allocated by the federation-owned
// temporal coordinator; the public RTI services cross this boundary only
// through the standard handle methods.
MessageRetractionHandle makeMessageRetractionHandle(std::uint64_t value);
MessageRetractionHandle decodeMessageRetractionHandle(
    VariableLengthData const& encodedValue);
[[nodiscard]] std::optional<std::uint64_t> messageRetractionHandleValue(
    MessageRetractionHandle const& handle) noexcept;

}  // namespace rti1516_2025::umbra_binding_detail
