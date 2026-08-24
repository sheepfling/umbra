#include "internal/runtime/reference_authorizer.hpp"

#include <RTI/VariableLengthData.h>
#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/auth/HLAplainTextPassword.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace umbra::detail {
namespace {

class ReferenceHlaAuthorizer final : public rti1516_2025::Authorizer {
 public:
  explicit ReferenceHlaAuthorizer(
      std::optional<std::wstring> globalPlainTextPassword)
      : globalPlainTextPassword_(std::move(globalPlainTextPassword)) {}

  rti1516_2025::AuthorizationResult authorizeRtiOperation(
      rti1516_2025::Credentials const& credentials) override {
    return authorize(credentials);
  }

  rti1516_2025::AuthorizationResult authorizeFederationOperation(
      rti1516_2025::Credentials const& credentials,
      std::wstring const& federationName) override {
    static_cast<void>(federationName);
    return authorize(credentials);
  }

  rti1516_2025::AuthorizationResult authorizeFederateOperation(
      rti1516_2025::Credentials const& credentials,
      std::wstring const& federationName,
      std::wstring const& federateName,
      std::wstring const& federateType) override {
    static_cast<void>(federationName);
    static_cast<void>(federateName);
    static_cast<void>(federateType);
    return authorize(credentials);
  }

  std::wstring getName() const override {
    return rti1516_2025::HLAauthorizerName;
  }

 private:
  [[nodiscard]] rti1516_2025::AuthorizationResult authorize(
      rti1516_2025::Credentials const& credentials) const {
    using rti1516_2025::AuthorizationResult;

    // A factory can be discovered before the RTI's RID is loaded. Do not
    // mistake that discovery-only state for an enabled authorization policy.
    if (!globalPlainTextPassword_) {
      return AuthorizationResult(
          AuthorizationResult::AUTHORIZATION_ERROR,
          L"The reference authorizer has no global plaintext password configured.");
    }

    if (credentials.getType() == rti1516_2025::HLAnoCredentialsType) {
      if (credentials.getData().size() != 0U) {
        return AuthorizationResult(
            AuthorizationResult::INVALID_CREDENTIALS,
            L"HLAnoCredentials must have an empty data value.");
      }
      return AuthorizationResult(
          AuthorizationResult::UNAUTHORIZED,
          L"No credentials do not match the configured authorization policy.");
    }

    if (credentials.getType() != rti1516_2025::HLAplainTextPasswordType) {
      // §12.6 requires an unrecognized credential type to act as an
      // unauthorized operation, rather than exposing an implementation detail.
      return AuthorizationResult(
          AuthorizationResult::UNAUTHORIZED,
          L"The credential type is not supported by HLAauthorizer.");
    }

    try {
      rti1516_2025::HLAplainTextPassword password(credentials.getData());
      if (password.decode() == *globalPlainTextPassword_) {
        return AuthorizationResult(AuthorizationResult::AUTHORIZED);
      }
    } catch (rti1516_2025::EncoderException const&) {
      return AuthorizationResult(
          AuthorizationResult::INVALID_CREDENTIALS,
          L"The HLAplainTextPassword credential data is invalid.");
    }

    return AuthorizationResult(
        AuthorizationResult::UNAUTHORIZED,
        L"The supplied credentials are not authorized.");
  }

  std::optional<std::wstring> globalPlainTextPassword_;
};

class ReferenceHlaAuthorizerFactory final : public rti1516_2025::AuthorizerFactory {
 public:
  explicit ReferenceHlaAuthorizerFactory(ReferenceAuthorizerConfiguration configuration)
      : configuration_(std::move(configuration)) {}

  std::unique_ptr<rti1516_2025::Authorizer> getAuthorizer() override {
    return std::make_unique<ReferenceHlaAuthorizer>(
        configuration_.globalPlainTextPassword);
  }

  std::wstring getName() const override {
    return rti1516_2025::HLAauthorizerName;
  }

 private:
  ReferenceAuthorizerConfiguration configuration_;
};

[[nodiscard]] std::unique_ptr<rti1516_2025::AuthorizerFactory>
makeReferenceAuthorizerFactory(ReferenceAuthorizerConfiguration configuration) {
  return std::make_unique<ReferenceHlaAuthorizerFactory>(std::move(configuration));
}

}  // namespace

std::unique_ptr<rti1516_2025::Authorizer> makeReferenceAuthorizer(
    ReferenceAuthorizerConfiguration configuration) {
  return std::make_unique<ReferenceHlaAuthorizer>(
      std::move(configuration.globalPlainTextPassword));
}

std::unique_ptr<rti1516_2025::AuthorizerFactory>
makeUnconfiguredReferenceAuthorizerFactory() {
  return makeReferenceAuthorizerFactory(ReferenceAuthorizerConfiguration{});
}

}  // namespace umbra::detail

namespace rti1516_2025 {
namespace {

[[nodiscard]] VariableLengthData encodePlainTextPassword(
    std::wstring const& password) {
  // IEEE 1516.1-2025 §12.6 deliberately reuses the standard
  // HLAunicodeString representation here.  In particular, this is a UTF-16BE
  // code-unit count, not an Umbra-specific UTF-8 or wchar_t byte sequence.
  return HLAunicodeString(password).encode();
}

}  // namespace

HLAplainTextPassword::HLAplainTextPassword(std::wstring const& password)
    : Credentials(HLAplainTextPasswordType, encodePlainTextPassword(password)) {}

HLAplainTextPassword::HLAplainTextPassword(
    VariableLengthData const& encodedPassword)
    : Credentials(HLAplainTextPasswordType, encodedPassword) {}

HLAplainTextPassword::~HLAplainTextPassword() noexcept = default;

HLAplainTextPassword::HLAplainTextPassword(HLAplainTextPassword const& rhs)
    : Credentials(rhs) {}

HLAplainTextPassword& HLAplainTextPassword::operator=(
    HLAplainTextPassword const& rhs) {
  Credentials::operator=(rhs);
  return *this;
}

std::wstring HLAplainTextPassword::decode() {
  HLAunicodeString decoded;
  decoded.decode(getData());
  return decoded.get();
}

VariableLengthData HLAplainTextPassword::encode(std::wstring const& basicString) {
  return encodePlainTextPassword(basicString);
}

std::unique_ptr<AuthorizerFactory> HLAauthorizerFactoryFactory::getAuthorizerFactory(
    std::wstring const& authorizerName) {
  if (authorizerName != HLAauthorizerName) {
    return {};
  }
  return umbra::detail::makeUnconfiguredReferenceAuthorizerFactory();
}

}  // namespace rti1516_2025
