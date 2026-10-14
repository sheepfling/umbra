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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies a successful ownership-acquisition cancellation report and callback using MOM. */
final class ServiceReportCancelOwnershipAcquisitionTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportCancelOwnershipAcquisitionTck() { }

   static void run(RtiFactory factory, String fom, String mim, String objectClassName,
         String attributeName, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-cancel-acquisition-" + suffix;
      String ownerName = "java-tck-cancel-acquisition-owner-" + suffix;
      String requesterName = "java-tck-cancel-acquisition-requester-" + suffix;
      String observerName = "java-tck-cancel-acquisition-observer-" + suffix;
      boolean ownerConnected = false, requesterConnected = false, observerConnected = false;
      boolean created = false, ownerJoined = false, requesterJoined = false, observerJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      ObjectInstanceHandle object = null;
      AttributeHandle requesterAttribute = null;
      AttributeHandleSet requesterAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            requesterName, "java-tck", federation);
         requesterJoined = true;
         owner.joinFederationExecution(ownerName, "java-tck", federation);
         ownerJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         owner.setServiceReportingSwitch(false);
         owner.setSendServiceReportsToFileSwitch(false);
         requester.setServiceReportingSwitch(false);
         requester.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null,
            "adapter-supplied ownership object class/attribute lookup failed");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle registered = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(registered != null,
            "ownership test registration returned a null object handle");
         await(requester, requesterRecorder, callbackModel,
            () -> requesterRecorder.discovery != null);
         object = requesterRecorder.discovery.object;
         JavaTckSupport.check(registered.equals(object),
            "ownership test discovery returned the wrong registered object");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);

         requester.attributeOwnershipAcquisition(object, requesterAttributes,
            new byte[] {(byte) 0xc0, (byte) 0xde, 0x25});
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(observerRecorder.interactions.isEmpty(),
            "disabled service reporting exposed setup acquisition through MOM");
         requester.setServiceReportingSwitch(true);
         requester.cancelAttributeOwnershipAcquisition(object, requesterAttributes);
         await(observer, observerRecorder, callbackModel,
            () -> !observerRecorder.interactions.isEmpty());
         await(requester, requesterRecorder, callbackModel,
            () -> !requesterRecorder.cancellations.isEmpty());

         JavaTckSupport.check(observerRecorder.interactions.size() == 1,
            "cancellation produced an unexpected service-report count");
         verifyReport(observer, requesterFederate, observerRecorder.interactions.get(0), encoder,
            reportClass, reportParameters, reliable, object, requesterAttribute);
         JavaTckSupport.check(requesterRecorder.cancellations.size() == 1
               && object.equals(requesterRecorder.cancellations.get(0).object)
               && requesterRecorder.cancellations.get(0).attributes.contains(requesterAttribute),
            "ownership cancellation callback returned the wrong object or attribute set");

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         requester.setServiceReportingSwitch(false);
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { requester.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (ownerJoined) resign(owner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (requesterJoined) resign(requester, ResignAction.NO_ACTION);
         if (observerJoined) resign(observer, ResignAction.NO_ACTION);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (requesterConnected) disconnect(requester);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         ObjectInstanceHandle object, AttributeHandle attribute) throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length && event.tag.length == 0
            && event.producer == null && event.regionsEmpty && reliable.equals(event.transport),
         "cancellation report had incorrect standard interaction metadata");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            "cancellation report omitted standard parameter " + name);
      }
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(event.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(event.parameters.get(parameters.get("HLAserialNumber")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check("CancelAttributeOwnershipAcquisition".equals(service.getValue())
            && serviceType.getValue() == 3 && success.getValue() && serial.getValue() == 0
            && exception.getValue().isEmpty(),
         "cancellation report returned incorrect service, status, serial, or exception");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(expectedFederate.equals(reportedFederate),
         "cancellation report identified the wrong invoking federate");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 2,
         "cancellation report returned the wrong supplied-argument count");
      verifyArgument(supplied.get(0), 37, "Object instance designator",
         quoted(object.toString()), "cancellation object argument");
      verifyArgument(supplied.get(1), 1, "Set of attribute designators",
         "[" + quoted(attribute.toString()) + "]", "cancellation attribute-set argument");
      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      verifyArgument(returned, 34, "", "null", "cancellation return value");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static void verifyArgument(HLAfixedRecord record, int type, String name,
         String value, String description) throws Exception {
      JavaTckSupport.check(record.size() == 3
            && ((HLAinteger32BE) record.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) record.get(1)).getValue())
            && value.equals(((HLAunicodeString) record.get(2)).getValue()),
         description + " had the wrong type, name, or value");
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static void await(RTIambassador rti, Recorder recorder, CallbackModel model,
         Ready ready) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!ready.get() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(ready.get(), "timed out waiting for ownership cancellation callback");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK ownership-cancellation callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == arguments[0];
      return null;
   }

   private static void resign(RTIambassador rti, ResignAction action) {
      try { rti.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Ready { boolean get(); }

   private static final class Cancellation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private Cancellation(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = attributes;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transport;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction,
            Map<ParameterHandle, byte[]> parameters, byte[] tag,
            TransportationTypeHandle transport, FederateHandle producer, boolean regionsEmpty) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag;
         this.transport = transport;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private Discovery(ObjectInstanceHandle object) { this.object = object; }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<Cancellation> cancellations = new CopyOnWriteArrayList<>();
      private volatile Discovery discovery;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length == 4) {
                  discovery = new Discovery((ObjectInstanceHandle) arguments[0]);
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(
                     method.getName()) && arguments != null && arguments.length == 2) {
                  cancellations.add(new Cancellation((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  Map<ParameterHandle, byte[]> values = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     values.put(handle, source.get(handle).clone());
                  }
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0], values,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], regions == null || regions.isEmpty()));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
