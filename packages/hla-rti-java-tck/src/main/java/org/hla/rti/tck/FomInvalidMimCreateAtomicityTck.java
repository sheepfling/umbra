package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Verifies invalid MIM creation leaves the federation name available. */
final class FomInvalidMimCreateAtomicityTck {
   private static final String[] INVALID_MIM_EXCEPTIONS = {
      "CouldNotOpenMIM", "ErrorReadingMIM", "InvalidMIM"
   };
   private static final String[] INVALID_FOM_EXCEPTIONS = {
      "CouldNotOpenFOM", "ErrorReadingFOM", "InvalidFOM", "InconsistentFOM"
   };

   private FomInvalidMimCreateAtomicityTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String[] invalidMimModules, String objectClassName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, invalidMimModules, objectClassName, timeImplementation,
         callbackModel, false);
   }

   static void runComposite(RtiFactory factory, String fom, String mim,
         String[] invalidFomModules, String objectClassName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, invalidFomModules, objectClassName, timeImplementation,
         callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String mim,
         String[] invalidModules, String objectClassName, String timeImplementation,
         CallbackModel callbackModel, boolean combineValidAndInvalidFom) throws Exception {
      JavaTckSupport.check(invalidModules.length > 0,
         "adapter must supply at least one invalid module input");
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-fom-invalid-mim-create-" + UUID.randomUUID();
      String creatorName = "java-tck-mim-create-owner";
      String memberName = "java-tck-mim-create-member";
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

         for (String invalidModule : invalidModules) {
            expectInvalidCreate(creator, federation, fom, mim, invalidModule,
               timeImplementation, combineValidAndInvalidFom);
         }

         creator.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle creatorHandle =
            creator.joinFederationExecution(creatorName, federation);
         creatorJoined = true;
         FederateHandle memberHandle = member.joinFederationExecution(memberName, federation);
         memberJoined = true;
         ObjectClassHandle creatorClass = creator.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(creatorClass.equals(memberClass)
               && creatorHandle.equals(creator.getFederateHandle(creatorName))
               && memberHandle.equals(creator.getFederateHandle(memberName)),
            "valid MIM create after invalid-MIM rejection did not compose declarations and members");
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
         String fom, String mim, String invalidModule, String timeImplementation,
         boolean combineValidAndInvalidFom) throws Exception {
      String[] modules = combineValidAndInvalidFom
         ? new String[] {fom, invalidModule} : new String[] {fom};
      String mimModule = combineValidAndInvalidFom ? mim : invalidModule;
      String[] allowedExceptions = combineValidAndInvalidFom
         ? INVALID_FOM_EXCEPTIONS : INVALID_MIM_EXCEPTIONS;
      try {
         creator.createFederationExecutionWithMIM(federation, modules, mimModule,
            timeImplementation);
      } catch (Throwable error) {
         for (Throwable current = error; current != null; current = current.getCause()) {
            if (Arrays.asList(allowedExceptions)
                  .contains(current.getClass().getSimpleName())) {
               return;
            }
         }
         throw new AssertionError("invalid module create threw unexpected " + error, error);
      }
      throw new AssertionError("invalid module create succeeded for adapter input: "
         + invalidModule);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK invalid MIM callbacks";
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
