package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Check active-save failure reporting and survivor service after adapter-induced loss. */
final class ConnectionLossCleanupTck {
   private static final String FEDERATE_MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_NAME = "HLAfederateName";
   private static final String REPORT_SERVICE_FILE = "HLAreportServiceFile";

   private ConnectionLossCleanupTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel,
         RtiConfiguration survivorConfiguration, RtiConfiguration lostConfiguration,
         String federation, String survivorName, String lostName, String readyMarkerPath,
         String connectionLostMarkerPath, long timeoutMillis) throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "adapter connection-loss readiness marker is required");
      JavaTckSupport.check(connectionLostMarkerPath != null && !connectionLostMarkerPath.isEmpty(),
         "adapter connection-loss callback marker is required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");
      JavaTckSupport.check(federation != null && !federation.isEmpty()
            && survivorName != null && !survivorName.isEmpty()
            && lostName != null && !lostName.isEmpty(),
         "adapter connection-loss federation and member names are required");

      RTIambassador survivor = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder survivorRecorder = new Recorder();
      Recorder lostRecorder = new Recorder();
      String saveLabel = "java-tck-save-on-connection-loss-" + UUID.randomUUID();
      boolean survivorConnected = false;
      boolean lostConnected = false;
      boolean survivorJoined = false;
      try {
         survivor.connect(survivorRecorder.proxy(), callbackModel, survivorConfiguration);
         survivorConnected = true;
         lost.connect(lostRecorder.proxy(), callbackModel, lostConfiguration);
         lostConnected = true;
         survivor.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         survivor.joinFederationExecution(survivorName, "java-tck", federation);
         survivorJoined = true;
         lost.joinFederationExecution(lostName, "java-tck", federation);

         ObjectClassHandle federateMomClass = lost.getObjectClassHandle(FEDERATE_MOM_CLASS);
         AttributeHandle federateNameAttribute =
            lost.getAttributeHandle(federateMomClass, FEDERATE_NAME);
         AttributeHandle reportFileAttribute =
            lost.getAttributeHandle(federateMomClass, REPORT_SERVICE_FILE);
         JavaTckSupport.check(federateMomClass != null
               && federateNameAttribute != null && reportFileAttribute != null,
            "connection-loss save case did not resolve the standard Federate MOM handles");
         AttributeHandleSet momAttributes = JavaTckSupport.attributeSet(lost,
            federateNameAttribute, reportFileAttribute);
         lost.subscribeObjectClassAttributes(federateMomClass, momAttributes, "");
         // This public service boundary lets the process fixture buffer an initial
         // pushed reflection before the selected callback model delivers it.
         JavaTckSupport.check(FEDERATE_MOM_CLASS.equals(lost.getObjectClassName(federateMomClass)),
            "Federate MOM class name did not round-trip after subscription");
         Reflection survivorReflection = awaitReportFileReflection(
            new RTIambassador[] {lost, survivor}, callbackModel,
            encoder, lostRecorder, federateMomClass, federateNameAttribute,
            reportFileAttribute, survivorName, timeoutMillis);
         ObjectInstanceHandle survivorMomObject = survivorReflection.object;
         Map<AttributeHandle, byte[]> values = survivorReflection.values;
         JavaTckSupport.check(survivorMomObject != null
               && federateMomClass.equals(lostRecorder.discoveredObjects.get(survivorMomObject))
               && values.size() == 2
               && survivorName.equals(decode(encoder, values.get(federateNameAttribute))),
            "survivor's Federate MOM reflection had unexpected identity or delivery");
         String reportFileValue = decode(encoder, values.get(reportFileAttribute));
         JavaTckSupport.check(reportFileValue != null && !reportFileValue.trim().isEmpty(),
            "HLAreportServiceFile did not expose a non-empty standard Unicode value");
         Path reportFile = Path.of(reportFileValue);
         JavaTckSupport.check(Files.isRegularFile(reportFile),
            "surviving federate HLAreportServiceFile did not name an existing file");

         survivor.setServiceReportingSwitch(true);
         survivor.setSendServiceReportsToFileSwitch(true);
         int survivorSaveInitiationsBefore = survivorRecorder.saveInitiations.size();
         int lostSaveInitiationsBefore = lostRecorder.saveInitiations.size();
         survivor.requestFederationSave(saveLabel);
         await(callbackModel, timeoutMillis, new RTIambassador[] {survivor, lost},
            () -> survivorRecorder.saveInitiations.size() > survivorSaveInitiationsBefore
               && lostRecorder.saveInitiations.size() > lostSaveInitiationsBefore,
            "save initiation before connection loss");
         JavaTckSupport.check(saveLabel.equals(lastString(survivorRecorder.saveInitiations))
               && saveLabel.equals(lastString(lostRecorder.saveInitiations)),
            "Initiate Federate Save returned the wrong label");
         survivor.federateSaveBegun();
         lost.federateSaveBegun();
         int saveFailuresBefore = survivorRecorder.saveFailures.size();
         String reportBeforeLoss = Files.readString(reportFile, StandardCharsets.UTF_8);

         writeMarker(Path.of(readyMarkerPath), "ready\n");
         awaitConnectionLost(lost, callbackModel, lostRecorder, timeoutMillis);
         JavaTckSupport.check(lostRecorder.connectionLostDescription != null
               && !lostRecorder.connectionLostDescription.isEmpty(),
            "connectionLost returned an empty fault description");
         JavaTckSupport.check(survivorRecorder.connectionLostDescription == null,
            "the surviving member also received a connectionLost callback");
         writeMarker(Path.of(connectionLostMarkerPath), "callback-ok\n");

         String reportAfterLoss = Files.readString(reportFile, StandardCharsets.UTF_8);
         requireSaveFailureRecord(reportBeforeLoss, reportAfterLoss);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(survivorRecorder.saveFailures.size() == saveFailuresBefore,
               "evoked federationNotSaved callback ran before service-report inspection");
         }
         await(callbackModel, timeoutMillis, new RTIambassador[] {survivor},
            () -> survivorRecorder.saveFailures.size() > saveFailuresBefore,
            "RTI_DETECTED_FAILURE_DURING_SAVE callback");
         JavaTckSupport.check(lastReason(survivorRecorder.saveFailures)
               == SaveFailureReason.RTI_DETECTED_FAILURE_DURING_SAVE,
            "surviving federate received the wrong federationNotSaved reason");

         survivor.listFederationExecutionMembers(federation);
         await(callbackModel, timeoutMillis, new RTIambassador[] {survivor},
            () -> survivorRecorder.listsOnlyMember(federation, survivorName, "java-tck"),
            "membership cleanup after connection loss");
         JavaTckSupport.expectFailure(lost::listFederationExecutions, "NotConnected",
            "listing federation executions after connection loss");

         InteractionClassHandle interactionClass =
            survivor.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interactionClass != null,
            "surviving member could not resolve the configured interaction class");
         survivor.sendInteraction(interactionClass,
            survivor.getParameterHandleValueMapFactory().create(0), new byte[0]);
         survivor.setServiceReportingSwitch(false);
         survivor.setSendServiceReportsToFileSwitch(false);
      } finally {
         if (lostConnected) {
            try { lost.disconnect(); } catch (Exception ignored) { }
         }
         if (survivorJoined) {
            try { survivor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (survivorConnected) {
            try { survivor.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(CallbackModel callbackModel, long timeoutMillis,
         RTIambassador[] ambassadors, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         for (RTIambassador ambassador : ambassadors) service(ambassador, callbackModel);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static Reflection awaitReportFileReflection(RTIambassador[] ambassadors,
         CallbackModel callbackModel, EncoderFactory encoder, Recorder recorder,
         ObjectClassHandle momClass, AttributeHandle federateNameAttribute,
         AttributeHandle reportFileAttribute, String survivorName, long timeoutMillis)
         throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      Reflection reflection = findReportFileReflection(encoder, recorder, momClass,
         federateNameAttribute, reportFileAttribute, survivorName);
      while (reflection == null && System.nanoTime() < deadline) {
         for (RTIambassador ambassador : ambassadors) service(ambassador, callbackModel);
         Thread.yield();
         reflection = findReportFileReflection(encoder, recorder, momClass,
            federateNameAttribute, reportFileAttribute, survivorName);
      }
      JavaTckSupport.check(reflection != null,
         "timed out waiting for survivor HLAreportServiceFile MOM reflection");
      return reflection;
   }

   private static Reflection findReportFileReflection(EncoderFactory encoder,
         Recorder recorder, ObjectClassHandle momClass, AttributeHandle federateNameAttribute,
         AttributeHandle reportFileAttribute, String survivorName) throws Exception {
      for (ObjectInstanceHandle object : recorder.discoveredObjects.keySet()) {
         if (!momClass.equals(recorder.discoveredObjects.get(object))) continue;
         Map<AttributeHandle, byte[]> values = recorder.valuesForObject(object);
         if (values.containsKey(federateNameAttribute)
               && values.containsKey(reportFileAttribute)
               && survivorName.equals(decode(encoder, values.get(federateNameAttribute)))) {
            return new Reflection(object, values);
         }
      }
      return null;
   }

   private static void awaitConnectionLost(RTIambassador lost, CallbackModel callbackModel,
         Recorder recorder, long timeoutMillis) throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (recorder.connectionLostDescription == null && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            try {
               lost.evokeMultipleCallbacks(0.0, 0.05);
            } catch (Exception ignored) {
               // The transport failure may be surfaced before its callback is queued.
            }
         } else {
            Thread.sleep(1L);
         }
      }
      JavaTckSupport.check(recorder.connectionLostDescription != null,
         "timed out waiting for the standard connectionLost callback");
   }

   private static void service(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.05);
      } else {
         Thread.sleep(1L);
      }
   }

   private static void requireSaveFailureRecord(String before, String after) {
      JavaTckSupport.check(after.length() >= before.length() && after.startsWith(before),
         "HLAreportServiceFile did not retain its pre-loss contents");
      String appended = after.substring(before.length());
      String service = "\"HLAservice\":\"FederationSaved\"";
      int servicePosition = appended.indexOf(service);
      JavaTckSupport.check(servicePosition >= 0,
         "HLAreportServiceFile omitted FederationSaved after connection loss");
      int lineStart = appended.lastIndexOf('\n', servicePosition);
      int recordStart = lineStart < 0 ? 0 : lineStart + 1;
      int lineEnd = appended.indexOf('\n', servicePosition);
      String record = appended.substring(recordStart,
         lineEnd < 0 ? appended.length() : lineEnd);
      JavaTckSupport.check(record.contains(
            "\"HLAargumentType\":48,\"HLAargumentName\":\"Optional failure reason\","
               + "\"HLAargumentValue\":\"RTI_DETECTED_FAILURE_DURING_SAVE\""),
         "HLAreportServiceFile omitted type-48 RTI_DETECTED_FAILURE_DURING_SAVE");
   }

   private static String decode(EncoderFactory encoder, byte[] encoded) throws Exception {
      JavaTckSupport.check(encoded != null, "Federate MOM reflection omitted a required value");
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(encoded);
      return value.getValue();
   }

   private static String lastString(List<String> values) {
      return values.get(values.size() - 1);
   }

   private static SaveFailureReason lastReason(List<SaveFailureReason> values) {
      return values.get(values.size() - 1);
   }

   private static void writeMarker(Path marker, String value) throws Exception {
      Path parent = marker.toAbsolutePath().getParent();
      if (parent != null) Files.createDirectories(parent);
      Files.write(marker, value.getBytes(StandardCharsets.UTF_8));
   }

   private static final class Recorder {
      private volatile String connectionLostDescription;
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<MemberReport> memberReports = new CopyOnWriteArrayList<>();
      private final Map<ObjectInstanceHandle, ObjectClassHandle> discoveredObjects =
         new ConcurrentHashMap<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private Map<AttributeHandle, byte[]> valuesForObject(ObjectInstanceHandle object) {
         Map<AttributeHandle, byte[]> values = new HashMap<>();
         for (Reflection reflection : reflections) {
            if (object.equals(reflection.object)) {
               values.putAll(reflection.values);
            }
         }
         return values;
      }

      private boolean listsOnlyMember(String federation, String expectedName,
            String expectedType) {
         MemberReport latest = null;
         for (MemberReport report : memberReports) {
            if (federation.equals(report.federation)) latest = report;
         }
         if (latest == null || latest.members.size() != 1) return false;
         for (FederationExecutionMemberInformation member : latest.members) {
            return expectedName.equals(member.federateName)
               && expectedType.equals(member.federateType);
         }
         return false;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("connectionLost".equals(name)
                     && arguments != null && arguments.length == 1) {
                  connectionLostDescription = (String) arguments[0];
               } else if ("initiateFederateSave".equals(name)
                     && arguments != null && arguments.length > 0) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationNotSaved".equals(name)
                     && arguments != null && arguments.length == 1) {
                  saveFailures.add((SaveFailureReason) arguments[0]);
               } else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof ObjectClassHandle) {
                  discoveredObjects.put((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 4
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  Map<AttributeHandle, byte[]> values = new HashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     byte[] value = source.get(handle);
                     values.put(handle, value == null ? null : value.clone());
                  }
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], values));
               } else if ("reportFederationExecutionMembers".equals(name)
                     && arguments != null && arguments.length == 2
                     && arguments[1] instanceof FederationExecutionMemberInformationSet) {
                  memberReports.add(new MemberReport((String) arguments[0],
                     ((FederationExecutionMemberInformationSet) arguments[1]).clone()));
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values) {
         this.object = object;
         this.values = values;
      }
   }

   private static final class MemberReport {
      private final String federation;
      private final FederationExecutionMemberInformationSet members;

      private MemberReport(String federation, FederationExecutionMemberInformationSet members) {
         this.federation = federation;
         this.members = members;
      }
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK connection-loss callback";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }
}
