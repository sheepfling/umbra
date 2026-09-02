package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** P2.5 ordinary order and transportation controls. */
final class TransportOrderTck {
   private TransportOrderTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-transport-order-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), CallbackModel.HLA_EVOKED);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), CallbackModel.HLA_EVOKED);
         observerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-transport-owner", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-transport-observer", federation);
         observerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(observerClass,
            attributeName);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet observerAttributes = JavaTckSupport.attributeSet(observer,
            observerAttribute);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle observerInteraction =
            observer.getInteractionClassHandle(interactionClassName);
         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort = owner.getTransportationTypeHandle("HLAbestEffort");

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         JavaTckSupport.expectFailure(
            () -> owner.changeInteractionOrderType(ownerInteraction, OrderType.RECEIVE),
            "InteractionClassNotPublished", "unpublished interaction order change was accepted");
         owner.publishInteractionClass(ownerInteraction);
         observer.subscribeInteractionClass(observerInteraction);

         // The FOM's ordinary defaults start reliable for transport and
         // timestamped for order.  Exercise both default and per-instance
         // order controls before using the receive-order data path.
         owner.changeDefaultAttributeOrderType(ownerClass, ownerAttributes, OrderType.RECEIVE);
         ObjectInstanceHandle first = owner.registerObjectInstance(ownerClass);
         owner.changeAttributeOrderType(first, ownerAttributes, OrderType.TIMESTAMP);
         owner.changeAttributeOrderType(first, ownerAttributes, OrderType.RECEIVE);
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         owner.changeInteractionOrderType(ownerInteraction, OrderType.RECEIVE);

         // Changing the class default only affects subsequently registered
         // instances.  Query callbacks make the corresponding transport
         // default observable without depending on a provider handle number.
         owner.changeDefaultAttributeTransportationType(ownerClass, ownerAttributes, bestEffort);
         ObjectInstanceHandle second = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.drain(observer);
         JavaTckSupport.check(observerRecorder.discoveries.contains(first)
               && observerRecorder.discoveries.contains(second),
            "observer did not discover both transport-control objects");

         owner.queryAttributeTransportationType(first, ownerAttribute);
         owner.queryAttributeTransportationType(second, ownerAttribute);
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.attributeReports.size() >= 2,
            "attribute transportation queries did not produce report callbacks");
         AttributeTransportReport firstReport =
            ownerRecorder.attributeReports.get(ownerRecorder.attributeReports.size() - 2);
         AttributeTransportReport secondReport =
            ownerRecorder.attributeReports.get(ownerRecorder.attributeReports.size() - 1);
         JavaTckSupport.check(first.equals(firstReport.object)
               && sameTransport(owner, firstReport.transportation, reliable),
            "first object did not retain the reliable FOM/default transport");
         JavaTckSupport.check(second.equals(secondReport.object)
               && sameTransport(owner, secondReport.transportation, bestEffort),
            "second object did not capture the best-effort class default");

         // Requests commit at the owner callback boundary.  A duplicate while
         // pending is a standard failure; data sent before confirmation keeps
         // the old transport and data sent after confirmation uses the new one.
         owner.requestAttributeTransportationTypeChange(first, ownerAttributes, bestEffort);
         JavaTckSupport.expectFailure(
            () -> owner.requestAttributeTransportationTypeChange(first, ownerAttributes,
               reliable),
            "AttributeAlreadyBeingChanged", "duplicate attribute transport request was accepted");
         byte[] beforeTag = new byte[] {0x01};
         owner.updateAttributeValues(first, oneValue(owner, ownerAttribute, (byte) 0x01), beforeTag);
         JavaTckSupport.drain(observer);
         JavaTckSupport.check(observerRecorder.attributeReflections.size() >= 1,
            "pre-confirmation attribute update was not reflected");
         AttributeReflection before = observerRecorder.attributeReflections.get(
            observerRecorder.attributeReflections.size() - 1);
         JavaTckSupport.check(first.equals(before.object)
               && sameTransport(observer, before.transportation, observer.getTransportationTypeHandle(
                  "HLAreliable")),
            "pre-confirmation attribute update did not use reliable transport");
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.attributeConfirmations.size() == 1,
            "attribute transport request did not produce one confirmation callback");
         AttributeTransportConfirmation confirmation = ownerRecorder.attributeConfirmations.get(0);
         JavaTckSupport.check(first.equals(confirmation.object)
               && confirmation.attributes.contains(ownerAttribute)
               && sameTransport(owner, confirmation.transportation, bestEffort),
            "attribute transport confirmation did not commit best effort");

         byte[] afterTag = new byte[] {0x02};
         owner.updateAttributeValues(first, oneValue(owner, ownerAttribute, (byte) 0x02), afterTag);
         JavaTckSupport.drain(observer);
         AttributeReflection after = observerRecorder.attributeReflections.get(
            observerRecorder.attributeReflections.size() - 1);
         JavaTckSupport.check(first.equals(after.object)
               && sameTransport(observer, after.transportation,
                  observer.getTransportationTypeHandle("HLAbestEffort")),
            "post-confirmation attribute update did not use best-effort transport");
         owner.queryAttributeTransportationType(first, ownerAttribute);
         JavaTckSupport.drain(owner);
         AttributeTransportReport finalReport = ownerRecorder.attributeReports.get(
            ownerRecorder.attributeReports.size() - 1);
         JavaTckSupport.check(sameTransport(owner, finalReport.transportation, bestEffort),
            "attribute transportation query did not observe the committed change");

         // Interaction transport has the same callback commit boundary, but
         // its query is addressed to the publishing federate.
         owner.requestInteractionTransportationTypeChange(ownerInteraction, bestEffort);
         JavaTckSupport.expectFailure(
            () -> owner.requestInteractionTransportationTypeChange(ownerInteraction, reliable),
            "InteractionClassAlreadyBeingChanged",
            "duplicate interaction transport request was accepted");
         owner.sendInteraction(ownerInteraction, emptyParameters(owner), new byte[] {0x03});
         JavaTckSupport.drain(observer);
         JavaTckSupport.check(observerRecorder.interactionReceives.size() >= 1,
            "pre-confirmation interaction was not received");
         InteractionReceive interactionBefore = observerRecorder.interactionReceives.get(
            observerRecorder.interactionReceives.size() - 1);
         JavaTckSupport.check(sameTransport(observer, interactionBefore.transportation,
            observer.getTransportationTypeHandle("HLAreliable")),
            "pre-confirmation interaction did not use reliable transport");
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.interactionConfirmations.size() == 1,
            "interaction transport request did not produce one confirmation callback");
         JavaTckSupport.check(ownerInteraction.equals(
               ownerRecorder.interactionConfirmations.get(0).interaction)
               && sameTransport(owner, ownerRecorder.interactionConfirmations.get(0).transportation,
                  bestEffort),
            "interaction transport confirmation did not commit best effort");
         owner.sendInteraction(ownerInteraction, emptyParameters(owner), new byte[] {0x04});
         JavaTckSupport.drain(observer);
         InteractionReceive interactionAfter = observerRecorder.interactionReceives.get(
            observerRecorder.interactionReceives.size() - 1);
         JavaTckSupport.check(sameTransport(observer, interactionAfter.transportation,
            observer.getTransportationTypeHandle("HLAbestEffort")),
            "post-confirmation interaction did not use best-effort transport");
         owner.queryInteractionTransportationType(ownerFederate, ownerInteraction);
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.interactionReports.size() == 1,
            "interaction transportation query did not produce one report callback");
         JavaTckSupport.check(ownerInteraction.equals(ownerRecorder.interactionReports.get(0).interaction)
               && sameTransport(owner, ownerRecorder.interactionReports.get(0).transportation,
                  bestEffort),
            "interaction transportation query did not observe the committed change");

         observer.unsubscribeInteractionClass(observerInteraction);
         observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes);
         owner.unpublishInteractionClass(ownerInteraction);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         owner.unpublishObjectClass(ownerClass);
      } finally {
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static AttributeHandleValueMap oneValue(RTIambassador ambassador,
         AttributeHandle attribute, byte value) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, new byte[] {value});
      return values;
   }

   private static ParameterHandleValueMap emptyParameters(RTIambassador ambassador)
         throws Exception {
      return ambassador.getParameterHandleValueMapFactory().create(0);
   }

   private static boolean sameTransport(RTIambassador ambassador,
         TransportationTypeHandle actual, TransportationTypeHandle expected) throws Exception {
      return actual != null && expected != null
         && expected.equals(actual)
         && ambassador.getTransportationTypeName(actual)
            .equals(ambassador.getTransportationTypeName(expected));
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

   private static final class AttributeTransportConfirmation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final TransportationTypeHandle transportation;

      private AttributeTransportConfirmation(ObjectInstanceHandle object,
            AttributeHandleSet attributes, TransportationTypeHandle transportation) {
         this.object = object;
         this.attributes = attributes;
         this.transportation = transportation;
      }
   }

   private static final class AttributeTransportReport {
      private final ObjectInstanceHandle object;
      private final AttributeHandle attribute;
      private final TransportationTypeHandle transportation;

      private AttributeTransportReport(ObjectInstanceHandle object, AttributeHandle attribute,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.attribute = attribute;
         this.transportation = transportation;
      }
   }

   private static final class AttributeReflection {
      private final ObjectInstanceHandle object;
      private final TransportationTypeHandle transportation;

      private AttributeReflection(ObjectInstanceHandle object,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.transportation = transportation;
      }
   }

   private static final class InteractionTransportConfirmation {
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private InteractionTransportConfirmation(InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class InteractionTransportReport {
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private InteractionTransportReport(InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class InteractionReceive {
      private final TransportationTypeHandle transportation;

      private InteractionReceive(TransportationTypeHandle transportation) {
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<AttributeTransportConfirmation> attributeConfirmations =
         new ArrayList<>();
      private final List<AttributeTransportReport> attributeReports = new ArrayList<>();
      private final List<InteractionTransportConfirmation> interactionConfirmations =
         new ArrayList<>();
      private final List<InteractionTransportReport> interactionReports = new ArrayList<>();
      private final List<AttributeReflection> attributeReflections = new ArrayList<>();
      private final List<InteractionReceive> interactionReceives = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("confirmAttributeTransportationTypeChange".equals(name)) {
                  attributeConfirmations.add(new AttributeTransportConfirmation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("reportAttributeTransportationType".equals(name)) {
                  attributeReports.add(new AttributeTransportReport(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("confirmInteractionTransportationTypeChange".equals(name)) {
                  interactionConfirmations.add(new InteractionTransportConfirmation(
                     (InteractionClassHandle) arguments[0],
                     (TransportationTypeHandle) arguments[1]));
               } else if ("reportInteractionTransportationType".equals(name)) {
                  interactionReports.add(new InteractionTransportReport(
                     (InteractionClassHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("reflectAttributeValues".equals(name) && arguments.length == 6) {
                  attributeReflections.add(new AttributeReflection(
                     (ObjectInstanceHandle) arguments[0],
                     (TransportationTypeHandle) arguments[3]));
               } else if ("receiveInteraction".equals(name) && arguments.length == 6) {
                  interactionReceives.add(new InteractionReceive(
                     (TransportationTypeHandle) arguments[3]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK transport callbacks";
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

