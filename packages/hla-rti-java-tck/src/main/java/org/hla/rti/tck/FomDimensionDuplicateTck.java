package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Annex C.4 duplicate and unique dimension composition via the public API. */
final class FomDimensionDuplicateTck {
   private FomDimensionDuplicateTck() { }

   static void run(RtiFactory factory, String base, String second,
         String timeImplementation, CallbackModel callbackModel,
         boolean conflict) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-duplicate-" + UUID.randomUUID();
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
            creator.createFederationExecution(
               federation, new String[] {module}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         if (conflict) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {base, second}, timeImplementation),
               "InconsistentFOM", "conflicting upperBound on one named dimension");
            creator.createFederationExecution(
               federation, new String[] {base}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         } else {
            creator.createFederationExecution(
               federation, new String[] {base, second}, timeImplementation);
            created = true;
            member.joinFederationExecution("java-tck-dimension-duplicate-member", federation);
            memberJoined = true;
            DimensionHandle duplicate = member.getDimensionHandle("TckDuplicateDimension");
            DimensionHandle unique = member.getDimensionHandle("TckUniqueDimension");
            JavaTckSupport.check(duplicate != null && unique != null
                  && !duplicate.equals(unique)
                  && "TckDuplicateDimension".equals(member.getDimensionName(duplicate))
                  && "TckUniqueDimension".equals(member.getDimensionName(unique))
                  && member.getDimensionUpperBound(duplicate) == 100
                  && member.getDimensionUpperBound(unique) == 200,
               "equivalent duplicate or unique dimension did not survive composition");
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
               if ("toString".equals(method.getName())) return "Java TCK dimension callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
