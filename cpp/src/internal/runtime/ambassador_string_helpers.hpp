#pragma once

#include <string>
#include <string_view>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
[[nodiscard]] inline std::wstring wideAscii(std::string_view value) {
  std::wstring result;
  result.reserve(value.size());
  for (unsigned char character : value) {
    result.push_back(static_cast<wchar_t>(character));
  }
  return result;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
