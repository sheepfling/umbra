package org.hla.rti.tck;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.auth.AuthorizationResult;
import hla.rti1516_2025.auth.Authorizer;
import hla.rti1516_2025.auth.AuthorizerFactory;
import hla.rti1516_2025.auth.Credentials;
import hla.rti1516_2025.auth.HLAnoCredentials;
import hla.rti1516_2025.auth.HLAplainTextPassword;
import java.util.Arrays;

/** Provider-independent Java value and dispatch checks for configuration and authorization. */
final class ConfigurationAuthorizationTck {
   private ConfigurationAuthorizationTck() {
   }

   static void run() {
      configurationBuilder();
      configurationResult();
      informationValues();
      credentials();
      authorizerDispatch();
   }

   private static void configurationBuilder() {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration();
      RtiConfiguration returned = configuration.withConfigurationName("portable-config")
         .withRtiAddress("rti://portable")
         .withAdditionalSettings("{}");
      require(returned == configuration,
         "RtiConfiguration builder did not return its receiver");
      require("portable-config".equals(configuration.configurationName()),
         "RtiConfiguration lost its configuration name");
      require("rti://portable".equals(configuration.rtiAddress()),
         "RtiConfiguration lost its RTI address");
      require("{}".equals(configuration.additionalSettings()),
         "RtiConfiguration lost its additional settings");

      RtiConfiguration independent = RtiConfiguration.createConfiguration()
         .withConfigurationName("portable-independent");
      require("portable-config".equals(configuration.configurationName())
            && "portable-independent".equals(independent.configurationName()),
         "separate RtiConfiguration instances shared mutable state");
   }

   private static void configurationResult() {
      ConfigurationResult result = new ConfigurationResult(
         true, false, AdditionalSettingsResultCode.SETTINGS_APPLIED, "configured");
      require(result.configurationUsed, "ConfigurationResult lost configurationUsed");
      require(!result.addressUsed, "ConfigurationResult lost addressUsed");
      require(result.additionalSettingsResultCode
            == AdditionalSettingsResultCode.SETTINGS_APPLIED,
         "ConfigurationResult lost its additional-settings result code");
      require("configured".equals(result.message), "ConfigurationResult lost its message");
   }

   private static void informationValues() {
      FederationExecutionInformation execution =
         new FederationExecutionInformation("portable-federation", "HLAinteger64Time");
      FederationExecutionInformation equalExecution =
         new FederationExecutionInformation("portable-federation", "HLAinteger64Time");
      FederationExecutionMemberInformation member =
         new FederationExecutionMemberInformation("portable-federate", "portable-type");
      FederationExecutionMemberInformation equalMember =
         new FederationExecutionMemberInformation("portable-federate", "portable-type");
      require("portable-federation".equals(execution.federationExecutionName)
            && "HLAinteger64Time".equals(execution.logicalTimeImplementationName)
            && execution.equals(equalExecution)
            && execution.hashCode() == equalExecution.hashCode(),
         "federation execution information value semantics changed");
      require("portable-federate".equals(member.federateName)
            && "portable-type".equals(member.federateType)
            && member.equals(equalMember)
            && member.hashCode() == equalMember.hashCode(),
         "federation member information value semantics changed");
   }

   private static void credentials() {
      HLAnoCredentials none = new HLAnoCredentials();
      require(HLAnoCredentials.HLA_NO_CREDENTIALS_TYPE.equals(none.getType())
            && none.getData().length == 0,
         "HLAnoCredentials type or empty payload changed");

      HLAplainTextPassword password = new HLAplainTextPassword("portable-password");
      require(HLAplainTextPassword.HLA_PLAIN_TEXT_PASSWORD_TYPE.equals(password.getType())
            && "portable-password".equals(password.decode())
            && "portable-password".equals(new HLAplainTextPassword(password.getData()).decode()),
         "HLAplainTextPassword did not round-trip through its standard encoded data");
      HLAplainTextPassword unicodePassword = new HLAplainTextPassword("p\u00e4ss");
      require("p\u00e4ss".equals(unicodePassword.decode())
            && "p\u00e4ss".equals(new HLAplainTextPassword(unicodePassword.getData()).decode()),
         "HLAplainTextPassword did not preserve Unicode text");

      byte[] supplied = new byte[] {0x43, 0x52, 0x45};
      Credentials credentials = new Credentials("portable-custom", supplied);
      require("portable-custom".equals(credentials.getType())
            && Arrays.equals(credentials.getData(), supplied),
         "Credentials did not expose its configured type and data");
   }

   private static void authorizerDispatch() {
      Authorizer authorizer = new PortableAuthorizer();
      AuthorizerFactory factory = new PortableAuthorizerFactory();
      HLAnoCredentials credentials = new HLAnoCredentials();
      AuthorizationResult rtiResult = authorizer.authorizeRtiOperation(credentials);
      AuthorizationResult federationResult = authorizer.authorizeFederationOperation(
         credentials, "portable-federation");
      AuthorizationResult federateResult = authorizer.authorizeFederateOperation(
         credentials, "portable-federation", "portable-federate", "portable-type");
      require("portable-authorizer".equals(authorizer.getName()),
         "Authorizer returned an unexpected name");
      require(rtiResult.code == AuthorizationResult.Code.AUTHORIZED
            && "portable-rti".equals(rtiResult.message)
            && federationResult.code == AuthorizationResult.Code.AUTHORIZED
            && "portable-federation".equals(federationResult.message)
            && federateResult.code == AuthorizationResult.Code.AUTHORIZED
            && "portable-federate".equals(federateResult.message),
         "Authorizer operation dispatch did not preserve the supplied context");
      require("portable-authorizer-factory".equals(factory.getName())
            && "portable-authorizer".equals(factory.getAuthorizer().getName()),
         "AuthorizerFactory creation or naming semantics changed");
   }

   private static void require(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static final class PortableAuthorizer implements Authorizer {
      @Override
      public AuthorizationResult authorizeRtiOperation(Credentials credentials) {
         return new AuthorizationResult(AuthorizationResult.Code.AUTHORIZED, "portable-rti");
      }

      @Override
      public AuthorizationResult authorizeFederationOperation(
            Credentials credentials, String federationName) {
         return new AuthorizationResult(
            AuthorizationResult.Code.AUTHORIZED, federationName);
      }

      @Override
      public AuthorizationResult authorizeFederateOperation(
            Credentials credentials, String federationName, String federateName,
            String federateType) {
         return new AuthorizationResult(AuthorizationResult.Code.AUTHORIZED, federateName);
      }

      @Override
      public String getName() {
         return "portable-authorizer";
      }
   }

   private static final class PortableAuthorizerFactory implements AuthorizerFactory {
      @Override
      public String getName() {
         return "portable-authorizer-factory";
      }

      @Override
      public Authorizer getAuthorizer() {
         return new PortableAuthorizer();
      }
   }
}
