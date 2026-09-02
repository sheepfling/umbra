#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace umbra {
namespace utf8 {

inline void appendCodePoint(std::string& output, std::uint32_t codePoint) {
  if (codePoint <= 0x7FU) {
    output.push_back(static_cast<char>(codePoint));
  } else if (codePoint <= 0x7FFU) {
    output.push_back(static_cast<char>(0xC0U | (codePoint >> 6U)));
    output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
  } else if (codePoint <= 0xFFFFU) {
    output.push_back(static_cast<char>(0xE0U | (codePoint >> 12U)));
    output.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
  } else {
    output.push_back(static_cast<char>(0xF0U | (codePoint >> 18U)));
    output.push_back(static_cast<char>(0x80U | ((codePoint >> 12U) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
  }
}

inline bool isHighSurrogate(std::uint32_t value) {
  return value >= 0xD800U && value <= 0xDBFFU;
}

inline bool isLowSurrogate(std::uint32_t value) {
  return value >= 0xDC00U && value <= 0xDFFFU;
}

inline std::string encode(std::wstring const& value) {
  std::string result;
  result.reserve(value.size());
  for (std::size_t index = 0; index < value.size();) {
    std::uint32_t codePoint = static_cast<std::uint32_t>(value[index++]);
    if (sizeof(wchar_t) == 2U && isHighSurrogate(codePoint) &&
        index < value.size()) {
      std::uint32_t const low = static_cast<std::uint32_t>(value[index]);
      if (isLowSurrogate(low)) {
        ++index;
        codePoint = 0x10000U + ((codePoint - 0xD800U) << 10U) +
            (low - 0xDC00U);
      }
    }
    if (codePoint > 0x10FFFFU || isHighSurrogate(codePoint) ||
        isLowSurrogate(codePoint)) {
      codePoint = 0xFFFDU;
    }
    appendCodePoint(result, codePoint);
  }
  return result;
}

inline void appendWideCodePoint(std::wstring& output, std::uint32_t codePoint) {
  if (codePoint > 0x10FFFFU || isHighSurrogate(codePoint) ||
      isLowSurrogate(codePoint)) {
    codePoint = 0xFFFDU;
  }
  if (sizeof(wchar_t) == 2U && codePoint > 0xFFFFU) {
    codePoint -= 0x10000U;
    output.push_back(static_cast<wchar_t>(0xD800U | (codePoint >> 10U)));
    output.push_back(static_cast<wchar_t>(0xDC00U | (codePoint & 0x3FFU)));
  } else {
    output.push_back(static_cast<wchar_t>(codePoint));
  }
}

inline std::wstring decode(std::string const& value) {
  std::wstring result;
  result.reserve(value.size());
  for (std::size_t index = 0; index < value.size();) {
    unsigned char const first = static_cast<unsigned char>(value[index]);
    std::uint32_t codePoint = 0xFFFDU;
    std::size_t length = 1U;
    std::uint32_t minimum = 0U;
    if (first <= 0x7FU) {
      codePoint = first;
    } else if (first >= 0xC2U && first <= 0xDFU) {
      codePoint = first & 0x1FU;
      length = 2U;
      minimum = 0x80U;
    } else if (first >= 0xE0U && first <= 0xEFU) {
      codePoint = first & 0x0FU;
      length = 3U;
      minimum = 0x800U;
    } else if (first >= 0xF0U && first <= 0xF4U) {
      codePoint = first & 0x07U;
      length = 4U;
      minimum = 0x10000U;
    }
    if (length > 1U) {
      if (index + length > value.size()) {
        length = 1U;
      } else {
        for (std::size_t offset = 1U; offset < length; ++offset) {
          unsigned char const continuation =
              static_cast<unsigned char>(value[index + offset]);
          if ((continuation & 0xC0U) != 0x80U) {
            length = 1U;
            break;
          }
          codePoint = (codePoint << 6U) | (continuation & 0x3FU);
        }
        if (length > 1U &&
            (codePoint < minimum || codePoint > 0x10FFFFU ||
             isHighSurrogate(codePoint) || isLowSurrogate(codePoint))) {
          length = 1U;
        }
      }
    }
    appendWideCodePoint(result, length == 1U && codePoint != first
        ? 0xFFFDU : codePoint);
    index += length;
  }
  return result;
}

}  // namespace utf8
}  // namespace umbra
