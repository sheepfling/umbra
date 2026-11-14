#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <aclapi.h>
#else
#include <cerrno>
#include <cstdlib>
#endif

namespace umbra::test {

#if defined(_WIN32)
[[nodiscard]] inline bool restrictRidTestDirectoryAcl(
    std::filesystem::path const& directory) {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return false;
  }
  DWORD tokenBytes = 0U;
  static_cast<void>(
      GetTokenInformation(token, TokenUser, nullptr, 0U, &tokenBytes));
  std::vector<std::byte> tokenBuffer(tokenBytes);
  bool const hasTokenUser = tokenBytes != 0U &&
      GetTokenInformation(
          token,
          TokenUser,
          tokenBuffer.data(),
          tokenBytes,
          &tokenBytes) != FALSE;
  CloseHandle(token);
  if (!hasTokenUser) {
    return false;
  }

  auto const* tokenUser = reinterpret_cast<TOKEN_USER const*>(tokenBuffer.data());
  std::array<std::byte, SECURITY_MAX_SID_SIZE> administratorsBuffer{};
  std::array<std::byte, SECURITY_MAX_SID_SIZE> systemBuffer{};
  DWORD administratorsBytes = static_cast<DWORD>(administratorsBuffer.size());
  DWORD systemBytes = static_cast<DWORD>(systemBuffer.size());
  if (!CreateWellKnownSid(
          WinBuiltinAdministratorsSid,
          nullptr,
          administratorsBuffer.data(),
          &administratorsBytes) ||
      !CreateWellKnownSid(
          WinLocalSystemSid,
          nullptr,
          systemBuffer.data(),
          &systemBytes)) {
    return false;
  }

  auto makeEntry = [](PSID sid, TRUSTEE_TYPE trusteeType) {
    EXPLICIT_ACCESSW entry{};
    entry.grfAccessPermissions = FILE_ALL_ACCESS;
    entry.grfAccessMode = SET_ACCESS;
    entry.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    BuildTrusteeWithSidW(&entry.Trustee, sid);
    entry.Trustee.TrusteeType = trusteeType;
    return entry;
  };
  std::array<EXPLICIT_ACCESSW, 3> entries{
      makeEntry(tokenUser->User.Sid, TRUSTEE_IS_USER),
      makeEntry(administratorsBuffer.data(), TRUSTEE_IS_GROUP),
      makeEntry(systemBuffer.data(), TRUSTEE_IS_GROUP)};
  PACL dacl = nullptr;
  if (SetEntriesInAclW(
          static_cast<ULONG>(entries.size()), entries.data(), nullptr, &dacl) !=
      ERROR_SUCCESS) {
    return false;
  }
  DWORD const result = SetNamedSecurityInfoW(
      const_cast<LPWSTR>(directory.c_str()),
      SE_FILE_OBJECT,
      DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
      nullptr,
      nullptr,
      dacl,
      nullptr);
  LocalFree(dacl);
  return result == ERROR_SUCCESS;
}
#endif

class TemporaryRidFiles final {
 public:
  TemporaryRidFiles() {
    auto const base = std::filesystem::temp_directory_path();
    auto const stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned int attempt = 0U; attempt < 32U; ++attempt) {
      auto candidate = base /
          ("umbra-rid-test-" + std::to_string(stamp) + "-" +
           std::to_string(attempt));
      std::error_code error;
      if (std::filesystem::create_directory(candidate, error)) {
#if defined(_WIN32)
        if (!restrictRidTestDirectoryAcl(candidate)) {
          std::filesystem::remove(candidate, error);
          throw std::runtime_error(
              "Could not protect the Windows RID test directory.");
        }
#endif
        directory_ = std::move(candidate);
#if !defined(_WIN32)
        std::filesystem::permissions(
            directory_,
            std::filesystem::perms::owner_all,
            std::filesystem::perm_options::replace);
#endif
        return;
      }
      if (error && error != std::errc::file_exists) {
        throw std::filesystem::filesystem_error(
            "Could not create a private RID test directory.", candidate, error);
      }
    }
    throw std::runtime_error("Could not allocate a unique RID test directory.");
  }

  ~TemporaryRidFiles() {
    std::error_code ignored;
    std::filesystem::remove_all(directory_, ignored);
  }

  [[nodiscard]] std::filesystem::path writePrivateFile(
      std::string const& name,
      std::string const& contents) const {
    auto const path = directory_ / name;
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
      throw std::runtime_error("Could not create a RID test file.");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
      throw std::runtime_error("Could not write a RID test file.");
    }
#if !defined(_WIN32)
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace);
#endif
    return path;
  }

 private:
  std::filesystem::path directory_;
};

class ScopedRidFileEnvironment final {
 public:
  explicit ScopedRidFileEnvironment(std::filesystem::path const& ridPath) {
#if defined(_WIN32)
    constexpr wchar_t name[] = L"UMBRA_RTI_RID_FILE";
    SetLastError(ERROR_SUCCESS);
    DWORD const required = GetEnvironmentVariableW(name, nullptr, 0U);
    if (required != 0U) {
      std::vector<wchar_t> previous(required);
      DWORD const length = GetEnvironmentVariableW(
          name, previous.data(), static_cast<DWORD>(previous.size()));
      if (length == 0U || length >= previous.size()) {
        throw std::runtime_error("Could not preserve the RID environment setting.");
      }
      previous_ = std::wstring(previous.data(), length);
    } else {
      DWORD const error = GetLastError();
      if (error == ERROR_SUCCESS) {
        previous_ = std::wstring{};
      } else if (error != ERROR_ENVVAR_NOT_FOUND) {
        throw std::runtime_error("Could not inspect the RID environment setting.");
      }
    }
    auto const value = ridPath.wstring();
    if (!SetEnvironmentVariableW(name, value.c_str())) {
      throw std::runtime_error("Could not set the RID environment setting.");
    }
#else
    constexpr char name[] = "UMBRA_RTI_RID_FILE";
    if (char const* previous = std::getenv(name); previous != nullptr) {
      previous_ = std::string(previous);
    }
    auto const value = ridPath.string();
    if (setenv(name, value.c_str(), 1) != 0) {
      throw std::system_error(errno, std::generic_category(), "setenv failed");
    }
#endif
  }

  ~ScopedRidFileEnvironment() {
#if defined(_WIN32)
    constexpr wchar_t name[] = L"UMBRA_RTI_RID_FILE";
    static_cast<void>(SetEnvironmentVariableW(
        name, previous_ ? previous_->c_str() : nullptr));
#else
    constexpr char name[] = "UMBRA_RTI_RID_FILE";
    if (previous_) {
      static_cast<void>(setenv(name, previous_->c_str(), 1));
    } else {
      static_cast<void>(unsetenv(name));
    }
#endif
  }

 private:
#if defined(_WIN32)
  std::optional<std::wstring> previous_;
#else
  std::optional<std::string> previous_;
#endif
};

}  // namespace umbra::test
