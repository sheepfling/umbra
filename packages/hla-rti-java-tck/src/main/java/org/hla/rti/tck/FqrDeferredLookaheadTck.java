package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** One standard §8.13.4 deferred-lookahead branch after an early Flush Queue grant. */
final class FqrDeferredLookaheadTck {
   private FqrDeferredLookaheadTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "FQR deferred-lookahead has native evidence only for evoked callbacks");
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder requesterEvents = new Recorder();
      Recorder peerEvents = new Recorder();
      String federation = "java-tck-fqr-deferred-lookahead-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean peerConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean peerJoined = false;
      try {
         requester.connect(requesterEvents.proxy(), model);
         requesterConnected = true;
         peer.connect(peerEvents.proxy(), model);
         peerConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         requester.joinFederationExecution("fqr-deferred-requester", federation);
         requesterJoined = true;
         peer.joinFederationExecution("fqr-deferred-peer", federation);
         peerJoined = true;

         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> peerFactory = peer.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(peerFactory.getName()),
            "FQR deferred-lookahead members selected different time factories");
         LogicalTime<?, ?> initial = requester.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = requesterFactory.makeEpsilon();
         LogicalTimeInterval<?> lookahead10 = interval(epsilon, 10);
         LogicalTimeInterval<?> lookahead2 = interval(epsilon, 2);
         LogicalTimeInterval<?> lookahead5 = interval(epsilon, 5);
         LogicalTime<?, ?> time5 = offset(initial, epsilon, 5);
         LogicalTime<?, ?> time10 = offset(initial, epsilon, 10);

         requester.enableTimeRegulation(lookahead10);
         await(requester, () -> requesterEvents.regulationEnabled == 1,
            "requester time-regulation enablement");
         peer.enableTimeRegulation(interval(peerFactory.makeEpsilon(), 5));
         await(peer, () -> peerEvents.regulationEnabled == 1,
            "peer time-regulation enablement");

         requester.modifyLookahead(lookahead2);
         JavaTckSupport.check(lookahead10.equals(requester.queryLookahead()),
            "FQR lookahead decrease became actual before a grant");
         requester.flushQueueRequest(time10);
         await(requester, () -> requesterEvents.flushGrants == 1,
            "FQR deferred-lookahead grant");
         JavaTckSupport.check(requesterEvents.flushGrants == 1
               && time5.equals(requesterEvents.actualGrant)
               && time10.equals(requesterEvents.optimisticGrant)
               && requesterEvents.timeAdvanceGrants == 0,
            "FQR returned the wrong actual/optimistic grant or an extra Time Advance Grant");
         JavaTckSupport.check(time5.equals(requester.queryLogicalTime())
               && lookahead5.equals(requester.queryLookahead()),
            "FQR grant did not apply max(requested 2, old 10 - advance 5) = 5");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         requester.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (peerJoined) {
            try { peer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (peerConnected) {
            try { peer.disconnect(); } catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, BooleanSupplier ready,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline)
         ambassador.evokeCallback(0.0);
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index)
         result = (LogicalTime) result.add(epsilon);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval interval(LogicalTimeInterval epsilon,
         int steps) throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int index = 1; index < steps; ++index)
         result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int flushGrants;
      private volatile int timeAdvanceGrants;
      private volatile LogicalTime<?, ?> actualGrant;
      private volatile LogicalTime<?, ?> optimisticGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("timeAdvanceGrant".equals(name)) ++timeAdvanceGrants;
               else if ("flushQueueGrant".equals(name) && args != null
                     && args.length == 2) {
                  actualGrant = (LogicalTime<?, ?>) args[0];
                  optimisticGrant = (LogicalTime<?, ?>) args[1];
                  ++flushGrants;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK FQR deferred-lookahead callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
