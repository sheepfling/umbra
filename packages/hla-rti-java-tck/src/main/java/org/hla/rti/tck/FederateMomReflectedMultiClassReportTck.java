package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectClassHandleFactory;
import hla.rti1516_2025.ObjectInstanceHandle;
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
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies class-separated reflected-object counts across two application classes. */
final class FederateMomReflectedMultiClassReportTck {
   private static final String REFLECTED_REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected";
   private static final String REFLECTED_REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected";
   private static final String UPDATED_REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated";
   private static final String UPDATED_REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomReflectedMultiClassReportTck() { }

   static void run(RtiFactory factory, String fom, String secondFom, String mim,
         String timeImplementation,
         String firstClassName, String firstAttributeName, String secondClassName,
         String secondAttributeName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, secondFom, mim, timeImplementation, firstClassName,
         firstAttributeName, secondClassName, secondAttributeName, callbackModel, false);
   }

   static void runUpdated(RtiFactory factory, String fom, String secondFom, String mim,
         String timeImplementation, String firstClassName, String firstAttributeName,
         String secondClassName, String secondAttributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, secondFom, mim, timeImplementation, firstClassName,
         firstAttributeName, secondClassName, secondAttributeName, callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String secondFom, String mim,
         String timeImplementation, String firstClassName, String firstAttributeName,
         String secondClassName, String secondAttributeName, CallbackModel callbackModel,
         boolean updatedReport) throws Exception {
      RTIambassador target = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder targetRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-federate-mom-reflected-multi-class-" + UUID.randomUUID();
      boolean targetConnected = false, subjectConnected = false, requesterConnected = false;
      boolean created = false, targetJoined = false, subjectJoined = false, requesterJoined = false;
      boolean targetSubscribedA = false, targetSubscribedB = false;
      boolean requesterSubscribed = false, subjectPublishedA = false, subjectPublishedB = false;
      InteractionClassHandle requestClass = null, reportClass = null;
      ObjectClassHandle targetClassA = null, targetClassB = null;
      ObjectClassHandle subjectClassA = null, subjectClassB = null;
      AttributeHandle targetAttributeA = null, targetAttributeB = null;
      AttributeHandle subjectAttributeA = null, subjectAttributeB = null;
      try {
         target.connect(targetRecorder.proxy(), callbackModel);
         targetConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         subject.createFederationExecutionWithMIM(federation,
            new String[] {fom, secondFom}, mim, timeImplementation);
         created = true;
         FederateHandle targetFederate = target.joinFederationExecution(
            "mom-reflected-multi-target", "java-tck", federation);
         targetJoined = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-reflected-multi-subject", "java-tck", federation);
         subjectJoined = true;
         requester.joinFederationExecution("mom-reflected-multi-requester", "java-tck",
            federation);
         requesterJoined = true;

         String requestClassName = updatedReport ? UPDATED_REQUEST_CLASS : REFLECTED_REQUEST_CLASS;
         String reportClassName = updatedReport ? UPDATED_REPORT_CLASS : REFLECTED_REPORT_CLASS;
         requestClass = requester.getInteractionClassHandle(requestClassName);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         reportClass = requester.getInteractionClassHandle(reportClassName);
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass,
            "HLAobjectInstanceCounts");
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");

         targetClassA = target.getObjectClassHandle(firstClassName);
         targetAttributeA = target.getAttributeHandle(targetClassA, firstAttributeName);
         subjectClassA = subject.getObjectClassHandle(firstClassName);
         subjectAttributeA = subject.getAttributeHandle(subjectClassA, firstAttributeName);
         targetClassB = target.getObjectClassHandle(secondClassName);
         targetAttributeB = target.getAttributeHandle(targetClassB, secondAttributeName);
         subjectClassB = subject.getObjectClassHandle(secondClassName);
         subjectAttributeB = subject.getAttributeHandle(subjectClassB, secondAttributeName);
         JavaTckSupport.check(firstClassName.equals(target.getObjectClassName(targetClassA))
               && secondClassName.equals(target.getObjectClassName(targetClassB))
               && firstAttributeName.equals(target.getAttributeName(targetClassA, targetAttributeA))
               && secondAttributeName.equals(target.getAttributeName(targetClassB, targetAttributeB))
               && targetClassA.equals(subjectClassA) && targetClassB.equals(subjectClassB)
               && !targetClassA.equals(targetClassB)
               && targetAttributeA.equals(subjectAttributeA)
               && targetAttributeB.equals(subjectAttributeB)
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && "HLAobjectInstanceCounts".equals(requester.getParameterName(reportClass,
                  reportCounts))
               && "HLAreliable".equals(requester.getTransportationTypeName(reliable)),
            "multi-class MOM or adapter-supplied application handles were invalid or inconsistent");

         target.subscribeObjectClassAttributes(targetClassA,
            JavaTckSupport.attributeSet(target, targetAttributeA), "");
         targetSubscribedA = true;
         target.subscribeObjectClassAttributes(targetClassB,
            JavaTckSupport.attributeSet(target, targetAttributeB), "");
         targetSubscribedB = true;
         requester.subscribeInteractionClass(reportClass);
         requesterSubscribed = true;
         subject.publishObjectClassAttributes(subjectClassA,
            JavaTckSupport.attributeSet(subject, subjectAttributeA));
         subjectPublishedA = true;
         subject.publishObjectClassAttributes(subjectClassB,
            JavaTckSupport.attributeSet(subject, subjectAttributeB));
         subjectPublishedB = true;

         int discoveriesBefore = targetRecorder.discoveries.size();
         ObjectInstanceHandle firstA = subject.registerObjectInstance(subjectClassA);
         ObjectInstanceHandle secondA = subject.registerObjectInstance(subjectClassA);
         ObjectInstanceHandle firstB = subject.registerObjectInstance(subjectClassB);
         ObjectInstanceHandle secondB = subject.registerObjectInstance(subjectClassB);
         Set<ObjectInstanceHandle> expectedObjects = new LinkedHashSet<>(Arrays.asList(
            firstA, secondA, firstB, secondB));
         JavaTckSupport.check(!expectedObjects.contains(null) && expectedObjects.size() == 4,
            "multi-class registration returned invalid or duplicate object handles");
         await(target, callbackModel,
            () -> targetRecorder.hasDiscoveries(expectedObjects, discoveriesBefore),
            "discovery of all four objects from two classes");
         JavaTckSupport.check(targetRecorder.discoveries.size() == discoveriesBefore + 4
               && targetClassA.equals(target.getKnownObjectClassHandle(firstA))
               && targetClassA.equals(target.getKnownObjectClassHandle(secondA))
               && targetClassB.equals(target.getKnownObjectClassHandle(firstB))
               && targetClassB.equals(target.getKnownObjectClassHandle(secondB)),
            "multi-class discovery did not preserve each registered class");
         for (int i = discoveriesBefore; i < targetRecorder.discoveries.size(); ++i)
            JavaTckSupport.check(subjectFederate.equals(targetRecorder.discoveries.get(i).producer),
               "multi-class discovery reported the wrong producer");

         int firstReflection = targetRecorder.reflections.size();
         send(subject, firstA, subjectAttributeA, encoder, 11);
         send(subject, firstA, subjectAttributeA, encoder, 12);
         send(subject, secondA, subjectAttributeA, encoder, 13);
         send(subject, firstB, subjectAttributeB, encoder, 21);
         send(subject, firstB, subjectAttributeB, encoder, 22);
         send(subject, secondB, subjectAttributeB, encoder, 23);
         await(target, callbackModel,
            () -> reflectionsFor(targetRecorder, firstReflection, expectedObjects).size() >= 6,
            "six ordinary reflections across two classes");
         List<Reflection> reflections = reflectionsFor(targetRecorder, firstReflection,
            expectedObjects);
         JavaTckSupport.check(reflections.size() == 6,
            "multi-class updates delivered a duplicate or missing application reflection");
         List<Integer> firstAValues = new ArrayList<>(), secondAValues = new ArrayList<>();
         List<Integer> firstBValues = new ArrayList<>(), secondBValues = new ArrayList<>();
         for (Reflection reflection : reflections) {
            boolean classA = firstA.equals(reflection.object) || secondA.equals(reflection.object);
            AttributeHandle expectedAttribute = classA ? targetAttributeA : targetAttributeB;
            JavaTckSupport.check(reflection.values.size() == 1
                  && reflection.values.containsKey(expectedAttribute)
                  && reliable.equals(reflection.transportation) && reflection.tag.length == 0
                  && subjectFederate.equals(reflection.producer)
                  && (reflection.regions == null || reflection.regions.isEmpty()),
               "multi-class reflection returned an unexpected attribute or delivery metadata");
            HLAbyte value = encoder.createHLAbyte();
            value.decode(reflection.values.get(expectedAttribute));
            int decoded = value.getValue();
            if (firstA.equals(reflection.object)) firstAValues.add(decoded);
            else if (secondA.equals(reflection.object)) secondAValues.add(decoded);
            else if (firstB.equals(reflection.object)) firstBValues.add(decoded);
            else if (secondB.equals(reflection.object)) secondBValues.add(decoded);
            else throw new AssertionError("multi-class reflection named an unregistered object");
         }
         JavaTckSupport.check(firstAValues.equals(Arrays.asList(11, 12))
               && secondAValues.equals(Arrays.asList(13))
               && firstBValues.equals(Arrays.asList(21, 22))
               && secondBValues.equals(Arrays.asList(23)),
            "multi-class reflection values did not match each registered object and class");

         int reportStart = requesterRecorder.interactions.size();
         ParameterHandleValueMap parameters = requester.getParameterHandleValueMapFactory().create(1);
         parameters.put(requestFederate, encode(updatedReport ? subjectFederate : targetFederate));
         requester.sendInteraction(requestClass, parameters, new byte[0]);
         InteractionClassHandle resolvedReportClass = reportClass;
         await(requester, callbackModel,
            () -> findReport(requesterRecorder, reportStart, resolvedReportClass, reportCounts) != null,
            "multi-class reflected-object MOM report");
         List<Interaction> reports = reportsSince(requesterRecorder, reportStart,
            resolvedReportClass);
         JavaTckSupport.check(reports.size() == 1,
            "multi-class reflected-object request did not deliver exactly one report");
         Interaction report = reports.get(0);
         JavaTckSupport.check(report.parameters.size() == 1
               && report.parameters.containsKey(reportCounts)
               && reliable.equals(report.transportation) && report.tag.length == 0
               && report.producer == null && report.regionsEmpty,
            "multi-class reflected-object report had non-standard response metadata");
         List<ObjectClassCount> counts = decodeCounts(encoder,
            requester.getObjectClassHandleFactory(), report.parameters.get(reportCounts));
         Map<ObjectClassHandle, Integer> byClass = new LinkedHashMap<>();
         for (ObjectClassCount count : counts) {
            JavaTckSupport.check(!byClass.containsKey(count.objectClass),
               "MIM repeated a reflected-object class count record");
            byClass.put(count.objectClass, count.count);
         }
         JavaTckSupport.check(counts.size() == 2 && byClass.size() == 2
               && Integer.valueOf(2).equals(byClass.get(targetClassA))
               && Integer.valueOf(2).equals(byClass.get(targetClassB)),
            "reflected-object report did not map two distinct instances to each of two classes");

         requester.unsubscribeInteractionClass(reportClass);
         requesterSubscribed = false;
         target.unsubscribeObjectClassAttributes(targetClassA,
            JavaTckSupport.attributeSet(target, targetAttributeA));
         targetSubscribedA = false;
         target.unsubscribeObjectClassAttributes(targetClassB,
            JavaTckSupport.attributeSet(target, targetAttributeB));
         targetSubscribedB = false;
         subject.deleteObjectInstance(firstA, new byte[0]);
         subject.deleteObjectInstance(secondA, new byte[0]);
         subject.deleteObjectInstance(firstB, new byte[0]);
         subject.deleteObjectInstance(secondB, new byte[0]);
         subject.unpublishObjectClassAttributes(subjectClassA,
            JavaTckSupport.attributeSet(subject, subjectAttributeA));
         subjectPublishedA = false;
         subject.unpublishObjectClassAttributes(subjectClassB,
            JavaTckSupport.attributeSet(subject, subjectAttributeB));
         subjectPublishedB = false;
         target.resignFederationExecution(ResignAction.NO_ACTION);
         targetJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterSubscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (targetSubscribedA && targetJoined && targetClassA != null && targetAttributeA != null) try {
            target.unsubscribeObjectClassAttributes(targetClassA,
               JavaTckSupport.attributeSet(target, targetAttributeA));
         } catch (Exception ignored) { }
         if (targetSubscribedB && targetJoined && targetClassB != null && targetAttributeB != null) try {
            target.unsubscribeObjectClassAttributes(targetClassB,
               JavaTckSupport.attributeSet(target, targetAttributeB));
         } catch (Exception ignored) { }
         if (subjectPublishedA && subjectJoined && subjectClassA != null && subjectAttributeA != null) try {
            subject.unpublishObjectClassAttributes(subjectClassA,
               JavaTckSupport.attributeSet(subject, subjectAttributeA));
         } catch (Exception ignored) { }
         if (subjectPublishedB && subjectJoined && subjectClassB != null && subjectAttributeB != null) try {
            subject.unpublishObjectClassAttributes(subjectClassB,
               JavaTckSupport.attributeSet(subject, subjectAttributeB));
         } catch (Exception ignored) { }
         if (targetJoined) resign(target);
         if (subjectJoined) resign(subject);
         if (requesterJoined) resign(requester);
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) disconnect(requester);
         if (subjectConnected) disconnect(subject);
         if (targetConnected) disconnect(target);
      }
   }

   private static void send(RTIambassador sender, ObjectInstanceHandle object,
         AttributeHandle attribute, EncoderFactory encoder, int value) throws Exception {
      AttributeHandleValueMap values = sender.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, encoder.createHLAbyte((byte) value).toByteArray());
      sender.updateAttributeValues(object, values, new byte[0]);
   }

   private static List<Reflection> reflectionsFor(Recorder recorder, int first,
         Set<ObjectInstanceHandle> objects) {
      List<Reflection> result = new ArrayList<>();
      for (int i = Math.min(first, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (objects.contains(reflection.object)) result.add(reflection);
      }
      return result;
   }

   private static List<ObjectClassCount> decodeCounts(EncoderFactory encoder,
         ObjectClassHandleFactory classFactory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAoctet> classHandlePrototype = encoder.createHLAvariableArray(
         index -> encoder.createHLAoctet(), encoder.createHLAoctet());
      HLAfixedRecord recordPrototype = encoder.createHLAfixedRecord();
      recordPrototype.add(classHandlePrototype);
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

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }
   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final FederateHandle producer;
      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            FederateHandle producer) {
         this.object = object;
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
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
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
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private boolean hasDiscoveries(Set<ObjectInstanceHandle> objects, int first) {
         Set<ObjectInstanceHandle> found = new LinkedHashSet<>();
         for (int i = Math.min(first, discoveries.size()); i < discoveries.size(); ++i)
            found.add(discoveries.get(i).object);
         return found.containsAll(objects);
      }
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (FederateHandle) arguments[3]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK multi-class reflected report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
