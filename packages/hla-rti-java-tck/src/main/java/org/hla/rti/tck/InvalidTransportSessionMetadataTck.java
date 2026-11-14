package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.ConnectionFailed;
import java.lang.reflect.Proxy;

/** Reject malformed Table 33 session metadata during the public Connect service. */
final class InvalidTransportSessionMetadataTck {
   private InvalidTransportSessionMetadataTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel, String rtiAddress)
         throws Exception {
      JavaTckSupport.check(rtiAddress != null && rtiAddress.startsWith("tcp://"),
         "invalid session metadata requires an adapter-supplied TCP endpoint");

      expectConnectionFailure(factory, callbackModel, rtiAddress,
         "reserved New Session Status sequence number with a nonzero Session ID");
      expectConnectionFailure(factory, callbackModel, rtiAddress,
         "zero New Session Status Session ID with a valid sequence number");
   }

   private static void expectConnectionFailure(RtiFactory factory,
         CallbackModel callbackModel, String rtiAddress, String description)
         throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withRtiAddress(rtiAddress);
      boolean rejected = false;
      try {
         ambassador.connect(noOpFederateAmbassador(), callbackModel, configuration);
      } catch (ConnectionFailed expected) {
         rejected = true;
      }
      if (!rejected) {
         try { ambassador.disconnect(); }
         catch (Exception ignored) { }
      }
      JavaTckSupport.check(rejected,
         "RTIambassador.connect must report ConnectionFailed for " + description);
   }

   private static FederateAmbassador noOpFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK failed-connect callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
   }
}
