package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SynchronizationPointFailureReason;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Base64;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies synchronization service reports, registration callbacks, and barrier completion. */
final class ServiceReportSynchronizationTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportSynchronizationTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador participant = factory.getRtiAmbassador();
      RTIambassador reportSink = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder participantRecorder = new Recorder();
      Recorder reportRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-synchronization-" + suffix;
      String subjectName = "java-tck-sync-report-subject-" + suffix;
      String participantName = "java-tck-sync-report-participant-" + suffix;
      String reportSinkName = "java-tck-sync-report-sink-" + suffix;
      String label = "java-tck-service-report-synchronization-point-" + suffix;
      byte[] tag = {0x53, 0x59, 0x4e, 0x43};
      boolean subjectConnected = false, participantConnected = false, sinkConnected = false;
      boolean created = false, subjectJoined = false, participantJoined = false, sinkJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         participant.connect(participantRecorder.proxy(), callbackModel);
         participantConnected = true;
         reportSink.connect(reportRecorder.proxy(), callbackModel);
         sinkConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         FederateHandle participantFederate = participant.joinFederationExecution(
            participantName, "java-tck", federation);
         participantJoined = true;
         reportSink.joinFederationExecution(reportSinkName, "java-tck", federation);
         sinkJoined = true;

         reportClass = reportSink.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(reportSink.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = reportSink.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(reportSink.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         final InteractionClassHandle reportInteractionClass = reportClass;
         TransportationTypeHandle reliable = reportSink.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(reportSink.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         participant.setServiceReportingSwitch(false);
         participant.setSendServiceReportsToFileSwitch(false);
         reportSink.setServiceReportingSwitch(false);
         reportSink.setSendServiceReportsToFileSwitch(false);
         reportSink.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         FederateHandleSet synchronizationSet = subject.getFederateHandleSetFactory().create();
         synchronizationSet.add(subjectFederate);
         synchronizationSet.add(participantFederate);
         JavaTckSupport.check(synchronizationSet.size() == 2,
            "synchronization set did not retain both configured members");
         subject.registerFederationSynchronizationPoint(label, tag, synchronizationSet);
         await(callbackModel, subject, participant, reportSink,
            () -> subjectRecorder.registrations.size() == 1
               && subjectRecorder.announcements.size() == 1
               && participantRecorder.announcements.size() == 1
               && countReports(reportRecorder, encoder, reportInteractionClass, reportParameters,
                  "RegisterFederationSynchronizationPoint") == 1
               && countReports(reportRecorder, encoder, reportInteractionClass, reportParameters,
                  "ConfirmSynchronizationPointRegistration") == 1
               && countReports(reportRecorder, encoder, reportInteractionClass, reportParameters,
                  "AnnounceSynchronizationPoint") == 1);
         JavaTckSupport.check(subjectRecorder.registrations.get(0).equals(label)
               && subjectRecorder.announcements.get(0).matches(label, tag)
               && participantRecorder.announcements.get(0).matches(label, tag),
            "synchronization registration/announcement callbacks returned the wrong label or tag");
         verifyReport(reportSink, reportRecorder.interactions, encoder, reportClass,
            reportParameters, reliable, "RegisterFederationSynchronizationPoint", 0,
            new Arg[] {
               new Arg(53, "Synchronization point label", quoted(label)),
               new Arg(60, "User-supplied tag", quoted(Base64.getEncoder().encodeToString(tag))),
               new Arg(18, "Optional set of joined federate designators", null)
            }, null);
         verifyReport(reportSink, reportRecorder.interactions, encoder, reportClass,
            reportParameters, reliable, "ConfirmSynchronizationPointRegistration", 1,
            new Arg[] {
               new Arg(53, "Synchronization point label", quoted(label)),
               new Arg(6, "Registration-success indicator", null),
               new Arg(34, "Optional failure reason", null)
            }, null);
         verifyReport(reportSink, reportRecorder.interactions, encoder, reportClass,
            reportParameters, reliable, "AnnounceSynchronizationPoint", 2,
            new Arg[] {
               new Arg(53, "Synchronization point label", quoted(label)),
               new Arg(60, "User-supplied tag", quoted(Base64.getEncoder().encodeToString(tag)))
            }, null);

         // Duplicate registration is still a successful service invocation but its
         // registration callback reports the standard non-unique-label failure.
         subject.registerFederationSynchronizationPoint(label, tag, synchronizationSet);
         await(callbackModel, subject, participant, reportSink,
            () -> subjectRecorder.registrations.size() == 1
               && subjectRecorder.failures.size() == 1
               && countReports(reportRecorder, encoder, reportInteractionClass, reportParameters,
                  "RegisterFederationSynchronizationPoint") == 2
               && countReports(reportRecorder, encoder, reportInteractionClass, reportParameters,
                  "ConfirmSynchronizationPointRegistration") == 2);
         RegistrationFailure duplicate = subjectRecorder.failures.get(0);
         JavaTckSupport.check(duplicate.label.equals(label)
               && duplicate.reason == SynchronizationPointFailureReason
                  .SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE,
            "duplicate registration returned the wrong standard failure reason");
         verifyReport(reportSink, reportRecorder.interactions, encoder, reportClass,
            reportParameters, reliable, "RegisterFederationSynchronizationPoint", 3,
            new Arg[] {
               new Arg(53, "Synchronization point label", quoted(label)),
               new Arg(60, "User-supplied tag", null),
               new Arg(18, "Optional set of joined federate designators", null)
            }, null);
         verifyReport(reportSink, reportRecorder.interactions, encoder, reportClass,
            reportParameters, reliable, "ConfirmSynchronizationPointRegistration", 4,
            new Arg[] {
               new Arg(53, "Synchronization point label", quoted(label)),
               new Arg(6, "Registration-success indicator", null),
               new Arg(56, "Optional failure reason",
                  quoted("SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE"))
            }, quoted("SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE"));

         subject.synchronizationPointAchieved(label, true);
         participant.synchronizationPointAchieved(label, false);
         await(callbackModel, subject, participant, reportSink,
            () -> subjectRecorder.completions.size() == 1
               && participantRecorder.completions.size() == 1
               && countReports(reportRecorder, encoder, reportInteractionClass, reportParameters,
                  "SynchronizationPointAchieved") == 1
               && countReports(reportRecorder, encoder, reportInteractionClass, reportParameters,
                  "FederationSynchronized") == 1);
         Set<FederateHandle> expectedFailureSet = Set.of(participantFederate);
         JavaTckSupport.check(subjectRecorder.completions.get(0).matches(label,
               expectedFailureSet)
               && participantRecorder.completions.get(0).matches(label, expectedFailureSet),
            "synchronization completion callback returned the wrong failed-to-sync set");
         verifyReport(reportSink, reportRecorder.interactions, encoder, reportClass,
            reportParameters, reliable, "SynchronizationPointAchieved", 5,
            new Arg[] {
               new Arg(53, "Synchronization point label", quoted(label)),
               new Arg(6, "Optional synchronization-success indicator", null)
            }, null);
         verifyReport(reportSink, reportRecorder.interactions, encoder, reportClass,
            reportParameters, reliable, "FederationSynchronized", 6,
            new Arg[] {
               new Arg(53, "Synchronization point label", quoted(label)),
               new Arg(18, "Set of joined federate designators", null)
            }, null);
         JavaTckSupport.check(reportRecorder.interactions.size() == 7,
            "synchronization report sink received an unexpected report count");

         reportSink.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         participant.setServiceReportingSwitch(false);
         participant.resignFederationExecution(ResignAction.NO_ACTION);
         participantJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         reportSink.resignFederationExecution(ResignAction.NO_ACTION);
         sinkJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && sinkJoined && reportClass != null) {
            try { reportSink.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         try { participant.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (participantJoined) resign(participant);
         if (subjectJoined) resign(subject);
         if (sinkJoined) resign(reportSink);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (sinkConnected) disconnect(reportSink);
         if (participantConnected) disconnect(participant);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static int countReports(Recorder recorder, EncoderFactory encoder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         String service) throws Exception {
      int result = 0;
      for (Interaction event : recorder.interactions) {
         if (!reportClass.equals(event.interaction)) continue;
         HLAunicodeString decoded = encoder.createHLAunicodeString();
         decoded.decode(event.parameters.get(parameters.get("HLAservice")));
         if (service.equals(decoded.getValue())) ++result;
      }
      return result;
   }

   private static void verifyReport(RTIambassador reportSink, List<Interaction> reports,
         EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, int expectedSerial, Arg[] expectedArguments,
         String expectedLastArgumentValue) throws Exception {
      Interaction report = null;
      for (Interaction candidate : reports) {
         if (reportClass.equals(candidate.interaction)
               && decodeString(encoder, candidate, parameters.get("HLAservice"))
                  .equals(service)
               && decodeInt32(encoder, candidate, parameters.get("HLAserialNumber"))
                  == expectedSerial) {
            report = candidate;
            break;
         }
      }
      JavaTckSupport.check(report != null,
         service + " report with serial " + expectedSerial + " was not received");
      JavaTckSupport.check(report.parameters.size() == REPORT_PARAMETERS.length
            && report.tag.length == 0 && report.producer == null && report.regionsEmpty
            && reliable.equals(report.transport),
         service + " report returned non-standard identity or callback metadata");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(report.parameters.containsKey(parameters.get(name)),
            service + " report omitted standard parameter " + name);
      }
      JavaTckSupport.check(decodeInt16(encoder, report, parameters.get("HLAserviceType")) == 0
            && decodeBoolean(encoder, report, parameters.get("HLAsuccessIndicator")),
         service + " report returned the wrong federation-management type or status");
      JavaTckSupport.check(decodeInt32(encoder, report, parameters.get("HLAserialNumber"))
            == expectedSerial,
         service + " report returned the wrong serial number");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == expectedArguments.length,
         service + " report returned the wrong supplied-argument count");
      for (int index = 0; index < expectedArguments.length; ++index) {
         HLAfixedRecord actual = supplied.get(index);
         HLAinteger32BE type = (HLAinteger32BE) actual.get(0);
         HLAunicodeString name = (HLAunicodeString) actual.get(1);
         HLAunicodeString value = (HLAunicodeString) actual.get(2);
         Arg expected = expectedArguments[index];
         JavaTckSupport.check(type.getValue() == expected.type
               && expected.name.equals(name.getValue()),
            service + " report returned the wrong argument type/name at index " + index);
         if (expected.value != null) {
            JavaTckSupport.check(expected.value.equals(value.getValue()),
               service + " report returned the wrong argument value at index " + index);
         }
         if (expectedLastArgumentValue != null && index == 2) {
            JavaTckSupport.check(expectedLastArgumentValue.equals(value.getValue()),
               service + " report returned the wrong synchronization failure reason");
         }
      }

      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      verifyArgument(returned, 34, "", "null", service + " return argument");
      JavaTckSupport.check(decodeString(encoder, report, parameters.get("HLAexception")).isEmpty(),
         service + " successful invocation report carried an exception");
   }

   private static String decodeString(EncoderFactory encoder, Interaction event,
         ParameterHandle parameter) throws Exception {
      HLAunicodeString decoded = encoder.createHLAunicodeString();
      decoded.decode(event.parameters.get(parameter));
      return decoded.getValue();
   }

   private static int decodeInt16(EncoderFactory encoder, Interaction event,
         ParameterHandle parameter) throws Exception {
      HLAinteger16BE decoded = encoder.createHLAinteger16BE();
      decoded.decode(event.parameters.get(parameter));
      return decoded.getValue();
   }

   private static int decodeInt32(EncoderFactory encoder, Interaction event,
         ParameterHandle parameter) throws Exception {
      HLAinteger32BE decoded = encoder.createHLAinteger32BE();
      decoded.decode(event.parameters.get(parameter));
      return decoded.getValue();
   }

   private static boolean decodeBoolean(EncoderFactory encoder, Interaction event,
         ParameterHandle parameter) throws Exception {
      HLAboolean decoded = encoder.createHLAboolean();
      decoded.decode(event.parameters.get(parameter));
      return decoded.getValue();
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
         description + " had the wrong argument type, name, or value");
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static void await(CallbackModel model, RTIambassador subject,
         RTIambassador participant, RTIambassador reportSink, Ready ready) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!ready.get() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            subject.evokeCallback(0.01);
            participant.evokeCallback(0.01);
            reportSink.evokeCallback(0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(ready.get(), "timed out waiting for synchronization callbacks/reports");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK synchronization service-report callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == arguments[0];
      return null;
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Ready { boolean get() throws Exception; }

   private static final class Arg {
      private final int type;
      private final String name;
      private final String value;
      private Arg(int type, String name, String value) {
         this.type = type;
         this.name = name;
         this.value = value;
      }
   }

   private static final class RegistrationFailure {
      private final String label;
      private final SynchronizationPointFailureReason reason;
      private RegistrationFailure(String label, SynchronizationPointFailureReason reason) {
         this.label = label;
         this.reason = reason;
      }
   }

   private static final class Announcement {
      private final String label;
      private final byte[] tag;
      private Announcement(String label, byte[] tag) {
         this.label = label;
         this.tag = tag;
      }
      private boolean matches(String expectedLabel, byte[] expectedTag) {
         return label.equals(expectedLabel) && Arrays.equals(tag, expectedTag);
      }
   }

   private static final class Completion {
      private final String label;
      private final Set<FederateHandle> failedToSync;
      private Completion(String label, Set<FederateHandle> failedToSync) {
         this.label = label;
         this.failedToSync = failedToSync;
      }
      private boolean matches(String expectedLabel, Set<FederateHandle> expectedFailed) {
         return label.equals(expectedLabel) && failedToSync.equals(expectedFailed);
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

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<String> registrations = new CopyOnWriteArrayList<>();
      private final List<RegistrationFailure> failures = new CopyOnWriteArrayList<>();
      private final List<Announcement> announcements = new CopyOnWriteArrayList<>();
      private final List<Completion> completions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("synchronizationPointRegistrationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1) {
                  registrations.add((String) arguments[0]);
               } else if ("synchronizationPointRegistrationFailed".equals(name)
                     && arguments != null && arguments.length == 2) {
                  failures.add(new RegistrationFailure((String) arguments[0],
                     (SynchronizationPointFailureReason) arguments[1]));
               } else if ("announceSynchronizationPoint".equals(name)
                     && arguments != null && arguments.length == 2) {
                  announcements.add(new Announcement((String) arguments[0],
                     ((byte[]) arguments[1]).clone()));
               } else if ("federationSynchronized".equals(name)
                     && arguments != null && arguments.length == 2) {
                  completions.add(new Completion((String) arguments[0],
                     new HashSet<>((FederateHandleSet) arguments[1])));
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
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
