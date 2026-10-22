package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.function.BooleanSupplier;
import java.io.StringReader;
import javax.xml.XMLConstants;
import javax.xml.parsers.DocumentBuilderFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

final class FederationMomCurrentFddTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FederationMomCurrentFddTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String additionalFom,
         String timeImplementation, CallbackModel callbackModel, String federateType,
         String baseDeclaration, String extensionDeclaration) throws Exception {
      runImpl(factory, fom, mim, additionalFom, timeImplementation, callbackModel,
         federateType, baseDeclaration, extensionDeclaration, null, false);
   }

   static void runServiceUsageMerge(RtiFactory factory, String first, String second,
         String mim, String timeImplementation, CallbackModel callbackModel,
         String federateType) throws Exception {
      runImpl(factory, first, mim, second, timeImplementation, callbackModel,
         federateType, "TCK first service usage note.",
         "TCK second service usage note.", Boolean.FALSE, false);
      runImpl(factory, second, mim, first, timeImplementation, callbackModel,
         federateType, "TCK second service usage note.",
         "TCK first service usage note.", Boolean.TRUE, false);
   }

   static void runNoteCollisionRemap(RtiFactory factory, String first, String second,
         String mim, String timeImplementation, CallbackModel callbackModel,
         String federateType) throws Exception {
      runImpl(factory, first, mim, second, timeImplementation, callbackModel,
         federateType, "TCK first colliding note semantics.",
         "TCK second colliding note semantics.", null, true);
      runImpl(factory, second, mim, first, timeImplementation, callbackModel,
         federateType, "TCK second colliding note semantics.",
         "TCK first colliding note semantics.", null, true);
   }

   private static void runImpl(RtiFactory factory, String fom, String mim,
         String additionalFom, String timeImplementation, CallbackModel callbackModel,
         String federateType, String baseDeclaration, String extensionDeclaration,
         Boolean expectedBaseUsage, boolean expectNoteRemap) throws Exception {
      check(baseDeclaration != null && !baseDeclaration.isEmpty()
            && extensionDeclaration != null && !extensionDeclaration.isEmpty(),
         "current-FDD declaration markers must be adapter-configured and non-empty");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-current-fdd-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean extensionConnected = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean extensionJoined = false;
      boolean created = false;
      try {
         check(owner.connect(callbacks(), callbackModel) != null,
            "current-FDD owner connect returned null ConfigurationResult");
         ownerConnected = true;
         check(observer.connect(observerRecorder.proxy(), callbackModel) != null,
            "current-FDD observer connect returned null ConfigurationResult");
         observerConnected = true;
         check(extension.connect(callbacks(), callbackModel) != null,
            "current-FDD extension connect returned null ConfigurationResult");
         extensionConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution(federateType, federation);
         ownerJoined = true;
         observer.joinFederationExecution(federateType, federation);
         observerJoined = true;

         ObjectClassHandle federationClass = observer.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         AttributeHandle currentFddAttribute = observer.getAttributeHandle(
            federationClass, "HLAcurrentFDD");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         check(federationClass != null && currentFddAttribute != null && reliable != null,
            "standard federation MOM lookup returned a null handle");

         AttributeHandleSet currentFddAttributes =
            JavaTckSupport.attributeSet(observer, currentFddAttribute);
         observer.subscribeObjectClassAttributes(federationClass, currentFddAttributes);
         await(observer, callbackModel, () -> observerRecorder.discoveredObject != null,
            "standard federation MOM object discovery");
         ObjectInstanceHandle federationObject = observerRecorder.discoveredObject;
         check(federationClass.equals(observerRecorder.discoveredClass)
               && "HLAfederation".equals(observerRecorder.discoveredName),
            "federation MOM discovery reported the wrong class or object name");
         check(federationClass.equals(observer.getKnownObjectClassHandle(federationObject)),
            "federation MOM discovery returned the wrong known class");
         check("HLAfederation".equals(observer.getObjectInstanceName(federationObject))
               && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
            "federation MOM object identity did not round-trip");

         observerRecorder.clearTransportationReport();
         observer.queryAttributeTransportationType(federationObject, currentFddAttribute);
         await(observer, callbackModel,
            () -> reliable.equals(observerRecorder.reportedTransportation),
            "federation MOM transportation report");
         check(federationObject.equals(observerRecorder.reportedTransportationObject)
               && currentFddAttribute.equals(observerRecorder.reportedTransportationAttribute),
            "federation MOM transportation report identified the wrong object or attribute");

         String baseFdd = requestCurrentFdd(observer, observerRecorder, encoder,
            callbackModel, federationObject, currentFddAttribute, currentFddAttributes,
            reliable, "base current-FDD request");
         check(baseFdd.contains(baseDeclaration),
            "base current-FDD value omitted the adapter FOM declaration");
         check(!baseFdd.contains(extensionDeclaration),
            "base current-FDD value already contained the extension declaration");
         if (expectedBaseUsage != null) {
            check(disconnectServiceUsed(baseFdd) == expectedBaseUsage.booleanValue(),
               "base current FDD reported the wrong disconnect service usage");
         }

         observerRecorder.clearReflection();
         extension.joinFederationExecution("java-tck-fdd-extension", federateType, federation,
            new String[] {additionalFom});
         extensionJoined = true;
         await(observer, callbackModel, () -> {
            String current = observerRecorder.currentFdd(encoder, currentFddAttribute);
            return current != null && current.contains(extensionDeclaration);
         }, "current-FDD refresh following an additional-FOM join");
         String refreshedFdd = observerRecorder.currentFdd(encoder, currentFddAttribute);
         check(refreshedFdd != null && refreshedFdd.contains(baseDeclaration)
               && refreshedFdd.contains(extensionDeclaration)
               && refreshedFdd.length() > baseFdd.length(),
            "refreshed current-FDD value omitted the composed extension declaration");
         if (expectedBaseUsage != null) {
            check(disconnectServiceUsed(refreshedFdd),
               "composed current FDD did not logically OR disconnect service usage");
         }
         if (expectNoteRemap) requireDistinctReferencedNotes(refreshedFdd);

         String requestedFdd = requestCurrentFdd(observer, observerRecorder, encoder,
            callbackModel, federationObject, currentFddAttribute, currentFddAttributes,
            reliable, "post-composition current-FDD request");
         check(refreshedFdd.equals(requestedFdd),
            "direct current-FDD request disagreed with the join-triggered refresh");
      } finally {
         if (extensionJoined) resign(extension);
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (extensionConnected) disconnect(extension);
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   static boolean disconnectServiceUsed(String fdd) throws Exception {
      NodeList rows = parseFdd(fdd).getElementsByTagNameNS("*", "disconnect");
      Boolean found = null;
      for (int index = 0; index < rows.getLength(); index++) {
         Element row = (Element) rows.item(index);
         if (!"4.3".equals(row.getAttribute("section"))
               || !"false".equals(row.getAttribute("isCallback"))) continue;
         check(found == null, "current FDD duplicated the disconnect service row");
         String value = row.getAttribute("isUsed");
         check("true".equals(value) || "false".equals(value),
            "current FDD returned a non-Boolean disconnect usage value");
         found = Boolean.valueOf(value);
      }
      check(found != null, "current FDD omitted the adapter disconnect service row");
      return found.booleanValue();
   }

   private static Document parseFdd(String fdd) throws Exception {
      DocumentBuilderFactory builder = DocumentBuilderFactory.newInstance();
      builder.setNamespaceAware(true);
      builder.setFeature(XMLConstants.FEATURE_SECURE_PROCESSING, true);
      builder.setAttribute(XMLConstants.ACCESS_EXTERNAL_DTD, "");
      builder.setAttribute(XMLConstants.ACCESS_EXTERNAL_SCHEMA, "");
      return builder.newDocumentBuilder().parse(new InputSource(new StringReader(fdd)));
   }

   static void requireDistinctReferencedNotes(String fdd) throws Exception {
      Document document = parseFdd(fdd);
      NodeList notes = document.getElementsByTagNameNS("*", "note");
      Map<String, String> labels = new LinkedHashMap<>();
      for (int index = 0; index < notes.getLength(); index++) {
         Element note = (Element) notes.item(index);
         NodeList label = note.getElementsByTagNameNS("*", "label");
         NodeList semantics = note.getElementsByTagNameNS("*", "semantics");
         if (label.getLength() != 1 || semantics.getLength() != 1) continue;
         String meaning = semantics.item(0).getTextContent().trim().replaceAll("\\s+", " ");
         if ("TCK first colliding note semantics.".equals(meaning)
               || "TCK second colliding note semantics.".equals(meaning)) {
            check(!labels.containsKey(meaning), "current FDD duplicated a contributing note");
            labels.put(meaning, label.item(0).getTextContent().trim());
         }
      }
      String first = labels.get("TCK first colliding note semantics.");
      String second = labels.get("TCK second colliding note semantics.");
      check(first != null && second != null && !first.isEmpty() && !second.isEmpty()
            && !first.equals(second),
         "current FDD did not retain both notes with distinct labels");
      NodeList rows = document.getElementsByTagNameNS("*", "disconnect");
      boolean found = false;
      for (int index = 0; index < rows.getLength(); index++) {
         Element row = (Element) rows.item(index);
         if (!"4.3".equals(row.getAttribute("section"))
               || !"false".equals(row.getAttribute("isCallback"))) continue;
         check(!found, "current FDD duplicated the referenced disconnect row");
         found = true;
         String references = row.getAttribute("noteReferences");
         java.util.Set<String> referenceLabels = new java.util.HashSet<>();
         for (String reference : references.trim().split("\\s+")) {
            referenceLabels.add(reference);
         }
         check(referenceLabels.contains(first) && referenceLabels.contains(second),
            "current FDD disconnect row did not reference both remapped notes");
      }
      check(found, "current FDD omitted the referenced disconnect row");
   }

   private static String requestCurrentFdd(RTIambassador observer, Recorder recorder,
         EncoderFactory encoder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandle currentFddAttribute, AttributeHandleSet currentFddAttributes,
         TransportationTypeHandle reliable, String description) throws Exception {
      recorder.clearReflection();
      observer.requestAttributeValueUpdate(federationObject, currentFddAttributes, new byte[0]);
      await(observer, callbackModel,
         () -> recorder.currentFdd(encoder, currentFddAttribute) != null,
         description);
      Map<AttributeHandle, byte[]> values = recorder.reflectedValues;
      byte[] tag = recorder.reflectionTag;
      check(federationObject.equals(recorder.reflectedObject)
            && values != null && values.size() == 1
            && values.containsKey(currentFddAttribute),
         description + " reported the wrong MOM object or attribute");
      check(tag != null && tag.length == 0,
         description + " returned a non-empty MOM callback tag");
      check(reliable.equals(recorder.reflectionTransportation),
         description + " returned a non-reliable transportation type");
      check(recorder.reflectionRegionsEmpty,
         description + " returned region metadata");
      JavaTckSupport.requireNoProducingFederate(observer, recorder.reflectionProducer,
         description);
      return recorder.currentFdd(encoder, currentFddAttribute);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java RTI TCK callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile ObjectClassHandle discoveredClass;
      private volatile String discoveredName;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile Map<AttributeHandle, byte[]> reflectedValues;
      private volatile byte[] reflectionTag;
      private volatile TransportationTypeHandle reflectionTransportation;
      private volatile FederateHandle reflectionProducer;
      private volatile boolean reflectionRegionsEmpty;
      private volatile ObjectInstanceHandle reportedTransportationObject;
      private volatile AttributeHandle reportedTransportationAttribute;
      private volatile TransportationTypeHandle reportedTransportation;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
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
               } else if ("reportAttributeTransportationType".equals(method.getName())
                     && arguments != null && arguments.length >= 3) {
                  reportedTransportationObject = (ObjectInstanceHandle) arguments[0];
                  reportedTransportationAttribute = (AttributeHandle) arguments[1];
                  reportedTransportation = (TransportationTypeHandle) arguments[2];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java RTI TCK MOM callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }

      private void clearTransportationReport() {
         reportedTransportationObject = null;
         reportedTransportationAttribute = null;
         reportedTransportation = null;
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
