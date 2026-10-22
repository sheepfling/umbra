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

/** Checks all three replies to one cancellation over disjoint attribute subsets. */
final class OwnershipAcquisitionCancellationMixedRepliesTck {
   private static final long CALLBACK_TIMEOUT_SECONDS = 10;

   private OwnershipAcquisitionCancellationMixedRepliesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String cancelledName, String transferredName, String deniedName,
         String federateType, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "mixed ownership-cancellation replies require evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-mixed-cancellation-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      Thread canceller = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("mixed-cancellation-owner", federateType, federation);
         ownerJoined = true;
         requester.joinFederationExecution(
            "mixed-cancellation-requester", federateType, federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && ownerClass.equals(requesterClass),
            "mixed cancellation did not resolve one matching object class");
         AttributeHandle ownerCancelled = owner.getAttributeHandle(ownerClass, cancelledName);
         AttributeHandle ownerTransferred = owner.getAttributeHandle(ownerClass, transferredName);
         AttributeHandle ownerDenied = owner.getAttributeHandle(ownerClass, deniedName);
         AttributeHandle requesterCancelled = requester.getAttributeHandle(
            requesterClass, cancelledName);
         AttributeHandle requesterTransferred = requester.getAttributeHandle(
            requesterClass, transferredName);
         AttributeHandle requesterDenied = requester.getAttributeHandle(
            requesterClass, deniedName);
         JavaTckSupport.check(ownerCancelled != null && ownerTransferred != null
               && ownerDenied != null && ownerCancelled.equals(requesterCancelled)
               && ownerTransferred.equals(requesterTransferred)
               && ownerDenied.equals(requesterDenied)
               && !ownerCancelled.equals(ownerTransferred)
               && !ownerCancelled.equals(ownerDenied)
               && !ownerTransferred.equals(ownerDenied),
            "mixed cancellation requires three distinct matching attributes");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerCancelled, ownerTransferred, ownerDenied);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterCancelled, requesterTransferred, requesterDenied);
         AttributeHandleSet cancelled = JavaTckSupport.attributeSet(requester, requesterCancelled);
         AttributeHandleSet transferred = JavaTckSupport.attributeSet(
            requester, requesterTransferred);
         AttributeHandleSet denied = JavaTckSupport.attributeSet(requester, requesterDenied);
         JavaTckSupport.check(ownerAttributes.equals(requesterAttributes)
               && requesterAttributes.size() == 3,
            "mixed cancellation did not resolve the same three-attribute set");

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "mixed cancellation registration returned no object handle");
         await(requester, () -> requesterRecorder.discoveries.contains(object),
            "mixed cancellation object discovery");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         for (int pass = 0; pass < 8; ++pass) {
            JavaTckSupport.drain(owner);
            JavaTckSupport.drain(requester);
         }
         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();

         byte[] acquisitionTag = {(byte) 0xca, 0x31, 0x01};
         byte[] transferTag = {(byte) 0xd1, 0x31, 0x02};
         byte[] denialTag = {(byte) 0xde, 0x31, 0x03};
         CountDownLatch releaseEntered = new CountDownLatch(1);
         CountDownLatch cancellationCompleted = new CountDownLatch(1);
         AtomicReference<Throwable> cancellationFailure = new AtomicReference<>();
         AtomicReference<Throwable> ownerResolutionFailure = new AtomicReference<>();
         ownerRecorder.setReleaseHandler(event -> {
            releaseEntered.countDown();
            try {
               checkEvent(event, object, ownerAttributes, acquisitionTag,
                  "mixed cancellation ownership-release request");
               if (!cancellationCompleted.await(CALLBACK_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
                  throw new AssertionError(
                     "mixed cancellation did not complete inside the owner release callback");
               }
               AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
                  object, transferred, transferTag);
               JavaTckSupport.check(transferred.equals(divested),
                  "mixed cancellation did not transfer only the selected attribute");
               owner.attributeOwnershipReleaseDenied(object, denied, denialTag);
            } catch (Throwable failure) {
               ownerResolutionFailure.set(failure);
            }
         });
         canceller = new Thread(() -> {
            try {
               if (!releaseEntered.await(CALLBACK_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
                  throw new AssertionError("mixed cancellation release callback was not entered");
               }
               requester.cancelAttributeOwnershipAcquisition(object, requesterAttributes);
            } catch (Throwable failure) {
               cancellationFailure.set(failure);
            } finally {
               cancellationCompleted.countDown();
            }
         }, "java-tck-mixed-ownership-canceller");
         canceller.start();
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         await(owner, () -> releaseEntered.getCount() == 0,
            "mixed cancellation ownership-release callback");
         canceller.join(TimeUnit.SECONDS.toMillis(CALLBACK_TIMEOUT_SECONDS));
         JavaTckSupport.check(!canceller.isAlive(),
            "mixed cancellation worker did not finish");
         ownerRecorder.setReleaseHandler(null);
         JavaTckSupport.check(cancellationFailure.get() == null,
            "mixed cancellation request failed: " + cancellationFailure.get());
         JavaTckSupport.check(ownerResolutionFailure.get() == null,
            "mixed cancellation owner resolution failed: " + ownerResolutionFailure.get());

         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "mixed cancellation produced an unexpected ownership-release count");
         checkEvent(ownerRecorder.releaseRequests.get(0), object, ownerAttributes,
            acquisitionTag, "mixed cancellation ownership-release request");
         try {
            await(requester, () -> !requesterRecorder.cancellations.isEmpty()
                  && !requesterRecorder.acquisitions.isEmpty()
                  && !requesterRecorder.unavailabilities.isEmpty(),
               "all three mixed cancellation reply forms");
         } catch (AssertionError failure) {
            throw new AssertionError(failure.getMessage()
               + "; observed reply counts: cancellation="
               + requesterRecorder.cancellations.size() + ", acquisition="
               + requesterRecorder.acquisitions.size() + ", unavailable="
               + requesterRecorder.unavailabilities.size()
               + "; final ownership: cancelled-owner="
               + owner.isAttributeOwnedByFederate(object, ownerCancelled)
               + ", cancelled-requester="
               + requester.isAttributeOwnedByFederate(object, requesterCancelled)
               + ", transferred-requester="
               + requester.isAttributeOwnedByFederate(object, requesterTransferred)
               + ", denied-owner="
               + owner.isAttributeOwnedByFederate(object, ownerDenied), failure);
         }
         JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterRecorder.cancellations.size() == 1
               && requesterRecorder.acquisitions.size() == 1
               && requesterRecorder.unavailabilities.size() == 1,
            "one cancellation call did not produce exactly one of each reply form");
         checkEvent(requesterRecorder.cancellations.get(0), object, cancelled,
            null, "mixed cancellation confirmation");
         checkEvent(requesterRecorder.acquisitions.get(0), object, transferred,
            transferTag, "mixed cancellation acquisition notification");
         checkEvent(requesterRecorder.unavailabilities.get(0), object, denied,
            denialTag, "mixed cancellation unavailable reply");
         AttributeHandleSet observed = JavaTckSupport.attributeSet(requester);
         observed.addAll(requesterRecorder.cancellations.get(0).attributes);
         observed.addAll(requesterRecorder.acquisitions.get(0).attributes);
         observed.addAll(requesterRecorder.unavailabilities.get(0).attributes);
         JavaTckSupport.check(observed.equals(requesterAttributes),
            "three mixed cancellation replies did not cover exactly the requested attributes");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerCancelled)
               && requester.isAttributeOwnedByFederate(object, requesterTransferred)
               && owner.isAttributeOwnedByFederate(object, ownerDenied)
               && !requester.isAttributeOwnedByFederate(object, requesterCancelled)
               && !requester.isAttributeOwnedByFederate(object, requesterDenied),
            "mixed cancellation replies disagree with final ownership");
      } finally {
         if (canceller != null && canceller.isAlive()) {
            canceller.interrupt();
            canceller.join(TimeUnit.SECONDS.toMillis(CALLBACK_TIMEOUT_SECONDS));
         }
         ownerRecorder.setReleaseHandler(null);
         if (ownerJoined) {
            try { owner.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(
               ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(CALLBACK_TIMEOUT_SECONDS);
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         JavaTckSupport.drain(ambassador);
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static void checkEvent(Event event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object)
            && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attribute set, or tag");
   }

   @FunctionalInterface
   private interface ReleaseHandler {
      void handle(Event event);
   }

   private static final class Event {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private Event(ObjectInstanceHandle object, AttributeHandleSet attributes, byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Event> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<Event> acquisitions = new CopyOnWriteArrayList<>();
      private final List<Event> unavailabilities = new CopyOnWriteArrayList<>();
      private final List<Event> cancellations = new CopyOnWriteArrayList<>();
      private volatile ReleaseHandler releaseHandler;

      private void setReleaseHandler(ReleaseHandler handler) {
         releaseHandler = handler;
      }

      private void clearOwnershipEvents() {
         releaseRequests.clear();
         acquisitions.clear();
         unavailabilities.clear();
         cancellations.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  Event event = event(arguments);
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
                  cancellations.add(new Event((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK mixed cancellation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private static Event event(Object[] arguments) {
         return new Event((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
      }
   }
}
