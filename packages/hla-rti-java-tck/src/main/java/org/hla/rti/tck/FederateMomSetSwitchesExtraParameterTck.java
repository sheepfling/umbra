package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.exceptions.InteractionParameterNotDefined;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies standard HLAsetSwitches behavior when the FOM adds a parameter. */
final class FederateMomSetSwitchesExtraParameterTck {
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";
   private static final String SERVICE_REPORTING_PARAMETER = "HLAserviceReporting";
   private static final String EXTENSION_PARAMETER = "TckExtensionSwitchPayload";

   private FederateMomSetSwitchesExtraParameterTck() { }

   static void run(RtiFactory factory, String extensionFom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");

      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-set-switches-extension-" + suffix;
      String federateName = "java-tck-mom-set-switches-extension-federate-" + suffix;
      boolean connected = false, created = false, joined = false;
      try {
         rti.connect(callbackProxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {extensionFom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federateName, federateType, federation);
         joined = true;

         InteractionClassHandle setSwitches = rti.getInteractionClassHandle(SET_SWITCHES);
         JavaTckSupport.check(setSwitches != null,
            "the standard federate MOM HLAsetSwitches class was unavailable");
         ParameterHandle serviceReporting =
            rti.getParameterHandle(setSwitches, SERVICE_REPORTING_PARAMETER);
         ParameterHandle extensionPayload = rti.getParameterHandle(
            setSwitches, EXTENSION_PARAMETER);
         JavaTckSupport.check(serviceReporting != null && extensionPayload != null,
            "the standard HLAserviceReporting or FOM-added switch parameter was unavailable");

         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "the HLAserviceReporting switch did not start in its defined false state");
         ResignAction initialAutomaticResign = rti.getAutomaticResignDirective();

         byte[] payload = new byte[] {0x01, 0x02, 0x03};
         ParameterHandleValueMap extensionOnly =
            rti.getParameterHandleValueMapFactory().create(1);
         extensionOnly.put(extensionPayload, payload);
         try {
            rti.sendInteraction(setSwitches, extensionOnly, new byte[0]);
         } catch (InteractionParameterNotDefined ignored) {
            // The extension-only map may project to an empty standard view.
         }
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "an extension-only HLAsetSwitches parameter changed Service Reporting");
         JavaTckSupport.check(initialAutomaticResign.equals(rti.getAutomaticResignDirective()),
            "an extension-only HLAsetSwitches parameter changed Automatic Resign Action");

         HLAinteger32BE enabled = encoder.createHLAinteger32BE(1);
         ParameterHandleValueMap combined =
            rti.getParameterHandleValueMapFactory().create(2);
         combined.put(serviceReporting, enabled.toByteArray());
         combined.put(extensionPayload, payload);
         rti.sendInteraction(setSwitches, combined, new byte[0]);
         JavaTckSupport.check(rti.getServiceReportingSwitch(),
            "HLAsetSwitches did not process HLAserviceReporting with an extension parameter present");
         JavaTckSupport.check(initialAutomaticResign.equals(rti.getAutomaticResignDirective()),
            "the extension parameter changed Automatic Resign Action during a standard switch update");
      } finally {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK HLAsetSwitches extension callbacks";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
