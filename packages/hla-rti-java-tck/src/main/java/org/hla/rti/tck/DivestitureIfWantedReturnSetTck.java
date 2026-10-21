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

/** Checks the exact If Wanted return set when only one of two attributes is requested. */
final class DivestitureIfWantedReturnSetTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private DivestitureIfWantedReturnSetTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstName, String secondName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-if-wanted-return-" + suffix;
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-if-wanted-owner-" + suffix, federation);
         joined[0] = true;
         requester.joinFederationExecution("java-tck-if-wanted-requester-" + suffix, federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(requesterClass, firstName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(requesterClass, secondName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerFirst != null && ownerSecond != null
               && requesterFirst != null && requesterSecond != null
               && !ownerFirst.equals(ownerSecond) && !requesterFirst.equals(requesterSecond),
            "adapter FOM did not resolve two distinct ownership attributes");
         AttributeHandleSet ownerBoth = JavaTckSupport.attributeSet(owner, ownerFirst, ownerSecond);
         AttributeHandleSet requesterBoth = JavaTckSupport.attributeSet(
            requester, requesterFirst, requesterSecond);
         AttributeHandleSet ownerFirstOnly = JavaTckSupport.attributeSet(owner, ownerFirst);
         AttributeHandleSet ownerSecondOnly = JavaTckSupport.attributeSet(owner, ownerSecond);
         AttributeHandleSet requesterFirstOnly = JavaTckSupport.attributeSet(
            requester, requesterFirst);
         AttributeHandleSet requesterSecondOnly = JavaTckSupport.attributeSet(
            requester, requesterSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerBoth);
         requester.publishObjectClassAttributes(requesterClass, requesterBoth);
         requester.subscribeObjectClassAttributes(requesterClass, requesterBoth);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null, "If Wanted registration returned no object");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "If Wanted object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond),
            "registrant did not initially own both ordinary attributes");

         AttributeHandleSet none = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerBoth, new byte[0]);
         JavaTckSupport.check(none != null && none.isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond),
            "If Wanted transferred an attribute before an acquisition existed");

         byte[] requestTag = {(byte) 0xd1, (byte) 0xf2};
         byte[] transferTag = {(byte) 0xc7, 0x25};
         requester.attributeOwnershipAcquisition(object, requesterFirstOnly, requestTag);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && !requester.isAttributeOwnedByFederate(object, requesterFirst),
            "first acquisition transferred ownership before divestiture");
         AttributeHandleSet firstReturned = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerBoth, transferTag);
         JavaTckSupport.check(firstReturned != null
               && new HashSet<>(firstReturned).equals(new HashSet<>(ownerFirstOnly)),
            "If Wanted did not return exactly the first requested attribute");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond)
               && requester.isAttributeOwnedByFederate(object, requesterFirst)
               && !requester.isAttributeOwnedByFederate(object, requesterSecond),
            "partial return set did not match immediate ownership state");
         await(requester, callbackModel, () -> !requesterRecorder.acquisitions.isEmpty(),
            "first If Wanted acquisition notification");
         checkEvent(requesterRecorder.acquisitions.get(0), object,
            requesterFirstOnly, transferTag, "first If Wanted notification");

         requesterRecorder.acquisitions.clear();
         requester.attributeOwnershipAcquisition(object, requesterSecondOnly, requestTag);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerSecond)
               && !requester.isAttributeOwnedByFederate(object, requesterSecond),
            "earlier If Wanted call retained a deferred transfer of the unreturned attribute");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(requester);
            JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty(),
               "second attribute transferred before a new If Wanted call");
         }
         AttributeHandleSet secondReturned = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerSecondOnly, transferTag);
         JavaTckSupport.check(secondReturned != null
               && new HashSet<>(secondReturned).equals(new HashSet<>(ownerSecondOnly))
               && !owner.isAttributeOwnedByFederate(object, ownerSecond)
               && requester.isAttributeOwnedByFederate(object, requesterSecond),
            "new If Wanted call did not transfer exactly the later requested attribute");
         await(requester, callbackModel, () -> !requesterRecorder.acquisitions.isEmpty(),
            "later If Wanted acquisition notification");
         checkEvent(requesterRecorder.acquisitions.get(0), object,
            requesterSecondOnly, transferTag, "later If Wanted notification");

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

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
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
      private final List<Event> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new Event(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK If Wanted return-set callbacks";
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
