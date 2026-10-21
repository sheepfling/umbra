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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** A deleted object cannot produce a stale queued If Available result. */
final class IfAvailableDeletedObjectTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private IfAvailableDeletedObjectTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "queued If Available deletion requires evoked callback ordering");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder requesterRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-if-available-deleted-object-" + suffix;
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(new Recorder().proxy(), callbackModel);
         connected[0] = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-deletion-owner-" + suffix, federation);
         joined[0] = true;
         requester.joinFederationExecution("java-tck-deletion-requester-" + suffix, federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null,
            "adapter FOM did not resolve If Available deletion handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "If Available deletion registration returned no object");
         await(requester, () -> requesterRecorder.discoveries.contains(object),
            "If Available deletion object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "If Available deletion setup did not establish remote ownership");

         byte[] requestTag = {0x51, (byte) 0xa7};
         byte[] deletionTag = {0x44, 0x25};
         requester.attributeOwnershipAcquisitionIfAvailable(
            object, requesterAttributes, requestTag);
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty()
               && requesterRecorder.unavailable.isEmpty(),
            "If Available deletion delivered a result before callback servicing");
         owner.deleteObjectInstance(object, deletionTag);
         await(requester, () -> !requesterRecorder.removals.isEmpty(),
            "If Available deletion removal callback");
         JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterRecorder.removals.size() == 1
               && object.equals(requesterRecorder.removals.get(0).object)
               && Arrays.equals(deletionTag, requesterRecorder.removals.get(0).tag),
            "deleted object did not yield exactly one tagged removal");
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty()
               && requesterRecorder.unavailable.isEmpty(),
            "deleted object delivered a stale If Available ownership result");

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

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;

      private Removal(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         tag = ((byte[]) arguments[1]).clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> acquisitions = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> unavailable = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null) {
                  removals.add(new Removal(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null) {
                  acquisitions.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null) {
                  unavailable.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK If Available deletion callbacks";
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
