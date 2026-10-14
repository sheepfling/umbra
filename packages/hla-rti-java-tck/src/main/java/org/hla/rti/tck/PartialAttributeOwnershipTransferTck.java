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
import java.util.function.BooleanSupplier;

/** Cancels one member of a multi-attribute acquisition before divesting the remaining member. */
final class PartialAttributeOwnershipTransferTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private PartialAttributeOwnershipTransferTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName,
         String firstAttributeName, String secondAttributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String federation = "java-tck-partial-attribute-ownership-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      boolean acquirerSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle acquirerClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet acquirerAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), callbackModel);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-partial-ownership-owner", federation);
         ownerJoined = true;
         acquirer.joinFederationExecution("java-tck-partial-ownership-acquirer", federation);
         acquirerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && acquirerClass != null
               && ownerClass.equals(acquirerClass),
            "adapter multi-attribute FOM did not resolve matching object classes");
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle acquirerFirst = acquirer.getAttributeHandle(
            acquirerClass, firstAttributeName);
         AttributeHandle acquirerSecond = acquirer.getAttributeHandle(
            acquirerClass, secondAttributeName);
         JavaTckSupport.check(ownerFirst != null && ownerSecond != null
               && acquirerFirst != null && acquirerSecond != null
               && !ownerFirst.equals(ownerSecond) && !acquirerFirst.equals(acquirerSecond)
               && ownerFirst.equals(acquirerFirst) && ownerSecond.equals(acquirerSecond),
            "adapter multi-attribute FOM did not resolve two distinct attributes");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst, ownerSecond);
         acquirerAttributes = JavaTckSupport.attributeSet(acquirer, acquirerFirst, acquirerSecond);
         AttributeHandleSet ownerSecondOnly = JavaTckSupport.attributeSet(owner, ownerSecond);
         AttributeHandleSet acquirerFirstOnly = JavaTckSupport.attributeSet(acquirer, acquirerFirst);
         AttributeHandleSet acquirerSecondOnly = JavaTckSupport.attributeSet(acquirer, acquirerSecond);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned a null handle");
         await(acquirer, callbackModel, () -> acquirerRecorder.discoveries.contains(object),
            "ownership-test object discovery");
         JavaTckSupport.check(ownerRecorder.releaseRequests.isEmpty()
               && acquirerRecorder.acquisitions.isEmpty(),
            "ownership setup unexpectedly delivered ownership callbacks");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond),
            "owner did not initially own both attributes");
         JavaTckSupport.check(!acquirer.isAttributeOwnedByFederate(object, acquirerFirst)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerSecond),
            "acquirer initially owned an attribute");

         byte[] acquisitionTag = new byte[] {0x50, 0x41, 0x52};
         byte[] divestitureTag = new byte[] {0x44, 0x49, 0x56};
         acquirer.attributeOwnershipAcquisition(object, acquirerAttributes, acquisitionTag);
         await(owner, callbackModel, () -> ownerRecorder.releaseRequests.size() == 1,
            "owner release request for both attributes");
         OwnershipEvent release = ownerRecorder.releaseRequests.get(0);
         JavaTckSupport.check(object.equals(release.object)
               && release.attributes.equals(ownerAttributes)
               && Arrays.equals(acquisitionTag, release.tag),
            "release request returned the wrong object, attribute set, or acquisition tag");

         acquirer.cancelAttributeOwnershipAcquisition(object, acquirerFirstOnly);
         await(acquirer, callbackModel, () -> acquirerRecorder.cancellations.size() == 1,
            "cancellation confirmation for the first attribute");
         OwnershipEvent cancellation = acquirerRecorder.cancellations.get(0);
         JavaTckSupport.check(object.equals(cancellation.object)
               && cancellation.attributes.equals(acquirerFirstOnly),
            "cancellation confirmation returned more than the canceled attribute");

         AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(divested != null && divested.equals(ownerSecondOnly),
            "If Wanted divestiture returned more than the uncanceled attribute");
         await(acquirer, callbackModel, () -> acquirerRecorder.acquisitions.size() == 1,
            "acquisition notification for the remaining attribute");
         OwnershipEvent acquisition = acquirerRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.equals(acquirerSecondOnly)
               && Arrays.equals(divestitureTag, acquisition.tag),
            "acquisition notification returned the wrong attribute or divestiture tag");

         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && !owner.isAttributeOwnedByFederate(object, ownerSecond),
            "partial ownership transfer moved the canceled attribute or retained the other");
         JavaTckSupport.check(!acquirer.isAttributeOwnedByFederate(object, acquirerFirst)
               && acquirer.isAttributeOwnedByFederate(object, acquirerSecond),
            "partial ownership transfer did not transfer exactly the uncanceled attribute");
      } finally {
         if (acquirerJoined && acquirerSubscribed) {
            try { acquirer.unsubscribeObjectClassAttributes(acquirerClass, acquirerAttributes); }
            catch (Exception ignored) { }
         }
         if (acquirerJoined) {
            try { acquirer.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (acquirerConnected) { try { acquirer.disconnect(); } catch (Exception ignored) { } }
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
         (AttributeHandleSet) arguments[1], ((byte[]) arguments[2]).clone());
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();

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
                  releaseRequests.add(event(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(event(arguments));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)
                     && arguments != null && arguments.length == 2) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK partial ownership transfer callbacks";
                  }
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
