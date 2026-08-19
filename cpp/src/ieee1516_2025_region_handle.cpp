#include "internal/region_handle.hpp"
#include "internal/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>

namespace rti1516_2025 {
namespace {

constexpr std::size_t kRegionHandleEncodedLength =
    umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength;

std::uint64_t handleValue(RegionHandleImplementation const* implementation);

auto encodeValue(std::uint64_t value) {
  return umbra_binding_detail::encodeUmbraHandleVariableArray(value);
}

std::uint64_t decodeValue(VariableLengthData const& encodedValue) {
  auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue);
  if (!value.has_value()) {
    throw CouldNotDecode(
        L"An Umbra RegionHandle must contain an HLAvariableArray with exactly eight HLAbyte elements.");
  }
  return *value;
}

}  // namespace

class RegionHandleImplementation final {
 public:
  explicit RegionHandleImplementation(std::uint64_t value) : value_(value) {}

  [[nodiscard]] std::uint64_t value() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_ = 0;
};

namespace {

std::uint64_t handleValue(RegionHandleImplementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

class RegionHandleFriend final {
 public:
  static RegionHandle make(std::uint64_t value) {
    return RegionHandle(value == 0 ? nullptr : new RegionHandleImplementation(value));
  }

  static RegionHandle decode(VariableLengthData const& encodedValue) {
    return make(decodeValue(encodedValue));
  }

  static std::optional<std::uint64_t> value(RegionHandle const& handle) noexcept {
    std::uint64_t const value = handleValue(handle.getImplementation());
    if (value == 0) {
      return std::nullopt;
    }
    return value;
  }
};

RegionHandle::RegionHandle() : _impl(nullptr) {}

RegionHandle::~RegionHandle() noexcept {
  delete _impl;
}

RegionHandle::RegionHandle(RegionHandle const& rhs)
    : _impl(rhs._impl == nullptr
                ? nullptr
                : new RegionHandleImplementation(handleValue(rhs._impl))) {}

RegionHandle& RegionHandle::operator=(RegionHandle const& rhs) {
  if (this == &rhs) {
    return *this;
  }

  auto* replacement = rhs._impl == nullptr
      ? nullptr
      : new RegionHandleImplementation(handleValue(rhs._impl));
  delete _impl;
  _impl = replacement;
  return *this;
}

bool RegionHandle::isValid() const {
  return handleValue(_impl) != 0;
}

bool RegionHandle::operator==(RegionHandle const& rhs) const {
  return handleValue(_impl) == handleValue(rhs._impl);
}

bool RegionHandle::operator!=(RegionHandle const& rhs) const {
  return !(*this == rhs);
}

bool RegionHandle::operator<(RegionHandle const& rhs) const {
  return handleValue(_impl) < handleValue(rhs._impl);
}

long RegionHandle::hash() const {
  std::uint64_t const value = handleValue(_impl);
  std::uint64_t const folded = value ^ (value >> 32);
  return static_cast<long>(folded & static_cast<std::uint64_t>(std::numeric_limits<long>::max()));
}

VariableLengthData RegionHandle::encode() const {
  auto const encoded = encodeValue(handleValue(_impl));
  return VariableLengthData(encoded.data(), encoded.size());
}

void RegionHandle::encode(VariableLengthData& buffer) const {
  auto const encoded = encodeValue(handleValue(_impl));
  buffer.setData(encoded.data(), encoded.size());
}

size_t RegionHandle::encode(void* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < kRegionHandleEncodedLength) {
    throw CouldNotEncode(L"The RegionHandle output buffer must contain at least twelve bytes.");
  }

  auto const encoded = encodeValue(handleValue(_impl));
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

size_t RegionHandle::encodedLength() const {
  return kRegionHandleEncodedLength;
}

std::wstring RegionHandle::toString() const {
  if (!isValid()) {
    return L"RegionHandle(invalid)";
  }
  return L"RegionHandle(" + std::to_wstring(handleValue(_impl)) + L")";
}

RegionHandleImplementation const* RegionHandle::getImplementation() const {
  return _impl;
}

RegionHandleImplementation* RegionHandle::getImplementation() {
  return _impl;
}

RegionHandle::RegionHandle(RegionHandleImplementation* implementation)
    : _impl(implementation) {}

RegionHandle::RegionHandle(VariableLengthData const& encodedValue) : _impl(nullptr) {
  *this = RegionHandleFriend::decode(encodedValue);
}

std::wostream& operator<<(std::wostream& stream, RegionHandle const& handle) {
  stream << handle.toString();
  return stream;
}

namespace umbra_binding_detail {

RegionHandle makeRegionHandle(std::uint64_t value) {
  return RegionHandleFriend::make(value);
}

RegionHandle decodeRegionHandle(VariableLengthData const& encodedValue) {
  return RegionHandleFriend::decode(encodedValue);
}

std::optional<std::uint64_t> regionHandleValue(RegionHandle const& handle) noexcept {
  return RegionHandleFriend::value(handle);
}

}  // namespace umbra_binding_detail

}  // namespace rti1516_2025
