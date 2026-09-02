#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAopaqueData.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace rti1516e {

// The official 2010 header forward-declares this implementation type in the
// rti1516e namespace.  Keep the definition in that namespace (rather than an
// anonymous namespace) so VariableLengthData can legally own the pointer.
class VariableLengthDataImplementation {
 public:
  std::vector<Octet> owned;
  void const* pointer = nullptr;
  std::size_t pointer_size = 0;
  bool uses_pointer = false;
};

namespace {

template <typename T>
class ReferencedValue {
 public:
  ReferencedValue() = default;
  explicit ReferencedValue(T const& value) : owned_(value), pointer_(&owned_) {}
  explicit ReferencedValue(T* value) : pointer_(value == nullptr ? &owned_ : value) {}

  T const& get() const { return *pointer_; }
  void set(T value) { *pointer_ = std::move(value); }
  void setPointer(T* value) {
    if (value == nullptr) {
      throw EncoderException(L"A null encoding data pointer is invalid.");
    }
    pointer_ = value;
  }

 private:
  T owned_{};
  T* pointer_ = &owned_;
};

[[noreturn]] void invalidEncoding(wchar_t const* message) {
  throw EncoderException(message);
}

std::vector<Octet> toOctets(VariableLengthData const& input) {
  auto const* bytes = static_cast<Octet const*>(input.data());
  if (input.size() != 0U && bytes == nullptr) {
    invalidEncoding(L"The encoded data buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<Octet>{}
                          : std::vector<Octet>(bytes, bytes + input.size());
}

template <typename To, typename From>
To bitPreservingCast(From value) {
  static_assert(sizeof(To) == sizeof(From), "bit-preserving casts must have equal size");
  To result{};
  std::memcpy(&result, &value, sizeof(result));
  return result;
}

void appendUint16BE(std::vector<Octet>& output, std::uint16_t value) {
  output.push_back(static_cast<Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<Octet>(value & 0xffU));
}

void appendUint16LE(std::vector<Octet>& output, std::uint16_t value) {
  output.push_back(static_cast<Octet>(value & 0xffU));
  output.push_back(static_cast<Octet>((value >> 8U) & 0xffU));
}

std::uint16_t readUint16BE(std::vector<Octet> const& input, std::size_t index) {
  if (index > input.size() || input.size() - index < 2U) {
    invalidEncoding(L"The two-octet encoding is truncated.");
  }
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(static_cast<std::uint8_t>(input[index])) << 8U) |
      static_cast<std::uint8_t>(input[index + 1U]));
}

std::uint16_t readUint16LE(std::vector<Octet> const& input, std::size_t index) {
  if (index > input.size() || input.size() - index < 2U) {
    invalidEncoding(L"The two-octet encoding is truncated.");
  }
  return static_cast<std::uint16_t>(
      static_cast<std::uint8_t>(input[index]) |
      (static_cast<std::uint16_t>(static_cast<std::uint8_t>(input[index + 1U])) << 8U));
}

void appendUint32BE(std::vector<Octet>& output, std::uint32_t value) {
  output.push_back(static_cast<Octet>((value >> 24U) & 0xffU));
  output.push_back(static_cast<Octet>((value >> 16U) & 0xffU));
  output.push_back(static_cast<Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<Octet>(value & 0xffU));
}

void appendUint32LE(std::vector<Octet>& output, std::uint32_t value) {
  output.push_back(static_cast<Octet>(value & 0xffU));
  output.push_back(static_cast<Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<Octet>((value >> 16U) & 0xffU));
  output.push_back(static_cast<Octet>((value >> 24U) & 0xffU));
}

std::uint32_t readUint32BE(std::vector<Octet> const& input, std::size_t index) {
  if (index > input.size() || input.size() - index < 4U) {
    invalidEncoding(L"The four-octet encoding is truncated.");
  }
  return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index])) << 24U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 1U])) << 16U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 2U])) << 8U) |
      static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 3U]));
}

std::uint32_t readUint32LE(std::vector<Octet> const& input, std::size_t index) {
  if (index > input.size() || input.size() - index < 4U) {
    invalidEncoding(L"The four-octet encoding is truncated.");
  }
  return static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index])) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 1U])) << 8U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 2U])) << 16U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 3U])) << 24U);
}

void appendUint64BE(std::vector<Octet>& output, std::uint64_t value) {
  for (std::uint32_t shift = 56U;; shift -= 8U) {
    output.push_back(static_cast<Octet>((value >> shift) & 0xffU));
    if (shift == 0U) {
      break;
    }
  }
}

void appendUint64LE(std::vector<Octet>& output, std::uint64_t value) {
  for (std::uint32_t shift = 0U; shift < 64U; shift += 8U) {
    output.push_back(static_cast<Octet>((value >> shift) & 0xffU));
  }
}

std::uint64_t readUint64BE(std::vector<Octet> const& input, std::size_t index) {
  if (index > input.size() || input.size() - index < 8U) {
    invalidEncoding(L"The eight-octet encoding is truncated.");
  }
  std::uint64_t value = 0U;
  for (std::size_t offset = 0U; offset < 8U; ++offset) {
    value = (value << 8U) | static_cast<std::uint8_t>(input[index + offset]);
  }
  return value;
}

std::uint64_t readUint64LE(std::vector<Octet> const& input, std::size_t index) {
  if (index > input.size() || input.size() - index < 8U) {
    invalidEncoding(L"The eight-octet encoding is truncated.");
  }
  std::uint64_t value = 0U;
  for (std::size_t offset = 0U; offset < 8U; ++offset) {
    value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(input[index + offset])) <<
        (offset * 8U);
  }
  return value;
}

template <typename T>
void setEncoded(VariableLengthData& value, std::vector<T> const& bytes) {
  value.setData(bytes.data(), bytes.size());
}

bool isAscii(char value) { return static_cast<unsigned char>(value) <= 0x7fU; }

void validateAsciiString(std::string const& value) {
  if (!std::all_of(value.begin(), value.end(), isAscii)) {
    invalidEncoding(L"The HLAASCIIstring contains a non-ASCII character.");
  }
}

std::vector<std::uint16_t> utf16Units(std::wstring const& value) {
  std::vector<std::uint16_t> result;
  result.reserve(value.size());
  for (std::size_t index = 0U; index < value.size(); ++index) {
    auto const codePoint = static_cast<std::uint32_t>(value[index]);
    if (sizeof(wchar_t) == 2U) {
      auto const unit = static_cast<std::uint16_t>(codePoint);
      if (unit >= 0xd800U && unit <= 0xdbffU) {
        if (index + 1U == value.size()) {
          invalidEncoding(L"The HLAunicodeString contains an unmatched high surrogate.");
        }
        auto const following = static_cast<std::uint16_t>(value[index + 1U]);
        if (following < 0xdc00U || following > 0xdfffU) {
          invalidEncoding(L"The HLAunicodeString contains an unmatched high surrogate.");
        }
        result.push_back(unit);
        result.push_back(following);
        ++index;
        continue;
      }
      if (unit >= 0xdc00U && unit <= 0xdfffU) {
        invalidEncoding(L"The HLAunicodeString contains an unmatched low surrogate.");
      }
      result.push_back(unit);
      continue;
    }
    if (codePoint > 0x10ffffU || (codePoint >= 0xd800U && codePoint <= 0xdfffU)) {
      invalidEncoding(L"The HLAunicodeString contains an invalid Unicode scalar value.");
    }
    if (codePoint <= 0xffffU) {
      result.push_back(static_cast<std::uint16_t>(codePoint));
    } else {
      auto const adjusted = codePoint - 0x10000U;
      result.push_back(static_cast<std::uint16_t>(0xd800U + (adjusted >> 10U)));
      result.push_back(static_cast<std::uint16_t>(0xdc00U + (adjusted & 0x3ffU)));
    }
  }
  return result;
}

std::uint16_t utf16Unit(wchar_t value) {
  auto const codePoint = static_cast<std::uint32_t>(value);
  if (codePoint > 0xffffU) {
    invalidEncoding(L"The HLAunicodeChar cannot encode more than one UTF-16 code unit.");
  }
  return static_cast<std::uint16_t>(codePoint);
}

std::wstring fromUtf16Units(std::vector<std::uint16_t> const& units) {
  std::wstring result;
  result.reserve(units.size());
  for (std::size_t index = 0U; index < units.size(); ++index) {
    auto const unit = units[index];
    if (unit >= 0xd800U && unit <= 0xdbffU) {
      if (index + 1U == units.size() || units[index + 1U] < 0xdc00U ||
          units[index + 1U] > 0xdfffU) {
        invalidEncoding(L"The HLAunicodeString encoding contains an unmatched high surrogate.");
      }
      auto const codePoint = 0x10000U +
          ((static_cast<std::uint32_t>(unit) - 0xd800U) << 10U) +
          (static_cast<std::uint32_t>(units[++index]) - 0xdc00U);
      if (sizeof(wchar_t) == 2U) {
        result.push_back(static_cast<wchar_t>(unit));
        result.push_back(static_cast<wchar_t>(units[index]));
      } else {
        result.push_back(static_cast<wchar_t>(codePoint));
      }
      continue;
    }
    if (unit >= 0xdc00U && unit <= 0xdfffU) {
      invalidEncoding(L"The HLAunicodeString encoding contains an unmatched low surrogate.");
    }
    result.push_back(static_cast<wchar_t>(unit));
  }
  return result;
}

}  // namespace

VariableLengthData::VariableLengthData()
    : _impl(new VariableLengthDataImplementation()) {}

VariableLengthData::VariableLengthData(void const* data, size_t size)
    : _impl(new VariableLengthDataImplementation()) {
  setData(data, size);
}

VariableLengthData::VariableLengthData(VariableLengthData const& rhs)
    : _impl(new VariableLengthDataImplementation()) {
  setData(rhs.data(), rhs.size());
}

VariableLengthData::~VariableLengthData() { delete _impl; }

VariableLengthData& VariableLengthData::operator=(VariableLengthData const& rhs) {
  setData(rhs.data(), rhs.size());
  return *this;
}

void const* VariableLengthData::data() const {
  if (_impl->uses_pointer) {
    return _impl->pointer;
  }
  return _impl->owned.empty() ? nullptr : _impl->owned.data();
}

size_t VariableLengthData::size() const {
  return _impl->uses_pointer ? _impl->pointer_size : _impl->owned.size();
}

void VariableLengthData::setData(void const* data, size_t size) {
  _impl->uses_pointer = false;
  _impl->pointer = nullptr;
  _impl->pointer_size = 0U;
  auto const* bytes = static_cast<Octet const*>(data);
  if (bytes == nullptr || size == 0U) {
    _impl->owned.clear();
  } else {
    _impl->owned.assign(bytes, bytes + size);
  }
}

void VariableLengthData::setDataPointer(void* data, size_t size) {
  _impl->owned.clear();
  _impl->uses_pointer = true;
  _impl->pointer = data;
  _impl->pointer_size = size;
}

void VariableLengthData::takeDataPointer(
    void* data, size_t size, VariableLengthDataDeleteFunction function) {
  setData(data, size);
  if (function != nullptr) {
    function(data);
  } else {
    delete[] static_cast<Octet*>(data);
  }
}

DataElement::~DataElement() = default;

bool DataElement::isSameTypeAs(DataElement const& other) const {
  return typeid(*this) == typeid(other);
}

Integer64 DataElement::hash() const {
  auto const encoded = encode();
  auto const* bytes = static_cast<Octet const*>(encoded.data());
  std::uint64_t hash = 14695981039346656037ULL;
  for (std::size_t index = 0U; index < encoded.size(); ++index) {
    hash ^= static_cast<std::uint64_t>(static_cast<std::uint8_t>(bytes[index]));
    hash *= 1099511628211ULL;
  }
  return static_cast<Integer64>(hash);
}

EncoderException::EncoderException(std::wstring const& message) throw() : _msg(message) {}
std::wstring EncoderException::what() const throw() { return _msg; }

class HLAinteger16BEImplementation { public: ReferencedValue<Integer16> value; };
class HLAinteger16LEImplementation { public: ReferencedValue<Integer16> value; };
class HLAinteger32BEImplementation { public: ReferencedValue<Integer32> value; };
class HLAinteger32LEImplementation { public: ReferencedValue<Integer32> value; };
class HLAinteger64BEImplementation { public: ReferencedValue<Integer64> value; };
class HLAinteger64LEImplementation { public: ReferencedValue<Integer64> value; };
class HLAfloat32BEImplementation { public: ReferencedValue<float> value; };
class HLAfloat32LEImplementation { public: ReferencedValue<float> value; };
class HLAfloat64BEImplementation { public: ReferencedValue<double> value; };
class HLAfloat64LEImplementation { public: ReferencedValue<double> value; };
class HLAbyteImplementation { public: ReferencedValue<Octet> value; };
class HLAoctetImplementation { public: ReferencedValue<Octet> value; };
class HLAbooleanImplementation { public: ReferencedValue<bool> value; };
class HLAASCIIcharImplementation { public: ReferencedValue<char> value; };
class HLAASCIIstringImplementation { public: ReferencedValue<std::string> value; };
class HLAunicodeCharImplementation { public: ReferencedValue<wchar_t> value; };
class HLAunicodeStringImplementation { public: ReferencedValue<std::wstring> value; };
class HLAoctetPairBEImplementation { public: ReferencedValue<OctetPair> value; };
class HLAoctetPairLEImplementation { public: ReferencedValue<OctetPair> value; };

template <typename NATIVE, typename BITS>
NATIVE fromBits(BITS value) {
  return bitPreservingCast<NATIVE>(value);
}

std::uint16_t toBits16(Integer16 value) { return bitPreservingCast<std::uint16_t>(value); }
Integer16 fromBits16(std::uint16_t value) { return bitPreservingCast<Integer16>(value); }
std::uint32_t toBits32(Integer32 value) { return bitPreservingCast<std::uint32_t>(value); }
Integer32 fromBits32(std::uint32_t value) { return bitPreservingCast<Integer32>(value); }
std::uint64_t toBits64(Integer64 value) { return bitPreservingCast<std::uint64_t>(value); }
Integer64 fromBits64(std::uint64_t value) { return bitPreservingCast<Integer64>(value); }
std::uint32_t toFloat32Bits(float value) { return bitPreservingCast<std::uint32_t>(value); }
float fromFloat32Bits(std::uint32_t value) { return bitPreservingCast<float>(value); }
std::uint64_t toFloat64Bits(double value) { return bitPreservingCast<std::uint64_t>(value); }
double fromFloat64Bits(std::uint64_t value) { return bitPreservingCast<double>(value); }

#define UMBRA_2010_DEFINE_FIXED(TYPE, IMPL, NATIVE, BYTES, TO_BITS, FROM_BITS, APPEND, READ) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(NATIVE const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(NATIVE* value) : _impl(new IMPL()) { if (value != nullptr) _impl->value.setPointer(value); } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::auto_ptr<DataElement> TYPE::clone() const { return std::auto_ptr<DataElement>(new TYPE(*this)); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { APPEND(bytes, TO_BITS(_impl->value.get())); } \
  void TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != BYTES) invalidEncoding(L"The fixed-width encoding has trailing data."); } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { _impl->value.set(FROM_BITS(READ(bytes, index))); return index + BYTES; } \
  size_t TYPE::getEncodedLength() const { return BYTES; } \
  unsigned int TYPE::getOctetBoundary() const { return BYTES; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(NATIVE* value) { _impl->value.setPointer(value); } \
  void TYPE::set(NATIVE value) { _impl->value.set(value); } \
  NATIVE TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(NATIVE value) { set(value); return *this; } \
  TYPE::operator NATIVE() const { return get(); }

UMBRA_2010_DEFINE_FIXED(HLAinteger16BE, HLAinteger16BEImplementation, Integer16, 2U, toBits16, fromBits16, appendUint16BE, readUint16BE)
UMBRA_2010_DEFINE_FIXED(HLAinteger16LE, HLAinteger16LEImplementation, Integer16, 2U, toBits16, fromBits16, appendUint16LE, readUint16LE)
UMBRA_2010_DEFINE_FIXED(HLAinteger32BE, HLAinteger32BEImplementation, Integer32, 4U, toBits32, fromBits32, appendUint32BE, readUint32BE)
UMBRA_2010_DEFINE_FIXED(HLAinteger32LE, HLAinteger32LEImplementation, Integer32, 4U, toBits32, fromBits32, appendUint32LE, readUint32LE)
UMBRA_2010_DEFINE_FIXED(HLAinteger64BE, HLAinteger64BEImplementation, Integer64, 8U, toBits64, fromBits64, appendUint64BE, readUint64BE)
UMBRA_2010_DEFINE_FIXED(HLAinteger64LE, HLAinteger64LEImplementation, Integer64, 8U, toBits64, fromBits64, appendUint64LE, readUint64LE)
UMBRA_2010_DEFINE_FIXED(HLAfloat32BE, HLAfloat32BEImplementation, float, 4U, toFloat32Bits, fromFloat32Bits, appendUint32BE, readUint32BE)
UMBRA_2010_DEFINE_FIXED(HLAfloat32LE, HLAfloat32LEImplementation, float, 4U, toFloat32Bits, fromFloat32Bits, appendUint32LE, readUint32LE)
UMBRA_2010_DEFINE_FIXED(HLAfloat64BE, HLAfloat64BEImplementation, double, 8U, toFloat64Bits, fromFloat64Bits, appendUint64BE, readUint64BE)
UMBRA_2010_DEFINE_FIXED(HLAfloat64LE, HLAfloat64LEImplementation, double, 8U, toFloat64Bits, fromFloat64Bits, appendUint64LE, readUint64LE)

#undef UMBRA_2010_DEFINE_FIXED

#define UMBRA_2010_DEFINE_OCTET(TYPE, IMPL) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(Octet const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(Octet* value) : _impl(new IMPL()) { if (value != nullptr) _impl->value.setPointer(value); } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::auto_ptr<DataElement> TYPE::clone() const { return std::auto_ptr<DataElement>(new TYPE(*this)); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { bytes.push_back(_impl->value.get()); } \
  void TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 1U) invalidEncoding(L"The one-octet encoding has trailing data."); } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { if (index >= bytes.size()) invalidEncoding(L"The one-octet encoding is truncated."); _impl->value.set(bytes[index]); return index + 1U; } \
  size_t TYPE::getEncodedLength() const { return 1U; } \
  unsigned int TYPE::getOctetBoundary() const { return 1U; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(Octet* value) { _impl->value.setPointer(value); } \
  void TYPE::set(Octet value) { _impl->value.set(value); } \
  Octet TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(Octet value) { set(value); return *this; } \
  TYPE::operator Octet() const { return get(); }

UMBRA_2010_DEFINE_OCTET(HLAbyte, HLAbyteImplementation)
UMBRA_2010_DEFINE_OCTET(HLAoctet, HLAoctetImplementation)

#undef UMBRA_2010_DEFINE_OCTET

HLAboolean::HLAboolean() : _impl(new HLAbooleanImplementation()) {}
HLAboolean::HLAboolean(bool const& value) : _impl(new HLAbooleanImplementation()) { _impl->value.set(value); }
HLAboolean::HLAboolean(bool* value) : _impl(new HLAbooleanImplementation()) { if (value != nullptr) _impl->value.setPointer(value); }
HLAboolean::HLAboolean(HLAboolean const& other) : _impl(new HLAbooleanImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAboolean::~HLAboolean() { delete _impl; }
HLAboolean& HLAboolean::operator=(HLAboolean const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::auto_ptr<DataElement> HLAboolean::clone() const { return std::auto_ptr<DataElement>(new HLAboolean(*this)); }
VariableLengthData HLAboolean::encode() const { VariableLengthData value; encode(value); return value; }
void HLAboolean::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAboolean::encodeInto(std::vector<Octet>& bytes) const { appendUint32BE(bytes, _impl->value.get() ? 1U : 0U); }
void HLAboolean::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 4U) invalidEncoding(L"The HLAboolean encoding has trailing data."); }
size_t HLAboolean::decodeFrom(std::vector<Octet> const& bytes, size_t index) { auto const decoded = readUint32BE(bytes, index); if (decoded > 1U) invalidEncoding(L"The HLAboolean encoding is not HLAfalse or HLAtrue."); _impl->value.set(decoded == 1U); return index + 4U; }
size_t HLAboolean::getEncodedLength() const { return 4U; }
unsigned int HLAboolean::getOctetBoundary() const { return 4U; }
Integer64 HLAboolean::hash() const { return DataElement::hash(); }
void HLAboolean::setDataPointer(bool* value) { _impl->value.setPointer(value); }
void HLAboolean::set(bool value) { _impl->value.set(value); }
bool HLAboolean::get() const { return _impl->value.get(); }
HLAboolean& HLAboolean::operator=(bool value) { set(value); return *this; }
HLAboolean::operator bool() const { return get(); }

HLAASCIIchar::HLAASCIIchar() : _impl(new HLAASCIIcharImplementation()) {}
HLAASCIIchar::HLAASCIIchar(char const& value) : _impl(new HLAASCIIcharImplementation()) { _impl->value.set(value); }
HLAASCIIchar::HLAASCIIchar(char* value) : _impl(new HLAASCIIcharImplementation()) { if (value != nullptr) _impl->value.setPointer(value); }
HLAASCIIchar::HLAASCIIchar(HLAASCIIchar const& other) : _impl(new HLAASCIIcharImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAASCIIchar::~HLAASCIIchar() { delete _impl; }
HLAASCIIchar& HLAASCIIchar::operator=(HLAASCIIchar const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::auto_ptr<DataElement> HLAASCIIchar::clone() const { return std::auto_ptr<DataElement>(new HLAASCIIchar(*this)); }
VariableLengthData HLAASCIIchar::encode() const { VariableLengthData value; encode(value); return value; }
void HLAASCIIchar::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAASCIIchar::encodeInto(std::vector<Octet>& bytes) const { auto const character = _impl->value.get(); if (!isAscii(character)) invalidEncoding(L"The HLAASCIIchar is not a standard ASCII character."); bytes.push_back(static_cast<Octet>(character)); }
void HLAASCIIchar::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 1U) invalidEncoding(L"The HLAASCIIchar encoding has trailing data."); }
size_t HLAASCIIchar::decodeFrom(std::vector<Octet> const& bytes, size_t index) { if (index >= bytes.size()) invalidEncoding(L"The HLAASCIIchar encoding is truncated."); auto const character = static_cast<char>(bytes[index]); if (!isAscii(character)) invalidEncoding(L"The HLAASCIIchar encoding is not a standard ASCII character."); _impl->value.set(character); return index + 1U; }
size_t HLAASCIIchar::getEncodedLength() const { return 1U; }
unsigned int HLAASCIIchar::getOctetBoundary() const { return 1U; }
Integer64 HLAASCIIchar::hash() const { return DataElement::hash(); }
void HLAASCIIchar::setDataPointer(char* value) { _impl->value.setPointer(value); }
void HLAASCIIchar::set(char value) { _impl->value.set(value); }
char HLAASCIIchar::get() const { return _impl->value.get(); }
HLAASCIIchar& HLAASCIIchar::operator=(char value) { set(value); return *this; }
HLAASCIIchar::operator char() const { return get(); }

HLAunicodeChar::HLAunicodeChar() : _impl(new HLAunicodeCharImplementation()) {}
HLAunicodeChar::HLAunicodeChar(wchar_t const& value) : _impl(new HLAunicodeCharImplementation()) { _impl->value.set(value); }
HLAunicodeChar::HLAunicodeChar(wchar_t* value) : _impl(new HLAunicodeCharImplementation()) { if (value != nullptr) _impl->value.setPointer(value); }
HLAunicodeChar::HLAunicodeChar(HLAunicodeChar const& other) : _impl(new HLAunicodeCharImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAunicodeChar::~HLAunicodeChar() { delete _impl; }
HLAunicodeChar& HLAunicodeChar::operator=(HLAunicodeChar const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::auto_ptr<DataElement> HLAunicodeChar::clone() const { return std::auto_ptr<DataElement>(new HLAunicodeChar(*this)); }
VariableLengthData HLAunicodeChar::encode() const { VariableLengthData value; encode(value); return value; }
void HLAunicodeChar::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAunicodeChar::encodeInto(std::vector<Octet>& bytes) const { appendUint16BE(bytes, utf16Unit(_impl->value.get())); }
void HLAunicodeChar::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 2U) invalidEncoding(L"The HLAunicodeChar encoding has trailing data."); }
size_t HLAunicodeChar::decodeFrom(std::vector<Octet> const& bytes, size_t index) { _impl->value.set(static_cast<wchar_t>(readUint16BE(bytes, index))); return index + 2U; }
size_t HLAunicodeChar::getEncodedLength() const { return 2U; }
unsigned int HLAunicodeChar::getOctetBoundary() const { return 2U; }
Integer64 HLAunicodeChar::hash() const { return DataElement::hash(); }
void HLAunicodeChar::setDataPointer(wchar_t* value) { _impl->value.setPointer(value); }
void HLAunicodeChar::set(wchar_t value) { _impl->value.set(value); }
wchar_t HLAunicodeChar::get() const { return _impl->value.get(); }
HLAunicodeChar& HLAunicodeChar::operator=(wchar_t value) { set(value); return *this; }
HLAunicodeChar::operator wchar_t() const { return get(); }

#define UMBRA_2010_DEFINE_STRING(TYPE, IMPL, NATIVE, VALIDATE, COUNT, APPEND_VALUE, DECODE_VALUE, LENGTH) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(NATIVE const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(NATIVE* value) : _impl(new IMPL()) { if (value != nullptr) _impl->value.setPointer(value); } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::auto_ptr<DataElement> TYPE::clone() const { return std::auto_ptr<DataElement>(new TYPE(*this)); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { VALIDATE(_impl->value.get()); auto const units = COUNT(_impl->value.get()); appendUint32BE(bytes, static_cast<std::uint32_t>(units)); APPEND_VALUE(bytes, _impl->value.get()); } \
  void TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != getEncodedLength()) invalidEncoding(L"The string encoding has trailing data."); } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { auto const count = readUint32BE(bytes, index); auto const payload = index + 4U; if (bytes.size() - payload < LENGTH(count)) invalidEncoding(L"The string encoding is truncated."); _impl->value.set(DECODE_VALUE(bytes, payload, count)); return payload + LENGTH(count); } \
  size_t TYPE::getEncodedLength() const { return 4U + LENGTH(COUNT(_impl->value.get())); } \
  unsigned int TYPE::getOctetBoundary() const { return 4U; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(NATIVE* value) { _impl->value.setPointer(value); } \
  void TYPE::set(NATIVE value) { _impl->value.set(std::move(value)); } \
  NATIVE TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(NATIVE value) { set(std::move(value)); return *this; } \
  TYPE::operator NATIVE() const { return get(); }

std::size_t asciiCount(std::string const& value) { validateAsciiString(value); return value.size(); }
void appendAscii(std::vector<Octet>& bytes, std::string const& value) {
  bytes.insert(bytes.end(), value.begin(), value.end());
}
std::string decodeAscii(std::vector<Octet> const& bytes, std::size_t offset, std::uint32_t count) {
  std::string value(bytes.begin() + offset, bytes.begin() + offset + count);
  validateAsciiString(value);
  return value;
}
std::size_t unicodeCount(std::wstring const& value) { return utf16Units(value).size(); }
void appendUnicode(std::vector<Octet>& bytes, std::wstring const& value) {
  for (auto const unit : utf16Units(value)) appendUint16BE(bytes, unit);
}
std::wstring decodeUnicode(std::vector<Octet> const& bytes, std::size_t offset, std::uint32_t count) {
  std::vector<std::uint16_t> units;
  units.reserve(count);
  for (std::uint32_t index = 0U; index < count; ++index) {
    units.push_back(readUint16BE(bytes, offset + index * 2U));
  }
  return fromUtf16Units(units);
}
std::size_t unicodeLength(std::size_t count) { return count * 2U; }

UMBRA_2010_DEFINE_STRING(HLAASCIIstring, HLAASCIIstringImplementation, std::string, asciiCount, asciiCount, appendAscii, decodeAscii, [](std::size_t count) { return count; })
UMBRA_2010_DEFINE_STRING(HLAunicodeString, HLAunicodeStringImplementation, std::wstring, unicodeCount, unicodeCount, appendUnicode, decodeUnicode, unicodeLength)

#undef UMBRA_2010_DEFINE_STRING

HLAoctetPairBE::HLAoctetPairBE() : _impl(new HLAoctetPairBEImplementation()) {}
HLAoctetPairBE::HLAoctetPairBE(OctetPair const& value) : _impl(new HLAoctetPairBEImplementation()) { _impl->value.set(value); }
HLAoctetPairBE::HLAoctetPairBE(OctetPair* value) : _impl(new HLAoctetPairBEImplementation()) { if (value != nullptr) _impl->value.setPointer(value); }
HLAoctetPairBE::HLAoctetPairBE(HLAoctetPairBE const& other) : _impl(new HLAoctetPairBEImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAoctetPairBE::~HLAoctetPairBE() { delete _impl; }
HLAoctetPairBE& HLAoctetPairBE::operator=(HLAoctetPairBE const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::auto_ptr<DataElement> HLAoctetPairBE::clone() const { return std::auto_ptr<DataElement>(new HLAoctetPairBE(*this)); }
VariableLengthData HLAoctetPairBE::encode() const { VariableLengthData value; encode(value); return value; }
void HLAoctetPairBE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAoctetPairBE::encodeInto(std::vector<Octet>& bytes) const { auto const pair = _impl->value.get(); bytes.push_back(pair.first); bytes.push_back(pair.second); }
void HLAoctetPairBE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 2U) invalidEncoding(L"The HLAoctetPair encoding has trailing data."); }
size_t HLAoctetPairBE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { if (index > bytes.size() || bytes.size() - index < 2U) invalidEncoding(L"The HLAoctetPair encoding is truncated."); _impl->value.set(OctetPair{bytes[index], bytes[index + 1U]}); return index + 2U; }
size_t HLAoctetPairBE::getEncodedLength() const { return 2U; }
unsigned int HLAoctetPairBE::getOctetBoundary() const { return 2U; }
Integer64 HLAoctetPairBE::hash() const { return DataElement::hash(); }
void HLAoctetPairBE::setDataPointer(OctetPair* value) { _impl->value.setPointer(value); }
void HLAoctetPairBE::set(OctetPair value) { _impl->value.set(value); }
OctetPair HLAoctetPairBE::get() const { return _impl->value.get(); }
HLAoctetPairBE& HLAoctetPairBE::operator=(OctetPair value) { set(value); return *this; }
HLAoctetPairBE::operator OctetPair() const { return get(); }

HLAoctetPairLE::HLAoctetPairLE() : _impl(new HLAoctetPairLEImplementation()) {}
HLAoctetPairLE::HLAoctetPairLE(OctetPair const& value) : _impl(new HLAoctetPairLEImplementation()) { _impl->value.set(value); }
HLAoctetPairLE::HLAoctetPairLE(OctetPair* value) : _impl(new HLAoctetPairLEImplementation()) { if (value != nullptr) _impl->value.setPointer(value); }
HLAoctetPairLE::HLAoctetPairLE(HLAoctetPairLE const& other) : _impl(new HLAoctetPairLEImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAoctetPairLE::~HLAoctetPairLE() { delete _impl; }
HLAoctetPairLE& HLAoctetPairLE::operator=(HLAoctetPairLE const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::auto_ptr<DataElement> HLAoctetPairLE::clone() const { return std::auto_ptr<DataElement>(new HLAoctetPairLE(*this)); }
VariableLengthData HLAoctetPairLE::encode() const { VariableLengthData value; encode(value); return value; }
void HLAoctetPairLE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAoctetPairLE::encodeInto(std::vector<Octet>& bytes) const { auto const pair = _impl->value.get(); bytes.push_back(pair.second); bytes.push_back(pair.first); }
void HLAoctetPairLE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 2U) invalidEncoding(L"The HLAoctetPair encoding has trailing data."); }
size_t HLAoctetPairLE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { if (index > bytes.size() || bytes.size() - index < 2U) invalidEncoding(L"The HLAoctetPair encoding is truncated."); _impl->value.set(OctetPair{bytes[index + 1U], bytes[index]}); return index + 2U; }
size_t HLAoctetPairLE::getEncodedLength() const { return 2U; }
unsigned int HLAoctetPairLE::getOctetBoundary() const { return 2U; }
Integer64 HLAoctetPairLE::hash() const { return DataElement::hash(); }
void HLAoctetPairLE::setDataPointer(OctetPair* value) { _impl->value.setPointer(value); }
void HLAoctetPairLE::set(OctetPair value) { _impl->value.set(value); }
OctetPair HLAoctetPairLE::get() const { return _impl->value.get(); }
HLAoctetPairLE& HLAoctetPairLE::operator=(OctetPair value) { set(value); return *this; }
HLAoctetPairLE::operator OctetPair() const { return get(); }

// HLAopaqueData is the first 2010 composite element implemented on the
// direct C++ route.  Keep its storage and framing here with the scalar
// implementation so the pybind adapter uses the exact official class rather
// than a Python-only reimplementation.  The wire format is a four-octet
// HLAinteger32BE element count followed by the opaque octets.
class HLAopaqueDataImplementation {
 public:
  std::vector<Octet> owned;
  Octet** external = nullptr;
  std::size_t buffer_size = 0U;
  std::size_t data_size = 0U;

  void setInternal(Octet const* data, std::size_t size) {
    if (size != 0U && data == nullptr) {
      invalidEncoding(L"A nonempty HLAopaqueData value requires a data pointer.");
    }
    external = nullptr;
    buffer_size = 0U;
    data_size = 0U;
    owned.clear();
    if (size != 0U) {
      owned.assign(data, data + size);
    }
  }

  void setExternal(Octet** data, std::size_t buffer, std::size_t size) {
    if (data == nullptr || *data == nullptr || buffer == 0U) {
      invalidEncoding(L"HLAopaqueData external memory must be non-null with a nonzero buffer length.");
    }
    if (size > buffer) {
      invalidEncoding(L"The HLAopaqueData external data length exceeds its buffer length.");
    }
    owned.clear();
    external = data;
    buffer_size = buffer;
    data_size = size;
  }

  void setData(Octet const* data, std::size_t size) {
    if (size != 0U && data == nullptr) {
      invalidEncoding(L"A nonempty HLAopaqueData value requires a data pointer.");
    }
    if (external == nullptr) {
      setInternal(data, size);
      return;
    }
    if (*external == nullptr || size > buffer_size) {
      invalidEncoding(L"The HLAopaqueData value does not fit in its external buffer.");
    }
    if (size != 0U) {
      std::memmove(*external, data, size);
    }
    data_size = size;
  }

  Octet const* data() const {
    return external == nullptr ? (owned.empty() ? nullptr : owned.data()) : *external;
  }

  std::size_t bufferLength() const { return external == nullptr ? owned.size() : buffer_size; }
  std::size_t dataLength() const { return external == nullptr ? owned.size() : data_size; }
};

HLAopaqueData::HLAopaqueData() : _impl(new HLAopaqueDataImplementation()) {}

HLAopaqueData::HLAopaqueData(Octet const* data, std::size_t size)
    : _impl(new HLAopaqueDataImplementation()) {
  _impl->setInternal(data, size);
}

HLAopaqueData::HLAopaqueData(Octet** data, std::size_t buffer, std::size_t size)
    : _impl(new HLAopaqueDataImplementation()) {
  _impl->setExternal(data, buffer, size);
}

HLAopaqueData::HLAopaqueData(HLAopaqueData const& other)
    : _impl(new HLAopaqueDataImplementation()) {
  _impl->setInternal(other.get(), other.dataLength());
}

HLAopaqueData::~HLAopaqueData() { delete _impl; }

std::auto_ptr<DataElement> HLAopaqueData::clone() const {
  return std::auto_ptr<DataElement>(new HLAopaqueData(*this));
}

VariableLengthData HLAopaqueData::encode() const {
  VariableLengthData value;
  encode(value);
  return value;
}

void HLAopaqueData::encode(VariableLengthData& value) const {
  std::vector<Octet> bytes;
  encodeInto(bytes);
  value.setData(bytes.data(), bytes.size());
}

void HLAopaqueData::encodeInto(std::vector<Octet>& bytes) const {
  auto const size = _impl->dataLength();
  if (size > static_cast<std::size_t>(std::numeric_limits<Integer32>::max())) {
    invalidEncoding(L"The HLAopaqueData value is too large to encode.");
  }
  appendUint32BE(bytes, static_cast<std::uint32_t>(size));
  if (size == 0U) {
    return;
  }
  auto const* data = _impl->data();
  if (data == nullptr) {
    invalidEncoding(L"The HLAopaqueData data pointer is invalid.");
  }
  bytes.insert(bytes.end(), data, data + size);
}

void HLAopaqueData::decode(VariableLengthData const& value) {
  auto const bytes = toOctets(value);
  auto const next = decodeFrom(bytes, 0U);
  if (next != bytes.size()) {
    invalidEncoding(L"The HLAopaqueData encoding has trailing data.");
  }
}

std::size_t HLAopaqueData::decodeFrom(
    std::vector<Octet> const& bytes, std::size_t index) {
  auto const size = static_cast<std::size_t>(readUint32BE(bytes, index));
  auto const payload = index + 4U;
  if (payload > bytes.size() || bytes.size() - payload < size) {
    invalidEncoding(L"The HLAopaqueData encoding is truncated.");
  }
  _impl->setData(size == 0U ? nullptr : bytes.data() + payload, size);
  return payload + size;
}

std::size_t HLAopaqueData::getEncodedLength() const {
  auto const size = _impl->dataLength();
  if (size > static_cast<std::size_t>(std::numeric_limits<Integer32>::max())) {
    invalidEncoding(L"The HLAopaqueData value is too large to encode.");
  }
  return 4U + size;
}

unsigned int HLAopaqueData::getOctetBoundary() const { return 4U; }
std::size_t HLAopaqueData::bufferLength() const { return _impl->bufferLength(); }
std::size_t HLAopaqueData::dataLength() const { return _impl->dataLength(); }

void HLAopaqueData::setDataPointer(Octet** data, std::size_t buffer, std::size_t size) {
  _impl->setExternal(data, buffer, size);
}

void HLAopaqueData::set(Octet const* data, std::size_t size) {
  _impl->setData(data, size);
}

Octet const* HLAopaqueData::get() const { return _impl->data(); }
HLAopaqueData::operator const Octet*() const { return get(); }

}  // namespace rti1516e
