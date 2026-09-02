package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** P1.2 ordinary declaration, map-shape, and negative-service matrix. */
final class OrdinaryEdgesTck {
   private OrdinaryEdgesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador evoked = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador passive = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder evokedRecorder = new Recorder();
      Recorder immediateRecorder = new Recorder();
      Recorder passiveRecorder = new Recorder();
      String federation = "java-tck-ordinary-edges-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean evokedConnected = false;
      boolean immediateConnected = false;
      boolean passiveConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean evokedJoined = false;
      boolean immediateJoined = false;
      boolean passiveJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         evoked.connect(evokedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         evokedConnected = true;
         immediate.connect(immediateRecorder.proxy(), CallbackModel.HLA_IMMEDIATE);
         immediateConnected = true;
         passive.connect(passiveRecorder.proxy(), CallbackModel.HLA_EVOKED);
         passiveConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-edge-publisher", federation);
         publisherJoined = true;
         evoked.joinFederationExecution("java-tck-edge-evoked", federation);
         evokedJoined = true;
         immediate.joinFederationExecution("java-tck-edge-immediate", federation);
         immediateJoined = true;
         passive.joinFederationExecution("java-tck-edge-passive", federation);
         passiveJoined = true;

         ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = publisher.getAttributeHandle(objectClass, attributeName);
         AttributeHandle inherited = publisher.getAttributeHandle(objectClass, "Name");
         ObjectClassHandle employee = publisher.getObjectClassHandle("HLAobjectRoot.Employee");
         AttributeHandle employeeInherited = publisher.getAttributeHandle(employee, "Name");
         ObjectClassHandle drink = publisher.getObjectClassHandle("HLAobjectRoot.Food.Drink");
         AttributeHandle unrelatedAttribute = publisher.getAttributeHandle(drink, "NumberCups");
         InteractionClassHandle interactionClass =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle parentInteraction =
            publisher.getInteractionClassHandle("HLAinteractionRoot.CustomerTransactions");
         ParameterHandle parameter = publisher.getParameterHandle(interactionClass, parameterName);
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(
            publisher, attribute, inherited);
         JavaTckSupport.check(inherited.equals(employeeInherited),
            "inherited attribute lookup did not preserve the parent handle identity");

         // Repeated declarations are accepted as idempotent ordinary state.
         publisher.publishObjectClassAttributes(objectClass, attributes);
         publisher.publishObjectClassAttributes(objectClass, attributes);
         evoked.subscribeObjectClassAttributes(objectClass, attributes);
         evoked.subscribeObjectClassAttributes(objectClass, attributes);
         immediate.subscribeObjectClassAttributes(objectClass, attributes);
         passive.subscribeObjectClassAttributesPassively(objectClass, attributes);
         publisher.publishInteractionClass(interactionClass);
         publisher.publishInteractionClass(interactionClass);
         publisher.publishInteractionClass(parentInteraction);
         evoked.subscribeInteractionClass(interactionClass);
         evoked.subscribeInteractionClass(interactionClass);
         immediate.subscribeInteractionClass(interactionClass);
         passive.subscribeInteractionClassPassively(interactionClass);

         // A valid object gives the negative paths a standard-known identity.
         ObjectInstanceHandle object = publisher.registerObjectInstance(objectClass);
         publisher.publishObjectClassAttributes(drink,
            JavaTckSupport.attributeSet(publisher, unrelatedAttribute));
         ObjectInstanceHandle unknownObject = publisher.registerObjectInstance(drink);
         JavaTckSupport.drain(evoked);
         JavaTckSupport.check(evokedRecorder.discoveries == 1,
            "ordinary edge subscriber did not receive the known-class discovery callback");
         JavaTckSupport.check(immediateRecorder.discoveries == 1,
            "immediate ordinary edge subscriber did not receive the known-class discovery callback");
         JavaTckSupport.check(passiveRecorder.discoveries == 0,
            "passive ordinary edge subscription created an object discovery callback");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
         byte[] first = new byte[] {0x11};
         byte[] replacement = new byte[] {0x12};
         byte[] inheritedValue = new byte[] {0x21};
         JavaTckSupport.check(values.put(attribute, first) == null,
            "first attribute map insertion unexpectedly replaced a value");
         JavaTckSupport.check(Arrays.equals(values.put(attribute, replacement), first),
            "duplicate attribute key did not replace the prior value");
         values.put(inherited, inheritedValue);
         JavaTckSupport.check(values.size() == 2,
            "duplicate attribute key created a second map entry");
         byte[] tag = new byte[] {0x41, 0x42, 0x43};
         publisher.updateAttributeValues(object, values, tag);
         JavaTckSupport.drain(evoked);
         JavaTckSupport.check(evokedRecorder.reflections == 1,
            "ordinary edge update did not produce one evoked reflection");
         JavaTckSupport.check(evokedRecorder.lastReflectedValues.size() == 2,
            "ordinary edge reflection lost a valid attribute map entry");
         JavaTckSupport.check(Arrays.equals(evokedRecorder.lastTag, tag),
            "ordinary reflection did not preserve the user tag bytes");
         JavaTckSupport.check(evokedRecorder.lastTag != tag,
            "ordinary reflection reused the sender's mutable tag array");
         JavaTckSupport.check(immediateRecorder.reflections == 1,
            "immediate ordinary edge subscriber did not receive the reflection directly");
         JavaTckSupport.check(passiveRecorder.reflections == 0,
            "passive ordinary edge subscription received a reflection");

         // Empty ordinary passels are valid Java map values and must not force
         // a provider-specific data representation.
         AttributeHandleValueMap emptyAttributes =
            publisher.getAttributeHandleValueMapFactory().create(0);
         publisher.updateAttributeValues(object, emptyAttributes, new byte[0]);

         AttributeHandleValueMap invalidAttributes =
            publisher.getAttributeHandleValueMapFactory().create(1);
         invalidAttributes.put(unrelatedAttribute, new byte[] {0x01});
         JavaTckSupport.expectFailure(
            () -> publisher.updateAttributeValues(object, invalidAttributes, new byte[0]),
            "AttributeNotDefined", "mixed-invalid attribute map was accepted");

         ParameterHandleValueMap invalidParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         invalidParameters.put(parameter, new byte[] {0x02});
         JavaTckSupport.expectFailure(
            () -> publisher.sendInteraction(
               parentInteraction, invalidParameters, new byte[0]),
            "InteractionParameterNotDefined", "mixed-invalid parameter map was accepted");

         // Publication and ownership are independent negative boundaries.
         JavaTckSupport.expectFailure(
            () -> evoked.sendInteraction(
               interactionClass,
               evoked.getParameterHandleValueMapFactory().create(0),
               new byte[0]),
            "InteractionClassNotPublished", "unpublished interaction was sent");
         JavaTckSupport.expectFailure(
            () -> evoked.updateAttributeValues(object, values, new byte[0]),
            "AttributeNotOwned", "non-owner updated a published object");
         JavaTckSupport.expectFailure(
            () -> evoked.requestAttributeValueUpdate(
               unknownObject, attributes, new byte[0]),
            "ObjectInstanceNotKnown", "unknown object was accepted by an update request");
         JavaTckSupport.expectFailure(
            () -> evoked.getKnownObjectClassHandle(
               unknownObject),
            "ObjectInstanceNotKnown", "unknown object was accepted by a class query");

         // A member without an active declaration remains joined but is not a
         // recipient. Repeated removal is an idempotent cleanup operation.
         publisher.updateAttributeValues(object, values, new byte[] {0x55});
         JavaTckSupport.drain(passive);
         JavaTckSupport.check(passiveRecorder.reflections == 0,
            "a member with only passive declarations received ordinary data");
         passive.unsubscribeObjectClassAttributes(objectClass, attributes);
         passive.unsubscribeObjectClassAttributes(objectClass, attributes);
         passive.unsubscribeObjectClass(objectClass);
         passive.unsubscribeObjectClass(objectClass);
         passive.unsubscribeInteractionClass(interactionClass);
         passive.unsubscribeInteractionClass(interactionClass);

         evoked.unsubscribeInteractionClass(interactionClass);
         evoked.unsubscribeInteractionClass(interactionClass);
         evoked.unsubscribeObjectClassAttributes(objectClass, attributes);
         evoked.unsubscribeObjectClassAttributes(objectClass, attributes);
         evoked.unsubscribeObjectClass(objectClass);
         evoked.unsubscribeObjectClass(objectClass);
         publisher.unpublishInteractionClass(interactionClass);
         publisher.unpublishInteractionClass(interactionClass);
         publisher.unpublishInteractionClass(parentInteraction);
         publisher.unpublishObjectClassAttributes(objectClass, attributes);
         publisher.unpublishObjectClassAttributes(objectClass, attributes);
         publisher.unpublishObjectClass(objectClass);
         publisher.unpublishObjectClass(objectClass);
         publisher.unpublishObjectClassAttributes(drink,
            JavaTckSupport.attributeSet(publisher, unrelatedAttribute));
         publisher.unpublishObjectClass(drink);
      } finally {
         if (passiveJoined) resign(passive);
         if (immediateJoined) resign(immediate);
         if (evokedJoined) resign(evoked);
         if (publisherJoined) resign(publisher);
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (passiveConnected) disconnect(passive);
         if (immediateConnected) disconnect(immediate);
         if (evokedConnected) disconnect(evoked);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static final class Recorder {
      private int discoveries;
      private int reflections;
      private byte[] lastTag;
      private AttributeHandleValueMap lastReflectedValues;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())) {
                  discoveries++;
               }
               if ("reflectAttributeValues".equals(method.getName())
                     && arguments != null && arguments.length >= 3) {
                  reflections++;
                  lastReflectedValues = ((AttributeHandleValueMap) arguments[1]).clone();
                  lastTag = ((byte[]) arguments[2]).clone();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK edge callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
