package org.umbra.testfixture.rti1516e;

import hla.rti1516e.RTIambassador;
import hla.rti1516e.FederateAmbassador;
import hla.rti1516e.RtiFactory;
import hla.rti1516e.AttributeHandle;
import hla.rti1516e.AttributeHandleFactory;
import hla.rti1516e.AttributeHandleSet;
import hla.rti1516e.AttributeHandleSetFactory;
import hla.rti1516e.AttributeHandleValueMap;
import hla.rti1516e.AttributeHandleValueMapFactory;
import hla.rti1516e.AttributeRegionAssociation;
import hla.rti1516e.AttributeSetRegionSetPairList;
import hla.rti1516e.AttributeSetRegionSetPairListFactory;
import hla.rti1516e.DimensionHandle;
import hla.rti1516e.DimensionHandleFactory;
import hla.rti1516e.DimensionHandleSet;
import hla.rti1516e.DimensionHandleSetFactory;
import hla.rti1516e.FederateHandle;
import hla.rti1516e.FederateHandleFactory;
import hla.rti1516e.InteractionClassHandle;
import hla.rti1516e.InteractionClassHandleFactory;
import hla.rti1516e.LogicalTime;
import hla.rti1516e.RestoreFailureReason;
import hla.rti1516e.SaveFailureReason;
import hla.rti1516e.ObjectClassHandle;
import hla.rti1516e.ObjectClassHandleFactory;
import hla.rti1516e.ObjectInstanceHandle;
import hla.rti1516e.ObjectInstanceHandleFactory;
import hla.rti1516e.OrderType;
import hla.rti1516e.ParameterHandle;
import hla.rti1516e.ParameterHandleFactory;
import hla.rti1516e.ParameterHandleValueMap;
import hla.rti1516e.ParameterHandleValueMapFactory;
import hla.rti1516e.RangeBounds;
import hla.rti1516e.RegionHandle;
import hla.rti1516e.RegionHandleSet;
import hla.rti1516e.RegionHandleSetFactory;
import hla.rti1516e.TransportationTypeHandle;
import hla.rti1516e.TransportationTypeHandleFactory;
import hla.rti1516e.encoding.DataElement;
import hla.rti1516e.encoding.DataElementFactory;
import hla.rti1516e.encoding.ByteWrapper;
import hla.rti1516e.encoding.EncoderFactory;
import hla.rti1516e.encoding.HLAfixedRecord;
import hla.rti1516e.encoding.HLAinteger32BE;
import hla.rti1516e.encoding.HLAoctet;
import hla.rti1516e.encoding.HLAunicodeString;
import hla.rti1516e.encoding.HLAvariableArray;
import hla.rti1516e.exceptions.FederateServiceInvocationsAreBeingReportedViaMOM;
import hla.rti1516e.time.HLAfloat64Interval;
import hla.rti1516e.time.HLAfloat64Time;
import hla.rti1516e.time.HLAfloat64TimeFactory;
import hla.rti1516e.time.HLAinteger64Interval;
import hla.rti1516e.time.HLAinteger64Time;
import hla.rti1516e.time.HLAinteger64TimeFactory;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

/** Deliberately small ServiceRegistry fixture; it is not an HLA implementation. */
public final class MockRtiFactory implements RtiFactory {
   @Override public RTIambassador getRtiAmbassador() {
      AmbassadorHandler handler = new AmbassadorHandler();
      HANDLERS.add(handler);
      return proxy(RTIambassador.class, handler);
   }

   @Override public EncoderFactory getEncoderFactory() {
      return proxy(EncoderFactory.class, new EncoderHandler());
   }

   @Override public String rtiName() { return "Umbra mock Java 1516e"; }
   @Override public String rtiVersion() { return "fixture"; }

   @SuppressWarnings("unchecked")
   private static <T> T proxy(Class<T> type, InvocationHandler handler) {
      return (T) Proxy.newProxyInstance(type.getClassLoader(), new Class<?>[] {type}, handler);
   }

   private static final List<AmbassadorHandler> HANDLERS = new ArrayList<>();
   private static final Set<String> FEDERATIONS = new HashSet<>();
   private static final Map<String, String> SAVED_FEDERATIONS = new HashMap<>();
   private static final Map<String, String> FEDERATION_FOM_DATA = new HashMap<>();
   private static final Map<String, String> FEDERATION_MIM_DATA = new HashMap<>();
   private static final Map<String, String> FEDERATION_TIME_IMPLEMENTATIONS = new HashMap<>();
   private static final Map<String, SyncPointRecord> SYNCHRONIZATION_POINTS = new LinkedHashMap<>();
   private static final Map<String, ObjectRecord> OBJECTS = new HashMap<>();
   private static final Map<String, RegionRecord> REGIONS = new HashMap<>();
   private static final ScheduledExecutorService MOM_SCHEDULER =
         Executors.newScheduledThreadPool(1, new ThreadFactory() {
            @Override public Thread newThread(Runnable task) {
               Thread thread = new Thread(task, "umbra-mock-2010-mom");
               thread.setDaemon(true);
               return thread;
            }
         });
   private static int nextObjectId = 1;
   private static int nextRegionId = 1;

   private static final class AmbassadorHandler implements InvocationHandler {
      private FederateAmbassador callback;
      private String callbackModel = "HLA_IMMEDIATE";
      private String federation;
      private String federateName;
      private FederateHandle federateHandle;
      private boolean connected;
      private boolean joined;
      private final Set<String> publishedClasses = new HashSet<>();
      private final Map<String, Set<AttributeHandle>> publishedObjectAttributes = new HashMap<>();
      private final Set<String> subscribedClasses = new HashSet<>();
      private final Map<String, Set<AttributeHandle>> subscribedObjectAttributes = new HashMap<>();
      private final Set<String> publishedInteractions = new HashSet<>();
      private final Set<String> subscribedInteractions = new HashSet<>();
      // MOM object-instance reports count distinct object instances for which
      // this federate has invoked Update Attribute Values or received a
      // reflection.  Sets preserve the 1516e "number of instances" meaning
      // when the same instance is updated/reflected more than once.
      private final Map<String, Set<String>> updatedObjectInstances = new HashMap<>();
      private final Map<String, Set<String>> reflectedObjectInstances = new HashMap<>();
      // These ledgers count service invocations by standard transportation
      // bucket. They are intentionally separate from the distinct-instance
      // sets used by HLAreportObjectInstances{Updated,Reflected}.
      private final Map<String, Map<String, Integer>> updateCountsByTransportation =
            new LinkedHashMap<>();
      private final Map<String, Map<String, Integer>> reflectionCountsByTransportation =
            new LinkedHashMap<>();
      private final Map<String, String> attributeTransportation = new HashMap<>();
      private final Map<String, Map<String, Integer>> interactionSentCountsByTransportation =
            new LinkedHashMap<>();
      private final Map<String, Map<String, Integer>> interactionReceivedCountsByTransportation =
            new LinkedHashMap<>();
      private final Map<String, String> interactionTransportation = new HashMap<>();
      // Keep the fixture's state intentionally small, but retain the selected
      // standard carrier instead of truncating float64 values to long.  The
      // provider-neutral Python matrix uses fractional values to prove that
      // Java -> JPype -> Python preserves the 2010 time family.
      private String timeImplementation = HLAinteger64TimeFactory.NAME;
      private double logicalTime;
      private double lookahead;
      private boolean timeRegulating;
      private boolean timeConstrained;
      private boolean serviceReporting;
      private boolean exceptionReporting;
      private int momReportPeriod;
      private long momSerial;
      private ScheduledFuture<?> momTimingTask;
      private final List<AmbassadorHandler> pendingMomPeriodicRequests = new ArrayList<>();
      private String saveLabel;
      private LogicalTime saveTime;
      private boolean saveBegun;
      private boolean restoreInProgress;

      @Override public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
          String name = method.getName();
          if (name.equals("getHLAversion")) return "IEEE 1516.1-2010";
          if (name.equals("toString")) return "Mock2010RTIambassador";
          if (name.equals("connect") && args != null && args.length > 0
                && args[0] instanceof FederateAmbassador) {
             callback = (FederateAmbassador) args[0];
             callbackModel = args.length > 1 && args[1] != null
                   ? String.valueOf(args[1]) : "HLA_IMMEDIATE";
             connected = true;
             // Exercise the standard callback proxy at the Java boundary.
             try {
                callback.synchronizationPointRegistrationSucceeded("fixture-sync");
             } catch (Exception error) {
                throw new IllegalStateException("fixture callback failed", error);
             }
             return null;
          }
          if (name.equals("disconnect")) {
             cancelMomTiming();
             connected = false;
             joined = false;
             federation = null;
             saveLabel = null;
             saveTime = null;
             saveBegun = false;
             return null;
          }
          if (name.equals("createFederationExecution")) {
             String nameValue = (String) args[0];
             FEDERATIONS.add(nameValue);
             FEDERATION_TIME_IMPLEMENTATIONS.put(
                   nameValue, timeImplementationName(args));
             FEDERATION_FOM_DATA.put(nameValue,
                   readModuleData(args != null && args.length > 1 ? args[1] : null));
             FEDERATION_MIM_DATA.put(nameValue,
                   readModuleData(args != null && args.length > 2 ? args[2] : null));
             ObjectClassHandle momClass = objectClassHandle(
                   "HLAobjectRoot.HLAmanager.HLAfederation");
             ObjectInstanceHandle momObject = (ObjectInstanceHandle) handle(
                   ObjectInstanceHandle.class, "mom-federation-object:" + nameValue);
             OBJECTS.put(handleKey(momObject), new ObjectRecord(
                   momObject, momClass, "MOM-FEDERATION:" + nameValue, nameValue,
                   null, true));
             return null;
          }
          if (name.equals("destroyFederationExecution")) {
             String federationName = (String) args[0];
             FEDERATIONS.remove(federationName);
             FEDERATION_TIME_IMPLEMENTATIONS.remove(federationName);
             FEDERATION_FOM_DATA.remove(federationName);
             FEDERATION_MIM_DATA.remove(federationName);
             removeFederationSynchronizationPoints(federationName);
             removeFederationObjects(federationName);
             return null;
          }
          if (name.equals("joinFederationExecution")) {
             federation = lastString(args);
             timeImplementation = FEDERATION_TIME_IMPLEMENTATIONS.getOrDefault(
                   federation, HLAinteger64TimeFactory.NAME);
             // The two- and three-string overloads have the federation name
             // last.  URL[] overloads carry it immediately before the array.
             federateName = args != null && args.length > 0 && args[0] instanceof String
                   ? (String) args[0] : "fixture-federate";
             federateHandle = (FederateHandle) handle(
                   FederateHandle.class, "federate:" + federateName);
             joined = true;
             ObjectClassHandle momClass = objectClassHandle(
                   "HLAobjectRoot.HLAmanager.HLAfederate");
             ObjectInstanceHandle momObject = (ObjectInstanceHandle) handle(
                   ObjectInstanceHandle.class,
                   "mom-object:" + federation + ":" + federateName);
             OBJECTS.put(handleKey(momObject), new ObjectRecord(
                   momObject, momClass, "MOM:" + federateName, federation,
                   null, true));
             addFederateToSynchronizationPoints(this);
             return federateHandle;
          }
          if (name.equals("resignFederationExecution")) {
             cancelMomTiming();
             removeMomObject(federation, federateName);
             joined = false;
             saveLabel = null;
             saveTime = null;
             saveBegun = false;
             return null;
          }
          if (name.equals("requestFederationSave")) {
             String label = args != null && args.length > 0 ? (String) args[0] : "fixture-save";
             LogicalTime requestedTime = args != null && args.length > 1
                   ? (LogicalTime) args[1] : null;
             saveLabel = label;
             saveTime = requestedTime;
             for (AmbassadorHandler target : HANDLERS) {
                if (!target.connected || !target.joined
                      || !sameFederation(target.federation, federation)) continue;
                target.saveLabel = label;
                target.saveTime = requestedTime;
                try {
                   if (requestedTime == null) {
                      target.callback.initiateFederateSave(label);
                   } else {
                      target.callback.initiateFederateSave(label, requestedTime);
                   }
                } catch (Exception error) {
                   throw new IllegalStateException("fixture save-initiation callback failed", error);
                }
             }
             return null;
          }
          if (name.equals("federateSaveBegun")) {
             saveBegun = true;
             return null;
          }
          if (name.equals("federateSaveComplete")) {
             if (saveBegun && saveLabel != null) {
                SAVED_FEDERATIONS.put(federation, saveLabel);
                for (AmbassadorHandler target : HANDLERS) {
                   if (!target.connected || !target.joined
                         || !sameFederation(target.federation, federation)) continue;
                   try {
                      target.callback.federationSaved();
                   } catch (Exception error) {
                      throw new IllegalStateException("fixture federation-saved callback failed", error);
                   }
                   target.saveLabel = null;
                   target.saveTime = null;
                   target.saveBegun = false;
                }
             }
             return null;
          }
          if (name.equals("federateSaveNotComplete")
                || name.equals("abortFederationSave")) {
             if (saveLabel != null || saveBegun) {
                SaveFailureReason reason = name.equals("abortFederationSave")
                      ? SaveFailureReason.SAVE_ABORTED
                      : SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE;
                for (AmbassadorHandler target : HANDLERS) {
                   if (!target.connected || !target.joined
                         || !sameFederation(target.federation, federation)) continue;
                   try {
                      target.callback.federationNotSaved(reason);
                   } catch (Exception error) {
                      throw new IllegalStateException("fixture save-failure callback failed", error);
                   }
                   target.saveLabel = null;
                   target.saveTime = null;
                   target.saveBegun = false;
                }
             }
             return null;
          }
          if (name.equals("requestFederationRestore")) {
             String label = args != null && args.length > 0 ? (String) args[0] : "fixture-save";
             if (!label.equals(SAVED_FEDERATIONS.get(federation))) {
                if (callback != null) {
                   try {
                      callback.requestFederationRestoreFailed(label);
                   } catch (Exception error) {
                      throw new IllegalStateException("fixture restore-failure callback failed", error);
                   }
                }
                return null;
             }
             for (AmbassadorHandler target : HANDLERS) {
                if (!target.connected || !target.joined
                      || !sameFederation(target.federation, federation)) continue;
                target.restoreInProgress = true;
                try {
                   target.callback.requestFederationRestoreSucceeded(label);
                   target.callback.federationRestoreBegun();
                   target.callback.initiateFederateRestore(
                         label, target.federateName, target.federateHandle);
                } catch (Exception error) {
                   throw new IllegalStateException("fixture restore-initiation callback failed", error);
                }
             }
             return null;
          }
          if (name.equals("federateRestoreComplete")) {
             if (restoreInProgress) {
                restoreInProgress = false;
                for (AmbassadorHandler target : HANDLERS) {
                   if (!target.connected || !target.joined
                         || !sameFederation(target.federation, federation)) continue;
                   target.restoreInProgress = false;
                   try {
                      target.callback.federationRestored();
                   } catch (Exception error) {
                      throw new IllegalStateException("fixture federation-restored callback failed", error);
                   }
                }
             }
             return null;
          }
          if (name.equals("federateRestoreNotComplete")
                || name.equals("abortFederationRestore")) {
             if (restoreInProgress) {
                RestoreFailureReason reason = name.equals("abortFederationRestore")
                      ? RestoreFailureReason.RESTORE_ABORTED
                      : RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE;
                for (AmbassadorHandler target : HANDLERS) {
                   if (!target.connected || !target.joined
                         || !sameFederation(target.federation, federation)) continue;
                   try {
                      target.callback.federationNotRestored(reason);
                   } catch (Exception error) {
                      throw new IllegalStateException("fixture restore-failure callback failed", error);
                   }
                   target.restoreInProgress = false;
                }
             }
             return null;
          }
          if (name.equals("enableTimeRegulation")) {
             lookahead = numericValue(args[0]);
             timeRegulating = true;
             if (callback != null) {
               try {
                   callback.timeRegulationEnabled((LogicalTime) timeValue(logicalTime));
                } catch (Exception error) {
                   throw new IllegalStateException("fixture time-regulation callback failed", error);
                }
             }
             return null;
          }
          if (name.equals("disableTimeRegulation")) {
             timeRegulating = false;
             return null;
          }
          if (name.equals("enableTimeConstrained")) {
             timeConstrained = true;
             if (callback != null) {
                try {
                   callback.timeConstrainedEnabled((LogicalTime) timeValue(logicalTime));
                } catch (Exception error) {
                   throw new IllegalStateException("fixture time-constrained callback failed", error);
                }
             }
             return null;
          }
          if (name.equals("disableTimeConstrained")) {
             timeConstrained = false;
             return null;
          }
          if (name.equals("timeAdvanceRequest")
                || name.equals("timeAdvanceRequestAvailable")
                || name.equals("nextMessageRequest")
                || name.equals("nextMessageRequestAvailable")
                || name.equals("flushQueueRequest")) {
             logicalTime = numericValue(args[0]);
             if (callback != null) {
               try {
                   callback.timeAdvanceGrant((LogicalTime) timeValue(logicalTime));
                } catch (Exception error) {
                   throw new IllegalStateException("fixture time-advance callback failed", error);
                }
             }
             return null;
          }
          if (name.equals("modifyLookahead")) {
             lookahead = numericValue(args[0]);
             return null;
          }
          if (name.equals("queryLogicalTime")) {
             return timeValue(logicalTime);
          }
          if (name.equals("queryLookahead")) {
             return intervalValue(lookahead);
          }
          if (name.equals("registerFederationSynchronizationPoint")) {
             String label = (String) args[0];
             byte[] tag = (byte[]) args[1];
             registerSynchronizationPoint(label, this);
             for (AmbassadorHandler target : HANDLERS) {
                if (!target.connected || !target.joined
                      || !sameFederation(target.federation, federation)) continue;
                try {
                   if (target == this) {
                      target.callback.synchronizationPointRegistrationSucceeded(label);
                   } else {
                      target.callback.announceSynchronizationPoint(label, tag);
                   }
                } catch (Exception error) {
                   throw new IllegalStateException("fixture synchronization callback failed", error);
                }
             }
             return null;
          }
          if (name.equals("synchronizationPointAchieved")) {
             markSynchronizationPointAchieved((String) args[0], this);
             return null;
          }
          if (name.equals("evokeCallback") || name.equals("evokeMultipleCallbacks")) {
             return drainMomPeriodicCallback();
          }
          if (name.equals("getObjectClassHandle")) {
             ObjectClassHandle objectClass = objectClassHandle((String) args[0]);
             if (serviceReporting) notifyMomServiceInvocation("getObjectClassHandle");
             return objectClass;
          }
          if (name.equals("getObjectClassName")) {
             return objectClassName(args[0]);
          }
          if (name.equals("getAttributeHandle")) {
             return attributeHandle(args[0], (String) args[1]);
          }
          if (name.equals("getAttributeName")) {
             return attributeName(args[0], args[1]);
          }
          if (name.equals("getInteractionClassHandle")) {
             return interactionClassHandle((String) args[0]);
          }
          if (name.equals("getInteractionClassName")) {
             return interactionClassName(args[0]);
          }
          if (name.equals("getParameterHandle")) {
             return parameterHandle(args[0], (String) args[1]);
          }
          if (name.equals("getParameterName")) {
             return parameterName(args[0], args[1]);
          }
          if (name.equals("getTransportationTypeHandle")) {
             return handle(TransportationTypeHandle.class,
                   "transport:" + transportationName(args[0]));
          }
          if (name.equals("getTransportationTypeName")) {
             return transportationName(args[0]);
          }
          if (name.equals("getObjectInstanceName")) {
             ObjectRecord record = OBJECTS.get(handleKey(args[0]));
             return record == null ? "fixture-object" : record.name;
          }
          if (name.equals("getObjectInstanceHandle")) {
             String objectName = String.valueOf(args[0]);
             for (ObjectRecord record : OBJECTS.values()) {
                if (objectName.equals(record.name)
                      && sameFederation(record.federation, federation)) return record.object;
             }
             return handle(ObjectInstanceHandle.class, "object-name:" + objectName);
          }
          if (name.equals("getFederateHandle")) {
             return handle(FederateHandle.class, "federate:" + args[0]);
          }
          if (name.equals("getFederateName")) {
             String key = handleKey(args[0]);
             if (!key.startsWith("federate:")) key = String.valueOf(args[0]);
             return key.startsWith("federate:") ? key.substring("federate:".length()) : key;
          }
          if (name.equals("queryAttributeOwnership")) {
             ObjectRecord record = OBJECTS.get(handleKey(args[0]));
             if (record != null && callback != null) {
                try {
                   callback.informAttributeOwnership(
                         (ObjectInstanceHandle) args[0],
                         (AttributeHandle) args[1],
                         record.owner);
                } catch (Exception error) {
                   throw new IllegalStateException("fixture ownership callback failed", error);
                }
             }
             return null;
          }
          if (name.equals("isAttributeOwnedByFederate")) {
             ObjectRecord record = OBJECTS.get(handleKey(args[0]));
             return record != null && record.owner != null
                   && record.owner.equals(federateHandle);
          }
          if (name.equals("publishObjectClassAttributes")) {
             String objectClassKey = handleKey(args[0]);
             publishedClasses.add(objectClassKey);
             Set<AttributeHandle> attributes = new LinkedHashSet<>();
             if (args.length > 1 && args[1] instanceof AttributeHandleSet) {
                for (Object attribute : (AttributeHandleSet) args[1]) {
                   attributes.add((AttributeHandle) attribute);
                }
             }
             if (attributes.isEmpty()) {
                publishedObjectAttributes.remove(objectClassKey);
             } else {
                publishedObjectAttributes.put(objectClassKey, attributes);
             }
             return null;
          }
          if (name.equals("unpublishObjectClass") || name.equals("unpublishObjectClassAttributes")) {
             String objectClassKey = handleKey(args[0]);
             publishedClasses.remove(objectClassKey);
             publishedObjectAttributes.remove(objectClassKey);
             return null;
          }
          if (name.equals("subscribeObjectClassAttributes")
                || name.equals("subscribeObjectClassAttributesPassively")) {
             String objectClassKey = handleKey(args[0]);
             subscribedClasses.add(objectClassKey);
             AttributeHandleSet requested = args.length > 1 && args[1] instanceof AttributeHandleSet
                   ? (AttributeHandleSet) args[1] : null;
             Set<AttributeHandle> attributes = new LinkedHashSet<>();
             if (requested != null) {
                for (Object attribute : requested) attributes.add((AttributeHandle) attribute);
             }
             if (attributes.isEmpty()) {
                subscribedObjectAttributes.remove(objectClassKey);
             } else {
                subscribedObjectAttributes.put(objectClassKey, attributes);
             }
             notifyExistingMomObjects((ObjectClassHandle) args[0], requested);
             return null;
          }
          if (name.equals("unsubscribeObjectClass") || name.equals("unsubscribeObjectClassAttributes")) {
             String objectClassKey = handleKey(args[0]);
             subscribedClasses.remove(objectClassKey);
             subscribedObjectAttributes.remove(objectClassKey);
             return null;
          }
          if (name.equals("publishInteractionClass")) {
             publishedInteractions.add(handleKey(args[0]));
             return null;
          }
          if (name.equals("unpublishInteractionClass")) {
             publishedInteractions.remove(handleKey(args[0]));
             return null;
          }
          if (name.equals("subscribeInteractionClass")
                || name.equals("subscribeInteractionClassPassively")) {
             String interactionKey = handleKey(args[0]);
             String serviceReportKey = handleKey(interactionClassHandle(
                   "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                   + "HLAreportServiceInvocation"));
             if (serviceReporting && serviceReportKey.equals(interactionKey)) {
                throw new FederateServiceInvocationsAreBeingReportedViaMOM(
                      "service reporting is enabled for this federate");
             }
             subscribedInteractions.add(interactionKey);
             return null;
          }
          if (name.equals("unsubscribeInteractionClass")) {
             subscribedInteractions.remove(handleKey(args[0]));
             return null;
          }
          if (name.equals("registerObjectInstance")) {
             ObjectClassHandle objectClass = (ObjectClassHandle) args[0];
             String objectName = args.length > 1 ? (String) args[1]
                   : "fixture-object-" + nextObjectId;
             ObjectInstanceHandle object = (ObjectInstanceHandle) handle(
                   ObjectInstanceHandle.class, "object:" + nextObjectId++);
             OBJECTS.put(handleKey(object), new ObjectRecord(
                   object, objectClass, objectName, federation, federateHandle));
             notifyDiscovery(object, objectClass, objectName);
             return object;
          }
          if (name.equals("updateAttributeValues")) {
             ObjectRecord record = OBJECTS.get(handleKey(args[0]));
             if (record != null && !record.mom) {
                updatedObjectInstances.computeIfAbsent(handleKey(record.objectClass),
                      ignored -> new LinkedHashSet<>()).add(handleKey(record.object));
                incrementTransportCount(updateCountsByTransportation,
                      transportationNameForUpdate(record, args[1]), handleKey(record.objectClass));
             }
             LogicalTime timestamp = args.length > 3 ? (LogicalTime) args[3] : null;
             notifyReflection((ObjectInstanceHandle) args[0], (AttributeHandleValueMap) args[1],
                   (byte[]) args[2], timestamp);
             return null;
          }
          if (name.equals("requestAttributeTransportationTypeChange")) {
             ObjectRecord record = OBJECTS.get(handleKey(args[0]));
             if (record != null && args.length > 1 && args[1] instanceof AttributeHandleSet) {
                String transportation = transportationName(args.length > 2 ? args[2] : null);
                for (Object attribute : (AttributeHandleSet) args[1]) {
                   attributeTransportation.put(attributeKey(record.object, attribute), transportation);
                }
             }
             return null;
          }
          if (name.equals("requestInteractionTransportationTypeChange")) {
             if (args.length > 1) {
                interactionTransportation.put(handleKey(args[0]),
                      transportationName(args[1]));
             }
             return null;
          }
          if (name.equals("deleteObjectInstance")) {
             ObjectRecord record = OBJECTS.remove(handleKey(args[0]));
             if (record == null && exceptionReporting) {
                notifyMomException("RTIambassador.deleteObjectInstance", "ObjectInstanceNotKnown");
             }
             return null;
          }
          if (name.equals("sendInteraction")) {
             String interactionKey = handleKey(args[0]);
             if (interactionKey.endsWith(".HLAsetExceptionReporting")) {
                AmbassadorHandler requestedFederate = resolveRequestedFederate(
                      (ParameterHandleValueMap) args[1]);
                requestedFederate.exceptionReporting = true;
             }
            if (interactionKey.endsWith(".HLAsetServiceReporting")) {
                AmbassadorHandler requestedFederate = resolveRequestedFederate(
                      (ParameterHandleValueMap) args[1]);
                boolean enabled = parameterIsTrue((ParameterHandleValueMap) args[1],
                      "HLAreportingState");
                InteractionClassHandle reportClass = interactionClassHandle(
                      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                      + "HLAreportServiceInvocation");
                if (requestedFederate != null && enabled
                      && requestedFederate.subscribedInteractions.contains(handleKey(reportClass))) {
                   notifyMomMOMException(requestedFederate,
                         interactionKey.substring("interaction:".length()), false);
                } else if (requestedFederate != null) {
                   requestedFederate.serviceReporting = enabled;
                }
            }
             String transportation = interactionTransportation.getOrDefault(
                   interactionKey, "HLAreliable");
             if (isApplicationInteraction(interactionKey)) {
                incrementTransportCount(interactionSentCountsByTransportation,
                      transportation, interactionKey);
             }
             LogicalTime timestamp = args.length > 3 ? (LogicalTime) args[3] : null;
             notifyInteraction((InteractionClassHandle) args[0],
                   (ParameterHandleValueMap) args[1], (byte[]) args[2], transportation,
                   timestamp);
             if (interactionKey.endsWith(".HLArequestPublications")) {
                notifyMomPublicationReports(resolveRequestedFederate(
                      (ParameterHandleValueMap) args[1]));
             }
             if (interactionKey.endsWith(".HLArequestSubscriptions")) {
                notifyMomSubscriptionReports(resolveRequestedFederate(
                      (ParameterHandleValueMap) args[1]));
             }
            if (interactionKey.endsWith(".HLArequestObjectInstancesThatCanBeDeleted")) {
               notifyMomObjectInstancesThatCanBeDeleted(resolveRequestedFederate(
                     (ParameterHandleValueMap) args[1]));
            }
            if (interactionKey.endsWith(".HLArequestObjectInstancesUpdated")) {
               notifyMomObjectInstanceCounts(resolveRequestedFederate(
                     (ParameterHandleValueMap) args[1]), "HLAreportObjectInstancesUpdated");
            }
            if (interactionKey.endsWith(".HLArequestObjectInstancesReflected")) {
               notifyMomObjectInstanceCounts(resolveRequestedFederate(
                     (ParameterHandleValueMap) args[1]), "HLAreportObjectInstancesReflected");
            }
            if (interactionKey.endsWith(".HLArequestUpdatesSent")) {
               AmbassadorHandler requestedFederate = resolveRequestedFederate(
                     (ParameterHandleValueMap) args[1]);
               notifyMomTransportCounts(requestedFederate, "HLAreportUpdatesSent",
                     "HLAupdateCounts",
                     requestedFederate == null ? null : requestedFederate.updateCountsByTransportation);
            }
            if (interactionKey.endsWith(".HLArequestReflectionsReceived")) {
               AmbassadorHandler requestedFederate = resolveRequestedFederate(
                     (ParameterHandleValueMap) args[1]);
               notifyMomTransportCounts(requestedFederate, "HLAreportReflectionsReceived",
                     "HLAreflectCounts",
                     requestedFederate == null ? null : requestedFederate.reflectionCountsByTransportation);
            }
            if (interactionKey.endsWith(".HLArequestInteractionsSent")) {
               AmbassadorHandler requestedFederate = resolveRequestedFederate(
                     (ParameterHandleValueMap) args[1]);
               notifyMomInteractionCounts(requestedFederate,
                     "HLAreportInteractionsSent", requestedFederate == null
                           ? null : requestedFederate.interactionSentCountsByTransportation);
            }
            if (interactionKey.endsWith(".HLArequestInteractionsReceived")) {
               AmbassadorHandler requestedFederate = resolveRequestedFederate(
                     (ParameterHandleValueMap) args[1]);
               notifyMomInteractionCounts(requestedFederate,
                     "HLAreportInteractionsReceived", requestedFederate == null
                           ? null : requestedFederate.interactionReceivedCountsByTransportation);
            }
            if (interactionKey.endsWith(".HLArequestFOMmoduleData")) {
               if (interactionKey.contains(".HLAmanager.HLAfederation.")) {
                  notifyMomFederationFomModuleReport((ParameterHandleValueMap) args[1]);
               } else {
                  notifyMomFederateFomModuleReport(resolveRequestedFederate(
                        (ParameterHandleValueMap) args[1]), (ParameterHandleValueMap) args[1]);
               }
            }
            if (interactionKey.endsWith(".HLArequestMIMdata")) {
               notifyMomFederationMimDataReport();
            }
            if (interactionKey.endsWith(".HLArequestSynchronizationPoints")) {
               notifyMomSynchronizationPointsReport();
            }
            if (interactionKey.endsWith(".HLArequestSynchronizationPointStatus")) {
               notifyMomSynchronizationPointStatusReport(
                     decodeSynchronizationPointName((ParameterHandleValueMap) args[1]));
            }
            if (interactionKey.endsWith(".HLArequestObjectInstanceInformation")) {
                notifyMomObjectInstanceInformation((ParameterHandleValueMap) args[1]);
             }
            if (interactionKey.endsWith(".HLAsetTiming")) {
               ParameterHandleValueMap timingValues = (ParameterHandleValueMap) args[1];
               AmbassadorHandler requestedFederate = resolveRequestedFederate(timingValues);
               byte[] period = parameterValue(timingValues, "HLAreportPeriod");
               if (period == null || period.length != 4
                     || ByteBuffer.wrap(period).order(ByteOrder.BIG_ENDIAN).getInt() < 0) {
                  notifyMomMOMException(requestedFederate,
                        interactionKey.substring("interaction:".length()));
               } else {
                  int seconds = ByteBuffer.wrap(period)
                        .order(ByteOrder.BIG_ENDIAN).getInt();
                  if (requestedFederate != null) {
                     requestedFederate.armMomTiming(seconds);
                  }
               }
            }
             // Changing the service-reporting switch is a MOM control
             // interaction, not a reported RTI service invocation itself.
             if (serviceReporting && !interactionKey.endsWith(".HLAsetServiceReporting")) {
                notifyMomServiceInvocation("sendInteraction");
             }
             if (method.getReturnType().isInterface()) {
                return proxy(method.getReturnType(), new DefaultHandler());
             }
             return null;
          }
          if (name.equals("getDimensionHandle")) {
             return dimensionHandle((String) args[0]);
          }
          if (name.equals("getDimensionName")) {
             return dimensionName(args[0]);
          }
          if (name.equals("getDimensionUpperBound")) {
             return dimensionUpperBound(args[0]);
          }
          if (name.equals("createRegion")) {
             DimensionHandleSet dimensions = (DimensionHandleSet) args[0];
             RegionHandle region = (RegionHandle) handle(
                   RegionHandle.class, "region:" + nextRegionId++);
             REGIONS.put(handleKey(region), new RegionRecord(region, federation, dimensions));
             return region;
          }
          if (name.equals("getDimensionHandleSet")) {
             RegionRecord record = REGIONS.get(handleKey(args[0]));
             if (record == null) return null;
             return proxy(DimensionHandleSet.class,
                   new SetHandler(DimensionHandleSet.class, record.dimensions));
          }
          if (name.equals("setRangeBounds")) {
             RegionRecord record = REGIONS.get(handleKey(args[0]));
             if (record != null) record.bounds.put(handleKey(args[1]), (RangeBounds) args[2]);
             return null;
          }
          if (name.equals("getRangeBounds")) {
             RegionRecord record = REGIONS.get(handleKey(args[0]));
             return record == null ? null : record.bounds.get(handleKey(args[1]));
          }
          if (name.equals("commitRegionModifications")) {
             return null;
          }
          if (name.equals("deleteRegion")) {
             REGIONS.remove(handleKey(args[0]));
             return null;
          }
          if (name.equals("subscribeObjectClassAttributesWithRegions")
                || name.equals("subscribeObjectClassAttributesPassivelyWithRegions")) {
             subscribedClasses.add(handleKey(args[0]));
             return null;
          }
          if (name.equals("unsubscribeObjectClassAttributesWithRegions")) {
             subscribedClasses.remove(handleKey(args[0]));
             return null;
          }
          if (name.equals("subscribeInteractionClassWithRegions")
                || name.equals("subscribeInteractionClassPassivelyWithRegions")) {
             subscribedInteractions.add(handleKey(args[0]));
             return null;
          }
          if (name.equals("unsubscribeInteractionClassWithRegions")) {
             subscribedInteractions.remove(handleKey(args[0]));
             return null;
          }
          if (name.equals("getObjectClassHandleFactory")) {
            return proxy(ObjectClassHandleFactory.class, new HandleFactoryHandler(ObjectClassHandle.class));
          }
          if (name.equals("getFederateHandleFactory")) {
             return proxy(FederateHandleFactory.class, new HandleFactoryHandler(FederateHandle.class));
          }
          if (name.equals("getObjectInstanceHandleFactory")) {
             return proxy(ObjectInstanceHandleFactory.class, new HandleFactoryHandler(ObjectInstanceHandle.class));
          }
          if (name.equals("getAttributeHandleFactory")) {
             return proxy(AttributeHandleFactory.class, new HandleFactoryHandler(AttributeHandle.class));
          }
          if (name.equals("getInteractionClassHandleFactory")) {
             return proxy(InteractionClassHandleFactory.class,
                   new HandleFactoryHandler(InteractionClassHandle.class));
          }
          if (name.equals("getParameterHandleFactory")) {
             return proxy(ParameterHandleFactory.class, new HandleFactoryHandler(ParameterHandle.class));
          }
          if (name.equals("getDimensionHandleFactory")) {
             return proxy(DimensionHandleFactory.class, new HandleFactoryHandler(DimensionHandle.class));
          }
          if (name.equals("getTransportationTypeHandleFactory")) {
             return proxy(TransportationTypeHandleFactory.class,
                   new TransportationTypeFactoryHandler());
          }
          if (name.equals("getDimensionHandleSetFactory")) {
             return proxy(DimensionHandleSetFactory.class,
                   new SetFactoryHandler(DimensionHandleSet.class));
          }
          if (name.equals("getRegionHandleSetFactory")) {
             return proxy(RegionHandleSetFactory.class,
                   new SetFactoryHandler(RegionHandleSet.class));
          }
          if (name.equals("getAttributeSetRegionSetPairListFactory")) {
             return proxy(AttributeSetRegionSetPairListFactory.class,
                   new ListFactoryHandler(AttributeSetRegionSetPairList.class));
          }
          if (name.equals("getAttributeHandleSetFactory")) {
             return proxy(AttributeHandleSetFactory.class, new SetFactoryHandler(AttributeHandleSet.class));
          }
          if (name.equals("getAttributeHandleValueMapFactory")) {
             return proxy(AttributeHandleValueMapFactory.class,
                   new MapFactoryHandler(AttributeHandleValueMap.class));
          }
          if (name.equals("getParameterHandleValueMapFactory")) {
             return proxy(ParameterHandleValueMapFactory.class,
                   new MapFactoryHandler(ParameterHandleValueMap.class));
          }
          if (name.equals("getTimeFactory")) {
             if (usesFloatTime()) {
                return proxy(HLAfloat64TimeFactory.class, new FloatTimeFactoryHandler());
             }
             return proxy(HLAinteger64TimeFactory.class, new IntegerTimeFactoryHandler());
          }
         Class<?> returnType = method.getReturnType();
         if (returnType == void.class) return null;
         if (returnType == boolean.class) return false;
         if (returnType == byte.class) return (byte) 0;
         if (returnType == short.class) return (short) 0;
         if (returnType == int.class) return 0;
         if (returnType == long.class) return 0L;
         if (returnType == float.class) return 0.0f;
         if (returnType == double.class) return 0.0d;
         if (returnType == char.class) return '\0';
         if (returnType.isInterface()) return proxy(returnType, new DefaultHandler());
         return null;
      }

      private boolean usesFloatTime() {
         return HLAfloat64TimeFactory.NAME.equals(timeImplementation);
      }

      private Object timeValue(double value) {
         return usesFloatTime() ? floatTime(value) : integerTime(Math.round(value));
      }

      private Object intervalValue(double value) {
         return usesFloatTime() ? floatInterval(value) : integerInterval(Math.round(value));
      }

      private void notifyDiscovery(ObjectInstanceHandle object, ObjectClassHandle objectClass, String objectName) {
         for (AmbassadorHandler target : HANDLERS) {
            if (target == this || !target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedClasses.contains(handleKey(objectClass))) continue;
            try {
               target.callback.discoverObjectInstance(object, objectClass, objectName);
            } catch (Exception error) {
               throw new IllegalStateException("fixture discovery callback failed", error);
            }
         }
      }

      private void notifyReflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag) {
         notifyReflection(object, values, tag, null);
      }

      private void notifyReflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, LogicalTime timestamp) {
         ObjectRecord record = OBJECTS.get(handleKey(object));
         if (record == null) return;
         String transportation = transportationNameForUpdate(record, values);
         Object transport = handle(TransportationTypeHandle.class, "transport:" + transportation);
         Object supplement = proxy(FederateAmbassador.SupplementalReflectInfo.class,
               new SupplementalReflectHandler());
         for (AmbassadorHandler target : HANDLERS) {
            if (target == this || !target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedClasses.contains(handleKey(record.objectClass))) continue;
            try {
               target.reflectedObjectInstances.computeIfAbsent(handleKey(record.objectClass),
                     ignored -> new LinkedHashSet<>()).add(handleKey(record.object));
               incrementTransportCount(target.reflectionCountsByTransportation,
                     transportation, handleKey(record.objectClass));
               if (timestamp == null) {
                  target.callback.reflectAttributeValues(object, values, tag,
                        OrderType.RECEIVE, (TransportationTypeHandle) transport,
                        (FederateAmbassador.SupplementalReflectInfo) supplement);
               } else {
                  target.callback.reflectAttributeValues(object, values, tag,
                        OrderType.RECEIVE, (TransportationTypeHandle) transport,
                        timestamp, OrderType.RECEIVE,
                        (FederateAmbassador.SupplementalReflectInfo) supplement);
               }
            } catch (Exception error) {
               throw new IllegalStateException("fixture reflection callback failed", error);
            }
         }
      }

      private void notifyInteraction(InteractionClassHandle interaction,
            ParameterHandleValueMap values, byte[] tag, String transportation) {
         notifyInteraction(interaction, values, tag, transportation, null);
      }

      private void notifyInteraction(InteractionClassHandle interaction,
            ParameterHandleValueMap values, byte[] tag, String transportation,
            LogicalTime timestamp) {
         String callbackTransportation = "HLAreliable".equals(transportation)
               ? "HLAdefaultReliable" : transportation;
         Object transport = handle(TransportationTypeHandle.class,
               "transport:" + (isApplicationInteraction(handleKey(interaction))
                     ? callbackTransportation : "HLAdefaultReliable"));
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         for (AmbassadorHandler target : HANDLERS) {
            if (target == this || !target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(interaction))) continue;
            try {
               if (isApplicationInteraction(handleKey(interaction))) {
                  incrementTransportCount(target.interactionReceivedCountsByTransportation,
                        transportationName(callbackTransportation), handleKey(interaction));
               }
               if (timestamp == null) {
                  target.callback.receiveInteraction(interaction, values, tag,
                        OrderType.RECEIVE, (TransportationTypeHandle) transport,
                        (FederateAmbassador.SupplementalReceiveInfo) supplement);
               } else {
                  target.callback.receiveInteraction(interaction, values, tag,
                        OrderType.RECEIVE, (TransportationTypeHandle) transport,
                        timestamp, OrderType.RECEIVE,
                        (FederateAmbassador.SupplementalReceiveInfo) supplement);
               }
            } catch (Exception error) {
               throw new IllegalStateException("fixture interaction callback failed", error);
            }
         }
      }

      private void notifyMomServiceInvocation(String serviceName) {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAservice"),
               serviceName.getBytes(StandardCharsets.UTF_8));
         values.put(parameterHandle(reportClass, "HLAsuccessIndicator"), new byte[] {1});
         values.put(parameterHandle(reportClass, "HLAsuppliedArguments"), new byte[] {1});
         values.put(parameterHandle(reportClass, "HLAreturnedArguments"), new byte[] {2});
         values.put(parameterHandle(reportClass, "HLAexception"), new byte[0]);
         values.put(parameterHandle(reportClass, "HLAserialNumber"),
               ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt((int) momSerial++).array());
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         RegionHandle reportRegion = (RegionHandle) handle(RegionHandle.class,
               "mom-service-report-region:" + federation + ":" + federateName);
         RegionHandleSet reportRegions = proxy(RegionHandleSet.class,
               new SetHandler(RegionHandleSet.class, Arrays.asList(reportRegion)));
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler(reportRegions));
         for (AmbassadorHandler target : HANDLERS) {
            if (target == this || !target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture MOM report callback failed", error);
            }
         }
      }

      private AmbassadorHandler resolveRequestedFederate(ParameterHandleValueMap values) {
         if (values != null) {
            for (Map.Entry<ParameterHandle, byte[]> entry : values.entrySet()) {
               byte[] requested = entry.getValue();
               if (requested == null) continue;
               for (AmbassadorHandler candidate : HANDLERS) {
                  if (candidate.federateHandle != null
                        && Arrays.equals(requested, encoded(candidate.federateHandle))
                        && sameFederation(candidate.federation, federation)) {
                     return candidate;
                  }
               }
            }
         }
         return this;
      }

      private static boolean parameterIsTrue(ParameterHandleValueMap values, String name) {
         if (values == null) return false;
         for (Map.Entry<ParameterHandle, byte[]> entry : values.entrySet()) {
            if (handleKey(entry.getKey()).endsWith(":" + name)
                  && entry.getValue() != null && entry.getValue().length > 0) {
               return entry.getValue()[entry.getValue().length - 1] != 0;
            }
         }
         return false;
      }

      private static byte[] parameterValue(ParameterHandleValueMap values, String name) {
         if (values == null) return null;
         for (Map.Entry<ParameterHandle, byte[]> entry : values.entrySet()) {
            if (handleKey(entry.getKey()).endsWith(":" + name)) return entry.getValue();
         }
         return null;
      }

      private void registerSynchronizationPoint(String label, AmbassadorHandler registrar) {
         if (label == null || federation == null) return;
         String key = synchronizationPointKey(federation, label);
         SyncPointRecord record = SYNCHRONIZATION_POINTS.computeIfAbsent(
               key, ignored -> new SyncPointRecord(federation, label));
         for (AmbassadorHandler target : HANDLERS) {
            if (target.connected && target.joined
                  && sameFederation(target.federation, federation)) {
               record.participants.add(target);
            }
         }
         record.participants.add(registrar);
      }

      private static void addFederateToSynchronizationPoints(AmbassadorHandler federate) {
         if (federate == null || federate.federation == null) return;
         for (SyncPointRecord record : SYNCHRONIZATION_POINTS.values()) {
            if (sameFederation(record.federation, federate.federation)) {
               record.participants.add(federate);
            }
         }
      }

      private static void markSynchronizationPointAchieved(
            String label, AmbassadorHandler federate) {
         if (label == null || federate == null || federate.federation == null) return;
         SyncPointRecord record = SYNCHRONIZATION_POINTS.get(
               synchronizationPointKey(federate.federation, label));
         if (record == null) return;
         record.achieved.add(federate);
         if (!record.participants.isEmpty() && record.achieved.containsAll(record.participants)) {
            SYNCHRONIZATION_POINTS.remove(
                  synchronizationPointKey(record.federation, record.label));
         }
      }

      private static void removeFederationSynchronizationPoints(String federation) {
         if (federation == null) return;
         SYNCHRONIZATION_POINTS.entrySet().removeIf(
               entry -> sameFederation(entry.getValue().federation, federation));
      }

      private static String synchronizationPointKey(String federation, String label) {
         return String.valueOf(federation) + "\u0000" + String.valueOf(label);
      }

      private void notifyMomSynchronizationPointsReport() {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
               + "HLAreportSynchronizationPoints");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAsyncPoints"),
               encodedSynchronizationPointList(federation));
         deliverMomTextReport(reportClass, values);
      }

      private void notifyMomSynchronizationPointStatusReport(String label) {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
               + "HLAreportSynchronizationPointStatus");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAsyncPointName"),
               encodedSynchronizationPointName(label));
         values.put(parameterHandle(reportClass, "HLAsyncPointFederates"),
               encodedSynchronizationPointFederates(federation, label));
         deliverMomTextReport(reportClass, values);
      }

      private static byte[] encodedSynchronizationPointName(String label) {
         EncoderFactory encoder = new MockRtiFactory().getEncoderFactory();
         return encoder.createHLAunicodeString(label == null ? "" : label).toByteArray();
      }

      private static String decodeSynchronizationPointName(ParameterHandleValueMap values) {
         if (values == null) return "";
         for (Map.Entry<ParameterHandle, byte[]> entry : values.entrySet()) {
            if (!handleKey(entry.getKey()).endsWith(":HLAsyncPointName")
                  || entry.getValue() == null) continue;
            try {
               EncoderFactory encoder = new MockRtiFactory().getEncoderFactory();
               HLAunicodeString name = encoder.createHLAunicodeString();
               name.decode(entry.getValue());
               return name.getValue();
            } catch (Exception ignored) {
               return new String(entry.getValue(), StandardCharsets.UTF_16BE);
            }
         }
         return "";
      }

      @SuppressWarnings("unchecked")
      private static byte[] encodedSynchronizationPointList(String federation) {
         EncoderFactory encoder = new MockRtiFactory().getEncoderFactory();
         DataElementFactory<HLAunicodeString> factory =
               (DataElementFactory<HLAunicodeString>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
         @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAunicodeString();
                           }
                           return null;
                        }
                     });
         HLAvariableArray<HLAunicodeString> labels = encoder.createHLAvariableArray(factory);
         for (SyncPointRecord record : SYNCHRONIZATION_POINTS.values()) {
            if (sameFederation(record.federation, federation)) {
               labels.addElement(encoder.createHLAunicodeString(record.label));
            }
         }
         return labels.toByteArray();
      }

      @SuppressWarnings("unchecked")
      private static byte[] encodedSynchronizationPointFederates(
            String federation, String label) {
         EncoderFactory encoder = new MockRtiFactory().getEncoderFactory();
         DataElementFactory<HLAoctet> octetFactory =
               (DataElementFactory<HLAoctet>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAoctet((byte) 0);
                           }
                           return null;
                        }
                     });
         DataElementFactory<HLAfixedRecord> recordFactory =
               (DataElementFactory<HLAfixedRecord>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAfixedRecord();
                           }
                           return null;
                        }
                     });
         HLAvariableArray<HLAfixedRecord> records = encoder.createHLAvariableArray(recordFactory);
         SyncPointRecord point = SYNCHRONIZATION_POINTS.get(
               synchronizationPointKey(federation, label));
         if (point == null) return records.toByteArray();
         for (AmbassadorHandler federate : point.participants) {
            HLAvariableArray<HLAoctet> encodedFederate = encoder.createHLAvariableArray(octetFactory);
            for (byte value : encoded(federate.federateHandle)) {
               encodedFederate.addElement(encoder.createHLAoctet(value));
            }
            HLAfixedRecord record = encoder.createHLAfixedRecord();
            record.add(encodedFederate);
            record.add(encoder.createHLAinteger32BE(point.achieved.contains(federate) ? 3 : 2));
            records.addElement(record);
         }
         return records.toByteArray();
      }

      private void notifyMomPublicationReports(AmbassadorHandler requestedFederate) {
         if (requestedFederate == null) return;
         InteractionClassHandle interactionReportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportInteractionPublication");
         ParameterHandleValueMap interactionValues = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         interactionValues.put(parameterHandle(interactionReportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         interactionValues.put(parameterHandle(interactionReportClass, "HLAinteractionClassList"),
               encodedInteractionClassList(requestedFederate.publishedInteractions));
         deliverMomTextReport(interactionReportClass, interactionValues);
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectClassPublication");
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         List<Map.Entry<String, Set<AttributeHandle>>> publications = new ArrayList<>();
         for (Map.Entry<String, Set<AttributeHandle>> entry
               : requestedFederate.publishedObjectAttributes.entrySet()) {
            if (!entry.getValue().isEmpty()) publications.add(entry);
         }
         if (publications.isEmpty()) {
            deliverMomPublicationReport(reportClass, requestedFederate, null, null, 0,
                  transport, supplement);
            return;
         }
         int classCount = publications.size();
         for (Map.Entry<String, Set<AttributeHandle>> publication : publications) {
            ObjectClassHandle objectClass = objectClassHandle(
                  publication.getKey().substring("class:".length()));
            deliverMomPublicationReport(reportClass, requestedFederate, objectClass,
                  publication.getValue(), classCount, transport, supplement);
         }
      }

      private void deliverMomPublicationReport(InteractionClassHandle reportClass,
            AmbassadorHandler requestedFederate, ObjectClassHandle objectClass,
            Set<AttributeHandle> attributes, int classCount, Object transport,
            Object supplement) {
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         values.put(parameterHandle(reportClass, "HLAnumberOfClasses"),
               ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(classCount).array());
         if (objectClass != null && attributes != null) {
            values.put(parameterHandle(reportClass, "HLAobjectClass"), encoded(objectClass));
            values.put(parameterHandle(reportClass, "HLAattributeList"),
                  encodedAttributeList(attributes));
         }
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture MOM publication report callback failed", error);
            }
         }
      }

      private static byte[] encodedAttributeList(Set<AttributeHandle> attributes) {
         int length = 0;
         List<byte[]> encoded = new ArrayList<>();
         for (AttributeHandle attribute : attributes) {
            byte[] bytes = encoded(attribute);
            encoded.add(bytes);
            length += bytes.length;
         }
         byte[] result = new byte[length];
         int offset = 0;
         for (byte[] bytes : encoded) {
            System.arraycopy(bytes, 0, result, offset, bytes.length);
            offset += bytes.length;
         }
         return result;
      }

      private static byte[] encodedInteractionClassList(Set<String> interactions) {
         if (interactions == null || interactions.isEmpty()) return new byte[0];
         List<byte[]> encoded = new ArrayList<>();
         int length = 0;
         for (String interaction : interactions) {
            byte[] bytes = encoded(interactionClassHandle(
                  interaction.substring("interaction:".length())));
            encoded.add(bytes);
            length += bytes.length;
         }
         byte[] result = new byte[length];
         int offset = 0;
         for (byte[] bytes : encoded) {
            System.arraycopy(bytes, 0, result, offset, bytes.length);
            offset += bytes.length;
         }
         return result;
      }

      @SuppressWarnings("unchecked")
      private static byte[] encodedInteractionSubscriptionList(Set<String> interactions) {
         if (interactions == null || interactions.isEmpty()) return new byte[0];
         EncoderFactory encoder = new MockRtiFactory().getEncoderFactory();
         DataElementFactory<HLAoctet> octetFactory =
               (DataElementFactory<HLAoctet>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAoctet((byte) 0);
                           }
                           return null;
                        }
                     });
         DataElementFactory<HLAfixedRecord> recordFactory =
               (DataElementFactory<HLAfixedRecord>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAfixedRecord();
                           }
                           return null;
                        }
                     });
         HLAvariableArray<HLAfixedRecord> records = null;
         for (String interaction : interactions) {
            HLAvariableArray<HLAoctet> encodedInteraction =
                  encoder.createHLAvariableArray(octetFactory);
            for (byte value : encoded(interactionClassHandle(
                  interaction.substring("interaction:".length())))) {
               encodedInteraction.addElement(encoder.createHLAoctet(value));
            }
            HLAfixedRecord record = encoder.createHLAfixedRecord();
            record.add(encodedInteraction);
            record.add(encoder.createHLAboolean(true));
            if (records == null) records = encoder.createHLAvariableArray(recordFactory, record);
            else records.addElement(record);
         }
         return records == null ? new byte[0] : records.toByteArray();
      }

      private void notifyMomSubscriptionReports(AmbassadorHandler requestedFederate) {
         if (requestedFederate == null) return;
         InteractionClassHandle interactionReportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportInteractionSubscription");
         ParameterHandleValueMap interactionValues = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         interactionValues.put(parameterHandle(interactionReportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         interactionValues.put(parameterHandle(interactionReportClass, "HLAinteractionClassList"),
               encodedInteractionSubscriptionList(requestedFederate.subscribedInteractions));
         deliverMomTextReport(interactionReportClass, interactionValues);
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectClassSubscription");
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         List<Map.Entry<String, Set<AttributeHandle>>> subscriptions = new ArrayList<>();
         for (Map.Entry<String, Set<AttributeHandle>> entry
               : requestedFederate.subscribedObjectAttributes.entrySet()) {
            if (!entry.getValue().isEmpty()) subscriptions.add(entry);
         }
         if (subscriptions.isEmpty()) {
            deliverMomSubscriptionReport(reportClass, requestedFederate, null, null, 0,
                  transport, supplement);
            return;
         }
         int classCount = subscriptions.size();
         for (Map.Entry<String, Set<AttributeHandle>> subscription : subscriptions) {
            ObjectClassHandle objectClass = objectClassHandle(
                  subscription.getKey().substring("class:".length()));
            deliverMomSubscriptionReport(reportClass, requestedFederate, objectClass,
                  subscription.getValue(), classCount, transport, supplement);
         }
      }

      private void notifyMomObjectInstanceInformation(ParameterHandleValueMap requestValues) {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectInstanceInformation");
         ObjectRecord record = findObjectByEncodedHandle(requestValues);
         AmbassadorHandler requestedFederate = record == null
               ? resolveRequestedFederate(requestValues) : findFederate(record.owner);
         if (requestedFederate == null) return;
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         byte[] requestedObject = findRequestedObjectHandle(requestValues, record);
         values.put(parameterHandle(reportClass, "HLAobjectInstance"), requestedObject);
         values.put(parameterHandle(reportClass, "HLAownedInstanceAttributeList"),
               record == null ? new byte[0]
                     : encodedOwnedAttributeList(requestedFederate, record.objectClass));
         if (record != null) {
            values.put(parameterHandle(reportClass, "HLAregisteredClass"),
                  encoded(record.objectClass));
            values.put(parameterHandle(reportClass, "HLAknownClass"), encoded(record.objectClass));
         }
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException(
                     "fixture MOM object-information report callback failed", error);
            }
         }
      }

      private void notifyMomObjectInstancesThatCanBeDeleted(
            AmbassadorHandler requestedFederate) {
         if (requestedFederate == null) return;
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectInstancesThatCanBeDeleted");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         values.put(parameterHandle(reportClass, "HLAobjectInstanceCounts"),
               encodedObjectClassCounts(requestedFederate));
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException(
                     "fixture MOM deletable-object report callback failed", error);
            }
         }
      }

      private void notifyMomObjectInstanceCounts(AmbassadorHandler requestedFederate,
            String reportName) {
         if (requestedFederate == null) return;
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport." + reportName);
         Map<String, Integer> counts = new LinkedHashMap<>();
         Map<String, Set<String>> instances = reportName.equals("HLAreportObjectInstancesUpdated")
               ? requestedFederate.updatedObjectInstances
               : requestedFederate.reflectedObjectInstances;
         for (Map.Entry<String, Set<String>> entry : instances.entrySet()) {
            if (!entry.getValue().isEmpty()) counts.put(entry.getKey(), entry.getValue().size());
         }
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         values.put(parameterHandle(reportClass, "HLAobjectInstanceCounts"),
               encodedObjectClassCounts(counts));
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException(
                     "fixture MOM object-instance count report callback failed", error);
            }
         }
      }

      private void notifyMomTransportCounts(AmbassadorHandler requestedFederate,
            String reportName, String countsName,
            Map<String, Map<String, Integer>> ledger) {
         if (requestedFederate == null) return;
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport." + reportName);
         Object transport = handle(TransportationTypeHandle.class,
               "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         // HLAstandardMIM requires one response per supported transportation,
         // including an empty array for a bucket with no activity.
         for (String transportation : new String[] {"HLAreliable", "HLAbestEffort"}) {
            Map<String, Integer> counts = ledger == null ? null : ledger.get(transportation);
            if (counts == null) counts = new LinkedHashMap<>();
            ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
                  new MapHandler(ParameterHandleValueMap.class));
            values.put(parameterHandle(reportClass, "HLAtransportation"),
                  encoded(handle(TransportationTypeHandle.class, "transport:" + transportation)));
            values.put(parameterHandle(reportClass, countsName), encodedObjectClassCounts(counts));
            for (AmbassadorHandler target : HANDLERS) {
               if (!target.connected || !target.joined
                     || !sameFederation(target.federation, federation)
                     || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
               try {
                  target.callback.receiveInteraction(reportClass, values, new byte[0],
                        OrderType.RECEIVE, (TransportationTypeHandle) transport,
                        (FederateAmbassador.SupplementalReceiveInfo) supplement);
               } catch (Exception error) {
                  throw new IllegalStateException(
                        "fixture MOM transportation-count report callback failed", error);
               }
            }
         }
      }

      private void notifyMomInteractionCounts(AmbassadorHandler requestedFederate,
            String reportName, Map<String, Map<String, Integer>> ledger) {
         if (requestedFederate == null) return;
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport." + reportName);
         Object transport = handle(TransportationTypeHandle.class,
               "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         for (String transportation : new String[] {"HLAreliable", "HLAbestEffort"}) {
            Map<String, Integer> counts = ledger == null ? null : ledger.get(transportation);
            if (counts == null) counts = new LinkedHashMap<>();
            ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
                  new MapHandler(ParameterHandleValueMap.class));
            values.put(parameterHandle(reportClass, "HLAtransportation"),
                  encoded(handle(TransportationTypeHandle.class, "transport:" + transportation)));
            values.put(parameterHandle(reportClass, "HLAinteractionCounts"),
                  encodedInteractionCounts(counts));
            for (AmbassadorHandler target : HANDLERS) {
               if (!target.connected || !target.joined
                     || !sameFederation(target.federation, federation)
                     || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
               try {
                  target.callback.receiveInteraction(reportClass, values, new byte[0],
                        OrderType.RECEIVE, (TransportationTypeHandle) transport,
                        (FederateAmbassador.SupplementalReceiveInfo) supplement);
               } catch (Exception error) {
                  throw new IllegalStateException(
                        "fixture MOM interaction-count report callback failed", error);
               }
            }
         }
      }

      private void notifyMomFederateFomModuleReport(AmbassadorHandler requestedFederate,
            ParameterHandleValueMap requestValues) {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAFOMmoduleIndicator"),
               ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN)
                     .putInt(requestedFomModuleIndex(requestValues)).array());
         values.put(parameterHandle(reportClass, "HLAFOMmoduleData"),
               encodedUnicode(FEDERATION_FOM_DATA.getOrDefault(federation, "")));
         deliverMomTextReport(reportClass, values);
      }

      private void notifyMomFederationFomModuleReport(ParameterHandleValueMap requestValues) {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAFOMmoduleIndicator"),
               ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN)
                     .putInt(requestedFomModuleIndex(requestValues)).array());
         values.put(parameterHandle(reportClass, "HLAFOMmoduleData"),
               encodedUnicode(FEDERATION_FOM_DATA.getOrDefault(federation, "")));
         deliverMomTextReport(reportClass, values);
      }

      private void notifyMomFederationMimDataReport() {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAMIMdata"),
               encodedUnicode(FEDERATION_MIM_DATA.getOrDefault(federation, "")));
         deliverMomTextReport(reportClass, values);
      }

      private void deliverMomTextReport(InteractionClassHandle reportClass,
            ParameterHandleValueMap values) {
         Object transport = handle(TransportationTypeHandle.class,
               "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture MOM text-report callback failed", error);
            }
         }
      }

      private static int requestedFomModuleIndex(ParameterHandleValueMap values) {
         if (values != null) {
            for (Map.Entry<ParameterHandle, byte[]> entry : values.entrySet()) {
               if (handleKey(entry.getKey()).endsWith(":HLAFOMmoduleIndicator")
                     && entry.getValue() != null && entry.getValue().length >= 4) {
                  return ByteBuffer.wrap(entry.getValue()).order(ByteOrder.BIG_ENDIAN).getInt();
               }
            }
         }
         return 0;
      }

      private String transportationNameForUpdate(ObjectRecord record, Object values) {
         if (values instanceof AttributeHandleValueMap) {
            for (Object attribute : ((AttributeHandleValueMap) values).keySet()) {
               String configured = attributeTransportation.get(attributeKey(record.object, attribute));
               if (configured != null) return configured;
            }
         }
         return "HLAreliable";
      }

      private static String attributeKey(Object object, Object attribute) {
         return handleKey(object) + ":" + handleKey(attribute);
      }

      private static void incrementTransportCount(
            Map<String, Map<String, Integer>> ledger,
            String transportation, String objectClass) {
         ledger.computeIfAbsent(transportation, ignored -> new LinkedHashMap<>())
               .merge(objectClass, 1, Integer::sum);
      }

      private static boolean isApplicationInteraction(String key) {
         return key != null && key.startsWith("interaction:")
               && !key.contains(".HLAmanager.");
      }

      @SuppressWarnings("unchecked")
      private static byte[] encodedObjectClassCounts(AmbassadorHandler federate) {
         Map<String, Integer> counts = new LinkedHashMap<>();
         for (ObjectRecord record : OBJECTS.values()) {
            if (!record.mom && sameFederation(record.federation, federate.federation)
                  && federate.federateHandle.equals(record.owner)) {
               counts.merge(handleKey(record.objectClass), 1, Integer::sum);
            }
         }
         return encodedObjectClassCounts(counts);
      }

      @SuppressWarnings("unchecked")
      private static byte[] encodedObjectClassCounts(Map<String, Integer> counts) {
         if (counts == null || counts.isEmpty()) return new byte[0];
         EncoderFactory encoder = new MockRtiFactory().getEncoderFactory();
         DataElementFactory<HLAoctet> octetFactory =
               (DataElementFactory<HLAoctet>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAoctet((byte) 0);
                           }
                           return null;
                        }
                     });
         DataElementFactory<HLAfixedRecord> recordFactory =
               (DataElementFactory<HLAfixedRecord>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAfixedRecord();
                           }
                           return null;
                        }
                     });
         try {
            HLAvariableArray<HLAfixedRecord> records = null;
            for (Map.Entry<String, Integer> entry : counts.entrySet()) {
               HLAvariableArray<HLAoctet> encodedObjectClass =
                     encoder.createHLAvariableArray(octetFactory);
               byte[] classBytes = encoded(objectClassHandle(
                     entry.getKey().substring("class:".length())));
               for (byte value : classBytes) {
                  encodedObjectClass.addElement(encoder.createHLAoctet(value));
               }
               HLAfixedRecord record = encoder.createHLAfixedRecord();
               record.add(encodedObjectClass);
               record.add(encoder.createHLAinteger32BE(entry.getValue()));
               if (records == null) {
                  records = encoder.createHLAvariableArray(recordFactory, record);
               } else {
                  records.addElement(record);
               }
            }
            return records == null ? new byte[0] : records.toByteArray();
         } catch (Exception error) {
            throw new IllegalStateException("fixture MOM object-count encoding failed", error);
         }
      }

      @SuppressWarnings("unchecked")
      private static byte[] encodedInteractionCounts(Map<String, Integer> counts) {
         if (counts == null || counts.isEmpty()) return new byte[0];
         EncoderFactory encoder = new MockRtiFactory().getEncoderFactory();
         DataElementFactory<HLAoctet> octetFactory =
               (DataElementFactory<HLAoctet>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAoctet((byte) 0);
                           }
                           return null;
                        }
                     });
         DataElementFactory<HLAfixedRecord> recordFactory =
               (DataElementFactory<HLAfixedRecord>) proxy(DataElementFactory.class,
                     new InvocationHandler() {
                        @Override public Object invoke(Object proxy, Method method, Object[] args) {
                           if (method.getName().equals("createElement")) {
                              return encoder.createHLAfixedRecord();
                           }
                           return null;
                        }
                     });
         try {
            HLAvariableArray<HLAfixedRecord> records = null;
            for (Map.Entry<String, Integer> entry : counts.entrySet()) {
               HLAvariableArray<HLAoctet> encodedInteraction =
                     encoder.createHLAvariableArray(octetFactory);
               byte[] classBytes = encoded(interactionClassHandle(
                     entry.getKey().substring("interaction:".length())));
               for (byte value : classBytes) {
                  encodedInteraction.addElement(encoder.createHLAoctet(value));
               }
               HLAfixedRecord record = encoder.createHLAfixedRecord();
               record.add(encodedInteraction);
               record.add(encoder.createHLAinteger32BE(entry.getValue()));
               if (records == null) {
                  records = encoder.createHLAvariableArray(recordFactory, record);
               } else {
                  records.addElement(record);
               }
            }
            return records == null ? new byte[0] : records.toByteArray();
         } catch (Exception error) {
            throw new IllegalStateException("fixture MOM interaction-count encoding failed", error);
         }
      }

      private static ObjectRecord findObjectByEncodedHandle(ParameterHandleValueMap requestValues) {
         if (requestValues == null) return null;
         for (byte[] requested : requestValues.values()) {
            if (requested == null) continue;
            for (ObjectRecord record : OBJECTS.values()) {
               if (Arrays.equals(requested, encoded(record.object))) return record;
            }
         }
         return null;
      }

      private static byte[] findRequestedObjectHandle(ParameterHandleValueMap requestValues,
            ObjectRecord record) {
         if (record != null) return encoded(record.object);
         if (requestValues != null) {
            for (Map.Entry<ParameterHandle, byte[]> entry : requestValues.entrySet()) {
               if (handleKey(entry.getKey()).endsWith(":HLAobjectInstance")
                     && entry.getValue() != null) return entry.getValue();
            }
         }
         return new byte[0];
      }

      private static AmbassadorHandler findFederate(FederateHandle handle) {
         for (AmbassadorHandler candidate : HANDLERS) {
            if (candidate.federateHandle != null && candidate.federateHandle.equals(handle)) {
               return candidate;
            }
         }
         return null;
      }

      private static byte[] encodedOwnedAttributeList(AmbassadorHandler federate,
            ObjectClassHandle objectClass) {
         Set<AttributeHandle> attributes = federate.publishedObjectAttributes.get(
               handleKey(objectClass));
         return attributes == null ? new byte[0] : encodedAttributeList(attributes);
      }

      private void notifyMomException(String serviceName, String exceptionName) {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAfederate"), encoded(federateHandle));
         values.put(parameterHandle(reportClass, "HLAservice"),
               serviceName.getBytes(StandardCharsets.UTF_8));
         values.put(parameterHandle(reportClass, "HLAexception"),
               exceptionName.getBytes(StandardCharsets.UTF_8));
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture MOM exception report callback failed", error);
            }
         }
      }

      private void notifyMomMOMException(AmbassadorHandler requestedFederate,
            String serviceName) {
         notifyMomMOMException(requestedFederate, serviceName, true);
      }

      private void notifyMomMOMException(AmbassadorHandler requestedFederate,
            String serviceName, boolean parameterError) {
         InteractionClassHandle reportClass = interactionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         values.put(parameterHandle(reportClass, "HLAservice"),
               serviceName.getBytes(StandardCharsets.UTF_8));
         values.put(parameterHandle(reportClass, "HLAexception"),
               "missing required MOM parameter".getBytes(StandardCharsets.UTF_8));
         values.put(parameterHandle(reportClass, "HLAparameterError"),
               new byte[] {(byte) (parameterError ? 1 : 0)});
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReceiveInfo.class,
               new SupplementalReceiveHandler());
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture MOM exception interaction callback failed", error);
            }
         }
      }

      private void deliverMomSubscriptionReport(InteractionClassHandle reportClass,
            AmbassadorHandler requestedFederate, ObjectClassHandle objectClass,
            Set<AttributeHandle> attributes, int classCount, Object transport,
            Object supplement) {
         ParameterHandleValueMap values = proxy(ParameterHandleValueMap.class,
               new MapHandler(ParameterHandleValueMap.class));
         values.put(parameterHandle(reportClass, "HLAfederate"),
               encoded(requestedFederate.federateHandle));
         values.put(parameterHandle(reportClass, "HLAnumberOfClasses"),
               ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(classCount).array());
         if (objectClass != null && attributes != null) {
            values.put(parameterHandle(reportClass, "HLAobjectClass"), encoded(objectClass));
            values.put(parameterHandle(reportClass, "HLAactive"), new byte[] {1});
            values.put(parameterHandle(reportClass, "HLAmaxUpdateRate"),
                  "fixture".getBytes(StandardCharsets.UTF_8));
            values.put(parameterHandle(reportClass, "HLAattributeList"),
                  encodedAttributeList(attributes));
         }
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, federation)
                  || !target.subscribedInteractions.contains(handleKey(reportClass))) continue;
            try {
               target.callback.receiveInteraction(reportClass, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReceiveInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture MOM subscription report callback failed", error);
            }
         }
      }

      private void notifyExistingMomObjects(ObjectClassHandle subscribedClass,
            AttributeHandleSet requestedAttributes) {
         String subscribedClassKey = handleKey(subscribedClass);
         boolean federateClass = "class:HLAobjectRoot.HLAmanager.HLAfederate".equals(
               subscribedClassKey);
         boolean federationClass = "class:HLAobjectRoot.HLAmanager.HLAfederation".equals(
               subscribedClassKey);
         if (!federateClass && !federationClass) {
            return;
         }
         if (callback == null) return;
         Object transport = handle(TransportationTypeHandle.class, "transport:HLAdefaultReliable");
         Object supplement = proxy(FederateAmbassador.SupplementalReflectInfo.class,
               new SupplementalReflectHandler());
         for (ObjectRecord record : new ArrayList<>(OBJECTS.values())) {
            if (!record.mom || !sameFederation(record.federation, federation)
                  || !subscribedClassKey.equals(handleKey(record.objectClass))) continue;
            try {
               callback.discoverObjectInstance(record.object, record.objectClass, record.name);
               AttributeHandleValueMap values = proxy(AttributeHandleValueMap.class,
                     new MapHandler(AttributeHandleValueMap.class));
               if (federateClass) {
                  AttributeHandle nameAttribute = attributeHandle(record.objectClass, "HLAfederateName");
                  if (requestedAttributes == null || requestedAttributes.contains(nameAttribute)) {
                     values.put(nameAttribute,
                           record.name.substring("MOM:".length()).getBytes(StandardCharsets.UTF_8));
                  }
                  AttributeHandle versionAttribute = attributeHandle(record.objectClass, "HLARTIversion");
                  if (requestedAttributes == null || requestedAttributes.contains(versionAttribute)) {
                     values.put(versionAttribute, "fixture".getBytes(StandardCharsets.UTF_8));
                  }
               } else {
                  AttributeHandle federationName = attributeHandle(record.objectClass, "HLAfederationName");
                  if (requestedAttributes == null || requestedAttributes.contains(federationName)) {
                     values.put(federationName, record.federation.getBytes(StandardCharsets.UTF_8));
                  }
               }
               callback.reflectAttributeValues(record.object, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReflectInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture MOM object callback failed", error);
            }
         }
      }

      /**
       * Arm the bounded standard-MIM periodic projection for this joined
       * federate.  The fixture deliberately schedules only the two
       * provider-neutral time attributes; the complete MOM periodic matrix is
       * outside this portable conformance slice.
       */
      private synchronized void armMomTiming(int seconds) {
         cancelMomTiming();
         momReportPeriod = seconds;
         if (seconds == 0 || !connected || !joined) {
            return;
         }
         momTimingTask = MOM_SCHEDULER.scheduleAtFixedRate(() -> {
            if (!connected || !joined || momReportPeriod <= 0) return;
            try {
               queueOrDeliverMomPeriodic(this);
            } catch (Throwable ignored) {
               // A fixture scheduler must not terminate the shared executor
               // because a test callback rejected one periodic projection.
            }
         }, seconds, seconds, TimeUnit.SECONDS);
      }

      private synchronized void cancelMomTiming() {
         momReportPeriod = 0;
         if (momTimingTask != null) {
            momTimingTask.cancel(false);
            momTimingTask = null;
         }
         for (AmbassadorHandler target : HANDLERS) {
            synchronized (target.pendingMomPeriodicRequests) {
               target.pendingMomPeriodicRequests.removeIf(requested -> requested == this);
            }
         }
      }

      private boolean drainMomPeriodicCallback() {
         AmbassadorHandler requested = null;
         synchronized (pendingMomPeriodicRequests) {
            if (!pendingMomPeriodicRequests.isEmpty()) {
               requested = pendingMomPeriodicRequests.remove(0);
            }
         }
         if (requested == null) return false;
         deliverMomPeriodicReflection(requested, this);
         return true;
      }

      private static void queueOrDeliverMomPeriodic(AmbassadorHandler requested) {
         String objectClassKey = "class:HLAobjectRoot.HLAmanager.HLAfederate";
         for (AmbassadorHandler target : HANDLERS) {
            if (!target.connected || !target.joined
                  || !sameFederation(target.federation, requested.federation)
                  || !target.subscribedClasses.contains(objectClassKey)) continue;
            if (target.callbackModel.contains("EVOKED")) {
               synchronized (target.pendingMomPeriodicRequests) {
                  target.pendingMomPeriodicRequests.add(requested);
               }
            } else {
               deliverMomPeriodicReflection(requested, target);
            }
         }
      }

      private static void deliverMomPeriodicReflection(AmbassadorHandler requested,
            AmbassadorHandler target) {
         if (requested == null || target == null || target.callback == null
               || !requested.connected || !requested.joined
               || !target.connected || !target.joined
               || !sameFederation(target.federation, requested.federation)) return;
         ObjectClassHandle federateClass = objectClassHandle(
               "HLAobjectRoot.HLAmanager.HLAfederate");
         String classKey = handleKey(federateClass);
         Set<AttributeHandle> subscribedAttributes = target.subscribedObjectAttributes.get(classKey);
         for (ObjectRecord record : new ArrayList<>(OBJECTS.values())) {
            if (!record.mom || !sameFederation(record.federation, requested.federation)
                  || !("MOM:" + requested.federateName).equals(record.name)
                  || !classKey.equals(handleKey(record.objectClass))) continue;
            AttributeHandleValueMap values = proxy(AttributeHandleValueMap.class,
                  new MapHandler(AttributeHandleValueMap.class));
            AttributeHandle logicalTime = attributeHandle(record.objectClass, "HLAlogicalTime");
            if (subscribedAttributes == null || subscribedAttributes.contains(logicalTime)) {
               values.put(logicalTime, ByteBuffer.allocate(8).order(ByteOrder.BIG_ENDIAN)
                     .putLong(timeWireValue(requested, requested.logicalTime)).array());
            }
            AttributeHandle lookahead = attributeHandle(record.objectClass, "HLAlookahead");
            if (subscribedAttributes == null || subscribedAttributes.contains(lookahead)) {
               values.put(lookahead, ByteBuffer.allocate(8).order(ByteOrder.BIG_ENDIAN)
                     .putLong(timeWireValue(requested, requested.lookahead)).array());
            }
            if (values.isEmpty()) return;
            Object transport = handle(TransportationTypeHandle.class,
                  "transport:HLAdefaultReliable");
            Object supplement = proxy(FederateAmbassador.SupplementalReflectInfo.class,
                  new SupplementalReflectHandler());
            try {
               target.callback.reflectAttributeValues(record.object, values, new byte[0],
                     OrderType.RECEIVE, (TransportationTypeHandle) transport,
                     (FederateAmbassador.SupplementalReflectInfo) supplement);
            } catch (Exception error) {
               throw new IllegalStateException("fixture periodic MOM callback failed", error);
            }
         }
      }

      private static long timeWireValue(AmbassadorHandler handler, double value) {
         return handler.usesFloatTime() ? Double.doubleToLongBits(value) : Math.round(value);
      }
   }

   private static void removeMomObject(String federation, String federateName) {
      if (federation == null || federateName == null) return;
      OBJECTS.entrySet().removeIf(entry -> {
         ObjectRecord record = entry.getValue();
         return record.mom && federation.equals(record.federation)
               && ("MOM:" + federateName).equals(record.name);
      });
   }

   private static void removeFederationObjects(String federation) {
      if (federation == null) return;
      OBJECTS.entrySet().removeIf(entry -> federation.equals(entry.getValue().federation));
   }

   private static final class SyncPointRecord {
      final String federation;
      final String label;
      final Set<AmbassadorHandler> participants = new LinkedHashSet<>();
      final Set<AmbassadorHandler> achieved = new LinkedHashSet<>();

      SyncPointRecord(String federation, String label) {
         this.federation = federation;
         this.label = label;
      }
   }

   private static final class ObjectRecord {
      final ObjectInstanceHandle object;
      final ObjectClassHandle objectClass;
      final String name;
      final String federation;
      final FederateHandle owner;
      final boolean mom;

      ObjectRecord(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, String federation, FederateHandle owner) {
         this(object, objectClass, name, federation, owner, false);
      }

      ObjectRecord(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, String federation, FederateHandle owner, boolean mom) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
         this.federation = federation;
         this.owner = owner;
         this.mom = mom;
      }
   }

   private static final class RegionRecord {
      final RegionHandle region;
      final String federation;
      final Set<DimensionHandle> dimensions = new LinkedHashSet<>();
      final Map<String, RangeBounds> bounds = new HashMap<>();

      RegionRecord(RegionHandle region, String federation, DimensionHandleSet dimensions) {
         this.region = region;
         this.federation = federation;
         if (dimensions != null) this.dimensions.addAll(dimensions);
      }
   }

   private static String lastString(Object[] args) {
      if (args != null) {
         for (int index = args.length - 1; index >= 0; index--) {
            if (args[index] instanceof String) return (String) args[index];
         }
      }
      return "fixture-federation";
   }

   private static String timeImplementationName(Object[] args) {
      if (args != null) {
         for (Object value : args) {
            if (HLAfloat64TimeFactory.NAME.equals(value)) {
               return HLAfloat64TimeFactory.NAME;
            }
            if (HLAinteger64TimeFactory.NAME.equals(value)) {
               return HLAinteger64TimeFactory.NAME;
            }
         }
      }
      return HLAinteger64TimeFactory.NAME;
   }

   private static boolean sameFederation(String left, String right) {
      return left != null && left.equals(right);
   }

   private static ObjectClassHandle objectClassHandle(String name) {
      return (ObjectClassHandle) handle(ObjectClassHandle.class, "class:" + name);
   }

   private static String objectClassName(Object value) {
      String key = handleKey(value);
      return key.startsWith("class:") ? key.substring("class:".length()) : key;
   }

   private static DimensionHandle dimensionHandle(String name) {
      return (DimensionHandle) handle(DimensionHandle.class, "dimension:" + name);
   }

   private static String dimensionName(Object value) {
      String key = handleKey(value);
      return key.startsWith("dimension:") ? key.substring("dimension:".length()) : key;
   }

   private static byte[] encodedUnicode(String value) {
      return (value == null ? "" : value).getBytes(StandardCharsets.UTF_16BE);
   }

   private static String readModuleData(Object value) {
      URL url = null;
      if (value instanceof URL) {
         url = (URL) value;
      } else if (value instanceof URL[]) {
         URL[] urls = (URL[]) value;
         if (urls.length > 0) url = urls[0];
      } else if (value instanceof Object[]) {
         Object[] values = (Object[]) value;
         if (values.length > 0 && values[0] instanceof URL) url = (URL) values[0];
      }
      if (url == null) return "";
      try (InputStream stream = url.openStream()) {
         return new String(stream.readAllBytes(), StandardCharsets.UTF_8);
      } catch (Exception ignored) {
         return "";
      }
   }

   private static String transportationName(Object value) {
      String key = handleKey(value);
      if (key.startsWith("transport:")) key = key.substring("transport:".length());
      if ("HLAdefaultReliable".equals(key) || "HLAreliable".equals(key)) {
         return "HLAreliable";
      }
      if ("HLAdefaultBestEffort".equals(key) || "HLAbestEffort".equals(key)) {
         return "HLAbestEffort";
      }
      return key == null || key.isEmpty() ? "HLAreliable" : key;
   }

   private static long dimensionUpperBound(Object value) {
      switch (dimensionName(value)) {
         case "SodaFlavor": return 3L;
         case "BarQuantity": return 25L;
         case "WaiterId": return 20L;
         default: return 100L;
      }
   }

   private static AttributeHandle attributeHandle(Object objectClass, String name) {
      return (AttributeHandle) handle(
            AttributeHandle.class, "attribute:" + handleKey(objectClass) + ":" + name);
   }

   private static String attributeName(Object objectClass, Object attribute) {
      String key = handleKey(attribute);
      String prefix = "attribute:" + handleKey(objectClass) + ":";
      return key.startsWith(prefix) ? key.substring(prefix.length()) : key;
   }

   private static InteractionClassHandle interactionClassHandle(String name) {
      return (InteractionClassHandle) handle(InteractionClassHandle.class, "interaction:" + name);
   }

   private static String interactionClassName(Object value) {
      String key = handleKey(value);
      return key.startsWith("interaction:") ? key.substring("interaction:".length()) : key;
   }

   private static ParameterHandle parameterHandle(Object interaction, String name) {
      return (ParameterHandle) handle(
            ParameterHandle.class, "parameter:" + handleKey(interaction) + ":" + name);
   }

   private static String parameterName(Object interaction, Object parameter) {
      String key = handleKey(parameter);
      String prefix = "parameter:" + handleKey(interaction) + ":";
      return key.startsWith(prefix) ? key.substring(prefix.length()) : key;
   }

   private static Object handle(Class<?> type, String key) {
      return proxy(type, new HandleHandler(key));
   }

   private static String handleKey(Object value) {
      if (value == null) return "";
      if (Proxy.isProxyClass(value.getClass())) {
         InvocationHandler handler = Proxy.getInvocationHandler(value);
         if (handler instanceof HandleHandler) return ((HandleHandler) handler).key;
      }
      return String.valueOf(value);
   }

   private static byte[] encoded(Object value) {
      if (value instanceof byte[]) return (byte[]) value;
      return String.valueOf(value).getBytes(StandardCharsets.UTF_8);
   }

   private static final class HandleHandler implements InvocationHandler {
      final String key;
      final byte[] bytes;

      HandleHandler(String key) {
         this.key = key;
         this.bytes = key.getBytes(StandardCharsets.UTF_8);
      }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "equals": return args != null && args.length == 1
                  && key.equals(handleKey(args[0]));
            case "hashCode": return Arrays.hashCode(bytes);
            case "encodedLength": return bytes.length;
            case "encode":
               byte[] target = (byte[]) args[0];
               int offset = ((Number) args[1]).intValue();
               System.arraycopy(bytes, 0, target, offset, bytes.length);
               return null;
            case "toString": return key;
            default: return null;
         }
      }
   }

   private static final class HandleFactoryHandler implements InvocationHandler {
      private final Class<?> handleType;

      HandleFactoryHandler(Class<?> handleType) { this.handleType = handleType; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("decode")) {
            byte[] bytes = (byte[]) args[0];
            int offset = ((Number) args[1]).intValue();
            return handle(handleType, new String(bytes, offset, bytes.length - offset, StandardCharsets.UTF_8));
         }
         if (method.getName().equals("toString")) return "Mock handle factory";
         return null;
      }
   }

   private static final class TransportationTypeFactoryHandler implements InvocationHandler {
      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "getHLAdefaultReliable":
               return handle(TransportationTypeHandle.class, "transport:HLAreliable");
            case "getHLAdefaultBestEffort":
               return handle(TransportationTypeHandle.class, "transport:HLAbestEffort");
            case "decode":
               if (args != null && args.length > 0 && args[0] instanceof byte[]) {
                  byte[] bytes = (byte[]) args[0];
                  int offset = args.length > 1 ? ((Number) args[1]).intValue() : 0;
                  return handle(TransportationTypeHandle.class,
                        new String(bytes, offset, bytes.length - offset, StandardCharsets.UTF_8));
               }
               return handle(TransportationTypeHandle.class, "transport:HLAreliable");
            case "toString": return "Mock transportation handle factory";
            default: return defaultCollectionResult(method.getReturnType());
         }
      }
   }

   private static final class SetFactoryHandler implements InvocationHandler {
      private final Class<?> setType;

      SetFactoryHandler(Class<?> setType) { this.setType = setType; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("create")) return proxy(setType, new SetHandler(setType));
         if (method.getName().equals("toString")) return "Mock handle-set factory";
         return null;
      }
   }

   private static final class SetHandler implements InvocationHandler {
      private final Class<?> setType;
      private final Set<Object> values = new LinkedHashSet<>();

      SetHandler(Class<?> setType) { this.setType = setType; }

      SetHandler(Class<?> setType, Iterable<?> initialValues) {
         this.setType = setType;
         if (initialValues != null) {
            for (Object value : initialValues) values.add(value);
         }
      }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "add": return values.add(args[0]);
            case "remove": return values.remove(args[0]);
            case "contains": return values.contains(args[0]);
            case "iterator": return values.iterator();
            case "size": return values.size();
            case "isEmpty": return values.isEmpty();
            case "clear": values.clear(); return null;
            case "toArray": return args == null || args.length == 0
                  ? values.toArray() : values.toArray((Object[]) args[0]);
            case "clone": return proxy(setType, new SetHandler(setType));
            case "toString": return values.toString();
            case "hashCode": return values.hashCode();
            case "equals": return args != null && args.length == 1 && values.equals(args[0]);
            default: return defaultCollectionResult(method.getReturnType());
         }
      }
   }

   private static final class ListFactoryHandler implements InvocationHandler {
      private final Class<?> listType;

      ListFactoryHandler(Class<?> listType) { this.listType = listType; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("create")) {
            return proxy(listType, new ListHandler(listType));
         }
         if (method.getName().equals("toString")) return "Mock attribute-region list factory";
         return defaultCollectionResult(method.getReturnType());
      }
   }

   private static final class ListHandler implements InvocationHandler {
      private final Class<?> listType;
      private final List<Object> values = new ArrayList<>();

      ListHandler(Class<?> listType) { this.listType = listType; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "add":
               if (args != null && args.length == 1) return values.add(args[0]);
               if (args != null && args.length == 2) {
                  values.add(((Number) args[0]).intValue(), args[1]);
                  return null;
               }
               return false;
            case "get": return values.get(((Number) args[0]).intValue());
            case "set": return values.set(((Number) args[0]).intValue(), args[1]);
            case "remove":
               if (args != null && args.length == 1 && args[0] instanceof Number) {
                  return values.remove(((Number) args[0]).intValue());
               }
               return values.remove(args[0]);
            case "size": return values.size();
            case "isEmpty": return values.isEmpty();
            case "contains": return values.contains(args[0]);
            case "iterator": return values.iterator();
            case "listIterator": return values.listIterator();
            case "clear": values.clear(); return null;
            case "toArray":
               return args == null || args.length == 0
                     ? values.toArray() : values.toArray((Object[]) args[0]);
            case "clone":
               ListHandler clone = new ListHandler(listType);
               clone.values.addAll(values);
               return proxy(listType, clone);
            case "toString": return values.toString();
            case "hashCode": return values.hashCode();
            case "equals": return args != null && args.length == 1 && values.equals(args[0]);
            default: return defaultCollectionResult(method.getReturnType());
         }
      }
   }

   private static final class MapFactoryHandler implements InvocationHandler {
      private final Class<?> mapType;

      MapFactoryHandler(Class<?> mapType) { this.mapType = mapType; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("create")) {
            return proxy(mapType, new MapHandler(mapType));
         }
         if (method.getName().equals("toString")) return "Mock attribute-map factory";
         return null;
      }
   }

   private static final class MapHandler implements InvocationHandler {
      private final Class<?> mapType;
      private final Map<Object, byte[]> values = new LinkedHashMap<>();

      MapHandler(Class<?> mapType) { this.mapType = mapType; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "put": return values.put(args[0], (byte[]) args[1]);
            case "get": return values.get(args[0]);
            case "remove": return values.remove(args[0]);
            case "containsKey": return values.containsKey(args[0]);
            case "containsValue": return values.containsValue(args[0]);
            case "entrySet": return values.entrySet();
            case "keySet": return values.keySet();
            case "values": return values.values();
            case "size": return values.size();
            case "isEmpty": return values.isEmpty();
            case "clear": values.clear(); return null;
            case "putAll": values.putAll((Map<?, byte[]>) args[0]); return null;
            case "clone": return proxy(mapType, new MapHandler(mapType));
            case "toString": return values.toString();
            case "hashCode": return values.hashCode();
            case "equals": return args != null && args.length == 1 && values.equals(args[0]);
            case "getValueReference": return null;
            default: return defaultCollectionResult(method.getReturnType());
         }
      }
   }

   private static final class SupplementalReflectHandler implements InvocationHandler {
      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("hasProducingFederate")
               || method.getName().equals("hasSentRegions")) return false;
         if (method.getName().equals("toString")) return "Mock supplemental reflect info";
         return null;
      }
   }

   private static final class SupplementalReceiveHandler implements InvocationHandler {
      private final RegionHandleSet sentRegions;

      SupplementalReceiveHandler() { this(null); }

      SupplementalReceiveHandler(RegionHandleSet sentRegions) {
         this.sentRegions = sentRegions;
      }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("hasProducingFederate")) return false;
         if (method.getName().equals("hasSentRegions")) return sentRegions != null;
         if (method.getName().equals("getSentRegions")) return sentRegions;
         if (method.getName().equals("toString")) return "Mock supplemental receive info";
         return null;
      }
   }

   private static Object defaultCollectionResult(Class<?> type) {
      if (type == boolean.class) return false;
      if (type == int.class) return 0;
      if (type == long.class) return 0L;
      if (type == byte.class) return (byte) 0;
      if (type == short.class) return (short) 0;
      if (type == float.class) return 0.0f;
      if (type == double.class) return 0.0d;
      if (type == char.class) return '\0';
      return null;
   }

   private static final class DefaultHandler implements InvocationHandler {
      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("toString")) return "Mock2010Value";
         Class<?> returnType = method.getReturnType();
         if (returnType == boolean.class) return false;
         if (returnType == int.class) return 0;
         if (returnType == long.class) return 0L;
         if (returnType == void.class) return null;
         return null;
      }
   }

   private static final class EncoderHandler implements InvocationHandler {
      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         if (method.getName().equals("createHLAinteger32BE")) {
            int value = args != null && args.length == 1 ? ((Number) args[0]).intValue() : 0;
            return proxy(HLAinteger32BE.class, new Integer32Handler(value));
         }
         Class<?> returnType = method.getReturnType();
         String returnName = returnType.getSimpleName();
         if (returnType.isInterface() && (returnName.equals("HLAASCIIstring") || returnName.equals("HLAunicodeString"))) {
            Object value = args != null && args.length == 1 ? args[0] : "";
            return proxy(returnType, new StringHandler(returnName, String.valueOf(value)));
         }
         if (returnType.isInterface() && returnName.equals("HLAopaqueData")) {
            byte[] value = args != null && args.length == 1 ? (byte[]) args[0] : new byte[0];
            return proxy(returnType, new OpaqueHandler(value));
         }
         if (returnType.isInterface() && returnName.equals("HLAfixedRecord")) {
            return proxy(returnType, new CollectionHandler("record", null, new ArrayList<>(), -1));
         }
         if (returnType.isInterface() && returnName.equals("HLAfixedArray")) {
            return proxy(returnType, new CollectionHandler("fixed", factoryArgument("fixed", args), elementsArgument("fixed", args), fixedSize(args)));
         }
         if (returnType.isInterface() && returnName.equals("HLAvariableArray")) {
            return proxy(returnType, new CollectionHandler("variable", factoryArgument("variable", args), elementsArgument("variable", args), -1));
         }
         if (returnType.isInterface() && returnName.equals("HLAvariantRecord")) {
            Object discriminant = args != null && args.length == 1 ? args[0] : null;
            return proxy(returnType, new VariantHandler(discriminant));
         }
         if (returnType.isInterface() && isNumericScalar(returnType.getSimpleName())) {
            Object value = args != null && args.length == 1 ? args[0] : null;
            return proxy(returnType, new ScalarHandler(returnType.getSimpleName(), value));
         }
         if (returnType.isInterface() && returnType.getSimpleName().equals("HLAboolean")) {
            Object value = args != null && args.length == 1 ? args[0] : Boolean.FALSE;
            return proxy(returnType, new BooleanHandler(value));
         }
         if (returnType.isInterface()) return proxy(returnType, new DefaultHandler());
         return null;
      }

      private static Object factoryArgument(String kind, Object[] args) {
         if (args == null || args.length == 0) return null;
         if (kind.equals("variable")) return args[0];
         return args.length == 2 && args[1] instanceof Number ? args[0] : null;
      }

      private static List<Object> elementsArgument(String kind, Object[] args) {
         List<Object> elements = new ArrayList<>();
         if (args == null) return elements;
         int start = kind.equals("variable") || (args.length == 2 && args[1] instanceof Number) ? 1 : 0;
         for (int index = start; index < args.length; index++) {
            if (kind.equals("fixed") && args.length == 2 && args[1] instanceof Number) continue;
            if (args[index] instanceof Object[]) {
               elements.addAll(Arrays.asList((Object[]) args[index]));
            } else if (args[index] != null) {
               elements.add(args[index]);
            }
         }
         return elements;
      }

      private static int fixedSize(Object[] args) {
         return args != null && args.length == 2 && args[1] instanceof Number
               ? ((Number) args[1]).intValue() : -1;
      }

      private static boolean isNumericScalar(String name) {
         return name.startsWith("HLAinteger") || name.startsWith("HLAfloat")
               || name.equals("HLAbyte") || name.equals("HLAoctet")
               || name.equals("HLAASCIIchar") || name.equals("HLAunicodeChar")
               || name.startsWith("HLAoctetPair");
      }
   }

   private static final class Integer32Handler implements InvocationHandler {
      private int value;
      Integer32Handler(int value) { this.value = value; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "getValue": return value;
            case "setValue": value = ((Number) args[0]).intValue(); return null;
            case "getEncodedLength": return 4;
            case "getOctetBoundary": return 4;
            case "toByteArray": return ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(value).array();
            case "decode":
               if (args[0] instanceof ByteWrapper) {
                  value = ((ByteWrapper) args[0]).getInt();
                  return null;
               }
               byte[] bytes = args[0] instanceof byte[] ? (byte[]) args[0] : null;
               if (bytes == null || bytes.length != 4) throw new IllegalArgumentException("malformed integer");
               value = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN).getInt();
               return null;
            case "encode":
               if (args[0] != null) {
                  try { args[0].getClass().getMethod("put", byte[].class).invoke(args[0], (Object) toBytes()); }
                  catch (ReflectiveOperationException ignored) {}
               }
               return null;
            case "toString": return Integer.toString(value);
            default: return null;
         }
      }

      private byte[] toBytes() {
         return ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(value).array();
      }
   }

   private static final class ScalarHandler implements InvocationHandler {
      private final String type;
      private Object value;

      ScalarHandler(String type, Object initial) {
         this.type = type;
         this.value = initial == null ? defaultValue(type) : initial;
      }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "getValue": return valueForReturnType();
            case "setValue": value = args[0]; return null;
            case "getEncodedLength": return size();
            case "getOctetBoundary": return size();
            case "toByteArray": return toBytes();
            case "decode":
               value = fromBytes((byte[]) args[0]);
               return null;
            case "encode":
               if (args[0] != null) {
                  try { args[0].getClass().getMethod("put", byte[].class).invoke(args[0], (Object) toBytes()); }
                  catch (ReflectiveOperationException ignored) {}
               }
               return null;
            case "toString": return String.valueOf(value);
            default: return null;
         }
      }

      private static Object defaultValue(String type) {
         return type.startsWith("HLAfloat") ? 0.0d : 0L;
      }

      private int size() {
         if (type.contains("16") || type.equals("HLAunicodeChar") || type.startsWith("HLAoctetPair")) return 2;
         if (type.contains("32")) return 4;
         if (type.contains("64")) return 8;
         return 1;
      }

      private boolean littleEndian() { return type.endsWith("LE"); }

      private Object valueForReturnType() {
         if (type.startsWith("HLAfloat32")) return ((Number) value).floatValue();
         if (type.startsWith("HLAfloat64")) return ((Number) value).doubleValue();
         if (size() == 2) return ((Number) value).shortValue();
         if (size() == 4) return ((Number) value).intValue();
         if (size() == 8) return ((Number) value).longValue();
         return ((Number) value).byteValue();
      }

      private byte[] toBytes() {
         ByteBuffer buffer = ByteBuffer.allocate(size()).order(littleEndian() ? ByteOrder.LITTLE_ENDIAN : ByteOrder.BIG_ENDIAN);
         if (type.startsWith("HLAfloat32")) buffer.putFloat(((Number) value).floatValue());
         else if (type.startsWith("HLAfloat64")) buffer.putDouble(((Number) value).doubleValue());
         else if (size() == 2) buffer.putShort(((Number) value).shortValue());
         else if (size() == 4) buffer.putInt(((Number) value).intValue());
         else if (size() == 8) buffer.putLong(((Number) value).longValue());
         else buffer.put(((Number) value).byteValue());
         return buffer.array();
      }

      private Object fromBytes(byte[] bytes) {
         if (bytes == null || bytes.length != size()) throw new IllegalArgumentException("malformed scalar");
         ByteBuffer buffer = ByteBuffer.wrap(bytes).order(littleEndian() ? ByteOrder.LITTLE_ENDIAN : ByteOrder.BIG_ENDIAN);
         if (type.startsWith("HLAfloat32")) return buffer.getFloat();
         if (type.startsWith("HLAfloat64")) return buffer.getDouble();
         if (size() == 2) return buffer.getShort();
         if (size() == 4) return buffer.getInt();
         if (size() == 8) return buffer.getLong();
         return buffer.get();
      }
   }

   private static final class BooleanHandler implements InvocationHandler {
      private boolean value;
      BooleanHandler(Object initial) { this.value = initial instanceof Boolean && (Boolean) initial; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "getValue": return value;
            case "setValue": value = (Boolean) args[0]; return null;
            case "getEncodedLength": return 1;
            case "getOctetBoundary": return 1;
            case "toByteArray": return new byte[] {(byte) (value ? 1 : 0)};
            case "decode":
               byte[] bytes = (byte[]) args[0];
               if (bytes == null || bytes.length != 1 || (bytes[0] != 0 && bytes[0] != 1)) {
                  throw new IllegalArgumentException("malformed boolean");
               }
               value = bytes[0] != 0;
               return null;
            case "encode":
               if (args[0] != null) {
                  try { args[0].getClass().getMethod("put", int.class).invoke(args[0], value ? 1 : 0); }
                  catch (ReflectiveOperationException ignored) {}
               }
               return null;
            case "toString": return Boolean.toString(value);
            default: return null;
         }
      }
   }

   private static final class StringHandler implements InvocationHandler {
      private final String type;
      private String value;
      StringHandler(String type, String value) { this.type = type; this.value = value; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "getValue": return value;
            case "setValue": value = (String) args[0]; return null;
            case "getEncodedLength": return encoded().length;
            case "getOctetBoundary": return type.equals("HLAunicodeString") ? 2 : 1;
            case "toByteArray": return encoded();
            case "decode":
               byte[] bytes = (byte[]) args[0];
               if (bytes == null || bytes.length < 4) {
                  throw new IllegalArgumentException("malformed variable string");
               }
               int characterCount = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN).getInt();
               int payloadLength = type.equals("HLAunicodeString")
                     ? characterCount * 2 : characterCount;
               if (characterCount < 0 || bytes.length != 4 + payloadLength) {
                  throw new IllegalArgumentException("malformed variable string");
               }
               value = new String(bytes, 4, payloadLength, charset());
               return null;
            case "encode":
               if (args[0] != null) {
                  try { args[0].getClass().getMethod("put", byte[].class).invoke(args[0], (Object) encoded()); }
                  catch (ReflectiveOperationException ignored) {}
               }
               return null;
            case "toString": return value;
            default: return null;
         }
      }

      private java.nio.charset.Charset charset() {
         return type.equals("HLAunicodeString") ? StandardCharsets.UTF_16BE : StandardCharsets.US_ASCII;
      }

      private byte[] encoded() {
         byte[] payload = value.getBytes(charset());
         return ByteBuffer.allocate(4 + payload.length).order(ByteOrder.BIG_ENDIAN)
               .putInt(type.equals("HLAunicodeString") ? value.length() : payload.length)
               .put(payload)
               .array();
      }
   }

   private static final class OpaqueHandler implements InvocationHandler {
      private byte[] value;
      OpaqueHandler(byte[] value) { this.value = value.clone(); }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "size": return value.length;
            case "get": return value[((Number) args[0]).intValue()];
            case "getValue": return value.clone();
            case "setValue": value = ((byte[]) args[0]).clone(); return null;
            case "iterator":
               List<Byte> boxed = new ArrayList<>();
               for (byte item : value) boxed.add(item);
               return boxed.iterator();
            case "getEncodedLength": return value.length;
            case "getOctetBoundary": return 1;
            case "toByteArray": return value.clone();
            case "decode": value = ((byte[]) args[0]).clone(); return null;
            case "encode":
               if (args[0] != null) {
                  try { args[0].getClass().getMethod("put", byte[].class).invoke(args[0], (Object) value); }
                  catch (ReflectiveOperationException ignored) {}
               }
               return null;
            case "toString": return Arrays.toString(value);
            default: return null;
         }
      }
   }

   private static final class CollectionHandler implements InvocationHandler {
      private final String kind;
      private final Object factory;
      private final List<Object> elements;

      CollectionHandler(String kind, Object factory, List<Object> initial, int fixedSize) {
         this.kind = kind;
         this.factory = factory;
         this.elements = new ArrayList<>(initial);
         if (fixedSize >= 0) resize(fixedSize);
      }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "add":
            case "addElement":
               elements.add(args[0]);
               return null;
            case "resize":
               resize(((Number) args[0]).intValue());
               return null;
            case "size": return elements.size();
            case "get": return elements.get(((Number) args[0]).intValue());
            case "iterator": return elements.iterator();
            case "getOctetBoundary": return octetBoundary();
            case "getEncodedLength": return encoded().length;
            case "toByteArray": return encoded();
            case "encode":
               if (args[0] != null) {
                  try { args[0].getClass().getMethod("put", byte[].class).invoke(args[0], (Object) encoded()); }
                  catch (ReflectiveOperationException ignored) {}
               }
               return null;
            case "decode":
               decodeBytes((byte[]) args[0]);
               return null;
            case "toString": return elements.toString();
            default: return null;
         }
      }

      private void resize(int size) {
         if (size < 0) throw new IllegalArgumentException("negative array size");
         while (elements.size() < size) {
            if (factory == null) throw new IllegalArgumentException("no DataElementFactory");
            try {
               elements.add(((DataElementFactory<?>) factory).createElement(elements.size()));
            } catch (Exception error) {
               throw new IllegalArgumentException("fixture element factory failed", error);
            }
         }
         while (elements.size() > size) elements.remove(elements.size() - 1);
      }

      private void decodeBytes(byte[] bytes) {
         if (bytes == null) throw new IllegalArgumentException("null collection encoding");
         decodeAt(bytes, 0, kind.equals("variable") ? -1 : elements.size());
      }

      private int decodeAt(byte[] bytes, int start, int expectedSize) {
         int offset = start;
         int count = expectedSize;
         if (kind.equals("variable")) {
            if (offset + 4 > bytes.length) throw new IllegalArgumentException("truncated variable array");
            count = ByteBuffer.wrap(bytes, offset, 4).order(ByteOrder.BIG_ENDIAN).getInt();
            if (count < 0) throw new IllegalArgumentException("negative variable array size");
            offset += 4;
            elements.clear();
         }
         if (count < 0) throw new IllegalArgumentException("unknown collection size");
         if (!kind.equals("record")) resize(count);
         for (int index = 0; index < count; index++) {
            DataElement element;
            if (kind.equals("record")) {
               element = (DataElement) elements.get(index);
            } else {
               element = (DataElement) elements.get(index);
            }
            int boundary = Math.max(1, element.getOctetBoundary());
            while (offset % boundary != 0) {
               if (offset >= bytes.length) throw new IllegalArgumentException("truncated collection padding");
               offset++;
            }
            offset = decodeElement(element, bytes, offset);
         }
         return offset;
      }

      private static int decodeElement(DataElement element, byte[] bytes, int offset) {
         InvocationHandler handler = Proxy.isProxyClass(element.getClass())
               ? Proxy.getInvocationHandler(element) : null;
         if (handler instanceof CollectionHandler) {
            return ((CollectionHandler) handler).decodeAt(bytes, offset,
                  ((CollectionHandler) handler).kind.equals("variable")
                        ? -1 : ((CollectionHandler) handler).elements.size());
         }
         int length;
         if (handler instanceof StringHandler) {
            if (offset + 4 > bytes.length) throw new IllegalArgumentException("truncated string");
            int count = ByteBuffer.wrap(bytes, offset, 4).order(ByteOrder.BIG_ENDIAN).getInt();
            if (count < 0) throw new IllegalArgumentException("negative string size");
            length = 4 + (((StringHandler) handler).type.equals("HLAunicodeString")
                  ? count * 2 : count);
         } else {
            length = element.getEncodedLength();
         }
         if (length < 0 || offset + length > bytes.length) {
            throw new IllegalArgumentException("truncated collection element");
         }
         byte[] value = Arrays.copyOfRange(bytes, offset, offset + length);
         try {
            element.decode(value);
         } catch (Exception error) {
            throw new IllegalArgumentException("collection element decode failed", error);
         }
         return offset + length;
      }

      private int octetBoundary() {
         if (kind.equals("variable") && factory != null) {
            try {
               Object prototype = ((DataElementFactory<?>) factory).createElement(0);
               return Math.max(1, ((DataElement) prototype).getOctetBoundary());
            } catch (Exception ignored) {
               return 1;
            }
         }
         int boundary = 1;
         for (Object element : elements) {
            boundary = Math.max(boundary, ((DataElement) element).getOctetBoundary());
         }
         return boundary;
      }

      private byte[] encoded() {
         ByteArrayOutputStream output = new ByteArrayOutputStream();
         if (kind.equals("variable")) {
            output.write((elements.size() >>> 24) & 0xff);
            output.write((elements.size() >>> 16) & 0xff);
            output.write((elements.size() >>> 8) & 0xff);
            output.write(elements.size() & 0xff);
         }
         for (Object element : elements) {
            DataElement dataElement = (DataElement) element;
            int boundary = Math.max(1, dataElement.getOctetBoundary());
            while (output.size() % boundary != 0) output.write(0);
            byte[] encoded = dataElement.toByteArray();
            output.write(encoded, 0, encoded.length);
         }
         return output.toByteArray();
      }
   }

   private static final class VariantHandler implements InvocationHandler {
      private Object discriminant;
      private final Map<String, Object> variants = new HashMap<>();
      VariantHandler(Object discriminant) { this.discriminant = discriminant; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "setVariant": variants.put(key(args[0]), args[1]); return null;
            case "setDiscriminant": discriminant = args[0]; return null;
            case "getDiscriminant": return discriminant;
            case "getValue": return variants.get(key(discriminant));
            case "getOctetBoundary": return 1;
            case "getEncodedLength":
               Object value = variants.get(key(discriminant));
               return value == null ? 0 : ((DataElement) value).getEncodedLength();
            case "toByteArray":
               Object element = variants.get(key(discriminant));
               return element == null ? new byte[0] : ((DataElement) element).toByteArray();
            case "encode": return null;
            case "decode": return null;
            case "toString": return String.valueOf(getValue(variants, discriminant));
            default: return null;
         }
      }

      private static String key(Object value) { return String.valueOf(value); }

      private static Object getValue(Map<String, Object> values, Object key) { return values.get(key(key)); }
   }

   /** A deterministic provider-owned integer logical-time fixture. */
   private static final class IntegerTimeFactoryHandler implements InvocationHandler {
      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "getName": return HLAinteger64TimeFactory.NAME;
            case "makeInitial": return integerTime(0L);
            case "makeFinal": return integerTime(Long.MAX_VALUE);
            case "makeZero": return integerInterval(0L);
            case "makeEpsilon": return integerInterval(1L);
            case "makeTime": return integerTime(((Number) args[0]).longValue());
            case "makeInterval": return integerInterval(((Number) args[0]).longValue());
            case "decodeTime": return integerTime(decodeLong((byte[]) args[0], ((Number) args[1]).intValue()));
            case "decodeInterval": return integerInterval(decodeLong((byte[]) args[0], ((Number) args[1]).intValue()));
            case "toString": return "MockHLAinteger64TimeFactory";
            default: return null;
         }
      }
   }

   private static final class IntegerTimeHandler implements InvocationHandler {
      private long value;
      IntegerTimeHandler(long value) { this.value = value; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "isInitial": return value == 0L;
            case "isFinal": return value == Long.MAX_VALUE;
            case "add": return integerTime(value + longValue(args[0]));
            case "subtract": return integerTime(value - longValue(args[0]));
            case "distance": return integerInterval(value - longValue(args[0]));
            case "compareTo": return Long.compare(value, longValue(args[0]));
            case "encodedLength": return 8;
            case "encode":
               writeLong((byte[]) args[0], ((Number) args[1]).intValue(), value);
               return null;
            case "getValue": return value;
            case "toString": return Long.toString(value);
            default: return null;
         }
      }
   }

   private static final class IntegerIntervalHandler implements InvocationHandler {
      private long value;
      IntegerIntervalHandler(long value) { this.value = value; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "isZero": return value == 0L;
            case "isEpsilon": return value == 1L;
            case "add": return integerInterval(value + longValue(args[0]));
            case "subtract": return integerInterval(value - longValue(args[0]));
            case "compareTo": return Long.compare(value, longValue(args[0]));
            case "encodedLength": return 8;
            case "encode":
               writeLong((byte[]) args[0], ((Number) args[1]).intValue(), value);
               return null;
            case "getValue": return value;
            case "toString": return Long.toString(value);
            default: return null;
         }
      }
   }

   /** A deterministic provider-owned float64 logical-time fixture. */
   private static final class FloatTimeFactoryHandler implements InvocationHandler {
      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "getName": return HLAfloat64TimeFactory.NAME;
            case "makeInitial": return floatTime(0.0d);
            case "makeFinal": return floatTime(Double.MAX_VALUE);
            case "makeZero": return floatInterval(0.0d);
            case "makeEpsilon": return floatInterval(Math.nextUp(0.0d));
            case "makeTime": return floatTime(((Number) args[0]).doubleValue());
            case "makeInterval": return floatInterval(((Number) args[0]).doubleValue());
            case "decodeTime": return floatTime(decodeDouble(
                  (byte[]) args[0], ((Number) args[1]).intValue()));
            case "decodeInterval": return floatInterval(decodeDouble(
                  (byte[]) args[0], ((Number) args[1]).intValue()));
            case "toString": return "MockHLAfloat64TimeFactory";
            default: return null;
         }
      }
   }

   private static final class FloatTimeHandler implements InvocationHandler {
      private double value;
      FloatTimeHandler(double value) { this.value = value; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "isInitial": return value == 0.0d;
            case "isFinal": return value == Double.MAX_VALUE;
            case "add": return floatTime(value + numericValue(args[0]));
            case "subtract": return floatTime(value - numericValue(args[0]));
            case "distance": return floatInterval(value - numericValue(args[0]));
            case "compareTo": return Double.compare(value, numericValue(args[0]));
            case "encodedLength": return 8;
            case "encode":
               writeDouble((byte[]) args[0], ((Number) args[1]).intValue(), value);
               return null;
            case "getValue": return value;
            case "toString": return Double.toString(value);
            default: return null;
         }
      }
   }

   private static final class FloatIntervalHandler implements InvocationHandler {
      private double value;
      FloatIntervalHandler(double value) { this.value = value; }

      @Override public Object invoke(Object proxy, Method method, Object[] args) {
         switch (method.getName()) {
            case "isZero": return value == 0.0d;
            case "isEpsilon": return value == Math.nextUp(0.0d);
            case "add": return floatInterval(value + numericValue(args[0]));
            case "subtract": return floatInterval(value - numericValue(args[0]));
            case "compareTo": return Double.compare(value, numericValue(args[0]));
            case "encodedLength": return 8;
            case "encode":
               writeDouble((byte[]) args[0], ((Number) args[1]).intValue(), value);
               return null;
            case "getValue": return value;
            case "toString": return Double.toString(value);
            default: return null;
         }
      }
   }

   private static Object integerTime(long value) {
      return proxy(HLAinteger64Time.class, new IntegerTimeHandler(value));
   }

   private static Object integerInterval(long value) {
      return proxy(HLAinteger64Interval.class, new IntegerIntervalHandler(value));
   }

   private static Object floatTime(double value) {
      return proxy(HLAfloat64Time.class, new FloatTimeHandler(value));
   }

   private static Object floatInterval(double value) {
      return proxy(HLAfloat64Interval.class, new FloatIntervalHandler(value));
   }

   private static double numericValue(Object value) {
      InvocationHandler handler = Proxy.isProxyClass(value.getClass())
            ? Proxy.getInvocationHandler(value) : null;
      if (handler instanceof IntegerTimeHandler) return ((IntegerTimeHandler) handler).value;
      if (handler instanceof IntegerIntervalHandler) return ((IntegerIntervalHandler) handler).value;
      if (handler instanceof FloatTimeHandler) return ((FloatTimeHandler) handler).value;
      if (handler instanceof FloatIntervalHandler) return ((FloatIntervalHandler) handler).value;
      return ((Number) value).doubleValue();
   }

   private static long longValue(Object value) {
      return (long) numericValue(value);
   }

   private static long decodeLong(byte[] bytes, int offset) {
      if (bytes == null || offset < 0 || offset + Long.BYTES > bytes.length) {
         throw new IllegalArgumentException("malformed integer time");
      }
      return ByteBuffer.wrap(bytes, offset, Long.BYTES).order(ByteOrder.BIG_ENDIAN).getLong();
   }

   private static void writeLong(byte[] bytes, int offset, long value) {
      if (bytes == null || offset < 0 || offset + Long.BYTES > bytes.length) {
         throw new IllegalArgumentException("time buffer is too small");
      }
      ByteBuffer.wrap(bytes, offset, Long.BYTES).order(ByteOrder.BIG_ENDIAN).putLong(value);
   }

   private static double decodeDouble(byte[] bytes, int offset) {
      if (bytes == null || offset < 0 || offset + Double.BYTES > bytes.length) {
         throw new IllegalArgumentException("malformed float time");
      }
      return ByteBuffer.wrap(bytes, offset, Double.BYTES).order(ByteOrder.BIG_ENDIAN).getDouble();
   }

   private static void writeDouble(byte[] bytes, int offset, double value) {
      if (bytes == null || offset < 0 || offset + Double.BYTES > bytes.length) {
         throw new IllegalArgumentException("time buffer is too small");
      }
      ByteBuffer.wrap(bytes, offset, Double.BYTES).order(ByteOrder.BIG_ENDIAN).putDouble(value);
   }
}
