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
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies that RTI-owned federate MOM attributes cannot be transferred. */
final class MomOwnershipTransferProhibitionTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private MomOwnershipTransferProhibitionTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-ownership-transfer-" + suffix;
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean observerPublished = false;
      ObjectClassHandle momClass = null;
      AttributeHandle federateHandleAttribute = null;
      AttributeHandle federateNameAttribute = null;
      AttributeHandleSet observerAttributes = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;

         observer.joinFederationExecution(
            "java-tck-mom-ownership-observer-" + suffix, "java-tck", federation);
         observerJoined = true;
         momClass = observer.getObjectClassHandle(MOM_CLASS);
         federateHandleAttribute = observer.getAttributeHandle(momClass, "HLAfederateHandle");
         federateNameAttribute = observer.getAttributeHandle(momClass, "HLAfederateName");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(observer.getAttributeName(
                  momClass, federateHandleAttribute))
               && "HLAfederateName".equals(observer.getAttributeName(
                  momClass, federateNameAttribute)),
            "standard federate MOM object lookup did not round-trip");
         observer.subscribeObjectClassAttributes(momClass,
            JavaTckSupport.attributeSet(
               observer, federateHandleAttribute, federateNameAttribute), "");

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-mom-ownership-subject-" + suffix, "java-tck", federation);
         subjectJoined = true;
         byte[] encodedSubject = JavaTckSupport.encode(
            subjectFederate.encodedLength(), subjectFederate::encode);
         final AttributeHandle handleAttribute = federateHandleAttribute;
         final ObjectClassHandle expectedClass = momClass;
         await(observer, subject, callbackModel,
            () -> observerRecorder.subjectMomReflection(
               handleAttribute, encodedSubject, expectedClass) != null,
            "joined-federate MOM object discovery and reflection");
         Reflection subjectReflection = observerRecorder.subjectMomReflection(
            federateHandleAttribute, encodedSubject, momClass);
         JavaTckSupport.check(subjectReflection != null
               && momClass.equals(observer.getKnownObjectClassHandle(subjectReflection.object))
               && subjectReflection.values.containsKey(federateNameAttribute),
            "the public MOM reflection did not identify the subject federate object");

         observerAttributes = JavaTckSupport.attributeSet(observer, federateNameAttribute);
         observer.publishObjectClassAttributes(momClass, observerAttributes);
         observerPublished = true;
         JavaTckSupport.check(subjectRecorder.releaseRequests.isEmpty(),
            "the subject received an unrelated release request before the MOM ownership checks");
         verifyRtiOwnership(observer, subject, observerRecorder, callbackModel,
            subjectReflection.object, federateNameAttribute, "before ownership acquisition");
         int acquisitionEventsBefore = observerRecorder.acquisitions.size();
         attemptAcquisition(observer, subject, observerRecorder, subjectRecorder,
            callbackModel, subjectReflection.object, federateNameAttribute,
            acquisitionEventsBefore, true);
         verifyRtiOwnership(observer, subject, observerRecorder, callbackModel,
            subjectReflection.object, federateNameAttribute,
            "after acquisition-if-available");
         attemptAcquisition(observer, subject, observerRecorder, subjectRecorder,
            callbackModel, subjectReflection.object, federateNameAttribute,
            acquisitionEventsBefore, false);
         verifyRtiOwnership(observer, subject, observerRecorder, callbackModel,
            subjectReflection.object, federateNameAttribute, "after negotiated acquisition");
      } finally {
         if (observerPublished && observerJoined && momClass != null
               && observerAttributes != null) {
            try { observer.unpublishObjectClassAttributes(momClass, observerAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void verifyRtiOwnership(RTIambassador observer, RTIambassador subject,
         Recorder recorder, CallbackModel callbackModel, ObjectInstanceHandle object,
         AttributeHandle attribute, String phase) throws Exception {
      int ownedByRtiBefore = recorder.ownedByRti.size();
      int notOwnedBefore = recorder.notOwned.size();
      observer.queryAttributeOwnership(object, JavaTckSupport.attributeSet(observer, attribute));
      await(observer, subject, callbackModel,
         () -> recorder.ownedByRti.size() > ownedByRtiBefore
            || recorder.notOwned.size() > notOwnedBefore,
         phase + " ownership query for the predefined HLAfederateName attribute");
      JavaTckSupport.check(recorder.ownedByRti.size() == ownedByRtiBefore + 1
            && recorder.notOwned.size() == notOwnedBefore,
         "the RTI no longer owned the predefined HLAfederateName MOM attribute " + phase);
      OwnershipEvent ownedByRti = recorder.ownedByRti.get(recorder.ownedByRti.size() - 1);
      JavaTckSupport.check(object.equals(ownedByRti.object)
            && new HashSet<>(List.of(attribute)).equals(ownedByRti.attributes),
         "the ownership query returned the wrong object or attribute set " + phase);
   }

   private static void attemptAcquisition(RTIambassador observer, RTIambassador subject,
         Recorder observerRecorder, Recorder subjectRecorder, CallbackModel callbackModel,
         ObjectInstanceHandle object, AttributeHandle attribute, int acquisitionsBefore,
         boolean ifAvailable) throws Exception {
      AttributeHandleSet attributes = JavaTckSupport.attributeSet(observer, attribute);
      try {
         if (ifAvailable) {
            observer.attributeOwnershipAcquisitionIfAvailable(object, attributes, new byte[0]);
         } else {
            observer.attributeOwnershipAcquisition(object, attributes, new byte[0]);
         }
      } catch (Exception ignored) {
         // The public API does not prescribe a response exception for this prohibited transfer.
      }

      long deadline = System.nanoTime() + 250_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            observer.evokeCallback(0.01);
            subject.evokeCallback(0.01);
         } else {
            Thread.sleep(1);
         }
      }
      JavaTckSupport.check(observerRecorder.acquisitions.size() == acquisitionsBefore,
         ifAvailable
            ? "the RTI transferred a MOM attribute after acquisition-if-available"
            : "the RTI transferred a MOM attribute after negotiated acquisition");
      JavaTckSupport.check(subjectRecorder.releaseRequests.isEmpty(),
         ifAvailable
            ? "the RTI requested release of a MOM attribute during acquisition-if-available"
            : "the RTI requested release of a MOM attribute during negotiated acquisition");
   }

   private static void await(RTIambassador observer, RTIambassador subject,
         CallbackModel callbackModel, BooleanSupplier ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            observer.evokeCallback(0.01);
            subject.evokeCallback(0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK MOM ownership prohibition callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Recorder {
      private final Map<ObjectInstanceHandle, ObjectClassHandle> discovered =
         new ConcurrentHashMap<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownedByRti = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> notOwned = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  discovered.put(
                     (ObjectInstanceHandle) arguments[0], (ObjectClassHandle) arguments[1]);
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length >= 2
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent(arguments));
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  releaseRequests.add(new OwnershipEvent(arguments));
               } else if ("attributeIsOwnedByRTI".equals(name)
                     && arguments != null && arguments.length == 2) {
                  ownedByRti.add(new OwnershipEvent(arguments));
               } else if ("attributeIsNotOwned".equals(name)
                     && arguments != null && arguments.length == 2) {
                  notOwned.add(new OwnershipEvent(arguments));
               }
               return objectMethod(proxy, name, arguments);
            });
      }

      private Reflection subjectMomReflection(AttributeHandle handle, byte[] expected,
            ObjectClassHandle expectedClass) {
         for (Reflection reflection : reflections) {
            byte[] actual = reflection.values.get(handle);
            if (actual != null && Arrays.equals(expected, actual)
                  && expectedClass.equals(discovered.get(reflection.object))) {
               return reflection;
            }
         }
         return null;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new java.util.LinkedHashMap<>();

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
         for (AttributeHandle handle : source.keySet()) {
            values.put(handle, source.get(handle).clone());
         }
      }
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final java.util.Set<AttributeHandle> attributes;

      private OwnershipEvent(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
      }
   }
}
