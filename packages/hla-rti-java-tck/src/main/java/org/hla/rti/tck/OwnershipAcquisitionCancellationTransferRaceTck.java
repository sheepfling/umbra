package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

/** Resolves acquisition cancellation racing a Divestiture If Wanted ownership transfer. */
final class OwnershipAcquisitionCancellationTransferRaceTck {
   private static final long CALLBACK_TIMEOUT_SECONDS = 10;

   private OwnershipAcquisitionCancellationTransferRaceTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      runCase(factory, fom, objectClassName, attributeName, callbackModel, false);
   }

   static void runDenied(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "ownership cancellation denial race requires evoked callbacks");
      runCase(factory, fom, objectClassName, attributeName, callbackModel, true);
   }

   private static void runCase(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel, boolean denial) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-cancellation-"
         + (denial ? "denial-" : "transfer-") + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerPublished = false;
      boolean requesterPublished = false;
      boolean requesterSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle requesterClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet requesterAttributes = null;
      Thread canceller = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-ownership-race-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-ownership-race-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
          JavaTckSupport.check(ownerClass != null && requesterClass != null,
             "adapter FOM did not resolve ownership-race object classes");
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null
               && ownerAttribute.equals(requesterAttribute),
            "adapter FOM did not resolve matching ownership-race handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         final AttributeHandleSet ownerAttributeSet = ownerAttributes;
         final AttributeHandleSet requesterAttributeSet = requesterAttributes;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requesterSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
          JavaTckSupport.check(object != null,
             "ownership cancellation-race registration returned an invalid object handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "ownership cancellation-race object discovery");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;
          for (int pass = 0; pass < 8; ++pass) {
             if (callbackModel == CallbackModel.HLA_EVOKED) {
                owner.evokeCallback(0.0);
                requester.evokeCallback(0.0);
             } else {
                Thread.yield();
             }
          }

         byte[] acquisitionTag = new byte[] {(byte) 0xc1, 0x25};
         byte[] divestitureTag = new byte[] {(byte) 0xd1, 0x25};
         byte[] denialTag = new byte[] {(byte) 0xde, 0x25};
         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
            AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
               object, ownerAttributes, divestitureTag);
            JavaTckSupport.check(ownerAttributes.equals(divested),
               "immediate ownership race returned the wrong divested set");
         } else {
            CountDownLatch releaseCallbackEntered = new CountDownLatch(1);
            CountDownLatch cancellationCompleted = new CountDownLatch(1);
            AtomicReference<Throwable> cancellationFailure = new AtomicReference<>();
            AtomicReference<Throwable> divestitureFailure = new AtomicReference<>();
            ownerRecorder.setReleaseHandler(event -> {
               JavaTckSupport.check(object.equals(event.object)
                     && event.attributes.equals(ownerAttributeSet),
                  "ownership release callback returned the wrong object or attribute set");
               releaseCallbackEntered.countDown();
               if (!cancellationCompleted.await(CALLBACK_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
                  throw new AssertionError("cancellation did not complete inside release callback");
               }
               try {
                  if (denial) {
                     owner.attributeOwnershipReleaseDenied(
                        object, ownerAttributeSet, denialTag);
                  } else {
                     AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
                        object, ownerAttributeSet, divestitureTag);
                     JavaTckSupport.check(ownerAttributeSet.equals(divested),
                        "evoked ownership race returned the wrong divested set");
                  }
               } catch (Throwable failure) {
                  divestitureFailure.set(failure);
               }
            });
            canceller = new Thread(() -> {
               try {
                  if (!releaseCallbackEntered.await(CALLBACK_TIMEOUT_SECONDS,
                        TimeUnit.SECONDS)) {
                     throw new AssertionError("release callback was not entered");
                  }
                  requester.cancelAttributeOwnershipAcquisition(object, requesterAttributeSet);
               } catch (Throwable failure) {
                  cancellationFailure.set(failure);
               } finally {
                  cancellationCompleted.countDown();
               }
            }, "java-tck-ownership-cancellation-race");
            canceller.start();
            requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
            await(owner, callbackModel, () -> releaseCallbackEntered.getCount() == 0,
               "ownership cancellation-race release callback");
            canceller.join(TimeUnit.SECONDS.toMillis(CALLBACK_TIMEOUT_SECONDS));
            JavaTckSupport.check(!canceller.isAlive(),
               "ownership cancellation worker did not finish");
            ownerRecorder.setReleaseHandler(null);
            JavaTckSupport.check(cancellationFailure.get() == null,
               "ownership race cancellation failed: " + cancellationFailure.get());
            JavaTckSupport.check(divestitureFailure.get() == null,
               "ownership race divestiture failed: " + divestitureFailure.get());
         }

         await(owner, callbackModel, () -> !ownerRecorder.releaseRequests.isEmpty(),
            "ownership cancellation-race release request");
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "ownership acquisition produced an unexpected release-request count");
         checkEvent(ownerRecorder.releaseRequests.get(0), object, ownerAttributes,
            acquisitionTag, "ownership cancellation-race release request");
         await(requester, callbackModel,
            () -> denial ? !requesterRecorder.unavailabilities.isEmpty()
               : !requesterRecorder.acquisitions.isEmpty(),
            denial ? "ownership cancellation-race unavailable reply"
               : "ownership cancellation-race acquisition notification");
          if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterRecorder.cancellations.isEmpty(),
            "ownership race delivered a stale acquisition-cancellation confirmation");
         if (denial) {
            JavaTckSupport.check(requesterRecorder.unavailabilities.size() == 1
                  && requesterRecorder.acquisitions.isEmpty(),
               "ownership denial race delivered an extra or wrong acquisition reply");
            checkEvent(requesterRecorder.unavailabilities.get(0), object,
               requesterAttributes, denialTag, "ownership cancellation-race unavailable reply");
            JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
                  && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
               "ownership cancellation denial race changed the owner");
         } else {
            JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1,
               "ownership transfer delivered an unexpected acquisition-notification count");
            checkEvent(requesterRecorder.acquisitions.get(0), object,
               requesterAttributes, divestitureTag,
               "ownership cancellation-race acquisition notification");
            JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
                  && requester.isAttributeOwnedByFederate(object, requesterAttribute),
               "ownership cancellation race did not complete the transfer");
         }
      } finally {
         if (canceller != null && canceller.isAlive()) {
            canceller.interrupt();
            canceller.join(TimeUnit.SECONDS.toMillis(CALLBACK_TIMEOUT_SECONDS));
         }
         ownerRecorder.setReleaseHandler(null);
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined && requesterPublished && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
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

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(CALLBACK_TIMEOUT_SECONDS);
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface ReleaseHandler {
      void handle(OwnershipEvent event) throws Exception;
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> unavailabilities = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();
      private volatile ReleaseHandler releaseHandler;

      private void setReleaseHandler(ReleaseHandler handler) { releaseHandler = handler; }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  OwnershipEvent event = event(arguments);
                  releaseRequests.add(event);
                  ReleaseHandler handler = releaseHandler;
                  if (handler != null) handler.handle(event);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(event(arguments));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length == 3) {
                  unavailabilities.add(event(arguments));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)
                     && arguments != null && arguments.length == 2) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership cancellation race callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         releaseRequests.clear();
         acquisitions.clear();
         unavailabilities.clear();
         cancellations.clear();
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
      }
   }
}
