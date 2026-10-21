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
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** A divesting resignation releases every published attribute on every registered object. */
final class ResignDivestsAllInstanceAttributesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ResignDivestsAllInstanceAttributesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador survivor = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-resign-divests-all-" + suffix;
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(new Recorder().proxy(), callbackModel);
         connected[0] = true;
         survivor.connect(recorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-resign-owner-" + suffix, federation);
         joined[0] = true;
         survivor.joinFederationExecution("java-tck-resign-survivor-" + suffix, federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle survivorClass = survivor.getObjectClassHandle(objectClassName);
         AttributeHandle firstOwner = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle secondOwner = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle firstSurvivor = survivor.getAttributeHandle(survivorClass,
            firstAttributeName);
         AttributeHandle secondSurvivor = survivor.getAttributeHandle(survivorClass,
            secondAttributeName);
         JavaTckSupport.check(ownerClass != null && survivorClass != null
               && firstOwner != null && secondOwner != null
               && firstSurvivor != null && secondSurvivor != null
               && !firstOwner.equals(secondOwner) && !firstSurvivor.equals(secondSurvivor),
            "adapter FOM did not resolve two distinct resign-time attributes");
         owner.publishObjectClassAttributes(ownerClass,
            JavaTckSupport.attributeSet(owner, firstOwner, secondOwner));
         AttributeHandleSet subscribed = JavaTckSupport.attributeSet(
            survivor, firstSurvivor, secondSurvivor);
         survivor.subscribeObjectClassAttributes(survivorClass, subscribed);

         ObjectInstanceHandle firstObject = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle secondObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "registration did not return two distinct objects");
         await(survivor, callbackModel, () -> recorder.discoveries.contains(firstObject)
               && recorder.discoveries.contains(secondObject), "two-object discovery");
         for (ObjectInstanceHandle object : new ObjectInstanceHandle[] {firstObject, secondObject}) {
            JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, firstOwner)
                  && owner.isAttributeOwnedByFederate(object, secondOwner)
                  && !survivor.isAttributeOwnedByFederate(object, firstSurvivor)
                  && !survivor.isAttributeOwnedByFederate(object, secondSurvivor),
               "registrant did not own both attributes on each object before resignation");
         }

         recorder.removals.clear();
         owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[0] = false;
         for (ObjectInstanceHandle object : new ObjectInstanceHandle[] {firstObject, secondObject}) {
            JavaTckSupport.check(survivorClass.equals(
                  survivor.getKnownObjectClassHandle(object))
                  && !survivor.isAttributeOwnedByFederate(object, firstSurvivor)
                  && !survivor.isAttributeOwnedByFederate(object, secondSurvivor),
               "divesting resignation did not retain and release each object");
            recorder.ownership.clear();
            survivor.queryAttributeOwnership(object, subscribed);
            await(survivor, callbackModel, () -> recorder.hasUnowned(object, subscribed),
               "post-resignation unowned attribute report");
            JavaTckSupport.check(recorder.onlyUnowned(object, subscribed),
               "post-resignation query did not report both attributes unowned");
         }
         JavaTckSupport.check(recorder.removals.isEmpty(),
            "unconditional divestiture removed a registered object");

         survivor.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) resign(survivor);
         if (joined[0]) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[1]) { try { survivor.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
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

   private static final class OwnershipEvent {
      private final String kind;
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;

      private OwnershipEvent(String kind, Object[] arguments) {
         this.kind = kind;
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownership = new CopyOnWriteArrayList<>();

      private boolean hasUnowned(ObjectInstanceHandle object, AttributeHandleSet expected) {
         Set<AttributeHandle> reported = new HashSet<>();
         for (OwnershipEvent event : ownership) {
            if ("attributeIsNotOwned".equals(event.kind) && object.equals(event.object)) {
               reported.addAll(event.attributes);
            }
         }
         return reported.containsAll(expected);
      }

      private boolean onlyUnowned(ObjectInstanceHandle object, AttributeHandleSet expected) {
         Set<AttributeHandle> reported = new HashSet<>();
         for (OwnershipEvent event : ownership) {
            if (!object.equals(event.object) || !"attributeIsNotOwned".equals(event.kind)
                  || !new HashSet<>(expected).containsAll(event.attributes)) return false;
            reported.addAll(event.attributes);
         }
         return reported.equals(new HashSet<>(expected));
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               } else if (("attributeIsNotOwned".equals(name)
                     || "informAttributeOwnership".equals(name)
                     || "attributeIsOwnedByRTI".equals(name)) && arguments != null) {
                  ownership.add(new OwnershipEvent(name, arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resignation ownership callbacks";
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
