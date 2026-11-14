package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Preserves one pending receive-order interaction through save and restore. */
final class SaveRestoreQueuedReceiveOrderInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] PARAMETER = new byte[] {0x52, 0x4f, 0x49};
   private static final byte[] TAG = new byte[] {0x53, 0x41, 0x56, 0x45};

   private SaveRestoreQueuedReceiveOrderInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "queued receive-order save/restore is scoped to HLA_EVOKED");
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty(),
         "queued receive-order save/restore requires an adapter FOM and time implementation");

      String federation = "java-tck-save-restore-queued-ro-" + UUID.randomUUID();
      String publisherName = "java-tck-queued-ro-save-publisher";
      String subscriberName = "java-tck-queued-ro-save-subscriber";
      String saveLabel = "java-tck-save-queued-ro-interaction";

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;

      RTIambassador restoredPublisher = factory.getRtiAmbassador();
      RTIambassador restoredSubscriber = factory.getRtiAmbassador();
      Recorder restoredPublisherRecorder = new Recorder();
      Recorder restoredSubscriberRecorder = new Recorder();
      boolean restoredPublisherConnected = false;
      boolean restoredSubscriberConnected = false;
      boolean restoredCreated = false;
      boolean restoredPublisherJoined = false;
      boolean restoredSubscriberJoined = false;

      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(publisherName, federateType, federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(subscriberName, federateType, federation);
         subscriberJoined = true;

         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subscriberInteraction =
            subscriber.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle subscriberParameter =
            subscriber.getParameterHandle(subscriberInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction != null && subscriberInteraction != null
               && publisherParameter != null && subscriberParameter != null,
            "adapter FOM omitted the selected receive-order interaction or parameter");
         publisher.publishInteractionClass(publisherInteraction);
         subscriber.subscribeInteractionClass(subscriberInteraction);
         drainPair(publisher, subscriber);

         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, PARAMETER.clone());
         publisher.sendInteraction(publisherInteraction, parameters, TAG.clone());
         JavaTckSupport.check(subscriberRecorder.interactions.isEmpty(),
            "receive-order message was delivered before the federation save request");

         publisher.requestFederationSave(saveLabel);
         await(publisher, subscriber, callbackModel,
            () -> publisherRecorder.saveInitiations.size() == 1
               && subscriberRecorder.saveInitiations.size() == 1,
            "queued receive-order federation save initiation");
         JavaTckSupport.check(publisherRecorder.saveInitiations.equals(Arrays.asList(saveLabel))
               && subscriberRecorder.saveInitiations.equals(Arrays.asList(saveLabel)),
            "queued receive-order save used an unexpected label");
         publisher.federateSaveBegun();
         subscriber.federateSaveBegun();
         publisher.federateSaveComplete();
         subscriber.federateSaveComplete();
         await(publisher, subscriber, callbackModel,
            () -> publisherRecorder.savedCount.get() == 1
               && subscriberRecorder.savedCount.get() == 1,
            "queued receive-order federation save completion");
         JavaTckSupport.check(publisherRecorder.saveFailures.isEmpty()
               && subscriberRecorder.saveFailures.isEmpty(),
            "queued receive-order federation save reported failure");

         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
         subscriber.disconnect();
         subscriberConnected = false;
         publisher.disconnect();
         publisherConnected = false;

         restoredPublisher.connect(restoredPublisherRecorder.proxy(), callbackModel);
         restoredPublisherConnected = true;
         restoredSubscriber.connect(restoredSubscriberRecorder.proxy(), callbackModel);
         restoredSubscriberConnected = true;
         restoredPublisher.createFederationExecution(federation, fom, timeImplementation);
         restoredCreated = true;
         restoredPublisher.joinFederationExecution(publisherName, federateType, federation);
         restoredPublisherJoined = true;
         restoredSubscriber.joinFederationExecution(subscriberName, federateType, federation);
         restoredSubscriberJoined = true;

         InteractionClassHandle restoredInteraction =
            restoredSubscriber.getInteractionClassHandle(interactionClassName);
         ParameterHandle restoredParameter =
            restoredSubscriber.getParameterHandle(restoredInteraction, parameterName);
         JavaTckSupport.check(restoredInteraction != null && restoredParameter != null
               && restoredSubscriberRecorder.interactions.isEmpty(),
            "queued interaction was delivered before the saved federation was restored");

         restoredPublisher.requestFederationRestore(saveLabel);
         await(restoredPublisher, restoredSubscriber, callbackModel,
            () -> !restoredPublisherRecorder.restoreRequestFailures.isEmpty()
               || (restoredPublisherRecorder.restoreSuccesses.size() == 1
                  && restoredPublisherRecorder.restoreInitiations.size() == 1
                  && restoredSubscriberRecorder.restoreInitiations.size() == 1),
            "queued receive-order federation restore request result");
         JavaTckSupport.check(restoredPublisherRecorder.restoreRequestFailures.isEmpty()
               && restoredPublisherRecorder.restoreSuccesses.equals(Arrays.asList(saveLabel)),
            "queued receive-order restore was rejected or returned a different label");
         JavaTckSupport.check(restoredPublisherRecorder.restoreInitiations.size() == 1
               && restoredSubscriberRecorder.restoreInitiations.size() == 1,
            "queued receive-order restore did not initiate for both saved federates");
         RestoreInitiation publisherRestore =
            restoredPublisherRecorder.restoreInitiations.get(0);
         RestoreInitiation subscriberRestore =
            restoredSubscriberRecorder.restoreInitiations.get(0);
         JavaTckSupport.check(publisherRestore.matches(
               saveLabel, publisherName) && subscriberRestore.matches(
               saveLabel, subscriberName),
            "queued receive-order restore metadata mismatch: publisher=("
               + publisherRestore.label + ", " + publisherRestore.federateName + ", handle="
               + (publisherRestore.postRestoreHandle != null) + "), subscriber=("
               + subscriberRestore.label + ", " + subscriberRestore.federateName + ", handle="
               + (subscriberRestore.postRestoreHandle != null) + ")");

         restoredPublisher.federateRestoreComplete();
         restoredSubscriber.federateRestoreComplete();
         await(restoredPublisher, restoredSubscriber, callbackModel,
            () -> restoredPublisherRecorder.restoredCount.get() == 1
               && restoredSubscriberRecorder.restoredCount.get() == 1
               && !restoredSubscriberRecorder.interactions.isEmpty(),
            "queued receive-order interaction delivery after restore");

         JavaTckSupport.check(restoredSubscriberRecorder.interactions.size() == 1,
            "saved receive-order interaction was not delivered exactly once after restore");
         Interaction received = restoredSubscriberRecorder.interactions.get(0);
         JavaTckSupport.check(restoredInteraction.equals(received.interaction)
               && publisherRestore.postRestoreHandle.equals(received.producer)
               && received.parameters.size() == 1
               && received.parameters.containsKey(restoredParameter)
               && Arrays.equals(PARAMETER, received.parameters.get(restoredParameter))
               && Arrays.equals(TAG, received.tag)
               && received.transportation != null
               && received.transportation.encodedLength() > 0
               && received.regionsEmpty,
            "restored receive-order interaction changed its identity or payload");
         drainPair(restoredPublisher, restoredSubscriber);
         JavaTckSupport.check(restoredSubscriberRecorder.interactions.size() == 1,
            "restored receive-order interaction was delivered more than once");
      } finally {
         if (restoredSubscriberJoined) resign(restoredSubscriber);
         if (restoredPublisherJoined) resign(restoredPublisher);
         if (restoredCreated) destroy(restoredPublisher, federation);
         if (restoredSubscriberConnected) disconnect(restoredSubscriber);
         if (restoredPublisherConnected) disconnect(restoredPublisher);
         if (subscriberJoined) resign(subscriber);
         if (publisherJoined) resign(publisher);
         if (created) destroy(publisher, federation);
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void drainPair(RTIambassador first, RTIambassador second) throws Exception {
      for (int pass = 0; pass < 32; ++pass) {
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
      }
   }

   private static void await(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel, Ready ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !ready.isReady()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         } else {
            Thread.sleep(1L);
         }
      }
      JavaTckSupport.check(ready.isReady(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void destroy(RTIambassador ambassador, String federation) {
      try { ambassador.destroyFederationExecution(federation); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Ready { boolean isReady(); }

   private static final class RestoreInitiation {
      private final String label;
      private final String federateName;
      private final FederateHandle postRestoreHandle;

      private RestoreInitiation(String label, String federateName,
            FederateHandle postRestoreHandle) {
         this.label = label;
         this.federateName = federateName;
         this.postRestoreHandle = postRestoreHandle;
      }

      private boolean matches(String expectedLabel, String expectedName) {
         return expectedLabel.equals(label) && expectedName.equals(federateName)
            && postRestoreHandle != null;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet regions = (RegionHandleSet) arguments[5];
         regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final AtomicInteger savedCount = new AtomicInteger();
      private final AtomicInteger restoredCount = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount.incrementAndGet();
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add(arguments[0]);
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures.add((String) arguments[0]);
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               } else if ("federationRestored".equals(name)) {
                  restoredCount.incrementAndGet();
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add(arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  interactions.add(new Interaction(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK queued receive-order callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
