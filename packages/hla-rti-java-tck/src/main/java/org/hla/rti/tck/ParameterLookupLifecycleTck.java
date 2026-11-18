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
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-parameter-lookup-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         InteractionClassHandle invalidInteractionClass =
            JavaTckSupport.invalidHandle(InteractionClassHandle.class);
         ParameterHandle invalidParameter = JavaTckSupport.invalidHandle(ParameterHandle.class);
         JavaTckSupport.expectFailure(
            () -> member.getParameterHandle(invalidInteractionClass, inheritedParameterName),
            "NotConnected", "looking up a parameter before connecting");
         JavaTckSupport.expectFailure(
            () -> member.getParameterName(invalidInteractionClass, invalidParameter),
            "NotConnected", "looking up a parameter name before connecting");

         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         JavaTckSupport.expectFailure(
            () -> member.getParameterHandle(invalidInteractionClass, inheritedParameterName),
            "FederateNotExecutionMember",
            "looking up a parameter after connecting but before joining");
         JavaTckSupport.expectFailure(
            () -> member.getParameterName(invalidInteractionClass, invalidParameter),
            "FederateNotExecutionMember",
            "looking up a parameter name after connecting but before joining");

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
         InteractionClassHandle unissuedInteractionClass =
            unissuedInteractionClassHandle(owner, derivedInteraction);
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
         JavaTckSupport.expectFailure(
            () -> owner.getParameterName(baseInteraction, derivedParameter),
            "InteractionParameterNotDefined",
            "looking up a derived parameter handle on its base interaction class");
         JavaTckSupport.expectFailure(
            () -> owner.getParameterHandle(unissuedInteractionClass, inheritedParameterName),
            "InvalidInteractionClassHandle",
            "looking up a parameter from an invalid interaction-class handle");
         JavaTckSupport.expectFailure(
            () -> owner.getParameterName(unissuedInteractionClass, baseInherited),
            "InvalidInteractionClassHandle",
            "looking up a parameter name from an invalid interaction-class handle");
         JavaTckSupport.expectFailure(
            () -> owner.getParameterName(derivedInteraction, invalidParameter),
            "InvalidParameterHandle",
            "looking up a parameter name from an invalid parameter handle");

         member.joinFederationExecution("java-tck-parameter-lookup-member",
            "java-tck-parameter-lookup-member", federation);
         memberJoined = true;
         InteractionClassHandle memberBase =
            member.getInteractionClassHandle(baseInteractionName);
         ParameterHandle memberBaseInherited =
            member.getParameterHandle(memberBase, inheritedParameterName);
         InteractionClassHandle memberDerived =
            member.getInteractionClassHandle(derivedInteractionName);
         ParameterHandle memberInherited =
            member.getParameterHandle(memberDerived, inheritedParameterName);
         ParameterHandle memberDerivedParameter =
            member.getParameterHandle(memberDerived, derivedParameterName);
         JavaTckSupport.check(baseInteraction.equals(memberBase)
               && derivedInteraction.equals(memberDerived)
               && baseInherited.equals(memberBaseInherited)
               && baseInherited.equals(memberInherited)
               && derivedParameter.equals(memberDerivedParameter)
               && inheritedParameterName.equals(
                  member.getParameterName(memberBase, memberBaseInherited))
               && inheritedParameterName.equals(
                  member.getParameterName(memberDerived, memberInherited))
               && derivedParameterName.equals(
                  member.getParameterName(memberDerived, memberDerivedParameter)),
            "inherited parameter handles or names did not retain cross-federate identity");
         JavaTckSupport.expectFailure(
            () -> member.getParameterHandle(memberBase, derivedParameterName), "NameNotFound",
            "looking up a derived parameter on its base interaction class through the member");
         JavaTckSupport.expectFailure(
            () -> member.getParameterName(memberBase, memberDerivedParameter),
            "InteractionParameterNotDefined",
            "looking up a derived parameter handle on its base interaction class through the member");

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
      }
   }

   private static InteractionClassHandle unissuedInteractionClassHandle(
         RTIambassador ambassador, InteractionClassHandle knownHandle) throws Exception {
      byte[] encoding = JavaTckSupport.encode(knownHandle.encodedLength(), knownHandle::encode);
      encoding[encoding.length - 1] ^= 0x01;
      InteractionClassHandle unissued =
         ambassador.getInteractionClassHandleFactory().decode(encoding, 0);
      JavaTckSupport.check(!knownHandle.equals(unissued),
         "derived unissued interaction-class handle equals the known handle");
      return unissued;
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
