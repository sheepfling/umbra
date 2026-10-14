package org.hla.rti.tck;

import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.NullFederateAmbassador;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/** Provider-independent checks for the official no-op federate ambassador. */
final class NullFederateAmbassadorTck {
   private NullFederateAmbassadorTck() {
   }

   static void run() throws Exception {
      FederateAmbassador callbacks = new NullFederateAmbassador();
      Method[] methods = FederateAmbassador.class.getDeclaredMethods();
      int invoked = 0;
      for (Method method : methods) {
         if (method.getReturnType() != void.class
               || !Modifier.isAbstract(method.getModifiers())) {
            throw new AssertionError("unexpected non-callback API method: " + method);
         }
         Object[] arguments = new Object[method.getParameterCount()];
         for (Class<?> parameterType : method.getParameterTypes()) {
            if (parameterType.isPrimitive()) {
               throw new AssertionError("unexpected primitive callback parameter: " + method);
            }
         }
         try {
            method.invoke(callbacks, arguments);
         } catch (InvocationTargetException error) {
            throw new AssertionError("null ambassador callback failed: " + method,
               error.getCause());
         }
         invoked++;
      }
      if (invoked != methods.length || invoked == 0) {
         throw new AssertionError("not all standard callback overloads were invoked");
      }
   }
}
