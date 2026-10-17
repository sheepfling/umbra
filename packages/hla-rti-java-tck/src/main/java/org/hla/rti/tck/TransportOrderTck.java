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
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/** P2.5 ordinary order and transportation controls. */
final class TransportOrderTck {
   private TransportOrderTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
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
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
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
          ObjectClassHandle invalidObjectClass =
             JavaTckSupport.invalidHandle(ObjectClassHandle.class);
          AttributeHandleSet invalidAttributes = JavaTckSupport.attributeSet(owner,
             JavaTckSupport.invalidHandle(AttributeHandle.class));
          ObjectInstanceHandle invalidObject =
             JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
          InteractionClassHandle invalidInteraction =
             JavaTckSupport.invalidHandle(InteractionClassHandle.class);
          TransportationTypeHandle invalidTransportation =
             JavaTckSupport.invalidHandle(TransportationTypeHandle.class);

          JavaTckSupport.expectFailure(
             () -> owner.changeDefaultAttributeOrderType(invalidObjectClass,
                ownerAttributes, OrderType.RECEIVE),
             "ObjectClassNotDefined", "default attribute order accepted an invalid class");
          JavaTckSupport.expectFailure(
             () -> owner.changeDefaultAttributeOrderType(ownerClass,
                invalidAttributes, OrderType.RECEIVE),
             "AttributeNotDefined", "default attribute order accepted an invalid attribute");
          JavaTckSupport.expectFailure(
             () -> owner.changeDefaultAttributeTransportationType(invalidObjectClass,
                ownerAttributes, bestEffort),
             "ObjectClassNotDefined", "default attribute transport accepted an invalid class");
          JavaTckSupport.expectFailure(
             () -> owner.changeDefaultAttributeTransportationType(ownerClass,
                invalidAttributes, bestEffort),
             "AttributeNotDefined", "default attribute transport accepted an invalid attribute");
          JavaTckSupport.expectFailure(
             () -> owner.changeDefaultAttributeTransportationType(ownerClass,
                ownerAttributes, invalidTransportation),
             "InvalidTransportationTypeHandle",
             "default attribute transport accepted an invalid transportation handle");
          JavaTckSupport.expectFailure(
             () -> owner.requestAttributeTransportationTypeChange(invalidObject,
                ownerAttributes, bestEffort),
             "ObjectInstanceNotKnown", "attribute transport accepted an unknown object");
          JavaTckSupport.expectFailure(
             () -> owner.queryAttributeTransportationType(invalidObject, ownerAttribute),
             "ObjectInstanceNotKnown", "attribute transport query accepted an unknown object");
          JavaTckSupport.expectFailure(
             () -> owner.changeInteractionOrderType(invalidInteraction, OrderType.RECEIVE),
             "InteractionClassNotDefined", "interaction order accepted an invalid class");
          JavaTckSupport.expectFailure(
             () -> owner.requestInteractionTransportationTypeChange(
                invalidInteraction, bestEffort),
             "InteractionClassNotDefined", "interaction transport accepted an invalid class");
          JavaTckSupport.expectFailure(
             () -> owner.queryInteractionTransportationType(ownerFederate, invalidInteraction),
             "InteractionClassNotDefined", "interaction transport query accepted an invalid class");

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         JavaTckSupport.expectFailure(
            () -> owner.changeInteractionOrderType(ownerInteraction, OrderType.RECEIVE),
            "InteractionClassNotPublished", "unpublished interaction order change was accepted");
         owner.publishInteractionClass(ownerInteraction);
         observer.subscribeInteractionClass(observerInteraction);
          JavaTckSupport.expectFailure(
             () -> owner.requestInteractionTransportationTypeChange(
                ownerInteraction, invalidTransportation),
             "InvalidTransportationTypeHandle",
             "published interaction transport accepted an invalid transportation handle");

         // The FOM's ordinary defaults start reliable for transport and
         // timestamped for order.  Exercise both default and per-instance
         // order controls before using the receive-order data path.
         owner.changeDefaultAttributeOrderType(ownerClass, ownerAttributes, OrderType.RECEIVE);
         ObjectInstanceHandle first = owner.registerObjectInstance(ownerClass);
          JavaTckSupport.expectFailure(
             () -> owner.changeAttributeOrderType(invalidObject, ownerAttributes,
                OrderType.RECEIVE),
             "ObjectInstanceNotKnown", "per-instance attribute order accepted an unknown object");
          JavaTckSupport.expectFailure(
             () -> owner.changeAttributeOrderType(first, invalidAttributes, OrderType.TIMESTAMP),
             "AttributeNotDefined", "per-instance attribute order accepted an invalid attribute");
          JavaTckSupport.expectFailure(
             () -> owner.requestAttributeTransportationTypeChange(first,
                invalidAttributes, bestEffort),
             "AttributeNotDefined", "attribute transport accepted an invalid attribute");
          JavaTckSupport.expectFailure(
             () -> owner.requestAttributeTransportationTypeChange(first,
                ownerAttributes, invalidTransportation),
             "InvalidTransportationTypeHandle",
             "attribute transport accepted an invalid transportation handle");
          JavaTckSupport.expectFailure(
             () -> owner.queryAttributeTransportationType(first,
                JavaTckSupport.invalidHandle(AttributeHandle.class)),
             "AttributeNotDefined", "attribute transport query accepted an invalid attribute");
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
         AttributeTransportReport firstReport = findAttributeReport(
            ownerRecorder.attributeReports, first, ownerAttribute);
         AttributeTransportReport secondReport = findAttributeReport(
            ownerRecorder.attributeReports, second, ownerAttribute);
         JavaTckSupport.check(first.equals(firstReport.object)
               && sameTransport(owner, firstReport.transportation, reliable),
            "first object did not retain the reliable FOM/default transport");
         JavaTckSupport.check(second.equals(secondReport.object)
               && sameTransport(owner, secondReport.transportation, bestEffort),
            "second object did not capture the best-effort class default");

         // Evoked callbacks leave the request pending until serviced; immediate
         // callbacks commit it before the request service returns.
         owner.requestAttributeTransportationTypeChange(first, ownerAttributes, bestEffort);
         boolean evoked = callbackModel == CallbackModel.HLA_EVOKED;
         if (evoked) {
            JavaTckSupport.expectFailure(
               () -> owner.requestAttributeTransportationTypeChange(first, ownerAttributes,
                  reliable),
               "AttributeAlreadyBeingChanged",
               "duplicate attribute transport request was accepted while pending");
         }
         byte[] beforeTag = new byte[] {0x01};
         owner.updateAttributeValues(first, oneValue(owner, ownerAttribute, (byte) 0x01), beforeTag);
         JavaTckSupport.drain(observer);
         JavaTckSupport.check(observerRecorder.attributeReflections.size() >= 1,
            "first attribute update after the transport request was not reflected");
         AttributeReflection before = observerRecorder.attributeReflections.get(
            observerRecorder.attributeReflections.size() - 1);
         JavaTckSupport.check(first.equals(before.object)
               && before.values.size() == 1
               && before.values.containsKey(observerAttribute)
               && Arrays.equals(before.values.get(observerAttribute), new byte[] {0x01})
               && Arrays.equals(before.tag, beforeTag)
               && ownerFederate.equals(before.producer)
               && sameTransport(observer, before.transportation, observer.getTransportationTypeHandle(
                  evoked ? "HLAreliable" : "HLAbestEffort")),
            "attribute reflection payload or transport differed for the configured callback model");
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.attributeConfirmations.size() == 1,
            "attribute transport request did not produce one confirmation callback");
         AttributeTransportConfirmation confirmation = ownerRecorder.attributeConfirmations.get(0);
         JavaTckSupport.check(first.equals(confirmation.object)
               && confirmation.attributes.size() == 1
               && confirmation.attributes.contains(ownerAttribute)
               && sameTransport(owner, confirmation.transportation, bestEffort),
            "attribute transport confirmation did not commit best effort");

         byte[] afterTag = new byte[] {0x02};
         owner.updateAttributeValues(first, oneValue(owner, ownerAttribute, (byte) 0x02), afterTag);
         JavaTckSupport.drain(observer);
         AttributeReflection after = observerRecorder.attributeReflections.get(
            observerRecorder.attributeReflections.size() - 1);
         JavaTckSupport.check(first.equals(after.object)
               && after.values.size() == 1
               && after.values.containsKey(observerAttribute)
               && Arrays.equals(after.values.get(observerAttribute), new byte[] {0x02})
               && Arrays.equals(after.tag, afterTag)
               && ownerFederate.equals(after.producer)
               && sameTransport(observer, after.transportation,
                  observer.getTransportationTypeHandle("HLAbestEffort")),
            "post-confirmation attribute reflection payload or transport was incorrect");
         owner.queryAttributeTransportationType(first, ownerAttribute);
         JavaTckSupport.drain(owner);
         AttributeTransportReport finalReport = findAttributeReport(
            ownerRecorder.attributeReports, first, ownerAttribute);
         JavaTckSupport.check(first.equals(finalReport.object)
               && ownerAttribute.equals(finalReport.attribute)
               && sameTransport(owner, finalReport.transportation, bestEffort),
            "attribute transportation query did not observe the committed change");

         // Interaction transport has the same callback commit boundary, but
         // its query is addressed to the publishing federate.
         owner.queryInteractionTransportationType(ownerFederate, ownerInteraction);
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.interactionReports.size() == 1,
            "initial interaction transportation query did not produce one report callback");
         InteractionTransportReport initialInteractionReport =
            ownerRecorder.interactionReports.get(0);
         JavaTckSupport.check(ownerFederate.equals(initialInteractionReport.federate)
               && ownerInteraction.equals(initialInteractionReport.interaction)
               && sameTransport(owner, initialInteractionReport.transportation, reliable),
            "interaction did not begin with the reliable transportation type");

         // A request for the already-achieved type still confirms that type.
         owner.requestInteractionTransportationTypeChange(ownerInteraction, reliable);
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.interactionConfirmations.size() == 1,
            "unchanged interaction transportation request did not produce one confirmation");
         InteractionTransportConfirmation unchangedConfirmation =
            ownerRecorder.interactionConfirmations.get(0);
         JavaTckSupport.check(ownerInteraction.equals(unchangedConfirmation.interaction)
               && sameTransport(owner, unchangedConfirmation.transportation, reliable),
            "unchanged interaction transportation confirmation did not report reliable");

         owner.requestInteractionTransportationTypeChange(ownerInteraction, bestEffort);
         if (evoked) {
            JavaTckSupport.expectFailure(
               () -> owner.requestInteractionTransportationTypeChange(ownerInteraction, reliable),
               "InteractionClassAlreadyBeingChanged",
               "duplicate interaction transport request was accepted while pending");
         }
         owner.sendInteraction(ownerInteraction, emptyParameters(owner), new byte[] {0x03});
         JavaTckSupport.drain(observer);
         JavaTckSupport.check(observerRecorder.interactionReceives.size() >= 1,
            "first interaction after the transport request was not received");
         InteractionReceive interactionBefore = observerRecorder.interactionReceives.get(
            observerRecorder.interactionReceives.size() - 1);
         JavaTckSupport.check(sameTransport(observer, interactionBefore.transportation,
               observer.getTransportationTypeHandle(evoked ? "HLAreliable" : "HLAbestEffort"))
               && observerInteraction.equals(interactionBefore.interaction)
               && interactionBefore.parameters.isEmpty()
               && Arrays.equals(interactionBefore.tag, new byte[] {0x03})
               && ownerFederate.equals(interactionBefore.producer),
            "first interaction payload or transport differed for the configured callback model");
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.interactionConfirmations.size() == 2,
            "changed interaction transportation request did not produce its confirmation callback");
         JavaTckSupport.check(ownerInteraction.equals(
               ownerRecorder.interactionConfirmations.get(1).interaction)
               && sameTransport(owner, ownerRecorder.interactionConfirmations.get(1).transportation,
                  bestEffort),
            "interaction transport confirmation did not commit best effort");
         owner.sendInteraction(ownerInteraction, emptyParameters(owner), new byte[] {0x04});
         JavaTckSupport.drain(observer);
         InteractionReceive interactionAfter = observerRecorder.interactionReceives.get(
            observerRecorder.interactionReceives.size() - 1);
         JavaTckSupport.check(observerInteraction.equals(interactionAfter.interaction)
               && interactionAfter.parameters.isEmpty()
               && Arrays.equals(interactionAfter.tag, new byte[] {0x04})
               && ownerFederate.equals(interactionAfter.producer)
               && sameTransport(observer, interactionAfter.transportation,
                  observer.getTransportationTypeHandle("HLAbestEffort")),
            "post-confirmation interaction payload or transport was incorrect");
         owner.queryInteractionTransportationType(ownerFederate, ownerInteraction);
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.interactionReports.size() == 2,
            "final interaction transportation query did not produce its report callback");
         JavaTckSupport.check(ownerInteraction.equals(ownerRecorder.interactionReports.get(1).interaction)
               && ownerFederate.equals(ownerRecorder.interactionReports.get(1).federate)
               && sameTransport(owner, ownerRecorder.interactionReports.get(1).transportation,
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

   private static AttributeTransportReport findAttributeReport(
         List<AttributeTransportReport> reports, ObjectInstanceHandle object,
         AttributeHandle attribute) {
      for (int index = reports.size() - 1; index >= 0; --index) {
         AttributeTransportReport report = reports.get(index);
         if (object.equals(report.object) && attribute.equals(report.attribute)) return report;
      }
      throw new AssertionError("no attribute transportation report for requested object/attribute");
   }

   private static Map<AttributeHandle, byte[]> copyValues(AttributeHandleValueMap source) {
      Map<AttributeHandle, byte[]> copy = new HashMap<>();
      for (AttributeHandle handle : source.keySet()) copy.put(handle, source.get(handle).clone());
      return copy;
   }

   private static Map<ParameterHandle, byte[]> copyParameters(ParameterHandleValueMap source) {
      Map<ParameterHandle, byte[]> copy = new HashMap<>();
      for (ParameterHandle handle : source.keySet()) {
         copy.put(handle, source.get(handle).clone());
      }
      return copy;
   }

   private static byte[] copyTag(byte[] tag) {
      return tag == null ? null : tag.clone();
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
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;

      private AttributeReflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer) {
         this.object = object;
         this.values = copyValues(values);
         this.tag = copyTag(tag);
         this.transportation = transportation;
         this.producer = producer;
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
      private final FederateHandle federate;
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private InteractionTransportReport(FederateHandle federate, InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.federate = federate;
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class InteractionReceive {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;

      private InteractionReceive(InteractionClassHandle interaction,
            ParameterHandleValueMap parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer) {
         this.interaction = interaction;
         this.parameters = copyParameters(parameters);
         this.tag = copyTag(tag);
         this.transportation = transportation;
         this.producer = producer;
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
                     (FederateHandle) arguments[0],
                     (InteractionClassHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("reflectAttributeValues".equals(name) && arguments.length == 6) {
                  attributeReflections.add(new AttributeReflection(
                     (ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4]));
               } else if ("receiveInteraction".equals(name) && arguments.length == 6) {
                  interactionReceives.add(new InteractionReceive(
                     (InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4]));
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

