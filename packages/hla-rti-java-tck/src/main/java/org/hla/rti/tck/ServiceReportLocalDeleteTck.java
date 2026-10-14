package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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

/** Verifies the standard MOM report for a successful local object deletion. */
final class ServiceReportLocalDeleteTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportLocalDeleteTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         callbackModel, false);
   }

   static void runFailureBoundaries(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         boolean failureBoundaries) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-local-delete-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, reportSubscribed = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      InteractionClassHandle reportClass = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-local-delete-report-owner", "java-tck", federation);
         ownerJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
            "java-tck-local-delete-report-requester", "java-tck", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && ownerAttribute != null
               && requesterClass != null && requesterAttribute != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute)),
            "adapter-supplied object class or attribute lookup failed");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);

         reportClass = owner.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS_NAME.equals(owner.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETER_NAMES) {
            ParameterHandle handle = owner.getParameterHandle(reportClass, name);
            JavaTckSupport.check(handle != null
                  && name.equals(owner.getParameterName(reportClass, handle)),
               "standard service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, handle);
         }
         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(owner.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup/name round-trip failed");

         owner.setServiceReportingSwitch(false);
         owner.setSendServiceReportsToFileSwitch(false);
         requester.setServiceReportingSwitch(false);
         requester.setSendServiceReportsToFileSwitch(false);
         owner.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         subscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "local-delete service-report object registration returned an invalid handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.size() == 1,
            "object discovery before local deletion");
         JavaTckSupport.check(object.equals(requesterRecorder.discoveries.get(0)),
            "requester discovered the wrong object instance");

         ownerRecorder.interactions.clear();
         requester.setServiceReportingSwitch(true);
         if (failureBoundaries) {
            ObjectInstanceHandle invalidObject =
               JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
            JavaTckSupport.expectFailure(
               () -> requester.localDeleteObjectInstance(invalidObject),
               "ObjectInstanceNotKnown", "localDeleteObjectInstance with an unknown object");
            verifyFailureReport(owner, ownerRecorder, reportClass, reportParameters,
               reliable, requesterHandle, encoder, invalidObject.toString(), 0, callbackModel);
         }
         requester.localDeleteObjectInstance(object);
         await(owner, callbackModel, () -> !ownerRecorder.interactions.isEmpty(),
            "LocalDeleteObjectInstance HLAreportServiceInvocation callback");
         JavaTckSupport.check(ownerRecorder.interactions.size() == 1,
            "local deletion did not produce exactly one service report");
         Interaction report = ownerRecorder.interactions.get(0);
         JavaTckSupport.check(reportClass.equals(report.interaction)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length
               && report.tag.length == 0 && report.producer == null && report.regionsEmpty
               && reliable.equals(report.transportation),
            "local-delete service report returned invalid identity or callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
               "local-delete service report omitted standard parameter " + name);
         }
         FederateHandle reportedFederate = owner.getFederateHandleFactory().decode(
            report.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(requesterHandle.equals(reportedFederate),
            "local-delete service report identified the wrong invoking federate");

         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(report.parameters.get(reportParameters.get("HLAservice")));
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(report.parameters.get(reportParameters.get("HLAserviceType")));
         HLAboolean success = encoder.createHLAboolean();
         success.decode(report.parameters.get(reportParameters.get("HLAsuccessIndicator")));
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(report.parameters.get(reportParameters.get("HLAexception")));
         HLAinteger32BE serial = encoder.createHLAinteger32BE();
         serial.decode(report.parameters.get(reportParameters.get("HLAserialNumber")));
         JavaTckSupport.check("LocalDeleteObjectInstance".equals(service.getValue())
               && serviceType.getValue() == 2 && success.getValue()
               && exception.getValue().isEmpty()
               && serial.getValue() == (failureBoundaries ? 1 : 0),
            "local-delete service report contained an incorrect result or serial number");
         ownerRecorder.interactions.clear();
         if (failureBoundaries) {
            JavaTckSupport.expectFailure(
               () -> requester.localDeleteObjectInstance(object),
               "ObjectInstanceNotKnown", "localDeleteObjectInstance with a stale object");
            verifyFailureReport(owner, ownerRecorder, reportClass, reportParameters,
               reliable, requesterHandle, encoder, object.toString(), 2, callbackModel);
         }
      } finally {
         if (requesterJoined) {
            try { requester.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (ownerJoined && reportSubscribed) {
            try { owner.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (requesterJoined && subscribed) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyFailureReport(RTIambassador owner, Recorder recorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         TransportationTypeHandle reliable, FederateHandle requester, EncoderFactory encoder,
         String objectDesignator, int serialNumber, CallbackModel callbackModel) throws Exception {
      await(owner, callbackModel, () -> !recorder.interactions.isEmpty(),
         "local-delete failure HLAreportServiceInvocation callback");
      JavaTckSupport.check(recorder.interactions.size() == 1,
         "local-delete failure did not produce exactly one service report");
      Interaction event = recorder.interactions.get(0);
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETER_NAMES.length
            && event.tag.length == 0 && event.producer == null && event.regionsEmpty
            && reliable.equals(event.transportation),
         "local-delete failure report had invalid callback identity or metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            "local-delete failure report omitted standard parameter " + name);
      }
      FederateHandle reportedFederate = owner.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(requester.equals(reportedFederate),
         "local-delete failure report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(event.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(event.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check("LocalDeleteObjectInstance".equals(service.getValue())
            && serviceType.getValue() == 2 && !success.getValue()
            && exception.getValue().contains("ObjectInstanceNotKnown")
            && serial.getValue() == serialNumber,
         "local-delete failure report returned the wrong service result");

      HLAfixedRecord prototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), prototype);
      supplied.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 1,
         "local-delete failure report returned the wrong supplied-argument count");
      HLAfixedRecord argument = supplied.get(0);
      JavaTckSupport.check(((HLAinteger32BE) argument.get(0)).getValue() == 37
            && "Object instance designator".equals(
               ((HLAunicodeString) argument.get(1)).getValue())
            && ("\"" + objectDesignator + "\"").equals(
               ((HLAunicodeString) argument.get(2)).getValue()),
         "local-delete failure report returned the wrong object argument");
      HLAfixedRecord returned = fixedRecordPrototype(encoder);
      returned.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returned.get(0)).getValue() == 34
            && ((HLAunicodeString) returned.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returned.get(2)).getValue()),
         "local-delete failure report returned the wrong Null result");
      recorder.interactions.clear();
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE());
      record.add(encoder.createHLAunicodeString());
      record.add(encoder.createHLAunicodeString());
      return record;
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
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : values.keySet()) {
                     event.parameters.put(handle, values.get(handle).clone());
                  }
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK local-delete report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
