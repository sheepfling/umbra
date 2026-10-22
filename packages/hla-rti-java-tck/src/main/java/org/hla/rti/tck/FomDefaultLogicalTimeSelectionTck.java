package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Observes the default Create logical-time selection through standard services. */
final class FomDefaultLogicalTimeSelectionTck {
   private FomDefaultLogicalTimeSelectionTck() { }

   static void run(RtiFactory factory, String unconstrainedFom,
         String integerTimeFom, CallbackModel callbackModel) throws Exception {
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-fom-default-logical-time-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(callbacks(), callbackModel);
         connected = true;
         JavaTckSupport.expectFailure(
            () -> member.createFederationExecution(federation, integerTimeFom),
            "InconsistentFOM", "default time against an integer64 FDD");
         member.createFederationExecution(federation, unconstrainedFom);
         created = true;
         FederateHandle handle = member.joinFederationExecution(
            "java-tck-default-time-member", federation);
         joined = true;
         JavaTckSupport.check(handle != null,
            "default-time federation Join returned no handle");
         JavaTckSupport.check(member.getTimeFactory() != null
               && "HLAfloat64Time".equals(member.getTimeFactory().getName()),
            "Create without a time name did not select HLAfloat64Time");
      } finally {
         if (joined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { member.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { member.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK default time callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
