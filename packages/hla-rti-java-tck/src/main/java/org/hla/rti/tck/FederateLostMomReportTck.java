package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.time.LogicalTimeFactory;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

/** Verifies HLAreportFederateLost after an adapter-managed connection fault. */
final class FederateLostMomReportTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REPORT_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost";

   private FederateLostMomReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, String readyMarkerPath, String completionMarkerPath,
         long timeoutMillis) throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty()
            && completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "adapter connection-loss readiness and completion markers are required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");

      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      Recorder observerRecorder = new Recorder();
      AtomicReference<String> connectionLostDescription = new AtomicReference<>();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-lost-mom-" + UUID.randomUUID();
      String observerName = "java-tck-lost-mom-observer-" + UUID.randomUUID();
      String lostName = "java-tck-lost-mom-member-" + UUID.randomUUID();
      boolean observerConnected = false, lostConnected = false, created = false;
      boolean observerJoined = false, lostJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         lost.connect(connectionLostCallbacks(connectionLostDescription), callbackModel);
         lostConnected = true;
         observer.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;
         FederateHandle lostFederate = lost.joinFederationExecution(
            lostName, "java-tck", federation);
         lostJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_NAME);
         ParameterHandle federateParameter = observer.getParameterHandle(reportClass, "HLAfederate");
         ParameterHandle nameParameter = observer.getParameterHandle(reportClass, "HLAfederateName");
         ParameterHandle timeParameter = observer.getParameterHandle(reportClass, "HLAtimeStamp");
         ParameterHandle faultParameter = observer.getParameterHandle(reportClass, "HLAfaultDescription");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reportClass != null && federateParameter != null
               && nameParameter != null && timeParameter != null && faultParameter != null
               && reliable != null,
            "standard HLAreportFederateLost class, parameter, or transportation lookup failed");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         writeMarker(Path.of(readyMarkerPath), "ready\n");
         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while ((connectionLostDescription.get() == null
               || observerRecorder.report == null) && System.nanoTime() < deadline) {
            service(observer);
            service(lost);
            if (connectionLostDescription.get() == null || observerRecorder.report == null) {
               Thread.sleep(1L);
            }
         }
         JavaTckSupport.check(connectionLostDescription.get() != null
               && !connectionLostDescription.get().isEmpty(),
            "timed out waiting for a non-empty standard connectionLost callback");
         JavaTckSupport.check(observerRecorder.report != null,
            "timed out waiting for the subscribed HLAreportFederateLost interaction");

         Interaction report = observerRecorder.report;
         JavaTckSupport.check(reportClass.equals(report.interaction)
               && report.parameters.size() == 4
               && report.parameters.containsKey(federateParameter)
               && report.parameters.containsKey(nameParameter)
               && report.parameters.containsKey(timeParameter)
               && report.parameters.containsKey(faultParameter)
               && reliable.equals(report.transportation)
               && report.tag.length == 0 && report.regionsEmpty,
            "HLAreportFederateLost returned a non-standard interaction or parameter set");
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            report.parameters.get(federateParameter), 0);
         JavaTckSupport.check(lostFederate.equals(reportedFederate),
            "HLAreportFederateLost identified the wrong federate");
         HLAunicodeString reportedName = encoder.createHLAunicodeString();
         reportedName.decode(report.parameters.get(nameParameter));
         JavaTckSupport.check(lostName.equals(reportedName.getValue()),
            "HLAreportFederateLost returned the wrong federate name");
         Object reportedTime = observer.getTimeFactory().decodeTime(
            report.parameters.get(timeParameter), 0);
         JavaTckSupport.check(reportedTime != null && !reportedTime.toString().isEmpty(),
            "HLAreportFederateLost did not encode a decodable logical timestamp");
         HLAunicodeString reportedFault = encoder.createHLAunicodeString();
         reportedFault.decode(report.parameters.get(faultParameter));

         writeMarker(Path.of(completionMarkerPath), "callback-ok\n");
      } finally {
         if (lostConnected) try { lost.disconnect(); } catch (Exception ignored) { }
         if (observerJoined && reportSubscribed && reportClass != null) try {
            observer.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (observerJoined) try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { observer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void service(RTIambassador ambassador) {
      try { ambassador.evokeMultipleCallbacks(0.0, 0.05); }
      catch (Exception ignored) {
         // The provider may first surface transport EOF through callback servicing.
      }
   }

   private static FederateAmbassador connectionLostCallbacks(
         AtomicReference<String> description) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("connectionLost".equals(method.getName())
                  && arguments != null && arguments.length == 1) {
               description.set((String) arguments[0]);
            }
            return objectMethod(proxy, method.getName(), arguments);
         });
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK federate-lost callback";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static void writeMarker(Path marker, String value) throws Exception {
      Path parent = marker.toAbsolutePath().getParent();
      if (parent != null) Files.createDirectories(parent);
      Files.write(marker, value.getBytes(StandardCharsets.UTF_8));
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private volatile Interaction report;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
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
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  report = event;
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
