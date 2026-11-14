#pragma once

#include "internal/encoding/byte_order.hpp"

#include <RTI/VariableLengthData.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// The standard MIM declares each public handle value as an
// HLAvariableArray<HLAbyte>. Umbra uses a nine-octet payload: one stable
// handle-kind discriminator followed by the eight-octet handle identity.
// Keep these discriminator values stable across language bindings so a value
// encoded by one API can be reconstituted by another API as the same type.
enum class HlaHandleType : unsigned char {
  Federate = 0x01U,
  ObjectClass = 0x02U,
  InteractionClass = 0x03U,
  ObjectInstance = 0x04U,
  Attribute = 0x05U,
  Parameter = 0x06U,
  Dimension = 0x07U,
  MessageRetraction = 0x08U,
  Region = 0x09U,
  TransportationType = 0x0aU,
};

inline constexpr std::size_t kUmbraHandleIdentityOctetCount = 8U;
inline constexpr std::size_t kUmbraHandleTypeOctetCount = 1U;
inline constexpr std::size_t kHlaVariableArrayCountOctetCount = 4U;
inline constexpr std::uint32_t kUmbraHandleVariableArrayElementCount =
    static_cast<std::uint32_t>(
        kUmbraHandleIdentityOctetCount + kUmbraHandleTypeOctetCount);
inline constexpr std::size_t kUmbraHandleVariableArrayEncodedLength =
    kHlaVariableArrayCountOctetCount +
    static_cast<std::size_t>(kUmbraHandleVariableArrayElementCount);

[[nodiscard]] inline std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength>
encodeUmbraHandleVariableArray(std::uint64_t value, HlaHandleType type) noexcept {
  std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength> encoded{};
  auto const count = umbra::detail::encodeUnsigned(
      kUmbraHandleVariableArrayElementCount, umbra::detail::ByteOrder::big);
  auto const identity = umbra::detail::encodeUnsigned(
      value, umbra::detail::ByteOrder::big);
  std::memcpy(encoded.data(), count.data(), count.size());
  encoded[kHlaVariableArrayCountOctetCount] = static_cast<unsigned char>(type);
  std::memcpy(
      encoded.data() + kHlaVariableArrayCountOctetCount +
          kUmbraHandleTypeOctetCount,
      identity.data(),
      identity.size());
  return encoded;
}

[[nodiscard]] inline std::optional<std::uint64_t> decodeUmbraHandleVariableArray(
    VariableLengthData const& encodedValue,
    HlaHandleType expectedType) noexcept {
  if (encodedValue.size() != kUmbraHandleVariableArrayEncodedLength ||
      encodedValue.data() == nullptr) {
    return std::nullopt;
  }

  auto const* bytes = static_cast<unsigned char const*>(encodedValue.data());
  std::uint32_t elementCount = 0U;
  if (!umbra::detail::readUnsigned(
          bytes,
          kHlaVariableArrayCountOctetCount,
          umbra::detail::ByteOrder::big,
          elementCount) ||
      elementCount != kUmbraHandleVariableArrayElementCount ||
      bytes[kHlaVariableArrayCountOctetCount] !=
          static_cast<unsigned char>(expectedType)) {
    return std::nullopt;
  }

  std::uint64_t value = 0U;
  if (!umbra::detail::readUnsigned(
          bytes + kHlaVariableArrayCountOctetCount +
              kUmbraHandleTypeOctetCount,
          kUmbraHandleIdentityOctetCount,
          umbra::detail::ByteOrder::big,
          value)) {
    return std::nullopt;
  }
  return value;
}

}  // namespace rti1516_2025::umbra_binding_detail
