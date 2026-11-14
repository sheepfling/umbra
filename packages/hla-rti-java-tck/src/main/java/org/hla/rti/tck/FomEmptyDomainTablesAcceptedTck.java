package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Accepts an IEEE 1516.2 FOM whose domain-specific OMT tables are empty. */
final class FomEmptyDomainTablesAcceptedTck {
   private FomEmptyDomainTablesAcceptedTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-fom-empty-domain-tables-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean creatorJoined = false;
      boolean memberJoined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;

         creator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle creatorHandle = creator.joinFederationExecution(
            "java-tck-empty-domain-creator", federation);
         creatorJoined = true;
         FederateHandle memberHandle = member.joinFederationExecution(
            "java-tck-empty-domain-member", federation);
         memberJoined = true;

         ObjectClassHandle creatorObjectRoot = creator.getObjectClassHandle("HLAobjectRoot");
         ObjectClassHandle creatorFederationMom = creator.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         InteractionClassHandle creatorInteractionRoot = creator.getInteractionClassHandle(
            "HLAinteractionRoot");
         ObjectClassHandle memberObjectRoot = member.getObjectClassHandle("HLAobjectRoot");
         ObjectClassHandle memberFederationMom = member.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         InteractionClassHandle memberInteractionRoot = member.getInteractionClassHandle(
            "HLAinteractionRoot");
         JavaTckSupport.check(creatorHandle != null && memberHandle != null
               && creatorObjectRoot != null && creatorFederationMom != null
               && creatorInteractionRoot != null && memberObjectRoot != null
               && memberFederationMom != null && memberInteractionRoot != null
               && creatorObjectRoot.equals(memberObjectRoot)
               && creatorFederationMom.equals(memberFederationMom)
               && creatorInteractionRoot.equals(memberInteractionRoot),
            "root-only FOM creation did not provide shared roots and the standard federation MOM");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         creatorJoined = false;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) resign(member);
         if (creatorJoined) resign(creator);
         if (created) {
            try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (creatorConnected) disconnect(creator);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK empty-domain callbacks";
            if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
            if ("equals".equals(method.getName())) {
               return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }
}
