#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kObjectInstanceHandleEncodedLength =
    umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength;

std::uint64_t handleValue(ObjectInstanceHandleImplementation const* implementation);

auto encodeValue(std::uint64_t value) {
  return umbra_binding_detail::encodeUmbraHandleVariableArray(value);
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue);
  if (!value.has_value()) {
    throw CouldNotDecode(
        L"An Umbra ObjectInstanceHandle must contain an HLAvariableArray with exactly eight HLAbyte elements.");
  }
  return *value;
}

}  // namespace

class ObjectInstanceHandleImplementation final {
 public:
  explicit ObjectInstanceHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(ObjectInstanceHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class ObjectInstanceHandleFriend final {
 public:
  static ObjectInstanceHandle make(std::uint64_t value) {
    return ObjectInstanceHandle(value == 0 ? nullptr : new ObjectInstanceHandleImplementation(value));
  }

  static ObjectInstanceHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(ObjectInstanceHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

ObjectInstanceHandle::ObjectInstanceHandle() : _impl(nullptr) {}

ObjectInstanceHandle::~ObjectInstanceHandle() noexcept {
  delete _impl;
}

ObjectInstanceHandle::ObjectInstanceHandle(ObjectInstanceHandle const& rhs)
    : _impl(rhs._impl == nullptr ? nullptr : new ObjectInstanceHandleImplementation(handleValue(rhs._impl))) {}

ObjectInstanceHandle& ObjectInstanceHandle::operator=(ObjectInstanceHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new ObjectInstanceHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool ObjectInstanceHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool ObjectInstanceHandle::operator==(ObjectInstanceHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool ObjectInstanceHandle::operator!=(ObjectInstanceHandle const& rhs) const {
  return !(*this == rhs);
}

bool ObjectInstanceHandle::operator<(ObjectInstanceHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long ObjectInstanceHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData ObjectInstanceHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void ObjectInstanceHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t ObjectInstanceHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kObjectInstanceHandleEncodedLength) {
    throw CouldNotEncode(L"The ObjectInstanceHandle output buffer must contain at least twelve bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t ObjectInstanceHandle::encodedLength() const {
  return kObjectInstanceHandleEncodedLength;
}

std::wstring ObjectInstanceHandle::toString() const {
  if (!isValid()) {
    return L"ObjectInstanceHandle(invalid)";
  }
  return L"ObjectInstanceHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

ObjectInstanceHandleImplementation const* ObjectInstanceHandle::getImplementation() const {
  return _impl;
}

ObjectInstanceHandleImplementation* ObjectInstanceHandle::getImplementation() {
  return _impl;
}

ObjectInstanceHandle::ObjectInstanceHandle(ObjectInstanceHandleImplementation* implementation) : _impl(implementation) {}

ObjectInstanceHandle::ObjectInstanceHandle(VariableLengthData const& encodedValue)
    : _impl(nullptr) {
  *this = ObjectInstanceHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, ObjectInstanceHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

ObjectInstanceHandle makeObjectInstanceHandle(std::uint64_t value) {
  return ObjectInstanceHandleFriend::make(value);
}

ObjectInstanceHandle decodeObjectInstanceHandle(VariableLengthData const& encodedValue) {
  return ObjectInstanceHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> objectInstanceHandleValue(ObjectInstanceHandle const& handle) noexcept {
  return ObjectInstanceHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
