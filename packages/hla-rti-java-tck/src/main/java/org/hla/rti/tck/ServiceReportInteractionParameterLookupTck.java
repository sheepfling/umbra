package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import hla.rti1516_2025.RtiConfiguration;
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
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies interaction-class and parameter lookup results through standard MOM reports. */
final class ServiceReportInteractionParameterLookupTck {
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

   private ServiceReportInteractionParameterLookupTck() { }

   static void run(RtiFactory factory, String fom, String foreignFom, String mim,
         String interactionName,
         String parameterName, String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration subjectConfiguration, String additionalSettings,
         String serviceReportDirectory) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-parameter-" + UUID.randomUUID();
      String foreignFederation = federation + "-foreign";
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean foreignConnected = false, foreignCreated = false, foreignJoined = false;
      boolean fileReportingRequested =
         serviceReportDirectoryRequested(additionalSettings, serviceReportDirectory);
      InteractionClassHandle reportClass = null;
      ObjectClassHandle federateMomClass = null;
      AttributeHandleSet federateMomAttributes = null;
      boolean federateMomSubscribed = false;
      Path reportFile = null;
      String subjectName = "java-tck-interaction-parameter-subject-" + UUID.randomUUID();
      JavaTckSupport.check(foreignFom != null && !foreignFom.isEmpty(),
         "interaction/parameter lookup report case requires an adapter-supplied foreign FOM");
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
         foreign.connect(emptyCallbacks(), callbackModel);
         foreignConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         foreign.createFederationExecutionWithMIM(
            foreignFederation, new String[] {foreignFom}, mim, timeImplementation);
         foreignCreated = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution("java-tck-interaction-parameter-observer", "java-tck",
            federation);
         observerJoined = true;
         foreign.joinFederationExecution("java-tck-interaction-parameter-foreign", "java-tck",
            foreignFederation);
         foreignJoined = true;

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
            AttributeHandle finalReportFileAttribute = reportFileAttribute;
            await(observer, recorder, callbackModel,
               () -> recorder.hasAttributeForFederate(
                  federateNameAttribute, encodedSubjectName, finalReportFileAttribute),
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
         TransportationTypeHandle reliable = subject.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(subject.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         InteractionClassHandle interaction = subject.getInteractionClassHandle(interactionName);
         ParameterHandle parameter = subject.getParameterHandle(interaction, parameterName);
         InteractionClassHandle unrelatedInteraction = subject.getInteractionClassHandle(
            "HLAinteractionRoot.TckTransportRegionalInteraction");
         ParameterHandle unrelatedParameter = subject.getParameterHandle(
            unrelatedInteraction, "RegionalPayload");
         InteractionClassHandle foreignInteraction = foreign.getInteractionClassHandle(
            "HLAinteractionRoot.TckExtensionInteraction");
         JavaTckSupport.check(interaction != null && parameter != null,
            "adapter-supplied interaction/parameter lookup returned a null standard handle");
         JavaTckSupport.check(unrelatedInteraction != null && unrelatedParameter != null
               && foreignInteraction != null
               && "HLAinteractionRoot.TckTransportRegionalInteraction".equals(
                  subject.getInteractionClassName(unrelatedInteraction))
               && "RegionalPayload".equals(
                  subject.getParameterName(unrelatedInteraction, unrelatedParameter))
               && "HLAinteractionRoot.TckExtensionInteraction".equals(
                  foreign.getInteractionClassName(foreignInteraction)),
            "same-federation or foreign-FOM interaction handles did not resolve by public API");
         subject.setServiceReportingSwitch(true);
         InteractionClassHandle interactionByName =
            subject.getInteractionClassHandle(interactionName);
         String interactionResolvedName = subject.getInteractionClassName(interaction);
         ParameterHandle parameterByName = subject.getParameterHandle(interaction, parameterName);
         String parameterResolvedName = subject.getParameterName(interaction, parameter);
         JavaTckSupport.check(interaction.equals(interactionByName)
               && interactionName.equals(interactionResolvedName)
               && parameter.equals(parameterByName)
               && parameterName.equals(parameterResolvedName),
            "interaction/parameter lookup did not round-trip the adapter-supplied names");

         String missingInteractionName = interactionName + "-missing";
         String missingParameterName = parameterName + "-missing";
         JavaTckSupport.expectFailure(
            () -> subject.getParameterHandle(interaction, missingParameterName),
            "NameNotFound", "reported unknown parameter-name lookup");
         JavaTckSupport.expectFailure(
            () -> subject.getParameterHandle(foreignInteraction, parameterName),
            "InvalidInteractionClassHandle", "reported parameter lookup with a foreign class");
         JavaTckSupport.expectFailure(
            () -> subject.getParameterName(foreignInteraction, parameter),
            "InvalidInteractionClassHandle", "reported parameter-name lookup with a foreign class");
         JavaTckSupport.expectFailure(
            () -> subject.getInteractionClassName(foreignInteraction),
            "InvalidInteractionClassHandle", "reported foreign interaction-class name lookup");
         JavaTckSupport.expectFailure(
            () -> subject.getInteractionClassHandle(missingInteractionName),
            "NameNotFound", "reported unknown interaction-class name lookup");
         JavaTckSupport.expectFailure(
            () -> subject.getParameterName(interaction, unrelatedParameter),
            "InteractionParameterNotDefined", "reported parameter defined on another class");
         await(observer, recorder, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 10,
            "interaction/parameter lookups produced an unexpected report count");

         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetInteractionClassHandle",
            new Argument[] {new Argument(53, "Interaction class name", quoted(interactionName))},
            new Argument(27, "Interaction class handle", quoted(interactionByName.toString())),
            0, true, "");
         verifyReport(observer, subjectFederate, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable, "GetInteractionClassName",
            new Argument[] {new Argument(27, "Interaction class handle",
               quoted(interaction.toString()))},
            new Argument(53, "Interaction class name", quoted(interactionResolvedName)),
            1, true, "");
         verifyReport(observer, subjectFederate, recorder.events.get(2), encoder, reportClass,
            reportParameters, reliable, "GetParameterHandle",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(interaction.toString())),
               new Argument(53, "Parameter name", quoted(parameterName))
            },
            new Argument(39, "Parameter handle", quoted(parameterByName.toString())),
            2, true, "");
         verifyReport(observer, subjectFederate, recorder.events.get(3), encoder, reportClass,
            reportParameters, reliable, "GetParameterName",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(interaction.toString())),
               new Argument(39, "Parameter handle", quoted(parameter.toString()))
            },
            new Argument(53, "Parameter name", quoted(parameterResolvedName)),
            3, true, "");

         verifyFailureReport(observer, subjectFederate, recorder.events.get(4), encoder,
            reportClass, reportParameters, reliable, "GetParameterHandle",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(interaction.toString())),
               new Argument(53, "Parameter name", quoted(missingParameterName))
            }, "NameNotFound", 4);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(5), encoder,
            reportClass, reportParameters, reliable, "GetParameterHandle",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(foreignInteraction.toString())),
               new Argument(53, "Parameter name", quoted(parameterName))
            }, "InvalidInteractionClassHandle", 5);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(6), encoder,
            reportClass, reportParameters, reliable, "GetParameterName",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(foreignInteraction.toString())),
               new Argument(39, "Parameter handle", quoted(parameter.toString()))
            }, "InvalidInteractionClassHandle", 6);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(7), encoder,
            reportClass, reportParameters, reliable, "GetInteractionClassName",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(foreignInteraction.toString()))
            }, "InvalidInteractionClassHandle", 7);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(8), encoder,
            reportClass, reportParameters, reliable, "GetInteractionClassHandle",
            new Argument[] {
               new Argument(53, "Interaction class name", quoted(missingInteractionName))
            }, "NameNotFound", 8);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(9), encoder,
            reportClass, reportParameters, reliable, "GetParameterName",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(interaction.toString())),
               new Argument(39, "Parameter handle", quoted(unrelatedParameter.toString()))
            }, "InteractionParameterNotDefined", 9);

         if (reportFile != null) {
            observer.unsubscribeInteractionClass(reportClass);
            reportSubscribed = false;
            subject.setServiceReportingSwitch(false);
            subject.setSendServiceReportsToFileSwitch(true);
            subject.setServiceReportingSwitch(true);
            String previousContents = Files.readString(reportFile, StandardCharsets.UTF_8);
            InteractionClassHandle fileReportedInteraction =
               subject.getInteractionClassHandle(interactionName);
            String afterContents = Files.readString(reportFile, StandardCharsets.UTF_8);
            JavaTckSupport.check(interactionByName.equals(fileReportedInteraction),
               "File-backed interaction-class lookup returned a different handle");
            int servicePosition = afterContents.indexOf(
               "\"HLAservice\":\"GetInteractionClassHandle\"", previousContents.length());
            int recordLineBreak = afterContents.lastIndexOf('\n', servicePosition);
            int recordStart = recordLineBreak < 0 ? 0 : recordLineBreak + 1;
            int recordEnd = servicePosition < 0 ? -1 : afterContents.indexOf('\n', servicePosition);
            String record = servicePosition < 0 ? ""
               : afterContents.substring(recordStart, recordEnd < 0 ? afterContents.length()
                  : recordEnd);
            String handleText = fileReportedInteraction.toString();
            String expectedHandleArgument = "\"HLAargumentType\":27,"
               + "\"HLAargumentValue\":\"" + handleText + "\"";
            JavaTckSupport.check(servicePosition >= previousContents.length()
                  && record.contains(expectedHandleArgument),
               "GetInteractionClassHandle file report omitted its Table 5 type-27 value");
            subject.setServiceReportingSwitch(false);
            subject.setSendServiceReportsToFileSwitch(false);
         }

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         if (federateMomSubscribed) {
            observer.unsubscribeObjectClassAttributes(
               federateMomClass, federateMomAttributes);
            federateMomSubscribed = false;
         }
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
         foreign.resignFederationExecution(ResignAction.NO_ACTION);
         foreignJoined = false;
         foreign.destroyFederationExecution(foreignFederation);
         foreignCreated = false;
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

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial,
         boolean expectedSuccess, String expectedException)
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
            && serviceType.getValue() == 6 && success.getValue() == expectedSuccess
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

   private static void verifyFailureReport(RTIambassador observer,
         FederateHandle expectedFederate, Interaction event, EncoderFactory encoder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         TransportationTypeHandle reliable, String service, Argument[] expectedSupplied,
         String expectedException, int serial) throws Exception {
      verifyReport(observer, expectedFederate, event, encoder, reportClass, parameters, reliable,
         service, expectedSupplied, new Argument(34, "", "null"), serial,
         false, expectedException);
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
      await(observer, recorder, model, () -> recorder.events.size() >= 10,
         "interaction/parameter lookup service reports");
   }

   private static void await(RTIambassador observer, Recorder recorder, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK interaction lookup callbacks";
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
}
