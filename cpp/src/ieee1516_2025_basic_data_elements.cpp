#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace {

template <typename T>
struct ReferencedValue {
  T owned{};
  T* pointer = &owned;

  ReferencedValue() = default;
  explicit ReferencedValue(T const& value) : owned(value) {}
  explicit ReferencedValue(T* value) : pointer(value == nullptr ? &owned : value) {}

  [[nodiscard]] T const& get() const { return *pointer; }
  void set(T value) { *pointer = std::move(value); }
  void setPointer(T* value) {
    if (value == nullptr) {
      throw rti1516_2025::EncoderException(L"A null encoding data pointer is invalid.");
    }
    pointer = value;
  }
};

[[noreturn]] void invalidEncoding(wchar_t const* message) {
  throw rti1516_2025::EncoderException(message);
}

void appendUint16BE(std::vector<rti1516_2025::Octet>& output, std::uint16_t value) {
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>(value & 0xffU));
}

void appendUint16LE(std::vector<rti1516_2025::Octet>& output, std::uint16_t value) {
  output.push_back(static_cast<rti1516_2025::Octet>(value & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 8U) & 0xffU));
}

[[nodiscard]] std::uint16_t readUint16BE(
    std::vector<rti1516_2025::Octet> const& input,
    std::size_t index) {
  if (index > input.size() || input.size() - index < 2U) {
    invalidEncoding(L"The two-octet encoding is truncated.");
  }
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(static_cast<std::uint8_t>(input[index])) << 8U) |
      static_cast<std::uint8_t>(input[index + 1U]));
}

[[nodiscard]] std::uint16_t readUint16LE(
    std::vector<rti1516_2025::Octet> const& input,
    std::size_t index) {
  if (index > input.size() || input.size() - index < 2U) {
    invalidEncoding(L"The two-octet encoding is truncated.");
  }
  return static_cast<std::uint16_t>(
      static_cast<std::uint8_t>(input[index]) |
      (static_cast<std::uint16_t>(static_cast<std::uint8_t>(input[index + 1U])) << 8U));
}

void appendUint32BE(std::vector<rti1516_2025::Octet>& output, std::uint32_t value) {
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 24U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 16U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>(value & 0xffU));
}

void appendUint32LE(std::vector<rti1516_2025::Octet>& output, std::uint32_t value) {
  output.push_back(static_cast<rti1516_2025::Octet>(value & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 16U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 24U) & 0xffU));
}

[[nodiscard]] std::uint32_t readUint32BE(
    std::vector<rti1516_2025::Octet> const& input,
    std::size_t index) {
  if (index > input.size() || input.size() - index < 4U) {
    invalidEncoding(L"The HLAinteger32BE encoding is truncated.");
  }
  return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index])) << 24U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 1U])) << 16U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 2U])) << 8U) |
      static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 3U]));
}

[[nodiscard]] std::uint32_t readUint32LE(
    std::vector<rti1516_2025::Octet> const& input,
    std::size_t index) {
  if (index > input.size() || input.size() - index < 4U) {
    invalidEncoding(L"The HLAinteger32LE encoding is truncated.");
  }
  return static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index])) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 1U])) << 8U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 2U])) << 16U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 3U])) << 24U);
}

void appendUint64BE(std::vector<rti1516_2025::Octet>& output, std::uint64_t value) {
  for (std::uint32_t shift = 56U;; shift -= 8U) {
    output.push_back(static_cast<rti1516_2025::Octet>((value >> shift) & 0xffU));
    if (shift == 0U) {
      break;
    }
  }
}

void appendUint64LE(std::vector<rti1516_2025::Octet>& output, std::uint64_t value) {
  for (std::uint32_t shift = 0U; shift < 64U; shift += 8U) {
    output.push_back(static_cast<rti1516_2025::Octet>((value >> shift) & 0xffU));
  }
}

[[nodiscard]] std::uint64_t readUint64BE(
    std::vector<rti1516_2025::Octet> const& input,
    std::size_t index) {
  if (index > input.size() || input.size() - index < 8U) {
    invalidEncoding(L"The HLAinteger64BE encoding is truncated.");
  }
  std::uint64_t value = 0U;
  for (std::size_t offset = 0U; offset < 8U; ++offset) {
    value = (value << 8U) | static_cast<std::uint8_t>(input[index + offset]);
  }
  return value;
}

[[nodiscard]] std::uint64_t readUint64LE(
    std::vector<rti1516_2025::Octet> const& input,
    std::size_t index) {
  if (index > input.size() || input.size() - index < 8U) {
    invalidEncoding(L"The HLAinteger64LE encoding is truncated.");
  }
  std::uint64_t value = 0U;
  for (std::size_t offset = 0U; offset < 8U; ++offset) {
    value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(input[index + offset])) <<
        (offset * 8U);
  }
  return value;
}

[[nodiscard]] std::vector<rti1516_2025::Octet> toOctets(
    rti1516_2025::VariableLengthData const& input) {
  auto const* bytes = static_cast<rti1516_2025::Octet const*>(input.data());
  if (input.size() != 0U && bytes == nullptr) {
    invalidEncoding(L"The encoded data buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<rti1516_2025::Octet>{}
                          : std::vector<rti1516_2025::Octet>(bytes, bytes + input.size());
}

template <typename To, typename From>
[[nodiscard]] To bitPreservingCast(From value) {
  static_assert(sizeof(To) == sizeof(From));
  To result{};
  std::memcpy(&result, &value, sizeof(result));
  return result;
}

static_assert(sizeof(float) == sizeof(std::uint32_t));
static_assert(std::numeric_limits<float>::is_iec559);
static_assert(sizeof(double) == sizeof(std::uint64_t));
static_assert(std::numeric_limits<double>::is_iec559);

[[nodiscard]] bool isAscii(char value) {
  return static_cast<unsigned char>(value) <= 0x7fU;
}

void validateAsciiString(std::string const& value) {
  if (!std::all_of(value.begin(), value.end(), isAscii)) {
    invalidEncoding(L"The HLAASCIIstring contains a non-ASCII character.");
  }
}

[[nodiscard]] std::vector<std::uint16_t> utf16Units(std::wstring const& value) {
  std::vector<std::uint16_t> result;
  result.reserve(value.size());
  for (std::size_t index = 0; index < value.size(); ++index) {
    std::uint32_t codePoint = static_cast<std::uint32_t>(value[index]);
    if constexpr (sizeof(wchar_t) == 2) {
      auto const unit = static_cast<std::uint16_t>(codePoint);
      if (unit >= 0xd800U && unit <= 0xdbffU) {
        if (index + 1U == value.size()) {
          invalidEncoding(L"The HLAunicodeString contains an unmatched high surrogate.");
        }
        auto const followingUnit = static_cast<std::uint16_t>(value[index + 1U]);
        if (followingUnit < 0xdc00U || followingUnit > 0xdfffU) {
          invalidEncoding(L"The HLAunicodeString contains an unmatched high surrogate.");
        }
        result.push_back(unit);
        result.push_back(followingUnit);
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
      continue;
    }
    codePoint -= 0x10000U;
    result.push_back(static_cast<std::uint16_t>(0xd800U + (codePoint >> 10U)));
    result.push_back(static_cast<std::uint16_t>(0xdc00U + (codePoint & 0x3ffU)));
  }
  return result;
}

[[nodiscard]] std::uint16_t utf16Unit(wchar_t value) {
  auto const codePoint = static_cast<std::uint32_t>(value);
  if (codePoint > 0xffffU) {
    invalidEncoding(L"The HLAunicodeChar cannot encode more than one UTF-16 code unit.");
  }
  return static_cast<std::uint16_t>(codePoint);
}

[[nodiscard]] std::wstring fromUtf16Units(std::vector<std::uint16_t> const& units) {
  std::wstring result;
  result.reserve(units.size());
  for (std::size_t index = 0; index < units.size(); ++index) {
    auto const unit = units[index];
    if (unit >= 0xd800U && unit <= 0xdbffU) {
      if (index + 1U == units.size() || units[index + 1U] < 0xdc00U || units[index + 1U] > 0xdfffU) {
        invalidEncoding(L"The HLAunicodeString encoding contains an unmatched high surrogate.");
      }
      auto const codePoint = 0x10000U + ((static_cast<std::uint32_t>(unit) - 0xd800U) << 10U) +
          (static_cast<std::uint32_t>(units[++index]) - 0xdc00U);
      if constexpr (sizeof(wchar_t) == 2) {
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

namespace rti1516_2025 {

class HLAinteger32BEImplementation {
 public:
  ReferencedValue<rti1516_2025::Integer32> value;
};

class HLAinteger32LEImplementation {
 public:
  ReferencedValue<rti1516_2025::Integer32> value;
};

class HLAinteger64BEImplementation {
 public:
  ReferencedValue<rti1516_2025::Integer64> value;
};

class HLAinteger64LEImplementation {
 public:
  ReferencedValue<rti1516_2025::Integer64> value;
};

class HLAinteger16BEImplementation {
 public:
  ReferencedValue<rti1516_2025::Integer16> value;
};

class HLAinteger16LEImplementation {
 public:
  ReferencedValue<rti1516_2025::Integer16> value;
};

class HLAunsignedInteger32BEImplementation {
 public:
  ReferencedValue<rti1516_2025::UnsignedInteger32> value;
};

class HLAunsignedInteger32LEImplementation {
 public:
  ReferencedValue<rti1516_2025::UnsignedInteger32> value;
};

class HLAunsignedInteger64BEImplementation {
 public:
  ReferencedValue<rti1516_2025::UnsignedInteger64> value;
};

class HLAunsignedInteger64LEImplementation {
 public:
  ReferencedValue<rti1516_2025::UnsignedInteger64> value;
};

class HLAunsignedInteger16BEImplementation {
 public:
  ReferencedValue<rti1516_2025::UnsignedInteger16> value;
};

class HLAunsignedInteger16LEImplementation {
 public:
  ReferencedValue<rti1516_2025::UnsignedInteger16> value;
};

class HLAbooleanImplementation {
 public:
  ReferencedValue<bool> value;
};

class HLAASCIIcharImplementation {
 public:
  ReferencedValue<char> value;
};

class HLAASCIIstringImplementation {
 public:
  ReferencedValue<std::string> value;
};

class HLAunicodeCharImplementation {
 public:
  ReferencedValue<wchar_t> value;
};

class HLAbyteImplementation {
 public:
  ReferencedValue<rti1516_2025::Octet> value;
};

class HLAoctetImplementation {
 public:
  ReferencedValue<rti1516_2025::Octet> value;
};

class HLAfloat32BEImplementation {
 public:
  ReferencedValue<float> value;
};

class HLAfloat32LEImplementation {
 public:
  ReferencedValue<float> value;
};

class HLAfloat64BEImplementation {
 public:
  ReferencedValue<double> value;
};

class HLAfloat64LEImplementation {
 public:
  ReferencedValue<double> value;
};

class HLAoctetPairBEImplementation {
 public:
  ReferencedValue<rti1516_2025::OctetPair> value;
};

class HLAoctetPairLEImplementation {
 public:
  ReferencedValue<rti1516_2025::OctetPair> value;
};

class HLAunicodeStringImplementation {
 public:
  ReferencedValue<std::wstring> value;
};

DataElement::~DataElement() = default;

bool DataElement::isSameTypeAs(DataElement const& other) const {
  return typeid(*this) == typeid(other);
}

Integer64 DataElement::hash() const {
  auto const encoded = encode();
  auto const* bytes = static_cast<Octet const*>(encoded.data());
  std::uint64_t hash = 14695981039346656037ULL;
  for (std::size_t index = 0; index < encoded.size(); ++index) {
    hash ^= static_cast<std::uint64_t>(bytes[index]);
    hash *= 1099511628211ULL;
  }
  return static_cast<Integer64>(hash);
}

EncoderException::EncoderException(std::wstring const& message) noexcept : _msg(message) {}
std::wstring EncoderException::what() const noexcept { return _msg; }
std::wstring EncoderException::name() const noexcept { return L"EncoderException"; }

#define UMBRA_DEFINE_FIXED_16_ENCODER(TYPE, IMPL, NATIVE, TO_BITS, FROM_BITS, APPEND, READ) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(NATIVE const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(NATIVE* value) : _impl(new IMPL()) { if (value != nullptr) { _impl->value.setPointer(value); } } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::unique_ptr<DataElement> TYPE::clone() const { return std::make_unique<TYPE>(*this); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { APPEND(bytes, TO_BITS(_impl->value.get())); } \
  TYPE& TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 2U) invalidEncoding(L"The fixed-width encoding has trailing data."); return *this; } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { _impl->value.set(FROM_BITS(READ(bytes, index))); return index + 2U; } \
  size_t TYPE::getEncodedLength() const { return 2U; } \
  unsigned int TYPE::getOctetBoundary() const { return 2U; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(NATIVE* value) { _impl->value.setPointer(value); } \
  TYPE& TYPE::set(NATIVE value) { _impl->value.set(value); return *this; } \
  NATIVE TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(NATIVE value) { return set(value); } \
  TYPE::operator NATIVE() const { return get(); }

#define UMBRA_DEFINE_OCTET_PAIR_ENCODER( \
    TYPE, IMPL, ENCODE_FIRST, ENCODE_SECOND, DECODE_FIRST_INDEX, DECODE_SECOND_INDEX) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(OctetPair const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(OctetPair* value) : _impl(new IMPL()) { if (value != nullptr) { _impl->value.setPointer(value); } } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::unique_ptr<DataElement> TYPE::clone() const { return std::make_unique<TYPE>(*this); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { auto const pair = _impl->value.get(); bytes.push_back(pair.ENCODE_FIRST); bytes.push_back(pair.ENCODE_SECOND); } \
  TYPE& TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 2U) invalidEncoding(L"The HLAoctetPair encoding has trailing data."); return *this; } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { if (index > bytes.size() || bytes.size() - index < 2U) invalidEncoding(L"The HLAoctetPair encoding is truncated."); _impl->value.set(OctetPair{bytes[index + DECODE_FIRST_INDEX], bytes[index + DECODE_SECOND_INDEX]}); return index + 2U; } \
  size_t TYPE::getEncodedLength() const { return 2U; } \
  unsigned int TYPE::getOctetBoundary() const { return 2U; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(OctetPair* value) { _impl->value.setPointer(value); } \
  TYPE& TYPE::set(OctetPair value) { _impl->value.set(std::move(value)); return *this; } \
  OctetPair TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(OctetPair value) { return set(std::move(value)); } \
  TYPE::operator OctetPair() const { return get(); }

UMBRA_DEFINE_FIXED_16_ENCODER(
    HLAinteger16BE,
    HLAinteger16BEImplementation,
    Integer16,
    [](Integer16 value) { return bitPreservingCast<std::uint16_t>(value); },
    [](std::uint16_t value) { return bitPreservingCast<Integer16>(value); },
    appendUint16BE,
    readUint16BE)

UMBRA_DEFINE_FIXED_16_ENCODER(
    HLAinteger16LE,
    HLAinteger16LEImplementation,
    Integer16,
    [](Integer16 value) { return bitPreservingCast<std::uint16_t>(value); },
    [](std::uint16_t value) { return bitPreservingCast<Integer16>(value); },
    appendUint16LE,
    readUint16LE)

UMBRA_DEFINE_FIXED_16_ENCODER(
    HLAunsignedInteger16BE,
    HLAunsignedInteger16BEImplementation,
    UnsignedInteger16,
    [](UnsignedInteger16 value) { return static_cast<std::uint16_t>(value); },
    [](std::uint16_t value) { return static_cast<UnsignedInteger16>(value); },
    appendUint16BE,
    readUint16BE)

UMBRA_DEFINE_FIXED_16_ENCODER(
    HLAunsignedInteger16LE,
    HLAunsignedInteger16LEImplementation,
    UnsignedInteger16,
    [](UnsignedInteger16 value) { return static_cast<std::uint16_t>(value); },
    [](std::uint16_t value) { return static_cast<UnsignedInteger16>(value); },
    appendUint16LE,
    readUint16LE)

UMBRA_DEFINE_OCTET_PAIR_ENCODER(
    HLAoctetPairBE,
    HLAoctetPairBEImplementation,
    first,
    second,
    0U,
    1U)

UMBRA_DEFINE_OCTET_PAIR_ENCODER(
    HLAoctetPairLE,
    HLAoctetPairLEImplementation,
    second,
    first,
    1U,
    0U)

#undef UMBRA_DEFINE_OCTET_PAIR_ENCODER
#undef UMBRA_DEFINE_FIXED_16_ENCODER

#define UMBRA_DEFINE_FIXED_8_ENCODER(TYPE, IMPL) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(Octet const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(Octet* value) : _impl(new IMPL()) { if (value != nullptr) { _impl->value.setPointer(value); } } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::unique_ptr<DataElement> TYPE::clone() const { return std::make_unique<TYPE>(*this); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { bytes.push_back(_impl->value.get()); } \
  TYPE& TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 1U) invalidEncoding(L"The one-octet encoding has trailing data."); return *this; } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { if (index >= bytes.size()) invalidEncoding(L"The one-octet encoding is truncated."); _impl->value.set(bytes[index]); return index + 1U; } \
  size_t TYPE::getEncodedLength() const { return 1U; } \
  unsigned int TYPE::getOctetBoundary() const { return 1U; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(Octet* value) { _impl->value.setPointer(value); } \
  TYPE& TYPE::set(Octet value) { _impl->value.set(value); return *this; } \
  Octet TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(Octet value) { return set(value); } \
  TYPE::operator Octet() const { return get(); }

UMBRA_DEFINE_FIXED_8_ENCODER(HLAbyte, HLAbyteImplementation)
UMBRA_DEFINE_FIXED_8_ENCODER(HLAoctet, HLAoctetImplementation)

#undef UMBRA_DEFINE_FIXED_8_ENCODER

#define UMBRA_DEFINE_FIXED_32_ENCODER(TYPE, IMPL, NATIVE, TO_BITS, FROM_BITS, APPEND, READ) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(NATIVE const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(NATIVE* value) : _impl(new IMPL()) { if (value != nullptr) { _impl->value.setPointer(value); } } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::unique_ptr<DataElement> TYPE::clone() const { return std::make_unique<TYPE>(*this); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { APPEND(bytes, TO_BITS(_impl->value.get())); } \
  TYPE& TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 4U) invalidEncoding(L"The fixed-width encoding has trailing data."); return *this; } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { _impl->value.set(FROM_BITS(READ(bytes, index))); return index + 4U; } \
  size_t TYPE::getEncodedLength() const { return 4U; } \
  unsigned int TYPE::getOctetBoundary() const { return 4U; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(NATIVE* value) { _impl->value.setPointer(value); } \
  TYPE& TYPE::set(NATIVE value) { _impl->value.set(value); return *this; } \
  NATIVE TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(NATIVE value) { return set(value); } \
  TYPE::operator NATIVE() const { return get(); }

UMBRA_DEFINE_FIXED_32_ENCODER(
    HLAfloat32BE,
    HLAfloat32BEImplementation,
    float,
    [](float value) { return bitPreservingCast<std::uint32_t>(value); },
    [](std::uint32_t value) { return bitPreservingCast<float>(value); },
    appendUint32BE,
    readUint32BE)

UMBRA_DEFINE_FIXED_32_ENCODER(
    HLAfloat32LE,
    HLAfloat32LEImplementation,
    float,
    [](float value) { return bitPreservingCast<std::uint32_t>(value); },
    [](std::uint32_t value) { return bitPreservingCast<float>(value); },
    appendUint32LE,
    readUint32LE)

UMBRA_DEFINE_FIXED_32_ENCODER(
    HLAinteger32BE,
    HLAinteger32BEImplementation,
    Integer32,
    [](Integer32 value) { return bitPreservingCast<std::uint32_t>(value); },
    [](std::uint32_t value) { return bitPreservingCast<Integer32>(value); },
    appendUint32BE,
    readUint32BE)

UMBRA_DEFINE_FIXED_32_ENCODER(
    HLAinteger32LE,
    HLAinteger32LEImplementation,
    Integer32,
    [](Integer32 value) { return bitPreservingCast<std::uint32_t>(value); },
    [](std::uint32_t value) { return bitPreservingCast<Integer32>(value); },
    appendUint32LE,
    readUint32LE)

UMBRA_DEFINE_FIXED_32_ENCODER(
    HLAunsignedInteger32BE,
    HLAunsignedInteger32BEImplementation,
    UnsignedInteger32,
    [](UnsignedInteger32 value) { return static_cast<std::uint32_t>(value); },
    [](std::uint32_t value) { return static_cast<UnsignedInteger32>(value); },
    appendUint32BE,
    readUint32BE)

UMBRA_DEFINE_FIXED_32_ENCODER(
    HLAunsignedInteger32LE,
    HLAunsignedInteger32LEImplementation,
    UnsignedInteger32,
    [](UnsignedInteger32 value) { return static_cast<std::uint32_t>(value); },
    [](std::uint32_t value) { return static_cast<UnsignedInteger32>(value); },
    appendUint32LE,
    readUint32LE)

#define UMBRA_DEFINE_FIXED_64_ENCODER(TYPE, IMPL, NATIVE, TO_BITS, FROM_BITS, APPEND, READ) \
  TYPE::TYPE() : _impl(new IMPL()) {} \
  TYPE::TYPE(NATIVE const& value) : _impl(new IMPL()) { _impl->value.set(value); } \
  TYPE::TYPE(NATIVE* value) : _impl(new IMPL()) { if (value != nullptr) { _impl->value.setPointer(value); } } \
  TYPE::TYPE(TYPE const& other) : _impl(new IMPL()) { _impl->value.set(other._impl->value.get()); } \
  TYPE::~TYPE() { delete _impl; } \
  TYPE& TYPE::operator=(TYPE const& other) { _impl->value.set(other._impl->value.get()); return *this; } \
  std::unique_ptr<DataElement> TYPE::clone() const { return std::make_unique<TYPE>(*this); } \
  VariableLengthData TYPE::encode() const { VariableLengthData value; encode(value); return value; } \
  void TYPE::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); } \
  void TYPE::encodeInto(std::vector<Octet>& bytes) const { APPEND(bytes, TO_BITS(_impl->value.get())); } \
  TYPE& TYPE::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 8U) invalidEncoding(L"The fixed-width encoding has trailing data."); return *this; } \
  size_t TYPE::decodeFrom(std::vector<Octet> const& bytes, size_t index) { _impl->value.set(FROM_BITS(READ(bytes, index))); return index + 8U; } \
  size_t TYPE::getEncodedLength() const { return 8U; } \
  unsigned int TYPE::getOctetBoundary() const { return 8U; } \
  Integer64 TYPE::hash() const { return DataElement::hash(); } \
  void TYPE::setDataPointer(NATIVE* value) { _impl->value.setPointer(value); } \
  TYPE& TYPE::set(NATIVE value) { _impl->value.set(value); return *this; } \
  NATIVE TYPE::get() const { return _impl->value.get(); } \
  TYPE& TYPE::operator=(NATIVE value) { return set(value); } \
  TYPE::operator NATIVE() const { return get(); }

UMBRA_DEFINE_FIXED_64_ENCODER(
    HLAfloat64BE,
    HLAfloat64BEImplementation,
    double,
    [](double value) { return bitPreservingCast<std::uint64_t>(value); },
    [](std::uint64_t value) { return bitPreservingCast<double>(value); },
    appendUint64BE,
    readUint64BE)

UMBRA_DEFINE_FIXED_64_ENCODER(
    HLAfloat64LE,
    HLAfloat64LEImplementation,
    double,
    [](double value) { return bitPreservingCast<std::uint64_t>(value); },
    [](std::uint64_t value) { return bitPreservingCast<double>(value); },
    appendUint64LE,
    readUint64LE)

UMBRA_DEFINE_FIXED_64_ENCODER(
    HLAinteger64BE,
    HLAinteger64BEImplementation,
    Integer64,
    [](Integer64 value) { return bitPreservingCast<std::uint64_t>(value); },
    [](std::uint64_t value) { return bitPreservingCast<Integer64>(value); },
    appendUint64BE,
    readUint64BE)

UMBRA_DEFINE_FIXED_64_ENCODER(
    HLAinteger64LE,
    HLAinteger64LEImplementation,
    Integer64,
    [](Integer64 value) { return bitPreservingCast<std::uint64_t>(value); },
    [](std::uint64_t value) { return bitPreservingCast<Integer64>(value); },
    appendUint64LE,
    readUint64LE)

UMBRA_DEFINE_FIXED_64_ENCODER(
    HLAunsignedInteger64BE,
    HLAunsignedInteger64BEImplementation,
    UnsignedInteger64,
    [](UnsignedInteger64 value) { return static_cast<std::uint64_t>(value); },
    [](std::uint64_t value) { return static_cast<UnsignedInteger64>(value); },
    appendUint64BE,
    readUint64BE)

UMBRA_DEFINE_FIXED_64_ENCODER(
    HLAunsignedInteger64LE,
    HLAunsignedInteger64LEImplementation,
    UnsignedInteger64,
    [](UnsignedInteger64 value) { return static_cast<std::uint64_t>(value); },
    [](std::uint64_t value) { return static_cast<UnsignedInteger64>(value); },
    appendUint64LE,
    readUint64LE)

#undef UMBRA_DEFINE_FIXED_64_ENCODER

HLAboolean::HLAboolean() : _impl(new HLAbooleanImplementation()) {}
HLAboolean::HLAboolean(bool const& value) : _impl(new HLAbooleanImplementation()) { _impl->value.set(value); }
HLAboolean::HLAboolean(bool* value) : _impl(new HLAbooleanImplementation()) { if (value != nullptr) { _impl->value.setPointer(value); } }
HLAboolean::HLAboolean(HLAboolean const& other) : _impl(new HLAbooleanImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAboolean::~HLAboolean() { delete _impl; }
HLAboolean& HLAboolean::operator=(HLAboolean const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::unique_ptr<DataElement> HLAboolean::clone() const { return std::make_unique<HLAboolean>(*this); }
VariableLengthData HLAboolean::encode() const { VariableLengthData value; encode(value); return value; }
void HLAboolean::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAboolean::encodeInto(std::vector<Octet>& bytes) const { appendUint32BE(bytes, _impl->value.get() ? 1U : 0U); }
HLAboolean& HLAboolean::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 4U) invalidEncoding(L"The HLAboolean encoding has trailing data."); return *this; }
size_t HLAboolean::decodeFrom(std::vector<Octet> const& bytes, size_t index) { auto const decoded = readUint32BE(bytes, index); if (decoded > 1U) invalidEncoding(L"The HLAboolean encoding is not HLAfalse or HLAtrue."); _impl->value.set(decoded == 1U); return index + 4U; }
size_t HLAboolean::getEncodedLength() const { return 4U; }
unsigned int HLAboolean::getOctetBoundary() const { return 4U; }
Integer64 HLAboolean::hash() const { return DataElement::hash(); }
void HLAboolean::setDataPointer(bool* value) { _impl->value.setPointer(value); }
HLAboolean& HLAboolean::set(bool value) { _impl->value.set(value); return *this; }
bool HLAboolean::get() const { return _impl->value.get(); }
HLAboolean& HLAboolean::operator=(bool value) { return set(value); }
HLAboolean::operator bool() const { return get(); }

HLAASCIIchar::HLAASCIIchar() : _impl(new HLAASCIIcharImplementation()) {}
HLAASCIIchar::HLAASCIIchar(char const& value) : _impl(new HLAASCIIcharImplementation()) { _impl->value.set(value); }
HLAASCIIchar::HLAASCIIchar(char* value) : _impl(new HLAASCIIcharImplementation()) { if (value != nullptr) { _impl->value.setPointer(value); } }
HLAASCIIchar::HLAASCIIchar(HLAASCIIchar const& other) : _impl(new HLAASCIIcharImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAASCIIchar::~HLAASCIIchar() { delete _impl; }
HLAASCIIchar& HLAASCIIchar::operator=(HLAASCIIchar const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::unique_ptr<DataElement> HLAASCIIchar::clone() const { return std::make_unique<HLAASCIIchar>(*this); }
VariableLengthData HLAASCIIchar::encode() const { VariableLengthData value; encode(value); return value; }
void HLAASCIIchar::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAASCIIchar::encodeInto(std::vector<Octet>& bytes) const { auto const character = _impl->value.get(); if (!isAscii(character)) invalidEncoding(L"The HLAASCIIchar is not a standard ASCII character."); bytes.push_back(static_cast<Octet>(character)); }
HLAASCIIchar& HLAASCIIchar::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 1U) invalidEncoding(L"The HLAASCIIchar encoding has trailing data."); return *this; }
size_t HLAASCIIchar::decodeFrom(std::vector<Octet> const& bytes, size_t index) { if (index >= bytes.size()) invalidEncoding(L"The HLAASCIIchar encoding is truncated."); auto const character = static_cast<char>(bytes[index]); if (!isAscii(character)) invalidEncoding(L"The HLAASCIIchar encoding is not a standard ASCII character."); _impl->value.set(character); return index + 1U; }
size_t HLAASCIIchar::getEncodedLength() const { return 1U; }
unsigned int HLAASCIIchar::getOctetBoundary() const { return 1U; }
Integer64 HLAASCIIchar::hash() const { return DataElement::hash(); }
void HLAASCIIchar::setDataPointer(char* value) { _impl->value.setPointer(value); }
HLAASCIIchar& HLAASCIIchar::set(char value) { _impl->value.set(value); return *this; }
char HLAASCIIchar::get() const { return _impl->value.get(); }
HLAASCIIchar& HLAASCIIchar::operator=(char value) { return set(value); }
HLAASCIIchar::operator char() const { return get(); }

HLAunicodeChar::HLAunicodeChar() : _impl(new HLAunicodeCharImplementation()) {}
HLAunicodeChar::HLAunicodeChar(wchar_t const& value) : _impl(new HLAunicodeCharImplementation()) { _impl->value.set(value); }
HLAunicodeChar::HLAunicodeChar(wchar_t* value) : _impl(new HLAunicodeCharImplementation()) { if (value != nullptr) { _impl->value.setPointer(value); } }
HLAunicodeChar::HLAunicodeChar(HLAunicodeChar const& other) : _impl(new HLAunicodeCharImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAunicodeChar::~HLAunicodeChar() { delete _impl; }
HLAunicodeChar& HLAunicodeChar::operator=(HLAunicodeChar const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::unique_ptr<DataElement> HLAunicodeChar::clone() const { return std::make_unique<HLAunicodeChar>(*this); }
VariableLengthData HLAunicodeChar::encode() const { VariableLengthData value; encode(value); return value; }
void HLAunicodeChar::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAunicodeChar::encodeInto(std::vector<Octet>& bytes) const { appendUint16BE(bytes, utf16Unit(_impl->value.get())); }
HLAunicodeChar& HLAunicodeChar::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != 2U) invalidEncoding(L"The HLAunicodeChar encoding has trailing data."); return *this; }
size_t HLAunicodeChar::decodeFrom(std::vector<Octet> const& bytes, size_t index) { _impl->value.set(static_cast<wchar_t>(readUint16BE(bytes, index))); return index + 2U; }
size_t HLAunicodeChar::getEncodedLength() const { return 2U; }
unsigned int HLAunicodeChar::getOctetBoundary() const { return 2U; }
Integer64 HLAunicodeChar::hash() const { return DataElement::hash(); }
void HLAunicodeChar::setDataPointer(wchar_t* value) { _impl->value.setPointer(value); }
HLAunicodeChar& HLAunicodeChar::set(wchar_t value) { _impl->value.set(value); return *this; }
wchar_t HLAunicodeChar::get() const { return _impl->value.get(); }
HLAunicodeChar& HLAunicodeChar::operator=(wchar_t value) { return set(value); }
HLAunicodeChar::operator wchar_t() const { return get(); }

HLAASCIIstring::HLAASCIIstring() : _impl(new HLAASCIIstringImplementation()) {}
HLAASCIIstring::HLAASCIIstring(std::string const& value) : _impl(new HLAASCIIstringImplementation()) { _impl->value.set(value); }
HLAASCIIstring::HLAASCIIstring(std::string* value) : _impl(new HLAASCIIstringImplementation()) { if (value != nullptr) { _impl->value.setPointer(value); } }
HLAASCIIstring::HLAASCIIstring(HLAASCIIstring const& other) : _impl(new HLAASCIIstringImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAASCIIstring::~HLAASCIIstring() { delete _impl; }
HLAASCIIstring& HLAASCIIstring::operator=(HLAASCIIstring const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::unique_ptr<DataElement> HLAASCIIstring::clone() const { return std::make_unique<HLAASCIIstring>(*this); }
VariableLengthData HLAASCIIstring::encode() const { VariableLengthData value; encode(value); return value; }
void HLAASCIIstring::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAASCIIstring::encodeInto(std::vector<Octet>& bytes) const { auto const& text = _impl->value.get(); validateAsciiString(text); if (text.size() > static_cast<std::size_t>(std::numeric_limits<Integer32>::max())) invalidEncoding(L"The HLAASCIIstring is too large to encode."); appendUint32BE(bytes, static_cast<std::uint32_t>(text.size())); for (auto const character : text) { bytes.push_back(static_cast<Octet>(character)); } }
HLAASCIIstring& HLAASCIIstring::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != getEncodedLength()) invalidEncoding(L"The HLAASCIIstring encoding has trailing data."); return *this; }
size_t HLAASCIIstring::decodeFrom(std::vector<Octet> const& bytes, size_t index) { auto const elementCount = readUint32BE(bytes, index); if (elementCount > static_cast<std::uint32_t>(std::numeric_limits<Integer32>::max()) || index > bytes.size() || bytes.size() - index < 4U) invalidEncoding(L"The HLAASCIIstring encoding has an invalid element count."); auto const payloadOffset = index + 4U; auto const payloadLength = static_cast<std::size_t>(elementCount); if (bytes.size() - payloadOffset < payloadLength) invalidEncoding(L"The HLAASCIIstring encoding is truncated."); std::string text; text.reserve(payloadLength); for (std::size_t offset = payloadOffset; offset < payloadOffset + payloadLength; ++offset) { auto const character = static_cast<char>(bytes[offset]); if (!isAscii(character)) invalidEncoding(L"The HLAASCIIstring encoding contains a non-ASCII character."); text.push_back(character); } _impl->value.set(std::move(text)); return payloadOffset + payloadLength; }
size_t HLAASCIIstring::getEncodedLength() const { auto const& text = _impl->value.get(); validateAsciiString(text); if (text.size() > static_cast<std::size_t>(std::numeric_limits<Integer32>::max())) invalidEncoding(L"The HLAASCIIstring is too large to encode."); return 4U + text.size(); }
unsigned int HLAASCIIstring::getOctetBoundary() const { return 4U; }
Integer64 HLAASCIIstring::hash() const { return DataElement::hash(); }
void HLAASCIIstring::setDataPointer(std::string* value) { _impl->value.setPointer(value); }
HLAASCIIstring& HLAASCIIstring::set(std::string value) { _impl->value.set(std::move(value)); return *this; }
std::string HLAASCIIstring::get() const { return _impl->value.get(); }
HLAASCIIstring& HLAASCIIstring::operator=(std::string value) { return set(std::move(value)); }
HLAASCIIstring::operator std::string() const { return get(); }

HLAunicodeString::HLAunicodeString() : _impl(new HLAunicodeStringImplementation()) {}
HLAunicodeString::HLAunicodeString(std::wstring const& value) : _impl(new HLAunicodeStringImplementation()) { _impl->value.set(value); }
HLAunicodeString::HLAunicodeString(std::wstring* value) : _impl(new HLAunicodeStringImplementation()) { if (value != nullptr) { _impl->value.setPointer(value); } }
HLAunicodeString::HLAunicodeString(HLAunicodeString const& other) : _impl(new HLAunicodeStringImplementation()) { _impl->value.set(other._impl->value.get()); }
HLAunicodeString::~HLAunicodeString() { delete _impl; }
HLAunicodeString& HLAunicodeString::operator=(HLAunicodeString const& other) { _impl->value.set(other._impl->value.get()); return *this; }
std::unique_ptr<DataElement> HLAunicodeString::clone() const { return std::make_unique<HLAunicodeString>(*this); }
VariableLengthData HLAunicodeString::encode() const { VariableLengthData value; encode(value); return value; }
void HLAunicodeString::encode(VariableLengthData& value) const { std::vector<Octet> bytes; encodeInto(bytes); value.setData(bytes.data(), bytes.size()); }
void HLAunicodeString::encodeInto(std::vector<Octet>& bytes) const { auto const units = utf16Units(_impl->value.get()); if (units.size() > static_cast<std::size_t>(std::numeric_limits<Integer32>::max())) invalidEncoding(L"The HLAunicodeString is too large to encode."); appendUint32BE(bytes, static_cast<std::uint32_t>(units.size())); for (auto const unit : units) { bytes.push_back(static_cast<Octet>(unit >> 8U)); bytes.push_back(static_cast<Octet>(unit & 0xffU)); } }
HLAunicodeString& HLAunicodeString::decode(VariableLengthData const& value) { auto const bytes = toOctets(value); decodeFrom(bytes, 0U); if (bytes.size() != getEncodedLength()) invalidEncoding(L"The HLAunicodeString encoding has trailing data."); return *this; }
size_t HLAunicodeString::decodeFrom(std::vector<Octet> const& bytes, size_t index) { auto const elementCount = readUint32BE(bytes, index); if (elementCount > static_cast<std::uint32_t>(std::numeric_limits<Integer32>::max()) || index > bytes.size() || bytes.size() - index < 4U) invalidEncoding(L"The HLAunicodeString encoding has an invalid element count."); auto const encodedLength = static_cast<std::size_t>(elementCount) * 2U; auto const payloadOffset = index + 4U; if (bytes.size() - payloadOffset < encodedLength) invalidEncoding(L"The HLAunicodeString encoding is truncated."); std::vector<std::uint16_t> units; units.reserve(elementCount); for (std::size_t offset = payloadOffset; offset < payloadOffset + encodedLength; offset += 2U) { units.push_back(static_cast<std::uint16_t>((static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes[offset])) << 8U) | static_cast<std::uint8_t>(bytes[offset + 1U]))); } _impl->value.set(fromUtf16Units(units)); return payloadOffset + encodedLength; }
size_t HLAunicodeString::getEncodedLength() const { return 4U + utf16Units(_impl->value.get()).size() * 2U; }
unsigned int HLAunicodeString::getOctetBoundary() const { return 4U; }
Integer64 HLAunicodeString::hash() const { return DataElement::hash(); }
void HLAunicodeString::setDataPointer(std::wstring* value) { _impl->value.setPointer(value); }
HLAunicodeString& HLAunicodeString::set(std::wstring value) { _impl->value.set(std::move(value)); return *this; }
std::wstring HLAunicodeString::get() const { return _impl->value.get(); }
HLAunicodeString& HLAunicodeString::operator=(std::wstring value) { return set(std::move(value)); }
HLAunicodeString::operator std::wstring() const { return get(); }

#undef UMBRA_DEFINE_FIXED_32_ENCODER

}  // namespace rti1516_2025
