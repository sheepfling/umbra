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

/** Standard DDM metadata lookup without creating regions or exercising region services. */
final class DimensionLookupLifecycleTck {
   private DimensionLookupLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String interactionClassName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador lifecycle = factory.getRtiAmbassador();
      boolean lifecycleConnected = false;
      try {
         expectLifecycleFailures(lifecycle, firstDimensionName, "NotConnected", "before connect");
         lifecycle.connect(callbacks(), callbackModel);
         lifecycleConnected = true;
         expectLifecycleFailures(lifecycle, firstDimensionName,
            "FederateNotExecutionMember", "after connect before join");
      } finally {
         if (lifecycleConnected) {
            try { lifecycle.disconnect(); } catch (Exception ignored) { }
         }
      }

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-lookup-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-dimension-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-dimension-member", federation);
         memberJoined = true;

         ObjectClassHandle ownerObject = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberObject = member.getObjectClassHandle(objectClassName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle memberInteraction =
            member.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(ownerObject != null && memberObject != null
               && ownerInteraction != null && memberInteraction != null,
            "adapter FOM did not provide the requested object and interaction classes");

         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle memberFirst = member.getDimensionHandle(firstDimensionName);
         DimensionHandle memberSecond = member.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerFirst != null && ownerSecond != null
               && memberFirst != null && memberSecond != null && !ownerFirst.equals(ownerSecond)
               && !memberFirst.equals(memberSecond),
            "dimension lookup returned null or duplicate handles");
         JavaTckSupport.check(ownerFirst.equals(memberFirst) && ownerSecond.equals(memberSecond),
            "dimension handles did not retain identity across federates");
         JavaTckSupport.check(firstDimensionName.equals(owner.getDimensionName(ownerFirst))
               && secondDimensionName.equals(owner.getDimensionName(ownerSecond))
               && firstDimensionName.equals(member.getDimensionName(memberFirst))
               && secondDimensionName.equals(member.getDimensionName(memberSecond)),
            "dimension handle-to-name lookup did not round-trip");
         long firstBound = owner.getDimensionUpperBound(ownerFirst);
         long secondBound = owner.getDimensionUpperBound(ownerSecond);
         JavaTckSupport.check(firstBound > 0 && secondBound > 0
               && firstBound == member.getDimensionUpperBound(memberFirst)
               && secondBound == member.getDimensionUpperBound(memberSecond),
            "dimension upper bounds were not positive and stable across federates");

         DimensionHandleSet ownerObjectDimensions =
            owner.getAvailableDimensionsForObjectClass(ownerObject);
         DimensionHandleSet memberObjectDimensions =
            member.getAvailableDimensionsForObjectClass(memberObject);
         DimensionHandleSet ownerInteractionDimensions =
            owner.getAvailableDimensionsForInteractionClass(ownerInteraction);
         DimensionHandleSet memberInteractionDimensions =
            member.getAvailableDimensionsForInteractionClass(memberInteraction);
         JavaTckSupport.check(ownerObjectDimensions.contains(ownerFirst)
               && ownerObjectDimensions.contains(ownerSecond)
               && memberObjectDimensions.contains(memberFirst)
               && memberObjectDimensions.contains(memberSecond)
               && ownerInteractionDimensions.contains(ownerFirst)
               && ownerInteractionDimensions.contains(ownerSecond)
               && memberInteractionDimensions.contains(memberFirst)
               && memberInteractionDimensions.contains(memberSecond),
            "available class dimensions omitted an adapter-supplied dimension");

         String missingDimensionName = "TckMissingDimension"
            + UUID.randomUUID().toString().replace("-", "");
         JavaTckSupport.expectFailure(
            () -> owner.getDimensionHandle(missingDimensionName), "NameNotFound",
            "looking up an unknown dimension name");
         JavaTckSupport.expectFailure(
            () -> owner.getDimensionName(null), "InvalidDimensionHandle",
            "looking up an invalid dimension name");
         JavaTckSupport.expectFailure(
            () -> owner.getDimensionUpperBound(null), "InvalidDimensionHandle",
            "looking up an invalid dimension upper bound");
         JavaTckSupport.expectFailure(
            () -> owner.getAvailableDimensionsForObjectClass(null),
            "InvalidObjectClassHandle", "looking up dimensions for an invalid object class");
         JavaTckSupport.expectFailure(
            () -> owner.getAvailableDimensionsForInteractionClass(null),
            "InvalidInteractionClassHandle",
            "looking up dimensions for an invalid interaction class");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         expectLifecycleFailures(owner, firstDimensionName,
            "FederateNotExecutionMember", "after resignation");
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
         if (memberConnected) { try { member.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void expectLifecycleFailures(RTIambassador ambassador, String dimension,
         String exception, String phase) throws Exception {
      JavaTckSupport.expectFailure(() -> ambassador.getDimensionHandle(dimension), exception,
         "looking up a dimension handle " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.getDimensionName(null), exception,
         "looking up a dimension name " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.getDimensionUpperBound(null), exception,
         "looking up a dimension upper bound " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.getAvailableDimensionsForObjectClass(null), exception,
         "looking up object-class dimensions " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.getAvailableDimensionsForInteractionClass(null), exception,
         "looking up interaction-class dimensions " + phase);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK dimension lookup callback sink";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
