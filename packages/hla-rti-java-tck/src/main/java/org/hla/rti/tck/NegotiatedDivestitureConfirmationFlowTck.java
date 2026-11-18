package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Mirrors negotiated transfer and the new owner's first ordinary attribute update. */
final class NegotiatedDivestitureConfirmationFlowTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NegotiatedDivestitureConfirmationFlowTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String federation = "java-tck-negotiated-divestiture-confirmation-flow-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      boolean ownerPublished = false;
      boolean acquirerPublished = false;
      boolean ownerSubscribed = false;
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
         owner.joinFederationExecution("java-tck-negotiated-divestiture-owner", federation);
         ownerJoined = true;
         FederateHandle acquirerFederate = acquirer.joinFederationExecution(
            "java-tck-negotiated-divestiture-acquirer", federation);
         acquirerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && ownerClass.equals(acquirerClass),
            "adapter FOM did not resolve the same negotiated-divestiture object class");
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         acquirerAttribute = acquirer.getAttributeHandle(acquirerClass, attributeName);
         JavaTckSupport.check(ownerAttribute != null && ownerAttribute.equals(acquirerAttribute),
            "adapter FOM did not resolve the same negotiated-divestiture attribute");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         acquirerAttributes = JavaTckSupport.attributeSet(acquirer, acquirerAttribute);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         owner.subscribeObjectClassAttributes(ownerClass, ownerAttributes);
         ownerSubscribed = true;
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerPublished = true;
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerSubscribed = true;
         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "negotiated-divestiture registration returned a null object handle");
         ObjectInstanceHandle registeredObject = object;
         await(acquirer, callbackModel,
            () -> acquirerRecorder.discoveries.contains(registeredObject),
            "negotiated-divestiture object discovery");

         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "negotiated-divestiture setup did not establish initial ownership");

         byte[] offerTag = {0x10, (byte) 0xd1};
         byte[] acquisitionTag = {0x20, (byte) 0xa2};
         byte[] confirmationTag = {0x30, (byte) 0xc3};
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, offerTag);
         acquirer.attributeOwnershipAcquisition(object, acquirerAttributes, acquisitionTag);
         await(owner, callbackModel,
            () -> !ownerRecorder.confirmations.isEmpty(),
            "negotiated-divestiture confirmation request");
         JavaTckSupport.check(ownerRecorder.confirmations.size() == 1,
            "negotiated acquisition produced more than one confirmation request");
         OwnershipEvent request = ownerRecorder.confirmations.get(0);
         checkEvent(request, object, new HashSet<>(ownerAttributes), acquisitionTag,
            "negotiated-divestiture confirmation request");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute)
               && acquirerRecorder.acquisitions.isEmpty(),
            "negotiated divestiture transferred ownership before confirmation");

         owner.confirmDivestiture(object, ownerAttributes, confirmationTag);
         await(acquirer, callbackModel, () -> !acquirerRecorder.acquisitions.isEmpty(),
            "negotiated-divestiture acquisition notification");
         JavaTckSupport.check(acquirerRecorder.acquisitions.size() == 1,
            "confirmed negotiated divestiture produced more than one acquisition notification");
         OwnershipEvent acquisition = acquirerRecorder.acquisitions.get(0);
         checkEvent(acquisition, object, new HashSet<>(acquirerAttributes), confirmationTag,
            "negotiated-divestiture acquisition notification");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "Confirm Divestiture did not transfer ownership to the acquirer");

         byte[] updateValue = {0x51, (byte) 0x88};
         byte[] updateTag = {0x52, (byte) 0x89};
         AttributeHandleValueMap values = acquirer.getAttributeHandleValueMapFactory().create(1);
         values.put(acquirerAttribute, updateValue);
         acquirer.updateAttributeValues(object, values, updateTag);
         await(owner, callbackModel, () -> !ownerRecorder.reflections.isEmpty(),
            "new-owner update reflected to the former owner");
         JavaTckSupport.check(ownerRecorder.reflections.size() == 1,
            "new-owner update produced more than one reflection callback");
         ReflectionEvent reflection = ownerRecorder.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 1
               && reflection.values.containsKey(ownerAttribute)
               && Arrays.equals(updateValue, reflection.values.get(ownerAttribute))
               && Arrays.equals(updateTag, reflection.tag)
               && acquirerFederate.equals(reflection.producingFederate),
            "post-transfer reflection returned the wrong object, value, tag, or producer");
      } finally {
         if (acquirerJoined && object != null && acquirerAttribute != null
               && acquirerAttributes != null) {
            try {
               if (acquirer.isAttributeOwnedByFederate(object, acquirerAttribute)) {
                  acquirer.unconditionalAttributeOwnershipDivestiture(
                     object, acquirerAttributes, new byte[0]);
               }
            } catch (Exception ignored) { }
         }
         if (ownerJoined && ownerSubscribed && ownerClass != null && ownerAttributes != null) {
            try { owner.unsubscribeObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (acquirerJoined && acquirerSubscribed && acquirerClass != null
               && acquirerAttributes != null) {
            try { acquirer.unsubscribeObjectClassAttributes(acquirerClass, acquirerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (acquirerJoined && acquirerPublished && acquirerClass != null
               && acquirerAttributes != null) {
            try { acquirer.unpublishObjectClassAttributes(acquirerClass, acquirerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (acquirerJoined) {
            try { acquirer.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
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

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         HashSet<AttributeHandle> attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
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

   private static final class ReflectionEvent {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final FederateHandle producingFederate;

      private ReflectionEvent(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, FederateHandle producingFederate) {
         this.object = object;
         this.values = new HashMap<>();
         for (Map.Entry<AttributeHandle, byte[]> entry : values.entrySet()) {
            this.values.put(entry.getKey(), entry.getValue().clone());
         }
         this.tag = tag == null ? null : tag.clone();
         this.producingFederate = producingFederate;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> confirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<ReflectionEvent> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length == 3) {
                  confirmations.add(ownershipEvent(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(ownershipEvent(arguments));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 5) {
                  reflections.add(new ReflectionEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (FederateHandle) arguments[4]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK negotiated divestiture confirmation callbacks";
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
