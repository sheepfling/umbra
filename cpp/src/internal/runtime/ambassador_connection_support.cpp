#include "internal/runtime/ambassador_connection_support.hpp"

#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/RTI1516.h>
#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAnoCredentials.h>

#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace rti1516_2025::umbra_binding_detail {

namespace {

#if defined(_WIN32)
std::filesystem::path windowsEnvironmentPath(wchar_t const* name) {
  auto const required = GetEnvironmentVariableW(name, nullptr, 0U);
  if (required == 0U) {
    return {};
  }
  std::wstring value(static_cast<std::size_t>(required), L'\0');
  auto const written = GetEnvironmentVariableW(
      name,
      value.data(),
      required);
  if (written == 0U || written >= required) {
    return {};
  }
  value.resize(static_cast<std::size_t>(written));
  return std::filesystem::path(value);
}
#endif

std::filesystem::path defaultFederationSaveDirectory() {
#if defined(_WIN32)
  auto root = windowsEnvironmentPath(L"LOCALAPPDATA");
  if (root.empty()) {
    root = windowsEnvironmentPath(L"APPDATA");
  }
  if (!root.empty()) {
    return root / L"Umbra" / L"FederationSaves";
  }
#elif defined(__APPLE__)
  if (auto const* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
    return std::filesystem::path(home) / "Library" / "Application Support" /
        "Umbra" / "FederationSaves";
  }
#else
  if (auto const* stateHome = std::getenv("XDG_STATE_HOME");
      stateHome != nullptr && *stateHome != '\0') {
    return std::filesystem::path(stateHome) / "umbra" / "federation-saves";
  }
  if (auto const* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
    return std::filesystem::path(home) / ".local" / "state" / "umbra" /
        "federation-saves";
  }
#endif
  throw RTIinternalError(
      L"Umbra could not determine a persistent federation-save directory; "
      L"configure federationSaveDirectory explicitly.");
}

[[noreturn]] void throwInvalidProcessAddress() {
  throw ConnectionFailed(
      L"Umbra process RtiConfiguration::rtiAddress must use tcp://host:port.");
}

}  // namespace

void validateAmbassadorConnectionCallbackModel(CallbackModel callbackModel) {
  switch (callbackModel) {
    case HLA_IMMEDIATE:
    case HLA_EVOKED:
      return;
  }
  throw UnsupportedCallbackModel(
      L"Umbra supports only HLA_IMMEDIATE and HLA_EVOKED callback models.");
}

ConfigurationResult ignoredAmbassadorConnectionResult() {
  return ConfigurationResult(false, false, SETTINGS_IGNORED);
}

std::optional<umbra::detail::ProcessTransportAddress>
configuredAmbassadorProcessEndpoint(RtiConfiguration const* configuration) {
  if (configuration == nullptr || configuration->rtiAddress().empty() ||
      configuration->rtiAddress() == L"in-process") {
    return std::nullopt;
  }

  constexpr std::wstring_view prefix = L"tcp://";
  auto const& configuredAddress = configuration->rtiAddress();
  if (!configuredAddress.starts_with(prefix)) {
    throwInvalidProcessAddress();
  }
  auto const encodedAddress =
      umbra::detail::utf8FromWide(configuredAddress.substr(prefix.size()));
  if (!encodedAddress || encodedAddress->empty()) {
    throwInvalidProcessAddress();
  }

  std::string const authority = *encodedAddress;
  std::string host;
  std::string portText;
  if (authority.front() == '[') {
    auto const closingBracket = authority.find(']');
    if (closingBracket == std::string::npos || closingBracket <= 1U ||
        closingBracket + 1U >= authority.size() ||
        authority[closingBracket + 1U] != ':') {
      throwInvalidProcessAddress();
    }
    host = authority.substr(1U, closingBracket - 1U);
    portText = authority.substr(closingBracket + 2U);
  } else {
    auto const separator = authority.rfind(':');
    if (separator == std::string::npos || separator == 0U ||
        separator + 1U >= authority.size() ||
        authority.find(':') != separator) {
      throwInvalidProcessAddress();
    }
    host = authority.substr(0U, separator);
    portText = authority.substr(separator + 1U);
  }
  if (host.empty() || portText.empty()) {
    throwInvalidProcessAddress();
  }
  for (unsigned char character : portText) {
    if (character < '0' || character > '9') {
      throwInvalidProcessAddress();
    }
  }
  std::uint32_t port = 0U;
  auto const parsed = std::from_chars(
      portText.data(), portText.data() + portText.size(), port);
  if (parsed.ec != std::errc{} || parsed.ptr != portText.data() + portText.size() ||
      port == 0U || port > 65535U) {
    throwInvalidProcessAddress();
  }
  return umbra::detail::ProcessTransportAddress{
      std::move(host), static_cast<std::uint16_t>(port)};
}

std::string ambassadorProcessEndpointIdentity(
    RtiConfiguration const* configuration) {
  std::string identity = "umbra-rti-client";
  if (configuration != nullptr && !configuration->configurationName().empty()) {
    auto const encodedName =
        umbra::detail::utf8FromWide(configuration->configurationName());
    if (!encodedName || encodedName->empty()) {
      throw ConnectionFailed(
          L"Umbra process configurationName must be a nonempty UTF-8 endpoint identity.");
    }
    identity = *encodedName;
  }
  if (identity.size() > umbra::detail::kTransportMaximumEndpointIdBytes ||
      identity.find('\0') != std::string::npos) {
    throw ConnectionFailed(
        L"Umbra process configurationName exceeds the endpoint identity limit.");
  }
  return identity;
}

std::uint64_t nextAmbassadorProcessEndpointSessionId() noexcept {
  static std::atomic_uint64_t next{1U};
  auto const value = next.fetch_add(1U, std::memory_order_relaxed);
  return value == 0U ? next.fetch_add(1U, std::memory_order_relaxed) : value;
}

void requireNoCredentialsWhenAmbassadorAuthorizationIsDisabled(
    Credentials const& credentials) {
  if (credentials.getType() == HLAnoCredentialsType &&
      credentials.getData().size() == 0U) {
    return;
  }
  throw Unauthorized(
      L"Umbra has no authorization service configured for supplied credentials.");
}

void authorizeAmbassadorConnectIfConfigured(
    rti1516_2025::Authorizer& authorizer,
    Credentials const* credentials) {
  HLAnoCredentials noCredentials;
  auto const result = authorizer.authorizeRtiOperation(
      credentials == nullptr ? static_cast<Credentials const&>(noCredentials)
                             : *credentials);
  using Code = rti1516_2025::AuthorizationResult::Code;
  switch (result.getCode()) {
    case Code::AUTHORIZED:
      return;
    case Code::UNAUTHORIZED:
      throw Unauthorized(
          result.getMessage().empty() ? L"Connect is unauthorized."
                                      : result.getMessage());
    case Code::INVALID_CREDENTIALS:
      throw InvalidCredentials(
          result.getMessage().empty() ? L"Connect credentials are invalid."
                                      : result.getMessage());
    case Code::AUTHORIZATION_ERROR:
      throw RTIinternalError(
          result.getMessage().empty()
              ? L"The configured authorizer could not evaluate Connect."
              : result.getMessage());
  }
  throw RTIinternalError(L"The configured authorizer returned an unknown result.");
}

std::filesystem::path configuredAmbassadorServiceReportDirectory(
    RtiConfiguration const* configuration,
    bool& settingsApplied) {
  settingsApplied = false;
  std::filesystem::path defaultDirectory;
  try {
    defaultDirectory =
        std::filesystem::temp_directory_path() / "umbra-service-reports";
  } catch (std::filesystem::filesystem_error const&) {
    throw RTIinternalError(
        L"Umbra could not determine a default service-report directory.");
  }
  if (configuration == nullptr || configuration->additionalSettings().empty()) {
    return defaultDirectory;
  }
  constexpr std::wstring_view prefix = L"serviceReportDirectory=";
  auto const& settings = configuration->additionalSettings();
  std::size_t begin = 0U;
  while (begin <= settings.size()) {
    auto const end = settings.find(L';', begin);
    auto const token = std::wstring_view(settings).substr(
        begin,
        end == std::wstring::npos ? std::wstring::npos : end - begin);
    if (token.starts_with(prefix)) {
      auto const value = token.substr(prefix.size());
      if (value.empty()) {
        throw RTIinternalError(
            L"Umbra serviceReportDirectory requires a nonempty directory.");
      }
      settingsApplied = true;
      return std::filesystem::path(value);
    }
    if (end == std::wstring::npos) {
      break;
    }
    begin = end + 1U;
  }
  return defaultDirectory;
}

std::filesystem::path configuredAmbassadorFederationSaveDirectory(
    RtiConfiguration const* configuration,
    bool& settingsApplied) {
  settingsApplied = false;
  if (configuration == nullptr || configuration->additionalSettings().empty()) {
    return defaultFederationSaveDirectory();
  }
  constexpr std::wstring_view prefix = L"federationSaveDirectory=";
  auto const& settings = configuration->additionalSettings();
  std::size_t begin = 0U;
  while (begin <= settings.size()) {
    auto const end = settings.find(L';', begin);
    auto const token = std::wstring_view(settings).substr(
        begin,
        end == std::wstring::npos ? std::wstring::npos : end - begin);
    if (token.starts_with(prefix)) {
      auto const value = token.substr(prefix.size());
      if (value.empty()) {
        throw RTIinternalError(
            L"Umbra federationSaveDirectory requires a nonempty directory.");
      }
      settingsApplied = true;
      return std::filesystem::path(value);
    }
    if (end == std::wstring::npos) {
      break;
    }
    begin = end + 1U;
  }
  return defaultFederationSaveDirectory();
}

umbra::detail::FomStandardEdition configuredAmbassadorFomStandardEdition(
    RtiConfiguration const* configuration,
    bool& settingsApplied,
    bool& settingsFailedToParse) {
  settingsApplied = false;
  settingsFailedToParse = false;
  if (configuration == nullptr || configuration->additionalSettings().empty()) {
    return umbra::detail::FomStandardEdition::ieee1516_2025;
  }

  std::wstring const& settings = configuration->additionalSettings();
  constexpr std::wstring_view key = L"fomEdition=";
  std::size_t cursor = settings.find(key);
  while (cursor != std::wstring::npos) {
    bool const atTokenBoundary = cursor == 0 || settings[cursor - 1] == L';';
    if (atTokenBoundary) {
      std::size_t const valueBegin = cursor + key.size();
      std::size_t const valueEnd = settings.find(L';', valueBegin);
      std::wstring const value = settings.substr(
          valueBegin,
          valueEnd == std::wstring::npos ? std::wstring::npos : valueEnd - valueBegin);
      if (value == L"2010" || value == L"IEEE1516-2010") {
        settingsApplied = true;
        return umbra::detail::FomStandardEdition::ieee1516_2010;
      }
      if (value == L"2025" || value == L"IEEE1516-2025") {
        settingsApplied = true;
        return umbra::detail::FomStandardEdition::ieee1516_2025;
      }
      settingsFailedToParse = true;
      return umbra::detail::FomStandardEdition::ieee1516_2025;
    }
    cursor = settings.find(key, cursor + key.size());
  }
  return umbra::detail::FomStandardEdition::ieee1516_2025;
}

std::filesystem::path validateAmbassadorServiceReportDirectory(
    std::filesystem::path directory) {
  std::error_code error;
  auto absoluteDirectory = std::filesystem::absolute(directory, error);
  if (error) {
    throw RTIinternalError(
        L"Umbra could not resolve the configured service-report directory.");
  }
  std::filesystem::create_directories(absoluteDirectory, error);
  if (error || !std::filesystem::is_directory(absoluteDirectory, error) || error) {
    throw RTIinternalError(
        L"Umbra could not create or access the configured service-report directory.");
  }
  return absoluteDirectory.lexically_normal();
}

umbra::detail::CallbackDispatchModel ambassadorCallbackDispatchModel(
    CallbackModel callbackModel) {
  switch (callbackModel) {
    case HLA_IMMEDIATE:
      return umbra::detail::CallbackDispatchModel::immediate;
    case HLA_EVOKED:
      return umbra::detail::CallbackDispatchModel::evoked;
  }
  throw UnsupportedCallbackModel(
      L"Umbra supports only HLA_IMMEDIATE and HLA_EVOKED callback models.");
}

}  // namespace rti1516_2025::umbra_binding_detail
