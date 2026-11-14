package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Verifies the Table 7 publish-object-attributes request through public unpublication. */
final class Table7PublishObjectClassAttributesTck {
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassAttributes";

   private Table7PublishObjectClassAttributesTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty()
            && objectClassName != null && !objectClassName.isEmpty()
            && attributeName != null && !attributeName.isEmpty(),
         "Table 7 Publish Object Class Attributes testing requires adapter FOM, MIM, time, object-class, and attribute configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-publish-object-class-attributes-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean targetMayBePublished = false;
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
         subject.joinFederationExecution("java-tck-table7-publish-object-class-attributes-" + suffix,
            federation);
         joined = true;

         objectClass = subject.getObjectClassHandle(objectClassName);
         attribute = subject.getAttributeHandle(objectClass, attributeName);
         requestClass = subject.getInteractionClassHandle(REQUEST);
         JavaTckSupport.check(objectClass != null && attribute != null && requestClass != null,
            "Publish Object Class Attributes probe could not resolve adapter-FOM and MIM handles");
         ParameterHandle objectClassParameter =
            subject.getParameterHandle(requestClass, "HLAobjectClass");
         ParameterHandle attributeListParameter =
            subject.getParameterHandle(requestClass, "HLAattributeList");
         JavaTckSupport.check(objectClassParameter != null && attributeListParameter != null,
            "standard MIM omitted HLAobjectClass or HLAattributeList from HLApublishObjectClassAttributes");

         attributes = JavaTckSupport.attributeSet(subject, attribute);
         subject.publishInteractionClass(requestClass);
         requestPublished = true;
         byte[] encodedObjectClass = new byte[objectClass.encodedLength()];
         objectClass.encode(encodedObjectClass, 0);
         byte[] encodedAttribute = new byte[attribute.encodedLength()];
         attribute.encode(encodedAttribute, 0);
         byte[] encodedAttributeList = new byte[4 + encodedAttribute.length];
         encodedAttributeList[0] = 0;
         encodedAttributeList[1] = 0;
         encodedAttributeList[2] = 0;
         encodedAttributeList[3] = 1;
         System.arraycopy(encodedAttribute, 0, encodedAttributeList, 4, encodedAttribute.length);
         ParameterHandleValueMap parameters =
            subject.getParameterHandleValueMapFactory().create(2);
         parameters.put(objectClassParameter, encodedObjectClass);
         parameters.put(attributeListParameter, encodedAttributeList);
         targetMayBePublished = true;
         subject.sendInteraction(requestClass, parameters, new byte[0]);

         subject.unpublishObjectClassAttributes(objectClass, attributes);
         targetMayBePublished = false;
      } finally {
         if (targetMayBePublished && objectClass != null && attributes != null) {
            try { subject.unpublishObjectClassAttributes(objectClass, attributes); }
            catch (Exception ignored) { }
         }
         if (requestPublished && requestClass != null) {
            try { subject.unpublishInteractionClass(requestClass); }
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
