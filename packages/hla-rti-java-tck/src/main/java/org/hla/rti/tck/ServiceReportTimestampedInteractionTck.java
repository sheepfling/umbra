package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies successful timestamped interaction reporting and time-constrained delivery. */
final class ServiceReportTimestampedInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportTimestampedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-timestamped-interaction-report-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, observerConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      boolean observerJoined = false, published = false, receiverSubscribed = false;
      boolean reportSubscribed = false, publisherRegulating = false, receiverConstrained = false;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      InteractionClassHandle reportClass = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-timestamped-publisher-" + UUID.randomUUID(), "java-tck", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-timestamped-receiver-" + UUID.randomUUID(), "java-tck", federation);
         receiverJoined = true;
         observer.joinFederationExecution(
            "java-tck-timestamped-observer-" + UUID.randomUUID(), "java-tck", federation);
         observerJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction != null && receiverInteraction != null
               && publisherParameter != null && receiverParameter != null
               && publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "adapter-supplied interaction and parameter handles were inconsistent");

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
         reportSubscribed = true;
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         receiverSubscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiverTimeFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         for (int i = 0; i < 4; ++i) lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(receiver, callbackModel, () -> receiverRecorder.timeConstrainedEnabled,
            "timestamped interaction time-constrained callback");
         publisher.enableTimeRegulation(lookahead);
         publisherRegulating = true;
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabled,
            "timestamped interaction time-regulation callback");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime timestamp = (LogicalTime) publisherInitial.add(lookahead).add(epsilon);

         byte[] value = new byte[] {0x54, 0x53, 0x4f};
         byte[] tag = new byte[] {0x4d, 0x4f, 0x4d};
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, value);
         publisher.setServiceReportingSwitch(true);
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherInteraction, values, tag, timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped interaction send returned an invalid retraction handle");

         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "timestamped interaction service report");
         JavaTckSupport.check(observerRecorder.interactions.size() == 1
               && receiverRecorder.interactions.isEmpty()
               && receiverRecorder.grants.isEmpty(),
            "timestamped interaction produced an early delivery or unexpected report count");
         Interaction report = observerRecorder.interactions.get(0);
         JavaTckSupport.check(reportClass.equals(report.interaction)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length
               && report.tag.length == 0 && report.producer == null && report.regionsEmpty
               && reliable.equals(report.transportation),
            "timestamped service report returned invalid callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
               "timestamped service report omitted standard parameter " + name);
         }
         verifyReport(observer, report, reportParameters, encoder, publisherHandle,
            publisherInteraction, publisherParameter, value, tag, timestamp, retraction);

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         LogicalTime receiverTarget = (LogicalTime) receiverInitial.add(lookahead).add(epsilon);
         LogicalTime publisherTarget = (LogicalTime) publisherInitial.add(epsilon);
         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitPair(publisher, receiver, callbackModel,
            () -> !receiverRecorder.interactions.isEmpty()
               && !receiverRecorder.grants.isEmpty() && !publisherRecorder.grants.isEmpty(),
            "timestamped interaction delivery and time grants");

         Interaction delivered = receiverRecorder.interactions.get(0);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1
               && receiverInteraction.equals(delivered.interaction)
               && delivered.parameters.size() == 1
               && Arrays.equals(value, delivered.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivered.tag)
               && publisherHandle.equals(delivered.producer)
               && timestamp.equals(delivered.time)
               && OrderType.TIMESTAMP.equals(delivered.sentOrder)
               && OrderType.TIMESTAMP.equals(delivered.receivedOrder)
               && reliable.equals(delivered.transportation)
               && delivered.retraction != null && retraction.handle.equals(delivered.retraction)
               && receiverTarget.equals(receiverRecorder.grants.get(0))
               && publisherTarget.equals(publisherRecorder.grants.get(0)),
            "timestamped interaction delivery or grant returned incorrect standard metadata");
         JavaTckSupport.check(receiverRecorder.callbackOrder.size() >= 2
               && "receiveInteraction".equals(receiverRecorder.callbackOrder.get(0))
               && "timeAdvanceGrant".equals(receiverRecorder.callbackOrder.get(1)),
            "timestamped interaction was not delivered before its matching grant");
         receiver.disableTimeConstrained();
         receiverConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
      } finally {
         if (publisherJoined) {
            try { publisher.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { publisher.setSendServiceReportsToFileSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed && reportClass != null) try {
            observer.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (receiverJoined && receiverConstrained) try {
            receiver.disableTimeConstrained();
         } catch (Exception ignored) { }
         if (publisherJoined && publisherRegulating) try {
            publisher.disableTimeRegulation();
         } catch (Exception ignored) { }
         if (receiverJoined && receiverSubscribed && receiverInteraction != null) try {
            receiver.unsubscribeInteractionClass(receiverInteraction);
         } catch (Exception ignored) { }
         if (publisherJoined && published && publisherInteraction != null) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         if (observerJoined) try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); } catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void verifyReport(RTIambassador observer, Interaction report,
         Map<String, ParameterHandle> parameters, EncoderFactory encoder,
         FederateHandle publisher, InteractionClassHandle interaction,
         ParameterHandle interactionParameter, byte[] value, byte[] tag, LogicalTime timestamp,
         MessageRetractionReturn retraction) throws Exception {
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         report.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(publisher.equals(reportedFederate),
         "timestamped service report identified the wrong federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check("SendInteraction".equals(service.getValue())
            && serviceType.getValue() == 2 && success.getValue(),
         "timestamped interaction report returned the wrong service result");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 4,
         "timestamped interaction report returned the wrong supplied-argument count");
      verifyArgument(supplied.get(0), 27, "Interaction class designator",
         quoted(interaction.toString()));
      verifyArgument(supplied.get(1), 40,
         "Constrained set of interaction parameter designator and value pairs",
         "{\"" + interactionParameter.toString() + "\":\"" + base64(value) + "\"}");
      verifyArgument(supplied.get(2), 60, "User-supplied tag", quoted(base64(tag)));
      verifyArgument(supplied.get(3), 31, "Optional timestamp", quoted(timestamp.toString()));

      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      String retractionText = retraction.handle.toString();
      int open = retractionText.indexOf('(');
      int close = retractionText.indexOf(')');
      String reportedRetraction = ((HLAunicodeString) returned.get(2)).getValue();
      JavaTckSupport.check(returned.size() == 3
            && ((HLAinteger32BE) returned.get(0)).getValue() == 33
            && "Message retraction designator".equals(
               ((HLAunicodeString) returned.get(1)).getValue())
            && open >= 0 && close > open + 1
            && reportedRetraction.equals("\"MessageRetractionHandle<"
               + retractionText.substring(open + 1, close) + ">\""),
         "timestamped interaction report returned a mismatched retraction designator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().isEmpty(),
         "successful timestamped interaction report carried an exception");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == 0,
         "first timestamped service report did not use serial number zero");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static void verifyArgument(HLAfixedRecord argument, int type, String name,
         String value) throws Exception {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue())
            && value.equals(((HLAunicodeString) argument.get(2)).getValue()),
         "timestamped interaction report supplied the wrong argument " + name);
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static String base64(byte[] value) {
      return java.util.Base64.getEncoder().encodeToString(value);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isMet() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   private static void awaitPair(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isMet() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         } else {
            Thread.yield();
         }
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet(); }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] values = args == null ? new Object[0] : args;
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeAdvanceGrant".equals(name) && values.length == 1) {
                  grants.add((LogicalTime<?, ?>) values[0]);
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name) && values.length == 10) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) values[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) values[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     event.parameters.put(handle, source.get(handle).clone());
                  }
                  event.tag = ((byte[]) values[2]).clone();
                  event.transportation = (TransportationTypeHandle) values[3];
                  event.producer = (FederateHandle) values[4];
                  RegionHandleSet regions = (RegionHandleSet) values[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  event.time = (LogicalTime<?, ?>) values[6];
                  event.sentOrder = (OrderType) values[7];
                  event.receivedOrder = (OrderType) values[8];
                  event.retraction = (MessageRetractionHandle) values[9];
                  interactions.add(event);
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name) && values.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) values[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) values[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     event.parameters.put(handle, source.get(handle).clone());
                  }
                  event.tag = ((byte[]) values[2]).clone();
                  event.transportation = (TransportationTypeHandle) values[3];
                  event.producer = (FederateHandle) values[4];
                  RegionHandleSet regions = (RegionHandleSet) values[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  interactions.add(event);
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped interaction report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == values[0];
               }
               return null;
            });
      }
   }
}
