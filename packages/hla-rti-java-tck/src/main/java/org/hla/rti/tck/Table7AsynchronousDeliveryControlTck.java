package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Verifies the Table 7 MOM requests change the addressed federate's async-delivery state. */
final class Table7AsynchronousDeliveryControlTck {
   private static final String FOM_PROPERTY = "hla.rti.tck.momSLeafDirectedPublicationFom";
   private static final String DEFAULT_FOM = "packages/hla-rti-cpp-tck/fom/p0-tck.xml";
   private static final String DISABLE_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice."
         + "HLAdisableAsynchronousDelivery";
   private static final String ENABLE_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice."
         + "HLAenableAsynchronousDelivery";

   private Table7AsynchronousDeliveryControlTck() {
   }

   static void run(RtiFactory factory, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      String fom = System.getProperty(FOM_PROPERTY, DEFAULT_FOM);
      JavaTckSupport.check(Files.isRegularFile(Path.of(fom)),
         "Table 7 asynchronous-delivery FOM does not exist: " + fom);
      JavaTckSupport.check(mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 asynchronous-delivery testing requires MIM and logical-time configuration");

      RTIambassador member = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-async-delivery-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      List<InteractionClassHandle> published = new ArrayList<>();
      try {
         member.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         connected = true;
         member.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle memberHandle = member.joinFederationExecution(
            "java-tck-table7-async-delivery-" + suffix, federation);
         joined = true;

         InteractionClassHandle disableClass = member.getInteractionClassHandle(DISABLE_REQUEST);
         InteractionClassHandle enableClass = member.getInteractionClassHandle(ENABLE_REQUEST);
         ParameterHandle disableFederate =
            member.getParameterHandle(disableClass, "HLAfederate");
         ParameterHandle enableFederate =
            member.getParameterHandle(enableClass, "HLAfederate");
         JavaTckSupport.check(disableClass != null && enableClass != null
               && disableFederate != null && enableFederate != null,
            "standard MIM omitted asynchronous-delivery requests or HLAfederate");
         member.publishInteractionClass(disableClass);
         published.add(disableClass);
         member.publishInteractionClass(enableClass);
         published.add(enableClass);

         JavaTckSupport.check(memberHandle != null,
            "asynchronous-delivery effect probe could not resolve its federate handle");
         byte[] encodedFederate = new byte[memberHandle.encodedLength()];
         memberHandle.encode(encodedFederate, 0);
         member.enableAsynchronousDelivery();
         sendRequest(member, disableClass, disableFederate, encodedFederate, true);
         sendRequest(member, enableClass, enableFederate, encodedFederate, false);
      } finally {
         if (joined) {
            try { member.disableAsynchronousDelivery(); }
            catch (Exception ignored) { }
         }
         for (InteractionClassHandle request : published) {
            try { member.unpublishInteractionClass(request); }
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

   private static void sendRequest(RTIambassador member, InteractionClassHandle request,
         ParameterHandle federateParameter, byte[] encodedFederate, boolean verifyDisable)
         throws Exception {
      ParameterHandleValueMap parameters =
         member.getParameterHandleValueMapFactory().create(1);
      parameters.put(federateParameter, encodedFederate);
      member.sendInteraction(request, parameters, new byte[0]);
      if (verifyDisable) {
         // Success proves the MOM request disabled this federate before the call.
         member.enableAsynchronousDelivery();
         member.disableAsynchronousDelivery();
      } else {
         // Success proves the MOM request enabled asynchronous delivery.
         member.disableAsynchronousDelivery();
      }
   }
}
