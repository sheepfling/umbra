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
#include <string_view>
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
      L"Umbra process RtiConfiguration::rtiAddress must use tcp:// or tls:// followed by a host and optional port.");
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
configuredAmbassadorProcessEndpoint(
    RtiConfiguration const* configuration,
    bool& tlsCertificateValidationSettingApplied,
    bool& tlsCertificateValidationSettingFailedToParse) {
  tlsCertificateValidationSettingApplied = false;
  tlsCertificateValidationSettingFailedToParse = false;
  if (configuration == nullptr || configuration->rtiAddress().empty() ||
      configuration->rtiAddress() == L"in-process") {
    return std::nullopt;
  }

  auto const& configuredAddress = configuration->rtiAddress();
  constexpr std::wstring_view tcpPrefix = L"tcp://";
  constexpr std::wstring_view tlsPrefix = L"tls://";
  constexpr std::wstring_view secureWebSocketPrefix = L"wss://";
  constexpr std::wstring_view websocketPrefix = L"ws://";
  auto security =
      umbra::detail::ProcessTransportAddress::Security::tcp;
  auto framing =
      umbra::detail::ProcessTransportAddress::Framing::process_stream;
  auto defaultPort = static_cast<std::uint16_t>(15164U);
  std::wstring_view prefix;
  if (configuredAddress.starts_with(tcpPrefix)) {
    prefix = tcpPrefix;
  } else if (configuredAddress.starts_with(tlsPrefix)) {
    prefix = tlsPrefix;
    security = umbra::detail::ProcessTransportAddress::Security::tls13;
    defaultPort = static_cast<std::uint16_t>(15165U);
  } else if (configuredAddress.starts_with(secureWebSocketPrefix)) {
    prefix = secureWebSocketPrefix;
    security = umbra::detail::ProcessTransportAddress::Security::tls13;
    framing = umbra::detail::ProcessTransportAddress::Framing::websocket;
    defaultPort = static_cast<std::uint16_t>(443U);
  } else if (configuredAddress.starts_with(websocketPrefix)) {
    prefix = websocketPrefix;
    framing = umbra::detail::ProcessTransportAddress::Framing::websocket;
    defaultPort = static_cast<std::uint16_t>(80U);
  } else {
    throwInvalidProcessAddress();
  }
  auto const encodedAddress =
      umbra::detail::utf8FromWide(configuredAddress.substr(prefix.size()));
  if (!encodedAddress || encodedAddress->empty()) {
    throwInvalidProcessAddress();
  }

  std::string authority;
  std::string websocketResource = "/";
  if (framing ==
      umbra::detail::ProcessTransportAddress::Framing::websocket) {
    if (encodedAddress->find('#') != std::string::npos) {
      throwInvalidProcessAddress();
    }
    auto const resourceBegin = encodedAddress->find_first_of("/?");
    authority = encodedAddress->substr(0U, resourceBegin);
    if (resourceBegin != std::string::npos) {
      websocketResource = encodedAddress->substr(resourceBegin);
      if (websocketResource.front() == '?') {
        websocketResource.insert(websocketResource.begin(), '/');
      }
      for (unsigned char character : websocketResource) {
        if (character <= 0x20U || character >= 0x7fU) {
          throwInvalidProcessAddress();
        }
      }
    }
  } else {
    authority = *encodedAddress;
  }
  if (authority.empty()) {
    throwInvalidProcessAddress();
  }
  std::string websocketHostHeader;
  if (framing ==
      umbra::detail::ProcessTransportAddress::Framing::websocket) {
    websocketHostHeader = authority;
  }
  std::string host;
  std::optional<std::string> portText;
  if (authority.front() == '[') {
    auto const closingBracket = authority.find(']');
    if (closingBracket == std::string::npos || closingBracket <= 1U) {
      throwInvalidProcessAddress();
    }
    host = authority.substr(1U, closingBracket - 1U);
    if (closingBracket + 1U < authority.size()) {
      if (authority[closingBracket + 1U] != ':' ||
          closingBracket + 2U >= authority.size()) {
        throwInvalidProcessAddress();
      }
      portText = authority.substr(closingBracket + 2U);
    }
  } else {
    auto const separator = authority.find(':');
    if (separator == std::string::npos) {
      host = authority;
    } else {
      if (separator == 0U || separator + 1U >= authority.size() ||
          authority.find(':', separator + 1U) != std::string::npos) {
        throwInvalidProcessAddress();
      }
      host = authority.substr(0U, separator);
      portText = authority.substr(separator + 1U);
    }
  }
  if (host.empty() || authority.find_first_of("/?#@ \t") != std::string::npos) {
    throwInvalidProcessAddress();
  }
  std::uint32_t port = defaultPort;
  if (portText) {
    for (unsigned char character : *portText) {
      if (character < '0' || character > '9') {
        throwInvalidProcessAddress();
      }
    }
    auto const parsed = std::from_chars(
        portText->data(), portText->data() + portText->size(), port);
    if (parsed.ec != std::errc{} ||
        parsed.ptr != portText->data() + portText->size() ||
        port == 0U || port > 65535U) {
      throwInvalidProcessAddress();
    }
  }
  std::filesystem::path trustStoreFile;
  auto tlsCertificateValidationMode =
      umbra::detail::platform::TlsClientConfiguration::
          CertificateValidationMode::server_certificate;
  if (security ==
      umbra::detail::ProcessTransportAddress::Security::tls13) {
    auto const& settings = configuration->additionalSettings();
    constexpr std::wstring_view validationPrefix =
        L"tlsCertificateValidation=";
    bool validationSettingSeen = false;
    std::size_t settingsBegin = 0U;
    while (settingsBegin <= settings.size()) {
      auto const settingsEnd = settings.find(L';', settingsBegin);
      auto const token = std::wstring_view(settings).substr(
          settingsBegin,
          settingsEnd == std::wstring::npos
              ? std::wstring::npos
              : settingsEnd - settingsBegin);
      if (token.starts_with(validationPrefix)) {
        if (validationSettingSeen) {
          tlsCertificateValidationSettingApplied = false;
          tlsCertificateValidationSettingFailedToParse = true;
          tlsCertificateValidationMode =
              umbra::detail::platform::TlsClientConfiguration::
                  CertificateValidationMode::server_certificate;
          break;
        }
        validationSettingSeen = true;
        auto const value = token.substr(validationPrefix.size());
        if (value == L"none") {
          tlsCertificateValidationMode =
              umbra::detail::platform::TlsClientConfiguration::
                  CertificateValidationMode::encryption_only;
          tlsCertificateValidationSettingApplied = true;
        } else if (value == L"server") {
          tlsCertificateValidationMode =
              umbra::detail::platform::TlsClientConfiguration::
                  CertificateValidationMode::server_certificate;
          tlsCertificateValidationSettingApplied = true;
        } else {
          tlsCertificateValidationSettingFailedToParse = true;
          tlsCertificateValidationMode =
              umbra::detail::platform::TlsClientConfiguration::
                  CertificateValidationMode::server_certificate;
          break;
        }
      }
      if (settingsEnd == std::wstring::npos) {
        break;
      }
      settingsBegin = settingsEnd + 1U;
    }
    if (tlsCertificateValidationMode ==
        umbra::detail::platform::TlsClientConfiguration::
            CertificateValidationMode::server_certificate) {
      if (auto const* trustStore =
              std::getenv("UMBRA_RTI_TLS_TRUST_STORE_FILE");
          trustStore != nullptr && *trustStore != '\0') {
        trustStoreFile = std::filesystem::path(trustStore);
      }
    }
  }
  return umbra::detail::ProcessTransportAddress{
      std::move(host),
      static_cast<std::uint16_t>(port),
      security,
      std::move(trustStoreFile),
      tlsCertificateValidationMode,
      framing,
      std::move(websocketHostHeader),
      std::move(websocketResource)};
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

std::set<std::string> configuredAmbassadorUnsupportedTransportationTypes(
    RtiConfiguration const* configuration,
    bool& settingsApplied,
    bool& settingsFailedToParse) {
  settingsApplied = false;
  settingsFailedToParse = false;
  if (configuration == nullptr || configuration->additionalSettings().empty()) {
    return {};
  }

  constexpr std::wstring_view prefix = L"unsupportedTransportationTypes=";
  std::set<std::string> result;
  std::wstring const& settings = configuration->additionalSettings();
  std::size_t begin = 0U;
  while (begin <= settings.size()) {
    auto const end = settings.find(L';', begin);
    auto const token = std::wstring_view(settings).substr(
        begin,
        end == std::wstring::npos ? std::wstring::npos : end - begin);
    if (token.starts_with(prefix)) {
      settingsApplied = true;
      auto value = token.substr(prefix.size());
      if (value.empty()) {
        settingsFailedToParse = true;
        return {};
      }
      while (!value.empty()) {
        auto const comma = value.find(L',');
        auto const name = value.substr(
            0U,
            comma == std::wstring_view::npos ? std::wstring_view::npos : comma);
        auto const encodedName = umbra::detail::utf8FromWide(name);
        if (!encodedName || encodedName->empty() ||
            *encodedName == "HLAreliable" || *encodedName == "HLAbestEffort") {
          settingsFailedToParse = true;
          return {};
        }
        result.insert(*encodedName);
        if (comma == std::wstring_view::npos) {
          value = {};
        } else {
          value.remove_prefix(comma + 1U);
          if (value.empty()) {
            settingsFailedToParse = true;
            return {};
          }
        }
      }
    }
    if (end == std::wstring::npos) {
      break;
    }
    begin = end + 1U;
  }
  return result;
}

umbra::detail::ProcessFederationClientOptions
configuredAmbassadorProcessFederationClientOptions(
    RtiConfiguration const* configuration,
    bool& settingsApplied,
    bool& settingsFailedToParse) {
  settingsApplied = false;
  settingsFailedToParse = false;
  auto options = umbra::detail::ProcessFederationClientOptions{};
  if (configuration == nullptr || configuration->additionalSettings().empty()) {
    return options;
  }

  auto parseSeconds = [](std::wstring_view value,
                         std::chrono::milliseconds& duration) noexcept {
    if (value.empty()) {
      return false;
    }
    auto const separator = value.find(L'.');
    if (separator != std::wstring_view::npos &&
        value.find(L'.', separator + 1U) != std::wstring_view::npos) {
      return false;
    }
    auto const wholeText = value.substr(0U, separator);
    auto const fractionText = separator == std::wstring_view::npos
        ? std::wstring_view{}
        : value.substr(separator + 1U);
    if (wholeText.empty() ||
        (separator != std::wstring_view::npos &&
         (fractionText.empty() || fractionText.size() > 3U))) {
      return false;
    }

    auto const maximumMilliseconds = static_cast<std::uint64_t>(
        std::chrono::milliseconds::max().count());
    std::uint64_t wholeSeconds = 0U;
    for (wchar_t character : wholeText) {
      if (character < L'0' || character > L'9') {
        return false;
      }
      auto const digit = static_cast<std::uint64_t>(character - L'0');
      if (wholeSeconds >
          (maximumMilliseconds / 1000U - digit) / 10U) {
        return false;
      }
      wholeSeconds = wholeSeconds * 10U + digit;
    }

    std::uint64_t fractionMilliseconds = 0U;
    for (wchar_t character : fractionText) {
      if (character < L'0' || character > L'9') {
        return false;
      }
      fractionMilliseconds = fractionMilliseconds * 10U +
          static_cast<std::uint64_t>(character - L'0');
    }
    if (fractionText.size() == 1U) {
      fractionMilliseconds *= 100U;
    } else if (fractionText.size() == 2U) {
      fractionMilliseconds *= 10U;
    }
    if (wholeSeconds >
        (maximumMilliseconds - fractionMilliseconds) / 1000U) {
      return false;
    }
    auto const totalMilliseconds = wholeSeconds * 1000U + fractionMilliseconds;
    if (totalMilliseconds == 0U) {
      return false;
    }
    duration = std::chrono::milliseconds(totalMilliseconds);
    return true;
  };

  std::wstring const& settings = configuration->additionalSettings();
  constexpr std::wstring_view heartbeatKey = L"FED_INT_HEART=";
  constexpr std::wstring_view heartbeatTimeoutKey = L"FED_TIMEOUT_HEART=";
  constexpr std::wstring_view reconnectTimeoutKey = L"FED_TIMEOUT_RECONNECT=";
  bool heartbeatIntervalSeen = false;
  bool heartbeatTimeoutSeen = false;
  bool reconnectTimeoutSeen = false;
  std::size_t begin = 0U;
  while (begin <= settings.size()) {
    auto const end = settings.find(L';', begin);
    auto const token = std::wstring_view(settings).substr(
        begin,
        end == std::wstring::npos ? std::wstring::npos : end - begin);
    if (token.starts_with(heartbeatKey)) {
      if (heartbeatIntervalSeen ||
          !parseSeconds(
              token.substr(heartbeatKey.size()),
              options.federateHeartbeatInterval)) {
        settingsApplied = false;
        settingsFailedToParse = true;
        return umbra::detail::ProcessFederationClientOptions{};
      }
      heartbeatIntervalSeen = true;
      settingsApplied = true;
    } else if (token.starts_with(heartbeatTimeoutKey)) {
      if (heartbeatTimeoutSeen ||
          !parseSeconds(
              token.substr(heartbeatTimeoutKey.size()),
              options.federateHeartbeatTimeout)) {
        settingsApplied = false;
        settingsFailedToParse = true;
        return umbra::detail::ProcessFederationClientOptions{};
      }
      heartbeatTimeoutSeen = true;
      settingsApplied = true;
    } else if (token.starts_with(reconnectTimeoutKey)) {
      if (reconnectTimeoutSeen ||
          !parseSeconds(
              token.substr(reconnectTimeoutKey.size()),
              options.transportRecoveryWindow)) {
        settingsApplied = false;
        settingsFailedToParse = true;
        return umbra::detail::ProcessFederationClientOptions{};
      }
      reconnectTimeoutSeen = true;
      settingsApplied = true;
    }
    if (end == std::wstring::npos) {
      break;
    }
    begin = end + 1U;
  }
  return options;
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
