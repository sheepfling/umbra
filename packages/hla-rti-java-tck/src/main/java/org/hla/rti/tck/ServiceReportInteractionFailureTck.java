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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies standard MOM reports for failed ordinary interaction services. */
final class ServiceReportInteractionFailureTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportInteractionFailureTck() { }

   static void run(RtiFactory factory, String fom, String mim, String interactionName,
         String parameterName, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-failure-" + UUID.randomUUID();
      String publisherName = "java-tck-failure-publisher-" + UUID.randomUUID();
      String receiverName = "java-tck-failure-receiver-" + UUID.randomUUID();
      String observerName = "java-tck-failure-observer-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean observerJoined = false;
      boolean observerSubscribed = false;
      boolean receiverSubscribed = false;
      boolean publisherPublished = false;
      InteractionClassHandle reportClass = null;
      InteractionClassHandle sourceClass = null;
      FederateHandle publisherHandle = null;
      try {
         publisher.connect(emptyCallbacks(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         publisherHandle = publisher.joinFederationExecution(publisherName, "java-tck", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(receiverName, "java-tck", federation);
         receiverJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         sourceClass = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle sourceParameter = publisher.getParameterHandle(sourceClass, parameterName);
         final InteractionClassHandle publisherClass = sourceClass;
         InteractionClassHandle receiverClass = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(sourceClass != null && sourceParameter != null
               && sourceClass.equals(receiverClass) && sourceParameter.equals(receiverParameter),
            "adapter-supplied interaction declarations did not resolve consistently");

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS_NAME.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETER_NAMES) {
            ParameterHandle handle = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(handle != null
                  && name.equals(observer.getParameterName(reportClass, handle)),
               "standard service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, handle);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup/name round-trip failed");

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.setServiceReportingSwitch(false);
         receiver.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         observerSubscribed = true;
         publisher.publishInteractionClass(sourceClass);
         publisherPublished = true;
         receiver.subscribeInteractionClass(receiverClass);
         receiverSubscribed = true;

         publisher.setServiceReportingSwitch(true);
         publisher.setSendServiceReportsToFileSwitch(false);
         byte[] payload = new byte[] {0x53, 0x52};
         byte[] tag = new byte[] {0x46, 0x41, 0x49};
         ParameterHandleValueMap validValues =
            publisher.getParameterHandleValueMapFactory().create(1);
         validValues.put(sourceParameter, payload);

         JavaTckSupport.expectFailure(
            () -> publisher.sendInteraction(
               JavaTckSupport.invalidHandle(InteractionClassHandle.class), validValues, tag),
            "InteractionClassNotDefined", "sendInteraction with an invalid class handle");
         verifyFailureReport(observer, observerRecorder, reportClass, reportParameters,
            reliable, encoder, publisherHandle, 0, "InteractionClassNotDefined",
            "invalid interaction-class send", callbackModel);
         assertNoApplicationDelivery(receiverRecorder, callbackModel, receiver,
            "invalid interaction-class send");

         ParameterHandleValueMap invalidValues =
            publisher.getParameterHandleValueMapFactory().create(1);
         invalidValues.put(JavaTckSupport.invalidHandle(ParameterHandle.class), payload);
         JavaTckSupport.expectFailure(
            () -> publisher.sendInteraction(publisherClass, invalidValues, tag),
            "InteractionParameterNotDefined", "sendInteraction with an invalid parameter");
         verifyFailureReport(observer, observerRecorder, reportClass, reportParameters,
            reliable, encoder, publisherHandle, 1, "InteractionParameterNotDefined",
            "invalid-parameter send",
            callbackModel);
         assertNoApplicationDelivery(receiverRecorder, callbackModel, receiver,
            "invalid-parameter send");

         publisher.setServiceReportingSwitch(false);
         publisher.unpublishInteractionClass(sourceClass);
         publisherPublished = false;
         publisher.setServiceReportingSwitch(true);
         JavaTckSupport.expectFailure(
            () -> publisher.sendInteraction(publisherClass, validValues, tag),
            "InteractionClassNotPublished", "sendInteraction with an unpublished class");
         verifyFailureReport(observer, observerRecorder, reportClass, reportParameters,
            reliable, encoder, publisherHandle, 2, "InteractionClassNotPublished",
            "unpublished-class send",
            callbackModel);
         assertNoApplicationDelivery(receiverRecorder, callbackModel, receiver,
            "unpublished-class send");
      } finally {
         if (publisherJoined) {
            try { publisher.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
         }
         if (observerJoined && observerSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (receiverJoined && receiverSubscribed) {
            try { receiver.unsubscribeInteractionClass(
                  receiver.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && publisherPublished) {
            try { publisher.unpublishInteractionClass(sourceClass); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void verifyFailureReport(RTIambassador observer, Recorder observerRecorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         TransportationTypeHandle reliable, EncoderFactory encoder, FederateHandle publisher,
         int serialNumber,
         String exception, String description,
         CallbackModel callbackModel) throws Exception {
      await(observer, observerRecorder, callbackModel, description);
      Interaction report = observerRecorder.events.remove(0);
      JavaTckSupport.check(reportClass.equals(report.interaction)
            && report.parameters.size() == REPORT_PARAMETER_NAMES.length
            && report.producer == null
            && report.tag.length == 0
            && reliable.equals(report.transportation)
            && report.regionsEmpty,
         description + " report had incorrect interaction or callback metadata");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         report.parameters.get(reportParameters.get("HLAfederate")), 0);
      JavaTckSupport.check(publisher.equals(reportedFederate),
         description + " report identified the wrong invoking federate");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
            description + " report omitted standard parameter " + name);
      }

      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(reportParameters.get("HLAservice")));
      JavaTckSupport.check("SendInteraction".equals(service.getValue()),
         description + " report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(reportParameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 2,
         description + " report returned the wrong interaction service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(!success.getValue(), description + " report indicated success");

      HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), argumentPrototype);
      suppliedArguments.decode(report.parameters.get(reportParameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(suppliedArguments.size() == 4,
         description + " report omitted standard supplied arguments");
      verifySuppliedArgument(suppliedArguments.get(0), 27, "Interaction class designator");
      verifySuppliedArgument(suppliedArguments.get(1), 40,
         "Constrained set of interaction parameter designator and value pairs");
      verifySuppliedArgument(suppliedArguments.get(2), 60, "User-supplied tag");
      verifySuppliedArgument(suppliedArguments.get(3), 34, "Optional timestamp");
      HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
      returnedArgument.add(encoder.createHLAinteger32BE());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.decode(report.parameters.get(reportParameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(returnedArgument.size() == 3
            && ((HLAinteger32BE) returnedArgument.get(0)).getValue() == 34
            && ((HLAunicodeString) returnedArgument.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returnedArgument.get(2)).getValue()),
         description + " report returned the wrong Null return value");
      HLAunicodeString exceptionText = encoder.createHLAunicodeString();
      exceptionText.decode(report.parameters.get(reportParameters.get("HLAexception")));
      JavaTckSupport.check(exceptionText.getValue().contains(exception),
         description + " report returned the wrong exception name");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == serialNumber,
         description + " report returned the wrong serial number");
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static void verifySuppliedArgument(HLAfixedRecord argument, int type,
         String name) throws Exception {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue()),
         "service report supplied-argument record had the wrong shape for " + name);
   }

   private static void assertNoApplicationDelivery(Recorder receiverRecorder,
         CallbackModel callbackModel, RTIambassador receiver, String description)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(receiver);
      } else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
      JavaTckSupport.check(receiverRecorder.events.isEmpty(),
         description + " delivered an application interaction callback");
   }

   private static void await(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         else Thread.yield();
      }
      JavaTckSupport.check(!recorder.events.isEmpty(),
         "timed out waiting for HLAreportServiceInvocation after " + description);
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK service-report failure callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     event.parameters.put(handle, source.get(handle).clone());
                  }
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  events.add(event);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
