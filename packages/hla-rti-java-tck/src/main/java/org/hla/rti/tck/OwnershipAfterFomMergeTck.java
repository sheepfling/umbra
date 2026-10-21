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

/** Ownership of an attribute added by a later FOM join begins only at notification. */
final class OwnershipAfterFomMergeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipAfterFomMergeTck() { }

   static void run(RtiFactory factory, String baseFom, String extensionFom,
         String timeImplementation, String objectClassName, String originalAttributeName,
         String addedAttributeName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "post-join ownership notification timing requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador joiner = factory.getRtiAmbassador();
      Recorder joinerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-ownership-after-fom-merge-" + suffix;
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(new Recorder().proxy(), callbackModel);
         connected[0] = true;
         joiner.connect(joinerRecorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, baseFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-merge-owner-" + suffix, federation);
         joined[0] = true;
         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle originalAttribute = owner.getAttributeHandle(
            ownerClass, originalAttributeName);
         JavaTckSupport.check(ownerClass != null && originalAttribute != null,
            "adapter base FOM did not resolve original class and attribute");
         AttributeHandleSet originalOnly = JavaTckSupport.attributeSet(
            owner, originalAttribute);
         owner.publishObjectClassAttributes(ownerClass, originalOnly);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null
               && owner.isAttributeOwnedByFederate(object, originalAttribute),
            "registrant did not own the original attribute before FOM merge");

         joiner.joinFederationExecution("java-tck-fom-merge-joiner-" + suffix,
            "java-tck-fom-merge-joiner", federation, new String[] {extensionFom});
         joined[1] = true;
         ObjectClassHandle joinerClass = joiner.getObjectClassHandle(objectClassName);
         AttributeHandle joinerOriginal = joiner.getAttributeHandle(
            joinerClass, originalAttributeName);
         AttributeHandle addedAttribute = joiner.getAttributeHandle(
            joinerClass, addedAttributeName);
         JavaTckSupport.check(joinerClass != null && joinerOriginal != null
               && addedAttribute != null && !addedAttribute.equals(joinerOriginal),
            "adapter extension FOM did not add the expected attribute");
         AttributeHandleSet subscribed = JavaTckSupport.attributeSet(
            joiner, joinerOriginal, addedAttribute);
         AttributeHandleSet addedOnly = JavaTckSupport.attributeSet(joiner, addedAttribute);
         joiner.subscribeObjectClassAttributes(joinerClass, subscribed);
         await(joiner, () -> joinerRecorder.discoveries.contains(object),
            "preexisting object discovery after FOM merge");
         JavaTckSupport.check(joinerClass.equals(joiner.getKnownObjectClassHandle(object)),
            "FOM merge discoverer learned the wrong object class");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, originalAttribute)
               && !owner.isAttributeOwnedByFederate(object, addedAttribute)
               && !joiner.isAttributeOwnedByFederate(object, joinerOriginal)
               && !joiner.isAttributeOwnedByFederate(object, addedAttribute),
            "new FOM attribute inherited ownership of a preexisting instance");

         joiner.publishObjectClassAttributes(joinerClass, addedOnly);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, addedAttribute)
               && !joiner.isAttributeOwnedByFederate(object, addedAttribute),
            "publication alone transferred ownership of the added attribute");
         byte[] tag = {(byte) 0xa9, 0x25};
         joiner.attributeOwnershipAcquisitionIfAvailable(object, addedOnly, tag);
         JavaTckSupport.check(!joiner.isAttributeOwnedByFederate(object, addedAttribute)
               && joinerRecorder.acquisitions.isEmpty(),
            "added attribute transferred before acquisition notification");
         await(joiner, () -> !joinerRecorder.acquisitions.isEmpty(),
            "post-merge ownership acquisition notification");
         Event notification = joinerRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(notification.object)
               && notification.attributes.equals(new HashSet<>(addedOnly))
               && Arrays.equals(tag, notification.tag)
               && joinerRecorder.unavailable.isEmpty(),
            "post-merge acquisition returned the wrong standard callback");
         JavaTckSupport.check(joiner.isAttributeOwnedByFederate(object, addedAttribute)
               && !owner.isAttributeOwnedByFederate(object, addedAttribute),
            "post-merge acquisition notification did not establish ownership");

         joiner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[1] = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined[0] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) resign(joiner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (joined[0]) resign(owner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[1]) { try { joiner.disconnect(); } catch (Exception ignored) { } }
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
      private final List<ObjectInstanceHandle> unavailable = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null) {
                  acquisitions.add(new Event(arguments));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null) {
                  unavailable.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK post-join ownership callbacks";
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
