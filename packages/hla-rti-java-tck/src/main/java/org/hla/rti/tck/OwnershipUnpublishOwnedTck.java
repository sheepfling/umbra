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

/** Selective class unpublication releases only the matching owned instance attribute. */
final class OwnershipUnpublishOwnedTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipUnpublishOwnedTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String removedAttributeName, String retainedAttributeName,
         CallbackModel model) throws Exception {
      JavaTckSupport.check(!removedAttributeName.equals(retainedAttributeName),
         "owned-attribute unpublication needs distinct adapter attributes");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder observerEvents = new Recorder();
      String federation = "java-tck-unpublish-owned-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      try {
         owner.connect(ownerEvents.proxy(), model);
         ownerConnected = true;
         observer.connect(observerEvents.proxy(), model);
         observerConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-unpublish-owner", federation);
         ownerJoined = true;
         observer.joinFederationExecution(
            "java-tck-unpublish-observer", federation);
         observerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle removed = owner.getAttributeHandle(
            ownerClass, removedAttributeName);
         AttributeHandle retained = owner.getAttributeHandle(
            ownerClass, retainedAttributeName);
         AttributeHandle observerRemoved = observer.getAttributeHandle(
            observerClass, removedAttributeName);
         AttributeHandle observerRetained = observer.getAttributeHandle(
            observerClass, retainedAttributeName);
         JavaTckSupport.check(ownerClass.equals(observerClass)
               && !removed.equals(retained)
               && removed.equals(observerRemoved)
               && retained.equals(observerRetained),
            "owned-attribute unpublication did not resolve matching standard handles");
         AttributeHandleSet ownerBoth = JavaTckSupport.attributeSet(
            owner, removed, retained);
         AttributeHandleSet removedOnly = JavaTckSupport.attributeSet(owner, removed);
         AttributeHandleSet observerBoth = JavaTckSupport.attributeSet(
            observer, observerRemoved, observerRetained);
         AttributeHandleSet observerRemovedOnly = JavaTckSupport.attributeSet(
            observer, observerRemoved);
         AttributeHandleSet observerRetainedOnly = JavaTckSupport.attributeSet(
            observer, observerRetained);
         owner.publishObjectClassAttributes(ownerClass, ownerBoth);
         observer.subscribeObjectClassAttributes(observerClass, observerBoth);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "owned-attribute unpublication registered a null object handle");
         await(observer, model, objectClassName,
            () -> observerEvents.discoveries.contains(object), "object discovery");
         JavaTckSupport.check(observerClass.equals(
               observer.getKnownObjectClassHandle(object))
               && owner.isAttributeOwnedByFederate(object, removed)
               && owner.isAttributeOwnedByFederate(object, retained),
            "registered object did not start with both attributes owned");

         owner.unpublishObjectClassAttributes(ownerClass, removedOnly);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, removed)
               && owner.isAttributeOwnedByFederate(object, retained),
            "selective unpublication did not release only the removed attribute");
         observerEvents.clearOwnership();
         observer.queryAttributeOwnership(object, observerBoth);
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(observerEvents.information.isEmpty()
                  && observerEvents.notOwned.isEmpty(),
               "evoked ownership query delivered before callback servicing");
         }
         await(observer, model, objectClassName,
            () -> !observerEvents.information.isEmpty()
               && !observerEvents.notOwned.isEmpty(),
            "selective unpublication ownership reports");
         JavaTckSupport.check(observerEvents.information.size() == 1
               && observerEvents.notOwned.size() == 1,
            "ownership query did not partition removed and retained attributes");
         AttributeEvent unowned = observerEvents.notOwned.get(0);
         OwnerReport owned = observerEvents.information.get(0);
         JavaTckSupport.check(object.equals(unowned.object)
               && observerRemovedOnly.equals(unowned.attributes)
               && object.equals(owned.object)
               && observerRetainedOnly.equals(owned.attributes)
               && ownerHandle.equals(owned.owner),
            "ownership query reported the wrong attribute or retained owner");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (observerConnected) { try { observer.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else ambassador.getObjectClassHandle(objectClassName);
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

      private OwnerReport(ObjectInstanceHandle object,
            AttributeHandleSet attributes, FederateHandle owner) {
         super(object, attributes);
         this.owner = owner;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnerReport> information = new CopyOnWriteArrayList<>();
      private final List<AttributeEvent> notOwned = new CopyOnWriteArrayList<>();

      private void clearOwnership() {
         information.clear();
         notOwned.clear();
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
               } else if ("informAttributeOwnership".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  information.add(new OwnerReport((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2]));
               } else if ("attributeIsNotOwned".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  notOwned.add(new AttributeEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK unpublish ownership callbacks";
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
