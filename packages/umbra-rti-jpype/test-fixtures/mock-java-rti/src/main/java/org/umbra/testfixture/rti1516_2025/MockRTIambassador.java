package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleFactory;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleSetFactory;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeHandleValueMapFactory;
import hla.rti1516_2025.AttributeSetRegionSetPair;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleFactory;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.DimensionHandleSetFactory;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleFactory;
import hla.rti1516_2025.FederateHandleSetFactory;
import hla.rti1516_2025.FederateHandleSaveStatusPair;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import hla.rti1516_2025.FederateRestoreStatus;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleFactory;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.InteractionClassHandleSetFactory;
import hla.rti1516_2025.LogicalTime;
import hla.rti1516_2025.LogicalTimeFactory;
import hla.rti1516_2025.LogicalTimeInterval;
import hla.rti1516_2025.HLAfloat64TimeFactory;
import hla.rti1516_2025.HLAfloat64Time;
import hla.rti1516_2025.exceptions.RTIexception;
import hla.rti1516_2025.HLAfloat64Interval;
import hla.rti1516_2025.HLAinteger64Interval;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionHandleFactory;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.HLAinteger64Time;
import hla.rti1516_2025.HLAinteger64TimeFactory;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectClassHandleFactory;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ObjectInstanceHandleFactory;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleFactory;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ParameterHandleValueMapFactory;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleFactory;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RegionHandleSetFactory;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RestoreStatus;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.ServiceGroup;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.SaveStatus;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.TransportationTypeHandleFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.auth.Credentials;
import hla.rti1516_2025.exceptions.AlreadyConnected;
import hla.rti1516_2025.exceptions.NotConnected;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.HashSet;
import java.util.Set;

/**
 * Stateful fixture ambassador. It exposes ``queueConnectionLost`` solely so a
 * test can cause a callback through the normal public evoke operation.
 */
/**
 * Shared Java-shaped support base for test RTI facades.
 *
 * <p>The mock factory returns this class directly.  The experimental JNI
 * integration fixture subclasses it only to reuse the deliberately small
 * standard-shaped support-value and encoder surface; its native-backed
 * lifecycle services are overridden rather than routed through this mock
 * state machine.</p>
 */
public class MockRTIambassador implements RTIambassador {
   private static final Map<String, MockFederationState> SHARED_FEDERATIONS =
      new LinkedHashMap<>();

   private static final class MockFederationState {
      private final String federationName;
      private final String logicalTimeImplementationName;
      private final Set<MockRTIambassador> members = new HashSet<>();
      private final Map<String, MockObjectRecord> objectInstances = new LinkedHashMap<>();
      private final Map<MockSupportHandle, MockRetractionRecord> timedRetractions =
         new LinkedHashMap<>();
      private final Map<String, MockOwnershipRecord> attributeOwnership =
         new LinkedHashMap<>();
      private String saveLabel;
      private Double saveTimeValue;
      private boolean savePending;
      private boolean saveInProgress;
      private boolean hasSaveSnapshot;
      private final Set<MockRTIambassador> saveBegunMembers = new HashSet<>();
      private final Set<MockRTIambassador> saveCompletedMembers = new HashSet<>();
      private String restoreLabel;
      private boolean restoreInProgress;
      private final Set<MockRTIambassador> restoreCompletedMembers = new HashSet<>();
      private boolean delaySubscriptionEvaluation;
      private boolean allowRelaxedDDM;

      private MockFederationState(String federationName, String logicalTimeImplementationName) {
         this.federationName = federationName;
         this.logicalTimeImplementationName = logicalTimeImplementationName;
      }
   }

   private static final class MockOwnershipRecord {
      private MockRTIambassador owner;
      private MockRTIambassador pendingAcquirer;
      private byte[] pendingAcquisitionTag;
      private boolean divestitureOffered;
   }

   private static final class MockObjectRecord {
      private final MockRTIambassador owner;
      private final MockSupportHandle handle;
      private final String objectClassName;
      private final String objectInstanceName;
      private final Map<String, Set<MockSupportHandle>> attributeRegionsByName;

      private MockObjectRecord(
         MockRTIambassador owner,
         MockSupportHandle handle,
         String objectClassName,
         String objectInstanceName,
         Map<String, Set<MockSupportHandle>> attributeRegionsByName)
      {
         this.owner = owner;
         this.handle = handle;
         this.objectClassName = objectClassName;
         this.objectInstanceName = objectInstanceName;
         this.attributeRegionsByName = attributeRegionsByName;
      }

      private Set<MockSupportHandle> regionsForValues(AttributeHandleValueMap values) {
         Set<MockSupportHandle> regions = new HashSet<>();
         for (AttributeHandle attribute : values.keySet()) {
            regions.addAll(regionsForAttribute(attribute));
         }
         return regions;
      }

      private Set<MockSupportHandle> regionsForAttribute(AttributeHandle attribute) {
         String attributeName = handlePayload(attribute, "attribute");
         Set<MockSupportHandle> associated = attributeRegionsByName.get(attributeName);
         return associated == null ? new HashSet<>() : new HashSet<>(associated);
      }
   }

   private static final class MockRetractionRecord {
      private final MockSupportHandle handle;
      private final Set<MockRTIambassador> deliveredRecipients = new HashSet<>();
      private final Set<MockRTIambassador> pendingRecipients = new HashSet<>();
      private boolean retracted;

      private MockRetractionRecord(MockSupportHandle handle) {
         this.handle = handle;
      }
   }

   private static final class MockTimedDelivery {
      private final MockSupportHandle retraction;
      private final LogicalTime time;
      private final Runnable callback;

      private MockTimedDelivery(
         MockSupportHandle retraction, LogicalTime time, Runnable callback) {
         this.retraction = retraction;
         this.time = time;
         this.callback = callback;
      }
   }

   static {
      SHARED_FEDERATIONS.put(
         "Umbra Mock Federation",
         new MockFederationState("Umbra Mock Federation", "HLAinteger64Time"));
   }

   private FederateAmbassador federateAmbassador;
   private boolean connected;
   private boolean callbacksEnabled = true;
   private String queuedConnectionLoss;
   private FederationExecutionInformationSet queuedFederationExecutionReport;
   private String queuedFederationExecutionMembersName;
   private FederationExecutionMemberInformationSet queuedFederationExecutionMemberReport;
   private String queuedMissingFederationExecutionName;
   private CallbackModel callbackModel;
   private final Map<String, String> federationExecutions = new LinkedHashMap<>();
   private final Queue<Runnable> queuedCallbacks = new ArrayDeque<>();
   private final List<MockTimedDelivery> pendingTimedDeliveries = new ArrayList<>();
   private boolean joined;
   private MockFederationState federationState;
   private String federateName;
   private final Set<String> publishedObjectAttributes = new HashSet<>();
   private final Set<String> subscribedObjectAttributes = new HashSet<>();
   private final Set<String> passiveObjectSubscriptions = new HashSet<>();
   private final Map<String, Set<MockSupportHandle>> subscribedObjectRegions =
      new LinkedHashMap<>();
   private final Set<String> publishedInteractions = new HashSet<>();
   private final Set<String> subscribedInteractions = new HashSet<>();
   private final Set<String> passiveInteractionSubscriptions = new HashSet<>();
   private final Set<String> publishedDirectedInteractions = new HashSet<>();
   private final Set<String> subscribedDirectedInteractions = new HashSet<>();
   private final Map<String, Set<MockSupportHandle>> subscribedInteractionRegions =
      new LinkedHashMap<>();
   private boolean saveInProgress;
   private LogicalTime lastRequestedSaveTime;
   private String pendingSaveLabel;
   private LogicalTime pendingSaveTime;
   private boolean tsoSaveBoundaryProbe;
   private InteractionClassHandle pendingTimestampedInteractionClass;
   private ParameterHandleValueMap pendingTimestampedInteractionParameters;
   private byte[] pendingTimestampedInteractionTag;
   private LogicalTime pendingTimestampedInteractionTime;
   private boolean timestampedRegionalAttributeProbe;
   private ObjectInstanceHandle pendingTimestampedAttributeObject;
   private AttributeHandleValueMap pendingTimestampedAttributeValues;
   private byte[] pendingTimestampedAttributeTag;
   private LogicalTime pendingTimestampedAttributeTime;
   private boolean restoreInProgress;
   private final MockSupportHandleFactory supportHandleFactory = new MockSupportHandleFactory();
   private final MockFederateHandleFactory federateHandleFactory = new MockFederateHandleFactory();
   private final MockFederateHandleSetFactory federateHandleSetFactory =
      new MockFederateHandleSetFactory();
   private final MockAttributeHandleSetFactory attributeHandleSetFactory =
      new MockAttributeHandleSetFactory();
   private final MockAttributeHandleValueMapFactory attributeHandleValueMapFactory =
      new MockAttributeHandleValueMapFactory();
   private final MockParameterHandleValueMapFactory parameterHandleValueMapFactory =
      new MockParameterHandleValueMapFactory();
   private final MockDimensionHandleSetFactory dimensionHandleSetFactory =
      new MockDimensionHandleSetFactory();
   private final MockRegionHandleSetFactory regionHandleSetFactory =
      new MockRegionHandleSetFactory();
   private LogicalTimeFactory timeFactory = new HLAinteger64TimeFactory();
   private LogicalTime currentLogicalTime = timeFactory.makeInitial();
   private LogicalTimeInterval currentLookahead = timeFactory.makeZero();
   private LogicalTimeInterval pendingLookahead;
   private LogicalTime savedLogicalTime;
   private LogicalTimeInterval savedLookahead;
   private LogicalTimeInterval savedPendingLookahead;
   private boolean timeRegulating;
   private boolean timeConstrained;
   private boolean asynchronousDelivery;
   private final Map<String, MockSupportHandle> objectInstances = new LinkedHashMap<>();
   private final Set<String> knownObjectInstances = new HashSet<>();
   private int nextObjectInstance = 1;
   private final Set<String> reservedObjectInstanceNames = new HashSet<>();
   private int nextRegion = 1;
   private int nextRetraction = 1;
   private MessageRetractionHandle lastRetracted;
   private final Map<MockSupportHandle, Set<MockSupportHandle>> regionDimensions = new LinkedHashMap<>();
   private final Map<String, RangeBounds> regionBounds = new LinkedHashMap<>();
   private boolean conveyRegionDesignatorSets;
   private boolean objectClassRelevanceAdvisory;
   private boolean attributeRelevanceAdvisory;
   private boolean attributeScopeAdvisory;
   private boolean interactionRelevanceAdvisory;
   private ResignAction automaticResignDirective = ResignAction.NO_ACTION;
   private boolean serviceReporting;
   private boolean exceptionReporting;
   private boolean sendServiceReportsToFile;

   public MockRTIambassador() {
      federationExecutions.put("Umbra Mock Federation", "HLAinteger64Time");
   }

   private void selectTimeFactory(String implementationName) {
      if ("HLAinteger64Time".equals(implementationName)) {
         timeFactory = new HLAinteger64TimeFactory();
      } else if ("HLAfloat64Time".equals(implementationName)) {
         timeFactory = new HLAfloat64TimeFactory();
      } else {
         throw new IllegalArgumentException(
            "unsupported logical-time implementation: " + implementationName);
      }
      currentLogicalTime = timeFactory.makeInitial();
      currentLookahead = timeFactory.makeZero();
      pendingLookahead = null;
      savedLogicalTime = null;
      savedLookahead = null;
      savedPendingLookahead = null;
      pendingSaveLabel = null;
      pendingSaveTime = null;
      lastRequestedSaveTime = null;
      pendingTimestampedInteractionClass = null;
      pendingTimestampedInteractionParameters = null;
      pendingTimestampedInteractionTag = null;
      pendingTimestampedInteractionTime = null;
      timestampedRegionalAttributeProbe = false;
      pendingTimestampedAttributeObject = null;
      pendingTimestampedAttributeValues = null;
      pendingTimestampedAttributeTag = null;
      pendingTimestampedAttributeTime = null;
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel) throws AlreadyConnected
   {
      return connect(federateAmbassador, callbackModel, null, null);
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration) throws AlreadyConnected
   {
      return connect(federateAmbassador, callbackModel, configuration, null);
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      Credentials credentials) throws AlreadyConnected
   {
      return connect(federateAmbassador, callbackModel, null, credentials);
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration,
      Credentials credentials) throws AlreadyConnected
   {
      if (connected) {
         throw new AlreadyConnected("already connected");
      }
      this.federateAmbassador = federateAmbassador;
      this.callbackModel = callbackModel;
      connected = true;
      callbacksEnabled = true;
      return new ConfigurationResult(
         false,
         false,
         AdditionalSettingsResultCode.SETTINGS_IGNORED,
         "connected through the mock Java RTI using " + callbackModel.name());
   }

   @Override
   public void disconnect() throws NotConnected {
      requireConnected();
      leaveSharedFederation();
      connected = false;
      federateAmbassador = null;
      queuedConnectionLoss = null;
      queuedFederationExecutionReport = null;
      queuedFederationExecutionMembersName = null;
      queuedFederationExecutionMemberReport = null;
      queuedMissingFederationExecutionName = null;
      queuedCallbacks.clear();
      pendingTimedDeliveries.clear();
      saveInProgress = false;
      pendingSaveLabel = null;
      pendingSaveTime = null;
      lastRequestedSaveTime = null;
      pendingTimestampedInteractionClass = null;
      pendingTimestampedInteractionParameters = null;
      pendingTimestampedInteractionTag = null;
      pendingTimestampedInteractionTime = null;
      timestampedRegionalAttributeProbe = false;
      pendingTimestampedAttributeObject = null;
      pendingTimestampedAttributeValues = null;
      pendingTimestampedAttributeTag = null;
      pendingTimestampedAttributeTime = null;
      restoreInProgress = false;
      pendingLookahead = null;
      savedLogicalTime = null;
      savedLookahead = null;
      savedPendingLookahead = null;
   }

   @Override
   public boolean evokeCallback(double approximateMinimumTimeInSeconds) throws NotConnected {
      requireConnected();
      if (callbacksEnabled && queuedConnectionLoss != null) {
         String description = queuedConnectionLoss;
         queuedConnectionLoss = null;
         federateAmbassador.connectionLost(description);
         return true;
      }
      if (callbacksEnabled && queuedFederationExecutionReport != null) {
         FederationExecutionInformationSet report = queuedFederationExecutionReport;
         queuedFederationExecutionReport = null;
         federateAmbassador.reportFederationExecutions(report);
         return true;
      }
      if (callbacksEnabled && queuedFederationExecutionMemberReport != null) {
         String federationName = queuedFederationExecutionMembersName;
         FederationExecutionMemberInformationSet report = queuedFederationExecutionMemberReport;
         queuedFederationExecutionMembersName = null;
         queuedFederationExecutionMemberReport = null;
         federateAmbassador.reportFederationExecutionMembers(federationName, report);
         return true;
      }
      if (callbacksEnabled && queuedMissingFederationExecutionName != null) {
         String federationName = queuedMissingFederationExecutionName;
         queuedMissingFederationExecutionName = null;
         federateAmbassador.reportFederationExecutionDoesNotExist(federationName);
         return true;
      }
      if (callbacksEnabled && !queuedCallbacks.isEmpty()) {
         queuedCallbacks.remove().run();
         return true;
      }
      return false;
   }

   @Override
   public boolean evokeMultipleCallbacks(
      double approximateMinimumTimeInSeconds,
      double approximateMaximumTimeInSeconds) throws NotConnected
   {
      return evokeCallback(approximateMinimumTimeInSeconds);
   }

   @Override
   public void enableCallbacks() throws NotConnected {
      requireConnected();
      callbacksEnabled = true;
   }

   @Override
   public void disableCallbacks() throws NotConnected {
      requireConnected();
      callbacksEnabled = false;
   }

   public void queueConnectionLost(String description) throws NotConnected {
      requireConnected();
      queuedConnectionLoss = description;
   }

   @Override
   public void listFederationExecutions() throws NotConnected {
      requireConnected();
      MockFederationExecutionInformationSet report = new MockFederationExecutionInformationSet();
      for (Map.Entry<String, String> entry : federationExecutions.entrySet()) {
         report.add(new FederationExecutionInformation(entry.getKey(), entry.getValue()));
      }
      if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
         federateAmbassador.reportFederationExecutions(report);
      } else {
         queuedFederationExecutionReport = report;
      }
   }

   @Override
   public void listFederationExecutionMembers(String federationExecutionName) throws NotConnected {
      requireConnected();
      if (!federationExecutions.containsKey(federationExecutionName)) {
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            federateAmbassador.reportFederationExecutionDoesNotExist(federationExecutionName);
         } else {
            queuedMissingFederationExecutionName = federationExecutionName;
         }
         return;
      }
      MockFederationExecutionMemberInformationSet report =
         new MockFederationExecutionMemberInformationSet();
      if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
         federateAmbassador.reportFederationExecutionMembers(federationExecutionName, report);
      } else {
         queuedFederationExecutionMembersName = federationExecutionName;
         queuedFederationExecutionMemberReport = report;
      }
   }

   @Override
   public void createFederationExecution(
      String federationName,
      String fomModule,
      String logicalTimeImplementationName) throws NotConnected
   {
      requireConnected();
      federationExecutions.put(federationName, logicalTimeImplementationName);
      SHARED_FEDERATIONS.put(
         federationName,
         new MockFederationState(federationName, logicalTimeImplementationName));
   }

   @Override
   public void createFederationExecution(
      String federationName, String[] fomModules, String logicalTimeImplementationName)
      throws NotConnected
   {
      requireConnected();
      federationExecutions.put(federationName, logicalTimeImplementationName);
      SHARED_FEDERATIONS.put(
         federationName,
         new MockFederationState(federationName, logicalTimeImplementationName));
   }

   @Override
   public void createFederationExecutionWithMIM(
      String federationName,
      String[] fomModules,
      String mimModule,
      String logicalTimeImplementationName) throws NotConnected
   {
      requireConnected();
      federationExecutions.put(federationName, logicalTimeImplementationName);
      SHARED_FEDERATIONS.put(
         federationName,
         new MockFederationState(federationName, logicalTimeImplementationName));
   }

   @Override
   public void destroyFederationExecution(String federationName) throws NotConnected {
      requireConnected();
      federationExecutions.remove(federationName);
      MockFederationState state = SHARED_FEDERATIONS.get(federationName);
      if (state != null && state.members.isEmpty()) {
         SHARED_FEDERATIONS.remove(federationName);
      }
   }

   @Override
   public MockFederateHandle joinFederationExecution(
      String federateType, String federationExecutionName) throws NotConnected {
      return joinFederationExecution(federateType, federateType, federationExecutionName);
   }

   @Override
   public MockFederateHandle joinFederationExecution(
      String federateName, String federateType, String federationExecutionName) throws NotConnected {
      requireConnected();
      String selectedImplementationName = federationExecutions.get(federationExecutionName);
      if (selectedImplementationName == null) {
         MockFederationState shared = SHARED_FEDERATIONS.get(federationExecutionName);
         if (shared == null) {
            throw new IllegalArgumentException(
               "unknown federation execution: " + federationExecutionName);
         }
         selectedImplementationName = shared.logicalTimeImplementationName;
      }
      selectTimeFactory(selectedImplementationName);
      joined = true;
      this.federateName = federateName;
      federationState = SHARED_FEDERATIONS.get(federationExecutionName);
      if (federationState == null) {
         federationState = new MockFederationState(
            federationExecutionName, selectedImplementationName);
         SHARED_FEDERATIONS.put(federationExecutionName, federationState);
      }
      federationState.members.add(this);
      return new MockFederateHandle(federationExecutionName + ":" + federateName + ":" + federateType);
   }

   @Override
   public MockFederateHandle joinFederationExecution(
      String federateType, String federationExecutionName, String[] additionalFomModules)
      throws NotConnected {
      return joinFederationExecution(federateType, federationExecutionName);
   }

   @Override
   public MockFederateHandle joinFederationExecution(
      String federateName,
      String federateType,
      String federationExecutionName,
      String[] additionalFomModules) throws NotConnected {
      return joinFederationExecution(federateName, federateType, federationExecutionName);
   }

   @Override
   public void resignFederationExecution(hla.rti1516_2025.ResignAction resignAction)
      throws NotConnected {
      requireConnected();
      leaveSharedFederation();
   }

   @Override
   public void registerFederationSynchronizationPoint(String label, byte[] userSuppliedTag)
      throws NotConnected {
      requireJoined();
      byte[] copiedTag = userSuppliedTag.clone();
      deliverCallback(() -> federateAmbassador.synchronizationPointRegistrationSucceeded(label));
      deliverCallback(() -> federateAmbassador.announceSynchronizationPoint(label, copiedTag));
   }

   @Override
   public void registerFederationSynchronizationPoint(
      String label, byte[] userSuppliedTag, hla.rti1516_2025.FederateHandleSet synchronizationSet)
      throws NotConnected
   {
      registerFederationSynchronizationPoint(label, userSuppliedTag);
   }

   @Override
   public void synchronizationPointAchieved(String label, boolean successfully) throws NotConnected {
      requireJoined();
      MockFederateHandleSet failedToSync = new MockFederateHandleSet();
      if (!successfully) {
         failedToSync.add(new MockFederateHandle(
            federationState.federationName + ":" + federateName));
      }
      deliverCallback(() -> federateAmbassador.federationSynchronized(
         label, failedToSync));
   }

   @Override
   public void queryFederationSaveStatus() throws NotConnected {
      requireJoined();
      if (federationState != null && federationState.members.size() > 1) {
         FederateHandleSaveStatusPair[] response = new FederateHandleSaveStatusPair[
            federationState.members.size()];
         int index = 0;
         for (MockRTIambassador member : federationState.members) {
            SaveStatus status = federationState.savePending
               ? SaveStatus.FEDERATE_INSTRUCTED_TO_SAVE
               : federationState.saveInProgress
                  ? federationState.saveBegunMembers.contains(member)
                     ? SaveStatus.FEDERATE_SAVING
                     : SaveStatus.FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE
                  : SaveStatus.NO_SAVE_IN_PROGRESS;
            response[index++] = new FederateHandleSaveStatusPair(
               new MockFederateHandle(
                  federationState.federationName + ":" + member.federateName),
               status);
         }
         deliverCallback(() -> federateAmbassador.federationSaveStatusResponse(response));
         return;
      }
      SaveStatus status = saveInProgress
         ? SaveStatus.FEDERATE_SAVING
         : pendingSaveTime != null
            ? SaveStatus.FEDERATE_INSTRUCTED_TO_SAVE
            : SaveStatus.NO_SAVE_IN_PROGRESS;
      FederateHandleSaveStatusPair[] response = {
         new FederateHandleSaveStatusPair(
            new MockFederateHandle("save:mock-federate"), status)
      };
      deliverCallback(() -> federateAmbassador.federationSaveStatusResponse(response));
   }

   @Override
   public void requestFederationSave(String label) throws NotConnected {
      requireJoined();
      requestFederationSave(label, null);
   }

   @Override
   public void requestFederationSave(String label, LogicalTime time) throws NotConnected {
      requireJoined();
      if (time != null && !timeFactory.getName().equals(time.implementationName())) {
         throw new IllegalArgumentException("save time uses the wrong logical-time implementation");
      }
      if (federationState != null && federationState.members.size() > 1) {
         if (label.startsWith("missing-")) {
            deliverCallback(() -> federateAmbassador.federationNotSaved(
               SaveFailureReason.RTI_UNABLE_TO_SAVE));
            return;
         }
         if (federationState.savePending || federationState.saveInProgress) {
            throw new IllegalArgumentException("a federation save is already in progress");
         }
         if (time != null && (time.isInitial() || time.isFinal()
            || logicalTimeValue(time) <= logicalTimeValue(currentLogicalTime))) {
            throw new IllegalArgumentException(
               "timestamped save time must be later than current logical time");
         }
         federationState.saveLabel = label;
         federationState.saveTimeValue = time == null ? null : logicalTimeValue(time);
         federationState.savePending = time != null;
         federationState.saveInProgress = false;
         federationState.saveBegunMembers.clear();
         federationState.saveCompletedMembers.clear();
         if (time == null) {
            beginSharedFederationSave();
         }
         return;
      }
      lastRequestedSaveTime = time;
      if (label.startsWith("missing-")) {
         pendingSaveLabel = null;
         pendingSaveTime = null;
         deliverCallback(() -> federateAmbassador.federationNotSaved(
            SaveFailureReason.RTI_UNABLE_TO_SAVE));
         return;
      }
      if (time == null) {
         pendingSaveLabel = null;
         pendingSaveTime = null;
         beginFederationSave(label);
         return;
      }
      if (time.isInitial() || time.isFinal()
         || logicalTimeValue(time) <= logicalTimeValue(currentLogicalTime)) {
         throw new IllegalArgumentException(
            "timestamped save time must be later than current logical time");
      }
      pendingSaveLabel = label;
      pendingSaveTime = time;
   }

   private void beginFederationSave(String label) {
      beginFederationSave(label, null);
   }

   private void beginFederationSave(String label, LogicalTime time) {
      saveInProgress = true;
      if (time == null) {
         deliverCallback(() -> federateAmbassador.initiateFederateSave(label));
      } else {
         deliverCallback(() -> federateAmbassador.initiateFederateSave(label, time));
      }
   }

   private void beginSharedFederationSave() {
      if (federationState == null || federationState.saveLabel == null
         || federationState.saveInProgress) {
         return;
      }
      String label = federationState.saveLabel;
      federationState.savePending = false;
      federationState.saveInProgress = true;
      federationState.saveBegunMembers.clear();
      federationState.saveCompletedMembers.clear();
      for (MockRTIambassador member : federationState.members) {
         member.deliverCallback(() -> member.federateAmbassador.initiateFederateSave(label));
      }
   }

   /** Test-only inspection hook for verifying the Java overload received a typed time. */
   public LogicalTime getLastRequestedSaveTime() {
      return lastRequestedSaveTime;
   }

   @Override
   public void federateSaveBegun() throws NotConnected {
      requireJoined();
      if (federationState != null && federationState.members.size() > 1) {
         if (!federationState.saveInProgress) {
            throw new IllegalArgumentException("no federation save is in progress");
         }
         federationState.saveBegunMembers.add(this);
         saveInProgress = true;
         return;
      }
      saveInProgress = true;
   }

   @Override
   public void federateSaveComplete() throws NotConnected {
      requireJoined();
      if (federationState != null && federationState.members.size() > 1) {
         if (!federationState.saveInProgress) {
            throw new IllegalArgumentException("no federation save is in progress");
         }
         savedLogicalTime = copyLogicalTime(currentLogicalTime);
         savedLookahead = copyLogicalTimeInterval(currentLookahead);
         savedPendingLookahead = copyLogicalTimeInterval(pendingLookahead);
         federationState.saveCompletedMembers.add(this);
         federationState.saveBegunMembers.add(this);
         if (federationState.saveCompletedMembers.containsAll(federationState.members)) {
            federationState.saveInProgress = false;
            federationState.hasSaveSnapshot = true;
            federationState.saveLabel = null;
            federationState.saveTimeValue = null;
            for (MockRTIambassador member : federationState.members) {
               member.saveInProgress = false;
               member.deliverCallback(() -> member.federateAmbassador.federationSaved());
            }
         }
         return;
      }
      savedLogicalTime = copyLogicalTime(currentLogicalTime);
      savedLookahead = copyLogicalTimeInterval(currentLookahead);
      savedPendingLookahead = copyLogicalTimeInterval(pendingLookahead);
      saveInProgress = false;
      pendingSaveLabel = null;
      pendingSaveTime = null;
      deliverCallback(() -> federateAmbassador.federationSaved());
   }

   @Override
   public void federateSaveNotComplete() throws NotConnected {
      requireJoined();
      if (federationState != null && federationState.members.size() > 1) {
         finishSharedFederationSave(SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE);
         return;
      }
      saveInProgress = false;
      pendingSaveLabel = null;
      pendingSaveTime = null;
      deliverCallback(() -> federateAmbassador.federationNotSaved(
         SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE));
   }

   @Override
   public void abortFederationSave() throws NotConnected {
      requireJoined();
      if (federationState != null && federationState.members.size() > 1) {
         finishSharedFederationSave(SaveFailureReason.SAVE_ABORTED);
         return;
      }
      saveInProgress = false;
      pendingSaveLabel = null;
      pendingSaveTime = null;
      deliverCallback(() -> federateAmbassador.federationNotSaved(SaveFailureReason.SAVE_ABORTED));
   }

   private void finishSharedFederationSave(SaveFailureReason reason) {
      if (federationState == null) {
         return;
      }
      federationState.savePending = false;
      federationState.saveInProgress = false;
      federationState.saveLabel = null;
      federationState.saveTimeValue = null;
      federationState.saveBegunMembers.clear();
      federationState.saveCompletedMembers.clear();
      for (MockRTIambassador member : federationState.members) {
         member.saveInProgress = false;
         member.deliverCallback(() -> member.federateAmbassador.federationNotSaved(reason));
      }
   }

   @Override
   public void queryFederationRestoreStatus() throws NotConnected {
      requireJoined();
      if (federationState != null && federationState.members.size() > 1) {
         FederateRestoreStatus[] response = new FederateRestoreStatus[
            federationState.members.size()];
         int index = 0;
         for (MockRTIambassador member : federationState.members) {
            RestoreStatus status = federationState.restoreInProgress
               ? RestoreStatus.FEDERATE_RESTORING
               : RestoreStatus.NO_RESTORE_IN_PROGRESS;
            response[index++] = new FederateRestoreStatus(
               new MockFederateHandle(
                  federationState.federationName + ":" + member.federateName),
               new MockFederateHandle(
                  federationState.federationName + ":post-" + member.federateName),
               status);
         }
         deliverCallback(() -> federateAmbassador.federationRestoreStatusResponse(response));
         return;
      }
      RestoreStatus status = restoreInProgress
         ? RestoreStatus.FEDERATE_RESTORING
         : RestoreStatus.NO_RESTORE_IN_PROGRESS;
      FederateRestoreStatus[] response = {
         new FederateRestoreStatus(
            new MockFederateHandle("restore:pre"),
            new MockFederateHandle("restore:post"),
            status)
      };
      deliverCallback(() -> federateAmbassador.federationRestoreStatusResponse(response));
   }

   @Override
   public void requestFederationRestore(String label) throws NotConnected {
      requireConnected();
      if (label.startsWith("missing-")) {
         deliverCallback(() -> federateAmbassador.requestFederationRestoreFailed(label));
         return;
      }
      if (federationState != null && federationState.members.size() > 1) {
         if (!federationState.hasSaveSnapshot || federationState.restoreInProgress) {
            deliverCallback(() -> federateAmbassador.requestFederationRestoreFailed(label));
            return;
         }
         federationState.restoreLabel = label;
         federationState.restoreInProgress = true;
         federationState.restoreCompletedMembers.clear();
         deliverCallback(() -> federateAmbassador.requestFederationRestoreSucceeded(label));
         for (MockRTIambassador member : federationState.members) {
            String memberName = member.federateName;
            member.deliverCallback(() -> member.federateAmbassador.federationRestoreBegun());
            member.deliverCallback(() -> member.federateAmbassador.initiateFederateRestore(
               label,
               memberName,
               new MockFederateHandle(
                  federationState.federationName + ":post-" + memberName)));
         }
         return;
      }
      restoreInProgress = true;
      deliverCallback(() -> federateAmbassador.requestFederationRestoreSucceeded(label));
      deliverCallback(() -> federateAmbassador.federationRestoreBegun());
      deliverCallback(() -> federateAmbassador.initiateFederateRestore(
         label,
         "mock-federate",
         new MockFederateHandle("restore:" + label)));
   }

   @Override
   public void federateRestoreComplete() throws NotConnected {
      requireConnected();
      if (federationState != null && federationState.members.size() > 1) {
         if (!federationState.restoreInProgress) {
            throw new IllegalArgumentException("no federation restore is in progress");
         }
         federationState.restoreCompletedMembers.add(this);
         if (federationState.restoreCompletedMembers.containsAll(federationState.members)) {
            federationState.restoreInProgress = false;
            federationState.restoreLabel = null;
            for (MockRTIambassador member : federationState.members) {
               if (member.savedLogicalTime != null && member.savedLookahead != null) {
                  member.currentLogicalTime = member.copyLogicalTime(member.savedLogicalTime);
                  member.currentLookahead = member.copyLogicalTimeInterval(member.savedLookahead);
                  member.pendingLookahead = member.copyLogicalTimeInterval(
                     member.savedPendingLookahead);
               }
               member.deliverCallback(() -> member.federateAmbassador.federationRestored());
            }
            federationState.restoreCompletedMembers.clear();
         }
         return;
      }
      restoreInProgress = false;
      if (savedLogicalTime != null && savedLookahead != null) {
         currentLogicalTime = copyLogicalTime(savedLogicalTime);
         currentLookahead = copyLogicalTimeInterval(savedLookahead);
         pendingLookahead = copyLogicalTimeInterval(savedPendingLookahead);
      }
      deliverCallback(() -> federateAmbassador.federationRestored());
   }

   @Override
   public void federateRestoreNotComplete() throws NotConnected {
      requireConnected();
      if (federationState != null && federationState.members.size() > 1) {
         finishSharedFederationRestore(
            RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE);
         return;
      }
      restoreInProgress = false;
      deliverCallback(() -> federateAmbassador.federationNotRestored(
         RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE));
   }

   @Override
   public void abortFederationRestore() throws NotConnected {
      requireConnected();
      if (federationState != null && federationState.members.size() > 1) {
         finishSharedFederationRestore(RestoreFailureReason.RESTORE_ABORTED);
         return;
      }
      restoreInProgress = false;
      deliverCallback(() -> federateAmbassador.federationNotRestored(
         RestoreFailureReason.RESTORE_ABORTED));
   }

   private void finishSharedFederationRestore(RestoreFailureReason reason) {
      if (federationState == null) {
         return;
      }
      federationState.restoreInProgress = false;
      federationState.restoreLabel = null;
      federationState.restoreCompletedMembers.clear();
      for (MockRTIambassador member : federationState.members) {
         member.deliverCallback(() -> member.federateAmbassador.federationNotRestored(reason));
      }
   }

   @Override
   public ObjectClassHandle getObjectClassHandle(String objectClassName) throws NotConnected {
      requireJoined();
      return supportHandle("object", objectClassName);
   }

   @Override
   public String getObjectClassName(ObjectClassHandle objectClass) throws NotConnected {
      requireJoined();
      return handlePayload(objectClass, "object");
   }

   @Override
   public AttributeHandle getAttributeHandle(ObjectClassHandle objectClass, String attributeName)
      throws NotConnected {
      requireJoined();
      return supportHandle("attribute", handlePayload(objectClass, "object") + "\u001F" + attributeName);
   }

   @Override
   public String getAttributeName(ObjectClassHandle objectClass, AttributeHandle attribute)
      throws NotConnected {
      requireJoined();
      String objectName = handlePayload(objectClass, "object");
      String[] parts = handlePayload(attribute, "attribute").split("\u001F", 2);
      if (parts.length != 2 || !parts[0].equals(objectName)) {
         throw new IllegalArgumentException("attribute handle does not belong to object class");
      }
      return parts[1];
   }

   @Override
   public InteractionClassHandle getInteractionClassHandle(String interactionClassName)
      throws NotConnected {
      requireJoined();
      return supportHandle("interaction", interactionClassName);
   }

   @Override
   public String getInteractionClassName(InteractionClassHandle interactionClass) throws NotConnected {
      requireJoined();
      return handlePayload(interactionClass, "interaction");
   }

   @Override
   public ParameterHandle getParameterHandle(InteractionClassHandle interactionClass, String parameterName)
      throws NotConnected {
      requireJoined();
      return supportHandle(
         "parameter", handlePayload(interactionClass, "interaction") + "\u001F" + parameterName);
   }

   @Override
   public String getParameterName(InteractionClassHandle interactionClass, ParameterHandle parameter)
      throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      String[] parts = handlePayload(parameter, "parameter").split("\u001F", 2);
      if (parts.length != 2 || !parts[0].equals(interactionName)) {
         throw new IllegalArgumentException("parameter handle does not belong to interaction class");
      }
      return parts[1];
   }

   @Override
   public TransportationTypeHandle getTransportationTypeHandle(String transportationTypeName)
      throws NotConnected {
      requireJoined();
      return supportHandle("transportation", transportationTypeName);
   }

   @Override
   public String getTransportationTypeName(TransportationTypeHandle transportationType)
      throws NotConnected {
      requireJoined();
      return handlePayload(transportationType, "transportation");
   }

   @Override
   public DimensionHandle getDimensionHandle(String dimensionName) throws NotConnected {
      requireJoined();
      return supportHandle("dimension", dimensionName);
   }

   @Override
   public String getDimensionName(DimensionHandle dimension) throws NotConnected {
      requireJoined();
      return handlePayload(dimension, "dimension");
   }

   @Override
   public FederateHandle getFederateHandle(String federateName) throws NotConnected {
      requireJoined();
      return new MockFederateHandle("federate:" + federateName);
   }

   @Override
   public String getFederateName(FederateHandle federate) throws NotConnected {
      requireJoined();
      if (!(federate instanceof MockFederateHandle)) {
         throw new IllegalArgumentException("fixture requires a decoded federate handle");
      }
      String value = ((MockFederateHandle) federate).value();
      if (!value.startsWith("federate:")) {
         throw new IllegalArgumentException("unknown federate handle");
      }
      return value.substring("federate:".length());
   }

   @Override
   public ObjectClassHandle getKnownObjectClassHandle(ObjectInstanceHandle objectInstance)
      throws NotConnected {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      return supportHandle("object", "HLAobjectRoot.Employee.Server");
   }

   @Override
   public double getUpdateRateValue(String updateRateDesignator) throws NotConnected {
      requireJoined();
      if (updateRateDesignator == null || updateRateDesignator.isEmpty()) {
         throw new IllegalArgumentException("update-rate designator is empty");
      }
      return 1.0;
   }

   @Override
   public double getUpdateRateValueForAttribute(
      ObjectInstanceHandle objectInstance, AttributeHandle attribute) throws NotConnected {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      handlePayload(attribute, "attribute");
      return 1.0;
   }

   @Override
   public OrderType getOrderType(String orderTypeName) throws NotConnected {
      requireJoined();
      if ("Receive".equals(orderTypeName) || "RECEIVE".equals(orderTypeName)) {
         return OrderType.RECEIVE;
      }
      if ("TimeStamp".equals(orderTypeName) || "TIMESTAMP".equals(orderTypeName)) {
         return OrderType.TIMESTAMP;
      }
      throw new IllegalArgumentException("unknown order type");
   }

   @Override
   public String getOrderName(OrderType orderType) throws NotConnected {
      requireJoined();
      if (orderType == OrderType.RECEIVE) return "Receive";
      if (orderType == OrderType.TIMESTAMP) return "TimeStamp";
      throw new IllegalArgumentException("unknown order type");
   }

   @Override
   public DimensionHandleSet getAvailableDimensionsForObjectClass(ObjectClassHandle objectClass)
      throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      MockDimensionHandleSet result = new MockDimensionHandleSet();
      result.add(getDimensionHandle("SodaFlavor"));
      return result;
   }

   @Override
   public DimensionHandleSet getAvailableDimensionsForInteractionClass(
      InteractionClassHandle interactionClass) throws NotConnected {
      requireJoined();
      handlePayload(interactionClass, "interaction");
      MockDimensionHandleSet result = new MockDimensionHandleSet();
      result.add(getDimensionHandle("SodaFlavor"));
      return result;
   }

   @Override
   public long getDimensionUpperBound(DimensionHandle dimension) throws NotConnected {
      requireJoined();
      handlePayload(dimension, "dimension");
      return 100;
   }

   @Override
   public long normalizeServiceGroup(ServiceGroup serviceGroup) throws NotConnected {
      requireJoined();
      if (serviceGroup == null) throw new IllegalArgumentException("service group is null");
      return serviceGroup.ordinal();
   }

   @Override
   public long normalizeFederateHandle(FederateHandle federate) throws NotConnected {
      requireJoined();
      if (!(federate instanceof MockFederateHandle)) {
         throw new IllegalArgumentException("fixture requires a decoded federate handle");
      }
      if (!((MockFederateHandle) federate).value().startsWith("federate:")) {
         throw new IllegalArgumentException("handle has the wrong standard domain");
      }
      return 1;
   }

   @Override
   public long normalizeObjectClassHandle(ObjectClassHandle objectClass) throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      return 2;
   }

   @Override
   public long normalizeInteractionClassHandle(InteractionClassHandle interactionClass)
      throws NotConnected {
      requireJoined();
      handlePayload(interactionClass, "interaction");
      return 3;
   }

   @Override
   public long normalizeObjectInstanceHandle(ObjectInstanceHandle objectInstance)
      throws NotConnected {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      return 4;
   }

   @Override
   public MessageRetractionHandle deleteObjectInstanceWithTime(
      ObjectInstanceHandle objectInstance, byte[] userSuppliedTag, LogicalTime time)
      throws NotConnected {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      return supportHandle("retraction", Integer.toString(nextRetraction++));
   }

   @Override
   public MessageRetractionHandle updateAttributeValuesWithTime(
      ObjectInstanceHandle objectInstance,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      MockSupportHandle known = objectInstances.get(objectInstanceName);
      if (known == null) {
         throw new IllegalArgumentException("object instance is not known");
      }
      if (timestampedRegionalAttributeProbe) {
         pendingTimestampedAttributeObject = objectInstance;
         pendingTimestampedAttributeValues = new MockAttributeHandleValueMap();
         for (Map.Entry<AttributeHandle, byte[]> entry : attributeValues.entrySet()) {
            pendingTimestampedAttributeValues.put(
               entry.getKey(), entry.getValue().clone());
         }
         pendingTimestampedAttributeTag = userSuppliedTag.clone();
         pendingTimestampedAttributeTime = time;
      }
      if (federationState == null || federationState.members.size() <= 1) {
         return supportHandle("retraction", Integer.toString(nextRetraction++));
      }
      MockObjectRecord record = federationState.objectInstances.get(objectInstanceName);
      if (record == null) {
         throw new IllegalArgumentException("object instance is not shared");
      }
      MockAttributeHandleValueMap copiedValues = new MockAttributeHandleValueMap();
      for (Map.Entry<AttributeHandle, byte[]> entry : attributeValues.entrySet()) {
         handlePayload(entry.getKey(), "attribute");
         copiedValues.put(entry.getKey(), entry.getValue().clone());
      }
      byte[] copiedTag = userSuppliedTag.clone();
      MockSupportHandle retraction = nextSharedRetraction();
      federationState.timedRetractions.put(retraction, new MockRetractionRecord(retraction));
      boolean delayed = federationState.delaySubscriptionEvaluation;
      for (MockRTIambassador member : federationState.members) {
         if (member == this) {
            continue;
         }
         MockAttributeHandleValueMap subscribedValues =
            member.filterSubscribedAttributes(record.objectClassName, copiedValues);
         MockAttributeHandleValueMap eligibleValues =
            filterRegionRelevantAttributes(this, record, member, subscribedValues);
         if (!delayed && eligibleValues.isEmpty()) {
            continue;
         }
         LogicalTime deliveryTime = copyLogicalTime(time);
         OrderType receivedOrder = member.timeConstrained
            ? OrderType.TIMESTAMP
            : OrderType.RECEIVE;
         member.routeTimedDelivery(
            retraction,
            deliveryTime,
            () -> {
               MockAttributeHandleValueMap values = eligibleValues;
               if (delayed) {
                  values = filterRegionRelevantAttributes(
                     this,
                     record,
                     member,
                     member.filterSubscribedAttributes(record.objectClassName, copiedValues));
               }
               if (values.isEmpty()) {
                  return;
               }
               RegionHandleSet currentOptionalRegions = objectUpdateRegionDesignator(
                  member, record, values);
               member.federateAmbassador.reflectAttributeValues(
                  record.handle,
                  values,
                  copiedTag,
                  supportHandle("transportation", "HLAreliable"),
                  new MockFederateHandle(
                     federationState.federationName + ":" + federateName),
                  currentOptionalRegions,
                  deliveryTime,
                  OrderType.TIMESTAMP,
                  receivedOrder,
                  retraction);
            });
      }
      return retraction;
   }

   @Override
   public MessageRetractionHandle sendInteractionWithTime(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      if (tsoSaveBoundaryProbe) {
         pendingTimestampedInteractionClass = interactionClass;
         pendingTimestampedInteractionParameters = new MockParameterHandleValueMap();
         for (Map.Entry<ParameterHandle, byte[]> entry : parameterValues.entrySet()) {
            pendingTimestampedInteractionParameters.put(
               entry.getKey(), entry.getValue().clone());
         }
         pendingTimestampedInteractionTag = userSuppliedTag.clone();
         pendingTimestampedInteractionTime = time;
      }
      if (federationState == null || federationState.members.size() <= 1) {
         return supportHandle("retraction", Integer.toString(nextRetraction++));
      }
      MockParameterHandleValueMap copiedValues = copyParameters(parameterValues);
      byte[] copiedTag = userSuppliedTag.clone();
      MockSupportHandle retraction = nextSharedRetraction();
      federationState.timedRetractions.put(retraction, new MockRetractionRecord(retraction));
      for (MockRTIambassador member : federationState.members) {
         if (member == this || !member.isActiveInteractionSubscription(interactionName)) {
            continue;
         }
         LogicalTime deliveryTime = copyLogicalTime(time);
         OrderType receivedOrder = member.timeConstrained
            ? OrderType.TIMESTAMP
            : OrderType.RECEIVE;
         member.routeTimedDelivery(
            retraction,
            deliveryTime,
            () -> member.federateAmbassador.receiveInteraction(
               supportHandle("interaction", interactionName),
               copiedValues,
               copiedTag,
               supportHandle("transportation", "HLAreliable"),
               new MockFederateHandle(
                  federationState.federationName + ":" + federateName),
               null,
               deliveryTime,
               OrderType.TIMESTAMP,
               receivedOrder,
               retraction));
      }
      return retraction;
   }

   /** Enable the fixture's queued TSO/save-boundary probe for adapter tests. */
   public void enableTsoSaveBoundaryProbe() throws NotConnected {
      requireJoined();
      tsoSaveBoundaryProbe = true;
      pendingTimestampedInteractionClass = null;
      pendingTimestampedInteractionParameters = null;
      pendingTimestampedInteractionTag = null;
      pendingTimestampedInteractionTime = null;
   }

   /** Enable a fixture-only regional object callback probe for adapter tests. */
   public void enableTimestampedRegionalAttributeProbe() throws NotConnected {
      requireJoined();
      timestampedRegionalAttributeProbe = true;
      pendingTimestampedAttributeObject = null;
      pendingTimestampedAttributeValues = null;
      pendingTimestampedAttributeTag = null;
      pendingTimestampedAttributeTime = null;
   }

   /** Enable the creation-time delayed DDM policy for regional delivery tests. */
   public void enableDelayedSubscriptionEvaluation() throws NotConnected {
      requireJoined();
      if (federationState != null) {
         federationState.delaySubscriptionEvaluation = true;
      }
   }

   /** Enable the fixture-only relaxed DDM policy for boundary-touching ranges. */
   public void enableAllowRelaxedDDM() throws NotConnected {
      requireJoined();
      if (federationState != null) {
         federationState.allowRelaxedDDM = true;
      }
   }

   @Override
   public MessageRetractionHandle sendInteractionWithRegionsWithTime(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      RegionHandleSet regions,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      MockRegionHandleSet copiedRegions = new MockRegionHandleSet();
      Set<MockSupportHandle> sourceRegions = new HashSet<>();
      for (RegionHandle region : regions) {
         handlePayload(region, "region");
         MockSupportHandle sourceRegion = (MockSupportHandle) region;
         copiedRegions.add(sourceRegion);
         sourceRegions.add(sourceRegion);
      }
      if (federationState == null || federationState.members.size() <= 1) {
         return supportHandle("retraction", Integer.toString(nextRetraction++));
      }
      MockParameterHandleValueMap copiedValues = copyParameters(parameterValues);
      byte[] copiedTag = userSuppliedTag.clone();
      MockSupportHandle retraction = nextSharedRetraction();
      federationState.timedRetractions.put(retraction, new MockRetractionRecord(retraction));
      boolean delayed = federationState.delaySubscriptionEvaluation;
      for (MockRTIambassador member : federationState.members) {
         if (member == this) {
            continue;
         }
         Set<MockSupportHandle> subscribedRegions =
            member.subscribedInteractionRegions.get(interactionName);
         if (!delayed && (!member.isActiveInteractionSubscription(interactionName)
               || !regionsOverlap(this, sourceRegions, member, subscribedRegions))) {
            continue;
         }
         LogicalTime deliveryTime = copyLogicalTime(time);
         OrderType receivedOrder = member.timeConstrained
            ? OrderType.TIMESTAMP
            : OrderType.RECEIVE;
         RegionHandleSet optionalRegions = member.conveyRegionDesignatorSets
            ? copyRegionSet(copiedRegions)
            : null;
         member.routeTimedDelivery(
            retraction,
            deliveryTime,
            () -> {
               if (delayed && (!member.isActiveInteractionSubscription(interactionName)
                     || !regionsOverlap(this, sourceRegions, member,
                        member.subscribedInteractionRegions.get(interactionName)))) {
                  return;
               }
               member.federateAmbassador.receiveInteraction(
                  supportHandle("interaction", interactionName),
                  copiedValues,
                  copiedTag,
                  supportHandle("transportation", "HLAreliable"),
                  new MockFederateHandle(
                     federationState.federationName + ":" + federateName),
                  optionalRegions,
                  deliveryTime,
                  OrderType.TIMESTAMP,
                  receivedOrder,
                  retraction);
            });
      }
      return retraction;
   }

   /** Queue one of each timestamped callback shape for bridge integration tests. */
   public void emitTimestampedCallbacks(
      ObjectInstanceHandle objectInstance,
      InteractionClassHandle interactionClass,
      LogicalTime time) throws NotConnected
   {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      handlePayload(interactionClass, "interaction");
      MockAttributeHandleValueMap values = new MockAttributeHandleValueMap();
      values.put(supportHandle("attribute", "Efficiency"), new byte[] { 7, 8 });
      MockParameterHandleValueMap parameters = new MockParameterHandleValueMap();
      parameters.put(supportHandle("parameter", "TemperatureOk"), new byte[] { 1 });
      MockFederateHandle producer = new MockFederateHandle("mock-producing-federate");
      MockSupportHandle transport = supportHandle("transportation", "HLAreliable");
      MockRegionHandleSet sentRegions = new MockRegionHandleSet();
      sentRegions.add(supportHandle("region", "callback-region"));
      MockSupportHandle reflectionRetraction = supportHandle("retraction", "callback-reflect");
      MockSupportHandle interactionRetraction = supportHandle("retraction", "callback-interact");
      MockSupportHandle removalRetraction = supportHandle("retraction", "callback-remove");
      deliverCallback(() -> federateAmbassador.reflectAttributeValues(
         objectInstance, values, new byte[] { 9 }, transport, producer, sentRegions,
         time, OrderType.TIMESTAMP, OrderType.TIMESTAMP, reflectionRetraction));
      deliverCallback(() -> federateAmbassador.receiveInteraction(
         interactionClass, parameters, new byte[] { 10 }, transport, producer, sentRegions,
         time, OrderType.TIMESTAMP, OrderType.TIMESTAMP, interactionRetraction));
      deliverCallback(() -> federateAmbassador.removeObjectInstance(
         objectInstance, new byte[] { 11 }, producer, time,
         OrderType.TIMESTAMP, OrderType.TIMESTAMP, removalRetraction));
   }

   /** Concrete-handle bridge for the fixture's intentionally shared test handle. */
   public void emitTimestampedCallbacksForTest(
      Object objectInstance,
      Object interactionClass,
      Object time) throws NotConnected
   {
      emitTimestampedCallbacks(
         (ObjectInstanceHandle) (MockSupportHandle) objectInstance,
         (InteractionClassHandle) (MockSupportHandle) interactionClass,
         (LogicalTime) time);
   }

   /** Encoded test bridge avoids JPype overload resolution on the shared fixture handle. */
   public void emitTimestampedCallbacksForEncoded(
      byte[] objectInstanceEncoded,
      byte[] interactionEncoded,
      long timeValue) throws NotConnected
   {
      emitTimestampedCallbacks(
         (ObjectInstanceHandle) new MockSupportHandle(objectInstanceEncoded, 0),
         (InteractionClassHandle) new MockSupportHandle(interactionEncoded, 0),
         new HLAinteger64Time(timeValue));
   }

   /** Floating-time variant used to prove exact double values cross the JVM bridge. */
   public void emitTimestampedCallbacksForFloatEncoded(
      byte[] objectInstanceEncoded,
      byte[] interactionEncoded,
      double timeValue) throws NotConnected
   {
      if (!(timeFactory instanceof HLAfloat64TimeFactory)) {
         throw new IllegalArgumentException("fixture is not using HLAfloat64Time");
      }
      emitTimestampedCallbacks(
         (ObjectInstanceHandle) new MockSupportHandle(objectInstanceEncoded, 0),
         (InteractionClassHandle) new MockSupportHandle(interactionEncoded, 0),
         ((HLAfloat64TimeFactory) timeFactory).makeLogicalTime(timeValue));
   }

   /** Queue the scope and per-object update-relevance callback overloads. */
   public void emitAdvisoryCallbacksForEncoded(
      byte[] objectInstanceEncoded,
      byte[] attributeEncoded) throws NotConnected
   {
      requireJoined();
      MockSupportHandle objectInstance = new MockSupportHandle(objectInstanceEncoded, 0);
      MockSupportHandle attribute = new MockSupportHandle(attributeEncoded, 0);
      MockAttributeHandleSet attributes = new MockAttributeHandleSet();
      attributes.add(attribute);
      deliverCallback(() -> federateAmbassador.attributesInScope(objectInstance, attributes));
      deliverCallback(() -> federateAmbassador.attributesOutOfScope(objectInstance, attributes));
      deliverCallback(() -> federateAmbassador.turnUpdatesOnForObjectInstance(objectInstance, attributes));
      deliverCallback(() -> federateAmbassador.turnUpdatesOnForObjectInstance(
         objectInstance, attributes, "High"));
      deliverCallback(() -> federateAmbassador.turnUpdatesOffForObjectInstance(objectInstance, attributes));
   }

   /** Queue the remaining standard callback shapes for bridge integration tests. */
   public void emitAdditionalCallbacksForTest() throws NotConnected {
      requireJoined();
      MockSupportHandle retraction = supportHandle("retraction", "callback-request");
      deliverCallback(() -> federateAmbassador.federateResigned(
         "fixture RTI removed this federate"));
      deliverCallback(() -> federateAmbassador.flushQueueGrant(
         currentLogicalTime, currentLogicalTime));
      deliverCallback(() -> federateAmbassador.requestRetraction(retraction));
   }

   @Override
   public void retract(MessageRetractionHandle retraction) throws NotConnected {
      requireJoined();
      if (!(retraction instanceof MockSupportHandle)) {
         throw new IllegalArgumentException("fixture requires a decoded retraction handle");
      }
      lastRetracted = retraction;
      MockSupportHandle handle = (MockSupportHandle) retraction;
      if (federationState == null) {
         return;
      }
      MockRetractionRecord record = federationState.timedRetractions.get(handle);
      if (record == null || record.retracted) {
         return;
      }
      record.retracted = true;
      for (MockRTIambassador member : federationState.members) {
         member.pendingTimedDeliveries.removeIf(
            delivery -> delivery.retraction.equals(handle));
      }
      for (MockRTIambassador member : record.deliveredRecipients) {
         member.deliverCallback(() -> member.federateAmbassador.requestRetraction(handle));
      }
      record.pendingRecipients.clear();
   }

   @Override
   public FederateHandleFactory getFederateHandleFactory() throws NotConnected {
      requireConnected();
      return federateHandleFactory;
   }

   @Override
   public FederateHandleSetFactory getFederateHandleSetFactory() throws NotConnected {
      requireConnected();
      return federateHandleSetFactory;
   }

   @Override
   public ObjectClassHandleFactory getObjectClassHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public AttributeHandleFactory getAttributeHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public InteractionClassHandleFactory getInteractionClassHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public InteractionClassHandleSetFactory getInteractionClassHandleSetFactory() throws NotConnected {
      requireConnected();
      return new MockInteractionClassHandleSetFactory();
   }

   @Override
   public ParameterHandleFactory getParameterHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public TransportationTypeHandleFactory getTransportationTypeHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public DimensionHandleFactory getDimensionHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public RegionHandleFactory getRegionHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public MessageRetractionHandleFactory getMessageRetractionHandleFactory()
      throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public DimensionHandleSetFactory getDimensionHandleSetFactory() throws NotConnected {
      requireConnected();
      return dimensionHandleSetFactory;
   }

   @Override
   public RegionHandleSetFactory getRegionHandleSetFactory() throws NotConnected {
      requireConnected();
      return regionHandleSetFactory;
   }

   @Override
   public AttributeHandleSetFactory getAttributeHandleSetFactory() throws NotConnected {
      requireConnected();
      return attributeHandleSetFactory;
   }

   @Override
   public AttributeHandleValueMapFactory getAttributeHandleValueMapFactory() throws NotConnected {
      requireConnected();
      return attributeHandleValueMapFactory;
   }

   @Override
   public ParameterHandleValueMapFactory getParameterHandleValueMapFactory() throws NotConnected {
      requireConnected();
      return parameterHandleValueMapFactory;
   }

   @Override
   public void publishObjectClassAttributes(ObjectClassHandle objectClass, AttributeHandleSet attributes)
      throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (AttributeHandle attribute : attributes) {
         publishedObjectAttributes.add(
            objectAttributeKey(objectClassName, handlePayload(attribute, "attribute")));
      }
      deliverCallback(() -> federateAmbassador.startRegistrationForObjectClass(objectClass));
   }

   @Override
   public void unpublishObjectClass(ObjectClassHandle objectClass) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      publishedObjectAttributes.removeIf(key -> key.startsWith(objectClassName + "\u001F"));
      deliverCallback(() -> federateAmbassador.stopRegistrationForObjectClass(objectClass));
   }

   @Override
   public void unpublishObjectClassAttributes(
      ObjectClassHandle objectClass, AttributeHandleSet attributes) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (AttributeHandle attribute : attributes) {
         publishedObjectAttributes.remove(
            objectAttributeKey(objectClassName, handlePayload(attribute, "attribute")));
      }
   }

   @Override
   public void publishObjectClassDirectedInteractions(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (InteractionClassHandle interactionClass : interactionClasses) {
         publishedDirectedInteractions.add(
            directedInteractionKey(objectClassName, handlePayload(interactionClass, "interaction")));
      }
   }

   @Override
   public void unpublishObjectClassDirectedInteractions(ObjectClassHandle objectClass)
      throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      publishedDirectedInteractions.removeIf(
         key -> key.startsWith(objectClassName + "\u001F"));
   }

   @Override
   public void unpublishObjectClassDirectedInteractions(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (InteractionClassHandle interactionClass : interactionClasses) {
         publishedDirectedInteractions.remove(
            directedInteractionKey(objectClassName, handlePayload(interactionClass, "interaction")));
      }
   }

   public void subscribeObjectClassAttributes(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      boolean active,
      String updateRateDesignator) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (AttributeHandle attribute : attributes) {
         String key = objectAttributeKey(objectClassName, handlePayload(attribute, "attribute"));
         subscribedObjectAttributes.add(key);
         if (active) {
            passiveObjectSubscriptions.remove(key);
         } else {
            passiveObjectSubscriptions.add(key);
         }
         subscribedObjectRegions.putIfAbsent(key, new HashSet<>());
      }
   }

   @Override
   public void subscribeObjectClassAttributes(
      ObjectClassHandle objectClass, AttributeHandleSet attributes) throws NotConnected {
      subscribeObjectClassAttributes(objectClass, attributes, true, "");
   }

   @Override
   public void subscribeObjectClassAttributes(
      ObjectClassHandle objectClass, AttributeHandleSet attributes, String updateRateDesignator)
      throws NotConnected {
      subscribeObjectClassAttributes(objectClass, attributes, true, updateRateDesignator);
   }

   @Override
   public void subscribeObjectClassAttributesPassively(
      ObjectClassHandle objectClass, AttributeHandleSet attributes) throws NotConnected {
      subscribeObjectClassAttributes(objectClass, attributes, false, "");
   }

   @Override
   public void subscribeObjectClassAttributesPassively(
      ObjectClassHandle objectClass, AttributeHandleSet attributes, String updateRateDesignator)
      throws NotConnected {
      subscribeObjectClassAttributes(objectClass, attributes, false, updateRateDesignator);
   }

   @Override
   public void unsubscribeObjectClass(ObjectClassHandle objectClass) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      subscribedObjectAttributes.removeIf(key -> key.startsWith(objectClassName + "\u001F"));
      passiveObjectSubscriptions.removeIf(key -> key.startsWith(objectClassName + "\u001F"));
      subscribedObjectRegions.keySet().removeIf(
         key -> key.startsWith(objectClassName + "\u001F"));
   }

   @Override
   public void unsubscribeObjectClassAttributes(
      ObjectClassHandle objectClass, AttributeHandleSet attributes) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (AttributeHandle attribute : attributes) {
         String key = objectAttributeKey(objectClassName, handlePayload(attribute, "attribute"));
         subscribedObjectAttributes.remove(key);
         passiveObjectSubscriptions.remove(key);
         subscribedObjectRegions.remove(key);
      }
   }

   public void subscribeObjectClassDirectedInteractions(
      ObjectClassHandle objectClass,
      Set<InteractionClassHandle> interactionClasses,
      boolean universally) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (InteractionClassHandle interactionClass : interactionClasses) {
         subscribedDirectedInteractions.add(
            directedInteractionKey(objectClassName, handlePayload(interactionClass, "interaction")));
      }
   }

   @Override
   public void subscribeObjectClassDirectedInteractions(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected {
      subscribeObjectClassDirectedInteractions(objectClass, interactionClasses, false);
   }

   @Override
   public void subscribeObjectClassDirectedInteractionsUniversally(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected {
      subscribeObjectClassDirectedInteractions(objectClass, interactionClasses, true);
   }

   @Override
   public void unsubscribeObjectClassDirectedInteractions(ObjectClassHandle objectClass)
      throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      subscribedDirectedInteractions.removeIf(
         key -> key.startsWith(objectClassName + "\u001F"));
   }

   @Override
   public void unsubscribeObjectClassDirectedInteractions(
      ObjectClassHandle objectClass, InteractionClassHandleSet interactionClasses)
      throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      for (InteractionClassHandle interactionClass : interactionClasses) {
         subscribedDirectedInteractions.remove(
            directedInteractionKey(objectClassName, handlePayload(interactionClass, "interaction")));
      }
   }

   public void subscribeObjectClassAttributesWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      boolean active,
      String updateRateDesignator) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      validateAttributeRegionPairs(attributesAndRegions);
      Set<String> affectedKeys = new HashSet<>();
      for (AttributeSetRegionSetPair pair : attributesAndRegions) {
         for (AttributeHandle attribute : pair.getAttributes()) {
            affectedKeys.add(
               objectAttributeKey(objectClassName, handlePayload(attribute, "attribute")));
         }
      }
      Map<String, Map<String, Boolean>> scopeBefore = captureScopeStates(affectedKeys);
      for (AttributeSetRegionSetPair pair : attributesAndRegions) {
         for (AttributeHandle attribute : pair.getAttributes()) {
            String key = objectAttributeKey(objectClassName, handlePayload(attribute, "attribute"));
            subscribedObjectAttributes.add(key);
            if (active) {
               passiveObjectSubscriptions.remove(key);
            } else {
               passiveObjectSubscriptions.add(key);
            }
            Set<MockSupportHandle> existing = subscribedObjectRegions.get(key);
            boolean newlyCreated = existing == null;
            if (existing == null) {
               existing = new HashSet<>();
               subscribedObjectRegions.put(key, existing);
            }
            if (!newlyCreated && existing.isEmpty()) {
               continue;
            }
            for (RegionHandle region : pair.getRegions()) {
               existing.add((MockSupportHandle) region);
            }
         }
      }
      queueScopeChanges(scopeBefore);
      queueDiscoveriesForCurrentSubscriptions();
   }

   @Override
   public void unsubscribeObjectClassAttributesWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      validateAttributeRegionPairs(attributesAndRegions);
      Set<String> affectedKeys = new HashSet<>();
      for (AttributeSetRegionSetPair pair : attributesAndRegions) {
         for (AttributeHandle attribute : pair.getAttributes()) {
            affectedKeys.add(
               objectAttributeKey(objectClassName, handlePayload(attribute, "attribute")));
         }
      }
      Map<String, Map<String, Boolean>> scopeBefore = captureScopeStates(affectedKeys);
      for (AttributeSetRegionSetPair pair : attributesAndRegions) {
         for (AttributeHandle attribute : pair.getAttributes()) {
            String key = objectAttributeKey(objectClassName, handlePayload(attribute, "attribute"));
            Set<MockSupportHandle> existing = subscribedObjectRegions.get(key);
            if (existing != null && !existing.isEmpty()) {
               for (RegionHandle region : pair.getRegions()) {
                  existing.remove((MockSupportHandle) region);
               }
               if (existing.isEmpty()) {
                  subscribedObjectAttributes.remove(key);
                  passiveObjectSubscriptions.remove(key);
                  subscribedObjectRegions.remove(key);
               }
            } else {
               subscribedObjectAttributes.remove(key);
               passiveObjectSubscriptions.remove(key);
               subscribedObjectRegions.remove(key);
            }
         }
      }
      queueScopeChanges(scopeBefore);
   }

   @Override
   public void publishInteractionClass(InteractionClassHandle interactionClass) throws NotConnected {
      requireJoined();
      publishedInteractions.add(handlePayload(interactionClass, "interaction"));
      deliverCallback(() -> federateAmbassador.turnInteractionsOn(interactionClass));
   }

   @Override
   public void unpublishInteractionClass(InteractionClassHandle interactionClass) throws NotConnected {
      requireJoined();
      publishedInteractions.remove(handlePayload(interactionClass, "interaction"));
      deliverCallback(() -> federateAmbassador.turnInteractionsOff(interactionClass));
   }

   public void subscribeInteractionClass(InteractionClassHandle interactionClass, boolean active)
      throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      subscribedInteractions.add(interactionName);
      if (active) {
         passiveInteractionSubscriptions.remove(interactionName);
      } else {
         passiveInteractionSubscriptions.add(interactionName);
      }
      subscribedInteractionRegions.putIfAbsent(interactionName, new HashSet<>());
   }

   @Override
   public void subscribeInteractionClass(InteractionClassHandle interactionClass)
      throws NotConnected {
      subscribeInteractionClass(interactionClass, true);
   }

   @Override
   public void subscribeInteractionClassPassively(InteractionClassHandle interactionClass)
      throws NotConnected {
      subscribeInteractionClass(interactionClass, false);
   }

   public void subscribeInteractionClassWithRegions(
      InteractionClassHandle interactionClass, RegionHandleSet regions, boolean active)
      throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      for (RegionHandle region : regions) {
         handlePayload(region, "region");
      }
      subscribedInteractions.add(interactionName);
      if (active) {
         passiveInteractionSubscriptions.remove(interactionName);
      } else {
         passiveInteractionSubscriptions.add(interactionName);
      }
      Set<MockSupportHandle> existing = subscribedInteractionRegions.get(interactionName);
      boolean newlyCreated = existing == null;
      if (existing == null) {
         existing = new HashSet<>();
         subscribedInteractionRegions.put(interactionName, existing);
      }
      if (!newlyCreated && existing.isEmpty()) {
         return;
      }
      for (RegionHandle region : regions) {
         existing.add((MockSupportHandle) region);
      }
   }

   @Override
   public void subscribeInteractionClassWithRegions(
      InteractionClassHandle interactionClass, RegionHandleSet regions) throws NotConnected {
      subscribeInteractionClassWithRegions(interactionClass, regions, true);
   }

   @Override
   public void subscribeInteractionClassPassivelyWithRegions(
      InteractionClassHandle interactionClass, RegionHandleSet regions) throws NotConnected {
      subscribeInteractionClassWithRegions(interactionClass, regions, false);
   }

   @Override
   public void subscribeObjectClassAttributesWithRegions(
      ObjectClassHandle objectClass, AttributeSetRegionSetPairList attributesAndRegions)
      throws NotConnected {
      subscribeObjectClassAttributesWithRegions(objectClass, attributesAndRegions, true, "");
   }

   @Override
   public void subscribeObjectClassAttributesWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      String updateRateDesignator) throws NotConnected {
      subscribeObjectClassAttributesWithRegions(
         objectClass, attributesAndRegions, true, updateRateDesignator);
   }

   @Override
   public void subscribeObjectClassAttributesPassivelyWithRegions(
      ObjectClassHandle objectClass, AttributeSetRegionSetPairList attributesAndRegions)
      throws NotConnected {
      subscribeObjectClassAttributesWithRegions(objectClass, attributesAndRegions, false, "");
   }

   @Override
   public void subscribeObjectClassAttributesPassivelyWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      String updateRateDesignator) throws NotConnected {
      subscribeObjectClassAttributesWithRegions(
         objectClass, attributesAndRegions, false, updateRateDesignator);
   }

   @Override
   public void unsubscribeInteractionClass(InteractionClassHandle interactionClass) throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      subscribedInteractions.remove(interactionName);
      passiveInteractionSubscriptions.remove(interactionName);
      subscribedInteractionRegions.remove(interactionName);
   }

   @Override
   public void unsubscribeInteractionClassWithRegions(
      InteractionClassHandle interactionClass, RegionHandleSet regions) throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      for (RegionHandle region : regions) {
         handlePayload(region, "region");
      }
      Set<MockSupportHandle> existing = subscribedInteractionRegions.get(interactionName);
      if (existing != null && !existing.isEmpty()) {
         for (RegionHandle region : regions) {
            existing.remove((MockSupportHandle) region);
         }
         if (existing.isEmpty()) {
            subscribedInteractions.remove(interactionName);
            passiveInteractionSubscriptions.remove(interactionName);
            subscribedInteractionRegions.remove(interactionName);
         }
      } else {
         subscribedInteractions.remove(interactionName);
         passiveInteractionSubscriptions.remove(interactionName);
         subscribedInteractionRegions.remove(interactionName);
      }
   }

   @Override
   public void reserveObjectInstanceName(String objectInstanceName) throws NotConnected {
      requireJoined();
      if (reservedObjectInstanceNames.add(objectInstanceName) && !objectInstances.containsKey(objectInstanceName)) {
         deliverCallback(() -> federateAmbassador.objectInstanceNameReservationSucceeded(
            objectInstanceName));
      } else {
         deliverCallback(() -> federateAmbassador.objectInstanceNameReservationFailed(
            objectInstanceName));
      }
   }

   @Override
   public void releaseObjectInstanceName(String objectInstanceName) throws NotConnected {
      requireJoined();
      if (!reservedObjectInstanceNames.remove(objectInstanceName)) {
         throw new IllegalArgumentException("object instance name is not reserved");
      }
   }

   @Override
   public void reserveMultipleObjectInstanceNames(Set<String> objectInstanceNames)
      throws NotConnected {
      requireJoined();
      Set<String> succeeded = new HashSet<>();
      Set<String> failed = new HashSet<>();
      for (String objectInstanceName : objectInstanceNames) {
         if (reservedObjectInstanceNames.add(objectInstanceName)
            && !objectInstances.containsKey(objectInstanceName)) {
            succeeded.add(objectInstanceName);
         } else {
            failed.add(objectInstanceName);
         }
      }
      if (!succeeded.isEmpty()) {
         deliverCallback(() -> federateAmbassador.multipleObjectInstanceNameReservationSucceeded(
            new HashSet<>(succeeded)));
      }
      if (!failed.isEmpty()) {
         deliverCallback(() -> federateAmbassador.multipleObjectInstanceNameReservationFailed(
            new HashSet<>(failed)));
      }
   }

   @Override
   public void releaseMultipleObjectInstanceNames(Set<String> objectInstanceNames)
      throws NotConnected {
      requireJoined();
      for (String objectInstanceName : objectInstanceNames) {
         if (!reservedObjectInstanceNames.remove(objectInstanceName)) {
            throw new IllegalArgumentException("object instance name is not reserved");
         }
      }
   }

   @Override
   public ObjectInstanceHandle registerObjectInstance(ObjectClassHandle objectClass) throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      String objectInstanceName = "mock-object-" + nextObjectInstance++;
      return completeObjectInstanceRegistration(objectClass, objectInstanceName);
   }

   @Override
   public ObjectInstanceHandle registerObjectInstance(
      ObjectClassHandle objectClass, String objectInstanceName) throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      if (!reservedObjectInstanceNames.remove(objectInstanceName)) {
         throw new IllegalArgumentException("object instance name is not reserved");
      }
      return completeObjectInstanceRegistration(objectClass, objectInstanceName);
   }

   @Override
   public ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      validateAttributeRegionPairs(attributesAndRegions);
      String objectInstanceName = "mock-object-" + nextObjectInstance++;
      return completeObjectInstanceRegistration(
         objectClass, objectInstanceName, regionMapFromPairs(attributesAndRegions));
   }

   @Override
   public ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      String objectInstanceName) throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      validateAttributeRegionPairs(attributesAndRegions);
      if (!reservedObjectInstanceNames.remove(objectInstanceName)) {
         throw new IllegalArgumentException("object instance name is not reserved");
      }
      return completeObjectInstanceRegistration(
         objectClass, objectInstanceName, regionMapFromPairs(attributesAndRegions));
   }

   @Override
   public void associateRegionsForUpdates(
      ObjectInstanceHandle objectInstance,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeRegionPairs(attributesAndRegions);
      MockObjectRecord record = federationState == null
         ? null
         : federationState.objectInstances.get(objectInstanceName);
      if (record != null) {
         updateAssociatedRegionsForMembers(record, attributesAndRegions, false);
      }
   }

   @Override
   public void unassociateRegionsForUpdates(
      ObjectInstanceHandle objectInstance,
      AttributeSetRegionSetPairList attributesAndRegions) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeRegionPairs(attributesAndRegions);
      MockObjectRecord record = federationState == null
         ? null
         : federationState.objectInstances.get(objectInstanceName);
      if (record != null) {
         updateAssociatedRegionsForMembers(record, attributesAndRegions, true);
      }
   }

   private void updateAssociatedRegionsForMembers(
      MockObjectRecord record,
      AttributeSetRegionSetPairList attributesAndRegions,
      boolean remove) {
      Set<String> affectedKeys = affectedObjectAttributeKeys(
         record.objectClassName, attributesAndRegions);
      Map<MockRTIambassador, Map<String, Map<String, Boolean>>> scopeBeforeByMember =
         new LinkedHashMap<>();
      if (federationState != null) {
         for (MockRTIambassador member : federationState.members) {
            scopeBeforeByMember.put(member, member.captureScopeStates(affectedKeys));
         }
      }
      mergeAttributeRegions(record.attributeRegionsByName, attributesAndRegions, remove);
      for (Map.Entry<MockRTIambassador, Map<String, Map<String, Boolean>>> entry
         : scopeBeforeByMember.entrySet()) {
         entry.getKey().queueScopeChanges(entry.getValue());
      }
   }

   private Set<String> affectedObjectAttributeKeys(
      String objectClassName,
      AttributeSetRegionSetPairList attributesAndRegions) {
      Set<String> affectedKeys = new HashSet<>();
      for (AttributeSetRegionSetPair pair : attributesAndRegions) {
         for (AttributeHandle attribute : pair.getAttributes()) {
            affectedKeys.add(
               objectAttributeKey(
                  objectClassName,
                  handlePayload(attribute, "attribute")));
         }
      }
      return affectedKeys;
   }

   private ObjectInstanceHandle completeObjectInstanceRegistration(
      ObjectClassHandle objectClass, String objectInstanceName) {
      return completeObjectInstanceRegistration(
         objectClass, objectInstanceName, new LinkedHashMap<>());
   }

   private ObjectInstanceHandle completeObjectInstanceRegistration(
      ObjectClassHandle objectClass,
      String objectInstanceName,
      Map<String, Set<MockSupportHandle>> attributeRegionsByName) {
      MockSupportHandle handle = supportHandle("object-instance", objectInstanceName);
      objectInstances.put(objectInstanceName, handle);
      String objectClassName = handlePayload(objectClass, "object");
      if (federationState == null || federationState.members.size() <= 1) {
         knownObjectInstances.add(objectInstanceName);
         deliverCallback(() -> federateAmbassador.discoverObjectInstance(
            handle,
            objectClass,
            objectInstanceName,
            new MockFederateHandle("mock-producing-federate")));
      } else {
         MockObjectRecord record = new MockObjectRecord(
            this, handle, objectClassName, objectInstanceName, attributeRegionsByName);
         federationState.objectInstances.put(objectInstanceName, record);
         for (MockRTIambassador member : federationState.members) {
            if (member == this
               || !member.subscribesToObjectClass(objectClassName)
               || !member.isRegionRelevantObject(record)) {
               continue;
            }
            member.knownObjectInstances.add(objectInstanceName);
            ObjectClassHandle memberObjectClass = supportHandle("object", objectClassName);
            member.deliverCallback(() -> member.federateAmbassador.discoverObjectInstance(
               handle,
               memberObjectClass,
               objectInstanceName,
               new MockFederateHandle(
                  federationState.federationName + ":" + federateName)));
         }
      }
      return handle;
   }

   @Override
   public ObjectInstanceHandle getObjectInstanceHandle(String objectInstanceName) throws NotConnected {
      requireJoined();
      MockSupportHandle handle = objectInstances.get(objectInstanceName);
      if (handle == null) {
         throw new IllegalArgumentException("object instance is not known");
      }
      return handle;
   }

   @Override
   public String getObjectInstanceName(ObjectInstanceHandle objectInstance) throws NotConnected {
      requireJoined();
      return handlePayload(objectInstance, "object-instance");
   }

   @Override
   public ObjectInstanceHandleFactory getObjectInstanceHandleFactory() throws NotConnected {
      requireConnected();
      return supportHandleFactory;
   }

   @Override
   public void deleteObjectInstance(ObjectInstanceHandle objectInstance, byte[] userSuppliedTag)
      throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      MockSupportHandle known = objectInstances.remove(objectInstanceName);
      if (known == null) {
         throw new IllegalArgumentException("object instance is not known");
      }
      byte[] tag = userSuppliedTag.clone();
      knownObjectInstances.remove(objectInstanceName);
      deliverCallback(() -> federateAmbassador.removeObjectInstance(
         known,
         tag,
         new MockFederateHandle("mock-producing-federate")));
   }

   @Override
   public void localDeleteObjectInstance(ObjectInstanceHandle objectInstance) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      if (objectInstances.remove(objectInstanceName) == null) {
         throw new IllegalArgumentException("object instance is not known");
      }
   }

   @Override
   public void updateAttributeValues(
      ObjectInstanceHandle objectInstance,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag) throws NotConnected
   {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      MockSupportHandle known = objectInstances.get(objectInstanceName);
      if (known == null) {
         throw new IllegalArgumentException("object instance is not known");
      }
      MockAttributeHandleValueMap copiedValues = new MockAttributeHandleValueMap();
      for (Map.Entry<AttributeHandle, byte[]> entry : attributeValues.entrySet()) {
         handlePayload(entry.getKey(), "attribute");
         copiedValues.put(entry.getKey(), entry.getValue().clone());
      }
      byte[] tag = userSuppliedTag.clone();
      if (federationState != null && federationState.members.size() > 1) {
         MockObjectRecord record = federationState.objectInstances.get(objectInstanceName);
         if (record != null) {
            boolean delayed = federationState.delaySubscriptionEvaluation;
            for (MockRTIambassador member : federationState.members) {
               if (member == this) {
                  continue;
               }
               MockAttributeHandleValueMap subscribedValues =
                  member.filterSubscribedAttributes(record.objectClassName, copiedValues);
               MockAttributeHandleValueMap eligibleValues =
                  filterRegionRelevantAttributes(this, record, member, subscribedValues);
               if (!delayed && eligibleValues.isEmpty()) {
                  continue;
               }
               member.deliverCallback(() -> {
                  MockAttributeHandleValueMap values = eligibleValues;
                  if (delayed) {
                     values = filterRegionRelevantAttributes(
                        this,
                        record,
                        member,
                        member.filterSubscribedAttributes(record.objectClassName, copiedValues));
                  }
                  if (values.isEmpty()) {
                     return;
                  }
                  RegionHandleSet currentOptionalRegions = objectUpdateRegionDesignator(
                     member, record, values);
                  member.federateAmbassador.reflectAttributeValues(
                     record.handle,
                     values,
                     tag,
                     supportHandle("transportation", "HLAreliable"),
                     new MockFederateHandle(
                        federationState.federationName + ":" + federateName),
                     currentOptionalRegions);
               });
            }
         }
         return;
      }
      deliverCallback(() -> federateAmbassador.reflectAttributeValues(
         known,
         copiedValues,
         tag,
         supportHandle("transportation", "HLAreliable"),
         new MockFederateHandle("mock-producing-federate"),
         null));
   }

   @Override
   public void requestAttributeValueUpdate(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected
   {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      MockSupportHandle known = objectInstances.get(objectInstanceName);
      if (known == null) {
         throw new IllegalArgumentException("object instance is not known");
      }
      for (AttributeHandle attribute : attributes) {
         handlePayload(attribute, "attribute");
      }
      byte[] tag = userSuppliedTag.clone();
      deliverCallback(() -> federateAmbassador.provideAttributeValueUpdate(
         known, attributes, tag));
   }

   @Override
   public void requestAttributeValueUpdate(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected
   {
      requireJoined();
      handlePayload(objectClass, "object");
      for (AttributeHandle attribute : attributes) {
         handlePayload(attribute, "attribute");
      }
      byte[] tag = userSuppliedTag.clone();
      MockSupportHandle known = objectInstances.values().stream().findFirst().orElse(null);
      if (known != null) {
         deliverCallback(() -> federateAmbassador.provideAttributeValueUpdate(
            known, attributes, tag));
      }
   }

   @Override
   public void requestAttributeValueUpdateWithRegions(
      ObjectClassHandle objectClass,
      AttributeSetRegionSetPairList attributesAndRegions,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectClassName = handlePayload(objectClass, "object");
      validateAttributeRegionPairs(attributesAndRegions);
      if (federationState == null) {
         return;
      }
      Map<String, Set<MockSupportHandle>> requestedRegions =
         regionMapFromPairs(attributesAndRegions);
      for (MockObjectRecord record : federationState.objectInstances.values()) {
         if (!record.objectClassName.equals(objectClassName)) {
            continue;
         }
         MockAttributeHandleSet requestedAttributes = new MockAttributeHandleSet();
         for (Map.Entry<String, Set<MockSupportHandle>> entry : requestedRegions.entrySet()) {
            Set<MockSupportHandle> sourceRegions = record.attributeRegionsByName.get(
               entry.getKey());
            if (regionsOverlap(
               record.owner,
               sourceRegions == null ? new HashSet<>() : sourceRegions,
               this,
               entry.getValue())) {
               requestedAttributes.add(supportHandle("attribute", entry.getKey()));
            }
         }
         if (requestedAttributes.isEmpty()) {
            continue;
         }
         MockRTIambassador owner = record.owner;
         MockSupportHandle instance = record.handle;
         byte[] copiedTag = userSuppliedTag.clone();
         owner.deliverCallback(() -> owner.federateAmbassador.provideAttributeValueUpdate(
            instance, requestedAttributes, copiedTag));
      }
   }

   @Override
   public void changeAttributeOrderType(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      OrderType orderType) throws NotConnected {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      if (orderType == null) throw new IllegalArgumentException("order type is required");
   }

   @Override
   public void changeDefaultAttributeOrderType(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      OrderType orderType) throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      validateAttributeSet(attributes);
      if (orderType == null) throw new IllegalArgumentException("order type is required");
   }

   @Override
   public void changeInteractionOrderType(
      InteractionClassHandle interactionClass,
      OrderType orderType) throws NotConnected {
      requireJoined();
      handlePayload(interactionClass, "interaction");
      if (orderType == null) throw new IllegalArgumentException("order type is required");
   }

   @Override
   public void requestAttributeTransportationTypeChange(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      TransportationTypeHandle transportationType) throws NotConnected {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      handlePayload(transportationType, "transportation");
      deliverCallback(() -> federateAmbassador.confirmAttributeTransportationTypeChange(
         objectInstance, attributes, transportationType));
   }

   @Override
   public void changeDefaultAttributeTransportationType(
      ObjectClassHandle objectClass,
      AttributeHandleSet attributes,
      TransportationTypeHandle transportationType) throws NotConnected {
      requireJoined();
      handlePayload(objectClass, "object");
      validateAttributeSet(attributes);
      handlePayload(transportationType, "transportation");
   }

   @Override
   public void queryAttributeTransportationType(
      ObjectInstanceHandle objectInstance,
      AttributeHandle attribute) throws NotConnected {
      requireJoined();
      handlePayload(objectInstance, "object-instance");
      handlePayload(attribute, "attribute");
      deliverCallback(() -> federateAmbassador.reportAttributeTransportationType(
         objectInstance, attribute, supportHandle("transportation", "HLAreliable")));
   }

   @Override
   public void requestInteractionTransportationTypeChange(
      InteractionClassHandle interactionClass,
      TransportationTypeHandle transportationType) throws NotConnected {
      requireJoined();
      handlePayload(interactionClass, "interaction");
      handlePayload(transportationType, "transportation");
      deliverCallback(() -> federateAmbassador.confirmInteractionTransportationTypeChange(
         interactionClass, transportationType));
   }

   @Override
   public void queryInteractionTransportationType(
      FederateHandle federate,
      InteractionClassHandle interactionClass) throws NotConnected {
      requireJoined();
      if (!(federate instanceof MockFederateHandle)
         || !((MockFederateHandle) federate).value().startsWith("federate:")) {
         throw new IllegalArgumentException("fixture requires a decoded federate handle");
      }
      handlePayload(interactionClass, "interaction");
      deliverCallback(() -> federateAmbassador.reportInteractionTransportationType(
         federate, interactionClass, supportHandle("transportation", "HLAreliable")));
   }

   @Override
   public void queryAttributeOwnership(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      if (!objectInstances.containsKey(objectInstanceName)
         && (federationState == null || !federationState.objectInstances.containsKey(objectInstanceName))) {
         throw new IllegalArgumentException("object instance is not known");
      }
      for (AttributeHandle attribute : attributes) {
         handlePayload(attribute, "attribute");
      }
      if (federationState != null && federationState.members.size() > 1) {
         for (AttributeHandle attribute : attributes) {
            MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
            if (ownership.owner == null) {
               deliverCallback(() -> federateAmbassador.attributeIsNotOwned(
                  objectInstance, singletonAttributeSet(attribute)));
            } else {
               MockFederateHandle owner = new MockFederateHandle(
                  federationState.federationName + ":" + ownership.owner.federateName);
               deliverCallback(() -> federateAmbassador.informAttributeOwnership(
                  objectInstance, singletonAttributeSet(attribute), owner));
            }
         }
         return;
      }
      deliverCallback(() -> federateAmbassador.informAttributeOwnership(
         objectInstance,
         attributes,
         new MockFederateHandle("mock-owner")));
   }

   @Override
   public boolean isAttributeOwnedByFederate(
      ObjectInstanceHandle objectInstance,
      AttributeHandle attribute) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      handlePayload(attribute, "attribute");
      if (federationState != null && federationState.members.size() > 1) {
         return ownershipRecord(objectInstanceName, attribute).owner == this;
      }
      return objectInstances.containsKey(objectInstanceName);
   }

   @Override
   public void unconditionalAttributeOwnershipDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      if (federationState == null || federationState.members.size() <= 1) {
         return;
      }
      for (AttributeHandle attribute : attributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.owner != this) {
            throw new IllegalArgumentException("attribute is not owned by this federate");
         }
         ownership.owner = null;
         ownership.divestitureOffered = false;
         if (ownership.pendingAcquirer != null) {
            transferOwnership(objectInstance, attribute, ownership, userSuppliedTag);
         }
      }
   }

   @Override
   public void negotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      if (federationState == null || federationState.members.size() <= 1) {
         return;
      }
      for (AttributeHandle attribute : attributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.owner != this) {
            throw new IllegalArgumentException("attribute is not owned by this federate");
         }
         ownership.divestitureOffered = true;
      }
   }

   @Override
   public void confirmDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet confirmedAttributes,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(confirmedAttributes);
      if (federationState == null || federationState.members.size() <= 1) {
         return;
      }
      for (AttributeHandle attribute : confirmedAttributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.owner != this || ownership.pendingAcquirer == null) {
            throw new IllegalArgumentException("no pending ownership acquisition");
         }
         transferOwnership(objectInstance, attribute, ownership, userSuppliedTag);
      }
   }

   @Override
   public void cancelNegotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      if (federationState == null || federationState.members.size() <= 1) {
         return;
      }
      for (AttributeHandle attribute : attributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.owner != this) {
            throw new IllegalArgumentException("attribute is not owned by this federate");
         }
         ownership.divestitureOffered = false;
      }
   }

   @Override
   public void attributeOwnershipAcquisition(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet desiredAttributes,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(desiredAttributes);
      if (federationState == null || federationState.members.size() <= 1) {
         deliverCallback(() -> federateAmbassador.attributeOwnershipAcquisitionNotification(
            objectInstance, desiredAttributes, userSuppliedTag.clone()));
         return;
      }
      for (AttributeHandle attribute : desiredAttributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.owner == this) {
            throw new IllegalArgumentException("attribute is already owned by this federate");
         }
         if (ownership.pendingAcquirer != null) {
            throw new IllegalArgumentException("attribute ownership acquisition is pending");
         }
         MockAttributeHandleSet copiedAttributes = singletonAttributeSet(attribute);
         byte[] tag = userSuppliedTag.clone();
         if (ownership.owner == null) {
            ownership.owner = this;
            deliverCallback(() -> federateAmbassador.attributeOwnershipAcquisitionNotification(
               objectInstance, copiedAttributes, tag));
         } else {
            ownership.pendingAcquirer = this;
            ownership.pendingAcquisitionTag = tag;
            MockRTIambassador owner = ownership.owner;
            owner.deliverCallback(() -> owner.federateAmbassador.requestDivestitureConfirmation(
               objectInstance, copiedAttributes, tag));
         }
      }
   }

   @Override
   public void attributeOwnershipAcquisitionIfAvailable(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet desiredAttributes,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(desiredAttributes);
      if (federationState == null || federationState.members.size() <= 1) {
         attributeOwnershipAcquisition(objectInstance, desiredAttributes, userSuppliedTag);
         return;
      }
      for (AttributeHandle attribute : desiredAttributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         MockAttributeHandleSet copiedAttributes = singletonAttributeSet(attribute);
         byte[] tag = userSuppliedTag.clone();
         if (ownership.owner == this) {
            throw new IllegalArgumentException("attribute is already owned by this federate");
         }
         if (ownership.owner == null) {
            ownership.owner = this;
            deliverCallback(() -> federateAmbassador.attributeOwnershipAcquisitionNotification(
               objectInstance, copiedAttributes, tag));
         } else {
            deliverCallback(() -> federateAmbassador.attributeOwnershipUnavailable(
               objectInstance, copiedAttributes, tag));
         }
      }
   }

   @Override
   public void cancelAttributeOwnershipAcquisition(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      if (federationState == null || federationState.members.size() <= 1) {
         deliverCallback(() -> federateAmbassador.confirmAttributeOwnershipAcquisitionCancellation(
            objectInstance, attributes));
         return;
      }
      for (AttributeHandle attribute : attributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.pendingAcquirer != this) {
            throw new IllegalArgumentException("no pending ownership acquisition");
         }
         ownership.pendingAcquirer = null;
         ownership.pendingAcquisitionTag = null;
         deliverCallback(() -> federateAmbassador.confirmAttributeOwnershipAcquisitionCancellation(
            objectInstance, singletonAttributeSet(attribute)));
      }
   }

   @Override
   public void attributeOwnershipReleaseDenied(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      if (federationState == null || federationState.members.size() <= 1) {
         return;
      }
      for (AttributeHandle attribute : attributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.owner != this || ownership.pendingAcquirer == null) {
            throw new IllegalArgumentException("no pending ownership acquisition");
         }
         MockRTIambassador acquirer = ownership.pendingAcquirer;
         MockAttributeHandleSet copiedAttributes = singletonAttributeSet(attribute);
         byte[] copiedTag = userSuppliedTag.clone();
         ownership.pendingAcquirer = null;
         ownership.pendingAcquisitionTag = null;
         ownership.divestitureOffered = false;
         acquirer.deliverCallback(() -> acquirer.federateAmbassador.attributeOwnershipUnavailable(
            objectInstance, copiedAttributes, copiedTag));
      }
   }

   @Override
   public AttributeHandleSet attributeOwnershipDivestitureIfWanted(
      ObjectInstanceHandle objectInstance,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      validateAttributeSet(attributes);
      if (federationState == null || federationState.members.size() <= 1) {
         return attributes;
      }
      MockAttributeHandleSet wanted = new MockAttributeHandleSet();
      for (AttributeHandle attribute : attributes) {
         MockOwnershipRecord ownership = ownershipRecord(objectInstanceName, attribute);
         if (ownership.owner != this) {
            throw new IllegalArgumentException("attribute is not owned by this federate");
         }
         if (ownership.pendingAcquirer != null) {
            wanted.add(attribute);
            transferOwnership(objectInstance, attribute, ownership, userSuppliedTag);
         }
      }
      return wanted;
   }

   @Override
   public void sendInteraction(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag) throws NotConnected
   {
      requireJoined();
      handlePayload(interactionClass, "interaction");
      MockParameterHandleValueMap copiedValues = new MockParameterHandleValueMap();
      for (Map.Entry<ParameterHandle, byte[]> entry : parameterValues.entrySet()) {
         handlePayload(entry.getKey(), "parameter");
         copiedValues.put(entry.getKey(), entry.getValue().clone());
      }
      byte[] tag = userSuppliedTag.clone();
      if (federationState != null && federationState.members.size() > 1) {
         String interactionName = handlePayload(interactionClass, "interaction");
         for (MockRTIambassador member : federationState.members) {
            if (member == this || !member.isActiveInteractionSubscription(interactionName)) {
               continue;
            }
            member.deliverCallback(() -> member.federateAmbassador.receiveInteraction(
               supportHandle("interaction", interactionName),
               copiedValues,
               tag,
               supportHandle("transportation", "HLAreliable"),
               new MockFederateHandle(
                  federationState.federationName + ":" + federateName),
               null));
         }
         return;
      }
      deliverCallback(() -> federateAmbassador.receiveInteraction(
         interactionClass,
         copiedValues,
         tag,
         supportHandle("transportation", "HLAreliable"),
         new MockFederateHandle("mock-producing-federate"),
         null));
   }

   @Override
   public void sendDirectedInteraction(
      InteractionClassHandle interactionClass,
      ObjectInstanceHandle objectInstance,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag) throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      MockParameterHandleValueMap copiedValues = copyParameters(parameterValues);
      byte[] tag = userSuppliedTag.clone();
      if (federationState != null && federationState.members.size() > 1) {
         MockObjectRecord record = federationState.objectInstances.get(objectInstanceName);
         if (record != null) {
            String directedKey = directedInteractionKey(record.objectClassName, interactionName);
            for (MockRTIambassador member : federationState.members) {
               if (member == this || !member.subscribedDirectedInteractions.contains(directedKey)) {
                  continue;
               }
               member.deliverCallback(() -> member.federateAmbassador.receiveDirectedInteraction(
                  supportHandle("interaction", interactionName),
                  record.handle,
                  copiedValues,
                  tag,
                  supportHandle("transportation", "HLAreliable"),
                  new MockFederateHandle(
                     federationState.federationName + ":" + federateName)));
            }
            return;
         }
      }
      deliverCallback(() -> federateAmbassador.receiveDirectedInteraction(
         interactionClass,
         objectInstance,
         copiedValues,
         tag,
         supportHandle("transportation", "HLAreliable"),
         new MockFederateHandle("mock-producing-federate")));
   }

   @Override
   public MessageRetractionHandle sendDirectedInteraction(
      InteractionClassHandle interactionClass,
      ObjectInstanceHandle objectInstance,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      LogicalTime time) throws NotConnected {
      requireJoined();
      String interactionName = handlePayload(interactionClass, "interaction");
      String objectInstanceName = handlePayload(objectInstance, "object-instance");
      MockParameterHandleValueMap copiedValues = copyParameters(parameterValues);
      byte[] tag = userSuppliedTag.clone();
      if (federationState != null && federationState.members.size() > 1) {
         MockObjectRecord record = federationState.objectInstances.get(objectInstanceName);
         if (record != null) {
            String directedKey = directedInteractionKey(record.objectClassName, interactionName);
            MockSupportHandle sharedRetraction = nextSharedRetraction();
            federationState.timedRetractions.put(
               sharedRetraction, new MockRetractionRecord(sharedRetraction));
            for (MockRTIambassador member : federationState.members) {
               if (member == this || !member.subscribedDirectedInteractions.contains(directedKey)) {
                  continue;
               }
               LogicalTime deliveryTime = copyLogicalTime(time);
               OrderType receivedOrder = member.timeConstrained
                  ? OrderType.TIMESTAMP
                  : OrderType.RECEIVE;
               member.routeTimedDelivery(
                  sharedRetraction,
                  deliveryTime,
                  () -> member.federateAmbassador.receiveDirectedInteraction(
                     supportHandle("interaction", interactionName),
                     record.handle,
                     copiedValues,
                     tag,
                     supportHandle("transportation", "HLAreliable"),
                     new MockFederateHandle(
                        federationState.federationName + ":" + federateName),
                     deliveryTime,
                     OrderType.TIMESTAMP,
                     receivedOrder,
                     sharedRetraction));
            }
            return sharedRetraction;
         }
      }
      MockSupportHandle retraction = supportHandle("retraction", Integer.toString(nextRetraction++));
      deliverCallback(() -> federateAmbassador.receiveDirectedInteraction(
         interactionClass,
         objectInstance,
         copiedValues,
         tag,
         supportHandle("transportation", "HLAreliable"),
         new MockFederateHandle("mock-producing-federate"),
         time,
         OrderType.TIMESTAMP,
         OrderType.TIMESTAMP,
         retraction));
      return retraction;
   }

   @Override
   public void sendInteractionWithRegions(
      InteractionClassHandle interactionClass,
      ParameterHandleValueMap parameterValues,
      RegionHandleSet regions,
      byte[] userSuppliedTag) throws NotConnected
   {
      requireJoined();
      handlePayload(interactionClass, "interaction");
      MockParameterHandleValueMap copiedValues = new MockParameterHandleValueMap();
      for (Map.Entry<ParameterHandle, byte[]> entry : parameterValues.entrySet()) {
         handlePayload(entry.getKey(), "parameter");
         copiedValues.put(entry.getKey(), entry.getValue().clone());
      }
      MockRegionHandleSet copiedRegions = new MockRegionHandleSet();
      Set<MockSupportHandle> sourceRegions = new HashSet<>();
      for (RegionHandle region : regions) {
         handlePayload(region, "region");
         MockSupportHandle sourceRegion = (MockSupportHandle) region;
         copiedRegions.add(sourceRegion);
         sourceRegions.add(sourceRegion);
      }
      byte[] tag = userSuppliedTag.clone();
      if (federationState != null && federationState.members.size() > 1) {
         String interactionName = handlePayload(interactionClass, "interaction");
         boolean delayed = federationState.delaySubscriptionEvaluation;
         for (MockRTIambassador member : federationState.members) {
            if (member == this) {
               continue;
            }
            Set<MockSupportHandle> subscribedRegions =
               member.subscribedInteractionRegions.get(interactionName);
            if (!delayed && (!member.isActiveInteractionSubscription(interactionName)
                  || !regionsOverlap(this, sourceRegions, member, subscribedRegions))) {
               continue;
            }
            RegionHandleSet optionalRegions = member.conveyRegionDesignatorSets
               ? copyRegionSet(copiedRegions)
               : null;
            member.deliverCallback(() -> {
               if (delayed && (!member.isActiveInteractionSubscription(interactionName)
                     || !regionsOverlap(this, sourceRegions, member,
                        member.subscribedInteractionRegions.get(interactionName)))) {
                  return;
               }
               member.federateAmbassador.receiveInteraction(
                  supportHandle("interaction", interactionName),
                  copiedValues,
                  tag,
                  supportHandle("transportation", "HLAreliable"),
                  new MockFederateHandle(
                     federationState.federationName + ":" + federateName),
                  optionalRegions);
            });
         }
         return;
      }
      RegionHandleSet optionalRegions = conveyRegionDesignatorSets ? copiedRegions : null;
      deliverCallback(() -> federateAmbassador.receiveInteraction(
         interactionClass,
         copiedValues,
         tag,
         supportHandle("transportation", "HLAreliable"),
         new MockFederateHandle("mock-producing-federate"),
         optionalRegions));
   }

   @Override
   public RegionHandle createRegion(DimensionHandleSet dimensions) throws NotConnected {
      requireJoined();
      MockSupportHandle region = supportHandle("region", Integer.toString(nextRegion++));
      Set<MockSupportHandle> copied = new HashSet<>();
      for (hla.rti1516_2025.DimensionHandle dimension : dimensions) {
         copied.add((MockSupportHandle) dimension);
      }
      regionDimensions.put(region, copied);
      return region;
   }

   @Override
   public void commitRegionModifications(RegionHandleSet regions) throws NotConnected {
      requireJoined();
      for (RegionHandle region : regions) {
         if (!regionDimensions.containsKey((MockSupportHandle) region)) {
            throw new IllegalArgumentException("region is not known");
         }
      }
   }

   @Override
   public void deleteRegion(RegionHandle region) throws NotConnected {
      requireJoined();
      MockSupportHandle known = (MockSupportHandle) region;
      if (regionDimensions.remove(known) == null) {
         throw new IllegalArgumentException("region is not known");
      }
      regionBounds.keySet().removeIf(key -> key.startsWith(handlePayload(known, "region") + ":"));
   }

   @Override
   public DimensionHandleSet getDimensionHandleSet(RegionHandle region) throws NotConnected {
      requireJoined();
      Set<MockSupportHandle> dimensions = regionDimensions.get((MockSupportHandle) region);
      if (dimensions == null) {
         throw new IllegalArgumentException("region is not known");
      }
      MockDimensionHandleSet result = new MockDimensionHandleSet();
      for (MockSupportHandle dimension : dimensions) {
         result.add(dimension);
      }
      return result;
   }

   @Override
   public RangeBounds getRangeBounds(RegionHandle region, DimensionHandle dimension)
      throws NotConnected {
      requireJoined();
      MockSupportHandle knownRegion = (MockSupportHandle) region;
      Set<MockSupportHandle> dimensions = regionDimensions.get(knownRegion);
      MockSupportHandle knownDimension = (MockSupportHandle) dimension;
      if (dimensions == null || !dimensions.contains(knownDimension)) {
         throw new IllegalArgumentException("dimension is not in region");
      }
      return regionBounds.getOrDefault(
         rangeKey(knownRegion, knownDimension), new RangeBounds(0, 0));
   }

   @Override
   public void setRangeBounds(
      RegionHandle region, DimensionHandle dimension, RangeBounds rangeBounds)
      throws NotConnected {
      requireJoined();
      MockSupportHandle knownRegion = (MockSupportHandle) region;
      MockSupportHandle knownDimension = (MockSupportHandle) dimension;
      Set<MockSupportHandle> dimensions = regionDimensions.get(knownRegion);
      if (dimensions == null || !dimensions.contains(knownDimension)) {
         throw new IllegalArgumentException("dimension is not in region");
      }
      regionBounds.put(
         rangeKey(knownRegion, knownDimension),
         new RangeBounds(rangeBounds.getLowerBound(), rangeBounds.getUpperBound()));
   }

   @Override
   public boolean getConveyRegionDesignatorSetsSwitch() throws NotConnected {
      requireJoined();
      return conveyRegionDesignatorSets;
   }

   @Override
   public void setConveyRegionDesignatorSetsSwitch(boolean switchValue) throws NotConnected {
      requireJoined();
      conveyRegionDesignatorSets = switchValue;
   }

   @Override
   public boolean getObjectClassRelevanceAdvisorySwitch() throws NotConnected {
      requireJoined();
      return objectClassRelevanceAdvisory;
   }

   @Override
   public void setObjectClassRelevanceAdvisorySwitch(boolean switchValue) throws NotConnected {
      requireJoined();
      objectClassRelevanceAdvisory = switchValue;
   }

   @Override
   public boolean getAttributeRelevanceAdvisorySwitch() throws NotConnected {
      requireJoined();
      return attributeRelevanceAdvisory;
   }

   @Override
   public void setAttributeRelevanceAdvisorySwitch(boolean switchValue) throws NotConnected {
      requireJoined();
      attributeRelevanceAdvisory = switchValue;
   }

   @Override
   public boolean getAttributeScopeAdvisorySwitch() throws NotConnected {
      requireJoined();
      return attributeScopeAdvisory;
   }

   @Override
   public void setAttributeScopeAdvisorySwitch(boolean switchValue) throws NotConnected {
      requireJoined();
      attributeScopeAdvisory = switchValue;
   }

   @Override
   public boolean getInteractionRelevanceAdvisorySwitch() throws NotConnected {
      requireJoined();
      return interactionRelevanceAdvisory;
   }

   @Override
   public void setInteractionRelevanceAdvisorySwitch(boolean switchValue) throws NotConnected {
      requireJoined();
      interactionRelevanceAdvisory = switchValue;
   }

   @Override
   public ResignAction getAutomaticResignDirective() throws NotConnected {
      requireJoined();
      return automaticResignDirective;
   }

   @Override
   public void setAutomaticResignDirective(ResignAction resignAction) throws NotConnected {
      requireJoined();
      automaticResignDirective = resignAction;
   }

   @Override
   public boolean getServiceReportingSwitch() throws NotConnected {
      requireJoined();
      return serviceReporting;
   }

   @Override
   public void setServiceReportingSwitch(boolean switchValue) throws NotConnected {
      requireJoined();
      serviceReporting = switchValue;
   }

   @Override
   public boolean getExceptionReportingSwitch() throws NotConnected {
      requireJoined();
      return exceptionReporting;
   }

   @Override
   public void setExceptionReportingSwitch(boolean switchValue) throws NotConnected {
      requireJoined();
      exceptionReporting = switchValue;
   }

   @Override
   public boolean getSendServiceReportsToFileSwitch() throws NotConnected {
      requireJoined();
      return sendServiceReportsToFile;
   }

   @Override
   public void setSendServiceReportsToFileSwitch(boolean enabled) throws NotConnected {
      requireJoined();
      sendServiceReportsToFile = enabled;
   }

   @Override
   public String getHLAversion() {
      return "IEEE 1516.1-2025";
   }

   @Override
   public boolean getAutoProvideSwitch() throws NotConnected {
      requireJoined();
      return false;
   }

   @Override
   public boolean getDelaySubscriptionEvaluationSwitch() throws NotConnected {
      requireJoined();
      return federationState != null && federationState.delaySubscriptionEvaluation;
   }

   @Override
   public boolean getAdvisoriesUseKnownClassSwitch() throws NotConnected {
      requireJoined();
      return false;
   }

   @Override
   public boolean getAllowRelaxedDDMSwitch() throws NotConnected {
      requireJoined();
      return federationState != null && federationState.allowRelaxedDDM;
   }

   @Override
   public boolean getNonRegulatedGrantSwitch() throws NotConnected {
      requireJoined();
      return false;
   }

   @Override
   public LogicalTimeFactory getTimeFactory() throws NotConnected {
      requireJoined();
      return timeFactory;
   }

   @Override
   public void enableTimeRegulation(LogicalTimeInterval lookahead) throws NotConnected {
      requireJoined();
      if (lookahead == null) {
         throw new IllegalArgumentException("lookahead is required");
      }
      timeRegulating = true;
      currentLookahead = lookahead;
      pendingLookahead = null;
      deliverCallback(() -> federateAmbassador.timeRegulationEnabled(currentLogicalTime));
   }

   @Override
   public void disableTimeRegulation() throws NotConnected {
      requireJoined();
      timeRegulating = false;
      pendingLookahead = null;
   }

   @Override
   public void enableTimeConstrained() throws NotConnected {
      requireJoined();
      timeConstrained = true;
      deliverCallback(() -> federateAmbassador.timeConstrainedEnabled(currentLogicalTime));
   }

   @Override
   public void disableTimeConstrained() throws NotConnected {
      requireJoined();
      timeConstrained = false;
   }

   @Override
   public void enableAsynchronousDelivery() throws NotConnected {
      requireJoined();
      asynchronousDelivery = true;
   }

   @Override
   public void disableAsynchronousDelivery() throws NotConnected {
      requireJoined();
      asynchronousDelivery = false;
   }

   @Override
   public void modifyLookahead(LogicalTimeInterval lookahead) throws NotConnected {
      requireJoined();
      if (!timeFactory.getName().equals(lookahead.implementationName())) {
         throw new IllegalArgumentException("lookahead uses the wrong logical-time implementation");
      }
      if (logicalIntervalValue(lookahead) < logicalIntervalValue(currentLookahead)) {
         pendingLookahead = copyLogicalTimeInterval(lookahead);
      } else {
         currentLookahead = lookahead;
         pendingLookahead = null;
      }
   }

   @Override
   public LogicalTimeInterval queryLookahead() throws NotConnected {
      requireJoined();
      return currentLookahead;
   }

   @Override
   public void timeAdvanceRequest(LogicalTime time) throws NotConnected {
      advanceTo(time);
   }

   @Override
   public void timeAdvanceRequestAvailable(LogicalTime time) throws NotConnected {
      advanceTo(time);
   }

   @Override
   public void nextMessageRequest(LogicalTime time) throws NotConnected {
      advanceTo(time);
   }

   @Override
   public void nextMessageRequestAvailable(LogicalTime time) throws NotConnected {
      advanceTo(time);
   }

   @Override
   public void flushQueueRequest(LogicalTime time) throws NotConnected {
      advanceTo(time);
   }

   private void advanceTo(LogicalTime time) throws NotConnected {
      requireJoined();
      if (!timeFactory.getName().equals(time.implementationName())) {
         throw new IllegalArgumentException("time uses the wrong logical-time implementation");
      }
      LogicalTime previousLogicalTime = currentLogicalTime;
      currentLogicalTime = time;
      applyPendingLookahead(previousLogicalTime);
      flushPendingTimedDeliveries();
      if (pendingTimestampedAttributeTime != null
         && logicalTimeValue(currentLogicalTime)
            >= logicalTimeValue(pendingTimestampedAttributeTime)) {
         ObjectInstanceHandle objectInstance = pendingTimestampedAttributeObject;
         AttributeHandleValueMap attributeValues = pendingTimestampedAttributeValues;
         byte[] userSuppliedTag = pendingTimestampedAttributeTag;
         LogicalTime attributeTime = pendingTimestampedAttributeTime;
         pendingTimestampedAttributeObject = null;
         pendingTimestampedAttributeValues = null;
         pendingTimestampedAttributeTag = null;
         pendingTimestampedAttributeTime = null;
         timestampedRegionalAttributeProbe = false;
         MockRegionHandleSet sentRegions = new MockRegionHandleSet();
         sentRegions.add(supportHandle("region", "regional-attribute"));
         deliverCallback(() -> federateAmbassador.reflectAttributeValues(
            objectInstance,
            attributeValues,
            userSuppliedTag,
            supportHandle("transportation", "HLAreliable"),
            new MockFederateHandle("mock-producing-federate"),
            sentRegions,
            attributeTime,
            OrderType.TIMESTAMP,
            OrderType.TIMESTAMP,
            supportHandle("retraction", "regional-attribute")));
      }
      if (federationState != null && federationState.members.size() > 1
         && federationState.savePending && federationState.saveTimeValue != null
         && logicalTimeValue(currentLogicalTime) >= federationState.saveTimeValue) {
         beginSharedFederationSave();
      }
      if (pendingSaveTime != null
         && logicalTimeValue(currentLogicalTime) >= logicalTimeValue(pendingSaveTime)) {
         String label = pendingSaveLabel;
         LogicalTime saveTime = pendingSaveTime;
         pendingSaveLabel = null;
         pendingSaveTime = null;
         if (pendingTimestampedInteractionTime != null
            && logicalTimeValue(currentLogicalTime)
               >= logicalTimeValue(pendingTimestampedInteractionTime)) {
            InteractionClassHandle interactionClass = pendingTimestampedInteractionClass;
            ParameterHandleValueMap parameterValues = pendingTimestampedInteractionParameters;
            byte[] userSuppliedTag = pendingTimestampedInteractionTag;
            LogicalTime interactionTime = pendingTimestampedInteractionTime;
            pendingTimestampedInteractionClass = null;
            pendingTimestampedInteractionParameters = null;
            pendingTimestampedInteractionTag = null;
            pendingTimestampedInteractionTime = null;
            deliverCallback(() -> federateAmbassador.receiveInteraction(
               interactionClass,
               parameterValues,
               userSuppliedTag,
               supportHandle("transportation", "HLAreliable"),
               new MockFederateHandle("mock-producing-federate"),
               null,
               interactionTime,
               OrderType.TIMESTAMP,
               OrderType.TIMESTAMP,
               supportHandle("retraction", "tso-save-boundary")));
         }
         beginFederationSave(label, saveTime);
      }
      LogicalTime grantTime = currentLogicalTime;
      deliverCallback(() -> federateAmbassador.timeAdvanceGrant(grantTime));
   }

   private static double logicalTimeValue(LogicalTime time) {
      if (time instanceof HLAinteger64Time) {
         return ((HLAinteger64Time) time).getTime();
      }
      if (time instanceof HLAfloat64Time) {
         return ((HLAfloat64Time) time).getTime();
      }
      throw new IllegalArgumentException("fixture requires a supported logical-time value");
   }

   private static double logicalIntervalValue(LogicalTimeInterval interval) {
      if (interval instanceof HLAinteger64Interval) {
         return ((HLAinteger64Interval) interval).getInterval();
      }
      if (interval instanceof HLAfloat64Interval) {
         return ((HLAfloat64Interval) interval).getInterval();
      }
      throw new IllegalArgumentException("fixture requires a supported logical-time interval");
   }

   private LogicalTimeInterval makeLogicalTimeInterval(double value) {
      if (timeFactory instanceof HLAinteger64TimeFactory) {
         return ((HLAinteger64TimeFactory) timeFactory).makeLogicalTimeInterval(
            Math.round(value));
      }
      return ((HLAfloat64TimeFactory) timeFactory).makeLogicalTimeInterval(value);
   }

   private void applyPendingLookahead(LogicalTime previousLogicalTime) {
      if (pendingLookahead == null) {
         return;
      }
      double elapsed = logicalTimeValue(currentLogicalTime) - logicalTimeValue(previousLogicalTime);
      if (elapsed <= 0.0) {
         return;
      }
      double target = logicalIntervalValue(pendingLookahead);
      double candidate = logicalIntervalValue(currentLookahead) - elapsed;
      double applied = Math.max(target, candidate);
      currentLookahead = makeLogicalTimeInterval(applied);
      if (applied <= target) {
         pendingLookahead = null;
      }
   }

   private MockSupportHandle nextSharedRetraction() {
      String owner = federateName == null ? "unknown" : federateName;
      return supportHandle(
         "retraction",
         federationState.federationName + ":" + owner + ":" + nextRetraction++);
   }

   private void routeTimedDelivery(
      MockSupportHandle retraction, LogicalTime time, Runnable callback) {
      MockRetractionRecord record = federationState.timedRetractions.get(retraction);
      if (record == null || record.retracted) {
         return;
      }
      if (!timeConstrained || logicalTimeValue(currentLogicalTime) >= logicalTimeValue(time)) {
         record.deliveredRecipients.add(this);
         deliverCallback(callback);
      } else {
         record.pendingRecipients.add(this);
         pendingTimedDeliveries.add(new MockTimedDelivery(retraction, time, callback));
      }
   }

   private void flushPendingTimedDeliveries() {
      if (pendingTimedDeliveries.isEmpty() || federationState == null) {
         return;
      }
      List<MockTimedDelivery> ready = new ArrayList<>();
      for (MockTimedDelivery delivery : pendingTimedDeliveries) {
         if (logicalTimeValue(currentLogicalTime) >= logicalTimeValue(delivery.time)) {
            ready.add(delivery);
         }
      }
      if (ready.isEmpty()) {
         return;
      }
      pendingTimedDeliveries.removeAll(ready);
      for (MockTimedDelivery delivery : ready) {
         MockRetractionRecord record = federationState.timedRetractions.get(delivery.retraction);
         if (record == null || record.retracted) {
            continue;
         }
         record.pendingRecipients.remove(this);
         record.deliveredRecipients.add(this);
         deliverCallback(delivery.callback);
      }
   }

   private LogicalTime copyLogicalTime(LogicalTime value) {
      byte[] encoded = new byte[value.encodedLength()];
      value.encode(encoded, 0);
      try {
         return timeFactory.decodeLogicalTime(encoded, 0);
      } catch (RTIexception error) {
         throw new IllegalStateException("Fixture logical-time copy failed", error);
      }
   }

   private LogicalTimeInterval copyLogicalTimeInterval(LogicalTimeInterval value) {
      if (value == null) {
         return null;
      }
      byte[] encoded = new byte[value.encodedLength()];
      value.encode(encoded, 0);
      try {
         return timeFactory.decodeLogicalTimeInterval(encoded, 0);
      } catch (RTIexception error) {
         throw new IllegalStateException("Fixture logical-time interval copy failed", error);
      }
   }

   @Override
   public LogicalTime queryLogicalTime() throws NotConnected {
      requireJoined();
      return currentLogicalTime;
   }

   @Override
   public TimeQueryReturn queryGALT() throws NotConnected {
      requireJoined();
      return new TimeQueryReturn(true, currentLogicalTime);
   }

   @Override
   public TimeQueryReturn queryLITS() throws NotConnected {
      requireJoined();
      return new TimeQueryReturn(true, currentLogicalTime);
   }

   private static String rangeKey(MockSupportHandle region, MockSupportHandle dimension) {
      return handlePayload(region, "region") + ":" + handlePayload(dimension, "dimension");
   }

   private void validateAttributeRegionPairs(AttributeSetRegionSetPairList pairs) {
      for (AttributeSetRegionSetPair pair : pairs) {
         for (AttributeHandle attribute : pair.getAttributes()) {
            handlePayload(attribute, "attribute");
         }
         for (RegionHandle region : pair.getRegions()) {
            handlePayload(region, "region");
         }
      }
   }

   private Map<String, Set<MockSupportHandle>> regionMapFromPairs(
      AttributeSetRegionSetPairList pairs) {
      Map<String, Set<MockSupportHandle>> regionsByName = new LinkedHashMap<>();
      for (AttributeSetRegionSetPair pair : pairs) {
         Set<MockSupportHandle> regions = new HashSet<>();
         for (RegionHandle region : pair.getRegions()) {
            handlePayload(region, "region");
            regions.add((MockSupportHandle) region);
         }
         for (AttributeHandle attribute : pair.getAttributes()) {
            String attributeName = handlePayload(attribute, "attribute");
            Set<MockSupportHandle> associated = regionsByName.get(attributeName);
            if (associated == null) {
               associated = new HashSet<>();
               regionsByName.put(attributeName, associated);
            }
            associated.addAll(regions);
         }
      }
      return regionsByName;
   }

   private void mergeAttributeRegions(
      Map<String, Set<MockSupportHandle>> regionsByName,
      AttributeSetRegionSetPairList pairs,
      boolean remove) {
      for (AttributeSetRegionSetPair pair : pairs) {
         Set<MockSupportHandle> regions = new HashSet<>();
         for (RegionHandle region : pair.getRegions()) {
            handlePayload(region, "region");
            regions.add((MockSupportHandle) region);
         }
         for (AttributeHandle attribute : pair.getAttributes()) {
            String attributeName = handlePayload(attribute, "attribute");
            if (remove) {
               Set<MockSupportHandle> associated = regionsByName.get(attributeName);
               if (associated != null) {
                  associated.removeAll(regions);
                  if (associated.isEmpty()) {
                     regionsByName.remove(attributeName);
                  }
               }
            } else {
               Set<MockSupportHandle> associated = regionsByName.get(attributeName);
               if (associated == null) {
                  associated = new HashSet<>();
                  regionsByName.put(attributeName, associated);
               }
               associated.addAll(regions);
            }
         }
      }
   }

   private void validateAttributeSet(AttributeHandleSet attributes) {
      for (AttributeHandle attribute : attributes) {
         handlePayload(attribute, "attribute");
      }
   }

   private void validateInteractionClassSet(Set<InteractionClassHandle> interactionClasses) {
      for (InteractionClassHandle interactionClass : interactionClasses) {
         handlePayload(interactionClass, "interaction");
      }
   }

   private MockParameterHandleValueMap copyParameters(ParameterHandleValueMap parameterValues) {
      MockParameterHandleValueMap copiedValues = new MockParameterHandleValueMap();
      for (Map.Entry<ParameterHandle, byte[]> entry : parameterValues.entrySet()) {
         handlePayload(entry.getKey(), "parameter");
         copiedValues.put(entry.getKey(), entry.getValue().clone());
      }
      return copiedValues;
   }

   private MockRegionHandleSet copyRegionSet(RegionHandleSet regions) {
      MockRegionHandleSet copiedRegions = new MockRegionHandleSet();
      for (RegionHandle region : regions) {
         handlePayload(region, "region");
         copiedRegions.add((MockSupportHandle) region);
      }
      return copiedRegions;
   }

   private MockRegionHandleSet copyRegionSet(Set<MockSupportHandle> regions) {
      MockRegionHandleSet copiedRegions = new MockRegionHandleSet();
      for (MockSupportHandle region : regions) {
         copiedRegions.add(region);
      }
      return copiedRegions;
   }

   private void deliverCallback(Runnable callback) {
      if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
         callback.run();
      } else {
         queuedCallbacks.add(callback);
      }
   }

   private void leaveSharedFederation() {
      pendingTimedDeliveries.clear();
      if (federationState != null) {
         federationState.saveBegunMembers.remove(this);
         federationState.saveCompletedMembers.remove(this);
         federationState.restoreCompletedMembers.remove(this);
         federationState.members.remove(this);
      }
      federationState = null;
      joined = false;
      federateName = null;
      publishedObjectAttributes.clear();
      subscribedObjectAttributes.clear();
      passiveObjectSubscriptions.clear();
      subscribedObjectRegions.clear();
      knownObjectInstances.clear();
      publishedInteractions.clear();
      subscribedInteractions.clear();
      passiveInteractionSubscriptions.clear();
      publishedDirectedInteractions.clear();
      subscribedDirectedInteractions.clear();
      subscribedInteractionRegions.clear();
   }

   private boolean subscribesToObjectClass(String objectClassName) {
      String prefix = objectClassName + "\u001F";
      for (String key : subscribedObjectAttributes) {
         if (key.startsWith(prefix) && !passiveObjectSubscriptions.contains(key)) {
            return true;
         }
      }
      return false;
   }

   private void queueDiscoveriesForCurrentSubscriptions() {
      if (federationState == null) {
         return;
      }
      for (MockObjectRecord record : federationState.objectInstances.values()) {
         if (record.owner == this
            || !subscribesToObjectClass(record.objectClassName)
            || !isRegionRelevantObject(record)
            || !knownObjectInstances.add(record.objectInstanceName)) {
            continue;
         }
         ObjectClassHandle memberObjectClass = supportHandle("object", record.objectClassName);
         deliverCallback(() -> federateAmbassador.discoverObjectInstance(
            record.handle,
            memberObjectClass,
            record.objectInstanceName,
            new MockFederateHandle(
               federationState.federationName + ":" + record.owner.federateName)));
      }
   }

   private Map<String, Map<String, Boolean>> captureScopeStates(Set<String> keys) {
      Map<String, Map<String, Boolean>> states = new LinkedHashMap<>();
      if (federationState == null) {
         return states;
      }
      for (String key : keys) {
         Map<String, Boolean> perObject = new LinkedHashMap<>();
         for (MockObjectRecord record : federationState.objectInstances.values()) {
            if (record.owner != this && knownObjectInstances.contains(record.objectInstanceName)) {
               perObject.put(record.objectInstanceName, isAttributeInScope(record, key));
            }
         }
         states.put(key, perObject);
      }
      return states;
   }

   private void queueScopeChanges(Map<String, Map<String, Boolean>> scopeBefore) {
      if (!attributeScopeAdvisory || federationState == null) {
         return;
      }
      for (Map.Entry<String, Map<String, Boolean>> entry : scopeBefore.entrySet()) {
         String key = entry.getKey();
         String separator = "\u001F";
         int separatorIndex = key.indexOf(separator);
         String attributeName = separatorIndex < 0
            ? key
            : key.substring(separatorIndex + separator.length());
         for (MockObjectRecord record : federationState.objectInstances.values()) {
            Boolean before = entry.getValue().get(record.objectInstanceName);
            if (record.owner == this || before == null) {
               continue;
            }
            boolean after = isAttributeInScope(record, key);
            if (before == after) {
               continue;
            }
            MockAttributeHandleSet attributes = new MockAttributeHandleSet();
            attributes.add(supportHandle("attribute", attributeName));
            if (after) {
               deliverCallback(() -> federateAmbassador.attributesInScope(record.handle, attributes));
            } else {
               deliverCallback(() -> federateAmbassador.attributesOutOfScope(record.handle, attributes));
            }
         }
      }
   }

   private boolean isAttributeInScope(MockObjectRecord record, String key) {
      if (!subscribedObjectAttributes.contains(key)
         || passiveObjectSubscriptions.contains(key)) {
         return false;
      }
      Set<MockSupportHandle> subscribedRegions = subscribedObjectRegions.get(key);
      if (subscribedRegions == null || subscribedRegions.isEmpty()) {
         return true;
      }
      int separatorIndex = key.indexOf("\u001F");
      String attributeName = separatorIndex < 0
         ? key
         : key.substring(separatorIndex + 1);
      Set<MockSupportHandle> sourceRegions = record.attributeRegionsByName.get(attributeName);
      return regionsOverlap(
         record.owner,
         sourceRegions == null ? new HashSet<>() : sourceRegions,
         this,
         subscribedRegions);
   }

   private boolean isRegionRelevantObject(MockObjectRecord record) {
      String prefix = record.objectClassName + "\u001F";
      boolean hasSubscription = false;
      for (Map.Entry<String, Set<MockSupportHandle>> entry : subscribedObjectRegions.entrySet()) {
         if (!entry.getKey().startsWith(prefix)) {
            continue;
         }
         hasSubscription = true;
         if (entry.getValue().isEmpty()) {
            return true;
         }
         String attributeName = entry.getKey().substring(prefix.length());
         Set<MockSupportHandle> sourceRegions =
            record.attributeRegionsByName.get(attributeName);
         if (regionsOverlap(record.owner, sourceRegions == null
               ? new HashSet<>()
               : sourceRegions,
            this, entry.getValue())) {
            return true;
         }
      }
      return !hasSubscription;
   }

   private MockAttributeHandleValueMap filterSubscribedAttributes(
      String objectClassName, AttributeHandleValueMap values) {
      MockAttributeHandleValueMap filtered = new MockAttributeHandleValueMap();
      for (Map.Entry<AttributeHandle, byte[]> entry : values.entrySet()) {
         String attributeName = handlePayload(entry.getKey(), "attribute");
         String key = objectAttributeKey(objectClassName, attributeName);
         if (subscribedObjectAttributes.contains(key)
            && !passiveObjectSubscriptions.contains(key)) {
            filtered.put(entry.getKey(), entry.getValue().clone());
         }
      }
      return filtered;
   }

   private boolean isActiveInteractionSubscription(String interactionName) {
      return subscribedInteractions.contains(interactionName)
         && !passiveInteractionSubscriptions.contains(interactionName);
   }

   private static MockAttributeHandleValueMap filterRegionRelevantAttributes(
      MockRTIambassador source,
      MockObjectRecord record,
      MockRTIambassador member,
      AttributeHandleValueMap values) {
      MockAttributeHandleValueMap filtered = new MockAttributeHandleValueMap();
      for (Map.Entry<AttributeHandle, byte[]> entry : values.entrySet()) {
         String key = objectAttributeKey(
            record.objectClassName, handlePayload(entry.getKey(), "attribute"));
         Set<MockSupportHandle> subscribedRegions = member.subscribedObjectRegions.get(key);
         if (subscribedRegions == null
            || regionsOverlap(source, record.regionsForAttribute(entry.getKey()),
               member, subscribedRegions)) {
            filtered.put(entry.getKey(), entry.getValue().clone());
         }
      }
      return filtered;
   }

   /**
    * Preserve the default-region realization in conveyed object callbacks.
    * An empty optional set means the update used the RTI-provided default
    * region; null remains the ordinary non-regional callback form.
    */
   private static RegionHandleSet objectUpdateRegionDesignator(
      MockRTIambassador member,
      MockObjectRecord record,
      AttributeHandleValueMap values) {
      if (!member.conveyRegionDesignatorSets) {
         return null;
      }
      Set<MockSupportHandle> associatedRegions = record.regionsForValues(values);
      if (!associatedRegions.isEmpty()) {
         return copyRegionSetStatic(associatedRegions);
      }
      for (AttributeHandle attribute : values.keySet()) {
         String key = objectAttributeKey(
            record.objectClassName, handlePayload(attribute, "attribute"));
         Set<MockSupportHandle> subscribedRegions = member.subscribedObjectRegions.get(key);
         if (subscribedRegions != null && !subscribedRegions.isEmpty()) {
            return new MockRegionHandleSet();
         }
      }
      return null;
   }

  private static MockRegionHandleSet copyRegionSetStatic(Set<MockSupportHandle> regions) {
      MockRegionHandleSet copiedRegions = new MockRegionHandleSet();
      for (MockSupportHandle region : regions) {
         copiedRegions.add(region);
      }
      return copiedRegions;
  }

   private static boolean regionsOverlap(
      MockRTIambassador source,
      Set<MockSupportHandle> sourceRegions,
      MockRTIambassador target,
      Set<MockSupportHandle> targetRegions) {
      if (sourceRegions.isEmpty() || targetRegions == null || targetRegions.isEmpty()) {
         return true;
      }
      for (MockSupportHandle sourceRegion : sourceRegions) {
         Set<MockSupportHandle> sourceDimensions = source.regionDimensions.get(sourceRegion);
         if (sourceDimensions == null) {
            return true;
         }
         for (MockSupportHandle targetRegion : targetRegions) {
            Set<MockSupportHandle> targetDimensions = target.regionDimensions.get(targetRegion);
            if (targetDimensions == null) {
               return true;
            }
            for (MockSupportHandle dimension : sourceDimensions) {
               if (!targetDimensions.contains(dimension)) {
                  continue;
               }
               RangeBounds sourceBounds = source.regionBounds.getOrDefault(
                  rangeKey(sourceRegion, dimension), new RangeBounds(0, 0));
               RangeBounds targetBounds = target.regionBounds.getOrDefault(
                  rangeKey(targetRegion, dimension), new RangeBounds(0, 0));
               boolean strictOverlap = sourceBounds.getLowerBound() < targetBounds.getUpperBound()
                  && targetBounds.getLowerBound() < sourceBounds.getUpperBound();
               boolean relaxedOverlap = (source.federationState != null
                  && source.federationState.allowRelaxedDDM)
                  || (target.federationState != null
                  && target.federationState.allowRelaxedDDM);
               if (strictOverlap || (relaxedOverlap
                  && sourceBounds.getLowerBound() <= targetBounds.getUpperBound()
                  && targetBounds.getLowerBound() <= sourceBounds.getUpperBound())) {
                  return true;
               }
            }
         }
      }
      return false;
   }

   private void requireConnected() throws NotConnected {
      if (!connected) {
         throw new NotConnected("not connected");
      }
   }

   private void requireJoined() throws NotConnected {
      requireConnected();
      if (!joined) {
         throw new NotConnected("not joined");
      }
   }

   private static MockSupportHandle supportHandle(String kind, String value) {
      return new MockSupportHandle(kind + ":" + value);
   }

   private static String objectAttributeKey(String objectClassName, String attributeName) {
      return objectClassName + "\u001F" + attributeName;
   }

   private static String directedInteractionKey(
      String objectClassName, String interactionName) {
      return objectClassName + "\u001F" + interactionName;
   }

   private static String ownershipKey(String objectInstanceName, String attributeName) {
      return objectInstanceName + "\u001F" + attributeName;
   }

   private static MockAttributeHandleSet singletonAttributeSet(AttributeHandle attribute) {
      MockAttributeHandleSet result = new MockAttributeHandleSet();
      result.add(attribute);
      return result;
   }

   private MockOwnershipRecord ownershipRecord(
      String objectInstanceName, AttributeHandle attribute) {
      if (federationState == null) {
         throw new IllegalArgumentException("fixture requires a shared federation");
      }
      String attributeName = handlePayload(attribute, "attribute");
      String key = ownershipKey(objectInstanceName, attributeName);
      MockOwnershipRecord existing = federationState.attributeOwnership.get(key);
      if (existing != null) {
         return existing;
      }
      MockObjectRecord object = federationState.objectInstances.get(objectInstanceName);
      if (object == null) {
         throw new IllegalArgumentException("object instance is not known");
      }
      MockOwnershipRecord created = new MockOwnershipRecord();
      created.owner = object.owner;
      federationState.attributeOwnership.put(key, created);
      return created;
   }

   private void transferOwnership(
      ObjectInstanceHandle objectInstance,
      AttributeHandle attribute,
      MockOwnershipRecord ownership,
      byte[] userSuppliedTag) {
      MockRTIambassador acquirer = ownership.pendingAcquirer;
      if (acquirer == null) {
         return;
      }
      ownership.owner = acquirer;
      ownership.pendingAcquirer = null;
      ownership.pendingAcquisitionTag = null;
      ownership.divestitureOffered = false;
      MockAttributeHandleSet copiedAttributes = singletonAttributeSet(attribute);
      byte[] tag = userSuppliedTag.clone();
      acquirer.deliverCallback(() -> acquirer.federateAmbassador
         .attributeOwnershipAcquisitionNotification(objectInstance, copiedAttributes, tag));
   }

   private static String handlePayload(Object handle, String expectedKind) {
      if (!(handle instanceof MockSupportHandle)) {
         throw new IllegalArgumentException("fixture requires a decoded support handle");
      }
      MockSupportHandle supportHandle = (MockSupportHandle) handle;
      byte[] encoded = new byte[supportHandle.encodedLength()];
      supportHandle.encode(encoded, 0);
      String token = new String(encoded, java.nio.charset.StandardCharsets.UTF_8);
      String prefix = expectedKind + ":";
      if (!token.startsWith(prefix)) {
         throw new IllegalArgumentException("handle has the wrong standard domain");
      }
      return token.substring(prefix.length());
   }
}
