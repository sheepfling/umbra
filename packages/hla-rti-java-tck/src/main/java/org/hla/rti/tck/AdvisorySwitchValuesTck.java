package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies the standard Boolean round-trip behavior of the four advisory switches. */
final class AdvisorySwitchValuesTck {
   private AdvisorySwitchValuesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      String federation = "java-tck-advisory-switches-" + UUID.randomUUID();
      try {
         ambassador.connect(callbacks(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-advisory-switches", federation);
         joined = true;

         requireToggle(ambassador::getObjectClassRelevanceAdvisorySwitch,
            ambassador::setObjectClassRelevanceAdvisorySwitch,
            "object-class relevance advisory switch");
         requireToggle(ambassador::getAttributeRelevanceAdvisorySwitch,
            ambassador::setAttributeRelevanceAdvisorySwitch,
            "attribute relevance advisory switch");
         requireToggle(ambassador::getAttributeScopeAdvisorySwitch,
            ambassador::setAttributeScopeAdvisorySwitch,
            "attribute scope advisory switch");
         requireToggle(ambassador::getInteractionRelevanceAdvisorySwitch,
            ambassador::setInteractionRelevanceAdvisorySwitch,
            "interaction relevance advisory switch");

         ambassador.resignFederationExecution(
            hla.rti1516_2025.ResignAction.NO_ACTION);
         joined = false;
         ambassador.destroyFederationExecution(federation);
         created = false;
         ambassador.disconnect();
         connected = false;
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(
               hla.rti1516_2025.ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void requireToggle(CheckedGetter getter, CheckedSetter setter, String name)
         throws Exception {
      boolean original = getter.get();
      boolean changed = !original;
      setter.set(changed);
      JavaTckSupport.check(getter.get() == changed, name + " did not follow the set value");
      setter.set(original);
      JavaTckSupport.check(getter.get() == original, name + " did not restore its original value");
   }

   @FunctionalInterface
   private interface CheckedGetter { boolean get() throws Exception; }

   @FunctionalInterface
   private interface CheckedSetter { void set(boolean value) throws Exception; }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK advisory-switch callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }
}
