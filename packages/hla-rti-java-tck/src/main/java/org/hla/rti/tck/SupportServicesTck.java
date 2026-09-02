package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.ServiceGroup;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** P1.1 support lookups, normalization, and public handle identity checks. */
final class SupportServicesTck {
   private SupportServicesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, String dimensionName) throws Exception {
      RTIambassador unjoined = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      FederateAmbassador unjoinedCallbacks = noOpFederateAmbassador();
      FederateAmbassador memberCallbacks = noOpFederateAmbassador();
      String federation = "java-tck-support-" + UUID.randomUUID();
      boolean unjoinedConnected = false;
      boolean memberConnected = false;
      boolean joined = false;
      boolean created = false;
      try {
         JavaTckSupport.expectFailure(
            () -> unjoined.getOrderType("Receive"),
            "NotConnected", "order lookup before connect");
         JavaTckSupport.expectFailure(
            () -> unjoined.getTransportationTypeHandle("HLAreliable"),
            "NotConnected", "transportation lookup before connect");

         unjoined.connect(unjoinedCallbacks, CallbackModel.HLA_EVOKED);
         unjoinedConnected = true;
         JavaTckSupport.expectFailure(
            () -> unjoined.getOrderType("Receive"),
            "FederateNotExecutionMember", "order lookup before joining");
         // Handle factories are part of the connected support surface on some
         // providers and the joined surface on others.  Their required
         // lifecycle behavior is asserted below through decode, after a
         // concrete handle has been obtained.
         JavaTckSupport.check(unjoined.getFederateHandleFactory() != null,
            "federate handle factory was unavailable after connection");

         member.connect(memberCallbacks, CallbackModel.HLA_EVOKED);
         memberConnected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle federate = member.joinFederationExecution(
            "java-tck-support", federation);
         joined = true;
         JavaTckSupport.check(federate != null && federate.encodedLength() > 0,
            "support scenario received an invalid federate handle");

         ObjectClassHandle objectClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = member.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interactionClass =
            member.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = member.getParameterHandle(interactionClass, parameterName);
         DimensionHandle dimension = member.getDimensionHandle(dimensionName);
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0,
            "object-class support lookup returned an invalid handle");
         JavaTckSupport.check(attribute != null && attribute.encodedLength() > 0,
            "attribute support lookup returned an invalid handle");
         JavaTckSupport.check(interactionClass != null && interactionClass.encodedLength() > 0,
            "interaction support lookup returned an invalid handle");
         JavaTckSupport.check(parameter != null && parameter.encodedLength() > 0,
            "parameter support lookup returned an invalid handle");
         JavaTckSupport.check(dimension != null && dimension.encodedLength() > 0,
            "dimension support lookup returned an invalid handle");

         OrderType receive = member.getOrderType("Receive");
         OrderType timestamp = member.getOrderType("TimeStamp");
         JavaTckSupport.check(receive == OrderType.RECEIVE,
            "Receive did not resolve to the standard RECEIVE order type");
         JavaTckSupport.check(timestamp == OrderType.TIMESTAMP,
            "TimeStamp did not resolve to the standard TIMESTAMP order type");
         JavaTckSupport.check("Receive".equals(member.getOrderName(receive)),
            "RECEIVE order name did not round-trip");
         JavaTckSupport.check("TimeStamp".equals(member.getOrderName(timestamp)),
            "TIMESTAMP order name did not round-trip");
         JavaTckSupport.expectFailure(
            () -> member.getOrderType("JavaTckMissingOrder"),
            "InvalidOrderName", "unknown order name was accepted");

         TransportationTypeHandle reliable =
            member.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort =
            member.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(reliable != null && reliable.encodedLength() > 0,
            "HLAreliable lookup returned an invalid handle");
         JavaTckSupport.check(bestEffort != null && bestEffort.encodedLength() > 0,
            "HLAbestEffort lookup returned an invalid handle");
         JavaTckSupport.check("HLAreliable".equals(member.getTransportationTypeName(reliable)),
            "HLAreliable name did not round-trip");
         JavaTckSupport.check("HLAbestEffort".equals(
            member.getTransportationTypeName(bestEffort)),
            "HLAbestEffort name did not round-trip");
         JavaTckSupport.expectFailure(
            () -> member.getTransportationTypeHandle("JavaTckMissingTransportation"),
            "InvalidTransportationName", "unknown transportation name was accepted");

         JavaTckSupport.check(dimensionName.equals(member.getDimensionName(dimension)),
            "dimension name did not round-trip");
         JavaTckSupport.check(member.getDimensionUpperBound(dimension) > 0,
            "dimension upper bound was not positive");
         DimensionHandleSet objectDimensions =
            member.getAvailableDimensionsForObjectClass(objectClass);
         DimensionHandleSet interactionDimensions =
            member.getAvailableDimensionsForInteractionClass(interactionClass);
         JavaTckSupport.check(objectDimensions != null && objectDimensions.contains(dimension),
            "object-class available dimensions omitted the configured dimension");
         JavaTckSupport.check(interactionDimensions != null,
            "interaction-class available dimensions returned null");

         AttributeHandleSet published = JavaTckSupport.attributeSet(member, attribute);
         member.publishObjectClassAttributes(objectClass, published);
         ObjectInstanceHandle object = member.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "support scenario received an invalid object-instance handle");

         long serviceGroup = member.normalizeServiceGroup(ServiceGroup.SUPPORT_SERVICES);
         JavaTckSupport.check(serviceGroup ==
            member.normalizeServiceGroup(ServiceGroup.SUPPORT_SERVICES),
            "service-group normalization was not stable");
         long normalizedFederate = member.normalizeFederateHandle(federate);
         long normalizedObjectClass = member.normalizeObjectClassHandle(objectClass);
         long normalizedInteraction = member.normalizeInteractionClassHandle(interactionClass);
         long normalizedObject = member.normalizeObjectInstanceHandle(object);
         JavaTckSupport.check(normalizedFederate == member.normalizeFederateHandle(federate),
            "federate normalization was not stable");
         JavaTckSupport.check(normalizedObjectClass ==
            member.normalizeObjectClassHandle(objectClass),
            "object-class normalization was not stable");
         JavaTckSupport.check(normalizedInteraction ==
            member.normalizeInteractionClassHandle(interactionClass),
            "interaction-class normalization was not stable");
         JavaTckSupport.check(normalizedObject ==
            member.normalizeObjectInstanceHandle(object),
            "object-instance normalization was not stable");
         assertHandleRoundTrip(
            (bytes, offset) -> member.getFederateHandleFactory().decode(bytes, offset),
            federate.encodedLength(), federate::encode, federate, "federate handle");
         assertHandleRoundTrip(
            (bytes, offset) -> member.getObjectClassHandleFactory().decode(bytes, offset),
            objectClass.encodedLength(), objectClass::encode, objectClass,
            "object-class handle");
         assertHandleRoundTrip(
            (bytes, offset) -> member.getInteractionClassHandleFactory().decode(bytes, offset),
            interactionClass.encodedLength(), interactionClass::encode, interactionClass,
            "interaction-class handle");
         assertHandleRoundTrip(
            (bytes, offset) -> member.getObjectInstanceHandleFactory().decode(bytes, offset),
            object.encodedLength(), object::encode, object, "object-instance handle");
         assertHandleRoundTrip(
            (bytes, offset) -> member.getAttributeHandleFactory().decode(bytes, offset),
            attribute.encodedLength(), attribute::encode, attribute, "attribute handle");
         assertHandleRoundTrip(
            (bytes, offset) -> member.getParameterHandleFactory().decode(bytes, offset),
            parameter.encodedLength(), parameter::encode, parameter, "parameter handle");
         assertHandleRoundTrip(
            (bytes, offset) -> member.getDimensionHandleFactory().decode(bytes, offset),
            dimension.encodedLength(), dimension::encode, dimension, "dimension handle");

         byte[] encodedRetraction = new byte[] {
            0x00, 0x00, 0x00, 0x08, 0x01, 0x02, 0x03, 0x04,
            0x05, 0x06, 0x07, 0x08
         };
         MessageRetractionHandle retraction =
            member.getMessageRetractionHandleFactory().decode(encodedRetraction, 0);
         JavaTckSupport.check(retraction != null && retraction.encodedLength() > 0,
            "message-retraction handle decoder returned an invalid handle");

         JavaTckSupport.expectFailure(
            () -> member.getFederateHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty federate handle payload was accepted");
         JavaTckSupport.expectFailure(
            () -> member.getObjectClassHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty object-class handle payload was accepted");
         JavaTckSupport.expectFailure(
            () -> member.getInteractionClassHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty interaction-class handle payload was accepted");
         JavaTckSupport.expectFailure(
            () -> member.getObjectInstanceHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty object-instance handle payload was accepted");
         JavaTckSupport.expectFailure(
            () -> member.getAttributeHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty attribute handle payload was accepted");
         JavaTckSupport.expectFailure(
            () -> member.getParameterHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty parameter handle payload was accepted");
         JavaTckSupport.expectFailure(
            () -> member.getDimensionHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty dimension handle payload was accepted");
         JavaTckSupport.expectFailure(
            () -> member.getMessageRetractionHandleFactory().decode(new byte[0], 0),
            "CouldNotDecode", "empty message-retraction handle payload was accepted");
      } finally {
         if (joined) {
            try {
               member.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               member.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (memberConnected) {
            try {
               member.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (unjoinedConnected) {
            try {
               unjoined.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static void assertHandleRoundTrip(HandleDecoder decoder, int length,
         JavaTckSupport.HandleEncoder encoder, Object expected, String description)
         throws Exception {
      byte[] encoded = JavaTckSupport.encode(length, encoder);
      Object decoded = decoder.decode(encoded, 0);
      JavaTckSupport.check(expected.equals(decoded), description + " did not round-trip");
   }

   private interface HandleDecoder {
      Object decode(byte[] buffer, int offset) throws Exception;
   }

   private static FederateAmbassador noOpFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK no-op callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
