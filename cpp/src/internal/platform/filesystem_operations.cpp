#include "internal/platform/filesystem_operations.hpp"

#include <algorithm>
#include <cerrno>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace umbra::detail::platform {
namespace {

#if defined(_WIN32)
[[noreturn]] void throwFailure(
    std::string_view description,
    unsigned long systemError) {
  throw std::runtime_error(
      std::string{description} + " (Windows error " +
      std::to_string(systemError) + ").");
}

std::wstring windowsApiPath(std::filesystem::path const& path) {
  auto preferredPath = path;
  preferredPath.make_preferred();
  auto const native = preferredPath.native();
  if (native.size() < MAX_PATH || native.starts_with(L"\\\\?\\")) {
    return native;
  }
  if (native.starts_with(L"\\\\")) {
    return L"\\\\?\\UNC\\" + native.substr(2U);
  }
  if (native.size() >= 3U && native[1] == L':' && native[2] == L'\\') {
    return L"\\\\?\\" + native;
  }
  throw std::runtime_error(
      "A long Umbra filesystem path must be absolute on Windows.");
}
#endif

}  // namespace

std::filesystem::path directoryCreationPath(
    std::filesystem::path const& absolutePath) {
#if defined(_WIN32)
  return windowsApiPath(absolutePath);
#else
  return absolutePath;
#endif
}

void removeFileNoThrow(std::filesystem::path const& path) noexcept {
#if defined(_WIN32)
  try {
    auto const nativePath = windowsApiPath(path);
    static_cast<void>(::DeleteFileW(nativePath.c_str()));
  } catch (...) {
  }
#else
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
#endif
}

bool reserveFileExclusively(
    std::filesystem::path const& path,
    std::string_view failureDescription) {
#if defined(_WIN32)
  auto const nativePath = windowsApiPath(path);
  auto const file = ::CreateFileW(
      nativePath.c_str(),
      GENERIC_WRITE,
      0,
      nullptr,
      CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    auto const error = ::GetLastError();
    if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) {
      return false;
    }
    throwFailure(failureDescription, error);
  }
  if (::CloseHandle(file) == 0) {
    removeFileNoThrow(path);
    throw std::runtime_error(std::string{failureDescription});
  }
#else
  auto const descriptor = ::open(
      path.c_str(),
      O_WRONLY | O_CREAT | O_EXCL,
      S_IRUSR | S_IWUSR);
  if (descriptor < 0) {
    if (errno == EEXIST) {
      return false;
    }
    throw std::runtime_error(
        std::string{failureDescription} + " (" +
        std::error_code(errno, std::generic_category()).message() + ").");
  }
  if (::close(descriptor) != 0) {
    removeFileNoThrow(path);
    throw std::runtime_error(std::string{failureDescription});
  }
#endif
  return true;
}

void appendToExistingFile(
    std::filesystem::path const& path,
    std::string_view contents,
    std::string_view failureDescription) {
#if defined(_WIN32)
  auto const nativePath = windowsApiPath(path);
  auto const file = ::CreateFileW(
      nativePath.c_str(),
      FILE_APPEND_DATA,
      FILE_SHARE_READ,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    throw std::runtime_error(std::string{failureDescription});
  }
  try {
    std::size_t offset = 0U;
    while (offset < contents.size()) {
      auto const remaining = contents.size() - offset;
      auto const requested = static_cast<DWORD>(std::min<std::size_t>(
          remaining,
          static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
      DWORD written = 0U;
      if (::WriteFile(file, contents.data() + offset, requested, &written, nullptr) == 0 ||
          written != requested) {
        throw std::runtime_error(std::string{failureDescription});
      }
      offset += written;
    }
  } catch (...) {
    static_cast<void>(::CloseHandle(file));
    throw;
  }
  if (::CloseHandle(file) == 0) {
    throw std::runtime_error(std::string{failureDescription});
  }
#else
  auto const descriptor = ::open(path.c_str(), O_WRONLY | O_APPEND);
  if (descriptor < 0) {
    throw std::runtime_error(std::string{failureDescription});
  }
  try {
    std::size_t offset = 0U;
    while (offset < contents.size()) {
      auto const remaining = contents.size() - offset;
      auto const requested = std::min<std::size_t>(
          remaining,
          static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
      auto const written = ::write(descriptor, contents.data() + offset, requested);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        throw std::runtime_error(std::string{failureDescription});
      }
      offset += static_cast<std::size_t>(written);
    }
  } catch (...) {
    static_cast<void>(::close(descriptor));
    throw;
  }
  if (::close(descriptor) != 0) {
    throw std::runtime_error(std::string{failureDescription});
  }
#endif
}

}  // namespace umbra::detail::platform
