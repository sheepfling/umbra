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
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Mirrors a pending one-attribute acquisition completed by Divestiture If Wanted. */
final class AttributeOwnershipDivestitureIfWantedTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AttributeOwnershipDivestitureIfWantedTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-if-wanted-ownership-transfer-" + suffix;
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      boolean ownerPublished = false;
      boolean acquirerPublished = false;
      boolean acquirerSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle acquirerClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle acquirerAttribute = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet acquirerAttributes = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), callbackModel);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-if-wanted-owner-" + suffix, federation);
         ownerJoined = true;
         acquirer.joinFederationExecution("java-tck-if-wanted-acquirer-" + suffix, federation);
         acquirerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && acquirerClass != null
               && ownerClass.equals(acquirerClass),
            "adapter FOM did not resolve the same If Wanted object class");
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         acquirerAttribute = acquirer.getAttributeHandle(acquirerClass, attributeName);
         JavaTckSupport.check(ownerAttribute != null && acquirerAttribute != null
               && ownerAttribute.equals(acquirerAttribute),
            "adapter FOM did not resolve the same If Wanted attribute");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         acquirerAttributes = JavaTckSupport.attributeSet(acquirer, acquirerAttribute);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerPublished = true;
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerSubscribed = true;
         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "If Wanted registration returned a null object handle");
         ObjectInstanceHandle registeredObject = object;
         await(acquirer, callbackModel,
            () -> acquirerRecorder.discoveries.contains(registeredObject),
            "If Wanted object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "If Wanted setup did not establish initial ownership");

         byte[] acquisitionTag = {(byte) 0xa4, 0x61};
         byte[] divestitureTag = {(byte) 0xd4, 0x62};
         acquirer.attributeOwnershipAcquisition(object, acquirerAttributes, acquisitionTag);
         await(owner, callbackModel, () -> !ownerRecorder.releaseRequests.isEmpty(),
            "ordinary acquisition release request");
         drainIfEvoked(callbackModel, owner, acquirer);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "ordinary acquisition did not produce exactly one release request");
         checkEvent(ownerRecorder.releaseRequests.get(0), object,
            new HashSet<>(ownerAttributes), acquisitionTag,
            "ordinary acquisition release request");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "pending If Wanted acquisition changed ownership before divestiture");

         AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(divested != null
               && new HashSet<>(divested).equals(new HashSet<>(ownerAttributes)),
            "If Wanted did not return the complete transferred attribute set");
         await(acquirer, callbackModel, () -> !acquirerRecorder.acquisitions.isEmpty(),
            "If Wanted acquisition notification");
         drainIfEvoked(callbackModel, owner, acquirer);
         JavaTckSupport.check(acquirerRecorder.acquisitions.size() == 1,
            "If Wanted produced more than one acquisition notification");
         checkEvent(acquirerRecorder.acquisitions.get(0), object,
            new HashSet<>(acquirerAttributes), divestitureTag,
            "If Wanted acquisition notification");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "If Wanted did not transfer ownership to the pending acquirer");

         byte[] cleanupDivestitureTag = {(byte) 0xc1, 0x01};
         byte[] cleanupAcquisitionTag = {(byte) 0xc2, 0x02};
         acquirerRecorder.acquisitions.clear();
         ownerRecorder.assumptions.clear();
         ownerRecorder.acquisitions.clear();
         acquirer.unconditionalAttributeOwnershipDivestiture(
            object, acquirerAttributes, cleanupDivestitureTag);
         await(owner, callbackModel, () -> !ownerRecorder.assumptions.isEmpty(),
            "cleanup ownership-assumption callback");
         drainIfEvoked(callbackModel, owner, acquirer);
         owner.attributeOwnershipAcquisitionIfAvailable(
            object, ownerAttributes, cleanupAcquisitionTag);
         await(owner, callbackModel, () -> !ownerRecorder.acquisitions.isEmpty(),
            "cleanup ownership reacquisition");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "cleanup did not restore owner ownership");
      } finally {
         if (acquirerJoined && acquirerSubscribed && acquirerClass != null
               && acquirerAttributes != null) {
            try { acquirer.unsubscribeObjectClassAttributes(acquirerClass, acquirerAttributes); }
            catch (Exception ignored) { }
         }
         unpublishIfJoined(owner, ownerJoined, ownerPublished, ownerClass, ownerAttributes);
         unpublishIfJoined(acquirer, acquirerJoined, acquirerPublished,
            acquirerClass, acquirerAttributes);
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

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         HashSet<AttributeHandle> attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
   }

   private static void unpublishIfJoined(RTIambassador ambassador, boolean joined,
         boolean published, ObjectClassHandle objectClass, AttributeHandleSet attributes) {
      if (joined && published && objectClass != null && attributes != null) {
         try { ambassador.unpublishObjectClassAttributes(objectClass, attributes); }
         catch (Exception ignored) { }
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

   private static void drainIfEvoked(CallbackModel model, RTIambassador... ambassadors)
         throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
      }
   }

   private static OwnershipEvent ownershipEvent(Object[] arguments) {
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
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

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
                  releaseRequests.add(ownershipEvent(arguments));
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length == 3) {
                  assumptions.add(ownershipEvent(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(ownershipEvent(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK If Wanted ownership-transfer callbacks";
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
