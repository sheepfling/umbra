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
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Checks standard federation MOM FOM-module and MIM content reports. */
final class FederationMomContentReportsTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST_ROOT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.";
   private static final String REPORT_ROOT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.";

   private FederationMomContentReportsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-content-reports-" + UUID.randomUUID();
      boolean ownerConnected = false, observerConnected = false, created = false;
      boolean ownerJoined = false, observerJoined = false;
      boolean fomSubscribed = false, mimSubscribed = false;
      InteractionClassHandle fomReport = null, mimReport = null;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-content-report-owner", "tck-type", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-content-report-observer", "tck-type",
            federation);
         observerJoined = true;

         InteractionClassHandle fomRequest = observer.getInteractionClassHandle(
            REQUEST_ROOT + "HLArequestFOMmoduleData");
         ParameterHandle fomIndicator = observer.getParameterHandle(
            fomRequest, "HLAFOMmoduleIndicator");
         InteractionClassHandle mimRequest = observer.getInteractionClassHandle(
            REQUEST_ROOT + "HLArequestMIMdata");
         fomReport = observer.getInteractionClassHandle(REPORT_ROOT + "HLAreportFOMmoduleData");
         ParameterHandle reportIndicator = observer.getParameterHandle(
            fomReport, "HLAFOMmoduleIndicator");
         ParameterHandle fomData = observer.getParameterHandle(fomReport, "HLAFOMmoduleData");
         mimReport = observer.getInteractionClassHandle(REPORT_ROOT + "HLAreportMIMdata");
         ParameterHandle mimData = observer.getParameterHandle(mimReport, "HLAMIMdata");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(fomRequest != null && fomIndicator != null && mimRequest != null
               && fomReport != null && reportIndicator != null && fomData != null
               && mimReport != null && mimData != null && reliable != null,
            "standard federation MOM content-report lookup returned a null handle");
         JavaTckSupport.check("HLArequestFOMmoduleData".equals(
               observer.getInteractionClassName(fomRequest))
               && "HLAFOMmoduleIndicator".equals(observer.getParameterName(fomRequest, fomIndicator))
               && "HLArequestMIMdata".equals(observer.getInteractionClassName(mimRequest))
               && "HLAreportFOMmoduleData".equals(observer.getInteractionClassName(fomReport))
               && "HLAFOMmoduleData".equals(observer.getParameterName(fomReport, fomData))
               && "HLAreportMIMdata".equals(observer.getInteractionClassName(mimReport))
               && "HLAMIMdata".equals(observer.getParameterName(mimReport, mimData))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard federation MOM content-report handle names did not round-trip");

         observer.subscribeInteractionClass(fomReport);
         fomSubscribed = true;
         observer.subscribeInteractionClass(mimReport);
         mimSubscribed = true;

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            sendFomRequest(observer, fomRequest, fomIndicator, encoder);
            observer.unsubscribeInteractionClass(fomReport);
            fomSubscribed = false;
            settle(observer, callbackModel);
            JavaTckSupport.check(recorder.events.isEmpty(),
               "unsubscribed FOM-module report was delivered while callbacks were serviced");
            observer.subscribeInteractionClass(fomReport);
            fomSubscribed = true;
         } else {
            sendFomRequest(observer, fomRequest, fomIndicator, encoder);
            awaitCount(observer, recorder, 1, "initial FOM-module report delivery");
            assertReport(recorder.events.get(0), fomReport, reliable,
               "initial FOM-module report");
            observer.unsubscribeInteractionClass(fomReport);
            fomSubscribed = false;
            observer.subscribeInteractionClass(fomReport);
            fomSubscribed = true;
            recorder.events.clear();
         }

         sendFomRequest(observer, fomRequest, fomIndicator, encoder);
         awaitCount(observer, recorder, 1, "FOM-module content report delivery");
         Interaction fomEvent = recorder.events.get(0);
         assertReport(fomEvent, fomReport, reliable, "FOM-module content report");
         JavaTckSupport.check(fomEvent.parameters.size() == 2
               && fomEvent.parameters.containsKey(reportIndicator)
               && fomEvent.parameters.containsKey(fomData),
            "FOM-module report did not contain exactly its two standard parameters");
         HLAinteger32BE decodedIndicator = encoder.createHLAinteger32BE();
         decodedIndicator.decode(fomEvent.parameters.get(reportIndicator));
         HLAunicodeString decodedFom = encoder.createHLAunicodeString();
         decodedFom.decode(fomEvent.parameters.get(fomData));
         JavaTckSupport.check(decodedIndicator.getValue() == 0
               && !decodedFom.getValue().isEmpty()
               && decodedFom.getValue().contains("objectModel"),
            "FOM-module report did not contain module zero's standard XML content");

         recorder.events.clear();
         sendRequest(observer, mimRequest, null, null);
         awaitCount(observer, recorder, 1, "MIM content report delivery");
         Interaction mimEvent = recorder.events.get(0);
         assertReport(mimEvent, mimReport, reliable, "MIM content report");
         JavaTckSupport.check(mimEvent.parameters.size() == 1
               && mimEvent.parameters.containsKey(mimData),
            "MIM report did not contain exactly its standard content parameter");
         HLAunicodeString decodedMim = encoder.createHLAunicodeString();
         decodedMim.decode(mimEvent.parameters.get(mimData));
         JavaTckSupport.check(!decodedMim.getValue().isEmpty()
               && decodedMim.getValue().contains("HLArequestMIMdata")
               && decodedMim.getValue().contains("HLAreportMIMdata"),
            "MIM report did not contain standard request and report interaction definitions");
         JavaTckSupport.check(producer != null,
            "standard report request producer handle was null");
      } finally {
         if (mimSubscribed && observerJoined) {
            try { observer.unsubscribeInteractionClass(mimReport); } catch (Exception ignored) { }
         }
         if (fomSubscribed && observerJoined) {
            try { observer.unsubscribeInteractionClass(fomReport); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void sendFomRequest(RTIambassador ambassador,
         InteractionClassHandle request, ParameterHandle indicator, EncoderFactory encoder)
         throws Exception {
      HLAinteger32BE index = encoder.createHLAinteger32BE(0);
      sendRequest(ambassador, request, indicator, index.toByteArray());
   }

   private static void sendRequest(RTIambassador ambassador, InteractionClassHandle request,
         ParameterHandle parameter, byte[] value) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(
         parameter == null ? 0 : 1);
      if (parameter != null) values.put(parameter, value);
      ambassador.sendInteraction(request, values, new byte[0]);
   }

   private static void assertReport(Interaction event, InteractionClassHandle interaction,
         TransportationTypeHandle reliable, String description) throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.tag.length == 0 && reliable.equals(event.transportation)
            && event.regionsEmpty,
         description + " did not preserve class, empty tag, reliable transport, and no regions");
   }

   private static void settle(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) JavaTckSupport.drain(ambassador);
      if (model == CallbackModel.HLA_IMMEDIATE) Thread.yield();
   }

   private static void awaitCount(RTIambassador ambassador, Recorder recorder, int count,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && recorder.events.size() < count) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.events.size() >= count,
         "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK content-report callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Interaction> events = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  events.add(event);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
