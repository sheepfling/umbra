package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Bounded Annex C.3 object-attribute composition through the standard Java RTI API. */
final class FomObjectAttributeMergeTck {
   private FomObjectAttributeMergeTck() { }

   static void run(RtiFactory factory, String base, String disjoint,
         String conflict, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-attribute-merge-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         creator.createFederationExecution(
            federation, new String[] {base, disjoint}, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-attribute-member", federation);
         memberJoined = true;
         ObjectClassHandle objectClass = member.getObjectClassHandle(
            "HLAobjectRoot.TckAttributeMergeObject");
         AttributeHandle first = member.getAttributeHandle(objectClass, "FirstValue");
         AttributeHandle second = member.getAttributeHandle(objectClass, "SecondValue");
         JavaTckSupport.check(objectClass != null && first != null && second != null
               && !first.equals(second),
            "merged object attributes were not separately visible");
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         creator.destroyFederationExecution(federation);
         created = false;

         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(
               federation, new String[] {base, conflict}, timeImplementation),
            "InconsistentFOM", "conflicting semantics for one named object attribute");
         creator.createFederationExecution(
            federation, new String[] {base, disjoint}, timeImplementation);
         created = true;
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
               if ("toString".equals(method.getName())) return "Java TCK attribute merge callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
