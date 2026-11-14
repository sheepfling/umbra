#include "internal/runtime/authorizer_factory_provider.hpp"

#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace {

using rti1516_2025::AuthorizationResult;
using rti1516_2025::Authorizer;
using rti1516_2025::AuthorizerFactory;
using rti1516_2025::Credentials;

constexpr wchar_t kServiceName[] = L"TckFederationGate";
constexpr wchar_t kCreateCredentialType[] = L"TckFederationCreateCredential";
constexpr std::array<std::uint8_t, 6> kCreateCredentialBytes{
    0x43U, 0x52U, 0x45U, 0x41U, 0x54U, 0x45U};
constexpr wchar_t kDestroyServiceName[] = L"TckDestroyGate";
constexpr wchar_t kDestroyCredentialType[] = L"TckDestroyCredential";
constexpr wchar_t kJoinServiceName[] = L"TckJoinGate";
constexpr wchar_t kJoinCredentialType[] = L"TckJoinCredential";
constexpr wchar_t kNoCredentialsServiceName[] = L"TckNoCredentialsGate";
constexpr wchar_t kRidDenyServiceName[] = L"TckRidDenyGate";
constexpr wchar_t kRidAllowServiceName[] = L"TckRidAllowGate";
constexpr wchar_t kRidAlternateAllowServiceName[] = L"TckRidAlternateAllowGate";
constexpr std::array<std::uint8_t, 6> kDestroyCredentialBytes{
    0x44U, 0x45U, 0x53U, 0x54U, 0x52U, 0x59U};
constexpr wchar_t kDeniedFederation[] =
    L"tck-authorizer-denied-federation";

bool hasTckFederationCreateCredential(Credentials const& credentials) {
  auto const& data = credentials.getData();
  auto const* bytes = static_cast<std::uint8_t const*>(data.data());
  return credentials.getType() == kCreateCredentialType &&
         data.size() == kCreateCredentialBytes.size() && bytes != nullptr &&
         std::equal(
             kCreateCredentialBytes.begin(),
             kCreateCredentialBytes.end(),
             bytes);
}

class TckFederationGate final : public Authorizer {
 public:
  AuthorizationResult authorizeRtiOperation(
      Credentials const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK Connect allowed");
  }

  AuthorizationResult authorizeFederationOperation(
      Credentials const& credentials,
      std::wstring const& federationName) override {
    if (federationName == kDeniedFederation) {
      return AuthorizationResult(AuthorizationResult::UNAUTHORIZED,
                                 L"TCK federation operation denied");
    }
    if (!hasTckFederationCreateCredential(credentials)) {
      return AuthorizationResult(
          AuthorizationResult::UNAUTHORIZED,
          L"TCK federation creation credentials are missing or invalid");
    }
    return AuthorizationResult(AuthorizationResult::AUTHORIZED,
                               L"TCK federation operation allowed");
  }

  AuthorizationResult authorizeFederateOperation(
      Credentials const&,
      std::wstring const&,
      std::wstring const&,
      std::wstring const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK Join allowed");
  }

  std::wstring getName() const override { return kServiceName; }
};

class TckFederationGateFactory final : public AuthorizerFactory {
 public:
  std::unique_ptr<Authorizer> getAuthorizer() override {
    return std::make_unique<TckFederationGate>();
  }

  std::wstring getName() const override { return kServiceName; }
};

class TckDestroyGate final : public Authorizer {
 public:
  AuthorizationResult authorizeRtiOperation(
      Credentials const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK Connect allowed");
  }

  AuthorizationResult authorizeFederationOperation(
      Credentials const& credentials,
      std::wstring const&) override {
    auto const& data = credentials.getData();
    auto const* bytes = static_cast<std::uint8_t const*>(data.data());
    bool const credentialPermitsDestroy =
        credentials.getType() == kDestroyCredentialType &&
        data.size() == kDestroyCredentialBytes.size() && bytes != nullptr &&
        std::equal(
            kDestroyCredentialBytes.begin(),
            kDestroyCredentialBytes.end(),
            bytes);
    if (!credentialPermitsDestroy) {
      return AuthorizationResult(AuthorizationResult::UNAUTHORIZED,
                                 L"TCK Destroy credentials are missing or invalid");
    }
    return AuthorizationResult(AuthorizationResult::AUTHORIZED,
                               L"TCK Destroy credentials accepted");
  }

  AuthorizationResult authorizeFederateOperation(
      Credentials const&,
      std::wstring const&,
      std::wstring const&,
      std::wstring const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK Join allowed");
  }

  std::wstring getName() const override { return kDestroyServiceName; }

};

class TckDestroyGateFactory final : public AuthorizerFactory {
 public:
  std::unique_ptr<Authorizer> getAuthorizer() override {
    return std::make_unique<TckDestroyGate>();
  }

  std::wstring getName() const override { return kDestroyServiceName; }
};

class TckJoinGate final : public Authorizer {
 public:
  AuthorizationResult authorizeRtiOperation(
      Credentials const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK Connect allowed");
  }

  AuthorizationResult authorizeFederationOperation(
      Credentials const&,
      std::wstring const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK federation operation allowed");
  }

  AuthorizationResult authorizeFederateOperation(
      Credentials const& credentials,
      std::wstring const& federationName,
      std::wstring const& federateName,
      std::wstring const& federateType) override {
    bool permitted = false;
    auto const& credentialData = credentials.getData();
    auto const byteCount = credentialData.size();
    if (credentials.getType() == kJoinCredentialType && byteCount > 0U &&
        byteCount % sizeof(wchar_t) == 0U && credentialData.data() != nullptr) {
      std::wstring authorizedScope(byteCount / sizeof(wchar_t), L'\0');
      std::memcpy(authorizedScope.data(), credentialData.data(), byteCount);
      auto const firstSeparator = authorizedScope.find(L'\n');
      if (firstSeparator != std::wstring::npos) {
        auto const secondSeparator =
            authorizedScope.find(L'\n', firstSeparator + 1U);
        if (secondSeparator != std::wstring::npos &&
            authorizedScope.find(L'\n', secondSeparator + 1U) ==
                std::wstring::npos) {
          permitted =
              authorizedScope.substr(0U, firstSeparator) == federationName &&
              authorizedScope.substr(
                  firstSeparator + 1U,
                  secondSeparator - firstSeparator - 1U) == federateName &&
              authorizedScope.substr(secondSeparator + 1U) == federateType;
        }
      }
    }
    return AuthorizationResult(
        permitted ? AuthorizationResult::AUTHORIZED
                  : AuthorizationResult::UNAUTHORIZED,
        permitted ? L"TCK Join credentials permit the requested scope"
                  : L"TCK Join credentials do not permit the requested scope");
  }

  std::wstring getName() const override { return kJoinServiceName; }

};

class TckJoinGateFactory final : public AuthorizerFactory {
 public:
  std::unique_ptr<Authorizer> getAuthorizer() override {
    return std::make_unique<TckJoinGate>();
  }

  std::wstring getName() const override { return kJoinServiceName; }
};

class TckNoCredentialsGate final : public Authorizer {
 public:
  AuthorizationResult authorizeRtiOperation(
      Credentials const& credentials) override {
    if (credentials.getType() == rti1516_2025::HLAnoCredentialsType &&
        credentials.getData().size() == 0U) {
      return AuthorizationResult(
          AuthorizationResult::AUTHORIZED,
          L"TCK received HLAnoCredentials with empty data");
    }
    return AuthorizationResult(
        AuthorizationResult::UNAUTHORIZED,
        L"TCK expected HLAnoCredentials with empty data");
  }

  AuthorizationResult authorizeFederationOperation(
      Credentials const&,
      std::wstring const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK federation operation allowed");
  }

  AuthorizationResult authorizeFederateOperation(
      Credentials const&,
      std::wstring const&,
      std::wstring const&,
      std::wstring const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK Join allowed");
  }

  std::wstring getName() const override { return kNoCredentialsServiceName; }
};

class TckNoCredentialsGateFactory final : public AuthorizerFactory {
 public:
  std::unique_ptr<Authorizer> getAuthorizer() override {
    return std::make_unique<TckNoCredentialsGate>();
  }

  std::wstring getName() const override { return kNoCredentialsServiceName; }
};

template <bool AllowConnect, bool AlternateName = false>
class TckRidSelectionGate final : public Authorizer {
 public:
  AuthorizationResult authorizeRtiOperation(
      Credentials const& credentials) override {
    if (credentials.getType() != rti1516_2025::HLAnoCredentialsType ||
        credentials.getData().size() != 0U) {
      return AuthorizationResult(
          AuthorizationResult::UNAUTHORIZED,
          L"TCK RID selection gate expected empty HLAnoCredentials");
    }
    return AuthorizationResult(
        AllowConnect ? AuthorizationResult::AUTHORIZED
                     : AuthorizationResult::UNAUTHORIZED,
        AllowConnect ? L"TCK RID-selected allowing service"
                     : L"TCK RID-selected denying service");
  }

  AuthorizationResult authorizeFederationOperation(
      Credentials const&,
      std::wstring const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK federation operation allowed");
  }

  AuthorizationResult authorizeFederateOperation(
      Credentials const&,
      std::wstring const&,
      std::wstring const&,
      std::wstring const&) override {
    return AuthorizationResult(
        AuthorizationResult::AUTHORIZED, L"TCK Join allowed");
  }

  std::wstring getName() const override {
    if constexpr (AlternateName) {
      return kRidAlternateAllowServiceName;
    }
    return AllowConnect ? kRidAllowServiceName : kRidDenyServiceName;
  }
};

template <bool AllowConnect, bool AlternateName = false>
class TckRidSelectionGateFactory final : public AuthorizerFactory {
 public:
  std::unique_ptr<Authorizer> getAuthorizer() override {
    return std::make_unique<
        TckRidSelectionGate<AllowConnect, AlternateName>>();
  }

  std::wstring getName() const override {
    if constexpr (AlternateName) {
      return kRidAlternateAllowServiceName;
    }
    return AllowConnect ? kRidAllowServiceName : kRidDenyServiceName;
  }
};

}  // namespace

extern "C" UMBRA_AUTHORIZER_PROVIDER_EXPORT AuthorizerFactory*
umbra_authorizer_factory_v1(wchar_t const* serviceName) {
  if (serviceName == nullptr) {
    return nullptr;
  }
  auto const selectedName = std::wstring(serviceName);
  if (selectedName == kServiceName) {
    return new TckFederationGateFactory();
  }
  if (selectedName == kDestroyServiceName) {
    return new TckDestroyGateFactory();
  }
  if (selectedName == kJoinServiceName) {
    return new TckJoinGateFactory();
  }
  if (selectedName == kNoCredentialsServiceName) {
    return new TckNoCredentialsGateFactory();
  }
  if (selectedName == kRidDenyServiceName) {
    return new TckRidSelectionGateFactory<false>();
  }
  if (selectedName == kRidAllowServiceName) {
    return new TckRidSelectionGateFactory<true>();
  }
  if (selectedName == kRidAlternateAllowServiceName) {
    return new TckRidSelectionGateFactory<true, true>();
  }
  return nullptr;
}
