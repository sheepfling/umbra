#pragma once

#include <filesystem>
#include <optional>

namespace umbra::detail::platform {

// OS-backed accessors for the optional protected RTI initialization-data file.
[[nodiscard]] std::optional<std::filesystem::path>
configuredInitializationDataPath();
[[nodiscard]] bool isPrivateRegularFile(
    std::filesystem::path const& path);
[[nodiscard]] bool isPrivateDirectory(
    std::filesystem::path const& path);

}  // namespace umbra::detail::platform
