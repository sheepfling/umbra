package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
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
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies timestamped attribute delivery and its standard MOM service report. */
final class ServiceReportTimestampedAttributeUpdateTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportTimestampedAttributeUpdateTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-timestamped-attribute-report-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, observerConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      boolean observerJoined = false, published = false, subscribed = false;
      boolean reportSubscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
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
            "java-tck-tso-report-publisher-" + UUID.randomUUID(), "java-tck", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-tso-report-receiver-" + UUID.randomUUID(), "java-tck", federation);
         receiverJoined = true;
         observer.joinFederationExecution(
            "java-tck-tso-report-observer-" + UUID.randomUUID(), "java-tck", federation);
         observerJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null
               && publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "adapter-supplied object class and attribute did not resolve consistently");

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

         publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "timestamped service-report setup returned an invalid object handle");
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveredObject != null,
            "timestamped service-report object discovery");
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "timestamped service-report discovery identified the wrong object");

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.setServiceReportingSwitch(false);
         receiver.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         receiverRecorder.clearTimedEvents();
         observerRecorder.events.clear();

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverRecorder.timeConstrainedEnabled,
            "timestamped service-report time-constrained callback");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         for (int i = 0; i < 4; ++i) {
            lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         }
         publisher.enableTimeRegulation(lookahead);
         await(publisher, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled,
            "timestamped service-report time-regulation callback");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime timestamp = (LogicalTime) initial.add(lookahead).add(epsilon);
         byte[] payload = new byte[] {0x01, 0x02};
         byte[] tag = new byte[] {0x74, 0x73, 0x6f};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);
         publisher.setServiceReportingSwitch(true);
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, values, tag, timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped attribute update returned an invalid retraction handle");

         await(observer, callbackModel,
            () -> !observerRecorder.events.isEmpty(),
            "timestamped attribute-update service report");
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty()
               && receiverRecorder.grants.isEmpty(),
            "timestamped attribute update arrived before the receiver advanced time");
         Interaction report = observerRecorder.events.get(0);
         JavaTckSupport.check(observerRecorder.events.size() == 1
               && reportClass.equals(report.interaction)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length
               && report.tag.length == 0 && report.producer == null && report.regionsEmpty
               && observerReliable.equals(report.transportation),
            "timestamped service report returned invalid callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
               "timestamped service report omitted standard parameter " + name);
         }
         verifyReport(observer, report, reportParameters, encoder, publisherHandle, object,
            publisherAttribute, payload, tag, timestamp, retraction);

         receiver.timeAdvanceRequest(timestamp);
         LogicalTimeInterval publisherAdvanceInterval =
            (LogicalTimeInterval) epsilon.add(epsilon);
         LogicalTime publisherAdvance = (LogicalTime) initial.add(publisherAdvanceInterval);
         publisher.timeAdvanceRequest(publisherAdvance);
         awaitPair(publisher, receiver, callbackModel,
            () -> !receiverRecorder.reflections.isEmpty()
               && !receiverRecorder.grants.isEmpty() && !publisherRecorder.grants.isEmpty(),
            "timestamped attribute reflection and time advance grant");
         Reflection reflection = receiverRecorder.reflections.get(0);
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1
               && object.equals(reflection.object) && reflection.values.size() == 1
               && Arrays.equals(payload, reflection.values.get(receiverAttribute))
               && Arrays.equals(tag, reflection.tag) && timestamp.equals(reflection.time)
               && publisherHandle.equals(reflection.producer)
               && OrderType.TIMESTAMP.equals(reflection.sentOrder)
               && OrderType.TIMESTAMP.equals(reflection.receivedOrder)
               && reflection.regionsEmpty
               && receiverReliable.equals(reflection.transportation)
               && timestamp.equals(receiverRecorder.grants.get(0))
               && timestamp.equals(receiver.queryLogicalTime())
               && publisherAdvance.equals(publisherRecorder.grants.get(0))
               && publisherAdvance.equals(publisher.queryLogicalTime()),
            "timestamped reflection or matching grant returned incorrect standard metadata");
         JavaTckSupport.check(receiverRecorder.callbackOrder.size() >= 2
               && "reflectAttributeValues".equals(receiverRecorder.callbackOrder.get(0))
               && "timeAdvanceGrant".equals(receiverRecorder.callbackOrder.get(1)),
            "timestamped reflection was not delivered before its matching grant");
      } finally {
         if (receiverJoined && subscribed) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
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
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyReport(RTIambassador observer, Interaction report,
         Map<String, ParameterHandle> parameters, EncoderFactory encoder,
         FederateHandle publisher, ObjectInstanceHandle object, AttributeHandle attribute,
         byte[] payload, byte[] tag, LogicalTime timestamp,
         MessageRetractionReturn retraction) throws Exception {
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         report.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(publisher.equals(reportedFederate),
         "timestamped service report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      JavaTckSupport.check("UpdateAttributeValues".equals(service.getValue()),
         "timestamped service report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 2,
         "timestamped service report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue(), "timestamped service report indicated failure");
      HLAfixedRecord prototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), prototype);
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 4,
         "timestamped service report returned the wrong supplied-argument count");
      verifyArgument(supplied.get(0), 37, "Object instance designator",
         quoted(object.toString()));
      verifyArgument(supplied.get(1), 2,
         "Constrained set of attribute designator and value pairs",
         "{\"" + attribute.toString() + "\":\"" + base64(payload) + "\"}");
      verifyArgument(supplied.get(2), 60, "User-supplied tag", quoted(base64(tag)));
      verifyArgument(supplied.get(3), 31, "Optional timestamp", quoted(timestamp.toString()));
      HLAfixedRecord returned = encoder.createHLAfixedRecord();
      returned.add(encoder.createHLAinteger32BE());
      returned.add(encoder.createHLAunicodeString());
      returned.add(encoder.createHLAunicodeString());
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      String retractionArgument = ((HLAunicodeString) returned.get(2)).getValue();
      String handleText = retraction.handle.toString();
      int open = handleText.indexOf('(');
      int close = handleText.indexOf(')');
      JavaTckSupport.check(returned.size() == 3
            && ((HLAinteger32BE) returned.get(0)).getValue() == 33
            && "Message retraction designator".equals(
               ((HLAunicodeString) returned.get(1)).getValue())
            && open >= 0 && close > open + 1
            && retractionArgument.equals("\"MessageRetractionHandle<"
               + handleText.substring(open + 1, close) + ">\""),
         "timestamped service report returned a mismatched retraction designator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().isEmpty(),
         "successful timestamped service report carried an exception");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == 0,
         "timestamped service report did not start at serial number zero");
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
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
         "timestamped service report supplied the wrong argument " + name);
   }

   private static String quoted(String value) {
      return "\"" + value + "\"";
   }

   private static String base64(byte[] value) {
      return java.util.Base64.getEncoder().encodeToString(value);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isMet() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
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

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private LogicalTime<?, ?> time;
      private FederateHandle producer;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;

      private void clearTimedEvents() {
         reflections.clear();
         grants.clear();
         callbackOrder.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Reflection event = new Reflection();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.producer = (FederateHandle) arguments[4];
                  event.time = (LogicalTime<?, ?>) arguments[6];
                  event.sentOrder = (OrderType) arguments[7];
                  event.receivedOrder = (OrderType) arguments[8];
                  event.transportation = (TransportationTypeHandle) arguments[9];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  reflections.add(event);
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source =
                     (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     event.parameters.put(handle, source.get(handle).clone());
                  }
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  events.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped service-report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
