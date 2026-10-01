#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace umbra {
namespace detail {

// Wire encodings must name their byte order at the call site.  In particular,
// this keeps host endianness out of every HLA encoder and makes the wire
// contract visible beside the value being serialized.
enum class ByteOrder { big, little };

template <typename Byte, typename Unsigned>
void appendUnsigned(
    std::vector<Byte> &output,
    Unsigned value,
    ByteOrder order) {
  static_assert(std::is_unsigned<Unsigned>::value, "Unsigned wire values are required");
  for (std::size_t index = 0U; index < sizeof(Unsigned); ++index) {
    auto const shift = order == ByteOrder::big
        ? (sizeof(Unsigned) - index - 1U) * 8U
        : index * 8U;
    output.push_back(static_cast<Byte>((value >> shift) & static_cast<Unsigned>(0xffU)));
  }
}

template <typename Byte, typename Unsigned>
bool readUnsigned(
    std::vector<Byte> const &input,
    std::size_t index,
    ByteOrder order,
    Unsigned &value) noexcept {
  static_assert(std::is_unsigned<Unsigned>::value, "Unsigned wire values are required");
  if (index > input.size() || input.size() - index < sizeof(Unsigned)) {
    return false;
  }

  value = 0U;
  for (std::size_t offset = 0U; offset < sizeof(Unsigned); ++offset) {
    auto const shift = order == ByteOrder::big
        ? (sizeof(Unsigned) - offset - 1U) * 8U
        : offset * 8U;
    value |= static_cast<Unsigned>(static_cast<std::uint8_t>(input[index + offset])) << shift;
  }
  return true;
}

template <typename Unsigned>
std::array<unsigned char, sizeof(Unsigned)> encodeUnsigned(
    Unsigned value,
    ByteOrder order) noexcept {
  static_assert(std::is_unsigned<Unsigned>::value, "Unsigned wire values are required");
  std::array<unsigned char, sizeof(Unsigned)> encoded{};
  for (std::size_t index = 0U; index < sizeof(Unsigned); ++index) {
    auto const shift = order == ByteOrder::big
        ? (sizeof(Unsigned) - index - 1U) * 8U
        : index * 8U;
    encoded[index] = static_cast<unsigned char>((value >> shift) & static_cast<Unsigned>(0xffU));
  }
  return encoded;
}

template <typename Unsigned>
void writeUnsigned(
    Unsigned value,
    unsigned char *output,
    ByteOrder order) noexcept {
  auto const encoded = encodeUnsigned(value, order);
  for (std::size_t index = 0U; index < encoded.size(); ++index) {
    output[index] = encoded[index];
  }
}

template <typename Unsigned>
bool readUnsigned(
    void const *input,
    std::size_t size,
    ByteOrder order,
    Unsigned &value) noexcept {
  static_assert(std::is_unsigned<Unsigned>::value, "Unsigned wire values are required");
  if (input == nullptr || size != sizeof(Unsigned)) {
    return false;
  }
  auto const *bytes = static_cast<unsigned char const *>(input);
  value = 0U;
  for (std::size_t index = 0U; index < sizeof(Unsigned); ++index) {
    auto const shift = order == ByteOrder::big
        ? (sizeof(Unsigned) - index - 1U) * 8U
        : index * 8U;
    value |= static_cast<Unsigned>(bytes[index]) << shift;
  }
  return true;
}

}  // namespace detail
}  // namespace umbra
