#pragma once

#include "internal/handles/handle_variable_array_encoding.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

#define UMBRA_2025_HANDLE_WIDE_STRING_IMPL(value) L##value
#define UMBRA_2025_HANDLE_WIDE_STRING(value) \
  UMBRA_2025_HANDLE_WIDE_STRING_IMPL(#value)

// The IEEE 1516.1-2025 handle classes all have the same value semantics and
// HLAvariableArray wire representation. Keep the public classes distinct for
// type safety, but define their private implementation in one place.
#define UMBRA_2025_DEFINE_HANDLE(HandleKind, functionSuffix) \
  class HandleKind##HandleImplementation final { \
   public: \
    explicit HandleKind##HandleImplementation(std::uint64_t value) : value_(value) {} \
    [[nodiscard]] std::uint64_t value() const noexcept { return value_; } \
   private: \
    std::uint64_t value_ = 0; \
  }; \
  namespace { \
  std::uint64_t handleValue(HandleKind##HandleImplementation const* implementation) { \
    return implementation == nullptr ? 0 : implementation->value(); \
  } \
  auto encodeValue(std::uint64_t value) { \
    return umbra_binding_detail::encodeUmbraHandleVariableArray(value); \
  } \
  std::uint64_t decodeValue(VariableLengthData const& encodedValue) { \
    auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray(encodedValue); \
    if (!value.has_value()) { \
      throw CouldNotDecode( \
          L"An Umbra " UMBRA_2025_HANDLE_WIDE_STRING(HandleKind) \
          L"Handle must contain an HLAvariableArray with exactly eight HLAbyte elements."); \
    } \
    return *value; \
  } \
  } /* namespace */ \
  class HandleKind##HandleFriend final { \
   public: \
    static HandleKind##Handle make(std::uint64_t value) { \
      return HandleKind##Handle( \
          value == 0 ? nullptr : new HandleKind##HandleImplementation(value)); \
    } \
    static HandleKind##Handle decode(VariableLengthData const& encodedValue) { \
      return make(decodeValue(encodedValue)); \
    } \
    static std::optional<std::uint64_t> value( \
        HandleKind##Handle const& handle) noexcept { \
      std::uint64_t const value = handleValue(handle.getImplementation()); \
      if (value == 0) { \
        return std::nullopt; \
      } \
      return value; \
    } \
  }; \
  HandleKind##Handle::HandleKind##Handle() : _impl(nullptr) {} \
  HandleKind##Handle::~HandleKind##Handle() noexcept { delete _impl; } \
  HandleKind##Handle::HandleKind##Handle(HandleKind##Handle const& rhs) \
      : _impl(rhs._impl == nullptr \
          ? nullptr \
          : new HandleKind##HandleImplementation(handleValue(rhs._impl))) {} \
  HandleKind##Handle& HandleKind##Handle::operator=( \
      HandleKind##Handle const& rhs) { \
    if (this == &rhs) { \
      return *this; \
    } \
    auto* replacement = rhs._impl == nullptr \
        ? nullptr \
        : new HandleKind##HandleImplementation(handleValue(rhs._impl)); \
    delete _impl; \
    _impl = replacement; \
    return *this; \
  } \
  bool HandleKind##Handle::isValid() const { \
    return handleValue(_impl) != 0; \
  } \
  bool HandleKind##Handle::operator==(HandleKind##Handle const& rhs) const { \
    return handleValue(_impl) == handleValue(rhs._impl); \
  } \
  bool HandleKind##Handle::operator!=(HandleKind##Handle const& rhs) const { \
    return !(*this == rhs); \
  } \
  bool HandleKind##Handle::operator<(HandleKind##Handle const& rhs) const { \
    return handleValue(_impl) < handleValue(rhs._impl); \
  } \
  long HandleKind##Handle::hash() const { \
    std::uint64_t const value = handleValue(_impl); \
    std::uint64_t const folded = value ^ (value >> 32); \
    return static_cast<long>( \
        folded & static_cast<std::uint64_t>((std::numeric_limits<long>::max)())); \
  } \
  VariableLengthData HandleKind##Handle::encode() const { \
    auto const encoded = encodeValue(handleValue(_impl)); \
    return VariableLengthData(encoded.data(), encoded.size()); \
  } \
  void HandleKind##Handle::encode(VariableLengthData& buffer) const { \
    auto const encoded = encodeValue(handleValue(_impl)); \
    buffer.setData(encoded.data(), encoded.size()); \
  } \
  size_t HandleKind##Handle::encode(void* buffer, size_t bufferSize) const { \
    if (buffer == nullptr || \
        bufferSize < umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength) { \
      throw CouldNotEncode( \
          L"The " UMBRA_2025_HANDLE_WIDE_STRING(HandleKind) \
          L"Handle output buffer must contain at least twelve bytes."); \
    } \
    auto const encoded = encodeValue(handleValue(_impl)); \
    std::memcpy(buffer, encoded.data(), encoded.size()); \
    return encoded.size(); \
  } \
  size_t HandleKind##Handle::encodedLength() const { \
    return umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength; \
  } \
  std::wstring HandleKind##Handle::toString() const { \
    if (!isValid()) { \
      return UMBRA_2025_HANDLE_WIDE_STRING(HandleKind) L"Handle(invalid)"; \
    } \
    return UMBRA_2025_HANDLE_WIDE_STRING(HandleKind) L"Handle(" + \
        std::to_wstring(handleValue(_impl)) + L")"; \
  } \
  HandleKind##Handle##Implementation const* HandleKind##Handle::getImplementation() const { \
    return _impl; \
  } \
  HandleKind##Handle##Implementation* HandleKind##Handle::getImplementation() { \
    return _impl; \
  } \
  HandleKind##Handle::HandleKind##Handle( \
      HandleKind##Handle##Implementation* implementation) : _impl(implementation) {} \
  HandleKind##Handle::HandleKind##Handle(VariableLengthData const& encodedValue) \
      : _impl(nullptr) { \
    *this = HandleKind##Handle##Friend::decode(encodedValue); \
  } \
  std::wostream& operator<<( \
      std::wostream& stream, HandleKind##Handle const& handle) { \
    stream << handle.toString(); \
    return stream; \
  } \
  namespace umbra_binding_detail { \
  HandleKind##Handle make##HandleKind##Handle(std::uint64_t value) { \
    return HandleKind##Handle##Friend::make(value); \
  } \
  HandleKind##Handle decode##HandleKind##Handle( \
      VariableLengthData const& encodedValue) { \
    return HandleKind##Handle##Friend::decode(encodedValue); \
  } \
  std::optional<std::uint64_t> functionSuffix##HandleValue( \
      HandleKind##Handle const& handle) noexcept { \
    return HandleKind##Handle##Friend::value(handle); \
  } \
  }
