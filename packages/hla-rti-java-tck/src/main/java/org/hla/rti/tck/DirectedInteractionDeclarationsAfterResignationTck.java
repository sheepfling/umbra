package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks all ordinary directed declaration overloads at the membership boundary. */
final class DirectedInteractionDeclarationsAfterResignationTck {
   private DirectedInteractionDeclarationsAfterResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      String federation = "java-tck-directed-declarations-resign-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         rti.connect(callbacks(), callbackModel);
         connected = true;
         rti.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         rti.joinFederationExecution("java-tck-directed-declarations", federation);
         joined = true;

         ObjectClassHandle objectClass = rti.getObjectClassHandle(objectClassName);
         InteractionClassHandle interactionClass =
            rti.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(objectClassName.equals(rti.getObjectClassName(objectClass))
               && interactionClassName.equals(
                  rti.getInteractionClassName(interactionClass)),
            "adapter FOM did not resolve the directed-interaction association handles");
         InteractionClassHandleSet interactions =
            JavaTckSupport.interactionSet(rti, interactionClass);

         rti.publishObjectClassDirectedInteractions(objectClass, interactions);
         rti.unpublishObjectClassDirectedInteractions(objectClass, interactions);
         rti.publishObjectClassDirectedInteractions(objectClass, interactions);
         rti.unpublishObjectClassDirectedInteractions(objectClass);
         rti.subscribeObjectClassDirectedInteractions(objectClass, interactions);
         rti.unsubscribeObjectClassDirectedInteractions(objectClass, interactions);
         rti.subscribeObjectClassDirectedInteractions(objectClass, interactions);
         rti.unsubscribeObjectClassDirectedInteractions(objectClass);

         rti.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;

         JavaTckSupport.expectFailure(
            () -> rti.publishObjectClassDirectedInteractions(objectClass, interactions),
            "FederateNotExecutionMember",
            "publishing valid class-scoped directed interactions after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unpublishObjectClassDirectedInteractions(objectClass, interactions),
            "FederateNotExecutionMember",
            "unpublishing a valid directed-interaction set after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unpublishObjectClassDirectedInteractions(objectClass),
            "FederateNotExecutionMember",
            "unpublishing a valid object class after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.subscribeObjectClassDirectedInteractions(objectClass, interactions),
            "FederateNotExecutionMember",
            "subscribing to valid class-scoped directed interactions after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unsubscribeObjectClassDirectedInteractions(objectClass, interactions),
            "FederateNotExecutionMember",
            "unsubscribing a valid directed-interaction set after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unsubscribeObjectClassDirectedInteractions(objectClass),
            "FederateNotExecutionMember",
            "unsubscribing a valid object class after resignation");

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
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK directed-declaration callback sink";
               }
               if ("hashCode".equals(method.getName())) {
                  return System.identityHashCode(proxy);
               }
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
