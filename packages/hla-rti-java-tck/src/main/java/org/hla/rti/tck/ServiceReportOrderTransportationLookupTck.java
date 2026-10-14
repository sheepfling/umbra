package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.OrderType;
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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies standard order and transportation lookups through service-report MOM. */
final class ServiceReportOrderTransportationLookupTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportOrderTransportationLookupTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-order-transport-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            "java-tck-order-transport-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution("java-tck-order-transport-observer", "java-tck",
            federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = subject.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(subject.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         JavaTckSupport.check(OrderType.RECEIVE == subject.getOrderType("Receive"),
            "standard Receive order lookup returned the wrong order type");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         subject.setServiceReportingSwitch(true);
         OrderType timestamp = subject.getOrderType("TimeStamp");
         String receiveName = subject.getOrderName(OrderType.RECEIVE);
         TransportationTypeHandle reliableByName =
            subject.getTransportationTypeHandle("HLAreliable");
         String reliableName = subject.getTransportationTypeName(reliable);
         JavaTckSupport.check(timestamp == OrderType.TIMESTAMP
               && "Receive".equals(receiveName)
               && reliable.equals(reliableByName)
               && "HLAreliable".equals(reliableName),
            "standard order or transportation lookup returned an unexpected value");
         await(observer, recorder, callbackModel, 4);
         JavaTckSupport.check(recorder.events.size() == 4,
            "order/transportation lookups produced an unexpected number of reports");

         verifyReport(observer, subjectHandle, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable,
            "GetOrderType", 53, "Order name", "\"TimeStamp\"", 38, "Order type",
            "\"TIMESTAMP\"", 0, true, "");
         verifyReport(observer, subjectHandle, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable,
            "GetOrderName", 38, "Order type", "\"RECEIVE\"", 53, "Order name",
            "\"Receive\"", 1, true, "");
         verifyReport(observer, subjectHandle, recorder.events.get(2), encoder, reportClass,
            reportParameters, reliable,
            "GetTransportationTypeHandle", 53, "Transportation type name", "\"HLAreliable\"",
            59, "Transportation type handle", quoted(reliableByName.toString()), 2, true, "");
         verifyReport(observer, subjectHandle, recorder.events.get(3), encoder, reportClass,
            reportParameters, reliable,
            "GetTransportationTypeName", 59, "Transportation type handle",
            quoted(reliable.toString()), 53, "Transportation type name", "\"HLAreliable\"",
            3, true, "");

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runFailures(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-order-transport-failures-" + suffix;
      String subjectName = "java-tck-order-transport-failure-subject-" + suffix;
      String observerName = "java-tck-order-transport-failure-observer-" + suffix;
      String unsupportedOrder = "TckUnsupportedOrder";
      String unsupportedTransportation = "TckUnsupportedTransportation";
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = subject.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(subject.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         TransportationTypeHandle invalidTransportation =
            JavaTckSupport.invalidHandle(TransportationTypeHandle.class);
         subject.setServiceReportingSwitch(true);
         JavaTckSupport.expectFailure(() -> subject.getOrderType(unsupportedOrder),
            "InvalidOrderName", "reported unsupported order-name lookup");
         JavaTckSupport.expectFailure(() -> subject.getOrderName(null),
            "InvalidOrderType", "reported null Java order-type lookup");
         JavaTckSupport.expectFailure(() -> subject.getTransportationTypeHandle(
            unsupportedTransportation), "InvalidTransportationName",
            "reported unsupported transportation-name lookup");
         JavaTckSupport.expectFailure(() -> subject.getTransportationTypeName(
            invalidTransportation), "InvalidTransportationTypeHandle",
            "reported invalid transportation-handle lookup");
         OrderType receiveType = subject.getOrderType("Receive");
         JavaTckSupport.check(receiveType == OrderType.RECEIVE,
            "Receive order lookup did not recover after reported failures");
         await(observer, recorder, callbackModel, 5);
         JavaTckSupport.check(recorder.events.size() == 5,
            "order/transportation lookups produced an unexpected report count");

         verifyReport(observer, subjectHandle, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetOrderType", 53, "Order name",
            quoted(unsupportedOrder), 34, "", "null", 0, false, "InvalidOrderName");
         verifyReport(observer, subjectHandle, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable, "GetOrderName", 38, "Order type", quoted("null"),
            34, "", "null", 1, false, "InvalidOrderType");
         verifyReport(observer, subjectHandle, recorder.events.get(2), encoder, reportClass,
            reportParameters, reliable, "GetTransportationTypeHandle", 53,
            "Transportation type name", quoted(unsupportedTransportation), 34, "", "null",
            2, false, "InvalidTransportationName");
         verifyReport(observer, subjectHandle, recorder.events.get(3), encoder, reportClass,
            reportParameters, reliable, "GetTransportationTypeName", 59,
            "Transportation type handle", quoted(invalidTransportation.toString()),
            34, "", "null", 3, false, "InvalidTransportationTypeHandle");
         verifyReport(observer, subjectHandle, recorder.events.get(4), encoder, reportClass,
            reportParameters, reliable, "GetOrderType", 53, "Order name", quoted("Receive"),
            38, "Order type", quoted("RECEIVE"), 4, true, "");

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         TransportationTypeHandle reliable, String service, int suppliedType,
         String suppliedName, String suppliedValue, int returnedType, String returnedName,
         String returnedValue, int serial, boolean expectedSuccess,
         String expectedException) throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length
            && event.tag.length == 0 && event.producer == null && event.regionsEmpty
            && reliable.equals(event.transportation),
         service + " report had unexpected interaction, metadata, or parameter count");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            service + " report omitted standard parameter " + name);
      }
      HLAunicodeString decodedService = encoder.createHLAunicodeString();
      decodedService.decode(event.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serialNumber = encoder.createHLAinteger32BE();
      serialNumber.decode(event.parameters.get(parameters.get("HLAserialNumber")));
      boolean exceptionMatches = expectedSuccess ? exception.getValue().isEmpty()
            : exception.getValue().contains(expectedException);
      JavaTckSupport.check(service.equals(decodedService.getValue())
            && serviceType.getValue() == 6 && success.getValue() == expectedSuccess
            && exceptionMatches && serialNumber.getValue() == serial,
         service + " report returned incorrect service status, exception, or serial number");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(expectedFederate.equals(reportedFederate),
         service + " report identified the wrong joined federate");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      suppliedArguments.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(suppliedArguments.size() == 1,
         service + " report returned the wrong supplied argument count");
      verifyArgument(suppliedArguments.get(0), suppliedType, suppliedName, suppliedValue,
         service + " supplied argument");

      HLAfixedRecord returnedArgument = argumentPrototype(encoder);
      returnedArgument.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      verifyArgument(returnedArgument, returnedType, returnedName, returnedValue,
         service + " returned argument");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static void verifyArgument(HLAfixedRecord argument, int type, String name,
         String value, String description) throws Exception {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue())
            && value.equals(((HLAunicodeString) argument.get(2)).getValue()),
         description + " had the wrong type, name, or value");
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static void await(RTIambassador observer, Recorder recorder, CallbackModel model,
         int count) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.events.size() < count && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.events.size() >= count,
         "timed out waiting for order/transportation service-report callbacks");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK service-report lookup callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == arguments[0];
      return null;
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction,
            Map<ParameterHandle, byte[]> parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
      }
   }

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  Map<ParameterHandle, byte[]> values = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     values.put(handle, source.get(handle).clone());
                  }
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  events.add(new Interaction((InteractionClassHandle) arguments[0], values,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], regions == null || regions.isEmpty()));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
