#include "tck_harness.hpp"

#include <string_view>

namespace hla_rti_cpp_tck::detail {

void scenarioTls13TransportSupport(
    Options const& options, rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("tls://", 0U) == 0U,
      "TLS 1.3 transport coverage requires an adapter-supplied tls:// RTI address");

  Session session(options, model, "connection");
  auto const connection = session.connect();
  require(
      connection.addressUsed,
      "Connect did not report use of the adapter-supplied TLS RTI address");

  std::wstring const absentFederation =
      L"__cpp_tck_tls_transport_absent_federation__";
  session.rtiAmbassador().listFederationExecutions();
  waitFor(
      session,
      [&] {
        return session.recorder().lastFederationExecutionReportOmits(
            absentFederation);
      },
      options,
      "TLS federation-execution query response");
  session.disconnect();
}

void scenarioTls13EncryptionOnlyMode(
    Options const& options, rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("tls://", 0U) == 0U,
      "TLS encryption-only coverage requires an adapter-supplied tls:// RTI address");
  require(
      !options.additionalSettings.empty(),
      "TLS encryption-only coverage requires adapter-supplied mode settings");

  Session session(options, model, "connection");
  auto const connection = session.connect();
  require(
      connection.addressUsed,
      "Connect did not report use of the adapter-supplied TLS RTI address");
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "Connect did not report that the adapter-supplied TLS mode settings were applied");

  std::wstring const absentFederation =
      L"__cpp_tck_tls_encryption_only_absent_federation__";
  session.rtiAmbassador().listFederationExecutions();
  waitFor(
      session,
      [&] {
        return session.recorder().lastFederationExecutionReportOmits(
            absentFederation);
      },
      options,
      "TLS encryption-only federation-execution query response");
  session.disconnect();
}

void scenarioTls13ServerCertificateValidation(
    Options const& options, rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("tls://", 0U) == 0U,
      "TLS server-certificate validation requires an adapter-supplied tls:// RTI address");
  require(
      !options.additionalSettings.empty(),
      "TLS server-certificate validation requires adapter-supplied mode settings");

  Session session(options, model, "connection");
  auto const connection = session.connect();
  require(
      connection.addressUsed,
      "Connect did not report use of the adapter-supplied TLS RTI address");
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "Connect did not report that the adapter-supplied TLS validation settings were applied");

  std::wstring const absentFederation =
      L"__cpp_tck_tls_server_validation_absent_federation__";
  session.rtiAmbassador().listFederationExecutions();
  waitFor(
      session,
      [&] {
        return session.recorder().lastFederationExecutionReportOmits(
            absentFederation);
      },
      options,
      "TLS server-validation federation-execution query response");
  session.disconnect();
}

void scenarioTls13ServerCertificateValidationRejectsUntrusted(
    Options const& options, rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("tls://", 0U) == 0U,
      "TLS untrusted-server rejection requires an adapter-supplied tls:// RTI address");
  require(
      !options.additionalSettings.empty(),
      "TLS untrusted-server rejection requires adapter-supplied mode settings");

  Session session(options, model, "connection");
  bool rejected = false;
  try {
    static_cast<void>(session.connect());
  } catch (rti::ConnectionFailed const&) {
    rejected = true;
  }
  require(
      rejected,
      "Connect did not reject a TLS server certificate outside the configured trust store");
}

void scenarioTls13DefaultPortSupport(
    Options const& options, rti::CallbackModel model) {
  constexpr std::string_view scheme{"tls://"};
  require(
      options.rtiAddress.rfind(scheme, 0U) == 0U,
      "TLS default-port coverage requires an adapter-supplied tls:// RTI address");

  auto const authorityStart = scheme.size();
  auto const authorityEnd = options.rtiAddress.find_first_of("/?#", authorityStart);
  auto const authority = std::string_view{options.rtiAddress}.substr(
      authorityStart,
      authorityEnd == std::string::npos ? std::string::npos
                                        : authorityEnd - authorityStart);
  bool hasExplicitPort = false;
  if (!authority.empty() && authority.front() == '[') {
    auto const closingBracket = authority.find(']');
    require(
        closingBracket != std::string_view::npos,
        "TLS default-port RTI address has an unterminated IPv6 host");
    hasExplicitPort = closingBracket + 1U < authority.size();
  } else {
    hasExplicitPort = authority.find(':') != std::string_view::npos;
  }
  require(
      !hasExplicitPort,
      "TLS default-port coverage requires an RTI address with no explicit port");

  Session session(options, model, "connection");
  auto const connection = session.connect();
  require(
      connection.addressUsed,
      "Connect did not report use of the adapter-supplied TLS RTI address");

  std::wstring const absentFederation =
      L"__cpp_tck_tls_default_port_absent_federation__";
  session.rtiAmbassador().listFederationExecutions();
  waitFor(
      session,
      [&] {
        return session.recorder().lastFederationExecutionReportOmits(
            absentFederation);
      },
      options,
      "TLS default-port federation-execution query response");
  session.disconnect();
}

void scenarioTls13AlternateAddressPortConfiguration(
    Options const& options, rti::CallbackModel model) {
  constexpr std::string_view scheme{"tls://"};
  require(
      options.rtiAddress.rfind(scheme, 0U) == 0U,
      "Alternate RTI address/port coverage requires an adapter-supplied tls:// address");

  auto const authorityStart = scheme.size();
  auto const authorityEnd = options.rtiAddress.find_first_of("/?#", authorityStart);
  auto const authority = std::string_view{options.rtiAddress}.substr(
      authorityStart,
      authorityEnd == std::string::npos ? std::string::npos
                                        : authorityEnd - authorityStart);
  std::size_t portSeparator = std::string_view::npos;
  if (!authority.empty() && authority.front() == '[') {
    auto const closingBracket = authority.find(']');
    require(
        closingBracket != std::string_view::npos,
        "Alternate RTI address/port has an unterminated IPv6 host");
    portSeparator = closingBracket + 1U;
    require(
        portSeparator < authority.size() && authority[portSeparator] == ':',
        "Alternate RTI address/port coverage requires an explicit port");
  } else {
    portSeparator = authority.rfind(':');
    require(
        portSeparator != std::string_view::npos,
        "Alternate RTI address/port coverage requires an explicit port");
  }

  auto const portText = authority.substr(portSeparator + 1U);
  unsigned int port = 0U;
  require(
      !portText.empty(),
      "Alternate RTI address/port coverage requires a valid explicit port");
  for (char const character : portText) {
    require(
        character >= '0' && character <= '9',
        "Alternate RTI address/port coverage requires a numeric port");
    auto const digit = static_cast<unsigned int>(character - '0');
    require(
        port <= (65535U - digit) / 10U,
        "Alternate RTI address/port coverage requires a port in [1, 65535]");
    port = port * 10U + digit;
  }
  require(
      port > 0U,
      "Alternate RTI address/port coverage requires a port in [1, 65535]");
  require(
      port != 15165U,
      "Alternate RTI address/port coverage requires a port other than the TLS default 15165");

  scenarioTls13TransportSupport(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
