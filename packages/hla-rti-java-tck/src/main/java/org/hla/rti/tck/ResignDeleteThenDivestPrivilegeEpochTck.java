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

/** A new delete-privilege publication epoch affects later registrations, not earlier ones. */
final class ResignDeleteThenDivestPrivilegeEpochTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ResignDeleteThenDivestPrivilegeEpochTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder peerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-delete-then-divest-epoch-" + suffix;
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(new Recorder().proxy(), callbackModel);
         connected[0] = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-epoch-owner-" + suffix, federation);
         joined[0] = true;
         peer.joinFederationExecution("java-tck-epoch-peer-" + suffix, federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle peerClass = peer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle peerAttribute = peer.getAttributeHandle(peerClass, attributeName);
         AttributeHandle ownerPrivilege = owner.getAttributeHandle(
            ownerClass, "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(ownerClass != null && peerClass != null
               && ownerAttribute != null && peerAttribute != null && ownerPrivilege != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(peer.getObjectClassName(peerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(peer.getAttributeName(peerClass, peerAttribute)),
            "adapter FOM did not resolve the ordinary attribute and standard delete privilege");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet peerAttributes = JavaTckSupport.attributeSet(peer, peerAttribute);
         AttributeHandleSet privilegeOnly = JavaTckSupport.attributeSet(owner, ownerPrivilege);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         peer.subscribeObjectClassAttributes(peerClass, peerAttributes);
         peer.publishObjectClassAttributes(peerClass, peerAttributes);

         ObjectInstanceHandle retained = owner.registerObjectInstance(ownerClass);
         String retainedName = owner.getObjectInstanceName(retained);
         JavaTckSupport.check(retained != null && retainedName != null
               && !retainedName.isEmpty()
               && owner.isAttributeOwnedByFederate(retained, ownerAttribute)
               && owner.isAttributeOwnedByFederate(retained, ownerPrivilege),
            "first registration did not establish ordinary and delete-privilege ownership");
         await(peer, callbackModel, () -> peerRecorder.discoveries.size() == 1,
            "retained object discovery");
         ObjectInstanceHandle peerRetained = peer.getObjectInstanceHandle(retainedName);
         JavaTckSupport.check(peerRetained.equals(peerRecorder.discoveries.get(0)),
            "peer did not discover the first object under its local handle");

         owner.unpublishObjectClassAttributes(ownerClass, privilegeOnly);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(retained, ownerAttribute)
               && !owner.isAttributeOwnedByFederate(retained, ownerPrivilege),
            "unpublishing delete privilege did not preserve only ordinary ownership");
         owner.publishObjectClassAttributes(ownerClass, privilegeOnly);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(retained, ownerPrivilege),
            "republishing delete privilege restored ownership of the earlier instance");

         ObjectInstanceHandle deleted = owner.registerObjectInstance(ownerClass);
         String deletedName = owner.getObjectInstanceName(deleted);
         JavaTckSupport.check(deleted != null && !deleted.equals(retained)
               && deletedName != null && !deletedName.isEmpty()
               && !deletedName.equals(retainedName)
               && owner.isAttributeOwnedByFederate(deleted, ownerAttribute)
               && owner.isAttributeOwnedByFederate(deleted, ownerPrivilege),
            "later registration did not establish a new delete-privilege epoch");
         await(peer, callbackModel, () -> peerRecorder.discoveries.size() == 2,
            "deleted object discovery");
         ObjectInstanceHandle peerDeleted = peer.getObjectInstanceHandle(deletedName);
         JavaTckSupport.check(!peerDeleted.equals(peerRetained)
               && peerDeleted.equals(peerRecorder.discoveries.get(1)),
            "peer did not discover the later object under a distinct local handle");

         peerRecorder.removals.clear();
         peerRecorder.assumptions.clear();
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         joined[0] = false;
         await(peer, callbackModel,
            () -> peerRecorder.removals.size() == 1 && peerRecorder.assumptions.size() == 1,
            "delete-then-divest removal and assumption callbacks");
         Assumption assumption = peerRecorder.assumptions.get(0);
         JavaTckSupport.check(peerRecorder.removals.size() == 1
               && peerDeleted.equals(peerRecorder.removals.get(0))
               && peerRetained.equals(assumption.object)
               && assumption.attributes.equals(new HashSet<>(peerAttributes))
               && assumption.tag.length == 0,
            "delete-then-divest did not separate the retained and deleted objects");
         JavaTckSupport.check(peerRetained.equals(peer.getObjectInstanceHandle(retainedName)),
            "the instance without delete privilege was removed");
         JavaTckSupport.expectFailure(
            () -> peer.getObjectInstanceHandle(deletedName),
            "ObjectInstanceNotKnown", "looking up the deleted instance after resignation");

         peer.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         peer.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) resign(peer);
         if (joined[0]) resign(owner);
         if (created) {
            try { peer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[1]) { try { peer.disconnect(); } catch (Exception ignored) { } }
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
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Assumption {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final byte[] tag;

      private Assumption(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
         tag = ((byte[]) arguments[2]).clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();
      private final List<Assumption> assumptions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null) {
                  assumptions.add(new Assumption(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK delete-then-divest callbacks";
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
