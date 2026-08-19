#include "internal/interaction_class_handle.hpp"
#include "internal/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kInteractionClassHandleEncodedLength =
    umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength;

std::uint64_t handleValue(InteractionClassHandleImplementation const* implementation);

auto encodeValue(std::uint64_t value) {
  return umbra_binding_detail::encodeUmbraHandleVariableArray(value);
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue);
  if (!value.has_value()) {
    throw CouldNotDecode(
        L"An Umbra InteractionClassHandle must contain an HLAvariableArray with exactly eight HLAbyte elements.");
  }
  return *value;
}

}  // namespace

class InteractionClassHandleImplementation final {
 public:
  explicit InteractionClassHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(InteractionClassHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class InteractionClassHandleFriend final {
 public:
  static InteractionClassHandle make(std::uint64_t value) {
    return InteractionClassHandle(
        value == 0 ? nullptr : new InteractionClassHandleImplementation(value));
  }

  static InteractionClassHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(InteractionClassHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

InteractionClassHandle::InteractionClassHandle() : _impl(nullptr) {}

InteractionClassHandle::~InteractionClassHandle() noexcept {
  delete _impl;
}

InteractionClassHandle::InteractionClassHandle(InteractionClassHandle const& rhs)
    : _impl(rhs._impl == nullptr
          ? nullptr
          : new InteractionClassHandleImplementation(handleValue(rhs._impl))) {}

InteractionClassHandle& InteractionClassHandle::operator=(InteractionClassHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new InteractionClassHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool InteractionClassHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool InteractionClassHandle::operator==(InteractionClassHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool InteractionClassHandle::operator!=(InteractionClassHandle const& rhs) const {
  return !(*this == rhs);
}

bool InteractionClassHandle::operator<(InteractionClassHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long InteractionClassHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData InteractionClassHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void InteractionClassHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t InteractionClassHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kInteractionClassHandleEncodedLength) {
    throw CouldNotEncode(
        L"The InteractionClassHandle output buffer must contain at least twelve bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t InteractionClassHandle::encodedLength() const {
  return kInteractionClassHandleEncodedLength;
}

std::wstring InteractionClassHandle::toString() const {
  if (!isValid()) {
    return L"InteractionClassHandle(invalid)";
  }
  return L"InteractionClassHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

InteractionClassHandleImplementation const* InteractionClassHandle::getImplementation() const {
  return _impl;
}

InteractionClassHandleImplementation* InteractionClassHandle::getImplementation() {
  return _impl;
}

InteractionClassHandle::InteractionClassHandle(
    InteractionClassHandleImplementation* implementation) : _impl(implementation) {}

InteractionClassHandle::InteractionClassHandle(VariableLengthData const& encodedValue)
    : _impl(nullptr) {
  *this = InteractionClassHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, InteractionClassHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

InteractionClassHandle makeInteractionClassHandle(std::uint64_t value) {
  return InteractionClassHandleFriend::make(value);
}

InteractionClassHandle decodeInteractionClassHandle(VariableLengthData const& encodedValue) {
  return InteractionClassHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> interactionClassHandleValue(
    InteractionClassHandle const& handle) noexcept {
  return InteractionClassHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
