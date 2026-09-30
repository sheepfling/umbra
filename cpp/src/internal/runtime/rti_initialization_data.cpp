#include "internal/runtime/rti_initialization_data.hpp"

#include "internal/runtime/reference_authorizer.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/auth/AuthorizerFactory.h>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#include <aclapi.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace umbra::detail {
namespace {

constexpr std::wstring_view kRidEnvironmentVariable = L"UMBRA_RTI_RID_FILE";
constexpr std::string_view kAuthorizationSectionPrefix = "authorization.";
constexpr std::size_t kMaximumRidBytes = 64U * 1024U;
constexpr std::size_t kMaximumPasswordBytes = 16U * 1024U;

struct AuthorizationProfile final {
  std::optional<std::string> service;
  std::optional<std::string> globalPasswordFile;
};

[[noreturn]] void invalidRid() {
  throw std::runtime_error(
      "Umbra could not read a valid, protected RTI initialization-data profile.");
}

[[nodiscard]] std::string_view trimAscii(std::string_view value) noexcept {
  auto isTrim = [](char character) {
    return character == ' ' || character == '\t' || character == '\r';
  };
  while (!value.empty() && isTrim(value.front())) {
    value.remove_prefix(1U);
  }
  while (!value.empty() && isTrim(value.back())) {
    value.remove_suffix(1U);
  }
  return value;
}

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

[[nodiscard]] bool hasPrivateFileSecurity(std::filesystem::path const& path) {
  return hasPrivatePathSecurity(path, false);
}

[[nodiscard]] bool hasPrivateDirectorySecurity(std::filesystem::path const& path) {
  return hasPrivatePathSecurity(path, true);
}

[[nodiscard]] std::optional<std::filesystem::path> configuredRidPath() {
#if defined(_WIN32)
  auto const variableName = std::wstring(kRidEnvironmentVariable);
  SetLastError(ERROR_SUCCESS);
  DWORD const required = GetEnvironmentVariableW(variableName.c_str(), nullptr, 0U);
  if (required == 0U) {
    DWORD const error = GetLastError();
    if (error == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    if (error == ERROR_SUCCESS) {
      invalidRid();
    }
    invalidRid();
  }
  std::vector<wchar_t> value(required);
  DWORD const length = GetEnvironmentVariableW(
      variableName.c_str(), value.data(), static_cast<DWORD>(value.size()));
  if (length == 0U || length >= value.size()) {
    invalidRid();
  }
  return std::filesystem::path(std::wstring(value.data(), length));
#else
  char const* value = std::getenv("UMBRA_RTI_RID_FILE");
  if (value == nullptr) {
    return std::nullopt;
  }
  if (*value == '\0') {
    invalidRid();
  }
  return std::filesystem::u8path(value);
#endif
}

[[nodiscard]] std::string readProtectedFile(
    std::filesystem::path const& path,
    std::size_t maximumBytes) {
  std::error_code error;
  auto const linkStatus = std::filesystem::symlink_status(path, error);
  if (error || !std::filesystem::is_regular_file(linkStatus) ||
      !hasPrivateFileSecurity(path)) {
    invalidRid();
  }
  auto parentDirectory = path.parent_path();
  if (parentDirectory.empty()) {
    parentDirectory = std::filesystem::current_path(error);
    if (error) {
      invalidRid();
    }
  }
  if (!hasPrivateDirectorySecurity(parentDirectory)) {
    invalidRid();
  }
  auto const size = std::filesystem::file_size(path, error);
  if (error || size > maximumBytes) {
    invalidRid();
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    invalidRid();
  }
  std::string contents{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  if (input.bad() || contents.size() != size) {
    invalidRid();
  }
  return contents;
}

[[nodiscard]] std::map<std::string, AuthorizationProfile> parseProfiles(
    std::string const& contents) {
  std::map<std::string, AuthorizationProfile> profiles;
  std::optional<std::string> activeProfile;
  std::size_t cursor = 0U;
  while (cursor <= contents.size()) {
    auto const newline = contents.find('\n', cursor);
    auto line = std::string_view(contents).substr(
        cursor,
        newline == std::string::npos ? std::string::npos : newline - cursor);
    cursor = newline == std::string::npos ? contents.size() + 1U : newline + 1U;
    line = trimAscii(line);
    if (line.empty() || line.front() == '#' || line.front() == ';') {
      continue;
    }
    if (line.front() == '[') {
      if (line.size() < 3U || line.back() != ']') {
        invalidRid();
      }
      auto const section = trimAscii(line.substr(1U, line.size() - 2U));
      if (section.starts_with(kAuthorizationSectionPrefix)) {
        auto const profileName = trimAscii(section.substr(kAuthorizationSectionPrefix.size()));
        if (profileName.empty()) {
          invalidRid();
        }
        std::string const name(profileName);
        if (!profiles.emplace(name, AuthorizationProfile{}).second) {
          invalidRid();
        }
        activeProfile = name;
      } else {
        activeProfile.reset();
      }
      continue;
    }
    if (!activeProfile) {
      continue;
    }
    auto const separator = line.find('=');
    if (separator == std::string_view::npos) {
      invalidRid();
    }
    auto const key = trimAscii(line.substr(0U, separator));
    auto const value = trimAscii(line.substr(separator + 1U));
    if (key.empty() || value.empty()) {
      invalidRid();
    }
    auto& profile = profiles.at(*activeProfile);
    if (key == "service") {
      if (profile.service) {
        invalidRid();
      }
      profile.service = std::string(value);
    } else if (key == "globalPasswordFile") {
      if (profile.globalPasswordFile) {
        invalidRid();
      }
      profile.globalPasswordFile = std::string(value);
    } else {
      invalidRid();
    }
  }
  return profiles;
}

[[nodiscard]] std::string selectedProfileName(
    rti1516_2025::RtiConfiguration const* configuration) {
  if (configuration == nullptr || configuration->configurationName().empty()) {
    return "default";
  }
  auto const name = utf8FromWide(configuration->configurationName());
  if (!name || name->empty()) {
    invalidRid();
  }
  return *name;
}

}  // namespace

std::unique_ptr<rti1516_2025::AuthorizerFactory>
makeAuthorizerFactoryFromRtiInitializationData(
    rti1516_2025::RtiConfiguration const* configuration) {
  auto const ridPath = configuredRidPath();
  if (!ridPath) {
    return {};
  }

  auto const ridContents = readProtectedFile(*ridPath, kMaximumRidBytes);
  auto profiles = parseProfiles(ridContents);
  if (profiles.empty()) {
    return {};
  }

  auto const profileName = selectedProfileName(configuration);
  auto const profileIterator = profiles.find(profileName);
  if (profileIterator == profiles.end()) {
    invalidRid();
  }
  auto const& profile = profileIterator->second;
  if (!profile.service) {
    if (profile.globalPasswordFile) {
      invalidRid();
    }
    return {};
  }
  if (*profile.service == "disabled") {
    if (profile.globalPasswordFile) {
      invalidRid();
    }
    return {};
  }
  if (*profile.service != "HLAauthorizer" || !profile.globalPasswordFile) {
    invalidRid();
  }

  auto passwordPath = std::filesystem::u8path(*profile.globalPasswordFile);
  if (passwordPath.is_relative()) {
    passwordPath = ridPath->parent_path() / passwordPath;
  }
  auto const encodedPassword =
      readProtectedFile(passwordPath, kMaximumPasswordBytes);
  auto const globalPassword = wideFromUtf8(encodedPassword);
  if (!globalPassword || globalPassword->find(L'\0') != std::wstring::npos) {
    invalidRid();
  }

  ReferenceAuthorizerConfiguration authorizerConfiguration;
  authorizerConfiguration.globalPlainTextPassword = *globalPassword;
  return makeReferenceAuthorizerFactory(std::move(authorizerConfiguration));
}

}  // namespace umbra::detail
