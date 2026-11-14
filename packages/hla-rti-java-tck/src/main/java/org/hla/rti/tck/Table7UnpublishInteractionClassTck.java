package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Verifies the Table 7 Unpublish Interaction Class request through send rejection. */
final class Table7UnpublishInteractionClassTck {
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishInteractionClass";

   private Table7UnpublishInteractionClassTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty()
            && interactionClassName != null && !interactionClassName.isEmpty(),
         "Table 7 Unpublish Interaction Class testing requires adapter FOM, MIM, time, and interaction-class configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-unpublish-interaction-class-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean requestPublished = false;
      boolean targetMayBePublished = false;
      InteractionClassHandle requestClass = null;
      InteractionClassHandle interactionClass = null;
      try {
         subject.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         connected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution("java-tck-table7-unpublish-interaction-class-" + suffix,
            federation);
         joined = true;

         requestClass = subject.getInteractionClassHandle(REQUEST);
         interactionClass = subject.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(requestClass != null && interactionClass != null,
            "Unpublish Interaction Class probe could not resolve its request or adapter-FOM target");
         final InteractionClassHandle targetInteractionClass = interactionClass;
         ParameterHandle interactionClassParameter =
            subject.getParameterHandle(requestClass, "HLAinteractionClass");
         JavaTckSupport.check(interactionClassParameter != null,
            "standard MIM omitted HLAinteractionClass from HLAunpublishInteractionClass");

         subject.publishInteractionClass(interactionClass);
         targetMayBePublished = true;
         subject.publishInteractionClass(requestClass);
         requestPublished = true;
         byte[] encodedInteractionClass = new byte[interactionClass.encodedLength()];
         interactionClass.encode(encodedInteractionClass, 0);
         ParameterHandleValueMap parameters =
            subject.getParameterHandleValueMapFactory().create(1);
         parameters.put(interactionClassParameter, encodedInteractionClass);
         subject.sendInteraction(requestClass, parameters, new byte[0]);

         ParameterHandleValueMap emptyParameters =
            subject.getParameterHandleValueMapFactory().create(0);
         JavaTckSupport.expectFailure(
            () -> subject.sendInteraction(targetInteractionClass, emptyParameters, new byte[0]),
            "InteractionClassNotPublished",
            "sending the configured interaction after HLAunpublishInteractionClass");
         targetMayBePublished = false;
      } finally {
         if (targetMayBePublished && interactionClass != null) {
            try { subject.unpublishInteractionClass(interactionClass); }
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
