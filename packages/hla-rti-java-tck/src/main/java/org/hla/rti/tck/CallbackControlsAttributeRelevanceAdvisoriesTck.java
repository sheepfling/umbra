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
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Callback gating for standard per-object attribute relevance advisories. */
final class CallbackControlsAttributeRelevanceAdvisoriesTck {
   private CallbackControlsAttributeRelevanceAdvisoriesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String nameAttributeName, String rateAttributeName,
         String attributeName, String namedRateName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(nameAttributeName != null && !nameAttributeName.trim().isEmpty()
            && rateAttributeName != null && !rateAttributeName.trim().isEmpty()
            && attributeName != null && !attributeName.trim().isEmpty(),
         "adapter must provide non-empty FOM attribute names");
      JavaTckSupport.check(!nameAttributeName.equals(rateAttributeName)
            && !nameAttributeName.equals(attributeName)
            && !rateAttributeName.equals(attributeName),
         "adapter FOM attribute names must identify three distinct attributes");
      JavaTckSupport.check(namedRateName != null && !namedRateName.trim().isEmpty(),
         "adapter must provide a non-empty named update-rate label");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-callback-attribute-relevance-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(
            "java-tck-callback-attribute-relevance-publisher", "publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-callback-attribute-relevance-subscriber", "subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherName = publisher.getAttributeHandle(publisherClass,
            nameAttributeName);
         AttributeHandle subscriberName = subscriber.getAttributeHandle(subscriberClass,
            nameAttributeName);
         AttributeHandle publisherPayRate = publisher.getAttributeHandle(publisherClass,
            rateAttributeName);
         AttributeHandle subscriberPayRate = subscriber.getAttributeHandle(subscriberClass,
            rateAttributeName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(subscriberClass,
            attributeName);
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherName.equals(subscriberName)
               && publisherPayRate.equals(subscriberPayRate)
               && publisherAttribute.equals(subscriberAttribute),
            "standard FOM lookups returned mismatched attribute-relevance handles");

         AttributeHandleSet publisherNameSet =
            JavaTckSupport.attributeSet(publisher, publisherName);
         AttributeHandleSet publisherPayRateSet =
            JavaTckSupport.attributeSet(publisher, publisherPayRate);
         AttributeHandleSet publisherAttributeSet =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet publisherAllSet = JavaTckSupport.attributeSet(publisher,
            publisherName, publisherPayRate, publisherAttribute);
         AttributeHandleSet subscriberNameSet =
            JavaTckSupport.attributeSet(subscriber, subscriberName);
         AttributeHandleSet subscriberPayRateSet =
            JavaTckSupport.attributeSet(subscriber, subscriberPayRate);
         AttributeHandleSet subscriberAttributeSet =
            JavaTckSupport.attributeSet(subscriber, subscriberAttribute);

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         publisher.publishObjectClassAttributes(publisherClass, publisherAllSet);
         await(publisher, callbackModel, objectClassName,
            () -> !publisherRecorder.registrationStarts.isEmpty(),
            "baseline start-registration advisory");
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(subscriber, callbackModel, objectClassName,
            () -> subscriberRecorder.discoveries.contains(object),
            "subscriber discovery of relevance-advisory object");

         for (int pass = 0; pass < 8; pass++) {
            service(publisher, callbackModel, objectClassName);
         }
         publisherRecorder.clearAdvisories();

         publisher.disableCallbacks();
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberPayRateSet);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.updatesOn.isEmpty(),
            "disabled callbacks exposed an unnamed turn-updates-on advisory");
         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.updatesOn.size() == 1,
            "re-enabled unnamed turn-updates-on advisory");
         checkOn(publisherRecorder.updatesOn.get(0), object, publisherPayRate, null,
            "unnamed turn-updates-on advisory");

         publisher.disableCallbacks();
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberPayRateSet);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.updatesOff.isEmpty(),
            "disabled callbacks exposed an unnamed turn-updates-off advisory");
         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.updatesOff.size() == 1,
            "re-enabled unnamed turn-updates-off advisory");
         checkOff(publisherRecorder.updatesOff.get(0), object, publisherPayRate,
            "unnamed turn-updates-off advisory");

         publisherRecorder.clearAdvisories();
         publisher.disableCallbacks();
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributeSet,
            namedRateName);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.updatesOn.isEmpty(),
            "disabled callbacks exposed a named turn-updates-on advisory");
         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.updatesOn.size() == 1,
            "re-enabled named turn-updates-on advisory");
         checkOn(publisherRecorder.updatesOn.get(0), object, publisherAttribute, namedRateName,
            "named turn-updates-on advisory");

         publisher.disableCallbacks();
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributeSet);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.updatesOff.isEmpty(),
            "disabled callbacks exposed a named turn-updates-off advisory");
         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.updatesOff.size() == 1,
            "re-enabled named turn-updates-off advisory");
         checkOff(publisherRecorder.updatesOff.get(0), object, publisherAttribute,
            "named turn-updates-off advisory");
      } finally {
         if (subscriberJoined) {
            try {
               subscriber.unsubscribeObjectClassAttributes(
                  subscriber.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(subscriber,
                     subscriber.getAttributeHandle(subscriber.getObjectClassHandle(objectClassName),
                        nameAttributeName)));
            } catch (Exception ignored) { }
            try { subscriber.unsubscribeObjectClass(
               subscriber.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
            resign(subscriber, ResignAction.NO_ACTION);
         }
         if (publisherJoined) {
            try {
               publisher.unpublishObjectClassAttributes(
                  publisher.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                        nameAttributeName),
                     publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                        rateAttributeName),
                     publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            try { publisher.unpublishObjectClass(
               publisher.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
            resign(publisher, ResignAction.DELETE_OBJECTS);
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      for (int attempt = 0; attempt < 1000 && !condition.isTrue(); attempt++) {
         service(ambassador, model, objectClassName);
         Thread.sleep(10);
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private static void checkOn(UpdateAdvisory advisory, ObjectInstanceHandle object,
         AttributeHandle attribute, String updateRate, String description) throws Exception {
      JavaTckSupport.check(object.equals(advisory.object)
            && advisory.attributes.size() == 1
            && advisory.attributes.contains(attribute)
            && (updateRate == null ? advisory.updateRate == null
               : updateRate.equals(advisory.updateRate)),
         description + " returned incorrect object, attributes, or update-rate designator");
   }

   private static void checkOff(UpdateAdvisory advisory, ObjectInstanceHandle object,
         AttributeHandle attribute, String description) throws Exception {
      JavaTckSupport.check(object.equals(advisory.object)
            && advisory.attributes.size() == 1
            && advisory.attributes.contains(attribute)
            && advisory.updateRate == null,
         description + " returned incorrect object, attributes, or update-rate metadata");
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private interface Condition {
      boolean isTrue();
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
      private final List<ObjectClassHandle> registrationStarts = new ArrayList<>();
      private final List<UpdateAdvisory> updatesOn = new ArrayList<>();
      private final List<UpdateAdvisory> updatesOff = new ArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();

      private void clearAdvisories() {
         registrationStarts.clear();
         updatesOn.clear();
         updatesOff.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("startRegistrationForObjectClass".equals(name)) {
                  registrationStarts.add((ObjectClassHandle) arguments[0]);
               } else if ("turnUpdatesOnForObjectInstance".equals(name)) {
                  updatesOn.add(new UpdateAdvisory((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleSet) arguments[1]).clone(),
                     arguments.length == 3 ? (String) arguments[2] : null));
               } else if ("turnUpdatesOffForObjectInstance".equals(name)) {
                  updatesOff.add(new UpdateAdvisory((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleSet) arguments[1]).clone(), null));
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute relevance callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
