package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.auth.Authorizer;
import hla.rti1516_2025.auth.AuthorizerFactory;

/** ServiceLoader entry for the C++-owned IEEE reference authorizer. */
public final class NativeAuthorizerFactory implements AuthorizerFactory {
   public static final String NAME = "HLAauthorizer";

   @Override public String getName() { return NAME; }

   @Override public Authorizer getAuthorizer() {
      return NativeAuthorizer.create();
   }
}
