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
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks that HLAservice acts in the member named by its inherited HLAfederate parameter. */
final class FederationMomServiceActorIdentityTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception";
   private static final String CHANGE_ATTRIBUTE_ORDER_TYPE =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType";
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private FederationMomServiceActorIdentityTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "HLAservice actor-identity testing requires adapter-supplied logical-time configuration");

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador invoker = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder invokerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-service-actor-" + suffix;
      String ownerName = "java-tck-mom-service-owner-" + suffix;
      String invokerName = "java-tck-mom-service-invoker-" + suffix;
      String observerName = "java-tck-mom-service-observer-" + suffix;

      boolean ownerConnected = false;
      boolean invokerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean invokerJoined = false;
      boolean observerJoined = false;
      boolean attributesPublished = false;
      boolean attributesSubscribed = false;
      boolean ownerReportSubscribed = false;
      boolean observerReportSubscribed = false;
      boolean requestPublished = false;
      boolean ownerRegulating = false;
      FederateHandle ownerFederate = null;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle invokerClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle invokerAttribute = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet invokerAttributes = null;
      InteractionClassHandle ownerReportClass = null;
      InteractionClassHandle observerReportClass = null;
      InteractionClassHandle serviceClass = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         invoker.connect(invokerRecorder.proxy(), callbackModel);
         invokerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         ownerFederate = owner.joinFederationExecution(ownerName, "java-tck", federation);
         ownerJoined = true;
         FederateHandle invokerFederate = invoker.joinFederationExecution(
            invokerName, "java-tck", federation);
         invokerJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         for (RTIambassador member : new RTIambassador[] {owner, invoker, observer}) {
            member.setServiceReportingSwitch(false);
            member.setSendServiceReportsToFileSwitch(false);
         }

         ownerClass = owner.getObjectClassHandle(objectClassName);
         invokerClass = invoker.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         invokerAttribute = invoker.getAttributeHandle(invokerClass, attributeName);
         JavaTckSupport.check(ownerClass != null && invokerClass != null
               && ownerAttribute != null && invokerAttribute != null
               && ownerClass.equals(invokerClass) && ownerAttribute.equals(invokerAttribute),
            "adapter FOM did not provide one shared object class and attribute");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         invokerAttributes = JavaTckSupport.attributeSet(invoker, invokerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         attributesPublished = true;
         invoker.subscribeObjectClassAttributes(invokerClass, invokerAttributes, "");
         attributesSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "HLAservice actor-identity target registration returned no object handle");
         await(invoker, callbackModel, () -> invokerRecorder.discoveries.contains(object),
            "the HLAservice target object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !invoker.isAttributeOwnedByFederate(object, invokerAttribute),
            "the HLAservice target attribute was not exclusively owned by another member");

         ownerReportClass = owner.getInteractionClassHandle(REPORT_CLASS);
         observerReportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle ownerFederateParameter = owner.getParameterHandle(
            ownerReportClass, "HLAfederate");
         ParameterHandle ownerServiceParameter = owner.getParameterHandle(
            ownerReportClass, "HLAservice");
         ParameterHandle ownerExceptionParameter = owner.getParameterHandle(
            ownerReportClass, "HLAexception");
         ParameterHandle ownerParameterErrorParameter = owner.getParameterHandle(
            ownerReportClass, "HLAparameterError");
         ParameterHandle observerFederateParameter = observer.getParameterHandle(
            observerReportClass, "HLAfederate");
         ParameterHandle observerServiceParameter = observer.getParameterHandle(
            observerReportClass, "HLAservice");
         ParameterHandle observerExceptionParameter = observer.getParameterHandle(
            observerReportClass, "HLAexception");
         ParameterHandle observerParameterErrorParameter = observer.getParameterHandle(
            observerReportClass, "HLAparameterError");
         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(ownerReportClass != null && observerReportClass != null
               && ownerReportClass.equals(observerReportClass)
               && ownerFederateParameter != null && ownerServiceParameter != null
               && ownerExceptionParameter != null && ownerParameterErrorParameter != null
               && observerFederateParameter != null && observerServiceParameter != null
               && observerExceptionParameter != null && observerParameterErrorParameter != null
               && reliable != null,
            "standard MIM HLAreportMOMexception handles were unavailable");
         owner.subscribeInteractionClass(ownerReportClass);
         ownerReportSubscribed = true;
         observer.subscribeInteractionClass(observerReportClass);
         observerReportSubscribed = true;

         serviceClass = invoker.getInteractionClassHandle(CHANGE_ATTRIBUTE_ORDER_TYPE);
         ParameterHandle targetFederateParameter = invoker.getParameterHandle(
            serviceClass, "HLAfederate");
         ParameterHandle objectParameter = invoker.getParameterHandle(
            serviceClass, "HLAobjectInstance");
         ParameterHandle attributesParameter = invoker.getParameterHandle(
            serviceClass, "HLAattributeList");
         ParameterHandle orderParameter = invoker.getParameterHandle(
            serviceClass, "HLAsendOrder");
         JavaTckSupport.check(serviceClass != null && targetFederateParameter != null
               && objectParameter != null
               && attributesParameter != null && orderParameter != null,
            "standard MIM omitted inherited HLAfederate or Change Attribute Order Type parameters");
         invoker.publishInteractionClass(serviceClass);
         requestPublished = true;

         ParameterHandleValueMap requestParameters =
            invoker.getParameterHandleValueMapFactory().create(4);
         requestParameters.put(targetFederateParameter, encode(invokerFederate));
         requestParameters.put(objectParameter, encode(object));
         requestParameters.put(attributesParameter, encodeAttributeHandleList(invokerAttribute));
         HLAinteger32BE receiveOrder = encoder.createHLAinteger32BE(1);
         requestParameters.put(orderParameter, receiveOrder.toByteArray());

         // HLAservice has no separate HLAfederate parameter. Target another member's
         // owned attribute to distinguish the sender context from the object's owner.
         boolean attributeNotOwned = false;
         ownerRecorder.interactions.clear();
         observerRecorder.interactions.clear();
         try {
            invoker.sendInteraction(serviceClass, requestParameters, new byte[0]);
         } catch (Exception serviceFailure) {
            // Java sendInteraction does not declare AttributeNotOwned. Preserve the
            // observable service exception check through any provider/API wrapper chain.
            for (Throwable current = serviceFailure; current != null; current = current.getCause()) {
               if ("AttributeNotOwned".equals(current.getClass().getSimpleName())) {
                  attributeNotOwned = true;
                  break;
               }
            }
         }
         JavaTckSupport.check(attributeNotOwned,
            "HLAchangeAttributeOrderType did not raise AttributeNotOwned for the sender");

         final InteractionClassHandle expectedOwnerReport = ownerReportClass;
         final InteractionClassHandle expectedObserverReport = observerReportClass;
         await(owner, callbackModel,
            () -> ownerRecorder.hasInteraction(expectedOwnerReport),
            "the owner HLAreportMOMexception callback");
         await(observer, callbackModel,
            () -> observerRecorder.hasInteraction(expectedObserverReport),
            "the observer HLAreportMOMexception callback");
         settle(owner, callbackModel);
         settle(observer, callbackModel);
         List<Interaction> ownerReports = ownerRecorder.interactions();
         List<Interaction> observerReports = observerRecorder.interactions();
         JavaTckSupport.check(ownerReports.size() == 1 && observerReports.size() == 1,
            "the failed HLAservice invocation was not reported exactly once to both subscribers");

         verifyReport(owner, ownerReports.get(0), ownerReportClass,
            ownerFederateParameter, ownerServiceParameter, ownerExceptionParameter,
            ownerParameterErrorParameter, invokerFederate, reliable, encoder, "owner");
         verifyReport(observer, observerReports.get(0), observerReportClass,
            observerFederateParameter, observerServiceParameter, observerExceptionParameter,
            observerParameterErrorParameter, invokerFederate, reliable, encoder, "observer");

         // A different sender now names the object owner as the target actor.
         // The target owner must be able to change its own attribute's order,
         // and that change is observed through a later public update callback.
         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         owner.enableTimeRegulation(epsilon);
         ownerRegulating = true;
         await(owner, callbackModel, () -> ownerRecorder.timeRegulationEnabled,
            "designated owner time-regulation callback");
         LogicalTime timestamp = offset(owner.queryLogicalTime(), epsilon, 5);

         ParameterHandleValueMap ownerRequest =
            invoker.getParameterHandleValueMapFactory().create(4);
         ownerRequest.put(targetFederateParameter, encode(ownerFederate));
         ownerRequest.put(objectParameter, encode(object));
         ownerRequest.put(attributesParameter, encodeAttributeHandleList(ownerAttribute));
         ownerRequest.put(orderParameter, encoder.createHLAinteger32BE(1).toByteArray());
         invoker.sendInteraction(serviceClass, ownerRequest, new byte[0]);

         byte[] value = new byte[] {0x4d, 0x4f, 0x44, 0x41};
         byte[] tag = new byte[] {0x4f, 0x52};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, value.clone());
         MessageRetractionReturn retraction = owner.updateAttributeValues(
            object, values, tag.clone(), timestamp);
         JavaTckSupport.check(retraction != null && !retraction.retractionHandleIsValid,
            "the designated owner did not apply RECEIVE order to its attribute");

         await(invoker, callbackModel,
            () -> !invokerRecorder.timestampedReflections.isEmpty(),
            "the HLAservice-designated owner's timestamp-bearing reflection");
         settle(invoker, callbackModel);
         JavaTckSupport.check(invokerRecorder.timestampedReflections.size() == 1
               && invokerRecorder.receiveOrderReflections.isEmpty(),
            "the designated-owner update did not use exactly one timestamped reflection");
         ReflectionDelivery delivery = invokerRecorder.timestampedReflections.get(0);
         JavaTckSupport.check(object.equals(delivery.object)
               && delivery.values.size() == 1
               && java.util.Arrays.equals(value, delivery.values.get(invokerAttribute))
               && java.util.Arrays.equals(tag, delivery.tag)
               && timestamp.equals(delivery.time)
               && ownerFederate.equals(delivery.producer)
               && delivery.sentOrder == OrderType.RECEIVE
               && delivery.receivedOrder == OrderType.RECEIVE
               && delivery.retraction == null,
            "HLAchangeAttributeOrderType did not execute as the HLAfederate-designated member");
      } finally {
         if (ownerRegulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (invokerJoined && attributesSubscribed && invokerClass != null
               && invokerAttributes != null) {
            try { invoker.unsubscribeObjectClassAttributes(invokerClass, invokerAttributes); }
            catch (Exception ignored) { }
         }
         if (invokerJoined && requestPublished && serviceClass != null) {
            try { invoker.unpublishInteractionClass(serviceClass); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && attributesPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerReportSubscribed && ownerReportClass != null) {
            try { owner.unsubscribeInteractionClass(ownerReportClass); }
            catch (Exception ignored) { }
         }
         if (observerJoined && observerReportSubscribed && observerReportClass != null) {
            try { observer.unsubscribeInteractionClass(observerReportClass); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (invokerJoined) {
            try { invoker.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try {
               owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (invokerConnected) {
            try { invoker.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyReport(RTIambassador reporter, Interaction report,
         InteractionClassHandle expectedClass, ParameterHandle federateParameter,
         ParameterHandle serviceParameter, ParameterHandle exceptionParameter,
         ParameterHandle parameterErrorParameter, FederateHandle expectedFederate,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(expectedClass.equals(report.interaction)
            && report.parameters.size() == 4
            && report.parameters.containsKey(federateParameter)
            && report.parameters.containsKey(serviceParameter)
            && report.parameters.containsKey(exceptionParameter)
            && report.parameters.containsKey(parameterErrorParameter)
            && reliable.equals(report.transportation)
            && report.tag.length == 0 && report.regionsEmpty,
         description + " received a non-standard HLAreportMOMexception parameter set or metadata");
      JavaTckSupport.requireNoProducingFederate(
         reporter, report.producer, description + " RTI-originated HLAreportMOMexception");

      FederateHandle reportedFederate = reporter.getFederateHandleFactory().decode(
         report.parameters.get(federateParameter), 0);
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(serviceParameter));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(exceptionParameter));
      HLAboolean parameterError = encoder.createHLAboolean();
      parameterError.decode(report.parameters.get(parameterErrorParameter));
      JavaTckSupport.check(expectedFederate.equals(reportedFederate)
            && CHANGE_ATTRIBUTE_ORDER_TYPE.equals(service.getValue())
            && exception.getValue() != null && !exception.getValue().isEmpty()
            && !parameterError.getValue(),
         description + " HLAreportMOMexception did not identify the sending federate and service");
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

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
         for (ParameterHandle handle : source.keySet()) {
            parameters.put(handle, source.get(handle).clone());
         }
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet regions = (RegionHandleSet) arguments[5];
         regionsEmpty = regions == null || regions.isEmpty();
      }
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
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<ReflectionDelivery> timestampedReflections =
         new CopyOnWriteArrayList<>();
      private final List<Object[]> receiveOrderReflections = new CopyOnWriteArrayList<>();

      private boolean hasInteraction(InteractionClassHandle expected) {
         for (Interaction interaction : interactions) {
            if (expected.equals(interaction.interaction)) return true;
         }
         return false;
      }

      private List<Interaction> interactions() {
         return new ArrayList<>(interactions);
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2 && arguments[0] instanceof ObjectInstanceHandle) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction(arguments));
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
                  if ("toString".equals(name)) return "Java TCK HLAservice actor-identity callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
