package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks HLAprivilegeToDeleteObject lookup across the P0 TCK object hierarchy. */
final class PrivilegeAttributeAvailabilityTck {
   private static final String PRIVILEGE_ATTRIBUTE = "HLAprivilegeToDeleteObject";
   private static final String[] P0_OBJECT_CLASSES = {
      "HLAobjectRoot",
      "HLAobjectRoot.TckObject",
      "HLAobjectRoot.TckObject.TckSecondaryObject"
   };

   private PrivilegeAttributeAvailabilityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      String federation = "java-tck-privilege-attribute-availability-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      try {
         owner.connect(new NullFederateAmbassador(), callbackModel);
         ownerConnected = true;
         observer.connect(new NullFederateAmbassador(), callbackModel);
         observerConnected = true;
         owner.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-privilege-attribute-owner-" + UUID.randomUUID(),
            "java-tck", federation);
         ownerJoined = true;
         observer.joinFederationExecution(
            "java-tck-privilege-attribute-observer-" + UUID.randomUUID(),
            "java-tck", federation);
         observerJoined = true;

         for (String className : P0_OBJECT_CLASSES) {
            ObjectClassHandle objectClass = owner.getObjectClassHandle(className);
            JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0,
               "P0 TCK object-class lookup failed for " + className);
            AttributeHandle privilege = owner.getAttributeHandle(
               objectClass, PRIVILEGE_ATTRIBUTE);
            JavaTckSupport.check(privilege != null && privilege.encodedLength() > 0,
               "HLAprivilegeToDeleteObject was unavailable on " + className);
            JavaTckSupport.check(PRIVILEGE_ATTRIBUTE.equals(
                  owner.getAttributeName(objectClass, privilege)),
               "HLAprivilegeToDeleteObject did not round-trip on " + className);
         }
      } finally {
         if (observerJoined) try {
            observer.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (created) try {
            owner.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }
}
