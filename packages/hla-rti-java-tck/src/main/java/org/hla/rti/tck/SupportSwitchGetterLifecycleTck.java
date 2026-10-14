package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks every standard support-switch getter while joined, resigned, and disconnected. */
final class SupportSwitchGetterLifecycleTck {
   private SupportSwitchGetterLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-support-switch-getters-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      try {
         ambassador.connect(callbacks(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-support-switch-getters", federation);
         joined = true;

         Getter[] getters = getters(ambassador);
         for (Getter getter : getters) getter.operation.run();

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         for (Getter getter : getters) {
            JavaTckSupport.expectFailure(getter.operation, "FederateNotExecutionMember",
               "reading " + getter.name + " after resignation");
         }
         ambassador.destroyFederationExecution(federation);
         created = false;
         ambassador.disconnect();
         connected = false;
         for (Getter getter : getters) {
            JavaTckSupport.expectFailure(getter.operation, "NotConnected",
               "reading " + getter.name + " after disconnect");
         }
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) { try { ambassador.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static Getter[] getters(RTIambassador ambassador) {
      return new Getter[] {
         new Getter("getObjectClassRelevanceAdvisorySwitch",
            ambassador::getObjectClassRelevanceAdvisorySwitch),
         new Getter("getAttributeRelevanceAdvisorySwitch",
            ambassador::getAttributeRelevanceAdvisorySwitch),
         new Getter("getAttributeScopeAdvisorySwitch", ambassador::getAttributeScopeAdvisorySwitch),
         new Getter("getInteractionRelevanceAdvisorySwitch",
            ambassador::getInteractionRelevanceAdvisorySwitch),
         new Getter("getConveyRegionDesignatorSetsSwitch",
            ambassador::getConveyRegionDesignatorSetsSwitch),
         new Getter("getServiceReportingSwitch", ambassador::getServiceReportingSwitch),
         new Getter("getExceptionReportingSwitch", ambassador::getExceptionReportingSwitch),
         new Getter("getSendServiceReportsToFileSwitch",
            ambassador::getSendServiceReportsToFileSwitch),
         new Getter("getAutoProvideSwitch", ambassador::getAutoProvideSwitch),
         new Getter("getDelaySubscriptionEvaluationSwitch",
            ambassador::getDelaySubscriptionEvaluationSwitch),
         new Getter("getAdvisoriesUseKnownClassSwitch",
            ambassador::getAdvisoriesUseKnownClassSwitch),
         new Getter("getAllowRelaxedDDMSwitch", ambassador::getAllowRelaxedDDMSwitch),
         new Getter("getNonRegulatedGrantSwitch", ambassador::getNonRegulatedGrantSwitch)
      };
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK support switch sink";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Getter {
      private final String name;
      private final JavaTckSupport.CheckedOperation operation;

      private Getter(String name, JavaTckSupport.CheckedOperation operation) {
         this.name = name;
         this.operation = operation;
      }
   }
}
