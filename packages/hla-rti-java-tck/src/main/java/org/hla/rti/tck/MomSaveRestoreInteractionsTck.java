package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Exercise the standard MOM save and restore interactions during active transitions. */
final class MomSaveRestoreInteractionsTck {
   private static final String SAVE_BEGUN =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateSaveBegun";
   private static final String SAVE_COMPLETE =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateSaveComplete";
   private static final String RESTORE_COMPLETE =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateRestoreComplete";
   private static final String SUCCESS_INDICATOR = "HLAsuccessIndicator";
   private static final String FEDERATE_TYPE = "tck-type";

   private MomSaveRestoreInteractionsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String applicationInteractionName, String applicationParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-save-restore-interactions-" + UUID.randomUUID();
      String subjectName = "java-tck-mom-save-restore-subject";
      String peerName = "java-tck-mom-save-restore-peer";
      String saveLabel = "java-tck-mom-save-restore-interactions";
      boolean subjectConnected = false;
      boolean peerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean peerJoined = false;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, FEDERATE_TYPE, federation);
         subjectJoined = true;
         peer.joinFederationExecution(peerName, FEDERATE_TYPE, federation);
         peerJoined = true;

         InteractionSet subjectMom = resolveMomInteractions(subject);
         InteractionSet peerMom = resolveMomInteractions(peer);
         JavaTckSupport.check(subjectMom.sameHandles(peerMom),
            "standard MOM interaction handles differed across federation members");
         InteractionClassHandle subjectApplication =
            subject.getInteractionClassHandle(applicationInteractionName);
         InteractionClassHandle peerApplication =
            peer.getInteractionClassHandle(applicationInteractionName);
         ParameterHandle subjectApplicationParameter =
            subject.getParameterHandle(subjectApplication, applicationParameterName);
         ParameterHandle peerApplicationParameter =
            peer.getParameterHandle(peerApplication, applicationParameterName);
         JavaTckSupport.check(subjectApplication.equals(peerApplication)
               && subjectApplicationParameter.equals(peerApplicationParameter),
            "application interaction handles differed across federation members");

         publish(subject, subjectMom, subjectApplication);
         publish(peer, peerMom, peerApplication);

         subject.requestFederationSave(saveLabel);
         await(callbackModel, subject, peer, subjectRecorder, peerRecorder,
            "save initiation", () -> subjectRecorder.saveInitiations.size() == 1
               && peerRecorder.saveInitiations.size() == 1);
         JavaTckSupport.check(saveLabel.equals(subjectRecorder.saveInitiations.get(0))
               && saveLabel.equals(peerRecorder.saveInitiations.get(0)),
            "save initiation returned the wrong label");
         JavaTckSupport.expectFailure(() -> sendApplication(subject, subjectApplication,
            subjectApplicationParameter), "SaveInProgress",
            "application interaction send during save");

         send(subject, subjectMom.saveBegun, subject.getParameterHandleValueMapFactory().create(0));
         send(peer, peerMom.saveBegun, peer.getParameterHandleValueMapFactory().create(0));
         ParameterHandleValueMap subjectSuccess = successParameter(
            subject, subjectMom.saveCompleteSuccess, encoder);
         ParameterHandleValueMap peerSuccess = successParameter(
            peer, peerMom.saveCompleteSuccess, encoder);
         send(subject, subjectMom.saveComplete, subjectSuccess);
         send(peer, peerMom.saveComplete, peerSuccess);
         await(callbackModel, subject, peer, subjectRecorder, peerRecorder,
            "save completion", () -> subjectRecorder.savedCount.get() == 1
               && peerRecorder.savedCount.get() == 1);
         JavaTckSupport.check(subjectRecorder.saveFailureCount.get() == 0
               && peerRecorder.saveFailureCount.get() == 0,
            "save reported a failure after successful MOM completion interactions");

         subject.requestFederationRestore(saveLabel);
         await(callbackModel, subject, peer, subjectRecorder, peerRecorder,
            "restore initiation", () -> subjectRecorder.restoreInitiations.size() == 1
               && peerRecorder.restoreInitiations.size() == 1);
         JavaTckSupport.check(saveLabel.equals(subjectRecorder.restoreInitiations.get(0))
               && saveLabel.equals(peerRecorder.restoreInitiations.get(0)),
            "restore initiation returned the wrong label");
         JavaTckSupport.expectFailure(() -> sendApplication(subject, subjectApplication,
            subjectApplicationParameter), "RestoreInProgress",
            "application interaction send during restore");

         send(subject, subjectMom.restoreComplete,
            successParameter(subject, subjectMom.restoreCompleteSuccess, encoder));
         send(peer, peerMom.restoreComplete,
            successParameter(peer, peerMom.restoreCompleteSuccess, encoder));
         await(callbackModel, subject, peer, subjectRecorder, peerRecorder,
            "restore completion", () -> subjectRecorder.restoredCount.get() == 1
               && peerRecorder.restoredCount.get() == 1);
         JavaTckSupport.check(subjectRecorder.restoreFailureCount.get() == 0
               && peerRecorder.restoreFailureCount.get() == 0,
            "restore reported a failure after successful MOM completion interactions");
      } finally {
         if (subjectJoined) resign(subject);
         if (peerJoined) resign(peer);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (peerConnected) disconnect(peer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static InteractionSet resolveMomInteractions(RTIambassador ambassador)
         throws Exception {
      InteractionClassHandle saveBegun = ambassador.getInteractionClassHandle(SAVE_BEGUN);
      InteractionClassHandle saveComplete = ambassador.getInteractionClassHandle(SAVE_COMPLETE);
      InteractionClassHandle restoreComplete =
         ambassador.getInteractionClassHandle(RESTORE_COMPLETE);
      ParameterHandle saveSuccess = ambassador.getParameterHandle(saveComplete, SUCCESS_INDICATOR);
      ParameterHandle restoreSuccess =
         ambassador.getParameterHandle(restoreComplete, SUCCESS_INDICATOR);
      return new InteractionSet(saveBegun, saveComplete, restoreComplete,
         saveSuccess, restoreSuccess);
   }

   private static void publish(RTIambassador ambassador, InteractionSet mom,
         InteractionClassHandle application) throws Exception {
      ambassador.publishInteractionClass(mom.saveBegun);
      ambassador.publishInteractionClass(mom.saveComplete);
      ambassador.publishInteractionClass(mom.restoreComplete);
      ambassador.publishInteractionClass(application);
   }

   private static ParameterHandleValueMap successParameter(RTIambassador ambassador,
         ParameterHandle handle, EncoderFactory encoder) throws Exception {
      HLAboolean success = encoder.createHLAboolean(true);
      ParameterHandleValueMap parameters =
         ambassador.getParameterHandleValueMapFactory().create(1);
      parameters.put(handle, success.toByteArray());
      return parameters;
   }

   private static void send(RTIambassador ambassador, InteractionClassHandle interaction,
         ParameterHandleValueMap parameters) throws Exception {
      ambassador.sendInteraction(interaction, parameters, new byte[0]);
   }

   private static void sendApplication(RTIambassador ambassador,
         InteractionClassHandle interaction, ParameterHandle parameter) throws Exception {
      ParameterHandleValueMap parameters =
         ambassador.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, new byte[] {0x53, 0x52});
      send(ambassador, interaction, parameters);
   }

   private static void await(CallbackModel callbackModel, RTIambassador subject,
         RTIambassador peer, Recorder subjectRecorder, Recorder peerRecorder,
         String description, Condition condition) throws Exception {
      long deadline = System.nanoTime() + 15_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         pump(callbackModel, subject, peer);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
      JavaTckSupport.check(subjectRecorder.unexpectedCallbackCount.get() == 0
            && peerRecorder.unexpectedCallbackCount.get() == 0,
         "save or restore transition reported an unsuccessful completion");
   }

   private static void pump(CallbackModel callbackModel, RTIambassador subject,
         RTIambassador peer) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         subject.evokeMultipleCallbacks(0, 1);
         peer.evokeMultipleCallbacks(0, 1);
      } else {
         subject.getInteractionClassHandle(SAVE_BEGUN);
         peer.getInteractionClassHandle(SAVE_BEGUN);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static final class InteractionSet {
      private final InteractionClassHandle saveBegun;
      private final InteractionClassHandle saveComplete;
      private final InteractionClassHandle restoreComplete;
      private final ParameterHandle saveCompleteSuccess;
      private final ParameterHandle restoreCompleteSuccess;

      private InteractionSet(InteractionClassHandle saveBegun,
            InteractionClassHandle saveComplete, InteractionClassHandle restoreComplete,
            ParameterHandle saveCompleteSuccess, ParameterHandle restoreCompleteSuccess) {
         this.saveBegun = saveBegun;
         this.saveComplete = saveComplete;
         this.restoreComplete = restoreComplete;
         this.saveCompleteSuccess = saveCompleteSuccess;
         this.restoreCompleteSuccess = restoreCompleteSuccess;
      }

      private boolean sameHandles(InteractionSet other) {
         return saveBegun.equals(other.saveBegun) && saveComplete.equals(other.saveComplete)
            && restoreComplete.equals(other.restoreComplete)
            && saveCompleteSuccess.equals(other.saveCompleteSuccess)
            && restoreCompleteSuccess.equals(other.restoreCompleteSuccess);
      }
   }

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<String> restoreInitiations = new CopyOnWriteArrayList<>();
      private final AtomicInteger savedCount = new AtomicInteger();
      private final AtomicInteger saveFailureCount = new AtomicInteger();
      private final AtomicInteger restoredCount = new AtomicInteger();
      private final AtomicInteger restoreFailureCount = new AtomicInteger();
      private final AtomicInteger unexpectedCallbackCount = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount.incrementAndGet();
               } else if ("federationNotSaved".equals(name)) {
                  saveFailureCount.incrementAndGet();
                  unexpectedCallbackCount.incrementAndGet();
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add((String) arguments[0]);
               } else if ("federationRestored".equals(name)) {
                  restoredCount.incrementAndGet();
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailureCount.incrementAndGet();
                  unexpectedCallbackCount.incrementAndGet();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK MOM save/restore recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
