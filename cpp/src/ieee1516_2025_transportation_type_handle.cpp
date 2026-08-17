#include "internal/transportation_type_handle.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kTransportationTypeHandleEncodedLength = 8;
constexpr std::uint64_t kHlaReliableTransportationType = 1;
constexpr std::uint64_t kHlaBestEffortTransportationType = 2;

std::uint64_t handleValue(TransportationTypeHandleImplementation const* implementation);

std::array<unsigned char, kTransportationTypeHandleEncodedLength> encodeValue(std::uint64_t value) {
  std::array<unsigned char, kTransportationTypeHandleEncodedLength> encoded{};
  for (std::size_t index = 0; index < encoded.size(); ++index) {
    std::size_t const shift = (encoded.size() - index - 1) * 8;
    encoded[index] = static_cast<unsigned char>(value >> shift);
  }
  return encoded;
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  if (encodedValue.size() != kTransportationTypeHandleEncodedLength ||
      encodedValue.data() == nullptr) {
    throw CouldNotDecode(L"An Umbra TransportationTypeHandle must contain exactly eight bytes.");
  }

  auto const* bytes = static_cast<unsigned char const*>(encodedValue.data());
  std::uint64_t value = 0;
  for (unsigned char byte : std::array<unsigned char, kTransportationTypeHandleEncodedLength>{
           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]}) {
    value = (value << 8) | byte;
  }
  return value;
}

}  // namespace

class TransportationTypeHandleImplementation final {
 public:
  explicit TransportationTypeHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(TransportationTypeHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class TransportationTypeHandleFriend final {
 public:
  static TransportationTypeHandle make(std::uint64_t value) {
    return TransportationTypeHandle(
        value == 0 ? nullptr : new TransportationTypeHandleImplementation(value));
  }

  static TransportationTypeHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(
      TransportationTypeHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

TransportationTypeHandle::TransportationTypeHandle() : _impl(nullptr) {}

TransportationTypeHandle::~TransportationTypeHandle() noexcept {
  delete _impl;
}

TransportationTypeHandle::TransportationTypeHandle(TransportationTypeHandle const& rhs)
    : _impl(rhs._impl == nullptr
          ? nullptr
          : new TransportationTypeHandleImplementation(handleValue(rhs._impl))) {}

TransportationTypeHandle& TransportationTypeHandle::operator=(TransportationTypeHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new TransportationTypeHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool TransportationTypeHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool TransportationTypeHandle::operator==(TransportationTypeHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool TransportationTypeHandle::operator!=(TransportationTypeHandle const& rhs) const {
  return !(*this == rhs);
}

bool TransportationTypeHandle::operator<(TransportationTypeHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long TransportationTypeHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData TransportationTypeHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void TransportationTypeHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t TransportationTypeHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kTransportationTypeHandleEncodedLength) {
    throw CouldNotEncode(
        L"The TransportationTypeHandle output buffer must contain at least eight bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t TransportationTypeHandle::encodedLength() const {
  return kTransportationTypeHandleEncodedLength;
}

std::wstring TransportationTypeHandle::toString() const {
  if (!isValid()) {
    return L"TransportationTypeHandle(invalid)";
  }
  return L"TransportationTypeHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

TransportationTypeHandleImplementation const* TransportationTypeHandle::getImplementation() const {
  return _impl;
}

TransportationTypeHandleImplementation* TransportationTypeHandle::getImplementation() {
  return _impl;
}

TransportationTypeHandle::TransportationTypeHandle(
    TransportationTypeHandleImplementation* implementation) : _impl(implementation) {}

TransportationTypeHandle::TransportationTypeHandle(VariableLengthData const& encodedValue)
    : _impl(nullptr) {
  *this = TransportationTypeHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(
    std::wostream& stream,
    TransportationTypeHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

TransportationTypeHandle makeTransportationTypeHandle(std::uint64_t value) {
  return TransportationTypeHandleFriend::make(value);
}

TransportationTypeHandle decodeTransportationTypeHandle(VariableLengthData const& encodedValue) {
  return TransportationTypeHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> transportationTypeHandleValue(
    TransportationTypeHandle const& handle) noexcept {
  return TransportationTypeHandleFriend::value(handle);
}

std::optional<std::uint64_t> standardTransportationTypeValue(std::wstring_view name) noexcept {
  if (name == L"HLAreliable") {
    return kHlaReliableTransportationType;
  }
  if (name == L"HLAbestEffort") {
    return kHlaBestEffortTransportationType;
  }
  return std::nullopt;
}

std::optional<std::wstring_view> standardTransportationTypeName(std::uint64_t value) noexcept {
  switch (value) {
    case kHlaReliableTransportationType:
      return L"HLAreliable";
    case kHlaBestEffortTransportationType:
      return L"HLAbestEffort";
    default:
      return std::nullopt;
  }
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
