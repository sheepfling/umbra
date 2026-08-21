package hla.rti1516_2025.auth;

/** Standard authorization service shape used by the JNI fixture build. */
public interface Authorizer {
   AuthorizationResult authorizeRtiOperation(Credentials credentials);
   AuthorizationResult authorizeFederationOperation(
      Credentials credentials, String federationName);
   AuthorizationResult authorizeFederateOperation(
      Credentials credentials,
      String federationName,
      String federateName,
      String federateType);
   String getName();
}
