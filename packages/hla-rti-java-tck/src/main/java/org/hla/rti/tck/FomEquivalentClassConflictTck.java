package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Equivalent duplicate and conflicting class definitions through the standard API. */
final class FomEquivalentClassConflictTck {
   private FomEquivalentClassConflictTck() { }

   static void run(RtiFactory factory, String base, String conflicting,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-equivalent-class-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {base, base}, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-equivalent-class-member", federation);
         joined = true;
         String className = "HLAobjectRoot.TckEquivalentClass";
         ObjectClassHandle objectClass = creator.getObjectClassHandle(className);
         JavaTckSupport.check(className.equals(creator.getObjectClassName(objectClass)),
            "equivalent duplicate modules did not preserve the class identity");
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         creator.destroyFederationExecution(federation);
         created = false;

         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(federation,
               new String[] {base, conflicting}, timeImplementation),
            "InconsistentFOM", "same-identity object class with conflicting sharing");
         creator.createFederationExecution(federation,
            new String[] {base, base}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) try { creator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK class composition callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
