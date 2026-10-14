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

/** Verifies standard reports for rejected ordinary attribute updates. */
final class ServiceReportAttributeUpdateFailureTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportAttributeUpdateFailureTck() { }

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
      String federation = "java-tck-service-report-attribute-update-failure-" + UUID.randomUUID();
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
            "java-tck-attribute-failure-publisher-" + UUID.randomUUID(), "java-tck", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-attribute-failure-receiver-" + UUID.randomUUID(), "java-tck", federation);
         receiverJoined = true;
         observer.joinFederationExecution(
            "java-tck-attribute-failure-observer-" + UUID.randomUUID(), "java-tck", federation);
         observerJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null,
            "adapter-supplied object class or attribute lookup returned an invalid handle");

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
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(receiver, callbackModel, () -> receiverRecorder.discoveredObject != null,
            "ordinary attribute-update failure object discovery");
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "ordinary attribute-update failure discovery returned the wrong object");

         byte[] value = new byte[] {0x01, 0x02};
         byte[] tag = new byte[] {0x72, 0x65, 0x67};
         AttributeHandleValueMap validValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         validValues.put(publisherAttribute, value);
         AttributeHandleValueMap invalidValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         AttributeHandle invalidAttribute =
            JavaTckSupport.invalidHandle(AttributeHandle.class);
         invalidValues.put(invalidAttribute, value);
         int reflectionsBeforeFailures = receiverRecorder.reflections.size();
         publisher.setServiceReportingSwitch(true);
         publisher.setSendServiceReportsToFileSwitch(false);

         ObjectInstanceHandle invalidObject =
            JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
         JavaTckSupport.expectFailure(
            () -> publisher.updateAttributeValues(invalidObject, validValues, tag),
            "ObjectInstanceNotKnown", "updateAttributeValues with an unknown object");
         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "unknown-object attribute-update failure report");
         verifyFailureReport(observer, observerRecorder.interactions.get(0), reportClass,
            reportParameters, encoder, publisherHandle, invalidObject.toString(),
            publisherAttribute.toString(), value, tag, "ObjectInstanceNotKnown", 0, reliable);
         observerRecorder.interactions.clear();
         settle(receiver, callbackModel);
         JavaTckSupport.check(receiverRecorder.reflections.size() == reflectionsBeforeFailures,
            "unknown-object attribute update delivered an ordinary reflection");

         JavaTckSupport.expectFailure(
            () -> publisher.updateAttributeValues(object, invalidValues, tag),
            "AttributeNotDefined", "updateAttributeValues with an undefined attribute");
         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "undefined-attribute update failure report");
         verifyFailureReport(observer, observerRecorder.interactions.get(0), reportClass,
            reportParameters, encoder, publisherHandle, object.toString(),
            invalidAttribute.toString(), value, tag, "AttributeNotDefined", 1, reliable);
         observerRecorder.interactions.clear();
         settle(receiver, callbackModel);
         JavaTckSupport.check(receiverRecorder.reflections.size() == reflectionsBeforeFailures,
            "undefined-attribute update delivered an ordinary reflection");
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

   private static void verifyFailureReport(RTIambassador observer, Interaction report,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         EncoderFactory encoder, FederateHandle publisher, String objectValue,
         String attributeValue, byte[] value, byte[] tag, String exceptionName,
         int serialNumber, TransportationTypeHandle reliable) throws Exception {
      JavaTckSupport.check(reportClass.equals(report.interaction)
            && report.parameters.size() == REPORT_PARAMETER_NAMES.length
            && report.tag.length == 0 && report.producer == null && report.regionsEmpty
            && reliable.equals(report.transportation),
         "attribute-update failure report returned invalid callback metadata");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         report.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(publisher.equals(reportedFederate),
         "attribute-update failure report identified the wrong invoking federate");

      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check("UpdateAttributeValues".equals(service.getValue())
            && serviceType.getValue() == 2 && !success.getValue(),
         "attribute-update failure report returned the wrong service result");

      HLAfixedRecord prototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), prototype);
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 4,
         "attribute-update failure report returned the wrong supplied-argument count");
      verifyArgument(supplied.get(0), 37, "Object instance designator", quoted(objectValue));
      verifyArgument(supplied.get(1), 2,
         "Constrained set of attribute designator and value pairs",
         "{\"" + attributeValue + "\":\"" + base64(value) + "\"}");
      verifyArgument(supplied.get(2), 60, "User-supplied tag", quoted(base64(tag)));
      verifyArgument(supplied.get(3), 34, "Optional timestamp", "null");

      HLAfixedRecord returned = fixedRecordPrototype(encoder);
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returned.get(0)).getValue() == 34
            && ((HLAunicodeString) returned.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returned.get(2)).getValue()),
         "attribute-update failure report returned the wrong null value");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().contains(exceptionName),
         "attribute-update failure report named the wrong exception");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == serialNumber,
         "attribute-update failure report returned the wrong serial number");
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
         "attribute-update failure report supplied the wrong argument " + name);
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

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      } else {
         for (int i = 0; i < 64; ++i) Thread.yield();
      }
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

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> reflections = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  reflections.add((ObjectInstanceHandle) arguments[0]);
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
                  if ("toString".equals(name)) return "Java TCK attribute-update failure callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
