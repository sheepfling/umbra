package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicReference;

/** Portable Java subset of deterministic standard service-exception boundaries. */
final class StandardExceptionBoundariesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private StandardExceptionBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      RTIambassador disconnected = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-standard-exception-boundaries-" + UUID.randomUUID();
      String ownerName = "java-tck-standard-exception-owner-" + UUID.randomUUID();
      String memberName = "java-tck-standard-exception-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         JavaTckSupport.expectFailure(
            () -> disconnected.createFederationExecution(federation, fom, timeImplementation),
            "NotConnected", "creating a federation execution before connect");
         JavaTckSupport.expectFailure(
            () -> disconnected.joinFederationExecution(ownerName, federation),
            "NotConnected", "joining a federation execution before connect");
         JavaTckSupport.expectFailure(
            () -> disconnected.destroyFederationExecution(federation),
            "NotConnected", "destroying a federation execution before connect");

         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         member.joinFederationExecution(memberName, federation);
         memberJoined = true;

         JavaTckSupport.expectFailure(() -> member.disconnect(),
            "FederateIsExecutionMember", "disconnecting while still a federation member");

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(ownerClass != null && ownerAttribute != null
               && memberClass != null && memberAttribute != null,
            "standard exception boundary lookup returned a null handle");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(owner, member, callbackModel,
            () -> memberRecorder.discoveries.contains(object), "object discovery");

         JavaTckSupport.expectFailure(
            () -> owner.cancelAttributeOwnershipAcquisition(object, ownerAttributes),
            "AttributeAlreadyOwned", "canceling acquisition of an owned attribute");
         JavaTckSupport.expectFailure(
            () -> member.cancelAttributeOwnershipAcquisition(object, memberAttributes),
            "AttributeAcquisitionWasNotRequested", "canceling an unrequested acquisition");
         JavaTckSupport.expectFailure(
            () -> owner.confirmDivestiture(object, ownerAttributes, new byte[0]),
            "AttributeDivestitureWasNotRequested", "confirming unrequested divestiture");
         JavaTckSupport.expectFailure(
            () -> member.deleteObjectInstance(object, new byte[0]),
            "DeletePrivilegeNotHeld", "deleting an object without delete privilege");

         String reservedName = federation + "-named-object";
         owner.reserveObjectInstanceName(reservedName);
         await(owner, member, callbackModel,
            () -> ownerRecorder.reservations.contains(reservedName), "name reservation callback");
         owner.registerObjectInstance(ownerClass, reservedName);
         JavaTckSupport.expectFailure(
            () -> owner.registerObjectInstance(ownerClass, reservedName),
            "ObjectInstanceNameInUse", "registering a duplicate object name");
      } finally {
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
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

   private static void await(RTIambassador owner, RTIambassador member,
         CallbackModel callbackModel, java.util.function.BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            owner.evokeMultipleCallbacks(0.0, 0.01);
            member.evokeMultipleCallbacks(0.0, 0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private final Set<ObjectInstanceHandle> discoveries = ConcurrentHashMap.newKeySet();
      private final Set<String> reservations = ConcurrentHashMap.newKeySet();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("objectInstanceNameReservationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1) {
                  reservations.add((String) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK standard exception callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
