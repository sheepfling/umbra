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
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies regional interaction delivery and its standard service-invocation report. */
final class ServiceReportRegionalInteractionTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportRegionalInteractionTck() { }

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
      String federation = "java-tck-service-report-regional-interaction-" + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean created = false;
      boolean[] joined = new boolean[3];
      boolean reportSubscribed = false;
      boolean regionalSubscribed = false;
      boolean published = false;
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
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "regional-report-publisher", "java-tck", federation);
         joined[0] = true;
         receiver.joinFederationExecution("regional-report-receiver", "java-tck", federation);
         joined[1] = true;
         observer.joinFederationExecution("regional-report-observer", "java-tck", federation);
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
               && receiver.getInteractionClassName(receiverInteraction).equals(interactionName)
               && publisher.getParameterName(publisherInteraction, publisherParameter)
                  .equals(parameterName)
               && receiver.getParameterName(receiverInteraction, receiverParameter)
                  .equals(parameterName),
            "adapter FOM interaction/parameter declarations did not round-trip");
         JavaTckSupport.check(publisherFirst.equals(receiverFirst)
               && publisherSecond.equals(receiverSecond)
               && publisher.getAvailableDimensionsForInteractionClass(publisherInteraction)
                  .contains(publisherFirst)
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
            "adapter interaction class does not expose the configured dimensional routing space");

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle handle = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(handle != null
                  && name.equals(observer.getParameterName(reportClass, handle)),
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
         regionalSubscribed = true;

         publisher.setServiceReportingSwitch(true);
         publisher.setSendServiceReportsToFileSwitch(false);
         publisherCallbacks.interactions.clear();
         receiverCallbacks.interactions.clear();
         observerCallbacks.interactions.clear();
         byte[] value = new byte[] {0x52, 0x45, 0x47};
         byte[] tag = new byte[] {0x52, 0x53, 0x52};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, value.clone());
         publisher.sendInteractionWithRegions(
            publisherInteraction, values, sourceRegions, tag.clone());
         awaitBoth(receiver, observer, callbackModel,
            () -> !receiverCallbacks.interactions.isEmpty()
               && !observerCallbacks.interactions.isEmpty());

         Interaction report = observerCallbacks.lastInteraction();
         JavaTckSupport.check(reportClass.equals(report.interaction)
               && report.parameters.size() == reportParameters.size()
               && report.producer == null && report.tag.length == 0 && report.regionsEmpty
               && reliable.equals(report.transportation),
            "regional service report had incorrect identity, producer, tag, region, or transport metadata");
         for (ParameterHandle handle : reportParameters.values()) {
            JavaTckSupport.check(report.parameters.containsKey(handle),
               "regional service report omitted a standard parameter");
         }
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(report.parameters.get(reportParameters.get("HLAservice")));
         JavaTckSupport.check("SendInteractionWithRegions".equals(service.getValue()),
            "regional service report named the wrong service");
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(report.parameters.get(reportParameters.get("HLAserviceType")));
         JavaTckSupport.check(serviceType.getValue() == 2,
            "regional service report returned the wrong interaction service type");
         HLAboolean success = encoder.createHLAboolean();
         success.decode(report.parameters.get(reportParameters.get("HLAsuccessIndicator")));
         JavaTckSupport.check(success.getValue(),
            "regional service report did not report a successful invocation");
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(report.parameters.get(reportParameters.get("HLAexception")));
         JavaTckSupport.check(exception.getValue().isEmpty(),
            "successful regional service report carried an exception");
         HLAinteger32BE serial = encoder.createHLAinteger32BE();
         serial.decode(report.parameters.get(reportParameters.get("HLAserialNumber")));
         JavaTckSupport.check(serial.getValue() == 0,
            "first regional service report did not use serial number zero");

         Interaction delivery = receiverCallbacks.lastInteraction();
         JavaTckSupport.check(receiverInteraction.equals(delivery.interaction)
               && delivery.parameters.size() == 1
               && Arrays.equals(value, delivery.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivery.tag)
               && publisherFederate.equals(delivery.producer)
               && delivery.transportation != null
               && !receiver.getTransportationTypeName(delivery.transportation).isEmpty()
               && delivery.regions != null && delivery.regions.size() == 1
               && delivery.regions.contains(sourceRegion),
            "regional application callback did not preserve payload and source-region metadata");

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         regionalSubscribed = false;
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
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[0]) {
            try { publisher.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { publisher.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
         }
         if (reportSubscribed && joined[2]) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (regionalSubscribed && joined[1]) {
            try {
               receiver.unsubscribeInteractionClassWithRegions(
                  receiverInteraction, regionSet(receiver, receiverRegion));
            } catch (Exception ignored) { }
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
         if (joined[0]) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = 2; index >= 0; index--) {
            if (connected[index]) {
               try { ambassador(index, publisher, receiver, observer).disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
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
      RegionHandleSet singleton = regionSet(ambassador, region);
      ambassador.commitRegionModifications(singleton);
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static RTIambassador ambassador(int index, RTIambassador publisher,
         RTIambassador receiver, RTIambassador observer) {
      return index == 0 ? publisher : index == 1 ? receiver : observer;
   }

   private static void awaitBoth(RTIambassador receiver, RTIambassador observer,
         CallbackModel callbackModel, java.util.function.BooleanSupplier condition)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            receiver.evokeCallback(0.01);
            observer.evokeCallback(0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for regional application and service-report callbacks");
   }

   private static void resign(RTIambassador ambassador) {
      resign(ambassador, ResignAction.NO_ACTION);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
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
         if (callbackRegions == null) {
            regions = null;
            regionsEmpty = true;
         } else {
            regions = (RegionHandleSet) callbackRegions.clone();
            regionsEmpty = callbackRegions.isEmpty();
         }
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
                     && arguments.length == 6) {
                  interactions.add(new Interaction(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional service-report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private Interaction lastInteraction() {
         return interactions.get(interactions.size() - 1);
      }
   }
}
