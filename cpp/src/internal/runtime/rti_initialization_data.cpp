#include "internal/runtime/rti_initialization_data.hpp"

#include "internal/runtime/reference_authorizer.hpp"
#include "internal/runtime/authorizer_factory_provider.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/platform/protected_initialization_data.hpp"

#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>

#include <chrono>
#include <cstdint>
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
constexpr std::string_view kRtiSectionPrefix = "rti.";
constexpr std::string_view kDisabledAuthorizationService = "disabled";
constexpr std::size_t kMaximumRidBytes = 64U * 1024U;
constexpr std::size_t kMaximumPasswordBytes = 16U * 1024U;

struct RtiInitializationProfile final {
  std::optional<std::string> service;
  std::optional<std::string> globalPasswordFile;
  std::optional<std::chrono::milliseconds> rtiTimeoutHeart;
  std::optional<std::chrono::milliseconds> rtiTimeoutPurge;
  bool authorizationSectionSeen = false;
  bool rtiSectionSeen = false;
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

[[nodiscard]] std::chrono::milliseconds parsePositiveSeconds(
    std::string_view value) {
  if (value.empty()) {
    invalidRid();
  }
  auto const separator = value.find('.');
  if (separator != std::string_view::npos &&
      value.find('.', separator + 1U) != std::string_view::npos) {
    invalidRid();
  }
  auto const wholeText = value.substr(0U, separator);
  auto const fractionText = separator == std::string_view::npos
      ? std::string_view{}
      : value.substr(separator + 1U);
  if (wholeText.empty() ||
      (separator != std::string_view::npos &&
       (fractionText.empty() || fractionText.size() > 3U))) {
    invalidRid();
  }

  auto const maximumMilliseconds = static_cast<std::uint64_t>(
      std::chrono::milliseconds::max().count());
  std::uint64_t wholeSeconds = 0U;
  for (char const character : wholeText) {
    if (character < '0' || character > '9') {
      invalidRid();
    }
    auto const digit = static_cast<std::uint64_t>(character - '0');
    if (wholeSeconds > (maximumMilliseconds / 1000U - digit) / 10U) {
      invalidRid();
    }
    wholeSeconds = wholeSeconds * 10U + digit;
  }

  std::uint64_t fractionMilliseconds = 0U;
  for (char const character : fractionText) {
    if (character < '0' || character > '9') {
      invalidRid();
    }
    fractionMilliseconds = fractionMilliseconds * 10U +
        static_cast<std::uint64_t>(character - '0');
  }
  if (fractionText.size() == 1U) {
    fractionMilliseconds *= 100U;
  } else if (fractionText.size() == 2U) {
    fractionMilliseconds *= 10U;
  }
  if (wholeSeconds >
      (maximumMilliseconds - fractionMilliseconds) / 1000U) {
    invalidRid();
  }
  auto const totalMilliseconds =
      wholeSeconds * 1000U + fractionMilliseconds;
  if (totalMilliseconds == 0U) {
    invalidRid();
  }
  return std::chrono::milliseconds(totalMilliseconds);
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
[[nodiscard]] std::map<std::string, RtiInitializationProfile> parseProfiles(
    std::string const& contents) {
  std::map<std::string, RtiInitializationProfile> profiles;
  std::optional<std::string> activeProfile;
  enum class ActiveSection { none, authorization, rti };
  auto activeSection = ActiveSection::none;
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
      auto const profilePrefix = section.starts_with(kAuthorizationSectionPrefix)
          ? kAuthorizationSectionPrefix
          : section.starts_with(kRtiSectionPrefix) ? kRtiSectionPrefix
                                                   : std::string_view{};
      if (!profilePrefix.empty()) {
        auto const profileName =
            trimAscii(section.substr(profilePrefix.size()));
        if (profileName.empty()) {
          invalidRid();
        }
        std::string const name(profileName);
        auto [profileIterator, inserted] = profiles.try_emplace(name);
        static_cast<void>(inserted);
        auto& profile = profileIterator->second;
        if (profilePrefix == kAuthorizationSectionPrefix) {
          if (profile.authorizationSectionSeen) {
            invalidRid();
          }
          profile.authorizationSectionSeen = true;
          activeSection = ActiveSection::authorization;
        } else {
          if (profile.rtiSectionSeen) {
            invalidRid();
          }
          profile.rtiSectionSeen = true;
          activeSection = ActiveSection::rti;
        }
        activeProfile = name;
      } else {
        activeProfile.reset();
        activeSection = ActiveSection::none;
      }
      continue;
    }
    if (!activeProfile || activeSection == ActiveSection::none) {
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
    if (activeSection == ActiveSection::authorization) {
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
    } else if (activeSection == ActiveSection::rti) {
      if (key == "RTI_TIMEOUT_HEART") {
        if (profile.rtiTimeoutHeart) {
          invalidRid();
        }
        profile.rtiTimeoutHeart = parsePositiveSeconds(value);
      } else if (key == "RTI_TIMEOUT_PURGE") {
        if (profile.rtiTimeoutPurge) {
          invalidRid();
        }
        profile.rtiTimeoutPurge = parsePositiveSeconds(value);
      } else {
        invalidRid();
      }
    }
    if (profile.rtiTimeoutHeart && profile.rtiTimeoutPurge &&
        *profile.rtiTimeoutPurge < *profile.rtiTimeoutHeart) {
      invalidRid();
    }
  }
  return profiles;
}

[[nodiscard]] std::string selectedProfileName(
    std::wstring_view configurationName) {
  if (configurationName.empty()) {
    return "default";
  }
  auto const name = utf8FromWide(configurationName);
  if (!name || name->empty()) {
    invalidRid();
  }
  return *name;
}

[[nodiscard]] std::optional<RtiInitializationProfile> selectedProfile(
    std::wstring_view configurationName,
    std::optional<std::filesystem::path>* ridPath = nullptr) {
  auto configuredPath = platform::configuredInitializationDataPath();
  if (ridPath != nullptr) {
    *ridPath = configuredPath;
  }
  if (!configuredPath) {
    return std::nullopt;
  }

  auto profiles = parseProfiles(
      readProtectedFile(*configuredPath, kMaximumRidBytes));
  if (profiles.empty()) {
    return std::nullopt;
  }
  auto const profileIterator =
      profiles.find(selectedProfileName(configurationName));
  if (profileIterator == profiles.end()) {
    invalidRid();
  }
  return profileIterator->second;
}

}  // namespace

std::unique_ptr<rti1516_2025::AuthorizerFactory>
makeAuthorizerFactoryFromRtiInitializationData(
    rti1516_2025::RtiConfiguration const* configuration) {
  std::optional<std::filesystem::path> ridPath;
  auto profile = selectedProfile(
      configuration == nullptr
          ? std::wstring_view{}
          : std::wstring_view(configuration->configurationName()),
      &ridPath);
  if (!ridPath || !profile) {
    return {};
  }
  auto const& selected = *profile;
  if (!selected.service) {
    if (selected.globalPasswordFile) {
      invalidRid();
    }
    return {};
  }
  if (*selected.service == kDisabledAuthorizationService) {
    if (selected.globalPasswordFile) {
      invalidRid();
    }
    return {};
  }
  auto const authorizerName = utf8FromWide(rti1516_2025::HLAauthorizerName);
  if (!authorizerName) {
    invalidRid();
  }

  if (*selected.service != *authorizerName) {
    if (selected.globalPasswordFile) {
      invalidRid();
    }
    auto const serviceName = wideFromUtf8(*selected.service);
    if (!serviceName || serviceName->empty()) {
      invalidRid();
    }
    auto provider = makeAuthorizerFactoryFromProviderLibrary(*serviceName);
    if (provider.configured && !provider.available) {
      invalidRid();
    }
    if (provider.factory) {
      return std::move(provider.factory);
    }
    auto fallback =
        rti1516_2025::HLAauthorizerFactoryFactory::getAuthorizerFactory(
            *serviceName);
    if (!fallback || fallback->getName() != *serviceName) {
      invalidRid();
    }
    return fallback;
  }
  if (!selected.globalPasswordFile) {
    invalidRid();
  }

  auto passwordPath =
      std::filesystem::u8path(*selected.globalPasswordFile);
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

RtiProtocolTimeoutSettings loadRtiProtocolTimeoutSettingsFromInitializationData(
    const std::wstring_view configurationName) {
  RtiProtocolTimeoutSettings settings;
  auto const profile = selectedProfile(configurationName);
  if (!profile) {
    return settings;
  }
  if (profile->rtiTimeoutHeart) {
    settings.rtiTimeoutHeart = *profile->rtiTimeoutHeart;
  }
  if (profile->rtiTimeoutPurge) {
    settings.rtiTimeoutPurge = *profile->rtiTimeoutPurge;
  }
  if (settings.rtiTimeoutPurge < settings.rtiTimeoutHeart) {
    invalidRid();
  }
  return settings;
}

}  // namespace umbra::detail
