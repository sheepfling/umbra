package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.CallNotAllowedFromWithinCallback;
import java.lang.reflect.Proxy;

/** Checks the standard callback-servicing re-entry exception boundary. */
final class CallbackReentrancyTck {
   private CallbackReentrancyTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder(ambassador);
      boolean connected = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.listFederationExecutions();
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 10.0);
         }

         JavaTckSupport.check(recorder.reportCount == 1,
            "callback re-entrancy test did not receive exactly one federation-list callback");
         JavaTckSupport.check(recorder.evokeRejected && recorder.multipleRejected,
            "callback re-entrancy test admitted callback-service re-entry");
      } finally {
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static final class Recorder {
      private final RTIambassador ambassador;
      private int reportCount;
      private boolean evokeRejected;
      private boolean multipleRejected;

      private Recorder(RTIambassador ambassador) {
         this.ambassador = ambassador;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)) {
                  ++reportCount;
                  try {
                     ambassador.evokeCallback(0.0);
                  } catch (CallNotAllowedFromWithinCallback expected) {
                     evokeRejected = true;
                  }
                  try {
                     ambassador.evokeMultipleCallbacks(0.0, 0.0);
                  } catch (CallNotAllowedFromWithinCallback expected) {
                     multipleRejected = true;
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK callback-reentrancy recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
