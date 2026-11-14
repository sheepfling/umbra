package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreFailureReason;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the Table 5 initial record and save/restore failure reasons in the advertised file. */
final class ServiceReportSaveFailureReasonFileTck {
   private static final String FEDERATE_MOM_CLASS =
      "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String REPORT_DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final long TIMEOUT_NANOS = 30_000_000_000L;

   private ServiceReportSaveFailureReasonFileTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, RtiConfiguration subjectConfiguration,
         String additionalSettings, String serviceReportDirectory,
         String configurationName, String rtiAddress) throws Exception {
      JavaTckSupport.check(serviceReportDirectory != null && !serviceReportDirectory.isEmpty()
            && Files.isDirectory(Path.of(serviceReportDirectory)),
         "adapter-supplied service-report directory must exist");
      String expectedDirectorySetting = REPORT_DIRECTORY_SETTING
         + Path.of(serviceReportDirectory);
      JavaTckSupport.check(subjectConfiguration != null && additionalSettings != null
            && Arrays.asList(additionalSettings.split(";")).contains(expectedDirectorySetting),
         "adapter did not configure the expected serviceReportDirectory setting");

      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      MomRecorder momRecorder = new MomRecorder();
      FederationRecorder subjectRecorder = new FederationRecorder();
      FederationRecorder observerRecorder = new FederationRecorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-save-failure-" + suffix;
      String subjectName = "java-tck-service-report-save-failure-subject-" + suffix;
      String observerName = "java-tck-service-report-save-failure-observer-" + suffix;

      boolean observerConnected = false;
      boolean subjectConnected = false;
      boolean federationCreated = false;
      boolean observerJoined = false;
      boolean subjectJoined = false;
      boolean subscribed = false;
      ObjectClassHandle federateMomClass = null;
      hla.rti1516_2025.AttributeHandleSet federateMomAttributes = null;
      try {
         observer.connect(momRecorder.proxy(observerRecorder), callbackModel);
         observerConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel, subjectConfiguration);
         subjectConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         federationCreated = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "initial-record coverage requires file reporting to be disabled at Join");
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         federateMomClass = observer.getObjectClassHandle(FEDERATE_MOM_CLASS);
         AttributeHandle nameAttribute = observer.getAttributeHandle(
            federateMomClass, "HLAfederateName");
         AttributeHandle reportFileAttribute = observer.getAttributeHandle(
            federateMomClass, "HLAreportServiceFile");
         JavaTckSupport.check(FEDERATE_MOM_CLASS.equals(
               observer.getObjectClassName(federateMomClass))
               && "HLAfederateName".equals(
                  observer.getAttributeName(federateMomClass, nameAttribute))
               && "HLAreportServiceFile".equals(
                  observer.getAttributeName(federateMomClass, reportFileAttribute)),
            "Federate MOM report-file handles did not round-trip by public name");
         federateMomAttributes = JavaTckSupport.attributeSet(
            observer, nameAttribute, reportFileAttribute);
         observer.subscribeObjectClassAttributes(federateMomClass, federateMomAttributes, "");
         subscribed = true;

         byte[] encodedSubjectName =
            encoder.createHLAunicodeString(subjectName).toByteArray();
         await(observer, subject, callbackModel,
            () -> momRecorder.hasAttributeForFederate(
               nameAttribute, encodedSubjectName, reportFileAttribute),
            "subject HLAreportServiceFile MOM reflection");
         ObjectInstanceHandle subjectMomObject =
            momRecorder.objectForFederate(nameAttribute, encodedSubjectName);
         byte[] reportFileValue = momRecorder.valuesForObject(subjectMomObject)
            .get(reportFileAttribute);
         String reportFileName = decode(encoder, reportFileValue);
         JavaTckSupport.check(reportFileName != null && !reportFileName.trim().isEmpty(),
            "subject HLAreportServiceFile reflection did not contain a file name");
         Path reportFile = Path.of(reportFileName);
         JavaTckSupport.check(Files.isRegularFile(reportFile),
            "subject HLAreportServiceFile did not name an existing report file");
         String initialContents = Files.readString(reportFile, StandardCharsets.UTF_8);
         JavaTckSupport.check(!initialContents.isEmpty(),
            "subject HLAreportServiceFile did not contain its initial record");
         verifyInitialRecord(initialContents, federation, subjectName, "java-tck",
            timeImplementation, callbackModel, configurationName, rtiAddress,
            additionalSettings);

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         subject.setSendServiceReportsToFileSwitch(true);
         subject.setServiceReportingSwitch(true);

         String saveLabel = "java-tck-save-failure-reason-" + suffix;
         final int subjectSaveStarts = subjectRecorder.saveInitiations.size();
         final int observerSaveStarts = observerRecorder.saveInitiations.size();
         subject.requestFederationSave(saveLabel);
         await(observer, subject, callbackModel,
            () -> hasSaveInitiationSince(subjectRecorder, subjectSaveStarts, saveLabel)
               && hasSaveInitiationSince(observerRecorder, observerSaveStarts, saveLabel),
            "SAVE_ABORTED save-initiation callbacks");

         String beforeAbort = Files.readString(reportFile, StandardCharsets.UTF_8);
         final int subjectFailuresBeforeAbort = subjectRecorder.saveFailures.size();
         final int observerFailuresBeforeAbort = observerRecorder.saveFailures.size();
         subject.abortFederationSave();
         String afterAbort = Files.readString(reportFile, StandardCharsets.UTF_8);
         requireSaveFailureRecord(beforeAbort, afterAbort, "SAVE_ABORTED");
         requireNoEvokedSaveFailureBeforeFile(callbackModel, subjectRecorder,
            subjectFailuresBeforeAbort, observerRecorder, observerFailuresBeforeAbort,
            "SAVE_ABORTED");
         await(observer, subject, callbackModel,
            () -> hasSaveFailureSince(subjectRecorder, subjectFailuresBeforeAbort,
                  SaveFailureReason.SAVE_ABORTED)
               && hasSaveFailureSince(observerRecorder, observerFailuresBeforeAbort,
                  SaveFailureReason.SAVE_ABORTED),
            "SAVE_ABORTED Federation Not Saved callbacks");

         String federateFailureLabel = "java-tck-federate-reported-save-failure-" + suffix;
         final int subjectFederateFailureSaveStarts = subjectRecorder.saveInitiations.size();
         final int observerFederateFailureSaveStarts = observerRecorder.saveInitiations.size();
         subject.requestFederationSave(federateFailureLabel);
         await(observer, subject, callbackModel,
            () -> hasSaveInitiationSince(subjectRecorder, subjectFederateFailureSaveStarts,
                  federateFailureLabel)
               && hasSaveInitiationSince(observerRecorder, observerFederateFailureSaveStarts,
                  federateFailureLabel),
            "federate-reported failure save-initiation callbacks");
         subject.federateSaveBegun();
         observer.federateSaveBegun();
         beforeAbort = Files.readString(reportFile, StandardCharsets.UTF_8);
         final int subjectFederateFailuresBeforeAbort = subjectRecorder.saveFailures.size();
         final int observerFederateFailuresBeforeAbort = observerRecorder.saveFailures.size();
         observer.federateSaveNotComplete();
         afterAbort = Files.readString(reportFile, StandardCharsets.UTF_8);
         requireSaveFailureRecord(beforeAbort, afterAbort,
            "FEDERATE_REPORTED_FAILURE_DURING_SAVE");
         requireNoEvokedSaveFailureBeforeFile(callbackModel, subjectRecorder,
            subjectFederateFailuresBeforeAbort, observerRecorder,
            observerFederateFailuresBeforeAbort,
            "FEDERATE_REPORTED_FAILURE_DURING_SAVE");
         await(observer, subject, callbackModel,
            () -> hasSaveFailureSince(subjectRecorder, subjectFederateFailuresBeforeAbort,
                  SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE)
               && hasSaveFailureSince(observerRecorder, observerFederateFailuresBeforeAbort,
                  SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE),
            "FEDERATE_REPORTED_FAILURE_DURING_SAVE callbacks");

         String restoreSaveLabel = "java-tck-restore-failure-setup-save-" + suffix;
         final int subjectRestoreSetupSaveStarts = subjectRecorder.saveInitiations.size();
         final int observerRestoreSetupSaveStarts = observerRecorder.saveInitiations.size();
         subject.requestFederationSave(restoreSaveLabel);
         await(observer, subject, callbackModel,
            () -> hasSaveInitiationSince(subjectRecorder, subjectRestoreSetupSaveStarts,
                  restoreSaveLabel)
               && hasSaveInitiationSince(observerRecorder, observerRestoreSetupSaveStarts,
                  restoreSaveLabel),
            "RestoreFailureReason setup save-initiation callbacks");
         subject.federateSaveBegun();
         observer.federateSaveBegun();
         final int subjectSavedBefore = subjectRecorder.savedCount;
         final int observerSavedBefore = observerRecorder.savedCount;
         subject.federateSaveComplete();
         observer.federateSaveComplete();
         await(observer, subject, callbackModel,
            () -> subjectRecorder.savedCount > subjectSavedBefore
               && observerRecorder.savedCount > observerSavedBefore,
            "RestoreFailureReason setup save completion callbacks");

         String restoreLabel = restoreSaveLabel;
         final int subjectRestoreRequests = subjectRecorder.restoreSuccesses.size();
         final int subjectRestoreBegun = subjectRecorder.restoreBegunCount;
         final int observerRestoreBegun = observerRecorder.restoreBegunCount;
         final int subjectRestoreStarts = subjectRecorder.restoreInitiations.size();
         final int observerRestoreStarts = observerRecorder.restoreInitiations.size();
         subject.requestFederationRestore(restoreLabel);
         await(observer, subject, callbackModel,
            () -> hasRestoreSuccessSince(subjectRecorder, subjectRestoreRequests, restoreLabel)
               && subjectRecorder.restoreBegunCount > subjectRestoreBegun
               && observerRecorder.restoreBegunCount > observerRestoreBegun
               && hasRestoreInitiationSince(subjectRecorder, subjectRestoreStarts, restoreLabel)
               && hasRestoreInitiationSince(observerRecorder, observerRestoreStarts, restoreLabel),
            "RestoreFailureReason restore-initiation callbacks");

         String beforeRestoreAbort = Files.readString(reportFile, StandardCharsets.UTF_8);
         final int subjectRestoreFailures = subjectRecorder.restoreFailures.size();
         final int observerRestoreFailures = observerRecorder.restoreFailures.size();
         subject.abortFederationRestore();
         String afterRestoreAbort = Files.readString(reportFile, StandardCharsets.UTF_8);
         requireRestoreFailureRecord(beforeRestoreAbort, afterRestoreAbort);
         requireNoEvokedRestoreFailureBeforeFile(callbackModel, subjectRecorder,
            subjectRestoreFailures, observerRecorder, observerRestoreFailures);
         await(observer, subject, callbackModel,
            () -> hasRestoreFailureSince(subjectRecorder, subjectRestoreFailures,
                  RestoreFailureReason.RESTORE_ABORTED)
               && hasRestoreFailureSince(observerRecorder, observerRestoreFailures,
                  RestoreFailureReason.RESTORE_ABORTED),
            "RESTORE_ABORTED Federation Not Restored callbacks");

         String resignedSaveLabel = "java-tck-federate-resigned-during-save-" + suffix;
         final int subjectResignedSaveStarts = subjectRecorder.saveInitiations.size();
         final int observerResignedSaveStarts = observerRecorder.saveInitiations.size();
         subject.requestFederationSave(resignedSaveLabel);
         await(observer, subject, callbackModel,
            () -> hasSaveInitiationSince(subjectRecorder, subjectResignedSaveStarts,
                  resignedSaveLabel)
               && hasSaveInitiationSince(observerRecorder, observerResignedSaveStarts,
                  resignedSaveLabel),
            "federate-resignation save-initiation callbacks");
         subject.federateSaveBegun();
         observer.federateSaveBegun();
         final int subjectFailuresBeforeResignation = subjectRecorder.saveFailures.size();
         String beforeResignation = Files.readString(reportFile, StandardCharsets.UTF_8);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subscribed = false;
         String afterResignation = Files.readString(reportFile, StandardCharsets.UTF_8);
         requireSaveFailureRecord(beforeResignation, afterResignation,
            "FEDERATE_RESIGNED_DURING_SAVE");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(
               subjectRecorder.saveFailures.size() == subjectFailuresBeforeResignation,
               "Evoked federationNotSaved callback ran before service-report file inspection");
         }
         await(subject, null, callbackModel,
            () -> hasSaveFailureSince(subjectRecorder, subjectFailuresBeforeResignation,
               SaveFailureReason.FEDERATE_RESIGNED_DURING_SAVE),
            "FEDERATE_RESIGNED_DURING_SAVE survivor callback");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         federationCreated = false;
         observer.disconnect();
         observerConnected = false;
         subject.disconnect();
         subjectConnected = false;
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { subject.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
            resign(subject);
         }
         if (subscribed && observerJoined && federateMomClass != null
               && federateMomAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(
                  federateMomClass, federateMomAttributes);
            } catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (federationCreated && subjectConnected) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) disconnect(subject);
         if (observerConnected) disconnect(observer);
      }
   }

   private static void verifyInitialRecord(String contents, String federation,
         String subjectName, String federateType, String timeImplementation,
         CallbackModel callbackModel, String configurationName, String rtiAddress,
         String additionalSettings) {
      String initialRecord = firstJsonObjectRecord(contents);
      JavaTckSupport.check(initialRecord != null,
         "HLAreportServiceFile did not begin with a complete initial record");
      String configuration = jsonObjectRecord(initialRecord, "Configuration");
      String federationAttributes = jsonObjectRecord(
         initialRecord, "HLAmanager.HLAfederation");
      String federateAttributes = jsonObjectRecord(
         initialRecord, "HLAmanager.HLAfederate");
      JavaTckSupport.check(configuration != null && federationAttributes != null
            && federateAttributes != null,
         "initial service-report record omitted a Table 5 component");

      String callbackModelValue = jsonStringToken(configuration, "CallbackModel");
      String configurationNameValue = jsonStringToken(configuration, "ConfigurationName");
      String addressValue = jsonStringToken(configuration, "RTIaddress");
      String additionalSettingsValue = jsonStringToken(configuration, "AdditionalSettings");
      String optionalInternalData = jsonObjectRecord(configuration, "OptionalInternalData");
      JavaTckSupport.check(jsonStringLiteral(callbackModel.name()).equals(callbackModelValue)
            && jsonStringLiteral(configurationName).equals(configurationNameValue)
            && jsonStringLiteral(rtiAddress).equals(addressValue)
            && jsonStringLiteral(additionalSettings).equals(additionalSettingsValue)
            && "{}".equals(optionalInternalData),
         "ServiceReportInitialRecord Configuration did not preserve Connect values and "
            + "the empty OptionalInternalData PairList");

      Integer fomModuleCount = jsonStringArrayLength(
         federationAttributes, "HLAFOMmoduleDesignatorList");
      Boolean autoProvide = jsonBooleanValue(federationAttributes, "HLAautoProvide");
      JavaTckSupport.check(jsonStringLiteral(federation).equals(
               jsonStringToken(federationAttributes, "HLAfederationName"))
            && jsonStringToken(federationAttributes, "HLARTIversion") != null
            && jsonStringToken(federationAttributes, "HLAMIMDesignator") != null
            && fomModuleCount != null && fomModuleCount > 0
            && jsonStringLiteral(timeImplementation).equals(
               jsonStringToken(federationAttributes, "HLAtimeImplementationName"))
            && autoProvide != null,
         "ServiceReportInitialRecord FederationAttributes did not contain the six "
            + "Table 5 fields with the configured federation and logical-time values");

      String handleValue = jsonStringToken(federateAttributes, "HLAfederateHandle");
      Integer federateFomModuleCount = jsonStringArrayLength(
         federateAttributes, "HLAFOMmoduleDesignatorList");
      JavaTckSupport.check(handleValue != null && !"\"\"".equals(handleValue)
            && jsonStringLiteral(subjectName).equals(
               jsonStringToken(federateAttributes, "HLAfederateName"))
            && jsonStringLiteral(federateType).equals(
               jsonStringToken(federateAttributes, "HLAfederateType"))
            && jsonStringToken(federateAttributes, "HLAfederateHost") != null
            && federateFomModuleCount != null,
         "ServiceReportInitialRecord FederateAttributes did not contain its five "
            + "Table 5 fields with the joined federate name/type");
   }

   private static void requireSaveFailureRecord(String before, String after, String reason) {
      String record = appendedServiceRecord(before, after, "FederationSaved");
      String marker = "\"HLAargumentType\":48,\"HLAargumentName\":"
         + "\"Optional failure reason\",\"HLAargumentValue\":"
         + jsonStringLiteral(reason);
      JavaTckSupport.check(record.contains(marker),
         "HLAreportServiceFile did not append the type-48 " + reason + " Table 5 value");
   }

   private static void requireRestoreFailureRecord(String before, String after) {
      String record = appendedServiceRecord(before, after, "FederationRestored");
      String failureMarker = "\"HLAargumentType\":45,\"HLAargumentName\":"
         + "\"Optional failure reason\",\"HLAargumentValue\":\"RESTORE_ABORTED\"";
      String successMarker = "\"HLAargumentName\":\"Federation restore-success indicator\","
         + "\"HLAargumentValue\":false";
      JavaTckSupport.check(record.contains(failureMarker) && record.contains(successMarker),
         "HLAreportServiceFile did not append the failed FederationRestored result "
            + "and type-45 RESTORE_ABORTED Table 5 value");
   }

   private static String appendedServiceRecord(String before, String after, String service) {
      int servicePosition = after.indexOf(
         "\"HLAservice\":\"" + service + "\"", before.length());
      JavaTckSupport.check(servicePosition >= 0,
         "HLAreportServiceFile did not append " + service);
      int previousLineBreak = after.lastIndexOf('\n', servicePosition);
      int recordStart = previousLineBreak < 0 ? 0 : previousLineBreak + 1;
      int recordEnd = after.indexOf('\n', servicePosition);
      if (recordEnd < 0) recordEnd = after.length();
      return after.substring(recordStart, recordEnd).trim();
   }

   private static void requireNoEvokedSaveFailureBeforeFile(CallbackModel model,
         FederationRecorder subject, int subjectBefore, FederationRecorder observer,
         int observerBefore, String description) {
      if (model == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.check(subject.saveFailures.size() == subjectBefore
               && observer.saveFailures.size() == observerBefore,
            "Evoked federationNotSaved callback ran before inspecting " + description);
      }
   }

   private static void requireNoEvokedRestoreFailureBeforeFile(CallbackModel model,
         FederationRecorder subject, int subjectBefore, FederationRecorder observer,
         int observerBefore) {
      if (model == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.check(subject.restoreFailures.size() == subjectBefore
               && observer.restoreFailures.size() == observerBefore,
            "Evoked federationNotRestored callback ran before service-report file inspection");
      }
   }

   private static boolean hasSaveInitiationSince(FederationRecorder recorder,
         int previousCount, String label) {
      return recorder.saveInitiations.size() == previousCount + 1
         && label.equals(recorder.saveInitiations.get(previousCount));
   }

   private static boolean hasRestoreInitiationSince(FederationRecorder recorder,
         int previousCount, String label) {
      return recorder.restoreInitiations.size() == previousCount + 1
         && label.equals(recorder.restoreInitiations.get(previousCount));
   }

   private static boolean hasRestoreSuccessSince(FederationRecorder recorder,
         int previousCount, String label) {
      return recorder.restoreSuccesses.size() == previousCount + 1
         && label.equals(recorder.restoreSuccesses.get(previousCount));
   }

   private static boolean hasSaveFailureSince(FederationRecorder recorder,
         int previousCount, SaveFailureReason expected) {
      return recorder.saveFailures.size() == previousCount + 1
         && recorder.saveFailures.get(previousCount) == expected;
   }

   private static boolean hasRestoreFailureSince(FederationRecorder recorder,
         int previousCount, RestoreFailureReason expected) {
      return recorder.restoreFailures.size() == previousCount + 1
         && recorder.restoreFailures.get(previousCount) == expected;
   }

   private static void await(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.01);
            if (second != null) second.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static String firstJsonObjectRecord(String document) {
      if (document.isEmpty() || document.charAt(0) != '{') return null;
      return balancedJsonObject(document, 0);
   }

   private static String jsonObjectRecord(String document, String key) {
      String marker = "\"" + key + "\":{";
      int member = document.indexOf(marker);
      if (member < 0) return null;
      return balancedJsonObject(document, member + marker.length() - 1);
   }

   private static String balancedJsonObject(String document, int begin) {
      int depth = 0;
      boolean inString = false;
      boolean escaped = false;
      for (int index = begin; index < document.length(); index++) {
         char character = document.charAt(index);
         if (inString) {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') inString = false;
         } else if (character == '"') {
            inString = true;
         } else if (character == '{') {
            depth++;
         } else if (character == '}') {
            if (depth == 0) return null;
            depth--;
            if (depth == 0) return document.substring(begin, index + 1);
         }
      }
      return null;
   }

   private static String jsonStringToken(String document, String key) {
      String marker = "\"" + key + "\":";
      int member = document.indexOf(marker);
      if (member < 0) return null;
      int begin = member + marker.length();
      if (begin >= document.length() || document.charAt(begin) != '"') return null;
      boolean escaped = false;
      for (int index = begin + 1; index < document.length(); index++) {
         char character = document.charAt(index);
         if (escaped) escaped = false;
         else if (character == '\\') escaped = true;
         else if (character == '"') return document.substring(begin, index + 1);
      }
      return null;
   }

   private static Integer jsonStringArrayLength(String document, String key) {
      String marker = "\"" + key + "\":";
      int member = document.indexOf(marker);
      if (member < 0) return null;
      int index = member + marker.length();
      if (index >= document.length() || document.charAt(index++) != '[') return null;
      index = skipWhitespace(document, index);
      if (index < document.length() && document.charAt(index) == ']') return 0;
      Set<String> values = new HashSet<>();
      int count = 0;
      while (index < document.length()) {
         if (document.charAt(index) != '"') return null;
         int tokenBegin = index;
         index++;
         boolean escaped = false;
         boolean closed = false;
         while (index < document.length()) {
            char character = document.charAt(index++);
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') {
               closed = true;
               break;
            }
         }
         if (!closed || !values.add(document.substring(tokenBegin, index))) return null;
         count++;
         index = skipWhitespace(document, index);
         if (index >= document.length()) return null;
         if (document.charAt(index) == ']') return count;
         if (document.charAt(index++) != ',') return null;
         index = skipWhitespace(document, index);
      }
      return null;
   }

   private static int skipWhitespace(String document, int index) {
      while (index < document.length()) {
         char character = document.charAt(index);
         if (character != ' ' && character != '\t' && character != '\r' && character != '\n') {
            break;
         }
         index++;
      }
      return index;
   }

   private static Boolean jsonBooleanValue(String document, String key) {
      String marker = "\"" + key + "\":";
      int member = document.indexOf(marker);
      if (member < 0) return null;
      int begin = member + marker.length();
      if (document.startsWith("true", begin)) return Boolean.TRUE;
      if (document.startsWith("false", begin)) return Boolean.FALSE;
      return null;
   }

   private static String jsonStringLiteral(String value) {
      StringBuilder result = new StringBuilder("\"");
      for (int index = 0; index < value.length(); index++) {
         char character = value.charAt(index);
         switch (character) {
            case '"': result.append("\\\""); break;
            case '\\': result.append("\\\\"); break;
            case '\b': result.append("\\b"); break;
            case '\f': result.append("\\f"); break;
            case '\n': result.append("\\n"); break;
            case '\r': result.append("\\r"); break;
            case '\t': result.append("\\t"); break;
            default:
               if (character < 0x20) {
                  result.append(String.format("\\u%04x", (int) character));
               } else {
                  result.append(character);
               }
         }
      }
      return result.append('"').toString();
   }

   private static String decode(EncoderFactory encoder, byte[] bytes) throws Exception {
      JavaTckSupport.check(bytes != null,
         "HLAreportServiceFile reflection omitted the report-file value");
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(bytes);
      return value.getValue();
   }

   private static Object objectMethod(Object proxy, String method, Object[] arguments) {
      if ("toString".equals(method)) return "Java TCK service-report save/restore callbacks";
      if ("hashCode".equals(method)) return System.identityHashCode(proxy);
      if ("equals".equals(method)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap source) {
         this.object = object;
         this.values = new LinkedHashMap<>();
         for (AttributeHandle attribute : source.keySet()) {
            values.put(attribute, source.get(attribute).clone());
         }
      }
   }

   private static final class MomRecorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private ObjectInstanceHandle objectForFederate(AttributeHandle nameAttribute, byte[] name) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(nameAttribute);
            if (value != null && Arrays.equals(name, value)) return reflection.object;
         }
         return null;
      }

      private boolean hasAttributeForFederate(AttributeHandle nameAttribute, byte[] name,
            AttributeHandle attribute) {
         ObjectInstanceHandle object = objectForFederate(nameAttribute, name);
         return object != null && valuesForObject(object).containsKey(attribute);
      }

      private Map<AttributeHandle, byte[]> valuesForObject(ObjectInstanceHandle object) {
         Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
         if (object == null) return values;
         for (Reflection reflection : reflections) {
            if (object.equals(reflection.object)) values.putAll(reflection.values);
         }
         return values;
      }

      private FederateAmbassador proxy(FederationRecorder federationRecorder) {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reflectAttributeValues".equals(method.getName()) && arguments != null
                     && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1]));
               }
               federationRecorder.record(method.getName(), arguments);
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }

   private static final class FederationRecorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<RestoreFailureReason> restoreFailures = new CopyOnWriteArrayList<>();
      private volatile int savedCount;
      private volatile int restoreBegunCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               record(method.getName(), arguments);
               return objectMethod(proxy, method.getName(), arguments);
            });
      }

      private void record(String name, Object[] arguments) {
         if ("initiateFederateSave".equals(name) && arguments != null
               && arguments.length >= 1) {
            saveInitiations.add((String) arguments[0]);
         } else if ("federationSaved".equals(name)) {
            savedCount++;
         } else if ("federationNotSaved".equals(name) && arguments != null
               && arguments.length == 1) {
            saveFailures.add((SaveFailureReason) arguments[0]);
         } else if ("requestFederationRestoreSucceeded".equals(name)
               && arguments != null && arguments.length == 1) {
            restoreSuccesses.add((String) arguments[0]);
         } else if ("federationRestoreBegun".equals(name)) {
            restoreBegunCount++;
         } else if ("initiateFederateRestore".equals(name)
               && arguments != null && arguments.length >= 1) {
            restoreInitiations.add((String) arguments[0]);
         } else if ("federationNotRestored".equals(name) && arguments != null
               && arguments.length == 1) {
            restoreFailures.add((RestoreFailureReason) arguments[0]);
         }
      }
   }
}
