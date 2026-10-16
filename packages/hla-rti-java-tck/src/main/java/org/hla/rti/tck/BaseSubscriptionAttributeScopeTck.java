package org.hla.rti.tck;

import java.util.UUID;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;

/** Portable class-scoped attribute validation for ordinary object subscriptions. */
final class BaseSubscriptionAttributeScopeTck {
   private BaseSubscriptionAttributeScopeTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseObjectClassName, String derivedObjectClassName,
         String baseAttributeName, String derivedAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador member = factory.getRtiAmbassador();
      FederateAmbassador callbacks = new NullFederateAmbassador();
      String federation = "java-tck-base-subscription-scope-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(callbacks, callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-base-subscription-scope", federation);
         joined = true;

         ObjectClassHandle baseClass = member.getObjectClassHandle(baseObjectClassName);
         ObjectClassHandle derivedClass = member.getObjectClassHandle(derivedObjectClassName);
         AttributeHandle baseAttribute = member.getAttributeHandle(baseClass, baseAttributeName);
         AttributeHandle derivedAttribute =
            member.getAttributeHandle(derivedClass, derivedAttributeName);
         JavaTckSupport.check(baseClass != null && derivedClass != null
               && !baseClass.equals(derivedClass) && baseAttribute != null
               && derivedAttribute != null,
            "adapter FOM did not provide the configured base/derived class and attribute handles");

         AttributeHandleSet baseAttributes = JavaTckSupport.attributeSet(member, baseAttribute);
         AttributeHandleSet derivedAttributes =
            JavaTckSupport.attributeSet(member, derivedAttribute);
         member.subscribeObjectClassAttributes(baseClass, baseAttributes);
         member.unsubscribeObjectClassAttributes(baseClass, baseAttributes);
         member.subscribeObjectClassAttributes(derivedClass, derivedAttributes);
         member.unsubscribeObjectClassAttributes(derivedClass, derivedAttributes);
         JavaTckSupport.expectFailure(
            () -> member.subscribeObjectClassAttributes(
               baseClass, JavaTckSupport.attributeSet(member, derivedAttribute)),
            "AttributeNotDefined",
            "subscribing to a base class with an attribute defined only by its derived class");
      } finally {
         if (joined) {
            member.resignFederationExecution(ResignAction.NO_ACTION);
         }
         if (created) {
            try {
               member.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (connected) {
            member.disconnect();
         }
      }
   }
}
