#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#include <aclapi.h>
#else
#include <cerrno>
#include <cstdlib>
#endif

#include "internal/runtime/reference_authorizer.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

namespace {

using rti1516_2025::EncoderException;
using rti1516_2025::AuthorizationResult;
using rti1516_2025::Authorizer;
using rti1516_2025::AuthorizerFactory;
using rti1516_2025::Credentials;
using rti1516_2025::HLAauthorizerFactoryFactory;
using rti1516_2025::HLAauthorizerName;
using rti1516_2025::HLAnoCredentials;
using rti1516_2025::HLAnoCredentialsType;
using rti1516_2025::HLAplainTextPassword;
using rti1516_2025::HLAplainTextPasswordType;
using rti1516_2025::VariableLengthData;

#if defined(_WIN32)
[[nodiscard]] bool restrictRidTestDirectoryAcl(
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

template <std::size_t Size>
bool encodedEquals(
    VariableLengthData const& actual,
    std::array<unsigned char, Size> const& expected) {
  return actual.size() == expected.size() &&
         std::memcmp(actual.data(), expected.data(), expected.size()) == 0;
}

class TemporaryRidFiles final {
 public:
  TemporaryRidFiles() {
    auto const base = std::filesystem::temp_directory_path();
    auto const stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned int attempt = 0U; attempt < 32U; ++attempt) {
      auto candidate = base /
          ("umbra-rid-authorization-" + std::to_string(stamp) + "-" +
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

struct FederationAuthorizationProbe final {
  std::vector<std::wstring> federationNames;
  std::vector<std::wstring> credentialTypes;
  std::vector<std::vector<unsigned char>> credentialPayloads;
  std::vector<AuthorizationResult::Code> results{
      AuthorizationResult::UNAUTHORIZED,
      AuthorizationResult::INVALID_CREDENTIALS};
  std::vector<std::wstring> federateFederationNames;
  std::vector<std::wstring> federateNames;
  std::vector<std::wstring> federateTypes;
  std::vector<std::wstring> federateCredentialTypes;
  std::vector<std::vector<unsigned char>> federateCredentialPayloads;
  std::vector<AuthorizationResult::Code> federateResults{
      AuthorizationResult::UNAUTHORIZED,
      AuthorizationResult::AUTHORIZED};
};

class RecordingFederationAuthorizer final : public Authorizer {
 public:
  explicit RecordingFederationAuthorizer(
      std::shared_ptr<FederationAuthorizationProbe> probe)
      : probe_(std::move(probe)) {}

  AuthorizationResult authorizeRtiOperation(
      Credentials const&) override {
    return AuthorizationResult(AuthorizationResult::AUTHORIZED);
  }

  AuthorizationResult authorizeFederationOperation(
      Credentials const& credentials,
      std::wstring const& federationName) override {
    auto const callIndex = probe_->federationNames.size();
    auto const resultCode = probe_->results.at(callIndex);
    probe_->federationNames.push_back(federationName);
    probe_->credentialTypes.push_back(credentials.getType());
    auto const data = credentials.getData();
    auto const* first = static_cast<unsigned char const*>(data.data());
    std::vector<unsigned char> payload;
    if (data.size() != 0U) {
      payload.assign(first, first + data.size());
    }
    probe_->credentialPayloads.push_back(std::move(payload));
    return AuthorizationResult(
        resultCode,
        L"Federation operation denied by the test authorizer.");
  }

  AuthorizationResult authorizeFederateOperation(
      Credentials const& credentials,
      std::wstring const& federationName,
      std::wstring const& federateName,
      std::wstring const& federateType) override {
    auto const callIndex = probe_->federateNames.size();
    auto const resultCode = probe_->federateResults.at(callIndex);
    probe_->federateFederationNames.push_back(federationName);
    probe_->federateNames.push_back(federateName);
    probe_->federateTypes.push_back(federateType);
    probe_->federateCredentialTypes.push_back(credentials.getType());
    auto const data = credentials.getData();
    auto const* first = static_cast<unsigned char const*>(data.data());
    std::vector<unsigned char> payload;
    if (data.size() != 0U) {
      payload.assign(first, first + data.size());
    }
    probe_->federateCredentialPayloads.push_back(std::move(payload));
    return AuthorizationResult(
        resultCode,
        L"Federate operation denied by the test authorizer.");
  }

  std::wstring getName() const override {
    return L"RecordingFederationAuthorizer";
  }

 private:
  std::shared_ptr<FederationAuthorizationProbe> probe_;
};

class RecordingFederationAuthorizerFactory final : public AuthorizerFactory {
 public:
  explicit RecordingFederationAuthorizerFactory(
      std::shared_ptr<FederationAuthorizationProbe> probe)
      : probe_(std::move(probe)) {}

  std::unique_ptr<Authorizer> getAuthorizer() override {
    return std::make_unique<RecordingFederationAuthorizer>(probe_);
  }

  std::wstring getName() const override {
    return L"RecordingFederationAuthorizer";
  }

 private:
  std::shared_ptr<FederationAuthorizationProbe> probe_;
};

class ScopedTestFederationExecution final {
 public:
  ScopedTestFederationExecution(
      rti1516_2025::RTIambassador& ambassador,
      std::wstring federationName)
      : ambassador_(ambassador), federationName_(std::move(federationName)) {}

  ~ScopedTestFederationExecution() {
    if (active_) {
      try {
        ambassador_.destroyFederationExecution(federationName_);
      } catch (...) {
      }
    }
  }

  void destroyNow() {
    if (active_) {
      ambassador_.destroyFederationExecution(federationName_);
      active_ = false;
    }
  }

 private:
  rti1516_2025::RTIambassador& ambassador_;
  std::wstring federationName_;
  bool active_ = true;
};

}  // namespace

TEST_CASE(
    "HLAplainTextPassword stores the standard HLAunicodeString credential payload",
    "[baseline][authorization][credentials][unit][foundation][plain-text-password-hlaunicode-payload]") {
  HLAplainTextPassword password(L"p\u00E4ss");
  std::array<unsigned char, 12> const expected{
      0x00, 0x00, 0x00, 0x04,
      0x00, 0x70, 0x00, 0xE4,
      0x00, 0x73, 0x00, 0x73};

  REQUIRE(password.getType() == HLAplainTextPasswordType);
  REQUIRE(encodedEquals(password.getData(), expected));
  REQUIRE(password.decode() == L"p\u00E4ss");

  VariableLengthData encoded(expected.data(), expected.size());
  HLAplainTextPassword fromEncoded(encoded);
  REQUIRE(fromEncoded.getType() == HLAplainTextPasswordType);
  REQUIRE(encodedEquals(fromEncoded.getData(), expected));
  REQUIRE(fromEncoded.decode() == L"p\u00E4ss");

  HLAplainTextPassword copied(password);
  HLAplainTextPassword assigned(L"unused");
  assigned = password;
  password = HLAplainTextPassword(L"changed");
  REQUIRE(copied.decode() == L"p\u00E4ss");
  REQUIRE(assigned.decode() == L"p\u00E4ss");
  REQUIRE(password.decode() == L"changed");
}

TEST_CASE(
    "HLAplainTextPassword rejects malformed HLAunicodeString credential payloads",
    "[baseline][authorization][credentials][unit][foundation][plain-text-password-malformed-payload]") {
  std::array<unsigned char, 3> const tooShort{0x00, 0x00, 0x00};
  std::array<unsigned char, 6> const truncated{0x00, 0x00, 0x00, 0x02, 0x00, 0x70};
  std::array<unsigned char, 7> const trailing{0x00, 0x00, 0x00, 0x01, 0x00, 0x70, 0x5A};
  std::array<unsigned char, 6> const loneSurrogate{0x00, 0x00, 0x00, 0x01, 0xD8, 0x00};

  REQUIRE_THROWS_AS(
      HLAplainTextPassword(VariableLengthData(tooShort.data(), tooShort.size())).decode(),
      EncoderException);
  REQUIRE_THROWS_AS(
      HLAplainTextPassword(VariableLengthData(truncated.data(), truncated.size())).decode(),
      EncoderException);
  REQUIRE_THROWS_AS(
      HLAplainTextPassword(VariableLengthData(trailing.data(), trailing.size())).decode(),
      EncoderException);
  REQUIRE_THROWS_AS(
      HLAplainTextPassword(VariableLengthData(loneSurrogate.data(), loneSurrogate.size())).decode(),
      EncoderException);
}

TEST_CASE(
    "HLAauthorizer factories select and forward the standard reference service",
    "[baseline][authorization][authorizer-factory][unit][foundation][standard-authorizer-factory-factory]") {
  auto factory = HLAauthorizerFactoryFactory::getAuthorizerFactory(HLAauthorizerName);
  REQUIRE(factory);
  REQUIRE(factory->getName() == HLAauthorizerName);
  REQUIRE_FALSE(HLAauthorizerFactoryFactory::getAuthorizerFactory(L"unknown"));

  auto forwarded = rti1516_2025::AuthorizerFactoryFactory::getAuthorizerFactory(
      HLAauthorizerName);
  REQUIRE(forwarded);
  REQUIRE(forwarded->getName() == HLAauthorizerName);
  REQUIRE_FALSE(
      rti1516_2025::AuthorizerFactoryFactory::getAuthorizerFactory(L"unknown"));

  auto unconfigured = factory->getAuthorizer();
  REQUIRE(unconfigured);
  REQUIRE(unconfigured->getName() == HLAauthorizerName);
  HLAnoCredentials noCredentials;
  REQUIRE(
      unconfigured->authorizeRtiOperation(noCredentials).getCode() ==
      AuthorizationResult::AUTHORIZATION_ERROR);
}

TEST_CASE(
    "Configured reference HLAauthorizer recognizes only valid matching plaintext credentials",
    "[baseline][authorization][authorizer][unit][foundation][reference-authorizer-global-password]") {
  umbra::detail::ReferenceAuthorizerConfiguration configuration;
  configuration.globalPlainTextPassword = L"test-password";
  auto authorizer = umbra::detail::makeReferenceAuthorizer(std::move(configuration));
  REQUIRE(authorizer);
  REQUIRE(authorizer->getName() == HLAauthorizerName);

  HLAplainTextPassword matching(L"test-password");
  HLAplainTextPassword wrong(L"wrong-password");
  HLAnoCredentials noCredentials;
  Credentials unknown(L"UmbraUnknownCredential", VariableLengthData{});
  std::array<unsigned char, 3> const malformedBytes{0x00, 0x00, 0x00};
  Credentials malformed(
      HLAplainTextPasswordType,
      VariableLengthData(malformedBytes.data(), malformedBytes.size()));
  std::array<unsigned char, 1> const nonemptyNoCredentialsBytes{0x01};
  Credentials malformedNoCredentials(
      HLAnoCredentialsType,
      VariableLengthData(
          nonemptyNoCredentialsBytes.data(), nonemptyNoCredentialsBytes.size()));

  REQUIRE(
      authorizer->authorizeRtiOperation(matching).getCode() ==
      AuthorizationResult::AUTHORIZED);
  REQUIRE(
      authorizer->authorizeFederationOperation(matching, L"federation").getCode() ==
      AuthorizationResult::AUTHORIZED);
  REQUIRE(
      authorizer
          ->authorizeFederateOperation(matching, L"federation", L"federate", L"type")
          .getCode() == AuthorizationResult::AUTHORIZED);
  REQUIRE(
      authorizer->authorizeRtiOperation(wrong).getCode() ==
      AuthorizationResult::UNAUTHORIZED);
  REQUIRE(
      authorizer->authorizeRtiOperation(noCredentials).getCode() ==
      AuthorizationResult::UNAUTHORIZED);
  REQUIRE(
      authorizer->authorizeRtiOperation(unknown).getCode() ==
      AuthorizationResult::UNAUTHORIZED);
  REQUIRE(
      authorizer->authorizeRtiOperation(malformed).getCode() ==
      AuthorizationResult::INVALID_CREDENTIALS);
  REQUIRE(
      authorizer->authorizeRtiOperation(malformedNoCredentials).getCode() ==
      AuthorizationResult::INVALID_CREDENTIALS);
}

TEST_CASE(
    "Embedded Connect delegates credentials to the configured HLAauthorizer factory",
    "[baseline][federation-management][authorization][credentials][integration][connect-authorizer-factory-path]") {
  umbra::detail::ReferenceAuthorizerConfiguration configuration;
  configuration.globalPlainTextPassword = L"test-password";
  auto factory = umbra::detail::makeReferenceAuthorizerFactory(
      std::move(configuration));
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador rti(
      rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador::
          AuthorizerFactoryTestSeam{},
      std::move(factory));
  rti1516_2025::NullFederateAmbassador federate;

  HLAplainTextPassword wrong(L"wrong-password");
  REQUIRE_THROWS_AS(
      rti.connect(federate, rti1516_2025::HLA_EVOKED, wrong),
      rti1516_2025::Unauthorized);

  std::array<unsigned char, 3> const malformedBytes{0x00, 0x00, 0x00};
  Credentials malformed(
      HLAplainTextPasswordType,
      VariableLengthData(malformedBytes.data(), malformedBytes.size()));
  REQUIRE_THROWS_AS(
      rti.connect(federate, rti1516_2025::HLA_EVOKED, malformed),
      rti1516_2025::InvalidCredentials);

  REQUIRE_THROWS_AS(
      rti.connect(federate, rti1516_2025::HLA_EVOKED),
      rti1516_2025::Unauthorized);

  HLAplainTextPassword matching(L"test-password");
  auto connectionConfiguration =
      rti1516_2025::RtiConfiguration::createConfiguration();
  REQUIRE_NOTHROW(rti.connect(
      federate,
      rti1516_2025::HLA_EVOKED,
      connectionConfiguration,
      matching));
  REQUIRE_NOTHROW(rti.disconnect());
}

TEST_CASE(
    "Factory-created RTIambassadors load the configured HLAauthorizer from the RID",
    "[baseline][federation-management][authorization][credentials][integration][connect-authorizer-rid-runtime]") {
  TemporaryRidFiles files;
  static_cast<void>(
      files.writePrivateFile("global-password.txt", "test-password"));
  auto const ridFile = files.writePrivateFile(
      "umbra.rid",
      "[authorization.default]\n"
      "service=HLAauthorizer\n"
      "globalPasswordFile=global-password.txt\n"
      "[authorization.named]\n"
      "service=HLAauthorizer\n"
      "globalPasswordFile=global-password.txt\n");
  ScopedRidFileEnvironment ridEnvironment(ridFile);

  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  rti1516_2025::NullFederateAmbassador federate;

  HLAplainTextPassword wrong(L"wrong-password");
  REQUIRE_THROWS_AS(
      rti->connect(federate, rti1516_2025::HLA_EVOKED, wrong),
      rti1516_2025::Unauthorized);
  REQUIRE_THROWS_AS(
      rti->connect(federate, rti1516_2025::HLA_EVOKED),
      rti1516_2025::Unauthorized);
  std::array<unsigned char, 3> const malformedBytes{0x00, 0x00, 0x00};
  Credentials malformed(
      HLAplainTextPasswordType,
      VariableLengthData(malformedBytes.data(), malformedBytes.size()));
  REQUIRE_THROWS_AS(
      rti->connect(federate, rti1516_2025::HLA_EVOKED, malformed),
      rti1516_2025::InvalidCredentials);

  auto unknownConfiguration =
      rti1516_2025::RtiConfiguration::createConfiguration();
  unknownConfiguration.withConfigurationName(L"missing-profile");
  HLAplainTextPassword matching(L"test-password");
  REQUIRE_THROWS_AS(
      rti->connect(
          federate,
          rti1516_2025::HLA_EVOKED,
          unknownConfiguration,
          matching),
      rti1516_2025::RTIinternalError);

  auto namedConfiguration = rti1516_2025::RtiConfiguration::createConfiguration();
  namedConfiguration.withConfigurationName(L"named");
  REQUIRE_NOTHROW(rti->connect(
      federate,
      rti1516_2025::HLA_EVOKED,
      namedConfiguration,
      matching));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "RTIambassador applies the configured HLAauthorizer to Create Federation Execution",
    "[baseline][federation-management][authorization][credentials][integration][create-federation-authorizer-rid-runtime]") {
  auto probe = std::make_shared<FederationAuthorizationProbe>();
  auto authorizerFactory =
      std::make_unique<RecordingFederationAuthorizerFactory>(probe);
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador rti(
      rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador::
          AuthorizerFactoryTestSeam{},
      std::move(authorizerFactory));
  rti1516_2025::NullFederateAmbassador federate;
  HLAplainTextPassword credentials(L"create-token");

  REQUIRE_NOTHROW(rti.connect(
      federate,
      rti1516_2025::HLA_EVOKED,
      credentials));
  REQUIRE_THROWS_AS(
      rti.createFederationExecution(
          L"denied-federation",
          std::vector<std::wstring>{L"missing-create-test-fom.xml"},
          L"HLAinteger64Time"),
      rti1516_2025::Unauthorized);
  REQUIRE_THROWS_AS(
      rti.createFederationExecutionWithMIM(
          L"denied-federation-with-mim",
          std::vector<std::wstring>{L"missing-create-test-fom.xml"},
          L"missing-create-test-mim.xml",
          L"HLAinteger64Time"),
      rti1516_2025::Unauthorized);

  REQUIRE((probe->federationNames == std::vector<std::wstring>{
      L"denied-federation", L"denied-federation-with-mim"}));
  REQUIRE((probe->credentialTypes == std::vector<std::wstring>{
      HLAplainTextPasswordType, HLAplainTextPasswordType}));
  REQUIRE(probe->credentialPayloads.size() == 2U);
  for (auto const& payload : probe->credentialPayloads) {
    HLAplainTextPassword observed(VariableLengthData(payload.data(), payload.size()));
    REQUIRE(observed.decode() == L"create-token");
  }
  REQUIRE_NOTHROW(rti.disconnect());
}

TEST_CASE(
    "RTIambassador applies the configured HLAauthorizer to Destroy Federation Execution",
    "[baseline][federation-management][authorization][credentials][integration][destroy-federation-authorizer-rid-runtime]") {
  auto const stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  std::wstring const federationName =
      L"destroy-authorizer-" + std::to_wstring(stamp);
  std::wstring const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();

  umbra::detail::ReferenceAuthorizerConfiguration creatorConfiguration;
  creatorConfiguration.globalPlainTextPassword = L"creator-token";
  auto creatorFactory = umbra::detail::makeReferenceAuthorizerFactory(
      std::move(creatorConfiguration));
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador creator(
      rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador::
          AuthorizerFactoryTestSeam{},
      std::move(creatorFactory));
  rti1516_2025::NullFederateAmbassador creatorFederate;
  HLAplainTextPassword creatorCredentials(L"creator-token");
  REQUIRE_NOTHROW(creator.connect(
      creatorFederate,
      rti1516_2025::HLA_EVOKED,
      creatorCredentials));
  REQUIRE_NOTHROW(creator.createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  ScopedTestFederationExecution federationCleanup(creator, federationName);

  auto probe = std::make_shared<FederationAuthorizationProbe>();
  probe->results = {
      AuthorizationResult::UNAUTHORIZED,
      AuthorizationResult::AUTHORIZED};
  auto authorizerFactory =
      std::make_unique<RecordingFederationAuthorizerFactory>(probe);
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador destroyer(
      rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador::
          AuthorizerFactoryTestSeam{},
      std::move(authorizerFactory));
  rti1516_2025::NullFederateAmbassador destroyerFederate;
  HLAplainTextPassword destroyerCredentials(L"destroy-token");
  REQUIRE_NOTHROW(destroyer.connect(
      destroyerFederate,
      rti1516_2025::HLA_EVOKED,
      destroyerCredentials));

  REQUIRE_THROWS_AS(
      destroyer.destroyFederationExecution(federationName),
      rti1516_2025::Unauthorized);
  REQUIRE((probe->federationNames == std::vector<std::wstring>{
      federationName}));
  REQUIRE((probe->credentialTypes == std::vector<std::wstring>{
      HLAplainTextPasswordType}));
  REQUIRE(probe->credentialPayloads.size() == 1U);
  for (auto const& payload : probe->credentialPayloads) {
    HLAplainTextPassword observed(VariableLengthData(payload.data(), payload.size()));
    REQUIRE(observed.decode() == L"destroy-token");
  }

  REQUIRE_THROWS_AS(
      creator.createFederationExecution(
          federationName,
          fomModule,
          L"HLAinteger64Time"),
      rti1516_2025::FederationExecutionAlreadyExists);

  REQUIRE_NOTHROW(destroyer.destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(creator.createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(federationCleanup.destroyNow());
  REQUIRE_NOTHROW(destroyer.disconnect());
  REQUIRE_NOTHROW(creator.disconnect());
}

TEST_CASE(
    "RTIambassador applies the configured HLAauthorizer to Join Federation Execution",
    "[baseline][federation-management][authorization][credentials][integration][join-federation-authorizer-rid-runtime]") {
  auto const stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  std::wstring const federationName =
      L"join-authorizer-" + std::to_wstring(stamp);
  std::wstring const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();

  umbra::detail::ReferenceAuthorizerConfiguration creatorConfiguration;
  creatorConfiguration.globalPlainTextPassword = L"creator-token";
  auto creatorFactory = umbra::detail::makeReferenceAuthorizerFactory(
      std::move(creatorConfiguration));
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador creator(
      rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador::
          AuthorizerFactoryTestSeam{},
      std::move(creatorFactory));
  rti1516_2025::NullFederateAmbassador creatorFederate;
  HLAplainTextPassword creatorCredentials(L"creator-token");
  REQUIRE_NOTHROW(creator.connect(
      creatorFederate,
      rti1516_2025::HLA_EVOKED,
      creatorCredentials));
  REQUIRE_NOTHROW(creator.createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  ScopedTestFederationExecution federationCleanup(creator, federationName);

  auto probe = std::make_shared<FederationAuthorizationProbe>();
  probe->federateResults = {
      AuthorizationResult::UNAUTHORIZED,
      AuthorizationResult::AUTHORIZED,
      AuthorizationResult::INVALID_CREDENTIALS,
      AuthorizationResult::AUTHORIZED};
  auto authorizerFactory =
      std::make_unique<RecordingFederationAuthorizerFactory>(probe);
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador joiner(
      rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador::
          AuthorizerFactoryTestSeam{},
      std::move(authorizerFactory));
  rti1516_2025::NullFederateAmbassador joinerFederate;
  HLAplainTextPassword joinerCredentials(L"join-token");
  auto const reportDirectory = std::filesystem::temp_directory_path() /
      ("umbra-join-authorizer-report-" + std::to_string(stamp));
  auto joinerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration();
  joinerConfiguration.withAdditionalSettings(
      L"serviceReportDirectory=" + reportDirectory.wstring());
  REQUIRE_NOTHROW(joiner.connect(
      joinerFederate,
      rti1516_2025::HLA_EVOKED,
      joinerConfiguration,
      joinerCredentials));

  REQUIRE_THROWS_AS(
      joiner.joinFederationExecution(
          L"named-joiner", L"test-type", federationName),
      rti1516_2025::Unauthorized);
  auto const namedHandle = joiner.joinFederationExecution(
      L"named-joiner", L"test-type", federationName);
  REQUIRE(joiner.getFederateName(namedHandle) == L"named-joiner");
  REQUIRE_NOTHROW(joiner.resignFederationExecution(rti1516_2025::NO_ACTION));

  REQUIRE_THROWS_AS(
      joiner.joinFederationExecution(L"test-type", federationName),
      rti1516_2025::Unauthorized);
  auto const unnamedHandle =
      joiner.joinFederationExecution(L"test-type", federationName);
  REQUIRE_FALSE(joiner.getFederateName(unnamedHandle).empty());
  REQUIRE_NOTHROW(joiner.resignFederationExecution(rti1516_2025::NO_ACTION));

  REQUIRE((probe->federateFederationNames == std::vector<std::wstring>{
      federationName, federationName, federationName, federationName}));
  REQUIRE((probe->federateNames == std::vector<std::wstring>{
      L"named-joiner", L"named-joiner", L"", L""}));
  REQUIRE((probe->federateTypes == std::vector<std::wstring>{
      L"test-type", L"test-type", L"test-type", L"test-type"}));
  REQUIRE((probe->federateCredentialTypes == std::vector<std::wstring>{
      HLAplainTextPasswordType,
      HLAplainTextPasswordType,
      HLAplainTextPasswordType,
      HLAplainTextPasswordType}));
  REQUIRE(probe->federateCredentialPayloads.size() == 4U);
  for (auto const& payload : probe->federateCredentialPayloads) {
    HLAplainTextPassword observed(VariableLengthData(payload.data(), payload.size()));
    REQUIRE(observed.decode() == L"join-token");
  }

  REQUIRE_NOTHROW(joiner.disconnect());
  REQUIRE_NOTHROW(federationCleanup.destroyNow());
  REQUIRE_NOTHROW(creator.disconnect());
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}
