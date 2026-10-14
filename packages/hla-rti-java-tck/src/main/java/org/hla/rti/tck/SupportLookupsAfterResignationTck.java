package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks valid class/member lookup inputs are rejected after resignation. */
final class SupportLookupsAfterResignationTck {
   private SupportLookupsAfterResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-support-lookups-after-resignation-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      try {
         ambassador.connect(callbacks(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-support-lookups-resigned", federation);
         joined = true;

         ObjectClassHandle objectClass = ambassador.getObjectClassHandle(objectClassName);
         var attribute = ambassador.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interactionClass =
            ambassador.getInteractionClassHandle(interactionClassName);
         var parameter = ambassador.getParameterHandle(interactionClass, parameterName);
         JavaTckSupport.check(objectClassName.equals(
               ambassador.getObjectClassName(objectClass))
               && attributeName.equals(ambassador.getAttributeName(objectClass, attribute))
               && interactionClassName.equals(
                  ambassador.getInteractionClassName(interactionClass))
               && parameterName.equals(ambassador.getParameterName(interactionClass, parameter)),
            "adapter FOM class/member lookups did not round-trip while joined");

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         JavaTckSupport.expectFailure(() -> ambassador.getObjectClassHandle(objectClassName),
            "FederateNotExecutionMember", "looking up a valid object class after resignation");
         JavaTckSupport.expectFailure(() -> ambassador.getObjectClassName(objectClass),
            "FederateNotExecutionMember", "looking up a valid object-class name after resignation");
         JavaTckSupport.expectFailure(
            () -> ambassador.getAttributeHandle(objectClass, attributeName),
            "FederateNotExecutionMember", "looking up a valid attribute after resignation");
         JavaTckSupport.expectFailure(() -> ambassador.getAttributeName(objectClass, attribute),
            "FederateNotExecutionMember", "looking up a valid attribute name after resignation");
         JavaTckSupport.expectFailure(
            () -> ambassador.getInteractionClassHandle(interactionClassName),
            "FederateNotExecutionMember",
            "looking up a valid interaction class after resignation");
         JavaTckSupport.expectFailure(
            () -> ambassador.getInteractionClassName(interactionClass),
            "FederateNotExecutionMember",
            "looking up a valid interaction-class name after resignation");
         JavaTckSupport.expectFailure(
            () -> ambassador.getParameterHandle(interactionClass, parameterName),
            "FederateNotExecutionMember", "looking up a valid parameter after resignation");
         JavaTckSupport.expectFailure(
            () -> ambassador.getParameterName(interactionClass, parameter),
            "FederateNotExecutionMember", "looking up a valid parameter name after resignation");

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
                  return "Java TCK post-resignation lookup callback sink";
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
