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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Requests one joined federate's FOM module through the standard federate MOM. */
final class FederateMomFomModuleContentReportTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData";
   private static final String REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData";

   private FederateMomFomModuleContentReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String additionalFom,
         String timeImplementation, CallbackModel callbackModel, String federateType)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-fom-module-" + UUID.randomUUID();
      String subjectName = "java-tck-fom-module-subject-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false, subscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         requester.connect(recorder.proxy(), callbackModel);
         requesterConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, federateType, federation,
            new String[] {additionalFom});
         subjectJoined = true;
         requester.joinFederationExecution("java-tck-fom-module-requester", federateType,
            federation);
         requesterJoined = true;

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         ParameterHandle requestIndicator = requester.getParameterHandle(
            requestClass, "HLAFOMmoduleIndicator");
         reportClass = requester.getInteractionClassHandle(REPORT);
         ParameterHandle reportFederate = requester.getParameterHandle(reportClass, "HLAfederate");
         ParameterHandle reportIndicator = requester.getParameterHandle(
            reportClass, "HLAFOMmoduleIndicator");
         ParameterHandle reportData = requester.getParameterHandle(reportClass, "HLAFOMmoduleData");
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         FederateHandle targetFederate = requester.getFederateHandle(subjectName);
         JavaTckSupport.check(requestClass != null && requestFederate != null
               && requestIndicator != null && reportClass != null && reportFederate != null
               && reportIndicator != null && reportData != null && reliable != null
               && targetFederate != null,
            "standard federate MOM FOM-module lookup returned a null handle");
         JavaTckSupport.check(REQUEST.equals(requester.getInteractionClassName(requestClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && "HLAFOMmoduleIndicator".equals(
                  requester.getParameterName(requestClass, requestIndicator))
               && REPORT.equals(requester.getInteractionClassName(reportClass))
               && "HLAfederate".equals(requester.getParameterName(reportClass, reportFederate))
               && "HLAFOMmoduleIndicator".equals(
                  requester.getParameterName(reportClass, reportIndicator))
               && "HLAFOMmoduleData".equals(requester.getParameterName(reportClass, reportData))
               && "HLAreliable".equals(requester.getTransportationTypeName(reliable)),
            "standard federate MOM FOM-module handles did not round-trip");

         requester.subscribeInteractionClass(reportClass);
         subscribed = true;
         ParameterHandleValueMap request = requester.getParameterHandleValueMapFactory().create(2);
         request.put(requestFederate, encode(targetFederate));
         HLAinteger32BE indicator = encoder.createHLAinteger32BE(0);
         request.put(requestIndicator, indicator.toByteArray());
         requester.sendInteraction(requestClass, request, new byte[0]);

         await(requester, callbackModel, () -> !recorder.events.isEmpty(),
            "federate-scoped FOM-module content report");
         JavaTckSupport.check(recorder.events.size() == 1,
            "one federate-scoped FOM-module request did not deliver exactly one report");
         Interaction report = recorder.events.get(0);
         JavaTckSupport.check(report.interaction.equals(reportClass)
               && reliable.equals(report.transportation) && report.tag.length == 0
               && report.regionsEmpty,
            "federate FOM-module report did not preserve class, reliable delivery, and empty metadata");
         boolean targetIncluded = report.parameters.containsKey(reportFederate);
         JavaTckSupport.check(report.parameters.containsKey(reportIndicator)
               && report.parameters.containsKey(reportData)
               && report.parameters.size() == (targetIncluded ? 3 : 2),
            "federate FOM-module report omitted a declared parameter or included unknown values");
         if (targetIncluded) {
            JavaTckSupport.check(java.util.Arrays.equals(
                  encode(targetFederate), report.parameters.get(reportFederate)),
               "federate FOM-module report identified a different joined federate");
         }
         HLAinteger32BE decodedIndicator = encoder.createHLAinteger32BE();
         decodedIndicator.decode(report.parameters.get(reportIndicator));
         HLAunicodeString decodedModule = encoder.createHLAunicodeString();
         decodedModule.decode(report.parameters.get(reportData));
         JavaTckSupport.check(decodedIndicator.getValue() == 0
               && !decodedModule.getValue().isEmpty()
               && decodedModule.getValue().contains("<objectModel"),
            "federate FOM-module report did not return module zero's standard XML content");
      } finally {
         if (subscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (subjectJoined) try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
         if (subjectConnected) try { subject.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> objectMethod(proxy, method.getName(), args));
   }

   private static Object objectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK federate FOM-module subject callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
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
      private final List<Interaction> events = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("receiveInteraction".equals(method.getName())
                     && args != null && args.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) args[0];
                  event.parameters = ((ParameterHandleValueMap) args[1]).clone();
                  event.tag = ((byte[]) args[2]).clone();
                  event.transportation = (TransportationTypeHandle) args[3];
                  RegionHandleSet regions = (RegionHandleSet) args[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  events.add(event);
               }
               return objectMethod(proxy, method.getName(), args);
            });
      }
   }
}
