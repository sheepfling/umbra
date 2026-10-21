package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Keeps a queued If Available request pending through negotiated confirmation. */
final class IfAvailableDuringNegotiatedDivestitureTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private IfAvailableDuringNegotiatedDivestitureTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "negotiated If Available eligibility requires evoked callback ordering");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-negotiated-if-available-" + suffix;
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-negotiated-if-available-owner-" + suffix,
            federation);
         joined[0] = true;
         requester.joinFederationExecution("java-tck-negotiated-if-available-requester-" + suffix,
            federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null,
            "adapter FOM did not resolve negotiated If Available handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "negotiated If Available registration returned no object");
         await(requester, () -> requesterRecorder.discoveries.contains(object),
            "negotiated If Available object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "registrant did not initially own the negotiated attribute");

         byte[] requestTag = {0x6a, 0x71};
         byte[] divestitureTag = {0x6b, 0x71};
         byte[] confirmationTag = {0x6c, 0x71};
         requester.attributeOwnershipAcquisitionIfAvailable(
            object, requesterAttributes, requestTag);
         owner.negotiatedAttributeOwnershipDivestiture(
            object, ownerAttributes, divestitureTag);
         await(owner, () -> !ownerRecorder.confirmations.isEmpty(),
            "negotiated If Available confirmation request");
         checkEvent(ownerRecorder.confirmations.get(0), object, ownerAttributes,
            requestTag, "negotiated If Available confirmation request");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute)
               && requesterRecorder.acquisitions.isEmpty(),
            "negotiated If Available transferred ownership before owner confirmation");

         owner.confirmDivestiture(object, ownerAttributes, confirmationTag);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "negotiated If Available did not transfer ownership at confirmation");
         await(requester, () -> !requesterRecorder.acquisitions.isEmpty(),
            "negotiated If Available acquisition notification");
         checkEvent(requesterRecorder.acquisitions.get(0), object, requesterAttributes,
            confirmationTag, "negotiated If Available acquisition notification");

         owner.deleteObjectInstance(object, new byte[0]);
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) resign(requester, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (joined[0]) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[1]) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         JavaTckSupport.drain(ambassador);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void checkEvent(Event event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object)
            && event.attributes.equals(new HashSet<>(attributes))
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
   }

   private static final class Event {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;

      private Event(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
         tag = ((byte[]) arguments[2]).clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Event> confirmations = new CopyOnWriteArrayList<>();
      private final List<Event> acquisitions = new CopyOnWriteArrayList<>();

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
                  confirmations.add(new Event(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new Event(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK negotiated If Available callbacks";
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
