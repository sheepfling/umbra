#include "internal/runtime/rti_initialization_data.hpp"

#include "internal/runtime/reference_authorizer.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/platform/protected_initialization_data.hpp"

#include <RTI/auth/AuthorizerFactory.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace umbra::detail {
namespace {

constexpr std::string_view kAuthorizationSectionPrefix = "authorization.";
constexpr std::string_view kDisabledAuthorizationService = "disabled";
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
// Keep this view-based trim local to the RID parser: its grammar trims only
// ASCII space, tab, and carriage return around tokens.
[[nodiscard]] std::string_view trimAscii(std::string_view value) noexcept {
  auto isTrim = [](const char character) {
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

[[nodiscard]] std::string readProtectedFile(
    std::filesystem::path const& path,
    const std::size_t maximumBytes) {
  std::error_code error;
  auto const linkStatus = std::filesystem::symlink_status(path, error);
  if (error || !std::filesystem::is_regular_file(linkStatus) ||
      !platform::isPrivateRegularFile(path)) {
    invalidRid();
  }
  auto parentDirectory = path.parent_path();
  if (parentDirectory.empty()) {
    parentDirectory = std::filesystem::current_path(error);
    if (error) {
      invalidRid();
    }
  }
  if (!platform::isPrivateDirectory(parentDirectory)) {
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

// The implementation-defined RID profile grammar is documented in
// docs/design/AUTHORIZATION-DESIGN.md.
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
  auto const ridPath = platform::configuredInitializationDataPath();
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
  if (*profile.service == kDisabledAuthorizationService) {
    if (profile.globalPasswordFile) {
      invalidRid();
    }
    return {};
  }
  auto const authorizerName = utf8FromWide(rti1516_2025::HLAauthorizerName);
  if (!authorizerName || *profile.service != *authorizerName ||
      !profile.globalPasswordFile) {
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
