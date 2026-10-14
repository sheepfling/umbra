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
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies QueryAttributeOwnership MOM reporting and its mixed result callbacks. */
final class ServiceReportQueryOwnershipTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportQueryOwnershipTck() { }

   static void run(RtiFactory factory, String fom, String mim, String objectClassName,
         String firstAttributeName, String secondAttributeName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-query-ownership-" + suffix;
      String ownerName = "java-tck-query-ownership-owner-" + suffix;
      String requesterName = "java-tck-query-ownership-requester-" + suffix;
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            ownerName, "java-tck", federation);
         ownerJoined = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            requesterName, "java-tck", federation);
         requesterJoined = true;

         reportClass = owner.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(owner.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = owner.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(owner.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(owner.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         owner.setServiceReportingSwitch(false);
         owner.setSendServiceReportsToFileSwitch(false);
         requester.setServiceReportingSwitch(false);
         requester.setSendServiceReportsToFileSwitch(false);
         owner.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst =
            requester.getAttributeHandle(requesterClass, firstAttributeName);
         AttributeHandle requesterSecond =
            requester.getAttributeHandle(requesterClass, secondAttributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerFirst != null && ownerSecond != null
               && requesterFirst != null && requesterSecond != null
               && !ownerFirst.equals(ownerSecond) && !requesterFirst.equals(requesterSecond),
            "adapter-supplied ownership FOM did not resolve two distinct attributes");
         JavaTckSupport.check(ownerFirst.equals(requesterFirst)
               && ownerSecond.equals(requesterSecond),
            "federates resolved inconsistent attribute handles from the shared FOM");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterFirst, requesterSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle registered = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(registered != null,
            "ownership-query registration returned a null object handle");
         await(requester, requesterRecorder, callbackModel,
            () -> requesterRecorder.discovery != null);
         ObjectInstanceHandle object = requesterRecorder.discovery.object;
         JavaTckSupport.check(registered.equals(object),
            "ownership-query discovery returned the wrong object handle");

         requesterRecorder.clearOwnershipResults();
         requester.setServiceReportingSwitch(true);
         requester.queryAttributeOwnership(object, requesterAttributes);
         await(owner, ownerRecorder, callbackModel,
            () -> !ownerRecorder.interactions.isEmpty());
         await(requester, requesterRecorder, callbackModel,
            () -> requesterRecorder.ownershipInformation.size() == 1
               && requesterRecorder.ownershipNotOwned.size() == 1);

         JavaTckSupport.check(ownerRecorder.interactions.size() == 1,
            "QueryAttributeOwnership produced an unexpected service-report count");
         verifyReport(owner, requesterFederate, ownerRecorder.interactions.get(0), encoder,
            reportClass, reportParameters, reliable, object, requesterAttributes,
            requesterFirst, requesterSecond);
         JavaTckSupport.check(requesterRecorder.ownershipInformation.size() == 1
               && requesterRecorder.ownershipNotOwned.size() == 1,
            "mixed ownership query did not return exactly one callback of each result kind");
         OwnershipInformation information = requesterRecorder.ownershipInformation.get(0);
         OwnershipNotOwned notOwned = requesterRecorder.ownershipNotOwned.get(0);
         JavaTckSupport.check(object.equals(information.object)
               && information.attributes.equals(Set.of(requesterFirst))
               && ownerFederate.equals(information.owner),
            "ownership query returned the wrong owned subset or owner");
         JavaTckSupport.check(object.equals(notOwned.object)
               && notOwned.attributes.equals(Set.of(requesterSecond)),
            "ownership query returned the wrong unowned subset");

         owner.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         requester.setServiceReportingSwitch(false);
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && ownerJoined && reportClass != null) {
            try { owner.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { requester.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (requesterJoined) resign(requester, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void verifyReport(RTIambassador owner, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         ObjectInstanceHandle object, AttributeHandleSet queriedAttributes,
         AttributeHandle firstAttribute, AttributeHandle secondAttribute) throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length && event.tag.length == 0
            && event.producer == null && event.regionsEmpty && reliable.equals(event.transport),
         "ownership-query report had incorrect standard interaction metadata");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            "ownership-query report omitted standard parameter " + name);
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
      JavaTckSupport.check("QueryAttributeOwnership".equals(service.getValue())
            && serviceType.getValue() == 3 && success.getValue() && serial.getValue() == 0
            && exception.getValue().isEmpty(),
         "ownership-query report returned incorrect service, status, serial, or exception");
      FederateHandle reportedFederate = owner.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(expectedFederate.equals(reportedFederate),
         "ownership-query report identified the wrong invoking federate");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 2,
         "ownership-query report returned the wrong supplied-argument count");
      verifyArgument(supplied.get(0), 37, "Object instance designator",
         quoted(object.toString()), "ownership-query object argument");
      HLAfixedRecord setArgument = supplied.get(1);
      HLAinteger32BE setType = (HLAinteger32BE) setArgument.get(0);
      HLAunicodeString setName = (HLAunicodeString) setArgument.get(1);
      HLAunicodeString setValue = (HLAunicodeString) setArgument.get(2);
      JavaTckSupport.check(setArgument.size() == 3 && setType.getValue() == 1
            && "Set of attribute designators".equals(setName.getValue())
            && decodeQuotedHandleSet(setValue.getValue()).equals(Set.of(
               firstAttribute.toString(), secondAttribute.toString()))
            && queriedAttributes.size() == 2,
         "ownership-query report returned the wrong attribute designator set");
      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      verifyArgument(returned, 34, "", "null", "ownership-query return value");
   }

   private static Set<String> decodeQuotedHandleSet(String encoded) {
      JavaTckSupport.check(encoded.length() >= 2 && encoded.charAt(0) == '['
            && encoded.charAt(encoded.length() - 1) == ']',
         "ownership-query attribute set did not use bracketed standard report form");
      Set<String> result = new HashSet<>();
      String contents = encoded.substring(1, encoded.length() - 1);
      if (contents.isEmpty()) return result;
      for (String item : contents.split(",", -1)) {
         JavaTckSupport.check(item.length() >= 2 && item.charAt(0) == '"'
               && item.charAt(item.length() - 1) == '"',
            "ownership-query attribute designator was not quoted");
         JavaTckSupport.check(result.add(item.substring(1, item.length() - 1)),
            "ownership-query report repeated an attribute designator");
      }
      return result;
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
      JavaTckSupport.check(ready.get(), "timed out waiting for ownership-query callback");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK ownership-query callbacks";
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

   private static final class OwnershipInformation {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final FederateHandle owner;
      private OwnershipInformation(ObjectInstanceHandle object, Set<AttributeHandle> attributes,
            FederateHandle owner) {
         this.object = object;
         this.attributes = attributes;
         this.owner = owner;
      }
   }

   private static final class OwnershipNotOwned {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private OwnershipNotOwned(ObjectInstanceHandle object, Set<AttributeHandle> attributes) {
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
      private final List<OwnershipInformation> ownershipInformation = new CopyOnWriteArrayList<>();
      private final List<OwnershipNotOwned> ownershipNotOwned = new CopyOnWriteArrayList<>();
      private volatile Discovery discovery;

      private void clearOwnershipResults() {
         ownershipInformation.clear();
         ownershipNotOwned.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length == 4) {
                  discovery = new Discovery((ObjectInstanceHandle) arguments[0]);
               } else if ("informAttributeOwnership".equals(method.getName())
                     && arguments != null && arguments.length == 3) {
                  ownershipInformation.add(new OwnershipInformation(
                     (ObjectInstanceHandle) arguments[0],
                     new HashSet<>((AttributeHandleSet) arguments[1]),
                     (FederateHandle) arguments[2]));
               } else if ("attributeIsNotOwned".equals(method.getName())
                     && arguments != null && arguments.length == 2) {
                  ownershipNotOwned.add(new OwnershipNotOwned(
                     (ObjectInstanceHandle) arguments[0],
                     new HashSet<>((AttributeHandleSet) arguments[1])));
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
