package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Verifies the Table 7 unconditional-divestiture request through public ownership state. */
final class Table7UnconditionalAttributeOwnershipDivestitureTck {
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunconditionalAttributeOwnershipDivestiture";

   private Table7UnconditionalAttributeOwnershipDivestitureTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty()
            && objectClassName != null && !objectClassName.isEmpty()
            && attributeName != null && !attributeName.isEmpty(),
         "Table 7 unconditional-divestiture testing requires adapter FOM, MIM, time, object-class, and attribute configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-unconditional-divestiture-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean objectAttributesPublished = false;
      boolean requestPublished = false;
      ObjectClassHandle objectClass = null;
      AttributeHandle attribute = null;
      AttributeHandleSet attributes = null;
      InteractionClassHandle requestClass = null;
      try {
         subject.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         connected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution("java-tck-table7-unconditional-divestiture-" + suffix,
            federation);
         joined = true;

         objectClass = subject.getObjectClassHandle(objectClassName);
         attribute = subject.getAttributeHandle(objectClass, attributeName);
         requestClass = subject.getInteractionClassHandle(REQUEST);
         JavaTckSupport.check(objectClass != null && attribute != null && requestClass != null,
            "unconditional-divestiture probe could not resolve adapter FOM and MIM handles");
         ParameterHandle objectParameter =
            subject.getParameterHandle(requestClass, "HLAobjectInstance");
         ParameterHandle attributeListParameter =
            subject.getParameterHandle(requestClass, "HLAattributeList");
         JavaTckSupport.check(objectParameter != null && attributeListParameter != null,
            "standard MIM omitted HLAobjectInstance or HLAattributeList from unconditional divestiture");

         attributes = JavaTckSupport.attributeSet(subject, attribute);
         subject.publishObjectClassAttributes(objectClass, attributes);
         objectAttributesPublished = true;
         ObjectInstanceHandle object = subject.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "unconditional-divestiture probe could not register its test object");
         JavaTckSupport.check(subject.isAttributeOwnedByFederate(object, attribute),
            "subject did not own the newly registered object's attribute");

         subject.publishInteractionClass(requestClass);
         requestPublished = true;
         byte[] encodedObject = new byte[object.encodedLength()];
         object.encode(encodedObject, 0);
         byte[] encodedAttribute = new byte[attribute.encodedLength()];
         attribute.encode(encodedAttribute, 0);
         byte[] encodedAttributeList = new byte[4 + encodedAttribute.length];
         encodedAttributeList[0] = 0;
         encodedAttributeList[1] = 0;
         encodedAttributeList[2] = 0;
         encodedAttributeList[3] = 1;
         System.arraycopy(encodedAttribute, 0, encodedAttributeList, 4,
            encodedAttribute.length);
         ParameterHandleValueMap parameters =
            subject.getParameterHandleValueMapFactory().create(2);
         parameters.put(objectParameter, encodedObject);
         parameters.put(attributeListParameter, encodedAttributeList);
         subject.sendInteraction(requestClass, parameters, new byte[0]);

         JavaTckSupport.check(!subject.isAttributeOwnedByFederate(object, attribute),
            "HLAunconditionalAttributeOwnershipDivestiture did not divest the requested attribute");
      } finally {
         if (requestPublished && requestClass != null) {
            try { subject.unpublishInteractionClass(requestClass); }
            catch (Exception ignored) { }
         }
         if (objectAttributesPublished && objectClass != null && attributes != null) {
            try { subject.unpublishObjectClassAttributes(objectClass, attributes); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { subject.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }
}
