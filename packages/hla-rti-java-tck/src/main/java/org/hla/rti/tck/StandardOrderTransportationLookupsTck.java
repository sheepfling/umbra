package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.TransportationTypeHandleFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Mandatory order and transportation lookups across connection and membership states. */
final class StandardOrderTransportationLookupsTck {
   private StandardOrderTransportationLookupsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      Member unjoined = new Member(factory.getRtiAmbassador());
      Member owner = new Member(factory.getRtiAmbassador());
      Member peer = new Member(factory.getRtiAmbassador());
      String federation = "java-tck-standard-support-lookups-" + UUID.randomUUID();
      try {
         expectLifecycle(unjoined.rti, "NotConnected", "before connect");
         unjoined.connect(callbackModel);
         expectLifecycle(unjoined.rti, "FederateNotExecutionMember",
            "after connect before join");
         unjoined.close();

         owner.connect(callbackModel);
         peer.connect(callbackModel);
         owner.rti.createFederationExecution(federation, fom, timeImplementation);
         owner.createdFederation = federation;
         owner.rti.joinFederationExecution("java-tck-order-owner", federation);
         owner.joined = true;
         peer.rti.joinFederationExecution("java-tck-order-peer", federation);
         peer.joined = true;

         checkMandatoryNames(owner.rti, "owner");
         checkMandatoryNames(peer.rti, "peer");

         TransportationTypeHandle ownerReliable =
            owner.rti.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle ownerBestEffort =
            owner.rti.getTransportationTypeHandle("HLAbestEffort");
         TransportationTypeHandle peerReliable =
            peer.rti.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle peerBestEffort =
            peer.rti.getTransportationTypeHandle("HLAbestEffort");
         TransportationTypeHandleFactory ownerFactory =
            owner.rti.getTransportationTypeHandleFactory();
         TransportationTypeHandleFactory peerFactory =
            peer.rti.getTransportationTypeHandleFactory();
         JavaTckSupport.check(ownerFactory != null && peerFactory != null,
            "transportation handle factory was unavailable after join");
         JavaTckSupport.check(ownerReliable.equals(ownerFactory.getHLAdefaultReliable())
               && ownerBestEffort.equals(ownerFactory.getHLAdefaultBestEffort()),
            "owner transportation factory defaults differed from standard name lookups");
         JavaTckSupport.check(peerReliable.equals(peerFactory.getHLAdefaultReliable())
               && peerBestEffort.equals(peerFactory.getHLAdefaultBestEffort()),
            "peer transportation factory defaults differed from standard name lookups");
         JavaTckSupport.check(ownerReliable != null && ownerBestEffort != null
               && peerReliable != null && peerBestEffort != null,
            "mandatory transportation lookup returned null");
         JavaTckSupport.check(!ownerReliable.equals(ownerBestEffort)
               && !peerReliable.equals(peerBestEffort),
            "mandatory transportation handles were not distinct");
         JavaTckSupport.check(ownerReliable.equals(peerReliable)
               && ownerBestEffort.equals(peerBestEffort),
            "mandatory transportation handles were not stable across federates");
         checkTransportationName(owner.rti, ownerReliable, "HLAreliable", "owner reliable");
         checkTransportationName(owner.rti, ownerBestEffort, "HLAbestEffort", "owner best effort");
         checkTransportationName(peer.rti, peerReliable, "HLAreliable", "peer reliable");
         checkTransportationName(peer.rti, peerBestEffort, "HLAbestEffort", "peer best effort");

         String unknownOrderName = "MissingOrder"
            + UUID.randomUUID().toString().replace("-", "");
         String unknownTransportationName = "MissingTransportation"
            + UUID.randomUUID().toString().replace("-", "");
         JavaTckSupport.expectFailure(() -> owner.rti.getOrderType(unknownOrderName),
            "InvalidOrderName", "unknown order name");
         JavaTckSupport.expectFailure(
            () -> owner.rti.getTransportationTypeHandle(unknownTransportationName),
            "InvalidTransportationName", "unknown transportation name");
      } finally {
         peer.close();
         owner.close();
         unjoined.close();
      }
   }

   private static void expectLifecycle(RTIambassador ambassador, String exception,
         String phase) throws Exception {
      JavaTckSupport.expectFailure(() -> ambassador.getOrderType("Receive"), exception,
         "Receive order type lookup " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.getOrderName(OrderType.RECEIVE), exception,
         "Receive order name lookup " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.getOrderType("TimeStamp"), exception,
         "TimeStamp order type lookup " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.getOrderName(OrderType.TIMESTAMP), exception,
         "TimeStamp order name lookup " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.getTransportationTypeHandle("HLAreliable"), exception,
         "reliable transportation lookup " + phase);
   }

   private static void checkMandatoryNames(RTIambassador ambassador, String federate)
         throws Exception {
      JavaTckSupport.check(ambassador.getOrderType("Receive") == OrderType.RECEIVE,
         federate + " Receive lookup returned the wrong order type");
      JavaTckSupport.check(ambassador.getOrderType("TimeStamp") == OrderType.TIMESTAMP,
         federate + " TimeStamp lookup returned the wrong order type");
      JavaTckSupport.check("Receive".equals(ambassador.getOrderName(OrderType.RECEIVE)),
         federate + " RECEIVE order name did not round-trip");
      JavaTckSupport.check("TimeStamp".equals(ambassador.getOrderName(OrderType.TIMESTAMP)),
         federate + " TIMESTAMP order name did not round-trip");
   }

   private static void checkTransportationName(RTIambassador ambassador,
         TransportationTypeHandle handle, String expected, String description) throws Exception {
      JavaTckSupport.check(expected.equals(ambassador.getTransportationTypeName(handle)),
         description + " transportation name did not round-trip");
   }

   private static final class Member {
      private final RTIambassador rti;
      private boolean connected;
      private boolean joined;
      private String createdFederation;

      private Member(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel model) throws Exception {
         rti.connect(emptyFederateAmbassador(), model);
         connected = true;
      }

      private void close() {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
            joined = false;
         }
         if (createdFederation != null) {
            try { rti.destroyFederationExecution(createdFederation); }
            catch (Exception ignored) { }
            createdFederation = null;
         }
         if (connected) {
            try { rti.disconnect(); } catch (Exception ignored) { }
            connected = false;
         }
      }
   }

   private static FederateAmbassador emptyFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK lookup callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
