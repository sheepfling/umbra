package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies direct and conditional standard MOM values for Automatic Resign Action. */
final class FederateAutomaticResignActionMomAttributeTck {
   private static final String MOM_FEDERATE_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String AUTOMATIC_RESIGN_ACTION = "HLAautomaticResignAction";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateAutomaticResignActionMomAttributeTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador monitor = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-auto-resign-mom-attribute-" + suffix;
      String monitorName = "java-tck-auto-resign-mom-monitor-" + suffix;
      String subjectName = "java-tck-auto-resign-mom-subject-" + suffix;
      boolean monitorConnected = false;
      boolean subjectConnected = false;
      boolean created = false;
      boolean monitorJoined = false;
      boolean subjectJoined = false;
      boolean subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet subscription = null;
      try {
         monitor.connect(recorder.proxy(), callbackModel);
         monitorConnected = true;
         subject.connect(callbackProxy(), callbackModel);
         subjectConnected = true;
         if (timeImplementation == null || timeImplementation.isEmpty()) {
            monitor.createFederationExecutionWithMIM(federation, new String[] {fom}, mim);
         } else {
            monitor.createFederationExecutionWithMIM(
               federation, new String[] {fom}, mim, timeImplementation);
         }
         created = true;
         monitor.joinFederationExecution(monitorName, "java-tck", federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_FEDERATE_CLASS);
         final ObjectClassHandle expectedMomClass = momClass;
         AttributeHandle federateHandleAttribute = monitor.getAttributeHandle(
            momClass, FEDERATE_HANDLE);
         AttributeHandle actionAttribute = monitor.getAttributeHandle(
            momClass, AUTOMATIC_RESIGN_ACTION);
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_FEDERATE_CLASS.equals(monitor.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(monitor.getAttributeName(momClass,
                  federateHandleAttribute))
               && AUTOMATIC_RESIGN_ACTION.equals(monitor.getAttributeName(momClass,
                  actionAttribute))
               && reliable != null && "HLAreliable".equals(
                  monitor.getTransportationTypeName(reliable)),
            "Automatic Resign Action MOM lookup did not round-trip standard MIM handles");
         subscription = JavaTckSupport.attributeSet(
            monitor, federateHandleAttribute, actionAttribute);
         monitor.subscribeObjectClassAttributes(momClass, subscription, "");
         subscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         byte[] encodedFederate = encode(subjectFederate);
         await(monitor, callbackModel,
            () -> recorder.findMomObject(federateHandleAttribute, encodedFederate,
               expectedMomClass) != null,
            "subject federate MOM object discovery");
         ObjectInstanceHandle subjectMomObject = recorder.findMomObject(
            federateHandleAttribute, encodedFederate, momClass);
         JavaTckSupport.check(subjectMomObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(subjectMomObject)),
            "Automatic Resign Action discovery returned the wrong standard MOM object");

         ResignAction initialAction = subject.getAutomaticResignDirective();
         requestAndVerify(monitor, callbackModel, recorder, subjectMomObject, actionAttribute,
            reliable, initialAction, encoder, "initial direct MOM value request");

         ResignAction changedAction = initialAction == ResignAction.NO_ACTION
            ? ResignAction.DELETE_OBJECTS : ResignAction.NO_ACTION;
         int before = recorder.reflections.size();
         subject.setAutomaticResignDirective(changedAction);
         JavaTckSupport.check(subject.getAutomaticResignDirective() == changedAction,
            "setAutomaticResignDirective did not retain the requested standard action");
         final int changedStart = before;
         await(monitor, callbackModel,
            () -> recorder.valueReflectionsSince(changedStart, subjectMomObject, actionAttribute)
               .size() > 0,
            "conditional Automatic Resign Action reflection");
         List<Reflection> changed = recorder.valueReflectionsSince(
            changedStart, subjectMomObject, actionAttribute);
         JavaTckSupport.check(changed.size() == 1,
            "Automatic Resign Action change did not produce exactly one conditional reflection");
         verifyReflection(changed.get(0), subjectMomObject, actionAttribute, reliable,
            changedAction, encoder, "conditional MOM reflection");
         requestAndVerify(monitor, callbackModel, recorder, subjectMomObject, actionAttribute,
            reliable, changedAction, encoder, "changed direct MOM value request");

         before = recorder.reflections.size();
         subject.setAutomaticResignDirective(initialAction);
         JavaTckSupport.check(subject.getAutomaticResignDirective() == initialAction,
            "setAutomaticResignDirective did not restore the initial standard action");
         final int restoredStart = before;
         await(monitor, callbackModel,
            () -> recorder.valueReflectionsSince(restoredStart, subjectMomObject, actionAttribute)
               .size() > 0,
            "restored conditional Automatic Resign Action reflection");
         List<Reflection> restored = recorder.valueReflectionsSince(
            restoredStart, subjectMomObject, actionAttribute);
         JavaTckSupport.check(restored.size() == 1,
            "restoring Automatic Resign Action did not produce exactly one conditional reflection");
         verifyReflection(restored.get(0), subjectMomObject, actionAttribute, reliable,
            initialAction, encoder, "restored conditional MOM reflection");
         requestAndVerify(monitor, callbackModel, recorder, subjectMomObject, actionAttribute,
            reliable, initialAction, encoder, "restored direct MOM value request");
      } finally {
         if (subscribed && monitorJoined && momClass != null && subscription != null) {
            try { monitor.unsubscribeObjectClassAttributes(momClass, subscription); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (monitorJoined) {
            try { monitor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { monitor.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); }
            catch (Exception ignored) { }
         }
         if (monitorConnected) {
            try { monitor.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void requestAndVerify(RTIambassador monitor, CallbackModel callbackModel,
         Recorder recorder, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, ResignAction expected, EncoderFactory encoder,
         String description) throws Exception {
      int before = recorder.reflections.size();
      monitor.requestAttributeValueUpdate(object, JavaTckSupport.attributeSet(monitor, attribute),
         new byte[0]);
      await(monitor, callbackModel,
         () -> recorder.valueReflectionsSince(before, object, attribute).size() > 0,
         description);
      List<Reflection> values = recorder.valueReflectionsSince(before, object, attribute);
      JavaTckSupport.check(values.size() == 1,
         description + " delivered an unexpected number of direct MOM reflections");
      verifyReflection(values.get(0), object, attribute, reliable, expected, encoder, description);
   }

   private static void verifyReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, TransportationTypeHandle reliable, ResignAction expected,
         EncoderFactory encoder, String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && reflection.values.containsKey(attribute)
            && Arrays.equals(new byte[0], reflection.tag)
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty())
            && !reflection.timestampOrdered,
         description + " had non-standard MOM reflection metadata");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(reflection.values.get(attribute));
      JavaTckSupport.check(value.getValue() == mimValue(expected),
         description + " returned the wrong standard HLAresignAction value");
   }

   private static int mimValue(ResignAction action) {
      if (action == ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES) return 0;
      if (action == ResignAction.DELETE_OBJECTS) return 1;
      if (action == ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS) return 2;
      if (action == ResignAction.DELETE_OBJECTS_THEN_DIVEST) return 3;
      if (action == ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST) return 4;
      if (action == ResignAction.NO_ACTION) return 5;
      throw new AssertionError("unmapped standard ResignAction: " + action);
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] result = new byte[handle.encodedLength()];
      handle.encode(result, 0);
      return result;
   }

   private static void await(RTIambassador rti, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
         else Thread.yield();
         Thread.sleep(1L);
      }
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> objectMethod(proxy, method.getName(), args));
   }

   private static Object objectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK automatic-resign MOM subject callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final boolean timestampOrdered;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, boolean timestampOrdered) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions;
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final Map<ObjectInstanceHandle, ObjectClassHandle> discovered =
         new java.util.concurrent.ConcurrentHashMap<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("discoverObjectInstance".equals(method.getName()) && args != null
                     && args.length >= 2) {
                  discovered.put((ObjectInstanceHandle) args[0], (ObjectClassHandle) args[1]);
               } else if ("reflectAttributeValues".equals(method.getName()) && args != null
                     && args.length >= 6 && args[1] instanceof AttributeHandleValueMap) {
                  AttributeHandleValueMap source = (AttributeHandleValueMap) args[1];
                  Map<AttributeHandle, byte[]> copied = new LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     copied.put(handle, source.get(handle).clone());
                  }
                  reflections.add(new Reflection((ObjectInstanceHandle) args[0], copied,
                     ((byte[]) args[2]).clone(), (TransportationTypeHandle) args[3],
                     (FederateHandle) args[4], (RegionHandleSet) args[5], args.length != 6));
               }
               return objectMethod(proxy, method.getName(), args);
            });
      }

      private ObjectInstanceHandle findMomObject(AttributeHandle federateHandleAttribute,
            byte[] encodedFederate, ObjectClassHandle expectedClass) {
         for (Reflection reflection : reflections) {
            byte[] actual = reflection.values.get(federateHandleAttribute);
            if (actual != null && Arrays.equals(encodedFederate, actual)
                  && expectedClass.equals(discovered.get(reflection.object))) {
               return reflection.object;
            }
         }
         return null;
      }

      private List<Reflection> valueReflectionsSince(int first, ObjectInstanceHandle object,
            AttributeHandle attribute) {
         List<Reflection> result = new ArrayList<>();
         for (int index = Math.min(first, reflections.size()); index < reflections.size(); ++index) {
            Reflection reflection = reflections.get(index);
            if (object.equals(reflection.object) && reflection.values.containsKey(attribute)) {
               result.add(reflection);
            }
         }
         return result;
      }
   }
}
