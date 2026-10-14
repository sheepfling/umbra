package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Track the standard joined-federate MOM state through save and restore. */
final class JoinedFederateMomFederateStateSaveRestoreTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String HANDLE_ATTRIBUTE = "HLAfederateHandle";
   private static final String STATE_ATTRIBUTE = "HLAfederateState";

   private JoinedFederateMomFederateStateSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-federate-state-" + UUID.randomUUID();
      String subjectName = "java-tck-mom-state-subject";
      String observerName = "java-tck-mom-state-observer";
      String saveLabel = "java-tck-mom-federate-state-save";
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         observer.joinFederationExecution(observerName, "tck-type", federation);
         observerJoined = true;

         ObjectClassHandle observerClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle observerHandleAttribute =
            observer.getAttributeHandle(observerClass, HANDLE_ATTRIBUTE);
         AttributeHandle observerStateAttribute =
            observer.getAttributeHandle(observerClass, STATE_ATTRIBUTE);
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(observerClass != null && observerHandleAttribute != null
               && observerStateAttribute != null && reliable != null
               && MOM_CLASS.equals(observer.getObjectClassName(observerClass))
               && HANDLE_ATTRIBUTE.equals(observer.getAttributeName(
                  observerClass, observerHandleAttribute))
               && STATE_ATTRIBUTE.equals(observer.getAttributeName(
                  observerClass, observerStateAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "joined-federate MOM state lookup did not resolve the standard surface");
         AttributeHandleSet observerAttributes = JavaTckSupport.attributeSet(
            observer, observerHandleAttribute, observerStateAttribute);
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);

         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "tck-type", federation);
         subjectJoined = true;
         ObjectClassHandle subjectClass = subject.getObjectClassHandle(MOM_CLASS);
         AttributeHandle subjectHandleAttribute = subject.getAttributeHandle(
            subjectClass, HANDLE_ATTRIBUTE);
         AttributeHandle subjectStateAttribute = subject.getAttributeHandle(
            subjectClass, STATE_ATTRIBUTE);
         JavaTckSupport.check(observerClass.equals(subjectClass)
               && observerHandleAttribute.equals(subjectHandleAttribute)
               && observerStateAttribute.equals(subjectStateAttribute),
            "joined-federate MOM state handles differed across members");
         subject.subscribeObjectClassAttributes(subjectClass,
            JavaTckSupport.attributeSet(subject, subjectHandleAttribute, subjectStateAttribute));

         byte[] encodedSubjectHandle = JavaTckSupport.encode(
            subjectFederate.encodedLength(), (buffer, offset) -> subjectFederate.encode(buffer, offset));
         await(callbackModel, MOM_CLASS, "subject MOM object discovery", () ->
            findHandleReflection(observerRecorder.reflections, observerHandleAttribute,
               encodedSubjectHandle) != null
               && findHandleReflection(subjectRecorder.reflections, subjectHandleAttribute,
                  encodedSubjectHandle) != null,
            subject, observer);
         Reflection observerInitial = findHandleReflection(
            observerRecorder.reflections, observerHandleAttribute, encodedSubjectHandle);
         Reflection subjectInitial = findHandleReflection(
            subjectRecorder.reflections, subjectHandleAttribute, encodedSubjectHandle);
         JavaTckSupport.check(observerInitial != null && subjectInitial != null
               && observerInitial.object.encodedLength() > 0
               && subjectInitial.object.encodedLength() > 0
               && hasDiscovery(observerRecorder, observerInitial.object, observerClass)
               && hasDiscovery(subjectRecorder, subjectInitial.object, subjectClass)
               && observerClass.equals(observer.getKnownObjectClassHandle(observerInitial.object))
               && subjectClass.equals(subject.getKnownObjectClassHandle(subjectInitial.object))
               && !observerInitial.values.containsKey(observerStateAttribute)
               && !subjectInitial.values.containsKey(subjectStateAttribute),
            "joined-federate MOM discovery was not conditional or returned the wrong object");

         ObjectInstanceHandle observerMomObject = observerInitial.object;
         ObjectInstanceHandle subjectMomObject = subjectInitial.object;
         int observerInitialReflectionStart = observerRecorder.reflections.size();
         int subjectInitialReflectionStart = subjectRecorder.reflections.size();
         observer.requestAttributeValueUpdate(observerMomObject,
            JavaTckSupport.attributeSet(observer, observerStateAttribute), new byte[0]);
         subject.requestAttributeValueUpdate(subjectMomObject,
            JavaTckSupport.attributeSet(subject, subjectStateAttribute), new byte[0]);
         requireState(callbackModel, subject, observer, observerRecorder, observerMomObject,
            observerStateAttribute, observerInitialReflectionStart, reliable, encoder, 1,
            "initial observer state request");
         requireState(callbackModel, subject, observer, subjectRecorder, subjectMomObject,
            subjectStateAttribute, subjectInitialReflectionStart, reliable, encoder, 1,
            "initial self state request");

         int observerBeforeSave = observerRecorder.reflections.size();
         int subjectBeforeSave = subjectRecorder.reflections.size();
         subject.requestFederationSave(saveLabel);
         await(callbackModel, MOM_CLASS, "MOM save initiation", () ->
            subjectRecorder.saveInitiations.size() == 1
               && observerRecorder.saveInitiations.size() == 1
               && findStateReflection(observerRecorder.reflections, observerMomObject,
                  observerStateAttribute, observerBeforeSave) != null,
            subject, observer);
         JavaTckSupport.check(saveLabel.equals(subjectRecorder.saveInitiations.get(0))
               && saveLabel.equals(observerRecorder.saveInitiations.get(0)),
            "MOM state save initiation returned the wrong label");
         requireState(callbackModel, subject, observer, observerRecorder, observerMomObject,
            observerStateAttribute, observerBeforeSave, reliable, encoder, 3,
            "save initiation observer state");
         pump(callbackModel, MOM_CLASS, 8, subject, observer);
         JavaTckSupport.check(findStateReflection(subjectRecorder.reflections, subjectMomObject,
               subjectStateAttribute, subjectBeforeSave) == null,
            "saving federate observed its own MOM state transition");

         subject.federateSaveBegun();
         observer.federateSaveBegun();
         subject.federateSaveComplete();
         int observerBeforeSaveCompletion = observerRecorder.reflections.size();
         observer.federateSaveComplete();
         await(callbackModel, MOM_CLASS, "MOM save completion", () ->
            subjectRecorder.savedCount == 1 && observerRecorder.savedCount == 1
               && findExpectedStateReflection(observerRecorder.reflections, observerMomObject,
                  observerStateAttribute, observerBeforeSaveCompletion, 1, encoder) != null,
            subject, observer);
         requireState(callbackModel, subject, observer, observerRecorder, observerMomObject,
            observerStateAttribute, observerBeforeSaveCompletion, reliable, encoder, 1,
            "save completion observer state");
         JavaTckSupport.check(subjectRecorder.saveFailureCount == 0
               && observerRecorder.saveFailureCount == 0,
            "joined-federate MOM state save reported a failure");

         int observerBeforeRestore = observerRecorder.reflections.size();
         subject.requestFederationRestore(saveLabel);
         await(callbackModel, MOM_CLASS, "MOM restore initiation", () ->
            subjectRecorder.restoreSuccesses.size() == 1
               && subjectRecorder.restoreBegunCount == 1 && observerRecorder.restoreBegunCount == 1
               && subjectRecorder.restoreInitiations.size() == 1
               && observerRecorder.restoreInitiations.size() == 1
               && findStateReflection(observerRecorder.reflections, observerMomObject,
                  observerStateAttribute, observerBeforeRestore) != null,
            subject, observer);
         RestoreInitiation subjectRestore = subjectRecorder.restoreInitiations.get(0);
         RestoreInitiation observerRestore = observerRecorder.restoreInitiations.get(0);
         JavaTckSupport.check(saveLabel.equals(subjectRecorder.restoreSuccesses.get(0))
               && subjectRecorder.restoreRequestFailures == 0
               && observerRecorder.restoreRequestFailures == 0
               && validRestore(subjectRestore, saveLabel, subjectName)
               && validRestore(observerRestore, saveLabel, observerName),
            "joined-federate MOM restore initiation returned incorrect identity metadata");
         requireState(callbackModel, subject, observer, observerRecorder, observerMomObject,
            observerStateAttribute, observerBeforeRestore, reliable, encoder, 5,
            "restore initiation observer state");

         subject.federateRestoreComplete();
         int observerBeforeRestoreCompletion = observerRecorder.reflections.size();
         observer.federateRestoreComplete();
         await(callbackModel, MOM_CLASS, "MOM restore completion", () ->
            subjectRecorder.restoredCount == 1 && observerRecorder.restoredCount == 1
               && findExpectedStateReflection(observerRecorder.reflections, observerMomObject,
                  observerStateAttribute, observerBeforeRestoreCompletion, 1, encoder) != null,
            subject, observer);
         requireState(callbackModel, subject, observer, observerRecorder, observerMomObject,
            observerStateAttribute, observerBeforeRestoreCompletion, reliable, encoder, 1,
            "restore completion observer state");
         JavaTckSupport.check(subjectRecorder.restoreFailureCount == 0
               && observerRecorder.restoreFailureCount == 0,
            "joined-federate MOM state restore reported a failure");
      } finally {
         if (subjectJoined) resign(subject);
         if (observerJoined) resign(observer);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void requireState(CallbackModel callbackModel, RTIambassador subject,
         RTIambassador observer, Recorder recorder, ObjectInstanceHandle object,
         AttributeHandle stateAttribute, int before, TransportationTypeHandle reliable,
         EncoderFactory encoder, int expected, String description) throws Exception {
      await(callbackModel, MOM_CLASS, description, () -> findExpectedStateReflection(
         recorder.reflections, object, stateAttribute, before, expected, encoder) != null,
         subject, observer);
      Reflection reflection = findExpectedStateReflection(
         recorder.reflections, object, stateAttribute, before, expected, encoder);
      JavaTckSupport.check(reflection != null && reflection.values.size() == 1
            && reliable.equals(reflection.transportation) && reflection.tag.length == 0
            && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata");
   }

   private static Reflection findHandleReflection(List<Reflection> reflections,
         AttributeHandle handleAttribute, byte[] expectedHandle) {
      for (Reflection reflection : reflections) {
         byte[] received = reflection.values.get(handleAttribute);
         if (received != null && Arrays.equals(received, expectedHandle)) return reflection;
      }
      return null;
   }

   private static boolean hasDiscovery(Recorder recorder, ObjectInstanceHandle object,
         ObjectClassHandle objectClass) {
      for (Discovery discovery : recorder.discoveries) {
         if (object.equals(discovery.object) && objectClass.equals(discovery.objectClass)
               && "HLAfederate".equals(discovery.name)) return true;
      }
      return false;
   }

   private static Reflection findStateReflection(List<Reflection> reflections,
         ObjectInstanceHandle object, AttributeHandle stateAttribute, int before) {
      for (int index = Math.max(0, before); index < reflections.size(); index++) {
         Reflection reflection = reflections.get(index);
         if (reflection.object.equals(object) && reflection.values.containsKey(stateAttribute)) {
            return reflection;
         }
      }
      return null;
   }

   private static Reflection findExpectedStateReflection(List<Reflection> reflections,
         ObjectInstanceHandle object, AttributeHandle stateAttribute, int before,
         int expected, EncoderFactory encoder) throws Exception {
      for (int index = Math.max(0, before); index < reflections.size(); index++) {
         Reflection reflection = reflections.get(index);
         byte[] value = reflection.values.get(stateAttribute);
         if (reflection.object.equals(object) && value != null
               && decodeState(encoder, value) == expected) return reflection;
      }
      return null;
   }

   private static int decodeState(EncoderFactory encoder, byte[] encoded) throws Exception {
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(encoded);
      return value.getValue();
   }

   private static boolean validRestore(RestoreInitiation initiation,
         String label, String federateName) {
      return label.equals(initiation.label) && federateName.equals(initiation.federateName)
         && initiation.postRestoreHandle != null && initiation.postRestoreHandle.encodedLength() > 0;
   }

   private static void await(CallbackModel callbackModel, String servicingClass,
         String description, Condition condition, RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + 15_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         pump(callbackModel, servicingClass, 1, ambassadors);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   private static void pump(CallbackModel callbackModel, String servicingClass, int passes,
         RTIambassador... ambassadors) throws Exception {
      for (int pass = 0; pass < passes; pass++) {
         for (RTIambassador ambassador : ambassadors) {
            if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0, 1);
            else ambassador.getObjectClassHandle(servicingClass);
         }
      }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class RestoreInitiation {
      private final String label;
      private final String federateName;
      private final FederateHandle postRestoreHandle;

      private RestoreInitiation(String label, String federateName, FederateHandle handle) {
         this.label = label;
         this.federateName = federateName;
         postRestoreHandle = handle;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions;
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass, String name) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private volatile int savedCount;
      private volatile int saveFailureCount;
      private volatile int restoreBegunCount;
      private volatile int restoreRequestFailures;
      private volatile int restoredCount;
      private volatile int restoreFailureCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2]));
               } else if ("reflectAttributeValues".equals(name)
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  Map<AttributeHandle, byte[]> values = new HashMap<>();
                  AttributeHandleValueMap received = (AttributeHandleValueMap) arguments[1];
                  for (Map.Entry<AttributeHandle, byte[]> entry : received.entrySet()) {
                     values.put(entry.getKey(), entry.getValue().clone());
                  }
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], values,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], (RegionHandleSet) arguments[5]));
               } else if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount++;
               } else if ("federationNotSaved".equals(name)) {
                  saveFailureCount++;
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures++;
               } else if ("federationRestoreBegun".equals(name)) {
                  restoreBegunCount++;
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               } else if ("federationRestored".equals(name)) {
                  restoredCount++;
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailureCount++;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK joined-federate MOM recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

   }

}
