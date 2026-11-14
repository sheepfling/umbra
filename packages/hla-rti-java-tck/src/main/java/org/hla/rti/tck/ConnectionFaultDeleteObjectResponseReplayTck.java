package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Replays a successful DeleteObjectInstance response after an adapter-managed fault. */
final class ConnectionFaultDeleteObjectResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long CALLBACK_SETTLE_NANOS = 20_000_000L;

   private ConnectionFaultDeleteObjectResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel, RtiConfiguration ownerConfiguration,
         RtiConfiguration memberConfiguration) throws Exception {
      Federate owner = new Federate(factory.getRtiAmbassador());
      Federate member = new Federate(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-delete-object-response-replay-" + suffix;
      String ownerName = "java-tck-delete-owner-" + suffix;
      String memberName = "java-tck-delete-member-" + suffix;
      boolean created = false;
      try {
         owner.connect(callbackModel, ownerConfiguration);
         owner.rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.join(ownerName, federation);

         member.connect(callbackModel, memberConfiguration);
         member.join(memberName, federation);

         ObjectClassHandle ownerClass = owner.rti.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.rti.getAttributeHandle(ownerClass, attributeName);
         ObjectClassHandle memberClass = member.rti.getObjectClassHandle(objectClassName);
         AttributeHandle memberAttribute = member.rti.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(ownerClass != null && ownerClass.encodedLength() > 0
               && ownerAttribute != null && ownerAttribute.encodedLength() > 0
               && memberClass != null && memberClass.encodedLength() > 0
               && memberAttribute != null && memberAttribute.encodedLength() > 0,
            "DeleteObjectInstance response replay could not resolve the object class and attribute");
         owner.rti.publishObjectClassAttributes(
            ownerClass, JavaTckSupport.attributeSet(owner.rti, ownerAttribute));
         member.rti.subscribeObjectClassAttributes(
            memberClass, JavaTckSupport.attributeSet(member.rti, memberAttribute));

         ObjectInstanceHandle object = owner.rti.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "DeleteObjectInstance response replay setup returned an invalid object handle");
         String objectName = owner.rti.getObjectInstanceName(object);
         member.awaitDiscovery(object, objectName, objectClassName, callbackModel);
         JavaTckSupport.check(object.equals(member.rti.getObjectInstanceHandle(objectName)),
            "discovered object name did not resolve to the registered object handle");

         byte[] deletionTag = new byte[] {(byte) 0xA4, (byte) 0xB5, (byte) 0xC6};
         owner.rti.deleteObjectInstance(object, deletionTag);
         JavaTckSupport.expectFailure(
            () -> owner.rti.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "owner lookup after replayed DeleteObjectInstance");

         member.awaitRemoval(objectClassName, callbackModel);
         member.settleCallbacks(objectClassName, callbackModel);
         List<Removal> removals = member.removals();
         JavaTckSupport.check(removals.size() == 1,
            "DeleteObjectInstance response replay delivered duplicate removal callbacks");
         Removal removal = removals.get(0);
         JavaTckSupport.check(object.equals(removal.object)
               && Arrays.equals(deletionTag, removal.tag)
               && ownerHandle.equals(removal.producer),
            "DeleteObjectInstance response replay changed removal object, tag, or producer");
         JavaTckSupport.expectFailure(
            () -> member.rti.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "member lookup after replayed DeleteObjectInstance");

         member.resign(ResignAction.NO_ACTION);
         owner.resign(ResignAction.NO_ACTION);
         owner.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         member.resignQuietly(ResignAction.NO_ACTION);
         owner.resignQuietly(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         if (created) {
            try { owner.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         member.disconnectQuietly();
         owner.disconnectQuietly();
      }
   }

   private static FederateAmbassador callbacks(Federate federate) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("discoverObjectInstance".equals(name)
                  && arguments != null && arguments.length >= 3) {
               federate.discoveries.add(new Discovery(
                  (ObjectInstanceHandle) arguments[0], (String) arguments[2]));
            } else if ("removeObjectInstance".equals(name)
                  && arguments != null && arguments.length == 3) {
               federate.removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                  (byte[]) arguments[1], (FederateHandle) arguments[2]));
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK delete-replay callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final String name;

      private Discovery(ObjectInstanceHandle object, String name) {
         this.object = object;
         this.name = name;
      }
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

   private static final class Federate {
      private final RTIambassador rti;
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private boolean connected;
      private boolean joined;

      private Federate(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel callbackModel,
            RtiConfiguration configuration) throws Exception {
         rti.connect(callbacks(this), callbackModel, configuration);
         connected = true;
      }

      private FederateHandle join(String name, String federation) throws Exception {
         FederateHandle handle = rti.joinFederationExecution(
            name, "java-tck-federate", federation);
         joined = true;
         return handle;
      }

      private void awaitDiscovery(ObjectInstanceHandle object, String objectName,
            String objectClassName, CallbackModel callbackModel) throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && !hasDiscovery(object, objectName)) {
            serviceCallbacks(objectClassName, callbackModel);
         }
         JavaTckSupport.check(hasDiscovery(object, objectName),
            "member timed out waiting for object discovery before DeleteObjectInstance");
      }

      private void awaitRemoval(String objectClassName, CallbackModel callbackModel)
            throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && removals.isEmpty()) {
            serviceCallbacks(objectClassName, callbackModel);
         }
         JavaTckSupport.check(!removals.isEmpty(),
            "member timed out waiting for RemoveObjectInstance after response replay");
      }

      private void settleCallbacks(String objectClassName, CallbackModel callbackModel)
            throws Exception {
         long deadline = System.nanoTime() + CALLBACK_SETTLE_NANOS;
         while (System.nanoTime() < deadline) {
            serviceCallbacks(objectClassName, callbackModel);
         }
      }

      private void serviceCallbacks(String objectClassName, CallbackModel callbackModel)
            throws Exception {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            rti.evokeMultipleCallbacks(0.0, 0.001);
         } else {
            rti.getObjectClassHandle(objectClassName);
            Thread.sleep(1L);
         }
      }

      private boolean hasDiscovery(ObjectInstanceHandle object, String objectName) {
         return discoveries.stream().anyMatch(discovery ->
            object.equals(discovery.object) && objectName.equals(discovery.name));
      }

      private List<Removal> removals() { return List.copyOf(removals); }

      private void resign(ResignAction action) throws Exception {
         if (joined) {
            rti.resignFederationExecution(action);
            joined = false;
         }
      }

      private void resignQuietly(ResignAction action) {
         try { resign(action); }
         catch (Exception ignored) { joined = false; }
      }

      private void disconnectQuietly() {
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
            connected = false;
         }
      }
   }
}
