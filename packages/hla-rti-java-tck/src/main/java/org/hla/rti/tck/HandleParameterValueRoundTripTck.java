package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies an object-instance handle survives interaction transfer and typed decoding. */
final class HandleParameterValueRoundTripTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private HandleParameterValueRoundTripTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-handle-parameter-value-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean receiverJoined = false;
      boolean objectPublished = false;
      boolean objectSubscribed = false;
      boolean interactionPublished = false;
      boolean interactionSubscribed = false;
      ObjectClassHandle ownerObjectClass = null;
      ObjectClassHandle receiverObjectClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle receiverAttribute = null;
      InteractionClassHandle ownerInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         owner.connect(new NullFederateAmbassador(), callbackModel);
         ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-handle-value-owner-" + UUID.randomUUID(), federation);
         ownerJoined = true;
         receiver.joinFederationExecution(
            "java-tck-handle-value-receiver-" + UUID.randomUUID(), federation);
         receiverJoined = true;

         ownerObjectClass = owner.getObjectClassHandle(objectClassName);
         receiverObjectClass = receiver.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerObjectClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverObjectClass, attributeName);
         ownerInteraction = owner.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(
            ownerInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(ownerObjectClass != null && receiverObjectClass != null
               && ownerAttribute != null && receiverAttribute != null
               && ownerInteraction != null && receiverInteraction != null
               && ownerParameter != null && receiverParameter != null,
            "handle-value transfer FOM lookup returned a null designator");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerAttribute);
         AttributeHandleSet receiverAttributes = JavaTckSupport.attributeSet(
            receiver, receiverAttribute);
         owner.publishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         objectPublished = true;
         receiver.subscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);
         objectSubscribed = true;
         owner.publishInteractionClass(ownerInteraction);
         interactionPublished = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         interactionSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerObjectClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "handle-value transfer registration returned an invalid handle");
         String objectName = owner.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "handle-value transfer registration returned an empty name");
         await(receiver, callbackModel, () -> !receiverRecorder.discoveries.isEmpty(),
            "handle-value transfer object discovery");
         Discovery discovery = receiverRecorder.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && receiverObjectClass.equals(discovery.objectClass)
               && objectName.equals(discovery.name),
            "receiver discovered the wrong object-instance handle, class, or name");

         byte[] encodedHandle = JavaTckSupport.encode(object.encodedLength(), object::encode);
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, encodedHandle.clone());
         byte[] callerParameterValue = parameters.get(ownerParameter);
         JavaTckSupport.check(callerParameterValue != null && callerParameterValue.length > 0,
            "handle-value transfer parameter map did not retain its input value");
         receiver.disableCallbacks();
         owner.sendInteraction(ownerInteraction, parameters, new byte[0]);
         byte[] replacementParameterBytes = new byte[callerParameterValue.length];
         for (int i = 0; i < callerParameterValue.length; ++i) {
            replacementParameterBytes[i] = (byte) (callerParameterValue[i] ^ 0x5A);
         }
         System.arraycopy(replacementParameterBytes, 0, callerParameterValue, 0,
            callerParameterValue.length);
         JavaTckSupport.check(Arrays.equals(parameters.get(ownerParameter),
               replacementParameterBytes),
            "test did not mutate the caller-owned interaction parameter after Send Interaction returned");
         receiver.enableCallbacks();
         await(receiver, callbackModel, () -> !receiverRecorder.interactions.isEmpty(),
            "encoded object-instance handle interaction parameter");

         JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
            "receiver did not get exactly one handle-value interaction");
         Interaction received = receiverRecorder.interactions.get(0);
         byte[] receivedValue = received.parameters.get(receiverParameter);
         JavaTckSupport.check(receiverInteraction.equals(received.interaction)
               && ownerFederate.equals(received.producer) && receivedValue != null,
            "receiver did not get the encoded handle under its local interaction designators");
         JavaTckSupport.check(Arrays.equals(receivedValue, encodedHandle),
            "RTI changed the encoded object-instance handle while forwarding its parameter value");

         ObjectInstanceHandle decoded = receiver.getObjectInstanceHandleFactory().decode(
            receivedValue, 0);
         JavaTckSupport.check(decoded != null && decoded.equals(object)
               && receiver.getObjectInstanceHandle(objectName).equals(decoded)
               && objectName.equals(receiver.getObjectInstanceName(decoded)),
            "receiver did not reconstitute the sender's object-instance handle");

         AttributeHandleValueMap attributeValues =
            owner.getAttributeHandleValueMapFactory().create(1);
         attributeValues.put(ownerAttribute, encodedHandle.clone());
         byte[] callerAttributeValue = attributeValues.get(ownerAttribute);
         JavaTckSupport.check(callerAttributeValue != null && callerAttributeValue.length > 0,
            "handle-value transfer attribute map did not retain its input value");
         receiver.disableCallbacks();
         owner.updateAttributeValues(object, attributeValues, new byte[0]);
         byte[] replacementAttributeBytes = new byte[callerAttributeValue.length];
         for (int i = 0; i < callerAttributeValue.length; ++i) {
            replacementAttributeBytes[i] = (byte) (callerAttributeValue[i] ^ 0x6A);
         }
         System.arraycopy(replacementAttributeBytes, 0, callerAttributeValue, 0,
            callerAttributeValue.length);
         JavaTckSupport.check(Arrays.equals(attributeValues.get(ownerAttribute),
               replacementAttributeBytes),
            "test did not mutate the caller-owned attribute value after Update Attribute Values returned");
         receiver.enableCallbacks();
         await(receiver, callbackModel, () -> !receiverRecorder.reflections.isEmpty(),
            "encoded object-instance handle attribute value");

         JavaTckSupport.check(receiverRecorder.reflections.size() == 1,
            "receiver did not get exactly one handle-value attribute reflection");
         Reflection reflected = receiverRecorder.reflections.get(0);
         byte[] reflectedValue = reflected.values.get(receiverAttribute);
         JavaTckSupport.check(object.equals(reflected.object) && reflectedValue != null,
            "receiver did not get the encoded handle under its local attribute designator");
         JavaTckSupport.check(Arrays.equals(reflectedValue, encodedHandle),
            "RTI changed the encoded object-instance handle while forwarding its attribute value");
         ObjectInstanceHandle decodedAttributeValue =
            receiver.getObjectInstanceHandleFactory().decode(reflectedValue, 0);
         JavaTckSupport.check(decodedAttributeValue.equals(object)
               && receiver.getObjectInstanceHandle(objectName).equals(decodedAttributeValue),
            "receiver did not reconstitute the object-instance handle from the attribute value");

         JavaTckSupport.expectFailure(
            () -> receiver.getFederateHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as a federate handle");
         JavaTckSupport.expectFailure(
            () -> receiver.getObjectClassHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as an object-class handle");
         JavaTckSupport.expectFailure(
            () -> receiver.getInteractionClassHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as an interaction-class handle");
         JavaTckSupport.expectFailure(
            () -> receiver.getAttributeHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as an attribute handle");
         JavaTckSupport.expectFailure(
            () -> receiver.getParameterHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as a parameter handle");
         JavaTckSupport.expectFailure(
            () -> receiver.getDimensionHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as a dimension handle");
         JavaTckSupport.expectFailure(
            () -> receiver.getMessageRetractionHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as a message-retraction handle");
         JavaTckSupport.expectFailure(
            () -> receiver.getRegionHandleFactory().decode(receivedValue, 0),
            "CouldNotDecode", "reconstituting an object-instance handle as a region handle");
      } finally {
         if (receiverJoined && objectSubscribed && receiverObjectClass != null
               && receiverAttribute != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverObjectClass,
               JavaTckSupport.attributeSet(receiver, receiverAttribute)); }
            catch (Exception ignored) { }
         }
         if (receiverJoined && interactionSubscribed && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && objectPublished && ownerObjectClass != null && ownerAttribute != null) {
            try { owner.unpublishObjectClassAttributes(ownerObjectClass,
               JavaTckSupport.attributeSet(owner, ownerAttribute)); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && interactionPublished && ownerInteraction != null) {
            try { owner.unpublishInteractionClass(ownerInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador receiver, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private FederateHandle producer;
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values) {
         this.object = object;
         this.values = values;
      }
   }

   private static final class Recorder {
      private final CopyOnWriteArrayList<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length == 4) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.name = (String) arguments[2];
                  discoveries.add(event);
               } else if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     event.parameters.put(handle, source.get(handle).clone());
                  }
                  event.producer = (FederateHandle) arguments[4];
                  interactions.add(event);
               } else if ("reflectAttributeValues".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     byte[] value = source.get(handle);
                     values.put(handle, value == null ? null : value.clone());
                  }
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], values));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) {
                     return "Java TCK handle-parameter callback recorder";
                  }
                  if ("hashCode".equals(method.getName())) {
                     return System.identityHashCode(proxy);
                  }
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
