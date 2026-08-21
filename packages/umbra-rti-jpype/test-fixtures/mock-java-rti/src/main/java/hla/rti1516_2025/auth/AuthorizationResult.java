package hla.rti1516_2025.auth;

/** Standard authorization result value used by the JNI fixture build. */
public final class AuthorizationResult {
   public enum Code {
      AUTHORIZED, UNAUTHORIZED, INVALID_CREDENTIALS, AUTHORIZATION_ERROR
   }

   public final Code code;
   public final String message;

   public AuthorizationResult(Code code) {
      this(code, "");
   }

   public AuthorizationResult(Code code, String message) {
      this.code = code;
      this.message = message;
   }
}
