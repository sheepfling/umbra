#include "internal/parameter_handle.hpp"
#include "internal/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kParameterHandleEncodedLength =
    umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength;

std::uint64_t handleValue(ParameterHandleImplementation const* implementation);

auto encodeValue(std::uint64_t value) {
  return umbra_binding_detail::encodeUmbraHandleVariableArray(value);
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue);
  if (!value.has_value()) {
    throw CouldNotDecode(
        L"An Umbra ParameterHandle must contain an HLAvariableArray with exactly eight HLAbyte elements.");
  }
  return *value;
}

}  // namespace

class ParameterHandleImplementation final {
 public:
  explicit ParameterHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(ParameterHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class ParameterHandleFriend final {
 public:
  static ParameterHandle make(std::uint64_t value) {
    return ParameterHandle(value == 0 ? nullptr : new ParameterHandleImplementation(value));
  }

  static ParameterHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(ParameterHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

ParameterHandle::ParameterHandle() : _impl(nullptr) {}

ParameterHandle::~ParameterHandle() noexcept {
  delete _impl;
}

ParameterHandle::ParameterHandle(ParameterHandle const& rhs)
    : _impl(rhs._impl == nullptr ? nullptr : new ParameterHandleImplementation(handleValue(rhs._impl))) {}

ParameterHandle& ParameterHandle::operator=(ParameterHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new ParameterHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool ParameterHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool ParameterHandle::operator==(ParameterHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool ParameterHandle::operator!=(ParameterHandle const& rhs) const {
  return !(*this == rhs);
}

bool ParameterHandle::operator<(ParameterHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long ParameterHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData ParameterHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void ParameterHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t ParameterHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kParameterHandleEncodedLength) {
    throw CouldNotEncode(L"The ParameterHandle output buffer must contain at least twelve bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t ParameterHandle::encodedLength() const {
  return kParameterHandleEncodedLength;
}

std::wstring ParameterHandle::toString() const {
  if (!isValid()) {
    return L"ParameterHandle(invalid)";
  }
  return L"ParameterHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

ParameterHandleImplementation const* ParameterHandle::getImplementation() const {
  return _impl;
}

ParameterHandleImplementation* ParameterHandle::getImplementation() {
  return _impl;
}

ParameterHandle::ParameterHandle(ParameterHandleImplementation* implementation) : _impl(implementation) {}

ParameterHandle::ParameterHandle(VariableLengthData const& encodedValue) : _impl(nullptr) {
  *this = ParameterHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, ParameterHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

ParameterHandle makeParameterHandle(std::uint64_t value) {
  return ParameterHandleFriend::make(value);
}

ParameterHandle decodeParameterHandle(VariableLengthData const& encodedValue) {
  return ParameterHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> parameterHandleValue(ParameterHandle const& handle) noexcept {
  return ParameterHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
