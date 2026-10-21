package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Public-API parity for a timestamped response to an object-class value request. */
final class ObjectClassTimestampedProviderResponseTck {
   private ObjectClassTimestampedProviderResponseTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel model) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder(owner);
      Recorder requesterCallbacks = new Recorder(requester);
      String federation = "java-tck-class-timestamped-response-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false;
      boolean created = false, ownerJoined = false, requesterJoined = false;
      try {
         owner.connect(ownerCallbacks.proxy(), model);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), model);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "class-response-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("class-response-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         check(owner.getAutoProvideSwitch(), "adapter FOM did not enable Auto Provide");
         AttributeHandleSet published = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet subscribed = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, published);
         owner.changeDefaultAttributeOrderType(ownerClass, published, OrderType.TIMESTAMP);
         requester.subscribeObjectClassAttributes(requesterClass, subscribed);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(model, owner, requester,
            () -> object.equals(requesterCallbacks.discoveredObject)
               && ownerCallbacks.providedCount > 0,
            "class-request discovery and Auto Provide setup");
         check(requesterClass.equals(requester.getKnownObjectClassHandle(object)),
            "class-request discovery had the wrong known class");
         ownerCallbacks.providedCount = 0;

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         check(ownerFactory.getName().equals(requesterFactory.getName()),
            "class-request members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTime timestamp = addSteps(owner.queryLogicalTime(), epsilon, 2);
         LogicalTime requesterTarget = addSteps(requester.queryLogicalTime(),
            (LogicalTimeInterval) requesterFactory.makeEpsilon(), 2);
         byte[] requestTag = new byte[] {0x43, 0x4c, 0x53};
         byte[] responseTag = new byte[] {0x43, 0x4c, 0x53, 0x2d, 0x52};
         byte[] responseValue = new byte[] {0x6a};
         ownerCallbacks.responseTime = timestamp;
         ownerCallbacks.responseTag = responseTag;
         ownerCallbacks.responseValue = responseValue;

         requester.enableTimeConstrained();
         await(model, owner, requester, () -> requesterCallbacks.constrainedEnabled,
            "requester time-constrained callback");
         owner.enableTimeRegulation(epsilon);
         await(model, owner, requester, () -> ownerCallbacks.regulationEnabled,
            "owner time-regulation callback");
         requesterCallbacks.callbackOrder.clear();
         requester.requestAttributeValueUpdate(requesterClass, subscribed, requestTag);
         if (model == CallbackModel.HLA_EVOKED) {
            check(ownerCallbacks.providedCount == 0,
               "evoked provider callback ran synchronously");
         }
         await(model, owner, requester,
            () -> ownerCallbacks.providedCount == 1 && ownerCallbacks.response != null,
            "timestamped provider response");
         check(object.equals(ownerCallbacks.providedObject)
               && ownerCallbacks.providedAttributes.size() == 1
               && ownerCallbacks.providedAttributes.contains(ownerAttribute)
               && Arrays.equals(requestTag, ownerCallbacks.providedTag),
            "class request returned wrong provider metadata");
         MessageRetractionReturn response = ownerCallbacks.response;
         check(response.retractionHandleIsValid && response.handle != null,
            "timestamped provider response returned no retraction handle");
         check(requesterCallbacks.reflectionCount == 0,
            "constrained requester reflected before time advance");

         requester.timeAdvanceRequest(requesterTarget);
         owner.timeAdvanceRequest(timestamp);
         await(model, owner, requester,
            () -> requesterCallbacks.reflectionCount == 1
               && requesterCallbacks.grant != null,
            "timestamped reflection and grant");
         check(object.equals(requesterCallbacks.reflectedObject)
               && requesterCallbacks.reflectedValues.size() == 1
               && Arrays.equals(responseValue,
                  requesterCallbacks.reflectedValues.get(requesterAttribute))
               && Arrays.equals(responseTag, requesterCallbacks.reflectedTag)
               && producer.equals(requesterCallbacks.reflectedProducer)
               && requesterTarget.equals(requesterCallbacks.reflectedTime)
               && requesterTarget.equals(requesterCallbacks.grant)
               && requesterCallbacks.sentOrder == OrderType.TIMESTAMP
               && requesterCallbacks.receivedOrder == OrderType.TIMESTAMP
               && response.handle.equals(requesterCallbacks.reflectedRetraction)
               && requesterCallbacks.regions != null
               && requesterCallbacks.regions.isEmpty()
               && requesterCallbacks.transportation.equals(
                  owner.getTransportationTypeHandle("HLAreliable")),
            "timestamped class-request reflection lost standard callback metadata");
         check(List.of("reflectAttributeValues", "timeAdvanceGrant")
               .equals(requesterCallbacks.callbackOrder),
            "timestamped reflection did not precede its grant");
         JavaTckSupport.expectFailure(() -> owner.retract(response.handle),
            "MessageCanNoLongerBeRetracted", "delivered class-request response");

         requester.unsubscribeObjectClassAttributes(requesterClass, subscribed);
         owner.unpublishObjectClassAttributes(ownerClass, published);
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) try {
            requester.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static LogicalTime addSteps(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(CallbackModel model, RTIambassador owner,
         RTIambassador requester, BooleanSupplier condition, String label) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(owner);
            JavaTckSupport.drain(requester);
         } else Thread.yield();
      }
      check(condition.getAsBoolean(), "timed out waiting for " + label);
   }

   private static void check(boolean valid, String message) {
      if (!valid) throw new AssertionError(message);
   }

   private static final class Recorder {
      private final RTIambassador ambassador;
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile int providedCount;
      private volatile ObjectInstanceHandle providedObject;
      private volatile AttributeHandleSet providedAttributes;
      private volatile byte[] providedTag;
      private volatile LogicalTime responseTime;
      private volatile byte[] responseTag;
      private volatile byte[] responseValue;
      private volatile MessageRetractionReturn response;
      private volatile boolean constrainedEnabled, regulationEnabled;
      private volatile int reflectionCount;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile AttributeHandleValueMap reflectedValues;
      private volatile byte[] reflectedTag;
      private volatile FederateHandle reflectedProducer;
      private volatile LogicalTime<?, ?> reflectedTime, grant;
      private volatile OrderType sentOrder, receivedOrder;
      private volatile MessageRetractionHandle reflectedRetraction;
      private volatile RegionHandleSet regions;
      private volatile TransportationTypeHandle transportation;
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private Recorder(RTIambassador ambassador) { this.ambassador = ambassador; }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("provideAttributeValueUpdate".equals(name)) {
                  providedObject = (ObjectInstanceHandle) arguments[0];
                  providedAttributes = (AttributeHandleSet) arguments[1];
                  providedTag = ((byte[]) arguments[2]).clone();
                  ++providedCount;
                  if (responseTime != null) {
                     AttributeHandleValueMap values = ambassador
                        .getAttributeHandleValueMapFactory().create(1);
                     for (AttributeHandle attribute : providedAttributes)
                        values.put(attribute, responseValue);
                     response = ambassador.updateAttributeValues(
                        providedObject, values, responseTag, responseTime);
                  }
               } else if ("timeConstrainedEnabled".equals(name)) {
                  constrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled = true;
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  reflectedValues = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflectedTag = ((byte[]) arguments[2]).clone();
                  transportation = (TransportationTypeHandle) arguments[3];
                  reflectedProducer = (FederateHandle) arguments[4];
                  regions = (RegionHandleSet) arguments[5];
                  reflectedTime = (LogicalTime<?, ?>) arguments[6];
                  sentOrder = (OrderType) arguments[7];
                  receivedOrder = (OrderType) arguments[8];
                  reflectedRetraction = (MessageRetractionHandle) arguments[9];
                  ++reflectionCount;
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)) {
                  grant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK class-response callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
