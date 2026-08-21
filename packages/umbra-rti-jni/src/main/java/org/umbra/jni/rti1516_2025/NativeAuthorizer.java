package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.auth.Authorizer;
import hla.rti1516_2025.auth.AuthorizationResult;
import hla.rti1516_2025.auth.Credentials;

/** Java projection of Umbra's C++ reference authorizer. */
final class NativeAuthorizer implements Authorizer {
   private final String password;

   private NativeAuthorizer(String password) {
      this.password = password;
   }

   static NativeAuthorizer create() {
      return new NativeAuthorizer(
         System.getProperty("umbra.rti.jni.authorizer.password"));
   }

   @Override public AuthorizationResult authorizeRtiOperation(Credentials credentials) {
      return NativeBridge.nativeAuthorize(
         password, credentials, null, null, null);
   }

   @Override public AuthorizationResult authorizeFederationOperation(
      Credentials credentials, String federationName) {
      return NativeBridge.nativeAuthorize(
         password, credentials, federationName, null, null);
   }

   @Override public AuthorizationResult authorizeFederateOperation(
      Credentials credentials,
      String federationName,
      String federateName,
      String federateType) {
      return NativeBridge.nativeAuthorize(
         password, credentials, federationName, federateName, federateType);
   }

   @Override public String getName() { return NativeAuthorizerFactory.NAME; }
}
