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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** One If Available request has independent outcomes for held and unowned attributes. */
final class OwnershipMixedIfAvailableTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipMixedIfAvailableTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String freeAttributeName, String heldAttributeName, CallbackModel model)
         throws Exception {
      JavaTckSupport.check(!freeAttributeName.equals(heldAttributeName),
         "mixed If Available requires two distinct adapter attributes");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder requesterEvents = new Recorder();
      String federation = "java-tck-mixed-if-available-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      try {
         owner.connect(ownerEvents.proxy(), model);
         ownerConnected = true;
         requester.connect(requesterEvents.proxy(), model);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-mixed-owner", federation);
         ownerJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
            "java-tck-mixed-requester", federation);
         requesterJoined = true;
         JavaTckSupport.check(!ownerHandle.equals(requesterHandle),
            "mixed If Available fixture returned duplicate federate handles");

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle free = owner.getAttributeHandle(ownerClass, freeAttributeName);
         AttributeHandle held = owner.getAttributeHandle(ownerClass, heldAttributeName);
         AttributeHandle requesterFree = requester.getAttributeHandle(
            requesterClass, freeAttributeName);
         AttributeHandle requesterHeld = requester.getAttributeHandle(
            requesterClass, heldAttributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && !free.equals(held) && free.equals(requesterFree)
               && held.equals(requesterHeld),
            "mixed If Available fixture did not resolve matching standard handles");
         AttributeHandleSet ownerBoth = JavaTckSupport.attributeSet(owner, free, held);
         AttributeHandleSet requesterBoth = JavaTckSupport.attributeSet(
            requester, requesterFree, requesterHeld);
         AttributeHandleSet freeOnly = JavaTckSupport.attributeSet(owner, free);
         AttributeHandleSet heldOnly = JavaTckSupport.attributeSet(owner, held);
         requester.subscribeObjectClassAttributes(requesterClass, requesterBoth);
         owner.publishObjectClassAttributes(ownerClass, ownerBoth);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "mixed If Available registration returned a null object handle");
         await(requester, model, objectClassName,
            () -> requesterEvents.discoveries.contains(object), "object discovery");
         requester.publishObjectClassAttributes(requesterClass, requesterBoth);

         query(owner, ownerEvents, model, objectClassName,
            object, freeOnly, ownerHandle, true);
         query(owner, ownerEvents, model, objectClassName,
            object, heldOnly, ownerHandle, true);
         owner.unconditionalAttributeOwnershipDivestiture(
            object, freeOnly, new byte[] {0x54, 0x10});
         query(owner, ownerEvents, model, objectClassName,
            object, freeOnly, ownerHandle, false);
         query(owner, ownerEvents, model, objectClassName,
            object, heldOnly, ownerHandle, true);

         ownerEvents.clearOwnership();
         requesterEvents.clearOwnership();
         byte[] tag = new byte[] {(byte) 0xA7, 0x10, 0x11};
         requester.attributeOwnershipAcquisitionIfAvailable(
            object, requesterBoth, tag);
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requesterEvents.acquisitions.isEmpty()
                  && requesterEvents.unavailable.isEmpty(),
               "evoked mixed If Available callback arrived before servicing");
         }
         await(requester, model, objectClassName,
            () -> !requesterEvents.acquisitions.isEmpty()
               && !requesterEvents.unavailable.isEmpty(),
            "mixed If Available terminal callbacks");
         JavaTckSupport.check(requesterEvents.acquisitions.size() == 1
               && requesterEvents.unavailable.size() == 1,
            "mixed If Available request did not produce two distinct outcomes");
         checkEvent(requesterEvents.acquisitions.get(0), object, freeOnly, tag,
            "unowned attribute acquisition");
         checkEvent(requesterEvents.unavailable.get(0), object, heldOnly, tag,
            "held attribute unavailable");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         else owner.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerEvents.releaseRequests.isEmpty(),
            "If Available requested release of the still-owned attribute");
         query(owner, ownerEvents, model, objectClassName,
            object, freeOnly, requesterHandle, true);
         query(owner, ownerEvents, model, objectClassName,
            object, heldOnly, ownerHandle, true);
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(
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
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void query(RTIambassador ambassador, Recorder events,
         CallbackModel model, String objectClassName, ObjectInstanceHandle object,
         AttributeHandleSet attributes, FederateHandle expectedOwner, boolean owned)
         throws Exception {
      events.clearQuery();
      ambassador.queryAttributeOwnership(object, attributes);
      await(ambassador, model, objectClassName,
         () -> !events.information.isEmpty() || !events.notOwned.isEmpty(),
         "attribute ownership report");
      if (owned) {
         JavaTckSupport.check(events.information.size() == 1
               && events.notOwned.isEmpty(),
            "mixed If Available query did not report exactly one federate owner");
         OwnerReport report = events.information.get(0);
         JavaTckSupport.check(object.equals(report.object)
               && attributes.equals(report.attributes)
               && expectedOwner.equals(report.owner),
            "mixed If Available query reported the wrong federate owner");
      } else {
         JavaTckSupport.check(events.notOwned.size() == 1
               && events.information.isEmpty(),
            "mixed If Available query did not report the unowned attribute");
         AttributeEvent report = events.notOwned.get(0);
         JavaTckSupport.check(object.equals(report.object)
               && attributes.equals(report.attributes),
            "mixed If Available unowned report had the wrong object or attribute");
      }
   }

   private static void checkEvent(AttributeEvent event,
         ObjectInstanceHandle object, AttributeHandleSet attributes,
         byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object)
            && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " reported the wrong object, attributes, or tag");
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
      final byte[] tag;

      private AttributeEvent(ObjectInstanceHandle object,
            AttributeHandleSet attributes, byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag == null ? new byte[0] : tag.clone();
      }
   }

   private static final class OwnerReport extends AttributeEvent {
      private final FederateHandle owner;

      private OwnerReport(ObjectInstanceHandle object,
            AttributeHandleSet attributes, FederateHandle owner) {
         super(object, attributes, null);
         this.owner = owner;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnerReport> information = new CopyOnWriteArrayList<>();
      private final List<AttributeEvent> notOwned = new CopyOnWriteArrayList<>();
      private final List<AttributeEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<AttributeEvent> unavailable = new CopyOnWriteArrayList<>();
      private final List<AttributeEvent> releaseRequests = new CopyOnWriteArrayList<>();

      private void clearQuery() {
         information.clear();
         notOwned.clear();
      }

      private void clearOwnership() {
         clearQuery();
         acquisitions.clear();
         unavailable.clear();
         releaseRequests.clear();
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
                     (AttributeHandleSet) arguments[1], null));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  acquisitions.add(new AttributeEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  unavailable.add(new AttributeEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  releaseRequests.add(new AttributeEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK mixed ownership callbacks";
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
