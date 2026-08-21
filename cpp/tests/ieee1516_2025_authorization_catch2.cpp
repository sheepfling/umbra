#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <RTI/RTI1516.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>

#include "internal/reference_authorizer.hpp"

namespace {

using rti1516_2025::EncoderException;
using rti1516_2025::AuthorizationResult;
using rti1516_2025::AuthorizerFactory;
using rti1516_2025::Credentials;
using rti1516_2025::HLAauthorizerFactoryFactory;
using rti1516_2025::HLAauthorizerName;
using rti1516_2025::HLAnoCredentials;
using rti1516_2025::HLAnoCredentialsType;
using rti1516_2025::HLAplainTextPassword;
using rti1516_2025::HLAplainTextPasswordType;
using rti1516_2025::VariableLengthData;

template <std::size_t Size>
bool encodedEquals(
    VariableLengthData const& actual,
    std::array<unsigned char, Size> const& expected) {
  return actual.size() == expected.size() &&
         std::memcmp(actual.data(), expected.data(), expected.size()) == 0;
}

}  // namespace

TEST_CASE(
    "HLAplainTextPassword stores the standard HLAunicodeString credential payload",
    "[baseline][authorization][credentials][unit][foundation]") {
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
    "[baseline][authorization][credentials][unit][foundation]") {
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
    "[baseline][authorization][authorizer-factory][unit][foundation]") {
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
    "[baseline][authorization][authorizer][unit][foundation]") {
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
