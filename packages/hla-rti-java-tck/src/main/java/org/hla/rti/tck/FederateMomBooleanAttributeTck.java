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
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.exceptions.InvalidFederateHandle;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Tests standard joined-federate MOM values for Boolean support switches. */
final class FederateMomBooleanAttributeTck {
   private static final String MOM_FEDERATE_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String MOM_FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String TIME_CONSTRAINED_ATTRIBUTE = "HLAtimeConstrained";
   private static final String TIME_REGULATING_ATTRIBUTE = "HLAtimeRegulating";
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   @FunctionalInterface
   interface Getter { boolean get(RTIambassador ambassador) throws Exception; }
   @FunctionalInterface
   interface Setter { void set(RTIambassador ambassador, boolean value) throws Exception; }

   private FederateMomBooleanAttributeTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String attributeName, boolean booleanEncoding, Getter getter, Setter setter,
         CallbackModel callbackModel)
         throws Exception {
      RTIambassador monitor = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String federation = "java-tck-federate-mom-boolean-" + UUID.randomUUID();
      boolean monitorConnected = false, subjectConnected = false, created = false;
      boolean monitorJoined = false, subjectJoined = false, subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet subscription = null;
      try {
         monitor.connect(recorder.proxy(), callbackModel);
         monitorConnected = true;
         subject.connect(recorder.proxy(), callbackModel);
         subjectConnected = true;
         monitor.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         monitor.joinFederationExecution("boolean-mom-monitor", "java-tck", federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_FEDERATE_CLASS);
         AttributeHandle federateHandleAttribute = monitor.getAttributeHandle(momClass,
            MOM_FEDERATE_HANDLE);
         AttributeHandle valueAttribute = monitor.getAttributeHandle(momClass, attributeName);
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_FEDERATE_CLASS.equals(monitor.getObjectClassName(momClass))
               && MOM_FEDERATE_HANDLE.equals(monitor.getAttributeName(momClass,
                  federateHandleAttribute))
               && attributeName.equals(monitor.getAttributeName(momClass, valueAttribute))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            attributeName + " MOM lookup did not round-trip standard MIM handles");
         subscription = JavaTckSupport.attributeSet(monitor, federateHandleAttribute,
            valueAttribute);
         monitor.subscribeObjectClassAttributes(momClass, subscription, "");
         subscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "boolean-mom-subject", "java-tck", federation);
         subjectJoined = true;
         byte[] encodedSubject = new byte[subjectFederate.encodedLength()];
         subjectFederate.encode(encodedSubject, 0);
         await(monitor, callbackModel, () -> recorder.momObject(federateHandleAttribute,
            encodedSubject) != null, "subject federate MOM object discovery");
         ObjectInstanceHandle momObject = recorder.momObject(federateHandleAttribute,
            encodedSubject);
         JavaTckSupport.check(momObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(momObject)),
            attributeName + " discovery returned the wrong standard MOM object");

         boolean initialValue;
         if (getter == null) {
            initialValue = requestAndVerify(monitor, callbackModel, recorder, momObject,
               valueAttribute, reliable, null, booleanEncoding, encoder,
               "initial direct MOM attribute request");
         } else {
            initialValue = getter.get(subject);
            requestAndVerify(monitor, callbackModel, recorder, momObject, valueAttribute,
               reliable, initialValue, booleanEncoding, encoder,
               "initial direct MOM attribute request");
         }
         transition(monitor, subject, callbackModel, recorder, momObject, valueAttribute,
            reliable, attributeName, !initialValue, booleanEncoding, getter, setter, encoder,
            "conditional MOM reflection");
         transition(monitor, subject, callbackModel, recorder, momObject, valueAttribute,
            reliable, attributeName, initialValue, booleanEncoding, getter, setter, encoder,
            "restored conditional MOM reflection");

         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && monitorJoined && momClass != null && subscription != null) try {
            monitor.unsubscribeObjectClassAttributes(momClass, subscription);
         } catch (Exception ignored) { }
         if (subjectJoined) try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (monitorJoined) try { monitor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { monitor.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (subjectConnected) try { subject.disconnect(); } catch (Exception ignored) { }
         if (monitorConnected) try { monitor.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void runAsynchronousDelivery(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, "HLAasynchronousDelivery", true, null,
         (ambassador, enabled) -> {
            if (enabled) ambassador.enableAsynchronousDelivery();
            else ambassador.disableAsynchronousDelivery();
         }, callbackModel);
   }

   static void runTimeConstrained(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, TIME_CONSTRAINED_ATTRIBUTE, true, null,
         (ambassador, enabled) -> {
            if (enabled) ambassador.enableTimeConstrained();
            else ambassador.disableTimeConstrained();
         }, callbackModel);
   }

   static void runTimeRegulating(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, TIME_REGULATING_ATTRIBUTE, true, null,
         (ambassador, enabled) -> {
            if (enabled) {
               LogicalTimeInterval epsilon =
                  (LogicalTimeInterval) ambassador.getTimeFactory().makeEpsilon();
               ambassador.enableTimeRegulation(epsilon);
            } else {
               ambassador.disableTimeRegulation();
            }
         }, callbackModel);
   }

   private static void transition(RTIambassador monitor, RTIambassador subject,
         CallbackModel model, Recorder recorder, ObjectInstanceHandle object,
         AttributeHandle attribute, TransportationTypeHandle reliable, String attributeName,
         boolean expected, boolean booleanEncoding, Getter getter, Setter setter,
         EncoderFactory encoder, String description) throws Exception {
      int before = recorder.reflections.size();
      int timeCallbacksBefore = TIME_CONSTRAINED_ATTRIBUTE.equals(attributeName)
         ? recorder.timeConstrainedEnabled.size() : recorder.timeRegulationEnabled.size();
      setter.set(subject, expected);
      if (TIME_CONSTRAINED_ATTRIBUTE.equals(attributeName)
            || TIME_REGULATING_ATTRIBUTE.equals(attributeName)) {
         if (expected) {
            List<Boolean> timeCallbacks = TIME_CONSTRAINED_ATTRIBUTE.equals(attributeName)
               ? recorder.timeConstrainedEnabled : recorder.timeRegulationEnabled;
            await(subject, model, () -> timeCallbacks.size() >= timeCallbacksBefore + 1,
               TIME_CONSTRAINED_ATTRIBUTE.equals(attributeName)
                  ? "timeConstrainedEnabled callback after Enable Time Constrained"
                  : "timeRegulationEnabled callback after Enable Time Regulation");
            JavaTckSupport.check(timeCallbacks.size()
                  == timeCallbacksBefore + 1,
               "time-role enable service delivered an unexpected number of callbacks");
         } else {
            List<Boolean> timeCallbacks = TIME_CONSTRAINED_ATTRIBUTE.equals(attributeName)
               ? recorder.timeConstrainedEnabled : recorder.timeRegulationEnabled;
            JavaTckSupport.check(timeCallbacks.size() == timeCallbacksBefore,
               "time-role disable service delivered an enable callback");
         }
      }
      if (getter != null) JavaTckSupport.check(getter.get(subject) == expected,
         attributeName + " public setter did not retain the requested value");
      awaitAndVerifyValue(monitor, model, recorder, before, object, attribute, reliable,
         expected, booleanEncoding, encoder, attributeName + " " + description);
      requestAndVerify(monitor, model, recorder, object, attribute, reliable, expected,
         booleanEncoding, encoder, attributeName + " direct value request after " + description);
   }

   private static boolean requestAndVerify(RTIambassador monitor, CallbackModel model,
         Recorder recorder, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, Boolean expected, boolean booleanEncoding,
         EncoderFactory encoder,
         String description) throws Exception {
      int before = recorder.reflections.size();
      monitor.requestAttributeValueUpdate(object, JavaTckSupport.attributeSet(monitor, attribute),
         new byte[0]);
      await(monitor, model, () -> recorder.valueReflectionsSince(before, object, attribute).size() > 0,
         description);
      List<Reflection> reflections = recorder.valueReflectionsSince(before, object, attribute);
      JavaTckSupport.check(reflections.size() == 1,
         description + " delivered an unexpected number of direct MOM reflections");
      return validateReflection(monitor, reflections.get(0), object, attribute, reliable, expected,
         booleanEncoding, encoder, description);
   }

   private static void awaitAndVerifyValue(RTIambassador monitor, CallbackModel model,
         Recorder recorder, int before, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, boolean expected, boolean booleanEncoding,
         EncoderFactory encoder,
         String description) throws Exception {
      await(monitor, model, () -> recorder.valueReflectionsSince(before, object, attribute).size() > 0,
         description);
      List<Reflection> reflections = recorder.valueReflectionsSince(before, object, attribute);
      JavaTckSupport.check(reflections.size() == 1,
         description + " did not produce exactly one conditional MOM reflection");
      validateReflection(monitor, reflections.get(0), object, attribute, reliable, expected,
         booleanEncoding, encoder, description);
   }

   private static boolean validateReflection(RTIambassador monitor, Reflection reflection,
         ObjectInstanceHandle object,
         AttributeHandle attribute, TransportationTypeHandle reliable, Boolean expected,
         boolean booleanEncoding, EncoderFactory encoder, String description) throws Exception {
      RegionHandleSet regions = reflection.regions;
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(new byte[0], reflection.tag)
            && reliable.equals(reflection.transportation)
            && (regions == null || regions.isEmpty()) && !reflection.timestampOrdered,
         description + " had non-standard MOM reflection metadata: objectMatch="
            + object.equals(reflection.object) + ", attributeCount="
            + reflection.values.size() + ", tagLength=" + reflection.tag.length
            + ", reliable=" + reliable.equals(reflection.transportation)
            + ", regionsEmpty=" + (regions == null || regions.isEmpty())
            + ", timestampOrdered=" + reflection.timestampOrdered);
      if (reflection.producer != null) {
         boolean invalidProducer = false;
         try {
            monitor.getFederateName(reflection.producer);
         } catch (InvalidFederateHandle expectedInvalidHandle) {
            invalidProducer = true;
         }
         JavaTckSupport.check(invalidProducer,
            description + " supplied a valid producing FederateHandle for an RTI-originated MOM reflection");
      }
      boolean actual;
      if (booleanEncoding) {
         HLAboolean value = encoder.createHLAboolean();
         value.decode(reflection.values.get(attribute));
         actual = value.getValue();
      } else {
         HLAinteger32BE value = encoder.createHLAinteger32BE();
         value.decode(reflection.values.get(attribute));
         actual = value.getValue() != 0;
      }
      if (expected != null) JavaTckSupport.check(actual == expected,
         description + " returned the wrong standard Boolean value");
      return actual;
   }

   private static void await(RTIambassador monitor, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(monitor);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final boolean timestampOrdered;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, boolean timestampOrdered) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Boolean> timeConstrainedEnabled = new CopyOnWriteArrayList<>();
      private final List<Boolean> timeRegulationEnabled = new CopyOnWriteArrayList<>();

      private ObjectInstanceHandle momObject(AttributeHandle federateHandleAttribute,
            byte[] expectedHandle) {
         for (Reflection reflection : reflections) {
            byte[] actualHandle = reflection.values.get(federateHandleAttribute);
            if (actualHandle != null && Arrays.equals(expectedHandle, actualHandle))
               return reflection.object;
         }
         return null;
      }

      private List<Reflection> valueReflectionsSince(int first, ObjectInstanceHandle object,
            AttributeHandle attribute) {
         List<Reflection> result = new ArrayList<>();
         for (int i = Math.min(first, reflections.size()); i < reflections.size(); ++i) {
            Reflection reflection = reflections.get(i);
            if (reflection.object.equals(object) && reflection.values.containsKey(attribute))
               result.add(reflection);
         }
         return result;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("reflectAttributeValues".equals(name) && args != null && args.length >= 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) args[0],
                     (AttributeHandleValueMap) args[1], (byte[]) args[2],
                     (TransportationTypeHandle) args[3], (FederateHandle) args[4],
                     (RegionHandleSet) args[5], args.length != 6));
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled.add(Boolean.TRUE);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled.add(Boolean.TRUE);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK federate MOM callback recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
