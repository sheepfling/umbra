package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Dimension input-type preflight observed only through standard federation services. */
final class FomDimensionInputTypeTck {
   private FomDimensionInputTypeTck() { }

   static void run(RtiFactory factory, String rawBasic, String na, String tableType,
         String mim, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-input-type-" + UUID.randomUUID();
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
               federation, new String[] {rawBasic}, mim, timeImplementation),
            "InconsistentFOM", "raw basic representation as a dimension input type");
         for (String[] accepted : new String[][] {
               {na, "TckNoTypeDimensionInput"},
               {tableType, "TckTableDimensionInput"}}) {
            creator.createFederationExecutionWithMIM(federation,
               new String[] {accepted[0]}, mim, timeImplementation);
            created = true;
            member.joinFederationExecution("java-tck-dimension-input-member", federation);
            memberJoined = true;
            DimensionHandle dimension = member.getDimensionHandle(accepted[1]);
            JavaTckSupport.check(dimension != null
                  && accepted[1].equals(member.getDimensionName(dimension))
                  && member.getDimensionUpperBound(dimension) == 100,
               "accepted dimension input type was not publicly queryable");
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
