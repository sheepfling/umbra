#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kObjectClassHandleEncodedLength =
    umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength;

std::uint64_t handleValue(ObjectClassHandleImplementation const* implementation);

auto encodeValue(std::uint64_t value) {
  return umbra_binding_detail::encodeUmbraHandleVariableArray(value);
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue);
  if (!value.has_value()) {
    throw CouldNotDecode(
        L"An Umbra ObjectClassHandle must contain an HLAvariableArray with exactly eight HLAbyte elements.");
  }
  return *value;
}

}  // namespace

class ObjectClassHandleImplementation final {
 public:
  explicit ObjectClassHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(ObjectClassHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class ObjectClassHandleFriend final {
 public:
  static ObjectClassHandle make(std::uint64_t value) {
    return ObjectClassHandle(value == 0 ? nullptr : new ObjectClassHandleImplementation(value));
  }

  static ObjectClassHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(ObjectClassHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

ObjectClassHandle::ObjectClassHandle() : _impl(nullptr) {}

ObjectClassHandle::~ObjectClassHandle() noexcept {
  delete _impl;
}

ObjectClassHandle::ObjectClassHandle(ObjectClassHandle const& rhs)
    : _impl(rhs._impl == nullptr ? nullptr : new ObjectClassHandleImplementation(handleValue(rhs._impl))) {}

ObjectClassHandle& ObjectClassHandle::operator=(ObjectClassHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new ObjectClassHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool ObjectClassHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool ObjectClassHandle::operator==(ObjectClassHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool ObjectClassHandle::operator!=(ObjectClassHandle const& rhs) const {
  return !(*this == rhs);
}

bool ObjectClassHandle::operator<(ObjectClassHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long ObjectClassHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData ObjectClassHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void ObjectClassHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t ObjectClassHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kObjectClassHandleEncodedLength) {
    throw CouldNotEncode(L"The ObjectClassHandle output buffer must contain at least twelve bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t ObjectClassHandle::encodedLength() const {
  return kObjectClassHandleEncodedLength;
}

std::wstring ObjectClassHandle::toString() const {
  if (!isValid()) {
    return L"ObjectClassHandle(invalid)";
  }
  return L"ObjectClassHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

ObjectClassHandleImplementation const* ObjectClassHandle::getImplementation() const {
  return _impl;
}

ObjectClassHandleImplementation* ObjectClassHandle::getImplementation() {
  return _impl;
}

ObjectClassHandle::ObjectClassHandle(ObjectClassHandleImplementation* implementation) : _impl(implementation) {}

ObjectClassHandle::ObjectClassHandle(VariableLengthData const& encodedValue)
    : _impl(nullptr) {
  *this = ObjectClassHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, ObjectClassHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

ObjectClassHandle makeObjectClassHandle(std::uint64_t value) {
  return ObjectClassHandleFriend::make(value);
}

ObjectClassHandle decodeObjectClassHandle(VariableLengthData const& encodedValue) {
  return ObjectClassHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> objectClassHandleValue(ObjectClassHandle const& handle) noexcept {
  return ObjectClassHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
