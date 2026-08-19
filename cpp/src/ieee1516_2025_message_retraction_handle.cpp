#include "internal/message_retraction_handle.hpp"
#include "internal/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kMessageRetractionHandleEncodedLength =
    umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength;

std::uint64_t handleValue(MessageRetractionHandleImplementation const* implementation);

auto encodeValue(std::uint64_t value) {
  return umbra_binding_detail::encodeUmbraHandleVariableArray(value);
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue);
  if (!value.has_value()) {
    throw CouldNotDecode(
        L"An Umbra MessageRetractionHandle must contain an HLAvariableArray with exactly eight HLAbyte elements.");
  }
  return *value;
}

}  // namespace

class MessageRetractionHandleImplementation final {
 public:
  explicit MessageRetractionHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(MessageRetractionHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class MessageRetractionHandleFriend final {
 public:
  static MessageRetractionHandle make(std::uint64_t value) {
    return MessageRetractionHandle(
        value == 0 ? nullptr : new MessageRetractionHandleImplementation(value));
  }

  static MessageRetractionHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(
      MessageRetractionHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

MessageRetractionHandle::MessageRetractionHandle() : _impl(nullptr) {}

MessageRetractionHandle::~MessageRetractionHandle() noexcept {
  delete _impl;
}

MessageRetractionHandle::MessageRetractionHandle(MessageRetractionHandle const& rhs)
    : _impl(rhs._impl == nullptr
                ? nullptr
                : new MessageRetractionHandleImplementation(handleValue(rhs._impl))) {}

MessageRetractionHandle& MessageRetractionHandle::operator=(
    MessageRetractionHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new MessageRetractionHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool MessageRetractionHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool MessageRetractionHandle::operator==(MessageRetractionHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool MessageRetractionHandle::operator!=(MessageRetractionHandle const& rhs) const {
  return !(*this == rhs);
}

bool MessageRetractionHandle::operator<(MessageRetractionHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long MessageRetractionHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(
      folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData MessageRetractionHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void MessageRetractionHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t MessageRetractionHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kMessageRetractionHandleEncodedLength) {
    throw CouldNotEncode(
        L"The MessageRetractionHandle output buffer must contain at least twelve bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t MessageRetractionHandle::encodedLength() const {
  return kMessageRetractionHandleEncodedLength;
}

std::wstring MessageRetractionHandle::toString() const {
  if (!isValid()) {
    return L"MessageRetractionHandle(invalid)";
  }
  return L"MessageRetractionHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

MessageRetractionHandleImplementation const* MessageRetractionHandle::getImplementation() const {
  return _impl;
}

MessageRetractionHandleImplementation* MessageRetractionHandle::getImplementation() {
  return _impl;
}

MessageRetractionHandle::MessageRetractionHandle(
    MessageRetractionHandleImplementation* implementation)
    : _impl(implementation) {}

MessageRetractionHandle::MessageRetractionHandle(VariableLengthData const& encodedValue)
    : _impl(nullptr) {
  *this = MessageRetractionHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, MessageRetractionHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

MessageRetractionHandle makeMessageRetractionHandle(std::uint64_t value) {
  return MessageRetractionHandleFriend::make(value);
}

MessageRetractionHandle decodeMessageRetractionHandle(
    VariableLengthData const& encodedValue) {
  return MessageRetractionHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> messageRetractionHandleValue(
    MessageRetractionHandle const& handle) noexcept {
  return MessageRetractionHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
