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

   static void run(RtiFactory factory, String fom, String additionalFom,
         String timeImplementation,
         String objectClassName, String interactionClassName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      ForeignHandles foreignHandles = additionalFom == null || additionalFom.trim().isEmpty()
         ? null : createForeignHandles(factory, fom, additionalFom, timeImplementation,
            callbackModel);
      RTIambassador lifecycle = factory.getRtiAmbassador();
      boolean lifecycleConnected = false;
      try {
         expectDimensionHandleLifecycleFailure(
            lifecycle, firstDimensionName, "NotConnected", "before connect");
         if (foreignHandles != null) {
            expectForeignHandleLifecycleFailures(
               lifecycle, foreignHandles, "NotConnected", false, "before connect");
         }
         lifecycle.connect(callbacks(), callbackModel);
         lifecycleConnected = true;
         expectDimensionHandleLifecycleFailure(lifecycle, firstDimensionName,
            "FederateNotExecutionMember", "after connect before join");
         if (foreignHandles != null) {
            expectForeignHandleLifecycleFailures(
               lifecycle, foreignHandles, "FederateNotExecutionMember", true,
               "after connect before join");
         }
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
         owner.createFederationExecution(federation, fom, timeImplementation);
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
         if (foreignHandles != null) {
            expectForeignHandlesInvalid(owner, foreignHandles, "owner");
            expectForeignHandlesInvalid(member, foreignHandles, "joining member");
         }

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
         JavaTckSupport.check(firstBound == 10L && secondBound == 10L
               && firstBound == member.getDimensionUpperBound(memberFirst)
               && secondBound == member.getDimensionUpperBound(memberSecond),
            "dimension upper bounds did not match the FDD values or remain stable across federates");

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
            () -> member.getDimensionHandle(missingDimensionName), "NameNotFound",
            "looking up an unknown dimension name through the member");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         expectLifecycleFailuresAfterResignation(
            owner, firstDimensionName, ownerFirst, ownerObject, ownerInteraction,
            foreignHandles);
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

   private static void expectDimensionHandleLifecycleFailure(
         RTIambassador ambassador, String dimension, String exception, String phase)
         throws Exception {
      JavaTckSupport.expectFailure(() -> ambassador.getDimensionHandle(dimension), exception,
         "looking up a dimension handle " + phase);
   }

   private static void expectLifecycleFailuresAfterResignation(RTIambassador ambassador,
         String dimension, DimensionHandle validDimension, ObjectClassHandle validObjectClass,
         InteractionClassHandle validInteractionClass, ForeignHandles foreignHandles)
         throws Exception {
      String exception = "FederateNotExecutionMember";
      String phase = "after resignation";
      expectDimensionHandleLifecycleFailure(ambassador, dimension, exception, phase);
      JavaTckSupport.expectFailure(() -> ambassador.getDimensionName(validDimension), exception,
         "looking up a dimension name " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.getDimensionUpperBound(validDimension), exception,
         "looking up a dimension upper bound " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.getAvailableDimensionsForObjectClass(validObjectClass), exception,
         "looking up object-class dimensions " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.getAvailableDimensionsForInteractionClass(validInteractionClass),
         exception,
         "looking up interaction-class dimensions " + phase);
      if (foreignHandles != null) {
         expectForeignHandleLifecycleFailures(
            ambassador, foreignHandles, exception, true, phase);
      }
   }

   private static void expectForeignHandleLifecycleFailures(
         RTIambassador ambassador, ForeignHandles handles, String lifecycleException,
         boolean allowInvalidHandlePrecedence, String phase) throws Exception {
      expectLifecycleOrInvalid(
         () -> ambassador.getDimensionName(handles.dimension), lifecycleException,
         "InvalidDimensionHandle", allowInvalidHandlePrecedence,
         "looking up a foreign dimension name " + phase);
      expectLifecycleOrInvalid(
         () -> ambassador.getDimensionUpperBound(handles.dimension), lifecycleException,
         "InvalidDimensionHandle", allowInvalidHandlePrecedence,
         "looking up a foreign dimension upper bound " + phase);
      expectLifecycleOrInvalid(
         () -> ambassador.getAvailableDimensionsForObjectClass(handles.objectClass),
         lifecycleException, "InvalidObjectClassHandle", allowInvalidHandlePrecedence,
         "looking up foreign object-class dimensions " + phase);
      expectLifecycleOrInvalid(
         () -> ambassador.getAvailableDimensionsForInteractionClass(handles.interactionClass),
         lifecycleException, "InvalidInteractionClassHandle", allowInvalidHandlePrecedence,
         "looking up foreign interaction-class dimensions " + phase);
   }

   private static void expectLifecycleOrInvalid(CheckedOperation operation,
         String lifecycleException, String invalidHandleException,
         boolean allowInvalidHandlePrecedence, String description) throws Exception {
      if (allowInvalidHandlePrecedence) {
         expectFailureEither(operation, lifecycleException, invalidHandleException, description);
      } else {
         JavaTckSupport.expectFailure(operation::run, lifecycleException, description);
      }
   }

   private static void expectForeignHandlesInvalid(
         RTIambassador ambassador, ForeignHandles handles, String ambassadorDescription)
         throws Exception {
      JavaTckSupport.expectFailure(
         () -> ambassador.getDimensionName(handles.dimension), "InvalidDimensionHandle",
         "looking up a foreign dimension name through the " + ambassadorDescription);
      JavaTckSupport.expectFailure(
         () -> ambassador.getDimensionUpperBound(handles.dimension), "InvalidDimensionHandle",
         "looking up a foreign dimension upper bound through the " + ambassadorDescription);
      JavaTckSupport.expectFailure(
         () -> ambassador.getAvailableDimensionsForObjectClass(handles.objectClass),
         "InvalidObjectClassHandle",
         "looking up foreign object-class dimensions through the " + ambassadorDescription);
      JavaTckSupport.expectFailure(
         () -> ambassador.getAvailableDimensionsForInteractionClass(handles.interactionClass),
         "InvalidInteractionClassHandle",
         "looking up foreign interaction-class dimensions through the " + ambassadorDescription);
   }

   private static void expectFailureEither(CheckedOperation operation,
         String firstException, String secondException, String description) throws Exception {
      try {
         operation.run();
      } catch (Exception failure) {
         String actual = failure.getClass().getSimpleName();
         JavaTckSupport.check(actual.equals(firstException) || actual.equals(secondException),
            description + " threw " + actual + " instead of " + firstException
               + " or " + secondException);
         return;
      }
      throw new AssertionError(description + " did not throw " + firstException
         + " or " + secondException);
   }

   private static ForeignHandles createForeignHandles(RtiFactory factory, String fom,
         String additionalFom, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador foreign = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-lookup-foreign-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         foreign.connect(callbacks(), callbackModel);
         connected = true;
         foreign.createFederationExecution(
            federation, new String[] {fom, additionalFom}, timeImplementation);
         created = true;
         foreign.joinFederationExecution("java-tck-dimension-lookup-foreign", federation);
         joined = true;
         ForeignHandles result = new ForeignHandles(
            foreign.getDimensionHandle("TckAdditionalDimension"),
            foreign.getObjectClassHandle("HLAobjectRoot.TckExtensionObject"),
            foreign.getInteractionClassHandle("HLAinteractionRoot.TckExtensionInteraction"));
         JavaTckSupport.check(result.dimension != null && result.objectClass != null
               && result.interactionClass != null,
            "additional FOM did not provide foreign dimension and class handles");
         foreign.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         foreign.destroyFederationExecution(federation);
         created = false;
         return result;
      } finally {
         if (joined) {
            try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { foreign.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) { try { foreign.disconnect(); } catch (Exception ignored) { } }
      }
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
   }

   private static final class ForeignHandles {
      private final DimensionHandle dimension;
      private final ObjectClassHandle objectClass;
      private final InteractionClassHandle interactionClass;

      private ForeignHandles(DimensionHandle dimension, ObjectClassHandle objectClass,
            InteractionClassHandle interactionClass) {
         this.dimension = dimension;
         this.objectClass = objectClass;
         this.interactionClass = interactionClass;
      }
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
