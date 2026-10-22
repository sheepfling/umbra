package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Public dimension-name uniqueness preflight with legal cross-module controls. */
final class FomDimensionNameUniquenessTck {
   private FomDimensionNameUniquenessTck() { }

   static void run(RtiFactory factory, String duplicate, String base,
         String equivalent, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-name-uniqueness-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         JavaTckSupport.expectFailure(() -> creator.createFederationExecutionWithMIM(
               federation, new String[] {duplicate}, mim, timeImplementation),
            "InconsistentFOM", "duplicate dimension names within one module");
         for (String[] accepted : new String[][] {
               {base}, {base, equivalent}}) {
            creator.createFederationExecutionWithMIM(federation, accepted,
               mim, timeImplementation);
            created = true;
            member.joinFederationExecution("java-tck-dimension-name-member", federation);
            memberJoined = true;
            DimensionHandle shared = member.getDimensionHandle("TckDuplicateDimension");
            JavaTckSupport.check(shared != null
                  && "TckDuplicateDimension".equals(member.getDimensionName(shared))
                  && member.getDimensionUpperBound(shared) == 100,
               "accepted dimension was not publicly queryable");
            if (accepted.length == 2) {
               DimensionHandle unique = member.getDimensionHandle("TckUniqueDimension");
               JavaTckSupport.check(unique != null && !unique.equals(shared)
                     && "TckUniqueDimension".equals(member.getDimensionName(unique))
                     && member.getDimensionUpperBound(unique) == 200,
                  "legal cross-module dimension union was not publicly queryable");
            }
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
