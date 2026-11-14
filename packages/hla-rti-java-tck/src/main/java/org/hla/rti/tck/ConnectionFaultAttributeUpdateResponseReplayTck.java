package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
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

/** Replays an accepted UpdateAttributeValues response after an adapter-managed fault. */
final class ConnectionFaultAttributeUpdateResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long CALLBACK_SETTLE_NANOS = 20_000_000L;

   private ConnectionFaultAttributeUpdateResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel, RtiConfiguration ownerConfiguration,
         RtiConfiguration memberConfiguration) throws Exception {
      Federate owner = new Federate(factory.getRtiAmbassador());
      Federate member = new Federate(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-attribute-update-response-replay-" + suffix;
      String ownerName = "java-tck-attribute-update-owner-" + suffix;
      String memberName = "java-tck-attribute-update-member-" + suffix;
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
            "UpdateAttributeValues response replay could not resolve the object class and attribute");
         owner.rti.publishObjectClassAttributes(
            ownerClass, JavaTckSupport.attributeSet(owner.rti, ownerAttribute));
         member.rti.subscribeObjectClassAttributes(
            memberClass, JavaTckSupport.attributeSet(member.rti, memberAttribute));

         ObjectInstanceHandle object = owner.rti.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "UpdateAttributeValues response replay setup returned an invalid object handle");
         member.awaitDiscovery(object, objectClassName, callbackModel);
         member.clearReflections();

         byte[] firstValue = new byte[] {0x21, 0x22, 0x23};
         byte[] firstTag = new byte[] {(byte) 0xA1, (byte) 0xA2};
         owner.update(object, ownerAttribute, firstValue, firstTag);

         byte[] fenceValue = new byte[] {0x31, 0x32};
         byte[] fenceTag = new byte[] {(byte) 0xB1};
         owner.update(object, ownerAttribute, fenceValue, fenceTag);

         member.awaitReflections(objectClassName, callbackModel);
         member.settleCallbacks(objectClassName, callbackModel);
         List<Reflection> reflections = member.reflections();
         JavaTckSupport.check(reflections.size() == 2,
            "UpdateAttributeValues response replay lost or duplicated a reflection");
         JavaTckSupport.check(matches(reflections.get(0), object, memberAttribute,
               firstValue, firstTag, ownerHandle)
               && matches(reflections.get(1), object, memberAttribute,
                  fenceValue, fenceTag, ownerHandle),
            "UpdateAttributeValues response replay lost, reordered, or changed a reflection");

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

   private static boolean matches(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] expectedValue, byte[] expectedTag,
         FederateHandle expectedProducer) {
      return !reflection.timestampOrdered && object.equals(reflection.object)
         && reflection.values.size() == 1
         && Arrays.equals(expectedValue, reflection.values.get(attribute))
         && Arrays.equals(expectedTag, reflection.tag)
         && expectedProducer.equals(reflection.producer);
   }

   private static FederateAmbassador callbacks(Federate federate) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("discoverObjectInstance".equals(name)
                  && arguments != null && arguments.length >= 1) {
               federate.discoveries.add((ObjectInstanceHandle) arguments[0]);
            } else if ("reflectAttributeValues".equals(name)
                  && arguments != null && arguments.length >= 6) {
               federate.reflections.add(new Reflection(
                  (ObjectInstanceHandle) arguments[0],
                  (AttributeHandleValueMap) arguments[1],
                  (byte[]) arguments[2],
                  (FederateHandle) arguments[4],
                  arguments.length != 6));
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK attribute-update replay callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final boolean timestampOrdered;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, FederateHandle producer, boolean timestampOrdered) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag == null ? null : tag.clone();
         this.producer = producer;
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Federate {
      private final RTIambassador rti;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
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

      private void update(ObjectInstanceHandle object, AttributeHandle attribute,
            byte[] value, byte[] tag) throws Exception {
         AttributeHandleValueMap values = rti.getAttributeHandleValueMapFactory().create(1);
         values.put(attribute, value.clone());
         rti.updateAttributeValues(object, values, tag.clone());
      }

      private void awaitDiscovery(ObjectInstanceHandle object, String objectClassName,
            CallbackModel callbackModel) throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && !discoveries.contains(object)) {
            serviceCallbacks(objectClassName, callbackModel);
         }
         JavaTckSupport.check(discoveries.contains(object),
            "member timed out waiting for object discovery before UpdateAttributeValues");
      }

      private void awaitReflections(String objectClassName, CallbackModel callbackModel)
            throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && reflections.size() < 2) {
            serviceCallbacks(objectClassName, callbackModel);
         }
         JavaTckSupport.check(reflections.size() >= 2,
            "member timed out waiting for replay and callback-fence reflections");
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

      private void clearReflections() { reflections.clear(); }

      private List<Reflection> reflections() { return List.copyOf(reflections); }

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
