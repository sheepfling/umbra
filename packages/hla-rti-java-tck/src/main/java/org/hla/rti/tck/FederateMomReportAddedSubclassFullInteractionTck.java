package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies delivery of inherited and extension parameters on a FOM-added MOM subclass. */
final class FederateMomReportAddedSubclassFullInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REPORT_SUBCLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation"
         + ".TckReportServiceInvocationExtension";
   private static final List<String> PARAMETER_NAMES = List.of(
      "HLAfederate",
      "HLAservice",
      "HLAserviceType",
      "HLAsuccessIndicator",
      "HLAsuppliedArguments",
      "HLAreturnedArgument",
      "HLAexception",
      "HLAserialNumber",
      "TckReportServiceInvocationExtensionPayload");

   private FederateMomReportAddedSubclassFullInteractionTck() { }

   static void run(RtiFactory factory, String fom, String extensionFom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-report-subclass-delivery-" + suffix;
      String publisherName = "java-tck-mom-report-subclass-publisher-" + suffix;
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean subscribed = false, published = false;
      InteractionClassHandle publisherClass = null, subscriberClass = null;
      List<ParameterHandle> publisherParameters = new ArrayList<>();
      List<ParameterHandle> subscriberParameters = new ArrayList<>();
      try {
         publisher.connect(callbacks(), callbackModel);
         publisherConnected = true;
         subscriber.connect(recorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecutionWithMIM(federation,
            new String[] {fom, extensionFom}, mim, timeImplementation);
         created = true;
         publisher.joinFederationExecution(publisherName, federateType, federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-mom-report-subclass-subscriber-" + suffix,
            federateType, federation);
         subscriberJoined = true;

         publisherClass = publisher.getInteractionClassHandle(REPORT_SUBCLASS);
         subscriberClass = subscriber.getInteractionClassHandle(REPORT_SUBCLASS);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherClass.equals(subscriberClass)
               && REPORT_SUBCLASS.equals(publisher.getInteractionClassName(publisherClass))
               && REPORT_SUBCLASS.equals(subscriber.getInteractionClassName(subscriberClass)),
            "the publishable FOM-added HLAreportServiceInvocation subclass was unavailable or inconsistent");

         for (String name : PARAMETER_NAMES) {
            ParameterHandle publisherParameter =
               publisher.getParameterHandle(publisherClass, name);
            ParameterHandle subscriberParameter =
               subscriber.getParameterHandle(subscriberClass, name);
            JavaTckSupport.check(publisherParameter != null && subscriberParameter != null
                  && publisherParameter.equals(subscriberParameter),
               "a predefined or extension report parameter handle was unavailable or inconsistent: "
                  + name);
            publisherParameters.add(publisherParameter);
            subscriberParameters.add(subscriberParameter);
         }

         subscriber.subscribeInteractionClass(subscriberClass);
         subscribed = true;
         publisher.publishInteractionClass(publisherClass);
         published = true;
         FederateHandle publisherHandle = publisher.getFederateHandle(publisherName);
         JavaTckSupport.check(publisherHandle != null,
            "the interaction publisher handle was unavailable");
         TransportationTypeHandle reliable = subscriber.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null, "standard reliable transportation was unavailable");

         HLAunicodeString service = encoder.createHLAunicodeString("TckAddedSubclassDeliveryProbe");
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE((short) 6);
         HLAboolean success = encoder.createHLAboolean(true);
         HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
            index -> fixedRecordPrototype(encoder));
         HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
         returnedArgument.add(encoder.createHLAinteger32BE(34));
         returnedArgument.add(encoder.createHLAunicodeString());
         returnedArgument.add(encoder.createHLAunicodeString());
         HLAunicodeString exception = encoder.createHLAunicodeString();
         HLAinteger32BE serialNumber = encoder.createHLAinteger32BE(0);
         ParameterHandleValueMap sentParameters =
            publisher.getParameterHandleValueMapFactory().create(PARAMETER_NAMES.size());
         sentParameters.put(publisherParameters.get(0), encode(publisherHandle));
         sentParameters.put(publisherParameters.get(1), service.toByteArray());
         sentParameters.put(publisherParameters.get(2), serviceType.toByteArray());
         sentParameters.put(publisherParameters.get(3), success.toByteArray());
         sentParameters.put(publisherParameters.get(4), suppliedArguments.toByteArray());
         sentParameters.put(publisherParameters.get(5), returnedArgument.toByteArray());
         sentParameters.put(publisherParameters.get(6), exception.toByteArray());
         sentParameters.put(publisherParameters.get(7), serialNumber.toByteArray());
         sentParameters.put(publisherParameters.get(8), new byte[] {(byte) 0xD4});
         byte[] tag = new byte[] {0x54, 0x43, 0x4B};
         publisher.sendInteraction(publisherClass, sentParameters, tag);

         await(subscriber, callbackModel, () -> !recorder.interactions.isEmpty(),
            "the full FOM-added report-subclass interaction");
         JavaTckSupport.check(recorder.interactions.size() == 1
               && subscriberClass.equals(recorder.interactions.get(0).interaction),
            "the subclass subscriber did not receive exactly one interaction at the FOM-added class");
         Interaction interaction = recorder.interactions.get(0);
         JavaTckSupport.check(publisherHandle.equals(interaction.producer)
               && reliable.equals(interaction.transportation)
               && Arrays.equals(tag, interaction.tag)
               && interaction.regionsEmpty
               && interaction.parameters.size() == sentParameters.size(),
            "the received extension interaction lost its publisher, reliable transport, tag, region state, or parameter-map cardinality");

         for (int index = 0; index < PARAMETER_NAMES.size(); ++index) {
            byte[] sent = sentParameters.get(publisherParameters.get(index));
            byte[] delivered = interaction.parameters.get(subscriberParameters.get(index));
            JavaTckSupport.check(sent != null && delivered != null && Arrays.equals(sent, delivered),
               "the subclass subscriber did not receive parameter byte-for-byte: "
                  + PARAMETER_NAMES.get(index));
         }
         FederateHandle decodedPublisher = subscriber.getFederateHandleFactory().decode(
            interaction.parameters.get(subscriberParameters.get(0)), 0);
         JavaTckSupport.check(publisherHandle.equals(decodedPublisher),
            "the full subclass interaction did not preserve its inherited HLAfederate parameter");

         HLAunicodeString receivedService = encoder.createHLAunicodeString();
         receivedService.decode(interaction.parameters.get(subscriberParameters.get(1)));
         HLAinteger16BE receivedServiceType = encoder.createHLAinteger16BE();
         receivedServiceType.decode(interaction.parameters.get(subscriberParameters.get(2)));
         HLAboolean receivedSuccess = encoder.createHLAboolean();
         receivedSuccess.decode(interaction.parameters.get(subscriberParameters.get(3)));
         HLAvariableArray<HLAfixedRecord> receivedSuppliedArguments =
            encoder.createHLAvariableArray(index -> fixedRecordPrototype(encoder));
         receivedSuppliedArguments.decode(
            interaction.parameters.get(subscriberParameters.get(4)));
         HLAfixedRecord receivedReturnedArgument = fixedRecordPrototype(encoder);
         receivedReturnedArgument.decode(interaction.parameters.get(subscriberParameters.get(5)));
         HLAunicodeString receivedException = encoder.createHLAunicodeString();
         receivedException.decode(interaction.parameters.get(subscriberParameters.get(6)));
         HLAinteger32BE receivedSerialNumber = encoder.createHLAinteger32BE();
         receivedSerialNumber.decode(interaction.parameters.get(subscriberParameters.get(7)));
         JavaTckSupport.check("TckAddedSubclassDeliveryProbe".equals(receivedService.getValue())
               && receivedServiceType.getValue() == 6 && receivedSuccess.getValue(),
            "the full extension interaction did not preserve the decoded service, type, or success value");
         JavaTckSupport.check(receivedSuppliedArguments.size() == 0,
            "the full extension interaction did not preserve its empty supplied-arguments array");
         JavaTckSupport.check(receivedReturnedArgument.size() == 3
               && ((HLAinteger32BE) receivedReturnedArgument.get(0)).getValue() == 34
               && ((HLAunicodeString) receivedReturnedArgument.get(1)).getValue().isEmpty()
               && ((HLAunicodeString) receivedReturnedArgument.get(2)).getValue().isEmpty(),
            "the full extension interaction did not preserve its decoded returned argument");
         JavaTckSupport.check(receivedException.getValue().isEmpty()
               && receivedSerialNumber.getValue() == 0,
            "the full extension interaction did not preserve its empty exception and zero serial number");
         JavaTckSupport.check(Arrays.equals(
               interaction.parameters.get(subscriberParameters.get(8)),
               new byte[] {(byte) 0xD4}),
            "the full extension interaction did not preserve the added subclass payload value");
      } finally {
         if (subscriberJoined && subscribed && subscriberClass != null) {
            try { subscriber.unsubscribeInteractionClass(subscriberClass); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null) {
            try { publisher.unpublishInteractionClass(publisherClass); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK MOM report-subclass publisher callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  for (ParameterHandle handle : source.keySet()) {
                     interaction.parameters.put(handle, source.get(handle).clone());
                  }
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  interaction.regionsEmpty = regions == null || regions.isEmpty();
                  interactions.add(interaction);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
