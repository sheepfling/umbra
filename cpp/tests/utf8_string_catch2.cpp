#include <catch2/catch_test_macros.hpp>

#include "internal/utf8_string.hpp"

#include <string>

TEST_CASE("The private FOM name boundary round-trips valid Unicode scalars", "[unit][kernel][utf8]") {
  std::wstring const wide = L"Caf\u00e9 \U0001f642";
  std::string const utf8 = "Caf\xC3\xA9 \xF0\x9F\x99\x82";

  REQUIRE(umbra::detail::utf8FromWide(wide) == utf8);
  REQUIRE(umbra::detail::wideFromUtf8(utf8) == wide);
}

TEST_CASE("The private FOM name boundary rejects malformed Unicode", "[unit][kernel][utf8]") {
  std::wstring malformedWide;
  if constexpr (sizeof(wchar_t) == 2) {
    malformedWide.push_back(static_cast<wchar_t>(0xD800));
  } else {
    malformedWide.push_back(static_cast<wchar_t>(0x110000));
  }

  REQUIRE_FALSE(umbra::detail::utf8FromWide(malformedWide).has_value());
  REQUIRE_FALSE(umbra::detail::wideFromUtf8("\xC0\x80").has_value());
  REQUIRE_FALSE(umbra::detail::wideFromUtf8("\xED\xA0\x80").has_value());
  REQUIRE_FALSE(umbra::detail::wideFromUtf8("\xF4\x90\x80\x80").has_value());
}
