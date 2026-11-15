package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import hla.rti1516_2025.exceptions.InteractionParameterNotDefined;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.function.BooleanSupplier;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Checks standard federation MOM FOM-module and MIM content reports. */
final class FederationMomContentReportsTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST_ROOT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.";
   private static final String REPORT_ROOT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.";
   private static final String XML_NAME = "(?:[A-Za-z_][A-Za-z_0-9.-]*:)?";
   private static final Pattern XML_LEAF = Pattern.compile(
      "<([A-Za-z_][A-Za-z_0-9.:-]*)\\b[^>]*>([^<>]*)</\\1\\s*>");
   private static final Pattern MODEL_IDENTIFICATION = Pattern.compile(
      "<" + XML_NAME + "modelIdentification\\b[^>]*>([\\s\\S]*?)</"
         + XML_NAME + "modelIdentification\\s*>");
   private static final Pattern REFERENCE = Pattern.compile(
      "<" + XML_NAME + "reference\\b[^>]*>([\\s\\S]*?)</"
         + XML_NAME + "reference\\s*>");
   private static final Pattern IDENTIFICATION = Pattern.compile(
      "<" + XML_NAME + "identification\\b[^>]*>([^<>]*)</"
         + XML_NAME + "identification\\s*>");
   private static final Pattern NAME_ELEMENT = Pattern.compile(
      "<" + XML_NAME + "name\\b[^>]*>([^<>]*)</" + XML_NAME + "name\\s*>");
   private static final Pattern ARRAY_DATA_TYPES = Pattern.compile(
      "<arrayDataTypes\\b[^>]*>([\\s\\S]*?)</arrayDataTypes\\s*>");
   private static final Pattern ARRAY_DATA = Pattern.compile(
      "<arrayData\\b[^>]*>([\\s\\S]*?)</arrayData\\s*>");
   private static final Pattern SEMANTICS_ELEMENT = Pattern.compile(
      "<semantics\\b[^>]*>([^<>]*)</semantics\\s*>");

   private FederationMomContentReportsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String additionalFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(additionalFom != null && !additionalFom.isEmpty(),
         "the federation MOM content-report scenario requires a second FOM module");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-content-reports-" + UUID.randomUUID();
      boolean ownerConnected = false, observerConnected = false, created = false;
      boolean ownerJoined = false, observerJoined = false;
      boolean fomSubscribed = false, mimSubscribed = false, federationMomSubscribed = false;
      InteractionClassHandle fomReport = null, mimReport = null;
      ObjectClassHandle federationMomClass = null;
      AttributeHandleSet currentFddAttributes = null;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom, additionalFom}, mim, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-content-report-owner", "tck-type", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-content-report-observer", "tck-type",
            federation);
         observerJoined = true;

         InteractionClassHandle fomRequest = owner.getInteractionClassHandle(
            REQUEST_ROOT + "HLArequestFOMmoduleData");
         ParameterHandle fomIndicator = owner.getParameterHandle(
            fomRequest, "HLAFOMmoduleIndicator");
         InteractionClassHandle mimRequest = owner.getInteractionClassHandle(
            REQUEST_ROOT + "HLArequestMIMdata");
         fomReport = observer.getInteractionClassHandle(REPORT_ROOT + "HLAreportFOMmoduleData");
         ParameterHandle reportIndicator = observer.getParameterHandle(
            fomReport, "HLAFOMmoduleIndicator");
         ParameterHandle fomData = observer.getParameterHandle(fomReport, "HLAFOMmoduleData");
         mimReport = observer.getInteractionClassHandle(REPORT_ROOT + "HLAreportMIMdata");
         InteractionClassHandle standardMimReport = mimReport;
         ParameterHandle mimData = observer.getParameterHandle(mimReport, "HLAMIMdata");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         federationMomClass = observer.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         AttributeHandle currentFddAttribute = federationMomClass == null ? null
            : observer.getAttributeHandle(federationMomClass, "HLAcurrentFDD");
         JavaTckSupport.check(fomRequest != null && fomIndicator != null && mimRequest != null
               && fomReport != null && reportIndicator != null && fomData != null
               && mimReport != null && mimData != null && reliable != null
               && federationMomClass != null && currentFddAttribute != null,
            "standard federation MOM content-report lookup returned a null handle");
         JavaTckSupport.check((REQUEST_ROOT + "HLArequestFOMmoduleData").equals(
               owner.getInteractionClassName(fomRequest))
               && "HLAFOMmoduleIndicator".equals(owner.getParameterName(fomRequest, fomIndicator))
               && (REQUEST_ROOT + "HLArequestMIMdata").equals(
                  owner.getInteractionClassName(mimRequest))
               && (REPORT_ROOT + "HLAreportFOMmoduleData").equals(
                  observer.getInteractionClassName(fomReport))
               && "HLAFOMmoduleData".equals(observer.getParameterName(fomReport, fomData))
               && (REPORT_ROOT + "HLAreportMIMdata").equals(
                  observer.getInteractionClassName(mimReport))
               && "HLAMIMdata".equals(observer.getParameterName(mimReport, mimData))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard federation MOM content-report handle names did not round-trip");

         JavaTckSupport.expectFailure(
            () -> owner.getInteractionClassHandle(REQUEST_ROOT + "HLArequestMIMData"),
            "NameNotFound", "Clause 11 HLArequestMIMData spelling");
         JavaTckSupport.expectFailure(
            () -> observer.getInteractionClassHandle(REPORT_ROOT + "HLAreportMIMData"),
            "NameNotFound", "Clause 11 HLAreportMIMData spelling");
         JavaTckSupport.expectFailure(
            () -> observer.getParameterHandle(standardMimReport, "HLAMIMData"),
            "NameNotFound", "Clause 11 HLAMIMData spelling");

         currentFddAttributes = JavaTckSupport.attributeSet(observer, currentFddAttribute);
         observer.subscribeObjectClassAttributes(federationMomClass, currentFddAttributes);
         federationMomSubscribed = true;
         await(observer, () -> recorder.discoveredObject != null,
            "standard federation MOM object discovery");
         ObjectInstanceHandle federationObject = recorder.discoveredObject;
         JavaTckSupport.check(federationMomClass.equals(recorder.discoveredClass)
               && "HLAfederation".equals(recorder.discoveredName)
               && federationMomClass.equals(
                  observer.getKnownObjectClassHandle(federationObject))
               && "HLAfederation".equals(observer.getObjectInstanceName(federationObject))
               && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
            "federation MOM discovery did not preserve the standard object identity");
         recorder.clearReflection();
         observer.requestAttributeValueUpdate(federationObject, currentFddAttributes, new byte[0]);
         await(observer, () -> recorder.currentFdd(encoder,
               currentFddAttribute) != null,
            "composed Current FDD Object Model Identification table");
         JavaTckSupport.check(federationObject.equals(recorder.reflectedObject)
               && recorder.reflectedValues != null && recorder.reflectedValues.size() == 1
               && recorder.reflectedValues.containsKey(currentFddAttribute)
               && recorder.reflectionTag != null && recorder.reflectionTag.length == 0
               && reliable.equals(recorder.reflectionTransportation)
               && recorder.reflectionRegionsEmpty,
            "Current FDD reflection returned non-standard MOM metadata");
         JavaTckSupport.requireNoProducingFederate(observer, recorder.reflectionProducer,
            "Current FDD reflection");
         checkCurrentFddModuleReferences(recorder.currentFdd(encoder, currentFddAttribute),
            fom, additionalFom);

         observer.subscribeInteractionClass(fomReport);
         fomSubscribed = true;
         observer.subscribeInteractionClass(mimReport);
         mimSubscribed = true;

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            sendFomRequest(owner, fomRequest, fomIndicator, encoder, 0);
            observer.unsubscribeInteractionClass(fomReport);
            fomSubscribed = false;
            settle(observer, callbackModel);
            JavaTckSupport.check(recorder.events.isEmpty(),
               "unsubscribed FOM-module report was delivered while callbacks were serviced");
            observer.subscribeInteractionClass(fomReport);
            fomSubscribed = true;
         } else {
            sendFomRequest(owner, fomRequest, fomIndicator, encoder, 0);
            awaitCount(observer, recorder, 1, "initial FOM-module report delivery");
            assertReport(observer, recorder.events.get(0), fomReport, reliable,
               "initial FOM-module report");
            observer.unsubscribeInteractionClass(fomReport);
            fomSubscribed = false;
            observer.subscribeInteractionClass(fomReport);
            fomSubscribed = true;
            recorder.events.clear();
         }

         checkFomModuleReport(owner, observer, recorder, encoder, fomRequest, fomIndicator,
            fomReport, reportIndicator, fomData, reliable, 0, fom, "TckObject");
         checkFomModuleReport(owner, observer, recorder, encoder, fomRequest, fomIndicator,
            fomReport, reportIndicator, fomData, reliable, 1, additionalFom,
            "TCK Service Usage Second");

         recorder.events.clear();
         sendRequest(owner, mimRequest, null, null);
         awaitCount(observer, recorder, 1, "MIM content report delivery");
         Interaction mimEvent = recorder.events.get(0);
         assertReport(observer, mimEvent, mimReport, reliable, "MIM content report");
         JavaTckSupport.check(mimEvent.parameters.size() == 1
               && mimEvent.parameters.containsKey(mimData),
            "MIM report did not contain exactly its standard content parameter");
         HLAunicodeString decodedMim = encoder.createHLAunicodeString();
         decodedMim.decode(mimEvent.parameters.get(mimData));
         checkMimContent(decodedMim.getValue(), mim);

         recorder.events.clear();
         ParameterHandleValueMap invalidMimParameters =
            owner.getParameterHandleValueMapFactory().create(1);
         invalidMimParameters.put(fomIndicator,
            encoder.createHLAinteger32BE(0).toByteArray());
         boolean rejectedInvalidParameter = false;
         try {
            owner.sendInteraction(mimRequest, invalidMimParameters, new byte[0]);
         } catch (InteractionParameterNotDefined expected) {
            rejectedInvalidParameter = true;
         }
         JavaTckSupport.check(rejectedInvalidParameter,
            "MIM request with a FOM-only parameter did not throw InteractionParameterNotDefined");
         settle(observer, callbackModel);
         JavaTckSupport.check(recorder.events.isEmpty(),
            "malformed MOM MIM request produced an application-level report");
         JavaTckSupport.check(producer != null,
            "standard report request producer handle was null");
      } finally {
         if (mimSubscribed && observerJoined) {
            try { observer.unsubscribeInteractionClass(mimReport); } catch (Exception ignored) { }
         }
         if (federationMomSubscribed && observerJoined) {
            try {
               observer.unsubscribeObjectClassAttributes(federationMomClass,
                  currentFddAttributes);
            } catch (Exception ignored) { }
         }
         if (fomSubscribed && observerJoined) {
            try { observer.unsubscribeInteractionClass(fomReport); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkFomModuleReport(RTIambassador owner, RTIambassador observer,
         Recorder recorder, EncoderFactory encoder, InteractionClassHandle fomRequest,
         ParameterHandle fomIndicator, InteractionClassHandle fomReport,
         ParameterHandle reportIndicator, ParameterHandle fomData,
         TransportationTypeHandle reliable, int moduleIndex, String modulePath,
         String expectedMarker) throws Exception {
      recorder.events.clear();
      sendFomRequest(owner, fomRequest, fomIndicator, encoder, moduleIndex);
      awaitCount(observer, recorder, 1, "FOM-module content report for index " + moduleIndex);
      JavaTckSupport.check(recorder.events.size() == 1,
         "FOM-module request delivered more than one report for index " + moduleIndex);
      Interaction event = recorder.events.get(0);
      assertReport(observer, event, fomReport, reliable,
         "FOM-module content report for index " + moduleIndex);
      JavaTckSupport.check(event.parameters.size() == 2
            && event.parameters.containsKey(reportIndicator)
            && event.parameters.containsKey(fomData),
         "FOM-module report did not contain exactly its two standard parameters");

      HLAinteger32BE decodedIndicator = encoder.createHLAinteger32BE();
      decodedIndicator.decode(event.parameters.get(reportIndicator));
      HLAunicodeString decodedFom = encoder.createHLAunicodeString();
      decodedFom.decode(event.parameters.get(fomData));
      JavaTckSupport.check(decodedIndicator.getValue() == moduleIndex,
         "FOM-module report returned the wrong module indicator");
      List<String> actual = xmlLeafContent(decodedFom.getValue());
      List<String> expected = xmlLeafContent(readUtf8(modulePath,
         "adapter FOM module " + moduleIndex));
      JavaTckSupport.check(!expected.isEmpty(),
         "adapter FOM module did not contain inspectable XML leaf content");
      JavaTckSupport.check(actual.equals(expected),
         "FOM-module report differed from source module " + moduleIndex
            + " ordered XML leaf content at element " + firstDifferent(actual, expected)
            + " (report elements " + actual.size() + ", source elements "
            + expected.size() + ")");
      JavaTckSupport.check(!expectedMarker.isEmpty()
            && decodedFom.getValue().contains(expectedMarker),
         "FOM-module report omitted its expected source-module marker");
   }

   private static void checkCurrentFddModuleReferences(String currentFdd,
         String primaryFom, String additionalFom) throws Exception {
      Matcher modelIdentification = MODEL_IDENTIFICATION.matcher(currentFdd);
      JavaTckSupport.check(modelIdentification.find(),
         "the composed Current FDD omitted its modelIdentification table");
      String modelIdentificationXml = modelIdentification.group(1);
      List<String> references = new ArrayList<>();
      Matcher reference = REFERENCE.matcher(modelIdentificationXml);
      while (reference.find()) {
         Matcher identification = IDENTIFICATION.matcher(reference.group(1));
         if (identification.find()) references.add(normalizeXmlText(identification.group(1)));
      }

      String primaryModel = sourceModelName(primaryFom, "primary FOM module");
      String additionalModel = sourceModelName(additionalFom, "second FOM module");
      JavaTckSupport.check(!primaryModel.equals(additionalModel)
            && count(references, primaryModel) == 1
            && count(references, additionalModel) == 1,
         "the composed Object Model Identification table omitted or duplicated a "
            + "contributing FOM module reference; actual=" + references
            + " expected-at-least=" + List.of(primaryModel, additionalModel)
            + " modelIdentification=" + modelIdentificationXml);
   }

   private static String sourceModelName(String modulePath, String description)
         throws Exception {
      String xml = readUtf8(modulePath, description);
      Matcher modelIdentification = MODEL_IDENTIFICATION.matcher(xml);
      JavaTckSupport.check(modelIdentification.find(),
         description + " omitted its modelIdentification table");
      Matcher name = NAME_ELEMENT.matcher(modelIdentification.group(1));
      JavaTckSupport.check(name.find(), description + " omitted its model name");
      String value = normalizeXmlText(name.group(1));
      JavaTckSupport.check(!value.isEmpty(), description + " supplied an empty model name");
      return value;
   }

   private static void checkMimContent(String actualMim, String mimPath) throws Exception {
      String expectedMim = readUtf8(mimPath, "pinned standard MIM");
      List<String> actual = xmlLeafContent(actualMim);
      List<String> expected = xmlLeafContent(expectedMim);
      JavaTckSupport.check(expected.size() > 1000,
         "standard MIM comparison did not inspect its expected leaf content");
      JavaTckSupport.check(actual.equals(expected),
         "federation MOM MIM report differed from the pinned IEEE 1516-2025 XML "
            + "leaf content at element " + firstDifferent(actual, expected)
            + " (report elements " + actual.size() + ", expected elements "
            + expected.size() + ")");
      JavaTckSupport.check(actualMim.contains("HLArequestMIMdata")
            && actualMim.contains("HLAreportMIMdata")
            && actualMim.contains(
               "Standard MOM and Initialization Module (MIM) for HLA IEEE 1516-2025"),
         "federation MOM MIM report omitted its Annex G MIM content");

      Matcher arrayTable = ARRAY_DATA_TYPES.matcher(actualMim);
      JavaTckSupport.check(arrayTable.find(),
         "standard federation MOM MIM report omitted the array-data-type table");
      String table = arrayTable.group(1);
      String[] names = {"HLAASCIIstring", "HLAunicodeString", "HLAopaqueData", "HLAtoken"};
      String[] semantics = {"ASCII string representation", "Unicode string representation",
         "Uninterpreted sequence of bytes", "Token representation"};
      Matcher entries = ARRAY_DATA.matcher(table);
      for (int index = 0; index < names.length; ++index) {
         JavaTckSupport.check(entries.find(),
            "standard federation MOM MIM report had too few leading array entries");
         Matcher name = NAME_ELEMENT.matcher(entries.group(1));
         Matcher meaning = SEMANTICS_ELEMENT.matcher(entries.group(1));
         JavaTckSupport.check(name.find() && meaning.find()
               && name.group(1).equals(names[index])
               && normalizeXmlText(meaning.group(1)).equals(semantics[index]),
            "standard federation MOM MIM report omitted or reordered predefined array "
               + names[index] + " or its standardized semantics");
      }
   }

   private static List<String> xmlLeafContent(String xml) {
      List<String> result = new ArrayList<>();
      Matcher leaf = XML_LEAF.matcher(xml);
      while (leaf.find()) {
         result.add(leaf.group(1) + ":" + normalizeXmlText(leaf.group(2)));
      }
      return result;
   }

   private static String normalizeXmlText(String value) {
      return value.trim().replaceAll("\\s+", " ");
   }

   private static String readUtf8(String path, String description) throws Exception {
      Path file = Path.of(path);
      JavaTckSupport.check(Files.isRegularFile(file),
         description + " does not exist: " + file);
      return new String(Files.readAllBytes(file), StandardCharsets.UTF_8);
   }

   private static int firstDifferent(List<String> actual, List<String> expected) {
      int index = 0;
      while (index < actual.size() && index < expected.size()
            && actual.get(index).equals(expected.get(index))) ++index;
      return index;
   }

   private static int count(List<String> values, String expected) {
      int result = 0;
      for (String value : values) if (expected.equals(value)) ++result;
      return result;
   }

   private static void sendFomRequest(RTIambassador ambassador,
         InteractionClassHandle request, ParameterHandle indicator, EncoderFactory encoder,
         int moduleIndex) throws Exception {
      HLAinteger32BE index = encoder.createHLAinteger32BE(moduleIndex);
      sendRequest(ambassador, request, indicator, index.toByteArray());
   }

   private static void sendRequest(RTIambassador ambassador, InteractionClassHandle request,
         ParameterHandle parameter, byte[] value) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(
         parameter == null ? 0 : 1);
      if (parameter != null) values.put(parameter, value);
      ambassador.sendInteraction(request, values, new byte[0]);
   }

   private static void assertReport(RTIambassador observer, Interaction event,
         InteractionClassHandle interaction,
         TransportationTypeHandle reliable, String description) throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.tag.length == 0 && reliable.equals(event.transportation)
            && event.regionsEmpty,
         description + " did not preserve class, empty tag, reliable transport, and no regions");
      JavaTckSupport.requireNoProducingFederate(observer, event.producer, description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) JavaTckSupport.drain(ambassador);
      if (model == CallbackModel.HLA_IMMEDIATE) Thread.yield();
   }

   private static void awaitCount(RTIambassador ambassador, Recorder recorder, int count,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && recorder.events.size() < count) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.events.size() >= count,
         "timed out waiting for " + description);
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK content-report callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Interaction> events = new ArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile ObjectClassHandle discoveredClass;
      private volatile String discoveredName;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile Map<AttributeHandle, byte[]> reflectedValues;
      private volatile byte[] reflectionTag;
      private volatile TransportationTypeHandle reflectionTransportation;
      private volatile FederateHandle reflectionProducer;
      private volatile boolean reflectionRegionsEmpty;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length >= 3) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  discoveredClass = (ObjectClassHandle) arguments[1];
                  discoveredName = (String) arguments[2];
               } else if ("reflectAttributeValues".equals(method.getName())
                     && arguments != null && arguments.length == 6
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  Map<AttributeHandle, byte[]> copy = new LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     copy.put(handle, source.get(handle).clone());
                  }
                  reflectedValues = copy;
                  reflectionTag = arguments[2] instanceof byte[]
                     ? ((byte[]) arguments[2]).clone() : null;
                  reflectionTransportation = (TransportationTypeHandle) arguments[3];
                  reflectionProducer = (FederateHandle) arguments[4];
                  reflectionRegionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
               } else if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  events.add(event);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }

      private void clearReflection() {
         reflectedObject = null;
         reflectedValues = null;
         reflectionTag = null;
         reflectionTransportation = null;
         reflectionProducer = null;
         reflectionRegionsEmpty = false;
      }

      private String currentFdd(EncoderFactory encoder, AttributeHandle attribute) {
         Map<AttributeHandle, byte[]> values = reflectedValues;
         if (values == null || !values.containsKey(attribute)) return null;
         try {
            HLAunicodeString currentFdd = encoder.createHLAunicodeString();
            currentFdd.decode(values.get(attribute));
            return currentFdd.getValue();
         } catch (Exception error) {
            throw new AssertionError("could not decode standard HLAcurrentFDD", error);
         }
      }
   }
}
