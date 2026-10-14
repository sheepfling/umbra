package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks mandatory order and transportation lookup membership boundaries. */
final class StandardOrderTransportLookupsAfterResignationTck {
   private StandardOrderTransportLookupsAfterResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-order-transport-resignation-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      try {
         ambassador.connect(callbacks(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-order-transport-resigned", federation);
         joined = true;

         OrderType receive = ambassador.getOrderType("Receive");
         OrderType timestamp = ambassador.getOrderType("TimeStamp");
         TransportationTypeHandle reliable =
            ambassador.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort =
            ambassador.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(OrderType.RECEIVE.equals(receive)
               && OrderType.TIMESTAMP.equals(timestamp)
               && reliable != null && bestEffort != null && !reliable.equals(bestEffort),
            "mandatory order or transportation lookups returned unexpected values");
         JavaTckSupport.check("Receive".equals(ambassador.getOrderName(receive))
               && "TimeStamp".equals(ambassador.getOrderName(timestamp))
               && "HLAreliable".equals(ambassador.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(ambassador.getTransportationTypeName(bestEffort)),
            "mandatory order or transportation names did not round-trip while joined");

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         JavaTckSupport.expectFailure(() -> ambassador.getOrderType("Receive"),
            "FederateNotExecutionMember", "looking up Receive order after resignation");
         JavaTckSupport.expectFailure(() -> ambassador.getOrderName(receive),
            "FederateNotExecutionMember", "looking up Receive order name after resignation");
         JavaTckSupport.expectFailure(() -> ambassador.getOrderType("TimeStamp"),
            "FederateNotExecutionMember", "looking up TimeStamp order after resignation");
         JavaTckSupport.expectFailure(() -> ambassador.getOrderName(timestamp),
            "FederateNotExecutionMember", "looking up TimeStamp order name after resignation");
         JavaTckSupport.expectFailure(
            () -> ambassador.getTransportationTypeHandle("HLAreliable"),
            "FederateNotExecutionMember", "looking up reliable transportation after resignation");
         JavaTckSupport.expectFailure(() -> ambassador.getTransportationTypeName(reliable),
            "FederateNotExecutionMember", "looking up reliable transportation name after resignation");
         JavaTckSupport.expectFailure(
            () -> ambassador.getTransportationTypeHandle("HLAbestEffort"),
            "FederateNotExecutionMember", "looking up best-effort transportation after resignation");
         JavaTckSupport.expectFailure(() -> ambassador.getTransportationTypeName(bestEffort),
            "FederateNotExecutionMember", "looking up best-effort transportation name after resignation");

         ambassador.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) { try { ambassador.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK standard lookup callback sink";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
