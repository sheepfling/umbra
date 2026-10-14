package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RangeBounds;
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

/** Verifies service reports for active and passive regional subscriptions. */
final class ServiceReportRegionalInteractionSubscriptionTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportRegionalInteractionSubscriptionTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied dimensional FOM is required");
      JavaTckSupport.check(mim != null && !mim.isEmpty(),
         "adapter-supplied standard MIM is required");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectCallbacks = new Recorder();
      Recorder observerCallbacks = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-regional-interaction-subscription-"
         + UUID.randomUUID();
      boolean[] connected = new boolean[2];
      boolean created = false;
      boolean[] joined = new boolean[2];
      boolean reportSubscribed = false;
      boolean passiveSubscribed = false;
      boolean activeSubscribed = false;
      RegionHandle region = null;
      InteractionClassHandle interaction = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(subjectCallbacks.proxy(), callbackModel);
         connected[0] = true;
         observer.connect(observerCallbacks.proxy(), callbackModel);
         connected[1] = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution("regional-subscription-subject", "java-tck", federation);
         joined[0] = true;
         observer.joinFederationExecution("regional-subscription-observer", "java-tck", federation);
         joined[1] = true;

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);

         interaction = subject.getInteractionClassHandle(interactionName);
         DimensionHandle first = subject.getDimensionHandle(firstDimensionName);
         DimensionHandle second = subject.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(subject.getAvailableDimensionsForInteractionClass(interaction)
               .contains(first)
               && subject.getAvailableDimensionsForInteractionClass(interaction).contains(second)
               && subject.getDimensionUpperBound(first) >= 10
               && subject.getDimensionUpperBound(second) >= 10,
            "configured interaction class does not expose both routing dimensions");

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle handle = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(name.equals(observer.getParameterName(reportClass, handle)),
               "standard service-report parameter lookup/name round-trip failed: " + name);
            reportParameters.put(name, handle);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check("HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation handle did not round-trip");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandleSet dimensions = subject.getDimensionHandleSetFactory().create();
         dimensions.add(first);
         dimensions.add(second);
         region = subject.createRegion(dimensions);
         subject.setRangeBounds(region, first, new RangeBounds(0, 10));
         subject.setRangeBounds(region, second, new RangeBounds(0, 10));
         RegionHandleSet singleton = regionSet(subject, region);
         subject.commitRegionModifications(singleton);
         RegionHandleSet regionSet = singleton;
         String interactionDesignator = interaction.toString();
         String regionDesignator = region.toString();

         subject.setServiceReportingSwitch(true);
         subject.setSendServiceReportsToFileSwitch(false);
         subject.subscribeInteractionClassPassivelyWithRegions(interaction, regionSet);
         passiveSubscribed = true;
         verifyReport(observer, observerCallbacks, callbackModel, encoder,
            reportClass, reportParameters, reliable, 0, "SubscribeInteractionClassWithRegions",
            List.of(new ExpectedArgument(27, "Interaction class designator",
                  quote(interactionDesignator)),
               new ExpectedArgument(43, "Set of region designators",
                  "[\"" + regionDesignator + "\"]"),
               new ExpectedArgument(6, "Optional passive subscription indicator", "true")));
         subject.subscribeInteractionClassWithRegions(interaction, regionSet);
         activeSubscribed = true;
         verifyReport(observer, observerCallbacks, callbackModel, encoder,
            reportClass, reportParameters, reliable, 1, "SubscribeInteractionClassWithRegions",
            List.of(new ExpectedArgument(27, "Interaction class designator",
                  quote(interactionDesignator)),
               new ExpectedArgument(43, "Set of region designators",
                  "[\"" + regionDesignator + "\"]"),
               new ExpectedArgument(6, "Optional passive subscription indicator", "false")));
         subject.unsubscribeInteractionClassWithRegions(interaction, regionSet);
         activeSubscribed = false;
         passiveSubscribed = false;
         verifyReport(observer, observerCallbacks, callbackModel, encoder,
            reportClass, reportParameters, reliable, 2, "UnsubscribeInteractionClassWithRegions",
            List.of(new ExpectedArgument(27, "Interaction class designator",
                  quote(interactionDesignator)),
               new ExpectedArgument(43, "Set of region designators",
                  "[\"" + regionDesignator + "\"]")));

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         region = null;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         joined[0] = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[0]) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { subject.setSendServiceReportsToFileSwitch(false); } catch (Exception ignored) { }
         }
         if (reportSubscribed && joined[1]) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if ((passiveSubscribed || activeSubscribed) && joined[0] && region != null) {
            try { subject.unsubscribeInteractionClassWithRegions(interaction,
               regionSet(subject, region)); } catch (Exception ignored) { }
         }
         if (region != null && joined[0]) {
            try { subject.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (joined[1]) resign(observer);
         if (joined[0]) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected[1]) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (connected[0]) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyReport(RTIambassador observer, Recorder observerCallbacks,
         CallbackModel callbackModel, EncoderFactory encoder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         TransportationTypeHandle reliable, int serialNumber, String expectedService,
         List<ExpectedArgument> expectedArguments) throws Exception {
      await(observer, callbackModel, () -> !observerCallbacks.interactions.isEmpty());
      Interaction report = observerCallbacks.lastInteraction();
      JavaTckSupport.check(reportClass.equals(report.interaction)
            && report.parameters.size() == reportParameters.size()
            && report.producer == null && report.tag.length == 0 && report.regionsEmpty
            && reliable.equals(report.transportation),
         "regional subscription service report had incorrect callback metadata");
      for (ParameterHandle handle : reportParameters.values()) {
         JavaTckSupport.check(report.parameters.containsKey(handle),
            "regional subscription service report omitted a standard parameter");
      }

      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(reportParameters.get("HLAservice")));
      JavaTckSupport.check(expectedService.equals(service.getValue()),
         "regional subscription service report named the wrong service");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(reportParameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 5,
         "regional subscription service report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue(),
         "regional subscription service report did not report success");

      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), fixedRecordPrototype(encoder));
      suppliedArguments.decode(report.parameters.get(reportParameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(suppliedArguments.size() == expectedArguments.size(),
         "regional subscription report returned the wrong supplied-argument count");
      for (int index = 0; index < expectedArguments.size(); index++) {
         HLAfixedRecord argument = suppliedArguments.get(index);
         ExpectedArgument expected = expectedArguments.get(index);
         JavaTckSupport.check(argument.size() == 3
               && ((HLAinteger32BE) argument.get(0)).getValue() == expected.type
               && expected.name.equals(((HLAunicodeString) argument.get(1)).getValue())
               && expected.value.equals(((HLAunicodeString) argument.get(2)).getValue()),
            "regional subscription service report returned an incorrect argument at index "
               + index);
      }

      HLAfixedRecord returnedArgument = fixedRecordPrototype(encoder);
      returnedArgument.decode(report.parameters.get(reportParameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(returnedArgument.size() == 3
            && ((HLAinteger32BE) returnedArgument.get(0)).getValue() == 34
            && ((HLAunicodeString) returnedArgument.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returnedArgument.get(2)).getValue()),
         "regional subscription report returned the wrong Null return value");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(reportParameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().isEmpty(),
         "successful regional subscription service report carried an exception");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == serialNumber,
         "regional subscription service report returned the wrong serial number");
      observerCallbacks.interactions.clear();
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static String quote(String value) { return "\"" + value + "\""; }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for regional subscription service-report callback");
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static final class ExpectedArgument {
      private final int type;
      private final String name;
      private final String value;
      private ExpectedArgument(int type, String name, String value) {
         this.type = type;
         this.name = name;
         this.value = value;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
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
         regions = callbackRegions == null ? null : (RegionHandleSet) callbackRegions.clone();
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
                  if ("toString".equals(name)) return "Java TCK regional subscription reports";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
      private Interaction lastInteraction() { return interactions.get(interactions.size() - 1); }
   }
}
