package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Exercises semantic-invalid adapter FOM files and verifies each name remains reusable. */
final class FomStandardInvalidInputCorpusTck {
   private static final String[] INVALID_FOM_EXCEPTIONS = {
      "CouldNotOpenFOM", "ErrorReadingFOM", "InvalidFOM", "InconsistentFOM"
   };

   private FomStandardInvalidInputCorpusTck() { }

   static void run(RtiFactory factory, String validFom, String[] invalidFomModules,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(invalidFomModules.length > 0,
         "adapter must supply at least one standard-invalid FOM input");
      RTIambassador creator = factory.getRtiAmbassador();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      String federation = null;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         for (int index = 0; index < invalidFomModules.length; ++index) {
            federation = "java-tck-standard-invalid-fom-" + index + "-" + UUID.randomUUID();
            expectInvalidCreate(creator, federation, invalidFomModules[index],
               timeImplementation);

            creator.createFederationExecution(federation, validFom, timeImplementation);
            created = true;
            FederateHandle handle = creator.joinFederationExecution(
               "java-tck-invalid-fom-recovery-" + index, federation);
            joined = true;
            JavaTckSupport.check(handle != null,
               "valid FOM reuse after invalid input did not establish membership");
            creator.resignFederationExecution(ResignAction.NO_ACTION);
            joined = false;
            creator.destroyFederationExecution(federation);
            created = false;
         }
      } finally {
         if (joined) {
            try { creator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created && federation != null) {
            try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { creator.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void expectInvalidCreate(RTIambassador creator, String federation,
         String invalidFom, String timeImplementation) throws Exception {
      try {
         creator.createFederationExecution(federation, invalidFom, timeImplementation);
      } catch (Throwable error) {
         for (Throwable current = error; current != null; current = current.getCause()) {
            if (Arrays.asList(INVALID_FOM_EXCEPTIONS)
                  .contains(current.getClass().getSimpleName())) {
               return;
            }
         }
         throw new AssertionError("standard-invalid FOM input threw unexpected " + error, error);
      }
      throw new AssertionError("standard-invalid FOM input was accepted: " + invalidFom);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK invalid FOM corpus callbacks";
            if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
            if ("equals".equals(method.getName())) {
               return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
   }
}
