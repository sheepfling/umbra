package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies the standard MOM report and delivery of a timestamped directed interaction. */
final class ServiceReportTimestampedDirectedInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportTimestampedDirectedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-timestamped-directed-report-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, observerConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      boolean observerJoined = false, receiverAttributesSubscribed = false;
      boolean receiverDirectedSubscribed = false, reportSubscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandleSet receiverAttributes = null;
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
            "java-tck-timestamped-directed-publisher-" + UUID.randomUUID(),
            "java-tck", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-timestamped-directed-receiver-" + UUID.randomUUID(),
            "java-tck", federation);
         receiverJoined = true;
         observer.joinFederationExecution(
            "java-tck-timestamped-directed-observer-" + UUID.randomUUID(),
            "java-tck", federation);
         observerJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null
               && publisherInteraction != null && receiverInteraction != null
               && publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute)
               && publisherInteraction.equals(receiverInteraction),
            "adapter-supplied object, attribute, or interaction handles were inconsistent");

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
         TransportationTypeHandle observerReliable =
            observer.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle receiverReliable =
            receiver.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(observerReliable != null && receiverReliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(observerReliable))
               && "HLAreliable".equals(receiver.getTransportationTypeName(receiverReliable)),
            "standard reliable transportation lookup/name round-trip failed");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverAttributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         receiverDirectedSubscribed = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(target != null,
            "timestamped directed-interaction service-report registration returned an invalid handle");
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveredObject != null,
            "timestamped directed-interaction target discovery");
         JavaTckSupport.check(target.equals(receiverRecorder.discoveredObject),
            "timestamped directed-interaction discovery identified the wrong object");

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.setServiceReportingSwitch(false);
         receiver.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observerRecorder.interactions.clear();
         receiverRecorder.directedInteractions.clear();
         receiverRecorder.grants.clear();
         receiverRecorder.callbackOrder.clear();
         publisherRecorder.grants.clear();

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         receiver.enableTimeConstrained();
         await(receiver, callbackModel, () -> receiverRecorder.timeConstrainedEnabled,
            "timestamped directed-interaction time-constrained callback");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         for (int i = 0; i < 4; ++i) {
            lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         }
         publisher.enableTimeRegulation(lookahead);
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabled,
            "timestamped directed-interaction time-regulation callback");
         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime timestamp = (LogicalTime) initial.add(lookahead).add(epsilon);
         byte[] tag = new byte[] {0x74, 0x73, 0x6f};
         publisher.setServiceReportingSwitch(true);
         MessageRetractionReturn retraction = publisher.sendDirectedInteraction(
            publisherInteraction, target,
            publisher.getParameterHandleValueMapFactory().create(0), tag, timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped directed-interaction send returned an invalid retraction handle");

         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "timestamped directed-interaction service report");
         JavaTckSupport.check(observerRecorder.interactions.size() == 1
               && receiverRecorder.directedInteractions.isEmpty()
               && receiverRecorder.grants.isEmpty(),
            "timestamped directed send produced an early delivery or unexpected report count");
         Interaction report = observerRecorder.interactions.get(0);
         JavaTckSupport.check(report.interaction.equals(reportClass)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length
               && report.tag.length == 0 && report.producer == null && report.regionsEmpty
               && observerReliable.equals(report.transportation),
            "timestamped directed-interaction service report returned non-standard metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
               "timestamped directed-interaction service report omitted " + name);
         }
         verifyReport(observer, report, reportParameters, encoder, publisherHandle,
            publisherInteraction, target, tag, timestamp, retraction);

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.timeAdvanceRequest(timestamp);
         LogicalTimeInterval publisherAdvanceInterval = (LogicalTimeInterval) epsilon.add(epsilon);
         LogicalTime publisherAdvance = (LogicalTime) initial.add(publisherAdvanceInterval);
         publisher.timeAdvanceRequest(publisherAdvance);
         awaitPair(publisher, receiver, callbackModel,
            () -> !receiverRecorder.directedInteractions.isEmpty()
               && !receiverRecorder.grants.isEmpty() && !publisherRecorder.grants.isEmpty(),
            "timestamped directed delivery and matching time grants");
         Directed directed = receiverRecorder.directedInteractions.get(0);
         JavaTckSupport.check(receiverRecorder.directedInteractions.size() == 1
               && directed.interaction.equals(receiverInteraction)
               && directed.target.equals(target) && directed.parameters.isEmpty()
               && Arrays.equals(tag, directed.tag)
               && receiverReliable.equals(directed.transportation)
               && publisherHandle.equals(directed.producer)
               && timestamp.equals(directed.time)
               && OrderType.TIMESTAMP.equals(directed.sentOrder)
               && OrderType.TIMESTAMP.equals(directed.receivedOrder)
               && retraction.handle.equals(directed.retraction)
               && receiverRecorder.grants.size() == 1
               && publisherRecorder.grants.size() == 1
               && timestamp.equals(receiverRecorder.grants.get(0))
               && publisherAdvance.equals(publisherRecorder.grants.get(0)),
            "timestamped directed-interaction callback or matching grant returned incorrect metadata");
         JavaTckSupport.check(receiverRecorder.callbackOrder.size() >= 2
               && "receiveDirectedInteraction".equals(receiverRecorder.callbackOrder.get(0))
               && "timeAdvanceGrant".equals(receiverRecorder.callbackOrder.get(1))
               && receiverRecorder.retractions.isEmpty(),
            "directed delivery did not precede its grant or produced an unexpected retraction callback");
      } finally {
         if (publisherConnected) try {
            publisher.setServiceReportingSwitch(false);
            publisher.setSendServiceReportsToFileSwitch(false);
         } catch (Exception ignored) { }
         if (receiverJoined && receiverDirectedSubscribed && receiverClass != null) try {
            receiver.unsubscribeObjectClassDirectedInteractions(receiverClass);
         } catch (Exception ignored) { }
         if (receiverJoined && receiverAttributesSubscribed && receiverClass != null
               && receiverAttributes != null) try {
            receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         } catch (Exception ignored) { }
         if (observerJoined && reportSubscribed && reportClass != null) try {
            observer.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (observerJoined) try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
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
         ObjectInstanceHandle target, byte[] tag, LogicalTime timestamp,
         MessageRetractionReturn retraction) throws Exception {
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         report.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(publisher.equals(reportedFederate),
         "timestamped directed-interaction service report identified the wrong federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      JavaTckSupport.check("SendDirectedInteraction".equals(service.getValue()),
         "timestamped directed-interaction service report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 2,
         "timestamped directed-interaction service report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue(),
         "timestamped directed-interaction service report indicated failure");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 5,
         "timestamped directed-interaction report returned the wrong argument count");
      verifyArgument(supplied.get(0), 27, "Interaction class designator",
         quoted(interaction.toString()));
      verifyArgument(supplied.get(1), 37, "Object instance designator", quoted(target.toString()));
      verifyArgument(supplied.get(2), 40,
         "Constrained set of interaction parameter designator and value pairs", "{}");
      verifyArgument(supplied.get(3), 60, "User-supplied tag", quoted(base64(tag)));
      verifyArgument(supplied.get(4), 31, "Optional timestamp", quoted(timestamp.toString()));

      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      String handleText = retraction.handle.toString();
      int open = handleText.indexOf('(');
      int close = handleText.indexOf(')');
      String encodedRetraction = ((HLAunicodeString) returned.get(2)).getValue();
      JavaTckSupport.check(returned.size() == 3
            && ((HLAinteger32BE) returned.get(0)).getValue() == 33
            && "Message retraction designator".equals(
               ((HLAunicodeString) returned.get(1)).getValue())
            && open >= 0 && close > open + 1
            && encodedRetraction.equals("\"MessageRetractionHandle<"
               + handleText.substring(open + 1, close) + ">\""),
         "timestamped directed-interaction service report returned a mismatched retraction designator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().isEmpty(),
         "successful timestamped directed-interaction report carried an exception");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == 0,
         "timestamped directed-interaction service report did not start at serial zero");
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
         "timestamped directed-interaction report supplied the wrong argument " + name);
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
   }

   private static final class Directed {
      private InteractionClassHandle interaction;
      private ObjectInstanceHandle target;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<Directed> directedInteractions = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] values = args == null ? new Object[0] : args;
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && values.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) values[0];
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeAdvanceGrant".equals(name) && values.length == 1) {
                  grants.add((LogicalTime<?, ?>) values[0]);
                  callbackOrder.add(name);
               } else if ("receiveDirectedInteraction".equals(name) && values.length == 10) {
                  Directed event = new Directed();
                  event.interaction = (InteractionClassHandle) values[0];
                  event.target = (ObjectInstanceHandle) values[1];
                  event.parameters = ((ParameterHandleValueMap) values[2]).clone();
                  event.tag = ((byte[]) values[3]).clone();
                  event.transportation = (TransportationTypeHandle) values[4];
                  event.producer = (FederateHandle) values[5];
                  event.time = (LogicalTime<?, ?>) values[6];
                  event.sentOrder = (OrderType) values[7];
                  event.receivedOrder = (OrderType) values[8];
                  event.retraction = (MessageRetractionHandle) values[9];
                  directedInteractions.add(event);
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name) && values.length == 1) {
                  retractions.add((MessageRetractionHandle) values[0]);
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
                  if ("toString".equals(name)) return "Java TCK timestamped directed report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == values[0];
               }
               return null;
            });
      }
   }
}
