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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Reissues owner advisories when an active attribute's maximum rate changes. */
final class AttributeRelevanceRateReissueTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AttributeRelevanceRateReissueTck() { }

   static void run(RtiFactory factory, String rateFom, String timeImplementation,
         String objectClassName, String attributeName, String fastRateName, String slowRateName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador fastSubscriber = factory.getRtiAmbassador();
      RTIambassador slowSubscriber = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder fastRecorder = new Recorder();
      Recorder slowRecorder = new Recorder();
      String federation = "java-tck-attribute-relevance-rate-reissue-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean fastConnected = false;
      boolean slowConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean fastJoined = false;
      boolean slowJoined = false;
      boolean published = false;
      boolean fastSubscribed = false;
      boolean slowSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle fastClass = null;
      ObjectClassHandle slowClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle fastAttribute = null;
      AttributeHandle slowAttribute = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         fastSubscriber.connect(fastRecorder.proxy(), callbackModel);
         fastConnected = true;
         slowSubscriber.connect(slowRecorder.proxy(), callbackModel);
         slowConnected = true;
         owner.createFederationExecution(federation, rateFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-rate-reissue-owner", federation);
         ownerJoined = true;
         fastSubscriber.joinFederationExecution("java-tck-rate-reissue-fast", federation);
         fastJoined = true;
         slowSubscriber.joinFederationExecution("java-tck-rate-reissue-slow", federation);
         slowJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         fastClass = fastSubscriber.getObjectClassHandle(objectClassName);
         slowClass = slowSubscriber.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         fastAttribute = fastSubscriber.getAttributeHandle(fastClass, attributeName);
         slowAttribute = slowSubscriber.getAttributeHandle(slowClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(fastClass) && ownerClass.equals(slowClass)
               && ownerAttribute.equals(fastAttribute) && ownerAttribute.equals(slowAttribute),
            "adapter rate FOM handles did not retain identity across federates");

         double fastRate = fastSubscriber.getUpdateRateValue(fastRateName);
         double slowRate = slowSubscriber.getUpdateRateValue(slowRateName);
         JavaTckSupport.check(fastRate > slowRate && slowRate > 0.0,
            "adapter update rates must be positive with fast rate greater than slow rate");
         JavaTckSupport.check(owner.getAttributeRelevanceAdvisorySwitch()
               && fastSubscriber.getAttributeRelevanceAdvisorySwitch()
               && slowSubscriber.getAttributeRelevanceAdvisorySwitch(),
            "adapter did not enable the standard attribute relevance advisory switch");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet fastAttributes =
            JavaTckSupport.attributeSet(fastSubscriber, fastAttribute);
         AttributeHandleSet slowAttributes =
            JavaTckSupport.attributeSet(slowSubscriber, slowAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);

         fastSubscriber.subscribeObjectClassAttributes(fastClass, fastAttributes, fastRateName);
         fastSubscribed = true;
         await(fastSubscriber, callbackModel, objectClassName,
            () -> fastRecorder.discoveries.contains(object), "fast subscriber object discovery");
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.updatesOn.isEmpty(), "initial fast-rate Turn Updates On");
         requireTurnOn(ownerRecorder, object, ownerAttribute, fastRateName,
            "initial fast-rate advisory");
         ownerRecorder.clearAdvisories();

         slowSubscriber.subscribeObjectClassAttributes(slowClass, slowAttributes, slowRateName);
         slowSubscribed = true;
         await(slowSubscriber, callbackModel, objectClassName,
            () -> slowRecorder.discoveries.contains(object), "slow subscriber object discovery");
         for (int pass = 0; pass < 8; ++pass) service(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.updatesOn.isEmpty()
               && ownerRecorder.updatesOff.isEmpty(),
            "adding a lower-rate subscriber spuriously reissued a relevance advisory");

         fastSubscriber.subscribeObjectClassAttributes(fastClass, fastAttributes, slowRateName);
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.updatesOn.isEmpty(), "lower-maximum-rate advisory reissue");
         requireTurnOn(ownerRecorder, object, ownerAttribute, slowRateName,
            "lower-maximum-rate advisory reissue");
         ownerRecorder.clearAdvisories();

         fastSubscriber.subscribeObjectClassAttributes(fastClass, fastAttributes, fastRateName);
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.updatesOn.isEmpty(), "restored-maximum-rate advisory reissue");
         requireTurnOn(ownerRecorder, object, ownerAttribute, fastRateName,
            "restored-maximum-rate advisory reissue");
         ownerRecorder.clearAdvisories();

         fastSubscriber.unsubscribeObjectClassAttributes(fastClass, fastAttributes);
         fastSubscribed = false;
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.updatesOn.isEmpty(), "fast-peer-removal advisory reissue");
         requireTurnOn(ownerRecorder, object, ownerAttribute, slowRateName,
            "fast-peer-removal advisory reissue");
         ownerRecorder.clearAdvisories();

         slowSubscriber.unsubscribeObjectClassAttributes(slowClass, slowAttributes);
         slowSubscribed = false;
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.updatesOff.isEmpty(), "final Turn Updates Off advisory");
         JavaTckSupport.check(ownerRecorder.updatesOff.size() == 1
               && object.equals(ownerRecorder.updatesOff.get(0).object)
               && ownerRecorder.updatesOff.get(0).attributes.size() == 1
               && ownerRecorder.updatesOff.get(0).attributes.contains(ownerAttribute)
               && ownerRecorder.updatesOn.isEmpty(),
            "final Turn Updates Off lost its object/attribute or emitted an extra Turn On");
      } finally {
         if (fastJoined && fastSubscribed && fastClass != null && fastAttribute != null) {
            try { fastSubscriber.unsubscribeObjectClassAttributes(fastClass,
               JavaTckSupport.attributeSet(fastSubscriber, fastAttribute)); }
            catch (Exception ignored) { }
         }
         if (slowJoined && slowSubscribed && slowClass != null && slowAttribute != null) {
            try { slowSubscriber.unsubscribeObjectClassAttributes(slowClass,
               JavaTckSupport.attributeSet(slowSubscriber, slowAttribute)); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published && ownerClass != null && ownerAttribute != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass,
               JavaTckSupport.attributeSet(owner, ownerAttribute)); }
            catch (Exception ignored) { }
         }
         if (fastJoined) resign(fastSubscriber, ResignAction.NO_ACTION);
         if (slowJoined) resign(slowSubscriber, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (slowConnected) disconnect(slowSubscriber);
         if (fastConnected) disconnect(fastSubscriber);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void requireTurnOn(Recorder recorder, ObjectInstanceHandle object,
         AttributeHandle attribute, String expectedRate, String description) {
      JavaTckSupport.check(recorder.updatesOn.size() == 1
            && recorder.updatesOn.get(0).object.equals(object)
            && recorder.updatesOn.get(0).attributes.size() == 1
            && recorder.updatesOn.get(0).attributes.contains(attribute)
            && expectedRate.equals(recorder.updatesOn.get(0).updateRate)
            && recorder.updatesOff.isEmpty(),
         description + " returned an incorrect Turn Updates On advisory");
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         String objectClassName, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         service(ambassador, callbackModel, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel callbackModel,
         String objectClassName) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class UpdateAdvisory {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final String updateRate;

      private UpdateAdvisory(ObjectInstanceHandle object, AttributeHandleSet attributes,
            String updateRate) {
         this.object = object;
         this.attributes = attributes;
         this.updateRate = updateRate;
      }
   }

   private static final class Recorder {
      private final List<UpdateAdvisory> updatesOn = new CopyOnWriteArrayList<>();
      private final List<UpdateAdvisory> updatesOff = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private void clearAdvisories() {
         updatesOn.clear();
         updatesOff.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length > 0) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("turnUpdatesOnForObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  updatesOn.add(new UpdateAdvisory((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleSet) arguments[1]).clone(),
                     arguments.length >= 3 ? (String) arguments[2] : null));
               } else if ("turnUpdatesOffForObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  updatesOff.add(new UpdateAdvisory((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleSet) arguments[1]).clone(), null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK rate-reissue callbacks";
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
