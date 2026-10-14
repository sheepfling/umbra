package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies timestamped object deletion and its standard MOM service report. */
final class ServiceReportTimestampedObjectDeletionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportTimestampedObjectDeletionTck() { }

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
      String federation = "java-tck-timestamped-deletion-report-" + UUID.randomUUID();
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
            "java-tck-timestamped-delete-publisher-" + UUID.randomUUID(), "java-tck", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-timestamped-delete-receiver-" + UUID.randomUUID(), "java-tck", federation);
         receiverJoined = true;
         observer.joinFederationExecution(
            "java-tck-timestamped-delete-observer-" + UUID.randomUUID(), "java-tck", federation);
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
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "timestamped deletion service-report registration returned an invalid object");
         await(receiver, callbackModel, () -> receiverRecorder.discoveredObject != null,
            "timestamped deletion service-report object discovery");
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "timestamped deletion service-report discovery identified the wrong object");

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.setServiceReportingSwitch(false);
         receiver.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         receiverRecorder.removals.clear();
         receiverRecorder.grants.clear();
         receiverRecorder.callbackOrder.clear();
         observerRecorder.interactions.clear();

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         receiver.enableTimeConstrained();
         await(receiver, callbackModel, () -> receiverRecorder.timeConstrainedEnabled,
            "timestamped deletion service-report time-constrained callback");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         for (int i = 0; i < 4; ++i) lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         publisher.enableTimeRegulation(lookahead);
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabled,
            "timestamped deletion service-report time-regulation callback");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime timestamp = (LogicalTime) initial.add(lookahead).add(epsilon);
         byte[] tag = new byte[] {0x74, 0x73, 0x6f};
         publisher.setServiceReportingSwitch(true);
         MessageRetractionReturn retraction = publisher.deleteObjectInstance(object, tag, timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Delete Object Instance returned an invalid retraction handle");

         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "timestamped Delete Object Instance service report");
         JavaTckSupport.check(receiverRecorder.removals.isEmpty()
               && receiverRecorder.grants.isEmpty(),
            "timestamped object deletion arrived before the constrained receiver advanced");
         Interaction report = observerRecorder.interactions.get(0);
         JavaTckSupport.check(observerRecorder.interactions.size() == 1
               && reportClass.equals(report.interaction)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length
               && report.tag.length == 0 && report.producer == null && report.regionsEmpty
               && observerReliable.equals(report.transportation),
            "timestamped deletion service report returned invalid callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
               "timestamped deletion service report omitted standard parameter " + name);
         }
         verifyReport(observer, report, reportParameters, encoder, publisherHandle,
            object, tag, timestamp, retraction);

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.timeAdvanceRequest(timestamp);
         JavaTckSupport.check(receiverRecorder.removals.isEmpty(),
            "timestamped object deletion arrived before the regulating publisher advanced");
         LogicalTimeInterval publisherAdvanceInterval =
            (LogicalTimeInterval) epsilon.add(epsilon);
         LogicalTime publisherAdvance = (LogicalTime) initial.add(publisherAdvanceInterval);
         publisher.timeAdvanceRequest(publisherAdvance);
         awaitPair(publisher, receiver, callbackModel,
            () -> !receiverRecorder.removals.isEmpty()
               && !receiverRecorder.grants.isEmpty() && !publisherRecorder.grants.isEmpty(),
            "timestamped deletion callback and both time advance grants");
         Removal removal = receiverRecorder.removals.get(0);
         JavaTckSupport.check(receiverRecorder.removals.size() == 1
               && object.equals(removal.object) && Arrays.equals(tag, removal.tag)
               && publisherHandle.equals(removal.producer)
               && timestamp.equals(removal.time)
               && OrderType.TIMESTAMP.equals(removal.sentOrder)
               && OrderType.TIMESTAMP.equals(removal.receivedOrder)
               && retraction.handle.equals(removal.retraction)
               && timestamp.equals(receiverRecorder.grants.get(0))
               && timestamp.equals(receiver.queryLogicalTime())
               && publisherAdvance.equals(publisherRecorder.grants.get(0))
               && publisherAdvance.equals(publisher.queryLogicalTime()),
            "timestamped removal or matching grants returned incorrect standard metadata");
         JavaTckSupport.check(receiverRecorder.callbackOrder.size() >= 2
               && "removeObjectInstance".equals(receiverRecorder.callbackOrder.get(0))
               && "timeAdvanceGrant".equals(receiverRecorder.callbackOrder.get(1)),
            "timestamped removal was not delivered before its matching grant");
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
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
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
         FederateHandle publisher, ObjectInstanceHandle object, byte[] tag,
         LogicalTime timestamp, MessageRetractionReturn retraction) throws Exception {
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         report.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(publisher.equals(reportedFederate),
         "timestamped deletion report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      JavaTckSupport.check("DeleteObjectInstance".equals(service.getValue()),
         "timestamped deletion report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 2,
         "timestamped deletion report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue(), "timestamped deletion report indicated failure");

      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), fixedRecordPrototype(encoder));
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 3,
         "timestamped deletion report returned the wrong supplied-argument count");
      verifyArgument(supplied.get(0), 37, "Object instance designator", quoted(object.toString()));
      verifyArgument(supplied.get(1), 60, "User-supplied tag", quoted(base64(tag)));
      verifyArgument(supplied.get(2), 31, "Optional timestamp", quoted(timestamp.toString()));

      HLAfixedRecord returned = encoder.createHLAfixedRecord();
      returned.add(encoder.createHLAinteger32BE());
      returned.add(encoder.createHLAunicodeString());
      returned.add(encoder.createHLAunicodeString());
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      String handleText = retraction.handle.toString();
      int open = handleText.indexOf('(');
      int close = handleText.indexOf(')');
      JavaTckSupport.check(returned.size() == 3
            && ((HLAinteger32BE) returned.get(0)).getValue() == 33
            && "Message retraction designator".equals(
               ((HLAunicodeString) returned.get(1)).getValue())
            && open >= 0 && close > open + 1
            && ("\"MessageRetractionHandle<" + handleText.substring(open + 1, close) + ">\"")
               .equals(((HLAunicodeString) returned.get(2)).getValue()),
         "timestamped deletion report returned a mismatched retraction designator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().isEmpty(),
         "successful timestamped deletion report carried an exception");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == 0,
         "timestamped deletion report did not start at serial number zero");
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE());
      record.add(encoder.createHLAunicodeString());
      record.add(encoder.createHLAunicodeString());
      return record;
   }

   private static void verifyArgument(HLAfixedRecord argument, int type, String name,
         String value) throws Exception {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue())
            && value.equals(((HLAunicodeString) argument.get(2)).getValue()),
         "timestamped deletion report supplied the wrong argument " + name);
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

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;

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
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 7) {
                  Removal event = new Removal();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.tag = ((byte[]) arguments[1]).clone();
                  event.producer = (FederateHandle) arguments[2];
                  event.time = (LogicalTime<?, ?>) arguments[3];
                  event.sentOrder = (OrderType) arguments[4];
                  event.receivedOrder = (OrderType) arguments[5];
                  event.retraction = (MessageRetractionHandle) arguments[6];
                  removals.add(event);
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
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
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped deletion callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
