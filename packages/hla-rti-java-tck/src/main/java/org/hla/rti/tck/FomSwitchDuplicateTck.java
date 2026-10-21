package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Public switch-value witnesses for IEEE 1516.2-2025 Annex C.8. */
final class FomSwitchDuplicateTck {
   private FomSwitchDuplicateTck() { }

   static void run(RtiFactory factory, String disabled, String second,
         String timeImplementation, CallbackModel callbackModel,
         boolean orderConflict, boolean omitted) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-switch-duplicate-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         for (String single : new String[] {disabled, second}) {
            creator.createFederationExecution(federation,
               new String[] {single}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         for (int order = 0; order < (orderConflict || omitted ? 2 : 1); order++) {
            String first = order == 0 ? disabled : second;
            String last = order == 0 ? second : disabled;
            creator.createFederationExecution(federation,
               new String[] {first, last}, timeImplementation);
            created = true;
            member.joinFederationExecution("java-tck-switch-duplicate-member", federation);
            memberJoined = true;
            JavaTckSupport.check(member.getNonRegulatedGrantSwitch()
                  == (orderConflict && order == 1),
               "public non-regulated-grant query did not preserve first switch setting");
            member.resignFederationExecution(ResignAction.NO_ACTION);
            memberJoined = false;
            creator.destroyFederationExecution(federation);
            created = false;
         }
      } finally {
         if (memberJoined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); }
            catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK switch callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
