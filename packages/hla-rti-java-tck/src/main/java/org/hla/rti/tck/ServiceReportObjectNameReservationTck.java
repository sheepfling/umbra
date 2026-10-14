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
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.LinkedHashSet;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies MOM reporting and success callback for an object-instance name reservation. */
final class ServiceReportObjectNameReservationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportObjectNameReservationTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel, false);
   }

   static void runRelease(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel, true);
   }

   static void runMultipleRelease(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel, true, true);
   }

   private static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, boolean release) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel, release, false);
   }

   private static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, boolean release, boolean multiple) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-reserve-name-" + UUID.randomUUID();
      boolean ownerConnected = false, observerConnected = false, created = false;
      boolean ownerJoined = false, observerJoined = false, reportSubscribed = false;
      String objectName = federation + "-reserved-name";
      Set<String> objectNames = new LinkedHashSet<>();
      objectNames.add(federation + "-reserved-a");
      objectNames.add(federation + "-reserved-b");
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-reserve-name-owner", "java-tck", federation);
         ownerJoined = true;
         observer.joinFederationExecution(
            "java-tck-reserve-name-observer", "java-tck", federation);
         observerJoined = true;

         InteractionClassHandle reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
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

         owner.setServiceReportingSwitch(false);
         owner.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         ownerRecorder.reservations.clear();
         observerRecorder.interactions.clear();

         if (release) {
            if (multiple) owner.reserveMultipleObjectInstanceNames(objectNames);
            else owner.reserveObjectInstanceName(objectName);
            await(owner, callbackModel, () -> multiple
                  ? ownerRecorder.multipleReservations.contains(objectNames)
                  : ownerRecorder.reservations.contains(objectName),
               "reservation setup callback before name release");
            ownerRecorder.reservations.clear();
            ownerRecorder.multipleReservations.clear();
         }
         owner.setServiceReportingSwitch(true);
         if (release && multiple) owner.releaseMultipleObjectInstanceNames(objectNames);
         else if (release) owner.releaseObjectInstanceName(objectName);
         else owner.reserveObjectInstanceName(objectName);
         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            (release ? "Release" : "Reserve")
               + "ObjectInstanceName HLAreportServiceInvocation callback");
         if (!release) {
            await(owner, callbackModel, () -> ownerRecorder.reservations.contains(objectName),
               "objectInstanceNameReservationSucceeded callback");
            JavaTckSupport.check(ownerRecorder.reservations.size() == 1
                  && objectName.equals(ownerRecorder.reservations.get(0)),
               "object-name reservation success callback returned the wrong reservation");
         }
         JavaTckSupport.check(observerRecorder.interactions.size() == 1,
            "object-name reservation produced an unexpected report count");
         Interaction event = observerRecorder.interactions.get(0);
         JavaTckSupport.check(reportClass.equals(event.interaction)
               && event.parameters.size() == REPORT_PARAMETER_NAMES.length
               && event.tag.length == 0 && event.producer == null && event.regionsEmpty
               && reliable.equals(event.transportation),
            "object-name reservation report had invalid identity or callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
               "object-name reservation report omitted standard parameter " + name);
         }
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            event.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(ownerHandle.equals(reportedFederate),
            "object-name reservation report identified the wrong invoking federate");
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
         String expectedService = !release ? "ReserveObjectInstanceName"
            : multiple ? "ReleaseMultipleObjectInstanceNames" : "ReleaseObjectInstanceName";
         JavaTckSupport.check(expectedService
               .equals(service.getValue())
               && serviceType.getValue() == 2 && success.getValue()
               && exception.getValue().isEmpty() && serial.getValue() == 0,
            "object-name reservation report returned an incorrect service result");
      } finally {
         if (ownerJoined) {
            try { owner.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            if (multiple) {
               try { owner.releaseMultipleObjectInstanceNames(objectNames); }
               catch (Exception ignored) { }
            } else {
               try { owner.releaseObjectInstanceName(objectName); } catch (Exception ignored) { }
            }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(
               observer.getInteractionClassHandle(REPORT_CLASS_NAME)); }
            catch (Exception ignored) { }
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
      private final List<String> reservations = new CopyOnWriteArrayList<>();
      private final List<Set<String>> multipleReservations = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("objectInstanceNameReservationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1) {
                  reservations.add((String) arguments[0]);
               }
               if ("multipleObjectInstanceNameReservationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1) {
                  Set<String> reservedNames = new LinkedHashSet<>();
                  for (Object reservedName : (Set<?>) arguments[0]) {
                     if (reservedName instanceof String) reservedNames.add((String) reservedName);
                  }
                  multipleReservations.add(Set.copyOf(reservedNames));
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
                  if ("toString".equals(name)) return "Java TCK name-reservation report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
