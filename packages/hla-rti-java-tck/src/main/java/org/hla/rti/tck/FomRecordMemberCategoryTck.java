package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Record member data-type category validation through the standard Java API. */
final class FomRecordMemberCategoryTck {
   private FomRecordMemberCategoryTck() { }

   static void run(RtiFactory factory, String valid, String fieldTypes,
         String omitted, String encodingMismatch, String basicField,
         String basicAlternative, String reservedHlaName, String reservedNaName,
         String reservedHlaFieldName, String reservedNaFieldName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-record-member-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {valid}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
         creator.createFederationExecution(federation,
            new String[] {fieldTypes}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
         creator.createFederationExecution(federation,
            new String[] {omitted}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;

         String[][] invalidModules = {
            {basicField, "raw basic-data fixed-record Field Type"},
            {basicAlternative, "raw basic-data variant-record Alternative Type"},
            {encodingMismatch,
               "predefined variant-record encoding assigned to a fixed record"},
            {reservedHlaName,
               "case-insensitive HLA-reserved fixed-record Record Name"},
            {reservedNaName,
               "case-insensitive NA-marker fixed-record Record Name"},
            {reservedHlaFieldName,
               "case-insensitive HLA-reserved fixed-record Field Name"},
            {reservedNaFieldName,
               "case-insensitive NA-marker fixed-record Field Name"}
         };
         for (String[] invalid : invalidModules) {
            // If Create unexpectedly succeeds, retain cleanup ownership before the assertion fails.
            created = true;
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {invalid[0]}, timeImplementation),
               "InconsistentFOM", invalid[1]);
            created = false;
         }
         creator.createFederationExecution(federation,
            new String[] {valid}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK record callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
