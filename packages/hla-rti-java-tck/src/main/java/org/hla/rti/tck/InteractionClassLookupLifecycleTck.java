package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Standard interaction-class lookup boundaries and FOM-module composition. */
final class InteractionClassLookupLifecycleTck {
   private InteractionClassLookupLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String additionalFom,
         String timeImplementation, String baseInteractionName,
         String extensionInteractionName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(baseInteractionName != null && !baseInteractionName.trim().isEmpty()
            && extensionInteractionName != null && !extensionInteractionName.trim().isEmpty(),
         "adapter must provide the base and additional-module interaction-class names");
      JavaTckSupport.check(!baseInteractionName.equals(extensionInteractionName),
         "additional-module interaction class must differ from the base interaction class");
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      String federation = "java-tck-interaction-lookup-" + UUID.randomUUID();
      boolean observerConnected = false;
      boolean ownerConnected = false;
      boolean extensionConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean extensionJoined = false;
      try {
         JavaTckSupport.expectFailure(
            () -> observer.getInteractionClassHandle(baseInteractionName), "NotConnected",
            "looking up an interaction class before connecting");
         JavaTckSupport.expectFailure(
            () -> observer.getInteractionClassName(null), "NotConnected",
            "looking up an interaction-class name before connecting");

         observer.connect(callbacks(), callbackModel);
         observerConnected = true;
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         extension.connect(callbacks(), callbackModel);
         extensionConnected = true;

         JavaTckSupport.expectFailure(
            () -> observer.getInteractionClassHandle(baseInteractionName),
            "FederateNotExecutionMember",
            "looking up an interaction class after connecting but before joining");
         JavaTckSupport.expectFailure(
            () -> observer.getInteractionClassName(null), "FederateNotExecutionMember",
            "looking up an interaction-class name after connecting but before joining");

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-interaction-lookup-owner", federation);
         ownerJoined = true;

         InteractionClassHandle base = owner.getInteractionClassHandle(baseInteractionName);
         JavaTckSupport.check(base != null
               && base.equals(owner.getInteractionClassHandle(baseInteractionName))
               && baseInteractionName.equals(owner.getInteractionClassName(base)),
            "ordinary interaction-class lookup did not round-trip a stable base handle");
         String unknownInteractionClassName = "HLAinteractionRoot.TckMissingInteraction"
            + UUID.randomUUID().toString().replace("-", "");
         JavaTckSupport.expectFailure(
            () -> owner.getInteractionClassHandle(unknownInteractionClassName), "NameNotFound",
            "looking up an unknown interaction class");
         // Java exposes no public invalid-handle constructor; null is its portable invalid input.
         JavaTckSupport.expectFailure(
            () -> owner.getInteractionClassName(null), "InvalidInteractionClassHandle",
            "looking up an interaction-class name from an invalid handle");
         JavaTckSupport.expectFailure(
            () -> owner.getInteractionClassHandle(extensionInteractionName), "NameNotFound",
            "looking up the extension interaction before an additional-module join");

         extension.joinFederationExecution("java-tck-interaction-lookup-extension",
            "java-tck-interaction-lookup-extension", federation,
            new String[] {additionalFom});
         extensionJoined = true;
         InteractionClassHandle ownerExtension =
            owner.getInteractionClassHandle(extensionInteractionName);
         InteractionClassHandle memberExtension =
            extension.getInteractionClassHandle(extensionInteractionName);
         JavaTckSupport.check(ownerExtension != null && ownerExtension.equals(memberExtension)
               && extensionInteractionName.equals(owner.getInteractionClassName(ownerExtension))
               && base.equals(owner.getInteractionClassHandle(baseInteractionName)),
            "additional-module join did not compose a shared extension interaction or preserve the base handle");

         extension.resignFederationExecution(ResignAction.NO_ACTION);
         extensionJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (extensionJoined) {
            try { extension.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (extensionConnected) { try { extension.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
         if (observerConnected) { try { observer.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK interaction-class lookup callback sink";
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
