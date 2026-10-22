package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies mixed ownership-query result partitions and suppresses stale results on removal. */
final class OwnershipQueryPartitionCleanupTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipQueryPartitionCleanupTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String firstAttributeName, String secondAttributeName, CallbackModel callbackModel)
         throws Exception {
      runConfigured(factory, fom, objectClassName, firstAttributeName,
         secondAttributeName, callbackModel, true);
   }

   static void runNullification(RtiFactory factory, String fom, String objectClassName,
         String firstAttributeName, String secondAttributeName, CallbackModel callbackModel)
         throws Exception {
      runConfigured(factory, fom, objectClassName, firstAttributeName,
         secondAttributeName, callbackModel, false);
   }

   private static void runConfigured(RtiFactory factory, String fom, String objectClassName,
         String firstAttributeName, String secondAttributeName, CallbackModel callbackModel,
         boolean includeInvalidHandleBoundaries) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-query-partition-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerPublished = false;
      boolean requesterSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle requesterClass = null;
      AttributeHandle ownerFirst = null;
      AttributeHandle requesterFirst = null;
      AttributeHandle requesterSecond = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet requesterAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-ownership-query-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-ownership-query-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
          JavaTckSupport.check(ownerClass != null && requesterClass != null,
             "adapter multi-attribute FOM returned an invalid object-class handle");
         ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         requesterFirst = requester.getAttributeHandle(requesterClass, firstAttributeName);
         requesterSecond = requester.getAttributeHandle(requesterClass, secondAttributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerFirst != null && ownerSecond != null
               && requesterFirst != null && requesterSecond != null
               && !ownerFirst.equals(ownerSecond),
            "adapter multi-attribute FOM did not resolve two distinct ownership attributes");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst);
         requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterFirst, requesterSecond);
         final AttributeHandleSet queryAttributes = requesterAttributes;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requesterSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
          JavaTckSupport.check(object != null,
             "ownership-query registration returned an invalid object handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "ownership-query object discovery");
          JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
                && !owner.isAttributeOwnedByFederate(object, ownerSecond)
                && !requester.isAttributeOwnedByFederate(object, requesterFirst)
                && !requester.isAttributeOwnedByFederate(object, requesterSecond),
             "ownership-query setup did not establish the one-owned/one-unowned split");

         if (includeInvalidHandleBoundaries) {
            ObjectInstanceHandle invalidObject =
               JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
            AttributeHandle invalidAttribute = JavaTckSupport.invalidHandle(AttributeHandle.class);
            AttributeHandleSet invalidAttributes = JavaTckSupport.attributeSet(
               requester, invalidAttribute);
            JavaTckSupport.expectFailure(
               () -> requester.queryAttributeOwnership(invalidObject, queryAttributes),
               "ObjectInstanceNotKnown", "querying an unknown object");
            JavaTckSupport.expectFailure(
               () -> requester.queryAttributeOwnership(object, invalidAttributes),
               "AttributeNotDefined", "querying an undefined attribute");
         }

         requesterRecorder.clearOwnershipEvents();
         requester.queryAttributeOwnership(object, requesterAttributes);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requesterRecorder.information.isEmpty()
                  && requesterRecorder.notOwned.isEmpty(),
               "ownership-query result escaped before evoked callback service");
         }
         await(requester, callbackModel,
            () -> requesterRecorder.information.size() == 1
               && requesterRecorder.notOwned.size() == 1,
            "partitioned ownership-query callbacks");
         OwnershipEvent information = requesterRecorder.information.get(0);
         OwnershipEvent notOwned = requesterRecorder.notOwned.get(0);
         JavaTckSupport.check(object.equals(information.object)
               && information.attributes.equals(new HashSet<>(List.of(requesterFirst)))
               && ownerFederate.equals(information.owner),
            "ownership query returned the wrong owned subset or owner");
         JavaTckSupport.check(object.equals(notOwned.object)
               && notOwned.attributes.equals(new HashSet<>(List.of(requesterSecond))),
            "ownership query returned the wrong unowned subset");
         JavaTckSupport.check(requesterRecorder.ownedByRti.isEmpty(),
            "ordinary unowned attribute was reported as RTI-owned");
          JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
                && !owner.isAttributeOwnedByFederate(object, ownerSecond)
                && !requester.isAttributeOwnedByFederate(object, requesterFirst)
                && !requester.isAttributeOwnedByFederate(object, requesterSecond),
             "ownership query changed the object's owned/unowned state");

         requesterRecorder.clearOwnershipEvents();
         requester.queryAttributeOwnership(object, requesterAttributes);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requesterRecorder.information.isEmpty()
                  && requesterRecorder.notOwned.isEmpty(),
               "queued ownership-query result escaped before object removal");
         }
         int informationBeforeRemoval = requesterRecorder.information.size();
         int notOwnedBeforeRemoval = requesterRecorder.notOwned.size();
         owner.deleteObjectInstance(object, new byte[0]);
         await(requester, callbackModel, () -> !requesterRecorder.removals.isEmpty(),
            "object removal after ownership query");
         JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterRecorder.removals.size() == 1
               && object.equals(requesterRecorder.removals.get(0)),
            "object removal callback returned the wrong object or count");
         JavaTckSupport.check(requesterRecorder.information.size() == informationBeforeRemoval
               && requesterRecorder.notOwned.size() == notOwnedBeforeRemoval,
            "object removal delivered stale ownership-query callbacks");
      } finally {
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static OwnershipEvent event(Object[] arguments) {
      return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
         (AttributeHandleSet) arguments[1],
         arguments.length >= 3 && arguments[2] instanceof FederateHandle
            ? (FederateHandle) arguments[2] : null);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final FederateHandle owner;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.owner = owner;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> information = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> notOwned = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownedByRti = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("informAttributeOwnership".equals(name)
                     && arguments != null && arguments.length == 3) {
                  information.add(event(arguments));
               } else if ("attributeIsNotOwned".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  notOwned.add(event(arguments));
               } else if ("attributeIsOwnedByRTI".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  ownedByRti.add(event(arguments));
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership-query callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         information.clear();
         notOwned.clear();
         ownedByRti.clear();
      }
   }
}
