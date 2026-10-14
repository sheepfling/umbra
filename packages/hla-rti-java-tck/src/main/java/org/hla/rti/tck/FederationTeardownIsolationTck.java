package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Keeps named update-rate admission state isolated when another federation is destroyed. */
final class FederationTeardownIsolationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long SUPPRESSION_WINDOW_NANOS = 250_000_000L;

   private FederationTeardownIsolationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String rateName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisherA = factory.getRtiAmbassador();
      RTIambassador receiverA = factory.getRtiAmbassador();
      RTIambassador publisherB = factory.getRtiAmbassador();
      RTIambassador receiverB = factory.getRtiAmbassador();
      Recorder recorderA = new Recorder();
      Recorder recorderB = new Recorder();
      String federationA = "java-tck-rate-isolation-" + UUID.randomUUID();
      String federationB = federationA + "-child";
      State state = new State();
      try {
         publisherA.connect(callbacks(), callbackModel); state.publisherAConnected = true;
         receiverA.connect(recorderA.proxy(), callbackModel); state.receiverAConnected = true;
         publisherB.connect(callbacks(), callbackModel); state.publisherBConnected = true;
         receiverB.connect(recorderB.proxy(), callbackModel); state.receiverBConnected = true;

         publisherA.createFederationExecution(federationA, fom, timeImplementation);
         state.federationACreated = true;
         publisherB.createFederationExecution(federationB, fom, timeImplementation);
         state.federationBCreated = true;
         publisherA.joinFederationExecution("java-tck-rate-a-publisher", federationA);
         state.publisherAJoined = true;
         receiverA.joinFederationExecution("java-tck-rate-a-receiver", federationA);
         state.receiverAJoined = true;
         publisherB.joinFederationExecution("java-tck-rate-b-publisher", federationB);
         state.publisherBJoined = true;
         receiverB.joinFederationExecution("java-tck-rate-b-receiver", federationB);
         state.receiverBJoined = true;

         Fixture fixtureA = configure(publisherA, receiverA, recorderA, callbackModel,
            objectClassName, attributeName, rateName, "Federation A");
         Fixture fixtureB = configure(publisherB, receiverB, recorderB, callbackModel,
            objectClassName, attributeName, rateName, "Federation B");

         recorderA.clearReflections();
         send(publisherA, fixtureA, (byte) 0x11);
         expectReflection(receiverA, recorderA, fixtureA, (byte) 0x11,
            callbackModel, "Federation A first update");
         recorderA.clearReflections();
         send(publisherA, fixtureA, (byte) 0x12);
         observeSuppression(receiverA, recorderA, callbackModel,
            "Federation A second update");

         recorderB.clearReflections();
         send(publisherB, fixtureB, (byte) 0x21);
         expectReflection(receiverB, recorderB, fixtureB, (byte) 0x21,
            callbackModel, "Federation B first update");
         recorderB.clearReflections();
         send(publisherB, fixtureB, (byte) 0x22);
         observeSuppression(receiverB, recorderB, callbackModel,
            "Federation B second update");

         receiverA.resignFederationExecution(ResignAction.NO_ACTION);
         state.receiverAJoined = false;
         publisherA.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         state.publisherAJoined = false;
         publisherA.destroyFederationExecution(federationA);
         state.federationACreated = false;

         recorderB.clearReflections();
         send(publisherB, fixtureB, (byte) 0x23);
         observeSuppression(receiverB, recorderB, callbackModel,
            "Federation B update after Federation A teardown");

         receiverB.resignFederationExecution(ResignAction.NO_ACTION);
         state.receiverBJoined = false;
         publisherB.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         state.publisherBJoined = false;
         publisherB.destroyFederationExecution(federationB);
         state.federationBCreated = false;
      } finally {
         if (state.receiverBJoined) resign(receiverB, ResignAction.NO_ACTION);
         if (state.publisherBJoined) resign(publisherB, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (state.federationBCreated) destroy(publisherB, federationB);
         if (state.receiverAJoined) resign(receiverA, ResignAction.NO_ACTION);
         if (state.publisherAJoined) resign(publisherA, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (state.federationACreated) destroy(publisherA, federationA);
         if (state.receiverBConnected) disconnect(receiverB);
         if (state.publisherBConnected) disconnect(publisherB);
         if (state.receiverAConnected) disconnect(receiverA);
         if (state.publisherAConnected) disconnect(publisherA);
      }
   }

   private static Fixture configure(RTIambassador publisher, RTIambassador receiver,
         Recorder recorder, CallbackModel callbackModel, String objectClassName,
         String attributeName, String rateName, String description) throws Exception {
      ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
      ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
      AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
      AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
      check(publisherClass != null && receiverClass != null && publisherAttribute != null
            && receiverAttribute != null,
         description + " update-rate lookup returned a null standard handle");
      AttributeHandleSet published = JavaTckSupport.attributeSet(publisher, publisherAttribute);
      AttributeHandleSet subscribed = JavaTckSupport.attributeSet(receiver, receiverAttribute);
      publisher.publishObjectClassAttributes(publisherClass, published);
      receiver.subscribeObjectClassAttributes(receiverClass, subscribed, rateName);
      check(receiver.getUpdateRateValue(rateName) > 0.0,
         description + " named update-rate designator did not resolve");
      ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
      await(receiver, callbackModel, () -> object.equals(recorder.discoveredObject),
         description + " object discovery");
      return new Fixture(publisherAttribute, receiverAttribute, object);
   }

   private static void send(RTIambassador publisher, Fixture fixture, byte value)
         throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(fixture.publisherAttribute, new byte[] {value});
      publisher.updateAttributeValues(fixture.object, values, new byte[0]);
   }

   private static void expectReflection(RTIambassador receiver, Recorder recorder,
         Fixture fixture, byte expected, CallbackModel model, String description)
         throws Exception {
      await(receiver, model, () -> !recorder.reflections.isEmpty(), description + " reflection");
      check(recorder.reflections.size() == 1,
         description + " delivered an unexpected reflection count");
      Reflection reflection = recorder.reflections.get(0);
      check(fixture.object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(new byte[] {expected}, reflection.values.get(fixture.receiverAttribute)),
         description + " delivered the wrong object, attribute, or value");
   }

   private static void observeSuppression(RTIambassador receiver, Recorder recorder,
         CallbackModel model, String description) throws Exception {
      long deadline = System.nanoTime() + SUPPRESSION_WINDOW_NANOS;
      while (System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         Thread.sleep(5L);
      }
      check(recorder.reflections.isEmpty(),
         description + " was delivered inside the active named update-rate interval");
   }

   private static void await(RTIambassador receiver, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) throw new AssertionError("timed out waiting for " + description);
         if (model == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> null);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void destroy(RTIambassador ambassador, String federation) {
      try { ambassador.destroyFederationExecution(federation); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static final class Fixture {
      private final AttributeHandle publisherAttribute;
      private final AttributeHandle receiverAttribute;
      private final ObjectInstanceHandle object;

      private Fixture(AttributeHandle publisherAttribute, AttributeHandle receiverAttribute,
            ObjectInstanceHandle object) {
         this.publisherAttribute = publisherAttribute;
         this.receiverAttribute = receiverAttribute;
         this.object = object;
      }
   }

   private static final class State {
      private boolean publisherAConnected, receiverAConnected, publisherBConnected, receiverBConnected;
      private boolean publisherAJoined, receiverAJoined, publisherBJoined, receiverBJoined;
      private boolean federationACreated, federationBCreated;
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
      private volatile ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new java.util.concurrent.CopyOnWriteArrayList<>();

      private void clearReflections() { reflections.clear(); }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               if ("discoverObjectInstance".equals(method.getName()) && args != null
                     && args.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) args[0];
               } else if ("reflectAttributeValues".equals(method.getName()) && args != null
                     && args.length >= 2 && args[1] instanceof AttributeHandleValueMap) {
                  AttributeHandleValueMap source = (AttributeHandleValueMap) args[1];
                  Map<AttributeHandle, byte[]> copy = new LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     copy.put(handle, source.get(handle).clone());
                  }
                  reflections.add(new Reflection((ObjectInstanceHandle) args[0], copy));
               }
               return null;
            });
      }
   }
}
