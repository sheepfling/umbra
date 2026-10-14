#include "tck_configuration_authorization.hpp"

namespace hla_rti_cpp_tck::detail {

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


}  // namespace hla_rti_cpp_tck::detail
