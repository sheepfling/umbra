package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleFactory;
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Verifies the federate-lost MOM report and queued TSO cutoff after adapter-managed loss. */
final class FederateLostMomReportTck {
   private static final String REPORT_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost";
   private static final String FEDERATION_MOM_CLASS =
      "HLAobjectRoot.HLAmanager.HLAfederation";

   private FederateLostMomReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel,
         RtiConfiguration configuration, String federation, String observerName, String lostName,
         String readyMarkerPath, String completionMarkerPath, long timeoutMillis) throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty()
            && completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "adapter connection-loss readiness and completion markers are required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");
      JavaTckSupport.check(interactionClassName != null && !interactionClassName.isEmpty(),
         "adapter application interaction class is required for the lost-regulator TSO cutoff");
      JavaTckSupport.check(federation != null && !federation.isEmpty()
            && observerName != null && !observerName.isEmpty()
            && lostName != null && !lostName.isEmpty(),
         "adapter connection-loss federation and member names are required");

      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      Recorder observerRecorder = new Recorder();
      Recorder lostRecorder = new Recorder();
      AtomicReference<String> connectionLostDescription = new AtomicReference<>();
      EncoderFactory encoder = factory.getEncoderFactory();
      boolean observerConnected = false, lostConnected = false, created = false;
      boolean observerJoined = false;
      InteractionClassHandle reportClass = null, observerInteraction = null;
      try {
         observer.connect(observerRecorder.proxy(null), callbackModel, configuration);
         observerConnected = true;
         lost.connect(lostRecorder.proxy(connectionLostDescription), callbackModel, configuration);
         lostConnected = true;
         observer.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle observerFederate = observer.joinFederationExecution(
            observerName, "java-tck", federation);
         observerJoined = true;
         FederateHandle lostFederate = lost.joinFederationExecution(
            lostName, "java-tck", federation);

         ObjectClassHandle federationMomClass =
            observer.getObjectClassHandle(FEDERATION_MOM_CLASS);
         AttributeHandle rosterAttribute = observer.getAttributeHandle(
            federationMomClass, "HLAfederatesInFederation");
         JavaTckSupport.check(federationMomClass != null && rosterAttribute != null,
            "standard HLAfederation roster lookup failed");
         AttributeHandleSet rosterAttributes =
            JavaTckSupport.attributeSet(observer, rosterAttribute);
         FederateHandleFactory federateHandleFactory = observer.getFederateHandleFactory();
         JavaTckSupport.check(federateHandleFactory != null,
            "standard federate-handle factory was null");
         observerRecorder.watchFederationRoster(federationMomClass, rosterAttribute);
         observer.subscribeObjectClassAttributes(federationMomClass, rosterAttributes);
         await(observer, callbackModel, interactionClassName,
            () -> observerRecorder.federationMomObject != null,
            "federation MOM object discovery", timeoutMillis);
         JavaTckSupport.check(federationMomClass.equals(
               observerRecorder.federationMomObjectClass)
               && "HLAfederation".equals(observerRecorder.federationMomObjectName),
            "federation MOM discovery did not preserve the HLAfederation identity");

         reportClass = observer.getInteractionClassHandle(REPORT_NAME);
         ParameterHandle federateParameter = observer.getParameterHandle(reportClass, "HLAfederate");
         ParameterHandle nameParameter = observer.getParameterHandle(reportClass, "HLAfederateName");
         ParameterHandle timeParameter = observer.getParameterHandle(reportClass, "HLAtimeStamp");
         ParameterHandle faultParameter = observer.getParameterHandle(reportClass, "HLAfaultDescription");
         JavaTckSupport.check(reportClass != null && federateParameter != null
               && nameParameter != null && timeParameter != null && faultParameter != null,
            "standard HLAreportFederateLost class or parameter lookup failed");
         observer.subscribeInteractionClass(reportClass);

         InteractionClassHandle lostInteraction = lost.getInteractionClassHandle(interactionClassName);
         observerInteraction = lostInteraction;
         JavaTckSupport.check(lostInteraction != null,
            "adapter application interaction lookup failed on the lost member");
         lost.publishInteractionClass(lostInteraction);
         lost.changeInteractionOrderType(lostInteraction, OrderType.TIMESTAMP);
         observer.subscribeInteractionClass(observerInteraction);

         LogicalTimeFactory<?, ?> lostTimeFactory = lost.getTimeFactory();
         LogicalTimeFactory<?, ?> observerTimeFactory = observer.getTimeFactory();
         JavaTckSupport.check(lostTimeFactory.getName().equals(observerTimeFactory.getName()),
            "adapter members selected different logical-time factories");
         LogicalTimeInterval<?> lookahead = (LogicalTimeInterval<?>) lostTimeFactory.makeEpsilon();
         LogicalTimeInterval<?> observerEpsilon =
            (LogicalTimeInterval<?>) observerTimeFactory.makeEpsilon();
         LogicalTime<?, ?> lostInitial = lostTimeFactory.makeInitial();
         LogicalTime<?, ?> observerInitial = observerTimeFactory.makeInitial();
         LogicalTime cutoff = offset(lostInitial, lookahead, 6);
         LogicalTime observerCutoff = offset(observerInitial, observerEpsilon, 6);
         JavaTckSupport.check(cutoff.equals(observerCutoff)
               && Arrays.equals(encode(cutoff), encode(observerCutoff)),
            "adapter members' initial logical times did not produce the same cutoff");

         observer.enableTimeConstrained();
         await(observer, callbackModel, interactionClassName,
            () -> observerRecorder.timeConstrainedEnabled,
            "survivor time-constrained role callback", timeoutMillis);
         lost.enableTimeRegulation(lookahead);
         await(lost, callbackModel, interactionClassName,
            () -> lostRecorder.timeRegulationEnabled,
            "lost member time-regulation role callback", timeoutMillis);
         lost.setAutomaticResignDirective(ResignAction.NO_ACTION);
         observer.enableAsynchronousDelivery();

         byte[] tag = {0x43, 0x55, 0x54};
         ParameterHandleValueMap emptyParameters = lost.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn send = lost.sendInteraction(
            lostInteraction, emptyParameters, tag.clone(), cutoff);
         JavaTckSupport.check(send != null && send.retractionHandleIsValid && send.handle != null,
            "lost regulator cutoff send returned an invalid retraction handle");
         observer.timeAdvanceRequest(observerCutoff);
         lost.timeAdvanceRequest(cutoff);
         await(lost, callbackModel, interactionClassName,
            () -> !lostRecorder.timeAdvanceGrants.isEmpty(),
            "lost member grant before connection-loss injection", timeoutMillis);
         JavaTckSupport.check(lostRecorder.timeAdvanceGrants.size() == 1
               && cutoff.equals(lostRecorder.timeAdvanceGrants.get(0).time)
               && Arrays.equals(encode(cutoff), encode(lostRecorder.timeAdvanceGrants.get(0).time)),
            "lost member did not reach the timestamped cutoff before fault injection");

         int membershipReflectionsBeforeLoss = observerRecorder.rosterReflectionCount.get();
         writeMarker(Path.of(readyMarkerPath), "ready\n");
         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while (System.nanoTime() < deadline && (connectionLostDescription.get() == null
               || observerRecorder.reports.size() < 1
               || observerRecorder.timestamped.size() < 1
               || observerRecorder.timeAdvanceGrants.size() < 1
               || observerRecorder.rosterReflectionCount.get() <= membershipReflectionsBeforeLoss
               || !Set.of(observerFederate).equals(observerRecorder.currentRoster(
                  encoder, federateHandleFactory, rosterAttribute)))) {
            service(lost, callbackModel, interactionClassName, true);
            service(observer, callbackModel, interactionClassName, false);
            if (connectionLostDescription.get() == null || observerRecorder.reports.size() < 1
                  || observerRecorder.timestamped.size() < 1
                  || observerRecorder.timeAdvanceGrants.size() < 1
                  || observerRecorder.rosterReflectionCount.get() <= membershipReflectionsBeforeLoss
                  || !Set.of(observerFederate).equals(observerRecorder.currentRoster(
                     encoder, federateHandleFactory, rosterAttribute))) {
               Thread.sleep(1L);
            }
         }
         JavaTckSupport.check(connectionLostDescription.get() != null
               && !connectionLostDescription.get().isEmpty(),
            "timed out waiting for a non-empty standard connectionLost callback");
         JavaTckSupport.check(observerRecorder.reports.size() == 1,
            "timed out waiting for exactly one subscribed HLAreportFederateLost interaction");
         JavaTckSupport.check(observerRecorder.timestamped.size() == 1,
            "timed out waiting for exactly one cutoff timestamped interaction");
         JavaTckSupport.check(observerRecorder.timeAdvanceGrants.size() == 1,
            "timed out waiting for exactly one survivor timeAdvanceGrant");

         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         Interaction report = observerRecorder.reports.get(0);
         JavaTckSupport.check(reportClass.equals(report.interaction)
               && report.parameters.size() == 4
               && report.parameters.containsKey(federateParameter)
               && report.parameters.containsKey(nameParameter)
               && report.parameters.containsKey(timeParameter)
               && report.parameters.containsKey(faultParameter)
               && reliable.equals(report.transportation)
               && report.tag.length == 0 && report.regionsEmpty,
            "HLAreportFederateLost returned a non-standard interaction or parameter set");
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            report.parameters.get(federateParameter), 0);
         JavaTckSupport.check(lostFederate.equals(reportedFederate),
            "HLAreportFederateLost identified the wrong federate");
         HLAunicodeString reportedName = encoder.createHLAunicodeString();
         reportedName.decode(report.parameters.get(nameParameter));
         JavaTckSupport.check(lostName.equals(reportedName.getValue()),
            "HLAreportFederateLost returned the wrong federate name");
         LogicalTime<?, ?> reportedTime = (LogicalTime<?, ?>) observer.getTimeFactory().decodeTime(
            report.parameters.get(timeParameter), 0);
         JavaTckSupport.check(reportedTime != null && !reportedTime.toString().isEmpty()
               && cutoff.equals(reportedTime)
               && Arrays.equals(encode(cutoff), encode(reportedTime)),
            "HLAreportFederateLost HLAtimeStamp did not match the lost member cutoff");
         HLAunicodeString reportedFault = encoder.createHLAunicodeString();
         reportedFault.decode(report.parameters.get(faultParameter));

         TimedInteraction timed = observerRecorder.timestamped.get(0);
         Grant survivorGrant = observerRecorder.timeAdvanceGrants.get(0);
         JavaTckSupport.check(observerInteraction.equals(timed.interaction)
               && timed.parameters.isEmpty()
               && Arrays.equals(tag, timed.tag)
               && lostFederate.equals(timed.producer)
               && cutoff.equals(timed.time)
               && Arrays.equals(encode(cutoff), encode(timed.time))
               && timed.sentOrder == OrderType.TIMESTAMP
               && timed.receivedOrder == OrderType.TIMESTAMP
               && timed.retraction != null
               && send.handle.equals(timed.retraction)
               && Arrays.equals(encode(send.handle), encode(timed.retraction))
               && timed.regionsEmpty,
            "federate loss changed the queued timestamped interaction");
         JavaTckSupport.check(cutoff.equals(survivorGrant.time)
               && Arrays.equals(encode(cutoff), encode(survivorGrant.time)),
            "federate loss granted the survivor at the wrong cutoff time");
         JavaTckSupport.check(timed.callbackSequence < survivorGrant.callbackSequence,
            "survivor received its cutoff grant before the lost member's TSO message");

         JavaTckSupport.check(observerRecorder.rosterReflectionCount.get()
               > membershipReflectionsBeforeLoss
               && Set.of(observerFederate).equals(observerRecorder.currentRoster(
                  encoder, federateHandleFactory, rosterAttribute)),
            "HLAfederatesInFederation did not remove the automatically resigned member");
         observer.listFederationExecutionMembers(federation);
         await(observer, callbackModel, interactionClassName,
            () -> federation.equals(observerRecorder.memberReportFederation)
               && observerRecorder.memberReport != null,
            "post-loss federation member listing", timeoutMillis);
         Set<String> memberNames = new LinkedHashSet<>();
         for (FederationExecutionMemberInformation member : observerRecorder.memberReport) {
            memberNames.add(member.federateName);
         }
         JavaTckSupport.check(memberNames.equals(Set.of(observerName)),
            "post-loss federation membership query retained the automatically resigned member");

         String synchronizationLabel = federation + "-post-loss-survivor-synchronization";
         byte[] synchronizationTag = {0x53, 0x59, 0x4E};
         observer.registerFederationSynchronizationPoint(
            synchronizationLabel, synchronizationTag);
         await(observer, callbackModel, interactionClassName,
            () -> synchronizationLabel.equals(observerRecorder.synchronizationRegistrationLabel)
               && synchronizationLabel.equals(observerRecorder.synchronizationAnnouncementLabel),
            "single-survivor synchronization registration and announcement", timeoutMillis);
         JavaTckSupport.check(!synchronizationLabel.equals(
               observerRecorder.synchronizationRegistrationFailureLabel)
               && Arrays.equals(synchronizationTag, observerRecorder.synchronizationTag),
            "single-survivor synchronization registration failed or changed its tag");
         observer.synchronizationPointAchieved(synchronizationLabel);
         await(observer, callbackModel, interactionClassName,
            () -> synchronizationLabel.equals(observerRecorder.federationSynchronizedLabel)
               && observerRecorder.failedToSynchronize != null,
            "single-survivor Federation Synchronized callback", timeoutMillis);
         JavaTckSupport.check(observerRecorder.failedToSynchronize.isEmpty(),
            "single-survivor synchronization returned an unexpected failed set");

         writeMarker(Path.of(completionMarkerPath), "callback-ok\n");
      } finally {
         if (lostConnected) try { lost.disconnect(); } catch (Exception ignored) { }
         if (observerJoined) try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { observer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         String interactionClassName, Check condition, String description, long timeoutMillis)
         throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (!condition.isTrue() && System.nanoTime() < deadline) {
         service(ambassador, callbackModel, interactionClassName, true);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel callbackModel,
         String interactionClassName, boolean tolerateFailure) throws Exception {
      try {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.05);
         } else {
            Thread.sleep(1L);
         }
      } catch (Exception error) {
         if (!tolerateFailure) throw error;
         // The faulted member may surface transport EOF before queueing connectionLost.
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval<?> interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static byte[] encode(LogicalTime<?, ?> time) throws Exception {
      byte[] encoded = new byte[time.encodedLength()];
      time.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encode(MessageRetractionHandle handle) {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK federate-lost callback";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static void writeMarker(Path marker, String value) throws Exception {
      Path parent = marker.toAbsolutePath().getParent();
      if (parent != null) Files.createDirectories(parent);
      Files.write(marker, value.getBytes(StandardCharsets.UTF_8));
   }

   @FunctionalInterface
   private interface Check {
      boolean isTrue();
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
         parameters = new LinkedHashMap<>();
         for (ParameterHandle handle : source.keySet()) {
            parameters.put(handle, source.get(handle).clone());
         }
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         RegionHandleSet regions = (RegionHandleSet) arguments[5];
         regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class TimedInteraction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private final boolean regionsEmpty;
      private final int callbackSequence;

      private TimedInteraction(Object[] arguments, int callbackSequence) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         RegionHandleSet regions = (RegionHandleSet) arguments[5];
         regionsEmpty = regions == null || regions.isEmpty();
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
         this.callbackSequence = callbackSequence;
      }
   }

   private static final class Grant {
      private final LogicalTime<?, ?> time;
      private final int callbackSequence;

      private Grant(LogicalTime<?, ?> time, int callbackSequence) {
         this.time = time;
         this.callbackSequence = callbackSequence;
      }
   }

   private static final class Recorder {
      private final CopyOnWriteArrayList<Interaction> reports = new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<TimedInteraction> timestamped =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<Grant> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private final AtomicInteger callbackSequence = new AtomicInteger();
      private final AtomicInteger rosterReflectionCount = new AtomicInteger();
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;
      private volatile ObjectClassHandle federationMomObjectClass;
      private volatile ObjectInstanceHandle federationMomObject;
      private volatile String federationMomObjectName;
      private volatile AttributeHandle rosterAttribute;
      private volatile Map<AttributeHandle, byte[]> rosterValues;
      private volatile String memberReportFederation;
      private volatile Set<FederationExecutionMemberInformation> memberReport;
      private volatile String synchronizationRegistrationLabel;
      private volatile String synchronizationRegistrationFailureLabel;
      private volatile String synchronizationAnnouncementLabel;
      private volatile byte[] synchronizationTag;
      private volatile String federationSynchronizedLabel;
      private volatile Set<FederateHandle> failedToSynchronize;

      private void watchFederationRoster(ObjectClassHandle objectClass,
            AttributeHandle attribute) {
         federationMomObjectClass = objectClass;
         rosterAttribute = attribute;
      }

      private Set<FederateHandle> currentRoster(EncoderFactory encoder,
            FederateHandleFactory handleFactory, AttributeHandle attribute) {
         Map<AttributeHandle, byte[]> values = rosterValues;
         if (values == null || !values.containsKey(attribute)) return null;
         try {
            return decodeRoster(encoder, handleFactory, values.get(attribute));
         } catch (Exception error) {
            throw new AssertionError("could not decode HLAfederatesInFederation", error);
         }
      }

      private FederateAmbassador proxy(AtomicReference<String> connectionLostDescription) {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("connectionLost".equals(name) && connectionLostDescription != null
                     && arguments != null && arguments.length == 1) {
                  connectionLostDescription.set((String) arguments[0]);
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3
                     && federationMomObjectClass != null
                     && federationMomObjectClass.equals(arguments[1])
                     && "HLAfederation".equals(arguments[2])) {
                  federationMomObject = (ObjectInstanceHandle) arguments[0];
                  federationMomObjectName = (String) arguments[2];
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6
                     && arguments[1] instanceof AttributeHandleValueMap
                     && federationMomObject != null
                     && federationMomObject.equals(arguments[0])) {
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  AttributeHandle attribute = rosterAttribute;
                  if (attribute != null && source.containsKey(attribute)) {
                     Map<AttributeHandle, byte[]> copy = new LinkedHashMap<>();
                     for (AttributeHandle handle : source.keySet()) {
                        copy.put(handle, source.get(handle).clone());
                     }
                     rosterValues = copy;
                     rosterReflectionCount.incrementAndGet();
                  }
               } else if ("receiveInteraction".equals(name) && arguments != null) {
                  if (arguments.length == 10) {
                     timestamped.add(new TimedInteraction(arguments,
                        callbackSequence.incrementAndGet()));
                  } else if (arguments.length == 6) {
                     reports.add(new Interaction(arguments));
                  }
               } else if ("reportFederationExecutionMembers".equals(name)
                     && arguments != null && arguments.length == 2
                     && arguments[1] instanceof FederationExecutionMemberInformationSet) {
                  memberReportFederation = (String) arguments[0];
                  memberReport = new LinkedHashSet<>(
                     (FederationExecutionMemberInformationSet) arguments[1]);
               } else if ("synchronizationPointRegistrationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1) {
                  synchronizationRegistrationLabel = (String) arguments[0];
               } else if ("synchronizationPointRegistrationFailed".equals(name)
                     && arguments != null && arguments.length == 2) {
                  synchronizationRegistrationFailureLabel = (String) arguments[0];
               } else if ("announceSynchronizationPoint".equals(name)
                     && arguments != null && arguments.length == 2) {
                  synchronizationAnnouncementLabel = (String) arguments[0];
                  synchronizationTag = ((byte[]) arguments[1]).clone();
               } else if ("federationSynchronized".equals(name)
                     && arguments != null && arguments.length == 2
                     && arguments[1] instanceof FederateHandleSet) {
                  federationSynchronizedLabel = (String) arguments[0];
                  failedToSynchronize = new LinkedHashSet<>(
                     (FederateHandleSet) arguments[1]);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrants.add(new Grant((LogicalTime<?, ?>) arguments[0],
                     callbackSequence.incrementAndGet()));
               }
               return objectMethod(proxy, name, arguments);
            });
      }

      private static Set<FederateHandle> decodeRoster(EncoderFactory encoder,
            FederateHandleFactory handleFactory, byte[] encoded) throws Exception {
         HLAvariableArray<HLAvariableArray<HLAbyte>> references =
            encoder.createHLAvariableArray(
               outer -> encoder.createHLAvariableArray(inner -> encoder.createHLAbyte()));
         references.decode(encoded);
         Set<FederateHandle> result = new LinkedHashSet<>();
         for (HLAvariableArray<HLAbyte> reference : references) {
            FederateHandle decoded = handleFactory.decode(reference.toByteArray(), 0);
            JavaTckSupport.check(decoded != null,
               "HLAfederatesInFederation contained a null handle");
            JavaTckSupport.check(result.add(decoded),
               "HLAfederatesInFederation contained a duplicate handle");
         }
         return result;
      }
   }
}
