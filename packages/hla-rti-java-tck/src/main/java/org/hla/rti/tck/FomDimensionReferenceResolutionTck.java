package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Later-module available-dimension resolution through the standard API. */
final class FomDimensionReferenceResolutionTck {
   private FomDimensionReferenceResolutionTck() { }

   static void run(RtiFactory factory, String consumer, String provider,
         String missing, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-reference-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {consumer, provider}, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-dimension-reference-member", federation);
         joined = true;
         DimensionHandle dimension = creator.getDimensionHandle("TckDeferredDimension");
         ObjectClassHandle objectClass = creator.getObjectClassHandle(
            "HLAobjectRoot.TckDimensionReferenceObject");
         InteractionClassHandle interaction = creator.getInteractionClassHandle(
            "HLAinteractionRoot.TckDimensionReferenceInteraction");
         DimensionHandleSet objectDimensions =
            creator.getAvailableDimensionsForObjectClass(objectClass);
         DimensionHandleSet interactionDimensions =
            creator.getAvailableDimensionsForInteractionClass(interaction);
         JavaTckSupport.check(dimension != null && objectClass != null &&
               interaction != null && objectDimensions.size() == 1 &&
               objectDimensions.contains(dimension) && interactionDimensions.size() == 1 &&
               interactionDimensions.contains(dimension) &&
               creator.getDimensionUpperBound(dimension) == 100L,
            "later FOM module did not supply the available dimension to both classes");
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         creator.destroyFederationExecution(federation);
         created = false;

         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(federation,
               new String[] {missing}, timeImplementation),
            "InconsistentFOM", "unresolved available dimension");
         creator.createFederationExecution(federation,
            new String[] {consumer, provider}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) try { creator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK dimension-reference callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
