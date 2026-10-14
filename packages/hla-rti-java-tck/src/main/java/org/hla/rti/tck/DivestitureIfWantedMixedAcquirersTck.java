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
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Transfers separately requested attributes to regular and If Available candidates. */
final class DivestitureIfWantedMixedAcquirersTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private DivestitureIfWantedMixedAcquirersTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-divestiture-if-wanted-mixed-" + suffix;
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         connected[1] = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         connected[2] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-if-wanted-owner-" + suffix, federation);
         joined[0] = true;
         first.joinFederationExecution("java-tck-if-wanted-first-" + suffix, federation);
         joined[1] = true;
         second.joinFederationExecution("java-tck-if-wanted-second-" + suffix, federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && firstClass != null && secondClass != null
               && ownerClass.equals(firstClass) && ownerClass.equals(secondClass),
            "adapter multi-attribute FOM did not resolve the same object class");
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, firstAttributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(
            secondClass, secondAttributeName);
         JavaTckSupport.check(ownerClass != null && firstClass != null && secondClass != null
               && ownerFirst != null && ownerSecond != null
               && firstAttribute != null && secondAttribute != null
               && ownerClass.equals(firstClass) && ownerClass.equals(secondClass)
               && !ownerFirst.equals(ownerSecond)
               && ownerFirst.equals(firstAttribute) && ownerSecond.equals(secondAttribute),
            "adapter multi-attribute FOM did not resolve two distinct matching attributes");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerFirst, ownerSecond);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributes =
            JavaTckSupport.attributeSet(second, secondAttribute);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         first.publishObjectClassAttributes(firstClass, firstAttributes);
         second.publishObjectClassAttributes(secondClass, secondAttributes);
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned a null handle");
         await(first, callbackModel, () -> firstRecorder.discoveries.contains(object),
            "first requester did not discover the object");
         await(second, callbackModel, () -> secondRecorder.discoveries.contains(object),
            "second requester did not discover the object");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond),
            "owner did not initially own both attributes");

         AttributeHandleSet emptyDivested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, new byte[0]);
         JavaTckSupport.check(emptyDivested != null && emptyDivested.isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond),
            "If Wanted changed ownership or returned attributes without pending acquirers");

         ownerRecorder.clearOwnershipEvents();
         firstRecorder.clearOwnershipEvents();
         secondRecorder.clearOwnershipEvents();
         byte[] firstAcquisitionTag = {(byte) 0xa1, 0x25};
         byte[] secondAcquisitionTag = {(byte) 0xa2, 0x25};
         byte[] divestitureTag = {(byte) 0xd1, 0x25};
         first.attributeOwnershipAcquisition(object, firstAttributes, firstAcquisitionTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            second.attributeOwnershipAcquisitionIfAvailable(
               object, secondAttributes, secondAcquisitionTag);
         } else {
            second.attributeOwnershipAcquisitionIfAvailable(
               object, secondAttributes, secondAcquisitionTag);
            await(second, callbackModel, () -> !secondRecorder.unavailable.isEmpty(),
               "immediate If Available unavailable result");
            JavaTckSupport.check(secondRecorder.unavailable.size() == 1,
               "immediate If Available returned an unexpected unavailable-callback count");
            checkEvent(secondRecorder.unavailable.get(0), object, secondAttributes,
               secondAcquisitionTag, "immediate If Available unavailable result");
            second.attributeOwnershipAcquisition(object, secondAttributes, secondAcquisitionTag);
         }

         // Any synchronous release/unavailable result belongs to the request phase. The
         // transfer assertions below are specifically about work left stale after divestiture.
         ownerRecorder.clearOwnershipEvents();
         firstRecorder.clearOwnershipEvents();
         secondRecorder.clearOwnershipEvents();
         AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(divested != null && divested.equals(ownerAttributes),
            "If Wanted did not transfer exactly the attributes with pending acquirers");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerFirst)
               && !owner.isAttributeOwnedByFederate(object, ownerSecond)
               && first.isAttributeOwnedByFederate(object, firstAttribute)
               && second.isAttributeOwnedByFederate(object, secondAttribute),
            "If Wanted did not transfer each requested attribute to its candidate");

         await(first, callbackModel, () -> !firstRecorder.acquisitions.isEmpty(),
            "first candidate acquisition notification");
         await(second, callbackModel, () -> !secondRecorder.acquisitions.isEmpty(),
            "second candidate acquisition notification");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         }
         checkEvent(firstRecorder.acquisitions.get(0), object, firstAttributes, divestitureTag,
            "first candidate acquisition notification");
         checkEvent(secondRecorder.acquisitions.get(0), object, secondAttributes, divestitureTag,
            "second candidate acquisition notification");
         JavaTckSupport.check(firstRecorder.acquisitions.size() == 1
               && secondRecorder.acquisitions.size() == 1
               && firstRecorder.unavailable.isEmpty()
               && secondRecorder.unavailable.isEmpty(),
            "mixed-acquirer transfer delivered duplicate or stale requester callbacks");

         serviceOwner(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.releaseRequests.isEmpty(),
            "If Wanted delivered a stale owner-release callback after transfer");

         first.unpublishObjectClassAttributes(firstClass, firstAttributes);
         second.unpublishObjectClassAttributes(secondClass, secondAttributes);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         first.unsubscribeObjectClassAttributes(firstClass, firstAttributes);
         second.unsubscribeObjectClassAttributes(secondClass, secondAttributes);
         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) resign(first, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (joined[2]) resign(second, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (joined[0]) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = 2; i >= 0; --i) {
            if (connected[i]) {
               try { ambassador(i, owner, first, second).disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static RTIambassador ambassador(int index, RTIambassador owner,
         RTIambassador first, RTIambassador second) {
      return index == 0 ? owner : index == 1 ? first : second;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void serviceOwner(RTIambassador owner, CallbackModel callbackModel,
         String objectClassName) throws Exception {
      for (int i = 0; i < 8; ++i) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         else {
            owner.getObjectClassHandle(objectClassName);
            Thread.yield();
         }
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

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object)
            && new HashSet<>(attributes).equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
   }

   private static OwnershipEvent event(Object[] arguments) {
      return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
         (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> unavailable = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(event(arguments));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length == 2) {
                  unavailable.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  releaseRequests.add(event(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK mixed-acquirer callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         acquisitions.clear();
         unavailable.clear();
         releaseRequests.clear();
      }
   }
}
