package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectClassHandleFactory;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAoctet;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies timestamped reflection delivery and its class-scoped MOM object count. */
final class FederateMomReflectedTimestampedObjectReportTck {
   private static final String REFLECTED_REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected";
   private static final String REFLECTED_REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected";
   private static final String UPDATED_REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated";
   private static final String UPDATED_REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomReflectedTimestampedObjectReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         callbackModel, false);
   }

   static void runUpdated(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         boolean updatedReport) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador monitor = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder requesterRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      Recorder monitorRecorder = new Recorder();
      String federation = "java-tck-federate-mom-reflected-timestamped-report-"
         + UUID.randomUUID();
      boolean requesterConnected = false, subjectConnected = false, monitorConnected = false;
      boolean created = false, requesterJoined = false, subjectJoined = false, monitorJoined = false;
      boolean requesterSubscribed = false, monitorSubscribed = false, subjectPublished = false;
      InteractionClassHandle requestClass = null, reportClass = null;
      ObjectClassHandle requesterObjectClass = null, subjectObjectClass = null;
      AttributeHandle requesterAttribute = null, subjectAttribute = null;
      try {
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         monitor.connect(monitorRecorder.proxy(), callbackModel);
         monitorConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            "mom-reflected-timestamped-target", "java-tck", federation);
         requesterJoined = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-reflected-timestamped-subject", "java-tck", federation);
         subjectJoined = true;
         monitor.joinFederationExecution("mom-reflected-timestamped-monitor", "java-tck",
            federation);
         monitorJoined = true;

         String requestClassName = updatedReport ? UPDATED_REQUEST_CLASS : REFLECTED_REQUEST_CLASS;
         String reportClassName = updatedReport ? UPDATED_REPORT_CLASS : REFLECTED_REPORT_CLASS;
         requestClass = monitor.getInteractionClassHandle(requestClassName);
         ParameterHandle requestFederate = monitor.getParameterHandle(requestClass, "HLAfederate");
         reportClass = monitor.getInteractionClassHandle(reportClassName);
         ParameterHandle reportCounts = monitor.getParameterHandle(reportClass,
            "HLAobjectInstanceCounts");
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(requestClassName.equals(monitor.getInteractionClassName(requestClass))
               && "HLAfederate".equals(monitor.getParameterName(requestClass, requestFederate))
               && reportClassName.equals(monitor.getInteractionClassName(reportClass))
               && "HLAobjectInstanceCounts".equals(monitor.getParameterName(reportClass,
                  reportCounts))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            "standard reflected-object MOM request/report handles did not round-trip");
         monitor.subscribeInteractionClass(reportClass);
         monitorSubscribed = true;

         requesterObjectClass = requester.getObjectClassHandle(objectClassName);
         requesterAttribute = requester.getAttributeHandle(requesterObjectClass, attributeName);
         subjectObjectClass = subject.getObjectClassHandle(objectClassName);
         subjectAttribute = subject.getAttributeHandle(subjectObjectClass, attributeName);
         JavaTckSupport.check(requesterObjectClass.equals(subjectObjectClass)
               && requesterAttribute.equals(subjectAttribute)
               && objectClassName.equals(requester.getObjectClassName(requesterObjectClass))
               && attributeName.equals(requester.getAttributeName(requesterObjectClass,
                  requesterAttribute)),
            "adapter-supplied application handles did not resolve consistently");
         requester.subscribeObjectClassAttributes(requesterObjectClass,
            JavaTckSupport.attributeSet(requester, requesterAttribute), "");
         requesterSubscribed = true;
         subject.publishObjectClassAttributes(subjectObjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute));
         subjectPublished = true;
         subject.changeDefaultAttributeOrderType(subjectObjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute), OrderType.TIMESTAMP);

         int discoveriesBefore = requesterRecorder.discoveries.size();
         ObjectInstanceHandle firstObject = subject.registerObjectInstance(subjectObjectClass);
         ObjectInstanceHandle secondObject = subject.registerObjectInstance(subjectObjectClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "timestamped reflected-object registration returned invalid or duplicate handles");
         await(requester, callbackModel,
            () -> requesterRecorder.discovery(firstObject, discoveriesBefore) != null
               && requesterRecorder.discovery(secondObject, discoveriesBefore) != null,
            "timestamped reflected-object discovery");
         Discovery firstDiscovery = requesterRecorder.discovery(firstObject, discoveriesBefore);
         Discovery secondDiscovery = requesterRecorder.discovery(secondObject, discoveriesBefore);
         JavaTckSupport.check(requesterRecorder.discoveries.size() == discoveriesBefore + 2
               && requesterObjectClass.equals(firstDiscovery.objectClass)
               && requesterObjectClass.equals(secondDiscovery.objectClass)
               && subjectFederate.equals(firstDiscovery.producer)
               && subjectFederate.equals(secondDiscovery.producer)
               && requesterObjectClass.equals(requester.getKnownObjectClassHandle(firstObject))
               && requesterObjectClass.equals(requester.getKnownObjectClassHandle(secondObject)),
            "timestamped object discovery returned incorrect class, producer, or known-object state");

         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(requester.getTimeFactory().getName()),
            "timestamped object-report federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         subject.enableTimeRegulation(epsilon);
         await(subject, callbackModel, () -> subjectRecorder.timeRegulationEnabled != null,
            "subject time regulation enablement");
         requester.enableTimeConstrained();
         await(requester, callbackModel, () -> requesterRecorder.timeConstrainedEnabled != null,
            "requester time constrained enablement");
         LogicalTime messageTime = offset(subject.queryLogicalTime(), epsilon, 3);
         LogicalTime requesterTarget = offset(requester.queryLogicalTime(), epsilon, 3);
         JavaTckSupport.check(messageTime.equals(requesterTarget),
            "timestamped reflected-object sender and receiver derived different times");

         byte[][] values = updatedReport
            ? new byte[][] {{0x41}, {0x42}}
            : new byte[][] {{1}, {2}, {3}};
         byte[][] tags = updatedReport
            ? new byte[][] {{0x51, 0x52}, {0x61, 0x62}}
            : new byte[][] {{0x31, 0x32}, {0x41, 0x42}, {0x51, 0x52}};
         ObjectInstanceHandle[] objects = updatedReport
            ? new ObjectInstanceHandle[] {firstObject, secondObject}
            : new ObjectInstanceHandle[] {firstObject, firstObject, secondObject};
         List<MessageRetractionHandle> retractions = new ArrayList<>();
         for (int i = 0; i < values.length; ++i) {
            AttributeHandleValueMap update = subject.getAttributeHandleValueMapFactory().create(1);
            update.put(subjectAttribute, encoder.createHLAbyte(values[i][0]).toByteArray());
            MessageRetractionReturn result = subject.updateAttributeValues(objects[i], update,
               tags[i], messageTime);
            JavaTckSupport.check(result != null && result.retractionHandleIsValid
                  && result.handle != null,
               "timestamped reflected-object update returned an invalid retraction handle");
            retractions.add(result.handle);
         }
         JavaTckSupport.check(requesterRecorder.reflections.isEmpty(),
            "timestamped reflection arrived before the constrained time advance");
         requester.timeAdvanceRequest(requesterTarget);
         subject.timeAdvanceRequest(messageTime);
         awaitBoth(subject, requester, callbackModel,
            () -> subjectRecorder.timeAdvanceGrant != null
               && requesterRecorder.timeAdvanceGrant != null
               && requesterRecorder.reflections.size() >= values.length,
            "timestamped reflections and time-advance grants");

         List<Reflection> reflections = requesterRecorder.reflections;
         JavaTckSupport.check(reflections.size() == values.length
               && messageTime.equals(subjectRecorder.timeAdvanceGrant)
               && requesterTarget.equals(requesterRecorder.timeAdvanceGrant),
            "timestamped reflection delivered duplicate callbacks or incorrect grants");
         for (int i = 0; i < values.length; ++i) {
            Reflection reflection = findReflection(reflections, objects[i], tags[i]);
            HLAbyte decoded = encoder.createHLAbyte();
            JavaTckSupport.check(reflection != null && reflection.values.size() == 1
                  && reflection.values.containsKey(requesterAttribute)
                  && Arrays.equals(encoder.createHLAbyte(values[i][0]).toByteArray(),
                     reflection.values.get(requesterAttribute))
                  && Arrays.equals(tags[i], reflection.tag)
                  && messageTime.equals(reflection.time)
                  && reflection.sentOrder == OrderType.TIMESTAMP
                  && reflection.receivedOrder == OrderType.TIMESTAMP
                  && subjectFederate.equals(reflection.producer)
                  && reliable.equals(reflection.transportation)
                  && retractions.get(i).equals(reflection.retraction)
                  && (reflection.regions == null || reflection.regions.isEmpty()),
               "timestamped reflection changed its value, tag, logical time, order, or delivery metadata");
            decoded.decode(reflection.values.get(requesterAttribute));
            JavaTckSupport.check(decoded.getValue() == values[i][0],
               "timestamped reflection carried the wrong encoded byte value");
         }
         int firstGrant = requesterRecorder.callbackOrder.indexOf("timeAdvanceGrant");
         JavaTckSupport.check(firstGrant >= values.length
               && requesterRecorder.callbackOrder.subList(0, firstGrant).stream()
                  .allMatch("reflectAttributeValues"::equals)
               && requesterRecorder.callbackOrder.stream()
                  .filter("reflectAttributeValues"::equals).count() == values.length,
            "timestamped reflections did not all precede the requester time-advance grant");

         int reportStart = monitorRecorder.interactions.size();
         ParameterHandleValueMap parameters = monitor.getParameterHandleValueMapFactory().create(1);
         parameters.put(requestFederate,
            encode(updatedReport ? subjectFederate : requesterFederate));
         monitor.sendInteraction(requestClass, parameters, new byte[0]);
         InteractionClassHandle resolvedReportClass = reportClass;
         await(monitor, callbackModel,
            () -> findReport(monitorRecorder, reportStart, resolvedReportClass, reportCounts) != null,
            "timestamped reflected-object MOM report");
         List<Interaction> reports = reportsSince(monitorRecorder, reportStart,
            resolvedReportClass);
         JavaTckSupport.check(reports.size() == 1,
            "timestamped reflected-object request did not deliver exactly one report");
         Interaction report = reports.get(0);
         JavaTckSupport.check(report.parameters.size() == 1
               && report.parameters.containsKey(reportCounts)
               && reliable.equals(report.transportation) && report.tag.length == 0
               && report.producer == null && report.regionsEmpty,
            "timestamped reflected-object report had non-standard response metadata");
         List<ObjectClassCount> counts = decodeObjectClassCounts(encoder,
            monitor.getObjectClassHandleFactory(), report.parameters.get(reportCounts));
         JavaTckSupport.check(counts.size() == 1
               && requesterObjectClass.equals(counts.get(0).objectClass)
               && counts.get(0).count == 2,
            "timestamped reflected-object report did not count the two distinct instances");

         monitor.unsubscribeInteractionClass(reportClass);
         monitorSubscribed = false;
         requester.unsubscribeObjectClassAttributes(requesterObjectClass,
            JavaTckSupport.attributeSet(requester, requesterAttribute));
         requesterSubscribed = false;
         subject.deleteObjectInstance(firstObject, new byte[0]);
         subject.deleteObjectInstance(secondObject, new byte[0]);
         subject.unpublishObjectClassAttributes(subjectObjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute));
         subjectPublished = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (monitorSubscribed && monitorJoined && reportClass != null) try {
            monitor.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (requesterSubscribed && requesterJoined && requesterObjectClass != null
               && requesterAttribute != null) try {
            requester.unsubscribeObjectClassAttributes(requesterObjectClass,
               JavaTckSupport.attributeSet(requester, requesterAttribute));
         } catch (Exception ignored) { }
         if (subjectPublished && subjectJoined && subjectObjectClass != null
               && subjectAttribute != null) try {
            subject.unpublishObjectClassAttributes(subjectObjectClass,
               JavaTckSupport.attributeSet(subject, subjectAttribute));
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester);
         if (subjectJoined) resign(subject);
         if (monitorJoined) resign(monitor);
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (monitorConnected) disconnect(monitor);
         if (subjectConnected) disconnect(subject);
         if (requesterConnected) disconnect(requester);
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static Reflection findReflection(List<Reflection> reflections,
         ObjectInstanceHandle object, byte[] tag) {
      for (Reflection reflection : reflections)
         if (object.equals(reflection.object) && Arrays.equals(tag, reflection.tag)) return reflection;
      return null;
   }

   private static List<ObjectClassCount> decodeObjectClassCounts(EncoderFactory encoder,
         ObjectClassHandleFactory classFactory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAoctet> handlePrototype = encoder.createHLAvariableArray(
         index -> encoder.createHLAoctet(), encoder.createHLAoctet());
      HLAfixedRecord recordPrototype = encoder.createHLAfixedRecord();
      recordPrototype.add(handlePrototype);
      recordPrototype.add(encoder.createHLAinteger32BE());
      HLAvariableArray<HLAfixedRecord> records = encoder.createHLAvariableArray(index -> {
         HLAfixedRecord record = encoder.createHLAfixedRecord();
         record.add(encoder.createHLAvariableArray(
            inner -> encoder.createHLAoctet(), encoder.createHLAoctet()));
         record.add(encoder.createHLAinteger32BE());
         return record;
      }, recordPrototype);
      records.decode(encoded);
      List<ObjectClassCount> result = new ArrayList<>();
      for (HLAfixedRecord record : records) {
         @SuppressWarnings("unchecked")
         HLAvariableArray<HLAoctet> handle = (HLAvariableArray<HLAoctet>) record.get(0);
         result.add(new ObjectClassCount(classFactory.decode(handle.toByteArray(), 0),
            ((HLAinteger32BE) record.get(1)).getValue()));
      }
      return result;
   }

   private static Interaction findReport(Recorder recorder, int first,
         InteractionClassHandle reportClass, ParameterHandle counts) {
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (reportClass.equals(interaction.interaction) && interaction.parameters.containsKey(counts))
            return interaction;
      }
      return null;
   }

   private static List<Interaction> reportsSince(Recorder recorder, int first,
         InteractionClassHandle reportClass) {
      List<Interaction> result = new ArrayList<>();
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (reportClass.equals(interaction.interaction)) result.add(interaction);
      }
      return result;
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void await(RTIambassador rti, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.0);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.0);
            second.evokeCallback(0.0);
         } else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }
   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectClassHandle objectClass;
      private final FederateHandle producer;
      private Discovery(ObjectClassHandle objectClass, FederateHandle producer) {
         this.objectClass = objectClass;
         this.producer = producer;
      }
   }
   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, LogicalTime<?, ?> time, OrderType sentOrder,
            OrderType receivedOrder, MessageRetractionHandle retraction) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }
   private static final class ObjectClassCount {
      private final ObjectClassHandle objectClass;
      private final int count;
      private ObjectClassCount(ObjectClassHandle objectClass, int count) {
         this.objectClass = objectClass;
         this.count = count;
      }
   }
   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         for (ParameterHandle parameter : values.keySet())
            parameters.put(parameter, values.get(parameter).clone());
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
      }
   }
   private static final class Recorder {
      private final List<DiscoveryRecord> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private volatile LogicalTime<?, ?> timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;
      private Discovery discovery(ObjectInstanceHandle object, int first) {
         for (int i = Math.min(first, discoveries.size()); i < discoveries.size(); ++i)
            if (object.equals(discoveries.get(i).object)) return discoveries.get(i).discovery;
         return null;
      }
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 4) {
                  discoveries.add(new DiscoveryRecord((ObjectInstanceHandle) arguments[0],
                     new Discovery((ObjectClassHandle) arguments[1],
                        (FederateHandle) arguments[3])));
               } else if ("timeRegulationEnabled".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeConstrainedEnabled".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5], (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7], (OrderType) arguments[8],
                     (MessageRetractionHandle) arguments[9]));
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped reflected-object report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
   private static final class DiscoveryRecord {
      private final ObjectInstanceHandle object;
      private final Discovery discovery;
      private DiscoveryRecord(ObjectInstanceHandle object, Discovery discovery) {
         this.object = object;
         this.discovery = discovery;
      }
   }
}
