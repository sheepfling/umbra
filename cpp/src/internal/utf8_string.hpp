#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace umbra::detail {

// The FOM catalog retains XML's UTF-8 text while the official C++ binding uses
// std::wstring. These conversions reject malformed scalar values instead of
// applying a platform code page or lossy replacement.
[[nodiscard]] std::optional<std::string> utf8FromWide(std::wstring_view value);
[[nodiscard]] std::optional<std::wstring> wideFromUtf8(std::string_view value);

}  // namespace umbra::detail
