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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Ensures an acting federate receives no out-of-scope callback for its ownership acquisition. */
final class AttributeScopeOwnershipAcquisitionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final long OBSERVATION_MILLIS = 10_000L;

   private AttributeScopeOwnershipAcquisitionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder acquirerCallbacks = new Recorder();
      String federation = "java-tck-attribute-scope-ownership-acquisition-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      AttributeHandle ownerAttribute = null;
      AttributeHandle acquirerAttribute = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet acquirerAttributes = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerCallbacks.proxy(), callbackModel);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution(
            "attribute-scope-acquisition-owner", "tck-type", federation);
         ownerJoined = true;
         acquirer.joinFederationExecution(
            "attribute-scope-acquisition-acquirer", "tck-type", federation);
         acquirerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         acquirerAttribute = acquirer.getAttributeHandle(acquirerClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(acquirerClass)
               && ownerAttribute.equals(acquirerAttribute),
            "adapter DDM FOM resolved inconsistent ownership-acquisition handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         acquirerAttributes = JavaTckSupport.attributeSet(acquirer, acquirerAttribute);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         JavaTckSupport.check(!acquirer.getAttributeScopeAdvisorySwitch(),
            "Attribute Scope Advisory Switch did not start disabled");
         acquirer.setAttributeScopeAdvisorySwitch(true);
         JavaTckSupport.check(acquirer.getAttributeScopeAdvisorySwitch(),
            "Attribute Scope Advisory Switch did not enable for the acquiring federate");
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);

         object = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle registeredObject = object;
         await(acquirer, callbackModel,
            () -> acquirerCallbacks.discoveries.contains(registeredObject),
            "ownership-acquisition object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "the attribute was not initially owned by the other federate");

         settle(acquirer, callbackModel);
         acquirerCallbacks.clearScopeAdvisories();

         byte[] emptyTag = new byte[0];
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, emptyTag);
         acquirer.attributeOwnershipAcquisition(object, acquirerAttributes, emptyTag);
         await(owner, callbackModel,
            () -> !ownerCallbacks.divestitureConfirmations.isEmpty(),
            "ownership-acquisition request at the current owner");
         checkEvent(ownerCallbacks.divestitureConfirmations.get(0), object, ownerAttributes,
            "divestiture confirmation request");
         owner.confirmDivestiture(object, ownerAttributes, emptyTag);
         await(acquirer, callbackModel,
            () -> !acquirerCallbacks.acquisitions.isEmpty(),
            "ownership-acquisition notification");
         checkEvent(acquirerCallbacks.acquisitions.get(0), object, acquirerAttributes,
            "ownership-acquisition notification");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "ownership acquisition did not transfer the attribute to the acting federate");

         long observationDeadline = System.nanoTime()
               + OBSERVATION_MILLIS * 1_000_000L;
         while (System.nanoTime() < observationDeadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               while (acquirer.evokeMultipleCallbacks(0.0, 0.0)) { }
            } else {
               Thread.sleep(1L);
            }
         }
         JavaTckSupport.check(acquirerCallbacks.attributesOutOfScope.isEmpty(),
            "ownership acquisition emitted an out-of-scope advisory to the acting federate");
      } finally {
         if (object != null && acquirerJoined && acquirerAttributes != null) {
            try {
               acquirer.unconditionalAttributeOwnershipDivestiture(
                  object, acquirerAttributes, new byte[0]);
            } catch (Exception ignored) { }
         }
         if (object != null && ownerJoined) {
            try { owner.deleteObjectInstance(object, new byte[0]); }
            catch (Exception ignored) { }
         }
         if (acquirerJoined) {
            try { acquirer.resignFederationExecution(
               ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (acquirerConnected) {
            try { acquirer.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            while (ambassador.evokeMultipleCallbacks(0.0, 0.0)) { }
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         while (ambassador.evokeMultipleCallbacks(0.0, 0.0)) { }
      } else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, String description) {
      JavaTckSupport.check(event.object.equals(object) && event.attributes.equals(attributes),
         description + " identified the wrong object or attributes");
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = attributes;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureConfirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> attributesOutOfScope = new CopyOnWriteArrayList<>();

      private void clearScopeAdvisories() {
         attributesOutOfScope.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestDivestitureConfirmation".equals(name)) {
                  divestitureConfirmations.add(new OwnershipEvent(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(new OwnershipEvent(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1]));
               } else if ("attributesOutOfScope".equals(name)) {
                  attributesOutOfScope.add(new OwnershipEvent(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership scope callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
