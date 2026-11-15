package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.ServiceGroup;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies object, attribute, and update-rate lookup results through standard MOM reports. */
final class ServiceReportObjectAttributeUpdateRateLookupTck {
   private static final String FEDERATE_MOM_CLASS =
      "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String REPORT_DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportObjectAttributeUpdateRateLookupTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String updateRateName,
         CallbackModel callbackModel, RtiConfiguration subjectConfiguration,
         String additionalSettings, String serviceReportDirectory) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-object-attribute-rate-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean published = false;
      InteractionClassHandle reportClass = null;
      ObjectClassHandle objectClass = null;
      ObjectClassHandle federateMomClass = null;
      AttributeHandle attribute = null;
      ObjectInstanceHandle object = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeHandleSet federateMomAttributes = null;
      boolean federateMomSubscribed = false;
      boolean fileReportingRequested =
         serviceReportDirectoryRequested(additionalSettings, serviceReportDirectory);
      Path reportFile = null;
      String subjectName = "java-tck-object-attribute-rate-subject-" + UUID.randomUUID();
      if (fileReportingRequested) {
         validateServiceReportDirectory(
            subjectConfiguration, additionalSettings, serviceReportDirectory);
      }
      try {
         if (fileReportingRequested) {
            subject.connect(emptyCallbacks(), callbackModel, subjectConfiguration);
         } else {
            subject.connect(emptyCallbacks(), callbackModel);
         }
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution("java-tck-object-attribute-rate-observer", "java-tck",
            federation);
         observerJoined = true;

         if (fileReportingRequested) {
            federateMomClass = observer.getObjectClassHandle(FEDERATE_MOM_CLASS);
            AttributeHandle federateNameAttribute = observer.getAttributeHandle(
               federateMomClass, "HLAfederateName");
            AttributeHandle reportFileAttribute = observer.getAttributeHandle(
               federateMomClass, "HLAreportServiceFile");
            JavaTckSupport.check(FEDERATE_MOM_CLASS.equals(
                  observer.getObjectClassName(federateMomClass))
                  && "HLAfederateName".equals(
                     observer.getAttributeName(federateMomClass, federateNameAttribute))
                  && "HLAreportServiceFile".equals(
                     observer.getAttributeName(federateMomClass, reportFileAttribute)),
               "Federate MOM report-file handles did not round-trip by public name");
            federateMomAttributes = JavaTckSupport.attributeSet(
               observer, federateNameAttribute, reportFileAttribute);
            observer.subscribeObjectClassAttributes(
               federateMomClass, federateMomAttributes, "");
            federateMomSubscribed = true;
            byte[] encodedSubjectName =
               encoder.createHLAunicodeString(subjectName).toByteArray();
            await(observer, recorder, callbackModel,
               () -> recorder.hasAttributeForFederate(
                  federateNameAttribute, encodedSubjectName, reportFileAttribute),
               "subject HLAreportServiceFile MOM reflection");
            ObjectInstanceHandle subjectMomObject = recorder.objectForFederate(
               federateNameAttribute, encodedSubjectName);
            String reportFileName = decode(encoder,
               recorder.valuesForObject(subjectMomObject).get(reportFileAttribute));
            JavaTckSupport.check(reportFileName != null && !reportFileName.trim().isEmpty(),
               "subject HLAreportServiceFile reflection did not contain a file name");
            reportFile = Path.of(reportFileName);
            JavaTckSupport.check(Files.isRegularFile(reportFile)
                  && !Files.readString(reportFile, StandardCharsets.UTF_8).isEmpty(),
               "subject HLAreportServiceFile did not identify a file with its initial record");
         }

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

         objectClass = subject.getObjectClassHandle(objectClassName);
         attribute = subject.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && attributeName.equals(subject.getAttributeName(objectClass, attribute)),
            "adapter-supplied object class or attribute lookup/name did not round-trip");
         publishedAttributes = JavaTckSupport.attributeSet(subject, attribute);
         subject.publishObjectClassAttributes(objectClass, publishedAttributes);
         published = true;
         object = subject.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "adapter-supplied object class registration returned a null handle");
         String objectName = subject.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "registered object did not have a standard instance name");

         subject.setServiceReportingSwitch(true);
         ObjectClassHandle knownObjectClass = subject.getKnownObjectClassHandle(object);
         ObjectInstanceHandle objectByName = subject.getObjectInstanceHandle(objectName);
         String resolvedObjectName = subject.getObjectInstanceName(object);
         AttributeHandle attributeByClass = subject.getAttributeHandle(objectClass, attributeName);
         String resolvedAttributeName = subject.getAttributeName(objectClass, attribute);
         double maximumRate = subject.getUpdateRateValue(updateRateName);
         double attributeMaximumRate = subject.getUpdateRateValueForAttribute(object, attribute);
         JavaTckSupport.check(objectClass.equals(knownObjectClass)
               && object.equals(objectByName) && objectName.equals(resolvedObjectName)
               && attribute.equals(attributeByClass)
               && attributeName.equals(resolvedAttributeName)
               && maximumRate > 0.0 && attributeMaximumRate == 0.0,
            "object, attribute, or update-rate lookup returned an unexpected value");
         await(observer, recorder, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 7,
            "object/attribute/update-rate lookups produced an unexpected report count");

         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetKnownObjectClassHandle",
            new Argument[] {new Argument(37, "Object instance handle", quoted(object.toString()))},
            new Argument(36, "Object class handle", quoted(knownObjectClass.toString())), 0);
         verifyReport(observer, subjectFederate, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable, "GetObjectInstanceHandle",
            new Argument[] {new Argument(53, "Object instance name", quoted(objectName))},
            new Argument(37, "Object instance handle", quoted(objectByName.toString())), 1);
         verifyReport(observer, subjectFederate, recorder.events.get(2), encoder, reportClass,
            reportParameters, reliable, "GetObjectInstanceName",
            new Argument[] {new Argument(37, "Object instance handle", quoted(object.toString()))},
            new Argument(53, "Object instance name", quoted(resolvedObjectName)), 2);
         verifyReport(observer, subjectFederate, recorder.events.get(3), encoder, reportClass,
            reportParameters, reliable, "GetAttributeHandle",
            new Argument[] {
               new Argument(36, "Object class handle", quoted(objectClass.toString())),
               new Argument(53, "Class attribute name", quoted(attributeName))
            },
            new Argument(0, "Class attribute handle", quoted(attributeByClass.toString())), 3);
         verifyReport(observer, subjectFederate, recorder.events.get(4), encoder, reportClass,
            reportParameters, reliable, "GetAttributeName",
            new Argument[] {
               new Argument(36, "Object class handle", quoted(objectClass.toString())),
               new Argument(0, "Class attribute handle", quoted(attribute.toString()))
            },
            new Argument(53, "Class attribute name", quoted(resolvedAttributeName)), 4);
         verifyReport(observer, subjectFederate, recorder.events.get(5), encoder, reportClass,
            reportParameters, reliable, "GetUpdateRateValue",
            new Argument[] {new Argument(53, "Update rate name", quoted(updateRateName))},
            new Argument(35, "Maximum update rate value", formatRate(maximumRate)), 5);
         verifyReport(observer, subjectFederate, recorder.events.get(6), encoder, reportClass,
            reportParameters, reliable, "GetUpdateRateValueForAttribute",
            new Argument[] {
               new Argument(37, "Object instance handle", quoted(object.toString())),
               new Argument(0, "Attribute handle", quoted(attribute.toString()))
            },
            new Argument(35, "Maximum update rate value", formatRate(attributeMaximumRate)), 6);

         if (reportFile != null) {
            observer.unsubscribeInteractionClass(reportClass);
            reportSubscribed = false;
            String previousContents = Files.readString(reportFile, StandardCharsets.UTF_8);
            subject.setSendServiceReportsToFileSwitch(true);
            double fileMaximumRate = subject.getUpdateRateValue(updateRateName);
            String afterContents = Files.readString(reportFile, StandardCharsets.UTF_8);
            JavaTckSupport.check(fileMaximumRate == maximumRate,
               "GetUpdateRateValue returned a different result during the file-report check");
            int servicePosition = afterContents.indexOf(
               "\"HLAservice\":\"GetUpdateRateValue\"", previousContents.length());
            int recordLineBreak = afterContents.lastIndexOf('\n', servicePosition);
            int recordStart = recordLineBreak < 0 ? 0 : recordLineBreak + 1;
            int recordEnd = servicePosition < 0 ? -1 : afterContents.indexOf('\n', servicePosition);
            String record = servicePosition < 0 ? ""
               : afterContents.substring(recordStart, recordEnd < 0 ? afterContents.length()
                  : recordEnd);
            String numberArgumentPrefix = "\"HLAargumentType\":35,\"HLAargumentValue\":";
            int numberArgumentPosition = record.indexOf(numberArgumentPrefix);
            int numberValueStart = numberArgumentPosition < 0 ? -1
               : numberArgumentPosition + numberArgumentPrefix.length();
            int numberValueComma = numberValueStart < 0 ? -1
               : record.indexOf(',', numberValueStart);
            int numberValueClose = numberValueStart < 0 ? -1
               : record.indexOf('}', numberValueStart);
            int numberValueEnd = numberValueComma < 0 ? numberValueClose
               : numberValueClose < 0 ? numberValueComma
               : Math.min(numberValueComma, numberValueClose);
            String numberValue = numberValueStart < 0 || numberValueEnd < 0 ? ""
               : record.substring(numberValueStart, numberValueEnd);
            JavaTckSupport.check(servicePosition >= previousContents.length()
                  && numberArgumentPosition >= 0
                  && record.contains("\"HLAsuccessIndicator\":true")
                  && formatRate(fileMaximumRate).equals(numberValue),
               "GetUpdateRateValue file report did not return its successful type-35 Number");
         }

         if (reportSubscribed) {
            observer.unsubscribeInteractionClass(reportClass);
            reportSubscribed = false;
         }
         if (federateMomSubscribed) {
            observer.unsubscribeObjectClassAttributes(
               federateMomClass, federateMomAttributes);
            federateMomSubscribed = false;
         }
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         if (published) {
            subject.unpublishObjectClassAttributes(objectClass, publishedAttributes);
            published = false;
         }
         subject.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (federateMomSubscribed && observerJoined && federateMomClass != null
               && federateMomAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(
                  federateMomClass, federateMomAttributes);
            } catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         try { subject.setSendServiceReportsToFileSwitch(false); }
         catch (Exception ignored) { }
         if (published && subjectJoined && objectClass != null && publishedAttributes != null) {
            try { subject.unpublishObjectClassAttributes(objectClass, publishedAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static boolean serviceReportDirectoryRequested(
         String additionalSettings, String serviceReportDirectory) {
      return serviceReportDirectory != null && !serviceReportDirectory.isEmpty()
         || additionalSettings != null
            && additionalSettings.contains(REPORT_DIRECTORY_SETTING);
   }

   private static void validateServiceReportDirectory(
         RtiConfiguration subjectConfiguration, String additionalSettings,
         String serviceReportDirectory) {
      JavaTckSupport.check(subjectConfiguration != null,
         "service-report directory configuration is required");
      JavaTckSupport.check(serviceReportDirectory != null && !serviceReportDirectory.isEmpty()
            && Files.isDirectory(Path.of(serviceReportDirectory)),
         "adapter-supplied service-report directory must exist");
      String expectedDirectorySetting = REPORT_DIRECTORY_SETTING
         + Path.of(serviceReportDirectory);
      JavaTckSupport.check(additionalSettings != null
            && Arrays.asList(additionalSettings.split(";")).contains(expectedDirectorySetting),
         "adapter did not configure the expected serviceReportDirectory setting");
   }

   private static String decode(EncoderFactory encoder, byte[] bytes) throws Exception {
      JavaTckSupport.check(bytes != null,
         "HLAreportServiceFile reflection omitted the report-file value");
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(bytes);
      return value.getValue();
   }

   static void runDimensionUpperBound(RtiFactory factory, String fom, String mim,
         String additionalFom, String timeImplementation, String dimensionName,
         CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-upper-bound-" + UUID.randomUUID();
      String foreignFederation = federation + "-foreign";
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean foreignConnected = false, foreignCreated = false, foreignJoined = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         foreign.connect(emptyCallbacks(), callbackModel);
         foreignConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         foreign.createFederationExecutionWithMIM(
            foreignFederation, new String[] {fom, additionalFom}, mim, timeImplementation);
         foreignCreated = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-dimension-upper-bound-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-upper-bound-observer", "java-tck", federation);
         observerJoined = true;
         foreign.joinFederationExecution(
            "java-tck-dimension-upper-bound-foreign", "java-tck", foreignFederation);
         foreignJoined = true;

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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null
               && dimensionName.equals(subject.getDimensionName(dimension)),
            "adapter-supplied dimension lookup/name round-trip failed");
         DimensionHandle foreignDimension = foreign.getDimensionHandle("TckAdditionalDimension");
         JavaTckSupport.check(foreignDimension != null
               && "TckAdditionalDimension".equals(foreign.getDimensionName(foreignDimension)),
            "additional FOM did not resolve the foreign dimension handle");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension upper-bound test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound == 64L,
            "dimension upper-bound lookup did not return the adapter FOM's bound of 64");

         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionUpperBound delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetDimensionUpperBound",
            new Argument[] {new Argument(10, "Dimension handle", quoted(dimension.toString()))},
            new Argument(35, "Dimension upper bound", Long.toString(upperBound)), 0);

         JavaTckSupport.expectFailure(
            () -> subject.getDimensionUpperBound(foreignDimension),
            "InvalidDimensionHandle",
            "GetDimensionUpperBound with a dimension from another FOM execution");
         awaitReportCount(observer, recorder, callbackModel, 2,
            "failed GetDimensionUpperBound service report");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 2,
            "GetDimensionUpperBound delivered an unexpected failure-report count");
         verifyReport(observer, subjectFederate, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable, "GetDimensionUpperBound",
            new Argument[] {new Argument(10, "Dimension handle", null)},
            new Argument(34, "", "null"), 1, 6, true, false,
            "InvalidDimensionHandle");

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
         if (foreignJoined) resign(foreign);
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
         if (foreignConnected) disconnect(foreign);
      }
   }

   static void runDimensionName(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-name-" + UUID.randomUUID();
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
            "java-tck-dimension-name-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-name-observer", "java-tck", federation);
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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter-supplied dimension lookup returned a null handle");
         String resolvedName = subject.getDimensionName(dimension);
         JavaTckSupport.check(dimensionName.equals(resolvedName),
            "dimension-name lookup did not return the adapter FOM's dimension name");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension-name test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);
         String reportedName = subject.getDimensionName(dimension);
         JavaTckSupport.check(resolvedName.equals(reportedName),
            "reported dimension-name lookup returned a different name");

         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionName delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetDimensionName",
            new Argument[] {new Argument(10, "Dimension handle", quoted(dimension.toString()))},
            new Argument(53, "Dimension name", quoted(reportedName)), 0);

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

   static void runDimensionHandle(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-handle-" + UUID.randomUUID();
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
            "java-tck-dimension-handle-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-handle-observer", "java-tck", federation);
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
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension-handle test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionHandle delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetDimensionHandle",
            new Argument[] {new Argument(53, "Dimension name", quoted(dimensionName))},
            new Argument(10, "Dimension handle", quoted(dimension.toString())), 0);

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

   static void runDimensionHandleSet(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-handle-set-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
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
            "java-tck-dimension-handle-set-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-handle-set-observer", "java-tck", federation);
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
         observer.setConveyRegionDesignatorSetsSwitch(true);

         long reportedFederateValue = observer.normalizeFederateHandle(subjectFederate);
         long supportServicesGroup = observer.normalizeServiceGroup(ServiceGroup.SUPPORT_SERVICES);
         JavaTckSupport.check(reportedFederateValue >= 0 && reportedFederateValue < Long.MAX_VALUE
               && supportServicesGroup >= 0 && supportServicesGroup < Long.MAX_VALUE,
            "service-report routing point value cannot be represented as a half-open range");

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension-handle-set test could not select MOM delivery");
         InteractionClassHandle expectedReportClass = reportClass;
         recorder.setReceiveInteractionHook(event -> {
            JavaTckSupport.check(expectedReportClass.equals(event.interaction),
               "dimension-handle-set callback hook received the wrong interaction class");
            JavaTckSupport.check(event.regions != null && event.regions.size() == 1,
               "HLAreportServiceInvocation callback did not contain exactly one region designator");
            Map<String, hla.rti1516_2025.RangeBounds> callbackDimensionBounds =
               new LinkedHashMap<>();
            for (RegionHandle callbackRegion : event.regions) {
               DimensionHandleSet callbackDimensions;
               try {
                  callbackDimensions = observer.getDimensionHandleSet(callbackRegion);
               } catch (Exception exception) {
                  throw new IllegalStateException(
                     "GetDimensionHandleSet rejected the service-report callback region", exception);
               }
               for (DimensionHandle callbackDimension : callbackDimensions) {
                  String callbackDimensionName = observer.getDimensionName(callbackDimension);
                  hla.rti1516_2025.RangeBounds callbackBounds =
                     observer.getRangeBounds(callbackRegion, callbackDimension);
                  JavaTckSupport.check(callbackDimensionBounds.put(callbackDimensionName,
                        callbackBounds) == null,
                     "HLAreportServiceInvocation callback region repeated dimension "
                        + callbackDimensionName);
               }
            }
            JavaTckSupport.check(callbackDimensionBounds.size() == 2
                  && callbackDimensionBounds.containsKey("HLAfederate")
                  && callbackDimensionBounds.containsKey("HLAserviceGroup"),
               "HLAreportServiceInvocation callback region did not contain exactly HLAfederate and HLAserviceGroup");
            hla.rti1516_2025.RangeBounds federateBounds =
               callbackDimensionBounds.get("HLAfederate");
            JavaTckSupport.check(federateBounds.lower == reportedFederateValue
                  && federateBounds.upper == reportedFederateValue + 1,
               "HLAreportServiceInvocation callback region used the wrong normalized HLAfederate range");
            hla.rti1516_2025.RangeBounds serviceGroupBounds =
               callbackDimensionBounds.get("HLAserviceGroup");
            JavaTckSupport.check(serviceGroupBounds.lower == supportServicesGroup
                  && serviceGroupBounds.upper == supportServicesGroup + 1,
               "HLAreportServiceInvocation callback region used the wrong SUPPORT_SERVICES range");
         });
         subject.setServiceReportingSwitch(true);

         DimensionHandleSet returnedDimensions;
         try {
            returnedDimensions = subject.getDimensionHandleSet(region);
         } catch (Exception exception) {
            throw new IllegalStateException(
               "GetDimensionHandleSet rejected the subject's newly created region", exception);
         }
         JavaTckSupport.check(returnedDimensions.size() == 1
               && returnedDimensions.contains(dimension),
            "GetDimensionHandleSet did not return exactly the region's configured dimension");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionHandleSet delivered an unexpected number of service reports");
         Interaction report = recorder.events.get(0);
         recorder.checkReceiveInteractionHook();
         verifyReport(observer, subjectFederate, report, encoder, reportClass,
            reportParameters, reliable, "GetDimensionHandleSet",
            new Argument[] {new Argument(42, "Region handle", quoted(region.toString()))},
            new Argument(11, "A set of dimensions",
               "[\"" + dimension.toString() + "\"]"), 0, 6, false);

         subject.setServiceReportingSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
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
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
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

   static void runRangeBounds(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-range-bounds-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
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
            "java-tck-range-bounds-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-range-bounds-observer", "java-tck", federation);
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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound > 0,
            "range-bounds report requires a positive adapter dimension upper bound");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         long lowerBound = upperBound > 1 ? 1 : 0;
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(lowerBound, upperBound));
         RegionHandleSet committedRegions = subject.getRegionHandleSetFactory().create();
         committedRegions.add(region);
         subject.commitRegionModifications(committedRegions);
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "range-bounds test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);

         hla.rti1516_2025.RangeBounds actualBounds = subject.getRangeBounds(region, dimension);
         JavaTckSupport.check(actualBounds.lower == lowerBound
               && actualBounds.upper == upperBound,
            "GetRangeBounds did not return the adapter region's committed bounds");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetRangeBounds delivered an unexpected number of service reports");
         String boundsText = "{\"lower\":" + lowerBound + ",\"upper\":" + upperBound + "}";
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetRangeBounds",
            new Argument[] {
               new Argument(42, "Region handle", quoted(region.toString())),
               new Argument(10, "Dimension handle", quoted(dimension.toString()))
            }, new Argument(41, "Range bounds", boundsText), 0, 5);

         subject.setServiceReportingSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
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
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
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

   static void runSetRangeBounds(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-set-range-bounds-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
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
            "java-tck-set-range-bounds-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-set-range-bounds-observer", "java-tck", federation);
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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound > 0,
            "SetRangeBounds report requires a positive adapter dimension upper bound");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(0, upperBound));
         RegionHandleSet committedRegions = subject.getRegionHandleSetFactory().create();
         committedRegions.add(region);
         subject.commitRegionModifications(committedRegions);
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "SetRangeBounds test could not select MOM delivery");

         long lowerBound = upperBound > 1 ? 1 : 0;
         subject.setServiceReportingSwitch(true);
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(lowerBound, upperBound));
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "SetRangeBounds delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "SetRangeBounds",
            new Argument[] {
               new Argument(42, "Region handle", quoted(region.toString())),
               new Argument(10, "Dimension handle", quoted(dimension.toString())),
               new Argument(35, "Range lower bound", Long.toString(lowerBound)),
               new Argument(35, "Range upper bound", Long.toString(upperBound))
            }, new Argument(34, "", "null"), 0, 5);

         subject.setServiceReportingSwitch(false);
         hla.rti1516_2025.RangeBounds actualBounds = subject.getRangeBounds(region, dimension);
         JavaTckSupport.check(actualBounds.lower == lowerBound
               && actualBounds.upper == upperBound,
            "SetRangeBounds did not retain the adapter-valid bounds");
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
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
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
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

   static void runCommitRegionModifications(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-commit-region-modifications-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      boolean secondRegionCreated = false;
      RegionHandle region = null;
      RegionHandle secondRegion = null;
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
            "java-tck-commit-region-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-commit-region-observer", "java-tck", federation);
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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound > 0,
            "CommitRegionModifications report requires a positive adapter dimension upper bound");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         secondRegion = subject.createRegion(regionDimensions);
         secondRegionCreated = true;
         JavaTckSupport.check(secondRegion != null && !region.equals(secondRegion),
            "CommitRegionModifications fixture did not create two distinct regions");
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(0, upperBound));
         subject.setRangeBounds(secondRegion, dimension,
            new hla.rti1516_2025.RangeBounds(0, upperBound));
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "CommitRegionModifications test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         RegionHandleSet committedRegions = subject.getRegionHandleSetFactory().create();
         committedRegions.add(region);
         committedRegions.add(secondRegion);
         JavaTckSupport.check(committedRegions.size() == 2,
            "CommitRegionModifications fixture did not retain two distinct regions");
         subject.commitRegionModifications(committedRegions);
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "CommitRegionModifications delivered an unexpected number of service reports");
         String regionSetText = "[\"" + region.toString() + "\",\""
            + secondRegion.toString() + "\"]";
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "CommitRegionModifications",
            new Argument[] {
               new Argument(43, "Set of region designators",
                  regionSetText)
            }, new Argument(34, "", "null"), 0, 5);

         subject.setServiceReportingSwitch(false);
         hla.rti1516_2025.RangeBounds committedBounds = subject.getRangeBounds(region, dimension);
         hla.rti1516_2025.RangeBounds secondCommittedBounds =
            subject.getRangeBounds(secondRegion, dimension);
         JavaTckSupport.check(committedBounds.lower == 0
               && committedBounds.upper == upperBound
               && secondCommittedBounds.lower == 0
               && secondCommittedBounds.upper == upperBound,
            "CommitRegionModifications did not commit bounds for both adapter-valid regions");
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
         subject.deleteRegion(secondRegion);
         secondRegionCreated = false;
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
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
         if (secondRegionCreated && secondRegion != null) {
            try { subject.deleteRegion(secondRegion); }
            catch (Exception ignored) { }
         }
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

   static void runDeleteRegion(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-delete-region-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
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
            "java-tck-delete-region-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-delete-region-observer", "java-tck", federation);
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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "DeleteRegion test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         subject.deleteRegion(region);
         regionCreated = false;
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "DeleteRegion delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "DeleteRegion",
            new Argument[] {
               new Argument(42, "Region designator", quoted(region.toString()))
            }, new Argument(34, "", "null"), 0, 5);

         subject.setServiceReportingSwitch(false);
         RegionHandle deletedRegion = region;
         JavaTckSupport.expectFailure(() -> subject.getDimensionHandleSet(deletedRegion),
            "InvalidRegion", "looking up dimensions after region deletion");
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
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
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
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

   static void runCreateRegion(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-create-region-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
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
            "java-tck-create-region-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-create-region-observer", "java-tck", federation);
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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "CreateRegion test could not select MOM delivery");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);

         subject.setServiceReportingSwitch(true);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "CreateRegion delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "CreateRegion",
            new Argument[] {
               new Argument(11, "Set of dimension designators",
                  "[\"" + dimension.toString() + "\"]")
            }, new Argument(42, "Region designator", quoted(region.toString())), 0, 5);

         subject.setServiceReportingSwitch(false);
         subject.deleteRegion(region);
         regionCreated = false;
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
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

   static void runInteractionDimensions(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      runInteractionDimensionsInternal(factory, fom, mim, null, timeImplementation,
         dimensionName, interactionClassName, callbackModel);
   }

   static void runInteractionDimensionsInvalidClassContext(RtiFactory factory, String fom,
         String mim, String additionalFom, String timeImplementation, String dimensionName,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      runInteractionDimensionsInternal(factory, fom, mim, additionalFom, timeImplementation,
         dimensionName, interactionClassName, callbackModel);
   }

   private static void runInteractionDimensionsInternal(RtiFactory factory, String fom, String mim,
         String additionalFom, String timeImplementation, String dimensionName,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = additionalFom == null ? null : factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-dimensions-" + UUID.randomUUID();
      String foreignFederation = federation + "-foreign";
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean foreignConnected = false, foreignCreated = false, foreignJoined = false;
      InteractionClassHandle reportClass = null;
      InteractionClassHandle foreignInteractionClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         if (foreign != null) {
            foreign.connect(emptyCallbacks(), callbackModel);
            foreignConnected = true;
         }
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         if (foreign != null) {
            foreign.createFederationExecutionWithMIM(
               foreignFederation, new String[] {additionalFom}, mim, timeImplementation);
            foreignCreated = true;
         }
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-interaction-dimensions-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-interaction-dimensions-observer", "java-tck", federation);
         observerJoined = true;
         if (foreign != null) {
            foreign.joinFederationExecution(
               "java-tck-interaction-dimensions-foreign", "java-tck", foreignFederation);
            foreignJoined = true;
            foreignInteractionClass = foreign.getInteractionClassHandle(
               "HLAinteractionRoot.TckExtensionInteraction");
            JavaTckSupport.check(foreignInteractionClass != null
                  && "HLAinteractionRoot.TckExtensionInteraction".equals(
                     foreign.getInteractionClassName(foreignInteractionClass)),
               "additional FOM did not resolve the foreign interaction-class handle");
         }

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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         InteractionClassHandle interactionClass =
            subject.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interactionClass != null
               && interactionClassName.equals(subject.getInteractionClassName(interactionClass)),
            "adapter FOM interaction-class lookup/name round-trip failed");
         InteractionClassHandle rootInteractionClass =
            subject.getInteractionClassHandle("HLAinteractionRoot");
         JavaTckSupport.check(rootInteractionClass != null
               && "HLAinteractionRoot".equals(
                  subject.getInteractionClassName(rootInteractionClass)),
            "standard root interaction-class lookup/name round-trip failed");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "interaction-dimensions test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         DimensionHandleSet availableDimensions =
            subject.getAvailableDimensionsForInteractionClass(interactionClass);
         JavaTckSupport.check(availableDimensions != null
               && availableDimensions.size() == 1 && availableDimensions.contains(dimension),
            "adapter FOM interaction did not expose exactly its configured dimension");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetAvailableDimensionsForInteractionClass delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetAvailableDimensionsForInteractionClass",
            new Argument[] {
                new Argument(27, "Interaction class handle", null)
            }, new Argument(11, "A set of dimension handles", null), 0);

         DimensionHandleSet rootDimensions =
            subject.getAvailableDimensionsForInteractionClass(rootInteractionClass);
         JavaTckSupport.check(rootDimensions != null && rootDimensions.isEmpty(),
            "HLAinteractionRoot unexpectedly has available dimensions");
         awaitReportCount(observer, recorder, callbackModel, 2,
            "empty GetAvailableDimensionsForInteractionClass service report");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 2,
            "GetAvailableDimensionsForInteractionClass delivered an unexpected report count");
         verifyReport(observer, subjectFederate, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable, "GetAvailableDimensionsForInteractionClass",
            new Argument[] {
               new Argument(27, "Interaction class handle", null)
            }, new Argument(11, "A set of dimension handles", "[]"), 1);

         if (foreignInteractionClass != null) {
            InteractionClassHandle contextInvalidInteractionClass = foreignInteractionClass;
            JavaTckSupport.expectFailure(
               () -> subject.getAvailableDimensionsForInteractionClass(
                  contextInvalidInteractionClass),
               "InvalidInteractionClassHandle",
               "GetAvailableDimensionsForInteractionClass with an interaction class from another FOM execution");
            awaitReportCount(observer, recorder, callbackModel, 3,
               "failed GetAvailableDimensionsForInteractionClass service report");
            if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
            JavaTckSupport.check(recorder.events.size() == 3,
               "GetAvailableDimensionsForInteractionClass delivered an unexpected failure-report count");
            verifyReport(observer, subjectFederate, recorder.events.get(2), encoder, reportClass,
               reportParameters, reliable, "GetAvailableDimensionsForInteractionClass",
               new Argument[] {
                  new Argument(27, "Interaction class handle", null)
               }, new Argument(34, "", "null"), 2, 6, true, false,
               "InvalidInteractionClassHandle");
         }

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
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
         if (foreignJoined) resign(foreign);
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (foreignConnected) disconnect(foreign);
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runObjectDimensions(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      runObjectDimensionsInternal(factory, fom, mim, null, timeImplementation,
         dimensionName, objectClassName, callbackModel);
   }

   static void runObjectDimensionsInvalidClassContext(RtiFactory factory, String fom, String mim,
         String additionalFom, String timeImplementation, String dimensionName,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      runObjectDimensionsInternal(factory, fom, mim, additionalFom, timeImplementation,
         dimensionName, objectClassName, callbackModel);
   }

   private static void runObjectDimensionsInternal(RtiFactory factory, String fom, String mim,
         String additionalFom, String timeImplementation, String dimensionName,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = additionalFom == null ? null : factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-object-dimensions-" + UUID.randomUUID();
      String foreignFederation = federation + "-foreign";
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean foreignConnected = false, foreignCreated = false, foreignJoined = false;
      InteractionClassHandle reportClass = null;
      ObjectClassHandle foreignObjectClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         if (foreign != null) {
            foreign.connect(emptyCallbacks(), callbackModel);
            foreignConnected = true;
         }
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         if (foreign != null) {
            foreign.createFederationExecutionWithMIM(
               foreignFederation, new String[] {additionalFom}, mim, timeImplementation);
            foreignCreated = true;
         }
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-object-dimensions-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-object-dimensions-observer", "java-tck", federation);
         observerJoined = true;
         if (foreign != null) {
            foreign.joinFederationExecution(
               "java-tck-object-dimensions-foreign", "java-tck", foreignFederation);
            foreignJoined = true;
            foreignObjectClass = foreign.getObjectClassHandle(
               "HLAobjectRoot.TckExtensionObject");
            JavaTckSupport.check(foreignObjectClass != null
                  && "HLAobjectRoot.TckExtensionObject".equals(
                     foreign.getObjectClassName(foreignObjectClass)),
               "additional FOM did not resolve the foreign object-class handle");
         }

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

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(objectClass != null
               && objectClassName.equals(subject.getObjectClassName(objectClass)),
            "adapter FOM object-class lookup/name round-trip failed");
         ObjectClassHandle rootObjectClass;
         try {
            rootObjectClass = subject.getObjectClassHandle("HLAobjectRoot");
            JavaTckSupport.check(rootObjectClass != null
                  && "HLAobjectRoot".equals(subject.getObjectClassName(rootObjectClass)),
               "standard root object-class lookup/name round-trip failed");
         } catch (Exception exception) {
            throw new IllegalStateException("HLAobjectRoot handle lookup or name round-trip failed",
               exception);
         }
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "object-dimensions test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         DimensionHandleSet availableDimensions =
            subject.getAvailableDimensionsForObjectClass(objectClass);
         JavaTckSupport.check(availableDimensions != null
               && availableDimensions.size() == 1 && availableDimensions.contains(dimension),
            "adapter FOM object class did not expose exactly its configured dimension");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetAvailableDimensionsForObjectClass delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetAvailableDimensionsForObjectClass",
            new Argument[] {
               new Argument(36, "Object class handle", null)
            }, new Argument(11, "A set of dimension handles", null), 0);

         DimensionHandleSet rootDimensions;
         try {
            rootDimensions = subject.getAvailableDimensionsForObjectClass(rootObjectClass);
         } catch (Exception exception) {
            throw new IllegalStateException("empty HLAobjectRoot dimension query failed", exception);
         }
         JavaTckSupport.check(rootDimensions != null && rootDimensions.isEmpty(),
            "HLAobjectRoot unexpectedly has available dimensions");
         awaitReportCount(observer, recorder, callbackModel, 2,
            "empty GetAvailableDimensionsForObjectClass service report");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 2,
            "GetAvailableDimensionsForObjectClass delivered an unexpected report count");
         try {
            verifyReport(observer, subjectFederate, recorder.events.get(1), encoder, reportClass,
               reportParameters, reliable, "GetAvailableDimensionsForObjectClass",
               new Argument[] {
                  new Argument(36, "Object class handle", null)
               }, new Argument(11, "A set of dimension handles", "[]"), 1);
         } catch (Exception exception) {
            throw new IllegalStateException("empty HLAobjectRoot service-report verification failed",
               exception);
         }

         if (foreignObjectClass != null) {
            ObjectClassHandle contextInvalidObjectClass = foreignObjectClass;
            JavaTckSupport.expectFailure(
               () -> subject.getAvailableDimensionsForObjectClass(contextInvalidObjectClass),
               "InvalidObjectClassHandle",
               "GetAvailableDimensionsForObjectClass with an object class from another FOM execution");
            awaitReportCount(observer, recorder, callbackModel, 3,
               "failed GetAvailableDimensionsForObjectClass service report");
            if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
            JavaTckSupport.check(recorder.events.size() == 3,
               "GetAvailableDimensionsForObjectClass delivered an unexpected failure-report count");
            verifyReport(observer, subjectFederate, recorder.events.get(2), encoder, reportClass,
               reportParameters, reliable, "GetAvailableDimensionsForObjectClass",
               new Argument[] {
                  new Argument(36, "Object class handle", null)
               }, new Argument(34, "", "null"), 2, 6, true, false,
               "InvalidObjectClassHandle");
         }

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
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
         if (foreignJoined) resign(foreign);
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (foreignConnected) disconnect(foreign);
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial)
         throws Exception {
      verifyReport(observer, expectedFederate, event, encoder, reportClass, parameters,
         reliable, service, expectedSupplied, expectedReturned, serial, 6);
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial,
         int expectedServiceType) throws Exception {
      verifyReport(observer, expectedFederate, event, encoder, reportClass, parameters,
         reliable, service, expectedSupplied, expectedReturned, serial, expectedServiceType, true);
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial,
         int expectedServiceType, boolean expectedRegionsEmpty) throws Exception {
      verifyReport(observer, expectedFederate, event, encoder, reportClass, parameters, reliable,
         service, expectedSupplied, expectedReturned, serial, expectedServiceType,
         expectedRegionsEmpty, true, "");
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial,
         int expectedServiceType, boolean expectedRegionsEmpty, boolean expectedSuccess,
         String expectedException) throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length
            && event.tag.length == 0 && event.producer == null
            && event.regionsEmpty == expectedRegionsEmpty
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
            && serviceType.getValue() == expectedServiceType
            && success.getValue() == expectedSuccess
            && (expectedSuccess ? exception.getValue().isEmpty()
               : exception.getValue().contains(expectedException))
            && serialNumber.getValue() == serial,
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
      JavaTckSupport.check(record.size() == 3,
         description + " expected three fields but received " + record.size());
      int actualType = ((HLAinteger32BE) record.get(0)).getValue();
      String actualName = ((HLAunicodeString) record.get(1)).getValue();
      String actualValue = ((HLAunicodeString) record.get(2)).getValue();
      boolean valueMatches;
      if (expected.value != null) {
         valueMatches = expected.value.equals(actualValue);
      } else if (expected.type == 11 && "A set of dimension handles".equals(expected.name)) {
         valueMatches = actualValue.startsWith("[\"") && actualValue.endsWith("\"]")
            && actualValue.length() > 4
            && !actualValue.substring(2, actualValue.length() - 2).contains("\",\"");
      } else {
         valueMatches = actualValue.length() >= 2 && actualValue.startsWith("\"")
            && actualValue.endsWith("\"");
      }
      JavaTckSupport.check(actualType == expected.type
            && expected.name.equals(actualName) && valueMatches,
         description + " expected=(" + expected.type + ", " + expected.name + ", "
            + (expected.value == null ? "quoted provider handle" : expected.value)
            + ") actual=(" + actualType + ", " + actualName + ", "
            + actualValue + ")");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static String formatRate(double value) {
      return String.format(Locale.ROOT, "%.6f", value);
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static void await(RTIambassador observer, Recorder recorder, CallbackModel model)
         throws Exception {
      await(observer, recorder, model, () -> recorder.events.size() >= 7,
         "object/attribute/update-rate service reports");
   }

   private static void await(RTIambassador observer, Recorder recorder, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitOne(RTIambassador observer, Recorder recorder, CallbackModel model)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(!recorder.events.isEmpty(),
         "timed out waiting for GetDimensionUpperBound service report");
   }

   private static void awaitReportCount(RTIambassador observer, Recorder recorder,
         CallbackModel model, int count, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.events.size() < count && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.events.size() >= count,
         "timed out waiting for " + description);
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK object lookup callbacks";
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
      private final RegionHandleSet regions;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction,
            Map<ParameterHandle, byte[]> parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : regions.clone();
         this.regionsEmpty = this.regions == null || this.regions.isEmpty();
      }
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

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private volatile InteractionHook receiveInteractionHook;
      private volatile Throwable receiveInteractionHookFailure;

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

      private void setReceiveInteractionHook(InteractionHook hook) {
         receiveInteractionHook = hook;
      }

      private void checkReceiveInteractionHook() {
         JavaTckSupport.check(receiveInteractionHookFailure == null,
            "receiveInteraction callback verification failed: " + receiveInteractionHookFailure);
      }

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
                  Interaction event = new Interaction((InteractionClassHandle) arguments[0], values,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], regions);
                  events.add(event);
                  InteractionHook hook = receiveInteractionHook;
                  if (hook != null) {
                     try {
                        hook.receive(event);
                     } catch (Throwable failure) {
                        receiveInteractionHookFailure = failure;
                     }
                  }
               }
               if ("reflectAttributeValues".equals(method.getName()) && arguments != null
                     && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1]));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }

   @FunctionalInterface
   private interface InteractionHook {
      void receive(Interaction interaction) throws Exception;
   }
}
