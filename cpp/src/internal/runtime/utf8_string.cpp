#include "internal/runtime/utf8_string.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace umbra::detail {
namespace {

constexpr std::uint32_t kMaximumScalarValue = 0x10FFFF;
constexpr std::uint32_t kHighSurrogateFirst = 0xD800;
constexpr std::uint32_t kHighSurrogateLast = 0xDBFF;
constexpr std::uint32_t kLowSurrogateFirst = 0xDC00;
constexpr std::uint32_t kLowSurrogateLast = 0xDFFF;

bool isSurrogate(std::uint32_t value) {
  return value >= kHighSurrogateFirst && value <= kLowSurrogateLast;
}

bool isContinuationByte(unsigned char value) {
  return (value & 0xC0U) == 0x80U;
}

void appendUtf8(std::uint32_t scalar, std::string& output) {
  if (scalar <= 0x7F) {
    output.push_back(static_cast<char>(scalar));
  } else if (scalar <= 0x7FF) {
    output.push_back(static_cast<char>(0xC0U | (scalar >> 6)));
    output.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
  } else if (scalar <= 0xFFFF) {
    output.push_back(static_cast<char>(0xE0U | (scalar >> 12)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 6) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
  } else {
    output.push_back(static_cast<char>(0xF0U | (scalar >> 18)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 12) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 6) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
  }
}

void appendWide(std::uint32_t scalar, std::wstring& output) {
  if constexpr (sizeof(wchar_t) == 2) {
    if (scalar <= 0xFFFF) {
      output.push_back(static_cast<wchar_t>(scalar));
      return;
    }
    scalar -= 0x10000;
    output.push_back(static_cast<wchar_t>(kHighSurrogateFirst + (scalar >> 10)));
    output.push_back(static_cast<wchar_t>(kLowSurrogateFirst + (scalar & 0x3FF)));
  } else {
    output.push_back(static_cast<wchar_t>(scalar));
  }
}

std::optional<std::uint32_t> nextWideScalar(std::wstring_view value, std::size_t& index) {
  if (index >= value.size()) {
    return std::nullopt;
  }

  using UnsignedWchar = std::make_unsigned_t<wchar_t>;
  std::uint32_t const first = static_cast<std::uint32_t>(
      static_cast<UnsignedWchar>(value[index++]));
  if constexpr (sizeof(wchar_t) == 2) {
    if (first >= kHighSurrogateFirst && first <= kHighSurrogateLast) {
      if (index >= value.size()) {
        return std::nullopt;
      }
      std::uint32_t const second = static_cast<std::uint32_t>(
          static_cast<UnsignedWchar>(value[index++]));
      if (second < kLowSurrogateFirst || second > kLowSurrogateLast) {
        return std::nullopt;
      }
      return 0x10000 + ((first - kHighSurrogateFirst) << 10) + (second - kLowSurrogateFirst);
    }
    if (first >= kLowSurrogateFirst && first <= kLowSurrogateLast) {
      return std::nullopt;
    }
    return first;
  } else {
    if (first > kMaximumScalarValue || isSurrogate(first)) {
      return std::nullopt;
    }
    return first;
  }
}

std::optional<std::uint32_t> nextUtf8Scalar(std::string_view value, std::size_t& index) {
  if (index >= value.size()) {
    return std::nullopt;
  }

  unsigned char const first = static_cast<unsigned char>(value[index++]);
  if (first <= 0x7F) {
    return first;
  }

  std::size_t continuationCount = 0;
  std::uint32_t scalar = 0;
  std::uint32_t minimum = 0;
  if (first >= 0xC2 && first <= 0xDF) {
    continuationCount = 1;
    scalar = first & 0x1FU;
    minimum = 0x80;
  } else if (first >= 0xE0 && first <= 0xEF) {
    continuationCount = 2;
    scalar = first & 0x0FU;
    minimum = 0x800;
  } else if (first >= 0xF0 && first <= 0xF4) {
    continuationCount = 3;
    scalar = first & 0x07U;
    minimum = 0x10000;
  } else {
    return std::nullopt;
  }

  if (value.size() - index < continuationCount) {
    return std::nullopt;
  }
  for (std::size_t offset = 0; offset < continuationCount; ++offset) {
    unsigned char const continuation = static_cast<unsigned char>(value[index++]);
    if (!isContinuationByte(continuation)) {
      return std::nullopt;
    }
    scalar = (scalar << 6) | (continuation & 0x3FU);
  }
  if (scalar < minimum || scalar > kMaximumScalarValue || isSurrogate(scalar)) {
    return std::nullopt;
  }
  return scalar;
}

template <typename String, typename Char>
String quoteDiagnosticStringImpl(std::basic_string_view<Char> value) {
  String result;
  result.reserve(value.size() + 2U);
  result.push_back(static_cast<Char>('"'));
  constexpr char hexDigits[] = "0123456789abcdef";
  for (Char const character : value) {
    switch (character) {
      case static_cast<Char>('"'):
        result.push_back(static_cast<Char>('\\'));
        result.push_back(static_cast<Char>('"'));
        break;
      case static_cast<Char>('\\'):
        result.push_back(static_cast<Char>('\\'));
        result.push_back(static_cast<Char>('\\'));
        break;
      case static_cast<Char>('\n'):
        result.push_back(static_cast<Char>('\\'));
        result.push_back(static_cast<Char>('n'));
        break;
      case static_cast<Char>('\r'):
        result.push_back(static_cast<Char>('\\'));
        result.push_back(static_cast<Char>('r'));
        break;
      case static_cast<Char>('\t'):
        result.push_back(static_cast<Char>('\\'));
        result.push_back(static_cast<Char>('t'));
        break;
      default: {
        using UnsignedChar = std::make_unsigned_t<Char>;
        auto const unsignedCharacter = static_cast<std::uint32_t>(
            static_cast<UnsignedChar>(character));
        if (unsignedCharacter < 0x20U || unsignedCharacter == 0x7FU) {
          result.push_back(static_cast<Char>('\\'));
          result.push_back(static_cast<Char>('x'));
          result.push_back(static_cast<Char>(hexDigits[(unsignedCharacter >> 4U) & 0x0FU]));
          result.push_back(static_cast<Char>(hexDigits[unsignedCharacter & 0x0FU]));
        } else {
          result.push_back(character);
        }
        break;
      }
    }
  }
  result.push_back(static_cast<Char>('"'));
  return result;
}

}  // namespace

std::optional<std::string> utf8FromWide(std::wstring_view value) {
  std::string result;
  if (value.size() <= std::numeric_limits<std::size_t>::max() / 3) {
    result.reserve(value.size() * 3);
  }

  std::size_t index = 0;
  while (index < value.size()) {
    auto const scalar = nextWideScalar(value, index);
    if (!scalar) {
      return std::nullopt;
    }
    appendUtf8(*scalar, result);
  }
  return result;
}

std::optional<std::wstring> wideFromUtf8(std::string_view value) {
  std::wstring result;
  result.reserve(value.size());

  std::size_t index = 0;
  while (index < value.size()) {
    auto const scalar = nextUtf8Scalar(value, index);
    if (!scalar) {
      return std::nullopt;
    }
    appendWide(*scalar, result);
  }
  return result;
}

std::string quoteDiagnosticString(std::string_view value) {
  return quoteDiagnosticStringImpl<std::string>(value);
}

std::wstring quoteDiagnosticString(std::wstring_view value) {
  return quoteDiagnosticStringImpl<std::wstring>(value);
}

}  // namespace umbra::detail
