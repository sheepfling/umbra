package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** One named object-class semantics conflict through the standard Java RTI API. */
final class FomObjectClassSemanticsConflictTck {
   private FomObjectClassSemanticsConflictTck() { }

   static void run(RtiFactory factory, String base, String validExtension,
         String conflict, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-class-semantics-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(
               federation, new String[] {base, conflict}, timeImplementation),
            "InconsistentFOM", "conflicting semantics for one named object class");
         creator.createFederationExecution(
            federation, new String[] {base, validExtension}, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-class-semantics-member", federation);
         memberJoined = true;
         ObjectClassHandle objectClass = member.getObjectClassHandle(
            "HLAobjectRoot.TckAttributeMergeObject");
         JavaTckSupport.check(objectClass != null,
            "valid retry did not expose the named object class");
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
               if ("toString".equals(method.getName())) return "Java TCK class semantics callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
