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
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies ordinary object removal and the corresponding standard MOM report. */
final class ServiceReportObjectDeletionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportObjectDeletionTck() { }

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
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-delete-object-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet subjectAttributes = null, observerAttributes = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            "java-tck-delete-report-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-delete-report-observer", "java-tck", federation);
         observerJoined = true;

         objectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle subjectAttribute = subject.getAttributeHandle(objectClass, attributeName);
         ObjectClassHandle observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(observerClass, attributeName);
         JavaTckSupport.check(objectClass != null && subjectAttribute != null
               && observerClass != null && observerAttribute != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && attributeName.equals(subject.getAttributeName(objectClass, subjectAttribute)),
            "adapter-supplied object class or attribute lookup failed");
         subjectAttributes = JavaTckSupport.attributeSet(subject, subjectAttribute);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);

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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.publishObjectClassAttributes(objectClass, subjectAttributes);
         published = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         subscribed = true;

         ObjectInstanceHandle object = subject.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null, "object registration returned an invalid handle");
         await(observer, callbackModel, () -> recorder.discoveries.size() == 1,
            "object discovery before ordinary deletion");
         JavaTckSupport.check(object.equals(recorder.discoveries.get(0)),
            "observer discovered the wrong object instance");
         recorder.interactions.clear();
         recorder.removals.clear();

         byte[] tag = failureBoundaries
            ? new byte[] {0x72, 0x65, 0x67} : new byte[] {0x44, 0x45, 0x4c};
         subject.setServiceReportingSwitch(true);
         if (failureBoundaries) {
            ObjectInstanceHandle invalidObject =
               JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
            JavaTckSupport.expectFailure(
               () -> subject.deleteObjectInstance(invalidObject, tag),
               "ObjectInstanceNotKnown", "deleteObjectInstance with an unknown object");
            verifyFailureReport(observer, recorder, reportClass, reportParameters, reliable,
               subjectHandle, encoder, invalidObject.toString(), 0, callbackModel);
         }
         subject.deleteObjectInstance(object, tag);
         await(observer, callbackModel, () -> !recorder.interactions.isEmpty(),
            "DeleteObjectInstance HLAreportServiceInvocation callback");
         await(observer, callbackModel, () -> !recorder.removals.isEmpty(),
            "ordinary removeObjectInstance callback");
         JavaTckSupport.check(recorder.interactions.size() == 1 && recorder.removals.size() == 1,
            "object deletion produced an unexpected MOM report or removal callback count");
         Removal removal = recorder.removals.get(0);
         JavaTckSupport.check(object.equals(removal.object) && Arrays.equals(tag, removal.tag)
               && subjectHandle.equals(removal.producer),
            "removeObjectInstance callback returned the wrong object, tag, or producer");

         Interaction event = recorder.interactions.get(0);
         JavaTckSupport.check(reportClass.equals(event.interaction)
               && event.parameters.size() == REPORT_PARAMETER_NAMES.length
               && event.tag.length == 0 && event.producer == null && event.regionsEmpty
               && reliable.equals(event.transportation),
            "delete-object service report had invalid identity or callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
               "delete-object service report omitted standard parameter " + name);
         }
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            event.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(subjectHandle.equals(reportedFederate),
            "delete-object service report identified the wrong invoking federate");
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(event.parameters.get(reportParameters.get("HLAservice")));
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
         HLAboolean success = encoder.createHLAboolean();
         success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
         HLAinteger32BE serial = encoder.createHLAinteger32BE();
         serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
         JavaTckSupport.check("DeleteObjectInstance".equals(service.getValue())
               && serviceType.getValue() == 2 && success.getValue()
               && exception.getValue().isEmpty()
               && serial.getValue() == (failureBoundaries ? 1 : 0),
            "delete-object service report returned an incorrect service result");
         recorder.interactions.clear();
         recorder.removals.clear();
         if (failureBoundaries) {
            JavaTckSupport.expectFailure(
               () -> subject.deleteObjectInstance(object, tag),
               "ObjectInstanceNotKnown", "deleteObjectInstance with a stale object");
            verifyFailureReport(observer, recorder, reportClass, reportParameters, reliable,
               subjectHandle, encoder, object.toString(), 2, callbackModel);
         }
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined && subscribed) {
            try { observer.unsubscribeObjectClassAttributes(
               observer.getObjectClassHandle(objectClassName), observerAttributes); }
            catch (Exception ignored) { }
         }
         if (subjectJoined && published) {
            try { subject.unpublishObjectClassAttributes(objectClass, subjectAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyFailureReport(RTIambassador observer, Recorder recorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         TransportationTypeHandle reliable, FederateHandle subject, EncoderFactory encoder,
         String objectDesignator, int serialNumber, CallbackModel callbackModel) throws Exception {
      await(observer, callbackModel, () -> !recorder.interactions.isEmpty(),
         "failed DeleteObjectInstance HLAreportServiceInvocation callback");
      JavaTckSupport.check(recorder.interactions.size() == 1,
         "failed deletion did not produce exactly one service report");
      Interaction event = recorder.interactions.get(0);
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETER_NAMES.length
            && event.tag.length == 0 && event.producer == null && event.regionsEmpty
            && reliable.equals(event.transportation),
         "failed delete-object report had invalid callback identity or metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            "failed delete-object report omitted standard parameter " + name);
      }
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(subject.equals(reportedFederate),
         "failed delete-object report identified the wrong invoking federate");
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
      JavaTckSupport.check("DeleteObjectInstance".equals(service.getValue())
            && serviceType.getValue() == 2 && !success.getValue()
            && exception.getValue().contains("ObjectInstanceNotKnown")
            && serial.getValue() == serialNumber,
         "failed delete-object report returned the wrong service result");

      HLAfixedRecord prototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), prototype);
      supplied.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 3,
         "failed delete-object report returned the wrong supplied-argument count");
      HLAfixedRecord objectArgument = supplied.get(0);
      JavaTckSupport.check(((HLAinteger32BE) objectArgument.get(0)).getValue() == 37
            && "Object instance designator".equals(
               ((HLAunicodeString) objectArgument.get(1)).getValue())
            && ("\"" + objectDesignator + "\"").equals(
               ((HLAunicodeString) objectArgument.get(2)).getValue()),
         "failed delete-object report returned the wrong object argument");
      HLAfixedRecord returned = fixedRecordPrototype(encoder);
      returned.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returned.get(0)).getValue() == 34
            && ((HLAunicodeString) returned.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returned.get(2)).getValue()),
         "failed delete-object report returned the wrong Null result");
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

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
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
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
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
               if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  Removal removal = new Removal();
                  removal.object = (ObjectInstanceHandle) arguments[0];
                  removal.tag = ((byte[]) arguments[1]).clone();
                  removal.producer = (FederateHandle) arguments[2];
                  removals.add(removal);
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
                  if ("toString".equals(name)) return "Java TCK object-deletion report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK delete callbacks";
            if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
            if ("equals".equals(method.getName())) {
               return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
   }
}
