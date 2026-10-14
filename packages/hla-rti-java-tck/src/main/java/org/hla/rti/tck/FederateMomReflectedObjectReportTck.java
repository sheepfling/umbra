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
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies distinct reflected-object counts and the standard class-scoped MOM report. */
final class FederateMomReflectedObjectReportTck {
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomReflectedObjectReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador monitor = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder requesterRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      Recorder monitorRecorder = new Recorder();
      String federation = "java-tck-federate-mom-reflected-object-report-" + UUID.randomUUID();
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
            "mom-reflected-target", "java-tck", federation);
         requesterJoined = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-reflected-subject", "java-tck", federation);
         subjectJoined = true;
         monitor.joinFederationExecution("mom-reflected-monitor", "java-tck", federation);
         monitorJoined = true;

         requestClass = monitor.getInteractionClassHandle(REQUEST_CLASS);
         ParameterHandle requestFederate = monitor.getParameterHandle(requestClass, "HLAfederate");
         reportClass = monitor.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle reportCounts = monitor.getParameterHandle(reportClass,
            "HLAobjectInstanceCounts");
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(REQUEST_CLASS.equals(monitor.getInteractionClassName(requestClass))
               && "HLAfederate".equals(monitor.getParameterName(requestClass, requestFederate))
               && REPORT_CLASS.equals(monitor.getInteractionClassName(reportClass))
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
         JavaTckSupport.check(objectClassName.equals(requester.getObjectClassName(requesterObjectClass))
               && objectClassName.equals(subject.getObjectClassName(subjectObjectClass))
               && attributeName.equals(requester.getAttributeName(requesterObjectClass,
                  requesterAttribute))
               && attributeName.equals(subject.getAttributeName(subjectObjectClass, subjectAttribute))
               && requesterObjectClass.equals(subjectObjectClass)
               && requesterAttribute.equals(subjectAttribute),
            "adapter-supplied application handles did not round-trip consistently");
         requester.subscribeObjectClassAttributes(requesterObjectClass,
            JavaTckSupport.attributeSet(requester, requesterAttribute), "");
         requesterSubscribed = true;
         subject.publishObjectClassAttributes(subjectObjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute));
         subjectPublished = true;

         int discoveriesBefore = requesterRecorder.discoveries.size();
         ObjectInstanceHandle firstObject = subject.registerObjectInstance(subjectObjectClass);
         ObjectInstanceHandle secondObject = subject.registerObjectInstance(subjectObjectClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "reflected-object registration returned invalid or duplicate handles");
         await(requester, callbackModel,
            () -> requesterRecorder.discovery(firstObject, discoveriesBefore) != null
               && requesterRecorder.discovery(secondObject, discoveriesBefore) != null,
            "reflected-object discovery");
         JavaTckSupport.check(requesterRecorder.discoveries.size() == discoveriesBefore + 2,
            "reflected-object setup did not deliver exactly two discovery callbacks");
         Discovery firstDiscovery = requesterRecorder.discovery(firstObject, discoveriesBefore);
         Discovery secondDiscovery = requesterRecorder.discovery(secondObject, discoveriesBefore);
         JavaTckSupport.check(firstDiscovery != null && secondDiscovery != null
               && requesterObjectClass.equals(firstDiscovery.objectClass)
               && requesterObjectClass.equals(secondDiscovery.objectClass)
               && subjectFederate.equals(firstDiscovery.producer)
               && subjectFederate.equals(secondDiscovery.producer)
               && requesterObjectClass.equals(requester.getKnownObjectClassHandle(firstObject))
               && requesterObjectClass.equals(requester.getKnownObjectClassHandle(secondObject)),
            "reflected-object discovery had the wrong class, producer, or known-object state");

         int reflectionStart = requesterRecorder.reflections.size();
         update(subject, firstObject, subjectAttribute, encoder, (byte) 1);
         update(subject, firstObject, subjectAttribute, encoder, (byte) 2);
         update(subject, secondObject, subjectAttribute, encoder, (byte) 3);
         await(requester, callbackModel,
            () -> reflectionsFor(requesterRecorder, reflectionStart, firstObject, secondObject)
               .size() >= 3,
            "three receive-order application reflections");
         List<Reflection> applicationReflections = reflectionsFor(requesterRecorder,
            reflectionStart, firstObject, secondObject);
         List<Integer> firstValues = new ArrayList<>();
         List<Integer> secondValues = new ArrayList<>();
         for (Reflection reflection : applicationReflections) {
            JavaTckSupport.check(reflection.values.size() == 1
                  && reflection.values.containsKey(requesterAttribute)
                  && reliable.equals(reflection.transportation) && reflection.tag.length == 0
                  && subjectFederate.equals(reflection.producer)
                  && (reflection.regions == null || reflection.regions.isEmpty()),
               "application reflection returned unexpected attribute or transport metadata");
            HLAbyte value = encoder.createHLAbyte();
            value.decode(reflection.values.get(requesterAttribute));
            if (firstObject.equals(reflection.object)) firstValues.add((int) value.getValue());
            else secondValues.add((int) value.getValue());
         }
         JavaTckSupport.check(applicationReflections.size() == 3
               && firstValues.equals(Arrays.asList(1, 2))
               && secondValues.equals(Arrays.asList(3)),
            "application callbacks did not contain two updates for the first object and one for the second");

         int reportStart = monitorRecorder.interactions.size();
         ParameterHandleValueMap requestParameters = monitor.getParameterHandleValueMapFactory()
            .create(1);
         requestParameters.put(requestFederate, encode(requesterFederate));
         monitor.sendInteraction(requestClass, requestParameters, new byte[0]);
         InteractionClassHandle resolvedReportClass = reportClass;
         await(monitor, callbackModel,
            () -> findReport(monitorRecorder, reportStart, resolvedReportClass, reportCounts) != null,
            "joined-federate MOM reflected-object report");
         List<Interaction> reports = reportsSince(monitorRecorder, reportStart,
            resolvedReportClass);
         JavaTckSupport.check(reports.size() == 1,
            "reflected-object request did not deliver exactly one standard report");
         Interaction report = reports.get(0);
         JavaTckSupport.check(report.present && report.parameters.size() == 1
               && report.parameters.containsKey(reportCounts)
               && reliable.equals(report.transportation) && report.tag.length == 0
               && report.producer == null && report.regionsEmpty,
            "reflected-object report had a non-standard reliable one-parameter response shape");
         List<ObjectClassCount> counts = decodeObjectClassCounts(encoder,
            monitor.getObjectClassHandleFactory(), report.parameters.get(reportCounts));
         JavaTckSupport.check(counts.size() == 1
               && requesterObjectClass.equals(counts.get(0).objectClass)
               && counts.get(0).count == 2,
            "reflected-object report did not count two distinct instances by registered class");

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

   private static void update(RTIambassador subject, ObjectInstanceHandle object,
         AttributeHandle attribute, EncoderFactory encoder, byte value) throws Exception {
      AttributeHandleValueMap values = subject.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, encoder.createHLAbyte(value).toByteArray());
      subject.updateAttributeValues(object, values, new byte[0]);
   }

   private static List<Reflection> reflectionsFor(Recorder recorder, int first,
         ObjectInstanceHandle firstObject, ObjectInstanceHandle secondObject) {
      List<Reflection> result = new ArrayList<>();
      for (int i = Math.min(first, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (firstObject.equals(reflection.object) || secondObject.equals(reflection.object))
            result.add(reflection);
      }
      return result;
   }

   private static List<ObjectClassCount> decodeObjectClassCounts(EncoderFactory encoder,
         ObjectClassHandleFactory classFactory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAoctet> handlePrototype = encoder.createHLAvariableArray(
         index -> encoder.createHLAoctet(), encoder.createHLAoctet());
      HLAfixedRecord recordPrototype = encoder.createHLAfixedRecord();
      recordPrototype.add(handlePrototype);
      recordPrototype.add(encoder.createHLAinteger32BE());
      HLAvariableArray<HLAfixedRecord> records = encoder.createHLAvariableArray(
         index -> {
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
         HLAinteger32BE count = (HLAinteger32BE) record.get(1);
         result.add(new ObjectClassCount(classFactory.decode(handle.toByteArray(), 0),
            count.getValue()));
      }
      return result;
   }

   private static Interaction findReport(Recorder recorder, int first,
         InteractionClassHandle reportClass, ParameterHandle counts) {
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (interaction.present && reportClass.equals(interaction.interaction)
               && interaction.parameters.containsKey(counts)) return interaction;
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
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
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
      private final boolean present = true;
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
      private Discovery discovery(ObjectInstanceHandle object, int first) {
         for (int i = Math.min(first, discoveries.size()); i < discoveries.size(); ++i)
            if (object.equals(discoveries.get(i).object)) return discoveries.get(i);
         return null;
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
                  if ("toString".equals(name)) return "Java TCK reflected-object MOM report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
