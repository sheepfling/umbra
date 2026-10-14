package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Standard-API checks for the mutually exclusive report switch and report subscription. */
final class ServiceReportInterlockTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";

   private ServiceReportInterlockTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-interlock-" + suffix;
      String federate = "java-tck-service-report-interlock-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         rti.connect(emptyCallbacks(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federate, "java-tck", federation);
         joined = true;

         InteractionClassHandle reportClass = rti.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(rti.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "service reporting did not start disabled");

         rti.setServiceReportingSwitch(true);
         JavaTckSupport.check(rti.getServiceReportingSwitch(),
            "service-reporting switch did not enable");
         JavaTckSupport.expectFailure(
            () -> rti.subscribeInteractionClass(reportClass),
            "FederateServiceInvocationsAreBeingReportedViaMOM",
            "active HLAreportServiceInvocation subscription while reporting is enabled");
         JavaTckSupport.expectFailure(
            () -> rti.subscribeInteractionClassPassively(reportClass),
            "FederateServiceInvocationsAreBeingReportedViaMOM",
            "passive HLAreportServiceInvocation subscription while reporting is enabled");
         JavaTckSupport.check(rti.getServiceReportingSwitch(),
            "rejected report subscriptions changed the enabled reporting switch");

         rti.setServiceReportingSwitch(false);
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "service-reporting switch did not disable");
         rti.subscribeInteractionClass(reportClass);
         JavaTckSupport.expectFailure(
            () -> rti.setServiceReportingSwitch(true),
            "ReportServiceInvocationsAreSubscribed",
            "enabling reporting with an active report subscription");
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "rejected switch enable changed reporting while actively subscribed");
         rti.unsubscribeInteractionClass(reportClass);

         rti.subscribeInteractionClassPassively(reportClass);
         JavaTckSupport.expectFailure(
            () -> rti.setServiceReportingSwitch(true),
            "ReportServiceInvocationsAreSubscribed",
            "enabling reporting with a passive report subscription");
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "rejected switch enable changed reporting while passively subscribed");
         rti.unsubscribeInteractionClass(reportClass);

         rti.setServiceReportingSwitch(true);
         JavaTckSupport.check(rti.getServiceReportingSwitch(),
            "reporting could not be re-enabled after report unsubscribe");
         rti.setServiceReportingSwitch(false);
      } finally {
         if (joined) {
            try {
               rti.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) { }
         }
         if (created) {
            try {
               rti.destroyFederationExecution(federation);
            } catch (Exception ignored) { }
         }
         if (connected) {
            try {
               rti.disconnect();
            } catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK service-report interlock";
            if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
            if ("equals".equals(method.getName())) return proxy == arguments[0];
            return null;
         });
   }
}
