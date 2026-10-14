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
import java.util.Arrays;
import java.util.Base64;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies negotiated ownership transfer, confirmation, and its standard MOM report. */
final class ServiceReportConfirmDivestitureTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportConfirmDivestitureTck() { }

   static void run(RtiFactory factory, String fom, String mim, String objectClassName,
         String attributeName, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-confirm-divestiture-" + suffix;
      String ownerName = "java-tck-confirm-divestiture-owner-" + suffix;
      String acquirerName = "java-tck-confirm-divestiture-acquirer-" + suffix;
      String observerName = "java-tck-confirm-divestiture-observer-" + suffix;
      byte[] divestitureTag = {0x01, (byte) 0xd1};
      byte[] acquisitionTag = {0x02, (byte) 0xa2};
      byte[] confirmationTag = {0x00, (byte) 0xff, 0x10, (byte) 0xa5};
      boolean ownerConnected = false, acquirerConnected = false, observerConnected = false;
      boolean created = false, ownerJoined = false, acquirerJoined = false;
      boolean observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      ObjectInstanceHandle object = null;
      AttributeHandle ownerAttribute = null, acquirerAttribute = null;
      AttributeHandleSet ownerAttributes = null, acquirerAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), callbackModel);
         acquirerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            ownerName, "java-tck", federation);
         ownerJoined = true;
         acquirer.joinFederationExecution(acquirerName, "java-tck", federation);
         acquirerJoined = true;
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
         acquirer.setServiceReportingSwitch(false);
         acquirer.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         acquirerAttribute = acquirer.getAttributeHandle(acquirerClass, attributeName);
         JavaTckSupport.check(ownerClass != null && acquirerClass != null
               && ownerAttribute != null && acquirerAttribute != null
               && ownerAttribute.equals(acquirerAttribute),
            "adapter-supplied ownership FOM lookup returned inconsistent handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         acquirerAttributes = JavaTckSupport.attributeSet(acquirer, acquirerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "confirm-divestiture registration returned a null object handle");
         await(acquirer, callbackModel, () -> acquirerRecorder.discovery != null);
         JavaTckSupport.check(object.equals(acquirerRecorder.discovery.object),
            "confirm-divestiture discovery returned the wrong object handle");

         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, divestitureTag);
         acquirer.attributeOwnershipAcquisition(object, acquirerAttributes, acquisitionTag);
         await(owner, callbackModel, () -> !ownerRecorder.divestitureRequests.isEmpty());
         OwnershipEvent divestitureRequest = ownerRecorder.divestitureRequests.get(0);
         JavaTckSupport.check(object.equals(divestitureRequest.object)
               && divestitureRequest.attributes.equals(Set.of(ownerAttribute))
               && Arrays.equals(acquisitionTag, divestitureRequest.tag),
            "negotiated divestiture requested confirmation with the wrong object, attributes, or tag");

         owner.setServiceReportingSwitch(true);
         JavaTckSupport.check(owner.getServiceReportingSwitch(),
            "service reporting did not enable before confirmation");
         owner.confirmDivestiture(object, ownerAttributes, confirmationTag);
         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty());
         await(acquirer, callbackModel, () -> !acquirerRecorder.acquisitions.isEmpty());
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "confirm-divestiture did not transfer ownership to the acquirer");
         OwnershipEvent acquisition = acquirerRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.equals(Set.of(acquirerAttribute))
               && Arrays.equals(confirmationTag, acquisition.tag),
            "acquisition callback returned the wrong object, attributes, or confirmation tag");
         JavaTckSupport.check(observerRecorder.interactions.size() == 1,
            "ConfirmDivestiture produced an unexpected MOM report count");
         verifyReport(observer, ownerFederate, observerRecorder.interactions.get(0), encoder,
            reportClass, reportParameters, reliable, object, ownerAttribute, confirmationTag);
         JavaTckSupport.check(observerRecorder.timestampedInteractions.isEmpty(),
            "receive-order ConfirmDivestiture report used the timestamped callback overload");

         owner.setServiceReportingSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         acquirer.unconditionalAttributeOwnershipDivestiture(object, acquirerAttributes,
            new byte[0]);
         acquirer.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         acquirerJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { owner.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (acquirerJoined) resign(acquirer, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (ownerJoined) resign(owner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (observerJoined) resign(observer, ResignAction.NO_ACTION);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (acquirerConnected) disconnect(acquirer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] confirmationTag)
         throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length && event.tag.length == 0
            && event.producer == null && event.regionsEmpty && reliable.equals(event.transport),
         "ConfirmDivestiture report had incorrect standard receive-order metadata");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            "ConfirmDivestiture report omitted standard parameter " + name);
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
      JavaTckSupport.check("ConfirmDivestiture".equals(service.getValue())
            && serviceType.getValue() == 3 && success.getValue() && serial.getValue() == 0
            && exception.getValue().isEmpty(),
         "ConfirmDivestiture report returned incorrect service, status, serial, or exception");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(expectedFederate.equals(reportedFederate),
         "ConfirmDivestiture report identified the wrong invoking federate");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 3,
         "ConfirmDivestiture report returned the wrong supplied-argument count");
      verifyArgument(supplied.get(0), 37, "Object instance designator",
         quoted(object.toString()), "ConfirmDivestiture object argument");
      verifyArgument(supplied.get(1), 1, "Set of attribute designators",
         "[" + quoted(attribute.toString()) + "]", "ConfirmDivestiture attribute-set argument");
      String encodedTag = Base64.getEncoder().encodeToString(confirmationTag);
      verifyArgument(supplied.get(2), 60, "User-supplied tag", quoted(encodedTag),
         "ConfirmDivestiture confirmation-tag argument");
      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      verifyArgument(returned, 34, "", "null", "ConfirmDivestiture return value");
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

   private static void await(RTIambassador rti, CallbackModel model, Ready ready)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!ready.get() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(ready.get(), "timed out waiting for confirm-divestiture callbacks");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK confirm-divestiture callbacks";
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

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final byte[] tag;
      private OwnershipEvent(ObjectInstanceHandle object, Set<AttributeHandle> attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
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
      private final List<String> timestampedInteractions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private volatile Discovery discovery;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  discovery = new Discovery((ObjectInstanceHandle) arguments[0]);
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length == 3) {
                  divestitureRequests.add(ownershipEvent(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(ownershipEvent(arguments));
               } else if ("receiveInteraction".equals(name)
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
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length > 6) {
                  timestampedInteractions.add("timestamped");
               }
               return objectMethod(proxy, name, arguments);
            });
      }

      private static OwnershipEvent ownershipEvent(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            new HashSet<>((AttributeHandleSet) arguments[1]), ((byte[]) arguments[2]).clone());
      }
   }
}
