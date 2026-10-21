package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Annex C.7 update-rate duplicate cases through the standard API. */
final class FomUpdateRateDuplicateTck {
   private FomUpdateRateDuplicateTck() { }

   static void run(RtiFactory factory, String base, String second,
         String timeImplementation, CallbackModel callbackModel,
         boolean noteConflict) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-update-rate-duplicate-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         for (String module : new String[] {base, second}) {
            creator.createFederationExecution(federation,
               new String[] {module}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         if (noteConflict) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {base, second}, timeImplementation),
               "InconsistentFOM", "different note references on one update rate");
            creator.createFederationExecution(federation,
               new String[] {base}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         } else {
            creator.createFederationExecution(federation,
               new String[] {base, second}, timeImplementation);
            created = true;
            member.joinFederationExecution("java-tck-update-rate-duplicate-member", federation);
            memberJoined = true;
            JavaTckSupport.check(member.getUpdateRateValue("TckMergeRate") == 30.0,
               "equivalent update-rate declaration was not exposed by public lookup");
            member.resignFederationExecution(ResignAction.NO_ACTION);
            memberJoined = false;
            creator.destroyFederationExecution(federation);
            created = false;
         }
      } finally {
         if (memberJoined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); }
            catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK update-rate callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
