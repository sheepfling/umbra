package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Later-module transportation-name resolution through the standard API. */
final class FomTransportationReferenceResolutionTck {
   private FomTransportationReferenceResolutionTck() { }

   static void run(RtiFactory factory, String consumer, String provider,
         String missing, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-transportation-reference-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {consumer, provider}, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-transportation-reference-member", federation);
         joined = true;
         TransportationTypeHandle transport =
            creator.getTransportationTypeHandle("TckDeferredTransportation");
         ObjectClassHandle objectClass = creator.getObjectClassHandle(
            "HLAobjectRoot.TckTransportationReferenceObject");
         AttributeHandle attribute = creator.getAttributeHandle(objectClass, "Value");
         InteractionClassHandle interaction = creator.getInteractionClassHandle(
            "HLAinteractionRoot.TckTransportationReferenceInteraction");
         JavaTckSupport.check(transport != null && objectClass != null &&
               attribute != null && interaction != null &&
               "TckDeferredTransportation".equals(
                  creator.getTransportationTypeName(transport)) &&
               "Value".equals(creator.getAttributeName(objectClass, attribute)),
            "later FOM module did not resolve the transportation-backed classes and attribute");
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         creator.destroyFederationExecution(federation);
         created = false;

         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(federation,
               new String[] {missing}, timeImplementation),
            "InconsistentFOM", "unresolved transportation reference");
         creator.createFederationExecution(federation,
            new String[] {consumer, provider}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) try { creator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK transportation-reference callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
