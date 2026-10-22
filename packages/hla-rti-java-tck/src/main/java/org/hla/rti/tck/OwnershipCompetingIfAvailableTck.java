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

/** Competing If Available acquisitions resolve to one publicly queried owner. */
final class OwnershipCompetingIfAvailableTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipCompetingIfAvailableTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel model) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder firstEvents = new Recorder();
      Recorder secondEvents = new Recorder();
      String federation = "java-tck-competing-if-available-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean firstConnected = false;
      boolean secondConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean firstJoined = false;
      boolean secondJoined = false;
      try {
         owner.connect(ownerEvents.proxy(), model);
         ownerConnected = true;
         first.connect(firstEvents.proxy(), model);
         firstConnected = true;
         second.connect(secondEvents.proxy(), model);
         secondConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-competing-owner", federation);
         ownerJoined = true;
         FederateHandle firstHandle = first.joinFederationExecution(
            "java-tck-competing-first", federation);
         firstJoined = true;
         FederateHandle secondHandle = second.joinFederationExecution(
            "java-tck-competing-second", federation);
         secondJoined = true;
         JavaTckSupport.check(!ownerHandle.equals(firstHandle)
               && !ownerHandle.equals(secondHandle)
               && !firstHandle.equals(secondHandle),
            "competing If Available fixture returned duplicate federate handles");

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(firstClass)
               && ownerClass.equals(secondClass)
               && ownerAttribute.equals(firstAttribute)
               && ownerAttribute.equals(secondAttribute),
            "competing If Available fixture did not resolve matching standard handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributes = JavaTckSupport.attributeSet(
            second, secondAttribute);
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "competing If Available registration returned a null object handle");
         await(first, model, objectClassName,
            () -> firstEvents.discoveries.contains(object), "first candidate discovery");
         await(second, model, objectClassName,
            () -> secondEvents.discoveries.contains(object), "second candidate discovery");
         first.publishObjectClassAttributes(firstClass, firstAttributes);
         second.publishObjectClassAttributes(secondClass, secondAttributes);

         owner.unconditionalAttributeOwnershipDivestiture(
            object, ownerAttributes, new byte[] {0x54, (byte) 0xA7});
         owner.queryAttributeOwnership(object, ownerAttributes);
         await(owner, model, objectClassName,
            () -> !ownerEvents.notOwned.isEmpty(), "initial unowned report");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerEvents.notOwned.size() == 1
               && ownerEvents.information.isEmpty(),
            "competing If Available fixture did not establish an unowned attribute");
         checkAttributeEvent(ownerEvents.notOwned.get(0), object, ownerAttributes,
            "unowned query");

         first.attributeOwnershipAcquisitionIfAvailable(
            object, firstAttributes, new byte[] {(byte) 0xA7, 0x0E});
         second.attributeOwnershipAcquisitionIfAvailable(
            object, secondAttributes, new byte[] {(byte) 0xA7, 0x0F});
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(firstEvents.acquisitions.isEmpty()
                  && firstEvents.unavailable.isEmpty()
                  && secondEvents.acquisitions.isEmpty()
                  && secondEvents.unavailable.isEmpty(),
               "evoked competing acquisition delivered before callback servicing");
         }
         // Service the second candidate first; no winner order is prescribed.
         await(second, model, objectClassName,
            () -> !secondEvents.acquisitions.isEmpty() || !secondEvents.unavailable.isEmpty(),
            "second candidate terminal callback");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(second);
         await(first, model, objectClassName,
            () -> !firstEvents.acquisitions.isEmpty() || !firstEvents.unavailable.isEmpty(),
            "first candidate terminal callback");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(first);
         checkTerminal(firstEvents, object, firstAttributes, "first candidate");
         checkTerminal(secondEvents, object, secondAttributes, "second candidate");
         boolean firstWon = firstEvents.acquisitions.size() == 1;
         boolean secondWon = secondEvents.acquisitions.size() == 1;
         JavaTckSupport.check(firstWon != secondWon
               && firstEvents.unavailable.size() + secondEvents.unavailable.size() == 1,
            "competing If Available requests did not resolve to one winner and one loser");
         FederateHandle winner = firstWon ? firstHandle : secondHandle;

         ownerEvents.clearOwnership();
         owner.queryAttributeOwnership(object, ownerAttributes);
         await(owner, model, objectClassName,
            () -> !ownerEvents.information.isEmpty(), "winner owner report");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerEvents.information.size() == 1
               && ownerEvents.notOwned.isEmpty(),
            "winner query did not exclusively report one federate owner");
         OwnerReport report = ownerEvents.information.get(0);
         checkAttributeEvent(report, object, ownerAttributes, "winner query");
         JavaTckSupport.check(winner.equals(report.owner)
               && !winner.equals(ownerHandle),
            "winner query did not identify the successful candidate");
      } finally {
         if (secondJoined) {
            try { second.resignFederationExecution(
               ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { first.resignFederationExecution(
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
         if (secondConnected) { try { second.disconnect(); } catch (Exception ignored) { } }
         if (firstConnected) { try { first.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkTerminal(Recorder events, ObjectInstanceHandle object,
         AttributeHandleSet attributes, String phase) {
      JavaTckSupport.check(events.acquisitions.size() + events.unavailable.size() == 1,
         phase + " did not receive exactly one terminal callback");
      AttributeEvent result = events.acquisitions.isEmpty()
         ? events.unavailable.get(0) : events.acquisitions.get(0);
      checkAttributeEvent(result, object, attributes, phase);
   }

   private static void checkAttributeEvent(AttributeEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, String phase) {
      JavaTckSupport.check(object.equals(event.object)
            && attributes.equals(event.attributes),
         phase + " reported the wrong object or attributes");
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
      private final List<AttributeEvent> unavailable = new CopyOnWriteArrayList<>();

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
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  unavailable.add(new AttributeEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK competing ownership callbacks";
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
