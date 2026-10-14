package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies direct and periodic HLAobjectInstancesThatCanBeDeleted counts. */
final class FederateMomDeletableObjectCountTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomDeletableObjectCountTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-deletable-object-count-" + UUID.randomUUID();
      boolean ownerConnected = false, observerConnected = false, created = false;
      boolean ownerJoined = false, observerJoined = false, momSubscribed = false;
      boolean published = false;
      ObjectClassHandle momClass = null, ownerClass = null;
      AttributeHandleSet momAttributes = null, ownerAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         observer.joinFederationExecution("mom-deletable-observer", "java-tck", federation);
         observerJoined = true;

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = observer.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle deletableCountAttribute = observer.getAttributeHandle(momClass,
            "HLAobjectInstancesThatCanBeDeleted");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(observer.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesThatCanBeDeleted".equals(observer.getAttributeName(momClass,
                  deletableCountAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard joined-federate MOM deletable-object handles did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(observer, federateAttribute,
            deletableCountAttribute);
         observer.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         FederateHandle ownerFederate = owner.joinFederationExecution(
            "mom-deletable-owner", "java-tck", federation);
         ownerJoined = true;
         ObjectClassHandle ownerMomClass = owner.getObjectClassHandle(MOM_CLASS);
         AttributeHandle ownerFederateAttribute = owner.getAttributeHandle(ownerMomClass,
            "HLAfederateHandle");
         AttributeHandle ownerDeletableCountAttribute = owner.getAttributeHandle(ownerMomClass,
            "HLAobjectInstancesThatCanBeDeleted");
         JavaTckSupport.check(momClass.equals(ownerMomClass)
               && federateAttribute.equals(ownerFederateAttribute)
               && deletableCountAttribute.equals(ownerDeletableCountAttribute),
            "joined federates received different standard MOM handles");

         byte[] encodedOwner = encode(ownerFederate);
         await(observer, callbackModel,
            () -> observerRecorder.momObject(federateAttribute, encodedOwner) != null,
            "owner joined-federate MOM discovery");
         ObjectInstanceHandle ownerMomObject = observerRecorder.momObject(federateAttribute,
            encodedOwner);
         JavaTckSupport.check(ownerMomObject != null
               && momClass.equals(observer.getKnownObjectClassHandle(ownerMomObject)),
            "observer found the wrong owner MOM object");
         requestCount(observer, observerRecorder, callbackModel, ownerMomObject,
            deletableCountAttribute, reliable, encoder, 0, "initial deletable-object count");

         ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute)),
            "adapter-supplied owner object class or attribute lookup did not round-trip");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         ObjectInstanceHandle firstObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(firstObject != null,
            "first deletable-object registration returned an invalid handle");
         requestCount(observer, observerRecorder, callbackModel, ownerMomObject,
            deletableCountAttribute, reliable, encoder, 1, "count with one deletable object");
         ObjectInstanceHandle secondObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(secondObject != null && !firstObject.equals(secondObject),
            "second deletable-object registration returned an invalid or duplicate handle");
         requestCount(observer, observerRecorder, callbackModel, ownerMomObject,
            deletableCountAttribute, reliable, encoder, 2, "count with two deletable objects");

         InteractionClassHandle setTiming = observer.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = observer.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = observer.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(observer.getInteractionClassName(setTiming))
               && "HLAfederate".equals(observer.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(observer.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicStart = observerRecorder.reflections.size();
         sendTiming(observer, setTiming, timingFederate, reportPeriod, ownerFederate, 1, encoder);
         await(observer, callbackModel,
            () -> findCount(observerRecorder, periodicStart, ownerMomObject,
               deletableCountAttribute) != null,
            "periodic deletable-object count reflection");
         Reflection periodic = findCount(observerRecorder, periodicStart, ownerMomObject,
            deletableCountAttribute);
         verifyCount(periodic, deletableCountAttribute, reliable, encoder, 2,
            "periodic deletable-object count");
         sendTiming(observer, setTiming, timingFederate, reportPeriod, ownerFederate, 0, encoder);

         owner.deleteObjectInstance(firstObject, new byte[0]);
         requestCount(observer, observerRecorder, callbackModel, ownerMomObject,
            deletableCountAttribute, reliable, encoder, 1, "count after first object deletion");
         owner.deleteObjectInstance(secondObject, new byte[0]);
         requestCount(observer, observerRecorder, callbackModel, ownerMomObject,
            deletableCountAttribute, reliable, encoder, 0, "count after all object deletions");

         observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (momSubscribed && observerJoined && momClass != null && momAttributes != null) try {
            observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (published && ownerJoined && ownerClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void requestCount(RTIambassador observer, Recorder recorder, CallbackModel model,
         ObjectInstanceHandle momObject, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, int expected,
         String description) throws Exception {
      int start = recorder.reflections.size();
      observer.requestAttributeValueUpdate(momObject,
         JavaTckSupport.attributeSet(observer, attribute), new byte[0]);
      await(observer, model,
         () -> findCount(recorder, start, momObject, attribute) != null, description);
      Reflection reflection = findCount(recorder, start, momObject, attribute);
      verifyCount(reflection, attribute, reliable, encoder, expected, description);
   }

   private static void verifyCount(Reflection reflection, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, int expected,
         String description) throws Exception {
      JavaTckSupport.check(reflection != null && reflection.values.size() == 1
            && reflection.values.containsKey(attribute) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(reflection.values.get(attribute));
      JavaTckSupport.check(value.getValue() == expected,
         description + " returned a different deletable-object count");
   }

   private static Reflection findCount(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute) {
      for (int i = Math.min(first, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static void sendTiming(RTIambassador sender, InteractionClassHandle interaction,
         ParameterHandle targetFederate, ParameterHandle reportPeriod, FederateHandle target,
         int seconds, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(2);
      parameters.put(targetFederate, encode(target));
      parameters.put(reportPeriod, encoder.createHLAinteger32BE(seconds).toByteArray());
      sender.sendInteraction(interaction, parameters, new byte[0]);
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

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private ObjectInstanceHandle momObject(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(expected, value)) return reflection.object;
         }
         return null;
      }
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletable-object MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
