package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Public object-class local-name preflight against adapter-supplied FOMs. */
final class FomObjectClassNameConventionsTck {
   private FomObjectClassNameConventionsTck() { }

   static void run(RtiFactory factory, String period, String reservedHla,
         String reservedNa, String allowed, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-object-class-name-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         for (String[] invalid : new String[][] {
               {period, "period in object-class local name"},
               {reservedHla, "unknown HLA-prefixed object-class name"},
               {reservedNa, "reserved NA marker as an object-class name"}}) {
            JavaTckSupport.expectFailure(() -> creator.createFederationExecutionWithMIM(
                  federation, new String[] {invalid[0]}, mim, timeImplementation),
               "InconsistentFOM", invalid[1]);
         }
         creator.createFederationExecutionWithMIM(federation,
            new String[] {allowed}, mim, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-object-class-name-member", federation);
         memberJoined = true;
         String name = "HLAobjectRoot.Customer-2_0";
         ObjectClassHandle objectClass = member.getObjectClassHandle(name);
         JavaTckSupport.check(objectClass != null
               && name.equals(member.getObjectClassName(objectClass)),
            "accepted object-class name did not round-trip publicly");
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); }
            catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK FOM callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
