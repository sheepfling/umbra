package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the Table 7 S-leaf Change Attribute Order Type request through the Java API. */
final class FederationMomChangeAttributeOrderTypeTck {
   private static final String CHANGE_ORDER =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType";
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private FederationMomChangeAttributeOrderTypeTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "Change Attribute Order Type requires adapter-supplied logical-time configuration");
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-change-attribute-order-" + suffix;
      String subjectName = "java-tck-mom-change-attribute-order-subject-" + suffix;
      String observerName = "java-tck-mom-change-attribute-order-observer-" + suffix;
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean attributesPublished = false;
      boolean attributesSubscribed = false;
      boolean requestPublished = false;
      boolean regulating = false;
      ObjectClassHandle subjectClass = null;
      ObjectClassHandle observerClass = null;
      AttributeHandle subjectAttribute = null;
      AttributeHandle observerAttribute = null;
      AttributeHandleSet subjectAttributes = null;
      AttributeHandleSet observerAttributes = null;
      InteractionClassHandle request = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         subjectClass = subject.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         subjectAttribute = subject.getAttributeHandle(subjectClass, attributeName);
         observerAttribute = observer.getAttributeHandle(observerClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(subject.getObjectClassName(subjectClass))
               && objectClassName.equals(observer.getObjectClassName(observerClass))
               && attributeName.equals(subject.getAttributeName(subjectClass, subjectAttribute))
               && attributeName.equals(observer.getAttributeName(observerClass, observerAttribute))
               && subjectClass.equals(observerClass)
               && subjectAttribute.equals(observerAttribute),
            "adapter FOM did not provide matching object-class and attribute handles");
         subjectAttributes = JavaTckSupport.attributeSet(subject, subjectAttribute);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);
         subject.publishObjectClassAttributes(subjectClass, subjectAttributes);
         attributesPublished = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes, "");
         attributesSubscribed = true;
         ObjectInstanceHandle object = subject.registerObjectInstance(subjectClass);
         await(observer, callbackModel,
            () -> observerRecorder.discoveries.contains(object),
            "Change Attribute Order Type target discovery");

         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         subject.enableTimeRegulation(epsilon);
         regulating = true;
         await(subject, callbackModel, () -> subjectRecorder.timeRegulationEnabled,
            "sender time-regulation callback");
         LogicalTime timestamp = offset(subject.queryLogicalTime(), epsilon, 5);

         request = subject.getInteractionClassHandle(CHANGE_ORDER);
         ParameterHandle federateParameter = subject.getParameterHandle(request, "HLAfederate");
         ParameterHandle objectParameter = subject.getParameterHandle(request, "HLAobjectInstance");
         ParameterHandle attributesParameter = subject.getParameterHandle(request, "HLAattributeList");
         ParameterHandle orderParameter = subject.getParameterHandle(request, "HLAsendOrder");
         JavaTckSupport.check(request != null && objectParameter != null
               && federateParameter != null && attributesParameter != null && orderParameter != null,
            "standard MIM omitted Change Attribute Order Type request handles");
         subject.publishInteractionClass(request);
         requestPublished = true;

         ParameterHandleValueMap requestParameters =
            subject.getParameterHandleValueMapFactory().create(4);
         requestParameters.put(federateParameter, encode(subjectHandle));
         requestParameters.put(objectParameter, encode(object));
         requestParameters.put(attributesParameter, encodeAttributeHandleList(subjectAttribute));
         requestParameters.put(orderParameter,
            factory.getEncoderFactory().createHLAinteger32BE(1).toByteArray());
         subject.sendInteraction(request, requestParameters, new byte[0]);

         byte[] value = new byte[] {0x4d, 0x4f, 0x44, 0x41};
         byte[] tag = new byte[] {0x4f, 0x52};
         AttributeHandleValueMap values = subject.getAttributeHandleValueMapFactory().create(1);
         values.put(subjectAttribute, value.clone());
         MessageRetractionReturn retraction = subject.updateAttributeValues(
            object, values, tag.clone(), timestamp);
         JavaTckSupport.check(retraction != null && !retraction.retractionHandleIsValid,
            "MOM-selected RECEIVE order unexpectedly returned a retraction designator");

         await(observer, callbackModel,
            () -> !observerRecorder.timestampedReflections.isEmpty(),
            "Change Attribute Order Type timestamp-bearing update reflection");
         settle(observer, callbackModel);
         JavaTckSupport.check(observerRecorder.timestampedReflections.size() == 1
               && observerRecorder.receiveOrderReflections.isEmpty(),
            "timestamp-bearing update did not use exactly one timestamped reflection callback");
         ReflectionDelivery delivery = observerRecorder.timestampedReflections.get(0);
         JavaTckSupport.check(object.equals(delivery.object)
               && delivery.values.size() == 1
               && Arrays.equals(value, delivery.values.get(observerAttribute))
               && Arrays.equals(tag, delivery.tag)
               && timestamp.equals(delivery.time)
               && subjectHandle.equals(delivery.producer)
               && delivery.sentOrder == OrderType.RECEIVE
               && delivery.receivedOrder == OrderType.RECEIVE
               && delivery.retraction == null,
            "HLAchangeAttributeOrderType did not change the later attribute delivery to RECEIVE order");

         subject.disableTimeRegulation();
         regulating = false;
         observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes);
         attributesSubscribed = false;
         subject.unpublishObjectClassAttributes(subjectClass, subjectAttributes);
         attributesPublished = false;
         subject.unpublishInteractionClass(request);
         requestPublished = false;
      } finally {
         if (regulating && subjectJoined) {
            try { subject.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (requestPublished && request != null) {
            try { subject.unpublishInteractionClass(request); } catch (Exception ignored) { }
         }
         if (attributesSubscribed && observerClass != null && observerAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes); }
            catch (Exception ignored) { }
         }
         if (attributesPublished && subjectClass != null && subjectAttributes != null) {
            try { subject.unpublishObjectClassAttributes(subjectClass, subjectAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static byte[] encode(ObjectInstanceHandle object) {
      byte[] encoded = new byte[object.encodedLength()];
      object.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encode(FederateHandle federate) {
      byte[] encoded = new byte[federate.encodedLength()];
      federate.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encodeAttributeHandleList(AttributeHandle attribute) {
      byte[] encodedAttribute = new byte[attribute.encodedLength()];
      attribute.encode(encodedAttribute, 0);
      ByteBuffer encodedList = ByteBuffer.allocate(4 + encodedAttribute.length)
         .order(ByteOrder.BIG_ENDIAN);
      encodedList.putInt(1);
      encodedList.put(encodedAttribute);
      return encodedList.array();
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.sleep(25L);
   }

   private static final class ReflectionDelivery {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private ReflectionDelivery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
         source.forEach((handle, value) -> values.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ReflectionDelivery> timestampedReflections = new CopyOnWriteArrayList<>();
      private final List<Object[]> receiveOrderReflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2 && arguments[0] instanceof ObjectInstanceHandle) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestampedReflections.add(new ReflectionDelivery(arguments));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrderReflections.add(arguments.clone());
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK MOM attribute-order callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
