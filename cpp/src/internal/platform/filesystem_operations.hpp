#pragma once

#include <filesystem>
#include <string_view>

namespace umbra::detail::platform {

// OS-specific filesystem syscalls belong in this implementation boundary;
// RTI and storage-owner code should depend only on these portable operations.
// Reserve an empty file atomically. Returns false only when the path already
// exists; other platform failures throw with the supplied operation context.
[[nodiscard]] bool reserveFileExclusively(
    std::filesystem::path const& path,
    std::string_view failureDescription);

// Remove only the file at this exact path; cleanup failures are ignored.
void removeFileNoThrow(std::filesystem::path const& path) noexcept;

// Append bytes to an existing file without creating or replacing it.
void appendToExistingFile(
    std::filesystem::path const& path,
    std::string_view contents,
    std::string_view failureDescription);

// Resolve the platform-specific path form needed when creating directories.
[[nodiscard]] std::filesystem::path directoryCreationPath(
    std::filesystem::path const& absolutePath);

}  // namespace umbra::detail::platform
