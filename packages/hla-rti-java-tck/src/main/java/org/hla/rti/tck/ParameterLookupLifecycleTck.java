package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Standard parameter lookup boundaries, inheritance, and cross-member identity. */
final class ParameterLookupLifecycleTck {
   private ParameterLookupLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseInteractionName, String derivedInteractionName,
         String inheritedParameterName, String derivedParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-parameter-lookup-" + UUID.randomUUID();
      boolean observerConnected = false;
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         JavaTckSupport.expectFailure(
            () -> observer.getParameterHandle(null, inheritedParameterName), "NotConnected",
            "looking up a parameter before connecting");
         JavaTckSupport.expectFailure(
            () -> observer.getParameterName(null, null), "NotConnected",
            "looking up a parameter name before connecting");

         observer.connect(callbacks(), callbackModel);
         observerConnected = true;
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         JavaTckSupport.expectFailure(
            () -> observer.getParameterHandle(null, inheritedParameterName),
            "FederateNotExecutionMember", "looking up a parameter before joining");
         JavaTckSupport.expectFailure(
            () -> observer.getParameterName(null, null), "FederateNotExecutionMember",
            "looking up a parameter name before joining");

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-parameter-lookup-owner", federation);
         ownerJoined = true;

         InteractionClassHandle baseInteraction =
            owner.getInteractionClassHandle(baseInteractionName);
         InteractionClassHandle derivedInteraction =
            owner.getInteractionClassHandle(derivedInteractionName);
         ParameterHandle baseInherited =
            owner.getParameterHandle(baseInteraction, inheritedParameterName);
         ParameterHandle derivedInherited =
            owner.getParameterHandle(derivedInteraction, inheritedParameterName);
         ParameterHandle derivedParameter =
            owner.getParameterHandle(derivedInteraction, derivedParameterName);
         JavaTckSupport.check(baseInteraction != null && derivedInteraction != null
               && baseInherited != null && derivedInherited != null && derivedParameter != null,
            "inherited parameter lookup returned a null class or parameter handle");
         JavaTckSupport.check(baseInherited.equals(derivedInherited)
               && baseInherited.equals(owner.getParameterHandle(
                  derivedInteraction, inheritedParameterName))
               && inheritedParameterName.equals(
                  owner.getParameterName(baseInteraction, baseInherited))
               && inheritedParameterName.equals(
                  owner.getParameterName(derivedInteraction, derivedInherited))
               && derivedParameterName.equals(
                  owner.getParameterName(derivedInteraction, derivedParameter)),
            "inherited parameter lookup did not preserve stable handles and names");

         JavaTckSupport.expectFailure(
            () -> owner.getParameterHandle(baseInteraction, derivedParameterName),
            "NameNotFound", "looking up a derived parameter on its base interaction class");
         String unknownParameterName = "TckMissingParameter"
            + UUID.randomUUID().toString().replace("-", "");
         JavaTckSupport.expectFailure(
            () -> owner.getParameterHandle(derivedInteraction, unknownParameterName), "NameNotFound",
            "looking up an unknown parameter name");
         // Java exposes no public invalid-handle constructors; null supplies typed invalid inputs.
         JavaTckSupport.expectFailure(
            () -> owner.getParameterHandle(null, inheritedParameterName),
            "InvalidInteractionClassHandle",
            "looking up a parameter from an invalid interaction-class handle");
         JavaTckSupport.expectFailure(
            () -> owner.getParameterName(null, baseInherited), "InvalidInteractionClassHandle",
            "looking up a parameter name from an invalid interaction-class handle");
         JavaTckSupport.expectFailure(
            () -> owner.getParameterName(derivedInteraction, null), "InvalidParameterHandle",
            "looking up a parameter name from an invalid parameter handle");
         JavaTckSupport.expectFailure(
            () -> owner.getParameterName(baseInteraction, derivedParameter),
            "InteractionParameterNotDefined",
            "looking up a derived parameter handle on its base interaction class");

         member.joinFederationExecution("java-tck-parameter-lookup-member",
            "java-tck-parameter-lookup-member", federation);
         memberJoined = true;
         InteractionClassHandle memberDerived =
            member.getInteractionClassHandle(derivedInteractionName);
         ParameterHandle memberInherited =
            member.getParameterHandle(memberDerived, inheritedParameterName);
         ParameterHandle memberDerivedParameter =
            member.getParameterHandle(memberDerived, derivedParameterName);
         JavaTckSupport.check(derivedInteraction.equals(memberDerived)
               && baseInherited.equals(memberInherited)
               && derivedParameter.equals(memberDerivedParameter),
            "inherited parameter handles did not retain cross-federate identity");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (memberConnected) { try { member.disconnect(); } catch (Exception ignored) { } }
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
                  return "Java TCK parameter lookup callback sink";
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
