package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies transportation handles remain stable across additional-FOM composition. */
final class FomTransportationHandleStabilityTck {
   private FomTransportationHandleStabilityTck() { }

   static void run(RtiFactory factory, String fom, String additionalFom,
         String customTransportationName, String additionalTransportationName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      String federation = "java-tck-fom-transportation-stability-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean extensionConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean extensionJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         extension.connect(callbacks(), callbackModel);
         extensionConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-transport-owner", federation);
         ownerJoined = true;

         TransportationTypeHandle reliableBefore =
            owner.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle customBefore =
            owner.getTransportationTypeHandle(customTransportationName);
         JavaTckSupport.check(!reliableBefore.equals(customBefore),
            "adapter FOM did not expose distinct reliable and custom transportation handles");
         JavaTckSupport.check("HLAreliable".equals(
               owner.getTransportationTypeName(reliableBefore))
               && customTransportationName.equals(owner.getTransportationTypeName(customBefore)),
            "base-FOM transportation handles did not round-trip to their names");
         JavaTckSupport.expectFailure(
            () -> owner.getTransportationTypeHandle(additionalTransportationName),
            "InvalidTransportationName",
            "resolving the additional-module transportation before its module is joined");

         extension.joinFederationExecution("java-tck-fom-transport-extension",
            "java-tck-fom-transport-extension", federation, new String[] {additionalFom});
         extensionJoined = true;

         TransportationTypeHandle ownerReliableAfter =
            owner.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle ownerCustomAfter =
            owner.getTransportationTypeHandle(customTransportationName);
         TransportationTypeHandle ownerAdditional =
            owner.getTransportationTypeHandle(additionalTransportationName);
         TransportationTypeHandle extensionReliable =
            extension.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle extensionCustom =
            extension.getTransportationTypeHandle(customTransportationName);
         TransportationTypeHandle extensionAdditional =
            extension.getTransportationTypeHandle(additionalTransportationName);

         JavaTckSupport.check(reliableBefore.equals(ownerReliableAfter)
               && customBefore.equals(ownerCustomAfter)
               && reliableBefore.equals(extensionReliable)
               && customBefore.equals(extensionCustom),
            "additional-module join changed or disagreed on pre-existing transportation handles");
         JavaTckSupport.check(ownerAdditional.equals(extensionAdditional)
               && !ownerAdditional.equals(reliableBefore)
               && !ownerAdditional.equals(customBefore),
            "additional transportation did not receive one shared, distinct handle");
         JavaTckSupport.check(additionalTransportationName.equals(
               owner.getTransportationTypeName(ownerAdditional))
               && additionalTransportationName.equals(
                  extension.getTransportationTypeName(extensionAdditional)),
            "additional transportation handle did not round-trip to its name");
      } finally {
         if (extensionJoined) resign(extension);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (extensionConnected) disconnect(extension);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) {
               return "Java TCK FOM transportation callbacks";
            }
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
