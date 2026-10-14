package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks ordinary interaction declaration membership boundaries after resignation. */
final class InteractionDeclarationsAfterResignationTck {
   private InteractionDeclarationsAfterResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      String federation = "java-tck-interaction-declarations-resign-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         rti.connect(callbacks(), callbackModel);
         connected = true;
         rti.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         rti.joinFederationExecution("java-tck-interaction-declarations", federation);
         joined = true;

         InteractionClassHandle interactionClass =
            rti.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interactionClassName.equals(
               rti.getInteractionClassName(interactionClass)),
            "adapter FOM did not resolve the ordinary interaction-class handle");

         rti.publishInteractionClass(interactionClass);
         rti.unpublishInteractionClass(interactionClass);
         rti.publishInteractionClass(interactionClass);
         rti.unpublishInteractionClass(interactionClass);
         rti.subscribeInteractionClass(interactionClass);
         rti.unsubscribeInteractionClass(interactionClass);
         rti.subscribeInteractionClass(interactionClass);
         rti.unsubscribeInteractionClass(interactionClass);

         rti.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         JavaTckSupport.expectFailure(
            () -> rti.publishInteractionClass(interactionClass),
            "FederateNotExecutionMember",
            "publishing a valid interaction class after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unpublishInteractionClass(interactionClass),
            "FederateNotExecutionMember",
            "unpublishing a valid interaction class after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.subscribeInteractionClass(interactionClass),
            "FederateNotExecutionMember",
            "subscribing to a valid interaction class after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unsubscribeInteractionClass(interactionClass),
            "FederateNotExecutionMember",
            "unsubscribing from a valid interaction class after resignation");

         rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK interaction-declaration callback sink";
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
