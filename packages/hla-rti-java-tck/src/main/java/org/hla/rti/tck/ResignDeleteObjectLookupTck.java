package org.hla.rti.tck;

import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

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

/** Portable IEEE 1516.1-2025 object-lookup boundary after DELETE_OBJECTS resignation. */
final class ResignDeleteObjectLookupTck {
   private ResignDeleteObjectLookupTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      String federation = "java-tck-resign-delete-object-lookup-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean ownerResigned = false;
      boolean memberResigned = false;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "resign-delete-owner", "tck-type", federation);
         ownerJoined = true;
         member.joinFederationExecution("resign-delete-member", "tck-type", federation);
         memberJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);

         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "registration returned no object-instance handle");
         ObjectInstanceHandle registeredObject = object;
         await(() -> memberCallbacks.discovery() != null
               && registeredObject.equals(memberCallbacks.discovery()), member,
            callbackModel, "member did not discover the registered object");
         String objectName = owner.getObjectInstanceName(registeredObject);
         ObjectInstanceHandle discoveredObject = memberCallbacks.discovery();
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && registeredObject.equals(member.getObjectInstanceHandle(objectName))
               && objectName.equals(member.getObjectInstanceName(discoveredObject))
               && memberClass.equals(member.getKnownObjectClassHandle(discoveredObject)),
            "object name, handle, and known class did not resolve before resignation");

         memberCallbacks.clearRemovals();
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerResigned = true;
         await(() -> !memberCallbacks.removals().isEmpty(), member, callbackModel,
            "member did not receive the DELETE_OBJECTS removal callback");
         Removal removal = memberCallbacks.removals().get(0);
         JavaTckSupport.check(removal.object.equals(registeredObject)
               && Arrays.equals(removal.tag, new byte[0])
               && ownerHandle.equals(removal.producer),
            "resignation removal callback did not preserve object, empty tag, and producer");

         JavaTckSupport.expectFailure(
            () -> member.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "name-to-handle lookup after resign-time deletion");
         JavaTckSupport.expectFailure(
            () -> member.getObjectInstanceName(discoveredObject),
            "ObjectInstanceNotKnown", "handle-to-name lookup after resign-time deletion");
         JavaTckSupport.expectFailure(
            () -> member.getKnownObjectClassHandle(discoveredObject),
            "ObjectInstanceNotKnown", "known-class lookup after resign-time deletion");
      } finally {
         if (ownerJoined && !ownerResigned) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (memberJoined && !memberResigned) {
            try {
               member.resignFederationExecution(ResignAction.NO_ACTION);
               memberResigned = true;
            } catch (Exception ignored) { }
         }
         if (ownerJoined && ownerResigned && memberJoined && !memberResigned) {
            try {
               member.resignFederationExecution(ResignAction.NO_ACTION);
               memberResigned = true;
            } catch (Exception ignored) { }
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

   private static void await(BooleanSupplier condition, RTIambassador ambassador,
         CallbackModel callbackModel, String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(ambassador);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;

      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag == null ? null : tag.clone();
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resign-delete callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private ObjectInstanceHandle discovery() {
         return discoveries.isEmpty() ? null : discoveries.get(discoveries.size() - 1);
      }

      private List<Removal> removals() {
         return List.copyOf(removals);
      }

      private void clearRemovals() {
         removals.clear();
      }
   }
}
