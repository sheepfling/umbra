package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Re-evaluate ordinary attribute subscriptions at the callback boundary. */
final class DelaySubscriptionEvaluationAttributeTck {
   private DelaySubscriptionEvaluationAttributeTck() { }

   static void run(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel model) throws Exception {
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         objectClassName, attributeName, model, false);
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         objectClassName, attributeName, model, true);
   }

   private static void runConfiguration(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel model, boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder events = new Recorder();
      String federation = "java-tck-delay-subscription-attribute-" + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      try {
         publisher.connect(noopAmbassador(), model);
         publisherConnected = true;
         receiver.connect(events.proxy(), model);
         receiverConnected = true;
         String[] modules = enabled ? new String[] {fom, switchesFom} : new String[] {fom};
         publisher.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-delay-attribute-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-delay-attribute-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "adapter FOM returned inconsistent object/attribute handles");
         JavaTckSupport.check(publisher.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "Delay Subscription Evaluation switch did not match the composed FOM");

         publisher.publishObjectClassAttributes(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute));
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         if (model == CallbackModel.HLA_EVOKED) {
            while (receiver.evokeCallback(0.0)) { }
         }
         JavaTckSupport.check(object.equals(events.discoveredObject),
            "receiver did not discover the registered object");
         JavaTckSupport.check(receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "discovery did not establish the expected known object class");
         receiver.unsubscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));

         byte[] value = new byte[] {(byte) 0xD5, 0x45};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         if (model == CallbackModel.HLA_IMMEDIATE) receiver.disableCallbacks();
         publisher.updateAttributeValues(object, values, new byte[0]);
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         callbackBoundary(receiver, model);
         Reflection first = events.lastOrNull();
         JavaTckSupport.check(!enabled || first != null,
            "enabled Delay Subscription Evaluation discarded the attribute update");
         if (first != null) {
            verify(first, object, receiverAttribute, value, producer, receiver,
               "pre-subscription update");
         }

         events.clear();
         if (model == CallbackModel.HLA_IMMEDIATE) receiver.disableCallbacks();
         publisher.updateAttributeValues(object, values, new byte[0]);
         receiver.unsubscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         callbackBoundary(receiver, model);
         JavaTckSupport.check(events.empty(),
            "callback-boundary attribute unsubscription did not suppress reflection");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static FederateAmbassador noopAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK publisher callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }

   private static void callbackBoundary(RTIambassador receiver, CallbackModel model)
         throws Exception {
      if (model == CallbackModel.HLA_IMMEDIATE) receiver.enableCallbacks();
      else while (receiver.evokeCallback(0.0)) { }
   }

   private static void verify(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, FederateHandle producer,
         RTIambassador receiver, String phase) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 1
            && reflection.values.containsKey(attribute)
            && Arrays.equals(value, reflection.values.get(attribute))
            && reflection.tag.length == 0
            && producer.equals(reflection.producer)
            && reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regionsEmpty,
         phase + " lost standard attribute-reflection metadata");
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      final ObjectInstanceHandle object;
      final AttributeHandleValueMap values;
      final byte[] tag;
      final TransportationTypeHandle transportation;
      final FederateHandle producer;
      final boolean regionsEmpty;
      Reflection(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         values = ((AttributeHandleValueMap) args[1]).clone();
         tag = ((byte[]) args[2]).clone();
         transportation = (TransportationTypeHandle) args[3];
         producer = (FederateHandle) args[4];
         RegionHandleSet regions = (RegionHandleSet) args[5];
         regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class Recorder {
      volatile ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               if ("discoverObjectInstance".equals(method.getName()) && a.length == 4) {
                  discoveredObject = (ObjectInstanceHandle) a[0];
               } else if ("reflectAttributeValues".equals(method.getName()) && a.length == 6) {
                  reflections.add(new Reflection(a));
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK delayed attribute subscriptions";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == a[0];
               }
               return null;
            });
      }
      Reflection lastOrNull() { return reflections.isEmpty() ? null : reflections.get(reflections.size() - 1); }
      boolean empty() { return reflections.isEmpty(); }
      void clear() { reflections.clear(); }
   }
}
