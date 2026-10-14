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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies standard service reports for failing regional interaction sends. */
final class ServiceReportRegionalInteractionFailureTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportRegionalInteractionFailureTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied dimensional FOM is required");
      JavaTckSupport.check(mim != null && !mim.isEmpty(),
         "adapter-supplied standard MIM is required");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      Recorder observerCallbacks = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-regional-interaction-failure-"
         + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean created = false;
      boolean[] joined = new boolean[3];
      boolean published = false;
      boolean receiverSubscribed = false;
      boolean reportSubscribed = false;
      RegionHandle sourceRegion = null;
      RegionHandle receiverRegion = null;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      InteractionClassHandle reportClass = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         connected[0] = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         connected[1] = true;
         observer.connect(observerCallbacks.proxy(), callbackModel);
         connected[2] = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         publisher.joinFederationExecution("regional-failure-publisher", "java-tck", federation);
         joined[0] = true;
         receiver.joinFederationExecution("regional-failure-receiver", "java-tck", federation);
         joined[1] = true;
         observer.joinFederationExecution("regional-failure-observer", "java-tck", federation);
         joined[2] = true;

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.setServiceReportingSwitch(false);
         receiver.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter)
               && publisher.getInteractionClassName(publisherInteraction).equals(interactionName)
               && publisher.getParameterName(publisherInteraction, publisherParameter)
                  .equals(parameterName),
            "adapter FOM interaction/parameter declarations did not round-trip");
         JavaTckSupport.check(publisher.getAvailableDimensionsForInteractionClass(
                  publisherInteraction).contains(publisherFirst)
               && publisher.getAvailableDimensionsForInteractionClass(publisherInteraction)
                  .contains(publisherSecond)
               && receiver.getAvailableDimensionsForInteractionClass(receiverInteraction)
                  .contains(receiverFirst)
               && receiver.getAvailableDimensionsForInteractionClass(receiverInteraction)
                  .contains(receiverSecond)
               && publisher.getDimensionUpperBound(publisherFirst) >= 6
               && publisher.getDimensionUpperBound(publisherSecond) >= 6
               && receiver.getDimensionUpperBound(receiverFirst) >= 6
               && receiver.getDimensionUpperBound(receiverSecond) >= 6,
            "adapter interaction class does not expose both configured routing dimensions");

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

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         RegionHandleSet sourceRegions = regionSet(publisher, sourceRegion);
         RegionHandleSet receiverRegions = regionSet(receiver, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "convey-region-designator-sets switch did not turn on");
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         receiverSubscribed = true;

         publisher.setServiceReportingSwitch(true);
         publisher.setSendServiceReportsToFileSwitch(false);
         byte[] value = new byte[] {0x01, 0x02};
         byte[] tag = new byte[] {0x46, 0x41, 0x49};
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, value.clone());
         InteractionClassHandle invalidInteraction =
            JavaTckSupport.invalidHandle(InteractionClassHandle.class);
         expectReportedFailure(publisher, receiver, observer, receiverCallbacks,
            observerCallbacks, callbackModel, encoder, reportClass, reportParameters, reliable,
            invalidInteraction, values, sourceRegions, tag, "InteractionClassNotDefined", 0,
            "invalid interaction-class regional send");

         ParameterHandle invalidParameter = JavaTckSupport.invalidHandle(ParameterHandle.class);
         ParameterHandleValueMap invalidValues =
            publisher.getParameterHandleValueMapFactory().create(1);
         invalidValues.put(invalidParameter, value.clone());
         expectReportedFailure(publisher, receiver, observer, receiverCallbacks,
            observerCallbacks, callbackModel, encoder, reportClass, reportParameters, reliable,
            publisherInteraction, invalidValues, sourceRegions, tag,
            "InteractionParameterNotDefined", 1, "invalid-parameter regional send");

         RegionHandleSet invalidRegions = publisher.getRegionHandleSetFactory().create();
         invalidRegions.add(JavaTckSupport.invalidHandle(RegionHandle.class));
         expectReportedFailure(publisher, receiver, observer, receiverCallbacks,
            observerCallbacks, callbackModel, encoder, reportClass, reportParameters, reliable,
            publisherInteraction, values, invalidRegions, tag, "InvalidRegion", 2,
            "invalid-region regional send");

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         receiverSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         receiver.deleteRegion(receiverRegion);
         receiverRegion = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         joined[0] = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[0]) {
            try { publisher.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { publisher.setSendServiceReportsToFileSwitch(false); } catch (Exception ignored) { }
         }
         if (reportSubscribed && joined[2]) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (receiverSubscribed && joined[1] && receiverRegion != null) {
            try { receiver.unsubscribeInteractionClassWithRegions(receiverInteraction,
               regionSet(receiver, receiverRegion)); } catch (Exception ignored) { }
         }
         if (published && joined[0]) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverRegion != null && joined[1]) {
            try { receiver.deleteRegion(receiverRegion); } catch (Exception ignored) { }
         }
         if (sourceRegion != null && joined[0]) {
            try { publisher.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (joined[2]) resign(observer);
         if (joined[1]) resign(receiver);
         if (joined[0]) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected[2]) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (connected[1]) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (connected[0]) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void expectReportedFailure(RTIambassador publisher, RTIambassador receiver,
         RTIambassador observer, Recorder receiverCallbacks, Recorder observerCallbacks,
         CallbackModel callbackModel, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> reportParameters, TransportationTypeHandle reliable,
         InteractionClassHandle interaction, ParameterHandleValueMap values,
         RegionHandleSet regions, byte[] tag, String exceptionName, int serialNumber,
         String description) throws Exception {
      JavaTckSupport.expectFailure(
         () -> publisher.sendInteractionWithRegions(interaction, values, regions, tag.clone()),
         exceptionName, description);
      await(observer, callbackModel, () -> !observerCallbacks.interactions.isEmpty(),
         description + " HLAreportServiceInvocation callback");
      JavaTckSupport.check(observerCallbacks.interactions.size() == 1,
         description + " produced more than one service report");
      verifyFailureReport(observerCallbacks.lastInteraction(), reportClass, reportParameters,
         encoder, reliable, exceptionName, serialNumber, description);
      observerCallbacks.interactions.clear();
      settle(receiver, callbackModel);
      JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
         description + " delivered an application interaction");
   }

   private static void verifyFailureReport(Interaction report,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         EncoderFactory encoder, TransportationTypeHandle reliable, String expectedException,
         int expectedSerial, String description) throws Exception {
      JavaTckSupport.check(reportClass.equals(report.interaction)
            && report.parameters.size() == reportParameters.size()
            && report.producer == null && report.tag.length == 0 && report.regionsEmpty
            && reliable.equals(report.transportation),
         description + " report returned incorrect identity or callback metadata");
      for (ParameterHandle handle : reportParameters.values()) {
         JavaTckSupport.check(report.parameters.containsKey(handle),
            description + " report omitted a standard parameter");
      }
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(reportParameters.get("HLAservice")));
      JavaTckSupport.check("SendInteractionWithRegions".equals(service.getValue()),
         description + " report named the wrong service");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(reportParameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 2,
         description + " report returned the wrong interaction service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(!success.getValue(), description + " report incorrectly reported success");

      HLAvariableArray<HLAfixedRecord> arguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), fixedRecordPrototype(encoder));
      arguments.decode(report.parameters.get(reportParameters.get("HLAsuppliedArguments")));
      String[] names = {
         "Interaction class designator",
         "Constrained set of interaction parameter designator and value pairs",
         "Set of region designators", "User-supplied tag", "Optional timestamp"
      };
      int[] types = {27, 40, 43, 60, 34};
      JavaTckSupport.check(arguments.size() == names.length,
         description + " report omitted standard supplied arguments");
      for (int index = 0; index < names.length; index++) {
         HLAfixedRecord argument = arguments.get(index);
         JavaTckSupport.check(argument.size() == 3
               && ((HLAinteger32BE) argument.get(0)).getValue() == types[index]
               && names[index].equals(((HLAunicodeString) argument.get(1)).getValue()),
            description + " report returned the wrong supplied-argument shape at " + index);
      }

      HLAfixedRecord returned = fixedRecordPrototype(encoder);
      returned.decode(report.parameters.get(reportParameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(returned.size() == 3
            && ((HLAinteger32BE) returned.get(0)).getValue() == 34
            && ((HLAunicodeString) returned.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returned.get(2)).getValue()),
         description + " report returned the wrong Null return value");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(reportParameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().contains(expectedException),
         description + " report named the wrong exception");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == expectedSerial,
         description + " report returned the wrong serial number");
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else for (int index = 0; index < 64; index++) Thread.yield();
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
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
                  if ("toString".equals(name)) return "Java TCK regional service-report failures";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
      private Interaction lastInteraction() { return interactions.get(interactions.size() - 1); }
   }
}
