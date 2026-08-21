package hla.rti1516_2025.auth;

import java.util.ServiceLoader;

/** Standard ServiceLoader entry point for authorization providers. */
public final class AuthorizerFactoryFactory {
   private AuthorizerFactoryFactory() {}

   public static AuthorizerFactory getAuthorizerFactory(String name) {
      for (AuthorizerFactory provider : ServiceLoader.load(AuthorizerFactory.class)) {
         if (name.equals(provider.getName())) {
            return provider;
         }
      }
      throw new IllegalArgumentException("No authorizer factory: " + name);
   }
}
