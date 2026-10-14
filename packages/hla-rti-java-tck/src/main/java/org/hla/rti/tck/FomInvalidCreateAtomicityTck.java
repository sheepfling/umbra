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

/** Verifies invalid FOM creation leaves the requested federation name available. */
final class FomInvalidCreateAtomicityTck {
   private static final String[] INVALID_FOM_EXCEPTIONS = {
      "CouldNotOpenFOM", "ErrorReadingFOM", "InvalidFOM", "InconsistentFOM"
   };

   private FomInvalidCreateAtomicityTck() { }

   static void run(RtiFactory factory, String validFom, String[] invalidFomModules,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(invalidFomModules.length > 0,
         "adapter must supply at least one invalid FOM module input");
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-fom-invalid-create-" + UUID.randomUUID();
      String creatorName = "java-tck-fom-create-owner";
      String memberName = "java-tck-fom-create-member";
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean creatorJoined = false;
      boolean memberJoined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;

         for (String invalidFom : invalidFomModules) {
            expectInvalidCreate(creator, federation, invalidFom, timeImplementation);
         }

         creator.createFederationExecution(federation, validFom, timeImplementation);
         created = true;
         FederateHandle creatorHandle =
            creator.joinFederationExecution(creatorName, federation);
         creatorJoined = true;
         FederateHandle memberHandle = member.joinFederationExecution(memberName, federation);
         memberJoined = true;
         JavaTckSupport.check(creatorHandle != null && memberHandle != null
               && creatorHandle.equals(creator.getFederateHandle(creatorName))
               && memberHandle.equals(creator.getFederateHandle(memberName)),
            "valid create after invalid-FOM rejection did not establish member identity");
      } finally {
         if (memberJoined) resign(member);
         if (creatorJoined) resign(creator);
         if (created) {
            try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (creatorConnected) disconnect(creator);
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
         throw new AssertionError("invalid FOM create threw unexpected " + error, error);
      }
      throw new AssertionError("invalid FOM create succeeded for adapter input: " + invalidFom);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK invalid FOM callbacks";
            if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
            if ("equals".equals(method.getName())) {
               return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }
}
