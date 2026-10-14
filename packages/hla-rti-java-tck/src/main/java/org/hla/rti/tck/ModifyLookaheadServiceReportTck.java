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
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies the standard MOM report emitted for an accepted deferred Modify Lookahead request. */
final class ModifyLookaheadServiceReportTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ModifyLookaheadServiceReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      SubjectEvents subjectEvents = new SubjectEvents();
      Recorder observerEvents = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-modify-lookahead-service-report-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regulating = false;
      try {
         subject.connect(callbacks(subjectEvents), callbackModel);
         subjectConnected = true;
         observer.connect(observerEvents.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            "java-tck-modify-lookahead-report-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-modify-lookahead-report-observer", "tck-type", federation);
         observerJoined = true;

         InteractionClassHandle reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS_NAME.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETER_NAMES) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "standard service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup/name round-trip failed");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(observer.getTimeFactory().getName()),
            "MOM observer selected a different logical-time implementation");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval currentLookahead = intervalOffset(epsilon, 3);
         LogicalTimeInterval requestedLookahead = intervalOffset(epsilon, 1);
         subject.enableTimeRegulation(currentLookahead);
         regulating = true;
         await(subject, callbackModel, () -> subjectEvents.timeRegulationEnabled,
            "timeRegulationEnabled callback");
         JavaTckSupport.check(currentLookahead.equals(subject.queryLookahead()),
            "Modify Lookahead report setup did not establish the initial interval");

         subject.setServiceReportingSwitch(true);
         subject.modifyLookahead(requestedLookahead);
         JavaTckSupport.check(currentLookahead.equals(subject.queryLookahead()),
            "accepted lower lookahead changed the active interval before logical-time advancement");
         await(observer, callbackModel, () -> !observerEvents.interactions.isEmpty(),
            "ModifyLookahead HLAreportServiceInvocation callback");

         JavaTckSupport.check(observerEvents.interactions.size() == 1,
            "one accepted ModifyLookahead invocation did not produce exactly one MOM report");
         Interaction report = findServiceReport(observerEvents.interactions, encoder,
            reportParameters.get("HLAservice"));
         JavaTckSupport.check(report != null,
            "ModifyLookahead service report was not recorded");
         JavaTckSupport.check(report.interaction.equals(reportClass)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length
               && report.tag.length == 0 && reliable.equals(report.transportation)
               && report.regionsEmpty,
            "ModifyLookahead report returned non-standard callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
               "ModifyLookahead report omitted standard parameter " + name);
         }
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            report.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(subjectHandle.equals(reportedFederate),
            "ModifyLookahead service report identified the wrong invoking federate");

         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(report.parameters.get(reportParameters.get("HLAservice")));
         JavaTckSupport.check("ModifyLookahead".equals(service.getValue()),
            "ModifyLookahead report returned the wrong service name");
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(report.parameters.get(reportParameters.get("HLAserviceType")));
         JavaTckSupport.check(serviceType.getValue() == 4,
            "ModifyLookahead report returned the wrong standard service type");
         HLAboolean success = encoder.createHLAboolean();
         success.decode(report.parameters.get(reportParameters.get("HLAsuccessIndicator")));
         JavaTckSupport.check(success.getValue(),
            "accepted ModifyLookahead request was not reported as successful");
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(report.parameters.get(reportParameters.get("HLAexception")));
         JavaTckSupport.check(exception.getValue().isEmpty(),
            "successful ModifyLookahead report included an exception");
         HLAinteger32BE serial = encoder.createHLAinteger32BE();
         serial.decode(report.parameters.get(reportParameters.get("HLAserialNumber")));
         JavaTckSupport.check(serial.getValue() == 0,
            "first ModifyLookahead service report used a nonzero serial number");

         HLAfixedRecord argumentPrototype = argumentPrototype(encoder);
         HLAvariableArray<HLAfixedRecord> arguments = encoder.createHLAvariableArray(
            index -> argumentPrototype(encoder), argumentPrototype);
         arguments.decode(report.parameters.get(reportParameters.get("HLAsuppliedArguments")));
         JavaTckSupport.check(arguments.size() == 1,
            "ModifyLookahead report returned the wrong supplied-argument count");
         verifyArgument(arguments.get(0), 32, "Requested lookahead",
            quoted(requestedLookahead.toString()));

         HLAfixedRecord returned = argumentPrototype(encoder);
         returned.decode(report.parameters.get(reportParameters.get("HLAreturnedArgument")));
         verifyArgument(returned, 34, "", "null");
      } finally {
         if (reportSubscribed && observerJoined) {
            try { observer.unsubscribeInteractionClass(
               observer.getInteractionClassHandle(REPORT_CLASS_NAME)); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) try { subject.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
         if (regulating && subjectJoined) try { subject.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (observerJoined) try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (subjectJoined) try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); } catch (Exception ignored) { }
         if (subjectConnected) try { subject.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static Interaction findServiceReport(List<Interaction> interactions,
         EncoderFactory encoder, ParameterHandle serviceParameter) throws Exception {
      for (Interaction interaction : interactions) {
         byte[] bytes = interaction.parameters.get(serviceParameter);
         if (bytes == null) continue;
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(bytes);
         if ("ModifyLookahead".equals(service.getValue())) return interaction;
      }
      return null;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; i++) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE());
      record.add(encoder.createHLAunicodeString());
      record.add(encoder.createHLAunicodeString());
      return record;
   }

   private static void verifyArgument(HLAfixedRecord argument, int expectedType,
         String expectedName, String expectedValue) throws Exception {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == expectedType
            && expectedName.equals(((HLAunicodeString) argument.get(1)).getValue())
            && expectedValue.equals(((HLAunicodeString) argument.get(2)).getValue()),
         "ModifyLookahead report argument did not match standard type/name/value");
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static FederateAmbassador callbacks(SubjectEvents events) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("timeRegulationEnabled".equals(method.getName())) events.timeRegulationEnabled = true;
            return objectMethod(proxy, method.getName(), arguments);
         });
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK ModifyLookahead service-report callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
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
               return objectMethod(proxy, name, arguments);
            });
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final boolean regionsEmpty;
      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         for (ParameterHandle handle : values.keySet()) parameters.put(handle, values.get(handle).clone());
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         RegionHandleSet regions = (RegionHandleSet) arguments[5];
         regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class SubjectEvents {
      private volatile boolean timeRegulationEnabled;
   }

}
