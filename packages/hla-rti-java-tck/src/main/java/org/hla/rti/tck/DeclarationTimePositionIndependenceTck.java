package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Keeps object declaration and discovery effective across distinct logical times. */
final class DeclarationTimePositionIndependenceTck {
   private DeclarationTimePositionIndependenceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-declaration-time-position-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean regulating = false;
      boolean constrained = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle subscriberClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle subscriberAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet subscriberAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "java-tck-declaration-time-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-declaration-time-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass != null && publisherClass.equals(subscriberClass)
               && publisherAttribute != null && publisherAttribute.equals(subscriberAttribute),
            "adapter FOM did not provide stable object-class and attribute handles");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> subscriberFactory = subscriber.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(subscriberFactory.getName()),
            "time-position members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> subscriberInitial = subscriber.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(subscriberInitial),
            "time-position members began at different logical times");

         subscriber.enableTimeConstrained();
         constrained = true;
         await(subscriber, callbackModel,
            () -> subscriberRecorder.timeConstrainedEnabled,
            "time-position subscriber time-constrained callback");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         await(publisher, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled,
            "time-position publisher time-regulation callback");

         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, epsilon, 5);
         publisher.timeAdvanceRequest(publisherTarget);
         await(publisher, callbackModel,
            () -> !publisherRecorder.timeAdvanceGrants.isEmpty(),
            "time-position publisher grant at its later logical time");
         JavaTckSupport.check(publisherRecorder.timeAdvanceGrants.size() == 1
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrants.get(0))
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "time-position publisher advanced to the wrong logical time");

         LogicalTime<?, ?> publisherBeforeDeclaration = publisher.queryLogicalTime();
         LogicalTime<?, ?> subscriberBeforeDeclaration = subscriber.queryLogicalTime();
         JavaTckSupport.check(publisherTarget.equals(publisherBeforeDeclaration)
               && subscriberInitial.equals(subscriberBeforeDeclaration)
               && !publisherBeforeDeclaration.equals(subscriberBeforeDeclaration),
            "time-position test did not establish distinct member logical times");

         publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         subscriberAttributes = subscriber.getAttributeHandleSetFactory().create();
         subscriberAttributes.add(subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "time-position object registration returned no handle");
         await(subscriber, callbackModel,
            () -> !subscriberRecorder.discoveries.isEmpty(),
            "subscriber discovery after cross-time publication and subscription");
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 1,
            "cross-time object registration produced duplicate discovery callbacks");
         Discovery discovery = subscriberRecorder.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && subscriberClass.equals(discovery.objectClass)
               && publisherFederate.equals(discovery.producer),
            "cross-time discovery returned the wrong object, class, or producer");

         LogicalTime<?, ?> publisherAfterDiscovery = publisher.queryLogicalTime();
         LogicalTime<?, ?> subscriberAfterDiscovery = subscriber.queryLogicalTime();
         JavaTckSupport.check(publisherTarget.equals(publisherAfterDiscovery)
               && subscriberInitial.equals(subscriberAfterDiscovery)
               && subscriberRecorder.timeAdvanceGrants.isEmpty(),
            "declaration or discovery changed a member's logical time position");

         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
      } finally {
         if (subscribed && subscriberJoined) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime<?, ?> result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime<?, ?>) ((LogicalTime) result).add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         if (condition.getAsBoolean()) return;
         Thread.yield();
      }
      throw new AssertionError(description + " did not arrive before the deadline");
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final FederateHandle producer;

      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
         producer = arguments.length >= 4 ? (FederateHandle) arguments[3] : null;
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
               if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled = true;
               if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  discoveries.add(new Discovery(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time-position discovery callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
