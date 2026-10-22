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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Standard-API owner, unowned, and second-federate owner query sequence. */
final class OwnershipReacquisitionQueryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipReacquisitionQueryTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel model) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder acquirerEvents = new Recorder();
      String federation = "java-tck-ownership-reacquisition-query-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      try {
         owner.connect(ownerEvents.proxy(), model);
         ownerConnected = true;
         acquirer.connect(acquirerEvents.proxy(), model);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle originalOwner = owner.joinFederationExecution(
            "java-tck-reacquisition-owner", federation);
         ownerJoined = true;
         FederateHandle newOwner = acquirer.joinFederationExecution(
            "java-tck-reacquisition-acquirer", federation);
         acquirerJoined = true;
         JavaTckSupport.check(!originalOwner.equals(newOwner),
            "reacquisition fixture returned the same federate handle twice");

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle acquirerAttribute = acquirer.getAttributeHandle(
            acquirerClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(acquirerClass)
               && ownerAttribute.equals(acquirerAttribute),
            "reacquisition fixture did not resolve matching standard handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet acquirerAttributes = JavaTckSupport.attributeSet(
            acquirer, acquirerAttribute);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "reacquisition registration returned a null object handle");
         await(acquirer, model, () -> acquirerEvents.discoveries.contains(object),
            "reacquisition object discovery");
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);

         owner.queryAttributeOwnership(object, ownerAttributes);
         await(owner, model, () -> !ownerEvents.information.isEmpty(),
            "original owner report");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         checkOwner(ownerEvents, object, ownerAttributes, originalOwner,
            "original owner query");

         owner.unconditionalAttributeOwnershipDivestiture(
            object, ownerAttributes, new byte[] {0x54, (byte) 0xA7});
         ownerEvents.clearOwnership();
         owner.queryAttributeOwnership(object, ownerAttributes);
         await(owner, model, () -> !ownerEvents.notOwned.isEmpty(),
            "unowned attribute report");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerEvents.notOwned.size() == 1
               && ownerEvents.information.isEmpty(),
            "query after divestiture did not exclusively report unowned");
         AttributeEvent unowned = ownerEvents.notOwned.get(0);
         JavaTckSupport.check(object.equals(unowned.object)
               && ownerAttributes.equals(unowned.attributes),
            "unowned callback reported the wrong object or attributes");

         acquirer.attributeOwnershipAcquisition(
            object, acquirerAttributes, new byte[] {(byte) 0xA7, 0x0C});
         await(acquirer, model, () -> !acquirerEvents.acquisitions.isEmpty(),
            "second-federate acquisition notification");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(acquirer);
         JavaTckSupport.check(acquirerEvents.acquisitions.size() == 1,
            "second federate did not receive exactly one acquisition notification");
         AttributeEvent acquisition = acquirerEvents.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquirerAttributes.equals(acquisition.attributes),
            "acquisition notification reported the wrong object or attributes");

         ownerEvents.clearOwnership();
         owner.queryAttributeOwnership(object, ownerAttributes);
         await(owner, model, () -> !ownerEvents.information.isEmpty(),
            "new owner report");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         checkOwner(ownerEvents, object, ownerAttributes, newOwner,
            "post-acquisition owner query");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "ownership predicates did not follow the second-federate acquisition");
      } finally {
         if (acquirerJoined) {
            try { acquirer.resignFederationExecution(
               ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (acquirerConnected) {
            try { acquirer.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkOwner(Recorder events, ObjectInstanceHandle object,
         AttributeHandleSet attributes, FederateHandle expected, String phase) {
      JavaTckSupport.check(events.information.size() == 1 && events.notOwned.isEmpty(),
         phase + " did not exclusively identify one owner");
      OwnerReport report = events.information.get(0);
      JavaTckSupport.check(object.equals(report.object)
            && attributes.equals(report.attributes)
            && expected.equals(report.owner),
         phase + " identified the wrong object, attributes, or federate");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static class AttributeEvent {
      final ObjectInstanceHandle object;
      final AttributeHandleSet attributes;

      private AttributeEvent(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = attributes;
      }
   }

   private static final class OwnerReport extends AttributeEvent {
      private final FederateHandle owner;

      private OwnerReport(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner) {
         super(object, attributes);
         this.owner = owner;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnerReport> information = new CopyOnWriteArrayList<>();
      private final List<AttributeEvent> notOwned = new CopyOnWriteArrayList<>();
      private final List<AttributeEvent> acquisitions = new CopyOnWriteArrayList<>();

      private void clearOwnership() {
         information.clear();
         notOwned.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("informAttributeOwnership".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  information.add(new OwnerReport((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2]));
               } else if ("attributeIsNotOwned".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  notOwned.add(new AttributeEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  acquisitions.add(new AttributeEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK reacquisition callbacks";
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
