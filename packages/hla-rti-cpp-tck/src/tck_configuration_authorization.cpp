#include "tck_configuration_authorization.hpp"

#include <RTI/VariableLengthData.h>
#include <RTI/auth/Credentials.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>

namespace hla_rti_cpp_tck::detail {

static_assert(
    std::is_base_of_v<rti::Credentials, rti::HLAplainTextPassword>,
    "HLAplainTextPassword must derive from Credentials");

void verifyStandardConfigurationAndAuthorizationContract() {
  class PortableAuthorizer final : public rti::Authorizer {
  public:
    rti::AuthorizationResult authorizeRtiOperation(
        rti::Credentials const&) override {
      return rti::AuthorizationResult{
          rti::AuthorizationResult::AUTHORIZED,
          L"portable-rti"};
    }
    rti::AuthorizationResult authorizeFederationOperation(
        rti::Credentials const&,
        std::wstring const&) override {
      return rti::AuthorizationResult{
          rti::AuthorizationResult::AUTHORIZED,
          L"portable-federation"};
    }
    rti::AuthorizationResult authorizeFederateOperation(
        rti::Credentials const&,
        std::wstring const&,
        std::wstring const&,
        std::wstring const&) override {
      return rti::AuthorizationResult{
          rti::AuthorizationResult::AUTHORIZED,
          L"portable-federate"};
    }
    std::wstring getName() const override { return L"portable-authorizer"; }
  };

  class PortableAuthorizerFactory final : public rti::AuthorizerFactory {
  public:
    std::unique_ptr<rti::Authorizer> getAuthorizer() override {
      return std::make_unique<PortableAuthorizer>();
    }
    std::wstring getName() const override {
      return L"portable-authorizer-factory";
    }
  };

  rti::HLAnoCredentials authorizerCredentials;
  PortableAuthorizer authorizer;
  rti::Authorizer& authorizerInterface = authorizer;
  auto rtiAuthorization =
      authorizerInterface.authorizeRtiOperation(authorizerCredentials);
  auto federationAuthorization =
      authorizerInterface.authorizeFederationOperation(
          authorizerCredentials, L"portable-federation");
  auto federateAuthorization = authorizerInterface.authorizeFederateOperation(
      authorizerCredentials,
      L"portable-federation",
      L"portable-federate",
      L"portable-type");
  require(
      rtiAuthorization.getCode() == rti::AuthorizationResult::AUTHORIZED &&
          rtiAuthorization.getMessage() == L"portable-rti" &&
          federationAuthorization.getCode() ==
              rti::AuthorizationResult::AUTHORIZED &&
          federationAuthorization.getMessage() == L"portable-federation" &&
          federateAuthorization.getCode() ==
              rti::AuthorizationResult::AUTHORIZED &&
          federateAuthorization.getMessage() == L"portable-federate" &&
          authorizerInterface.getName() == L"portable-authorizer" &&
          rti::HLAauthorizerName == L"HLAauthorizer",
      "standard Authorizer dispatch or HLAauthorizer naming semantics were not preserved");

  PortableAuthorizerFactory authorizerFactory;
  rti::AuthorizerFactory& factoryInterface = authorizerFactory;
  auto createdAuthorizer = factoryInterface.getAuthorizer();
  require(
      createdAuthorizer != nullptr &&
          createdAuthorizer->getName() == L"portable-authorizer" &&
          factoryInterface.getName() == L"portable-authorizer-factory",
      "standard AuthorizerFactory creation or naming semantics were not preserved");

  auto configuration = rti::RtiConfiguration::createConfiguration();
  auto& configured = configuration
                         .withConfigurationName(L"portable-config")
                         .withRtiAddress(L"rti://portable")
                         .withAdditionalSettings(L"{}");
  require(
      &configured == &configuration &&
          configuration.configurationName() == L"portable-config" &&
          configuration.rtiAddress() == L"rti://portable" &&
          configuration.additionalSettings() == L"{}",
      "standard RtiConfiguration builder or accessor semantics were not preserved");
  auto copiedConfiguration = configuration;
  auto assignedConfiguration = rti::RtiConfiguration::createConfiguration();
  assignedConfiguration = configuration;
  copiedConfiguration
      .withConfigurationName(L"portable-copy")
      .withRtiAddress(L"rti://portable-copy")
      .withAdditionalSettings(L"{\"copy\":true}");
  assignedConfiguration.withConfigurationName(L"portable-assigned");
  require(
      configuration.configurationName() == L"portable-config" &&
          configuration.rtiAddress() == L"rti://portable" &&
          configuration.additionalSettings() == L"{}" &&
          copiedConfiguration.configurationName() == L"portable-copy" &&
          copiedConfiguration.rtiAddress() == L"rti://portable-copy" &&
          copiedConfiguration.additionalSettings() == L"{\"copy\":true}" &&
          assignedConfiguration.configurationName() == L"portable-assigned" &&
          assignedConfiguration.rtiAddress() == L"rti://portable" &&
          assignedConfiguration.additionalSettings() == L"{}",
      "standard RtiConfiguration copy, assignment, builder, or independence semantics were not preserved");

  rti::ConfigurationResult configurationResult{
      true,
      false,
      rti::SETTINGS_APPLIED,
      L"configured"};
  auto copiedConfigurationResult = configurationResult;
  rti::ConfigurationResult assignedConfigurationResult;
  assignedConfigurationResult = configurationResult;
  copiedConfigurationResult.message = L"copied";
  assignedConfigurationResult.configurationUsed = false;
  require(
      configurationResult.configurationUsed &&
          !configurationResult.addressUsed &&
          configurationResult.additionalSettingsResult ==
              rti::SETTINGS_APPLIED &&
          configurationResult.message == L"configured" &&
          copiedConfigurationResult.configurationUsed &&
          !copiedConfigurationResult.addressUsed &&
          copiedConfigurationResult.additionalSettingsResult ==
              rti::SETTINGS_APPLIED &&
          copiedConfigurationResult.message == L"copied" &&
          !assignedConfigurationResult.configurationUsed &&
          !assignedConfigurationResult.addressUsed &&
          assignedConfigurationResult.additionalSettingsResult ==
              rti::SETTINGS_APPLIED &&
          assignedConfigurationResult.message == L"configured",
      "standard ConfigurationResult copy, assignment, and independence semantics were not preserved");

  rti::FederationExecutionInformation executionInformation{
      L"portable-federation",
      L"HLAinteger64Time"};
  rti::FederationExecutionMemberInformation memberInformation{
      L"portable-federate",
      L"portable-type"};
  rti::FederateRestoreStatus restoreStatus{
      rti::FederateHandle{},
      rti::FederateHandle{},
      rti::FEDERATE_RESTORING};
  auto copiedExecutionInformation = executionInformation;
  rti::FederationExecutionInformation assignedExecutionInformation;
  assignedExecutionInformation = executionInformation;
  copiedExecutionInformation.federationExecutionName = L"copied-federation";
  assignedExecutionInformation.logicalTimeImplementationName = L"assigned-time";
  auto copiedMemberInformation = memberInformation;
  rti::FederationExecutionMemberInformation assignedMemberInformation;
  assignedMemberInformation = memberInformation;
  copiedMemberInformation.federateName = L"copied-federate";
  assignedMemberInformation.federateType = L"assigned-type";
  auto copiedRestoreStatus = restoreStatus;
  rti::FederateRestoreStatus assignedRestoreStatus;
  assignedRestoreStatus = restoreStatus;
  copiedRestoreStatus.status = rti::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN;
  assignedRestoreStatus.status = rti::NO_RESTORE_IN_PROGRESS;
  require(
      executionInformation.federationExecutionName == L"portable-federation" &&
          executionInformation.logicalTimeImplementationName == L"HLAinteger64Time" &&
          memberInformation.federateName == L"portable-federate" &&
          memberInformation.federateType == L"portable-type" &&
          !restoreStatus.preRestoreHandle.isValid() &&
          !restoreStatus.postRestoreHandle.isValid() &&
          restoreStatus.status == rti::FEDERATE_RESTORING &&
          copiedExecutionInformation.federationExecutionName ==
              L"copied-federation" &&
          copiedExecutionInformation.logicalTimeImplementationName ==
              L"HLAinteger64Time" &&
          assignedExecutionInformation.federationExecutionName ==
              L"portable-federation" &&
          assignedExecutionInformation.logicalTimeImplementationName ==
              L"assigned-time" &&
          copiedMemberInformation.federateName == L"copied-federate" &&
          copiedMemberInformation.federateType == L"portable-type" &&
          assignedMemberInformation.federateName == L"portable-federate" &&
          assignedMemberInformation.federateType == L"assigned-type" &&
          copiedRestoreStatus.status ==
              rti::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN &&
          assignedRestoreStatus.status == rti::NO_RESTORE_IN_PROGRESS,
      "standard federation and restore information record copy, assignment, or independence semantics were not preserved");

  rti::HLAnoCredentials noCredentials;
  rti::HLAnoCredentials copiedNoCredentials(noCredentials);
  rti::HLAnoCredentials assignedNoCredentials;
  assignedNoCredentials = noCredentials;
  require(
      noCredentials.getType() == rti::HLAnoCredentialsType &&
          noCredentials.getData().size() == 0U &&
          copiedNoCredentials.getType() == rti::HLAnoCredentialsType &&
          copiedNoCredentials.getData().size() == 0U &&
          assignedNoCredentials.getType() == rti::HLAnoCredentialsType &&
          assignedNoCredentials.getData().size() == 0U,
      "standard HLAnoCredentials type, copy, or assignment semantics were not preserved");

  rti::HLAplainTextPassword password{L"portable-password"};
  rti::HLAplainTextPassword copiedPassword(password);
  rti::HLAplainTextPassword assignedPassword{L"temporary-password"};
  assignedPassword = password;
  rti::HLAplainTextPassword decodedPassword(password.getData());
  require(
      password.getType() == rti::HLAplainTextPasswordType &&
          password.getType() == L"HLAplainTextPassword" &&
          password.decode() == L"portable-password" &&
          copiedPassword.decode() == L"portable-password" &&
          assignedPassword.decode() == L"portable-password" &&
          decodedPassword.decode() == L"portable-password",
      "standard HLAplainTextPassword encoding, copy, or assignment semantics were not preserved");

  rti::HLAplainTextPassword unicodePassword{L"p\u00E4ss"};
  rti::HLAplainTextPassword decodedUnicodePassword{unicodePassword.getData()};
  rti::HLAunicodeString expectedUnicodePasswordValue{L"p\u00E4ss"};
  require(
      unicodePassword.decode() == L"p\u00E4ss" &&
          decodedUnicodePassword.decode() == L"p\u00E4ss" &&
          copyBytes(unicodePassword.getData()) ==
              copyBytes(expectedUnicodePasswordValue.encode()),
      "standard HLAplainTextPassword did not encode Unicode text as HLAunicodeString");

  std::vector<std::uint8_t> customCredentialBytes{0x43U, 0x52U, 0x45U};
  rti::Credentials customCredentials{
      L"portable-custom",
      rti::VariableLengthData(
          customCredentialBytes.data(), customCredentialBytes.size())};
  auto copiedCredentials = customCredentials;
  rti::Credentials assignedCredentials{
      L"temporary-custom",
      rti::VariableLengthData()};
  assignedCredentials = customCredentials;
  customCredentialBytes[0] = 0x99U;
  require(
      customCredentials.getType() == L"portable-custom" &&
          copyBytes(customCredentials.getData()) ==
              std::vector<std::uint8_t>{0x43U, 0x52U, 0x45U} &&
          copiedCredentials.getType() == L"portable-custom" &&
          copyBytes(copiedCredentials.getData()) ==
              std::vector<std::uint8_t>{0x43U, 0x52U, 0x45U} &&
          assignedCredentials.getType() == L"portable-custom" &&
          copyBytes(assignedCredentials.getData()) ==
              std::vector<std::uint8_t>{0x43U, 0x52U, 0x45U},
      "standard Credentials type, data, copy, assignment, or ownership semantics were not preserved");

  rti::AuthorizationResult authorizationResult{
      rti::AuthorizationResult::AUTHORIZED,
      L"accepted"};
  auto copiedAuthorizationResult = authorizationResult;
  rti::AuthorizationResult assignedAuthorizationResult{
      rti::AuthorizationResult::UNAUTHORIZED,
      L"temporary"};
  assignedAuthorizationResult = authorizationResult;
  require(
      authorizationResult.getCode() == rti::AuthorizationResult::AUTHORIZED &&
          authorizationResult.getMessage() == L"accepted" &&
          copiedAuthorizationResult.getCode() ==
              rti::AuthorizationResult::AUTHORIZED &&
          copiedAuthorizationResult.getMessage() == L"accepted" &&
          assignedAuthorizationResult.getCode() ==
              rti::AuthorizationResult::AUTHORIZED &&
          assignedAuthorizationResult.getMessage() == L"accepted",
      "standard AuthorizationResult copy or assignment semantics were not preserved");
}


void verifyStandardAuthorizerFactoryFactoryContract() {
  auto libraryFactory = rti::AuthorizerFactoryFactory::getAuthorizerFactory(
      rti::HLAauthorizerName);
  require(
      libraryFactory != nullptr &&
          libraryFactory->getName() == rti::HLAauthorizerName,
      "standard authorizer library factory-factory did not forward the HLAauthorizer factory");
  auto libraryAuthorizer = libraryFactory->getAuthorizer();
  require(
      libraryAuthorizer != nullptr &&
          libraryAuthorizer->getName() == rti::HLAauthorizerName,
      "standard authorizer library factory-factory did not create the HLAauthorizer");
  require(
      rti::AuthorizerFactoryFactory::getAuthorizerFactory(L"unknown") == nullptr,
      "standard authorizer library factory-factory accepted an unsupported name");

  auto factory = rti::HLAauthorizerFactoryFactory::getAuthorizerFactory(
      rti::HLAauthorizerName);
  require(
      factory != nullptr && factory->getName() == rti::HLAauthorizerName,
      "standard HLAauthorizerFactoryFactory did not select the HLAauthorizer factory");
  auto authorizer = factory->getAuthorizer();
  require(
      authorizer != nullptr && authorizer->getName() == rti::HLAauthorizerName,
      "standard HLAauthorizerFactoryFactory did not create the named authorizer");
  require(
      rti::HLAauthorizerFactoryFactory::getAuthorizerFactory(L"unknown") == nullptr,
      "standard HLAauthorizerFactoryFactory accepted an unsupported authorizer name");
}

void scenarioAuthorizerConnectServiceDecision(
    Options const&,
    rti::CallbackModel callbackModel) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(L"tck-authorizer");

  rti::RTIambassadorFactory factory;
  rti::NullFederateAmbassador federate;
  rti::HLAnoCredentials noCredentials;
  auto noCredentialsAmbassador = factory.createRTIambassador();
  bool noCredentialsWereSupported = false;
  try {
    static_cast<void>(noCredentialsAmbassador->connect(
        federate, callbackModel, configuration, noCredentials));
  } catch (rti::Unauthorized const&) {
    noCredentialsWereSupported = true;
  }
  require(noCredentialsWereSupported,
          "HLAauthorizer did not recognize HLAnoCredentials before applying its password policy");

  rti::HLAplainTextPassword wrongPassword(L"wrong-tck-authorizer-password");
  auto deniedAmbassador = factory.createRTIambassador();
  bool unauthorized = false;
  try {
    deniedAmbassador->connect(
        federate, callbackModel, configuration, wrongPassword);
  } catch (rti::Unauthorized const&) {
    unauthorized = true;
  }
  require(
      unauthorized,
      "Connect did not reject credentials denied by the configured Authorizer");

  rti::HLAplainTextPassword matchingPassword(L"tck-authorizer-password");
  auto acceptedAmbassador = factory.createRTIambassador();
  acceptedAmbassador->connect(
      federate, callbackModel, configuration, matchingPassword);
  acceptedAmbassador->disconnect();
}

void scenarioAuthorizerUnrecognizedCredentialsUnauthorized(
    Options const&,
    rti::CallbackModel callbackModel) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(L"tck-authorizer");

  rti::Credentials unknown(
      L"TckUnknownCredentialType", rti::VariableLengthData{});
  rti::NullFederateAmbassador federate;
  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  bool unauthorized = false;
  try {
    static_cast<void>(ambassador->connect(
        federate, callbackModel, configuration, unknown));
  } catch (rti::Unauthorized const&) {
    unauthorized = true;
  }
  require(unauthorized,
          "HLAauthorizer did not treat an unrecognized credential type as unauthorized");
}

void scenarioAuthorizerDisabledServiceRejectsCredentials(
    Options const&,
    rti::CallbackModel callbackModel) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(
      L"tck-authorizer-disabled-credentials");

  rti::HLAplainTextPassword credentials(
      L"tck-disabled-authorization-password");
  rti::NullFederateAmbassador federate;
  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  bool unauthorized = false;
  try {
    static_cast<void>(ambassador->connect(
        federate, callbackModel, configuration, credentials));
  } catch (rti::Unauthorized const&) {
    unauthorized = true;
  }
  require(unauthorized,
          "Connect accepted supplied credentials while RID authorization was disabled");
}

void scenarioAuthorizerFederationServiceDecision(
    Options const& options,
    rti::CallbackModel callbackModel) {
  require(!options.fom.empty(),
          "Federation authorization TCK requires the adapter's FOM module");
  constexpr wchar_t customAuthorizerName[] = L"TckFederationGate";
  auto customFactory =
      rti::AuthorizerFactoryFactory::getAuthorizerFactory(
          customAuthorizerName);
  require(customFactory != nullptr &&
              customFactory->getName() == customAuthorizerName,
          "the public AuthorizerFactoryFactory did not select the configured custom factory");
  auto customAuthorizer = customFactory->getAuthorizer();
  require(customAuthorizer != nullptr &&
              customAuthorizer->getName() == customAuthorizerName,
          "the public custom AuthorizerFactory did not create its named Authorizer");
  require(
      rti::AuthorizerFactoryFactory::getAuthorizerFactory(
          L"UnsupportedTckAuthorizer") == nullptr,
      "the public AuthorizerFactoryFactory did not delegate unsupported custom names");

  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(L"tck-authorizer-federation");

  rti::RTIambassadorFactory factory;
  rti::HLAnoCredentials noCredentials;
  rti::NullFederateAmbassador noCredentialsFederate;
  auto noCredentialsAmbassador = factory.createRTIambassador();
  noCredentialsAmbassador->connect(
      noCredentialsFederate,
      callbackModel,
      configuration,
      noCredentials);

  std::wstring const credentialDeniedFederation =
      L"tck-authorizer-missing-create-credentials";
  bool missingCredentialsRejected = false;
  try {
    noCredentialsAmbassador->createFederationExecution(
        credentialDeniedFederation,
        options.fom.wstring(),
        options.logicalTimeImplementationName);
  } catch (rti::Unauthorized const&) {
    missingCredentialsRejected = true;
  }
  require(
      missingCredentialsRejected,
      "Create Federation Execution accepted Connect credentials that do not authorize federation creation");

  bool missingCredentialCreationLeftNoFederation = false;
  try {
    noCredentialsAmbassador->joinFederationExecution(
        L"tck-authorizer-missing-credential-join",
        options.federateType,
        credentialDeniedFederation);
  } catch (rti::FederationExecutionDoesNotExist const&) {
    missingCredentialCreationLeftNoFederation = true;
  }
  require(
      missingCredentialCreationLeftNoFederation,
      "credential-denied Create Federation Execution left a federation that could be joined");
  noCredentialsAmbassador->disconnect();

  constexpr wchar_t createCredentialType[] =
      L"TckFederationCreateCredential";
  constexpr std::array<std::uint8_t, 6> createCredentialBytes{
      0x43U, 0x52U, 0x45U, 0x41U, 0x54U, 0x45U};
  rti::VariableLengthData createCredentialData(
      createCredentialBytes.data(), createCredentialBytes.size());
  rti::Credentials createCredentials(
      createCredentialType, createCredentialData);
  rti::NullFederateAmbassador federate;
  auto ambassador = factory.createRTIambassador();
  ambassador->connect(
      federate,
      callbackModel,
      configuration,
      createCredentials);

  std::wstring const deniedFederation =
      L"tck-authorizer-denied-federation";
  bool unauthorized = false;
  try {
    ambassador->createFederationExecution(
        deniedFederation,
        options.fom.wstring(),
        options.logicalTimeImplementationName);
  } catch (rti::Unauthorized const&) {
    unauthorized = true;
  }
  require(unauthorized,
          "Create Federation Execution did not apply the configured custom Authorizer decision");

  bool deniedCreationLeftNoFederation = false;
  try {
    ambassador->joinFederationExecution(
        L"tck-authorizer-denied-join",
        options.federateType,
        deniedFederation);
  } catch (rti::FederationExecutionDoesNotExist const&) {
    deniedCreationLeftNoFederation = true;
  }
  require(deniedCreationLeftNoFederation,
          "denied Create Federation Execution left a federation that could be joined");

  auto const allowedFederation =
      federationName(options, "authorizer-allowed-federation");
  ambassador->createFederationExecution(
      allowedFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  ambassador->destroyFederationExecution(allowedFederation);
  ambassador->disconnect();
}

void scenarioAuthorizerDestroyServiceDecision(
    Options const& options,
    rti::CallbackModel callbackModel) {
  require(!options.fom.empty(),
          "Destroy authorization TCK requires the adapter's FOM module");
  constexpr wchar_t customAuthorizerName[] = L"TckDestroyGate";
  auto customFactory =
      rti::AuthorizerFactoryFactory::getAuthorizerFactory(
          customAuthorizerName);
  require(customFactory != nullptr &&
              customFactory->getName() == customAuthorizerName,
          "the public AuthorizerFactoryFactory did not select the Destroy test factory");
  auto customAuthorizer = customFactory->getAuthorizer();
  require(customAuthorizer != nullptr &&
              customAuthorizer->getName() == customAuthorizerName,
          "the public Destroy test factory did not create its named Authorizer");

  constexpr wchar_t credentialType[] = L"TckDestroyCredential";
  constexpr std::array<std::uint8_t, 6> credentialBytes{
      0x44U, 0x45U, 0x53U, 0x54U, 0x52U, 0x59U};
  rti::VariableLengthData credentialData(
      credentialBytes.data(), credentialBytes.size());
  rti::Credentials destroyCredentials(credentialType, credentialData);
  rti::HLAnoCredentials noCredentials;

  auto ownerConfiguration = rti::RtiConfiguration::createConfiguration();
  ownerConfiguration.withConfigurationName(L"tck-authorizer-destroy");
  auto challengerConfiguration = rti::RtiConfiguration::createConfiguration();
  challengerConfiguration.withConfigurationName(L"tck-authorizer-destroy");
  rti::RTIambassadorFactory factory;
  rti::NullFederateAmbassador ownerFederate;
  rti::NullFederateAmbassador challengerFederate;
  auto owner = factory.createRTIambassador();
  auto challenger = factory.createRTIambassador();
  owner->connect(
      ownerFederate, callbackModel, ownerConfiguration, destroyCredentials);
  challenger->connect(
      challengerFederate, callbackModel, challengerConfiguration, noCredentials);

  auto const federation =
      federationName(options, "authorizer-destroy-connect-credentials");
  owner->createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);

  bool destroyDenied = false;
  try {
    challenger->destroyFederationExecution(federation);
  } catch (rti::Unauthorized const&) {
    destroyDenied = true;
  }
  require(destroyDenied,
          "Destroy Federation Execution accepted a Connect without credentials that authorize destruction");

  auto const verifier = owner->joinFederationExecution(
      L"tck-authorizer-destroy-verifier",
      options.federateType,
      federation);
  require(verifier.isValid(),
          "the federation did not remain available after unauthorized Destroy");
  owner->resignFederationExecution(rti::NO_ACTION);

  owner->destroyFederationExecution(federation);
  challenger->disconnect();
  owner->disconnect();
}

void scenarioAuthorizerJoinServiceDecision(
    Options const& options,
    rti::CallbackModel callbackModel) {
  require(!options.fom.empty(),
          "Join authorization TCK requires the adapter's FOM module");
  constexpr wchar_t customAuthorizerName[] = L"TckJoinGate";
  auto customFactory =
      rti::AuthorizerFactoryFactory::getAuthorizerFactory(
          customAuthorizerName);
  require(customFactory != nullptr &&
              customFactory->getName() == customAuthorizerName,
          "the public AuthorizerFactoryFactory did not select the Join test factory");
  auto customAuthorizer = customFactory->getAuthorizer();
  require(customAuthorizer != nullptr &&
              customAuthorizer->getName() == customAuthorizerName,
          "the public Join test factory did not create its named Authorizer");

  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(L"tck-authorizer-join");

  constexpr wchar_t authorizedFederateName[] =
      L"tck-authorizer-join-retry";
  constexpr wchar_t authorizedFederateType[] =
      L"TckAuthorizedFederateType";
  auto const federation =
      federationName(options, "authorizer-join-denial-and-retry");
  auto const otherFederation = federation + L"-other-target";
  std::wstring const authorizedScope =
      federation + L"\n" + authorizedFederateName + L"\n" +
      authorizedFederateType;
  rti::VariableLengthData const credentialData(
      authorizedScope.data(), authorizedScope.size() * sizeof(wchar_t));
  rti::Credentials const credentials(L"TckJoinCredential", credentialData);

  rti::RTIambassadorFactory factory;
  rti::NullFederateAmbassador federate;
  auto ambassador = factory.createRTIambassador();
  ambassador->connect(federate, callbackModel, configuration, credentials);

  ambassador->createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  ambassador->createFederationExecution(
      otherFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);

  auto const joinDenied = [&](
                             std::wstring const& targetFederation,
                             std::wstring const& federateName,
                             std::wstring const& federateType) {
    try {
      static_cast<void>(ambassador->joinFederationExecution(
          federateName, federateType, targetFederation));
    } catch (rti::Unauthorized const&) {
      return true;
    }
    return false;
  };
  require(
      joinDenied(federation, L"tck-authorizer-wrong-name", authorizedFederateType),
      "Join authorization ignored the federate name bound to Connect credentials");
  require(
      joinDenied(federation, authorizedFederateName, L"TckWrongFederateType"),
      "Join authorization ignored the federate type bound to Connect credentials");
  require(
      joinDenied(otherFederation, authorizedFederateName, authorizedFederateType),
      "Join authorization ignored the federation name bound to Connect credentials");

  auto const joinedFederate = ambassador->joinFederationExecution(
      authorizedFederateName, authorizedFederateType, federation);
  require(joinedFederate.isValid(),
          "Join authorization rejected credentials matching the federation, federate name, and federate type");
  ambassador->resignFederationExecution(rti::NO_ACTION);
  ambassador->destroyFederationExecution(otherFederation);
  ambassador->destroyFederationExecution(federation);
  ambassador->disconnect();
}

void scenarioAuthorizerNoCredentialsConnect(
    Options const&,
    rti::CallbackModel callbackModel) {
  constexpr wchar_t customAuthorizerName[] = L"TckNoCredentialsGate";
  auto customFactory =
      rti::AuthorizerFactoryFactory::getAuthorizerFactory(
          customAuthorizerName);
  require(customFactory != nullptr &&
              customFactory->getName() == customAuthorizerName,
          "the public AuthorizerFactoryFactory did not select the no-credentials test factory");
  auto customAuthorizer = customFactory->getAuthorizer();
  require(customAuthorizer != nullptr &&
              customAuthorizer->getName() == customAuthorizerName,
          "the public no-credentials test factory did not create its named Authorizer");

  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(L"tck-authorizer-no-credentials");
  rti::HLAnoCredentials credentials;
  rti::NullFederateAmbassador federate;
  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  static_cast<void>(ambassador->connect(
      federate, callbackModel, configuration, credentials));
  ambassador->disconnect();
}

void scenarioAuthorizerRidConfigurationDecision(
    Options const&,
    rti::CallbackModel callbackModel) {
  rti::RTIambassadorFactory factory;
  rti::HLAnoCredentials credentials;
  auto connectWithProfile = [&](std::wstring const& profile) {
    auto configuration = rti::RtiConfiguration::createConfiguration();
    configuration.withConfigurationName(profile);
    rti::NullFederateAmbassador federate;
    auto ambassador = factory.createRTIambassador();
    try {
      static_cast<void>(ambassador->connect(
          federate, callbackModel, configuration, credentials));
    } catch (rti::Unauthorized const&) {
      return false;
    }
    ambassador->disconnect();
    return true;
  };

  require(!connectWithProfile(L"tck-authorizer-rid-denied"),
          "the RID-selected denying service did not reject Connect");
  require(connectWithProfile(L"tck-authorizer-rid-allowed"),
          "the RID-selected allowing service did not authorize Connect");
  require(connectWithProfile(L"tck-authorizer-rid-disabled"),
          "the RID disabled profile did not allow no-credentials Connect");
}

void scenarioAuthorizerSingleServicePerRidProfile(
    Options const&,
    rti::CallbackModel callbackModel) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(
      L"tck-authorizer-rid-multiple-services");
  rti::HLAnoCredentials credentials;
  rti::NullFederateAmbassador federate;
  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();

  bool connectRejected = false;
  try {
    static_cast<void>(ambassador->connect(
        federate, callbackModel, configuration, credentials));
  } catch (rti::Exception const&) {
    connectRejected = true;
  }
  if (!connectRejected) {
    ambassador->disconnect();
  }
  require(connectRejected,
          "Connect accepted a RID profile with two authorization services");
}


}  // namespace hla_rti_cpp_tck::detail
