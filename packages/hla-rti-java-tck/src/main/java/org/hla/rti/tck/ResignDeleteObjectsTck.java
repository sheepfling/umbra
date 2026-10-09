package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks NO_ACTION ownership failure and ordinary removal on DELETE_OBJECTS. */
final class ResignDeleteObjectsTck {
   private ResignDeleteObjectsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle objectClass = null;
      AttributeHandle attribute = null;
      String federation = "java-tck-resign-delete-boundaries-" + UUID.randomUUID();
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-resign-delete-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-resign-delete-member", federation);
         memberJoined = true;

         objectClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         attribute = owner.getAttributeHandle(objectClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(objectClass != null && memberClass != null
               && attribute != null && memberAttribute != null,
            "adapter FOM did not resolve the class and attribute");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, attribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member,
            memberAttribute);
         owner.publishObjectClassAttributes(objectClass, ownerAttributes);
         published = true;
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         subscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null, "object registration returned a null handle");
         String name = owner.getObjectInstanceName(object);
         JavaTckSupport.check(name != null && !name.isEmpty(),
            "object registration returned an empty instance name");
         await(member, () -> memberCallbacks.discoveries().size() == 1,
            "member did not discover the registered object");
         Discovery discovery = memberCallbacks.discoveries().get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && memberClass.equals(discovery.objectClass)
               && name.equals(discovery.name),
            "discovery callback did not preserve object, class, and name");

         expectFailure(() -> owner.resignFederationExecution(ResignAction.NO_ACTION),
            "FederateOwnsAttributes", "resign with NO_ACTION while owning attributes");
         memberCallbacks.clearRemovals();
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;

         await(member, () -> memberCallbacks.removals().size() == 1,
            "member did not receive resign-time object removal");
         Removal removal = memberCallbacks.removals().get(0);
         JavaTckSupport.check(object.equals(removal.object) && removal.tag.length == 0
               && producer.equals(removal.producer),
            "resign-time removal did not preserve object, empty tag, and producer");
         expectFailure(() -> member.getObjectInstanceHandle(name), "ObjectInstanceNotKnown",
            "object-name lookup after resign-time deletion");

         member.unsubscribeObjectClassAttributes(memberClass, memberAttributes);
         subscribed = false;
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined && subscribed) {
            try {
               member.unsubscribeObjectClassAttributes(
                  member.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(member,
                     member.getAttributeHandle(member.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try {
               owner.unpublishObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(owner, attribute));
            } catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (memberConnected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void expectFailure(CheckedCall call, String expected, String operation)
         throws Exception {
      try {
         call.run();
      } catch (Exception failure) {
         JavaTckSupport.check(expected.equals(failure.getClass().getSimpleName()),
            operation + " threw " + failure.getClass().getSimpleName()
               + " instead of " + expected);
         return;
      }
      throw new AssertionError(operation + " unexpectedly succeeded");
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   @FunctionalInterface
   private interface CheckedCall { void run() throws Exception; }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass, String name) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
      }
   }

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag == null ? new byte[0] : tag.clone();
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2]));
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resign deletion callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
      private List<Discovery> discoveries() { return List.copyOf(discoveries); }
      private List<Removal> removals() { return List.copyOf(removals); }
      private void clearRemovals() { removals.clear(); }
   }
}
