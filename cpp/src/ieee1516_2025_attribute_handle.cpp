#include "internal/attribute_handle.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kAttributeHandleEncodedLength = 8;

std::uint64_t handleValue(AttributeHandleImplementation const* implementation);

std::array<unsigned char, kAttributeHandleEncodedLength> encodeValue(std::uint64_t value) {
  std::array<unsigned char, kAttributeHandleEncodedLength> encoded{};
  for (std::size_t index = 0; index < encoded.size(); ++index) {
    std::size_t const shift = (encoded.size() - index - 1) * 8;
    encoded[index] = static_cast<unsigned char>(value >> shift);
  }
  return encoded;
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  if (encodedValue.size() != kAttributeHandleEncodedLength || encodedValue.data() == nullptr) {
    throw CouldNotDecode(L"An Umbra AttributeHandle must contain exactly eight bytes.");
  }

  auto const* bytes = static_cast<unsigned char const*>(encodedValue.data());
  std::uint64_t value = 0;
  for (unsigned char byte : std::array<unsigned char, kAttributeHandleEncodedLength>{
           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]}) {
    value = (value << 8) | byte;
  }
  return value;
}

}  // namespace

class AttributeHandleImplementation final {
 public:
  explicit AttributeHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(AttributeHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class AttributeHandleFriend final {
 public:
  static AttributeHandle make(std::uint64_t value) {
    return AttributeHandle(value == 0 ? nullptr : new AttributeHandleImplementation(value));
  }

  static AttributeHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(AttributeHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

AttributeHandle::AttributeHandle() : _impl(nullptr) {}

AttributeHandle::~AttributeHandle() noexcept {
  delete _impl;
}

AttributeHandle::AttributeHandle(AttributeHandle const& rhs)
    : _impl(rhs._impl == nullptr ? nullptr : new AttributeHandleImplementation(handleValue(rhs._impl))) {}

AttributeHandle& AttributeHandle::operator=(AttributeHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new AttributeHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool AttributeHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool AttributeHandle::operator==(AttributeHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool AttributeHandle::operator!=(AttributeHandle const& rhs) const {
  return !(*this == rhs);
}

bool AttributeHandle::operator<(AttributeHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long AttributeHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData AttributeHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void AttributeHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t AttributeHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kAttributeHandleEncodedLength) {
    throw CouldNotEncode(L"The AttributeHandle output buffer must contain at least eight bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t AttributeHandle::encodedLength() const {
  return kAttributeHandleEncodedLength;
}

std::wstring AttributeHandle::toString() const {
  if (!isValid()) {
    return L"AttributeHandle(invalid)";
  }
  return L"AttributeHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

AttributeHandleImplementation const* AttributeHandle::getImplementation() const {
  return _impl;
}

AttributeHandleImplementation* AttributeHandle::getImplementation() {
  return _impl;
}

AttributeHandle::AttributeHandle(AttributeHandleImplementation* implementation) : _impl(implementation) {}

AttributeHandle::AttributeHandle(VariableLengthData const& encodedValue) : _impl(nullptr) {
  *this = AttributeHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, AttributeHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

AttributeHandle makeAttributeHandle(std::uint64_t value) {
  return AttributeHandleFriend::make(value);
}

AttributeHandle decodeAttributeHandle(VariableLengthData const& encodedValue) {
  return AttributeHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> attributeHandleValue(AttributeHandle const& handle) noexcept {
  return AttributeHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
