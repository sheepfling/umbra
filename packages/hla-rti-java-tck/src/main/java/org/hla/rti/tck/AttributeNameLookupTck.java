package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks Get Attribute Name for one valid class and class-scoped attribute handle pair. */
final class AttributeNameLookupTck {
   private AttributeNameLookupTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-attribute-name-lookup-" + UUID.randomUUID();
      String ownerName = "java-tck-attribute-name-owner-" + UUID.randomUUID();
      String memberName = "java-tck-attribute-name-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(new NullFederateAmbassador(), callbackModel);
         ownerConnected = true;
         member.connect(new NullFederateAmbassador(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, "java-tck", federation);
         ownerJoined = true;
         member.joinFederationExecution(memberName, "java-tck", federation);
         memberJoined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0,
            "Object class handle lookup returned an invalid handle");
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(attribute != null && attribute.encodedLength() > 0,
            "Attribute handle lookup returned an invalid handle");
         JavaTckSupport.check(attributeName.equals(owner.getAttributeName(objectClass, attribute)),
            "Get Attribute Name did not return the name associated with the supplied "
               + "object-class and attribute handles");
      } finally {
         if (memberJoined) try {
            member.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try {
            owner.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }
}
