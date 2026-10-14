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
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.ResignAction;

/** Portable IEEE 1516.1-2025 remote-deletion lookup boundary. */
final class RemoteDeleteKnownObjectClassLookupTck {
   private RemoteDeleteKnownObjectClassLookupTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      String federation = "java-tck-remote-delete-known-class-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean deleteIssued = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle memberClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle memberAttribute = null;
      ObjectInstanceHandle object = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet memberAttributes = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "remote-delete-owner", "tck-type", federation);
         ownerJoined = true;
         member.joinFederationExecution("remote-delete-member", "tck-type", federation);
         memberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         memberClass = member.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         subscribed = true;

         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned no object-instance handle");
         ObjectInstanceHandle registeredObject = object;
         await(() -> memberCallbacks.discovery() != null
               && registeredObject.equals(memberCallbacks.discovery()), member,
            callbackModel, "member did not discover the registered object");
         JavaTckSupport.check(object.equals(memberCallbacks.discovery())
               && memberClass.equals(member.getKnownObjectClassHandle(object)),
            "discovered object did not resolve its known class before deletion");

         memberCallbacks.clearRemovals();
         byte[] tag = new byte[0];
         owner.deleteObjectInstance(object, tag);
         deleteIssued = true;
         JavaTckSupport.expectFailure(
            () -> owner.getKnownObjectClassHandle(registeredObject),
            "ObjectInstanceNotKnown", "known-class lookup after deletion by the owner");

         await(() -> !memberCallbacks.removals().isEmpty(), member, callbackModel,
            "member did not receive the remote object-removal callback");
         Removal removal = memberCallbacks.removals().get(0);
         JavaTckSupport.check(removal.object.equals(object)
               && Arrays.equals(removal.tag, tag)
               && ownerHandle.equals(removal.producer),
            "remote removal callback did not preserve object, tag, and producer");
         JavaTckSupport.expectFailure(
            () -> member.getKnownObjectClassHandle(memberCallbacks.discovery()),
            "ObjectInstanceNotKnown", "known-class lookup after remote removal");
      } finally {
         if (object != null && ownerJoined && !deleteIssued) {
            try { owner.deleteObjectInstance(object, new byte[0]); }
            catch (Exception ignored) { }
         }
         if (memberJoined && subscribed && memberClass != null && memberAttributes != null) {
            try { member.unsubscribeObjectClassAttributes(memberClass, memberAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
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

   static void runObjectNameLookup(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      String federation = "java-tck-remote-delete-object-name-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean deleteIssued = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle memberClass = null;
      ObjectInstanceHandle object = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet memberAttributes = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "remote-name-owner", "tck-type", federation);
         ownerJoined = true;
         member.joinFederationExecution("remote-name-member", "tck-type", federation);
         memberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         subscribed = true;

         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned no object-instance handle");
         ObjectInstanceHandle registeredObject = object;
         await(() -> memberCallbacks.discovery() != null
               && registeredObject.equals(memberCallbacks.discovery()), member,
            callbackModel, "member did not discover the registered object");
         String objectName = owner.getObjectInstanceName(registeredObject);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && objectName.equals(member.getObjectInstanceName(memberCallbacks.discovery()))
               && registeredObject.equals(owner.getObjectInstanceHandle(objectName)),
            "object name and handle did not round-trip on both federates");

         memberCallbacks.clearRemovals();
         byte[] tag = new byte[0];
         owner.deleteObjectInstance(registeredObject, tag);
         deleteIssued = true;
         JavaTckSupport.expectFailure(() -> owner.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "object-name lookup after deletion by the owner");
         await(() -> !memberCallbacks.removals().isEmpty(), member, callbackModel,
            "member did not receive the remote object-removal callback");
         Removal removal = memberCallbacks.removals().get(0);
         JavaTckSupport.check(removal.object.equals(registeredObject)
               && Arrays.equals(removal.tag, tag)
               && ownerHandle.equals(removal.producer),
            "remote removal callback did not preserve object, tag, and producer");
         JavaTckSupport.expectFailure(
            () -> member.getObjectInstanceName(memberCallbacks.discovery()),
            "ObjectInstanceNotKnown", "object-name lookup after remote removal");
      } finally {
         if (object != null && ownerJoined && !deleteIssued) {
            try { owner.deleteObjectInstance(object, new byte[0]); }
            catch (Exception ignored) { }
         }
         if (memberJoined && subscribed && memberClass != null && memberAttributes != null) {
            try { member.unsubscribeObjectClassAttributes(memberClass, memberAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
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
                  if ("toString".equals(name)) return "Java TCK remote-delete callbacks";
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
