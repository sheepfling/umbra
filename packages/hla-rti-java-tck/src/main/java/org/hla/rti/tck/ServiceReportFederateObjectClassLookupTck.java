package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
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

/** Verifies federate and object-class lookup results through standard MOM reports. */
final class ServiceReportFederateObjectClassLookupTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportFederateObjectClassLookupTck() { }

   static void run(RtiFactory factory, String fom, String mim, String objectClassName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-federate-object-class-" + suffix;
      String subjectName = "java-tck-federate-object-class-subject-" + suffix;
      String observerName = "java-tck-federate-object-class-observer-" + suffix;
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(objectClass != null
               && objectClassName.equals(subject.getObjectClassName(objectClass)),
            "adapter-supplied object-class handle/name did not round-trip");
         subject.setServiceReportingSwitch(true);
         FederateHandle federateByName = subject.getFederateHandle(subjectName);
         String resolvedFederateName = subject.getFederateName(subjectFederate);
         ObjectClassHandle objectClassByName = subject.getObjectClassHandle(objectClassName);
         String resolvedObjectClassName = subject.getObjectClassName(objectClass);
         JavaTckSupport.check(subjectFederate.equals(federateByName)
               && subjectName.equals(resolvedFederateName)
               && objectClass.equals(objectClassByName)
               && objectClassName.equals(resolvedObjectClassName),
            "federate/object-class lookup did not round-trip the adapter-supplied names");
         await(observer, recorder, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 4,
            "federate/object-class lookups produced an unexpected report count");

         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetFederateHandle",
            new Argument[] {new Argument(53, "Federate name", quoted(subjectName))},
            new Argument(15, "Federate handle", quoted(federateByName.toString())), 0);
         verifyReport(observer, subjectFederate, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable, "GetFederateName",
            new Argument[] {new Argument(15, "Federate handle", quoted(subjectFederate.toString()))},
            new Argument(53, "Federate name", quoted(resolvedFederateName)), 1);
         verifyReport(observer, subjectFederate, recorder.events.get(2), encoder, reportClass,
            reportParameters, reliable, "GetObjectClassHandle",
            new Argument[] {new Argument(53, "Object class name", quoted(objectClassName))},
            new Argument(36, "Object class handle", quoted(objectClassByName.toString())), 2);
         verifyReport(observer, subjectFederate, recorder.events.get(3), encoder, reportClass,
            reportParameters, reliable, "GetObjectClassName",
            new Argument[] {new Argument(36, "Object class handle", quoted(objectClass.toString()))},
            new Argument(53, "Object class name", quoted(resolvedObjectClassName)), 3);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial)
         throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length
            && event.tag.length == 0 && event.producer == null && event.regionsEmpty
            && reliable.equals(event.transportation),
         service + " report had unexpected interaction, metadata, or parameter count");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            service + " report omitted standard parameter " + name);
      }
      HLAunicodeString decodedService = encoder.createHLAunicodeString();
      decodedService.decode(event.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serialNumber = encoder.createHLAinteger32BE();
      serialNumber.decode(event.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(service.equals(decodedService.getValue())
            && serviceType.getValue() == 6 && success.getValue()
            && exception.getValue().isEmpty() && serialNumber.getValue() == serial,
         service + " report returned incorrect status, exception, or serial number");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(expectedFederate.equals(reportedFederate),
         service + " report identified the wrong joined federate");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == expectedSupplied.length,
         service + " report returned the wrong supplied-argument count");
      for (int index = 0; index < expectedSupplied.length; index++) {
         verifyArgument(supplied.get(index), expectedSupplied[index],
            service + " supplied argument " + index);
      }
      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      verifyArgument(returned, expectedReturned, service + " returned argument");
   }

   private static void verifyArgument(HLAfixedRecord record, Argument expected,
         String description) throws Exception {
      JavaTckSupport.check(record.size() == 3
            && ((HLAinteger32BE) record.get(0)).getValue() == expected.type
            && expected.name.equals(((HLAunicodeString) record.get(1)).getValue())
            && expected.value.equals(((HLAunicodeString) record.get(2)).getValue()),
         description + " had the wrong argument type, name, or value");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static void await(RTIambassador observer, Recorder recorder, CallbackModel model)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.events.size() < 4 && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.events.size() >= 4,
         "timed out waiting for federate/object-class lookup service reports");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK federate lookup callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == arguments[0];
      return null;
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Argument {
      private final int type;
      private final String name;
      private final String value;
      private Argument(int type, String name, String value) {
         this.type = type;
         this.name = name;
         this.value = value;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction,
            Map<ParameterHandle, byte[]> parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
      }
   }

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  Map<ParameterHandle, byte[]> values = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     values.put(handle, source.get(handle).clone());
                  }
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  events.add(new Interaction((InteractionClassHandle) arguments[0], values,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], regions == null || regions.isEmpty()));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
