package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks writable standard support-switch setters across membership lifecycles. */
final class SupportSwitchSetterLifecycleTck {
   private SupportSwitchSetterLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-support-switch-setters-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      try {
         ambassador.connect(callbacks(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-support-switch-setters", federation);
         joined = true;

         SwitchSetter[] setters = setters(ambassador);
         for (SwitchSetter setter : setters) {
            boolean original = setter.getter.get();
            setter.setter.set(original);
            JavaTckSupport.check(setter.getter.get() == original,
               setter.name + " did not preserve its observed value");
         }

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         for (SwitchSetter setter : setters) {
            JavaTckSupport.expectFailure(() -> setter.setter.set(false),
               "FederateNotExecutionMember", "setting " + setter.name + " after resignation");
         }
         ambassador.destroyFederationExecution(federation);
         created = false;
         ambassador.disconnect();
         connected = false;
         for (SwitchSetter setter : setters) {
            JavaTckSupport.expectFailure(() -> setter.setter.set(false), "NotConnected",
               "setting " + setter.name + " after disconnect");
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

   private static SwitchSetter[] setters(RTIambassador ambassador) {
      return new SwitchSetter[] {
         new SwitchSetter("setObjectClassRelevanceAdvisorySwitch",
            ambassador::getObjectClassRelevanceAdvisorySwitch,
            ambassador::setObjectClassRelevanceAdvisorySwitch),
         new SwitchSetter("setAttributeRelevanceAdvisorySwitch",
            ambassador::getAttributeRelevanceAdvisorySwitch,
            ambassador::setAttributeRelevanceAdvisorySwitch),
         new SwitchSetter("setAttributeScopeAdvisorySwitch",
            ambassador::getAttributeScopeAdvisorySwitch,
            ambassador::setAttributeScopeAdvisorySwitch),
         new SwitchSetter("setInteractionRelevanceAdvisorySwitch",
            ambassador::getInteractionRelevanceAdvisorySwitch,
            ambassador::setInteractionRelevanceAdvisorySwitch),
         new SwitchSetter("setConveyRegionDesignatorSetsSwitch",
            ambassador::getConveyRegionDesignatorSetsSwitch,
            ambassador::setConveyRegionDesignatorSetsSwitch),
         new SwitchSetter("setServiceReportingSwitch", ambassador::getServiceReportingSwitch,
            ambassador::setServiceReportingSwitch),
         new SwitchSetter("setExceptionReportingSwitch", ambassador::getExceptionReportingSwitch,
            ambassador::setExceptionReportingSwitch),
         new SwitchSetter("setSendServiceReportsToFileSwitch",
            ambassador::getSendServiceReportsToFileSwitch,
            ambassador::setSendServiceReportsToFileSwitch)
      };
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK support-switch setter sink";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private interface SwitchGetter { boolean get() throws Exception; }
   private interface SwitchSetterOperation { void set(boolean value) throws Exception; }

   private static final class SwitchSetter {
      private final String name;
      private final SwitchGetter getter;
      private final SwitchSetterOperation setter;

      private SwitchSetter(String name, SwitchGetter getter, SwitchSetterOperation setter) {
         this.name = name;
         this.getter = getter;
         this.setter = setter;
      }
   }
}
