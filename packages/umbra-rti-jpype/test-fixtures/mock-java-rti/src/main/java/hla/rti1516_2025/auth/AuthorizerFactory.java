package hla.rti1516_2025.auth;

/** Standard authorization-factory shape used by the JNI fixture build. */
public interface AuthorizerFactory {
   String HLA_AUTHORIZER_NAME = "HLAauthorizer";
   String getName();
   Authorizer getAuthorizer();
}
