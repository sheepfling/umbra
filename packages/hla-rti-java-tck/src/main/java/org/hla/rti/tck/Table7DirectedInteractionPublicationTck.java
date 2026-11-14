package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.UUID;

/** Verifies the Table 7 MOM request publishes one object-directed interaction pair. */
final class Table7DirectedInteractionPublicationTck {
   private static final String FOM_PROPERTY = "hla.rti.tck.momSLeafDirectedPublicationFom";
   private static final String DEFAULT_FOM = "packages/hla-rti-cpp-tck/fom/p0-tck.xml";
   private static final String OBJECT_CLASS = "HLAobjectRoot.TckObject";
   private static final String INTERACTION_CLASS = "HLAinteractionRoot.TckInteraction";
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice."
         + "HLApublishObjectClassDirectedInteractions";

   private Table7DirectedInteractionPublicationTck() {
   }

   static void run(RtiFactory factory, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      String fom = System.getProperty(FOM_PROPERTY, DEFAULT_FOM);
      JavaTckSupport.check(Files.isRegularFile(Path.of(fom)),
         "Table 7 directed-publication FOM does not exist: " + fom);

      RTIambassador member = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-sleaf-directed-publish-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean requestPublished = false;
      boolean directedPublicationPendingCleanup = false;
      ObjectClassHandle objectClass = null;
      InteractionClassHandle interactionClass = null;
      InteractionClassHandle requestClass = null;
      try {
         member.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         connected = true;
         member.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-sleaf-directed-publisher-" + suffix,
            federation);
         joined = true;

         objectClass = member.getObjectClassHandle(OBJECT_CLASS);
         interactionClass = member.getInteractionClassHandle(INTERACTION_CLASS);
         requestClass = member.getInteractionClassHandle(REQUEST_CLASS);
         JavaTckSupport.check(objectClass != null && interactionClass != null
               && requestClass != null,
            "P0 FOM or standard MIM omitted a directed-publication handle");
         ParameterHandle requestObjectClass =
            member.getParameterHandle(requestClass, "HLAobjectClass");
         ParameterHandle requestInteractionClasses =
            member.getParameterHandle(requestClass, "HLAinteractionClassList");
         JavaTckSupport.check(requestObjectClass != null && requestInteractionClasses != null,
            "standard MIM omitted directed-publication request parameters");

         member.publishInteractionClass(requestClass);
         requestPublished = true;

         byte[] encodedObjectClass = new byte[objectClass.encodedLength()];
         objectClass.encode(encodedObjectClass, 0);
         byte[] encodedInteractionClassList =
            new byte[Integer.BYTES + interactionClass.encodedLength()];
         encodedInteractionClassList[3] = 1;
         interactionClass.encode(encodedInteractionClassList, Integer.BYTES);
         ParameterHandleValueMap parameters =
            member.getParameterHandleValueMapFactory().create(2);
         parameters.put(requestObjectClass, encodedObjectClass);
         parameters.put(requestInteractionClasses, encodedInteractionClassList);
         member.sendInteraction(requestClass, parameters, new byte[0]);
         directedPublicationPendingCleanup = true;

         member.unpublishObjectClassDirectedInteractions(objectClass,
            JavaTckSupport.interactionSet(member, interactionClass));
         directedPublicationPendingCleanup = false;
      } finally {
         if (directedPublicationPendingCleanup && joined && objectClass != null
               && interactionClass != null) {
            try {
               member.unpublishObjectClassDirectedInteractions(objectClass,
                  JavaTckSupport.interactionSet(member, interactionClass));
            } catch (Exception ignored) {
            }
         }
         if (requestPublished && joined && requestClass != null) {
            try { member.unpublishInteractionClass(requestClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try {
               member.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try { member.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }
}
