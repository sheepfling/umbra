#include "internal/handles/federate_handle.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kFederateHandleEncodedLength =
    umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength;

std::uint64_t handleValue(FederateHandleImplementation const* implementation);

auto encodeValue(std::uint64_t value) {
  return umbra_binding_detail::encodeUmbraHandleVariableArray(value);
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue);
  if (!value.has_value()) {
    throw CouldNotDecode(
        L"An Umbra FederateHandle must contain an HLAvariableArray with exactly eight HLAbyte elements.");
  }
  return *value;
}

}  // namespace

class FederateHandleImplementation final {
 public:
  explicit FederateHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(FederateHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class FederateHandleFriend final {
 public:
  static FederateHandle make(std::uint64_t value) {
    return FederateHandle(value == 0 ? nullptr : new FederateHandleImplementation(value));
  }

  static FederateHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(FederateHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

FederateHandle::FederateHandle() : _impl(nullptr) {}

FederateHandle::~FederateHandle() noexcept {
  delete _impl;
}

FederateHandle::FederateHandle(FederateHandle const& rhs)
    : _impl(rhs._impl == nullptr ? nullptr : new FederateHandleImplementation(handleValue(rhs._impl))) {}

FederateHandle& FederateHandle::operator=(FederateHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new FederateHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool FederateHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool FederateHandle::operator==(FederateHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool FederateHandle::operator!=(FederateHandle const& rhs) const {
  return !(*this == rhs);
}

bool FederateHandle::operator<(FederateHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long FederateHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData FederateHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void FederateHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t FederateHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kFederateHandleEncodedLength) {
    throw CouldNotEncode(L"The FederateHandle output buffer must contain at least twelve bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t FederateHandle::encodedLength() const {
  return kFederateHandleEncodedLength;
}

std::wstring FederateHandle::toString() const {
  if (!isValid()) {
    return L"FederateHandle(invalid)";
  }
  return L"FederateHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

FederateHandleImplementation const* FederateHandle::getImplementation() const {
  return _impl;
}

FederateHandleImplementation* FederateHandle::getImplementation() {
  return _impl;
}

FederateHandle::FederateHandle(FederateHandleImplementation* implementation) : _impl(implementation) {}

FederateHandle::FederateHandle(VariableLengthData const& encodedValue)
    : _impl(nullptr) {
  *this = FederateHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, FederateHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

FederateHandle makeFederateHandle(std::uint64_t value) {
  return FederateHandleFriend::make(value);
}

FederateHandle decodeFederateHandle(VariableLengthData const& encodedValue) {
  return FederateHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> federateHandleValue(FederateHandle const& handle) noexcept {
  return FederateHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
