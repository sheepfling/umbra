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

/** Verifies the standard MOM report for a successful ordinary attribute update. */
final class ServiceReportAttributeUpdateTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportAttributeUpdateTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-attribute-update-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false;
      boolean created = false, subjectJoined = false, observerJoined = false;
      boolean published = false, reportSubscribed = false;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet attributes = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            "java-tck-service-report-attribute-subject-" + UUID.randomUUID(),
            "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-service-report-attribute-observer-" + UUID.randomUUID(),
            "java-tck", federation);
         observerJoined = true;

         objectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = subject.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null,
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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         attributes = JavaTckSupport.attributeSet(subject, attribute);
         subject.publishObjectClassAttributes(objectClass, attributes);
         published = true;
         ObjectInstanceHandle object = subject.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "ordinary update service-report object registration returned an invalid handle");

         AttributeHandleValueMap values = subject.getAttributeHandleValueMapFactory().create(1);
         values.put(attribute, new byte[] {0x01, 0x02});
         byte[] tag = new byte[] {0x00, 0x01, (byte) 0xFE, (byte) 0xFF};
         subject.setServiceReportingSwitch(true);
         subject.updateAttributeValues(object, values, tag);

         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "ordinary attribute-update HLAreportServiceInvocation callback");
         Interaction report = observerRecorder.interactions.get(0);
         JavaTckSupport.check(observerRecorder.interactions.size() == 1
               && reportClass.equals(report.interaction)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length,
            "ordinary attribute-update service report returned invalid interaction metadata");
         JavaTckSupport.check(report.tag.length == 0,
            "ordinary attribute-update service report returned a nonempty tag");
         JavaTckSupport.check(report.regionsEmpty,
            "ordinary attribute-update service report returned a nonempty region set");
         JavaTckSupport.check(report.producer == null,
            "ordinary attribute-update service report returned a producing federate");
         JavaTckSupport.check(reliable.equals(report.transportation),
            "ordinary attribute-update service report used the wrong transportation type");
         for (ParameterHandle handle : reportParameters.values()) {
            JavaTckSupport.check(report.parameters.containsKey(handle),
               "ordinary attribute-update service report omitted a standard parameter");
         }
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            report.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(subjectHandle.equals(reportedFederate),
            "ordinary attribute-update report identified the wrong invoking federate");

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
         JavaTckSupport.check("UpdateAttributeValues".equals(service.getValue())
               && serviceType.getValue() == 2 && success.getValue()
               && exception.getValue().isEmpty() && serial.getValue() == 0,
            "ordinary attribute-update report contained an incorrect service result");

         HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
         HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
            index -> fixedRecordPrototype(encoder), argumentPrototype);
         suppliedArguments.decode(report.parameters.get(
            reportParameters.get("HLAsuppliedArguments")));
         JavaTckSupport.check(suppliedArguments.size() == 4,
            "ordinary attribute-update report returned an unexpected supplied-argument count");
         HLAfixedRecord attributeValueArgument = suppliedArguments.get(1);
         JavaTckSupport.check(attributeValueArgument.size() == 3,
            "ordinary attribute-update map argument did not have three fields");
         int reportedAttributeMapType =
            ((HLAinteger32BE) attributeValueArgument.get(0)).getValue();
         String reportedAttributeMapName =
            ((HLAunicodeString) attributeValueArgument.get(1)).getValue();
         String reportedAttributeMapValue =
            ((HLAunicodeString) attributeValueArgument.get(2)).getValue();
         String expectedAttributeMapValue = "{\"" + attribute.toString() + "\":\"AQI=\"}";
         String mapPrefix = "{\"";
         String mapSuffix = "\":\"AQI=\"}";
         String keyValueSeparator = "\":\"";
         int keyValueSeparatorIndex = reportedAttributeMapValue.indexOf(keyValueSeparator);
         boolean mapContainsOneAttributeValue = reportedAttributeMapValue.startsWith(mapPrefix)
            && reportedAttributeMapValue.endsWith(mapSuffix)
            && keyValueSeparatorIndex > mapPrefix.length()
            && keyValueSeparatorIndex == reportedAttributeMapValue.lastIndexOf(keyValueSeparator)
            && reportedAttributeMapValue.length() > mapPrefix.length() + mapSuffix.length();
         JavaTckSupport.check(reportedAttributeMapType == 2
               && "Constrained set of attribute designator and value pairs".equals(
                  reportedAttributeMapName)
               && mapContainsOneAttributeValue
               && expectedAttributeMapValue.equals(reportedAttributeMapValue),
            "ordinary attribute-update report attribute-map record differed: type="
               + reportedAttributeMapType + ", name=" + reportedAttributeMapName
               + ", value=" + reportedAttributeMapValue + ", expected="
               + expectedAttributeMapValue);
         HLAfixedRecord userSuppliedTagArgument = suppliedArguments.get(2);
         JavaTckSupport.check(userSuppliedTagArgument.size() == 3,
            "ordinary attribute-update tag argument did not have three fields");
         JavaTckSupport.check("User-supplied tag".equals(
                  ((HLAunicodeString) userSuppliedTagArgument.get(1)).getValue())
               && "\"AAH+/w==\"".equals(
                  ((HLAunicodeString) userSuppliedTagArgument.get(2)).getValue()),
            "ordinary attribute-update report did not encode its binary tag as Base64");
      } finally {
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (subjectJoined && published) {
            try { subject.unpublishObjectClassAttributes(objectClass, attributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
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

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
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

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
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
                  if ("toString".equals(name)) return "Java TCK attribute-update report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
