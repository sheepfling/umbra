#include "internal/handles/2010_handle_factories.hpp"
#include "internal/handles/handle_variable_array_encoding_2010.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <ostream>
#include <utility>

namespace rti1516e {
namespace {

template <typename Implementation>
std::uint64_t handleValue(Implementation const* implementation) {
  return implementation == nullptr ? 0 : implementation->value();
}

}  // namespace

#define UMBRA_2010_DEFINE_HANDLE(HandleKind) \
  class HandleKind##Implementation final { \
   public: \
    explicit HandleKind##Implementation(std::uint64_t value) : value_(value) {} \
    std::uint64_t value() const noexcept { return value_; } \
   private: \
    std::uint64_t value_ = 0; \
  }; \
  namespace { \
  std::uint64_t HandleKind##ValueImpl(HandleKind##Implementation const* implementation) { \
    return handleValue(implementation); \
  } \
  std::uint64_t decode##HandleKind##Value(VariableLengthData const& encodedValue) { \
    auto const value = umbra_binding_detail::decodeUmbraHandleVariableArray2010(encodedValue); \
    if (!value.first) { \
      throw CouldNotDecode(L"An Umbra IEEE 1516.1-2010 " #HandleKind \
                           L" must contain an HLAvariableArray with exactly eight HLAbyte elements."); \
    } \
    return value.second; \
  } \
  } \
  class HandleKind##Friend final { \
   public: \
    static HandleKind make(std::uint64_t value) { \
      return HandleKind(value == 0 ? nullptr : new HandleKind##Implementation(value)); \
    } \
    static HandleKind decode(VariableLengthData const& encodedValue) { \
      return make(decode##HandleKind##Value(encodedValue)); \
    } \
    static std::pair<bool, std::uint64_t> value(HandleKind const& handle) noexcept { \
      std::uint64_t const value = HandleKind##ValueImpl(handle.getImplementation()); \
      return std::make_pair(value != 0, value); \
    } \
  }; \
  HandleKind::HandleKind() : _impl(nullptr) {} \
  HandleKind::~HandleKind() throw() { delete _impl; } \
  HandleKind::HandleKind(HandleKind const& rhs) \
      : _impl(rhs._impl == nullptr ? nullptr : new HandleKind##Implementation(HandleKind##ValueImpl(rhs._impl))) {} \
  HandleKind& HandleKind::operator=(HandleKind const& rhs) { \
    if (this == &rhs) return *this; \
    auto* replacement = rhs._impl == nullptr \
        ? nullptr : new HandleKind##Implementation(HandleKind##ValueImpl(rhs._impl)); \
    delete _impl; _impl = replacement; return *this; \
  } \
  bool HandleKind::isValid() const { return HandleKind##ValueImpl(_impl) != 0; } \
  bool HandleKind::operator==(HandleKind const& rhs) const { \
    return HandleKind##ValueImpl(_impl) == HandleKind##ValueImpl(rhs._impl); \
  } \
  bool HandleKind::operator!=(HandleKind const& rhs) const { return !(*this == rhs); } \
  bool HandleKind::operator<(HandleKind const& rhs) const { \
    return HandleKind##ValueImpl(_impl) < HandleKind##ValueImpl(rhs._impl); \
  } \
  long HandleKind::hash() const { \
    std::uint64_t const value = HandleKind##ValueImpl(_impl); \
    return static_cast<long>((value ^ (value >> 32U)) & \
                             static_cast<std::uint64_t>((std::numeric_limits<long>::max)())); \
  } \
  VariableLengthData HandleKind::encode() const { \
    auto const encoded = umbra_binding_detail::encodeUmbraHandleVariableArray2010(HandleKind##ValueImpl(_impl)); \
    return VariableLengthData(encoded.data(), encoded.size()); \
  } \
  void HandleKind::encode(VariableLengthData& buffer) const { \
    auto const encoded = umbra_binding_detail::encodeUmbraHandleVariableArray2010(HandleKind##ValueImpl(_impl)); \
    buffer.setData(encoded.data(), encoded.size()); \
  } \
  size_t HandleKind::encode(void* buffer, size_t bufferSize) const throw(CouldNotEncode) { \
    if (buffer == nullptr || bufferSize < umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength2010) { \
      throw CouldNotEncode(L"The IEEE 1516.1-2010 handle output buffer is too small."); \
    } \
    auto const encoded = umbra_binding_detail::encodeUmbraHandleVariableArray2010(HandleKind##ValueImpl(_impl)); \
    std::memcpy(buffer, encoded.data(), encoded.size()); return encoded.size(); \
  } \
  size_t HandleKind::encodedLength() const { \
    return umbra_binding_detail::kUmbraHandleVariableArrayEncodedLength2010; \
  } \
  std::wstring HandleKind::toString() const { \
    return isValid() ? std::wstring(L"#HandleKind " L"(") + std::to_wstring(HandleKind##ValueImpl(_impl)) + L")" \
                     : std::wstring(L"#HandleKind " L"(invalid)"); \
  } \
  HandleKind##Implementation const* HandleKind::getImplementation() const { return _impl; } \
  HandleKind##Implementation* HandleKind::getImplementation() { return _impl; } \
  HandleKind::HandleKind(HandleKind##Implementation* implementation) : _impl(implementation) {} \
  HandleKind::HandleKind(VariableLengthData const& encodedValue) : _impl(nullptr) { \
    *this = HandleKind##Friend::decode(encodedValue); \
  } \
  std::wostream& operator<<(std::wostream& stream, HandleKind const& handle) { \
    stream << handle.toString(); return stream; \
  } \
  namespace umbra_binding_detail { \
  HandleKind make##HandleKind(std::uint64_t value) { return HandleKind##Friend::make(value); } \
  HandleKind decode##HandleKind(VariableLengthData const& encodedValue) { return HandleKind##Friend::decode(encodedValue); } \
  std::pair<bool, std::uint64_t> HandleKind##Value(HandleKind const& handle) noexcept { return HandleKind##Friend::value(handle); } \
  } 

UMBRA_2010_DEFINE_HANDLE(FederateHandle)
UMBRA_2010_DEFINE_HANDLE(ObjectClassHandle)
UMBRA_2010_DEFINE_HANDLE(InteractionClassHandle)
UMBRA_2010_DEFINE_HANDLE(ObjectInstanceHandle)
UMBRA_2010_DEFINE_HANDLE(AttributeHandle)
UMBRA_2010_DEFINE_HANDLE(ParameterHandle)
UMBRA_2010_DEFINE_HANDLE(DimensionHandle)
UMBRA_2010_DEFINE_HANDLE(MessageRetractionHandle)
UMBRA_2010_DEFINE_HANDLE(RegionHandle)

#undef UMBRA_2010_DEFINE_HANDLE

}  // namespace rti1516e
