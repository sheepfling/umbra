package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Checks Java handle encodings against the standard HLAbyte variable-array type. */
final class HandleEncodingMimDataTypesTck {
   private HandleEncodingMimDataTypesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      RegulationRecorder recorder = new RegulationRecorder();
      String federation = "java-tck-handle-encoding-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean timeRegulating = false;
      RegionHandle region = null;
      try {
         owner.connect(recorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(new NullFederateAmbassador(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;

         FederateHandle federate = owner.joinFederationExecution(
            "java-tck-handle-encoding-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-handle-encoding-member", federation);
         memberJoined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interaction =
            owner.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = owner.getParameterHandle(interaction, parameterName);
         DimensionHandle dimension = owner.getDimensionHandle(dimensionName);
         TransportationTypeHandle transportation =
            owner.getTransportationTypeHandle("HLAreliable");

         AttributeHandleSet publishedAttributes =
            owner.getAttributeHandleSetFactory().create();
         publishedAttributes.add(attribute);
         owner.publishObjectClassAttributes(objectClass, publishedAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);

         DimensionHandleSet regionDimensions =
            owner.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = owner.createRegion(regionDimensions);

         owner.publishInteractionClass(interaction);
         owner.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);
         member.subscribeInteractionClass(interaction);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTime initial = owner.queryLogicalTime();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         owner.enableTimeRegulation(epsilon);
         timeRegulating = true;
         awaitRegulation(owner, callbackModel, recorder);
         JavaTckSupport.check(initial.equals(recorder.assignedTime),
            "Time Regulation Enabled changed the handle-encoding test's initial time");
         LogicalTime messageTime = (LogicalTime) initial.add(epsilon);

         ParameterHandleValueMap values =
            owner.getParameterHandleValueMapFactory().create(1);
         values.put(parameter, new byte[] {0x48, 0x4c, 0x41});
         MessageRetractionReturn retraction = owner.sendInteraction(
            interaction, values, new byte[] {0x54, 0x43, 0x4b}, messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Send Interaction returned no valid message-retraction handle");

         EncoderFactory encoder = factory.getEncoderFactory();
         verifyHandle(encoder, "HLAfederateHandle", federate,
            federate.encodedLength(), federate::encode,
            owner.getFederateHandleFactory()::decode);
         verifyHandle(encoder, "HLAobjectClassHandle", objectClass,
            objectClass.encodedLength(), objectClass::encode,
            owner.getObjectClassHandleFactory()::decode);
         verifyHandle(encoder, "HLAinteractionClassHandle", interaction,
            interaction.encodedLength(), interaction::encode,
            owner.getInteractionClassHandleFactory()::decode);
         verifyHandle(encoder, "HLAobjectInstanceHandle", object,
            object.encodedLength(), object::encode,
            owner.getObjectInstanceHandleFactory()::decode);
         verifyHandle(encoder, "HLAattributeHandle", attribute,
            attribute.encodedLength(), attribute::encode,
            owner.getAttributeHandleFactory()::decode);
         verifyHandle(encoder, "HLAparameterHandle", parameter,
            parameter.encodedLength(), parameter::encode,
            owner.getParameterHandleFactory()::decode);
         verifyHandle(encoder, "HLAdimensionHandle", dimension,
            dimension.encodedLength(), dimension::encode,
            owner.getDimensionHandleFactory()::decode);
         verifyHandle(encoder, "HLAmessageRetractionHandle", retraction.handle,
            retraction.handle.encodedLength(), retraction.handle::encode,
            owner.getMessageRetractionHandleFactory()::decode);
         verifyHandle(encoder, "HLAregionHandle", region,
            region.encodedLength(), region::encode,
            owner.getRegionHandleFactory()::decode);
         verifyHandle(encoder, "HLAtransportationTypeHandle", transportation,
            transportation.encodedLength(), transportation::encode,
            owner.getTransportationTypeHandleFactory()::decode);
      } finally {
         if (timeRegulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (ownerJoined && region != null) {
            try { owner.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static <T> void verifyHandle(EncoderFactory encoder, String description, T handle,
         int encodedLength, JavaTckSupport.HandleEncoder handleEncoder,
         HandleDecoder<T> decoder) throws Exception {
      JavaTckSupport.check(handle != null && encodedLength > 0,
         description + " was absent or had an empty encoding");
      byte[] encoded = new byte[encodedLength];
      handleEncoder.encode(encoded, 0);

      HLAvariableArray<HLAbyte> mimValue = encoder.createHLAvariableArray(
         index -> encoder.createHLAbyte());
      mimValue.decode(encoded);
      JavaTckSupport.check(Arrays.equals(mimValue.toByteArray(), encoded),
         description + " encoding did not round-trip as its HLAstandardMIM byte array");

      T decoded = decoder.decode(encoded, 0);
      JavaTckSupport.check(handle.equals(decoded),
         description + " did not round-trip through its typed Java handle factory");
   }

   private static void awaitRegulation(RTIambassador ambassador, CallbackModel model,
         RegulationRecorder recorder) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (recorder.assignedTime == null && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.assignedTime != null,
         "timed out waiting for Time Regulation Enabled before creating the retraction handle");
   }

   @FunctionalInterface
   private interface HandleDecoder<T> {
      T decode(byte[] encoded, int offset) throws Exception;
   }

   private static final class RegulationRecorder {
      private volatile LogicalTime<?, ?> assignedTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("timeRegulationEnabled".equals(method.getName())
                     && arguments != null && arguments.length == 1) {
                  assignedTime = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) {
                     return "Java TCK handle-encoding callback recorder";
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
