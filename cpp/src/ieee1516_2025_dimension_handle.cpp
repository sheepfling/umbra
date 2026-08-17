#include "internal/dimension_handle.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kDimensionHandleEncodedLength = 8;

std::uint64_t handleValue(DimensionHandleImplementation const* implementation);

std::array<unsigned char, kDimensionHandleEncodedLength> encodeValue(std::uint64_t value) {
  std::array<unsigned char, kDimensionHandleEncodedLength> encoded{};
  for (std::size_t index = 0; index < encoded.size(); ++index) {
    std::size_t const shift = (encoded.size() - index - 1) * 8;
    encoded[index] = static_cast<unsigned char>(value >> shift);
  }
  return encoded;
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  if (encodedValue.size() != kDimensionHandleEncodedLength || encodedValue.data() == nullptr) {
    throw CouldNotDecode(L"An Umbra DimensionHandle must contain exactly eight bytes.");
  }

  auto const* bytes = static_cast<unsigned char const*>(encodedValue.data());
  std::uint64_t value = 0;
  for (unsigned char byte : std::array<unsigned char, kDimensionHandleEncodedLength>{
           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]}) {
    value = (value << 8) | byte;
  }
  return value;
}

}  // namespace

class DimensionHandleImplementation final {
 public:
  explicit DimensionHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(DimensionHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class DimensionHandleFriend final {
 public:
  static DimensionHandle make(std::uint64_t value) {
    return DimensionHandle(value == 0 ? nullptr : new DimensionHandleImplementation(value));
  }

  static DimensionHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(DimensionHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

DimensionHandle::DimensionHandle() : _impl(nullptr) {}

DimensionHandle::~DimensionHandle() noexcept {
  delete _impl;
}

DimensionHandle::DimensionHandle(DimensionHandle const& rhs)
    : _impl(rhs._impl == nullptr ? nullptr : new DimensionHandleImplementation(handleValue(rhs._impl))) {}

DimensionHandle& DimensionHandle::operator=(DimensionHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new DimensionHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool DimensionHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool DimensionHandle::operator==(DimensionHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool DimensionHandle::operator!=(DimensionHandle const& rhs) const {
  return !(*this == rhs);
}

bool DimensionHandle::operator<(DimensionHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long DimensionHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData DimensionHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void DimensionHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t DimensionHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kDimensionHandleEncodedLength) {
    throw CouldNotEncode(L"The DimensionHandle output buffer must contain at least eight bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t DimensionHandle::encodedLength() const {
  return kDimensionHandleEncodedLength;
}

std::wstring DimensionHandle::toString() const {
  if (!isValid()) {
    return L"DimensionHandle(invalid)";
  }
  return L"DimensionHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

DimensionHandleImplementation const* DimensionHandle::getImplementation() const {
  return _impl;
}

DimensionHandleImplementation* DimensionHandle::getImplementation() {
  return _impl;
}

DimensionHandle::DimensionHandle(DimensionHandleImplementation* implementation)
    : _impl(implementation) {}

DimensionHandle::DimensionHandle(VariableLengthData const& encodedValue) : _impl(nullptr) {
  *this = DimensionHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, DimensionHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

DimensionHandle makeDimensionHandle(std::uint64_t value) {
  return DimensionHandleFriend::make(value);
}

DimensionHandle decodeDimensionHandle(VariableLengthData const& encodedValue) {
  return DimensionHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> dimensionHandleValue(DimensionHandle const& handle) noexcept {
  return DimensionHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
