package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the Table 7 synchronization-achievement request completes a pending point. */
final class Table7SynchronizationPointAchievementTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice."
         + "HLAsynchronizationPointAchieved";

   private Table7SynchronizationPointAchievementTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "Table 7 synchronization-achievement testing requires an adapter-supplied FOM");
      JavaTckSupport.check(mim != null && !mim.isEmpty(),
         "Table 7 synchronization-achievement testing requires the standard MIM");
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 synchronization-achievement testing requires logical-time configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-synchronization-achievement-" + suffix;
      String subjectName = "java-tck-table7-sync-subject-" + suffix;
      String observerName = "java-tck-table7-sync-observer-" + suffix;
      String label = "java-tck-table7-mom-achieved-" + suffix;
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean requestPublished = false;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         InteractionClassHandle request = subject.getInteractionClassHandle(REQUEST_CLASS);
         ParameterHandle labelParameter = subject.getParameterHandle(request, "HLAlabel");
         JavaTckSupport.check(request != null && labelParameter != null,
            "standard MIM omitted HLAsynchronizationPointAchieved or HLAlabel");
         subject.publishInteractionClass(request);
         requestPublished = true;

         subject.registerFederationSynchronizationPoint(label, new byte[0]);
         await(subject, observer, callbackModel,
            () -> (subjectRecorder.registrationSucceeded.contains(label)
               && observerRecorder.announcements.containsKey(label))
               || subjectRecorder.registrationFailed.contains(label),
            "synchronization-point registration and announcement");
         JavaTckSupport.check(subjectRecorder.registrationSucceeded.contains(label)
               && !subjectRecorder.registrationFailed.contains(label),
            "synchronization-point registration did not succeed");
         byte[] observerAnnouncementTag = observerRecorder.announcements.get(label);
         JavaTckSupport.check(observerAnnouncementTag != null
               && observerAnnouncementTag.length == 0,
            "observer did not receive the empty-tag synchronization announcement");

         observer.synchronizationPointAchieved(label);
         int subjectSynchronizationsBefore = subjectRecorder.synchronizations.size();
         int observerSynchronizationsBefore = observerRecorder.synchronizations.size();
         ParameterHandleValueMap parameters =
            subject.getParameterHandleValueMapFactory().create(1);
         HLAunicodeString encodedLabel = encoder.createHLAunicodeString(label);
         parameters.put(labelParameter, encodedLabel.toByteArray());
         subject.sendInteraction(request, parameters, new byte[0]);

         await(subject, observer, callbackModel,
            () -> subjectRecorder.synchronizations.size() == subjectSynchronizationsBefore + 1
               && observerRecorder.synchronizations.size() == observerSynchronizationsBefore + 1,
            "federation-synchronized callbacks after HLAsynchronizationPointAchieved");
         SynchronizationResult subjectResult =
            subjectRecorder.synchronizations.get(subjectSynchronizationsBefore);
         SynchronizationResult observerResult =
            observerRecorder.synchronizations.get(observerSynchronizationsBefore);
         JavaTckSupport.check(label.equals(subjectResult.label)
               && label.equals(observerResult.label)
               && subjectResult.failedToSync.isEmpty()
               && observerResult.failedToSync.isEmpty(),
            "HLAsynchronizationPointAchieved did not complete the announced point successfully");
      } finally {
         if (requestPublished && subjectJoined) {
            try {
               subject.unpublishInteractionClass(
                  subject.getInteractionClassHandle(REQUEST_CLASS));
            } catch (Exception ignored) {
            }
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

   private static void await(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.01);
            second.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static final class Recorder {
      private final CopyOnWriteArrayList<String> registrationSucceeded =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<String> registrationFailed =
         new CopyOnWriteArrayList<>();
      private final ConcurrentHashMap<String, byte[]> announcements =
         new ConcurrentHashMap<>();
      private final CopyOnWriteArrayList<SynchronizationResult> synchronizations =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("synchronizationPointRegistrationSucceeded".equals(name)) {
                  registrationSucceeded.add((String) arguments[0]);
               } else if ("synchronizationPointRegistrationFailed".equals(name)) {
                  registrationFailed.add((String) arguments[0]);
               } else if ("announceSynchronizationPoint".equals(name)) {
                  announcements.put((String) arguments[0], ((byte[]) arguments[1]).clone());
               } else if ("federationSynchronized".equals(name)) {
                  FederateHandleSet failedToSync = ((FederateHandleSet) arguments[1]).clone();
                  synchronizations.add(new SynchronizationResult(
                     (String) arguments[0], failedToSync));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Table 7 synchronization recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }

   private static final class SynchronizationResult {
      private final String label;
      private final FederateHandleSet failedToSync;

      private SynchronizationResult(String label, FederateHandleSet failedToSync) {
         this.label = label;
         this.failedToSync = failedToSync;
      }
   }
}
