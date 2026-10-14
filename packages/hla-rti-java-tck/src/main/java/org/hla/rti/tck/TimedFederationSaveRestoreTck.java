package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Preserves queued timestamped interaction delivery and retraction across save/restore. */
final class TimedFederationSaveRestoreTck {
   private TimedFederationSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder regulatorRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-timed-save-restore-" + UUID.randomUUID();
      String regulatorName = "java-tck-timed-save-regulator";
      String constrainedName = "java-tck-timed-save-constrained";
      String saveLabel = "java-tck-timed-save-restore";
      boolean regulatorConnected = false;
      boolean constrainedConnected = false;
      boolean created = false;
      boolean regulatorJoined = false;
      boolean constrainedJoined = false;
      boolean regulatorTimeRegulating = false;
      boolean constrainedTimeConstrained = false;
      boolean published = false;
      boolean subscribed = false;
      InteractionClassHandle regulatorInteraction = null;
      InteractionClassHandle constrainedInteraction = null;
      try {
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle regulatorHandle = regulator.joinFederationExecution(
            regulatorName, federation);
         regulatorJoined = true;
         constrained.joinFederationExecution(constrainedName, federation);
         constrainedJoined = true;

         regulatorInteraction = regulator.getInteractionClassHandle(interactionClassName);
         ParameterHandle regulatorParameter = regulator.getParameterHandle(
            regulatorInteraction, parameterName);
         constrainedInteraction = constrained.getInteractionClassHandle(interactionClassName);
         ParameterHandle constrainedParameter = constrained.getParameterHandle(
            constrainedInteraction, parameterName);
         JavaTckSupport.check(regulatorInteraction != null && regulatorParameter != null
               && regulatorInteraction.equals(constrainedInteraction)
               && regulatorParameter.equals(constrainedParameter),
            "timed save/restore interaction and parameter handles did not agree across members");
         regulator.publishInteractionClass(regulatorInteraction);
         published = true;
         constrained.subscribeInteractionClass(constrainedInteraction);
         subscribed = true;
         regulator.changeInteractionOrderType(regulatorInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(constrained.getTimeFactory().getName()),
            "timed save/restore members selected different logical-time factories");
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         regulator.enableTimeRegulation(lookahead);
         regulatorTimeRegulating = true;
         await(callbackModel, objectClassName, "time-regulation enablement",
            () -> regulatorRecorder.timeRegulationEnabled, regulator);
         constrained.enableTimeConstrained();
         constrainedTimeConstrained = true;
         await(callbackModel, objectClassName, "time-constrained enablement",
            () -> constrainedRecorder.timeConstrainedEnabled, constrained);

         LogicalTime regulatorInitial = regulator.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(constrainedInitial),
            "timed save/restore members began at different logical times");
         LogicalTime saveTime = offset(regulatorInitial, lookahead, 2);
         LogicalTime queuedTime = offset(regulatorInitial, lookahead, 4);
         LogicalTime releaseTime = offset(regulatorInitial, lookahead, 6);
         byte[] payload = new byte[] {0x54, 0x53, 0x4f};
         byte[] tag = new byte[] {0x53, 0x41, 0x56, 0x45};
         ParameterHandleValueMap parameters =
            regulator.getParameterHandleValueMapFactory().create(1);
         parameters.put(regulatorParameter, payload);
         MessageRetractionReturn sent = regulator.sendInteraction(
            regulatorInteraction, parameters, tag, queuedTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null && sent.handle.encodedLength() > 0,
            "timed save/restore send returned no valid message retraction handle");
         JavaTckSupport.check(constrainedRecorder.interactions.isEmpty(),
            "constrained member received the future interaction before its time advance");

         regulator.requestFederationSave(saveLabel, saveTime);
         constrained.timeAdvanceRequest(saveTime);
         regulator.timeAdvanceRequest(saveTime);
         await(callbackModel, objectClassName, "timed save initiation", () ->
            regulatorRecorder.saveInitiations.size() == 1
               && constrainedRecorder.saveInitiations.size() == 1,
            regulator, constrained);
         verifyTimedSave(regulatorRecorder.saveInitiations.get(0), saveLabel, saveTime);
         verifyTimedSave(constrainedRecorder.saveInitiations.get(0), saveLabel, saveTime);
         JavaTckSupport.check(saveTime.equals(regulatorRecorder.timeAdvanceGrant)
               && saveTime.equals(constrainedRecorder.timeAdvanceGrant)
               && constrainedRecorder.interactions.isEmpty(),
            "save boundary delivered the queued interaction or granted the wrong time");

         regulator.federateSaveBegun();
         constrained.federateSaveBegun();
         regulator.federateSaveComplete();
         constrained.federateSaveComplete();
         await(callbackModel, objectClassName, "timed federationSaved callbacks",
            () -> regulatorRecorder.savedCount == 1 && constrainedRecorder.savedCount == 1,
            regulator, constrained);
         JavaTckSupport.check(regulatorRecorder.saveFailures.isEmpty()
               && constrainedRecorder.saveFailures.isEmpty(),
            "successful timed federation save reported a failure");

         regulator.requestFederationRestore(saveLabel);
         await(callbackModel, objectClassName, "timed restore initiation", () ->
            regulatorRecorder.restoreSuccesses.size() == 1
               && regulatorRecorder.restoreBegunCount == 1
               && constrainedRecorder.restoreBegunCount == 1
               && regulatorRecorder.restoreInitiations.size() == 1
               && constrainedRecorder.restoreInitiations.size() == 1,
            regulator, constrained);
         JavaTckSupport.check(saveLabel.equals(regulatorRecorder.restoreSuccesses.get(0))
               && regulatorRecorder.restoreRequestFailures.isEmpty()
               && constrainedRecorder.restoreRequestFailures.isEmpty(),
            "timed restore request returned an unexpected result");
         RestoreInitiation regulatorRestore = regulatorRecorder.restoreInitiations.get(0);
         RestoreInitiation constrainedRestore = constrainedRecorder.restoreInitiations.get(0);
         checkRestore(regulatorRestore, saveLabel, regulatorName);
         checkRestore(constrainedRestore, saveLabel, constrainedName);

         regulator.federateRestoreComplete();
         constrained.federateRestoreComplete();
         await(callbackModel, objectClassName, "timed federationRestored callbacks",
            () -> regulatorRecorder.restoredCount == 1
               && constrainedRecorder.restoredCount == 1,
            regulator, constrained);
         JavaTckSupport.check(regulatorRecorder.restoreFailures.isEmpty()
               && constrainedRecorder.restoreFailures.isEmpty(),
            "successful timed federation restore reported a failure");
         JavaTckSupport.check(regulator.getFederateHandle(regulatorName).equals(
               regulatorRestore.postRestoreHandle)
               && constrained.getFederateHandle(constrainedName).equals(
                  constrainedRestore.postRestoreHandle),
            "timed restore did not make the post-restore handles authoritative");

         constrainedRecorder.clearDeliveryEvents();
         constrained.flushQueueRequest(releaseTime);
         await(callbackModel, objectClassName, "restored timestamped interaction and grant",
            () -> constrainedRecorder.interactions.size() == 1
               && constrainedRecorder.flushQueueGrantTimes.size() == 1,
            constrained);
         TimedInteraction restored = constrainedRecorder.interactions.get(0);
         JavaTckSupport.check(constrainedInteraction.equals(restored.interaction)
               && restored.parameters.size() == 1
               && Arrays.equals(payload, restored.parameters.get(constrainedParameter))
               && Arrays.equals(tag, restored.tag)
               && regulatorRestore.postRestoreHandle.equals(restored.producer)
               && queuedTime.equals(restored.time)
               && restored.sentOrder == OrderType.TIMESTAMP
               && restored.receivedOrder == OrderType.TIMESTAMP
               && restored.retraction != null && sent.handle.equals(restored.retraction),
            "restored timestamped interaction changed its standard delivery data");
         JavaTckSupport.check(constrainedRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "flushQueueGrant")),
            "restored timestamped interaction was not delivered before the Flush Queue grant");
         JavaTckSupport.check(releaseTime.equals(constrainedRecorder.flushQueueGrantTimes.get(0)),
            "Flush Queue grant returned the wrong requested release time");

         regulator.retract(sent.handle);
         await(callbackModel, objectClassName, "restored request-retraction callback",
            () -> constrainedRecorder.retractions.size() == 1, constrained);
         JavaTckSupport.check(sent.handle.equals(constrainedRecorder.retractions.get(0)),
            "restored retraction callback returned a different handle");
         JavaTckSupport.expectFailure(() -> regulator.retract(sent.handle),
            "MessageCanNoLongerBeRetracted",
            "second retraction of the restored timestamped interaction was accepted");
      } finally {
         if (constrainedTimeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulatorTimeRegulating && regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && constrainedJoined && constrainedInteraction != null) {
            try { constrained.unsubscribeInteractionClass(constrainedInteraction); }
            catch (Exception ignored) { }
         }
         if (published && regulatorJoined && regulatorInteraction != null) {
            try { regulator.unpublishInteractionClass(regulatorInteraction); }
            catch (Exception ignored) { }
         }
         if (constrainedJoined) resign(constrained);
         if (regulatorJoined) resign(regulator);
         if (created) {
            try { regulator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (constrainedConnected) disconnect(constrained);
         if (regulatorConnected) disconnect(regulator);
      }
   }

   private static void verifyTimedSave(SaveInitiation initiation, String label,
         LogicalTime<?, ?> expectedTime) {
      JavaTckSupport.check(label.equals(initiation.label)
            && expectedTime.equals(initiation.time),
         "timed save initiation returned the wrong label or timestamp");
   }

   private static void checkRestore(RestoreInitiation initiation, String label,
         String expectedName) {
      JavaTckSupport.check(label.equals(initiation.label)
            && expectedName.equals(initiation.federateName)
            && initiation.postRestoreHandle != null
            && initiation.postRestoreHandle.encodedLength() > 0,
         "timed restore initiation returned incorrect identity metadata");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(CallbackModel model, String objectClassName, String description,
         Condition condition, RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + 15_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         for (RTIambassador ambassador : ambassadors) {
            if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
            else ambassador.getObjectClassHandle(objectClassName);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
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

   private static final class SaveInitiation {
      private final String label;
      private final LogicalTime<?, ?> time;
      private SaveInitiation(String label, LogicalTime<?, ?> time) {
         this.label = label;
         this.time = time;
      }
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

   private static final class TimedInteraction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private final List<SaveInitiation> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<TimedInteraction> interactions = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> timeAdvanceGrant;
      private volatile int savedCount;
      private volatile int restoreBegunCount;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add(new SaveInitiation((String) arguments[0],
                     arguments.length > 1 ? (LogicalTime<?, ?>) arguments[1] : null));
               } else if ("federationSaved".equals(name)) {
                  savedCount++;
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add((SaveFailureReason) arguments[0]);
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures.add((String) arguments[0]);
               } else if ("federationRestoreBegun".equals(name)) {
                  restoreBegunCount++;
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               } else if ("federationRestored".equals(name)) {
                  restoredCount++;
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add(arguments[0]);
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               } else if ("receiveInteraction".equals(name) && arguments.length == 10) {
                  interactions.add(new TimedInteraction(arguments));
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name) && arguments.length == 2) {
                  flushQueueGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name) && arguments.length == 1) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timed save/restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private void clearDeliveryEvents() {
         interactions.clear();
         retractions.clear();
         callbackOrder.clear();
         flushQueueGrantTimes.clear();
      }
   }
}
