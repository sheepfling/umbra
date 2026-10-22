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
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

final class OwnershipAssumptionSearchAfterDeclineTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipAssumptionSearchAfterDeclineTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-assumption-search-after-decline-" + UUID.randomUUID();
      boolean[] connected = new boolean[3], joined = new boolean[3];
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
         owner.joinFederationExecution("java-tck-assumption-search-owner", federation);
         joined[0] = true;
         first.joinFederationExecution("java-tck-assumption-search-first", federation);
         joined[1] = true;
         second.joinFederationExecution("java-tck-assumption-search-second", federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         JavaTckSupport.check(ownerClass != null && firstClass != null && secondClass != null
               && ownerAttribute != null && firstAttribute != null && secondAttribute != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(first.getObjectClassName(firstClass))
               && objectClassName.equals(second.getObjectClassName(secondClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(first.getAttributeName(firstClass, firstAttribute))
               && attributeName.equals(second.getAttributeName(secondClass, secondAttribute)),
            "adapter FOM did not resolve assumption-search class and attribute handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributes = JavaTckSupport.attributeSet(
            second, secondAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "assumption-search registration returned a null object handle");
         String objectName = owner.getObjectInstanceName(object);
         await(first, callbackModel, () -> firstRecorder.discoveries.size() == 1,
            "first assumption candidate discovery");
         ObjectInstanceHandle firstObject = first.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(firstObject.equals(firstRecorder.discoveries.get(0)),
            "first candidate discovery did not resolve to its local object handle");
         await(second, callbackModel, () -> secondRecorder.discoveries.size() == 1,
            "second assumption candidate discovery");
         ObjectInstanceHandle secondObject = second.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(secondObject.equals(secondRecorder.discoveries.get(0)),
            "second candidate discovery did not resolve to its local object handle");
         first.publishObjectClassAttributes(firstClass, firstAttributes);
         second.publishObjectClassAttributes(secondClass, secondAttributes);

         byte[] acquisitionTag = new byte[] {(byte) 0xc1, 0x25};
         byte[] divestitureTag = new byte[] {(byte) 0xc2, 0x25};
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) second.disableCallbacks();
         second.attributeOwnershipAcquisition(secondObject, secondAttributes, acquisitionTag);
         second.cancelAttributeOwnershipAcquisition(secondObject, secondAttributes);

         CountDownLatch firstEntered = new CountDownLatch(1);
         CountDownLatch cancellationDelivered = new CountDownLatch(1);
         AtomicBoolean firstHookFinished = new AtomicBoolean();
         AtomicBoolean secondAssumptionBeforeFirstReturn = new AtomicBoolean();
         AtomicReference<Throwable> secondError = new AtomicReference<>();
         firstRecorder.assumptionHook.set(() -> {
            firstEntered.countDown();
            try {
               if (!cancellationDelivered.await(5, TimeUnit.SECONDS)) {
                  throw new IllegalStateException("second cancellation callback did not finish");
               }
               secondAssumptionBeforeFirstReturn.set(!secondRecorder.assumptions.isEmpty());
               firstHookFinished.set(true);
            } catch (InterruptedException interrupted) {
               Thread.currentThread().interrupt();
               throw new IllegalStateException("assumption callback was interrupted", interrupted);
            }
         });
         Thread secondCallbackThread = new Thread(() -> {
            try {
               if (!firstEntered.await(5, TimeUnit.SECONDS)) {
                  throw new IllegalStateException("first assumption callback did not enter");
               }
               if (callbackModel == CallbackModel.HLA_IMMEDIATE) second.enableCallbacks();
               await(second, callbackModel, () -> !secondRecorder.cancellations.isEmpty(),
                  "second candidate cancellation confirmation");
            } catch (Throwable failure) {
               secondError.set(failure);
            } finally {
               cancellationDelivered.countDown();
            }
         }, "java-tck-assumption-cancellation-callback");
         secondCallbackThread.start();
         Throwable firstError = null;
         try {
            owner.unconditionalAttributeOwnershipDivestiture(
               object, ownerAttributes, divestitureTag);
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               JavaTckSupport.check(secondRecorder.assumptions.isEmpty(),
                  "second candidate received an assumption before cancellation confirmation");
            }
            await(first, callbackModel, firstHookFinished::get,
               "first ownership-assumption callback return");
         } catch (Throwable failure) {
            firstError = failure;
         }
         secondCallbackThread.join(10_000L);
         firstRecorder.assumptionHook.set(null);
         JavaTckSupport.check(!secondCallbackThread.isAlive(),
            "second cancellation callback thread did not finish");
         if (firstError != null) throw new IllegalStateException(
            "first assumption callback failed", firstError);
         if (secondError.get() != null) throw new IllegalStateException(
            "second cancellation callback failed", secondError.get());
         JavaTckSupport.check(firstEntered.getCount() == 0
               && cancellationDelivered.getCount() == 0
               && firstHookFinished.get()
               && !secondAssumptionBeforeFirstReturn.get()
               && firstRecorder.assumptions.size() == 1
               && secondRecorder.cancellations.size() == 1
               && (callbackModel == CallbackModel.HLA_IMMEDIATE
                  || secondRecorder.assumptions.isEmpty()),
            "callback-boundary cancellation or first assumption occurred out of order");
         OwnershipEvent firstOffer = firstRecorder.assumptions.get(0);
         JavaTckSupport.check(firstObject.equals(firstOffer.object)
               && firstOffer.attributes.equals(new HashSet<>(firstAttributes))
               && Arrays.equals(divestitureTag, firstOffer.tag),
            "first ownership-assumption callback lost its local object, attribute, or tag");

         await(second, callbackModel, () -> !secondRecorder.assumptions.isEmpty(),
            "continued ownership assumption after declined callback");
         OwnershipEvent continued = secondRecorder.assumptions.get(0);
         JavaTckSupport.check(secondRecorder.assumptions.size() == 1
               && secondObject.equals(continued.object)
               && continued.attributes.equals(new HashSet<>(secondAttributes))
               && Arrays.equals(divestitureTag, continued.tag),
            "continued ownership assumption lost its object, attribute, or tag");

         first.unpublishObjectClass(firstClass);
         second.unpublishObjectClass(secondClass);
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined[0] = false;
         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         second.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[2]) { try { second.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[1]) { try { first.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[0]) { try { owner.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (created) { try { second.destroyFederationExecution(federation); }
            catch (Exception ignored) { } }
         if (connected[2]) { try { second.disconnect(); } catch (Exception ignored) { } }
         if (connected[1]) { try { first.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   static void runTransferEpoch(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-assumption-transfer-epoch-" + UUID.randomUUID();
      boolean[] connected = new boolean[3], joined = new boolean[3];
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
         owner.joinFederationExecution("java-tck-epoch-owner", federation);
         joined[0] = true;
         first.joinFederationExecution("java-tck-epoch-first", federation);
         joined[1] = true;
         second.joinFederationExecution("java-tck-epoch-second", federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         AttributeHandle privilege = owner.getAttributeHandle(
            ownerClass, "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(ownerClass != null && firstClass != null && secondClass != null
               && ownerAttribute != null && firstAttribute != null && secondAttribute != null
               && privilege != null,
            "adapter FOM did not resolve transfer-epoch class and attribute handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributes = JavaTckSupport.attributeSet(second, secondAttribute);
         AttributeHandleSet transferred = JavaTckSupport.attributeSet(
            owner, ownerAttribute, privilege);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null, "transfer-epoch registration returned null");
         await(first, callbackModel, () -> firstRecorder.discoveries.contains(object),
            "first transfer-epoch candidate discovery");
         await(second, callbackModel, () -> secondRecorder.discoveries.contains(object),
            "second transfer-epoch candidate discovery");
         first.publishObjectClassAttributes(firstClass, firstAttributes);
         second.publishObjectClassAttributes(secondClass, secondAttributes);

         byte[] firstTag = new byte[] {(byte) 0xe1, 0x25};
         byte[] secondTag = new byte[] {(byte) 0xe2, 0x25};
         byte[] acquisitionTag = new byte[] {(byte) 0xe3, 0x25};
         owner.unconditionalAttributeOwnershipDivestiture(object, transferred, firstTag);
         await(first, callbackModel, () -> firstRecorder.assumptions.size() == 1,
            "first transfer assumption");
         await(second, callbackModel, () -> secondRecorder.assumptions.size() == 1,
            "second transfer assumption");
         OwnershipEvent initialFirst = firstRecorder.assumptions.get(0);
         OwnershipEvent initialSecond = secondRecorder.assumptions.get(0);
         Set<AttributeHandle> expected = new HashSet<>(transferred);
         JavaTckSupport.check(object.equals(initialFirst.object)
               && object.equals(initialSecond.object)
               && expected.equals(initialFirst.attributes)
               && expected.equals(initialSecond.attributes)
               && Arrays.equals(firstTag, initialFirst.tag)
               && Arrays.equals(firstTag, initialSecond.tag),
            "initial transfer did not offer both candidates the original tag");

         first.attributeOwnershipAcquisition(object, transferred, acquisitionTag);
         await(first, callbackModel, () -> firstRecorder.acquisitions.size() == 1,
            "first candidate acquisition notification");
         OwnershipEvent acquisition = firstRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && expected.equals(acquisition.attributes)
               && first.isAttributeOwnedByFederate(object, firstAttribute),
            "first candidate did not acquire the transferred attributes");
         second.unpublishObjectClass(secondClass);
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         joined[0] = false;
         first.unconditionalAttributeOwnershipDivestiture(object, transferred, secondTag);
         second.publishObjectClassAttributes(secondClass, secondAttributes);
         await(second, callbackModel, () -> secondRecorder.assumptions.size() == 2,
            "fresh assumption after transfer");
         OwnershipEvent fresh = secondRecorder.assumptions.get(1);
         JavaTckSupport.check(secondRecorder.assumptions.size() == 2
               && object.equals(fresh.object) && expected.equals(fresh.attributes)
               && Arrays.equals(secondTag, fresh.tag),
            "old reservation suppressed a fresh tagged transfer-epoch offer");

         second.unpublishObjectClass(secondClass);
         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         second.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[2]) { try { second.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[1]) { try { first.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[0]) { try { owner.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (created) { try { second.destroyFederationExecution(federation); }
            catch (Exception ignored) { } }
         if (connected[2]) { try { second.disconnect(); } catch (Exception ignored) { } }
         if (connected[1]) { try { first.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final AtomicReference<Runnable> assumptionHook = new AtomicReference<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
                  Runnable hook = assumptionHook.get();
                  if (hook != null) hook.run();
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)
                     && arguments != null) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], new byte[0]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK assumption search callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
