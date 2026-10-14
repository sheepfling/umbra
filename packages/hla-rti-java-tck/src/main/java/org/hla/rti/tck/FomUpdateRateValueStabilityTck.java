package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks existing and newly composed FOM update-rate values across Join. */
final class FomUpdateRateValueStabilityTck {
   private FomUpdateRateValueStabilityTck() { }

   static void run(RtiFactory factory, String rateFom, String additionalFom,
         String baseRateName, String additionalRateName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      String federation = "java-tck-fom-rate-stability-" + UUID.randomUUID();
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
         owner.createFederationExecution(federation, rateFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-rate-owner", federation);
         ownerJoined = true;

         double baseRate = owner.getUpdateRateValue(baseRateName);
         JavaTckSupport.check(baseRate > 0.0,
            "adapter base FOM did not expose a positive update rate");
         JavaTckSupport.expectFailure(() -> owner.getUpdateRateValue(additionalRateName),
            "InvalidUpdateRateDesignator",
            "resolving the additional-module update rate before its module is joined");

         extension.joinFederationExecution("java-tck-fom-rate-extension",
            "java-tck-fom-rate-extension", federation, new String[] {additionalFom});
         extensionJoined = true;
         double ownerBaseAfter = owner.getUpdateRateValue(baseRateName);
         double extensionBase = extension.getUpdateRateValue(baseRateName);
         double ownerAdditional = owner.getUpdateRateValue(additionalRateName);
         double extensionAdditional = extension.getUpdateRateValue(additionalRateName);
         JavaTckSupport.check(ownerBaseAfter == baseRate && extensionBase == baseRate,
            "additional-module join changed the existing update-rate value");
         JavaTckSupport.check(ownerAdditional > 0.0
               && extensionAdditional == ownerAdditional,
            "additional-module update-rate value was not composed consistently across members");
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
            if ("toString".equals(method.getName())) return "Java TCK FOM update-rate callbacks";
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
