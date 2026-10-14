#include "internal/platform/protected_initialization_data.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <aclapi.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace umbra::detail::platform {
namespace {

[[nodiscard]] bool hasPrivatePathSecurity(
    std::filesystem::path const& path,
    bool expectDirectory) {
#if defined(_WIN32)
  DWORD const attributes = GetFileAttributesW(path.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) != expectDirectory ||
      (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
    return false;
  }

  PSID owner = nullptr;
  PACL dacl = nullptr;
  PSECURITY_DESCRIPTOR descriptor = nullptr;
  DWORD const securityResult = GetNamedSecurityInfoW(
      const_cast<LPWSTR>(path.c_str()),
      SE_FILE_OBJECT,
      OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
      &owner,
      nullptr,
      &dacl,
      nullptr,
      &descriptor);
  if (securityResult != ERROR_SUCCESS || owner == nullptr || dacl == nullptr ||
      descriptor == nullptr) {
    if (descriptor != nullptr) {
      LocalFree(descriptor);
    }
    return false;
  }

  bool privateToCurrentAccount = false;
  HANDLE token = nullptr;
  DWORD tokenBytes = 0U;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    static_cast<void>(GetTokenInformation(token, TokenUser, nullptr, 0U, &tokenBytes));
    std::vector<std::byte> tokenBuffer(tokenBytes);
    if (tokenBytes != 0U &&
        GetTokenInformation(
            token,
            TokenUser,
            tokenBuffer.data(),
            tokenBytes,
            &tokenBytes)) {
      auto const* tokenUser = reinterpret_cast<TOKEN_USER const*>(tokenBuffer.data());
      std::array<std::byte, SECURITY_MAX_SID_SIZE> administratorsBuffer{};
      std::array<std::byte, SECURITY_MAX_SID_SIZE> systemBuffer{};
      DWORD administratorsBytes = static_cast<DWORD>(administratorsBuffer.size());
      DWORD systemBytes = static_cast<DWORD>(systemBuffer.size());
      bool const haveAdministrators = CreateWellKnownSid(
          WinBuiltinAdministratorsSid,
          nullptr,
          administratorsBuffer.data(),
          &administratorsBytes) != FALSE;
      bool const haveSystem = CreateWellKnownSid(
          WinLocalSystemSid,
          nullptr,
          systemBuffer.data(),
          &systemBytes) != FALSE;
      PSID administrators = administratorsBuffer.data();
      PSID system = systemBuffer.data();
      bool const ownerIsCurrentUser = EqualSid(owner, tokenUser->User.Sid) != FALSE;

      ACL_SIZE_INFORMATION aclInformation{};
      bool const haveAclInformation = GetAclInformation(
          dacl,
          &aclInformation,
          sizeof(aclInformation),
          AclSizeInformation) != FALSE;
      bool allowedPrincipalsAreTrusted = haveAclInformation;
      if (haveAclInformation) {
        for (DWORD index = 0U; index < aclInformation.AceCount; ++index) {
          void* rawAce = nullptr;
          if (!GetAce(dacl, index, &rawAce) || rawAce == nullptr) {
            allowedPrincipalsAreTrusted = false;
            break;
          }
          auto const* header = static_cast<ACE_HEADER const*>(rawAce);
          if (header->AceType != ACCESS_ALLOWED_ACE_TYPE) {
            continue;
          }
          auto const* allowedAce = static_cast<ACCESS_ALLOWED_ACE const*>(rawAce);
          PSID allowedSid = const_cast<DWORD*>(&allowedAce->SidStart);
          bool const trusted =
              EqualSid(allowedSid, tokenUser->User.Sid) != FALSE ||
              (ownerIsCurrentUser && EqualSid(allowedSid, owner) != FALSE) ||
              (haveAdministrators && EqualSid(allowedSid, administrators) != FALSE) ||
              (haveSystem && EqualSid(allowedSid, system) != FALSE);
          if (!trusted) {
            allowedPrincipalsAreTrusted = false;
            break;
          }
        }
      }
      privateToCurrentAccount = ownerIsCurrentUser && allowedPrincipalsAreTrusted;
    }
    CloseHandle(token);
  }
  LocalFree(descriptor);
  return privateToCurrentAccount;
#else
  struct stat information {};
  if (lstat(path.c_str(), &information) != 0 ||
      (expectDirectory ? !S_ISDIR(information.st_mode) : !S_ISREG(information.st_mode)) ||
      information.st_uid != geteuid()) {
    return false;
  }
  if (expectDirectory) {
    return (information.st_mode & (S_IWGRP | S_IWOTH)) == 0 &&
           (information.st_mode & S_IXUSR) != 0;
  }
  if ((information.st_mode & (S_IRWXG | S_IRWXO)) != 0 ||
      (information.st_mode & S_IRUSR) == 0) {
    return false;
  }
  return true;
#endif
}

}  // namespace

std::optional<std::filesystem::path> configuredInitializationDataPath() {
#if defined(_WIN32)
  constexpr wchar_t variableName[] = L"UMBRA_RTI_RID_FILE";
  SetLastError(ERROR_SUCCESS);
  DWORD const required = GetEnvironmentVariableW(variableName, nullptr, 0U);
  if (required == 0U) {
    auto const error = GetLastError();
    if (error == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    throw std::runtime_error(
        "Umbra could not read a valid, protected RTI initialization-data profile.");
  }
  std::vector<wchar_t> value(required);
  DWORD const length = GetEnvironmentVariableW(
      variableName,
      value.data(),
      static_cast<DWORD>(value.size()));
  if (length == 0U || length >= value.size()) {
    throw std::runtime_error(
        "Umbra could not read a valid, protected RTI initialization-data profile.");
  }
  return std::filesystem::path(std::wstring(value.data(), length));
#else
  char const* value = std::getenv("UMBRA_RTI_RID_FILE");
  if (value == nullptr) {
    return std::nullopt;
  }
  if (*value == '\0') {
    throw std::runtime_error(
        "Umbra could not read a valid, protected RTI initialization-data profile.");
  }
  return std::filesystem::u8path(value);
#endif
}

bool isPrivateRegularFile(std::filesystem::path const& path) {
  return hasPrivatePathSecurity(path, false);
}

bool isPrivateDirectory(std::filesystem::path const& path) {
  return hasPrivatePathSecurity(path, true);
}

}  // namespace umbra::detail::platform
