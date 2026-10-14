package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Ensures an empty FOM module list neither creates nor reserves an execution. */
final class FomEmptyModuleValidationTck {
   private FomEmptyModuleValidationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-fom-empty-modules-" + UUID.randomUUID();
      String creatorName = "java-tck-empty-modules-owner";
      String memberName = "java-tck-empty-modules-member";
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

         JavaTckSupport.expectFailure(() -> creator.createFederationExecution(
               federation, new String[0], timeImplementation), "InvalidFOM",
            "creating a federation with an empty FOM module list");

         creator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle creatorHandle =
            creator.joinFederationExecution(creatorName, federation);
         creatorJoined = true;
         FederateHandle memberHandle = member.joinFederationExecution(memberName, federation);
         memberJoined = true;
         JavaTckSupport.check(creatorHandle != null && memberHandle != null,
            "valid FOM creation after empty-module rejection did not establish membership");
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
            if ("toString".equals(method.getName())) return "Java TCK empty-FOM callbacks";
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
