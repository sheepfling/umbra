package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
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

/** Standard Table 5 AttributeRegionAssociation service-report value witness. */
final class ServiceReportAssociateRegionsForUpdatesTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportAssociateRegionsForUpdatesTck() { }

   static void run(RtiFactory factory, String fom, String modelFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && modelFom != null && !modelFom.isEmpty()
            && mim != null && !mim.isEmpty(),
         "adapter must supply the base FOM, dimensional model FOM, and standard MIM");
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder observerCallbacks = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-associate-regions-" + UUID.randomUUID();
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean reportSubscribed = false;
      RegionHandle region = null;
      ObjectInstanceHandle object = null;
      AttributeSetRegionSetPairList associations = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(new Recorder().proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerCallbacks.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom, modelFom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            "java-tck-associate-regions-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-associate-regions-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            reportParameters.put(name, observer.getParameterHandle(reportClass, name));
         }
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = subject.getAttributeHandle(objectClass, attributeName);
         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(subject, attribute);
         subject.publishObjectClassAttributes(objectClass, attributes);
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound > 0,
            "adapter model FOM did not provide a positive dimension upper bound");
         DimensionHandleSet dimensions = subject.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = subject.createRegion(dimensions);
         subject.setRangeBounds(region, dimension, new RangeBounds(0, upperBound));
         RegionHandleSet regions = subject.getRegionHandleSetFactory().create();
         regions.add(region);
         subject.commitRegionModifications(regions);
         object = subject.registerObjectInstance(objectClass);
         associations = subject.getAttributeSetRegionSetPairListFactory().create(1);
         associations.add(new AttributeRegionAssociation(attributes, regions));

         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "adapter did not select standard MOM callback delivery");
         subject.setServiceReportingSwitch(true);
         subject.associateRegionsForUpdates(object, associations);
         await(observer, callbackModel, () -> observerCallbacks.interactions.size() >= 1);
         JavaTckSupport.check(observerCallbacks.interactions.size() == 1,
            "Associate Regions for Updates emitted an unexpected number of service reports");
         verifyReport(observer, observerCallbacks.interactions.get(0), encoder,
            reportClass, reportParameters, subjectHandle, object, attribute, region);

         subject.setServiceReportingSwitch(false);
         subject.unassociateRegionsForUpdates(object, associations);
         associations = null;
         subject.deleteRegion(region);
         region = null;
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (subjectJoined && object != null && associations != null) {
            try { subject.unassociateRegionsForUpdates(object, associations); }
            catch (Exception ignored) { }
         }
         if (subjectJoined && region != null) {
            try { subject.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) resign(subject, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (observerJoined) resign(observer, ResignAction.NO_ACTION);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void verifyReport(RTIambassador observer, Interaction report,
         EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, FederateHandle subject,
         ObjectInstanceHandle object, AttributeHandle attribute, RegionHandle region)
         throws Exception {
      JavaTckSupport.check(reportClass.equals(report.interaction)
            && report.parameters.size() == parameters.size()
            && report.tag.length == 0 && report.producer == null
            && report.regionsEmpty
            && observer.getTransportationTypeHandle("HLAreliable")
               .equals(report.transportation),
         "Associate Regions for Updates service report had incorrect callback metadata");
      for (ParameterHandle handle : parameters.values()) {
         JavaTckSupport.check(report.parameters.containsKey(handle),
            "Associate Regions for Updates report omitted a standard parameter");
      }
      JavaTckSupport.check(subject.equals(observer.getFederateHandleFactory().decode(
            report.parameters.get(parameters.get("HLAfederate")), 0)),
         "Associate Regions for Updates report named the wrong federate");

      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check("AssociateRegionsForUpdates".equals(service.getValue())
            && serviceType.getValue() == 5 && success.getValue(),
         "Associate Regions for Updates report had the wrong service or success metadata");

      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), fixedRecordPrototype(encoder));
      suppliedArguments.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(suppliedArguments.size() == 2,
         "Associate Regions for Updates report had the wrong supplied-argument count");
      String associationValue = "[{\"attributeHandleSet\":[" + quote(attribute.toString())
         + "],\"regionHandleSet\":[" + quote(region.toString()) + "]}]";
      checkArgument(suppliedArguments.get(0), 37,
         "Object instance designator", quote(object.toString()));
      checkArgument(suppliedArguments.get(1), 4,
         "Collection of attribute designator set and region designator set pairs",
         associationValue);
      HLAfixedRecord returned = fixedRecordPrototype(encoder);
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      checkArgument(returned, 34, "", "null");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(exception.getValue().isEmpty() && serial.getValue() == 0,
         "Associate Regions for Updates report had an exception or unexpected serial");
   }

   private static void checkArgument(HLAfixedRecord argument, int type,
         String name, String value) {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue())
            && value.equals(((HLAunicodeString) argument.get(2)).getValue()),
         "Associate Regions for Updates report had an incorrect argument: " + name);
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static String quote(String value) { return "\"" + value + "\""; }

   private static void await(RTIambassador ambassador, CallbackModel model,
         java.util.function.BooleanSupplier condition) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for Associate Regions for Updates service report");
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         for (ParameterHandle handle : values.keySet()) {
            parameters.put(handle, values.get(handle).clone());
         }
         tag = arguments[2] == null ? new byte[0] : ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet callbackRegions = (RegionHandleSet) arguments[5];
         regionsEmpty = callbackRegions == null || callbackRegions.isEmpty();
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) interactions.add(new Interaction(arguments));
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK association service reports";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
