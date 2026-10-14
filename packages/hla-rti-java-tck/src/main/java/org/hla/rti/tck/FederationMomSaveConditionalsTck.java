package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verify standard federation MOM save-conditionals before, during, and after a timed save. */
final class FederationMomSaveConditionalsTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederation";
   private static final String MOM_OBJECT = "HLAfederation";
   private static final String NEXT_NAME = "HLAnextSaveName";
   private static final String NEXT_TIME = "HLAnextSaveTime";
   private static final String LAST_NAME = "HLAlastSaveName";
   private static final String LAST_TIME = "HLAlastSaveTime";

   private FederationMomSaveConditionalsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federation-mom-save-conditionals-" + UUID.randomUUID();
      String ownerName = "java-tck-mom-save-owner";
      String observerName = "java-tck-mom-save-observer";
      String constrainedName = "java-tck-mom-save-constrained";
      String saveLabel = "java-tck-federation-mom-save-conditionals";
      boolean ownerConnected = false, observerConnected = false, constrainedConnected = false;
      boolean created = false, ownerJoined = false, observerJoined = false, constrainedJoined = false;
      boolean subscribed = false, regulating = false, constrainedEnabled = false;
      ObjectClassHandle federationClass = null;
      AttributeHandleSet conditionalAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, "tck-type", federation);
         ownerJoined = true;
         observer.joinFederationExecution(observerName, "tck-type", federation);
         observerJoined = true;
         constrained.joinFederationExecution(constrainedName, "tck-type", federation);
         constrainedJoined = true;

         ObjectClassHandle ownerFederationClass = owner.getObjectClassHandle(MOM_CLASS);
         federationClass = observer.getObjectClassHandle(MOM_CLASS);
         JavaTckSupport.check(ownerFederationClass != null && federationClass != null
               && ownerFederationClass.equals(federationClass)
               && MOM_CLASS.equals(observer.getObjectClassName(federationClass)),
            "federation MOM class lookup did not round-trip the standard class");

         AttributeHandle ownerNextName = owner.getAttributeHandle(ownerFederationClass, NEXT_NAME);
         AttributeHandle ownerNextTime = owner.getAttributeHandle(ownerFederationClass, NEXT_TIME);
         AttributeHandle ownerLastName = owner.getAttributeHandle(ownerFederationClass, LAST_NAME);
         AttributeHandle ownerLastTime = owner.getAttributeHandle(ownerFederationClass, LAST_TIME);
         AttributeHandle nextName = observer.getAttributeHandle(federationClass, NEXT_NAME);
         AttributeHandle nextTime = observer.getAttributeHandle(federationClass, NEXT_TIME);
         AttributeHandle lastName = observer.getAttributeHandle(federationClass, LAST_NAME);
         AttributeHandle lastTime = observer.getAttributeHandle(federationClass, LAST_TIME);
         JavaTckSupport.check(ownerNextName.equals(nextName) && ownerNextTime.equals(nextTime)
               && ownerLastName.equals(lastName) && ownerLastTime.equals(lastTime)
               && NEXT_NAME.equals(observer.getAttributeName(federationClass, nextName))
               && NEXT_TIME.equals(observer.getAttributeName(federationClass, nextTime))
               && LAST_NAME.equals(observer.getAttributeName(federationClass, lastName))
               && LAST_TIME.equals(observer.getAttributeName(federationClass, lastTime)),
            "federation MOM save-conditional attribute handles or names did not round-trip");

         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "federation MOM could not resolve standard reliable transportation");
         conditionalAttributes = JavaTckSupport.attributeSet(
            observer, nextName, nextTime, lastName, lastTime);
         observer.subscribeObjectClassAttributes(federationClass, conditionalAttributes);
         subscribed = true;
         await(callbackModel, MOM_CLASS, "federation MOM object discovery",
            () -> observerRecorder.discoveredObject != null, owner, observer, constrained);
         ObjectInstanceHandle federationObject = observerRecorder.discoveredObject;
         JavaTckSupport.check(federationClass.equals(observerRecorder.discoveredClass)
               && MOM_OBJECT.equals(observerRecorder.discoveredName)
               && federationObject.encodedLength() > 0,
            "federation MOM discovered the wrong standard object");

         LogicalTimeFactory<?, ?> observerTimeFactory = observer.getTimeFactory();
         ConditionalValues initial = requestConditionals(observer, observerRecorder, encoder,
            callbackModel, federationObject, conditionalAttributes, nextName, nextTime,
            lastName, lastTime, reliable, observerTimeFactory, MOM_CLASS, "initial values");
         JavaTckSupport.check(initial.nextName.isEmpty() && initial.nextTime.length == 0
               && initial.lastName.isEmpty() && initial.lastTime.length == 0,
            "initial federation MOM save-conditionals values were not empty");

         LogicalTimeFactory<?, ?> ownerTimeFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedTimeFactory = constrained.getTimeFactory();
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) ownerTimeFactory.makeEpsilon();
         JavaTckSupport.check(ownerTimeFactory.getName().equals(constrainedTimeFactory.getName())
               && ownerTimeFactory.getName().equals(observerTimeFactory.getName()),
            "federation MOM save-conditionals members selected different logical-time factories");
         owner.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, MOM_CLASS, "owner time-regulation enablement",
            () -> ownerRecorder.timeRegulationEnabled, owner, observer, constrained);
         constrained.enableTimeConstrained();
         constrainedEnabled = true;
         await(callbackModel, MOM_CLASS, "constrained time enablement",
            () -> constrainedRecorder.timeConstrainedEnabled, owner, observer, constrained);

         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime saveTime = offset(ownerInitial, epsilon, 7);
         owner.requestFederationSave(saveLabel, saveTime);
         ConditionalValues pending = requestConditionals(observer, observerRecorder, encoder,
            callbackModel, federationObject, conditionalAttributes, nextName, nextTime,
            lastName, lastTime, reliable, observerTimeFactory, MOM_CLASS, "pending values");
         JavaTckSupport.check(saveLabel.equals(pending.nextName)
               && Arrays.equals(encodeTime(saveTime), pending.nextTime)
               && pending.lastName.isEmpty() && pending.lastTime.length == 0,
            "pending federation MOM save-conditionals omitted the requested save");

         int admittedReflectionStart = observerRecorder.reflections.size();
         constrained.timeAdvanceRequest(saveTime);
         owner.timeAdvanceRequest(saveTime);
         await(callbackModel, MOM_CLASS, "timed save admission callbacks",
            () -> ownerRecorder.saveInitiations.size() == 1
               && constrainedRecorder.saveInitiations.size() == 1
               && observerRecorder.saveInitiations.size() == 1
               && ownerRecorder.timeAdvanceGrants > 0
               && constrainedRecorder.timeAdvanceGrants > 0,
            owner, observer, constrained);
         Reflection admittedReflection = collectConditionals(observer, observerRecorder,
            callbackModel, federationObject, reliable, admittedReflectionStart,
            new AttributeHandle[] {nextName, nextTime}, MOM_CLASS, "save admission reflection");
         ConditionalValues admitted = decodeConditionals(admittedReflection, encoder,
            observerTimeFactory, nextName, nextTime, lastName, lastTime);
         JavaTckSupport.check(admitted.nextName.isEmpty() && admitted.nextTime.length == 0
               && admitted.lastName.isEmpty() && admitted.lastTime.length == 0,
            "save admission did not clear pending federation MOM save-conditionals");
         JavaTckSupport.check(saveLabel.equals(ownerRecorder.saveInitiations.get(0))
               && saveLabel.equals(constrainedRecorder.saveInitiations.get(0))
               && saveLabel.equals(observerRecorder.saveInitiations.get(0)),
            "timed save admission returned the wrong label");
         JavaTckSupport.check(Arrays.equals(encodeTime(saveTime),
                  encodeTime(ownerRecorder.saveInitiationTimes.get(0)))
               && Arrays.equals(encodeTime(saveTime),
                  encodeTime(constrainedRecorder.saveInitiationTimes.get(0))),
            "timed save initiation returned the wrong logical time");

         owner.federateSaveBegun();
         constrained.federateSaveBegun();
         observer.federateSaveBegun();
         int completedReflectionStart = observerRecorder.reflections.size();
         owner.federateSaveComplete();
         constrained.federateSaveComplete();
         observer.federateSaveComplete();
         await(callbackModel, MOM_CLASS, "timed save completion callbacks",
            () -> ownerRecorder.savedCount == 1 && constrainedRecorder.savedCount == 1
               && observerRecorder.savedCount == 1,
            owner, observer, constrained);
         Reflection completedReflection = collectConditionals(observer, observerRecorder,
            callbackModel, federationObject, reliable, completedReflectionStart,
            new AttributeHandle[] {lastName, lastTime}, MOM_CLASS,
            "save completion reflection");
         ConditionalValues completed = decodeConditionals(completedReflection, encoder,
            observerTimeFactory, nextName, nextTime, lastName, lastTime);
         JavaTckSupport.check(completed.nextName.isEmpty() && completed.nextTime.length == 0
               && saveLabel.equals(completed.lastName)
               && Arrays.equals(encodeTime(saveTime), completed.lastTime),
            "save completion did not publish the last save name and time");
         JavaTckSupport.check(ownerRecorder.saveFailureCount == 0
               && constrainedRecorder.saveFailureCount == 0
               && observerRecorder.saveFailureCount == 0,
            "federation MOM save-conditionals reported a failed save");
      } finally {
         if (subscribed && federationClass != null && conditionalAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(federationClass, conditionalAttributes); }
            catch (Exception ignored) { }
         }
         if (constrainedEnabled) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedJoined) resign(constrained);
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (constrainedConnected) disconnect(constrained);
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static ConditionalValues requestConditionals(RTIambassador observer, Recorder recorder,
         EncoderFactory encoder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandleSet attributes, AttributeHandle nextName, AttributeHandle nextTime,
         AttributeHandle lastName, AttributeHandle lastTime, TransportationTypeHandle reliable,
         LogicalTimeFactory<?, ?> timeFactory, String servicingClass, String description)
         throws Exception {
      int start = recorder.reflections.size();
      observer.requestAttributeValueUpdate(federationObject, attributes, new byte[0]);
      Reflection reflection = collectConditionals(observer, recorder, callbackModel,
         federationObject, reliable, start,
         new AttributeHandle[] {nextName, nextTime, lastName, lastTime}, servicingClass,
         "direct value request " + description);
      JavaTckSupport.check(reflection.values.size() == 4,
         "direct federation MOM value request omitted a save conditional");
      return decodeConditionals(reflection, encoder, timeFactory,
         nextName, nextTime, lastName, lastTime);
   }

   private static Reflection collectConditionals(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         TransportationTypeHandle reliable, int start, AttributeHandle[] required,
         String servicingClass, String description) throws Exception {
      await(callbackModel, servicingClass, description,
         () -> {
            Map<AttributeHandle, byte[]> merged = merge(recorder.reflections, start,
               federationObject, reliable);
            for (AttributeHandle handle : required) if (!merged.containsKey(handle)) return false;
            return true;
         }, observer);
      Reflection merged = new Reflection(federationObject, merge(recorder.reflections, start,
         federationObject, reliable), new byte[0], reliable, null, null);
      return merged;
   }

   private static Map<AttributeHandle, byte[]> merge(List<Reflection> reflections, int start,
         ObjectInstanceHandle expectedObject, TransportationTypeHandle reliable) {
      Map<AttributeHandle, byte[]> merged = new HashMap<>();
      for (int index = start; index < reflections.size(); index++) {
         Reflection reflection = reflections.get(index);
         if (!expectedObject.equals(reflection.object)) continue;
         checkReflectionMetadata(reflection, expectedObject, reliable);
         merged.putAll(reflection.values);
      }
      return merged;
   }

   private static void checkReflectionMetadata(Reflection reflection,
         ObjectInstanceHandle federationObject, TransportationTypeHandle reliable) {
      JavaTckSupport.check(federationObject.equals(reflection.object)
            && reliable.equals(reflection.transportation)
            && reflection.tag.length == 0 && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         "federation MOM save-conditionals reflection had non-standard metadata");
   }

   private static ConditionalValues decodeConditionals(Reflection reflection,
         EncoderFactory encoder, LogicalTimeFactory<?, ?> timeFactory,
         AttributeHandle nextName, AttributeHandle nextTime,
         AttributeHandle lastName, AttributeHandle lastTime) throws Exception {
      ConditionalValues result = new ConditionalValues();
      decodePair(reflection.values, nextName, nextTime, encoder, timeFactory,
         (name, time) -> { result.nextName = name; result.nextTime = time; }, "next-save");
      decodePair(reflection.values, lastName, lastTime, encoder, timeFactory,
         (name, time) -> { result.lastName = name; result.lastTime = time; }, "last-save");
      return result;
   }

   private static void decodePair(Map<AttributeHandle, byte[]> values,
         AttributeHandle nameHandle, AttributeHandle timeHandle, EncoderFactory encoder,
         LogicalTimeFactory<?, ?> timeFactory, PairConsumer consumer, String description)
         throws Exception {
      boolean hasName = values.containsKey(nameHandle);
      boolean hasTime = values.containsKey(timeHandle);
      JavaTckSupport.check(hasName == hasTime,
         "federation MOM " + description + " name/time pair was split across reflections");
      if (!hasName) return;
      HLAunicodeString nameDecoder = encoder.createHLAunicodeString();
      nameDecoder.decode(values.get(nameHandle));
      byte[] encodedTime = values.get(timeHandle);
      if (encodedTime.length > 0) {
         Object decodedTime = timeFactory.decodeTime(encodedTime, 0);
         JavaTckSupport.check(decodedTime != null,
            "federation MOM " + description + " time was not decodable");
      }
      consumer.accept(nameDecoder.getValue(), encodedTime.clone());
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; index++) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static byte[] encodeTime(LogicalTime time) throws Exception {
      byte[] encoded = new byte[time.encodedLength()];
      time.encode(encoded, 0);
      return encoded;
   }

   private static void await(CallbackModel callbackModel, String servicingClass,
         String description, Condition condition, RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + 15_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         for (RTIambassador ambassador : ambassadors) {
            if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0, 1);
            else ambassador.getObjectClassHandle(servicingClass);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   @FunctionalInterface
   private interface PairConsumer {
      void accept(String name, byte[] time) throws Exception;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class ConditionalValues {
      private String nextName = "";
      private byte[] nextTime = new byte[0];
      private String lastName = "";
      private byte[] lastTime = new byte[0];
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<LogicalTime> saveInitiationTimes = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile ObjectClassHandle discoveredClass;
      private volatile String discoveredName;
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private volatile int timeAdvanceGrants;
      private volatile int savedCount;
      private volatile int saveFailureCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  discoveredClass = (ObjectClassHandle) arguments[1];
                  discoveredName = (String) arguments[2];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  Map<AttributeHandle, byte[]> copied = new HashMap<>();
                  AttributeHandleValueMap received = (AttributeHandleValueMap) arguments[1];
                  for (Map.Entry<AttributeHandle, byte[]> entry : received.entrySet()) {
                     copied.put(entry.getKey(), entry.getValue().clone());
                  }
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], copied,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], (RegionHandleSet) arguments[5]));
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrants++;
               } else if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
                  if (arguments.length > 1) saveInitiationTimes.add((LogicalTime) arguments[1]);
               } else if ("federationSaved".equals(name)) {
                  savedCount++;
               } else if ("federationNotSaved".equals(name)) {
                  saveFailureCount++;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK federation MOM save recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
