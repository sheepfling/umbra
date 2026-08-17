#include "internal/object_instance_handle.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kObjectInstanceHandleEncodedLength = 8;

std::uint64_t handleValue(ObjectInstanceHandleImplementation const* implementation);

std::array<unsigned char, kObjectInstanceHandleEncodedLength> encodeValue(std::uint64_t value) {
  std::array<unsigned char, kObjectInstanceHandleEncodedLength> encoded{};
  for (std::size_t index = 0; index < encoded.size(); ++index) {
    std::size_t const shift = (encoded.size() - index - 1) * 8;
    encoded[index] = static_cast<unsigned char>(value >> shift);
  }
  return encoded;
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  if (encodedValue.size() != kObjectInstanceHandleEncodedLength || encodedValue.data() == nullptr) {
    throw CouldNotDecode(L"An Umbra ObjectInstanceHandle must contain exactly eight bytes.");
  }

  auto const* bytes = static_cast<unsigned char const*>(encodedValue.data());
  std::uint64_t value = 0;
  for (unsigned char byte : std::array<unsigned char, kObjectInstanceHandleEncodedLength>{
           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]}) {
    value = (value << 8) | byte;
  }
  return value;
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
    throw CouldNotEncode(L"The ObjectInstanceHandle output buffer must contain at least eight bytes.");
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
