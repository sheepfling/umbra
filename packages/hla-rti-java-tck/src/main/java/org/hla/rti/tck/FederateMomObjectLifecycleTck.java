package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies joined-federate MOM object identity, values, requests, and resign removals. */
final class FederateMomObjectLifecycleTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATION_MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederation";
   private static final String FEDERATE_SUBCLASS =
      "HLAobjectRoot.HLAmanager.HLAfederate.TckFederateExtension";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String FEDERATE_NAME = "HLAfederateName";
   private static final String FEDERATE_TYPE = "HLAfederateType";
   private static final String FEDERATE_HOST = "HLAfederateHost";
   private static final String MODULES = "HLAFOMmoduleDesignatorList";
   private static final List<String> FEDERATE_TABLE8_P_ATTRIBUTES = Arrays.asList(
      "HLAfederateHandle", "HLAfederateName", "HLAfederateType", "HLAfederateHost",
      "HLARTIversion", "HLAFOMmoduleDesignatorList", "HLAtimeConstrained",
      "HLAtimeRegulating", "HLAasynchronousDelivery", "HLAfederateState",
      "HLAtimeManagerState", "HLAlogicalTime", "HLAlookahead", "HLAGALT", "HLALITS",
      "HLAROlength", "HLATSOlength", "HLAreflectionsReceived", "HLAupdatesSent",
      "HLAinteractionsReceived", "HLAinteractionsSent", "HLAdirectedInteractionsReceived",
      "HLAdirectedInteractionsSent", "HLAobjectInstancesThatCanBeDeleted",
      "HLAobjectInstancesUpdated", "HLAobjectInstancesReflected",
      "HLAobjectInstancesDeleted", "HLAobjectInstancesRemoved",
      "HLAobjectInstancesRegistered", "HLAobjectInstancesDiscovered", "HLAtimeGrantedTime",
      "HLAtimeAdvancingTime", "HLAobjectClassRelevanceAdvisory",
      "HLAattributeRelevanceAdvisory", "HLAattributeScopeAdvisory",
      "HLAinteractionRelevanceAdvisory", "HLAconveyRegionDesignatorSets",
      "HLAautomaticResignAction", "HLAserviceReporting", "HLAexceptionReporting",
      "HLAsendServiceReportsToFile", "HLAreportServiceFile");
   private static final List<String> FEDERATION_TABLE8_P_ATTRIBUTES = Arrays.asList(
      "HLAfederationName", "HLAfederatesInFederation", "HLARTIversion", "HLAMIMdesignator",
      "HLAFOMmoduleDesignatorList", "HLAcurrentFDD", "HLAtimeImplementationName",
      "HLAlastSaveName", "HLAlastSaveTime", "HLAnextSaveName", "HLAnextSaveTime",
      "HLAautoProvide", "HLAadvisoriesUseKnownClass", "HLAdelaySubscriptionEvaluation",
      "HLAnonRegulatedGrant", "HLAallowRelaxedDDM");
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomObjectLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String mim, String additionalFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      verifyMimBeforeFomComposition(factory, fom, mim, additionalFom,
         timeImplementation, callbackModel);
      RTIambassador moduleSubject = factory.getRtiAmbassador();
      RTIambassador noModuleSubject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder observerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-object-lifecycle-" + suffix;
      String moduleSubjectName = "java-tck-mom-module-subject-" + suffix;
      String noModuleSubjectName = "java-tck-mom-no-module-" + suffix;
      String observerName = "java-tck-mom-observer-" + suffix;
      boolean moduleConnected = false, noModuleConnected = false, observerConnected = false;
      boolean created = false, moduleJoined = false, noModuleJoined = false, observerJoined = false;
      boolean subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      ObjectInstanceHandle moduleSubjectObject = null, noModuleSubjectObject = null;
      try {
         moduleSubject.connect(callbacks(), callbackModel);
         moduleConnected = true;
         noModuleSubject.connect(callbacks(), callbackModel);
         noModuleConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         moduleSubject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;

         FederateHandle observerFederate = observer.joinFederationExecution(
            observerName, "java-tck", federation);
         observerJoined = true;
         momClass = observer.getObjectClassHandle(MOM_CLASS);
         ObjectClassHandle federateMomClass = momClass;
         AttributeHandle federateHandleAttribute = observer.getAttributeHandle(
            momClass, FEDERATE_HANDLE);
         AttributeHandle federateNameAttribute = observer.getAttributeHandle(
            momClass, FEDERATE_NAME);
         AttributeHandle federateTypeAttribute = observer.getAttributeHandle(
            momClass, FEDERATE_TYPE);
         AttributeHandle federateHostAttribute = observer.getAttributeHandle(
            momClass, FEDERATE_HOST);
         AttributeHandle moduleListAttribute = observer.getAttributeHandle(momClass, MODULES);
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(momClass != null && federateHandleAttribute != null
               && federateNameAttribute != null && federateTypeAttribute != null
               && federateHostAttribute != null && moduleListAttribute != null && reliable != null
               && MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(observer.getAttributeName(momClass,
                  federateHandleAttribute))
               && FEDERATE_NAME.equals(observer.getAttributeName(momClass, federateNameAttribute))
               && FEDERATE_TYPE.equals(observer.getAttributeName(momClass, federateTypeAttribute))
               && FEDERATE_HOST.equals(observer.getAttributeName(momClass, federateHostAttribute))
               && MODULES.equals(observer.getAttributeName(momClass, moduleListAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "joined-federate MOM lifecycle did not resolve standard MIM handles");
         momAttributes = resolveTable8Attributes(observer, momClass,
            FEDERATE_TABLE8_P_ATTRIBUTES, MOM_CLASS);
         observerRecorder.attributes = momAttributes;
         observer.subscribeObjectClassAttributes(momClass, momAttributes, "");
         subscribed = true;

         ObjectClassHandle federationMomClass =
            observer.getObjectClassHandle(FEDERATION_MOM_CLASS);
         AttributeHandleSet federationMomAttributes = resolveTable8Attributes(observer,
            federationMomClass, FEDERATION_TABLE8_P_ATTRIBUTES, FEDERATION_MOM_CLASS);
         observer.subscribeObjectClassAttributes(
            federationMomClass, federationMomAttributes, "");
         verifyFederationTable8(observer, observerRecorder, callbackModel,
            federationMomClass, federationMomAttributes);

         byte[] observerHandle = encode(observerFederate);
         await(observer, callbackModel,
            () -> {
               Reflection value = observerRecorder.findByFederate(federateHandleAttribute,
                  observerHandle);
               return value != null && federateMomClass.equals(
                  observerRecorder.discoveredObjects.get(value.object));
            },
            "observer's joined-federate MOM reflection");
         Reflection observerReflection = observerRecorder.findByFederate(
            federateHandleAttribute, observerHandle);
         JavaTckSupport.check(observerReflection != null
               && momClass.equals(observer.getKnownObjectClassHandle(observerReflection.object)),
            "observer's MOM discovery returned a different standard object class");

         FederateHandle moduleFederate = moduleSubject.joinFederationExecution(
            moduleSubjectName, "java-tck", federation, new String[] {additionalFom});
         moduleJoined = true;
         List<String> expectedModules = Collections.singletonList(additionalFom);
         await(observer, callbackModel,
            () -> {
               Reflection value = observerRecorder.findByFederate(federateHandleAttribute,
                  encodeUnchecked(moduleFederate));
               return value != null && federateMomClass.equals(
                  observerRecorder.discoveredObjects.get(value.object));
            },
            "additional-module federate MOM discovery and initial reflection");
         Reflection moduleReflection = observerRecorder.findByFederate(
            federateHandleAttribute, encode(moduleFederate));
         moduleSubjectObject = verifyInitialReflection(observer, observerRecorder, encoder,
            moduleReflection, momClass, federateHandleAttribute, federateNameAttribute,
            federateTypeAttribute, federateHostAttribute, moduleListAttribute, reliable,
            moduleFederate, moduleSubjectName, "java-tck", expectedModules,
            "additional-module federate");
         requestValues(observer, observerRecorder, encoder, callbackModel, moduleSubjectObject,
            federateHandleAttribute, federateNameAttribute, federateTypeAttribute,
            federateHostAttribute, moduleListAttribute, reliable, moduleFederate,
            moduleSubjectName, "java-tck", expectedModules, "additional-module federate");

         FederateHandle noModuleFederate = noModuleSubject.joinFederationExecution(
            noModuleSubjectName, "java-tck", federation);
         noModuleJoined = true;
         await(observer, callbackModel,
            () -> {
               Reflection value = observerRecorder.findByFederate(federateHandleAttribute,
                  encodeUnchecked(noModuleFederate));
               return value != null && federateMomClass.equals(
                  observerRecorder.discoveredObjects.get(value.object));
            },
            "no-additional-module federate MOM discovery and initial reflection");
         Reflection noModuleReflection = observerRecorder.findByFederate(
            federateHandleAttribute, encode(noModuleFederate));
         noModuleSubjectObject = verifyInitialReflection(observer, observerRecorder, encoder,
            noModuleReflection, momClass, federateHandleAttribute, federateNameAttribute,
            federateTypeAttribute, federateHostAttribute, moduleListAttribute, reliable,
            noModuleFederate, noModuleSubjectName, "java-tck", Collections.emptyList(),
            "no-additional-module federate");
         JavaTckSupport.check(!noModuleSubjectObject.equals(moduleSubjectObject),
            "distinct joined federates shared one MOM object instance");
         requestValues(observer, observerRecorder, encoder, callbackModel, noModuleSubjectObject,
            federateHandleAttribute, federateNameAttribute, federateTypeAttribute,
            federateHostAttribute, moduleListAttribute, reliable, noModuleFederate,
            noModuleSubjectName, "java-tck", Collections.emptyList(),
            "no-additional-module federate");
         requestClassWideTable8Values(observer, observerRecorder, callbackModel, momClass,
            moduleSubjectObject, momAttributes);

         waitForRemoval(observer, observerRecorder, callbackModel, moduleSubject,
            moduleSubjectObject, moduleSubjectName, moduleJoined);
         moduleJoined = false;
         waitForRemoval(observer, observerRecorder, callbackModel, noModuleSubject,
            noModuleSubjectObject, noModuleSubjectName, noModuleJoined);
         noModuleJoined = false;
      } finally {
         if (subscribed && observerJoined && momClass != null && momAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(momClass, momAttributes); }
            catch (Exception ignored) { }
         }
         if (moduleJoined) resign(moduleSubject);
         if (noModuleJoined) resign(noModuleSubject);
         if (observerJoined) resign(observer);
         if (created) {
            try { moduleSubject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (noModuleConnected) disconnect(noModuleSubject);
         if (moduleConnected) disconnect(moduleSubject);
      }
   }

   private static void verifyMimBeforeFomComposition(RtiFactory factory, String fom,
         String mim, String additionalFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador compositionSubject = factory.getRtiAmbassador();
      String federation = "java-tck-mim-before-fom-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         compositionSubject.connect(callbacks(), callbackModel);
         connected = true;
         compositionSubject.createFederationExecutionWithMIM(federation,
            new String[] {fom, additionalFom}, mim, timeImplementation);
         created = true;
         compositionSubject.joinFederationExecution("java-tck-mim-before-fom-probe",
            "java-tck", federation);
         joined = true;
         ObjectClassHandle extension = compositionSubject.getObjectClassHandle(FEDERATE_SUBCLASS);
         AttributeHandle extensionValue =
            compositionSubject.getAttributeHandle(extension, "TckExtensionValue");
         JavaTckSupport.check(extension != null && extensionValue != null
               && FEDERATE_SUBCLASS.equals(compositionSubject.getObjectClassName(extension))
               && "TckExtensionValue".equals(
                  compositionSubject.getAttributeName(extension, extensionValue)),
            "Create Federation Execution With MIM did not compose the FOM subclass of HLAfederate");
      } finally {
         if (joined) resign(compositionSubject);
         if (created) {
            try { compositionSubject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) disconnect(compositionSubject);
      }
   }

   private static AttributeHandleSet resolveTable8Attributes(RTIambassador ambassador,
         ObjectClassHandle objectClass, List<String> names, String className) throws Exception {
      JavaTckSupport.check(objectClass != null
            && className.equals(ambassador.getObjectClassName(objectClass)),
         className + " MOM class did not resolve through the standard MIM");
      AttributeHandleSet result = ambassador.getAttributeHandleSetFactory().create();
      for (String name : names) {
         AttributeHandle attribute = ambassador.getAttributeHandle(objectClass, name);
         JavaTckSupport.check(attribute != null
               && name.equals(ambassador.getAttributeName(objectClass, attribute)),
            className + " Table 8 attribute lookup did not round-trip: " + name);
         result.add(attribute);
      }
      JavaTckSupport.check(result.size() == names.size(),
         className + " Table 8 did not resolve " + names.size() + " distinct P attributes");
      return result;
   }

   private static void verifyFederationTable8(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel, ObjectClassHandle federationMomClass,
         AttributeHandleSet federationMomAttributes) throws Exception {
      await(observer, callbackModel,
         () -> recorder.discoveryCount(federationMomClass) == 1,
         "standard HLAfederation Table 8 MOM discovery");
      ObjectInstanceHandle federationObject = recorder.findDiscoveredObject(federationMomClass);
      JavaTckSupport.check(federationObject != null
            && recorder.discoveryCount(federationMomClass) == 1
            && federationMomClass.equals(observer.getKnownObjectClassHandle(federationObject))
            && "HLAfederation".equals(observer.getObjectInstanceName(federationObject))
            && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
         "HLAfederation Table 8 did not expose exactly one standard MOM instance");

      int start = recorder.reflections.size();
      observer.requestAttributeValueUpdate(federationObject, federationMomAttributes, new byte[0]);
      await(observer, callbackModel,
         () -> recorder.findCompleteReflection(
            federationObject, federationMomAttributes, start) != null,
         "complete HLAfederation Table 8 P-attribute request reflection");
      Reflection reflection = recorder.findCompleteReflection(
         federationObject, federationMomAttributes, start);
      TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
      JavaTckSupport.check(hasExactlyAttributes(reflection, federationMomAttributes)
            && reliable.equals(reflection.transportation),
         "HLAfederation request did not return all 16 Table 8 P values over HLAreliable");
   }

   private static void requestClassWideTable8Values(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel, ObjectClassHandle momClass, ObjectInstanceHandle object,
         AttributeHandleSet attributes) throws Exception {
      int start = recorder.reflections.size();
      observer.requestAttributeValueUpdate(momClass, attributes, new byte[0]);
      await(observer, callbackModel,
         () -> recorder.findCompleteReflection(object, attributes, start) != null,
         "class-wide HLAfederate Table 8 P-attribute request reflection");
      JavaTckSupport.check(hasExactlyAttributes(
            recorder.findCompleteReflection(object, attributes, start), attributes),
         "class-wide HLAfederate request did not return all 42 Table 8 P values");
   }

   private static boolean hasExactlyAttributes(Reflection reflection,
         AttributeHandleSet expected) {
      return reflection != null && expected != null
         && reflection.values.size() == expected.size()
         && reflection.values.keySet().containsAll(expected);
   }

   private static ObjectInstanceHandle verifyInitialReflection(RTIambassador observer,
         Recorder recorder, EncoderFactory encoder, Reflection reflection,
         ObjectClassHandle momClass, AttributeHandle federateHandleAttribute,
         AttributeHandle federateNameAttribute, AttributeHandle federateTypeAttribute,
         AttributeHandle federateHostAttribute, AttributeHandle moduleListAttribute,
         TransportationTypeHandle reliable, FederateHandle expectedHandle, String expectedName,
         String expectedType, List<String> expectedModules, String description) throws Exception {
      JavaTckSupport.check(reflection != null
            && momClass.equals(recorder.discoveredObjects.get(reflection.object))
            && momClass.equals(observer.getKnownObjectClassHandle(reflection.object))
             && reflection.values.size() >= 5
            && reflection.values.containsKey(federateHandleAttribute)
            && reflection.values.containsKey(federateNameAttribute)
            && reflection.values.containsKey(federateTypeAttribute)
            && reflection.values.containsKey(federateHostAttribute)
            && reflection.values.containsKey(moduleListAttribute)
            && reliable.equals(reflection.transportation),
         description + " MOM reflection had incomplete values or non-reliable transportation");
      JavaTckSupport.check(Arrays.equals(encode(expectedHandle),
               reflection.values.get(federateHandleAttribute))
            && expectedName.equals(decodeString(encoder,
               reflection.values.get(federateNameAttribute)))
            && expectedType.equals(decodeString(encoder,
               reflection.values.get(federateTypeAttribute)))
            && expectedModules.equals(decodeStrings(encoder,
               reflection.values.get(moduleListAttribute)))
            && !decodeString(encoder, reflection.values.get(federateHostAttribute)).isEmpty(),
         description + " MOM reflection changed federate identity, module list, or required host");
      String objectName = observer.getObjectInstanceName(reflection.object);
      JavaTckSupport.check(objectName != null && !objectName.isEmpty()
            && reflection.object.equals(observer.getObjectInstanceHandle(objectName)),
         description + " MOM object-name lookup did not round-trip");
      return reflection.object;
   }

   private static void requestValues(RTIambassador observer, Recorder recorder,
         EncoderFactory encoder, CallbackModel callbackModel, ObjectInstanceHandle object,
         AttributeHandle federateHandleAttribute, AttributeHandle federateNameAttribute,
         AttributeHandle federateTypeAttribute, AttributeHandle federateHostAttribute,
         AttributeHandle moduleListAttribute, TransportationTypeHandle reliable,
         FederateHandle federate, String name, String type, List<String> expectedModules,
         String description) throws Exception {
      int start = recorder.reflections.size();
      observer.requestAttributeValueUpdate(object, recorder.attributes, new byte[0]);
      byte[] encodedHandle = encode(federate);
      await(observer, callbackModel,
         () -> recorder.findByFederate(federateHandleAttribute, encodedHandle, start) != null,
         description + " known-object attribute-value request");
      Reflection reflection = recorder.findByFederate(federateHandleAttribute, encodedHandle, start);
      JavaTckSupport.check(reflection != null && reflection.object.equals(object)
             && hasExactlyAttributes(reflection, recorder.attributes)
            && reflection.values.containsKey(federateNameAttribute)
            && reflection.values.containsKey(federateTypeAttribute)
            && reflection.values.containsKey(federateHostAttribute)
            && reflection.values.containsKey(moduleListAttribute)
            && reliable.equals(reflection.transportation)
            && Arrays.equals(encodedHandle, reflection.values.get(federateHandleAttribute))
            && name.equals(decodeString(encoder, reflection.values.get(federateNameAttribute)))
            && type.equals(decodeString(encoder, reflection.values.get(federateTypeAttribute)))
            && expectedModules.equals(decodeStrings(encoder,
               reflection.values.get(moduleListAttribute)))
            && !decodeString(encoder, reflection.values.get(federateHostAttribute)).isEmpty(),
         description + " known-object request returned incomplete or unexpected MOM values");
   }

   private static void waitForRemoval(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel, RTIambassador subject, ObjectInstanceHandle object,
         String description, boolean joined) throws Exception {
      if (!joined || object == null) return;
      subject.resignFederationExecution(ResignAction.NO_ACTION);
      await(observer, callbackModel, () -> recorder.removals.contains(object),
         description + " MOM object removal after resign");
   }

   private static String decodeString(EncoderFactory encoder, byte[] bytes) throws Exception {
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(bytes);
      return value.getValue();
   }

   private static List<String> decodeStrings(EncoderFactory encoder, byte[] bytes)
         throws Exception {
      HLAvariableArray<HLAunicodeString> values = encoder.createHLAvariableArray(
         index -> encoder.createHLAunicodeString());
      values.decode(bytes);
      List<String> result = new ArrayList<>();
      for (HLAunicodeString value : values) result.add(value.getValue());
      return result;
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encodeUnchecked(FederateHandle handle) {
      try { return encode(handle); }
      catch (Exception error) { throw new AssertionError("could not encode federate handle", error); }
   }

   private static void await(RTIambassador rti, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (model == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String method, Object[] arguments) {
      if ("toString".equals(method)) return "Java RTI TCK MOM lifecycle callbacks";
      if ("hashCode".equals(method)) return System.identityHashCode(proxy);
      if ("equals".equals(method)) return proxy == arguments[0];
      return null;
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final TransportationTypeHandle transportation;
      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.values = values;
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();
      private final java.util.concurrent.ConcurrentHashMap<ObjectInstanceHandle,
         ObjectClassHandle> discoveredObjects = new java.util.concurrent.ConcurrentHashMap<>();
      private volatile AttributeHandleSet attributes;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName()) && arguments != null
                     && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof ObjectClassHandle) {
                  discoveredObjects.put((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]);
               } else if ("reflectAttributeValues".equals(method.getName()) && arguments != null
                     && arguments.length >= 4 && arguments[1] instanceof AttributeHandleValueMap) {
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  java.util.LinkedHashMap<AttributeHandle, byte[]> copy =
                     new java.util.LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     copy.put(handle, source.get(handle).clone());
                  }
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], copy,
                     (TransportationTypeHandle) arguments[3]));
               } else if ("removeObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length >= 1
                     && arguments[0] instanceof ObjectInstanceHandle) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }

      private Reflection findByFederate(AttributeHandle attribute, byte[] handle) {
         return findByFederate(attribute, handle, 0);
      }

      private Reflection findByFederate(AttributeHandle attribute, byte[] handle, int start) {
         for (int index = Math.max(0, start); index < reflections.size(); index++) {
            Reflection reflection = reflections.get(index);
            byte[] actual = reflection.values.get(attribute);
            if (actual != null && Arrays.equals(actual, handle)) return reflection;
         }
         return null;
      }

      private int discoveryCount(ObjectClassHandle objectClass) {
         int count = 0;
         for (ObjectClassHandle discoveredClass : discoveredObjects.values()) {
            if (objectClass.equals(discoveredClass)) count++;
         }
         return count;
      }

      private ObjectInstanceHandle findDiscoveredObject(ObjectClassHandle objectClass) {
         for (Map.Entry<ObjectInstanceHandle, ObjectClassHandle> entry :
               discoveredObjects.entrySet()) {
            if (objectClass.equals(entry.getValue())) return entry.getKey();
         }
         return null;
      }

      private Reflection findCompleteReflection(ObjectInstanceHandle object,
            AttributeHandleSet expected, int start) {
         for (int index = Math.max(0, start); index < reflections.size(); index++) {
            Reflection reflection = reflections.get(index);
            if (object.equals(reflection.object) && hasExactlyAttributes(reflection, expected)) {
               return reflection;
            }
         }
         return null;
      }
   }
}
