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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Local forgetting and public evidence of orphan object deletion. */
final class LocalDeleteObjectInstanceNameLookupBoundaryTck {
   private static final long CALLBACK_TIMEOUT_MILLIS = 3000L;

   private LocalDeleteObjectInstanceNameLookupBoundaryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel model) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-local-del-name-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean ownerPublished = false;
      boolean memberSubscribed = false;
      boolean ownerReservedName = false;
      boolean memberReservedName = false;
      String reservedOwnerName = null;
      String reservedMemberName = null;
      ObjectInstanceHandle object = null;
      ObjectClassHandle ownerClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet memberAttributes = null;
      try {
         JavaTckSupport.check(owner.connect(ownerRecorder.proxy(), model) != null,
            "owner connect returned no ConfigurationResult");
         ownerConnected = true;
         JavaTckSupport.check(member.connect(memberRecorder.proxy(), model) != null,
            "member connect returned no ConfigurationResult");
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("local-delete-owner", "tck-type", federation);
         ownerJoined = true;
         member.joinFederationExecution("local-delete-member", "tck-type", federation);
         memberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         AttributeHandle deletePrivilege =
            owner.getAttributeHandle(ownerClass, "HLAprivilegeToDeleteObject");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute, deletePrivilege);
         memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         memberSubscribed = true;

         String objectName = "java-tck-orphan-object-" + federation;
         owner.reserveObjectInstanceName(objectName);
         ownerReservedName = true;
         reservedOwnerName = objectName;
         awaitNameReservation(owner, ownerRecorder, model, objectName);
         memberRecorder.discoveredObject = null;
         object = owner.registerObjectInstance(ownerClass, objectName);
         ownerReservedName = false;
         reservedOwnerName = null;
         JavaTckSupport.check(object != null,
            "named orphan-cleanup registration returned no object handle");
         awaitDiscovery(member, memberRecorder, model, objectName);
         ObjectInstanceHandle memberObject = memberRecorder.discoveredObject;
         JavaTckSupport.check(objectName.equals(member.getObjectInstanceName(memberObject)),
            "discovered object name did not resolve before local deletion");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && owner.isAttributeOwnedByFederate(object, deletePrivilege),
            "orphan-cleanup setup did not own both published attributes");

         member.localDeleteObjectInstance(memberObject);
         expectObjectInstanceNotKnown(member, memberObject,
            "looking up a locally forgotten object instance");
         JavaTckSupport.check(objectName.equals(owner.getObjectInstanceName(object)),
            "subscriber-local deletion changed the publisher's object identity");

         owner.unconditionalAttributeOwnershipDivestiture(object, ownerAttributes, new byte[0]);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !owner.isAttributeOwnedByFederate(object, deletePrivilege),
            "orphan-cleanup setup left an instance attribute owned");
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = false;
         owner.localDeleteObjectInstance(object);
         expectObjectInstanceNotKnown(owner, object,
            "looking up the last federate's locally forgotten object instance");
         object = null;

         member.reserveObjectInstanceName(objectName);
         memberReservedName = true;
         reservedMemberName = objectName;
         awaitNameReservation(member, memberRecorder, model, objectName);
         member.releaseObjectInstanceName(objectName);
         memberReservedName = false;
         reservedMemberName = null;

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         String resignedObjectName = "java-tck-orphan-resigned-object-" + federation;
         owner.reserveObjectInstanceName(resignedObjectName);
         ownerReservedName = true;
         reservedOwnerName = resignedObjectName;
         awaitNameReservation(owner, ownerRecorder, model, resignedObjectName);
         memberRecorder.discoveredObject = null;
         object = owner.registerObjectInstance(ownerClass, resignedObjectName);
         ownerReservedName = false;
         reservedOwnerName = null;
         JavaTckSupport.check(object != null,
            "resignation-driven orphan registration returned no object handle");
         awaitDiscovery(member, memberRecorder, model, resignedObjectName);
         memberObject = memberRecorder.discoveredObject;
         JavaTckSupport.check(resignedObjectName.equals(
               member.getObjectInstanceName(memberObject)),
            "resignation-driven discovery did not resolve to the reserved name");
         member.localDeleteObjectInstance(memberObject);
         owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         ownerJoined = false;
         ownerPublished = false;
         object = null;

         member.reserveObjectInstanceName(resignedObjectName);
         memberReservedName = true;
         reservedMemberName = resignedObjectName;
         awaitNameReservation(member, memberRecorder, model, resignedObjectName);
         member.releaseObjectInstanceName(resignedObjectName);
         memberReservedName = false;
         reservedMemberName = null;
      } finally {
         if (memberReservedName && memberJoined) {
            try { member.releaseObjectInstanceName(reservedMemberName); }
            catch (Exception ignored) { }
         }
         if (ownerReservedName && ownerJoined) {
            try { owner.releaseObjectInstanceName(reservedOwnerName); }
            catch (Exception ignored) { }
         }
         if (object != null && ownerJoined) {
            try { owner.deleteObjectInstance(object, new byte[0]); }
            catch (Exception ignored) { }
         }
         if (memberSubscribed && memberAttributes != null) {
            try { member.unsubscribeObjectClassAttributes(
               member.getObjectClassHandle(objectClassName), memberAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerPublished && ownerAttributes != null && ownerClass != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
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
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void awaitDiscovery(RTIambassador ambassador, Recorder recorder,
         CallbackModel model, String expectedName) throws Exception {
      long deadline = System.currentTimeMillis() + CALLBACK_TIMEOUT_MILLIS;
      while (!expectedName.equals(recorder.discoveredObjectName)
            && model == CallbackModel.HLA_EVOKED
            && System.currentTimeMillis() < deadline) {
         ambassador.evokeCallback(0.01);
      }
      JavaTckSupport.check(expectedName.equals(recorder.discoveredObjectName)
            && recorder.discoveredObject != null,
         "discoverObjectInstance did not report the registered object name");
   }

   private static void awaitNameReservation(RTIambassador ambassador, Recorder recorder,
         CallbackModel model, String expectedName) throws Exception {
      long deadline = System.currentTimeMillis() + CALLBACK_TIMEOUT_MILLIS;
      while (!recorder.nameReservationFinished(expectedName)
            && model == CallbackModel.HLA_EVOKED
            && System.currentTimeMillis() < deadline) {
         ambassador.evokeCallback(0.01);
      }
      JavaTckSupport.check(!recorder.objectNameReservationFailed(expectedName),
         "RTI rejected the object name that should have become available: " + expectedName);
      JavaTckSupport.check(recorder.objectNameReservationSucceeded(expectedName),
         "objectInstanceNameReservationSucceeded did not report " + expectedName);
   }

   private static void expectObjectInstanceNotKnown(RTIambassador ambassador,
         ObjectInstanceHandle object, String description) throws Exception {
      try {
         ambassador.getObjectInstanceName(object);
      } catch (Exception error) {
         JavaTckSupport.check("ObjectInstanceNotKnown".equals(
               error.getClass().getSimpleName()),
            description + " raised " + error.getClass().getSimpleName());
         return;
      }
      throw new AssertionError(description + " did not throw ObjectInstanceNotKnown");
   }

   private static final class Recorder {
      private final List<String> reservationSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> reservationFailures = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile String discoveredObjectName;

      private boolean objectNameReservationSucceeded(String name) {
         return reservationSuccesses.contains(name);
      }

      private boolean objectNameReservationFailed(String name) {
         return reservationFailures.contains(name);
      }

      private boolean nameReservationFinished(String name) {
         return objectNameReservationSucceeded(name) || objectNameReservationFailed(name);
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] { FederateAmbassador.class },
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1
                     && arguments[0] instanceof ObjectInstanceHandle) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  if (arguments.length >= 3 && arguments[2] instanceof String) {
                     discoveredObjectName = (String) arguments[2];
                  }
               }
               if ("objectInstanceNameReservationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1
                     && arguments[0] instanceof String) {
                  reservationSuccesses.add((String) arguments[0]);
               }
               if ("objectInstanceNameReservationFailed".equals(name)
                     && arguments != null && arguments.length == 1
                     && arguments[0] instanceof String) {
                  reservationFailures.add((String) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Local-delete orphan-cleanup callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
