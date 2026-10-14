package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies valid dimension handles stop being queryable after federation resignation. */
final class DimensionLookupAfterResignationTck {
   private DimensionLookupAfterResignationTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String interactionClassName, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-lookup-resigned-" + UUID.randomUUID();
      boolean ownerConnected = false, memberConnected = false, created = false;
      boolean ownerJoined = false, memberJoined = false;
      try {
         owner.connect(noOpCallbacks(), callbackModel);
         ownerConnected = true;
         member.connect(noOpCallbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-dimension-lookup-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-dimension-lookup-member", federation);
         memberJoined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         InteractionClassHandle interactionClass =
            owner.getInteractionClassHandle(interactionClassName);
         DimensionHandle dimension = owner.getDimensionHandle(dimensionName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(objectClass))
               && interactionClassName.equals(
                  owner.getInteractionClassName(interactionClass))
               && dimensionName.equals(owner.getDimensionName(dimension))
               && owner.getDimensionUpperBound(dimension) > 0,
            "adapter FOM did not provide valid object, interaction, and dimension handles");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         JavaTckSupport.expectFailure(() -> owner.getDimensionName(dimension),
            "FederateNotExecutionMember", "dimension-name lookup after resignation");
         JavaTckSupport.expectFailure(() -> owner.getDimensionUpperBound(dimension),
            "FederateNotExecutionMember", "dimension-upper-bound lookup after resignation");
         JavaTckSupport.expectFailure(
            () -> owner.getAvailableDimensionsForObjectClass(objectClass),
            "FederateNotExecutionMember",
            "object-class available-dimensions lookup after resignation");
         JavaTckSupport.expectFailure(
            () -> owner.getAvailableDimensionsForInteractionClass(interactionClass),
            "FederateNotExecutionMember",
            "interaction-class available-dimensions lookup after resignation");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador noOpCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
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
